#pragma once

#include "Serialization/Serde.h"

namespace Hooks {
	bool Install();

	struct WeatherChangeHook
	{
		static bool InstallWeatherChangeHook();

		inline static void ChangeWeather(RE::TESRegion* a_region, RE::TESWeather* a_incomingWeather);
		inline static REL::Relocation<decltype(ChangeWeather)> _changeWeather;
	};

	struct PlayerUpdateHook
	{
		static void InstallPlayerUpdateHook();

		inline static void Update(RE::PlayerCharacter* a_this, float a_delta);
		inline static REL::Relocation<decltype(Update)> _update;
	};

	class WeatherManager : 
		public REX::Singleton<WeatherManager>,
		public Serialization::Serializable
	{
	public:
		void WeatherChanged(RE::TESWeather* a_incomingWeather);
		void TimeUpdate(float a_delta);

		void RegisterFormForEvents(RE::TESForm* a_form);

		// Serialiazable
		bool Save(SKSE::SerializationInterface* a_intfc) override;
		bool Load(SKSE::SerializationInterface* a_intfc) override;

	private:
		using WeatherType = RE::TESWeather::WeatherDataFlag;
		enum class WeatherState
		{
			None,
			Rainy,
			Pleasant
		};
		WeatherState GetWeatherState(RE::TESWeather* a_weather);

		bool                        _waitForSun = false;
		bool                        _waitForRain = false;
		float                       _timeSinceLastQuery = 0.0f;
		RE::TESWeather*             _lastWeather = nullptr;
		SKSE::RegistrationSet<bool> _registeredListeners = "OnWeatherChange"sv ;
	};
}