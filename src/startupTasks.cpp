#include "startupTasks.h"

#include "fireRegister.h"
#include "hitManager.h"
#include "hooks.h"
#include "iniParser.h"
#include "jsonParser.h"
#include "loadEventManager.h"
#include "papyrus.h"

namespace StartupTasks {
	bool ApplyPreDataLoadedChanges() {

		return true;
	}

	bool ApplyDataLoadedChanges() {

		return true;
	}
}