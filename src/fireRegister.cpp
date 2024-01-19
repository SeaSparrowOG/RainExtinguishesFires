#include "fireRegister.h"

namespace SettingsReader {
	bool BuildINI() {
		return true;
	}

	bool ReadConfigs() {
		return true;
	}
}

namespace CachedData {
	bool FireRegistry::GetCheckOcclusion() { return this->calculateOcclusion; }

	bool FireRegistry::GetCheckLights() { return this->lookupLights; }

	bool FireRegistry::GetCheckSmoke() { return this->lookupSmoke; }

	std::vector<RE::TESForm*> FireRegistry::GetStoredSmokeObjects() { return this->storedSmoke; }

	void FireRegistry::SetLookupSmoke(bool a_value) { this->lookupSmoke = a_value; }

	void FireRegistry::SetLookupLight(bool a_value) { this->lookupLights = a_value; }

	bool CachedData::FireRegistry::IsManagedFire(RE::TESForm* a_litFire) {
		bool response = false;
		if (this->fireRegister.find(a_litFire) != this->fireRegister.end()) response = true;
		if (!response && this->reverseFireRegister.find(a_litFire) != this->reverseFireRegister.end()) response = true;
		return response;
	}

	bool CachedData::FireRegistry::IsDynDOLODFire(RE::TESForm* a_litFire) {
		if (this->fireRegister.find(a_litFire) != this->fireRegister.end()) {
			return this->fireRegister[a_litFire].dyndolodFire;
		}
		return false;
	}

	bool FireRegistry::IsValidSmoke(RE::TESForm* a_smoke) { 
		if (this->smokeRegister.find(a_smoke) != this->smokeRegister.end()) return true;
		return false; 
	}

	FireData FireRegistry::GetOffForm(RE::TESForm* a_litFire) {
		if (this->fireRegister.find(a_litFire) != this->fireRegister.end()) {
			if (this->fireRegister.at(a_litFire).offVersion) {
				return this->fireRegister.at(a_litFire);
			}
		}
		return FireData();
	}

	bool FireRegistry::RegisterPair(RE::TESForm* a_lit, FireData a_data) {
		if (!a_data.offVersion) return false;
		this->fireRegister[a_lit] = a_data;
		return true;
	}

	bool FireRegistry::RegisterSmokeObject(RE::TESForm* a_smoke) {
		if (!a_smoke) return false;
		this->smokeRegister[a_smoke] = true;
		return true;
	}

	bool FireRegistry::BuildRegistry() {
		if (!(SettingsReader::BuildINI() || SettingsReader::ReadConfigs())) return false;
		return true;
	}
}