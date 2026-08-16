#pragma once

namespace Cache
{
	class FormCache final : public REX::Singleton<FormCache>
	{
	public:
		[[nodiscard]] bool Initialize();

		struct Overrides
		{
			std::optional<float> _sizeFactor = std::nullopt;
		};

		const std::unordered_set<RE::FormID>&             GetSmokes() const { return _smokes; }
		const std::unordered_set<RE::FormID>&             GetUnlitFires() const { return _unlitFires; }
		const std::unordered_map<RE::FormID, Overrides>&  GetOverrides() const { return _overrideData; }
		const std::unordered_map<RE::FormID, RE::FormID>& GetLitFires() const { return _litFires; }

	private:
		[[nodiscard]] bool ParseObject(const Json::Value& obj);
		[[nodiscard]] bool ParseArray(const Json::Value& arr);

		std::unordered_set<RE::FormID>             _smokes;
		std::unordered_set<RE::FormID>             _unlitFires;
		std::unordered_map<RE::FormID, Overrides>  _overrideData;
		std::unordered_map<RE::FormID, RE::FormID> _litFires;
	};

	[[nodiscard]]
	static inline bool InitializeCache() {
		logger::info("Initialiazing Form Cache..."sv);
		auto* cache = FormCache::GetSingleton();
		if (!cache) {
			logger::critical("  - Failed to retrieve internal Form Cache."sv);
			return false;
		}
		return cache->Initialize();
	}
}