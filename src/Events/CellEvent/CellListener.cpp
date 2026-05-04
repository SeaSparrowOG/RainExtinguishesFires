#include "CellListener.h"

namespace Events::CellEvent
{
	bool InitializeCellEventListener() {
		logger::info("  >Initializing Actor Cell Event Listener..."sv);
		auto* actorListener = ActorCellEventListener::GetSingleton();
		if (!actorListener) {
			logger::error("    Failed to get the internal Actor Cell Event Listener."sv);
			return false;
		}
		actorListener->Register('CLEL');

		bool success = actorListener->Initialize();
		return success;
	}

	bool ActorCellEventListener::Initialize() {
		auto* actorEventSource = RE::PlayerCharacter::GetSingleton();
		if (!actorEventSource) {
			logger::error("    Failed to get the game's Script Event Source Holder."sv);
			return false;
		}
		actorEventSource->AddEventSink(this);
		return true;
	}

	void ActorCellEventListener::RegisterFormForEvents(RE::TESForm* a_form) {
		_registeredListeners.Register(a_form);
	}

	bool ActorCellEventListener::Save(SKSE::SerializationInterface* a_intfc) {
		if (!_registeredListeners.Save(a_intfc)) {
			logger::error("Failed to write registered listeners."sv);
			return false;
		}
		return true;
	}

	bool ActorCellEventListener::Load(SKSE::SerializationInterface* a_intfc) {
		if (!_registeredListeners.Load(a_intfc)) {
			logger::error("Failed to read registered listeners."sv);
			return false;
		}
		auto* player = RE::PlayerCharacter::GetSingleton();
		auto* parentCell = player ? player->GetParentCell() : nullptr;
		_wasInInterior = parentCell ? parentCell->IsInteriorCell() : true;
		return true;
	}

	RE::BSEventNotifyControl ActorCellEventListener::ProcessEvent(const RE::BGSActorCellEvent* a_event, 
		RE::BSTEventSource<RE::BGSActorCellEvent>*)
	{
		if (!a_event || !a_event->actor.get()->IsPlayerRef()) {
			return EventControl::kContinue;
		}
		
		const auto* cell = RE::TESForm::LookupByID<RE::TESObjectCELL>(a_event->cellID);
		if (!cell) {
			return EventControl::kContinue;
		}

		const bool currentlyInExterior = cell->IsExteriorCell();
		if (_wasInInterior && currentlyInExterior) {

		}
		_wasInInterior = currentlyInExterior;
		return EventControl::kContinue;
	}
}