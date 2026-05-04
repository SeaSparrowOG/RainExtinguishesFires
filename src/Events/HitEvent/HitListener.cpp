#include "HitListener.h"

namespace Events::HitEvent
{
	bool InitializeHitEventListener() {
		logger::info("  >Initializing Hit Event Listener..."sv);
		auto* listener = HitEventListener::GetSingleton();
		if (!listener) {
			logger::error("    Failed to get the internal Hit Event Listener."sv);
			return false;
		}
		return listener->Initialize();
	}

	bool HitEventListener::Initialize() {
		auto* scriptEventSource = RE::ScriptEventSourceHolder::GetSingleton();
		if (!scriptEventSource) {
			logger::error("    Failed to get the game's Script Event Source Holder."sv);
			return false;
		}
		scriptEventSource->AddEventSink(this);
		return true;
	}

	RE::BSEventNotifyControl HitEventListener::ProcessEvent(const RE::TESHitEvent* a_event, 
		RE::BSTEventSource<RE::TESHitEvent>*)
	{
		return EventControl::kContinue;
	}
}