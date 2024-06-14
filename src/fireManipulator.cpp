#include "fireManipulator.h"
#include "fireRegister.h"
#include "raycastHelper.h"

namespace FireManipulator {
	void Manager::FreezeReference(RE::TESObjectREFR* a_ref) {
		if (this->frozenRefs.contains(a_ref)) return;
		this->frozenRefs[a_ref] = true;
	}

	void Manager::UnFreezeReference(RE::TESObjectREFR* a_ref) {
		if (!this->frozenRefs.contains(a_ref)) return;
		this->frozenRefs.erase(a_ref);
	}

	bool Manager::IsRefFrozen(RE::TESObjectREFR* a_ref) {
		return this->frozenRefs.contains(a_ref);
	}

	std::vector<RE::TESObjectREFR*> Manager::GetNearbyAssociatedReferences(RE::TESObjectREFR* a_center, const FireData* a_data) {
		std::vector<RE::TESObjectREFR*> response{};
		RE::TESObjectREFR* foundLight = nullptr;
		RE::TESObjectREFR* foundSmoke = nullptr;
		RE::TESObjectREFR* foundDynDOLODFire = nullptr;
		double lastLightDistance = a_data->lightLookupRadius;
		double lastDynDOLODDistance = a_data->referenceLookupRadius;
		double lastSmokeDistance = a_data->smokeLookupRadius;
		double radius = std::max({ lastLightDistance, lastSmokeDistance, lastDynDOLODDistance });
		auto refLocation = a_center->GetPosition();
		auto* cachedDataSingleton = CachedData::Fires::GetSingleton();
		auto* fireManipulatorSingleton = FireManipulator::Manager::GetSingleton();
		
		if (const auto TES = RE::TES::GetSingleton(); TES) {
			TES->ForEachReferenceInRange(a_center, radius, [&](RE::TESObjectREFR* a_ref) {
				if (fireManipulatorSingleton->IsRefFrozen(a_ref)) return continueContainer;
				if (!a_ref->Is3DLoaded()) return continueContainer;
				if (a_ref->IsDisabled()) return continueContainer;

				auto* baseForm = a_ref->GetBaseObject();
				if (!baseForm) return continueContainer;

				float distance = a_ref->GetPosition().GetDistance(refLocation);
				if (cachedDataSingleton->IsSmokeObject(baseForm) && a_data->disableSmoke && distance < lastSmokeDistance) {
					foundSmoke = a_ref;
					lastSmokeDistance = distance;
				}
				else if (cachedDataSingleton->IsDynDOLODFire(baseForm) && distance < lastDynDOLODDistance) {
					std::string edid = clib_util::editorID::get_editorID(baseForm);
					std::string originalEdid = clib_util::editorID::get_editorID(a_center->GetBaseObject()->As<RE::TESForm>());
					if (edid.contains(originalEdid)) {
						foundDynDOLODFire = a_ref;
						lastDynDOLODDistance = distance;
					}
				}
				else if ((baseForm->Is(RE::FormType::Light) || a_ref->Is(RE::FormType::Light))
					&& a_data->disableLight && distance < lastLightDistance) {
					foundLight = a_ref;
					lastLightDistance = distance;
				}
				return continueContainer;
				});
		}
		if (foundLight) {
			response.push_back(foundLight);
		}
		if (foundSmoke) {
			response.push_back(foundSmoke);
		}
		if (foundDynDOLODFire) {
			response.push_back(foundDynDOLODFire);
		}
		return response;
	}

	void Manager::ExtinguishFire(RE::TESObjectREFR* a_fire, const FireData* a_data, std::string_view a_mode) {
		//Step 1: Validate and gather additional references to extinguish
		if (this->frozenRefs.contains(a_fire)) return;
		auto* offForm = a_data->offVersion;
		std::vector<RE::TESObjectREFR*> tempFrozenRefs{};

		auto* referenceExtraList = &a_fire->extraList;
		bool hasEnableChildren = referenceExtraList ? referenceExtraList->HasType(RE::ExtraDataType::kEnableStateChildren) : false;
		bool hasEnableParents = referenceExtraList ? referenceExtraList->HasType(RE::ExtraDataType::kEnableStateParent) : false;
		bool dyndolodFire = a_data->dyndolodFire;

		if (hasEnableParents || (hasEnableChildren && !dyndolodFire)) {
			return;
		}
		else if (hasEnableChildren && dyndolodFire) {
			std::string editorID = clib_util::editorID::get_editorID(a_fire->GetBaseObject()->As<RE::TESForm>());
			if (!editorID.contains("DYNDOLOD")) return;
		}
		else if (hasEnableChildren) {
			return;
		}

		this->frozenRefs[a_fire] = true;
		tempFrozenRefs.push_back(a_fire);

		//Step 2: Place activator and initialize script data.
		auto offFire = a_fire->PlaceObjectAtMe(offForm, false);
		auto* offReference = offFire.get();
		this->frozenRefs[offReference] = true;
		tempFrozenRefs.push_back(offReference);

		offReference->MoveTo(a_fire);
		offReference->data.angle = a_fire->data.angle;

#ifndef SKYRIM_NG
		offReference->refScale = a_fire->refScale;
#else 
		offReference->GetReferenceRuntimeData().refScale = a_fire->GetReferenceRuntimeData().refScale;
#endif
		auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
		
		auto* handlePolicy = vm->GetObjectHandlePolicy();
		RE::VMHandle handle = handlePolicy->GetHandleForObject(offReference->GetFormType(), offReference);
		if (!handle || !vm->attachedScripts.contains(handle)) {
			for (auto* obj : tempFrozenRefs) {
				this->frozenRefs.erase(obj);
			}
			offReference->Disable();
			offReference->DeleteThis();
			return;
		}

		//Step 3: Send event and pray.
		bool success = false;
		for (auto& foundScript : vm->attachedScripts.find(handle)->second) {
			if (foundScript->GetTypeInfo()->GetName() != "REF_ObjectRefOffController"sv) continue;
			auto relatedFlame = foundScript->GetProperty("RelatedFlame");
			auto addExtProperty = foundScript->GetProperty("RelatedObjects");
			auto dayAttached = foundScript->GetProperty("DayAttached");
			if (!(relatedFlame && addExtProperty && dayAttached)) {
				continue;
			}

			RE::BSScript::PackValue(relatedFlame, a_fire);
			dayAttached->SetFloat(RE::Calendar::GetSingleton()->GetDaysPassed());
			auto callback = RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor>();
			auto args = RE::MakeFunctionArguments();
			const RE::BSFixedString functionName = a_mode;
			auto scriptObject = foundScript.get();
			auto object = RE::BSTSmartPointer<RE::BSScript::Object>(scriptObject);
			vm->DispatchMethodCall(object, functionName, args, callback);
			success = true;
			break;
		}
		
		if (!success) {
			if (a_fire->IsDisabled()) {
				a_fire->Enable(false);
			}
			offReference->Disable();
			offReference->DeleteThis();
			for (auto* obj : tempFrozenRefs) {
				this->frozenRefs.erase(obj);
			}
		}
	}

	void Manager::RelightFire(RE::TESObjectREFR* a_fire) {
		if (this->frozenRefs.contains(a_fire)) return;
		auto baseForm = a_fire->GetBaseObject();
		if (!baseForm) return;
		if (!CachedData::Fires::GetSingleton()->IsUnLitFire(baseForm)) return;
		
		this->frozenRefs[a_fire] = true;

		auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
		auto* handlePolicy = vm->GetObjectHandlePolicy();
		if (!handlePolicy) {
			this->frozenRefs.erase(a_fire);
			return;
		}

		RE::VMHandle handle = handlePolicy->GetHandleForObject(a_fire->GetFormType(), a_fire);
		if (!handle) {
			this->frozenRefs.erase(a_fire);
			return;
		}

		for (auto& foundScript : vm->attachedScripts.find(handle)->second) {
			if (!foundScript) continue;
			if (!foundScript->GetTypeInfo()) continue;
			if (foundScript->GetTypeInfo()->GetName() != "REF_ObjectRefOffController"sv) continue;

			auto callback = RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor>();
			auto args = RE::MakeFunctionArguments();
			const RE::BSFixedString functionName = "Relight"sv;
			auto scriptObject = foundScript.get();
			auto object = RE::BSTSmartPointer<RE::BSScript::Object>(scriptObject);
			vm->DispatchMethodCall(object, functionName, args, callback);
			break;
		}
	}

	void Manager::ExtinguishAllFires() {
		if (const auto TES = RE::TES::GetSingleton(); TES) {
			auto* cachedDataSingleton = CachedData::Fires::GetSingleton();
			auto* playerRef = RE::PlayerCharacter::GetSingleton()->AsReference();
			TES->ForEachReferenceInRange(playerRef, 0.0, [&](RE::TESObjectREFR* a_ref) {
				if (!a_ref->Is3DLoaded()) return continueContainer;
				if (a_ref->IsDisabled()) return continueContainer;

				auto* baseForm = a_ref ? a_ref->GetBaseObject() : nullptr;
				if (!baseForm) return continueContainer;
				if (!cachedDataSingleton->IsLitFire(baseForm)) return continueContainer;

				auto* fireData = cachedDataSingleton->GetFireData(baseForm);
				if (fireData) {
					if (fireData->checkOcclusion) {
						auto collision = Raycast::CheckClearance(a_ref);
						auto distance = a_ref->data.location.GetDistance(collision);
						if (distance < 5000.0f) return continueContainer;
					}

					ExtinguishFire(a_ref, fireData, "FireInTheRain"sv);
				}
				return continueContainer;
				});
		}
	}
}
