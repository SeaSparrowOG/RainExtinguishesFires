#pragma once
#include "fireRegister.h"

namespace Papyrus {
#define BIND(a_method, ...) a_vm.RegisterFunction(#a_method##sv, script, a_method __VA_OPT__(, ) __VA_ARGS__)
#define BIND_EVENT(a_method, ...) a_vm.RegisterFunction(#a_method##sv, script, a_method __VA_OPT__(, ) __VA_ARGS__)
#define STATIC_ARGS [[maybe_unused]] VM *a_vm, [[maybe_unused]] StackID a_stackID, RE::StaticFunctionTag *
#define rainyWeather RE::TESWeather::WeatherDataFlag::kRainy
#define snowyWeahter RE::TESWeather::WeatherDataFlag::kSnow

	using VM = RE::BSScript::Internal::VirtualMachine;
	using StackID = RE::VMStackID;
	inline constexpr auto script = "REF_UtilityFunctions"sv;
	bool RegisterFunctions(VM* a_vm);

	class Papyrus : public ISingleton<Papyrus> {
	public:
		void AddWeatherChangeListener(const RE::TESForm* a_form, bool a_listen);
		void AddInteriorExteriorListener(const RE::TESForm* a_form, bool a_listen);
		void ExtinguishFire(RE::TESObjectREFR* a_litFire, CachedData::FireData a_data);
		void RelightFire(RE::TESObjectREFR* a_litFire);
		void SendWeatherChange(RE::TESWeather* a_weather);
		void SendPlayerChangedInteriorExterior(bool a_movedToExterior);
		void SetIsRaining(bool a_isRaining);

		void DisablePapyrus();

	private:
		bool disable = false;
		/**
		* Event called when the weather changes.
		* @param bool True if raining.
		* @param float How long (in seconds) until the weather's rain/snow start. 0.0f if not rainy/snowy.
		*/
		SKSE::RegistrationSet<bool, float> weatherTransition{ "OnWeatherChange"sv };

		/**
		* Event called when the player moves from an interior to an exterior or vice-versa.
		* @param bool True if moving to an exterior.
		*/
		SKSE::RegistrationSet<bool> movedToExterior{ "OnPlayerInteriorExteriorChange"sv };

		//The currently stored weather.
		RE::TESWeather* currentWeather;

		//Is it raining? Actually pouring down?
		bool isRaining;

		//Fires take appreciable time to reload due to "Utility.Wait()".
		std::unordered_map<RE::TESObjectREFR*, bool>   frozenFiresRegister;
	};
}