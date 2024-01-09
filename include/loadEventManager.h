#pragma once

namespace LoadManager {
#define continueEvent RE::BSEventNotifyControl::kContinue

	class LoadManager :
		public RE::BSTEventSink<RE::TESCellAttachDetachEvent>,
		public ISingleton<LoadManager> {
	public:
		bool RegisterListener();
	private:
		RE::BSEventNotifyControl ProcessEvent(const RE::TESCellAttachDetachEvent* a_event, RE::BSTEventSource<RE::TESCellAttachDetachEvent>* a_eventSource) override;
	};

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