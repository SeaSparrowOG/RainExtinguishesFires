#include "fireRegister.h"
#include "settingsReader.h"

namespace FireRegistry {
	bool FireRegistry::IsValidLocation() {
		bool isInInterior = RE::PlayerCharacter::GetSingleton()->GetParentCell()->IsInteriorCell();
		if (isInInterior) return false;
		return true;
	}

	offFire FireRegistry::GetMatch(RE::TESForm* a_litFire) {
		if (this->fireRegister.find(a_litFire) != this->fireRegister.end()) {
			return this->fireRegister.at(a_litFire);
		}
		return offFire();
	}

	RE::TESForm* FireRegistry::GetOffMatch(RE::TESForm* a_offFire) {
		if (this->reverseRegister.find(a_offFire) != this->reverseRegister.end()) {
			return this->reverseRegister.at(a_offFire);
		}
		return nullptr;
	}

	bool FireRegistry::RegisterPair(RE::TESForm* a_lit, offFire a_off) {
		this->fireRegister[a_lit] = a_off;
		return true;
	}

	bool FireRegistry::RegisterReversePair(RE::TESForm* a_off, RE::TESForm* a_lit) {
		this->reverseRegister[a_off] = a_lit;
		return true;
	}

	bool FireRegistry::BuildRegistry() {
		Settings::Settings::GetSingleton()->ReadSettings();
		return true;
	}
}