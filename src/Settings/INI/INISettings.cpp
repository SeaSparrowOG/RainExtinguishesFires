#include "INISettings.h"

#include <SimpleIni.h>
#undef max
#undef min

namespace Settings::INI
{
	bool Read() {
		logger::INFO("Reading INI settings..."sv);
		auto* holder = Holder::GetSingleton();
		if (!holder) {
			logger::CRITICAL("  >Couldn't get INI settings holder."sv);
			return false;
		}
		return holder->StoreSettings();
	}

	bool Holder::StoreSettings() {
		bool encounteredError = false;

		std::string iniPath = fmt::format(R"(.\Data\SKSE\Plugins\{}.ini)"sv, Plugin::NAME);
		CSimpleIniA ini{};
		size_t settingCount = 0;
		logger::INFO("Reading and validating INI settings from {}.ini"sv, Plugin::NAME);

		try {
			ini.SetUnicode();
			ini.LoadFile(iniPath.data());

			std::list<CSimpleIniA::Entry> sections{};
			ini.GetAllSections(sections);

			if (sections.empty()) {
				if constexpr (EXPECTED_COUNT > 0) {
					logger::CRITICAL("  >INI has no settings, but expected {}."sv, EXPECTED_COUNT);
					return false;
				}
				return true;
			}

			for (const auto& section : sections) {
				std::list<CSimpleIniA::Entry> sectionKeys{};
				ini.GetAllKeys(section.pItem, sectionKeys);

				if (sectionKeys.empty()) {
					logger::WARN("  >INI section {} has no settings. This MAY be normal.", section.pItem);
					continue;
				}

				settingCount += sectionKeys.size();
				for (const auto& key : sectionKeys) {
					const std::string foundSetting = fmt::format<std::string>("{}|{}"sv, section.pItem, key.pItem);
					if (std::find(EXPECTED_SETTINGS.begin(), EXPECTED_SETTINGS.end(), foundSetting) == EXPECTED_SETTINGS.end()) {
						logger::CRITICAL("  >Unexpected setting found: {}", foundSetting);
						return false;
					}

					const auto settingKeyName = std::string(key.pItem);
					if (settingKeyName.size() < 1) {
						logger::WARN("  >Invalid setting in section {}."sv, key.pItem, section.pItem);
						encounteredError = true;
						continue;
					}

					const auto settingType = settingKeyName.substr(0, 1);
					if (settingType == "s") {
						const std::string value = ini.GetValue(section.pItem, key.pItem);
						if (value.empty()) {
							logger::WARN("  >Invalid value in string setting {}."sv, foundSetting);
							encounteredError = true;
						}
						else if (stringSettings.contains(foundSetting)) {
							logger::WARN("  >Setting redefinition {}."sv, foundSetting);
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
							logger::WARN("  >Setting redefinition {}."sv, foundSetting);
							encounteredError = true;
						}

						floatSettings.emplace(foundSetting, value);
					}
					else if (settingType == "b") {
						if (boolSettings.contains(foundSetting)) {
							logger::WARN("  >Setting redefinition {}."sv, foundSetting);
							encounteredError = true;
						}

						boolSettings.emplace(foundSetting, ini.GetBoolValue(section.pItem, key.pItem));
					}
					else if (settingType == "i") {
						const long value = ini.GetLongValue(section.pItem, key.pItem);
						if (longSettings.contains(foundSetting)) {
							logger::WARN("  >Setting redefinition {}."sv, foundSetting);
							encounteredError = true;
						}

						longSettings.emplace(foundSetting, value);
					}
					else {
						logger::WARN("  >Invalid setting {}. Settings must be prefixed by s, f, b, or i."sv, foundSetting);
						encounteredError = true;
					}
				}
			}
		}
		catch (std::exception& e) {
			logger::WARN("Caught exception {} while fetching INI settings.", e.what());
			return false;
		}

		logger::INFO("  >Finished reading {} settings.", std::to_string(settingCount));

		if (encounteredError) {
			logger::INFO("Errors were encountered while reading the INI file. See log for more details."sv);
			return false;
		}

		OverrideSettings();
		DumpSettings();
		return true;
	}

	void Holder::DumpSettings()
	{
		logger::INFO("Stored Settings:"sv);
		for (const auto& [name, value] : boolSettings) {
			logger::INFO("  >{} - {}", name, value ? "TRUE" : "FALSE");
		}
		for (const auto& [name, value] : stringSettings) {
			logger::INFO("  >{} - {}", name, value);
		}
		for (const auto& [name, value] : longSettings) {
			logger::INFO("  >{} - {}", name, value);
		}
		for (const auto& [name, value] : floatSettings) {
			logger::INFO("  >{} - {}", name, value);
		}
	}

	bool Holder::OverrideSettings() {
		logger::INFO("Checking the custom INI..."sv);
		std::string iniPath = fmt::format(R"(.\Data\SKSE\Plugins\{}_custom.ini)"sv, Plugin::NAME);
		if (!std::filesystem::exists(iniPath)) {
			logger::INFO("  >Custom INI not found."sv);
			return true;
		}

		CSimpleIniA ini{};
		try {
			ini.SetUnicode();
			ini.LoadFile(iniPath.data());

			std::list<CSimpleIniA::Entry> sections{};
			ini.GetAllSections(sections);

			if (sections.empty()) {
				logger::WARN("  >Finished reading Custom INI file, but found no overrides.");
				return true;
			}

			for (const auto& section : sections) {
				std::list<CSimpleIniA::Entry> sectionKeys{};
				ini.GetAllKeys(section.pItem, sectionKeys);

				if (sectionKeys.empty()) {
					logger::WARN("  >Custom INI section {} has no settings.", section.pItem);
					continue;
				}

				for (const auto& key : sectionKeys) {
					const std::string foundSetting = fmt::format<std::string>("{}|{}"sv, section.pItem, key.pItem);
					const auto settingKeyName = std::string(key.pItem);
					if (settingKeyName.size() < 1) {
						logger::WARN("  >Invalid setting in section {}."sv, key.pItem, section.pItem);
						continue;
					}

					const auto settingType = settingKeyName.substr(0, 1);
					if (settingType == "s") {
						const std::string value = ini.GetValue(section.pItem, key.pItem);
						if (value.empty()) {
							logger::WARN("  >Invalid value in string setting {} in custom INI."sv, foundSetting);
							continue;
						}
						else if (!stringSettings.contains(foundSetting)) {
							logger::WARN("  >Setting {} not defined in the base INI."sv, foundSetting);
							continue;
						}

						logger::INFO("  >Overrode {} with {}."sv, foundSetting, value);
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
							logger::WARN("  >Setting {} not defined in the base INI."sv, foundSetting);
							continue;
						}

						logger::INFO("  >Overrode {} with {}."sv, foundSetting, std::to_string(value));
						floatSettings[foundSetting] = value;
					}
					else if (settingType == "b") {
						if (!boolSettings.contains(foundSetting)) {
							logger::WARN("  >Setting {} not defined in the base INI."sv, foundSetting);
							continue;
						}

						logger::INFO("  >Overrode {} with {}."sv, foundSetting, ini.GetBoolValue(section.pItem, key.pItem));
						boolSettings[foundSetting] = ini.GetBoolValue(section.pItem, key.pItem);
					}
					else if (settingType == "i") {
						const long value = ini.GetLongValue(section.pItem, key.pItem);
						if (!longSettings.contains(foundSetting)) {
							logger::WARN("  >Setting {} not defined in the base INI."sv, foundSetting);
							continue;
						}

						logger::INFO("  >Overrode {} with {}."sv, foundSetting, std::to_string(value));
						longSettings[foundSetting] = value;
					}
					else {
						logger::WARN("  >Invalid setting {}. Settings must be prefixed by s, f, b, or i."sv, foundSetting);
					}
				}
			}
		}
		catch (std::exception& e) {
			logger::WARN("  >Caught exception {} while reading the CUSTOM ini.", e.what());
			return false;
		}

		return true;
	}
}