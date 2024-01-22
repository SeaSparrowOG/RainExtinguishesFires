#include "loadEventManager.h"
#include "fireRegister.h"
#include "papyrus.h"

namespace LoadManager {
	//Public - Register functions.
	bool LoadManager::RegisterListener() {
		auto* singleton = LoadManager::GetSingleton();
		if (!singleton) return false;

		RE::ScriptEventSourceHolder* eventHolder = RE::ScriptEventSourceHolder::GetSingleton();
		if (!eventHolder) return false;

		eventHolder->AddEventSink(singleton);
		return true;
	}

	bool ActorCellManager::RegisterListener() {
		auto* singleton = ActorCellManager::GetSingleton();
		if (!singleton) return false;

		RE::PlayerCharacter::GetSingleton()->AddEventSink(singleton);
		return true;
	}

	bool LoadManager::UnRegisterListener() {
		auto* singleton = LoadManager::GetSingleton();
		if (!singleton) return false;

		RE::ScriptEventSourceHolder::GetSingleton()->RemoveEventSink(singleton);
		return true;
	}

	//Process Event - Thing loaded
	RE::BSEventNotifyControl LoadManager::ProcessEvent(const RE::TESCellAttachDetachEvent* a_event, RE::BSTEventSource<RE::TESCellAttachDetachEvent>* a_eventSource) {
		if (!(a_event && a_eventSource)) return continueEvent;
		if (!a_event->attached) return continueEvent;
		auto* playerParentCell = RE::PlayerCharacter::GetSingleton()->GetParentCell();
		if (!playerParentCell || playerParentCell->IsInteriorCell()) return continueEvent;

		auto* eventReferencePtr = &a_event->reference;
		auto* eventReference = eventReferencePtr ? eventReferencePtr->get() : nullptr;
		auto* referenceBoundObject = eventReference ? eventReference->GetBaseObject() : nullptr;
		auto* referenceBaseObject = referenceBoundObject ? referenceBoundObject->As<RE::TESForm>() : nullptr;
		if (!referenceBaseObject) return continueEvent;

		auto offVersion = CachedData::FireRegistry::GetSingleton()->GetOffForm(referenceBaseObject);
		if (offVersion.offVersion) {
			auto* papyrusSingleton = Papyrus::Papyrus::GetSingleton();
			if (papyrusSingleton->IsRaining()) {
				Papyrus::Papyrus::GetSingleton()->ExtinguishFire(eventReference, offVersion);
			}
		}
		else if (CachedData::FireRegistry::GetSingleton()->IsManagedFire(referenceBaseObject)) {
			Papyrus::Papyrus::GetSingleton()->RelightFire(eventReference);
		}
		return continueEvent;
	}

	//Process Event - Actor Cell
	RE::BSEventNotifyControl ActorCellManager::ProcessEvent(const RE::BGSActorCellEvent* a_event, RE::BSTEventSource<RE::BGSActorCellEvent>* a_eventSource) {
		if (!(a_event && a_eventSource)) return continueEvent;

		auto eventActorHandle = a_event->actor;
		auto eventActorPtr = eventActorHandle ? eventActorHandle.get() : nullptr;
		auto eventActor = eventActorPtr ? eventActorPtr.get() : nullptr;
		if (!eventActor) return continueEvent;

		if (eventActor != RE::PlayerCharacter::GetSingleton()) return continueEvent;

		auto cellID = a_event->cellID;
		auto* cellForm = cellID ? RE::TESForm::LookupByID(cellID) : nullptr;
		auto* cell = cellForm ? cellForm->As<RE::TESObjectCELL>() : nullptr;
		if (!cell) return continueEvent;

		if (cell->IsInteriorCell()) {
			//Moved from exterior to interior.
			if (!this->wasInInterior) {
				Papyrus::Papyrus::GetSingleton()->SendPlayerChangedInteriorExterior(false);
			}
			this->wasInInterior = false;
		}
		else {
			//Moved from interior to exterior.
			if (this->wasInInterior) {
				Papyrus::Papyrus::GetSingleton()->SendPlayerChangedInteriorExterior(true);
			}
			this->wasInInterior = true;
		}
		return continueEvent;
	}
}