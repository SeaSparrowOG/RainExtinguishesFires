Scriptname REF_UtilityFunctions Hidden
{Functions in this script are handled by the accompanying DLL.}

;Returns the version of the DLL as Major-Minor-Patch. Example: [6, 0, 0]
Int[] Function GetVersion() Global Native

;Returns true if it is raining, false in every other scenario
Bool Function IsRaining() Global Native 

;While transitioning fires, the DLL "locks" them, so you can't spam lit/extinguish calls.
;When the transition completes, this function should be called to unlock them.
Function UnFreezeFire(ObjectReference a_kForm) Global Native

;In order to safely relight fires, we need to let the DLL know that we are "using" them.
Function FreezeFire(ObjectReference a_kForm) Global Native

;/
=========================================================
            DEPRECATED FUNCTIONS & EVENTS
 The following functions and events have been deprecated
              and no longer do anything
=========================================================
/;
Function RegisterForAccurateWeatherChange(Form akForm) Global Native
Function UnRegisterForAccurateWeatherChange(Form akForm) Global Native
Function RegisterForInteriorExteriorChange(Form akForm) Global Native
Function UnRegisterForInteriorExteriorChange(Form akForm) Global Native
Function SetRainingFlag(Bool a_isRaining) Global Native
ObjectReference[] Function GetNearbyAssociatedReferences(ObjectReference a_fire) Global Native

Event OnWeatherChange(Bool a_bIsRaning)
EndEvent