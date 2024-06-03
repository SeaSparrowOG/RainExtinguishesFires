#include "papyrus.h"

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
	}

	void UnRegisterForAccurateWeatherChange(STATIC_ARGS, const RE::TESForm* a_form) {
		if (!a_form) return;
	}

	void RegisterForPlayerCellChangeEvent(STATIC_ARGS, const RE::TESForm* a_form) {
		if (!a_form) return;
	}

	void UnRegisterForPlayerCellChangeEvent(STATIC_ARGS, const RE::TESForm* a_form) {
		if (!a_form) return;
	}

	void ExtinguishAllLoadedFires(STATIC_ARGS) {
	}

	void SetRainingFlag(STATIC_ARGS, bool a_isRaining) {
	}

	bool FreezeFire(STATIC_ARGS, RE::TESObjectREFR* a_fire) {
	}

	bool UnFreezeFire(STATIC_ARGS, RE::TESObjectREFR* a_fire) {
	}

	bool FreezeObject(STATIC_ARGS, RE::TESObjectREFR* a_ref) {
	}

	bool UnFreezeObject(STATIC_ARGS, RE::TESObjectREFR* a_ref) {
	}

	void Bind(VM& a_vm) {
		BIND(GetVersion);
		BIND(ExtinguishAllLoadedFires);
		BIND(FreezeFire);
		BIND(UnFreezeFire);
		BIND(FreezeObject);
		BIND(UnFreezeObject);
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