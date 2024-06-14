#include "eventListener.h"
#include "fireManipulator.h"
#include "fireRegister.h"
#include "raycastHelper.h"

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

			bool needsFire = CachedData::Fires::GetSingleton()->IsUnLitFire(eventBaseForm);
			bool extinguishFire = false;
			bool relightFire = false;
			RE::BSTArray<RE::Effect*> effectArray;

			if (eventWeapon) {
				auto* weaponEnchantment = eventWeapon->formEnchanting;
				if (!weaponEnchantment) return continueEvent;

				effectArray = weaponEnchantment->effects;
			}
			else {
				effectArray = eventSpell->effects;
			}

			bool hasFire = false;
			bool hasFrost = false;
			for (auto* effect : effectArray) {
				auto* baseEffect = effect->baseEffect;
				if (!baseEffect) return continueEvent;

				if (baseEffect->HasKeywordString("MagicDamageFire"sv)) {
					hasFire = true;
				}

				if (baseEffect->HasKeywordString("MagicDamageFrost"sv)) {
					hasFrost = true;
				}
			}
			if (hasFire && hasFrost) return continueEvent;

			if (hasFire && needsFire) relightFire = true;
			if (hasFrost && !needsFire) extinguishFire = true;
			if (!relightFire && !extinguishFire) return continueEvent;

			if (relightFire) {
				FireManipulator::Manager::GetSingleton()->RelightFire(eventTarget);
			}
			else {
				const auto* data = CachedData::Fires::GetSingleton()->GetFireData(eventBaseForm);
				FireManipulator::Manager::GetSingleton()->ExtinguishFire(eventTarget, data, "Extinguish"sv);
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

			auto* parentCell = eventReference->GetParentCell();
			bool isInterior = parentCell ? parentCell->IsInteriorCell() : true;
			if (isInterior) return continueEvent;

			bool isRaining = Events::Weather::WeatherEventManager::GetSingleton()->IsRaining();
			if (isRaining && CachedData::Fires::GetSingleton()->IsLitFire(eventBaseObject)) {
				auto* fireData = CachedData::Fires::GetSingleton()->GetFireData(eventBaseObject);
				if (!fireData) return continueEvent;

				if (fireData->checkOcclusion) {
					auto collision = Raycast::CheckClearance(eventReference);
					auto distance = eventReference->data.location.GetDistance(collision);
					if (distance > 5000.0f) return continueEvent;
				}
				FireManipulator::Manager::GetSingleton()->ExtinguishFire(eventReference, fireData, "FireInTheRain");
			}
			else if (!isRaining && CachedData::Fires::GetSingleton()->IsUnLitFire(eventBaseObject)) {
				FireManipulator::Manager::GetSingleton()->RelightFire(eventReference);
			}
			return continueEvent;
		}
	}

	namespace InteriorExterior {
		bool InteriorExteriorEventManager::RegisterListener() {
#ifndef SKYRIM_NG
			RE::PlayerCharacter::GetSingleton()->AddEventSink(this);
#else 
			RE::PlayerCharacter::GetSingleton()->AsBGSActorCellEventSource()->AddEventSink(this);
#endif
			return true;
		}

		void InteriorExteriorEventManager::AddInteriorExteriorListener(const RE::TESForm* a_form, bool a_listen) {
			if (a_listen) {
				this->interiorExteriorChange.Register(a_form);
			}
			else {
				this->interiorExteriorChange.Unregister(a_form);
			}
		}

		void InteriorExteriorEventManager::SendInteriorExteriorEvent(bool a_movedToInterior) {
			auto* skySingleton = RE::Sky::GetSingleton();
			auto* currentWeather = skySingleton ? skySingleton->currentWeather : nullptr;
			bool isRaining = currentWeather ? 
				currentWeather->data.flags & rainyFlag || currentWeather->data.flags & snowyFlag
				: false;

			this->interiorExteriorChange.QueueEvent(a_movedToInterior, isRaining);
		}

		RE::BSEventNotifyControl InteriorExteriorEventManager::ProcessEvent(const RE::BGSActorCellEvent* a_event, RE::BSTEventSource<RE::BGSActorCellEvent>* a_eventSource) {
			if (!(a_event && a_eventSource)) return continueEvent;
			if (a_event->flags & RE::BGSActorCellEvent::CellFlag::kLeave) return continueEvent;

			auto eventActor = a_event->actor.get().get();
			if (!eventActor) return continueEvent;
			if (eventActor != RE::PlayerCharacter::GetSingleton()) return continueEvent;

			auto* parentCell = RE::TESForm::LookupByID(a_event->cellID) ? RE::TESForm::LookupByID(a_event->cellID)->As<RE::TESObjectCELL>() : nullptr;
			if (!parentCell) return continueEvent;

			SendInteriorExteriorEvent(parentCell->IsInteriorCell());
			return continueEvent;
		}
	}

	namespace Weather {
		bool WeatherEventManager::InstallHook() {

#ifndef SKYRIM_NG
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(25684, 26231), OFFSET(0x44F, 0x46C) };
#else
			REL::Relocation<std::uintptr_t> target{ REL::VariantID(25684, 26231, 25684), REL::VariantOffset(0x44F, 0x46C, 0x44F)};
#endif
			stl::write_thunk_call<WeatherEventManager>(target.address());
			return true;
		}

		void WeatherEventManager::thunk(RE::TESRegion* a_region, RE::TESWeather* a_currentWeather) {
			func(a_region, a_currentWeather);
			if (!a_currentWeather) return;
			if (a_currentWeather == currentWeather) return;
			bool currentlyRaining = a_currentWeather ? a_currentWeather->data.flags & rainyFlag
				|| a_currentWeather->data.flags & snowyFlag : false;
			WeatherEventManager::GetSingleton()->SendWeatherChangeEvent(currentlyRaining);
			currentWeather = a_currentWeather;
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

		void WeatherEventManager::AddWeatherChangeListener(const RE::TESForm* a_form, bool a_listen) {
			if (a_listen) {
				this->weatherTransition.Register(a_form);
			}
			else {
				this->weatherTransition.Unregister(a_form);
			}
		}

		void WeatherEventManager::SendWeatherChangeEvent(bool newWeatherIsRainy) {
			this->weatherTransition.QueueEvent(newWeatherIsRainy);
		}
	}

	bool RegisterForEvents() {
		bool success = true;
		if (!Weather::WeatherEventManager::GetSingleton()->InstallHook()) success = false;
		if (success && !Hit::HitEvenetManager::GetSingleton()->RegisterListener()) success = false;
		if (success && !Load::LoadEventManager::GetSingleton()->RegisterListener()) success = false;
		if (success) {
			InteriorExterior::InteriorExteriorEventManager::GetSingleton()->RegisterListener();
		}

		if (!success) {
			Hit::HitEvenetManager::GetSingleton()->UnregisterListener();
			Load::LoadEventManager::GetSingleton()->UnregisterListener();
			return false;
		}

		_loggerInfo("Registered for game events and installed weather hook.");
		return true;
	}
}
