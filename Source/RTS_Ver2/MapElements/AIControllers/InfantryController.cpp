// Fill out your copyright notice in the Description page of Project Settings.

#include "InfantryController.h"
#include "Classes/Engine/World.h"
#include "Public/WorldCollision.h"
#include "Public/TimerManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/Canvas.h"
#include "Public/DisplayDebugHelpers.h"
#include "VisualLogger/VisualLoggerTypes.h"
#include "VisualLogger/VisualLogger.h"
#include "Animation/AnimInstance.h"
#include "Components/WidgetComponent.h"

#include "MapElements/Infantry.h"
#include "Statics/Statics.h"
#include "GameFramework/RTSPlayerState.h"
#include "GameFramework/RTSGameState.h"
#include "GameFramework/RTSGameInstance.h"
#include "GameFramework/RTSPlayerController.h"
#include "MapElements/Abilities/AbilityBase.h"
#include "MapElements/ResourceSpot.h"
#include "MapElements/Building.h"
#include "Statics/DevelopmentStatics.h"
#include "GameFramework/FactionInfo.h"
#include "UI/InMatchWidgets/InfantryControllerDebugWidget.h"


// Scheduale:
// Check when focus needs to be set/unset
// Maybe see if abilities should require facing aswell


// TODO bHasAttack behavior needs defining

AInfantryController::AInfantryController()
{
	DoOnMoveComplete = nullptr;
	PendingContextAction = nullptr;
	SetPendingContextActionType(EContextButton::None);

	//--------------------------------------------------
	//	Editable UPROPERTY defaults
	//--------------------------------------------------

	BehaviorTickRate = 1.0f;

	RangeLenienceFactor = 1.1f;
	AcceptanceRadiusModifier_MoveToActor = -75.f;
	AcceptanceRadiusModifier_MoveToAbilityLocation = -0.f;
	AcceptanceRadiusModifier_MoveToBuildingTargetingAbilityTarget = -0.f;
	AcceptanceRadiusModifier_MoveToEnterBuildingGarrison = -0.f;
	AcceptanceRadiusModifier_ResourceSpot = -0.f;
	AcceptanceRadiusModifier_ResourceDepot = -0.f;
	AcceptanceRadiusModifier_PotentialConstructionSite = -0.f;
	AcceptanceRadiusModifier_ConstructionSite = -0.f;

	Lenience_GroundLocationTargetingAbility = 0.f;
	Lenience_ResourceSpotCollection = 150.f;
	Lenience_ResourceDepotDropOff = 100.f;
	Lenience_PotentialConstructionSite = 150.f;
	Lenience_ConstructionSite = 150.f;

	Idle_TargetAquireRange = 600.f;
	AttackMove_TargetAquireRange = 600.f;
	Idle_LeashDistance = 800.f;
	AttackMove_LeashDistance = 800.f;
	Idle_GiveUpOnTargetDistance = 400.f;
	AttackMove_GiveUpOnTargetDistance = 400.f;
	Idle_LeashTargetAquireCooldown = 1.5f;
	AttackMove_LeashTargetAquireCooldown = 1.5f;
}

void AInfantryController::OnPossess(APawn * InPawn)
{
	assert(InPawn != nullptr);

	/* Appears to be called before units begin play. Set references for
	unit and set own here */

	Super::OnPossess(InPawn);

	GI = CastChecked<URTSGameInstance>(GetWorld()->GetGameInstance());

	/* Stop here if setting up just for faction info */
	if (GI->IsInitializingFactionInfo())
	{
		return;
	}

	SetupReferences(InPawn);

	OnUnitRangeChanged(Unit->GetAttackAttributes()->GetAttackRange());
	OnUnitHasAttackChanged(Unit->bHasAttack);
	OnUnitAttackFacingRotationRequiredChanged(Unit->AttackFacingRotationRequirement);
	bCanUnitBuildBuildings = Unit->Attributes.bCanBuildBuildings;
}

void AInfantryController::SetReferences(ARTSPlayerState * InPlayerState, ARTSGameState * InGameState, AFactionInfo * InFactionInfo, FVisibilityInfo * InTeamVisInfo)
{
	assert(InPlayerState != nullptr);
	assert(InGameState != nullptr);
	assert(InFactionInfo != nullptr);

	PS = InPlayerState;
	GS = InGameState; /* If crash here check unit BP has RangedInfantryController set as its AI controller, or sometimes happens after class changes in C++ */
	FI = InFactionInfo;
	TeamVisInfo = InTeamVisInfo;
}

#if !UE_BUILD_SHIPPING
void AInfantryController::DisplayDebug(UCanvas * Canvas, const FDebugDisplayInfo & DebugDisplay, float & YL, float & YPos)
{
	Super::DisplayDebug(Canvas, DebugDisplay, YL, YPos);

	static FName NAME_AI = FName(TEXT("AI"));
	if (DebugDisplay.IsDisplayOn(NAME_AI))
	{
		FDisplayDebugManager & DisplayDebugManager = Canvas->DisplayDebugManager;

		DisplayDebugManager.DrawString(FString::Printf(TEXT("      Unit State: %s"),
			TO_STRING(EUnitState, UnitState)));

		DisplayDebugManager.DrawString(FString::Printf(TEXT("      Anim State for behavior: %s"),
			TO_STRING(EUnitAnimState, AnimStateForBehavior)));

		DisplayDebugManager.DrawString(FString::Printf(TEXT("      Last completed move result: %s"),
			*LastCompletedMoveResult));
	}
}

#if ENABLE_VISUAL_LOG
void AInfantryController::GrabDebugSnapshot(FVisualLogEntry * Snapshot) const
{
	/* This function is called max once per frame. It will be called when UE_VLOG is first called
	in a frame. Calling UE_VLOG again in the same frame will still add whatever data you want
	to add with it to the visual log but will not change what was gathered by this func (this
	information appears on the left side in visual logger window) */

	Super::GrabDebugSnapshot(Snapshot);

	FVisualLogStatusCategory MyCategory;
	MyCategory.Category = TEXT("RTS AI Controller");

	MyCategory.Add(TEXT("Unit state"), ENUM_TO_STRING(EUnitState, UnitState));
	MyCategory.Add(TEXT("Anim state"), ENUM_TO_STRING(EUnitAnimState, AnimStateForBehavior));
	MyCategory.Add(TEXT("Last completed move result"), LastCompletedMoveResult);
	MyCategory.Add(TEXT("AttackTarget"), AttackTarget != nullptr ? AttackTarget->GetName() : "NULL");
	MyCategory.Add(TEXT("ContextActionTarget"), ContextActionTarget != nullptr ? ContextActionTarget->GetName() : "NULL");
	MyCategory.Add(TEXT("Facing towards"), CharacterMovement != nullptr ? CharacterMovement->bOrientRotationToMovement ? "Movement direction" : "Focus" : "Unknown");
	MyCategory.Add(TEXT("Attack off cooldown?"), CanFire() ? "Yes" : "No");

	Snapshot->Status.Add(MyCategory);
}
#endif // ENABLE_VISUAL_LOG

FString AInfantryController::FunctionPtrToString(FunctionPtrType FunctionPtr) const
{
	if (FunctionPtr == nullptr)
	{
		return "NULL";
	}
	else if (FunctionPtr == &AInfantryController::GoIdle)
	{
		return "GoIdle";
	}
	else if (FunctionPtr == &AInfantryController::PlayIdleAnim)
	{
		return "PlayIdleAnim";
	}
	else if (FunctionPtr == &AInfantryController::AttackMove_OnReturnedToLeashLoc)
	{
		return "AttackMove_OnReturnedToLeashLoc";
	}
	else
	{
		return "UNKNOWN";
	}
}
#endif // !UE_BUILD_SHIPPING

void AInfantryController::StartBehavior(ABuilding * BuildingSpawnedFrom)
{
	if (Unit->IsStartingSelectable()) // Player started match with this unit
	{
		GoIdle();
	}
	else // Unit was built during match
	{
		bool bIssuedCommand = false;
		
		if (Unit->Attributes.ResourceGatheringProperties.IsCollector() 
			&& BuildingSpawnedFrom->GetProducedUnitInitialBehavior() == EUnitInitialSpawnBehavior::CollectFromNearestResourceSpot)
		{
			// This func should return null if there is no viable resource spots
			AResourceSpot * ClosestResourceSpot = BuildingSpawnedFrom->GetClosestResourceSpot(Unit->Attributes.ResourceGatheringProperties);
			if (ClosestResourceSpot != nullptr)
			{
				StartResourceGatheringRoute(ClosestResourceSpot);
				bIssuedCommand = true;
			}
		}

		if (!bIssuedCommand)
		{
			/* Move to buildings initial move location then rally point. Just do regular move but may
			want to change this in future so unit cannot receive new orders during move to initial
			point */
			LeashLocation = BuildingSpawnedFrom->GetInitialMoveLocation();
			SetUnitState(EUnitState::MovingToBarracksInitialPoint);
			DoOnMoveComplete = &AInfantryController::MoveToRallyPoint;
			Move(BuildingSpawnedFrom->GetInitialMoveLocation());
		}
	}

	// Tick will do behavior
}

void AInfantryController::Tick(float DeltaTime)
{
	if (UnitState == EUnitState::PossessedUnitDestroyed || UnitState == EUnitState::BehaviorNotStarted)
	{
		Super::Tick(DeltaTime);
		return;
	}

	// Check if attack needs resetting and reset it if so
	bool bIsAttackCoolingDown = TimeTillAttackResets > 0.f;
	TimeTillAttackResets -= DeltaTime;
	if (bIsAttackCoolingDown && TimeTillAttackResets <= 0.f)
	{
		// Attack is usable again

		OnResetFire();
	}

	AccumulatedTickBehaviorTime += DeltaTime;
	if (AccumulatedTickBehaviorTime >= BehaviorTickRate)
	{
		AccumulatedTickBehaviorTime = 0.f;

		TickBehavior();
	}

	if (AnimStateForBehavior == EUnitAnimState::NotPlayingImportantAnim)
	{
		// Check if in a state that actually tries to attack target
		if (CanFire() && WantsToAttack())
		{
			if (Statics::IsValid(AttackTarget) && IsFacingAttackTarget()
				&& IsTargetAttackable() && IsTargetInRange())
			{
				/* To support attacking while moving in future there is no
				DoOnMoveComplete = nullptr; StopMovement(); here, but may need to be added
				to get behavior working properly */
				BeginAttackAnim();
			}
			else { /* Do I wanna possibly unaquire the target here if it's no longer attackable? 
				   Probably want range test failing or facing requirement failing to not do it */}
		}
	}

	Super::Tick(DeltaTime);
}

void AInfantryController::UpdateControlRotation(float DeltaTime, bool bUpdatePawn)
{
	/* Most of this copied from AAIController::UpdateControlRotation but extra part added
	in to ignore rotating towards focus in some cases (bIgnoreLookingAtFocus) */

	/* For this to work there is an assumption that the Gameplay focus priority is for the
	units target (either attack target or ability target) */

	if (Unit != nullptr)
	{
		FRotator NewControlRotation = GetControlRotation();

		/* The gist of code below is look at gameplay priority focus (AttackTarget or
		AbilityTarget) if they are set and outside fog of war. Otherwise look at Move
		focus (which is set automatically by MoveToLocation). But both them require
		bIgnoreLookingAtFocus to be false which is set right after unit attacks */

		bool bLookAtFocus = false;

		// Check if unit has a focus that is an actor. If yes then we have a target
		AActor * FocusActor = GetFocusActorForPriority(EAIFocusPriority::Gameplay);

		if (bIgnoreLookingAtFocus) // Ignore looking at target focus this should be
		{
			// Do nothing
		}
		else
		{
			if (FocusActor != nullptr)
			{
				// Check if focus actor is outside fog. May want to cache the ISelectable version 
				// of FocusActor if there is significant casting overhead
				if (Statics::IsValid(FocusActor) && Statics::IsOutsideFog(FocusActor, 
					CastChecked<ISelectable>(FocusActor), Unit->GetTeam(), Unit->GetTeamTag(), GS))
				{	
					bLookAtFocus = true;
				}
				else
				{
					// Stop looking at them. Clear AttackTarget and ContextActionTarget 
					ClearFocus(EAIFocusPriority::Gameplay);
					AttackTarget = ContextActionTarget = nullptr;
		
					// Change unit state as well? Will be updated next TickBehavior anyway
				}
			}
			else
			{
				// Do not have a target, so may want to look at move focus
				bLookAtFocus = true;
			}
		}

		const FVector FocalPoint = GetFocalPoint();

		if (/*bLookAtFocus && */FAISystem::IsValidLocation(FocalPoint))
		{
			NewControlRotation = (FocalPoint - Unit->GetPawnViewLocation()).Rotation();
		}
		else if (bSetControlRotationFromPawnOrientation)
		{
			NewControlRotation = Unit->GetActorRotation();
		}

		// Don't pitch view unless looking at target
		if (FocusActor == nullptr)
		{
			NewControlRotation.Pitch = 0.f;
		}

		SetControlRotation(NewControlRotation);

		if (bUpdatePawn)
		{
			const FRotator CurrentPawnRotation = Unit->GetActorRotation();

			if (CurrentPawnRotation.Equals(NewControlRotation, 1e-3f) == false)
			{
				Unit->FaceRotation(NewControlRotation, DeltaTime);
			}
		}
	}
}

void AInfantryController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult & Result)
{
	/* For this to be called basically unit must be moving @See UPathFollowingComponent::AbortMove
	and bIsUsingMetaPath needs to be false. I can't say for sure from looking at source that it
	will be but I'll assume it always is */

#if !UE_BUILD_SHIPPING
	LastCompletedMoveResult = Result.ToString();
#endif

	Super::OnMoveCompleted(RequestID, Result);

	// Deal with this func:
	// Because Move will actually call this func if already moving then need to distinguish those 
	// situations. I do this with the if statement below. If it doesn't work since for example 
	// Move is called, this func fires then the new move request does not call this func because 
	// already close enough to destination then will need to check if moving before making 
	// move requests and if moving will need to cache DoOnMoveComplete, then do not call it in 
	// this func but instead do DoOnMoveComplete = DoOnMoveCompleteCached

	/* The goal of this if statement is to evaluate true when a new move request comes in
	while there is already one being carried out */
	if (Result.HasFlag(FPathFollowingResultFlags::NewRequest))
	{
		UE_VLOG(this, RTSLOG, Verbose, TEXT("OnMoveCompleted called, result was NewRequest so "
			"nothing was done"));

		return;
	}

	/* This was recently added to stop this situation:
	Attack anim starts in Tick after the TickBehavior bit, then some time before next
	TickBehavior OnMoveCompleted is called which plays another anim (usually idle anim) which
	cancels the attack anim and will never move out of DoIngAttackAnim state
	OnMoveCompleted is called at some point between TickBehaviors */
	if (AnimStateForBehavior == EUnitAnimState::DoingAttackAnim)
	{
		return;
	}

	UE_VLOG(this, RTSLOG, Verbose, TEXT("OnMoveCompleted called, DoOnMoveComplete = %s"),
		*FunctionPtrToString(DoOnMoveComplete));

	// Face focus if set
	SetFacing_Focus();

	// Call func pointed to by DoOnMoveComplete if valid, and null it
	if (DoOnMoveComplete != nullptr)
	{
		FunctionPtrType TempPtr = DoOnMoveComplete;
		DoOnMoveComplete = nullptr;
		(this->* (TempPtr))();
	}
}

void AInfantryController::TickBehavior()
{
	/* TODO: make sure range checks take into account bounds of enemy */

	/* Call correct function based on state */
	(this->* (TickFunctions[UnitState]))();
}

void AInfantryController::TickAI_MovingToBarracksInitialPoint()
{
}

void AInfantryController::TickAI_MovingToBarracksRallyPoint()
{
}

void AInfantryController::TickAI_IdleWithoutTarget()
{
	/* If this throws it might be because I implemented garrisonable units. Something 
	might have had a unit as a target but agter it entered the garrison it crashes here. 
	Not that crashing here means anything. I haven't actually added any code at all for 
	Target after something enters a garrison.
	I'll need to think about what to do with units entering garrisons and the things that are 
	targeting them. 
	Some ideas: 
	- store which selectables have us as a target and make they untarget us 
	- add some if (Target->IsInGarrison()) checks at times and if returning true then perhaps 
	switch target to the building they are inside 
	Just some ideas */
	assert(DoOnMoveComplete == nullptr);

	/* If unit has no attack then do nothing */
	if (!bUnitHasAttack)
	{
		return;
	}

	if (AnimStateForBehavior == EUnitAnimState::NotPlayingImportantAnim)
	{
		AttackTarget = FindClosestValidEnemyInRange(Idle_TargetAquireRange, true);
		if (AttackTarget != nullptr)
		{
			SetUnitState(EUnitState::Idle_WithTarget);

			// Remember where we encountered enemy so we can return there later
			LeashLocation = EncounterPoint = Unit->GetActorLocation();

			if (IsTargetInRange())
			{
				assert(!IsMoving());

				if (CanFire())
				{
					if (IsFacingAttackTarget())
					{
						BeginAttackAnim();
					}
				}
			}
			else
			{
				/* Don't bother chasing if we can't move. Will end up doing nothing, Kinda
				like hold positon command */
				if (Idle_LeashDistance > 0.f)
				{
					DoOnMoveComplete = &AInfantryController::PlayIdleAnim;
					MoveTowardsAttackTarget();
				}
			}
		}
	}
}

void AInfantryController::TickAI_IdleWithTarget()
{
	// Assume this for now
	assert(bUnitHasAttack);

	if (AnimStateForBehavior == EUnitAnimState::NotPlayingImportantAnim)
	{
		const float DistanceFromLeashLocSqr = Statics::GetDistance2DSquared(LeashLocation,
			Unit->GetActorLocation());

		if (DistanceFromLeashLocSqr > FMath::Square(Idle_LeashDistance))
		{
			// Have moved far enough, return to leash location

			/* Start a timer handle to say when we can start aquiring targets again on our
			journey back */
			DelayZeroTimeOk(TimerHandle_TargetAquiring, &AInfantryController::DoNothing,
				Idle_LeashTargetAquireCooldown);

			SetUnitState(EUnitState::Idle_ReturningToLeashLocation);

			DoOnMoveComplete = &AInfantryController::GoIdle;
			Move(LeashLocation);
		}
		else
		{
			if (Statics::IsValid(AttackTarget) && IsTargetAttackable())
			{
				if (IsTargetInRange())
				{
					StandStill();

					if (CanFire())
					{
						if (IsFacingAttackTarget())
						{
							BeginAttackAnim();
						}
					}
				}
				else
				{
					const float DistanceFromEncounterPointSqr = Statics::GetDistance2DSquared(EncounterPoint,
						Unit->GetActorLocation());

					if (DistanceFromEncounterPointSqr > FMath::Square(Idle_GiveUpOnTargetDistance))
					{
						/* We have moved far enough to consider looking for a new target */

						AActor * PreviousTarget = AttackTarget;

						/* Use attack range for this sweep, not target aquire range, because if another
						target isn't in attack range then do not bother swapping. Whole point of this
						is to find a target in attack range */
						AttackTarget = FindClosestValidEnemyInRange(Range, false);

						if (AttackTarget != nullptr)
						{
							if (AttackTarget != PreviousTarget)
							{
								EncounterPoint = Unit->GetActorLocation();

								if (IsTargetInRange()) // Sweep range and my range are not consistent so check
								{
									StandStill();

									if (CanFire())
									{
										if (IsFacingAttackTarget())
										{
											BeginAttackAnim();
										}
									}
								}
								else
								{
									DoOnMoveComplete = &AInfantryController::PlayIdleAnim;
									MoveTowardsAttackTarget();
								}
							}
							else // Still current target is closest target
							{
								/* Already know they aren't in range so move towards them */
								if (!IsMoving())
								{
									MoveTowardsAttackTarget();
								}
							}
						}
						else
						{
							/* No targets nearby. Return to leash location */

							SetUnitState(EUnitState::Idle_ReturningToLeashLocation);
							DoOnMoveComplete = &AInfantryController::GoIdle;
							Move(LeashLocation);
						}
					}
					else
					{
						if (!IsMoving())
						{
							DoOnMoveComplete = &AInfantryController::PlayIdleAnim;
							MoveTowardsAttackTarget();
						}
					}
				}
			}
			else
			{
				// Try aquire new target

				AttackTarget = FindClosestValidEnemyInRange(Idle_TargetAquireRange, false);
				if (AttackTarget != nullptr)
				{
					EncounterPoint = Unit->GetActorLocation();

					if (IsTargetInRange())
					{
						StandStill();

						if (CanFire())
						{
							if (IsFacingAttackTarget())
							{
								BeginAttackAnim();
							}
						}
					}
					else
					{
						DoOnMoveComplete = &AInfantryController::PlayIdleAnim;
						MoveTowardsAttackTarget();
					}
				}
				else
				{
					// No targets within range. Return to leash location
					SetUnitState(EUnitState::Idle_ReturningToLeashLocation);
					DoOnMoveComplete = &AInfantryController::GoIdle;
					Move(LeashLocation);
				}
			}
		}
	}
}

void AInfantryController::TickAI_Idle_ReturningToLeashLocation()
{
	assert(DoOnMoveComplete == &AInfantryController::GoIdle);

	// This if only here in case new anims are implemented that may be "important" and can 
	// be triggered from events like taking damage
	if (AnimStateForBehavior == EUnitAnimState::NotPlayingImportantAnim)
	{
		/* Check user defined value which puts a cooldown on how often to start rechecking for
		targets */
		const bool bCheckForEnemies = Idle_LeashTargetAquireCooldown >= 0.f
			&& !GetWorldTimerManager().IsTimerActive(TimerHandle_TargetAquiring);

		if (bCheckForEnemies)
		{
			AttackTarget = FindClosestValidEnemyInRange(Idle_TargetAquireRange, true);
			if (AttackTarget != nullptr)
			{
				SetUnitState(EUnitState::Idle_WithTarget);

				EncounterPoint = Unit->GetActorLocation();

				if (IsTargetInRange())
				{
					StandStill();

					if (CanFire())
					{
						if (IsFacingAttackTarget())
						{
							BeginAttackAnim();
						}
					}
				}
				else
				{
					DoOnMoveComplete = &AInfantryController::PlayIdleAnim;
					MoveTowardsAttackTarget();
				}
			}
		}
	}
}

void AInfantryController::TickAI_HoldingPositionWithoutTarget()
{
	assert(DoOnMoveComplete == nullptr);

	if (AnimStateForBehavior == EUnitAnimState::NotPlayingImportantAnim)
	{
		AttackTarget = FindClosestValidEnemyInRange(Range, true);
		if (AttackTarget != nullptr)
		{
			SetUnitState(EUnitState::HoldingPositionWithTarget);

			if (CanFire() && IsTargetInRange())
			{
				if (IsFacingAttackTarget())
				{
					BeginAttackAnim();
				}
			}
		}
	}
}

void AInfantryController::TickAI_HoldingPositionWithTarget()
{
	assert(DoOnMoveComplete == nullptr);

	if (AnimStateForBehavior == EUnitAnimState::NotPlayingImportantAnim)
	{
		if (Statics::IsValid(AttackTarget) && IsTargetAttackable() && IsTargetInRange())
		{
			if (CanFire() && IsFacingAttackTarget())
			{
				BeginAttackAnim();
			}
		}
		else
		{
			AActor * PreviousTarget = AttackTarget;

			// Range may not be sufficient, could add a bit more
			AttackTarget = FindClosestValidEnemyInRange(Range, false);
			if (AttackTarget != nullptr)
			{
				/* We know PreviousTarget is already not attackable so no point checking again */
				if (AttackTarget != PreviousTarget)
				{
					if (CanFire() && IsTargetInRange() && IsFacingAttackTarget())
					{
						BeginAttackAnim();
					}
				}
			}
			else
			{
				SetUnitState(EUnitState::HoldingPositionWithoutTarget);
			}
		}
	}
}

void AInfantryController::TickAI_MovingToRightClickLocation()
{
}

void AInfantryController::TickAI_MovingToPointNearStaticSelectable()
{
}

void AInfantryController::TickAI_MoveCommandToFriendlyMobileSelectable()
{
}

void AInfantryController::TickAI_RightClickOnEnemy()
{
	if (AnimStateForBehavior == EUnitAnimState::NotPlayingImportantAnim)
	{
		if (Statics::IsValid(AttackTarget) && IsTargetAttackable())
		{
			if (IsTargetInRange())
			{
				StandStill();

				if (CanFire() && IsFacingAttackTarget())
				{
					BeginAttackAnim();
				}
			}
			else
			{
				if (!IsMoving())
				{
					// Start moving towards target
					DoOnMoveComplete = &AInfantryController::PlayIdleAnim;
					MoveTowardsAttackTarget();
				}
			}
		}
		else
		{
			// Target no longer around. Possibly in fog now so stand idle

			StopMovementAndGoIdle();
		}
	}
}

void AInfantryController::TickAI_AttackMoveCommandWithNoTargetAquired()
{
	assert(DoOnMoveComplete == &AInfantryController::GoIdle);

	if (AnimStateForBehavior == EUnitAnimState::NotPlayingImportantAnim)
	{
		AttackTarget = FindClosestValidEnemyInRange(AttackMove_TargetAquireRange, true);
		if (AttackTarget != nullptr)
		{
			SetUnitState(EUnitState::AttackMoveCommand_WithTargetAquired);

			LeashLocation = EncounterPoint = Unit->GetActorLocation();

			if (IsTargetInRange())
			{
				StandStill();

				if (CanFire() && IsFacingAttackTarget())
				{
					BeginAttackAnim();
				}
			}
			else
			{
				// Don't bother chasing if cannot move
				if (AttackMove_LeashDistance > 0.f)
				{
					DoOnMoveComplete = &AInfantryController::PlayIdleAnim;
					MoveTowardsAttackTarget();
				}
			}
		}
		else
		{
			if (!IsMoving())
			{
				DoOnMoveComplete = &AInfantryController::AttackMove_OnReturnedToLeashLoc;
				Move(ClickLocation);
			}
		}
	}
}

void AInfantryController::TickAI_AttackMoveCommandWithTargetAquired()
{
	assert(DoOnMoveComplete == &AInfantryController::PlayIdleAnim);

	if (AnimStateForBehavior == EUnitAnimState::NotPlayingImportantAnim)
	{
		const float DistanceFromLeashLocSqr = Statics::GetDistance2DSquared(LeashLocation,
			Unit->GetActorLocation());

		if (DistanceFromLeashLocSqr > FMath::Square(AttackMove_LeashDistance))
		{
			// Have moved far enough, return to leash location

			/* Start a timer handle to say when we can start aquiring targets again on our
			journey back */
			DelayZeroTimeOk(TimerHandle_TargetAquiring, &AInfantryController::DoNothing,
				AttackMove_LeashTargetAquireCooldown);

			SetUnitState(EUnitState::AttackMoveCommand_ReturningToLeashLocation);
			DoOnMoveComplete = &AInfantryController::AttackMove_OnReturnedToLeashLoc;
			Move(LeashLocation);
		}
		else
		{
			if (Statics::IsValid(AttackTarget) && IsTargetAttackable())
			{
				if (IsTargetInRange())
				{
					StandStill();

					if (CanFire())
					{
						if (IsFacingAttackTarget())
						{
							BeginAttackAnim();
						}
					}
				}
				else
				{
					const float DistanceFromEncounterPointSqr = Statics::GetDistance2DSquared(EncounterPoint,
						Unit->GetActorLocation());
					if (DistanceFromEncounterPointSqr > FMath::Square(AttackMove_GiveUpOnTargetDistance))
					{
						/* We have moved far enough to consider looking for a new target */

						AActor * PreviousTarget = AttackTarget;

						/* Use attack range for this sweep, not target aquire range, because if another
						target isn't in attack range then do not bother swapping. Whole point of this
						is to find a target in attack range */
						AttackTarget = FindClosestValidEnemyInRange(Range, false);

						if (AttackTarget != nullptr)
						{
							if (PreviousTarget != AttackTarget)
							{
								EncounterPoint = Unit->GetActorLocation();

								if (IsTargetInRange())
								{
									StandStill();

									if (CanFire())
									{
										if (IsTargetInRange())
										{
											BeginAttackAnim();
										}
									}
								}
								else
								{
									MoveTowardsAttackTarget();
								}
							}
							else
							{
								/* Closest target is still our current target. Already know they
								are not in range so don't bother trying to attack. Move towards
								them if not moving already */
								if (!IsMoving())
								{
									MoveTowardsAttackTarget();
								}
							}
						}
						else
						{
							// No targets nearby. Return to leash location

							SetUnitState(EUnitState::AttackMoveCommand_ReturningToLeashLocation);
							DoOnMoveComplete = &AInfantryController::AttackMove_OnReturnedToLeashLoc;
							Move(LeashLocation);
						}
					}
					else
					{
						if (!IsMoving())
						{
							MoveTowardsAttackTarget();
						}
					}
				}
			}
			else
			{
				// Try aquire new target

				AttackTarget = FindClosestValidEnemyInRange(AttackMove_TargetAquireRange, false);
				if (AttackTarget != nullptr)
				{
					EncounterPoint = Unit->GetActorLocation();

					if (IsTargetInRange())
					{
						StandStill();

						if (CanFire())
						{
							if (IsFacingAttackTarget())
							{
								BeginAttackAnim();
							}
						}
					}
					else
					{
						MoveTowardsAttackTarget();
					}
				}
				else
				{
					// No valid targets nearby. Return to leash location
					SetUnitState(EUnitState::AttackMoveCommand_ReturningToLeashLocation);
					DoOnMoveComplete = &AInfantryController::AttackMove_OnReturnedToLeashLoc;
					Move(LeashLocation);
				}
			}
		}
	}
}

void AInfantryController::TickAI_AttackMoveCommand_ReturningToLeashLocation()
{
	assert(DoOnMoveComplete == &AInfantryController::AttackMove_OnReturnedToLeashLoc);

	// This if only here in case new anims are implemented that may be "important" and can 
	// be triggered from events like taking damage
	if (AnimStateForBehavior == EUnitAnimState::NotPlayingImportantAnim)
	{
		/* Check user defined value which puts a cooldown on how often to start rechecking for
		targets */
		const bool bCheckForEnemies = AttackMove_LeashTargetAquireCooldown >= 0.f
			&& !GetWorldTimerManager().IsTimerActive(TimerHandle_TargetAquiring);

		if (bCheckForEnemies)
		{
			AttackTarget = FindClosestValidEnemyInRange(AttackMove_TargetAquireRange, true);
			if (AttackTarget != nullptr)
			{
				SetUnitState(EUnitState::AttackMoveCommand_WithTargetAquired);
				EncounterPoint = Unit->GetActorLocation();

				if (IsTargetInRange())
				{
					StandStill();

					if (CanFire())
					{
						if (IsFacingAttackTarget())
						{
							BeginAttackAnim();
						}
					}
				}
				else
				{
					DoOnMoveComplete = &AInfantryController::PlayIdleAnim;
					MoveTowardsAttackTarget();
				}
			}
		}
	}
}

void AInfantryController::TickAI_HeadingToContextCommandWorldLocation()
{
}

void AInfantryController::TickAI_ChasingTargetToDoContextCommand()
{
}

void AInfantryController::TickAI_HeadingToBuildingToDoBuildingTargetingAbility()
{
}

void AInfantryController::TickAI_HeadingToBuildingToEnterItsGarrison()
{
}

void AInfantryController::TickAI_InsideBuildingGarrison()
{
}

void AInfantryController::TickAI_HeadingToResourceSpot()
{
}

void AInfantryController::TickAI_WaitingToGatherResources()
{
}

void AInfantryController::TickAI_GatheringResources()
{
}

void AInfantryController::TickAI_ReturningToResourceDepot()
{
}

void AInfantryController::TickAI_DroppingOffResources()
{
}

void AInfantryController::TickAI_HeadingToConstructionSite()
{
}

void AInfantryController::TickAI_HeadingToPotentialConstructionSite()
{
}

void AInfantryController::TickAI_WaitingForFoundationsToBePlaced()
{
}

void AInfantryController::TickAI_ConstructingBuilding()
{
}

void AInfantryController::TickAI_DoingContextActionAnim()
{
}

void AInfantryController::TickAI_DoingSpecialBuildingTargetingAbility()
{
}

void AInfantryController::TickAI_GoingToPickUpInventoryItem()
{
}

void AInfantryController::TickAI_PickingUpInventoryItem()
{
}

void AInfantryController::OnCommand_ClearResourceGatheringProperties()
{
	if (UnitState == EUnitState::WaitingToGatherResources
		|| UnitState == EUnitState::GatheringResources)
	{
		AssignedResourceSpot->OnCollectorLeaveSpot(Unit);
	}

	GetWorldTimerManager().ClearTimer(TimerHandle_ResourceGathering);
}

void AInfantryController::OnCommand_ClearBuildingConstructingProperties()
{
	if (UnitState == EUnitState::ConstructingBuilding
		&& Statics::IsValid(AssignedConstructionSite))
	{
		assert(AnimStateForBehavior == EUnitAnimState::ConstructingBuildingAnim);
		AssignedConstructionSite->OnWorkerLost(Unit);
	}
}

void AInfantryController::OnCommand_ClearTargetFocus()
{
	// Clear focus on target
	ClearFocus(EAIFocusPriority::Gameplay);
}

void AInfantryController::OnHoldPositionCommand()
{
	/* TODO: on this command and every other except maybe right click on resource spot, need to
	call AssignedResourceSpot->OnCollectorLeaveSpot() if it is not null */

	// If already holding position then do nothing
	if (UnitState == EUnitState::HoldingPositionWithoutTarget || UnitState == EUnitState::HoldingPositionWithTarget)
	{
		return;
	}

	// Behavior is to ignore command if doing uninterruptible context action
	if (AnimStateForBehavior == EUnitAnimState::DoingUninterruptibleContextActionAnim)
	{
		return;
	}

	UE_VLOG(this, RTSLOG, Verbose, TEXT("Hold position command received"));

	OnCommand_ClearResourceGatheringProperties();
	OnCommand_ClearBuildingConstructingProperties();
	SetPendingContextActionType(EContextButton::None);

	// Set unit state
	if (Statics::IsValid(AttackTarget) && IsTargetAttackable())
	{
		SetUnitState(EUnitState::HoldingPositionWithTarget);
	}
	else
	{
		SetUnitState(EUnitState::HoldingPositionWithoutTarget);
	}

	/* If playing attack anim then do not interrupt it. Let the attack anim montage play idle
	pose loop at the end of it that should be set in editor */
	if (AnimStateForBehavior == EUnitAnimState::DoingAttackAnim)
	{
		DoOnMoveComplete = nullptr;
		StopMovement();
	}
	else
	{
		StandStill();
	}
}

void AInfantryController::OnAttackMoveCommand(const FVector & Location, ISelectable * TargetAsSelectable, 
	const FSelectableAttributesBase * TargetInfo)
{
	if (AnimStateForBehavior == EUnitAnimState::DoingUninterruptibleContextActionAnim)
	{
		return;
	}

	if (TargetAsSelectable != nullptr)
	{
		/* Clicked on another selectable. Behaviour will be that of a regular
		right-click on that selectable. This means selected units won't switch
		enemies along the way and will just persue and engage Target. Also implies
		cannot attack friendlies with attack move commands */
		OnRightClickCommand(TargetAsSelectable, *TargetInfo);

		return;
	}

	/* Clicked on ground or something */

	UE_VLOG(this, RTSLOG, Verbose, TEXT("Attack move command on world location received"));

	OnCommand_ClearResourceGatheringProperties();
	OnCommand_ClearBuildingConstructingProperties();
	SetPendingContextActionType(EContextButton::None);

	ClickLocation = Location;

	if (Statics::IsValid(AttackTarget) && IsTargetAttackable())
	{
		SetUnitState(EUnitState::AttackMoveCommand_WithTargetAquired);

		StandStill();
	}
	else
	{
		SetUnitState(EUnitState::AttackMoveCommand_WithNoTargetAquired);

		OnCommand_ClearTargetFocus();

		DoOnMoveComplete = &AInfantryController::GoIdle;
		Move(Location);
	}
}

// Right click command version where a world location was clicked on, not another selectable
void AInfantryController::OnRightClickCommand(const FVector & Location)
{
	if (AnimStateForBehavior == EUnitAnimState::DoingUninterruptibleContextActionAnim)
	{
		return;
	}

	UE_VLOG(this, RTSLOG, Verbose, TEXT("Right click command on world location received"));

	OnCommand_ClearResourceGatheringProperties();
	OnCommand_ClearBuildingConstructingProperties();
	OnCommand_ClearTargetFocus();
	SetPendingContextActionType(EContextButton::None);

	ClickLocation = Location;

	SetUnitState(EUnitState::MovingToRightClickLocation);

	DoOnMoveComplete = &AInfantryController::GoIdle;
	Move(Location);
}

void AInfantryController::OnRightClickCommand(ISelectable * TargetAsSelectable, const FSelectableAttributesBase & TargetInfo)
{
	if (AnimStateForBehavior == EUnitAnimState::DoingUninterruptibleContextActionAnim)
	{
		return;
	}

	UE_VLOG(this, RTSLOG, Verbose, TEXT("Right click command on another target received"));

	SetPendingContextActionType(EContextButton::None);

	// Target as an actor
	AActor * Target = CastChecked<AActor>(TargetAsSelectable);
	assert(Statics::IsValid(Target));

	if (TargetInfo.GetOwnerID() == Unit->GetAttributesBase().GetOwnerID())
	{
		/* Target is another object controlled by same player. Move to it but only if it's a
		building */
		if (TargetInfo.GetSelectableType() == ESelectableType::Building)
		{
			/* If we are on a resource gathering route and this is the depot we're returning to
			then do nothing because we're already heading there */
			if (UnitState == EUnitState::ReturningToResourceDepot && Depot == Target)
			{
				return;
			}
			else
			{
				// If already working on or heading to clicked building then do nothing
				if (Target == AssignedConstructionSite
					&& (UnitState == EUnitState::HeadingToConstructionSite
						|| UnitState == EUnitState::ConstructingBuilding))
				{
					/* Already doing command for working on this building */
					return;
				}

				OnCommand_ClearTargetFocus();

				ABuilding * AsBuilding = CastChecked<ABuilding>(Target);

				/* If we are a builder unit and building is under construction and can allow
				a worker to work on it then do that */
				if (bCanUnitBuildBuildings && AsBuilding->CanAcceptBuilder())
				{
					// Stop working on previous building if we were
					OnCommand_ClearBuildingConstructingProperties();

					AssignedConstructionSite = AsBuilding;

					/* If already at building then work on it now. Otherwise move to building and
					when arrived try and work on it */
					if (IsAtConstructionSite())
					{
						/* Make very sure not to toggle bIsBeingBuilt here to the wrong value */
						DoOnMoveComplete = nullptr;
						StopMovement();
						WorkOnBuilding();
					}
					else
					{	
						// Move to construction site and work on it when there
						SetUnitState(EUnitState::HeadingToConstructionSite);
						DoOnMoveComplete = &AInfantryController::OnMoveToConstructionSiteComplete;
						Move(AsBuilding->GetActorLocation(), GetConstructionSiteAcceptanceRadius());
					}
				}
				else
				{
					OnCommand_ClearBuildingConstructingProperties();

					/* If this is a depot for our held resource then drop them off there */
					if (Unit->HeldResourceType != EResourceType::None
						&& AsBuilding->IsDropPointFor(Unit->HeldResourceType))
					{
						Depot = AsBuilding;
						ReturnToDepot();
					}
					else // Regular building owned by us
					{
						OnCommand_ClearResourceGatheringProperties();

						SetUnitState(EUnitState::MovingToPointNearStaticSelectable);
						DoOnMoveComplete = &AInfantryController::GoIdle;
						float AcceptanceRadius = GetNonHostileMoveToLocationAcceptanceRadius(TargetAsSelectable);
						Move(Target->GetActorLocation(), AcceptanceRadius);
					}
				}
			}
		}
		else // Clicked on unit owned by us
		{
			/* If target is currently selected by player then they will try to move to themselves
			and so will every other selected unit. For now completly ignore right click commands
			on owned units but may eventually want to make units move to them if they are not
			selected or something */
		}
	}
	else if (TargetInfo.GetTeam() == PS->GetTeam()) /* Ally */
	{
		OnCommand_ClearResourceGatheringProperties();
		OnCommand_ClearBuildingConstructingProperties();
		OnCommand_ClearTargetFocus();

		SetUnitState(EUnitState::MovingToPointNearStaticSelectable);
		DoOnMoveComplete = &AInfantryController::GoIdle;

		// Move close to target but do not try to follow in any way
		float AcceptanceRadius = GetNonHostileMoveToLocationAcceptanceRadius(TargetAsSelectable);
		Move(Target->GetActorLocation(), AcceptanceRadius);
	}
	else if (TargetInfo.GetOwnerID() == Statics::NeutralID) /* Neutral */
	{
		/* Commands to go pick up items: PC should have called a more specific function 
		if this is the case like ISelectable::IssueCommand_PickUpItem */
		assert(TargetInfo.GetSelectableType() != ESelectableType::InventoryItem);
		
		OnCommand_ClearBuildingConstructingProperties();
		OnCommand_ClearTargetFocus();

		/* Check if it is a resource spot */
		if (Target->IsA(AResourceSpot::StaticClass()))
		{
			AResourceSpot * ResourceSpot = CastChecked<AResourceSpot>(Target);

			/* Check if unit can gather this type of resource */
			if (Unit->CanCollectResource(ResourceSpot->GetType()) && !ResourceSpot->IsDepleted())
			{
				StartResourceGatheringRoute(ResourceSpot);
				return;
			}
		}

		OnCommand_ClearResourceGatheringProperties();

		SetUnitState(EUnitState::MovingToPointNearStaticSelectable);
		DoOnMoveComplete = &AInfantryController::GoIdle;

		// Move close to them but do not try to follow in any way
		float AcceptanceRadius = GetNonHostileMoveToLocationAcceptanceRadius(TargetAsSelectable);
		Move(Target->GetActorLocation(), AcceptanceRadius);
	}
	else /* Assumed hostile */
	{
		OnCommand_ClearResourceGatheringProperties();
		OnCommand_ClearBuildingConstructingProperties();

		if (UnitState == EUnitState::RightClickOnEnemy && Target == AttackTarget)
		{
			/* Already attacking Target. Do nothing */
			return;
		}

		if (CanTargetBeAquired(Target))
		{
			if (AttackTarget != Target)
			{
				OnTargetChange(Target);
			}
			AttackTarget = Target;

			SetUnitState(EUnitState::RightClickOnEnemy);

			if (IsTargetInRange())
			{
				StandStill();

				if (CanFire())
				{
					if (IsFacingAttackTarget())
					{
						BeginAttackAnim();
					}
				}
			}
			else
			{
				DoOnMoveComplete = &AInfantryController::PlayIdleAnim;
				MoveTowardsAttackTarget();
			}
		}
		else
		{
			/* Target is not attackable. Can be several reasons such as it is flying and we cannot
			attack flying units. Do not show a HUD warning message since it is possible multiple
			units received this command and it would clog up HUD. Do not interrupt what we are
			doing in any way */
		}
	}
}

void AInfantryController::OnRightClickOnResourceSpotCommand(AResourceSpot * ClickedResourceSpot)
{
	OnCommand_ClearBuildingConstructingProperties();
	OnCommand_ClearTargetFocus();

	/* Check if unit can gather this type of resource */
	if (Unit->CanCollectResource(ClickedResourceSpot->GetType()) && !ClickedResourceSpot->IsDepleted())
	{
		StartResourceGatheringRoute(ClickedResourceSpot);
		return;
	}

	OnCommand_ClearResourceGatheringProperties();

	SetUnitState(EUnitState::MovingToPointNearStaticSelectable);
	DoOnMoveComplete = &AInfantryController::GoIdle;

	// Move close to them but do not try to follow in any way
	float AcceptanceRadius = GetNonHostileMoveToLocationAcceptanceRadius(ClickedResourceSpot);
	Move(ClickedResourceSpot->GetActorLocation(), AcceptanceRadius);
}

void AInfantryController::OnInstantContextCommand(const FContextButton & Button)
{
	OnAbilityCommandInner(Button.GetButtonType(), EAbilityUsageType::SelectablesActionBar, 0, 0);
}

// Version that takes a location on the ground as a target
void AInfantryController::OnTargetedContextCommand(EContextButton ActionType, const FVector & Location)
{
	OnAbilityCommandInner(ActionType, EAbilityUsageType::SelectablesActionBar, 0, 0, Location);
}

// Version that takes another selectable as a target
void AInfantryController::OnTargetedContextCommand(EContextButton ActionType, ISelectable * TargetAsSelectable, const FSelectableAttributesBase & TargetInfo)
{
	OnAbilityCommandInner(ActionType, EAbilityUsageType::SelectablesActionBar, 0, 0, TargetAsSelectable);
}

void AInfantryController::OnInstantUseInventoryItemCommand(uint8 InventorySlotIndex, EInventoryItem ItemType,
	const FContextButtonInfo & AbilityInfo)
{ 
	OnAbilityCommandInner(AbilityInfo.GetButtonType(), EAbilityUsageType::SelectablesInventory,
		InventorySlotIndex, static_cast<uint8>(ItemType));
}

void AInfantryController::OnLocationTargetingUseInventoryItemCommand(uint8 InventorySlotIndex, 
	EInventoryItem ItemType, const FContextButtonInfo & AbilityInfo, const FVector & TargetLocation)
{
	OnAbilityCommandInner(AbilityInfo.GetButtonType(), EAbilityUsageType::SelectablesInventory,
		InventorySlotIndex, static_cast<uint8>(ItemType), TargetLocation);
}

void AInfantryController::OnSelectableTargetingUseInventoryItemCommand(uint8 InventorySlotIndex, 
	EInventoryItem ItemType, const FContextButtonInfo & AbilityInfo, ISelectable * ItemUseTarget)
{
	OnAbilityCommandInner(AbilityInfo.GetButtonType(), EAbilityUsageType::SelectablesInventory,
		InventorySlotIndex, static_cast<uint8>(ItemType), ItemUseTarget);
}

void AInfantryController::OnSpecialBuildingTargetingAbilityCommand(ABuilding * TargetBuilding, const FBuildingTargetingAbilityPerSelectableInfo * AbilityInfo)
{
	if (AnimStateForBehavior == EUnitAnimState::DoingUninterruptibleContextActionAnim)
	{
		return;
	}

	const EBuildingTargetingAbility IncomingAbilityType = AbilityInfo->GetAbilityType();

	/* Check if already in the middle of animation for the same ability against the same building */
	if (PendingAbilityUsageCase == EAbilityUsageType ::SpecialBuildingTargetingAbility 
		&& IncomingAbilityType == GetPendingBuildingTargetingAbilityType()
		&& TargetBuilding == ContextActionTarget
		&& AnimStateForBehavior == EUnitAnimState::DoingInterruptibleBuildingTargetingAbilityAnim)
	{
		return;
	}

	UE_VLOG(this, RTSLOG, Verbose, TEXT("Special building targeting ability command received. Target: [%s]"), *TargetBuilding->GetName());

	OnCommand_ClearResourceGatheringProperties();
	OnCommand_ClearBuildingConstructingProperties();
	SetPendingContextActionType(IncomingAbilityType);
	PendingAbilityUsageCase = EAbilityUsageType::SpecialBuildingTargetingAbility;

	if (ContextActionTarget != TargetBuilding)
	{
		OnContextActionTargetChange(TargetBuilding, TargetBuilding);
		ContextActionTarget = TargetBuilding;
	}

	const FBuildingTargetingAbilityStaticInfo & AbilityStaticInfo = GI->GetBuildingTargetingAbilityInfo(IncomingAbilityType);

	/* Check if in range */
	bool bInRange;
	if (AbilityStaticInfo.HasUnlimitedRange())
	{
		bInRange = true;
	}
	else
	{
		const float Distance = GetDistanceFromAnotherSelectableForAbility(AbilityStaticInfo, TargetBuilding);
		bInRange = FMath::Square(Distance) <= FMath::Square(AbilityStaticInfo.GetRange());
	}

	if (bInRange)
	{
		/* Stop movement. Assuming that's what the ability wants */
		DoOnMoveComplete = nullptr;
		StopMovement();
		StartBuildingTargetingAbility();
	}
	else
	{
		/* Move to within range */
		
		SetUnitState(EUnitState::HeadingToBuildingToDoBuildingTargetingAbility);
		SetAnimStateForBehavior(EUnitAnimState::NotPlayingImportantAnim);

		DoOnMoveComplete = &AInfantryController::OnMoveToBuildingForBuildingTargetingAbilityComplete;
		MoveTowardsBuildingTargetingAbilityTarget(AbilityStaticInfo);
	}
}

void AInfantryController::OnEnterGarrisonCommand(ABuilding * TargetBuilding, const FBuildingGarrisonAttributes & GarrisonAttributes)
{
	/* You may want to rewrite this function a bit. Instead perhaps when a command to enter a 
	garrison is given AND we are in range we check if it can accept us. If no then perhaps 
	do not interrupt what we were doing if it's something important. Import would mean 
	like attacking something, but perhaps if we're moving to some location then we would want 
	to stop in that case. Things to think about */
	
	if (AnimStateForBehavior == EUnitAnimState::DoingUninterruptibleContextActionAnim)
	{
		return;
	}

	/* If already doing the exact same command then return */
	if (TargetBuilding == ContextActionTarget
		&& UnitState == EUnitState::HeadingToBuildingToEnterItsGarrison)
	{
		return;
	}

	UE_VLOG(this, RTSLOG, Verbose, TEXT("Enter garrisonable building command received. Target: [%s]"), *TargetBuilding->GetName());

	OnCommand_ClearResourceGatheringProperties();
	OnCommand_ClearBuildingConstructingProperties();
	OnCommand_ClearTargetFocus();
	SetPendingContextActionType(EContextButton::None);

	if (ContextActionTarget != TargetBuilding)
	{
		OnContextActionTargetChange(TargetBuilding, TargetBuilding);
		ContextActionTarget = TargetBuilding;
	}

	/* Check if in range now to enter garrison */
	const bool bInRange = FMath::Square(GetDistanceFromBuildingForEnteringItsGarrison(TargetBuilding)) <= FMath::Square(GarrisonAttributes.GetEnterRange());
	if (bInRange)
	{
		/* Check if garrison can accept us */
		if (GarrisonAttributes.HasEnoughCapacityToAcceptUnit(Unit))
		{
			/* Enter the garrison now */
			DoOnMoveComplete = nullptr;
			StopMovement();
			SetUnitState(EUnitState::InsideBuildingGarrison);
			Unit->ServerEnterBuildingGarrison(TargetBuilding, const_cast<FBuildingGarrisonAttributes&>(GarrisonAttributes));
		}
		else
		{
			/* Garrison is too full. Stand around */
			StopMovementAndGoIdle();
		}
	}
	else
	{
		/* Move to within range */

		SetUnitState(EUnitState::HeadingToBuildingToEnterItsGarrison);
		SetAnimStateForBehavior(EUnitAnimState::NotPlayingImportantAnim);

		DoOnMoveComplete = &AInfantryController::OnMoveToBuildingToEnterItsGarrisonComplete;
		MoveTowardsBuildingToEnterItsGarrison(GarrisonAttributes);
	}
}

void AInfantryController::OnUnitExitGarrison()
{
	GoIdle();
	/* Not sure if more needs to be done. 
	Maybe unaquire target? Maybe not? */
}

void AInfantryController::OnAbilityCommandInner(EContextButton AbilityType, EAbilityUsageType UsageCase, 
	uint8 AuxilleryData, uint8 MoreAuxilleryData)
{
	if (AnimStateForBehavior == EUnitAnimState::DoingUninterruptibleContextActionAnim)
	{
		return;
	}

	UE_VLOG(this, RTSLOG, Verbose, TEXT("Targetless ability command received, command is %s"),
		TO_STRING(EContextButton, AbilityType));

	OnCommand_ClearResourceGatheringProperties();
	OnCommand_ClearBuildingConstructingProperties();

	/* Assuming ButtonInfo.bCommandStopsMovement is always true. If false will need to figure
	out a way to know ability montage has finished and then decide whether to play moving or
	idle anim */

	const FContextButtonInfo & ButtonInfo = GI->GetContextInfo(AbilityType);

	if (ButtonInfo.DoesCommandStopMovement())
	{
		DoOnMoveComplete = nullptr;
		StopMovement();
	}

	/* Either play animation montage and let anim notify do action or do action now */
	if (ButtonInfo.UsesAnimation())
	{
		/* If this is the same action that we are already doing then we should stop here as opposed
		to going through and restarting animation. This means player can spam click the same ability
		and it won't keep resetting the animation causing the ability to never happen */
		if (UsageCase == PendingAbilityUsageCase && AbilityType == GetPendingContextActionType())
		{
			return;
		}

		SetUnitState(EUnitState::DoingContextActionAnim);
		if (ButtonInfo.DoesAnimationBlockCommands())
		{
			SetAnimStateForBehavior(EUnitAnimState::DoingUninterruptibleContextActionAnim);
		}
		else
		{
			SetAnimStateForBehavior(EUnitAnimState::DoingInterruptibleContextActionAnim);
		}

		/* So DoContextCommand() knows what ability to execute */
		SetPendingContextActionType(AbilityType);
		PendingAbilityUsageCase = UsageCase;
		PendingAbilityAuxilleryData = AuxilleryData;
		PendingAbilityMoreAuxilleryData = MoreAuxilleryData;

		/* Assign function to be triggered by anim notify */
		PendingContextAction = &AInfantryController::TryDoInstantContextCommand;
		
		/* 2nd param should be true in case we use the same animation for different context actions.
		We've already checked the action is different so animiation should start from beginning.
		Also make sure anim notify is placed in animation montage so ability actually happens */
		Unit->PlayAnimation(ButtonInfo.GetAnimationType(), true);
	}
	else
	{
		/* So DoContextCommand() knows what ability to execute */
		SetPendingContextActionType(AbilityType);
		PendingAbilityUsageCase = UsageCase;
		PendingAbilityAuxilleryData = AuxilleryData;
		PendingAbilityMoreAuxilleryData = MoreAuxilleryData;

		// Spawn the ability
		DoInstantContextCommand();

		if (ButtonInfo.DoesCommandStopMovement())
		{
			// Since action has no anim it has already happened so just stand around
			GoIdle();
		}
	}
}

void AInfantryController::OnAbilityCommandInner(EContextButton AbilityType, EAbilityUsageType UsageCase, 
	uint8 AuxilleryData, uint8 MoreAuxilleryData, const FVector & Location)
{
	if (AnimStateForBehavior == EUnitAnimState::DoingUninterruptibleContextActionAnim)
	{
		return;
	}

	UE_VLOG(this, RTSLOG, Verbose, TEXT("World location ability command received, command is %s"),
		TO_STRING(EContextButton, AbilityType));

	OnCommand_ClearResourceGatheringProperties();
	OnCommand_ClearBuildingConstructingProperties();
	OnCommand_ClearTargetFocus(); // Could add a bool to abilities that's like 'bUsageClearsTarget' and if false we do not do this
	SetPendingContextActionType(AbilityType);
	PendingAbilityUsageCase = UsageCase;
	PendingAbilityAuxilleryData = AuxilleryData;
	PendingAbilityMoreAuxilleryData = MoreAuxilleryData;

	ClickLocation = Location;

	const FContextButtonInfo & AbilityInfo = GI->GetContextInfo(AbilityType);

	// Check if in range. AbilityRange <= 0 implies unlimited range
	bool bInRange = true;
	if (AbilityInfo.GetMaxRange() > 0.f)
	{
		/* Statics::IsSelectableInRangeForAbility is what we should really call here, but since
		it just bubbles into calling these two funcs I will call them here explicity now. But
		should Statics::IsSelectableInRangeForAbility change then this should be updated */
		const float DistanceSqr = FMath::Square(GetDistanceFromLocationForAbility(AbilityInfo, Location));
		const float DistanceRequiredSqr = Statics::GetAbilityRangeSquared(AbilityInfo);
		bInRange = (DistanceSqr <= DistanceRequiredSqr);
	}

	if (bInRange)
	{
		if (AbilityInfo.DoesCommandStopMovement())
		{
			DoOnMoveComplete = nullptr;
			StopMovement();
		}

		StartLocationTargetedContextAction(AbilityInfo);
	}
	else
	{
		/* Not in range. Head towards location and do ability when within range */

		SetUnitState(EUnitState::HeadingToContextCommandWorldLocation);
		SetAnimStateForBehavior(EUnitAnimState::NotPlayingImportantAnim);

		DoOnMoveComplete = &AInfantryController::OnMoveToLocationTargetedContextActionComplete;
		Move(Location, GetLocationTargetedAcceptanceRadius(AbilityInfo));
	}
}

void AInfantryController::OnAbilityCommandInner(EContextButton AbilityType, EAbilityUsageType UsageCase, 
	uint8 AuxilleryData, uint8 MoreAuxilleryData, ISelectable * AbilityTarget)
{
	if (AnimStateForBehavior == EUnitAnimState::DoingUninterruptibleContextActionAnim)
	{
		return;
	}

	UE_VLOG(this, RTSLOG, Verbose, TEXT("Selectable targeting ability command received, command is %s"),
		TO_STRING(EContextButton, AbilityType));

	assert(AbilityTarget != nullptr);

	// Target but as an actor
	AActor * Target = CastChecked<AActor>(AbilityTarget);

	/* If we are in the middle of the animation for this ability already and the target is the same
	then return */
	if (PendingAbilityUsageCase == UsageCase && AbilityType == GetPendingContextActionType() && Target == ContextActionTarget
		&&
		(AnimStateForBehavior == EUnitAnimState::DoingInterruptibleContextActionAnim
			|| AnimStateForBehavior == EUnitAnimState::DoingUninterruptibleContextActionAnim))
	{
		return;
	}

	OnCommand_ClearResourceGatheringProperties();
	OnCommand_ClearBuildingConstructingProperties();
	SetPendingContextActionType(AbilityType);
	PendingAbilityUsageCase = UsageCase;
	PendingAbilityAuxilleryData = AuxilleryData;
	PendingAbilityMoreAuxilleryData = MoreAuxilleryData;

	if (ContextActionTarget != Target)
	{
		OnContextActionTargetChange(Target, AbilityTarget);
	}
	ContextActionTarget = Target;

	const FContextButtonInfo & AbilityInfo = GI->GetContextInfo(AbilityType);

	// Check if in range. AbilityRange <= implies unlimited range
	bool bInRange = true;
	if (AbilityInfo.GetMaxRange() > 0.f)
	{
		/* Statics::IsSelectableInRangeForAbility is what we should really call here, but since
		it just bubbles into calling these two funcs I will call them here explicity now. But
		should Statics::IsSelectableInRangeForAbility change then this should be updated */
		const float DistanceSqr = FMath::Square(GetDistanceFromAnotherSelectableForAbility(AbilityInfo, AbilityTarget));
		const float DistanceRequiredSqr = Statics::GetAbilityRangeSquared(AbilityInfo);
		bInRange = (DistanceSqr <= DistanceRequiredSqr);
	}

	/* If in range do ability now. Otherwise persue target */
	if (bInRange)
	{
		if (AbilityInfo.DoesCommandStopMovement())
		{
			DoOnMoveComplete = nullptr;
			StopMovement();
		}

		StartSingleTargetContextAction(AbilityInfo);
	}
	else
	{
		SetUnitState(EUnitState::ChasingTargetToDoContextCommand);
		SetAnimStateForBehavior(EUnitAnimState::NotPlayingImportantAnim);

		DoOnMoveComplete = &AInfantryController::OnMoveToSingleTargetContextActionTargetComplete;
		MoveTowardsContextActionTarget(AbilityInfo);
	}
}

void AInfantryController::OnPickUpInventoryItemCommand(const FVector & SomeLocation,
	AInventoryItem * TargetItem)
{
	// Wrote this func kind of quickly, may want to review it 

	if (AnimStateForBehavior == EUnitAnimState::DoingUninterruptibleContextActionAnim)
	{
		return;
	}

	UE_VLOG(this, RTSLOG, Verbose, TEXT("Go pick up inventory item off ground command received"));

	/* Check if already moving towards it. We're just using the context action target here. If 
	this is problematic then will need to create a seperate variable like PickUpItemTarget or 
	something. */
	if (UnitState == EUnitState::GoingToPickUpInventoryItem && ContextActionTarget == TargetItem)
	{
		return;
	}

	/* If we're already playing the animation to pick up this item then return */
	if (UnitState == EUnitState::PickingUpInventoryItem && ContextActionTarget == TargetItem)
	{
		return;
	}

	OnCommand_ClearResourceGatheringProperties();
	OnCommand_ClearBuildingConstructingProperties();
	OnCommand_ClearTargetFocus();
	SetPendingContextActionType(EContextButton::None);

	ContextActionTarget = TargetItem; // 95% sure I need this

	/* Check if in range already to pick up item */
	if (TargetItem->IsSelectableInRangeToPickUp(Unit))
	{
		/* Just an FYI: PickUpInventoryItemOffGround anim should have an anim notify added to it 
		and should have idle anim at the end of it */
		
		// Pick it up now. Check if animation is set.
		if (Unit->IsAnimationAssigned(EAnimation::PickingUpInventoryItem))
		{
			/* We'll check before playing the anim whether we can pick it up */
			const EGameWarning Warning = Unit->GetInventory()->CanItemEnterInventory(TargetItem->GetType(), 
				TargetItem->GetItemQuantity(), *TargetItem->GetItemInfo(), Unit);
			const bool bCanPickUpItem = (Warning == EGameWarning::None);
			if (bCanPickUpItem)
			{
				DoOnMoveComplete = nullptr;
				StopMovement();

				SetUnitState(EUnitState::PickingUpInventoryItem);
				SetAnimStateForBehavior(EUnitAnimState::NotPlayingImportantAnim); // I guess it's not important, but may want to change this

				// Anim notify will say when we have picked up the item
				Unit->PlayAnimation(EAnimation::PickingUpInventoryItem, true);
			}
			else
			{
				StopMovementAndGoIdle();
			}
		}
		else // No pick up item anim set. Pick up item instantly
		{
			StopMovementAndGoIdle();

			GS->Server_TryPutItemInInventory(Unit, Unit->GetAttributesBase().IsSelected(),
				Unit->GetAttributesBase().IsPrimarySelected(), PS, TargetItem->GetType(), TargetItem->GetItemQuantity(),
				*TargetItem->GetItemInfo(), EItemAquireReason::PickedUpOffGround, 
				Unit->GetLocalPC()->GetHUDWidget(), TargetItem);
		}
	}
	else
	{
		// Move in range to pick it up
		
		ClickLocation = SomeLocation;

		SetUnitState(EUnitState::GoingToPickUpInventoryItem);
		SetAnimStateForBehavior(EUnitAnimState::NotPlayingImportantAnim);

		DoOnMoveComplete = &AInfantryController::OnMoveToPickUpInventoryItemComplete;
		Move(SomeLocation, GetPickUpItemAcceptanceRadius(TargetItem));
	}
}

void AInfantryController::OnLayFoundationCommand(EBuildingType BuildingType, const FVector & Location, const FRotator & Rotation)
{
	UE_VLOG(this, RTSLOG, Verbose, TEXT("Lay foundation command received"));

	OnCommand_ClearResourceGatheringProperties();
	OnCommand_ClearBuildingConstructingProperties();
	OnCommand_ClearTargetFocus();
	SetPendingContextActionType(EContextButton::None);

	SetUnitState(EUnitState::HeadingToPotentialConstructionSite);
	SetAnimStateForBehavior(EUnitAnimState::NotPlayingImportantAnim);

#if !UE_BUILD_SHIPPING
	const FBuildingInfo * BuildingInfo = FI->GetBuildingInfo(BuildingType);
	const EBuildingBuildMethod BuildMethod = BuildingInfo->GetBuildingBuildMethod();
	assert(BuildMethod == EBuildingBuildMethod::LayFoundationsWhenAtLocation
		|| BuildMethod == EBuildingBuildMethod::Protoss);
#endif

	/* Store these for when we get to the site */
	ClickLocation = Location;
	FoundationType = BuildingType;
	FoundationRotation = Rotation;

	/* Move to a location considered close enough to put foundations down */
	DoOnMoveComplete = &AInfantryController::TryLayFoundations;
	Move(Location, GetPotentialConstructionSiteAcceptanceRadius());
}

void AInfantryController::OnContextMenuPlaceBuildingResult(ABuilding * PlacedBuilding, const FBuildingInfo & BuildingInfo, 
	EBuildingBuildMethod BuildMethod)
{
	/* No matter the build method (except BuildsInTab and possibly BuildsItself too) we will get
	this notfication when the building is tried to be placed (provided we issued the order from
	context menu) */

	if (UnitState != EUnitState::WaitingForFoundationsToBePlaced
		&& BuildMethod != EBuildingBuildMethod::LayFoundationsInstantly)
	{
		// Unit is doing something else now, do not interrupt
		return;
	}

	UE_VLOG(this, RTSLOG, Verbose, TEXT("Place building result has come through. Result was: %s"
		"\nBuild method: %s"), PlacedBuilding == nullptr ? *FString("Failure") : *FString("Success"),
		TO_STRING(EBuildingBuildMethod, BuildMethod));

	// Assumption during this function is that unit is not moving

	/* Null implies building was not placed. */
	if (PlacedBuilding == nullptr)
	{
		/* The LayFoundationsInstantly check is actually
		because this is the first time AI controller knows about the building being placed so if
		it failed being placed then don't interrupt current behavior */
		if (BuildMethod != EBuildingBuildMethod::LayFoundationsInstantly)
		{
			GoIdle();
		}

		return;
	}

	if (BuildMethod == EBuildingBuildMethod::Protoss
		|| BuildMethod == EBuildingBuildMethod::BuildsItself)
	{
		// Protoss may want to move out of foundations here

		GoIdle();

		return;
	}

	/* Kind of expect this to always be true - if you can place buildings it is expected you can
	work on them. If it isn't though then just enter idle state */
	if (!bCanUnitBuildBuildings)
	{
		GoIdle();

		return;
	}

	/* Stop units actions and go over to construction site to build recently placed building */

	OnCommand_ClearTargetFocus();

	// Stop working on previous building if unit was
	OnCommand_ClearBuildingConstructingProperties();

	AssignedConstructionSite = PlacedBuilding;

	SetUnitState(EUnitState::HeadingToConstructionSite);
	SetAnimStateForBehavior(EUnitAnimState::NotPlayingImportantAnim);

	if (BuildMethod == EBuildingBuildMethod::LayFoundationsInstantly)
	{
		if (AssignedConstructionSite->CanAcceptBuilderChecked())
		{
			if (IsAtConstructionSite())
			{
				WorkOnBuilding();
			}
			else
			{
				DoOnMoveComplete = &AInfantryController::OnMoveToConstructionSiteComplete;
				
				// Take into account the fact that the building's location is below ground, so move it up a bit
				// I should do this for the IsAtConstructionSite() check too.
				// Also if the building is part way through construction this won't work because 
				// it would have already raised by some amount
				const FVector MoveLoc = PlacedBuilding->GetActorLocation() + BuildingInfo.GetBoundsHeight() / 2;
				
				Move(MoveLoc, GetConstructionSiteAcceptanceRadius());
			}
		}
		else
		{
			/* Somehow another worker is assigned to the building we just placed. Don't even
			know if possible but could be possible with latent actions. Anyway stand idle */
			GoIdle();
		}
	}
	else // BuildMethod == EBuildingBuildMethod::LayFoundationsWhenAtLocation
	{
		assert(BuildMethod == EBuildingBuildMethod::LayFoundationsWhenAtLocation);

		if (AssignedConstructionSite->CanAcceptBuilderChecked())
		{
			/* Should already be at build location with this build method but check anyway */
			if (IsAtConstructionSite())
			{
				WorkOnBuilding();
			}
			else
			{
				DoOnMoveComplete = &AInfantryController::OnMoveToConstructionSiteComplete;
				
				// Take into account the fact that the building's location is below ground, so move it up a bit
				const FVector MoveLoc = PlacedBuilding->GetActorLocation() + BuildingInfo.GetBoundsHeight() / 2;
				
				Move(MoveLoc, GetConstructionSiteAcceptanceRadius());
			}
		}
		else
		{
			GoIdle();
		}
	}
}

void AInfantryController::OnWorkedOnBuildingConstructionComplete()
{
	UE_VLOG(this, RTSLOG, Verbose, TEXT("Work on construction site complete"));

	/* Expected to be in state that is 'working on building'. If assert fires then maybe just
	change to an if */
	assert(UnitState == EUnitState::ConstructingBuilding);
	assert(AnimStateForBehavior == EUnitAnimState::ConstructingBuildingAnim);

	GoIdle();
}

void AInfantryController::OnUnitEnterStealthMode()
{
}

void AInfantryController::OnUnitExitStealthMode()
{
}

void AInfantryController::OnUnitTakeDamage(AActor * DamageCauser, float DamageAmount)
{
	if (!bUnitHasAttack)
	{
		// Sit there and take it if we cannot attack back
		return;
	}

	/* Check if we should aquire them as the new target */
	if (ShouldChangeTargetOnDamage(DamageCauser, DamageAmount))
	{
		assert(CanTargetBeAquired(DamageCauser));

		OnTargetChange(DamageCauser);
		AttackTarget = DamageCauser;

		// Possibly change unit state because target aquired
		if (UnitState == EUnitState::Idle_WithoutTarget)
		{
			SetUnitState(EUnitState::Idle_WithTarget);
		}
		else if (UnitState == EUnitState::AttackMoveCommand_WithNoTargetAquired)
		{
			SetUnitState(EUnitState::AttackMoveCommand_WithTargetAquired);
		}
		else if (UnitState == EUnitState::HoldingPositionWithoutTarget)
		{
			SetUnitState(EUnitState::HoldingPositionWithTarget);
		}

		/* Assuming things like team where checked in ShouldChangeTargetOnDamage. If some of
		these conditions below were actually checked in ShouldChangeTargetOnDamage then they
		can be removed */
		if (IsTargetInRange())
		{
			if (CanFire())
			{
				if (IsFacingAttackTarget())
				{
					BeginAttackAnim();
				}
			}
		}
	}
}

void AInfantryController::OnPossessedUnitDestroyed(AActor * DamageCauser)
{
	DoOnMoveComplete = nullptr;
	StopMovement();

	StopBehaviour();

	/* Remove from resource spots queue if unit was collecting/waiting at resource spot. 
	This maybe optional */
	if (UnitState == EUnitState::WaitingToGatherResources
		|| UnitState == EUnitState::GatheringResources)
	{
		AssignedResourceSpot->OnCollectorLeaveSpot(Unit);
	}

	// Stop constructing building
	if (Statics::IsValid(AssignedConstructionSite)
		&& UnitState == EUnitState::ConstructingBuilding)
	{
		assert(AnimStateForBehavior == EUnitAnimState::ConstructingBuildingAnim);
		AssignedConstructionSite->OnWorkerLost(Unit);
	}

	/* Anim notify will handle destroying this */
	Unit->PlayAnimation(EAnimation::Destroyed, false);

	/* Award exp to damage causer */
	if (Statics::IsValid(DamageCauser))
	{
		/* Award exp only if enemy */
		ISelectable * AsSelectable = CastChecked<ISelectable>(DamageCauser);
		if (PS->GetTeam() != AsSelectable->GetTeam())
		{
			AsSelectable->OnEnemyDestroyed(*Unit->GetAttributes());
		}
	}
}

void AInfantryController::OnPossessedUnitConsumed(AActor * ConsumptionCauser)
{
	DoOnMoveComplete = nullptr;
	StopMovement();
	StopBehaviour();

	/* Remove from resource spots queue if unit was collecting/waiting at resource spot.
	This maybe optional */
	if (UnitState == EUnitState::WaitingToGatherResources
		|| UnitState == EUnitState::GatheringResources)
	{
		AssignedResourceSpot->OnCollectorLeaveSpot(Unit);
	}

	// Stop constructing building
	if (Statics::IsValid(AssignedConstructionSite)
		&& UnitState == EUnitState::ConstructingBuilding)
	{
		assert(AnimStateForBehavior == EUnitAnimState::ConstructingBuildingAnim);
		AssignedConstructionSite->OnWorkerLost(Unit);
	}

	/* Award exp to damage causer */
	if (Statics::IsValid(ConsumptionCauser))
	{
		/* Award exp only if enemy */
		ISelectable * AsSelectable = CastChecked<ISelectable>(ConsumptionCauser);
		if (PS->GetTeam() != AsSelectable->GetTeam())
		{
			AsSelectable->OnEnemyDestroyed(*Unit->GetAttributes());
		}
	}

}

void AInfantryController::OnControlledPawnKilledSomething(const FSelectableAttributesBasic & EnemyAttributes)
{
}

void AInfantryController::AnimNotify_OnWeaponFired()
{
	/* Only try damage them if they still exist. Pretty light on the checks here. Could do
	range/rotation/visibility too but right now just using the 'once anim starts you will
	get attack' scheme */
	if (Statics::IsValid(AttackTarget))
	{
		OnAttackMade();
	}
	else
	{
		UE_VLOG(this, RTSLOG, Verbose, TEXT("Attack target was not valid at time anim notify "
			"to fire projectile was called"));
	}
}

void AInfantryController::AnimNotify_OnAttackAnimationFinished()
{
	UE_VLOG(this, RTSLOG, Verbose, TEXT("AnimNotify_OnAttackAnimationFinished called"));

	// If throw then this anim notfiy may be on an animation that is not the attack anim
	assert(AnimStateForBehavior == EUnitAnimState::DoingAttackAnim);

	SetAnimStateForBehavior(EUnitAnimState::NotPlayingImportantAnim);

	if (!Unit->bAllowRotationStraightAfterAttack)
	{
		if (Statics::IsValid(AttackTarget) && IsTargetAttackable())
		{
			// Start facing attack target again
			bIgnoreLookingAtFocus = false;
		}
		else
		{
			/* Go idle. Assumed target is dead or no longer attackable */
			GoIdle();

			// Might want to null target here perhaps
		}
	}
}

void AInfantryController::AnimNotify_OnContextActionExecuted()
{
	(this->* (PendingContextAction))();
}

void AInfantryController::AnimNotify_OnContextAnimationFinished()
{
	assert((UnitState == EUnitState::DoingContextActionAnim
			&& (AnimStateForBehavior == EUnitAnimState::DoingInterruptibleContextActionAnim
				|| AnimStateForBehavior == EUnitAnimState::DoingUninterruptibleContextActionAnim)) 
		|| 
			(UnitState == EUnitState::DoingSpecialBuildingTargetingAbility 
			&& AnimStateForBehavior == EUnitAnimState::DoingInterruptibleBuildingTargetingAbilityAnim));

	// Do not need to play anim here though if idle loop is tacked onto context action anim
	SetUnitState(EUnitState::Idle_WithoutTarget);
	SetAnimStateForBehavior(EUnitAnimState::NotPlayingImportantAnim);
	// Set LeashLocation here?
}

void AInfantryController::AnimNotify_OnResourcesDroppedOff()
{
	DropOffResources();
}

void AInfantryController::AnimNotify_TryPickUpInventoryItemOffGround()
{
	AInventoryItem * TargetAsItem = CastChecked<AInventoryItem>(ContextActionTarget);
	
	if (TargetAsItem->IsInObjectPool() == false)
	{
		// Try put item into inventory
		GS->Server_TryPutItemInInventory(Unit, Unit->GetAttributesBase().IsSelected(),
			Unit->GetAttributesBase().IsPrimarySelected(), PS, TargetAsItem->GetType(), TargetAsItem->GetItemQuantity(),
			*TargetAsItem->GetItemInfo(), EItemAquireReason::PickedUpOffGround, 
			Unit->GetLocalPC()->GetHUDWidget(), TargetAsItem);
	}

	LeashLocation = Unit->GetActorLocation();
	SetUnitState(EUnitState::Idle_WithoutTarget);
	SetAnimStateForBehavior(EUnitAnimState::NotPlayingImportantAnim);

	/* No play idle anim here because it is assumed the pick up item anim has idle loop
	at the end of it */
}

void AInfantryController::PlayIdleAnim()
{
	Unit->PlayAnimation(EAnimation::Idle, false);
}

void AInfantryController::GoIdle()
{
	//AttackTarget = nullptr;
	LeashLocation = Unit->GetActorLocation();
	Unit->PlayAnimation(EAnimation::Idle, false);
	SetUnitState(EUnitState::Idle_WithoutTarget);
	SetAnimStateForBehavior(EUnitAnimState::NotPlayingImportantAnim);
}

void AInfantryController::StopMovementAndGoIdle()
{
	LeashLocation = Unit->GetActorLocation();
	DoOnMoveComplete = nullptr;
	StopMovement();
	Unit->PlayAnimation(EAnimation::Idle, false);
	SetUnitState(EUnitState::Idle_WithoutTarget);
	SetAnimStateForBehavior(EUnitAnimState::NotPlayingImportantAnim);
}

void AInfantryController::StandStill()
{
	Unit->PlayAnimation(EAnimation::Idle, false);
	SetAnimStateForBehavior(EUnitAnimState::NotPlayingImportantAnim);

	DoOnMoveComplete = nullptr;

	StopMovement();
}

void AInfantryController::AttackMove_OnReturnedToLeashLoc()
{
	// Continue moving to click location
	SetUnitState(EUnitState::AttackMoveCommand_WithNoTargetAquired);
	Move(ClickLocation);
}

void AInfantryController::MoveToRallyPoint()
{
	SetUnitState(EUnitState::MovingToBarracksRallyPoint);
	DoOnMoveComplete = &AInfantryController::GoIdle;
	Move(Unit->BarracksRallyLoc);
}

float AInfantryController::GetPickUpItemAcceptanceRadius(AInventoryItem * ItemOnGround) const
{
	// Not very thorough
	return Unit->GetBoundsLength();
}

void AInfantryController::OnMoveToPickUpInventoryItemComplete()
{
	/* If AInventoryItem are pooled then this is fine, otherwise we will need to null and 
	possibly validity check it first */
	AInventoryItem * TargetAsItem = CastChecked<AInventoryItem>(ContextActionTarget);
	
	/* Check if we're in range to pick up item. Because we originally moved to a location and 
	not an actor we have to check whether the item actor is still on the ground. It's actually 
	probably better to not do a MoveToLocation and instead do a MoveToActor, although if they're 
	pooled then that will become interesting */
	if (TargetAsItem->IsInObjectPool() == false)
	{
		if (TargetAsItem->IsSelectableInRangeToPickUp(Unit))
		{
			if (Unit->IsAnimationAssigned(EAnimation::PickingUpInventoryItem))
			{
				// Check if we can still pick up the item before playing the animation
				const EGameWarning Result = Unit->GetInventory()->CanItemEnterInventory(TargetAsItem->GetType(), 
					TargetAsItem->GetItemQuantity(), *TargetAsItem->GetItemInfo(), Unit);
				const bool bCanPickUpItem = (Result == EGameWarning::None);
				if (bCanPickUpItem)
				{
					LeashLocation = Unit->GetActorLocation();
					SetUnitState(EUnitState::PickingUpInventoryItem);
					SetAnimStateForBehavior(EUnitAnimState::NotPlayingImportantAnim);
					
					Unit->PlayAnimation(EAnimation::PickingUpInventoryItem, true);
				}
				else
				{
					GoIdle();
				}
			}
			else
			{
				GoIdle();

				GS->Server_TryPutItemInInventory(Unit, Unit->GetAttributesBase().IsSelected(),
					Unit->GetAttributesBase().IsPrimarySelected(), PS, TargetAsItem->GetType(), 
					TargetAsItem->GetItemQuantity(), *TargetAsItem->GetItemInfo(), 
					EItemAquireReason::PickedUpOffGround, Unit->GetLocalPC()->GetHUDWidget(), 
					TargetAsItem);
			}
		}
		else // Didn't move close enough
		{
			GoIdle();

			UE_VLOG(this, RTSLOG, Warning, TEXT("Did not move close enough to inventory item to "
				"be able to pick it up"));
		}
	}
	else // Item is no longer in world. Could have been picked up by someone else during our move to it
	{
		GoIdle();
	}
}

void AInfantryController::StartResourceGatheringRoute(AResourceSpot * ResourcesToGatherFrom)
{
	/* If we're already assigned to this resource spot and are either waiting to collect from
	it, collecting from it or moving to collect from it then do nothing */
	if (ResourcesToGatherFrom == AssignedResourceSpot)
	{
		if (UnitState == EUnitState::HeadingToResourceSpot
			|| UnitState == EUnitState::WaitingToGatherResources
			|| UnitState == EUnitState::GatheringResources)
		{
			return;
		}
	}

	// Remove from queue of old resource spot if unit was gathering from another spot
	OnCommand_ClearResourceGatheringProperties();

	AssignedResourceSpot = ResourcesToGatherFrom;

	if (IsAtResourceSpot())
	{
		const EResourceType ResourceType = ResourcesToGatherFrom->GetType();

		/* Check if we already hold this resource. In that case return to depot immediately */
		if (Unit->GetHeldResourceType() == ResourceType
			&& Unit->HeldResourceAmount == Unit->GetCapacityForResource(ResourceType))
		{
			Depot = AssignedResourceSpot->GetClosestDepot(PS);
			ReturnToDepot();
			return;
		}

		TryGatherResources();
	}
	else
	{
		SetUnitState(EUnitState::HeadingToResourceSpot);
		SetAnimStateForBehavior(EUnitAnimState::NotPlayingImportantAnim);

		DoOnMoveComplete = &AInfantryController::OnMoveToResourceSpotComplete;
		Move(ResourcesToGatherFrom->GetActorLocation(), GetResourceSpotAcceptanceRadius());
	}
}

void AInfantryController::OnMoveToResourceSpotComplete()
{
	const EResourceType ResourceType = AssignedResourceSpot->GetType();

	if (IsAtResourceSpot())
	{
		/* Check if we already hold this resource. In that case return to depot immediately */
		if (Unit->GetHeldResourceType() == ResourceType
			&& Unit->HeldResourceAmount == Unit->GetCapacityForResource(ResourceType))
		{
			Depot = AssignedResourceSpot->GetClosestDepot(PS);
			ReturnToDepot();
			return;
		}

		/* Just stand here if resources are depleted */
		if (AssignedResourceSpot->IsDepleted())
		{
			GoIdle();
			return;
		}

		TryGatherResources();
	}
	else
	{
		/* Log something */
		float Distance = Statics::GetDistance2D(Unit->GetActorLocation(), AssignedResourceSpot->GetActorLocation());
		float DistanceRequired = GetDistanceRequirementForResourceSpot();
		UE_VLOG(this, RTSLOG, Warning, TEXT("Did not move close enough to resource spot to gather "
			"from it."
			"\nUnit's distance from spot: %f"
			"\nDistance required: %f"
			"\nDifference: %f"),
			Distance, DistanceRequired, Distance - DistanceRequired);

		/* We completed our move but we're not close enough to the resource spot to collect from
		it. Just stand idle or maybe start up another timer handle to retry moving to resource
		spot again. */
		GoIdle();
	}
}

void AInfantryController::TryGatherResources()
{
	assert(IsAtResourceSpot());

	/* Queue us up to the spot's queue. If we're first in queue we will be told to gather
	straight away by resource spot. If not then stand idle and spot will say when we can gather */
	if (!AssignedResourceSpot->Enqueue(Unit))
	{
		SetUnitState(EUnitState::WaitingToGatherResources);
		SetAnimStateForBehavior(EUnitAnimState::NotPlayingImportantAnim);

		DoOnMoveComplete = nullptr;
		StopMovement();

		Unit->PlayAnimation(EAnimation::Idle, false);
	}
}

bool AInfantryController::IsAtResourceSpot() const
{
	const float DistanceSqr = Statics::GetDistance2DSquared(Unit->GetActorLocation(), AssignedResourceSpot->GetActorLocation());
	return DistanceSqr <= FMath::Square(GetDistanceRequirementForResourceSpot());
}

float AInfantryController::GetDistanceRequirementForResourceSpot() const
{
	return Unit->GetBoundsLength() + AssignedResourceSpot->GetCollectionAcceptanceRadius()
		+ Lenience_ResourceSpotCollection;
}

float AInfantryController::GetResourceSpotAcceptanceRadius() const
{
	return AssignedResourceSpot->GetCollectionAcceptanceRadius() - Unit->GetBoundsLength()
		+ AcceptanceRadiusModifier_ResourceSpot;
}

#if !UE_BUILD_SHIPPING
bool AInfantryController::IsWaitingToCollectResources() const
{
	return UnitState == EUnitState::WaitingToGatherResources && IsAtResourceSpot();
}
#endif

void AInfantryController::StartCollectingResources()
{
	const float GatherRate = Unit->GetResourceGatherRate(AssignedResourceSpot->GetType());

	if (GatherRate <= 0.f)
	{
		/* Gather resources instantly */
		OnResourceGatheringComplete();
	}
	else
	{
		SetUnitState(EUnitState::GatheringResources);
		SetAnimStateForBehavior(EUnitAnimState::PlayingGatheringResourcesAnim);

		Unit->PlayAnimation(EAnimation::GatheringResources, false);

		Delay(TimerHandle_ResourceGathering, &AInfantryController::OnResourceGatheringComplete,
			GatherRate);
	}
}

void AInfantryController::OnResourceGatheringComplete()
{
	const EResourceType ResourceType = AssignedResourceSpot->GetType();

	/* Take resources from resource spot */
	Unit->SetHeldResource(ResourceType, AssignedResourceSpot->TakeResourcesFrom(Unit->GetCapacityForResource(ResourceType), PS));

	/* Let resource spot know we are done collecting to allow another unit to collect from it */
	AssignedResourceSpot->OnCollectorLeaveSpot(Unit);

	/* Call explicitly because not called automatically on server */
	Unit->OnRep_HeldResourceType();

	/* Return to depot now */
	Depot = AssignedResourceSpot->GetClosestDepot(PS);
	ReturnToDepot();
}

void AInfantryController::ReturnToDepot()
{
	/* Important that PS::UpdateClosestDepots runs on client when a building completes/destroyed */
	if (Statics::IsValid(Depot))
	{
		SetUnitState(EUnitState::ReturningToResourceDepot);
		SetAnimStateForBehavior(EUnitAnimState::NotPlayingImportantAnim);

		/* Return to depot */
		if (IsAtDepot())
		{
			/* Already there */
			OnReturnedToDepot();
		}
		else
		{
			DoOnMoveComplete = &AInfantryController::OnReturnedToDepotMoveComplete;
			Move(Depot->GetActorLocation(), GetDepotAcceptanceRadius());
		}
	}
	else
	{
		/* No depot to return to. Stand idle */
		GoIdle();
	}
}

bool AInfantryController::IsAtDepot() const
{
	float Distance2DSqr = Statics::GetDistance2DSquared(Unit->GetActorLocation(), Depot->GetActorLocation());
	return Distance2DSqr <= FMath::Square(GetDistanceRequirementForDepot());
}

float AInfantryController::GetDistanceRequirementForDepot() const
{
	return Unit->GetBoundsLength() + Depot->GetBuildingAttributes()->GetResourceDropRadius()
		+ Lenience_ResourceDepotDropOff;
}

float AInfantryController::GetDepotAcceptanceRadius() const
{
	return Depot->GetBuildingAttributes()->GetResourceDropRadius() - Unit->GetBoundsLength()
		+ AcceptanceRadiusModifier_ResourceDepot;
}

void AInfantryController::OnReturnedToDepotMoveComplete()
{
	if (IsAtDepot())
	{
		OnReturnedToDepot();
	}
	else
	{
		/* Move to depot complete but not close enough. Could have been destroyed */
		if (!Statics::IsValid(Depot) || Statics::HasZeroHealth(Depot))
		{
			// Depot destroyed
		}
		else
		{
			/* Move did not move us close enough. Perhaps retry */

			float Distance = Statics::GetDistance2D(Unit->GetActorLocation(), Depot->GetActorLocation());
			float DistanceRequired = GetDistanceRequirementForDepot();
			UE_VLOG(this, RTSLOG, Warning, TEXT("Did not move close enough to resource depot to "
				"drop off resources."
				"\nUnit's distance from depot: %f"
				"\nDistance required: %f"
				"\nDifference: %f"),
				Distance, DistanceRequired, Distance - DistanceRequired);
		}

		GoIdle();
	}
}

void AInfantryController::OnReturnedToDepot()
{
	/* If a drop off animation is set then play that. Otherwise just drop off resources instantly
	and return to resource spot */
	if (Unit->IsAnimationAssigned(EAnimation::DropOffResources))
	{
		SetUnitState(EUnitState::DroppingOfResources);
		SetAnimStateForBehavior(EUnitAnimState::PlayingDropOffResourcesAnim);

		/* Anim notify should handle when the drop is considered complete */
		Unit->PlayAnimation(EAnimation::DropOffResources, false);
	}
	else
	{
		DropOffResources();
	}
}

void AInfantryController::DropOffResources()
{
	/* Add resources to players spendable resources */
	PS->GainResource(Unit->GetHeldResourceType(), Unit->GetHeldResourceAmount(), true);

	/* Remove the resource from unit */
	Unit->SetHeldResource(EResourceType::None, 0);
	Unit->OnRep_HeldResourceType();

	/* Return to the resource spot if not depleted */
	if (AssignedResourceSpot->IsDepleted())
	{
		GoIdle();
	}
	else
	{
		ReturnToResourceSpot();
	}
}

void AInfantryController::ReturnToResourceSpot()
{
	SetUnitState(EUnitState::HeadingToResourceSpot);
	SetAnimStateForBehavior(EUnitAnimState::NotPlayingImportantAnim);

	DoOnMoveComplete = &AInfantryController::OnMoveToResourceSpotComplete;
	Move(AssignedResourceSpot->GetActorLocation(), GetResourceSpotAcceptanceRadius());
}

bool AInfantryController::IsAtPotentialConstructionSite() const
{
	/* May want to throw some lenience on here. Also with current implementation we cannot be in
	the way of potential location so need to make sure raidus is large enough for this. Different
	unit capsule sizes could be a problem */
	float DistanceSqr = Statics::GetDistance2DSquared(Unit->GetActorLocation(), ClickLocation);
	return DistanceSqr <= FMath::Square(GetDistanceRequirementForPotentialConstructionSite());
}

float AInfantryController::GetDistanceRequirementForPotentialConstructionSite() const
{
	const FBuildingInfo * BuildingInfo = FI->GetBuildingInfo(FoundationType);
	return Unit->GetBoundsLength() + BuildingInfo->GetFoundationRadius()
		+ Lenience_PotentialConstructionSite;
}

float AInfantryController::GetPotentialConstructionSiteAcceptanceRadius() const
{
	const FBuildingInfo * BuildingInfo = FI->GetBuildingInfo(FoundationType);
	return BuildingInfo->GetFoundationRadius() - Unit->GetBoundsLength()
		+ AcceptanceRadiusModifier_PotentialConstructionSite;
}

void AInfantryController::TryLayFoundations()
{
	if (IsAtPotentialConstructionSite())
	{
		SetUnitState(EUnitState::WaitingForFoundationsToBePlaced);
		SetAnimStateForBehavior(EUnitAnimState::NotPlayingImportantAnim);

		/* Once this function completes it will then call OnContextMenuPlaceBuildingSuccess */
		PS->GetPC()->Server_PlaceBuilding(FoundationType, ClickLocation, FoundationRotation,
			Unit->GetSelectableID());

		// OnContextMenuPlaceBuildingResult will be called next by PC
	}
	else
	{
		/* Didn't move close enough */

		float Distance = Statics::GetDistance2D(Unit->GetActorLocation(), ClickLocation);
		float DistanceRequired = GetDistanceRequirementForPotentialConstructionSite();
		UE_VLOG(this, RTSLOG, Warning, TEXT("Did not move close enough to potential construction "
			"site to lay foundations"
			"\nUnit's distance from site: %f"
			"\nDistance required: %f"
			"\nDifference: %f"),
			Distance, DistanceRequired, Distance - DistanceRequired);

		GoIdle();
	}
}

bool AInfantryController::IsAtConstructionSite() const
{
	float Distance2DSqr = Statics::GetDistance2DSquared(Unit->GetActorLocation(), AssignedConstructionSite->GetActorLocation());
	return Distance2DSqr <= FMath::Square(GetDistanceRequirementForConstructionSite());
}

float AInfantryController::GetDistanceRequirementForConstructionSite() const
{
	return Unit->GetBoundsLength() + AssignedConstructionSite->GetBuildingAttributes()->GetFoundationsRadius()
		+ Lenience_ConstructionSite;
}

float AInfantryController::GetConstructionSiteAcceptanceRadius() const
{
	// Minus a bit so unit will try and move closer
	return AssignedConstructionSite->GetBuildingAttributes()->GetFoundationsRadius()
		- Unit->GetBoundsLength() + AcceptanceRadiusModifier_ConstructionSite;
}

void AInfantryController::OnMoveToConstructionSiteComplete()
{
	if (Statics::IsValid(AssignedConstructionSite))
	{
		if (IsAtConstructionSite())
		{
			if (AssignedConstructionSite->CanAcceptBuilderChecked())
			{
				WorkOnBuilding();
			}
			else // Building may have finished construction already or another worker got assigned to it
			{
				GoIdle();
			}
		}
		else
		{
			/* Didn't move close enough. Just stand idle */

			float Distance = Statics::GetDistance2D(Unit->GetActorLocation(), AssignedConstructionSite->GetActorLocation());
			float DistanceRequired = GetDistanceRequirementForConstructionSite();
			UE_VLOG(this, RTSLOG, Warning, TEXT("Did not move close enough to construction site to "
				"work on it"
				"\nUnit's distance from site: %f"
				"\nDistance required: %f"
				"\nDifference: %f"),
				Distance, DistanceRequired, Distance - DistanceRequired);

			GoIdle();
		}
	}
	else // Building no longer around
	{
		GoIdle();
	}
}

void AInfantryController::WorkOnBuilding()
{
	assert(AssignedConstructionSite->CanAcceptBuilder());
	/* Do not assert that state isn;t already ConstructingBuilding because may be switching from
	constructing one building to another */

	SetUnitState(EUnitState::ConstructingBuilding);
	SetAnimStateForBehavior(EUnitAnimState::ConstructingBuildingAnim);

	Unit->PlayAnimation(EAnimation::ConstructBuilding, false);

	AssignedConstructionSite->OnWorkerGained(Unit);
}

void AInfantryController::SetPendingContextActionType(EContextButton InActionType)
{
	PendingContextActionType = static_cast<uint8>(InActionType);
}

void AInfantryController::SetPendingContextActionType(EBuildingTargetingAbility InActionType)
{
	PendingContextActionType = static_cast<uint8>(InActionType);
}

EContextButton AInfantryController::GetPendingContextActionType() const
{
	assert(PendingAbilityUsageCase == EAbilityUsageType::SelectablesActionBar || PendingAbilityUsageCase == EAbilityUsageType::SelectablesInventory);
	return static_cast<EContextButton>(PendingContextActionType);
}

EBuildingTargetingAbility AInfantryController::GetPendingBuildingTargetingAbilityType() const
{
	assert(PendingAbilityUsageCase == EAbilityUsageType::SpecialBuildingTargetingAbility);
	return static_cast<EBuildingTargetingAbility>(PendingContextActionType);
}

void AInfantryController::TryDoInstantContextCommand()
{
	assert(Statics::IsValid(Unit) && !Statics::HasZeroHealth(Unit));
	
	const FContextButtonInfo & AbilityInfo = GI->GetContextInfo(GetPendingContextActionType());

	bool bCanDoAbility = false;
	EGameWarning Warning = EGameWarning::None;
	EAbilityRequirement CustomWarning = EAbilityRequirement::Uninitialized;

	/* This func is generally called after triggering an anim notify. The behavior we do is that 
	if at the time of the anim notify we do not meet the requirements to use the ability then 
	the ability will not be carried out and instead a HUD warning will be dispatched */
	if (PendingAbilityUsageCase == EAbilityUsageType::SelectablesInventory)
	{
		const FInventory * Inventory = Unit->GetInventory();
		const FInventorySlotState * InvSlot = &Inventory->GetSlotGivenServerIndex(PendingAbilityAuxilleryData);
	
		// Do some checks
		if (InvSlot->GetItemType() == static_cast<EInventoryItem>(PendingAbilityMoreAuxilleryData))
		{
			if (InvSlot->GetNumCharges() != 0)
			{
				if (Unit->HasEnoughSelectableResources(AbilityInfo))
				{
					AAbilityBase * AbilityEffectActor = AbilityInfo.GetEffectActor(GS);
					if (AbilityEffectActor->IsUsable_SelfChecks(Unit, CustomWarning))
					{
						bCanDoAbility = true;
					}
				}
				else
				{
					Warning = EGameWarning::NotEnoughSelectableResources;
				}
			}
			else
			{
				Warning = EGameWarning::ItemOutOfCharges;
			}
		}
		else
		{
			Warning = EGameWarning::ItemNoLongerInInventory;
		}
	}
	else // Assuming action bar
	{
		assert(PendingAbilityUsageCase == EAbilityUsageType::SelectablesActionBar);

		// Do some checks
		if (Unit->HasEnoughSelectableResources(AbilityInfo))
		{
			AAbilityBase * AbilityEffectActor = AbilityInfo.GetEffectActor(GS);
			if (AbilityEffectActor->IsUsable_SelfChecks(Unit, CustomWarning))
			{
				bCanDoAbility = true;
			}
		}
		else
		{
			Warning = EGameWarning::NotEnoughSelectableResources;
		}
	}

	//------------------------------------------------------------------
	//	Final stage - either do ability or dispatch warning
	//------------------------------------------------------------------

	if (bCanDoAbility)
	{
		DoInstantContextCommand();
	}
	else
	{
		if (Warning != EGameWarning::None)
		{
			PS->OnGameWarningHappened(Warning);
		}
		else // Assumed CustomWarning has something
		{
			PS->OnGameWarningHappened(CustomWarning);
		}

		SetPendingContextActionType(EContextButton::None);

		/* Because ability did not follow through we will cancel continuing to play the ability 
		anim and go idle instead */
		GoIdle();
	}
}

void AInfantryController::DoInstantContextCommand()
{
	GS->Server_CreateAbilityEffect(PendingAbilityUsageCase, PendingAbilityAuxilleryData,
		GI->GetContextInfo(GetPendingContextActionType()), Unit, Unit,
		Unit->GetTeam(), Unit->GetActorLocation(), Unit, Unit);

	SetPendingContextActionType(EContextButton::None);
}

float AInfantryController::GetDistanceFromLocationForAbility(const FContextButtonInfo & AbilityInfo,
	const FVector & Location) const
{
	// Note: can return negative numbers
	return Statics::GetDistance2D(Unit->GetActorLocation(), Location) 
		- Unit->GetBoundsLength() 
		- Lenience_GroundLocationTargetingAbility; 
}

float AInfantryController::GetLocationTargetedAcceptanceRadius(const FContextButtonInfo & AbilityInfo) const
{
	return AbilityInfo.GetMaxRange() - Unit->GetBoundsLength() + AcceptanceRadiusModifier_MoveToAbilityLocation;
}

void AInfantryController::OnMoveToLocationTargetedContextActionComplete()
{
	const FContextButtonInfo & AbilityInfo = GI->GetContextInfo(GetPendingContextActionType());

	/* Check if moved close enough. Should really call 
	Statics::GetSelectablesDistanceForAbilitySquared instead */ 
	const float DistanceSqr = FMath::Square(GetDistanceFromLocationForAbility(AbilityInfo, ClickLocation));
	const float DistanceRequiredSqr = Statics::GetAbilityRangeSquared(AbilityInfo);
	const bool bInRange = (DistanceSqr <= DistanceRequiredSqr);

	if (bInRange)
	{
		bool bCanDoAbility = false;
		EGameWarning Warning = EGameWarning::None;
		EAbilityRequirement CustomWarning = EAbilityRequirement::Uninitialized;

		if (PendingAbilityUsageCase == EAbilityUsageType::SelectablesInventory)
		{
			const FInventory * Inventory = Unit->GetInventory();
			const FInventorySlotState * InvSlot = &Inventory->GetSlotGivenServerIndex(PendingAbilityAuxilleryData);

			if (InvSlot->GetItemType() == static_cast<EInventoryItem>(PendingAbilityMoreAuxilleryData))
			{
				if (InvSlot->GetNumCharges() != 0)
				{
					if (Unit->HasEnoughSelectableResources(AbilityInfo))
					{
						/* Do we wanna check fog here? */

						AAbilityBase * AbilityEffectActor = AbilityInfo.GetEffectActor(GS);
						if (AbilityEffectActor->IsUsable_SelfChecks(Unit, CustomWarning))
						{
							bCanDoAbility = true;
						}
					}
					else
					{
						Warning = EGameWarning::NotEnoughSelectableResources;
					}
				}
				else
				{
					Warning = EGameWarning::ItemOutOfCharges;
				}
			}
			else
			{
				Warning = EGameWarning::ItemNoLongerInInventory;
			}
		}
		else // Assumed action bar
		{
			assert(PendingAbilityUsageCase == EAbilityUsageType::SelectablesActionBar);

			if (Unit->HasEnoughSelectableResources(AbilityInfo))
			{
				AAbilityBase * AbilityEffectActor = AbilityInfo.GetEffectActor(GS);
				if (AbilityEffectActor->IsUsable_SelfChecks(Unit, CustomWarning))
				{
					bCanDoAbility = true;
				}
			}
			else
			{
				Warning = EGameWarning::NotEnoughSelectableResources;
			}
		}

		//------------------------------------------------------------------
		//	Final stage - either continue with ability or dispatch warning
		//------------------------------------------------------------------

		if (bCanDoAbility)
		{
			StartLocationTargetedContextAction(AbilityInfo);
		}
		else
		{
			if (Warning != EGameWarning::None)
			{
				PS->OnGameWarningHappened(Warning);
			}
			else // Assumed custom warning has something
			{
				PS->OnGameWarningHappened(CustomWarning);
			}
		}
	}
	else
	{
		// Did not move close enough to click location

		UE_VLOG(this, RTSLOG, Warning, TEXT("Did not move close enough to start location targeting "
			"context action. \nAction was: %s\nUnit's distance from location: %f\nDistance required: %f"),
			TO_STRING(EContextButton, GetPendingContextActionType()),
			FMath::Sqrt(DistanceSqr), FMath::Sqrt(DistanceRequiredSqr));

		SetPendingContextActionType(EContextButton::None);
		GoIdle();
	}
}

void AInfantryController::StartLocationTargetedContextAction(const FContextButtonInfo & AbilityInfo)
{
	if (AbilityInfo.UsesAnimation())
	{
		SetUnitState(EUnitState::DoingContextActionAnim);
		if (AbilityInfo.DoesAnimationBlockCommands())
		{
			SetAnimStateForBehavior(EUnitAnimState::DoingUninterruptibleContextActionAnim);
		}
		else
		{
			SetAnimStateForBehavior(EUnitAnimState::DoingInterruptibleContextActionAnim);
		}

		PendingContextAction = &AInfantryController::TryDoLocationTargetedContextAction;

		/* Play animation. Let anim notify handle spawing the effect */
		Unit->PlayAnimation(AbilityInfo.GetAnimationType(), true);
	}
	else
	{
		/* Do ability now */
		DoLocationTargetedContextAction();

		// Go idle
		GoIdle();
	}
}

void AInfantryController::TryDoLocationTargetedContextAction()
{
	assert(Statics::IsValid(Unit) && !Statics::HasZeroHealth(Unit));

	const FContextButtonInfo & AbilityInfo = GI->GetContextInfo(GetPendingContextActionType());

	bool bCanDoAbility = false;
	EGameWarning Warning = EGameWarning::None;
	EAbilityRequirement CustomWarning = EAbilityRequirement::Uninitialized;
	
	// Do some checks
	if (PendingAbilityUsageCase == EAbilityUsageType::SelectablesInventory)
	{
		const FInventory * Inventory = Unit->GetInventory();
		const FInventorySlotState * InvSlot = &Inventory->GetSlotGivenServerIndex(PendingAbilityAuxilleryData);

		if (InvSlot->GetItemType() == static_cast<EInventoryItem>(PendingAbilityMoreAuxilleryData))
		{
			if (InvSlot->GetNumCharges() != 0)
			{
				if (Unit->HasEnoughSelectableResources(AbilityInfo))
				{
					/* Do we wanna check fog here? */
					
					AAbilityBase * AbilityEffectActor = AbilityInfo.GetEffectActor(GS);
					if (AbilityEffectActor->IsUsable_SelfChecks(Unit, CustomWarning))
					{
						bCanDoAbility = true;
					}
				}
				else
				{
					Warning = EGameWarning::NotEnoughSelectableResources;
				}
			}
			else
			{
				Warning = EGameWarning::ItemOutOfCharges;
			}
		}
		else
		{
			Warning = EGameWarning::ItemNoLongerInInventory;
		}
	}
	else // Assumed action bar
	{
		assert(PendingAbilityUsageCase == EAbilityUsageType::SelectablesActionBar);

		if (Unit->HasEnoughSelectableResources(AbilityInfo))
		{
			AAbilityBase * AbilityEffectActor = AbilityInfo.GetEffectActor(GS);
			if (AbilityEffectActor->IsUsable_SelfChecks(Unit, CustomWarning))
			{
				bCanDoAbility = true;
			}
		}
		else
		{
			Warning = EGameWarning::NotEnoughSelectableResources;
		}
	}

	//------------------------------------------------------------------
	//	Final stage - either do ability or dispatch warning
	//------------------------------------------------------------------

	if (bCanDoAbility)
	{
		DoLocationTargetedContextAction();
	}
	else
	{
		if (Warning != EGameWarning::None)
		{
			PS->OnGameWarningHappened(Warning);
		}
		else // Assumed custom warning has something
		{
			PS->OnGameWarningHappened(CustomWarning);
		}

		SetPendingContextActionType(EContextButton::None);
		GoIdle();
	}
}

void AInfantryController::DoLocationTargetedContextAction()
{
	/* Spawn the ability */
	GS->Server_CreateAbilityEffect(PendingAbilityUsageCase, PendingAbilityAuxilleryData, 
		GI->GetContextInfo(GetPendingContextActionType()), Unit, Unit,
		Unit->GetTeam(), ClickLocation, Unit, Unit);

	SetPendingContextActionType(EContextButton::None);

	/* AnimNotify_OnContextAnimationFinished() should be called some time after this */
}

void AInfantryController::OnMoveToBuildingForBuildingTargetingAbilityComplete()
{
	/* Check target is alive */
	if (Statics::IsValid(ContextActionTarget) && !Statics::HasZeroHealth(ContextActionTarget))
	{
		const FBuildingTargetingAbilityStaticInfo & AbilityInfo = GI->GetBuildingTargetingAbilityInfo(GetPendingBuildingTargetingAbilityType());
		ABuilding * TargetBuilding = CastChecked<ABuilding>(ContextActionTarget);

		/* Check if moved close enough */
		const float Distance = GetDistanceFromAnotherSelectableForAbility(AbilityInfo, TargetBuilding);
		const bool bInRange = FMath::Square(Distance) <= FMath::Square(AbilityInfo.GetRange());
		if (bInRange)
		{
			StartBuildingTargetingAbility();
		}
		else /* Did not move close enough */
		{
			UE_VLOG(this, RTSLOG, Warning, TEXT("Did not move close enough to building to carry "
				"out special building targeting ability"));

			SetPendingContextActionType(EContextButton::None);
			GoIdle();
		}
	}
	else /* Target no longer alive */
	{
		SetPendingContextActionType(EContextButton::None);
		GoIdle();
	}
}

void AInfantryController::StartBuildingTargetingAbility()
{
	const FBuildingTargetingAbilityStaticInfo & AbilityInfo = GI->GetBuildingTargetingAbilityInfo(GetPendingBuildingTargetingAbilityType());

	if (AbilityInfo.HasAnimation())
	{
		SetUnitState(EUnitState::DoingSpecialBuildingTargetingAbility);
		SetAnimStateForBehavior(EUnitAnimState::DoingInterruptibleBuildingTargetingAbilityAnim);
		
		PendingContextAction = &AInfantryController::TryDoBuildingTargetingAbility;

		/* Anim notifies should handle the rest */
		Unit->PlayAnimation(AbilityInfo.GetAnimation(), true);
	}
	else
	{
		DoBuildingTargetingAbility();
		GoIdle();
	}
}

void AInfantryController::TryDoBuildingTargetingAbility()
{
	DoBuildingTargetingAbility();
}

void AInfantryController::DoBuildingTargetingAbility()
{
	GS->Server_CreateAbilityEffect(GI->GetBuildingTargetingAbilityInfo(GetPendingBuildingTargetingAbilityType()),
		Unit, CastChecked<ABuilding>(ContextActionTarget));

	SetPendingContextActionType(EContextButton::None);
}

float AInfantryController::GetDistanceFromAnotherSelectableForAbility(const FContextButtonInfo & AbilityInfo,
	ISelectable * Target) const
{
	/* Take into account our own bounds and theirs. Also no lenience here but may need to add
	some */
	// Note: can return negative numbers
	return Statics::GetDistance2D(Unit->GetActorLocation(), Target->GetActorLocationSelectable())
		- Unit->GetBoundsLength()
		- ContextActionTargetBoundsLength;
}

float AInfantryController::GetDistanceFromAnotherSelectableForAbility(const FBuildingTargetingAbilityStaticInfo & AbilityInfo, 
	ABuilding * TargetBuilding) const
{
	/* Copy of overload, but could maybe do better */
	return Statics::GetDistance2D(Unit->GetActorLocation(), TargetBuilding->GetActorLocation())
		- Unit->GetBoundsLength()
		- ContextActionTargetBoundsLength;
}

float AInfantryController::GetDistanceFromBuildingForEnteringItsGarrison(ABuilding * TargetBuilding) const
{
	/* Could possibly do better with this */
	return Statics::GetDistance2D(Unit->GetActorLocation(), TargetBuilding->GetActorLocation())
		- Unit->GetBoundsLength()
		- ContextActionTargetBoundsLength;
}

float AInfantryController::GetSelectableTargetingAbilityAcceptanceRadius(const FContextButtonInfo & AbilityInfo) const
{
	return AbilityInfo.GetMaxRange() - Unit->GetBoundsLength() + AcceptanceRadiusModifier_MoveToActor;
}

float AInfantryController::GetBuildingTargetingAbilityAcceptanceRadius(const FBuildingTargetingAbilityStaticInfo & AbilityInfo) const
{
	return AbilityInfo.GetRange() - Unit->GetBoundsLength() + AcceptanceRadiusModifier_MoveToBuildingTargetingAbilityTarget;
}

float AInfantryController::GetBuildingEnterGarrisonAcceptanceRadius(const FBuildingGarrisonAttributes & BuildingsGarrisonAttributes) const
{
	return BuildingsGarrisonAttributes.GetEnterRange() - Unit->GetBoundsLength() + AcceptanceRadiusModifier_MoveToEnterBuildingGarrison;
}

void AInfantryController::OnMoveToSingleTargetContextActionTargetComplete()
{
	/* Check target is alive */
	if (Statics::IsValid(ContextActionTarget) && !Statics::HasZeroHealth(ContextActionTarget))
	{
		const FContextButtonInfo & AbilityInfo = GI->GetContextInfo(GetPendingContextActionType());
		ISelectable * TargetAsSelectable = CastChecked<ISelectable>(ContextActionTarget);

		/* Check we moved close enough to target. We should really call
		Statics::GetSelectablesDistanceForAbilitySquared but that just ultimatly calls
		AInfantryController::GetDistanceFromAnotherSelectableForAbilitySquared so we just call
		it directly */
		const float DistanceSqr = FMath::Square(GetDistanceFromAnotherSelectableForAbility(AbilityInfo, TargetAsSelectable));
		const float RequiredDistanceSqr = Statics::GetAbilityRangeSquared(AbilityInfo);
		const bool bInRange = (DistanceSqr <= RequiredDistanceSqr);
		if (bInRange)
		{
			bool bCanDoAbility = false;
			EGameWarning Warning = EGameWarning::None;
			EAbilityRequirement CustomWarning = EAbilityRequirement::Uninitialized;
				
			if (Statics::IsSelectableVisible(ContextActionTarget, TeamVisInfo))
			{
				if (Unit->HasEnoughSelectableResources(AbilityInfo))
				{
					/* Check ability's custom checks */
					AAbilityBase * EffectActor = AbilityInfo.GetEffectActor(GS);
					if (EffectActor->IsUsable_SelfChecks(Unit, CustomWarning))
					{
						if (EffectActor->IsUsable_TargetChecks(Unit, TargetAsSelectable, CustomWarning))
						{
							bCanDoAbility = true;
						}
					}
				}
				else
				{
					Warning = EGameWarning::NotEnoughSelectableResources;
				}
			}
			else
			{
				Warning = EGameWarning::TargetNotVisible;
			}

			//------------------------------------------------------------------
			//	Final stage - either do ability or dispatch warning
			//------------------------------------------------------------------

			if (bCanDoAbility)
			{
				StartSingleTargetContextAction(AbilityInfo);
			}
			else
			{
				/* Perhaps don't care about warnings instead? */
				if (Warning != EGameWarning::None)
				{
					PS->OnGameWarningHappened(Warning);
				}
				else // Assumed custom warning has something
				{
					PS->OnGameWarningHappened(CustomWarning);
				}

				SetPendingContextActionType(EContextButton::None);
				GoIdle();
			}
		}
		else
		{
			// Didn't move close enough

			UE_VLOG(this, RTSLOG, Warning, TEXT("Did not move close enough to target to use "
				"ability.\nAbility type: %s\nUnit's distance from target: %f\nDistance required: %f"),
				TO_STRING(EContextButton, GetPendingContextActionType()),
				FMath::Sqrt(DistanceSqr), FMath::Sqrt(RequiredDistanceSqr));

			SetPendingContextActionType(EContextButton::None);
			GoIdle();
		}
	}
	else
	{
		/* Target no longer alive */

		SetPendingContextActionType(EContextButton::None);
		GoIdle();
	}
}

void AInfantryController::StartSingleTargetContextAction(const FContextButtonInfo & AbilityInfo)
{
	if (AbilityInfo.UsesAnimation())
	{
		SetUnitState(EUnitState::DoingContextActionAnim);
		if (AbilityInfo.DoesAnimationBlockCommands())
		{
			SetAnimStateForBehavior(EUnitAnimState::DoingUninterruptibleContextActionAnim);
		}
		else
		{
			SetAnimStateForBehavior(EUnitAnimState::DoingInterruptibleContextActionAnim);
		}

		PendingContextAction = &AInfantryController::TryDoSingleTargetContextAction;

		/* Play animation. Let anim notify handle spawing the effect */
		Unit->PlayAnimation(AbilityInfo.GetAnimationType(), true);
	}
	else
	{
		/* Do ability now */
		DoSingleTargetContextAction();

		// Go idle
		GoIdle();
	}
}

void AInfantryController::TryDoSingleTargetContextAction()
{
	assert(Statics::IsValid(Unit) && !Statics::HasZeroHealth(Unit));

	const FContextButtonInfo & AbilityInfo = GI->GetContextInfo(GetPendingContextActionType());

	bool bCanDoAbility = false;
	EGameWarning Warning = EGameWarning::None;
	EAbilityRequirement CustomWarning = EAbilityRequirement::Uninitialized;

	// Do some checks
	if (PendingAbilityUsageCase == EAbilityUsageType::SelectablesInventory)
	{
		if (Statics::IsValid(ContextActionTarget) && !Statics::HasZeroHealth(ContextActionTarget))
		{
			const FInventory * Inventory = Unit->GetInventory();
			const FInventorySlotState * InvSlot = &Inventory->GetSlotGivenServerIndex(PendingAbilityAuxilleryData);

			if (InvSlot->GetItemType() == static_cast<EInventoryItem>(PendingAbilityMoreAuxilleryData))
			{
				if (InvSlot->GetNumCharges() != 0)
				{
					if (Unit->HasEnoughSelectableResources(AbilityInfo))
					{
						// This is ment to be a fog and stealth check
						if (AbilityInfo.PassesAnimNotifyTargetVisibilityCheck(ContextActionTarget))
						{
							// Will need to a bool to FContextButtonInfo saying whether to actually check anything
							ISelectable * TargetAsSelectable = CastChecked<ISelectable>(ContextActionTarget);
							if (AbilityInfo.PassesAnimNotifyRangeCheck<AInfantry, AInfantryController>(Unit, this, TargetAsSelectable))
							{
								AAbilityBase * AbilityEffect = AbilityInfo.GetEffectActor(GS);
								if (AbilityEffect->IsUsable_SelfChecks(Unit, CustomWarning))
								{
									if (AbilityEffect->IsUsable_TargetChecks(Unit, TargetAsSelectable, CustomWarning))
									{
										bCanDoAbility = true;
									}
								}
							}
							else
							{
								Warning = EGameWarning::AbilityOutOfRange;
							}
						}
						else
						{
							Warning = EGameWarning::TargetNotVisible;
						}
					}
					else
					{
						Warning = EGameWarning::NotEnoughSelectableResources;
					}
				}
				else
				{
					Warning = EGameWarning::ItemOutOfCharges;
				}
			}
			else
			{
				Warning = EGameWarning::ItemNoLongerInInventory;
			}
		}
		else
		{
			Warning = EGameWarning::TargetNoLongerAlive;
		}
	}
	else // Assumed action bar
	{
		assert(PendingAbilityUsageCase == EAbilityUsageType::SelectablesActionBar);

		if (Statics::IsValid(ContextActionTarget) && !Statics::HasZeroHealth(ContextActionTarget))
		{
			if (Unit->HasEnoughSelectableResources(AbilityInfo))
			{
				// This is ment to be a fog and stealth check
				if (AbilityInfo.PassesAnimNotifyTargetVisibilityCheck(ContextActionTarget))
				{
					// Will need to a bool to FContextButtonInfo saying whether to actually check anything
					ISelectable * TargetAsSelectable = CastChecked<ISelectable>(ContextActionTarget);
					if (AbilityInfo.PassesAnimNotifyRangeCheck<AInfantry, AInfantryController>(Unit, this, TargetAsSelectable))
					{
						AAbilityBase * AbilityEffect = AbilityInfo.GetEffectActor(GS);
						if (AbilityEffect->IsUsable_SelfChecks(Unit, CustomWarning))
						{
							if (AbilityEffect->IsUsable_TargetChecks(Unit, TargetAsSelectable, CustomWarning))
							{
								bCanDoAbility = true;
							}
						}
					}
					else
					{
						Warning = EGameWarning::AbilityOutOfRange;
					}
				}
				else
				{
					Warning = EGameWarning::TargetNotVisible;
				}
			}
			else
			{
				Warning = EGameWarning::NotEnoughSelectableResources;
			}
		}
		else
		{
			Warning = EGameWarning::TargetNoLongerAlive;
		}
	}

	//------------------------------------------------------------------
	//	Final stage - either do ability or dispatch warning
	//------------------------------------------------------------------

	if (bCanDoAbility)
	{
		DoSingleTargetContextAction();
	}
	else
	{
		if (Warning != EGameWarning::None)
		{
			PS->OnGameWarningHappened(Warning);
		}
		else // Assumed custom warning has something
		{
			PS->OnGameWarningHappened(CustomWarning);
		}

		SetPendingContextActionType(EContextButton::None);
		GoIdle();
	}
}

void AInfantryController::DoSingleTargetContextAction()
{
	assert(Statics::IsValid(ContextActionTarget));
	
	/* Spawn the ability */
	GS->Server_CreateAbilityEffect(PendingAbilityUsageCase, PendingAbilityAuxilleryData, 
		GI->GetContextInfo(GetPendingContextActionType()), Unit, Unit,
		Unit->GetTeam(), ContextActionTarget->GetActorLocation(), ContextActionTarget,
		CastChecked<ISelectable>(ContextActionTarget));
	
	SetPendingContextActionType(EContextButton::None);

	// AnimNotify_OnContextAnimationFinished() should be called some time after this
}

void AInfantryController::MoveTowardsAttackTarget()
{
	assert(AttackTarget != nullptr);

	if (Unit->IsAnimationAssigned(EAnimation::MovingWithResources)
		&& Unit->GetHeldResourceType() != EResourceType::None)
	{
		Unit->PlayAnimation(EAnimation::MovingWithResources, false);
	}
	else
	{
		Unit->PlayAnimation(EAnimation::Moving, false);
	}

	// Make unit face the direction they are moving with lag (by engine design)
	SetFacing_MovementDirection();

	MoveToActor(AttackTarget, GetMoveToAttackTargetAcceptanceRadius());
}

float AInfantryController::GetMoveToAttackTargetAcceptanceRadius() const
{
	// Use attack range plus user defined value
	return Range + AcceptanceRadiusModifier_MoveToActor;
}

float AInfantryController::GetNonHostileMoveToLocationAcceptanceRadius(ISelectable * SelectableToMoveTo) const
{
	return Unit->GetBoundsLength() + SelectableToMoveTo->GetBoundsLength();
}

void AInfantryController::MoveTowardsContextActionTarget(const FContextButtonInfo & AbilityInfo)
{
	assert(ContextActionTarget != nullptr);

	if (Unit->IsAnimationAssigned(EAnimation::MovingWithResources)
		&& Unit->GetHeldResourceType() != EResourceType::None)
	{
		Unit->PlayAnimation(EAnimation::MovingWithResources, false);
	}
	else
	{
		Unit->PlayAnimation(EAnimation::Moving, false);
	}

	SetFacing_MovementDirection();

	MoveToActor(ContextActionTarget, GetSelectableTargetingAbilityAcceptanceRadius(AbilityInfo));
}

void AInfantryController::MoveTowardsBuildingTargetingAbilityTarget(const FBuildingTargetingAbilityStaticInfo & AbilityInfo)
{
	assert(Statics::IsValid(ContextActionTarget));

	if (Unit->IsAnimationAssigned(EAnimation::MovingWithResources)
		&& Unit->GetHeldResourceType() != EResourceType::None)
	{
		Unit->PlayAnimation(EAnimation::MovingWithResources, false);
	}
	else
	{
		Unit->PlayAnimation(EAnimation::Moving, false);
	}

	SetFacing_MovementDirection();

	MoveToActor(ContextActionTarget, GetBuildingTargetingAbilityAcceptanceRadius(AbilityInfo));
}

void AInfantryController::MoveTowardsBuildingToEnterItsGarrison(const FBuildingGarrisonAttributes & BuildingsGarrisonAttributes)
{
	assert(Statics::IsValid(ContextActionTarget));

	if (Unit->IsAnimationAssigned(EAnimation::MovingWithResources)
		&& Unit->GetHeldResourceType() != EResourceType::None)
	{
		Unit->PlayAnimation(EAnimation::MovingWithResources, false);
	}
	else
	{
		Unit->PlayAnimation(EAnimation::Moving, false);
	}

	SetFacing_MovementDirection();

	MoveToActor(ContextActionTarget, GetBuildingEnterGarrisonAcceptanceRadius(BuildingsGarrisonAttributes));
}

void AInfantryController::OnMoveToBuildingToEnterItsGarrisonComplete()
{
	if (Statics::IsValid(ContextActionTarget) && !Statics::HasZeroHealth(ContextActionTarget))
	{
		ABuilding * Building = CastChecked<ABuilding>(ContextActionTarget);
		const FBuildingGarrisonAttributes & GarrisonAttributes = Building->GetBuildingAttributes()->GetGarrisonAttributes();

		/* Check if moved close enough */
		const bool bInRange = FMath::Square(GetDistanceFromBuildingForEnteringItsGarrison(Building)) <= FMath::Square(GarrisonAttributes.GetEnterRange());
		if (bInRange)
		{
			/* Check if garrison is empty enough */
			if (GarrisonAttributes.HasEnoughCapacityToAcceptUnit(Unit))
			{
				SetUnitState(EUnitState::InsideBuildingGarrison);
				Unit->ServerEnterBuildingGarrison(Building, const_cast<FBuildingGarrisonAttributes&>(GarrisonAttributes));
			}
			else /* Garrison too full */
			{
				GoIdle();
			}
		}
		else /* Did not move close enough */
		{
			UE_VLOG(this, RTSLOG, Warning, TEXT("Did not move close enough to building to enter "
				"it's garrison. Target building was: [%s]"), *Building->GetName());

			GoIdle();
		}
	}
	else /* Building no longer alive */
	{
		GoIdle();
	}
}

bool AInfantryController::ShouldChangeTargetOnDamage(const AActor * DamageCauser, float DamageAmount) const
{
	// If we are carrying out a right-click command on another selectable then we really don't 
	// have a choice to switch targets
	if (UnitState == EUnitState::RightClickOnEnemy)
	{
		return false;
	}

	// If already have a target and that target can attack then return false
	if (HasTarget() && Statics::HasAttack(AttackTarget))
	{
		return false;
	}

	if (!Statics::IsValid(DamageCauser))
	{
		return false;
	}

	// If damage causer is a type we aren't allowed to attack then return false
	if (!Statics::CanTypeBeTargeted(DamageCauser, Unit->AtkAttr.GetAcceptableTargetTypes()))
	{
		return false;
	}

	// If damage causer is an air unit and we can't attack air then return false
	if (!Unit->AtkAttr.CanAttackAir() && Statics::IsAirUnit(DamageCauser))
	{
		return false;
	}

	// If damage causer is on our team then return false
	if (Statics::IsFriendly(DamageCauser, PS->GetTeamTag()))
	{
		return false;
	}

	// If damage causer is inside fog of war or hidden by stealth then return false
	if (!Statics::IsSelectableVisible(DamageCauser, TeamVisInfo))
	{
		return false;
	}

	// If damage causer is no longer alive then return false
	if (Statics::HasZeroHealth(DamageCauser))
	{
		return false;
	}

	return true;
}

bool AInfantryController::CanTargetBeAquired(const AActor * PotentialNewTarget) const
{
	/* Capsule sweep should not be picking up friendlies in the first place */
	assert(Statics::IsHostile(PotentialNewTarget, PS->GetTeamTag()));

	return Statics::CanTypeBeTargeted(PotentialNewTarget, Unit->AtkAttr.GetAcceptableTargetTypes())
		&& (Unit->AtkAttr.CanAttackAir() || !Statics::IsAirUnit(PotentialNewTarget))
		&& Statics::IsSelectableVisible(PotentialNewTarget, TeamVisInfo) 
		&& !Statics::HasZeroHealth(PotentialNewTarget);
}

bool AInfantryController::HasTarget() const
{
	return Statics::IsValid(AttackTarget);
}

bool AInfantryController::IsTargetAttackable() const
{
	return IsSelectableAttackable(AttackTarget);
}

bool AInfantryController::IsSelectableAttackable(const AActor * Selectable) const
{
	/* Leaving IsValid out here. Assumed you checked that before calling */
	return Statics::IsSelectableVisible(Selectable, TeamVisInfo)
		&& !Statics::HasZeroHealth(Selectable)
		&& (Unit->AtkAttr.CanAttackAir() || !Statics::IsAirUnit(Selectable));
}

void AInfantryController::BeginAttackAnim()
{
	// All this should have been checked before calling this
	assert(CanFire() && Statics::IsValid(AttackTarget) && IsTargetInRange() &&
		CanTargetBeAquired(AttackTarget) && IsFacingAttackTarget());

	/* Will probably need to relax this assert in case the OnAttackAnimFinished anim notify fires
	after unit can attack again */
	//assert(AnimStateForBehavior != EUnitAnimState::DoingAttackAnim);

	SetAnimStateForBehavior(EUnitAnimState::DoingAttackAnim);
	Unit->PlayAnimation(EAnimation::Attack, true);
}

void AInfantryController::OnAttackMade()
{
	UE_VLOG(this, RTSLOG, Verbose, TEXT("Attack made"));

	// Spawn projectile
	Unit->Multicast_OnAttackMade(AttackTarget);

	if (!Unit->bAllowRotationStraightAfterAttack)
	{
		/* Stop looking at target temporarily */
		bIgnoreLookingAtFocus = true;
	}

	// Put attack on cooldown
	TimeTillAttackResets = Unit->GetAttackAttributes()->GetAttackRate();

	TargetOfLastAttack = AttackTarget;
}

void AInfantryController::OnResetFire()
{
}

AActor * AInfantryController::FindClosestValidEnemyInRange(float Radius, bool bForceOnTargetChangeCall)
{
	TArray<FHitResult> NearbyEnemies;

	/* Setup channels to query */
	FCollisionObjectQueryParams ObjectQueryParams;
	for (const uint8 & EnemyChannel : GS->GetEnemyChannels(PS->GetTeam()))
	{
		ObjectQueryParams.AddObjectTypesToQuery(static_cast<ECollisionChannel>(EnemyChannel));
	}

	/* This can be changed to a single sweep if two conditions are met:
	1. Radius <= Range is always true
	2. We don't prioritize attack capable selectables over non-attackers when choosing target */
	Statics::CapsuleSweep(GetWorld(), NearbyEnemies, Unit->GetActorLocation(), ObjectQueryParams,
		Radius);

	/* Closest enemy that isn't capable of attacking. Return this if all nearby enemies cannot
	attack */
	AActor * ClosestEnemyWithoutAttack = nullptr;

	/* Now return closest enemy. Prefer units/buildings that can attack over those that can't.
	Assumes array already ordered from closest to futherest away */
	for (int32 i = 0; i < NearbyEnemies.Num(); ++i)
	{
		AActor * Enemy = NearbyEnemies[i].GetActor();

		/* Can these sweeps pickup nulls or non-valids? If so then this may need to be changed to
		an if(UStatics::IsValid) instead */
		assert(Enemy != nullptr);
		assert(Enemy != Unit);

		/* Check if outside fog, type etc */
		if (CanTargetBeAquired(Enemy))
		{
			/* Check if enemy can attack because prefer attackers over non-attackers */
			if (Statics::HasAttack(Enemy))
			{
				if (Enemy != AttackTarget || bForceOnTargetChangeCall)
				{
					OnTargetChange(Enemy);
				}

				return Enemy;
			}
			else
			{
				/* Enemy cannot attack. Cache them but have to keep checking if any attack capable
				enemies are within range */
				if (ClosestEnemyWithoutAttack == nullptr)
				{
					ClosestEnemyWithoutAttack = Enemy;
				}
			}
		}
	}

	return ClosestEnemyWithoutAttack;
}

bool AInfantryController::IsMoving() const
{
	return GetMoveStatus() != EPathFollowingStatus::Idle;
}

bool AInfantryController::CanFire() const
{
	return TimeTillAttackResets <= 0;
}

bool AInfantryController::IsTargetInRange()
{
	// TODO maybe log the acceptance radius used for these moves to attack target and how 
	// close unit ends up because they seem to be working. Could use them as example

	const float Distance = Statics::GetDistance2D(Unit->GetActorLocation(), AttackTarget->GetActorLocation())
		- AttackTargetBoundsLength - Unit->GetBoundsLength();

	const bool bUseLenience = (AttackTarget == TargetOfLastAttack && bIsTargetStillInRange);

	if (bUseLenience)
	{
		if (Distance <= LenienceRange)
		{
			return true;
		}
		else
		{
			bIsTargetStillInRange = false;
			return false;
		}
	}
	else
	{
		if (Distance <= Range)
		{
			bIsTargetStillInRange = true;
			return true;
		}
		else
		{
			bIsTargetStillInRange = false;
			return false;
		}
	}
}

bool AInfantryController::IsFacingAttackTarget() const
{
	// Less than 0 is code for 'facing don't matter'
	if (AttackFacingRotationRequirement < 0.f)
	{
		return true;
	}

	/* Values LookAtRot and ControlRotation can take are (-180, 180) or [-180, 180), but
	doesn't really matter which one here */

	const FRotator LookAtRot = UKismetMathLibrary::FindLookAtRotation(Unit->GetActorLocation(),
		AttackTarget->GetActorLocation());

	const float Tolerance = AttackFacingRotationRequirement;
	const float LookAtYaw = LookAtRot.Yaw + 180.f;
	const float PawnYaw = Unit->GetActorRotation().Yaw + 180.f;

	return (PawnYaw < LookAtYaw + Tolerance) && (PawnYaw > LookAtYaw - Tolerance);
}

void AInfantryController::OnTargetChange(AActor * NewTarget)
{
	assert(NewTarget != Unit); // Cannot aquire self
	assert(CanTargetBeAquired(NewTarget) && Statics::IsValid(NewTarget));

	UE_VLOG(this, RTSLOG, Verbose, TEXT("AttackTarget changed: %s"), *NewTarget->GetName());

	ISelectable * AsSelectable = CastChecked<ISelectable>(NewTarget);

	/* Cache bounds length to avoid cast and virtual function call each
	time when checking range to target */
	AttackTargetBoundsLength = AsSelectable->GetBoundsLength();

	SetFacing_Focus();
	SetFocus(NewTarget, EAIFocusPriority::Gameplay);
}

void AInfantryController::OnContextActionTargetChange(AActor * NewContextActionTarget, ISelectable * TargetAsSelectable)
{
	assert(NewContextActionTarget != nullptr);

	ContextActionTargetBoundsLength = TargetAsSelectable->GetBoundsLength();

	SetFacing_Focus();
	SetFocus(NewContextActionTarget, EAIFocusPriority::Gameplay);
}

bool AInfantryController::WantsToAttack() const
{
	return UnitState == EUnitState::Idle_WithTarget
		|| UnitState == EUnitState::AttackMoveCommand_WithTargetAquired
		|| UnitState == EUnitState::RightClickOnEnemy
		|| UnitState == EUnitState::HoldingPositionWithTarget;
}

void AInfantryController::SetFacing_MovementDirection()
{
	CharacterMovement->bUseControllerDesiredRotation = false;
	CharacterMovement->bOrientRotationToMovement = true;

	// Set this in case it was set true after doing attack
	bIgnoreLookingAtFocus = false;
}

void AInfantryController::SetFacing_Focus()
{
	CharacterMovement->bOrientRotationToMovement = false;
	CharacterMovement->bUseControllerDesiredRotation = true;
}

void AInfantryController::StopBehaviour()
{
	SetUnitState(EUnitState::PossessedUnitDestroyed);

	// Do not let unit collect resources if currently doing so
	GetWorldTimerManager().ClearTimer(TimerHandle_ResourceGathering);
}

EPathFollowingRequestResult::Type AInfantryController::Move(const FVector & Location, float AcceptanceRadius)
{
	/* Play a 'moving while holding resources' anim if unit is holding resources and anim was
	set in editor */
	if (Unit->IsAnimationAssigned(EAnimation::MovingWithResources)
		&& Unit->GetHeldResourceType() != EResourceType::None)
	{
		Unit->PlayAnimation(EAnimation::MovingWithResources, false);
	}
	else
	{
		Unit->PlayAnimation(EAnimation::Moving, false);
	}

	SetAnimStateForBehavior(EUnitAnimState::NotPlayingImportantAnim);

	SetFacing_MovementDirection();

	return MoveToLocation(Location, AcceptanceRadius);
}

void AInfantryController::Delay(FTimerHandle & TimerHandle, void(AInfantryController::* Function)(), float Delay)
{
	assert(Delay > 0.f);
	GetWorldTimerManager().SetTimer(TimerHandle, this, Function, Delay, false);
}

void AInfantryController::DelayZeroTimeOk(FTimerHandle & TimerHandle, void(AInfantryController::* Function)(), float Delay)
{
	GetWorldTimerManager().SetTimer(TimerHandle, this, Function, Delay, false);
}

void AInfantryController::SetupReferences(APawn * InPawn)
{
	Unit = CastChecked<AInfantry>(InPawn);
	CharacterMovement = Unit->GetCharacterMovement();
}

TMap<EUnitState, AInfantryController::FunctionPtrType> AInfantryController::SetupTickFunctions() const
{
	TMap <EUnitState, AInfantryController::FunctionPtrType> Functions;

	const int32 NumContainerEntries = static_cast<uint8>(EUnitState::z_ALWAYS_LAST_IN_ENUM) - 2;
	Functions.Reserve(NumContainerEntries);

	AInfantryController::FunctionPtrType Ptr;

	Ptr = &AInfantryController::TickAI_AttackMoveCommandWithNoTargetAquired;
	Functions.Emplace(EUnitState::AttackMoveCommand_WithNoTargetAquired, Ptr);

	Ptr = &AInfantryController::TickAI_AttackMoveCommandWithTargetAquired;
	Functions.Emplace(EUnitState::AttackMoveCommand_WithTargetAquired, Ptr);

	Ptr = &AInfantryController::TickAI_AttackMoveCommand_ReturningToLeashLocation;
	Functions.Emplace(EUnitState::AttackMoveCommand_ReturningToLeashLocation, Ptr);

	Ptr = &AInfantryController::TickAI_Idle_ReturningToLeashLocation;
	Functions.Emplace(EUnitState::Idle_ReturningToLeashLocation, Ptr);

	Ptr = &AInfantryController::TickAI_ChasingTargetToDoContextCommand;
	Functions.Emplace(EUnitState::ChasingTargetToDoContextCommand, Ptr);

	Ptr = &AInfantryController::TickAI_HeadingToBuildingToDoBuildingTargetingAbility;
	Functions.Emplace(EUnitState::HeadingToBuildingToDoBuildingTargetingAbility, Ptr);

	Ptr = &AInfantryController::TickAI_HeadingToBuildingToEnterItsGarrison;
	Functions.Emplace(EUnitState::HeadingToBuildingToEnterItsGarrison, Ptr);

	Ptr = &AInfantryController::TickAI_InsideBuildingGarrison;
	Functions.Emplace(EUnitState::InsideBuildingGarrison, Ptr);

	Ptr = &AInfantryController::TickAI_HeadingToConstructionSite;
	Functions.Emplace(EUnitState::HeadingToConstructionSite, Ptr);

	Ptr = &AInfantryController::TickAI_HeadingToContextCommandWorldLocation;
	Functions.Emplace(EUnitState::HeadingToContextCommandWorldLocation, Ptr);

	Ptr = &AInfantryController::TickAI_HeadingToPotentialConstructionSite;
	Functions.Emplace(EUnitState::HeadingToPotentialConstructionSite, Ptr);

	Ptr = &AInfantryController::TickAI_HeadingToResourceSpot;
	Functions.Emplace(EUnitState::HeadingToResourceSpot, Ptr);

	Ptr = &AInfantryController::TickAI_HoldingPositionWithoutTarget;
	Functions.Emplace(EUnitState::HoldingPositionWithoutTarget, Ptr);

	Ptr = &AInfantryController::TickAI_HoldingPositionWithTarget;
	Functions.Emplace(EUnitState::HoldingPositionWithTarget, Ptr);

	Ptr = &AInfantryController::TickAI_IdleWithoutTarget;
	Functions.Emplace(EUnitState::Idle_WithoutTarget, Ptr);

	Ptr = &AInfantryController::TickAI_IdleWithTarget;
	Functions.Emplace(EUnitState::Idle_WithTarget, Ptr);

	Ptr = &AInfantryController::TickAI_MoveCommandToFriendlyMobileSelectable;
	Functions.Emplace(EUnitState::MoveCommandToFriendlyMobileSelectable, Ptr);

	Ptr = &AInfantryController::TickAI_MovingToRightClickLocation;
	Functions.Emplace(EUnitState::MovingToRightClickLocation, Ptr);

	Ptr = &AInfantryController::TickAI_ReturningToResourceDepot;
	Functions.Emplace(EUnitState::ReturningToResourceDepot, Ptr);

	Ptr = &AInfantryController::TickAI_RightClickOnEnemy;
	Functions.Emplace(EUnitState::RightClickOnEnemy, Ptr);

	Ptr = &AInfantryController::TickAI_WaitingForFoundationsToBePlaced;
	Functions.Emplace(EUnitState::WaitingForFoundationsToBePlaced, Ptr);

	Ptr = &AInfantryController::TickAI_WaitingToGatherResources;
	Functions.Emplace(EUnitState::WaitingToGatherResources, Ptr);

	Ptr = &AInfantryController::TickAI_GatheringResources;
	Functions.Emplace(EUnitState::GatheringResources, Ptr);

	Ptr = &AInfantryController::TickAI_DoingContextActionAnim;
	Functions.Emplace(EUnitState::DoingContextActionAnim, Ptr);

	Ptr = &AInfantryController::TickAI_DoingSpecialBuildingTargetingAbility;
	Functions.Emplace(EUnitState::DoingSpecialBuildingTargetingAbility, Ptr);

	Ptr = &AInfantryController::TickAI_ConstructingBuilding;
	Functions.Emplace(EUnitState::ConstructingBuilding, Ptr);

	Ptr = &AInfantryController::TickAI_DroppingOffResources;
	Functions.Emplace(EUnitState::DroppingOfResources, Ptr);

	Ptr = &AInfantryController::TickAI_MovingToPointNearStaticSelectable;
	Functions.Emplace(EUnitState::MovingToPointNearStaticSelectable, Ptr);

	Ptr = &AInfantryController::TickAI_MovingToBarracksInitialPoint;
	Functions.Emplace(EUnitState::MovingToBarracksInitialPoint, Ptr);

	Ptr = &AInfantryController::TickAI_MovingToBarracksRallyPoint;
	Functions.Emplace(EUnitState::MovingToBarracksRallyPoint, Ptr);

	Ptr = &AInfantryController::TickAI_GoingToPickUpInventoryItem;
	Functions.Emplace(EUnitState::GoingToPickUpInventoryItem, Ptr);

	Ptr = &AInfantryController::TickAI_PickingUpInventoryItem;
	Functions.Emplace(EUnitState::PickingUpInventoryItem, Ptr);

	assert(Functions.Num() == NumContainerEntries);

	return Functions;
}

void AInfantryController::DoNothing()
{
}

void AInfantryController::OnUnitHasAttackChanged(bool bNewHasAttack)
{
	bUnitHasAttack = bNewHasAttack;
}

void AInfantryController::OnUnitRangeChanged(float NewRange)
{
	Range = NewRange;
	LenienceRange = Range * RangeLenienceFactor;
}

void AInfantryController::OnUnitAttackFacingRotationRequiredChanged(float NewAngleRequired)
{
	AttackFacingRotationRequirement = NewAngleRequired;
}

#if WITH_EDITOR
void AInfantryController::DisplayOnScreenDebugInfo()
{
	MSG("Unit state", ENUM_TO_STRING(EUnitState, UnitState));
	MSG("Anim state", ENUM_TO_STRING(EUnitAnimState, AnimStateForBehavior));
}
#endif
