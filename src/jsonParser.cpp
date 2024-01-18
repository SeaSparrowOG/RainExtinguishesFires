#include "iniParser.h"

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

		Json::Value smokeData = a_json["Smokes"];
		if (smokeData && !smokeData.isArray()) return false;
		
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
		if (fireData && !fireData.isArray()) return false;

		for (auto object : fireData) {
			if (!object.isObject()) return false;

			auto source = object["Source"];
			auto sourceID = object["SourceID"];
			auto offSource = object["OffSource"];
			auto offSourceID = object["OffSourceID"];
			if (!(source && offSource && offSourceID && sourceID)) {
				return false;
			}

			if (!(source.isString() && offSource.isString() && offSourceID.isString() && sourceID.isString())) {
				return false;
			}

			if (!(IsHex(source.asString()) && IsHex(source.asString()) && IsHex(source.asString()) && IsHex(source.asString()))) {
				return false;
			}
		}
		return true;
	}

	bool ApplySettings() {
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

		auto* fireRegistry = FireRegistry::FireRegistry::GetSingleton();

		for (auto& config : configPaths) {
			std::ifstream rawJSON(config);
			Json::Reader  JSONReader;
			Json::Value   JSONFile;
			JSONReader.parse(rawJSON, JSONFile);
			if (!IsValidFireJSON(JSONFile)) continue;

			auto fireData = JSONFile["Fires"];
			for (auto& fire : fireData) {
				FireRegistry::offFire offFireData = FireRegistry::offFire();
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

				_loggerInfo("        [Fire Data]");
				_loggerInfo("            >Source: {}.", baseSource);

				std::string baseEDID = clib_util::editorID::get_editorID(litForm);
				if (baseEDID.empty()) {
					_loggerInfo("            >Base Fire: {} -> {}.", baseSource, baseFormIDstr);
					_loggerInfo("            >Off Fire: {} -> {}.", offSource, clib_util::editorID::get_editorID(offForm));
				}
				else {
					_loggerInfo("            >Base Fire: {} -> {}.", baseSource, baseEDID);
					_loggerInfo("            >Off Fire: {} -> {}.", offSource, offFormIDstr);
				}

				if (dyndoldFound) {
					if (!baseEDID.empty()) {
						std::string dyndolodEDID = baseEDID + "_DynDOLOD_BASE";
						auto* foundForm = RE::TESForm::LookupByEditorID(dyndolodEDID);

						if (foundForm) {
							_loggerInfo("            >Found DynDOLOD match: {}.", dyndolodEDID);
							offFireData.dyndolodFire = true;
						}
					}
				}
				fireRegistry->RegisterPair(litForm, offFireData);
				fireRegistry->RegisterReversePair(offForm, litForm);
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

		for (auto& config : configPaths) {
			std::ifstream rawJSON(config);
			Json::Reader  JSONReader;
			Json::Value   JSONFile;
			JSONReader.parse(rawJSON, JSONFile);
			if (!IsValidFireJSON(JSONFile)) continue;

			for (auto smokeData : JSONFile["Smoke"]) {
				std::string source = smokeData["Source"].asString();
				for (auto object : smokeData["IDs"]) {
					auto IDstr = object.asString();
					auto ID = StringToFormID(IDstr);
					if (!ID || ID == NULL) continue;

					auto* smokeForm = RE::TESDataHandler::GetSingleton()->LookupForm(ID, source);
					if (!smokeForm) continue;
					FireRegistry::FireRegistry::GetSingleton()->RegisterSmokeObject(smokeForm);

					auto smokeEDID = clib_util::editorID::get_editorID(smokeForm);
					if (smokeEDID.empty()) {
						_loggerInfo("            >Registered smoke object: {} -> {}.", source, IDstr);
					}
					else {
						_loggerInfo("            >Registered smoke object: {} -> {}.", source, smokeEDID);
					}
				}
			}
		}
		return true;
	}
}