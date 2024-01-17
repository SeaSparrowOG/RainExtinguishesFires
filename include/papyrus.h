#pragma once

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
		void AddFireToRegistry(RE::TESObjectREFR* a_fire);
		bool IsRaining();
		void RegisterFormForWeatherEvent(const RE::TESForm* a_form);
		void RegisterFormForExtinguishEvent(const RE::TESForm* a_form);
		void RegisterFormForCellChangeEvent(const RE::TESForm* a_form);
		void RegisterFormForRelightEvent(const RE::TESForm* a_form);
		void RemoveFireFromRegistry(RE::TESObjectREFR* a_fire);
		void ResetFrozenMaps();
		void SendWeatherChangeEvent(RE::TESWeather* a_weather);
		void SendExtinguishEvent(RE::TESObjectREFR* a_fire, RE::TESForm* a_offVersion, bool a_dyndolodFire, bool a_force = false);
		void SendRelightEvent(RE::TESObjectREFR* a_fire, bool a_bForce = false);
		void SendPlayerChangedInteriorExterior(bool a_movedToExterior);
		void SetIsRaining(bool a_isRaining);

	private:
		/**
		* Event called when the weather changes.
		* @param bool True if raining.
		* @param float How long (in seconds) until the weather's rain/snow start. 0.0f if not rainy/snowy.
		*/
		SKSE::RegistrationSet<bool, float> weatherTransition{ "OnWeatherTransitionComplete"sv };

		/**
		* Function for extinguishing a lit fire.
		* @param Reference* The lit fire.
		* @param Form* The "Off" version of the fire.
		* @param std::vector<RE::TESObjectREFR*> The related references to extinguish.
		*/
		SKSE::RegistrationSet<RE::TESObjectREFR*, RE::TESForm*, std::vector<RE::TESObjectREFR*>> extinguishFire{ "OnExtinguishEvent"sv };

		/**
		* Function for relighting an extinguished fire.
		* @param Reference* The unlit fire to relight.
		*/
		SKSE::RegistrationSet < RE::TESObjectREFR*, bool> relightFire{ "OnFireRelightEvent"sv };

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