Scriptname REF_UtilityFunctions Hidden
{Script for handling the extinguishing event.}

;/
Functions
In order to use any of them, import this script in your script, then
you can call them. You need to register for events during each load
in order to receive them.
/;

;Returns true if the DLL thinks it is raining.
Bool Function IsRaining() Global Native 

;Returns the version of the DLL. The format is this:
;Int[0] - MAJOR
;Int[1] - MINOR
;Int[2] - PATCH
Int[] Function GetVersion() Global Native

;Extinguishes all loaded registered fires. Does not extinguish
;fires with enable parents, or fires with enable children that are
;also not DynDOLOD fires.
Function ExtinguishAllLoadedFires() Global Native

;Sets the raining flag in the DLL.
Function SetRainingFlag(Bool a_isRaining) Global Native

;Freezes a given fire reference. This means that it will not be extinguished
;or relit until unfrozen.
Bool Function FreezeFire(ObjectReference a_kForm) Global Native

;Unfreezes a given fire, allowing the DLL to manipulate it again.
Bool Function UnFreezeFire(ObjectReference a_kForm) Global Native

;Register or unregister a given form (quest) for a given event.

Function RegisterForAccurateWeatherChange(Form akForm) Global Native
Function RegisterForPlayerCellChangeEvent(Form akForm) Global Native
Function UnRegisterForAccurateWeatherChange(Form akForm) Global Native
Function UnRegisterForPlayerCellChangeEvent(Form akForm) Global Native

;/
Events
/;

;Event called when the weather changes. Notifies if it is raining
;and how long it will take for the rain to fade in/out.
Event OnWeatherChange(Bool a_bIsRaning, Float a_remainingTime)
EndEvent

;Event called when the player moves from an interior to an exterior.
Event OnPlayerInteriorExteriorChange(Bool a_bMovedToExterior)
EndEvent