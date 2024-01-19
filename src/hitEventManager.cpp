#include "hitManager.h"
#include "fireRegister.h"
#include "papyrus.h"

namespace HitManager {
	bool ValidWeaponHit(RE::TESObjectWEAP* a_weap, bool a_needsFire) {
		auto* weaponEnchant = a_weap->formEnchanting;
		if (!weaponEnchant) return false;

		auto effects = weaponEnchant->effects;
		if (effects.empty()) return false;

		bool hasFire = false;
		bool hasFrost = false;

		for (auto* effect : effects) {
			auto* baseEffect = effect->baseEffect;
			if (!baseEffect) continue;

			if (baseEffect->HasKeywordString("MagicDamageFire")) hasFire = true;
			if (baseEffect->HasKeywordString("MagicDamageFrost")) hasFrost = true;
			if (hasFire && hasFrost) return false;
		}

		if (a_needsFire && hasFire) return true;
		if (!a_needsFire && hasFrost) return true;
		return false;
	}

	bool ValidSpellHit(RE::SpellItem* a_spell, bool a_needsFire) {
		auto effects = a_spell->effects;
		if (effects.empty()) return false;

		bool hasFire = false;
		bool hasFrost = false;

		for (auto* effect : effects) {
			auto* baseEffect = effect->baseEffect;
			if (!baseEffect) continue;
			if (baseEffect->HasKeywordString("MagicDamageFire")) hasFire = true;
			if (baseEffect->HasKeywordString("MagicDamageFrost")) hasFrost = true;
			if (hasFire && hasFrost) return false;
		}

		if (a_needsFire && hasFire) return true;
		if (!a_needsFire && hasFrost) return true;
		return false;
	}

	bool HitManager::RegisterListener() {
		auto* singleton = HitManager::GetSingleton();
		if (!singleton) return false;

		RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink(singleton);
		return true;
	}

	bool HitManager::HitManager::UnRegisterListener() {
		auto* singleton = HitManager::GetSingleton();
		if (!singleton) return false;

		RE::ScriptEventSourceHolder::GetSingleton()->RemoveEventSink(singleton);
		return true;
	}

	RE::BSEventNotifyControl HitManager::ProcessEvent(const RE::TESHitEvent* a_event, RE::BSTEventSource<RE::TESHitEvent>* a_eventSource) {
		if (!(a_event && a_eventSource)) return RE::BSEventNotifyControl::kContinue;

		auto hitRefPtr = a_event->target;
		auto* hitRef = hitRefPtr ? hitRefPtr.get() : nullptr;
		auto* hitBound = hitRef ? hitRef->GetBaseObject() : nullptr;
		auto* hitBase = hitBound ? hitBound->As<RE::TESForm>() : nullptr;
		if (!hitBase) return RE::BSEventNotifyControl::kContinue;

		auto hitSource = a_event->source;
		auto* hitForm = hitSource ? RE::TESForm::LookupByID(hitSource) : nullptr;
		if (!hitForm) return RE::BSEventNotifyControl::kContinue;

		RE::TESObjectWEAP* hitWeap = hitForm->As<RE::TESObjectWEAP>();
		RE::SpellItem* hitSpell = hitForm->As<RE::SpellItem>();
		if (!(hitWeap || hitSpell)) return RE::BSEventNotifyControl::kContinue;

		RE::TESForm* resultingFire = nullptr;
		bool needsFire = false;
		auto fireData = CachedData::FireRegistry::GetSingleton()->GetOffForm(hitBase);
		resultingFire = fireData.offVersion;
		if (resultingFire) needsFire = true;

		bool goodToGo = false;
		if (hitWeap && ValidWeaponHit(hitWeap, needsFire)) goodToGo = true;
		if (!goodToGo && hitSpell && ValidSpellHit(hitSpell, needsFire)) goodToGo = true;
		if (!goodToGo) {
			return RE::BSEventNotifyControl::kContinue;
		}

		if (needsFire) {
			Papyrus::Papyrus::GetSingleton()->RelightFire(hitRef);
		}
		else {
			Papyrus::Papyrus::GetSingleton()->ExtinguishFire(hitRef, fireData);
		}
		return RE::BSEventNotifyControl::kContinue;
	}
}