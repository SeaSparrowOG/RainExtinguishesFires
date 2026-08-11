#include "AttachDetachListener.h"

#include "Settings/INI/INISettings.h"

namespace Events::AttachDetachEvent
{
	bool InitializeAttachDetachEventListener() {
		logger::info("  >Initializing Cell Attach/Detach Listener..."sv);
		auto* listener = AttachDetachEventListener::GetSingleton();
		if (!listener) {
			logger::error("    Failed to get the internal Actor Cell Event Listener."sv);
			return false;
		}
		return listener->Initialize();
	}

	bool AttachDetachEventListener::Initialize() {
		auto* scriptEventSource = RE::ScriptEventSourceHolder::GetSingleton();
		if (!scriptEventSource) {
			logger::error("    Failed to get the game's Script Event Source Holder."sv);
			return false;
		}
		scriptEventSource->AddEventSink<RE::TESCellAttachDetachEvent>(this);
		scriptEventSource->AddEventSink<RE::TESCellFullyLoadedEvent>(this);
		return true;
	}

	RE::BSEventNotifyControl AttachDetachEventListener::ProcessEvent(const RE::TESCellAttachDetachEvent* a_event,
		RE::BSTEventSource<RE::TESCellAttachDetachEvent>*)
	{
		if (!a_event) {
			return EventControl::kContinue;
		}
		return EventControl::kContinue;
	}

	RE::BSEventNotifyControl AttachDetachEventListener::ProcessEvent(const RE::TESCellFullyLoadedEvent* a_event, 
		RE::BSTEventSource<RE::TESCellFullyLoadedEvent>*)
	{
		const auto* cell = a_event ? a_event->cell : nullptr;
		const auto cellID = cell ? cell->GetFormID() : 0;
		return EventControl::kContinue;
	}
}