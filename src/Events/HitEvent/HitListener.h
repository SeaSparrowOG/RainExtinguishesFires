#pragma once

namespace Events
{
	namespace HitEvent
	{
		bool InitializeHitEventListener();

		class HitEventListener : 
			public REX::Singleton<HitEventListener>, 
			public RE::BSTEventSink<RE::TESHitEvent>
		{
		public:
			bool Initialize();

		private:
			using EventControl = RE::BSEventNotifyControl;
			RE::BSEventNotifyControl ProcessEvent(const RE::TESHitEvent* a_event, RE::BSTEventSource<RE::TESHitEvent>*) override;
		};
	}
}