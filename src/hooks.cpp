#include "hooks.h"
#include "eventDispenser.h"
#include "papyrus.h"

namespace Hooks {
	void QueueWeatherChange(RE::TESWeather* a_currentWeather) { 
		Events::Papyrus::GetSingleton()->SendWeatherChange(a_currentWeather);
	}
	void Install() {
		REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(25684, 26231), OFFSET(0x44F, 0x46C) };
		//Note: write_thunk_call is defined in the PCH.h in the Include directory.
		stl::write_thunk_call<WeatherChange>(target.address());
	}
}