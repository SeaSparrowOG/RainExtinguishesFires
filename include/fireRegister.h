#pragma once

namespace FireRegistry {
	//Might seem redundant. More information will be made available here.
	struct offFire {
		RE::TESForm*      offVersion;
		bool              dyndolodFire;

		offFire() {
			this->offVersion = nullptr;
			this->dyndolodFire = false;
		}
	};

	class FireRegistry : public ISingleton<FireRegistry> {
	public:
		bool         BuildRegistry();
		bool         IsValidLocation();
		offFire      GetMatch(RE::TESForm* a_litFire);
		RE::TESForm* GetOffMatch(RE::TESForm* a_offFire);
		bool         RegisterPair(RE::TESForm* a_lit, offFire a_off);
		bool         RegisterReversePair(RE::TESForm* a_off, RE::TESForm* a_lit);
	private:
		std::unordered_map<RE::TESForm*, offFire>      fireRegister;
		std::unordered_map<RE::TESForm*, RE::TESForm*> reverseRegister;
	};
}