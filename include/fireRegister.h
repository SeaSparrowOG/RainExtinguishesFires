#pragma once

namespace FireRegistry {
	//Might seem redundant. More information will be made available here.
	struct offFire {
		bool                      dyndolodFire;
		RE::TESForm*              offVersion;

		offFire() {
			this->offVersion = nullptr;
			this->dyndolodFire = false;
		}
	};

	class FireRegistry : public ISingleton<FireRegistry> {
	public:
		bool                      BuildRegistry();
		bool                      IsValidLocation();
		bool                      IsValidSmoke(RE::TESForm* a_smoke);
		bool                      GetCheckOcclusion();
		bool                      GetCheckLights();
		bool                      GetCheckSmoke();
		offFire                   GetMatch(RE::TESForm* a_litFire);
		RE::TESForm*              GetOffMatch(RE::TESForm* a_offFire);
		std::vector<RE::TESForm*> GetStoredSmokes();
		bool                      RegisterPair(RE::TESForm* a_lit, offFire a_off);
		bool                      RegisterReversePair(RE::TESForm* a_off, RE::TESForm* a_lit);
		bool                      RegisterSmokeObject(RE::TESForm* a_smoke);
		void                      SetLookupSmoke(bool a_value);
		void                      SetLookupLight(bool a_value);
	private:
		bool                                           calculateOcclusion;
		bool                                           lookupLights;
		bool                                           lookupSmoke;
		std::unordered_map<RE::TESForm*, offFire>      fireRegister;
		std::unordered_map<RE::TESForm*, RE::TESForm*> reverseRegister;
		std::unordered_map<RE::TESForm*, bool>         smokeRegister;
	};
}