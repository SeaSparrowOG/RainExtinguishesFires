#pragma once

namespace FireManipulator
{
	enum class ReferenceType
	{
		LitFire,
		UnlitFire,
		Smoke,
		Light,
		None
	};

	struct ObjectData
	{
		ReferenceType       type = ReferenceType::None;
		RE::TESBoundObject* pair = nullptr;
	};

	namespace CellData
	{

		class CellData final : public SKSE::detail::TaskDelegate
		{
		public:
			CellData();

			void OnCellAttachDetach(RE::TESObjectCELL* cell);
			void ClearRef(const RE::FormID id);
			void ProcessRef(RE::TESObjectREFR* ref, const ObjectData& data);
			void ExtinguishRef(const RE::FormID id);

			void Freeze(const RE::FormID id);
			void UnFreeze(const RE::FormID id);

			void Run() override;
			void Dispose() override;

		private:
			struct PendingData
			{
				RE::TESBoundObject* unlit = nullptr;
				RE::TESObjectREFR* light = nullptr;
				RE::TESObjectREFR* smoke = nullptr;
				RE::TESObjectREFR* fire = nullptr;
			};

			void               ExtinguishImpl(const PendingData& data);
			RE::TESObjectREFR* FindClosestFrom(RE::TESObjectREFR* from,
												const std::vector<RE::FormID>& nearby,
												float radius);

			std::vector<RE::FormID> litFires;
			std::vector<RE::FormID> unlitFires;
			std::vector<RE::FormID> smokes;
			std::vector<RE::FormID> lights;

			std::unordered_set<RE::FormID>                transitioningFires;
			std::unordered_set<RE::FormID>                reservedSmokesAndLights;
			std::unordered_map<RE::FormID, ReferenceType> seen;

			std::stack<PendingData> _pendingExtinguishes;

			bool  squashLight;
			bool  squashSmoke;
			float lightDistance;
			float smokeDistance;

			bool _queued = false;
		};
	}
}