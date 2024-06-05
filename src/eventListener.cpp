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

			//Debug Block
			_loggerInfo("Registered hit on {}", eventTarget->GetBaseObject() ? _debugEDID(eventTarget->GetBaseObject()) : "No base form");
			if (a_event->target.get()) _loggerInfo("    Struck by: {}", a_event->cause.get()? a_event->cause.get()->GetName() : "No actor");
			if (eventWeapon) {
				_loggerInfo("    Struck with (weapon): {}", _debugEDID(eventWeapon));
				
			}
			if (eventSpell) {
				_loggerInfo("    Struck with (spell): {}", _debugEDID(eventSpell));
			}

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

			_loggerInfo("Loaded {}", _debugEDID(eventBaseObject));
			return continueEvent;
		}
	}

	namespace Cell {
		bool ActorCellManager::RegisterListener() {
			RE::PlayerCharacter::GetSingleton()->AddEventSink(this);
			return true;
		}

		void ActorCellManager::UpdateCellSetting() {
			auto* playerRef = RE::PlayerCharacter::GetSingleton()->AsReference();
			auto* playerCell = playerRef ? playerRef->GetParentCell() : nullptr;
			this->wasInInterior = playerCell ? playerCell->IsInteriorCell() : false;
			_loggerInfo("Updated player cell {} to {}", playerCell ? _debugEDID(playerCell) : "NULL", this->wasInInterior);
		}

		RE::BSEventNotifyControl ActorCellManager::ProcessEvent(const RE::BGSActorCellEvent* a_event, RE::BSTEventSource<RE::BGSActorCellEvent>* a_eventSource) {
			if (!(a_event && a_eventSource)) return continueEvent;
			auto* eventActor = a_event->actor.get().get();
			if (!eventActor->IsPlayer()) return continueEvent;

			auto* cellForm = RE::TESForm::LookupByID(a_event->cellID);
			auto* cell = cellForm ? cellForm->As<RE::TESObjectCELL>() : nullptr;
			if (!cell) return continueEvent;

			if (cell->IsInteriorCell() && !this->wasInInterior) {
				_loggerInfo("Moved from exterior to interior");
			}
			else if (!cell->IsInteriorCell() && this->wasInInterior) {
				_loggerInfo("Moved from interior to exterior");
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
			if (lastWeather && currentWeather != lastWeather) {
				bool currentlyRaining = currentWeather ? currentWeather->data.flags & RE::TESWeather::WeatherDataFlag::kRainy
					|| currentWeather->data.flags & RE::TESWeather::WeatherDataFlag::kSnow : false;

				if (WeatherEventManager::GetSingleton()->IsRaining() && !currentlyRaining 
					|| !WeatherEventManager::GetSingleton()->IsRaining() && currentlyRaining) {
					_loggerInfo("Hook triggered, is raining: {}", currentlyRaining);
				}
				WeatherEventManager::GetSingleton()->SetRainingFlag(currentlyRaining);
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
			_loggerInfo("Set weather flag for {} to rainy: {}", skyrimWeather ? _debugEDID(skyrimWeather) : "NULL", isRainy);
		}
	}

	bool RegisterForEvents() {
		bool success = true;
		if (!Weather::WeatherEventManager::GetSingleton()->InstallHook()) success = false;
		if (success && !Hit::HitEvenetManager::GetSingleton()->RegisterListener()) success = false;
		if (success && !Load::LoadEventManager::GetSingleton()->RegisterListener()) success = false;
		if (success && !Cell::ActorCellManager::GetSingleton()->RegisterListener()) success = false;

		if (!success) {
			Hit::HitEvenetManager::GetSingleton()->UnregisterListener();
			Load::LoadEventManager::GetSingleton()->UnregisterListener();
			return false;
		}

		_loggerInfo("Registered for game events and installed weather hook.");
		return true;
	}
}
