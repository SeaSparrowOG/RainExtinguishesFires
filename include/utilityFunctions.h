#pragma once

namespace UtilityFunction {
	bool IsModPresent(std::string a_modName) {
		return RE::TESDataHandler::GetSingleton()->LookupModByName(a_modName) ? true : false;
	}

	bool IsHex(std::string const& s) {
		return s.compare(0, 2, "0x") == 0
			&& s.size() > 2
			&& s.find_first_not_of("0123456789abcdefABCDEF", 2) == std::string::npos;
	}

	RE::TESBoundObject* ParseForm(const std::string& a_identifier) {
		if (const auto splitID = clib_util::string::split(a_identifier, "|"); splitID.size() == 2) {
			if (!IsHex(splitID[0])) return nullptr;
			const auto  formID = clib_util::string::to_num<RE::FormID>(splitID[0], true);

			const auto& modName = splitID[1];
			if (!IsModPresent(modName)) return nullptr;

			auto* baseForm = RE::TESDataHandler::GetSingleton()->LookupForm(formID, modName);
			return baseForm ? static_cast<RE::TESBoundObject*>(baseForm) : nullptr;
		}
		auto* form = RE::TESBoundObject::LookupByEditorID(a_identifier);
		if (form) return static_cast<RE::TESBoundObject*>(form);
		return nullptr;
	}
}