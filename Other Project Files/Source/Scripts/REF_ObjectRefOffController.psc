Scriptname REF_ObjectRefOffController Extends ObjectReference
{Handles the off version of the fire.}

ObjectReference Property RelatedFlame Auto
ObjectReference[] Property RelatedObjects Auto
Float Property DayAttached Auto 

Bool bWaitingForParent 
Bool bExtinguished = False 

Import REF_UtilityFunctions

Function CleanUp()

    RelatedFlame = NONE
    RelatedObjects = NONE
    UnFreezeFire(Self)
    Self.Disable()
    Self.Delete()
EndFunction

Function Extinguish()

    bWaitingForParent = False
    If !Self.IsEnabled()

        Self.Enable()

        If !Self.IsEnabled()

            CleanUp()
            UnFreezeFire(RelatedFlame)
            Return
        EndIf
    EndIf

    Utility.Wait(0.25)
    RelatedFlame.Disable(True)
    Int iIndex = RelatedObjects.Length

    While IIndex > 0

        iIndex -= 1
        RelatedObjects[iIndex].Disable(False)
    EndWhile

    bExtinguished = True 
    UnFreezeFire(RelatedFlame)
    UnFreezeFire(Self)
EndFunction

Function Relight()

    FreezeFire(RelatedFlame)
    If IsRaining()

        bWaitingForParent = True 
        RegisterForSingleUpdate(10.0)
    EndIf

    Int iIndex = RelatedObjects.Length

    While iIndex > 0

        iIndex -= 1
        RelatedObjects[iIndex].EnableNoWait()
    EndWhile

    RelatedFlame.Enable()
    Utility.Wait(1.25)
    bExtinguished = False 
    Self.Disable()

    If !bWaitingForParent
    
        UnFreezeFire(RelatedFlame)
        CleanUp()
    EndIf
EndFunction

Event OnUpdate()

    If IsRaining() && RelatedFlame.Is3DLoaded()

        Extinguish()
    Else 

        CleanUp()
    EndIf

    UnFreezeFire(RelatedFlame)
    UnFreezeFire(Self)
EndEvent