#include "loadEventManager.h"
#include "eventDispenser.h"
#include "fireRegister.h"
#include "papyrus.h"

namespace LoadManager {
	bool ActorCellManager::RegisterListener() {
		auto* singleton = ActorCellManager::GetSingleton();
		if (!singleton) return false;
		RE::PlayerCharacter::GetSingleton()->AddEventSink(singleton);
		return true;
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
				Events::Papyrus::GetSingleton()->SendPlayerChangedInteriorExterior(false);
			}
			this->wasInInterior = true;
		}
		else {
			//Moved from interior to exterior.
			if (this->wasInInterior) {
				Events::Papyrus::GetSingleton()->SendPlayerChangedInteriorExterior(true);
			}
			this->wasInInterior = false;

			auto* papyrusSingleton = Events::Papyrus::GetSingleton();
			bool isRaining = Events::Papyrus::GetSingleton()->IsRaining();

			if (const auto TES = RE::TES::GetSingleton(); TES) {
				TES->ForEachReferenceInRange(RE::PlayerCharacter::GetSingleton()->AsReference(), 0.0, [&](RE::TESObjectREFR* a_ref) {
					if (!(a_ref && a_ref->Is3DLoaded())) return continueContainer;
					auto* referenceBoundObject = a_ref ? a_ref->GetBaseObject() : nullptr;
					auto* referenceBaseObject = referenceBoundObject ? referenceBoundObject->As<RE::TESForm>() : nullptr;
					if (!referenceBaseObject) return continueContainer;

					if (CachedData::FireRegistry::GetSingleton()->IsOnFire(referenceBaseObject)) {
						auto offVersion = CachedData::FireRegistry::GetSingleton()->GetOffForm(referenceBaseObject);
						if (!offVersion.offVersion) {
							return continueContainer;
						}
						if (!isRaining) {
							return continueContainer;
						}
						papyrusSingleton->ExtinguishFire(a_ref, offVersion);
					}
					else if (CachedData::FireRegistry::GetSingleton()->IsOffFire(referenceBaseObject)) {
						auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
						auto* handlePolicy = vm ? vm->GetObjectHandlePolicy() : nullptr;
						RE::VMHandle handle = handlePolicy ? handlePolicy->GetHandleForObject(a_ref->GetFormType(), a_ref) : RE::VMHandle();
						

						for (auto& foundScript : vm->attachedScripts.find(handle)->second) {
							if (foundScript->GetTypeInfo()->GetName() != "REF_ObjectRefOffController"sv) continue;
							auto* dayAttachedProperty = foundScript->GetProperty("DayAttached");
							auto* relevantFireProperty = foundScript->GetProperty("RelatedFlame");
							if (!(dayAttachedProperty && relevantFireProperty)) continue;

							if (!isRaining) {
								float dayAttachedValue = RE::BSScript::UnpackValue<float>(dayAttachedProperty);
								auto currentDay = RE::Calendar::GetSingleton()->GetDaysPassed();
								if (CachedData::FireRegistry::GetSingleton()->GetRequiredOffTime() < currentDay - dayAttachedValue) {
									papyrusSingleton->RelightFire(a_ref);
								}
							}
							break;
						}
					}
					return continueContainer;
					});
			}
		}
		return continueEvent;
	}
}