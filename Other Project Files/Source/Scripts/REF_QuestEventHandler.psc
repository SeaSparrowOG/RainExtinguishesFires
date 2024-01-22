Scriptname REF_QuestEventHandler Extends Quest
{Puts the fires out.}

import REF_UtilityFunctions

Actor Property PlayerREF Auto

Float Property UpdateInterval Auto

Int[] Property Version Auto Hidden

Bool bRaining 
Float timeRemaining 
;/
==================================================================================
Events
==================================================================================
/;

Event OnInit()

    Maintenance()
EndEvent

Event OnWeatherChange(Bool a_bIsRaning, Float a_remainingTime)

    timeRemaining = a_remainingTime
    bRaining = a_bIsRaning

    If a_remainingTime < UpdateInterval

        If a_remainingTime < 1.0

            RegisterForSingleUpdate(1.0)
        Else 

            RegisterForSingleUpdate(a_remainingTime)
        EndIf
    Else

        RegisterForSingleUpdate(UpdateInterval)
    EndIf
EndEvent

Event OnPlayerInteriorExteriorChange(Bool a_bMovedToExterior)

    If (!a_bMovedToExterior)

        SetRainingFlag(False)
        bRaining = False
        UnregisterForUpdate() 
    Else

        CheckWeather()
    EndIf
EndEvent

Event OnUpdate()

    If PlayerREF.IsInInterior()

        SetRainingFlag(False)
        bRaining = False
        Return
    EndIf

    timeRemaining -= UpdateInterval

    If (timeRemaining <= 0.0 || Weather.GetCurrentWeatherTransition() >= 0.85)

        If bRaining

            SetRainingFlag(True)
            ExtinguishAllLoadedFires()
        Else 

            SetRainingFlag(False)
        EndIf
        Return
    EndIf

    If (timeRemaining < UpdateInterval)

        If (timeRemaining < 1.0)

            RegisterForSingleUpdate(1.0)
        Else

            RegisterForSingleUpdate(timeRemaining)
        EndIf
    Else 

        RegisterForSingleUpdate(UpdateInterval)
    EndIf
EndEvent

;/
==================================================================================
Functions
==================================================================================
/;

Int Function CheckVersion()

    Int[] iDLLVersion = GetVersion()

    If !iDLLVersion 

        Return -1
    EndIf

    If iDLLVersion[0] > 5

        Return -1
    EndIf

    If iDLLVersion[1] > 0

        Return 1
    EndIf

    Return 0
EndFunction

Function CheckWeather() 

    Int iWeatherClass = Weather.GetCurrentWeather().GetClassification() 

    If (!(iWeatherClass == 2 || iWeatherClass == 3))

        Return 
    EndIf

    Float fWeatherPct = Weather.GetCurrentWeatherTransition() 
    bRaining = True

    if (fWeatherPct >= 0.85)

        SetRainingFlag(True)
        ExtinguishAllLoadedFires()
    Else 

        timeRemaining = (0.9 - fWeatherPct) * 25.0

        If (timeRemaining < UpdateInterval)

            If (timeRemaining < 1.0)

                RegisterForSingleUpdate(1.0)
            Else

                RegisterForSingleUpdate(timeRemaining)
            EndIf
        Else 

            RegisterForSingleUpdate(UpdateInterval)
        EndIf
    EndIf
EndFunction

Function RegisterForEvents()
    
    RegisterForAccurateWeatherChange(self)
    RegisterForPlayerCellChangeEvent(Self)
EndFunction

Function Maintenance()

    If !Version 
        
        Version = GetVersion()

        If !Version 

            Return
        EndIf
    EndIf

    Int iResponse = CheckVersion()

    If iResponse == -1

        Return
    ElseIf iResponse == 1

        Self.Stop()
        Return
    EndIf

    RegisterForEvents()

    CheckWeather()
EndFunction 