#include "Manipulator.h"

#include "Cache/FormCache.h"
#include "RE/Offset.h"
#include "Settings/INI/INISettings.h"

namespace
{
	static bool IsRaining()
	{
		const auto* sky = RE::Sky::GetSingleton();
		if (!sky) {
			return false;
		}

		const auto* current = sky->currentWeather;
		if (!current) {
			return false;
		}

		const auto* last = sky->lastWeather;
		const auto& data = current->data;
		const auto* lastData = last ? &last->data : nullptr;
		if (data.flags.any(RE::TESWeather::WeatherDataFlag::kRainy, RE::TESWeather::WeatherDataFlag::kSnow)) {
			if (!lastData) {
				return true;
			}

			if (lastData->flags.any(RE::TESWeather::WeatherDataFlag::kRainy, RE::TESWeather::WeatherDataFlag::kSnow)) {
				return true;
			}

			const float fadeInPct = 1.0f + static_cast<float>(current->data.precipitationBeginFadeIn) / 255.0f;
			return fadeInPct <= sky->currentWeatherPct;
		}
		else if (lastData && lastData->flags.any(RE::TESWeather::WeatherDataFlag::kRainy, RE::TESWeather::WeatherDataFlag::kSnow)) {
			const float fadeOutPct = static_cast<float>(current->data.precipitationEndFadeOut) / 255.0f;
			return fadeOutPct > sky->currentWeatherPct;
		}
		return false;
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

		static const auto& unlitMap = cache->GetUnlitData();
		static const auto& smokeSet = cache->GetSmokes();
		static const auto& unlitSet = cache->GetUnlitFires();
		if (unlitMap.empty()) {
			return data;
		}
		const auto baseID = base->GetFormID();

		auto foundUnlitData = unlitMap.find(baseID);
		if (foundUnlitData != unlitMap.end()) {
			data.type = Type::LitFire;
			data.data = foundUnlitData->second;
			return data;
		}

		if (unlitSet.contains(baseID)) {
			data.type = Type::UnlitFire;
			return data;
		}

		if (smokeSet.contains(baseID)) {
			data.type = Type::Smoke;
		}
		return data;
	}

	using EventControl = RE::BSEventNotifyControl;

	bool Manipulator::HookWeatherChange() {
		logger::INFO("   - Installing Weather Change Hook..."sv);
		REL::Relocation<std::uintptr_t> target{ RE::Offset::Sky::UpdateWeather, RE::Offset::Sky::UpdateWeather__ChangeWeather };
		if (!REL::Pattern<"E8">().match(target.address())) {
			logger::CRITICAL("    Failed to validate OPCode."sv);
			return false;
		}
		auto& trampoline = REL::GetTrampoline();
		_changeWeather = trampoline.write_call<5>(target.address(), &ChangeWeather);
		return true;
	}

	void Manipulator::InstallPlayerUpdateHook() {
		logger::INFO("  - Installing Update Hook..."sv);
		REL::Relocation<std::uintptr_t> VTABLE{ RE::PlayerCharacter::VTABLE[0] };
		_update = VTABLE.write_vfunc(0xAD, Update);
	}

	bool Manipulator::RegisterForEvents() {
		logger::INFO("  - Registering event listeners..."sv);
		auto* sourceHolder = RE::ScriptEventSourceHolder::GetSingleton();
		if (!sourceHolder) {
			logger::INFO("    Failed to get the game's scripted event source holder. You will crash later, and it won't be my fault."sv);
			return false;
		}

		sourceHolder->AddEventSink<RE::TESCellAttachDetachEvent>(this);
		sourceHolder->AddEventSink<RE::TESCellFullyLoadedEvent>(this);
		sourceHolder->AddEventSink<RE::TESHitEvent>(this);

		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			logger::CRITICAL("    Failed to get the game's player singleton. You will crash later, and it won't be my fault."sv);
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
		if (!cell || 
			!cell->cellState.all(RE::TESObjectCELL::CellState::kAttached) || 
			cell->IsInteriorCell()) 
		{
			return EventControl::kContinue;
		}

		auto it = _cellDataMap.find(a_event->cellID);
		if (it == _cellDataMap.end()) {
			return EventControl::kContinue;
		}

		it->second.OnCellAttachDetach(cell);
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

		auto* base = ref->GetBaseObject();
		auto* cell = ref->GetParentCell();
		if (!cell || !base) {
			return EventControl::kContinue;
		}

		auto data = GetObjectData(base);

		if (data.type == ReferenceType::None) {
			return EventControl::kContinue;
		}
		else if (data.type != ReferenceType::UnlitFire && (refID & 0xFF000000) == 0xFF000000) {
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

	EventControl Manipulator::ProcessEvent(const RE::TESHitEvent* a_event,
		RE::BSTEventSource<RE::TESHitEvent>*)
	{
		if (!a_event || !a_event->target) {
			return EventControl::kContinue;
		}

		auto* targetBase = a_event->target->GetBaseObject();
		auto baseType = GetObjectData(targetBase);
		switch (baseType.type) {
		case ReferenceType::LitFire:
		case ReferenceType::UnlitFire:
			break;
		default:
			return EventControl::kContinue;
		}

		const auto* targetCell = a_event->target->GetParentCell();
		if (!targetCell || targetCell->cellState.none(RE::TESObjectCELL::CellState::kAttached)) {
			return EventControl::kContinue;
		}

		const auto cellID = targetCell->GetFormID();
		const auto targetCellData = _cellDataMap.find(cellID);
		if (targetCellData == _cellDataMap.end()) {
			return EventControl::kContinue;
		}

		const auto source = a_event->source;
		const auto* form = RE::TESForm::LookupByID(source);
		const auto type = form ? form->GetFormType() : RE::FormType::None;
		RE::BSTArray<RE::Effect*> effects;

		if (type == RE::TESObjectWEAP::FORMTYPE) {
			const auto* weap = form->As<RE::TESObjectWEAP>();
			assert(weap);
			const auto* ench = weap->formEnchanting;
			if (!ench) {
				return EventControl::kContinue;
			}
			effects = ench->effects;
		}
		else if (type == RE::SpellItem::FORMTYPE) {
			const auto* spell = form->As<RE::SpellItem>();
			assert(spell);
			effects = spell->effects;
		}

		if (effects.empty()) {
			return EventControl::kContinue;
		}

		const auto* fireKeyword = RE::TESForm::LookupByID<RE::BGSKeyword>(0x1CEAD);
		const auto* frostKeyword = RE::TESForm::LookupByID<RE::BGSKeyword>(0x1CEAE);
		assert(fireKeyword && frostKeyword);
		if (!fireKeyword || !frostKeyword) {
			return EventControl::kContinue;
		}

		bool isFire = false;
		bool isFrost = false;
		for (const auto* effect : effects) {
			const auto* base = effect ? effect->baseEffect : nullptr;
			if (!base) {
				continue;
			}

			isFire |= base->HasKeyword(fireKeyword);
			isFrost |= base->HasKeyword(frostKeyword);
			if (isFire && isFrost) {
				return EventControl::kContinue;
			}
		}

		if (isFire && baseType.type == ReferenceType::UnlitFire) {
			targetCellData->second.RelightRef(a_event->target->GetFormID());
		}
		else if (isFrost && baseType.type == ReferenceType::LitFire) {
			targetCellData->second.ExtinguishRef(a_event->target->GetFormID());
		}

		return EventControl::kContinue;
	}

	void Manipulator::TimeAdvanced(float delta) {
		if (delta <= 0.0f) {
			return;
		}
		_timeSinceLastQuery += delta;
		if (_timeSinceLastQuery < 1.0f) {
			return;
		}
		_timeSinceLastQuery = 0.0f;

		auto* sky = RE::Sky::GetSingleton();
		if (!sky) {
			return;
		}

		// Important: In 1.6.1170, Commonlib has a bug that makes it so that
		// IsRaining() and IsSnowing() always return true if the incoming weather
		// is rainy or snowy respectively. I made a PR, and it will be fixed...
		// but until the 1.7.99 update settles, a local helper will be used.
		// const bool raining = sky->IsRaining() || sky->IsSnowing();

		const bool raining = IsRaining();
		if (raining && _waitForRain) {
			_waitForRain = false;
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
			_waitForRain = false;
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

	static WeatherType GetWeatherType(const REX::TEnumSet<RE::TESWeather::WeatherDataFlag, uint8_t> flags) {
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
		_lastWeather = weather;
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
		logger::INFO("Register for events and installing hooks..."sv);
		auto* manipulator = Manipulator::GetSingleton();
		if (!manipulator) {
			logger::CRITICAL("Failed to get internal fire manipulator. Aborting load..."sv);
			return false;
		}
		if (!manipulator->RegisterForGameEvents()) {
			logger::CRITICAL("Failed to install all needed listeners."sv);
			return false;
		}
		logger::INFO("Startup completed - enjoy your game!"sv);
		return true;
	}
}