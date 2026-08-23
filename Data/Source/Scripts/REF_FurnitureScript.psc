Scriptname REF_FurnitureScript Extends ObjectReference

Actor Property PlayerREF Auto
Activator Property Smoke Auto
ImpactDataSet Property Sparks Auto
Static Property XMarker Auto

Float Property fSmokeDisperseTime = 3.5 Auto
{How long the smoke sticks around.}
Int Property iMaxUpdates = 7 Auto
{How many times the script will update - each update lasts 1.25 seconds.}
Int Property iUpdateShouldStrikeStone = 3 Auto
{After how many updates we will play the Sparks.}

Actor akActionActor

Bool bUsed = False

Int iCurrentUpdate
Int iThreshold
Int iSmokeThreshold

ObjectReference akFXMarker
ObjectReference akSmoke

ObjectReference Property UnlitFire Auto Hidden

Event OnActivate(ObjectReference akActionRef)
	
	If bUsed
	
		Return
	EndIf
	
	bUsed = True
	akActionActor = akActionRef As Actor
	
	If akActionActor == PlayerREF

		Game.DisablePlayerControls(true, true, true, false, true, false, false, false)
	EndIf
	
	;Make sure we're properly using the furniture before we start counting.
	Int iUpdateAttempts = 0
	
	While !Self.IsFurnitureInUse() && iUpdateAttempts < 10
	
		iUpdateAttempts += 1
		Utility.Wait(0.2)
	EndWhile
	
	If !Self.IsFurnitureInUse()
	
		;Something went wrong.
		Cleanup()
		Return
	EndIf
	
	;We're in the furniture and attempting to light the fire. Place the marker where the FX will appear.
	akFXMarker = UnlitFire.PlaceAtMe(XMarker)

	;Begin the update loop.
	iCurrentUpdate = 0
	iThreshold = iUpdateShouldStrikeStone
	iSmokeThreshold = Math.Ceiling(iMaxUpdates / 2)
	RegisterForSingleUpdate(1.25)
EndEvent

Event OnUpdate()

	If akActionActor.GetCombatState() != 0
	
		Cleanup()
		Return
	EndIf
	
	iCurrentUpdate += 1
	iThreshold -= 1
	iSmokeThreshold -= 1
	
	If iThreshold == 0
	
		akFXMarker.PlayImpactEffect(Sparks)
		iThreshold = iUpdateShouldStrikeStone
	EndIf
	
	If iSmokeThreshold == 0
	
		akSmoke = UnlitFire.PlaceAtMe(Smoke)
		akSmoke.SetAngle(UnlitFire.GetAngleX(), UnlitFire.GetAngleY(), UnlitFire.GetAngleZ())
	EndIf
	
	If iCurrentUpdate < iMaxUpdates
	
		RegisterForSingleUpdate(1.25)
	Else
	
		Cleanup(True)
	EndIf
EndEvent

Event OnCellDetatch()

	CleanUp()
EndEvent

Function Cleanup(Bool bCompleted = False)

	If bCompleted
	
		(UnlitFire As REF_ObjectRefOffController).Relight()
	EndIf
	
	If akActionActor == PlayerREF
		
		Game.EnablePlayerControls()
	EndIf
	
	Self.Activate(akActionActor)
	
	Self.Disable()
	Self.Delete()
	
	If akFXMarker
	
		akFXMarker.Disable()
		akFXMarker.Delete()
	EndIf
	
	Utility.Wait(fSmokeDisperseTime)
	
	If akSmoke
	
		akSmoke.Disable(True)
		akSmoke.Delete()
	EndIf
	
	;Clean up any pointers.
	UnlitFire = NONE
EndFunction