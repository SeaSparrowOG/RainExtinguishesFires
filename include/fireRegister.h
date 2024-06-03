#pragma once

namespace CachedData {
#define _debugEDID clib_util::editorID::get_editorID

	class Fires : public ISingleton<Fires> {
	public:
		bool IsFireFrozen(RE::TESObjectREFR* a_fire);
		void FreezeFire(RE::TESObjectREFR* a_fire);
		void UnFreezeFire(RE::TESObjectREFR* a_fire);

		const FireData* GetFireData(RE::TESBoundObject* a_form);
		bool GetCheckLights();
		bool GetCheckSmoke();
		bool IsFireObject(RE::TESBoundObject* a_form);
		bool IsSmokeObject(RE::TESBoundObject* a_form);
		bool IsDynDOLODFire(RE::TESBoundObject* a_form);
		void RegisterPair(RE::TESBoundObject* a_litForm, FireData fireData);
		void RegisterSmokeObject(RE::TESBoundObject* a_smokeObject);
		void SetFireLookupRadius(double a_newValue);
		void SetLookupLight(double a_newValue);
		void SetLookupSmoke(double a_newValue);
		void SetReferenceLookupRadius(double a_newValue);
		void SetRequiredOffTime(double a_newValue);

	private:
		bool checkSmoke{ false };
		bool checkLight{ false };
		double fireLookupRadius{ 0.0 };
		double lookupLightRadius{ 0.0 };
		double lookupSmokeRadius{ 0.0 };
		double lookupREferenceRadius{ 0.0 };
		double requiredOffTime{ 0.0 };

		//Fire and smoke storage
		std::unordered_map<RE::TESBoundObject*, FireData> fireMap{};
		std::unordered_map<RE::TESObjectREFR*, bool> frozenFires{};
		std::vector<RE::TESBoundObject*> validFires{};
		std::vector<RE::TESBoundObject*> dyndolodFires{};
		std::vector<RE::TESBoundObject*> smokeVector{};
	};
}