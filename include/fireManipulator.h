#pragma once

namespace FireManipulator {
#define continueContainer RE::BSContainer::ForEachResult::kContinue

	class Manager : public ISingleton<Manager> {
	public:
		void FreezeReference(RE::TESObjectREFR* a_ref);
		void UnFreezeReference(RE::TESObjectREFR* a_ref);
		bool IsRefFrozen(RE::TESObjectREFR* a_ref);

		void ExtinguishAllFires();
		void ExtinguishFire(RE::TESObjectREFR* a_fire, const FireData* a_data, std::string_view a_mode);
		void RelightFire(RE::TESObjectREFR* a_fire);
		std::vector<RE::TESObjectREFR*> GetNearbyAssociatedReferences(RE::TESObjectREFR* a_center, const FireData* a_data);

	private:
		std::unordered_map<RE::TESObjectREFR*, bool> frozenRefs;
	};
}