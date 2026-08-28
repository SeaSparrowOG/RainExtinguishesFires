#pragma once

#include "CellData/CellData.h"

namespace FireManipulator
{
	class Manipulator final : 
		public REX::TSingleton<Manipulator>,
		public RE::BSTEventSink<RE::BGSActorCellEvent>,
		public RE::BSTEventSink<RE::TESCellAttachDetachEvent>,
		public RE::BSTEventSink<RE::TESCellFullyLoadedEvent>,
		public RE::BSTEventSink<RE::TESHitEvent>
	{
	public:
		bool RegisterForGameEvents();

		void FreezeReference(RE::TESObjectREFR* ref);
		void UnFreezeReference(RE::TESObjectREFR* ref);

	private:
		bool HookWeatherChange();
		void InstallPlayerUpdateHook();
		bool RegisterForEvents();


		using EventControl = RE::BSEventNotifyControl;

		EventControl ProcessEvent(const RE::BGSActorCellEvent* a_event, 
												RE::BSTEventSource<RE::BGSActorCellEvent>*) override;
		EventControl ProcessEvent(const RE::TESCellAttachDetachEvent* a_event,
												RE::BSTEventSource<RE::TESCellAttachDetachEvent>*) override;
		EventControl ProcessEvent(const RE::TESCellFullyLoadedEvent* a_event,
												RE::BSTEventSource<RE::TESCellFullyLoadedEvent>*) override;
		EventControl ProcessEvent(const RE::TESHitEvent* a_event,
												RE::BSTEventSource<RE::TESHitEvent>*) override;
		void TimeAdvanced(float delta);
		void WeatherChanged(RE::TESWeather* weather);

		static inline void ChangeWeather(RE::TESRegion* a_region, RE::TESWeather* a_currentWeather);
		inline static void Update(RE::PlayerCharacter* a_this, float a_delta);

		inline static REL::Relocation<decltype(Update)>        _update;
		static inline REL::Relocation<decltype(ChangeWeather)> _changeWeather;

		// Weather change
		bool                        _waitForSun = false;
		bool                        _waitForRain = false;
		bool                        _rainy = false;
		bool                        _sunny = false;
		float                       _timeSinceLastQuery = 0.0f;
		RE::TESWeather*             _lastWeather = nullptr;
		SKSE::RegistrationSet<bool> _registeredListeners = "OnWeatherChange"sv; // unused

		std::unordered_map<RE::FormID, CellData::CellData> _cellDataMap;
	};

	bool Install();

	ObjectData GetObjectData(RE::TESBoundObject* base);
}