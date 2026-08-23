#pragma once

#include "Cache/FormCache.h"

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
		ReferenceType                   type = ReferenceType::None;
		std::optional<Cache::UnlitData> data = std::nullopt;
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
			void RelightRef(const RE::FormID id);

			void Freeze(const RE::FormID id);
			void UnFreeze(const RE::FormID id);

			void Run() override;
			void Dispose() override;

		private:
			enum class ActionType
			{
				Extinguish,
				Relight
			};

			struct PendingData
			{
				ActionType          type = ActionType::Extinguish;

				RE::TESObjectREFR*  light = nullptr;
				RE::TESObjectREFR*  smoke = nullptr;
				RE::TESObjectREFR*  fire = nullptr;
				RE::TESBoundObject* unlit = nullptr;

				std::optional<bool>  overrideOcclusion = std::nullopt;
				std::optional<float> sizeOverride = std::nullopt;
			};

			void               RelightImpl(const PendingData& data);
			void               ExtinguishImpl(const PendingData& data);
			RE::TESObjectREFR* FindClosestFrom(RE::TESObjectREFR* from,
												const std::vector<RE::FormID>& nearby,
												float radius);

			std::vector<RE::FormID> smokes;
			std::vector<RE::FormID> lights;
			std::vector<RE::FormID> litFires;
			std::vector<RE::FormID> unlitFires;

			std::unordered_set<RE::FormID>                transitioningFires;
			std::unordered_set<RE::FormID>                reservedSmokesAndLights;
			std::unordered_map<RE::FormID, ReferenceType> seen;

			std::stack<PendingData> _pendingExtinguishes;

			bool  squashLight;
			bool  squashSmoke;
			bool  checkOcclusion;
			float lightDistance;
			float smokeDistance;
			float resetDays;

			bool _queued = false;
		};
	}
}