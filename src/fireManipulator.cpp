#include "fireManipulator.h"
#include "fireRegister.h"

namespace {
#define continueContainer RE::BSContainer::ForEachResult::kContinue

	/*
	* Returns a vector of nearby objects that can be extinguished.
	* @param a_center The fire from which to find the matching objects.
	* @param a_radius The radius over which to search.
	* @return Vector of references.
	*/
	std::vector<RE::TESObjectREFR*> GetNearbyMatchingObjects(RE::TESObjectREFR* a_center, float a_radius, CachedData::Fires* a_fireRegistry) {
		std::vector<RE::TESObjectREFR*> response;
		RE::TESObjectREFR* foundLight = nullptr;
		RE::TESObjectREFR* foundSmoke = nullptr;
		RE::TESObjectREFR* foundDynDOLODFire = nullptr;
		float lastLightDistance = a_radius;
		float lastDynDOLODDistance = a_radius;
		float lastSmokeDistance = a_radius;
		auto refLocation = a_center->GetPosition();

		if (const auto TES = RE::TES::GetSingleton(); TES) {
			TES->ForEachReferenceInRange(a_center, a_radius, [&](RE::TESObjectREFR* a_ref) {
				if (a_fireRegistry->IsFireFrozen(a_ref)) return continueContainer;
				if (!a_ref->Is3DLoaded()) return continueContainer;
				if (a_ref->IsDisabled()) return continueContainer;

				const auto baseBound = a_ref ? a_ref->GetBaseObject() : nullptr;
				bool isManaged = a_fireRegistry->IsFireObject(baseBound);
				if (!isManaged) return RE::BSContainer::ForEachResult::kContinue;

				float distance = a_ref->data.location.GetDistance(a_center->data.location);
				if (a_fireRegistry->IsSmokeObject(baseBound) && a_fireRegistry->GetCheckSmoke() && distance < lastSmokeDistance) {
					foundSmoke = a_ref;
				}
				else if (a_fireRegistry->IsDynDOLODFire(baseBound) && distance < lastDynDOLODDistance) {
					std::string edid = clib_util::editorID::get_editorID(baseBound);
					std::string originalEdid = clib_util::editorID::get_editorID(a_center->GetBaseObject()->As<RE::TESForm>());
					if (!originalEdid.empty() && edid.contains(originalEdid)) {
						foundDynDOLODFire = a_ref;
					}
				}
				else if (baseBound->Is(RE::FormType::Light) && a_fireRegistry->GetCheckLights() && distance < lastLightDistance) {
					foundLight = a_ref;
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
}
namespace FireManipulator {
    void ExtinguishFire(RE::TESObjectREFR* a_fire, const FireData* a_data, bool ignoreConditions) {
    }

    void RelightFire(RE::TESObjectREFR* a_fire, const FireData* a_data, bool ignoreConditions) {
    }
}