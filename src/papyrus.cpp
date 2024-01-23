#include "papyrus.h"
#include "eventDispenser.h"

namespace Papyrus {
	std::vector<int> GetVersion(STATIC_ARGS) {
		std::vector<int> response;
		response.push_back(Version::MAJOR);
		response.push_back(Version::MINOR);
		response.push_back(Version::PATCH);

		return response;
	}

	void RegisterForAccurateWeatherChange(STATIC_ARGS, const RE::TESForm* a_form) {
		if (!a_form) return;
		Events::Papyrus::GetSingleton()->AddWeatherChangeListener(a_form, true);
	}

	void UnRegisterForAccurateWeatherChange(STATIC_ARGS, const RE::TESForm* a_form) {
		if (!a_form) return;
		Events::Papyrus::GetSingleton()->AddWeatherChangeListener(a_form, false);
	}

	void RegisterForPlayerCellChangeEvent(STATIC_ARGS, const RE::TESForm* a_form) {
		if (!a_form) return;
		Events::Papyrus::GetSingleton()->AddInteriorExteriorListener(a_form, true);
	}

	void UnRegisterForPlayerCellChangeEvent(STATIC_ARGS, const RE::TESForm* a_form) {
		if (!a_form) return;
		Events::Papyrus::GetSingleton()->AddInteriorExteriorListener(a_form, false);
	}

	void ExtinguishAllLoadedFires(STATIC_ARGS) {
		Events::Papyrus::GetSingleton()->ExtinguishAllFires();
	}

	void SetRainingFlag(STATIC_ARGS, bool a_isRaining) {
		Events::Papyrus::GetSingleton()->SetIsRaining(a_isRaining);
	}

	bool FreezeFire(STATIC_ARGS, RE::TESObjectREFR* a_fire) {
		return Events::Papyrus::GetSingleton()->ManipulateFireRegistry(a_fire, true);
	}

	bool UnFreezeFire(STATIC_ARGS, RE::TESObjectREFR* a_fire) {
		return Events::Papyrus::GetSingleton()->ManipulateFireRegistry(a_fire, false);
	}

	void Bind(VM& a_vm) {
		BIND(GetVersion);
		BIND(ExtinguishAllLoadedFires);
		BIND(FreezeFire);
		BIND(UnFreezeFire);
		BIND(SetRainingFlag);
		BIND_EVENT(RegisterForAccurateWeatherChange, true);
		BIND_EVENT(RegisterForPlayerCellChangeEvent, true);
		BIND_EVENT(UnRegisterForAccurateWeatherChange, true);
		BIND_EVENT(UnRegisterForPlayerCellChangeEvent, true);
	}

	bool RegisterFunctions(VM* a_vm) {
		Bind(*a_vm);
		return true;
	}
}