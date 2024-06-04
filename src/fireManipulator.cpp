#include "fireManipulator.h"
#include "fireRegister.h"

namespace {
#define continueContainer RE::BSContainer::ForEachResult::kContinue

	bool FreezeUnfreezeReference(RE::TESObjectREFR* a_ref, bool a_add) {
		auto* fireRegister = CachedData::Fires::GetSingleton();
		if (a_add) {
			if (fireRegister->IsFireFrozen(a_ref)) return false;
			fireRegister->FreezeFire(a_ref);
		}
		else {
			if (!fireRegister->IsFireFrozen(a_ref)) return false;
			fireRegister->UnFreezeFire(a_ref);
		}
		return true;
	}

	std::vector<RE::TESObjectREFR*> GetNearbyAdditionalExtinguishes(RE::TESObjectREFR* a_center, const FireData* a_data) {
		std::vector<RE::TESObjectREFR*> response{};
		RE::TESObjectREFR* foundLight = nullptr;
		RE::TESObjectREFR* foundSmoke = nullptr;
		RE::TESObjectREFR* foundDynDOLODFire = nullptr;
		double lastLightDistance = a_data->lightLookupRadius;
		double lastDynDOLODDistance = a_data->referenceLookupRadius;
		double lastSmokeDistance = a_data->smokeLookupRadius;
		auto refLocation = a_center->GetPosition();
		auto radius = std::max({ lastLightDistance, lastDynDOLODDistance, lastSmokeDistance });

		if (const auto TES = RE::TES::GetSingleton(); TES) {
			TES->ForEachReferenceInRange(a_center, radius, [&](RE::TESObjectREFR* a_ref) {
				
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
}
namespace FireManipulator {
    void RelightFire(RE::TESObjectREFR* a_fire, const FireData* a_data) {

    }

    void ExtinguishFire(RE::TESObjectREFR* a_fire, const FireData* a_data) {
		auto* fireRegistry = CachedData::Fires::GetSingleton();
		if (fireRegistry->IsFireFrozen(a_fire)) return;
		if (!a_fire || !a_data) return;

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
    }
}