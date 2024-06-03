#pragma once

namespace {
	/*
	* Class for easier event registration.
	*/
	template <typename T, class E>
	class EventClass : public RE::BSTEventSink<T> {
	public:
		static E* GetSingleton() {
			static E singleton;
			return std::addressof(singleton);
		}

		bool RegisterListener() {
			RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink(this);
			return true;
		}

		bool UnregisterListener() {
			RE::ScriptEventSourceHolder::GetSingleton()->RemoveEventSink(this);
			return true;
		}

	protected:
		EventClass() = default;
		~EventClass() = default;

		EventClass(const EventClass&) = delete;
		EventClass(EventClass&&) = delete;
		EventClass& operator=(const EventClass&) = delete;
		EventClass& operator=(EventClass&&) = delete;
	};
}

namespace Events {
#define continueEvent RE::BSEventNotifyControl::kContinue

	class HitEvenetManager :
		public EventClass<RE::TESHitEvent, HitEvenetManager> {
	private:
		RE::BSEventNotifyControl ProcessEvent(const RE::TESHitEvent* a_event, RE::BSTEventSource<RE::TESHitEvent>* a_eventSource) override;
	};

	class LoadEventManager :
		public EventClass<RE::TESCellAttachDetachEvent, LoadEventManager> {
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

	class RainEventManager : public ISingleton<RainEventManager> {
	public:
		bool IsRaining();

	private:
		//Members
		bool isRaining{ false };
	};

	bool RegisterForEvents();
}