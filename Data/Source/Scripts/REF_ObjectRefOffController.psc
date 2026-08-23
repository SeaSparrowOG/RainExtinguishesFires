Scriptname REF_ObjectRefOffController Extends ObjectReference
{Handles the off version of the fire.}

ObjectReference Property RelatedFlame Auto Hidden
;SeaSparrow - Version 6.0.0 -> New Properties.
ObjectReference Property RelatedLight Auto Hidden
ObjectReference Property RelatedSmoke Auto Hidden
GlobalVariable Property DaysPassed Auto

ObjectReference[] Property RelatedObjects Auto Hidden ;SeaSparrow - Version 6.0.0 -> Deprecated usage of this.
Float Property DayAttached Auto Hidden

Furniture Property REF_FRN_RelightFurniture Auto

Import REF_UtilityFunctions

Bool Locked = False

Bool Function AcquireLock()

    Int iAttempt = 0
    While (Locked && iAttempt < 10)

        Utility.Wait(0.3)
        iAttempt += 1
    EndWhile

    If (iAttempt >= 10)

        Return False
    EndIf

    Locked = True
    Return True
EndFunction

Function FreeReferences()

    UnFreezeFire(RelatedFlame)
    UnfreezeFire(Self)
    UnfreezeFire(RelatedSmoke)
    UnfreezeFire(RelatedLight)
EndFunction

Function CleanUp()

    Int iAttempt = 0
    While (iAttempt < 5 && !AcquireLock())

        iAttempt += 1
        Utility.Wait(0.3)
    EndWhile

    RelatedFlame.Enable(True)
    Int iIndex = RelatedObjects.Length
    While IIndex > 0
        iIndex -= 1
        RelatedObjects[iIndex].EnableNoWait(False)
        UnFreezeFire(RelatedObjects[iIndex])
    EndWhile
    
    RelatedLight.Enable(False)
    RelatedSmoke.Enable(False)
    
    FreeReferences()

    RelatedFlame = NONE
    RelatedObjects = NONE

    Self.DisableNoWait()
    Self.Delete()
    Locked = False
EndFunction

;/
Some explanation: This script has 3 functions -> Extinguish, FireInTheRain, Relight.

Extinguish:
Extinguish is called when the fire is struck by Ice. It immediately extinguishes,
requesting nearby related objects from the DLL if they are not present.

Relight:
Relight is called when the fire is struck by Fire. It is immediately relit.

FireInTheRain:
FireInTheRain is called when a fire is LOADED while it is raining, or it just
begun raining. In this scenario, we wait 1 second before extinguishing to give
lights and smoke a chance to load, and then call the Extinguish event.
/;

Function Extinguish()

    If !Self.IsEnabled()
        Self.Enable()
        If !Self.IsEnabled()
            CleanUp()
            Return
        EndIf
    EndIf

    If (!AcquireLock())

        Cleanup()
        Return
    EndIf

    ;RelatedObjects = GetNearbyAssociatedReferences(RelatedFlame)
    ;Utility.Wait(0.25)
    RelatedFlame.Disable(True)
    ;Int iIndex = RelatedObjects.Length

    ;While IIndex > 0
     ;   iIndex -= 1
     ;   RelatedObjects[iIndex].DisableNoWait(False)
     ;   UnFreezeFire(RelatedObjects[iIndex])
    ;EndWhile

    RelatedSmoke.DisableNoWait(True)
    RelatedLight.DisableNoWait(False)
    FreeReferences()
    Locked = False
EndFunction

Function Relight()

    If (!AcquireLock())

        Cleanup()
        Return
    EndIf

    FreezeFire(RelatedFlame)
    ;Int iIndex = RelatedObjects.Length
    ;While iIndex > 0
    ;
    ;    iIndex -= 1
    ;    RelatedObjects[iIndex].EnableNoWait()
    ;EndWhile

    RelatedFlame.Enable(True)
    Utility.Wait(1.25)
    RelatedSmoke.Enable(True)
    RelatedLight.Enable(False)
    Self.Disable()
    
    Locked = False
    If (IsRaining())

        RegisterForSingleUpdate(10.0)
    Else
        UnFreezeFire(RelatedFlame)
        CleanUp()
    EndIf
EndFunction

Function FireInTheRain()

    RegisterForSingleUpdate(1.0)
EndFunction

Event OnUpdate()

    If (IsRaining())
   
        Extinguish()
    Else
        
        UnFreezeFire(RelatedFlame)
        CleanUp()
    EndIf
EndEvent

Event OnActivate(ObjectReference a_kActionRef)

    Actor akActionActor = a_kActionRef As Actor
	
	If !akActionActor
	
		Return
	EndIf
	
	ObjectReference akFurnitureRef = a_kActionRef.PlaceAtMe(REF_FRN_RelightFurniture)
	REF_FurnitureScript FurnitureController = akFurnitureRef As REF_FurnitureScript
	FurnitureController.UnlitFire = Self
	
	akFurnitureRef.Activate(akActionActor)
EndEvent

Event OnLoad()

    ;/
    Int iIndex = RelatedObjects.Length

    While IIndex > 0
        iIndex -= 1
        RelatedObjects[iIndex].DisableNoWait(False)
        UnFreezeFire(RelatedObjects[iIndex])
    EndWhile
    /;
EndEvent