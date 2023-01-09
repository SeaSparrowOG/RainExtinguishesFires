#include "papyrusManager.h"

namespace PapyrusManager {
	bool Bind(VM* a_vm)
	{

		BIND(GetAmmoEnchantment);
        BIND(IsCrossbow);
		return true;
	}

    bool IsCrossbow(VM*, StackID a_StackID, RE::StaticFunctionTag*, RE::TESObjectWEAP* a_akWeapon)
    {
        if (!a_akWeapon) {

            return false;
        }

        if (a_akWeapon->IsCrossbow()) {

            return true;
        }

        return false;
    }

	RE::EnchantmentItem* GetAmmoEnchantment(VM*, StackID a_StackID, RE::StaticFunctionTag*, RE::Actor* a_akAggressor)
	{
        if (!a_akAggressor) {

            return nullptr;
        }

        const auto process = a_akAggressor->GetActorRuntimeData().currentProcess;
        const auto middleHigh = process ? process->middleHigh : nullptr;
        const auto bothHands = middleHigh ? middleHigh->bothHands : nullptr;
        if (bothHands && bothHands->object) {
            if (const auto ammo = bothHands->object->As<RE::TESAmmo>()) {
                const auto projectile = ammo ? ammo->GetRuntimeData().data.projectile : nullptr;
                const auto explosion = projectile ? projectile->data.explosionType : nullptr;
                auto enchantment = explosion ? explosion->formEnchanting : nullptr;
                
                if (const auto& extraLists = bothHands->extraLists) {
                    for (const auto& extraList : *extraLists) {
                        const auto exEnch = extraList->GetByType<RE::ExtraEnchantment>();
                        if (exEnch && exEnch->enchantment) {
                            enchantment = exEnch->enchantment;
                        }
                    }
                }

                return enchantment;
            }
        }

        return nullptr;
	}
}