#include "eventListener.h"
#include "fireManipulator.h"
#include "fireRegister.h"

namespace HitEventHelper {
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
}

namespace Events {

	bool ActorCellManager::RegisterListener() {
		RE::PlayerCharacter::GetSingleton()->AddEventSink(this);
		return true;
	}

	bool RegisterForEvents() {
		bool success = true;
		if (!HitEvenetManager::GetSingleton()->RegisterListener()) success = false;
		if (success && !LoadEventManager::GetSingleton()->RegisterListener()) success = false;
		if (success && !ActorCellManager::GetSingleton()->RegisterListener()) success = false;

		if (!success) {
			HitEvenetManager::GetSingleton()->UnregisterListener();
			LoadEventManager::GetSingleton()->UnregisterListener();
		}
		return success;
	}

	RE::BSEventNotifyControl HitEvenetManager::ProcessEvent(const RE::TESHitEvent* a_event, RE::BSTEventSource<RE::TESHitEvent>* a_eventSource) {
		if (!a_event || !a_eventSource) return continueEvent;

		auto hitRefPtr = a_event->target;
		auto* hitRef = hitRefPtr ? hitRefPtr.get() : nullptr;
		auto* hitBound = hitRef ? hitRef->GetBaseObject() : nullptr;
		if (!hitBound) return continueEvent;
		if (!CachedData::Fires::GetSingleton()->IsFireObject(hitBound)) return continueEvent;

		auto hitSource = a_event->source;
		auto* hitForm = hitSource ? RE::TESForm::LookupByID(hitSource) : nullptr;
		if (!hitForm) return continueEvent;

		RE::TESObjectWEAP* hitWeap = hitForm->As<RE::TESObjectWEAP>();
		RE::SpellItem* hitSpell = hitForm->As<RE::SpellItem>();
		if (!(hitWeap || hitSpell)) return continueEvent;

		bool needsFire = true;
		auto* fireData = CachedData::Fires::GetSingleton()->GetFireData(hitBound);
		if (!fireData) needsFire = false;

		bool goodToGo = false;
		if (hitWeap && HitEventHelper::ValidWeaponHit(hitWeap, needsFire)) goodToGo = true;
		if (!goodToGo && hitSpell && HitEventHelper::ValidSpellHit(hitSpell, needsFire)) goodToGo = true;
		if (!goodToGo) return continueEvent;

		if (needsFire) {
			FireManipulator::RelightFire(hitRef, fireData);
		}
		else {
			FireManipulator::ExtinguishFire(hitRef, fireData);
		}
		return continueEvent;
	}

	RE::BSEventNotifyControl LoadEventManager::ProcessEvent(const RE::TESCellAttachDetachEvent* a_event, RE::BSTEventSource<RE::TESCellAttachDetachEvent>* a_eventSource) {
		if (!a_event || !a_eventSource) return continueEvent;
		if (!a_event->attached) return continueEvent;

		auto* eventReference = a_event->reference.get();
		auto* eventBaseForm = eventReference ? eventReference->GetBaseObject() : nullptr;
		if (!eventBaseForm) return continueEvent;
		if (!CachedData::Fires::GetSingleton()->IsFireObject(eventBaseForm)) return continueEvent;

		auto* fireData = CachedData::Fires::GetSingleton()->GetFireData(eventBaseForm);
		if (!fireData) return continueEvent;


		if (RainEventManager::GetSingleton()->IsRaining()) {
			FireManipulator::ExtinguishFire(eventReference, fireData);
		}
		else {
			FireManipulator::RelightFire(eventReference, fireData);
		}
		return continueEvent;
	}

	RE::BSEventNotifyControl ActorCellManager::ProcessEvent(const RE::BGSActorCellEvent* a_event, RE::BSTEventSource<RE::BGSActorCellEvent>* a_eventSource) {
		if (!a_event || !a_eventSource) return continueEvent;
		return continueEvent;
	}

	bool RainEventManager::IsRaining() {
		return this->isRaining;
	}
}