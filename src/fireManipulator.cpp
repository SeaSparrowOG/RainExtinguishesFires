#include "fireManipulator.h"

namespace FireManipulator {
	void Manager::FreezeReference(RE::TESObjectREFR* a_ref) {
		if (this->frozenRefs.contains(a_ref)) return;
		this->frozenRefs[a_ref] = true;
	}

	void Manager::UnFreezeReference(RE::TESObjectREFR* a_ref) {
		if (!this->frozenRefs.contains(a_ref)) return;
		this->frozenRefs.erase(a_ref);
	}

	void Manager::ExtinguishAllFires() {
	}

	void Manager::ExtinguishFire(RE::TESObjectREFR* a_fire, const FireData* a_data) {
	}

	void Manager::RelightFire(RE::TESObjectREFR* a_fire) {
	}
}
