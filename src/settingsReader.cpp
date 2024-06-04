#include "settingsReader.h"
#include "fireRegister.h"

namespace {
	bool IsModPresent(std::string a_modName) {
		return RE::TESDataHandler::GetSingleton()->LookupModByName(a_modName) ? true : false;
	}

	bool IsHex(std::string const& s) {
		return s.compare(0, 2, "0x") == 0
			&& s.size() > 2
			&& s.find_first_not_of("0123456789abcdefABCDEF", 2) == std::string::npos;
	}

	RE::TESBoundObject* ParseForm(const std::string& a_identifier) {
		if (const auto splitID = clib_util::string::split(a_identifier, "|"); splitID.size() == 2) {
			if (!IsHex(splitID[0])) return nullptr;
			const auto  formID = clib_util::string::to_num<RE::FormID>(splitID[0], true);

			const auto& modName = splitID[1];
			if (!IsModPresent(modName)) return nullptr;

			auto* baseForm = RE::TESDataHandler::GetSingleton()->LookupForm(formID, modName);
			return baseForm ? static_cast<RE::TESBoundObject*>(baseForm) : nullptr;
		}
		auto* form = RE::TESBoundObject::LookupByEditorID(a_identifier);
		if (form) return static_cast<RE::TESBoundObject*>(form);
		return nullptr;
	}
}

namespace INI {
	bool ShouldRebuildINI(CSimpleIniA* a_ini) {
		const char* section = "General";
		const char* keys[] = { "bSquashLights", "bSquashSmoke", "fReferenceLookupRadius", "fLightLookupRadius", "fSmokeLookupRadius", "fDaysToReset" };
		int sectionLength = sizeof(keys) / sizeof(keys[0]);
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
			ini.SetDoubleValue("General", "fReferenceLookupRadius", 300.0, ";The distance over which to search for dyndolod fires.");
			ini.SetDoubleValue("General", "fLightLookupRadius", 300.0, ";The distance over which to search for smoke.");
			ini.SetDoubleValue("General", "fSmokeLookupRadius", 300.0, ";The distance over which to search for light.");
			ini.SetDoubleValue("General", "fDaysToReset", 3.0, ";How long it will take for unlit fires to be re-lit.");
			ini.SaveFile(f.c_str());
		}
		
		auto* cachedDataSingleton = CachedData::Fires::GetSingleton();
		cachedDataSingleton->UpdateSetting(CachedData::Setting::kReferenceRadius,ini.GetDoubleValue("General", "fReferenceLookupRadius", 300.0));
		cachedDataSingleton->UpdateSetting(CachedData::Setting::kResetTime, ini.GetDoubleValue("General", "fDaysToReset", 3.0));
		cachedDataSingleton->UpdateSetting(CachedData::Setting::kLightEnabled, ini.GetBoolValue("General", "bSquashLights", true));
		cachedDataSingleton->UpdateSetting(CachedData::Setting::kSmokeEnabled, ini.GetBoolValue("General", "bSquashSmoke", true));
		cachedDataSingleton->UpdateSetting(CachedData::Setting::kLightRadius, ini.GetDoubleValue("General", "fLightLookupRadius", 300.0));
		cachedDataSingleton->UpdateSetting(CachedData::Setting::kSmokeRadius, ini.GetDoubleValue("General", "fSmokeLookupRadius", 300.0));

		std::filesystem::path custom{ "./Data/SKSE/Plugins/RainExtinguishesFires_custom.ini" };
		if (!std::filesystem::exists(f)) {
			return true;
		}

		CSimpleIniA customINI;
		customINI.SetUnicode();
		customINI.LoadFile(custom.c_str());

		if (customINI.KeyExists("General", "fReferenceLookupRadius")) {
			cachedDataSingleton->UpdateSetting(CachedData::Setting::kReferenceRadius, customINI.GetDoubleValue("General", "fReferenceLookupRadius", 300.0));
		}

		if (customINI.KeyExists("General", "fDaysToReset")) {
			cachedDataSingleton->UpdateSetting(CachedData::Setting::kResetTime, customINI.GetDoubleValue("General", "fDaysToReset", 3.0));
		}

		if (customINI.KeyExists("General", "bSquashLights")) {
			cachedDataSingleton->UpdateSetting(CachedData::Setting::kLightEnabled, customINI.GetBoolValue("General", "bSquashLights", true));
		}

		if (customINI.KeyExists("General", "bSquashSmoke")) {
			cachedDataSingleton->UpdateSetting(CachedData::Setting::kSmokeEnabled, customINI.GetBoolValue("General", "bSquashSmoke", true));
		}

		if (customINI.KeyExists("General", "fLightLookupRadius")) {
			cachedDataSingleton->UpdateSetting(CachedData::Setting::kLightRadius, customINI.GetDoubleValue("General", "fLightLookupRadius", 300.0));
		}

		if (customINI.KeyExists("General", "fSmokeLookupRadius")) {
			cachedDataSingleton->UpdateSetting(CachedData::Setting::kSmokeRadius, customINI.GetDoubleValue("General", "fSmokeLookupRadius", 300.0));
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

				RE::TESBoundObject* onForm = ParseForm(sourceField.asString());
				RE::TESBoundObject* offForm = ParseForm(offField.asString());
				if (!onForm || !offForm) continue;

				auto& lightRadius = fire["Light"];
				auto& smokeRadius = fire["Smoke"];

				if (lightRadius && lightRadius.isDouble()) {
					offFireData.disableLight = true;
					offFireData.lightLookupRadius = lightRadius.asDouble();
				}

				if (smokeRadius && smokeRadius.isDouble()) {
					offFireData.disableSmoke = true;
					offFireData.smokeLookupRadius = smokeRadius.asDouble();
				}
				
				std::string baseEDID = clib_util::editorID::get_editorID(onForm);
				if (dyndoldFound && !baseEDID.empty()) {
					std::string dyndolodEDID = baseEDID + "_DynDOLOD_BASE";
					auto* foundForm = RE::TESForm::LookupByEditorID(dyndolodEDID);
					auto* dyndoForm = foundForm ? static_cast<RE::TESBoundObject*>(foundForm) : nullptr;

					if (dyndoForm) {
						offFireData.dyndolodVersion = dyndoForm;
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
					auto* smokeForm = ParseForm(smokeField.asString());
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