#include "ModObjectManager.h"

namespace Data
{
	bool ModObjectManager::PreLoad() {
		logger::INFO("  >Looking for script {} on quest {}..."sv, ScriptName, QuestName);
		const auto quest = RE::TESForm::LookupByEditorID<RE::TESQuest>(QuestName);
		if (!quest) {
			if (EXPECTED_OBJECTS.empty()) {
				logger::INFO("    >No need to preload mod objects."sv);
				return true;
			}
			logger::CRITICAL("  >Failed to lookup quest."sv);
			return false;
		}

		const auto vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
		if (!vm) {
			logger::CRITICAL("  >Failed to get VM"sv);
			return false;
		}

		const auto bindPolicy = vm->GetObjectBindPolicy();
		const auto handlePolicy = vm->GetObjectHandlePolicy();
		if (!bindPolicy || !handlePolicy) {
			logger::CRITICAL("  >Failed to get VM object policies"sv);
			return false;
		}

		const auto handle = handlePolicy->GetHandleForObject(RE::TESQuest::FORMTYPE, quest);
		RE::BSTScrapHashMap<RE::BSFixedString, RE::BSScript::Variable> properties;
		std::uint32_t nonConverted;
		bindPolicy->GetInitialPropertyValues(handle, ScriptName, properties, nonConverted);
		objects.clear();

		for (const auto& [name, var] : properties) {
			if (var.IsObjectArray()) {
				auto varArray = var.GetArray();

				RE::BSScript::Array::size_type index = 0;
				for (const auto& element : *varArray) {
					if (const auto foundForm = element.Unpack<RE::TESForm*>()) {
						auto keyName = fmt::format<std::string>("{}|{}", std::to_string(index), name.c_str());
						objects.emplace(keyName, foundForm);
					}
					else {
						logger::CRITICAL("  >Undefined element in array propery {}."sv, name.c_str());
						return false;
					}
					++index;
				}
			}
			else if (var.IsObject()) {
				const auto value = var.Unpack<RE::TESForm*>();
				if (!value) {
					logger::CRITICAL("  >Property {} is null."sv, name.c_str());
					return false;
				}
				objects.emplace(name, value);
			}
			else {
				logger::CRITICAL("  >Undefined property {}", name.c_str());
				return false;
			}
		}

		vm->ResetAllBoundObjects(handle);
		logger::INFO("  >Found {} Mod Objects."sv, properties.size());
		logger::INFO("Done."sv);
		return Verify();
	}

	RE::TESForm* ModObjectManager::Get(std::string_view a_key) const {
		if (const auto it = objects.find(a_key); it != objects.end()) {
			return it->second;
		}
		REX::FAIL(fmt::format("Mod Object {} was requested, but was not found."sv, a_key));
		return nullptr;
	}

	bool ModObjectManager::Verify() {
		logger::INFO("Verifying discovered objects:"sv);
		bool foundAll = true;
		for (const auto* objectName : EXPECTED_OBJECTS) {
			if (!objects.contains(std::string(objectName))) {
				foundAll = false;
				logger::CRITICAL("  >Failed to find {}.", objectName);
			}
			else {
				logger::INFO("  >Found {}", objectName);
			}
		}
		return foundAll;
	}

	bool PreloadModObjects() {
		logger::INFO("Preloading Mod Objects..."sv);
		auto* objectManager = ModObjectManager::GetSingleton();
		if (!objectManager) {
			logger::CRITICAL("  >Failed to get Mod Object manager."sv);
			return false;
		}
		return objectManager->PreLoad();
	}
}