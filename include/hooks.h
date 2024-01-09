#pragma once

namespace Hooks {
	void QueueWeatherChange(RE::TESWeather* a_currentWeather);

	/*
	Hook for when the game changes the weather.
	Credit: PowerOfThree
	Nexus:  https://www.nexusmods.com/skyrimspecialedition/users/2148728
	Github: https://github.com/powerof3
	*/
	struct WeatherChange {
		static void thunk(RE::TESRegion* a_region, RE::TESWeather* a_currentWeather) {
			if (currentWeather != a_currentWeather) {
				currentWeather = a_currentWeather;
				QueueWeatherChange(a_currentWeather);
			}
			func(a_region, a_currentWeather);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	private:
		static inline RE::TESWeather* currentWeather{ nullptr };
	};

	void Install();
}