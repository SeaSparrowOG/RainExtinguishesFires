#include "AttachDetachListener.h"

#include "FireManager/FireRegistry.h"
#include "Settings/INI/INISettings.h"

namespace Events::AttachDetachEvent
{
	bool InitializeAttachDetachEventListener() {
		logger::info("  >Initializing Cell Attach/Detach Listener..."sv);
		auto* listener = AttachDetachEventListener::GetSingleton();
		if (!listener) {
			logger::error("    Failed to get the internal Actor Cell Event Listener."sv);
			return false;
		}
		return listener->Initialize();
	}

	bool AttachDetachEventListener::Initialize() {
		auto* scriptEventSource = RE::ScriptEventSourceHolder::GetSingleton();
		if (!scriptEventSource) {
			logger::error("    Failed to get the game's Script Event Source Holder."sv);
			return false;
		}
		scriptEventSource->AddEventSink<RE::TESCellAttachDetachEvent>(this);
		scriptEventSource->AddEventSink<RE::TESCellFullyLoadedEvent>(this);
		return true;
	}

	AttachDetachEventListener::NearestLightAndSmoke AttachDetachEventListener::GetNearestLightAndSmoke(RE::TESObjectREFR* a_from)
	{
		auto result = NearestLightAndSmoke();
		auto* parentCell = a_from ? a_from->GetParentCell() : nullptr;
		auto parentCellID = parentCell ? parentCell->GetFormID() : 0;
		if (parentCellID == 0) {
			return result;
		}

		bool searchForLight = Settings::INI::GetSetting<bool>(Settings::INI::GENERAL_SQUASH_LIGHTS.data()).value_or(false);
		bool searchForSmoke = Settings::INI::GetSetting<bool>(Settings::INI::GENERAL_SQUASH_SMOKE.data()).value_or(false);
		if (!searchForLight && !searchForSmoke) {
			return result;
		}
		return result;
	}

	void AttachDetachEventListener::SetExtinguishFires(bool a_extinguish) { _extinguishOnLoad = a_extinguish; }

	RE::BSEventNotifyControl AttachDetachEventListener::ProcessEvent(const RE::TESCellAttachDetachEvent* a_event,
		RE::BSTEventSource<RE::TESCellAttachDetachEvent>*)
	{
		if (!a_event) {
			return EventControl::kContinue;
		}

		const auto& ref = a_event->reference;
		auto* cell = ref->GetParentCell();
		auto cellID = cell ? cell->GetFormID() : 0;

		if (cellID != 0) {
			auto it = _cellData.find(cellID);
			if (it == _cellData.end()) {
				auto data = CellData();
				data.UpdateObject(ref.get(), a_event->attached);
				if (!data.IsEmpty()) {
					_cellData.emplace(cellID, std::move(data));
				}
			}
			else {
				auto& currentData = (*it).second;
				currentData.UpdateObject(ref.get(), a_event->attached);
				if (currentData.IsEmpty()) {
					_cellData.erase(it);
				}
			}
		}

		return EventControl::kContinue;
	}

	RE::BSEventNotifyControl AttachDetachEventListener::ProcessEvent(const RE::TESCellFullyLoadedEvent* a_event, 
		RE::BSTEventSource<RE::TESCellFullyLoadedEvent>*)
	{
		const auto* cell = a_event ? a_event->cell : nullptr;
		const auto cellID = cell ? cell->GetFormID() : 0;
		if (cellID != 0) {
			auto it = _cellData.find(cellID);
			if (it != _cellData.end()) {
				auto& cellData = it->second;
				cellData.Debug(cell);
			}
		}
		return EventControl::kContinue;
	}

	bool AttachDetachEventListener::CellData::IsEmpty() const {
		return _cellSmoke.empty() && _cellLight.empty() && _cellFires.empty();
	}

	void AttachDetachEventListener::CellData::Debug(const RE::TESObjectCELL* a_cell) const {
		LOG_DEBUG("Cell Loaded: {}"sv, clib_util::editorID::get_editorID(a_cell));
		if (!_cellSmoke.empty()) {
			LOG_DEBUG("  >Smoke Objects:"sv);
			for (const auto id : _cellSmoke) {
				const auto* ref = RE::TESForm::LookupByID<RE::TESObjectREFR>(id);
				const auto* base = ref->GetBaseObject();
				LOG_DEBUG("    {}"sv, clib_util::editorID::get_editorID(base));
			}
		}
		if (!_cellFires.empty()) {
			LOG_DEBUG("  >Fire Objects:"sv);
			for (const auto id : _cellFires) {
				const auto* ref = RE::TESForm::LookupByID<RE::TESObjectREFR>(id);
				const auto* base = ref->GetBaseObject();
				LOG_DEBUG("    {}"sv, clib_util::editorID::get_editorID(base));
			}
		}
		if (!_cellLight.empty()) {
			LOG_DEBUG("  >Light Objects:"sv);
			for (const auto id : _cellLight) {
				const auto* ref = RE::TESForm::LookupByID<RE::TESObjectREFR>(id);
				const auto* base = ref->GetBaseObject();
				LOG_DEBUG("    {}"sv, clib_util::editorID::get_editorID(base));
			}
		}
	}

	void AttachDetachEventListener::CellData::ExtinguishLocalFires() {
		if (_cellFires.empty()) {
			return;
		}

		static auto* registry = FireManager::FireRegistry::GetSingleton();
		for (const auto& id : _cellFires) {
			auto* fireRef = RE::TESForm::LookupByID<RE::TESObjectREFR>(id);
			if (!fireRef || fireRef->IsDisabled()) {
				continue;
			}
			
			auto pos = fireRef->GetPosition();
			auto additionalRefs = FindAdditionalRefs(pos);
			if (additionalRefs.light) {
				_boundReferences.insert(additionalRefs.light->GetFormID());
			}
			if (additionalRefs.smoke) {
				_boundReferences.insert(additionalRefs.smoke->GetFormID());
			}
			registry->ExtinguishFire(fireRef, { additionalRefs.light, additionalRefs.smoke });
		}
		_boundReferences.clear();
	}

	void AttachDetachEventListener::CellData::UpdateObject(RE::TESObjectREFR* a_ref, bool a_attached) {
		auto* base = a_ref->GetBaseObject();
		auto baseID = base ? base->GetFormID() : 0;
		if (baseID == 0) {
			return;
		}

		std::vector<RE::FormID>* targetCache = nullptr;
		if (FireManager::IsSmokeObject(base)) {
			targetCache = &_cellSmoke;
		}
		else if (FireManager::IsFireObject(base)) {
			targetCache = &_cellFires;
		}
		else if (base->GetFormType() == RE::FormType::Light) {
			targetCache = &_cellLight;
		}

		if (!targetCache) {
			return;
		}

		auto& vec = *targetCache;
		if (a_attached) {
			auto cacheEnd = targetCache->end();
			auto refID = a_ref->GetFormID();
			if (std::find(targetCache->begin(), cacheEnd, refID) == cacheEnd) {
				targetCache->emplace_back(refID);
			}
		}
		else {
			std::erase_if(vec, [refID = a_ref->GetFormID()](const RE::FormID id) {
				return id == refID;
			});
		}
	}

	AttachDetachEventListener::CellData::AdditionalRefs AttachDetachEventListener::CellData::FindAdditionalRefs(const RE::NiPoint3& a_from)
	{
		AdditionalRefs result;
		if (!_cellLight.empty()) {
			const float lookupDistance = Settings::INI::GetSetting<float>(Settings::INI::GENERAL_LOOKUP_LIGHT.data()).value_or(600.0f);
			result.light = NearestRefOfType(a_from, _cellLight, lookupDistance);
		}
		if (!_cellSmoke.empty()) {
			const float lookupDistance = Settings::INI::GetSetting<float>(Settings::INI::GENERAL_LOOKUP_SMOKE.data()).value_or(600.0f);
			result.smoke = NearestRefOfType(a_from, _cellSmoke, lookupDistance);
		}
		return result;
	}

	RE::TESObjectREFR* AttachDetachEventListener::CellData::NearestRefOfType(const RE::NiPoint3& a_from, 
		std::vector<RE::FormID>& a_lookIn, 
		float a_distance)
	{
		a_distance *= a_distance;
		RE::TESObjectREFR* a_result = nullptr;
		for (const auto& id : a_lookIn) {
			if (_boundReferences.contains(id)) {
				continue;
			}
			auto* ref = RE::TESForm::LookupByID<RE::TESObjectREFR>(id);
			if (!ref || ref->IsDisabled()) {
				continue;
			}
			const float distanceSquared = a_from.GetSquaredDistance(ref->GetPosition());
			if (distanceSquared > a_distance) {
				continue;
			}
			a_result = ref;
			a_distance = distanceSquared;
		}
		return a_result;
	}
}