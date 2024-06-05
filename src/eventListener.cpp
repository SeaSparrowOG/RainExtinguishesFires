#include "eventListener.h"
#include "fireManipulator.h"
#include "fireRegister.h"

namespace Events {
	namespace Hit {
		RE::BSEventNotifyControl HitEvenetManager::ProcessEvent(const RE::TESHitEvent* a_event, RE::BSTEventSource<RE::TESHitEvent>* a_eventSource) {
			if (!(a_event && a_eventSource)) return continueEvent;
			auto* eventTarget = a_event->target.get();
			auto* eventBaseForm = eventTarget ? eventTarget->GetBaseObject() : nullptr;
			if (!(eventBaseForm && CachedData::Fires::GetSingleton()->IsFireObject(eventBaseForm))) return continueEvent;

			auto* eventSource = RE::TESForm::LookupByID(a_event->source);
			auto* eventWeapon = eventSource ? eventSource->As<RE::TESObjectWEAP>() : nullptr;
			auto* eventSpell = eventSource ? eventSource->As<RE::SpellItem>() : nullptr;
			if (!(eventWeapon || eventSpell)) return continueEvent;

			return continueEvent;
		}
	}

	namespace Load {
		RE::BSEventNotifyControl LoadEventManager::ProcessEvent(const RE::TESCellAttachDetachEvent* a_event, RE::BSTEventSource<RE::TESCellAttachDetachEvent>* a_eventSource) {
			if (!(a_event && a_eventSource)) return continueEvent;
			if (!a_event->attached) return continueEvent;

			auto* eventReference = a_event->reference.get();
			auto* eventBaseObject = eventReference ? eventReference->GetBaseObject() : nullptr;
			bool isValidFire = eventBaseObject ? CachedData::Fires::GetSingleton()->IsFireObject(eventBaseObject) : false;
			if (!isValidFire) return continueEvent;

			auto* parentCell = eventReference->GetParentCell();
			bool isInterior = parentCell ? parentCell->IsInteriorCell() : true;
			if (isInterior) return continueEvent;

			bool isRaining = Events::Weather::WeatherEventManager::GetSingleton()->IsRaining();
			if (isRaining && CachedData::Fires::GetSingleton()->IsLitFire(eventBaseObject)) {
				auto* fireData = CachedData::Fires::GetSingleton()->GetFireData(eventBaseObject);
				if (!fireData) return continueEvent;

				FireManipulator::Manager::GetSingleton()->ExtinguishFire(eventReference, fireData);
			}
			else if (!isRaining && CachedData::Fires::GetSingleton()->IsUnLitFire(eventBaseObject)) {
				FireManipulator::Manager::GetSingleton()->RelightFire(eventReference);
			}
			return continueEvent;
		}
	}

	namespace Weather {
		bool WeatherEventManager::InstallHook() {
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(25684, 26231), OFFSET(0x44F, 0x46C) }; \
			stl::write_thunk_call<WeatherEventManager>(target.address());
			return true;
		}

		void WeatherEventManager::thunk(RE::TESRegion* a_region, RE::TESWeather* a_currentWeather) {
			func(a_region, a_currentWeather);
			auto* lastWeather = RE::Sky::GetSingleton()->lastWeather;
			if (a_currentWeather && currentWeather != a_currentWeather) {
				currentWeather = a_currentWeather;
			} 
			bool currentlyRaining = currentWeather ? currentWeather->data.flags & rainyFlag
				|| currentWeather->data.flags & snowyFlag : false;
			WeatherEventManager::GetSingleton()->SetRainingFlag(currentlyRaining);

			if (lastWeather && currentWeather != lastWeather) {
				if (WeatherEventManager::GetSingleton()->IsRaining() && !currentlyRaining
					|| !WeatherEventManager::GetSingleton()->IsRaining() && currentlyRaining) {
					FireManipulator::Manager::GetSingleton()->ExtinguishAllFires();
				}
			}
		}

		bool WeatherEventManager::IsRaining() {
			return this->isRaining;
		}

		void WeatherEventManager::SetRainingFlag(bool a_isRaining) {
			this->isRaining = a_isRaining;
		}

		void WeatherEventManager::UpdateWeatherFlag() {
			auto* skyrimWeather = RE::Sky::GetSingleton()->currentWeather;
			bool isRainy = skyrimWeather ? skyrimWeather->data.flags & RE::TESWeather::WeatherDataFlag::kRainy
				|| skyrimWeather->data.flags & RE::TESWeather::WeatherDataFlag::kSnow : false;
			this->isRaining = isRainy;
		}
	}

	bool RegisterForEvents() {
		bool success = true;
		if (!Weather::WeatherEventManager::GetSingleton()->InstallHook()) success = false;
		if (success && !Hit::HitEvenetManager::GetSingleton()->RegisterListener()) success = false;
		if (success && !Load::LoadEventManager::GetSingleton()->RegisterListener()) success = false;

		if (!success) {
			Hit::HitEvenetManager::GetSingleton()->UnregisterListener();
			Load::LoadEventManager::GetSingleton()->UnregisterListener();
			return false;
		}

		_loggerInfo("Registered for game events and installed weather hook.");
		return true;
	}
}
