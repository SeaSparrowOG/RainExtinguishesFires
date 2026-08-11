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

		private:
			using EventControl = RE::BSEventNotifyControl;
			using CellToReferencesMap = std::unordered_map<RE::FormID, std::vector<RE::FormID>>;
			RE::BSEventNotifyControl ProcessEvent(const RE::TESCellAttachDetachEvent* a_event, RE::BSTEventSource<RE::TESCellAttachDetachEvent>*) override;
			RE::BSEventNotifyControl ProcessEvent(const RE::TESCellFullyLoadedEvent* a_event, RE::BSTEventSource<RE::TESCellFullyLoadedEvent>*) override;

		};
	}
}