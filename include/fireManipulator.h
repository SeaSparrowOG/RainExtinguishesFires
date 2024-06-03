#pragma once

namespace FireManipulator {
	void ExtinguishFire(RE::TESObjectREFR* a_fire, const FireData* a_data, bool ignoreConditions = false);
	void RelightFire(RE::TESObjectREFR* a_fire, const FireData* a_data, bool ignoreConditions = false);
}