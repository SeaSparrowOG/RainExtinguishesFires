#pragma once

namespace FireManipulator {
	class Manager : public ISingleton<Manager> {
	public:
		void FreezeReference(RE::TESObjectREFR* a_ref);
		void UnFreezeReference(RE::TESObjectREFR* a_ref);
		void ExtinguishFire(RE::TESObjectREFR* a_fire, const FireData* a_data);
		void RelightFire(RE::TESObjectREFR* a_fire, const FireData* a_data);

	private:
		std::unordered_map<RE::TESObjectREFR*, bool> frozenRefs;
	};
}