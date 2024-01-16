#include "settingsReader.h"
#include "fireRegister.h"

namespace Settings {
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

	bool Settings::ReadSettings() {
		/*
		CSimpleIniA ini;
		ini.SetUnicode();
		ini.LoadFile(R"(.\Data\SKSE\Plugins\RainExtinguishesFires.ini)");
		if (ini.IsEmpty()) return false;

		this->checkForOcclusion = ini.GetBoolValue("", "bCheckForOcclusion");
		this->searchForLights = ini.GetBoolValue("", "bFindNearbyLights");
		this->searchForSmoke = ini.GetBoolValue("", "bFindNearbySmoke");
		*/

		this->checkForOcclusion = false;
		this->searchForLights = true;
		this->searchForSmoke = true;

		//INI Found, settings stored. Read the JSON.
		//TODO: This will require validation in the future.
		std::vector<std::string> configPaths = clib_util::distribution::get_configs(R"(Data\SKSE\Plugins\Rain Extinguishes Fires\)", "_FIRE", ".json"sv);

		bool dyndoldFound = RE::TESDataHandler::GetSingleton()->LookupLoadedModByName("DynDOLOD.esp") ? true : false;
		if (dyndoldFound) _loggerInfo("Found DynDOLOD. REF will attempt to patch automatically.");

		if (configPaths.empty()) return false;
		auto* fireRegistry = FireRegistry::FireRegistry::GetSingleton();

		for (auto& config : configPaths) {
			std::ifstream rawJSON(config);
			Json::Reader  JSONReader;
			Json::Value   JSONFile;
			JSONReader.parse(rawJSON, JSONFile);
			Json::Value fireData = JSONFile["Fires"];
			Json::Value smokeData = JSONFile["Smokes"];

			if (!fireData.empty()) {
				//Assuming fireData exists and is an array of Objects.
				for (auto& fire : fireData) {
					FireRegistry::offFire offFireData = FireRegistry::offFire();
					std::string baseSource = fire["Source"].asString();
					std::string offSource = fire["OffSource"].asString();
					std::string baseFormIDstr = fire["SourceID"].asString();
					std::string offFormIDstr = fire["OffSourceID"].asString();

					RE::FormID baseFormID = StringToFormID(baseFormIDstr);
					RE::FormID offFormID = StringToFormID(offFormIDstr);

					RE::TESForm* litForm = RE::TESDataHandler::GetSingleton()->LookupForm(baseFormID, baseSource);
					RE::TESForm* offForm = RE::TESDataHandler::GetSingleton()->LookupForm(offFormID, offSource);

					offFireData.offVersion = offForm;
					if (!litForm || !offForm) continue;

					_loggerInfo("    [Swap]");
					_loggerInfo("    >Source: {}.", baseSource);

					std::string baseEDID = clib_util::editorID::get_editorID(litForm);
					if (baseEDID.empty()) {
						_loggerInfo("    >Base Fire: {}.", baseFormIDstr);
						_loggerInfo("    >Off Fire: {} -> {}.", offSource, clib_util::editorID::get_editorID(offForm));
					}
					else {
						_loggerInfo("    >Base Fire: {}.", baseEDID);
						_loggerInfo("    >Off Fire: {} -> {}.", offSource, offFormIDstr);
					}

					if (!smokeData.empty()) {
						Json::Value sourceVal = smokeData["Source"];
						if (!sourceVal) continue;

						std::string source = smokeData["Source"].asString();
						for (auto object : smokeData["IDs"]) {
							std::string IDstr = object.asString();
							RE::FormID ID = StringToFormID(IDstr);
							if (!ID || ID == NULL) continue;

							RE::TESForm* smokeForm = RE::TESDataHandler::GetSingleton()->LookupForm(ID, source);
							offFireData.validSmokes.push_back(smokeForm);

							std::string smokeEDID = clib_util::editorID::get_editorID(smokeForm);
							if (smokeEDID.empty()) {
								_loggerInfo("    >Related smoke object: {} -> {}.", source, IDstr);
							}
							else {
								_loggerInfo("    >Related smoke object: {} -> {}.", source, smokeEDID);
							}
						}
					}

					if (dyndoldFound) {
						if (!baseEDID.empty()) {
							std::string dyndolodEDID = baseEDID + "_DynDOLOD_BASE";
							auto* foundForm = RE::TESForm::LookupByEditorID(dyndolodEDID);

							if (foundForm) {
								_loggerInfo("    >Found DynDOLOD match: {}.", dyndolodEDID);
								offFireData.dyndolodFire = true;
							}
						}
					}
					fireRegistry->RegisterPair(litForm, offFireData);
					fireRegistry->RegisterReversePair(offForm, litForm);
				}
			}
		}
		return true;
	}

	bool Settings::CheckForOcclusion() { return this->checkForOcclusion; }
	bool Settings::SearchForLights() { return this->searchForLights; }
	bool Settings::SearchForSmoke() { return this->searchForSmoke; }
}