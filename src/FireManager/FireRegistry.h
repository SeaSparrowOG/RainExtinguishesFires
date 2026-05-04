#pragma once

namespace FireManager
{
	bool IsSmokeObject(const RE::TESBoundObject* a_baseObject);
	bool IsFireObject(const RE::TESBoundObject* a_baseObject);
	bool IsUnlitFireObject(const RE::TESBoundObject* a_baseObject);

	bool ExtinguishFire(RE::TESObjectREFR* a_fire, const std::vector<RE::TESObjectREFR*>& a_additionalExtinguishes);

	class FireRegistry : public REX::Singleton<FireRegistry>
	{
	public:
		bool IsObjectSmokeObject(const RE::TESBoundObject* a_baseObject) const;
		bool IsObjectFlameObject(const RE::TESBoundObject* a_baseObject) const;
		bool IsObjectUnlitFlameObject(const RE::TESBoundObject* a_baseObject) const;

		bool ExtinguishFire(RE::TESObjectREFR* a_fire, const std::vector<RE::TESObjectREFR*>& a_additionalExtinguishes);
	private:
		std::unordered_set<RE::FormID> _smokeObjects{};
		std::unordered_set<RE::FormID> _flameObjects{};
		std::unordered_set<RE::FormID> _unlitFlameObjects{};

		std::unordered_set<RE::FormID> _currentlyChangingState{};

		RE::TESBoundObject* GetOffForm(RE::TESBoundObject* a_litForm) const;
	};
}