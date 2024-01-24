#pragma once

namespace LoadManager {
#define continueEvent RE::BSEventNotifyControl::kContinue
#define continueContainer RE::BSContainer::ForEachResult::kContinue

	class ActorCellManager :
		public RE::BSTEventSink<RE::BGSActorCellEvent>,
		public ISingleton<ActorCellManager> {
	public:
		bool RegisterListener();
	private:
		RE::BSEventNotifyControl ProcessEvent(const RE::BGSActorCellEvent* a_event, RE::BSTEventSource<RE::BGSActorCellEvent>* a_eventSource) override;

		//Members
		bool wasInInterior;
	};
}