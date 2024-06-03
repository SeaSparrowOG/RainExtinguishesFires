#include "eventListener.h"

namespace Events {

	bool ActorCellManager::RegisterListener() {
		RE::PlayerCharacter::GetSingleton()->AddEventSink(this);
		return true;
	}

	bool RegisterForEvents() {
		bool success = true;
		if (!HitEvenetManager::GetSingleton()->RegisterListener()) success = false;
		if (success && !LoadEventManager::GetSingleton()->RegisterListener()) success = false;
		if (success && !ActorCellManager::GetSingleton()->RegisterListener()) success = false;

		if (!success) {
			HitEvenetManager::GetSingleton()->UnregisterListener();
			LoadEventManager::GetSingleton()->UnregisterListener();
		}
		return success;
	}

	RE::BSEventNotifyControl HitEvenetManager::ProcessEvent(const RE::TESHitEvent* a_event, RE::BSTEventSource<RE::TESHitEvent>* a_eventSource) {
		if (!a_event || !a_eventSource) return continueEvent;
		return continueEvent;
	}

	RE::BSEventNotifyControl LoadEventManager::ProcessEvent(const RE::TESCellAttachDetachEvent* a_event, RE::BSTEventSource<RE::TESCellAttachDetachEvent>* a_eventSource) {
		if (!a_event || !a_eventSource) return continueEvent;
		return continueEvent;
	}

	RE::BSEventNotifyControl ActorCellManager::ProcessEvent(const RE::BGSActorCellEvent* a_event, RE::BSTEventSource<RE::BGSActorCellEvent>* a_eventSource) {
		if (!a_event || !a_eventSource) return continueEvent;
		return continueEvent;
	}
}