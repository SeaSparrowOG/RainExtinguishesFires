#include "CellData.h"

#include "FireManipulation/Manipulator.h"
#include "Settings/INI/INISettings.h"

namespace
{
	// TODO:
	// This shouldn't be here. Until clib updates, I have to
	// rely on this.
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

	static bool IsOccluded(RE::TESObjectREFR* caster) {
		auto* player = RE::PlayerCharacter::GetSingleton();
		auto* parentCell = caster ? caster->GetParentCell() : nullptr;
		auto* bhkWorld = parentCell ? parentCell->GetbhkWorld() : nullptr;
		if (!bhkWorld || !player) {
			return false;
		}

		auto boundData = caster->GetBaseObject()->boundData;
		auto havokWorldScale = RE::bhkWorld::GetWorldScale();
		RE::bhkPickData pick_data;
		RE::NiPoint3 ray_start, ray_end;

		ray_start = caster->data.location;
		ray_start.z += 100.0f + boundData.boundMax.z;
		ray_end = ray_start;
		ray_end.z += 50000.0f;

		pick_data.rayInput.from = ray_start * havokWorldScale;
		pick_data.rayInput.to = ray_end * havokWorldScale;

		auto filter = RE::CFilter();
		player->GetCollisionFilterInfo(filter);
		filter.SetCollisionLayer(RE::COL_LAYER::kCharController);
		pick_data.rayInput.filterInfo = filter;

		bhkWorld->PickObject(pick_data);
		return pick_data.rayOutput.HasHit();
	}

	static bool CanExtinguishFire(RE::TESObjectREFR* fire, bool checkOcclusion) {
		const auto formID = fire->GetFormID();
		if ((formID & 0xFF000000) == 0xFF000000) {
			return false;
		}

		auto& xList = fire->extraList;
		if (xList.HasType(RE::ExtraDataType::kEnableStateParent)) {
			return false;
		}
		if (xList.HasType(RE::ExtraDataType::kEnableStateChildren)) {
			return false;
		}
		
		return !checkOcclusion || IsOccluded(fire);
	}
}

namespace FireManipulator::CellData
{
	CellData::CellData() {
		squashLight = Settings::INI::GetSetting<bool>(
			Settings::INI::GENERAL_SQUASH_LIGHTS.data())
			.value_or(true);
		squashSmoke = Settings::INI::GetSetting<bool>(
			Settings::INI::GENERAL_SQUASH_SMOKE.data())
			.value_or(true);
		checkOcclusion = Settings::INI::GetSetting<bool>(
			Settings::INI::GENERAL_CHECK_OCCLUSION.data())
			.value_or(false);
		lightDistance = Settings::INI::GetSetting<float>(
			Settings::INI::GENERAL_LOOKUP_LIGHT.data())
			.value_or(250.0f);
		smokeDistance = Settings::INI::GetSetting<float>(
			Settings::INI::GENERAL_LOOKUP_SMOKE.data())
			.value_or(250.0f);
		resetDays = Settings::INI::GetSetting<float>(
			Settings::INI::GENERAL_RESET_DAYS.data())
			.value_or(2.0f);

		litFires.reserve(12);
		unlitFires.reserve(12);
		smokes.reserve(12);
		lights.reserve(12);
	}

	RE::TESObjectREFR* CellData::FindClosestFrom(RE::TESObjectREFR* from,
													const std::vector<RE::FormID>& nearby,
													float radius)
	{
		RE::TESObjectREFR* found = nullptr;
		auto origin = from->GetPosition();
		for (auto current = nearby.begin();current < nearby.end(); ++current) {
			if (reservedSmokesAndLights.contains(*current)) {
				continue;
			}

			auto* ref = RE::TESForm::LookupByID<RE::TESObjectREFR>(*current);
			if (!ref || ref->IsDisabled()) {
				continue;
			}

			const float distance = ref->GetPosition().GetDistance(origin);
			if (distance < radius) {
				radius = distance;
				found = ref;
			}
		}
		if (found != nullptr) {
			reservedSmokesAndLights.insert(found->GetFormID());
		}
		return found;
	}

	void CellData::OnCellAttachDetach(RE::TESObjectCELL* cell) {
		if (!cell || 
			cell->cellState.none(RE::TESObjectCELL::CellState::kAttached) || 
			cell->IsInteriorCell()) 
		{
			return;
		}

		static auto* manipulator = Manipulator::GetSingleton();
		static auto* cache = Cache::FormCache::GetSingleton();
		auto* sky = RE::Sky::GetSingleton();
		if (!sky || !manipulator || !cache) {
			return;
		}

		auto* tasks = SKSE::GetTaskInterface();
		if (!tasks) {
			return; // ???
		}

		const auto& unlitData = cache->GetUnlitData();
		if (unlitData.empty()) {
			return;
		}

		const bool raining = IsRaining();
		if (raining && !litFires.empty()) {

			for (const auto& lit : litFires) {
				if (transitioningFires.contains(lit)) {
					continue;
				}

				auto* ref = RE::TESForm::LookupByID<RE::TESObjectREFR>(lit);
				if (!ref || ref->IsDisabled() || !ref->Is3DLoaded()) {
					continue;
				}

				auto* base = ref->GetBaseObject();
				auto pair = base ? unlitData.find(base->GetFormID()) : unlitData.end();
				auto* unlitBase = pair != unlitData.end() ?
					RE::TESForm::LookupByID<RE::TESBoundObject>(pair->second._offFireID) : 
					nullptr;
				if (!unlitBase) {
					continue;
				}

				const bool refNeedsOcclusionCheck = checkOcclusion || 
					pair->second._forceOcclusionCheck.value_or(false);
				if (!CanExtinguishFire(ref, refNeedsOcclusionCheck)) {
					continue;
				}

				transitioningFires.insert(lit);

				PendingData data;
				data.fire = ref;
				data.unlit = unlitBase;
				data.sizeOverride = pair->second._resizeByPercent;
				data.overrideOcclusion = pair->second._forceOcclusionCheck;

				if (squashLight) {
					data.light = FindClosestFrom(ref, lights, lightDistance);
				}
				if (squashSmoke) {
					data.smoke = FindClosestFrom(ref, smokes, smokeDistance);
				}
				_pendingExtinguishes.push(std::move(data));
			}

			if (!_pendingExtinguishes.empty() && !_queued) {
				_queued = true;
				tasks->AddTask(reinterpret_cast<::TaskDelegate*>(this));
			}
		}
		else if (!unlitFires.empty() && !raining) {
			const float currentDay = RE::Calendar::GetSingleton()->GetDay();
			for (const auto& unlit : unlitFires) {
				auto* ref = RE::TESForm::LookupByID<RE::TESObjectREFR>(unlit);
				if (!ref) {
					continue;
				}

				auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
				auto* handlePolicy = vm ? vm->GetObjectHandlePolicy() : nullptr;
				if (!handlePolicy) {
					return;
				}

				RE::VMHandle handle = handlePolicy->GetHandleForObject(RE::TESObjectREFR::FORMTYPE, ref);
				auto found = vm->attachedScripts.find(handle);
				if (found == vm->attachedScripts.end()) {
					continue;
				}

				for (auto& script : found->second) {
					auto* info = script ? script->GetTypeInfo() : nullptr;
					if (!info || info->GetName() != "REF_ObjectRefOffController"sv) {
						continue;
					}

					auto* dayAttachedProperty = script->GetProperty("DayAttached");
					const float dayAttached = dayAttachedProperty ?
						dayAttachedProperty->GetFloat() :
						0.0f;
					const float timeElapsed = currentDay - dayAttached;

					if (timeElapsed > resetDays || dayAttached > currentDay + 1.0f) {
						PendingData data;
						data.fire = ref;
						data.type = ActionType::Relight;
						_pendingExtinguishes.push(std::move(data));
					}
				}
			}

			if (!_pendingExtinguishes.empty() && !_queued) {
				_queued = true;
				tasks->AddTask(reinterpret_cast<::TaskDelegate*>(this));
			}
		}
	}

	void CellData::ClearRef(const RE::FormID id) {
		auto it = seen.find(id);
		if (it == seen.end()) {
			return;
		}

		switch (it->second) {
		case ReferenceType::Light:
		case ReferenceType::Smoke:
		case ReferenceType::LitFire:
			break;
		case ReferenceType::UnlitFire:
			std::erase_if(unlitFires, [&](const auto& e) {
				return e == id;
				});
			break;
		default: std::unreachable();
		}

		seen.erase(it);
	}

	void CellData::ProcessRef(RE::TESObjectREFR* ref, const ObjectData& data) {
		const auto id = ref->GetFormID();
		if (seen.contains(id)) {
			return;
		}
		seen.emplace(id, data.type);
		switch (data.type) {
		case ReferenceType::Light:
			lights.push_back(id);
			break;
		case ReferenceType::Smoke:
			smokes.push_back(id);
			break;
		case ReferenceType::LitFire:
			litFires.push_back(id);
			break;
		case ReferenceType::UnlitFire:
			unlitFires.push_back(id);
			break;
		default: std::unreachable();
		}
	}

	void CellData::ExtinguishRef(const RE::FormID id) {
		if (transitioningFires.contains(id)) {
			return;
		}

		auto* tasks = SKSE::GetTaskInterface();
		if (!tasks) {
			return; // what
		}

		auto* ref = RE::TESForm::LookupByID<RE::TESObjectREFR>(id);
		auto* base = ref ? ref->GetBaseObject() : nullptr;
		auto refData = GetObjectData(base);
		if (refData.type != ReferenceType::LitFire || !refData.data.has_value()) {
			return;
		}

		auto* parent = ref->GetParentCell();
		if (!parent || parent->cellState.none(RE::TESObjectCELL::CellState::kAttached)) {
			return;
		}

		const auto& refDataValue = refData.data.value();
		auto* offForm = RE::TESForm::LookupByID<RE::TESBoundObject>(refDataValue._offFireID);
		if (!offForm) {
			return;
		}
		transitioningFires.insert(id);

		PendingData data;
		data.fire = ref;
		data.overrideOcclusion = refDataValue._forceOcclusionCheck;
		data.sizeOverride = refDataValue._resizeByPercent;
		data.unlit = offForm;

		if (squashLight) {
			data.light = FindClosestFrom(ref, lights, lightDistance);
		}
		if (squashSmoke) {
			data.smoke = FindClosestFrom(ref, smokes, smokeDistance);
		}

		_pendingExtinguishes.emplace(std::move(data));
		if (!_pendingExtinguishes.empty() && !_queued) {
			_queued = true;
			tasks->AddTask(reinterpret_cast<::TaskDelegate*>(this));
		}
	}

	void CellData::RelightRef(const RE::FormID id) {
		if (transitioningFires.contains(id)) {
			return;
		}
		auto* tasks = SKSE::GetTaskInterface();
		if (!tasks) {
			return; // what
		}

		auto* ref = RE::TESForm::LookupByID<RE::TESObjectREFR>(id);
		auto* base = ref ? ref->GetBaseObject() : nullptr;
		auto refData = GetObjectData(base);
		if (refData.type != ReferenceType::UnlitFire) {
			return;
		}

		auto* parent = ref->GetParentCell();
		if (!parent || parent->cellState.none(RE::TESObjectCELL::CellState::kAttached)) {
			return;
		}
		transitioningFires.insert(id);

		PendingData data;
		data.fire = ref;
		data.type = ActionType::Relight;
		_pendingExtinguishes.emplace(std::move(data));
		if (!_pendingExtinguishes.empty() && !_queued) {
			_queued = true;
			tasks->AddTask(reinterpret_cast<::TaskDelegate*>(this));
		}
	}

	void CellData::UnFreeze(const RE::FormID id) {
		auto it = transitioningFires.find(id);
		if (it != transitioningFires.end()) {
			transitioningFires.erase(it);
			return;
		}
		it = reservedSmokesAndLights.find(id);
		if (it != reservedSmokesAndLights.end()) {
			reservedSmokesAndLights.erase(it);
		}
	}

	void CellData::Freeze(const RE::FormID id) {
		auto it = seen.find(id);
		if (it == seen.end()) {
			return;
		}
		switch (it->second) {
		case ReferenceType::Light:
		case ReferenceType::Smoke:
			reservedSmokesAndLights.insert(id);
			break;
		case ReferenceType::LitFire:
		case ReferenceType::UnlitFire:
			transitioningFires.insert(id);
			break;
		default: 
			break;
		}
	}

	void CellData::Run() {
		while (!_pendingExtinguishes.empty()) {
			const auto& back = _pendingExtinguishes.top();
			switch (back.type) {
			case ActionType::Extinguish:
				ExtinguishImpl(back);
				break;
			case ActionType::Relight:
				RelightImpl(back);
				break;
			default: std::unreachable();
			}
			_pendingExtinguishes.pop();
		}
	}

	void CellData::Dispose() {
		_queued = false;
	}

	void CellData::RelightImpl(const PendingData& data) {
		auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
		auto* handlePolicy = vm ? vm->GetObjectHandlePolicy() : nullptr;
		if (!handlePolicy) {
			return;
		}

		auto* offFire = data.fire;
		if (!offFire) {
			return;
		}

		RE::VMHandle handle = handlePolicy->GetHandleForObject(RE::TESObjectREFR::FORMTYPE, offFire);
		auto found = vm->attachedScripts.find(handle);
		if (found == vm->attachedScripts.end()) {
			return;
		}

		for (auto& script : found->second) {
			auto* info = script ? script->GetTypeInfo() : nullptr;
			if (!info || info->GetName() != "REF_ObjectRefOffController"sv) {
				continue;
			}

			auto callback = RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor>();
			auto args = RE::MakeFunctionArguments();
			const RE::BSFixedString functionName = "Relight";
			auto scriptObject = script.get();
			auto object = RE::BSTSmartPointer<RE::BSScript::Object>(scriptObject);
			vm->DispatchMethodCall(object, functionName, args, callback);
			break;
		}
	}

	void CellData::ExtinguishImpl(const PendingData& data) {
#ifndef NDEBUG
		const auto* base = data.fire->GetBaseObject();
		const std::string edid = clib_util::editorID::get_editorID(base);
		LOG_DEBUG("Extinguishing: {}"sv, edid);
#endif
		auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
		auto* handlePolicy = vm ? vm->GetObjectHandlePolicy() : nullptr;
		if (!handlePolicy) {
			LOG_DEBUG("  - No Handle Policy"sv);
			return;
		}

		auto* lit = data.fire;
		auto placedOff = lit->PlaceObjectAtMe(data.unlit, false);
		if (!placedOff) {
			LOG_DEBUG("  - No <OFF> placed."sv);
			return;
		}

		RE::VMHandle handle = handlePolicy->GetHandleForObject(RE::TESObjectREFR::FORMTYPE, placedOff.get());
		auto found = vm->attachedScripts.find(handle);
		if (found == vm->attachedScripts.end()) {
			placedOff->DeleteThis();
			LOG_DEBUG("  - No <Scripts>"sv);
			return;
		}

		float scale = lit->GetScale();
		if (data.sizeOverride.has_value()) {
			scale *= data.sizeOverride.value();
		}

		placedOff->SetScale(scale);
		placedOff->MoveTo(lit);
		placedOff->SetAngle(lit->GetAngle());

		bool errored = true;
		for (auto& script : found->second) {
			auto* info = script ? script->GetTypeInfo() : nullptr;
			if (!info || info->GetName() != "REF_ObjectRefOffController"sv) {
				continue;
			}

			auto relatedFlame = script->GetProperty("RelatedFlame");
			auto relatedLight = script->GetProperty("RelatedLight");
			auto relatedSmoke = script->GetProperty("RelatedSmoke");
			auto dayAttached = script->GetProperty("DayAttached");
			if (!(relatedFlame && relatedLight && relatedSmoke && dayAttached)) {
				continue;
			}

			relatedFlame->Pack(data.fire);
			relatedLight->Pack(data.light);
			relatedSmoke->Pack(data.smoke);
			dayAttached->SetFloat(RE::Calendar::GetSingleton()->GetDaysPassed());

			auto callback = RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor>();
			auto args = RE::MakeFunctionArguments();
			const RE::BSFixedString functionName = "Extinguish";
			auto scriptObject = script.get();
			auto object = RE::BSTSmartPointer<RE::BSScript::Object>(scriptObject);
			vm->DispatchMethodCall(object, functionName, args, callback);
			errored = false;
			break;
		}

		if (errored) {
			LOG_DEBUG("  - Errored"sv);
			transitioningFires.erase(data.fire->GetFormID());
			if (data.light) {
				reservedSmokesAndLights.erase(data.light->GetFormID());
			}
			if (data.smoke) {
				reservedSmokesAndLights.erase(data.smoke->GetFormID());
			}
			placedOff->DeleteThis();
			return;
		}
		transitioningFires.insert(placedOff->GetFormID());
	}
}