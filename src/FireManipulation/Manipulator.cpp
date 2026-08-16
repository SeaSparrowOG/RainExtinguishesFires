#include "Manipulator.h"

#include "Cache/FormCache.h"
#include "RE/Offset.h"
#include "Settings/INI/INISettings.h"

namespace
{
	static bool CanExtinguishFire(RE::TESObjectREFR* fire) {
		auto& xLists = fire->extraList;
		for (const auto& xList : xLists) {
			switch (xList.GetType()) {
			case RE::ExtraDataType::kEnableStateChildren:
			case RE::ExtraDataType::kEnableStateParent:
				return false;
			default:
				break;
			}
		}
		return true;
	}
}

namespace FireManipulator
{
	ObjectData GetObjectData(RE::TESBoundObject* base) {
		using Type = FireManipulator::ReferenceType;

		ObjectData data;

		if (base->Is(RE::FormType::Light)) {
			data.type = Type::Light;
			return data;
		}

		static const auto* cache = Cache::FormCache::GetSingleton();
		if (!cache) {
			return data;
		}

		static const auto& litMap = cache->GetLitFires();
		static const auto& unlitMap = cache->GetUnlitFires();
		static const auto& smokeSet = cache->GetSmokes();
		if (litMap.empty()) {
			return data;
		}
		const auto baseID = base->GetFormID();
		auto foundUnlitPair = litMap.find(baseID);
		if (foundUnlitPair != litMap.end()) {
			auto* foundForm = RE::TESForm::LookupByID(foundUnlitPair->second);
			if (foundForm) {
				data.type = Type::LitFire;
				data.pair = skyrim_cast<RE::TESBoundObject*>(foundForm);
				return data;
			}
		}
		auto foundLitPair = unlitMap.find(baseID);
		if (foundLitPair != unlitMap.end()) {
			auto* foundForm = RE::TESForm::LookupByID(foundLitPair->second);
			if (foundForm) {
				data.type = Type::UnlitFire;
				data.pair = skyrim_cast<RE::TESBoundObject*>(foundForm);
				return data;
			}
		}
		if (smokeSet.contains(baseID)) {
			data.type = Type::Smoke;
		}
		return data;
	}

	using EventControl = RE::BSEventNotifyControl;

	bool Manipulator::HookWeatherChange() {
		logger::info("   - Installing Weather Change Hook..."sv);
		REL::Relocation<std::uintptr_t> target{ RE::Offset::Sky::UpdateWeather, RE::Offset::Sky::UpdateWeather__ChangeWeather };
		if (!REL::make_pattern<"E8">().match(target.address())) {
			logger::error("    Failed to validate OPCode."sv);
			return false;
		}
		auto& trampoline = SKSE::GetTrampoline();
		_changeWeather = trampoline.write_call<5>(target.address(), &ChangeWeather);
		return true;
	}

	void Manipulator::InstallPlayerUpdateHook() {
		logger::info("  - Installing Update Hook..."sv);
		REL::Relocation<std::uintptr_t> VTABLE{ RE::PlayerCharacter::VTABLE[0] };
		_update = VTABLE.write_vfunc(0xAD, Update);
	}

	bool Manipulator::RegisterForEvents() {
		logger::info("  - Registering event listeners..."sv);
		auto* sourceHolder = RE::ScriptEventSourceHolder::GetSingleton();
		if (!sourceHolder) {
			logger::info("    Failed to get the game's scripted event source holder. You will crash later, and it won't be my fault."sv);
			return false;
		}

		sourceHolder->AddEventSink<RE::TESCellAttachDetachEvent>(this);
		sourceHolder->AddEventSink<RE::TESCellFullyLoadedEvent>(this);
		sourceHolder->AddEventSink<RE::TESHitEvent>(this);

		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			logger::critical("    Failed to get the game's player singleton. You will crash later, and it won't be my fault."sv);
			return false;
		}

		player->AddEventSink<RE::BGSActorCellEvent>(this);
		return true;
	}

	EventControl Manipulator::ProcessEvent(const RE::BGSActorCellEvent* a_event,
		RE::BSTEventSource<RE::BGSActorCellEvent>*)
	{
		if (!a_event || 
			!a_event->actor || 
			!a_event->flags.all(RE::BGSActorCellEvent::CellFlag::kEnter) ||
			!a_event->actor.get()->IsPlayerRef()) 
		{
			return EventControl::kContinue;
		}

		auto* cell = RE::TESForm::LookupByID<RE::TESObjectCELL>(a_event->cellID);
		
		return EventControl::kContinue;
	}

	EventControl Manipulator::ProcessEvent(const RE::TESCellAttachDetachEvent* a_event,
		RE::BSTEventSource<RE::TESCellAttachDetachEvent>*)
	{
		if (!a_event || !a_event->reference) {
			return EventControl::kContinue;
		}

		auto& ref = a_event->reference;
		const auto refID = ref->GetFormID();
		if ((refID & 0xFF000000) == 0xFF000000) {
			return EventControl::kContinue;
		}

		auto* base = ref->GetBaseObject();
		auto* cell = ref->GetParentCell();
		if (!cell || !base) {
			return EventControl::kContinue;
		}

		auto data = GetObjectData(base);
		if (data.type == ReferenceType::None) {
			return EventControl::kContinue;
		}

		auto it = _cellDataMap.find(cell->GetFormID());
		if (it == _cellDataMap.end()) {
			auto [where, inserted] = _cellDataMap.emplace(cell->GetFormID(), CellData::CellData());
			if (!inserted) {
				return EventControl::kContinue;
			}
			it = where;
		}

		if (a_event->attached) {
			it->second.ProcessRef(ref.get(), data);
		}
		else {
			it->second.ClearRef(refID);
		}
		return EventControl::kContinue;
	}

	EventControl Manipulator::ProcessEvent(const RE::TESCellFullyLoadedEvent* a_event,
		RE::BSTEventSource<RE::TESCellFullyLoadedEvent>*)
	{
		if (!a_event || !a_event->cell) {
			return EventControl::kContinue;
		}

		auto* cell = a_event->cell;
		auto it = _cellDataMap.find(cell->GetFormID());
		if (it == _cellDataMap.end()) {
			return EventControl::kContinue;
		}

		it->second.OnCellAttachDetach(cell);
		return EventControl::kContinue;
	}

	// TODO: Hit event
	EventControl Manipulator::ProcessEvent(const RE::TESHitEvent* a_event,
		RE::BSTEventSource<RE::TESHitEvent>*)
	{
		if (!a_event) {
			return EventControl::kContinue;
		}
		return EventControl::kContinue;
	}

	void Manipulator::TimeAdvanced(float delta) {
		auto* sky = RE::Sky::GetSingleton();
		if (!sky) {
			return;
		}

		const bool raining = sky->IsRaining();
		if (raining && _waitForRain) {
			_waitForSun = false;
			_waitForSun = false;
			_sunny = false;
			_rainy = true;

			for (auto& [cellID, cellData] : _cellDataMap) {
				RE::TESObjectCELL* cell = RE::TESForm::LookupByID<RE::TESObjectCELL>(cellID);
				if (!cell || cell->cellState.none(RE::TESObjectCELL::CellState::kAttached)) {
					continue;
				}
				cellData.OnCellAttachDetach(cell);
			}
		}
		else if (!raining && _waitForSun) {
			_waitForSun = false;
			_waitForSun = false;
			_sunny = true;
			_rainy = false;
		}
	}

	enum class WeatherType
	{
		Rainy,
		Sunny,
		None
	};

	static WeatherType GetWeatherType(const REX::EnumSet<RE::TESWeather::WeatherDataFlag, uint8_t> flags) {
		using WeatherFlag = RE::TESWeather::WeatherDataFlag;
		if (flags.any(WeatherFlag::kRainy, WeatherFlag::kSnow)) {
			return WeatherType::Rainy;
		}
		if (flags.any(WeatherFlag::kPleasant, WeatherFlag::kCloudy)) {
			return WeatherType::Sunny;
		}
		return WeatherType::None;
	}

	void Manipulator::WeatherChanged(RE::TESWeather* weather) {
		if (!weather) {
			return; // probably not possible
		}

		const auto lastWeatherType = _lastWeather ? GetWeatherType(_lastWeather->data.flags) : WeatherType::None;
		if (lastWeatherType == WeatherType::None) {
			_lastWeather = weather;
			const auto type = GetWeatherType(_lastWeather->data.flags);
			switch (type) {
			case WeatherType::None:
			case WeatherType::Rainy:
				_waitForRain = true;
				_waitForSun = false;
				_sunny = false;
				_rainy = false;
				break;
			case WeatherType::Sunny:
				_waitForRain = false;
				_waitForSun = true;
				_sunny = false;
				_rainy = false;
				break;
			default: std::unreachable();
			}
			return;
		}

		const auto currentWeatherType = GetWeatherType(weather->data.flags);
		switch (lastWeatherType) {
		case WeatherType::Rainy:
			switch (currentWeatherType) {
			case WeatherType::Sunny:
			case WeatherType::None:
				_waitForRain = false;
				_waitForSun = true;
				_sunny = false;
				_rainy = false;
				break;
			case WeatherType::Rainy:
				break;
			default: std::unreachable();
			}
			break;
		case WeatherType::Sunny:
			switch (currentWeatherType) {
			case WeatherType::Rainy:
			case WeatherType::None:
				_waitForRain = true;
				_waitForSun = false;
				_sunny = false;
				_rainy = false;
				break;
			case WeatherType::Sunny:
				break;
			default: std::unreachable();
			}
			break;
		default: std::unreachable();
		}
	}

	inline void Manipulator::Update(RE::PlayerCharacter* a_this, float a_delta) {
		_update(a_this, a_delta);

		static auto* manipulator = Manipulator::GetSingleton();
		if (!manipulator) {
			return;
		}
		manipulator->TimeAdvanced(a_delta);
	}

	void Manipulator::ChangeWeather(RE::TESRegion* a_region, RE::TESWeather* a_incomingWeather) {
		_changeWeather(a_region, a_incomingWeather);

		static auto* manipulator = Manipulator::GetSingleton();
		if (!manipulator) {
			return;
		}
		manipulator->WeatherChanged(a_incomingWeather);
	}

	bool Manipulator::RegisterForGameEvents() {
		bool success = true;
		SKSE::AllocTrampoline(14u);
		InstallPlayerUpdateHook();
		success &= HookWeatherChange();
		success &= RegisterForEvents();
		return success;
	}

	void Manipulator::FreezeReference(RE::TESObjectREFR* ref) {
		auto* cell = ref ? ref->GetParentCell() : nullptr;
		auto it = cell ? _cellDataMap.find(cell->GetFormID()) : _cellDataMap.end();
		if (it == _cellDataMap.end()) {
			return;
		}
		it->second.Freeze(ref->GetFormID());
	}

	void Manipulator::UnFreezeReference(RE::TESObjectREFR* ref) {
		auto* cell = ref ? ref->GetParentCell() : nullptr;
		auto it = cell ? _cellDataMap.find(cell->GetFormID()) : _cellDataMap.end();
		if (it == _cellDataMap.end()) {
			return;
		}
		it->second.UnFreeze(ref->GetFormID());
	}

	bool Install() {
		logger::info("Register for events and installing hooks..."sv);
		auto* manipulator = Manipulator::GetSingleton();
		if (!manipulator) {
			logger::critical("Failed to get internal fire manipulator. Aborting load..."sv);
			return false;
		}
		if (!manipulator->RegisterForGameEvents()) {
			logger::critical("Failed to install all needed listeners."sv);
			return false;
		}
		logger::info("Startup completed - enjoy your game!"sv);
		return true;
	}
}