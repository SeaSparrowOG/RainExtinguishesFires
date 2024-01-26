#include "hooks.h"
#include "eventDispenser.h"
#include "papyrus.h"

namespace Hooks {
	void QueueWeatherChange(RE::TESWeather* a_currentWeather) { 
		Events::Papyrus::GetSingleton()->SendWeatherChange(a_currentWeather);
	}
	void Install() {
#ifdef BUILD_NG
		REL::Relocation<std::uintptr_t> target{ REL::RelocationID(25684, 26231), REL::VariantOffset(0x44F, 0x46C, 0x0) };
#else 
		REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(25684, 26231), OFFSET(0x44F, 0x46C) };
#endif
		//Note: write_thunk_call is defined in the PCH.h in the Include directory.
		stl::write_thunk_call<WeatherChange>(target.address());
	}
}