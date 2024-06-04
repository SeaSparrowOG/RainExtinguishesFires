#include "eventListener.h"
#include "fireManipulator.h"
#include "fireRegister.h"

namespace Events {
	namespace Hit {
		RE::BSEventNotifyControl HitEvenetManager::ProcessEvent(const RE::TESHitEvent* a_event, RE::BSTEventSource<RE::TESHitEvent>* a_eventSource) {
			return continueEvent;
		}
	}

	namespace Load {
		RE::BSEventNotifyControl LoadEventManager::ProcessEvent(const RE::TESCellAttachDetachEvent* a_event, RE::BSTEventSource<RE::TESCellAttachDetachEvent>* a_eventSource) {
			return continueEvent;
		}
	}

	namespace Cell {
		bool ActorCellManager::RegisterListener() {
			return true;
		}

		RE::BSEventNotifyControl ActorCellManager::ProcessEvent(const RE::BGSActorCellEvent* a_event, RE::BSTEventSource<RE::BGSActorCellEvent>* a_eventSource) {
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
			if (currentWeather != a_currentWeather) {
				currentWeather = a_currentWeather;
			}
			func(a_region, a_currentWeather);
		}

		void WeatherEventManager::ProcessWeatherChange(bool wasRaining, bool isRaining) {
		}

		bool WeatherEventManager::IsRaining() {
			return this->isRaining;
		}

		void WeatherEventManager::SetRainingFlag() {
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
