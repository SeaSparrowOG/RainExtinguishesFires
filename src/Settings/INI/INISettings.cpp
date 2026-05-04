#include "INISettings.h"

#include <SimpleIni.h>
#undef max
#undef min

namespace Settings::INI
{
	bool Read() {
		logger::info("Reading INI settings..."sv);
		auto* holder = Holder::GetSingleton();
		if (!holder) {
			logger::critical("  >Couldn't get INI settings holder."sv);
			return false;
		}
		return holder->StoreSettings();
	}

	bool Holder::StoreSettings() {
		bool encounteredError = false;

		std::string iniPath = fmt::format(R"(.\Data\SKSE\Plugins\{}.ini)"sv, Plugin::NAME);
		CSimpleIniA ini{};
		size_t settingCount = 0;
		logger::info("Reading and validating INI settings from {}.ini"sv, Plugin::NAME);

		try {
			ini.SetUnicode();
			ini.LoadFile(iniPath.data());

			std::list<CSimpleIniA::Entry> sections{};
			ini.GetAllSections(sections);

			if (sections.empty()) {
				if constexpr (EXPECTED_COUNT > 0) {
					logger::critical("  >INI has no settings, but expected {}."sv, EXPECTED_COUNT);
					return false;
				}
				return true;
			}

			for (const auto& section : sections) {
				std::list<CSimpleIniA::Entry> sectionKeys{};
				ini.GetAllKeys(section.pItem, sectionKeys);

				if (sectionKeys.empty()) {
					logger::warn("  >INI section {} has no settings. This MAY be normal.", section.pItem);
					continue;
				}

				settingCount += sectionKeys.size();
				for (const auto& key : sectionKeys) {
					const std::string foundSetting = fmt::format<std::string>("{}|{}"sv, section.pItem, key.pItem);
					if (std::find(EXPECTED_SETTINGS.begin(), EXPECTED_SETTINGS.end(), foundSetting) == EXPECTED_SETTINGS.end()) {
						logger::critical("  >Unexpected setting found: {}", foundSetting);
						return false;
					}

					const auto settingKeyName = std::string(key.pItem);
					if (settingKeyName.size() < 1) {
						logger::error("  >Invalid setting in section {}."sv, key.pItem, section.pItem);
						encounteredError = true;
						continue;
					}

					const auto settingType = settingKeyName.substr(0, 1);
					if (settingType == "s") {
						const std::string value = ini.GetValue(section.pItem, key.pItem);
						if (value.empty()) {
							logger::error("  >Invalid value in string setting {}."sv, foundSetting);
							encounteredError = true;
						}
						else if (stringSettings.contains(foundSetting)) {
							logger::error("  >Setting redefinition {}."sv, foundSetting);
							encounteredError = true;
						}

						stringSettings.emplace(foundSetting, value);
					}
					else if (settingType == "f") {
						const double raw = ini.GetDoubleValue(section.pItem, key.pItem);
						const float value = raw > std::numeric_limits<float>::max() ?
							std::numeric_limits<float>::max() :
							raw < std::numeric_limits<float>::lowest() ?
							std::numeric_limits<float>::lowest() :
							static_cast<float>(raw);
						if (floatSettings.contains(foundSetting)) {
							logger::error("  >Setting redefinition {}."sv, foundSetting);
							encounteredError = true;
						}

						floatSettings.emplace(foundSetting, value);
					}
					else if (settingType == "b") {
						if (boolSettings.contains(foundSetting)) {
							logger::error("  >Setting redefinition {}."sv, foundSetting);
							encounteredError = true;
						}

						boolSettings.emplace(foundSetting, ini.GetBoolValue(section.pItem, key.pItem));
					}
					else if (settingType == "i") {
						const long value = ini.GetLongValue(section.pItem, key.pItem);
						if (longSettings.contains(foundSetting)) {
							logger::error("  >Setting redefinition {}."sv, foundSetting);
							encounteredError = true;
						}

						longSettings.emplace(foundSetting, value);
					}
					else {
						logger::error("  >Invalid setting {}. Settings must be prefixed by s, f, b, or i."sv, foundSetting);
						encounteredError = true;
					}
				}
			}
		}
		catch (std::exception& e) {
			logger::error("Caught exception {} while fetching INI settings.", e.what());
			return false;
		}

		logger::info("  >Finished reading {} settings.", std::to_string(settingCount));

		if (encounteredError) {
			logger::info("Errors were encountered while reading the INI file. See log for more details."sv);
			return false;
		}

		OverrideSettings();
		DumpSettings();
		return true;
	}

	void Holder::DumpSettings()
	{
		logger::info("Stored Settings:"sv);
		for (const auto& [name, value] : boolSettings) {
			logger::info("  >{} - {}", name, value ? "TRUE" : "FALSE");
		}
		for (const auto& [name, value] : stringSettings) {
			logger::info("  >{} - {}", name, value);
		}
		for (const auto& [name, value] : longSettings) {
			logger::info("  >{} - {}", name, value);
		}
		for (const auto& [name, value] : floatSettings) {
			logger::info("  >{} - {}", name, value);
		}
	}

	bool Holder::OverrideSettings() {
		logger::info("Checking the custom INI..."sv);
		std::string iniPath = fmt::format(R"(.\Data\SKSE\Plugins\{}_custom.ini)"sv, Plugin::NAME);
		if (!std::filesystem::exists(iniPath)) {
			logger::info("  >Custom INI not found."sv);
			return true;
		}

		CSimpleIniA ini{};
		try {
			ini.SetUnicode();
			ini.LoadFile(iniPath.data());

			std::list<CSimpleIniA::Entry> sections{};
			ini.GetAllSections(sections);

			if (sections.empty()) {
				logger::warn("  >Finished reading Custom INI file, but found no overrides.");
				return true;
			}

			for (const auto& section : sections) {
				std::list<CSimpleIniA::Entry> sectionKeys{};
				ini.GetAllKeys(section.pItem, sectionKeys);

				if (sectionKeys.empty()) {
					logger::warn("  >Custom INI section {} has no settings.", section.pItem);
					continue;
				}

				for (const auto& key : sectionKeys) {
					const std::string foundSetting = fmt::format<std::string>("{}|{}"sv, section.pItem, key.pItem);
					const auto settingKeyName = std::string(key.pItem);
					if (settingKeyName.size() < 1) {
						logger::error("  >Invalid setting in section {}."sv, key.pItem, section.pItem);
						continue;
					}

					const auto settingType = settingKeyName.substr(0, 1);
					if (settingType == "s") {
						const std::string value = ini.GetValue(section.pItem, key.pItem);
						if (value.empty()) {
							logger::error("  >Invalid value in string setting {} in custom INI."sv, foundSetting);
							continue;
						}
						else if (!stringSettings.contains(foundSetting)) {
							logger::error("  >Setting {} not defined in the base INI."sv, foundSetting);
							continue;
						}

						logger::info("  >Overrode {} with {}."sv, foundSetting, value);
						stringSettings[foundSetting] = value;
					}
					else if (settingType == "f") {
						const double raw = ini.GetDoubleValue(section.pItem, key.pItem);
						const float value = raw > std::numeric_limits<float>::max() ?
							std::numeric_limits<float>::max() :
							raw < std::numeric_limits<float>::lowest() ?
							std::numeric_limits<float>::lowest() :
							static_cast<float>(raw);
						if (!floatSettings.contains(foundSetting)) {
							logger::error("  >Setting {} not defined in the base INI."sv, foundSetting);
							continue;
						}

						logger::info("  >Overrode {} with {}."sv, foundSetting, std::to_string(value));
						floatSettings[foundSetting] = value;
					}
					else if (settingType == "b") {
						if (!boolSettings.contains(foundSetting)) {
							logger::error("  >Setting {} not defined in the base INI."sv, foundSetting);
							continue;
						}

						logger::info("  >Overrode {} with {}."sv, foundSetting, ini.GetBoolValue(section.pItem, key.pItem));
						boolSettings[foundSetting] = ini.GetBoolValue(section.pItem, key.pItem);
					}
					else if (settingType == "i") {
						const long value = ini.GetLongValue(section.pItem, key.pItem);
						if (!longSettings.contains(foundSetting)) {
							logger::error("  >Setting {} not defined in the base INI."sv, foundSetting);
							continue;
						}

						logger::info("  >Overrode {} with {}."sv, foundSetting, std::to_string(value));
						longSettings[foundSetting] = value;
					}
					else {
						logger::error("  >Invalid setting {}. Settings must be prefixed by s, f, b, or i."sv, foundSetting);
					}
				}
			}
		}
		catch (std::exception& e) {
			logger::error("  >Caught exception {} while reading the CUSTOM ini.", e.what());
			return false;
		}

		return true;
	}
}