#pragma once

namespace CachedData {
	struct FireData {
		bool         dyndolodFire;
		RE::TESForm* offVersion;
		RE::TESForm* dyndolodVersion;

		FireData() {
			this->dyndolodFire = false;
			this->dyndolodVersion = nullptr;
			this->offVersion = nullptr;
		}
	};

	/**
	* FireRegistry holds settings and valid fires.
	*/
	class FireRegistry : public ISingleton<FireRegistry> {
	public:
		bool                      BuildRegistry();
		bool                      IsOffFire(RE::TESForm* a_offForm);
		bool                      IsOnFire(RE::TESForm* a_offForm);
		bool                      IsDynDOLODFire(RE::TESForm* a_litFire);
		bool                      IsSmokeObject(RE::TESForm* a_smoke);
		bool                      IsValidObject(RE::TESForm* a_form);
		bool                      GetCheckOcclusion();
		bool                      GetCheckLights();
		bool                      GetCheckSmoke();
		FireData                  GetOffForm(RE::TESForm* a_litFire);
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
		std::unordered_map<RE::TESForm*, bool>     allStoredObjects;
		std::unordered_map<RE::TESForm*, bool>     smokeRegister;
		std::unordered_map<RE::TESForm*, bool>     dyndolodFireRegister;
		std::unordered_map<RE::TESForm*, bool>     reverseFireRegister;
		std::unordered_map<RE::TESForm*, FireData> fireRegister;
	};
}