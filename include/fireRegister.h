#pragma once

namespace CachedData {
	struct FireData {
		bool         dyndolodFire;
		RE::TESForm* offVersion;
	};

	/**
	* FireRegistry holds settings and valid fires.
	*/
	class FireRegistry : public ISingleton<FireRegistry> {
	public:
		bool                      BuildRegistry();
		bool                      IsValidSmoke(RE::TESForm* a_smoke);
		bool                      IsManagedFire(RE::TESForm* a_litFire);
		bool                      IsDynDOLODFire(RE::TESForm* a_litFire);
		bool                      GetCheckOcclusion();
		bool                      GetCheckLights();
		bool                      GetCheckSmoke();
		FireData                  GetOffForm(RE::TESForm* a_litFire);
		std::vector<RE::TESForm*> GetStoredSmokeObjects();
		float                     GetSmokeSearchDistance();
		float                     GetLightSearchDistance();
		float                     GetRequiredOffTime();
		bool                      RegisterPair(RE::TESForm* a_lit, FireData a_fireData);
		bool                      RegisterSmokeObject(RE::TESForm* a_smoke);
		void                      SetLookupSmoke(bool a_value);
		void                      SetLookupLight(bool a_value);
		void                      SetLookupSmokeDistance(float a_value);
		void                      SetLookupLightDistance(float a_value);
		void                      SetRequiredOffTime(float a_value);

	private:
		bool                                       calculateOcclusion;
		bool                                       lookupLights;
		bool                                       lookupSmoke;
		float                                      lightLookupRadius;
		float                                      requiredOffTime;
		float                                      smokeLookupRadius;
		std::vector<RE::TESForm*>                  storedSmoke;
		std::unordered_map<RE::TESForm*, bool>     smokeRegister;
		std::unordered_map<RE::TESForm*, bool>     reverseFireRegister;
		std::unordered_map<RE::TESForm*, FireData> fireRegister;
	};
}