#pragma once

namespace CachedData {
#define _debugEDID clib_util::editorID::get_editorID

	class Fires : public ISingleton<Fires> {
	public:
		void RegisterPair(RE::TESForm* a_litForm, FireData fireData);
		void RegisterSmokeObject(RE::TESForm* a_smokeObject);
		void SetFireLookupRadius(double a_newValue);
		void SetLookupLight(double a_newValue);
		void SetLookupSmoke(double a_newValue);
		void SetReferenceLookupRadius(double a_newValue);
		void SetRequiredOffTime(double a_newValue);

	private:
		double fireLookupRadius{ 0.0 };
		double lookupLightRadius{ 0.0 };
		double lookupSmokeRadius{ 0.0 };
		double lookupREferenceRadius{ 0.0 };
		double requiredOffTime{ 0.0 };

		//Fire and smoke storage
		std::unordered_map<RE::TESForm*, FireData> fireMap{};
		std::vector<RE::TESForm*> smokeVector{};
	};
}