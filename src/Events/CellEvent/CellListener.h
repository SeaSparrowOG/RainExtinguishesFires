#pragma once

#include "Serialization/Serde.h"

namespace Events
{
	namespace CellEvent
	{
		bool InitializeCellEventListener();

		class ActorCellEventListener :
			public REX::Singleton<ActorCellEventListener>,
			public RE::BSTEventSink<RE::BGSActorCellEvent>,
			public Serialization::Serializable
		{
		public:
			bool Initialize();
			
			void RegisterFormForEvents(RE::TESForm* a_form);

			// Serialization
			bool Save(SKSE::SerializationInterface* a_intfc) override;
			bool Load(SKSE::SerializationInterface* a_intfc) override;

		private:
			using EventControl = RE::BSEventNotifyControl;
			EventControl ProcessEvent(const RE::BGSActorCellEvent* a_event, RE::BSTEventSource<RE::BGSActorCellEvent>*) override;

			bool _wasInInterior = false;
			SKSE::RegistrationSet<bool> _registeredListeners = "OnInteriorToExterior"sv;
		};
	}
}