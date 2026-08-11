#include "papyrus.h"

#include "Events/CellEvent/CellListener.h"
#include "Hooks/Hooks.h"

namespace Papyrus
{
	static std::vector<int> GetVersion(STATIC_ARGS) {
		return { Plugin::VERSION[0], Plugin::VERSION[1], Plugin::VERSION[2] };
	}

	static bool IsRaining(STATIC_ARGS) {
		auto* sky = RE::Sky::GetSingleton();
		if (!sky) {
			a_vm->TraceStack("[IsRaining]: Failed to get internal Sky singleton.", 
				a_stackID, 
				RE::BSScript::IVirtualMachine::Severity::kWarning);
			return false;
		}
		return sky->IsRaining() || sky->IsSnowing();
	}

	static void ExtinguishAllLoadedFires(STATIC_ARGS) {

	}

	static bool RegisterForAllEvents(STATIC_ARGS, RE::TESForm* a_form) {
		if (!a_form) {
			a_vm->TraceStack("[RegisterForAllEvents]: Cannot call with a NONE form.", a_stackID, RE::BSScript::IVirtualMachine::Severity::kWarning);
			return false;
		}
		auto* actorCellEvent = Events::CellEvent::ActorCellEventListener::GetSingleton();
		auto* weatherEvent = Hooks::WeatherManager::GetSingleton();
		if (!actorCellEvent || !weatherEvent) {
			a_vm->TraceStack("[RegisterForAllEvents]: Failed to get internal event listeners.", a_stackID, RE::BSScript::IVirtualMachine::Severity::kWarning);
			return false;
		}

		actorCellEvent->RegisterFormForEvents(a_form);
		weatherEvent->RegisterFormForEvents(a_form);
		return true;
	}

	static void Bind(VM& a_vm) {
		logger::info("  >Binding GetVersion..."sv);
		BIND(GetVersion);
		logger::info("  >Binding IsRaining..."sv);
		BIND(IsRaining);
		logger::info("  >Binding ExtinguishAllLoadedFires..."sv);
		BIND(ExtinguishAllLoadedFires);
		logger::info("  >Binding RegisterForAllEvents..."sv);
		BIND(RegisterForAllEvents);
	}

	bool RegisterFunctions(VM* a_vm) {
		logger::info("Binding papyrus functions in utility script {}..."sv, script);
		Bind(*a_vm);
		logger::info("Finished binding functions."sv);
		return true;
	}
}
