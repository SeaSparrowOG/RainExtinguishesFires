#pragma once

namespace PapyrusManager {

#define BIND(a_method, ...) a_vm->RegisterFunction(#a_method##sv, script, a_method __VA_OPT__(, ) __VA_ARGS__)
    using VM = RE::BSScript::Internal::VirtualMachine;
    using StackID = RE::VMStackID;
    inline constexpr auto script = "REF_UtilityFunctions"sv;

    bool              Bind(VM* a_vm);
    bool              IsCrossbow(VM*, StackID a_StackID, RE::StaticFunctionTag*, RE::TESObjectWEAP* a_akWeapon);
    RE::EnchantmentItem* GetAmmoEnchantment(VM*, StackID a_StackID, RE::StaticFunctionTag*, RE::Actor* a_akAggressor);
}