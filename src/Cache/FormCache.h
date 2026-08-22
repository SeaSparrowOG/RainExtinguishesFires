#pragma once

namespace Cache
{
	struct UnlitData
	{
		RE::FormID           _offFireID = 0;
		std::optional<bool>  _forceOcclusionCheck = std::nullopt;
		std::optional<float> _resizeByPercent = std::nullopt;
	};

	class FormCache final : public REX::Singleton<FormCache>
	{
	public:
		[[nodiscard]] bool Initialize();

		const std::unordered_set<RE::FormID>&             GetSmokes() const { return _smokes; }
		const std::unordered_set<RE::FormID>&             GetUnlitFires() const { return _unlitFires; }
		const std::unordered_map<RE::FormID, UnlitData>&  GetUnlitData() const { return _unlitData; }

	private:
		[[nodiscard]] bool ParseObject(const Json::Value& obj);
		[[nodiscard]] bool ParseArray(const Json::Value& arr);

		std::unordered_set<RE::FormID>             _smokes;
		std::unordered_set<RE::FormID>             _unlitFires;
		std::unordered_map<RE::FormID, UnlitData>  _unlitData;
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