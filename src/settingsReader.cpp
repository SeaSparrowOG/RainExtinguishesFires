#include "settingsReader.h"
#include "fireRegister.h"
#include "utilityFunctions.h"

namespace INI {
	bool ShouldRebuildINI(CSimpleIniA* a_ini) {
		const char* section = "General";
		const char* keys[5] = { "bSquashLights", "bSquashSmoke", "fReferenceLookupRadius", "fFireLookupRadius", "fDaysToReset" };
		int sectionLength = 5;
		std::list<CSimpleIniA::Entry> keyHolder;

		a_ini->GetAllKeys(section, keyHolder);
		if (std::size(keyHolder) != sectionLength) return true;
		for (auto* key : keys) {
			if (!a_ini->KeyExists(section, key)) return true;
		}
		return false;
	}

	bool ReadSettings() {
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
			ini.SetBoolValue("General", "bSquashLights", true, ";Disables the nearest found light.");
			ini.SetBoolValue("General", "bSquashSmoke", true, ";Disables the nearest found smoke object.");
			ini.SetDoubleValue("General", "fReferenceLookupRadius", 300.0, ";The distance over which to search for light/smoke/dyndolod fires.");
			ini.SetDoubleValue("General", "fFireLookupRadius", 0.0, ";The distance over which to search for fires when a new cell loads. 0.0 will fetch from all loaded cells.");
			ini.SetDoubleValue("General", "fDaysToReset", 3.0, ";How long it will take for unlit fires to be re-lit.");
			ini.SaveFile(f.c_str());
		}
		
		auto settingsSingleton = CachedData::Fires::GetSingleton();
		settingsSingleton->SetLookupLight(ini.GetBoolValue("General", "bSquashLights", true));
		settingsSingleton->SetLookupSmoke(ini.GetBoolValue("General", "bSquashSmoke", true));
		settingsSingleton->SetFireLookupRadius(ini.GetDoubleValue("General", "fFireLookupRadius", 0.0));
		settingsSingleton->SetReferenceLookupRadius(ini.GetDoubleValue("General", "fReferenceLookupRadius", 300.0));
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

		if (customINI.KeyExists("General", "fFireLookupRadius")) {
			settingsSingleton->SetFireLookupRadius(customINI.GetDoubleValue("General", "fFireLookupRadius", 300.0));
		}

		if (customINI.KeyExists("General", "fReferenceLookupRadius")) {
			settingsSingleton->SetReferenceLookupRadius(customINI.GetDoubleValue("General", "fReferenceLookupRadius", 300.0));
		}

		if (customINI.KeyExists("General", "fDaysToReset")) {
			settingsSingleton->SetRequiredOffTime(customINI.GetDoubleValue("General", "fDaysToReset", 3.0));
		}
		return true;
	}

}

namespace JSON {
	bool ReadSettings() {
		std::vector<std::string> configPaths = std::vector<std::string>();
		try {
			configPaths = clib_util::distribution::get_configs(R"(Data\SKSE\Plugins\Rain Extinguishes Fires\)", "", ".json"sv);
		}
		catch (std::exception e) {
			_loggerError("Caught error {} while trying to fetch fire config files.", e.what());
			return false;
		}
		if (configPaths.empty()) return true;

		bool dyndoldFound = RE::TESDataHandler::GetSingleton()->LookupLoadedModByName("DynDOLOD.esp") ? true : false;
		if (dyndoldFound) _loggerInfo("    Found DynDOLOD. REF will attempt to patch automatically. Automatic patching requires PO3's Tweaks.");
		auto* cachedDataSingleton = CachedData::Fires::GetSingleton();

		for (auto& config : configPaths) {
			std::ifstream rawJSON(config);
			Json::Reader  JSONReader;
			Json::Value   JSONFile;
			JSONReader.parse(rawJSON, JSONFile);

			auto& fireData = JSONFile["FireData"];
			for (auto& fire : fireData) {
				FireData offFireData = FireData();
				auto& sourceField = fire["Source"];
				auto& offField = fire["Off"];
				if (!(sourceField && sourceField.isString() && offField && offField.isString())) continue;

				RE::TESForm* onForm = UtilityFunction::ParseForm(sourceField.asString());
				RE::TESForm* offForm = UtilityFunction::ParseForm(offField.asString());
				if (!onForm || !offForm) continue;

				auto& lightRadius = fire["Light"];
				auto& smokeRadius = fire["Smoke"];

				if (lightRadius && lightRadius.isDouble()) {
					offFireData.lightLookupRadius = lightRadius.asDouble();
				}

				if (smokeRadius && smokeRadius.isDouble()) {
					offFireData.smokeLookupRadius = smokeRadius.asDouble();
				}

				std::string baseEDID = clib_util::editorID::get_editorID(onForm);
				if (dyndoldFound && !baseEDID.empty()) {
					std::string dyndolodEDID = baseEDID + "_DynDOLOD_BASE";
					auto* foundForm = RE::TESForm::LookupByEditorID(dyndolodEDID);

					if (foundForm) {
						offFireData.dyndolodVersion = foundForm;
						offFireData.dyndolodFire = true;
					}
				}

				offFireData.offVersion = offForm;
				cachedDataSingleton->RegisterPair(onForm, offFireData);
			}

			auto& smokeData = JSONFile["SmokeData"];
			for (auto& smoke : smokeData) {
				auto& smokeField = smoke["Smoke"];
				if (smokeField && smokeField.isString()) {
					RE::TESForm* smokeForm = UtilityFunction::ParseForm(smokeField.asString());
					if (!smokeForm) continue;

					cachedDataSingleton->RegisterSmokeObject(smokeForm);
				}
			}
		}
		return true;
	}
}

namespace Settings {
	bool InitializeINISettings() {
		return INI::ReadSettings();
	}

	bool InitializeFireSettings() {
		return JSON::ReadSettings();
	}
}