#pragma once

namespace HitManager {
#define continueEvent RE::BSEventNotifyControl::kContinue

	class HitManager : 
		public RE::BSTEventSink<RE::TESHitEvent>,
		public ISingleton<HitManager> {
	public:
		bool RegisterListener();
	private:
		RE::BSEventNotifyControl ProcessEvent(const RE::TESHitEvent* a_event, RE::BSTEventSource<RE::TESHitEvent>* a_eventSource) override;
	};
}