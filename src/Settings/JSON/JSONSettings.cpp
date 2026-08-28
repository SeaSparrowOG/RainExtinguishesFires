#include "JSONSettings.h"

namespace Settings::JSON
{
	bool Holder::Load() {
		Release();

		std::string jsonFolder = fmt::format(R"(.\Data\SKSE\Plugins\{})"sv, Plugin::NAME);
		logger::INFO("  >Settings folder: {}."sv, jsonFolder);
		if (!std::filesystem::exists(jsonFolder)) {
			logger::INFO("    >No settings folder found."sv);
			return true;
		}

		std::vector<std::string> paths{};
		try {
			for (const auto& entry : std::filesystem::directory_iterator(jsonFolder)) {
				if (entry.is_regular_file() && entry.path().extension() == ".json") {
					paths.push_back(entry.path().string());
				}
			}

			std::sort(paths.begin(), paths.end());
			logger::INFO("    >Found {} configuration files."sv, std::to_string(paths.size()));
		}
		catch (const std::exception& e) {
			logger::WARN("Caught {} while reading files."sv, e.what());
			return false;
		}

		if (paths.empty()) {
			logger::INFO("    >No settings found"sv);
			return true;
		}

		bool success = true;
		for (const auto& path : paths) {
			auto configName = path.substr(jsonFolder.size() + 1, path.size() - 1);
			logger::INFO("    >Reading config {}..."sv, configName);
			Json::CharReaderBuilder builder;
			builder["collectComments"] = false;

			_currentStatus = ConfigStatus();
			try {
				std::ifstream rawJSON(path);
				if (!rawJSON.is_open()) {
					logger::WARN("      >Failed to open: {}"sv, path);
					success = false;
					continue;
				}

				std::string errs;
				Json::Value JSONFile;
				if (!Json::parseFromStream(builder, rawJSON, &JSONFile, &errs)) {
					logger::WARN("      >Failed to parse {}: {}", path, errs);
					success = false;
					continue;
				}

				if (JSONFile.isObject() || JSONFile.isArray()) {
					CanonicalizeObject(JSONFile, configName);
				}

				if (_currentStatus.IsValid()) {
					_configs.emplace(configName, std::move(JSONFile));
				}
				else {
					success = false;
					_errors.emplace(configName, _currentStatus);
				}
			}
			catch (const std::exception& e) {
				logger::WARN("Caught {} while reading files.", e.what());
				success = false;
				continue;
			}
		}

		logger::INFO("Finished reading all settings."sv);
		return success;
	}

	void Holder::Release() {
		_errors.clear();
		_configs.clear();
		_currentStatus = ConfigStatus();
	}

	void Holder::LogErrors() const {
		logger::WARN("Finished preloading JSON settings. Found {} problematic configs:", _errors.size());
		for (const auto& [config, error] : _errors) {
			logger::WARN("  >{}"sv, config);
			if (!error.deeplyNestedObjects.empty()) {
				logger::WARN("    The following fields were nested too deeply:"sv);
				for (const auto& instance : error.deeplyNestedObjects) {
					logger::WARN("      - {}"sv, instance);
				}
			}
			if (!error.duplicateKeys.empty()) {
				logger::WARN("    The following fields were defined multiple times:"sv);
				for (const auto& instance : error.duplicateKeys) {
					logger::WARN("      - {}"sv, instance);
				}
			}
			if (!error.emptyObjects.empty()) {
				logger::WARN("    The following fields were empty:"sv);
				for (const auto& instance : error.emptyObjects) {
					logger::WARN("      - {}"sv, instance);
				}
			}
		}
	}

	Holder::StringMap Holder::GatherData(const Json::Value& a_obj, const std::string& a_path) {
		StringMap mappings;

		auto members = a_obj.getMemberNames();
		auto compositePath = a_path;
		auto trimTo = compositePath.size();
		std::unordered_set<std::string> seen;
		std::unordered_set<std::string> clearFromMappings;

		for (const auto& key : members) {
			auto lower = clib_util::string::tolower(key);
			if (seen.contains(lower)) {
				compositePath.push_back('|');
				compositePath += key;
				_currentStatus.duplicateKeys.push_back(compositePath);
				compositePath.resize(trimTo);

				for (auto it = mappings.begin(); it != mappings.end(); ++it) {
					const auto& [previousKey, previousLower] = *it;
					if (lower != previousLower) {
						continue;
					}
					clearFromMappings.insert(previousKey);
					break;
				}
				continue;
			}

			const auto& val = a_obj[key];
			if ((val.isArray() || val.isObject()) && val.empty()) {
				compositePath.push_back('|');
				compositePath += key;
				_currentStatus.emptyObjects.push_back(compositePath);
				compositePath.resize(trimTo);
				continue;
			}

			seen.insert(lower);
			mappings.emplace(key, lower);
		}

		if (!clearFromMappings.empty()) {
			for (const auto& toClear : clearFromMappings) {
				mappings.erase(toClear);
			}
		}
		return mappings;
	}

	void Holder::ShallowCanonicalization(Json::Value& a_obj, const StringMap& a_mappings) {
		for (const auto& [key, lowercase] : a_mappings) {
			if (key == lowercase) {
				continue;
			}
			a_obj[lowercase] = std::move(a_obj[key]);
			a_obj.removeMember(key);
		}
	}

	void Holder::CanonicalizeObject(Json::Value& a_obj, std::string& a_path, std::size_t a_depth) {
		assert(a_obj.isObject() || a_obj.isArray());
		assert(!a_obj.empty());

		if (a_depth > RECURSION_LIMIT) {
			_currentStatus.deeplyNestedObjects.push_back(a_path);
			return;
		}

		auto trimTo = a_path.size();
		if (a_obj.isArray()) {
			a_path.push_back('[');
			auto arrayTrimTo = trimTo + 1u;
			for (Json::Value::ArrayIndex i = 0u; i < a_obj.size(); ++i) {
				auto& val = a_obj[i];
				if (!val.isObject() && !val.isArray()) {
					continue;
				}

				a_path += std::to_string(i);
				a_path.push_back(']');
				if (val.empty()) {
					_currentStatus.emptyObjects.push_back(a_path);
				}
				else {
					CanonicalizeObject(val, a_path, a_depth + 1u);
				}
				a_path.resize(arrayTrimTo);
			}
			a_path.resize(trimTo);
			return;
		}

		auto mappings = GatherData(a_obj, a_path);
		if (mappings.empty()) {
			return;
		}
		ShallowCanonicalization(a_obj, mappings);

		for (const auto& pair : mappings) {
			const auto& lowerKey = pair.second;

			auto& val = a_obj[lowerKey];
			if (!val.isObject() && !val.isArray()) {
				continue;
			}
			assert(!val.empty());
			a_path.push_back('|');
			a_path += lowerKey;
			CanonicalizeObject(val, a_path, a_depth + 1u);
			a_path.resize(trimTo);
		}
	}

	bool Preload() {
		logger::INFO("Preloading JSON settings..."sv);
		auto* manager = Holder::GetSingleton();
		if (!manager) {
			logger::WARN("  >Failed to fetch internal JSON settings holder."sv);
			return false;
		}
		if (!manager->Load()) {
			manager->LogErrors();
			return false;
		}
		logger::INFO("Finished preloading JSON settings."sv);
		return true;
	}
}