Scriptname REF_QuestEventHandler Extends Quest
{Puts the fires out.}

import REF_UtilityFunctions

Actor Property PlayerREF Auto
Message Property REF_MSG_Updating Auto 
Message Property REF_MSG_Startup Auto   
Message Property REF_MSG_IncompatibleUpdate Auto 
Message Property REF_MSG_NoDLLFound Auto

Float Property UpdateInterval Auto

Int[] Property Version Auto Hidden

Bool bRaining = False
Bool bWasInInterior = True
Float timeRemaining = 0.0
;/
==================================================================================
Events
==================================================================================
/;

Event OnInit()

    Maintenance()
EndEvent

;/
SeaSparrow - Version 6.0.0 -> Removed, since they are unecessary.
Event OnWeatherChange(Bool a_bIsRaining)
    UnRegisterForUpdate()

    bRaining = a_bIsRaining
    timeRemaining = 20.0 * (1.0 - Weather.GetCurrentWeatherTransition())

    If timeRemaining < 0.0 && bRaining

        SetRainingFlag(bRaining)
        ExtinguishAllLoadedFires()
    ElseIf timeRemaining < UpdateInterval

        If timeRemaining < 1.0

            RegisterForSingleUpdate(1.0)
        Else 

            RegisterForSingleUpdate(timeRemaining)
        EndIf
    Else

        RegisterForSingleUpdate(UpdateInterval)
    EndIf
EndEvent

Event OnInteriorExteriorChange(Bool a_movedToInterior, Bool a_bIsRaining)

    if (a_movedToInterior) 

        UnRegisterForUpdate()
        bWasInInterior = True
        Return
    EndIf

    If (!bWasInInterior)

        Return 
    EndIf

    bRaining = a_bIsRaining
    bWasInInterior = False
    timeRemaining = 20.0 * (1.0 - Weather.GetCurrentWeatherTransition())

    If timeRemaining < 0.0 && bRaining

        SetRainingFlag(bRaining)
        ExtinguishAllLoadedFires()
    ElseIf timeRemaining < UpdateInterval

        If timeRemaining < 1.0

            RegisterForSingleUpdate(1.0)
        Else 

            RegisterForSingleUpdate(timeRemaining)
        EndIf
    Else

        RegisterForSingleUpdate(UpdateInterval)
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

        SetRainingFlag(bRaining)
        If bRaining

            ExtinguishAllLoadedFires()
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
/;

;/
==================================================================================
Functions
==================================================================================
/;

Int Function CheckVersion()

    If !Version 
        
        Version = GetVersion()

        If !Version 

            REF_MSG_NoDLLFound.Show()
            Return -1
        EndIf
        ;SeaSparrow - Version 6.0.0 -> Removed because it annoyed me greatly.
        ;REF_MSG_Startup.Show()
    EndIf

    Int[] iDLLVersion = GetVersion()

    If !iDLLVersion 

        Return -1
    EndIf

    If Version[0] < iDLLVersion[0]

        Return -1
    EndIf

    If Version[1] < iDLLVersion[1]

        Return 1
    EndIf

    Return 0
EndFunction

;/
SeaSparrow - Version 6.0.0 -> Deprecated. Handled in the DLL.
Function CheckWeather() 

    Int iWeatherClass = Weather.GetCurrentWeather().GetClassification() 

    If (!(iWeatherClass == 2 || iWeatherClass == 3))

        bRaining = False 
        SetRainingFlag(False)
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
    RegisterForInteriorExteriorChange(self)
EndFunction
/;

Function Maintenance()

    ;/ 
    SeaSparrow - Version 6.0.0 -> Moved to "CheckVersion"
    If !Version 
        
        Version = GetVersion()

        If !Version 

            REF_MSG_NoDLLFound.Show()
            Return
        EndIf
        ;SeaSparrow - Version 6.0.0 -> Removed because it annoyed me greatly.
        ;REF_MSG_Startup.Show()
    EndIf
    /;

    Int iResponse = CheckVersion()
    If iResponse == -1

        REF_MSG_IncompatibleUpdate.Show()
        Return
    ElseIf iResponse == 1

        REF_MSG_Updating.Show()
        Version = NONE
        Self.Stop()
        Return
    EndIf

    ;SeaSparrow - Version 6.0.0 -> Removed, not necessary.
    ;RegisterForEvents()
EndFunction 