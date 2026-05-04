#include "Events.h"

#include "AttachDetach/AttachDetachListener.h"
#include "CellEvent/CellListener.h"
#include "HitEvent/HitListener.h"

namespace Events
{
	bool InitializeListeners() {
		logger::info("Initializing Event Listeners..."sv);
		bool success = true;
		success &= AttachDetachEvent::InitializeAttachDetachEventListener();
		success &= CellEvent::InitializeCellEventListener();
		success &= HitEvent::InitializeHitEventListener();

		if (!success) {
			logger::error("Failed to initialize one or more listeners."sv);
		}
		return success;
	}
}