#pragma once

namespace CachedData {
#define _debugEDID clib_util::editorID::get_editorID

	enum Setting {
		kLightEnabled,
		kSmokeEnabled,
		kLightRadius,
		kSmokeRadius,
		kReferenceRadius, 
		kResetTime
	};

	class Fires : public ISingleton<Fires> {
	public:
		const FireData* GetFireData(RE::TESBoundObject* a_form);
		bool IsFireObject(RE::TESBoundObject* a_form);
		bool IsLitFire(RE::TESBoundObject* a_form);
		bool IsUnLitFire(RE::TESBoundObject* a_form);
		bool IsSmokeObject(RE::TESBoundObject* a_form);
		bool IsDynDOLODFire(RE::TESBoundObject* a_form);
		void RegisterPair(RE::TESBoundObject* a_litForm, FireData fireData);
		void RegisterSmokeObject(RE::TESBoundObject* a_litForm);
		void UpdateSetting(Setting a_setting, bool a_settingBool = false, double a_settingDouble = 0.0);

		void Report();

	private:
		bool checkSmoke{ false };
		bool checkLight{ false };
		double lookupReferenceRadius{ 0.0 };
		double lookupLightRadius{ 0.0 };
		double lookupSmokeRadius{ 0.0 };
		double requiredOffTime{ 0.0 };

		//Fire and smoke storage
		std::unordered_map<RE::TESBoundObject*, FireData> fireDataMap{};

		std::vector<RE::TESBoundObject*> validFires{};
		std::vector<RE::TESBoundObject*> validOffFires{};
		std::vector<RE::TESBoundObject*> dyndolodFires{};
		std::vector<RE::TESBoundObject*> smokeVector{};
	};
}