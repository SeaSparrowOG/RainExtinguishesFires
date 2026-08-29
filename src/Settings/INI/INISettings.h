#pragma once

namespace Settings
{
	namespace INI
	{
		bool Read();

		class Holder :
			public REX::TSingleton<Holder>
		{
		public:
			bool StoreSettings();
			void DumpSettings();

			template <typename T>
			std::optional<T> GetStoredSetting(const std::string& a_settingName) {
				if constexpr (std::is_same_v<T, float>) {
					auto it = floatSettings.find(a_settingName);
					if (it != floatSettings.end()) return it->second;
				}
				else if constexpr (std::is_same_v<T, std::string>) {
					auto it = stringSettings.find(a_settingName);
					if (it != stringSettings.end()) return it->second;
				}
				else if constexpr (std::is_same_v<T, long>) {
					auto it = longSettings.find(a_settingName);
					if (it != longSettings.end()) return it->second;
				}
				else if constexpr (std::is_same_v<T, bool>) {
					auto it = boolSettings.find(a_settingName);
					if (it != boolSettings.end()) return it->second;
				}
				else {
					static_assert(always_false<T>, "Called GetStoredSetting with unsupported type.");
				}
				return std::nullopt;
			}

		private:
			std::map<std::string, long>        longSettings;
			std::map<std::string, bool>        boolSettings;
			std::map<std::string, float>       floatSettings;
			std::map<std::string, std::string> stringSettings;

			bool OverrideSettings();
		};

		inline static constexpr std::string_view GENERAL_SQUASH_LIGHTS = "General|bSquashLights"sv;
		inline static constexpr std::string_view GENERAL_SQUASH_SMOKE = "General|bSquashSmoke"sv;
		inline static constexpr std::string_view GENERAL_CHECK_OCCLUSION = "General|bCheckOcclusion"sv;
		inline static constexpr std::string_view GENERAL_LOOKUP_REFERENCE = "General|fReferenceLookupRadius"sv;
		inline static constexpr std::string_view GENERAL_LOOKUP_LIGHT = "General|fLightLookupRadius"sv;
		inline static constexpr std::string_view GENERAL_LOOKUP_SMOKE = "General|fSmokeLookupRadius"sv;
		inline static constexpr std::string_view GENERAL_RESET_DAYS = "General|fDaysToReset"sv;


		inline static constexpr const std::uint8_t EXPECTED_COUNT = 7;
		inline static constexpr const std::array<std::string_view, EXPECTED_COUNT> EXPECTED_SETTINGS = {
			GENERAL_SQUASH_LIGHTS,
			GENERAL_SQUASH_SMOKE,
			GENERAL_CHECK_OCCLUSION,
			GENERAL_LOOKUP_REFERENCE,
			GENERAL_LOOKUP_LIGHT,
			GENERAL_LOOKUP_SMOKE,
			GENERAL_RESET_DAYS
		};

		template <typename T>
		std::optional<T> GetSetting(const std::string& a_settingName) {
			static auto* holder = Holder::GetSingleton();
			return holder->GetStoredSetting<T>(a_settingName);
		}
	}
}