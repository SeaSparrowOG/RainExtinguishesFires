#include "FormCache.h"

#include "Settings/JSON/JSONSettings.h"

#include <expected>

namespace {
	enum class FailureReason
	{
		ModNotLoaded,

		InvalidStringSize,
		MissingFormID,
		DataHandlerError
	};

	bool IsFailureFatal(FailureReason reason) {
		switch (reason) {
		case FailureReason::ModNotLoaded: return false;
		default: return true;
		}
	}

	std::expected<RE::TESForm*, FailureReason> FormFromString(const std::string& str) {
		static auto* dh = RE::TESDataHandler::GetSingleton();
		if (!dh) {
			return nullptr;
		}

		const auto parts = clib_util::string::split(str, "|"sv);
		if (parts.size() == 2u) {
			std::string_view file;
			RE::FormID formID = 0u;
			if (clib_util::string::is_only_hex(parts[0])) {
				formID = clib_util::string::to_num<RE::FormID>(parts[0]);
				file = parts[1];
			}
			else if (clib_util::string::is_only_hex(parts[1])) {
				formID = clib_util::string::to_num<RE::FormID>(parts[1]);
				file = parts[0];
			}
			else {
				return std::unexpected(FailureReason::MissingFormID);
			}

			if (!dh->LookupModByName(file)) {
				return std::unexpected(FailureReason::ModNotLoaded);
			}

			return dh->LookupForm(formID, file);
		}
		else if (parts.size() == 1u) {
			return RE::TESForm::LookupByEditorID(str);
		}
		return std::unexpected(FailureReason::InvalidStringSize);
	}
}

namespace Cache
{
	bool FormCache::Initialize() {
		static auto* settings = Settings::JSON::Holder::GetSingleton();
		if (!settings) {
			logger::critical("  - Failed to get interla Settings singleton."sv);
			return false;
		}

		bool success = true;
		const auto& configs = settings->GetConfigs();
		for (const auto& [name, config] : configs) {
			if (config.isArray()) {
				if (!ParseArray(config)) {
					logger::critical("  - Error reading config: {}"sv, name);
					success = false;
				}
			}
			else if (config.isObject()) {
				if (!ParseObject(config)) {
					logger::critical("  - Error reading config: {}"sv, name);
				}
			}
		}

		return success;
	}

	bool FormCache::ParseObject(const Json::Value& obj) {
		const auto& litRaw = obj["lit"];
		const auto& unlitRaw = obj["unlit"];
		const auto& smokeRaw = obj["smoke"];

		if (litRaw && unlitRaw) {
			auto parsedLit = FormFromString(litRaw.asString());
			auto parsedUnlit = FormFromString(unlitRaw.asString());

			if (!parsedLit) {
				return !IsFailureFatal(parsedLit.error());
			}
			else if (!parsedUnlit) {
				return !IsFailureFatal(parsedUnlit.error());
			}

			auto* lit = parsedLit.value();
			auto* unlit = parsedUnlit.value();
			if (!lit || !unlit) {
				return true;
			}
			
			if (!lit->Is(RE::FormType::MovableStatic) && !lit->Is(RE::FormType::Static)) {
				return false;
			}
			else if (!unlit->Is(RE::FormType::MovableStatic) && !unlit->Is(RE::FormType::Static)) {
				return false;
			}
			auto litID = lit->GetFormID();
			auto unlitID = unlit->GetFormID();

			_litFires[litID] = unlitID; // Intentional overwrite.
			_unlitFires[unlitID] = litID; // Same - just bad biderectionality.
			return true;
		}
		else if (smokeRaw) {
			auto parsedSmoke = FormFromString(smokeRaw.asString());
			if (!parsedSmoke) {
				return !IsFailureFatal(parsedSmoke.error());
			}

			auto* smoke = parsedSmoke.value();
			if (!smoke) {
				return true;
			}
			if (!smoke->Is(RE::FormType::MovableStatic) && !smoke->Is(RE::FormType::Static)) {
				return false;
			}
			_smokes.insert(smoke->GetFormID());
			return true;
		}
		return false;
	}

	bool FormCache::ParseArray(const Json::Value& arr) {
		bool success = true;
		for (const auto& val : arr) {
			if (!val.isObject()) {
				continue;
			}
			success &= ParseObject(val);
		}
		return success;
	}
}