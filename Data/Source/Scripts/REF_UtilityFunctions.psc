Scriptname REF_UtilityFunctions Hidden
{Functions in this script are handled by the accompanying DLL.}

;Returns Sky::IsRaining() - true if:
;  The current weather is Rainy/Snowy AND has transitioned enough to visibly rain
;  The previous weather was Rainy/Snowy AND hasn't faded enough to not be visible.
Bool Function IsRaining() Global Native 

;Returns the version of the DLL as Major-Minor-Patch. Example: [6, 0, 0]
Int[] Function GetVersion() Global Native

;Returns nearby lights, smoke objects, and DynDOLOD fires that might not have loaded on cell attach.
ObjectReference[] Function GetNearbyAssociatedReferences(ObjectReference a_fire) Global Native

;Extinguishes all currently loaded fires.
Function ExtinguishAllLoadedFires() Global Native


;/
=========================================================
            DEPRECATED FUNCTIONS & EVENTS
 The following functions and events have been deprecated
              and no longer do anything
=========================================================
/;
Function FreezeFire(ObjectReference a_kForm) Global Native
Function UnFreezeFire(ObjectReference a_kForm) Global Native
Function RegisterForAccurateWeatherChange(Form akForm) Global Native
Function UnRegisterForAccurateWeatherChange(Form akForm) Global Native
Function RegisterForInteriorExteriorChange(Form akForm) Global Native
Function UnRegisterForInteriorExteriorChange(Form akForm) Global Native
Function SetRainingFlag(Bool a_isRaining) Global Native

Event OnWeatherChange(Bool a_bIsRaning)
EndEvent