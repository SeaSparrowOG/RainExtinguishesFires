#include "Hooks/hooks.h"

#include "RE/Offset.h"

namespace Hooks {
	bool Install() {
		SECTION_SEPARATOR;
		logger::info("Installing hooks..."sv);

		SKSE::AllocTrampoline(14u);

		bool success = true;
		success &= WeatherChangeHook::InstallWeatherChangeHook();
		PlayerUpdateHook::InstallPlayerUpdateHook();

		if (!success) {
			logger::error("Failed to install at least one hook."sv);
		}

		auto* weatherManager = WeatherManager::GetSingleton();
		if (!weatherManager) {
			logger::error("Failed to get the internal Weather Manager singleton."sv);
			return false;
		}
		weatherManager->Register('WEMA');
		return success;
	}

	bool WeatherChangeHook::InstallWeatherChangeHook() {
		logger::info("  >Installing Weather Change Hook..."sv);
		REL::Relocation<std::uintptr_t> target{ RE::Offset::Sky::UpdateWeather, RE::Offset::Sky::UpdateWeather__ChangeWeather };
		if (!REL::make_pattern<"E8">().match(target.address())) {
			logger::error("    Failed to validate OPCode."sv);
			return false;
		}
		auto& trampoline = SKSE::GetTrampoline();
		_changeWeather = trampoline.write_call<5>(target.address(), &ChangeWeather);
		return true;
	}

	inline void WeatherChangeHook::ChangeWeather(RE::TESRegion* a_region, RE::TESWeather* a_incomingWeather) {
		_changeWeather(a_region, a_incomingWeather);
		if (a_incomingWeather) {
			static auto* weatherManager = WeatherManager::GetSingleton();
			weatherManager->WeatherChanged(a_incomingWeather);
		}
	}

	void PlayerUpdateHook::InstallPlayerUpdateHook() {
		REL::Relocation<std::uintptr_t> VTABLE{ RE::PlayerCharacter::VTABLE[0] };
		_update = VTABLE.write_vfunc(0xAD, Update);
	}

	inline void PlayerUpdateHook::Update(RE::PlayerCharacter* a_this, float a_delta) {
		_update(a_this, a_delta);

		static auto* weatherManager = WeatherManager::GetSingleton();
		weatherManager->TimeUpdate(a_delta);
	}

	void WeatherManager::WeatherChanged(RE::TESWeather* a_incomingWeather) {
		auto pastState = GetWeatherState(_lastWeather);
		if (!a_incomingWeather || a_incomingWeather == _lastWeather) {
			return;
		}
		_lastWeather = a_incomingWeather;

		auto currentState = GetWeatherState(a_incomingWeather);
		if (currentState == WeatherState::Pleasant && pastState == WeatherState::Rainy) {
			_waitForSun = true;
			_waitForRain = false;
		}
		else if (pastState == WeatherState::Pleasant && currentState == WeatherState::Rainy) {
			_waitForRain = true;
			_waitForSun = false;
		}
		else {
			_waitForRain = false;
			_waitForSun = false;
		}
	}

	void WeatherManager::TimeUpdate(float a_delta) {
		_timeSinceLastQuery += a_delta;
		if (a_delta < 0.5f) {
			return;
		}

		_timeSinceLastQuery = 0.0f;
		auto* sky = RE::Sky::GetSingleton();
		if (!sky) {
			return;
		}

		const bool isOvercast = sky->IsRaining() || sky->IsSnowing();
		if (_waitForSun && !isOvercast) {
			_waitForSun = false;
			_registeredListeners.QueueEvent(false);
		}
		else if (_waitForRain && isOvercast) {
			_waitForRain = false;
			_registeredListeners.QueueEvent(true);
		}
	}

	void WeatherManager::RegisterFormForEvents(RE::TESForm* a_form) {
		_registeredListeners.Register(a_form);
	}

	bool WeatherManager::Save(SKSE::SerializationInterface* a_intfc) {
		if (!_registeredListeners.Save(a_intfc)) {
			logger::error("Failed to write registered listeners."sv);
			return false;
		}
		if (!a_intfc->WriteRecordData(_waitForSun)) {
			logger::error("Failed to write _waitForSun data to cosave."sv);
			return false;
		}
		if (!a_intfc->WriteRecordData(_waitForRain)) {
			logger::error("Failed to write _waitForRain data to cosave."sv);
			return false;
		}
		return true;
	}

	bool WeatherManager::Load(SKSE::SerializationInterface* a_intfc) {
		if (!_registeredListeners.Load(a_intfc)) {
			logger::error("Failed to read registered listeners."sv);
			return false;
		}
		if (!a_intfc->ReadRecordData(_waitForSun)) {
			logger::critical("Failed to read _waitForSun from the cosave."sv);
			return false;
		}
		if (!a_intfc->ReadRecordData(_waitForRain)) {
			logger::critical("Failed to read _waitForRain from the cosave."sv);
			return false;
		}

		auto* sky = RE::Sky::GetSingleton();
		if (!sky) {
			logger::error("Failed to get the game's Sky manager."sv);
			return false;
		}
		_lastWeather = sky->currentWeather;
		_timeSinceLastQuery = 0.0f;
		return true;
	}

	WeatherManager::WeatherState WeatherManager::GetWeatherState(RE::TESWeather* a_weather) {
		if (!a_weather) {
			return WeatherState::None;
		}
		if (a_weather->data.flags.any(WeatherType::kCloudy, WeatherType::kNone, WeatherType::kPleasant)) {
			return WeatherState::Pleasant;
		}
		return WeatherState::Rainy;
	}
}