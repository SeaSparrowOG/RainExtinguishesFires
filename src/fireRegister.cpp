#include "fireRegister.h"
#include "jsonParser.h"

namespace FireRegistry {
	bool FireRegistry::IsValidLocation() {
		bool isInInterior = RE::PlayerCharacter::GetSingleton()->GetParentCell()->IsInteriorCell();
		if (isInInterior) return false;
		return true;
	}

	bool FireRegistry::FireRegistry::IsValidSmoke(RE::TESForm* a_smoke) { 
		if (this->smokeRegister.find(a_smoke) != this->smokeRegister.end()) return true;
		return false; 
	}

	offFire FireRegistry::GetMatch(RE::TESForm* a_litFire) {
		if (this->fireRegister.find(a_litFire) != this->fireRegister.end()) {
			return this->fireRegister.at(a_litFire);
		}
		return offFire();
	}

	bool FireRegistry::FireRegistry::GetCheckOcclusion() { return this->calculateOcclusion; }

	bool FireRegistry::FireRegistry::GetCheckLights() { return this->lookupLights; }

	bool FireRegistry::FireRegistry::GetCheckSmoke() { return this->lookupSmoke; }

	void FireRegistry::FireRegistry::SetLookupSmoke(bool a_value) { this->lookupSmoke = a_value; }

	void FireRegistry::FireRegistry::SetLookupLight(bool a_value) { this->lookupLights = a_value; }

	RE::TESForm* FireRegistry::GetOffMatch(RE::TESForm* a_offFire) {
		if (this->reverseRegister.find(a_offFire) != this->reverseRegister.end()) {
			return this->reverseRegister.at(a_offFire);
		}
		return nullptr;
	}

	std::vector<RE::TESForm*> FireRegistry::FireRegistry::GetStoredSmokes() {
		auto response = std::vector<RE::TESForm*>();

		for (auto smoke : this->smokeRegister) {
			response.push_back(smoke.first);
		}

		return response;
	}

	bool FireRegistry::RegisterPair(RE::TESForm* a_lit, offFire a_off) {
		this->fireRegister[a_lit] = a_off;
		return true;
	}

	bool FireRegistry::RegisterReversePair(RE::TESForm* a_off, RE::TESForm* a_lit) {
		this->reverseRegister[a_off] = a_lit;
		return true;
	}

	bool FireRegistry::FireRegistry::RegisterSmokeObject(RE::TESForm* a_smoke) {
		this->smokeRegister[a_smoke] = true;
		return false;
	}

	bool FireRegistry::BuildRegistry() {
		Settings::ApplySettings();
		return true;
	}
}