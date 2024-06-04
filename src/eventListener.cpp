#include "eventListener.h"
#include "fireManipulator.h"
#include "fireRegister.h"

namespace Events {
	bool RegisterForEvents() {
		bool success = true;
		if (!HitEvenetManager::GetSingleton()->RegisterListener()) success = false;
		if (success && !LoadEventManager::GetSingleton()->RegisterListener()) success = false;
		if (success && !ActorCellManager::GetSingleton()->RegisterListener()) success = false;

		if (!success) {
			HitEvenetManager::GetSingleton()->UnregisterListener();
			LoadEventManager::GetSingleton()->UnregisterListener();
		}
		return success;
	}

}