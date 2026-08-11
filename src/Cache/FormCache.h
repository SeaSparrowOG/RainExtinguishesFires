#pragma once

namespace Cache
{
	class FormCache final : public REX::Singleton<FormCache>
	{
	public:
		using Stored = RE::FormID;
		using Fires = std::unordered_map<Stored, Stored>;
		using Smokes = std::unordered_set<Stored>;

		[[nodiscard]] bool Initialize();

		const Fires& GetLitFires() const { return _litFires; }
		const Fires& GetUnlitFires() const { return _unlitFires; }
		const Smokes& GetSmokes() const { return _smokes; }

	private:
		[[nodiscard]] bool ParseObject(const Json::Value& obj);
		[[nodiscard]] bool ParseArray(const Json::Value& arr);

		Fires _litFires;
		Fires _unlitFires;
		Smokes _smokes;
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