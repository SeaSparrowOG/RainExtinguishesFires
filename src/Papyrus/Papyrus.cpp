#include "papyrus.h"

#include "FireManipulation/Manipulator.h"

namespace Papyrus
{
	static std::vector<int> GetVersion(STATIC_ARGS) {
		return { Plugin::VERSION[0], Plugin::VERSION[1], Plugin::VERSION[2] };
	}

	static bool IsRaining(STATIC_ARGS) {
		auto* sky = RE::Sky::GetSingleton();
		if (!sky) {
			a_vm->TraceStack("[IsRaining]: Failed to get the game's Sky singleton.", 
				a_stackID, 
				RE::BSScript::IVirtualMachine::Severity::kWarning);
			return false;
		}
		return sky->IsRaining() || sky->IsSnowing();
	}

	static void UnFreezeFire(STATIC_ARGS, RE::TESObjectREFR* ref) {
		static auto* manipulator = FireManipulator::Manipulator::GetSingleton();
		if (!manipulator) {
			a_vm->TraceStack("[UnFreezeFire]: Failed to get Fire Manipulator.",
				a_stackID,
				RE::BSScript::IVirtualMachine::Severity::kWarning);
			return;
		}
		if (!ref) {
			a_vm->TraceStack("[UnFreezeFire]: Cannot call this function with a NONE reference.",
				a_stackID,
				RE::BSScript::IVirtualMachine::Severity::kWarning);
			return;
		}
		manipulator->UnFreezeReference(ref);
	}

	static void FreezeFire(STATIC_ARGS, RE::TESObjectREFR* ref) {
		static auto* manipulator = FireManipulator::Manipulator::GetSingleton();
		if (!manipulator) {
			a_vm->TraceStack("[FreezeFire]: Failed to get Fire Manipulator.",
				a_stackID,
				RE::BSScript::IVirtualMachine::Severity::kWarning);
			return;
		}
		if (!ref) {
			a_vm->TraceStack("[FreezeFire]: Cannot call this function with a NONE reference.",
				a_stackID,
				RE::BSScript::IVirtualMachine::Severity::kWarning);
			return;
		}
		manipulator->FreezeReference(ref);
	}

	static void Bind(VM& a_vm) {
		logger::info("  >Binding GetVersion..."sv);
		BIND(GetVersion);
		logger::info("  >Binding IsRaining..."sv);
		BIND(IsRaining);
		logger::info("  >Binding ExtinguishAllLoadedFires..."sv);
		BIND(UnFreezeFire);
		logger::info("  >Binding RegisterForAllEvents..."sv);
		BIND(FreezeFire);
	}

	bool RegisterFunctions(VM* a_vm) {
		logger::info("Binding papyrus functions in utility script {}..."sv, script);
		Bind(*a_vm);
		logger::info("Finished binding functions."sv);
		return true;
	}
}
