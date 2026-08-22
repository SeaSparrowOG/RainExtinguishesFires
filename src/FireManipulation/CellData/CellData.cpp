#include "CellData.h"

#include "FireManipulation/Manipulator.h"
#include "Settings/INI/INISettings.h"

namespace FireManipulator::CellData
{
	CellData::CellData() {
		squashLight = Settings::INI::GetSetting<bool>(
			Settings::INI::GENERAL_SQUASH_LIGHTS.data())
			.value_or(false);
		squashSmoke = Settings::INI::GetSetting<bool>(
			Settings::INI::GENERAL_SQUASH_SMOKE.data())
			.value_or(false);
		lightDistance = Settings::INI::GetSetting<float>(
			Settings::INI::GENERAL_LOOKUP_LIGHT.data())
			.value_or(250.0f);
		smokeDistance = Settings::INI::GetSetting<float>(
			Settings::INI::GENERAL_LOOKUP_SMOKE.data())
			.value_or(250.0f);

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

		const bool raining = sky->IsRaining();
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
			for (const auto& unlit : unlitFires) {
				auto* ref = RE::TESForm::LookupByID<RE::TESObjectREFR>(unlit);
				auto* base = ref ? ref->GetBaseObject() : nullptr;
				if (!base) {
					continue;
				}
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
			std::erase_if(lights, [&](const auto& e) {
				return e == id;
				});
			break;
		case ReferenceType::Smoke:
			std::erase_if(smokes, [&](const auto& e) {
				return e == id;
				});
			break;
		case ReferenceType::LitFire:
			LOG_DEBUG("Clearing Lit Fire"sv);
			std::erase_if(litFires, [&](const auto& e) {
				return e == id;
				});
			break;
		case ReferenceType::UnlitFire:
			LOG_DEBUG("Clearing UnLit Fire"sv);
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
			ExtinguishImpl(back);
			_pendingExtinguishes.pop();
		}
	}

	void CellData::Dispose() {
		_queued = false;
	}

	void CellData::ExtinguishImpl(const PendingData& data) {
		auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
		auto* handlePolicy = vm ? vm->GetObjectHandlePolicy() : nullptr;
		if (!handlePolicy) {
			return;
		}

		auto* lit = data.fire;
		auto placedOff = lit->PlaceObjectAtMe(data.unlit, false);
		if (!placedOff) {
			return;
		}

		RE::VMHandle handle = handlePolicy->GetHandleForObject(RE::TESObjectREFR::FORMTYPE, placedOff.get());
		auto found = vm->attachedScripts.find(handle);
		if (found == vm->attachedScripts.end()) {
			placedOff->DeleteThis();
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