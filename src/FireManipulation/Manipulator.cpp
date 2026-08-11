#include "Manipulator.h"

#include "Cache/FormCache.h"
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

	static RE::TESObjectREFR* FindClosestFrom(RE::TESObjectREFR* from, 
		std::vector<RE::TESObjectREFR*>& nearby,
		float radius) 
	{
		auto it = nearby.end();
		for (auto current = nearby.begin();current < nearby.end(); ++current) {
			const float distance = (*current)->GetDistance(from);
			if (distance < radius) {
				radius = distance;
				it = current;
			}
		}
		if (it != nearby.end()) {
			auto* nearest = *it;
			nearby.erase(it);
			return nearest;
		}
		return nullptr;
	}

	static bool IsSmoke(RE::TESObjectREFR* smoke) {
		static const auto* cache = Cache::FormCache::GetSingleton();
		if (!cache) {
			return false;
		}

		const auto& smokes = cache->GetSmokes();
		const auto* base = smoke ? smoke->GetBaseObject() : nullptr;
		return base && smokes.contains(base->GetFormID());
	}

	static RE::TESForm* FindUnlitPair(RE::FormID id) {
		static const auto* cache = Cache::FormCache::GetSingleton();
		if (!cache) {
			return nullptr;
		}

		const auto& fireMap = cache->GetLitFires();
		if (fireMap.empty()) {
			return nullptr;
		}
		const auto found = fireMap.find(id);
		if (found == fireMap.end()) {
			return nullptr;
		}

		return RE::TESForm::LookupByID(found->second);
	}

	static RE::TESForm* FindLitPair(RE::FormID id) {
		static const auto* cache = Cache::FormCache::GetSingleton();
		if (!cache) {
			return nullptr;
		}

		const auto& fireMap = cache->GetUnlitFires();
		if (fireMap.empty()) {
			return nullptr;
		}
		const auto found = fireMap.find(id);
		if (found == fireMap.end()) {
			return nullptr;
		}

		return RE::TESForm::LookupByID(found->second);
	}
}

namespace FireManipulator
{
	void Extinguish(RE::TESObjectREFR* fire) {
		static auto* manipulator = Manipulator::GetSingleton();
		if (!manipulator || !fire) {
			return;
		}

		manipulator->Extinguish(fire);
	}

	void Relight(RE::TESObjectREFR* fire) {
		static auto* manipulator = Manipulator::GetSingleton();
		if (!manipulator || !fire) {
			return;
		}

		manipulator->Relight(fire);
	}

	void ExtinguishCell(RE::TESObjectCELL* cell) {
		static auto* manipulator = Manipulator::GetSingleton();
		if (!manipulator || !cell) {
			return;
		}

		manipulator->MassExtinguish(cell);
	}

	void Manipulator::MassExtinguish(RE::TESObjectCELL* cell) {
		assert(cell);

		bool squashLight = Settings::INI::GetSetting<bool>(
			Settings::INI::GENERAL_SQUASH_LIGHTS.data())
			.value_or(false);
		bool squashSmoke = Settings::INI::GetSetting<bool>(
			Settings::INI::GENERAL_SQUASH_SMOKE.data())
			.value_or(false);
		float lightDistance = Settings::INI::GetSetting<bool>(
			Settings::INI::GENERAL_LOOKUP_LIGHT.data())
			.value_or(250.0f);
		float smokeDistance = Settings::INI::GetSetting<bool>(
			Settings::INI::GENERAL_LOOKUP_SMOKE.data())
			.value_or(250.0f);

		std::vector<RE::TESObjectREFR*> foundSmokes;
		std::vector<RE::TESObjectREFR*> foundLights;
		std::vector<RE::TESObjectREFR*> foundLitFires;
		std::vector<RE::TESObjectREFR*> foundUnlitFires;

		foundSmokes.reserve(16);
		foundLights.reserve(16);
		foundLitFires.reserve(16);
		foundUnlitFires.reserve(16);

		auto* tasks = SKSE::GetTaskInterface();
		if (!tasks) {
			return; //what
		}

		cell->ForEachReference([&](RE::TESObjectREFR* ref) {
			using ContainerResult = RE::BSContainer::ForEachResult;
			if (!ref || !ref->Is3DLoaded() || ref->IsDisabled()) {
				return ContainerResult::kContinue;
			}

			const auto* base = ref ? ref->GetBaseObject() : nullptr;
			const auto formID = base ? base->GetFormID() : 0;
			if (formID == 0) {
				return RE::BSContainer::ForEachResult::kContinue;
			}

			if (IsSmoke(ref)) {
				foundSmokes.emplace_back(ref);
			}
			else if (base->Is(RE::FormType::Light)) {
				foundLights.emplace_back(ref);
			}
			else if (auto* unlit = FindUnlitPair(formID); unlit) {
				foundLitFires.push_back(ref);
			}
			else if (auto* lit = FindLitPair(formID); lit) {
				foundUnlitFires.emplace_back(ref);
			}
			return ContainerResult::kContinue;
		});

		for (auto* lit : foundLitFires) {
			auto id = lit->GetFormID();
			if (_frozen.contains(id)) {
				continue;
			}
			_frozen.insert(id);

			auto* litBase = lit->GetBaseObject();
			auto litBaseID = litBase ? litBase->GetFormID() : 0;
			auto* unlitBase = FindUnlitPair(litBaseID);
			auto* unlitBound = unlitBase ? skyrim_cast<RE::TESBoundObject*>(unlitBase) : nullptr;
			if (!unlitBound) {
				continue;
			}

			PendingData data;
			data.fire = lit;
			data.unlit = unlitBound;
			if (squashLight && !foundLights.empty()) {
				auto* candidate = FindClosestFrom(lit, foundLights, lightDistance);
				if (candidate) {
					data.light = candidate;
				}
			}
			if (squashSmoke && !foundSmokes.empty()) {
				auto* candidate = FindClosestFrom(lit, foundSmokes, smokeDistance);
				if (candidate) {
					data.smoke = candidate;
				}
			}

			_pending.emplace_back(std::move(data));
		}

		if (!queued) {
			queued = true;
			tasks->AddTask(reinterpret_cast<::TaskDelegate*>(this));
		}
	}

	void Manipulator::Extinguish(RE::TESObjectREFR* fire) {
		assert(fire);
		const auto formID = fire->GetFormID();

		auto* unlit = FindUnlitPair(formID);
		if (!unlit) {
			return;
		}

		auto* cell = fire->GetParentCell();
		if (!cell) {
			return;
		}
		if (!cell->cellState.all(RE::TESObjectCELL::CellState::kAttached)) {
			return;
		}

		if (_frozen.contains(formID)) {
			return;
		}
		_frozen.insert(formID);

		RE::TESObjectREFR* smoke = nullptr;

		float lastDistance = 9999.9f;
		cell->ForEachReference([&fire, &lastDistance, &smoke](RE::TESObjectREFR* ref) {
			if (!IsSmoke(ref) || !ref->Is3DLoaded()) {
				return RE::BSContainer::ForEachResult::kContinue;
			}
			const auto distance = ref->GetDistance(fire);
			if (distance < lastDistance) {
				lastDistance = distance;
				smoke = ref;
			}
			return RE::BSContainer::ForEachResult::kContinue;
		});
	}

	void Manipulator::Relight(RE::TESObjectREFR* fire) {
		
	}

	void Manipulator::Run() {
		if (running) {
			return;
		}
		running = true;

		for (const auto& pending : _pending) {
			ExtinguishImpl(pending);
		}
	}

	void Manipulator::Dispose() {
		_pending.clear();
		_frozen.clear();
		running = false;
		queued = false;
	}

	void Manipulator::ExtinguishImpl(const PendingData& data) {
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
		if (!vm->attachedScripts.contains(handle)) {
			placedOff->DeleteThis();
			return;
		}

		auto found = vm->attachedScripts.find(handle);
		if (found == vm->attachedScripts.end()) {
			placedOff->DeleteThis();
			return;
		}

		_frozen.insert(placedOff->GetFormID());
		placedOff->SetScale(lit->GetScale());
		placedOff->MoveTo(lit);
		placedOff->SetAngle(lit->GetAngle());

		bool errored = true;
		for (auto& script : found->second) {
			auto* info = script ? script->GetTypeInfo() : nullptr;
			if (!info || info->GetName() != "REF_ObjectRefOffController"sv) {
				continue;
			}

			auto relatedFlame = script->GetProperty("RelatedFlame");
			auto addExtProperty = script->GetProperty("RelatedObjects");
			auto dayAttached = script->GetProperty("DayAttached");
			if (!(relatedFlame && addExtProperty && dayAttached)) {
				continue;
			}

			RE::BSScript::PackValue(relatedFlame, lit);
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
			_frozen.erase(placedOff->GetFormID());
			placedOff->DeleteThis();
			return;
		}
	}
}