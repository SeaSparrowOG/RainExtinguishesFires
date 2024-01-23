#include "fireRegister.h"

#include <iostream>
#include <fstream>  

namespace SettingsReader {
	/**
	* Converts given string into a usable FormID. Credids to Colinswrath.
	* Nexus:
	* Github:
	* @param a_str The string to convert.
	* @return The FormID as RE::FormID.
	*/
	RE::FormID StringToFormID(std::string a_str) {
		RE::FormID result;
		std::istringstream ss{ a_str };
		ss >> std::hex >> result;
		return result;
	}

	/**
	* Yay for StackOverflow.
	* Source: https://stackoverflow.com/questions/8899069/how-to-find-if-a-given-string-conforms-to-hex-notation-eg-0x34ff-without-regex
	*/
	bool IsHex(std::string const& s) {
		return s.compare(0, 2, "0x") == 0
			&& s.size() > 2
			&& s.find_first_not_of("0123456789abcdefABCDEF", 2) == std::string::npos;
	}

	bool IsValidSmokeJSON(Json::Value a_json) {
		if (!a_json || a_json.empty()) return false;

		Json::Value smokeData = a_json["Smoke"];
		if (!smokeData || !smokeData.isArray()) return false;

		for (auto object : smokeData) {
			if (!object.isObject()) return false;
			if (!(object["IDs"] && object["Source"])) return false;
			if (!object["IDs"].isArray()) return false;
			if (!object["Source"].isString()) return false;

			for (auto id : object["IDs"]) {
				if (!id.isString()) return false;
			}
		}
	}

	bool IsValidFireJSON(Json::Value a_json) {
		if (!a_json || a_json.empty()) return false;

		Json::Value fireData = a_json["Fires"];
		if (!fireData || !fireData.isArray()) return false;

		for (auto object : fireData) {
			if (!object.isObject()) return false;

			auto source = object["Source"];
			auto sourceID = object["SourceID"];
			auto offSource = object["OffSource"];
			auto offSourceID = object["OffSourceID"];
			if (!(source && offSource && offSourceID && sourceID)) return false;
			if (!(source.isString() && offSource.isString() && offSourceID.isString() && sourceID.isString())) return false;
			if (!(IsHex(sourceID.asString()) && IsHex(offSourceID.asString()))) return false;
		}
		return true;
	}

	bool ShouldRebuildINI(CSimpleIniA* a_ini) {
		const char* section = "General";
		const char* keys[5] = { "bSquashLights", "bSquashSmoke", "fLightSearchRadius", "fSmokeSearchRadius", "fDaysToReset"};
		int sectionLength = 5;
		std::list<CSimpleIniA::Entry> keyHolder;

		a_ini->GetAllKeys(section, keyHolder);
		if (std::size(keyHolder) != sectionLength) return true;
		for (auto* key : keys) {
			if (!a_ini->KeyExists(section, key)) return true;
		}
		return false;
	}

	bool BuildINI() {
		std::filesystem::path f{ "./Data/SKSE/Plugins/RainExtinguishesFires.ini" };
		bool createEntries = false;
		if (!std::filesystem::exists(f)) {
			std::fstream createdINI;
			createdINI.open(f, std::ios::out);
			createdINI.close();
			createEntries = true;
		}

		CSimpleIniA ini;
		ini.SetUnicode(); 
		ini.LoadFile(f.c_str());
		if (!createEntries) { createEntries = ShouldRebuildINI(&ini); }

		if (createEntries) {
			ini.Delete("General", NULL);
			ini.SetBoolValue("General",   "bSquashLights", true, ";Disables the nearest found light.");
			ini.SetBoolValue("General",   "bSquashSmoke", true, ";Disables the nearest found smoke object.");
			ini.SetDoubleValue("General", "fLightSearchRadius", 300.0, ";The distance over which to search for the light.");
			ini.SetDoubleValue("General", "fSmokeSearchRadius", 300.0, ";The distance over which to search for the smoke object.");
			ini.SetDoubleValue("General", "fDaysToReset", 3.0, ";How long it will take for unlit fires to be re-lit.");
			ini.SaveFile(f.c_str());
		}

		auto settingsSingleton = CachedData::FireRegistry::GetSingleton();
		settingsSingleton->SetLookupLight(ini.GetBoolValue("General", "bSquashLights", true));
		settingsSingleton->SetLookupSmoke(ini.GetBoolValue("General", "bSquashSmoke", true));
		settingsSingleton->SetLookupLightDistance(ini.GetDoubleValue("General", "fLightSearchRadius", 300.0));
		settingsSingleton->SetLookupSmokeDistance(ini.GetDoubleValue("General", "fSmokeSearchRadius", 300.0));
		settingsSingleton->SetRequiredOffTime(ini.GetDoubleValue("General", "fDaysToReset", 3.0));

		std::filesystem::path custom{ "./Data/SKSE/Plugins/RainExtinguishesFires_custom.ini" };
		if (!std::filesystem::exists(f)) {
			return true;
		}

		CSimpleIniA customINI;
		customINI.SetUnicode();
		customINI.LoadFile(custom.c_str());

		if (customINI.KeyExists("General", "bSquashLights")) {
			settingsSingleton->SetLookupLight(customINI.GetBoolValue("General", "bSquashLights", true));
		}

		if (customINI.KeyExists("General", "bSquashSmoke")) {
			settingsSingleton->SetLookupSmoke(customINI.GetBoolValue("General", "bSquashSmoke", true));
		}

		if (customINI.KeyExists("General", "fLightSearchRadius")) {
			settingsSingleton->SetLookupLightDistance(customINI.GetDoubleValue("General", "fLightSearchRadius", 300.0));
		}

		if (customINI.KeyExists("General", "fSmokeSearchRadius")) {
			settingsSingleton->SetLookupSmokeDistance(customINI.GetDoubleValue("General", "fSmokeSearchRadius", 300.0));
		}

		if (customINI.KeyExists("General", "fDaysToReset")) {
			settingsSingleton->SetLookupSmokeDistance(customINI.GetDoubleValue("General", "fDaysToReset", 3.0));
		}
		return true;
	}

	bool ReadConfigs() {
		std::vector<std::string> configPaths = std::vector<std::string>();
		try {
			configPaths = clib_util::distribution::get_configs(R"(Data\SKSE\Plugins\Rain Extinguishes Fires\)", "_FIRE", ".json"sv);
		}
		catch (std::exception e) {
			_loggerError("Caught error {} while trying to fetch fire config files.", e.what());
			return false;
		}

		if (configPaths.empty()) return true;

		bool dyndoldFound = RE::TESDataHandler::GetSingleton()->LookupLoadedModByName("DynDOLOD.esp") ? true : false;
		if (dyndoldFound) _loggerInfo("    Found DynDOLOD. REF will attempt to patch automatically.");

		auto* fireRegistry = CachedData::FireRegistry::GetSingleton();

		for (auto& config : configPaths) {
			std::ifstream rawJSON(config);
			Json::Reader  JSONReader;
			Json::Value   JSONFile;
			JSONReader.parse(rawJSON, JSONFile);
			if (!IsValidFireJSON(JSONFile)) {
				_loggerInfo("Could not validate JSON.");
				continue;
			}

			auto fireData = JSONFile["Fires"];
			for (auto& fire : fireData) {
				CachedData::FireData offFireData = CachedData::FireData();
				auto baseSource = fire["Source"].asString();
				auto offSource = fire["OffSource"].asString();
				auto baseFormIDstr = fire["SourceID"].asString();
				auto offFormIDstr = fire["OffSourceID"].asString();

				auto baseFormID = StringToFormID(baseFormIDstr);
				auto offFormID = StringToFormID(offFormIDstr);
				if (!(baseFormID && offFormID)) continue;

				auto* litForm = RE::TESDataHandler::GetSingleton()->LookupForm(baseFormID, baseSource);
				auto* offForm = RE::TESDataHandler::GetSingleton()->LookupForm(offFormID, offSource);
				if (!litForm || !offForm) continue;

				offFireData.offVersion = offForm;
				std::string baseEDID = clib_util::editorID::get_editorID(litForm);
				if (baseEDID.empty()) {
					_loggerInfo("        >Registered Fire: {} -> {}.", baseFormIDstr, offFormIDstr);
				}
				else {
					_loggerInfo("        >Registered Fire: {} -> {}.", clib_util::editorID::get_editorID(litForm), clib_util::editorID::get_editorID(offForm));
				}

				if (dyndoldFound) {
					if (!baseEDID.empty()) {
						std::string dyndolodEDID = baseEDID + "_DynDOLOD_BASE";
						auto* foundForm = RE::TESForm::LookupByEditorID(dyndolodEDID);

						if (foundForm) {
							_loggerInfo("            >DynDOLOD match: {}.", dyndolodEDID);
							offFireData.dyndolodFire = true;
						}
					}
				}
				
				fireRegistry->RegisterPair(litForm, offFireData);
			}
		}

		try {
			configPaths = clib_util::distribution::get_configs(R"(Data\SKSE\Plugins\Rain Extinguishes Fires\)", "_SMOKE", ".json"sv);
		}
		catch (std::exception e) {
			_loggerError("Caught error {} while trying to fetch smoke config files.", e.what());
			return false;
		}
		if (configPaths.empty()) return true;

		_loggerInfo("    [Smoke Objects]");
		for (auto& config : configPaths) {
			std::ifstream rawJSON(config);
			Json::Reader  JSONReader;
			Json::Value   JSONFile;
			JSONReader.parse(rawJSON, JSONFile);
			if (!IsValidSmokeJSON(JSONFile)) continue;

			for (auto smokeData : JSONFile["Smoke"]) {
				std::string source = smokeData["Source"].asString();
				for (auto object : smokeData["IDs"]) {
					auto IDstr = object.asString();
					auto ID = StringToFormID(IDstr);
					if (!ID || ID == NULL) continue;

					auto* smokeForm = RE::TESDataHandler::GetSingleton()->LookupForm(ID, source);
					if (!smokeForm) continue;
					CachedData::FireRegistry::GetSingleton()->RegisterSmokeObject(smokeForm);

					auto smokeEDID = clib_util::editorID::get_editorID(smokeForm);
					if (smokeEDID.empty()) {
						_loggerInfo("        >Registered smoke object: {} -> {}.", source, IDstr);
					}
					else {
						_loggerInfo("        >Registered smoke object: {} -> {}.", source, smokeEDID);
					}
				}
			}
		}
		return true;
	}
}

namespace CachedData {
	bool FireRegistry::GetCheckOcclusion() { return this->calculateOcclusion; }

	bool FireRegistry::GetCheckLights() { return this->lookupLights; }

	bool FireRegistry::GetCheckSmoke() { return this->lookupSmoke; }

	std::vector<RE::TESForm*> FireRegistry::GetStoredSmokeObjects() { return this->storedSmoke; }

	float CachedData::FireRegistry::GetSmokeSearchDistance() { return this->smokeLookupRadius; }

	float CachedData::FireRegistry::GetLightSearchDistance() { return this->lightLookupRadius; }

	float CachedData::FireRegistry::GetRequiredOffTime() { return this->requiredOffTime; }

	void FireRegistry::SetLookupSmoke(bool a_value) { this->lookupSmoke = a_value; }

	void FireRegistry::SetLookupLight(bool a_value) { this->lookupLights = a_value; }

	void CachedData::FireRegistry::SetLookupSmokeDistance(float a_value) { this->smokeLookupRadius = a_value; }

	void CachedData::FireRegistry::SetLookupLightDistance(float a_value) { this->lightLookupRadius = a_value; }

	void CachedData::FireRegistry::SetRequiredOffTime(float a_value) { this->requiredOffTime = a_value; }

	bool FireRegistry::IsOnFire(RE::TESForm* a_litFire) {
		if (this->fireRegister.contains(a_litFire)) return true;
		return false;
	}

	bool FireRegistry::IsOffFire(RE::TESForm* a_offForm) {
		if (this->reverseFireRegister.contains(a_offForm)) return true;
		return false;
	}

	bool FireRegistry::IsDynDOLODFire(RE::TESForm* a_litFire) {
		if (this->fireRegister.find(a_litFire) != this->fireRegister.end()) {
			return this->fireRegister[a_litFire].dyndolodFire;
		}
		return false;
	}

	FireData FireRegistry::GetOffForm(RE::TESForm* a_litFire) {
		if (this->fireRegister.contains(a_litFire)) {
			return this->fireRegister.at(a_litFire);
		}
		return FireData();
	}

	bool FireRegistry::RegisterPair(RE::TESForm* a_lit, FireData a_data) {
		if (!a_data.offVersion) return false;
		this->fireRegister[a_lit] = a_data;
		this->reverseFireRegister[a_data.offVersion] = true;
		return true;
	}

	bool FireRegistry::RegisterSmokeObject(RE::TESForm* a_smoke) {
		if (!a_smoke) return false;
		if (this->smokeRegister.contains(a_smoke)) return false;
		this->smokeRegister[a_smoke] = true;
		this->storedSmoke.push_back(a_smoke);
		return true;
	}

	bool FireRegistry::BuildRegistry() {
		if (!(SettingsReader::BuildINI() && SettingsReader::ReadConfigs())) return false;
		return true;
	}
}