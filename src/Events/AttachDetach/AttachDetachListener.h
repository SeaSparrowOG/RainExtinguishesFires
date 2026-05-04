#pragma once

namespace Events
{
	namespace AttachDetachEvent
	{
		bool InitializeAttachDetachEventListener();

		class AttachDetachEventListener :
			public REX::Singleton<AttachDetachEventListener>,
			public RE::BSTEventSink<RE::TESCellAttachDetachEvent>,
			public RE::BSTEventSink<RE::TESCellFullyLoadedEvent>
		{
		public:
			bool Initialize();

			struct NearestLightAndSmoke
			{
				RE::TESObjectREFR* smoke = nullptr;
				RE::TESObjectREFR* light = nullptr;
			};
			NearestLightAndSmoke GetNearestLightAndSmoke(RE::TESObjectREFR* a_from);

			void SetExtinguishFires(bool a_extinguish);

		private:
			class CellData
			{
			public:
				CellData() {
					_cellSmoke.reserve(vecAllocSize);
					_cellLight.reserve(vecAllocSize);
					_cellFires.reserve(vecAllocSize);
				}

				bool IsEmpty() const;
				void Debug(const RE::TESObjectCELL* a_cell) const;

				void ExtinguishLocalFires();
				void UpdateObject(RE::TESObjectREFR* a_ref, bool a_attached);

			private:
				std::vector<RE::FormID> _cellSmoke = {};
				std::vector<RE::FormID> _cellLight = {};
				std::vector<RE::FormID> _cellFires = {};
				std::unordered_set<RE::FormID> _boundReferences = {};

				inline static constexpr std::size_t vecAllocSize = 10u;

				struct AdditionalRefs
				{
					RE::TESObjectREFR* light = nullptr;
					RE::TESObjectREFR* smoke = nullptr;
				};
				AdditionalRefs FindAdditionalRefs(const RE::NiPoint3& a_from);
				RE::TESObjectREFR* NearestRefOfType(const RE::NiPoint3& a_from, std::vector<RE::FormID>& a_lookIn, float a_distance);
			};

			using EventControl = RE::BSEventNotifyControl;
			using CellToReferencesMap = std::unordered_map<RE::FormID, std::vector<RE::FormID>>;
			RE::BSEventNotifyControl ProcessEvent(const RE::TESCellAttachDetachEvent* a_event, RE::BSTEventSource<RE::TESCellAttachDetachEvent>*) override;
			RE::BSEventNotifyControl ProcessEvent(const RE::TESCellFullyLoadedEvent* a_event, RE::BSTEventSource<RE::TESCellFullyLoadedEvent>*) override;

			bool                                     _extinguishOnLoad = false;
			std::unordered_map<RE::FormID, CellData> _cellData = {};
		};
	}
}