#include "FireRegistry.h"

namespace FireManager
{
	bool IsSmokeObject(const RE::TESBoundObject* a_baseObject) {
		static const auto* registry = FireRegistry::GetSingleton();
		return registry->IsObjectSmokeObject(a_baseObject);
	}

	bool IsFireObject(const RE::TESBoundObject* a_baseObject) {
		static const auto* registry = FireRegistry::GetSingleton();
		return registry->IsObjectFlameObject(a_baseObject);
	}

	bool IsUnlitFireObject(const RE::TESBoundObject* a_baseObject) {
		static const auto* registry = FireRegistry::GetSingleton();
		return registry->IsObjectUnlitFlameObject(a_baseObject);
	}

	bool FireRegistry::IsObjectSmokeObject(const RE::TESBoundObject* a_baseObject) const {
		auto formID = a_baseObject->GetFormID();
		return _smokeObjects.contains(formID);
	}

	bool FireRegistry::IsObjectFlameObject(const RE::TESBoundObject* a_baseObject) const {
#ifndef NDEBUG
		static constexpr std::size_t emberCount = 7u;
		static constexpr std::array<RE::FormID, emberCount> emberIDs = {
			0xCD824,
			0xCD823,
			0xAA71C,
			0x33DA4,
			0x33DA9,
			0x4318B,
			0x1092E2
		};

		static std::array<RE::TESBoundObject*, emberCount> embers = {};
		if (embers.at(0) == nullptr) {
			for (std::size_t i = 0; i < emberCount; ++i) {
				auto id = emberIDs.at(i);
				auto ember = RE::TESForm::LookupByID<RE::TESBoundObject>(id);
				if (!ember) {
					SKSE::stl::report_and_fail(fmt::format("Failed to get flame: {}", i));
				}
				embers.at(i) = ember;
			}
		}

		for (const auto* ember : embers) {
			if (ember == a_baseObject) {
				return true;
			}
		}
#endif
		auto formID = a_baseObject->GetFormID();
		return _flameObjects.contains(formID);
	}

	bool FireRegistry::IsObjectUnlitFlameObject(const RE::TESBoundObject* a_baseObject) const {
		auto formID = a_baseObject->GetFormID();
		return _unlitFlameObjects.contains(formID);
	}

	bool FireRegistry::ExtinguishFire(RE::TESObjectREFR* a_fire, 
		const std::vector<RE::TESObjectREFR*>& a_additionalExtinguishes)
	{
		auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
		auto* handlePolicy = vm ? vm->GetObjectHandlePolicy() : nullptr;
		if (!handlePolicy) {
			logger::warn("Failed to fetch the game's VM or HandlePolicy."sv);
			return false;
		}

		const auto fireID = a_fire->GetFormID();
		if (_currentlyChangingState.contains(fireID)) {
			return false;
		}

		const auto& xLists = a_fire->extraList;
		const bool hasEnableChildren = xLists.HasType<RE::ExtraEnableStateChildren>();
		if (hasEnableChildren) {
			return false;
		}
		const bool hasEnableParents = xLists.HasType<RE::ExtraEnableStateParent>();
		if (hasEnableParents) {
			return false;
		}

		_currentlyChangingState.insert(fireID);
		auto* baseForm = a_fire->GetBaseObject();
		auto* offForm = GetOffForm(baseForm);

		auto offRef = a_fire->PlaceObjectAtMe(offForm, false);
		_currentlyChangingState.insert(offRef->GetFormID());

		offRef->MoveTo(a_fire);
		offRef->data.angle = a_fire->data.angle;
		offRef->SetScale(a_fire->GetScale());

		RE::VMHandle handle = handlePolicy->GetHandleForObject(offRef->GetFormType(), offRef.get());
		if (!handle || !vm->attachedScripts.contains(handle)) {
			_currentlyChangingState.erase(offRef->GetFormID());
			_currentlyChangingState.erase(fireID);
			offRef->Disable();
			offRef->DeleteThis();
			return false;
		}

		auto it = vm->attachedScripts.find(handle);
		if (it == vm->attachedScripts.end()) {
			_currentlyChangingState.erase(offRef->GetFormID());
			_currentlyChangingState.erase(fireID);
			offRef->Disable();
			offRef->DeleteThis();
			return false;
		}

		auto& scripts = it->second;
		RE::BSReadWriteLock lock;
		lock.LockForRead();
		bool sent = false;
		for (auto scriptIt = scripts.begin(); !sent && scriptIt != scripts.end(); ++scriptIt) {
			const auto& script = *scriptIt;
			const auto info = script ? script->GetTypeInfo() : nullptr;
			if (!info || strcmp("REF_ObjectRefOffController", info->GetName()) != 0) {
				continue;
			}

			auto callback = RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor>();
			auto args = RE::MakeFunctionArguments(
				static_cast<RE::TESObjectREFR*>(a_fire),
				static_cast<RE::TESObjectREFR*>(a_additionalExtinguishes.at(0)),
				static_cast<RE::TESObjectREFR*>(a_additionalExtinguishes.at(1))
			);
			const RE::BSFixedString functionName = "Extinguish";
			auto scriptObject = script.get();
			auto object = RE::BSTSmartPointer<RE::BSScript::Object>(scriptObject);
			vm->DispatchMethodCall(object, functionName, args, callback);
			sent = true;
			break;
		}
		lock.UnlockForRead();

		if (!sent) {
			_currentlyChangingState.erase(offRef->GetFormID());
			_currentlyChangingState.erase(fireID);
			offRef->Disable();
			offRef->DeleteThis();
		}
		return sent;
	}

	RE::TESBoundObject* FireRegistry::GetOffForm(RE::TESBoundObject* a_litForm) const {
		return nullptr;
	}
}