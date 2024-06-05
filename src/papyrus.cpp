#include "papyrus.h"
#include "eventListener.h"
#include "fireManipulator.h"

namespace Papyrus {
	std::vector<int> GetVersion(STATIC_ARGS) {
		std::vector<int> response{};
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

	void ExtinguishAllLoadedFires(STATIC_ARGS) {
		FireManipulator::Manager::GetSingleton()->ExtinguishAllFires();
	}

	void SetRainingFlag(STATIC_ARGS, bool a_isRaining) {
		Events::Weather::WeatherEventManager::GetSingleton()->SetRainingFlag(a_isRaining);
	}

	void FreezeFire(STATIC_ARGS, RE::TESObjectREFR* a_ref) {
		FireManipulator::Manager::GetSingleton()->FreezeReference(a_ref);
	}

	void UnFreezeFire(STATIC_ARGS, RE::TESObjectREFR* a_ref) {
		FireManipulator::Manager::GetSingleton()->UnFreezeReference(a_ref);
	}

	void Bind(VM& a_vm) {
		BIND(GetVersion);
		BIND(ExtinguishAllLoadedFires);
		BIND(FreezeFire);
		BIND(UnFreezeFire);
		BIND(SetRainingFlag);
		BIND_EVENT(RegisterForAccurateWeatherChange, true);
		BIND_EVENT(UnRegisterForAccurateWeatherChange, true);
	}

	bool RegisterFunctions(VM* a_vm) {
		Bind(*a_vm);
		return true;
	}
}