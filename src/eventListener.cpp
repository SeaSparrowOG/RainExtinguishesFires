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
			return true;
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
		}
		return success;
	}

}
