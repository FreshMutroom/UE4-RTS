// Fill out your copyright notice in the Description page of Project Settings.

#include "Statics.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Particles/ParticleSystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "Public/AudioDevice.h"
#include "GameFramework/WorldSettings.h"
#include "Public/ContentStreaming.h"

#include "GameFramework/RTSPlayerState.h"
#include "GameFramework/RTSGameState.h"
#include "MapElements/Building.h"
#include "Managers/FogOfWarManager.h"
#include "Statics/DamageTypes.h"
#include "GameFramework/RTSGameInstance.h"
#include "GameFramework/FactionInfo.h"
#include "Miscellaneous/StartingGrid/StartingGrid.h"
#include "Statics/DevelopmentStatics.h"
#include "Settings/ProjectSettings.h"
#include "Audio/FogObeyingAudioComponent.h"


const FName Statics::HARDWARE_CURSOR_PATH = FName("Slate/HardwareCursors/");
const FVector Statics::POOLED_ACTOR_SPAWN_LOCATION = FVector(0.f);
const FVector Statics::INFO_ACTOR_SPAWN_LOCATION = FVector(0.f);
const FName Statics::MeshFloorSocket = FName("RTSSocket_Floor");
const FName Statics::MeshMiddleSocket = FName("RTSSocket_Body");
const FName Statics::MeshHeadSocket = FName("RTSSocket_Head");
const FString Statics::LOBBY_OBSERVER_COMBO_BOX_OPTION = FString("Observer");
TArray <TSubclassOf <UDamageType>> Statics::DamageTypes = TArray<TSubclassOf <UDamageType>>();
const TArray < FName, TFixedAllocator < Statics::NUM_CUSTOM_MESH_SOCKETS > > Statics::SocketNames = InitSocketNames();
const TArray < FName, TFixedAllocator < Statics::NUM_TARGETING_TYPES > > Statics::TargetTypeFNames = InitTargetTypeFNames();
#if !UE_BUILD_SHIPPING
const TSet < EAnimation > Statics::ImportantAnims = Statics::InitImportantAnims();
#endif


bool Statics::IsValid(const AActor * Actor)
{
	return Actor != nullptr && !Actor->IsPendingKill();
}

bool Statics::IsValid(AActor * Actor)
{
	return Actor != nullptr && !Actor->IsPendingKill();
}

bool Statics::IsValid(UObject * Object)
{
	return Object != nullptr && !Object->IsPendingKill();
}

bool Statics::IsValid(const TScriptInterface<ISelectable> & Selectable)
{
	return Selectable != nullptr && Selectable.GetObject() && !Selectable.GetObject()->IsPendingKill();
}

bool Statics::IsValid(TScriptInterface<ISelectable> & Selectable)
{
	return Selectable != nullptr && Selectable.GetObject() && !Selectable.GetObject()->IsPendingKill();
}

ISelectable * Statics::ToSelectablePtr(const TScriptInterface<ISelectable>& Obj)
{
	// Hoping this works
	return static_cast<ISelectable*>(Obj.GetInterface());
}

ABuilding * Statics::SpawnBuilding(const FBuildingInfo * BuildingInfo, const FVector & Loc, 
	const FRotator & Rot, ARTSPlayerState * Owner, ARTSGameState * GameState, UWorld * World, 
	ESelectableCreationMethod CreationMethod)
{
	/* This function will take into account whether the building needs to rise up from the 
	ground or not */
	
	const TSubclassOf <AActor> Building_BP = BuildingInfo->GetSelectableBP();

	assert(World->IsServer());
	assert(Building_BP != nullptr);
	assert(Building_BP->IsChildOf(ABuilding::StaticClass()));
	assert(Owner != nullptr);

	FVector SpawnLoc = Loc;
	// (CreationMethod != ESelectableCreationMethod::StartingSelectable) = quick fix for starting buildings being spawned lower than desired
	if (CreationMethod != ESelectableCreationMethod::StartingSelectable 
		&& BuildingInfo->ShouldBuildingRiseFromGround())
	{
		/* Spawn below ground so it can rise up as being constructed */
		SpawnLoc.Z -= BuildingInfo->GetBoundsHeight();
	}

	// TODO: this doesn't need to be spawn deferred I think (At least if bIsStartingBuilding is false). Use FActorSpawnParameters and 
	// set owner through that. Also spawning as ISelectable? Can use AActor perfectly fine I think

	const FTransform SpawnTransform = FTransform(Rot, SpawnLoc, FVector::OneVector);
	ABuilding * const Building = World->SpawnActorDeferred<ABuilding>(Building_BP, SpawnTransform, Owner);

	// 2nd param not correct but not anticipated to be used
	Building->SetOnSpawnValues(CreationMethod, nullptr);

	// Also set ID because possibly not repping on initial
	Building->SetSelectableIDAndGameTickCount(Owner->GenerateSelectableID(Building), GameState->GetGameTickCounter());

	AActor * AsActor = UGameplayStatics::FinishSpawningActor(Building, SpawnTransform);
	assert(AsActor != nullptr);
	assert(Building != nullptr);

	return Building;
}

ABuilding * Statics::SpawnBuildingForFactionInfo(TSubclassOf<ABuilding> Building_BP, const FVector & Loc, const FRotator & Rot, UWorld * World)
{
	/* If thrown check faction info buildings array does not have a 'none' in it in editor */
	assert(Building_BP != nullptr);
	assert(World != nullptr);

	ABuilding * Building = World->SpawnActorDeferred<ABuilding>(Building_BP, FTransform(Rot, Loc));
	
	/* I hope it's not too late doing this now. Also I'm not sure it does anything, but it makes 
	sense since we're only spawning it to extract info and it gets destroyed straight away. 
	I have commented it since I don't see any difference with it */
	//Building->SetReplicates(false);
	
	// Who knows, might not even need this
	UGameplayStatics::FinishSpawningActor(Building, FTransform::Identity);

	return Building;
}

ISelectable * Statics::SpawnUnitAsSelectable(TSubclassOf <AActor> Unit_BP, const FVector & Loc,
	const FRotator & Rot, ARTSPlayerState * Owner, ARTSGameState * GameState, UWorld * World, 
	ESelectableCreationMethod CreationMethod, ABuilding * Builder)
{
	/* If thrown check your faction info BPs for 'none' entries for units */
	assert(Unit_BP != nullptr);
	assert(World != nullptr);
	assert(Owner != nullptr);

	const FTransform SpawnTransform = FTransform(Rot, Loc, FVector::OneVector);
	ISelectable * const Unit = World->SpawnActorDeferred<ISelectable>(Unit_BP, SpawnTransform, Owner);
	assert(Unit != nullptr);

	// TODO this can be done at same time as SetSelectableID below. Do same for similar spawn funcs
	Unit->SetOnSpawnValues(CreationMethod, Builder);

	AActor * AsActor = CastChecked<AActor>(Unit);

	// Set ID and tick counter now because possibly not repping on initial
	Unit->SetSelectableIDAndGameTickCount(Owner->GenerateSelectableID(AsActor), GameState->GetGameTickCounter());

	AsActor = UGameplayStatics::FinishSpawningActor(AsActor, SpawnTransform);
	assert(AsActor != nullptr);

	return Unit;
}

AActor * Statics::SpawnUnit(TSubclassOf <AActor> Unit_BP, const FVector & Loc, const FRotator & Rot,
	ARTSPlayerState * Owner, ARTSGameState * GameState, UWorld * World, 
	ESelectableCreationMethod CreationMethod, ABuilding * Builder)
{
	assert(World->IsServer());
	assert(Unit_BP != nullptr);
	assert(Owner != nullptr);

	const FTransform SpawnTransform = FTransform(Rot, Loc, FVector::OneVector);
	ISelectable * const Unit = World->SpawnActorDeferred<ISelectable>(Unit_BP, SpawnTransform, Owner);
	assert(Unit != nullptr);

	Unit->SetOnSpawnValues(CreationMethod, Builder);

	AActor * AsActor = CastChecked<AActor>(Unit);

	// Set ID and tick counter now because possibly not repping on initial
	Unit->SetSelectableIDAndGameTickCount(Owner->GenerateSelectableID(AsActor), GameState->GetGameTickCounter());

	AsActor = UGameplayStatics::FinishSpawningActor(AsActor, SpawnTransform);
	assert(AsActor != nullptr);

	return AsActor;
}

AInfantry * Statics::SpawnUnitForFactionInfo(TSubclassOf<AActor> Unit_BP, const FVector & Loc, const FRotator & Rot, UWorld * World)
{
	assert(World != nullptr);

	AInfantry * const Unit = World->SpawnActor<AInfantry>(Unit_BP, Loc, Rot);
	assert(Unit != nullptr);

	return Unit;
}

void Statics::SpawnStartingSelectables(ARTSPlayerState * Owner, const FTransform & GridTransform, 
	URTSGameInstance * GameInstance, TArray < EBuildingType > & OutSpawnedBuildingTypes, 
	TArray < EUnitType > & OutSpawnedUnitTypes)
{
	assert(Owner != nullptr);

	const AFactionInfo * const FactionInfo = GameInstance->GetFactionInfo(Owner->GetFaction());
	TSubclassOf <AStartingGrid> GridBP = FactionInfo->GetStartingGrid();

	UE_CLOG(GridBP == nullptr, RTSLOG, Warning, TEXT("Faction [%s] does not have a starting grid set in its faction "
		"info, therefore no starting selectables will be spawned for this faction."),
		TO_STRING(EFaction, Owner->GetFaction()));

	/* Will check for null. We'll allow null - means you start the match with nothing */
	if (GridBP != nullptr)
	{
		UWorld * World = Owner->GetWorld();
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Owner;
		AStartingGrid * Grid = World->SpawnActor<AStartingGrid>(GridBP, GridTransform, SpawnParams);

		OutSpawnedBuildingTypes = Grid->GetSpawnedBuildingTypes();
		OutSpawnedUnitTypes = Grid->GetSpawnedUnitTypes();

		Grid->Destroy();
	}
}

UParticleSystemComponent * Statics::SpawnFogParticles(UParticleSystem * Template, ARTSGameState * GameState, 
	const FVector & Location, const FRotator & Rotation, const FVector & Scale3D)
{
	assert(Template != nullptr);

	/* I will probably want to add a warmup time param to this func like I Have with 
	:SpawnFogParticlesAttached. */

	UParticleSystemComponent * Particles = UGameplayStatics::SpawnEmitterAtLocation(GameState,
		Template, Location, Rotation, Scale3D, true);
	assert(Particles != nullptr);

	/* Set initial visibility. Alternatively if this doesn't work then may have to start hidden and
	let fog manager reveal it later, possibly one frame behind */
#if GAME_THREAD_FOG_OF_WAR
	const bool bVisible = GameState->GetFogManager()->IsLocationLocallyVisibleNotChecked(Location);
#elif MULTITHREADED_FOG_OF_WAR
	const bool bVisible = MultithreadedFogOfWarManager::Get().IsLocationLocallyVisible(Location);
#else // No fog at all
	const bool bVisible = true;
#endif
	Particles->SetHiddenInGame(!bVisible);

	GameState->RegisterFogParticles(Particles);

	return Particles;
}

UParticleSystemComponent * Statics::SpawnFogParticlesAttached(ARTSGameState * GameState,
	UParticleSystem * EmitterTemplate, USceneComponent * AttachToComponent, FName AttachPointName,
	float WarmUpTime, FVector Location, FRotator Rotation, FVector Scale, EAttachLocation::Type LocationType,
	bool bAutoDestroy, EPSCPoolMethod PoolingMethod)
{
	/* If thrown then ISelectable::GetAttachComponent will need to be overridden and return a
	component and optional socket name */
	assert(AttachToComponent != nullptr);
	assert(EmitterTemplate != nullptr);
	
	//-------------------------------------------------------------------------------------------
	//	Begin basically a copy of UGameplayStatics::SpawnEmitterAttached except I set warmup 
	//	time at some point
	//-------------------------------------------------------------------------------------------

	UParticleSystemComponent* PSC = nullptr;
	if (EmitterTemplate)
	{
		if (AttachToComponent == nullptr)
		{
			UE_LOG(LogScript, Warning, TEXT("UGameplayStatics::SpawnEmitterAttached: NULL AttachComponent specified!"));
		}
		else
		{
			UWorld* const World = AttachToComponent->GetWorld();
			if (World && !World->IsNetMode(NM_DedicatedServer))
			{
				if (PoolingMethod != EPSCPoolMethod::None)
				{
					//If system is set to auto destroy the we should be safe to automatically allocate from a the world pool.
					PSC = World->GetPSCPool().CreateWorldParticleSystem(EmitterTemplate, World, PoolingMethod);
				}
				else
				{
					AActor * Actor = AttachToComponent->GetOwner();
					PSC = NewObject<UParticleSystemComponent>((Actor ? Actor : (UObject*)World));
					PSC->bAutoDestroy = bAutoDestroy;
					PSC->bAllowAnyoneToDestroyMe = true;
					PSC->SecondsBeforeInactive = 0.0f;
					PSC->bAutoActivate = false;
					PSC->SetTemplate(EmitterTemplate);
					PSC->bOverrideLODMethod = false;
					PSC->WarmupTime = WarmUpTime; // Basically the only line I added
				}

				if (PSC != nullptr)
				{
					PSC->SetupAttachment(AttachToComponent, AttachPointName);

					if (LocationType == EAttachLocation::KeepWorldPosition)
					{
						const FTransform ParentToWorld = AttachToComponent->GetSocketTransform(AttachPointName);
						const FTransform ComponentToWorld(Rotation, Location, Scale);
						const FTransform RelativeTM = ComponentToWorld.GetRelativeTransform(ParentToWorld);
						PSC->RelativeLocation = RelativeTM.GetLocation();
						PSC->RelativeRotation = RelativeTM.GetRotation().Rotator();
						PSC->RelativeScale3D = RelativeTM.GetScale3D();
					}
					else
					{
						PSC->RelativeLocation = Location;
						PSC->RelativeRotation = Rotation;

						if (LocationType == EAttachLocation::SnapToTarget)
						{
							// SnapToTarget indicates we "keep world scale", this indicates we we want the inverse of the parent-to-world scale 
							// to calculate world scale at Scale 1, and then apply the passed in Scale
							const FTransform ParentToWorld = AttachToComponent->GetSocketTransform(AttachPointName);
							PSC->RelativeScale3D = Scale * ParentToWorld.GetSafeScaleReciprocal(ParentToWorld.GetScale3D());
						}
						else
						{
							PSC->RelativeScale3D = Scale;
						}
					}

					PSC->RegisterComponentWithWorld(World);
					PSC->ActivateSystem(true);

					// Notify the texture streamer so that PSC gets managed as a dynamic component.
					IStreamingManager::Get().NotifyPrimitiveUpdated(PSC);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
					if (PSC->Template && PSC->Template->IsImmortal())
					{
						const FString OnScreenMessage = FString::Printf(TEXT("SpawnEmitterAttached spawned potentially immortal particle system! %s (%s) may stay in world despite never spawning particles after burst spawning is over."), *(PSC->GetPathName()), *(PSC->Template->GetName()));
						GEngine->AddOnScreenDebugMessage((uint64)((PTRINT)AttachToComponent), 3.f, FColor::Red, OnScreenMessage);
						UE_LOG(RTSLOG, Log, TEXT("SpawnEmitterAttached spawned potentially immortal particle system! %s (%s) may stay in world despite never spawning particles after burst spawning is over."),
							*(PSC->GetPathName()), *(PSC->Template->GetPathName())
						);
					}
#endif
				}
			}
		}
	}

	//-------------------------------------------------------------------------------------------
	//	End basically a copy of UGameplayStatics::SpawnEmitterAttached
	//-------------------------------------------------------------------------------------------

	/* Set inital visibility */
#if GAME_THREAD_FOG_OF_WAR
	const bool bVisible = GameState->GetFogManager()->IsLocationLocallyVisible(Location);
#elif MULTITHREADED_FOG_OF_WAR
	const bool bVisible = MultithreadedFogOfWarManager::Get().IsLocationLocallyVisible(Location);
#else
	const bool bVisible = true;
#endif
	PSC->SetHiddenInGame(!bVisible);

	GameState->RegisterFogParticles(PSC);

	return PSC;
}

UParticleSystemComponent * Statics::SpawnParticles(UParticleSystem * Template, ARTSGameState * GameState, 
	const FVector & Location, const FRotator & Rotation, const FVector & Scale3D)
{
	return UGameplayStatics::SpawnEmitterAtLocation(GameState, Template, Location, Rotation, Scale3D, true);
}

FString Statics::GetMapName(const UWorld * World)
{
	if (World->WorldType == EWorldType::PIE)
	{
		/* PIE will add "UEDPIE_0_" onto the front of the map name, so need to account 
		for this. Unsure if the number keeps getting bigger on successive PIE sessions - worth 
		checking this */
		return World->GetMapName().RightChop(9);
	}
	else
	{
		return World->GetMapName();
	}
}

bool Statics::IsBuildableLocation(const UWorld * World, ARTSGameState * GameState, 
	ARTSPlayerState * PlayerState, AFactionInfo * FactionInfo, EBuildingType BuildingType, 
	const FVector & Location, const FRotator & Rotation, bool bShowHUDMessage)
{
	/* TODO: corners might not be enough. maybe do 4 more locations like the middle of the sides,
	too */

	/* TODO: will need to not allow being built if colliding with the map boundries */

	/* Going to do a lot in this function. Potentially a box sweep, capsule sweep and 5 (short)
	line traces.

	The following are done in order:
	- Check ghost is outside fog of war
	- Check ghost is not colliding with any selectables
	- If applicable check ghost is within distance of another owned/allied building
	- Check ground below ghost is considered flat enough

	If all these pass then location is buildable */

	// TODO work on setting up fog points on buildings placed. Figure out if ghost is outside 
	// fog and if so then just transfer the pointers over to building, or use the 5 corner 
	// method for ghost then create more rigurous method for actual building


	/* Get build info which has stored in it the bounds extent which we will use for checking
	for any overlaps and doing traces from later */
	const FBuildingInfo * BuildingInfo = FactionInfo->GetBuildingInfo(BuildingType);

	/* First check if ghost is outside fog of war because we do not allow placing building inside
	fog. To do this check if center plus the 4 corners of the box component root are outside fog */

	const FVector Extent = BuildingInfo->GetScaledBoundsExtent();

	/* Center is the center of the bounds root component of building. I'm just assuming
	that the param Location is that location */
	const FVector Center = Location;

	const ETeam Team = PlayerState->GetTeam();

	FVector BoxCompCorners[4];

	/* Get the 4 bottom corners of box extent without any rotation considered */
	BoxCompCorners[0] = FVector(Extent.X, Extent.Y, -Extent.Z);
	BoxCompCorners[1] = FVector(Extent.X, -Extent.Y, -Extent.Z);
	BoxCompCorners[2] = FVector(-Extent.X, Extent.Y, -Extent.Z);
	BoxCompCorners[3] = FVector(-Extent.X, -Extent.Y, -Extent.Z);

	/* Note to self: Above vectors do not add Center to each because for loop below will just minus
	it straight away to rotate around Center */

	/* TODO: could be nice for user to specify a default rotation that the box comp
	should be, because box comp is root it cannot be rotated. So in editor user makes
	sure their building fits as best as it can inside box comp then when ghost is
	spawned the user defined rotation is applied, so actually ghost spawning is really
	where that should be applied - the param of this func should be correct if rot
	is applied there */
	/* Now rotate each corner around Center by Rotation param */
	for (int32 i = 0; i < 4; ++i)
	{
		BoxCompCorners[i] = Rotation.RotateVector(BoxCompCorners[i]);
		BoxCompCorners[i] += Center;
	}

	/* Let CPU players ignore fog for no reason whatsoever */
	if (PlayerState->bIsABot == false)
	{
#if GAME_THREAD_FOG_OF_WAR
		/* PC::FogOfWarManager can be null if a client calls this during server RPC. Really
		this method belongs outside PC */
		AFogOfWarManager * LocalFogManager = GameState->GetFogManager();

		/* Check center point plus each of the 4 corners are outside fog */
		if (LocalFogManager->IsLocationVisibleNotChecked(Center, Team))
		{
			for (int32 i = 0; i < 4; ++i)
			{
				if (!LocalFogManager->IsLocationVisibleNotChecked(BoxCompCorners[i], Team))
				{
					return IsBuildableLocationReturn(PlayerState, EGameWarning::Building_InsideFog, bShowHUDMessage);
				}
			}
		}
		else
		{
			return IsBuildableLocationReturn(PlayerState, EGameWarning::Building_InsideFog, bShowHUDMessage);
		}
#elif MULTITHREADED_FOG_OF_WAR
		MultithreadedFogOfWarManager & FogManager = MultithreadedFogOfWarManager::Get();
		if (FogManager.IsLocationVisibleNotChecked(Center, Team))
		{
			for (int32 i = 0; i < 4; ++i)
			{
				if (!FogManager.IsLocationVisibleNotChecked(BoxCompCorners[i], Team))
				{
					return IsBuildableLocationReturn(PlayerState, EGameWarning::Building_InsideFog, bShowHUDMessage);
				}
			}
		}
		else
		{
			return IsBuildableLocationReturn(PlayerState, EGameWarning::Building_InsideFog, bShowHUDMessage);
		}
#endif
	}

	/* Fog check passed. Now check for overlaps with other buildings and units */
	const FCollisionObjectQueryParams & CollisionQueryParams = GameState->GetAllTeamsQueryParams();

	/* Check if any selectable collides with the ghosts location */
	const bool bFoundHit = World->OverlapAnyTestByObjectType(Location, FQuat(Rotation),
		CollisionQueryParams, FCollisionShape::MakeBox(Extent));

	if (bFoundHit)
	{
		return IsBuildableLocationReturn(PlayerState, EGameWarning::Building_SelectableInTheWay, bShowHUDMessage);
	}

	/* If faction/building has a rule that it must build buildings within a certain range of other
	buildings then check that now */
	if (BuildingInfo->GetBuildProximityRange() > 0.f)
	{
		/* Trace for all friendlies. Kind of not optimal - may want to do just our own selectables
		if faction does not allow building off allied buildings, but didn't implement collision
		like that so don't have a choice */
		FCollisionObjectQueryParams ProximityParams = FCollisionObjectQueryParams(GameState->GetTeamCollisionChannel(Team));

		TArray <FHitResult> HitFriendlies;
		Statics::CapsuleSweep(World, HitFriendlies, Location, ProximityParams,
			BuildingInfo->GetBuildProximityRange());

		bool bIsNearAnotherBuilding = false;

		for (const auto & Elem : HitFriendlies)
		{
			const AActor * HitActor = Elem.GetActor();

			/* Check that they are a building as opposed to a unit and they are still alive */
			if (Statics::IsValid(HitActor) && Statics::IsABuilding(HitActor)
				&& !Statics::HasZeroHealth(HitActor))
			{
				/* Check they are either owned by us or that they are allied and we can build off
				them */
				if (FactionInfo->CanBuildOffAllies() || Statics::IsOwned(HitActor, PlayerState))
				{
					/* Expected since we traced against our own teams collision channel */
					assert(Statics::IsFriendly(HitActor, PlayerState->GetTeamTag()));

					bIsNearAnotherBuilding = true;
					break;
				}
			}
		}

		if (!bIsNearAnotherBuilding)
		{
			return IsBuildableLocationReturn(PlayerState, EGameWarning::Building_NotCloseEnoughToBase,
				bShowHUDMessage);
		}
	}

	/* Now check if ground is flat enough */

	/* Distance of line trace */
	const float TraceDistance = 64.f;

	/* How much higher or lower a corner point trace impact can be from the center point trace
	impact. Needs to be <= TraceDistance / 2  */
	const float HeightDiffAllowance = 20.f;

	/* Do trace from center of bounds but shifted to the building's floor, then shifted up by half
	TraceDistance */
	FHitResult CenterHit;
	if (Statics::LineTraceSingleByChannel(World, CenterHit, 
		Center - FVector(0.f, 0.f, Extent.Z) + FVector(0.f, 0.f, TraceDistance / 2),
		Center - FVector(0.f, 0.f, Extent.Z) - FVector(0.f, 0.f, TraceDistance / 2),
		GROUND_CHANNEL))
	{
		/* Center trace hit something. Now work out where the 4 corners of the building are
		in world space. This cannot be precomputed because player may have rotated the
		ghost while placing it */

		int32 NumAcceptableCorners = 0;

		for (const auto & Point : BoxCompCorners)
		{
			FHitResult CornerHit;
			if (Statics::LineTraceSingleByChannel(World, CornerHit, Point + FVector(0.f, 0.f, TraceDistance / 2),
				Point - FVector(0.f, 0.f, TraceDistance / 2), GROUND_CHANNEL))
			{
				/* Check if corner trace Z coord is close enough to center trace's Z coord */
				if (CornerHit.Distance <= CenterHit.Distance + HeightDiffAllowance
					&& CornerHit.Distance >= CenterHit.Distance - HeightDiffAllowance)
				{
					NumAcceptableCorners++;
				}
				else
				{
					/* Not close enough. Stop here to avoid doing needless traces */
					break;
				}
			}
			else
			{
				/* If trace did not hit then return false */
				break;
			}
		}

		const EGameWarning Warning = (NumAcceptableCorners == 4)
			? EGameWarning::None : EGameWarning::Building_GroundNotFlatEnough;

		/* If all 4 corners are close enough then return true */
		return IsBuildableLocationReturn(PlayerState, Warning, bShowHUDMessage);
	}
	else
	{
		/* Trace hit nothing, return false */
		return IsBuildableLocationReturn(PlayerState, EGameWarning::Building_GroundNotFlatEnough, bShowHUDMessage);
	}
}

bool Statics::IsBuildableLocationReturn(ARTSPlayerState * PlayerState, EGameWarning PlacementError,
	bool bShowHUDMessage)
{
	const bool bCanPlaceBuilding = (PlacementError == EGameWarning::None);

	if (!bCanPlaceBuilding && bShowHUDMessage)
	{
		PlayerState->OnGameWarningHappened(PlacementError);
	}

	return bCanPlaceBuilding;
}

EBuffOrDebuffApplicationOutcome Statics::Server_TryApplyBuffOrDebuff(AActor * BuffOrDebuffInstigator, ISelectable * InstigatorAsSelectable,
	ISelectable * Target, EStaticBuffAndDebuffType Type, URTSGameInstance * GameInstance)
{
	assert(BuffOrDebuffInstigator->GetWorld()->IsServer() == true);
	assert(Type != EStaticBuffAndDebuffType::None);
	
	if (Target->CanClassAcceptBuffsAndDebuffs())
	{
		const FStaticBuffOrDebuffInfo * BuffOrDebuffInfo = GameInstance->GetBuffOrDebuffInfo(Type);

		return BuffOrDebuffInfo->ExecuteTryApplyBehavior(BuffOrDebuffInstigator, InstigatorAsSelectable,
			Target);
	}
	else
	{
		return EBuffOrDebuffApplicationOutcome::Failure;
	}
}

EBuffOrDebuffApplicationOutcome Statics::Server_TryApplyBuffOrDebuff(AActor * BuffOrDebuffInstigator, ISelectable * InstigatorAsSelectable,
	ISelectable * Target, ETickableBuffAndDebuffType Type, URTSGameInstance * GameInstance)
{
	assert(BuffOrDebuffInstigator->GetWorld()->IsServer() == true);

#if TICKABLE_BUFF_AND_DEBUFF_ENABLED_GAME

	assert(Type != ETickableBuffAndDebuffType::None);
	
	if (Target->CanClassAcceptBuffsAndDebuffs())
	{
		const FTickableBuffOrDebuffInfo * BuffOrDebuffInfo = GameInstance->GetBuffOrDebuffInfo(Type);

		return BuffOrDebuffInfo->ExecuteTryApplyBehavior(BuffOrDebuffInstigator, InstigatorAsSelectable,
			Target);
	}
	else
	{
		return EBuffOrDebuffApplicationOutcome::Failure;
	}
	
#else

	UE_LOG(RTSLOG, Fatal, TEXT("Trying to apply a tickable buff/debuff (%s) to a target but project "
		"has flagged tickable buffs/debuffs as disabled. If this is not what you want then enable "
		"tickable buffs/debuffs in ProjectSettings.h. Otherwise try and track down what logic is "
		"applying this buff/debuff (abilities are likely) and remove it"), 
		TO_STRING<ETickableBuffAndDebuffType>(stringify(ETickableBuffAndDebuffType), Type));

	return EBuffOrDebuffApplicationOutcome::Failure;

#endif
}

void Statics::Client_ApplyBuffOrDebuffGivenOutcome(AActor * BuffOrDebuffInstigator, ISelectable * InstigatorAsSelectable, ISelectable * Target, EStaticBuffAndDebuffType Type, URTSGameInstance * GameInstance, EBuffOrDebuffApplicationOutcome Result)
{
	assert(0);// No implementation
}

void Statics::Client_ApplyBuffOrDebuffGivenOutcome(AActor * BuffOrDebuffInstigator, ISelectable * InstigatorAsSelectable, ISelectable * Target, ETickableBuffAndDebuffType Type, URTSGameInstance * GameInstance, EBuffOrDebuffApplicationOutcome Result)
{
	assert(0);// No implementation
}

const FLinearColor & Statics::GetPlayerColor(const ARTSPlayerState * PlayerState)
{
	assert(SelectableColoringOptions::Rule == ESelectableColoringRule::Player);

	/* There is an assumption here that the player IDs of players given to them by the GS 
	start at 0 and increase by 1 from there for each player so 0, 1, 2, etc. */
	return SelectableColoringOptions::Colors[FCString::Atoi(*PlayerState->GetPlayerID().ToString()) - 1];
}

const FLinearColor & Statics::GetTeamColor(ETeam Team)
{
	assert(SelectableColoringOptions::Rule == ESelectableColoringRule::Team);
	
	return SelectableColoringOptions::Colors[Statics::TeamToArrayIndex(Team)];
}

AActor * Statics::GetActorFromOverlap(const FOverlapInfo & OverlapInfo)
{
	/* Not sure if this will work. The UPrimitiveComponent funcs seem to do 
	OverlapInfo.OverlapInfo.Component.Get()->GetOwner() */
	return OverlapInfo.OverlapInfo.GetActor();
}

void Statics::PlaySound2D(const UObject * WorldContextObject, USoundBase * Sound,
	float VolumeMultiplier, float PitchMultiplier, float StartTime,
	USoundConcurrency * ConcurrencySettings, AActor * OwningActor)
{
	assert(Sound != nullptr);

	UGameplayStatics::PlaySound2D(WorldContextObject, Sound, VolumeMultiplier, PitchMultiplier,
		StartTime, ConcurrencySettings, OwningActor);
}

UFogObeyingAudioComponent * Statics::SpawnSoundAtLocation(ARTSGameState * GameState, ETeam SoundOwnersTeam,
	USoundBase * Sound, FVector Location, ESoundFogRules FogRules, FRotator Rotation,
	float VolumeMultiplier, float PitchMultiplier, float StartTime,
	USoundAttenuation * AttenuationSettings, USoundConcurrency * ConcurrencySettings,
	AActor * OwningActor, bool bAutoDestroy)
{
#if GAME_THREAD_FOG_OF_WAR
	const ETeam LocalPlayersTeam = GameState->GetLocalPlayersTeam();
	
	// Couple of early exits
	if (FogRules == ESoundFogRules::InstigatingTeamOnly)
	{
		if (SoundOwnersTeam != LocalPlayersTeam)
		{
			return nullptr;
		}
	}
	else if (FogRules == ESoundFogRules::DecideOnSpawn)
	{
		const bool bOutsideFog = GameState->GetFogManager()->IsLocationVisible(Location, LocalPlayersTeam);
		if (!bOutsideFog)
		{
			return nullptr;
		}
	}

	if (!Sound || !GEngine || !GEngine->UseSound()) 
	{
		return nullptr;
	}

	UWorld * ThisWorld = GameState->GetWorld();
	if (!ThisWorld || !ThisWorld->bAllowAudioPlayback || ThisWorld->IsNetMode(NM_DedicatedServer))
	{
		return nullptr;
	}

	const bool bIsInGameWorld = ThisWorld->IsGameWorld();

	Statics::FCreateRTSAudioCompParams Params(ThisWorld);
	Params.SetLocation(Location);
	Params.AttenuationSettings = AttenuationSettings;
	
	if (ConcurrencySettings)
	{
		Params.ConcurrencySet.Add(ConcurrencySettings);
	}

	UFogObeyingAudioComponent * AudioComponent = nullptr;

	//--------------------------------------------------
	check(IsInGameThread());

	if (Sound && Params.AudioDevice && GEngine && GEngine->UseSound())
	{
		// Avoid creating component if we're trying to play a sound on an already destroyed actor.
		if (Params.Actor == nullptr || !Params.Actor->IsPendingKill())
		{
			// Listener position could change before long sounds finish
			const FSoundAttenuationSettings * AttenuationSettingsToApply = (Params.AttenuationSettings ? &Params.AttenuationSettings->Attenuation : Sound->GetAttenuationSettingsToApply());

			bool bIsAudible = true;
			// If a sound is a long duration, the position might change before sound finishes so assume it's audible
			if (Params.bLocationSet && Sound->GetDuration() <= 0.5f) // Changed from the engine's threshold of 1 to 0.5
			{
				float MaxDistance = 0.0f;
				float FocusFactor = 0.0f;
				Params.AudioDevice->GetMaxDistanceAndFocusFactor(Sound, Params.World, Params.Location, AttenuationSettingsToApply, MaxDistance, FocusFactor);
				bIsAudible = Params.AudioDevice->SoundIsAudible(Sound, Params.World, Params.Location, AttenuationSettingsToApply, MaxDistance, FocusFactor);
			}

			if (bIsAudible)
			{
				// Use actor as outer if we have one.
				if (Params.Actor)
				{
					AudioComponent = NewObject<UFogObeyingAudioComponent>(Params.Actor, (Params.AudioComponentClass != nullptr) ? (UClass*)Params.AudioComponentClass : UFogObeyingAudioComponent::StaticClass());
				}
				// Let engine pick the outer (transient package).
				else
				{
					AudioComponent = NewObject<UFogObeyingAudioComponent>((Params.AudioComponentClass != nullptr) ? (UClass*)Params.AudioComponentClass : UFogObeyingAudioComponent::StaticClass());
				}

				check(AudioComponent);

				AudioComponent->FogObeyingRule = FogRules;

				AudioComponent->Sound = Sound;
				AudioComponent->bAutoActivate = false;
				AudioComponent->bIsUISound = false;
				AudioComponent->bAutoDestroy = Params.bPlay && Params.bAutoDestroy;
				AudioComponent->bStopWhenOwnerDestroyed = Params.bStopWhenOwnerDestroyed;
#if WITH_EDITORONLY_DATA
				AudioComponent->bVisualizeComponent = false;
#endif
				AudioComponent->AttenuationSettings = Params.AttenuationSettings;
				AudioComponent->ConcurrencySet = Params.ConcurrencySet;

				if (Params.bLocationSet)
				{
					AudioComponent->SetWorldLocation(Params.Location);
				}

				// AudioComponent used in PlayEditorSound sets World to nullptr to avoid situations where the world becomes invalid
				// and the component is left with invalid pointer.
				if (Params.World)
				{
					AudioComponent->RegisterComponentWithWorld(Params.World);
				}
				else
				{
					AudioComponent->AudioDeviceHandle = Params.AudioDevice->DeviceHandle;
				}

				if (Params.bPlay)
				{
					AudioComponent->Play();
				}
			}
			else
			{
				// Don't create a sound component for short sounds that start out of range of any listener
				UE_LOG(RTSLOG, Log, TEXT("FogObeyingAudioComponent not created for out of range Sound %s"), *Sound->GetName());
			}
		}
	}

	//--------------------------------------------------
	
	if (AudioComponent != nullptr)
	{
		AudioComponent->SetWorldLocationAndRotation(Location, Rotation);
		AudioComponent->SetVolumeMultiplier(VolumeMultiplier);
		AudioComponent->SetPitchMultiplier(PitchMultiplier);
		AudioComponent->bAllowSpatialization = bIsInGameWorld;
		AudioComponent->bIsUISound = !bIsInGameWorld;
		AudioComponent->bAutoDestroy = bAutoDestroy;
		AudioComponent->SubtitlePriority = Sound->GetSubtitlePriority();
		AudioComponent->Play(StartTime);

		/* Add audio component to container and possibly mute it if it shouldn't be heard right now. 
		If I can still hear sounds briefly maybe try muting before Play(StartTime) call ?*/
		if (FogRules == ESoundFogRules::AlwaysKnownOnceHeard)
		{
			const bool bOutsideFog = GameState->GetFogManager()->IsLocationVisibleNotChecked(Location, LocalPlayersTeam);
			if (!bOutsideFog)
			{
				GameState->FogSoundsContainer_AlwaysKnownOnceHeard.Add(AudioComponent);
				AudioComponent->bInContainer = true;

				AudioComponent->PseudoMute();
			}
		}
		else if (FogRules == ESoundFogRules::DynamicExceptForInstigatorsTeam)
		{
			if (SoundOwnersTeam != LocalPlayersTeam)
			{
				GameState->FogSoundsContainer_DynamicExceptForInstigatorsTeam.Add(AudioComponent);
				AudioComponent->bInContainer = true;

				const bool bOutsideFog = GameState->GetFogManager()->IsLocationVisibleNotChecked(Location, LocalPlayersTeam);
				if (!bOutsideFog)
				{
					AudioComponent->PseudoMute();
				}
			}
		}
		else if (FogRules == ESoundFogRules::Dynamic)
		{
			GameState->FogSoundsContainer_Dynamic.Add(AudioComponent);
			AudioComponent->bInContainer = true;

			const bool bOutsideFog = GameState->GetFogManager()->IsLocationVisibleNotChecked(Location, LocalPlayersTeam);
			if (!bOutsideFog)
			{
				AudioComponent->PseudoMute();
			}
		}
	}

	return AudioComponent;

#elif MULITHREADED_FOG_OF_WAR

	// TODO

#else // No fog of war

	// Just always play the sound
	return UGameplayStatics::SpawnSoundAtLocation(GameState, Sound, Location, Rotation,
		VolumeMultiplier, PitchMultiplier, StartTime, AttenuationSettings, ConcurrencySettings,
		OwningActor);

#endif
}

UFogObeyingAudioComponent * Statics::SpawnSoundAttached(USoundBase * Sound, USceneComponent * AttachToComponent,
	ESoundFogRules FogRules, FName AttachPointName, FVector Location, FRotator Rotation,
	EAttachLocation::Type LocationType, bool bStopWhenAttachedToDestroyed, float VolumeMultiplier,
	float PitchMultiplier, float StartTime, USoundAttenuation * AttenuationSettings,
	USoundConcurrency * ConcurrencySettings, bool bAutoDestroy)
{
	// TODO take into account fog. Currently always returns null too since I cast to fog obeying comp
	return (UFogObeyingAudioComponent*) UGameplayStatics::SpawnSoundAttached(Sound, AttachToComponent, AttachPointName, Location,
		Rotation, LocationType, bStopWhenAttachedToDestroyed, VolumeMultiplier, PitchMultiplier,
		StartTime, AttenuationSettings, ConcurrencySettings, bAutoDestroy);
}

Statics::FCreateRTSAudioCompParams::FCreateRTSAudioCompParams() 
	: World(nullptr) 
	, Actor(nullptr)
{
	AudioDevice = (GEngine ? GEngine->GetMainAudioDevice() : nullptr);
	CommonInit();
}

Statics::FCreateRTSAudioCompParams::FCreateRTSAudioCompParams(UWorld * InWorld, AActor * InActor) 
	: World(InWorld)
{
	if (InActor)
	{
		check(InActor->GetWorld() == InWorld);
		Actor = InActor;
	}
	else
	{
		Actor = (World ? World->GetWorldSettings() : nullptr);
	}

	AudioDevice = (World ? World->GetAudioDevice() : nullptr);
	CommonInit();
}

Statics::FCreateRTSAudioCompParams::FCreateRTSAudioCompParams(AActor * InActor) 
	: Actor(InActor)
{
	World = (Actor ? Actor->GetWorld() : nullptr);
	AudioDevice = (World ? World->GetAudioDevice() : nullptr);
	CommonInit();
}

Statics::FCreateRTSAudioCompParams::FCreateRTSAudioCompParams(FAudioDevice * InAudioDevice)
	: World(nullptr)
	, Actor(nullptr)
	, AudioDevice(InAudioDevice)
{
	CommonInit();
}

void Statics::FCreateRTSAudioCompParams::SetLocation(FVector InLocation)
{
	if (World != nullptr)
	{
		bLocationSet = true;
		Location = InLocation;
	}
	else
	{
		UE_LOG(LogAudio, Warning, TEXT("AudioComponents created without a World cannot have a location."));
	}
}

void Statics::FCreateRTSAudioCompParams::CommonInit()
{
	AudioComponentClass = UFogObeyingAudioComponent::StaticClass();
	bPlay = false;
	bStopWhenOwnerDestroyed = true;
	bLocationSet = false;
	AttenuationSettings = nullptr;
	ConcurrencySet.Reset();
	Location = FVector::ZeroVector;
}

UDecalComponent * Statics::SpawnDecalAtLocation(UObject * WorldContextObject, UMaterialInterface * DecalMaterial,
	FVector DecalSize, FVector Location, FRotator Rotation, float LifeSpan)
{
	return UGameplayStatics::SpawnDecalAtLocation(WorldContextObject, DecalMaterial, DecalSize, Location, 
		Rotation, LifeSpan);
}

UDecalComponent * Statics::SpawnDecalAtLocation(UObject * WorldContextObject, const FSpawnDecalInfo & DecalInfo, FVector Location)
{
	return Statics::SpawnDecalAtLocation(WorldContextObject, DecalInfo.GetDecal(), DecalInfo.GetSize(), 
		Location, DecalInfo.GetRotation(), DecalInfo.GetDuration());
}

bool Statics::LineTraceSingleByChannel(const UWorld * World, FHitResult & OutHitResult, 
	const FVector & TraceStart, const FVector & TraceEnd, ECollisionChannel Channel, 
	const FCollisionQueryParams & QueryParams)
{
	return World->LineTraceSingleByChannel(OutHitResult, TraceStart, TraceEnd, Channel, QueryParams);

	//return World->SweepSingleByChannel(OutHitResult, TraceStart, TraceEnd, FQuat::Identity,
	//	Channel, FCollisionShape::MakeSphere(0.01f));
}

bool Statics::LineTraceMultiByChannel(const UWorld * World, TArray<FHitResult>& OutHitResults, const FVector & TraceStart, const FVector & TraceEnd, ECollisionChannel Channel, const FCollisionQueryParams & QueryParams)
{
	return World->LineTraceMultiByChannel(OutHitResults, TraceStart, TraceEnd, Channel, QueryParams);
}

bool Statics::LineTraceSingleByObjectType(const UWorld * World, FHitResult & OutHit, const FVector & Start, const FVector & End, const FCollisionObjectQueryParams & ObjectQueryParams, const FCollisionQueryParams & Params)
{
	return World->LineTraceSingleByObjectType(OutHit, Start, End, ObjectQueryParams, Params);
}

bool Statics::CapsuleSweep(const UWorld * World, TArray<FHitResult>& OutNearbyHits, 
	const FVector & Location, const FCollisionObjectQueryParams & QueryParams, float Radius)
{
	// TODO this and other sweeps: are they ok where they are or do they need to move a bit?

	/* Not required but expected */
	assert(OutNearbyHits.Num() == 0);

	return World->SweepMultiByObjectType(OutNearbyHits, Location, Location + 0.01f, FQuat::Identity,
		QueryParams, FCollisionShape::MakeCapsule(Radius, Statics::SWEEP_HEIGHT));
}

float Statics::GetDistance2D(const FVector & Loc1, const FVector & Loc2)
{
	return (FVector2D(Loc1) - FVector2D(Loc2)).Size();
}

float Statics::GetDistance2DSquared(const FVector & Loc1, const FVector & Loc2)
{
	return (FVector2D(Loc1) - FVector2D(Loc2)).SizeSquared();
}

float Statics::GetPathDistanceSqr(const AActor * Actor1, const AActor * Actor2)
{
	return (Actor1->GetActorLocation() - Actor2->GetActorLocation()).SizeSquared();
}

bool Statics::IsSelectableInRangeForAbility(const FContextButtonInfo & AbilityInfo, ISelectable * AbilityUser, ISelectable * AbilityTarget)
{ 
	return Statics::GetSelectablesDistanceForAbilitySquared(AbilityInfo, AbilityUser, AbilityTarget) <= Statics::GetAbilityRangeSquared(AbilityInfo);
}

bool Statics::IsSelectableInRangeForAbility(const FContextButtonInfo & AbilityInfo, ISelectable * AbilityUser, const FVector & Location)
{
	return Statics::GetSelectablesDistanceForAbilitySquared(AbilityInfo, AbilityUser, Location) <= Statics::GetAbilityRangeSquared(AbilityInfo);
}

float Statics::GetAbilityRangeSquared(const FContextButtonInfo & AbilityInfo)
{
	/* 0 is code for 'infinite range'. No point calling this if this true */
	assert(!AbilityInfo.IsInstant() && AbilityInfo.GetMaxRange() > 0.f);

	return FMath::Square(AbilityInfo.GetMaxRange());
}

float Statics::GetSelectablesDistanceForAbilitySquared(const FContextButtonInfo & AbilityInfo, 
	ISelectable * AbilityUser, ISelectable * Target)
{
	// 0 range is code for 'infinite range'. No point calling this if that is the case 
	assert(!AbilityInfo.IsInstant() && AbilityInfo.GetMaxRange() > 0.f);
	
	return AbilityUser->GetDistanceFromAnotherForAbilitySquared(AbilityInfo, Target);
}

float Statics::GetSelectablesDistanceForAbilitySquared(const FContextButtonInfo & AbilityInfo, 
	ISelectable * AbilityUser, const FVector & Location)
{
	// 0 range is code for 'infinite range'. No point calling this if that is the case 
	assert(!AbilityInfo.IsInstant() && AbilityInfo.GetMaxRange() > 0.f);
	
	return AbilityUser->GetDistanceFromLocationForAbilitySquared(AbilityInfo, Location);
}

float Statics::GetSelectablesDistanceSquaredFromInventoryItemActor(ISelectable * Selectable, AActor * InventoryItem)
{
	float Dist = (Selectable->GetActorLocationSelectable() - InventoryItem->GetActorLocation()).SizeSquared2D();

	// No bounds, but may want to in the future

	return Dist;
}

bool Statics::CanBeSelected(const AActor * Selectable)
{
#if GAME_THREAD_FOG_OF_WAR || MULTITHREADED_FOG_OF_WAR
	return Selectable->GetRootComponent()->bVisible == true;
#else
	return true; // Not correct, what if it's stealthed
#endif
}

bool Statics::IsSelectableVisibleLocally(const AActor * Selectable)
{
	return Selectable->GetRootComponent()->bVisible == true;
}

bool Statics::IsSelectableVisible(const AActor * Selectable, const FVisibilityInfo * TeamVisibilityInfo)
{
	assert(Statics::IsValid(Selectable));
	return TeamVisibilityInfo->Actors[Selectable] == true;
}

bool Statics::IsOutsideFog(const AActor * Selectable, const ISelectable * AsSelectable, ETeam Team,
	const FName & TeamTag, const ARTSGameState * GameState)
{
#if GAME_THREAD_FOG_OF_WAR
	/* Because we don't create a FVisibilityInfo entry for selectables on our own team 
	check if the selectable is on our own team first. May want to relook at why that 
	is done and possibly remove it so this if statement can go away FIXME */
	assert(Selectable->Tags[Statics::GetTeamTagIndex()] != NAME_None);
	if (Selectable->Tags[Statics::GetTeamTagIndex()] == TeamTag)
	{
		// Selectables on our own team are always outside fog 
		return true;
	}
	
	const FVisibilityInfo & TeamVisibilityInfo = GameState->GetTeamVisibilityInfo(Team);
	
	assert(Statics::IsValid(Selectable));
	UE_CLOG(TeamVisibilityInfo.Actors.Contains(Selectable) == false, RTSLOG, Fatal,
		TEXT("[%s] not found in team [%s] visiblity info"), *Selectable->GetName(),
		TO_STRING(ETeam, Team));
	
	return TeamVisibilityInfo.IsVisible(Selectable);
#elif MULTITHREADED_FOG_OF_WAR
	return MultithreadedFogOfWarManager::Get().IsSelectableOutsideFog(AsSelectable, Team);
#else
	return true;
#endif
}

bool Statics::IsLocationOutsideFog(const FVector & Location, ETeam Team, AFogOfWarManager * FogManager)
{
#if GAME_THREAD_FOG_OF_WAR
	return FogManager->IsLocationVisibleNotChecked(Location, Team);
#elif MULTITHREADED_FOG_OF_WAR
	return MultithreadedFogOfWarManager::Get()....
#else
	return true;
#endif
}

bool Statics::IsLocationOutsideFogLocally(const FVector & Location, AFogOfWarManager * FogManager)
{
#if GAME_THREAD_FOG_OF_WAR
	return FogManager->IsLocationLocallyVisible(Location);
#elif MULTITHREADED_FOG_OF_WAR
	return MultithreadedFogOfWarManager::Get().IsLocationLocallyVisible(Location);
#else
	return true;
#endif
}

bool Statics::IsLocationOutsideFogLocallyNotChecked(const FVector & Location, AFogOfWarManager * FogManager)
{
#if GAME_THREAD_FOG_OF_WAR
	return FogManager->IsLocationLocallyVisibleNotChecked(Location);
#elif MULTITHREADED_FOG_OF_WAR
	return MultithreadedFogOfWarManager::Get().IsLocationLocallyVisibleNotChecked(Location);
#else
	return true;
#endif
}

EFogStatus Statics::GetLocationVisionStatusLocally(const FVector & Location, AFogOfWarManager * FogManager)
{
#if GAME_THREAD_FOG_OF_WAR
	return FogManager->GetLocationVisibilityStatusLocally(Location);
#elif MULTITHREADED_FOG_OF_WAR
	// I probably need to implement this function
	return MultithreadedFogOfWarManager::Get().GetLocationVisibilityStatusLocally(Location);
#else
	/* Probably not correct. It could be StealthRevealed also. Perhaps add another preproc 
	like ALLOW_STEALTH. If false then yes this is correct but if true then no this is not correct TODO */
	return EFogStatus::Revealed;
#endif
}

EFogStatus Statics::GetLocationVisionStatusLocallyNotChecked(const FVector & Location, AFogOfWarManager * FogManager)
{
#if GAME_THREAD_FOG_OF_WAR
	return FogManager->GetLocationVisibilityStatusLocallyNotChecked(Location);
#elif MULTITHREADED_FOG_OF_WAR
	//TODO
#else
	/* Probably not correct. It could be StealthRevealed also. Perhaps add another preproc
	like ALLOW_STEALTH. If false then yes this is correct but if true then no this is not correct 
	TODO */
	return EFogStatus::Revealed;
#endif
}

#if !UE_BUILD_SHIPPING
bool Statics::IsShouldBeSetAnim(EAnimation AnimType)
{
	return Statics::ImportantAnims.Contains(AnimType);
}
#endif


//////////////////////////////////////////////////////////////
//	Selectable Actor Tag System
//////////////////////////////////////////////////////////////

const int32 Statics::NUM_ACTOR_TAGS = 8;

const FName Statics::NeutralID = FName("-1");
const FName Statics::NEUTRAL_TEAM_TAG = FName("NeutralTeam");
const FName Statics::OBSERVER_TEAM_TAG = FName("ObserverTeam");
const FName Statics::UNTARGETABLE_TAG = FName("RTS_Untargetable");
const FName Statics::BuildingTag = FName("Building");
const FName Statics::UnitTag = FName("Unit");
const FName Statics::InventoryItemTag = FName("InventoryItem");
const FName Statics::AirTag = FName("Air");
const FName Statics::NotAirTag = FName("NotAir");
const FName Statics::HasAttackTag = FName("HasAttack");
const FName Statics::NotHasAttackTag = FName("DoesNotHaveAttack");
const FName Statics::HasZeroHealthTag = FName("ZeroHealth");
const FName Statics::AboveZeroHealthTag = FName("AboveZeroHealth");
const FName Statics::NotHasInventoryTag = FName("NoInventory");
const FName Statics::HasZeroCapacityInventoryTag = FName("ZeroCapacityInventory");
const FName Statics::HasInventoryWithCapacityGreaterThanZeroTag = FName("HasInventory");
const FName  Statics::IsShopThatAcceptsRefundsTag = FName("ShopThatAcceptsRefunds");

int32 Statics::GetPlayerIDTagIndex()
{
	return 0;
}

int32 Statics::GetTeamTagIndex()
{
	return 1;
}

int32 Statics::GetTargetingTypeTagIndex()
{
	return 2;
}

int32 Statics::GetAirTagIndex()
{
	return 3;
}

int32 Statics::GetSelectableTypeTagIndex()
{
	return 4;
}

int32 Statics::GetHasAttackTagIndex()
{
	return 5;
}

int32 Statics::GetZeroHealthTagIndex()
{
	return 6;
}

int32 Statics::GetInventoryTagIndex()
{
	return 7;
}

FName Statics::GenerateTeamTag(int32 SomeNum)
{
	/* Not 100% sure how FName hashes. Hoping this won't cause collisions in TMap */
	return *(FString("Team") + FString::FromInt(SomeNum));
}

bool Statics::HasZeroHealth(const AActor * Selectable)
{
	assert(Selectable != nullptr);
	return Selectable->Tags[Statics::GetZeroHealthTagIndex()] == Statics::HasZeroHealthTag;
}

bool Statics::IsABuilding(const AActor * Selectable)
{
	return Selectable->Tags[Statics::GetSelectableTypeTagIndex()] == Statics::BuildingTag;
}

bool Statics::IsAUnit(const AActor * Selectable)
{
	return Selectable->Tags[Statics::GetSelectableTypeTagIndex()] == Statics::UnitTag;
}

bool Statics::IsOwned(const AActor * Selectable, ARTSPlayerState * PlayerState)
{
	return Selectable->Tags[Statics::GetPlayerIDTagIndex()] == PlayerState->GetPlayerID();
}

bool Statics::IsOwned(const AActor * Selectable, const FName & PlayerID)
{
	return Selectable->Tags[Statics::GetPlayerIDTagIndex()] == PlayerID;
}

bool Statics::IsFriendly(const AActor * Selectable, const FName & LocalPlayersTeamTag)
{
	return Selectable->Tags[Statics::GetTeamTagIndex()] == LocalPlayersTeamTag;
}

bool Statics::IsHostile(const AActor * Selectable, const FName & LocalPlayersTeamTag)
{
	return !IsFriendly(Selectable, LocalPlayersTeamTag);
}

bool Statics::CanTypeBeTargeted(const AActor * Target, const TArray<FName>& TypesThatCanBeTargeted)
{
	return TypesThatCanBeTargeted.Contains(GetTargetingType(Target));
}

bool Statics::CanTypeBeTargeted(const AActor * Target, const TSet<FName>& TypesThatCanBeTargeted)
{
	return TypesThatCanBeTargeted.Contains(GetTargetingType(Target));;
}

bool Statics::HasAttack(const AActor * Selectable)
{
	return Selectable->Tags[Statics::GetHasAttackTagIndex()] == Statics::HasAttackTag;
}

const FName & Statics::GetTargetingType(const AActor * Selectable)
{
	/* TODO: To allow user to add their own tags when setting up selectable save those added tags
	and add them to the end of the Tags array after adding the RTS tags */

	return Selectable->Tags[Statics::GetTargetingTypeTagIndex()];
}

const FName & Statics::GetTargetingType(ETargetingType TargetingType)
{
	assert(TargetingType != ETargetingType::None);
	const int32 ArrayIndex = TargetingTypeToArrayIndex(TargetingType);
	
	// Probably only the engine's hidden MAX value will trigger and it will likely be because adding new 
	// values to ETargetingType
	UE_CLOG(TargetTypeFNames.IsValidIndex(ArrayIndex) == false, RTSLOG, Fatal, TEXT("Array out of "
		"bounds error for targeting type [%s]"), TO_STRING(ETargetingType, TargetingType));
	
	return TargetTypeFNames[ArrayIndex];
}

bool Statics::IsAirUnit(const AActor * Selectable)
{
	return Selectable->Tags[Statics::GetAirTagIndex()] == Statics::AirTag;
}

bool Statics::HasInventory(const AActor * Selectable)
{
	return Selectable->Tags[Statics::GetInventoryTagIndex()] == Statics::HasInventoryWithCapacityGreaterThanZeroTag;
}

bool Statics::IsAShopThatAcceptsRefunds(const AActor * Selectable)
{
	return Selectable->Tags[Statics::GetInventoryTagIndex()] == Statics::IsShopThatAcceptsRefundsTag;
}

const FName & Statics::GetSocketName(ESelectableBodySocket SocketType)
{
	return SocketNames[SocketLocationToArrayIndex(SocketType)];
}

int32 Statics::SocketLocationToArrayIndex(ESelectableBodySocket AttachLocation)
{
	return static_cast<uint8>(AttachLocation) - 1;
}

TArray<FName, TFixedAllocator<Statics::NUM_CUSTOM_MESH_SOCKETS>> Statics::InitSocketNames()
{
	TArray<FName, TFixedAllocator<Statics::NUM_CUSTOM_MESH_SOCKETS>> Array;

	// Order is important 
	Array.Emplace(MeshFloorSocket);
	Array.Emplace(MeshMiddleSocket);
	Array.Emplace(MeshHeadSocket);

	return Array;
}

TArray<FName, TFixedAllocator<Statics::NUM_TARGETING_TYPES>> Statics::InitTargetTypeFNames()
{
	TArray <FName, TFixedAllocator<Statics::NUM_TARGETING_TYPES>> Array;

	for (uint8 i = 0; i < Statics::NUM_TARGETING_TYPES; ++i)
	{
		Array.Emplace(*FString("RTS_TargetingType_" + FString::FromInt(i)));
	}

	return Array;
}

#if !UE_BUILD_SHIPPING
TSet<EAnimation> Statics::InitImportantAnims()
{
	// Possibly not exhaustive
	
	TSet < EAnimation > Set;
	
	Set.Emplace(EAnimation::Attack);
	Set.Emplace(EAnimation::Destroyed);
	
	// Add all the user-specified animation types
	for (uint8 i = AnimationToArrayIndex(EAnimation::DropOffResources) + 1; i < NUM_ANIMATIONS; ++i)
	{
		Set.Emplace(ArrayIndexToAnimation(i));
	}

	return Set;
}
#endif

FText Statics::ConcateText(const FText & Text, int32 Length)
{
	const FString AsString = Text.ToString();
	const int32 StringLength = AsString.Len();
	if (StringLength > Length)
	{
		FString ConcatedString;
		/* Limit string to first x chars, where x is Length */
		for (int32 i = 0; i < Length; ++i)
		{
			ConcatedString += AsString[i];
		}

		return FText::FromString(ConcatedString);
	}
	else
	{
		return FText::FromString(AsString);
	}
}

int32 Statics::TextToInt(const FText & Text)
{
	return FCString::Atoi(*Text.ToString());;
}

FText Statics::IntToText(int32 Int)
{
	return FText::FromString((FString::FromInt(Int)));
}

FString Statics::TeamToStringSlow(ETeam Team)
{
	if (Team == ETeam::Observer)
	{
		return Statics::LOBBY_OBSERVER_COMBO_BOX_OPTION;
	}
	else
	{
		const uint8 AsInt = Statics::TeamToArrayIndex(Team);

		return FString::FromInt(AsInt + 1);
	}
}

ETeam Statics::StringToTeamSlow(const FString & AsString)
{
	if (AsString == Statics::LOBBY_OBSERVER_COMBO_BOX_OPTION)
	{
		return ETeam::Observer;
	}
	else
	{
		const uint8 AsInt = FCString::Atoi(*AsString);

		return Statics::ArrayIndexToTeam(AsInt - 1);
	}
}

ECPUDifficulty Statics::StringToCPUDifficultySlow(const FString & AsString, URTSGameInstance * GameInstance)
{
	for (const auto & Elem : GameInstance->GetCPUDifficultyInfo())
	{
		if (Elem.Value.GetName() == AsString)
		{
			return Elem.Key;
		}
	}

	return ECPUDifficulty::None;
}

FString Statics::FactionToString(EFaction Faction)
{
	return FString::FromInt(Statics::FactionToArrayIndex(Faction));
}

EFaction Statics::StringToFaction(const FString & AsString)
{
	const uint8 AsInt = FCString::Atoi(*AsString);

	return Statics::ArrayIndexToFaction(AsInt);
}

ECollisionChannel Statics::IntToCollisionChannel(uint64 AsInt)
{
	return static_cast<ECollisionChannel>(AsInt);
}

int32 Statics::TeamToArrayIndex(ETeam Team, bool bNeutralAndObserverAcceptable)
{
#if !UE_BUILD_SHIPPING
	assert(Team != ETeam::Uninitialized);

	if (!bNeutralAndObserverAcceptable)
	{
		assert(Team != ETeam::Neutral && Team != ETeam::Observer);
	}
#endif

	return static_cast<int32>(Team) - 1;
}

ETeam Statics::ArrayIndexToTeam(int32 ArrayIndex)
{
	assert(ArrayIndex <= UINT8_MAX);
	return static_cast<ETeam>(ArrayIndex + 1);
}

int32 Statics::AnimationToArrayIndex(EAnimation Animation)
{
	return static_cast<int32>(Animation) - 1;
}

EAnimation Statics::ArrayIndexToAnimation(int32 ArrayIndex)
{
	assert(ArrayIndex < UINT8_MAX);
	return static_cast<EAnimation>(ArrayIndex + 1);
}

int32 Statics::ContextButtonToArrayIndex(EContextButton InButton)
{
	/* This throws a lot after post edits. Temporary solution that works most of the time is to
	add button to context menu then remove it */
	assert(InButton != EContextButton::None);
	return static_cast<int32>(InButton) - 1;
}

EContextButton Statics::ArrayIndexToContextButton(int32 ArrayIndex)
{
	assert(ArrayIndex < UINT8_MAX);
	return static_cast<EContextButton>(ArrayIndex + 1);
}

int32 Statics::ArmourTypeToArrayIndex(EArmourType ArmourType)
{
	return static_cast<int32>(ArmourType) - 1;
}

EArmourType Statics::ArrayIndexToArmourType(int32 ArrayIndex)
{
	assert(ArrayIndex < UINT8_MAX);
	return static_cast<EArmourType>(ArrayIndex + 1);
}

int32 Statics::TargetingTypeToArrayIndex(ETargetingType TargetingType)
{
	const int32 Index = static_cast<int32>(TargetingType) - 1;

	/* If thrown then targeting type needs to be ste in blueprint of selectable */
	assert(Index != -1);

	return Index;
}

int32 Statics::ResourceTypeToArrayIndex(EResourceType ResourceType)
{
	assert(ResourceType != EResourceType::None);
	return static_cast<int32>(ResourceType) - 1;
}

EResourceType Statics::ArrayIndexToResourceType(int32 Index)
{
	assert(Index < UINT8_MAX);
	return static_cast<EResourceType>(Index + 1);
}

int32 Statics::HousingResourceTypeToArrayIndex(EHousingResourceType ResourceType)
{
	return static_cast<int32>(ResourceType) - 1;
}

EHousingResourceType Statics::ArrayIndexToHousingResourceType(int32 ArrayIndex)
{
	const EHousingResourceType Type = static_cast<EHousingResourceType>(ArrayIndex + 1);
	assert(Type != EHousingResourceType::None);
	return Type;
}

int32 Statics::StartingResourceAmountToArrayIndex(EStartingResourceAmount Amount)
{
	assert(Amount != EStartingResourceAmount::Default);
	return static_cast<int32>(Amount) - 1;
}

EStartingResourceAmount Statics::ArrayIndexToStartingResourceAmount(int32 ArrayIndex)
{
	assert(ArrayIndex < UINT8_MAX);
	return static_cast<EStartingResourceAmount>(ArrayIndex + 1);
}

EGameNotification Statics::ArrayIndexToGameNotification(int32 ArrayIndex)
{
	return static_cast<EGameNotification>(ArrayIndex + 1);
}

int32 Statics::FactionToArrayIndex(EFaction AsFaction)
{
	// To pick up on uninitialized values
	assert(AsFaction != EFaction::None);
	return static_cast<int32>(AsFaction) - 1;
}

EFaction Statics::ArrayIndexToFaction(int32 ArrayIndex)
{
	EFaction Faction = static_cast<EFaction>(ArrayIndex + 1);
	assert(Faction != EFaction::None && Faction != EFaction::z_ALWAYS_LAST_IN_ENUM);
	return Faction;
}

EHUDPersistentTabType Statics::ArrayIndexToPersistentTabType(int32 ArrayIndex)
{
	assert(ArrayIndex <= UINT8_MAX);
	// No + 1 to include None
	return static_cast<EHUDPersistentTabType>(ArrayIndex);
}

EAffiliation Statics::ArrayIndexToAffiliation(int32 ArrayIndex)
{
	return static_cast<EAffiliation>(ArrayIndex + 1);
}

ECommandTargetType Statics::ArrayIndexToCommandTargetType(int32 ArrayIndex)
{
	return static_cast<ECommandTargetType>(ArrayIndex);
}

ECommandTargetType Statics::AffiliationToCommandTargetType(EAffiliation Affiliation)
{
	// Observers shouldn't be able to issue commands
	assert(Affiliation != EAffiliation::Unknown && Affiliation != EAffiliation::Observed);
	return static_cast<ECommandTargetType>(static_cast<uint8>(Affiliation));
}

int32 Statics::DefeatConditionToArrayIndex(EDefeatCondition DefeatCondition)
{
	assert(DefeatCondition != EDefeatCondition::None);
	return static_cast<int32>(DefeatCondition) - 1;
}

EDefeatCondition Statics::ArrayIndexToDefeatCondition(int32 ArrayIndex)
{
	return static_cast<EDefeatCondition>(ArrayIndex + 1);
}

EGameWarning Statics::ArrayIndexToGameWarning(int32 ArrayIndex)
{
	return static_cast<EGameWarning>(ArrayIndex + 1);
}

EMessageRecipients Statics::ArrayIndexToMessageRecipientType(int32 ArrayIndex)
{
	return static_cast<EMessageRecipients>(ArrayIndex + 1);
}

ECPUDifficulty Statics::ArrayIndexToCPUDifficulty(int32 ArrayIndex)
{
	return static_cast<ECPUDifficulty>(ArrayIndex + 2);
}

EMatchWidgetType Statics::ArrayIndexToMatchWidgetType(int32 ArrayIndex)
{
	return static_cast<EMatchWidgetType>(ArrayIndex + 1);
}

EUnitType Statics::ArrayIndexToUnitType(int32 ArrayIndex)
{
	const EUnitType UnitType = static_cast<EUnitType>(ArrayIndex + 1);
	assert(UnitType != EUnitType::None);
	return UnitType;
}

int32 Statics::UnitTypeToArrayIndex(EUnitType UnitType)
{
	assert(UnitType != EUnitType::None);
	return static_cast<int32>(UnitType) - 1;
}

EUpgradeType Statics::ArrayIndexToUpgradeType(int32 ArrayIndex)
{
	return static_cast<EUpgradeType>(ArrayIndex);
}

EStaticBuffAndDebuffType Statics::ArrayIndexToStaticBuffOrDebuffType(int32 ArrayIndex)
{
	return static_cast<EStaticBuffAndDebuffType>(ArrayIndex + 1);
}

ETickableBuffAndDebuffType Statics::ArrayIndexToTickableBuffOrDebuffType(int32 ArrayIndex)
{
	return static_cast<ETickableBuffAndDebuffType>(ArrayIndex + 1);
}

EBuffAndDebuffSubType Statics::ArrayIndexToBuffOrDebuffSubType(int32 ArrayIndex)
{
	return static_cast<EBuffAndDebuffSubType>(ArrayIndex + 1);
}

EAbilityRequirement Statics::ArrayIndexToAbilityRequirement(int32 ArrayIndex)
{
	return static_cast<EAbilityRequirement>(ArrayIndex + 2);
}

ESelectableResourceType Statics::ArrayIndexToSelectableResourceType(int32 ArrayIndex)
{
	ESelectableResourceType Type = static_cast<ESelectableResourceType>(ArrayIndex);
	assert(Type != ESelectableResourceType::None);
	return Type;
}

EInventoryItem Statics::ArrayIndexToInventoryItem(int32 ArrayIndex)
{ 
	return static_cast<EInventoryItem>(ArrayIndex);
}

int32 Statics::InventoryItemToArrayIndex(EInventoryItem InventoryItem)
{
	return static_cast<int32>(InventoryItem);
}

ECommanderAbility Statics::ArrayIndexToCommanderAbilityType(int32 ArrayIndex)
{
	return static_cast<ECommanderAbility>(ArrayIndex + 1);
}

int32 Statics::CommanderAbilityTypeToArrayIndex(ECommanderAbility AbilityType)
{
	return static_cast<int32>(AbilityType) - 1;
}

int32 Statics::CommanderSkillTreeNodeToArrayIndex(ECommanderSkillTreeNodeType NodeType)
{
	return static_cast<int32>(NodeType) - 1;
}

EControlSettingType Statics::ArrayIndexToControlSettingType(int32 ArrayIndex)
{
	return static_cast<EControlSettingType>(ArrayIndex + 1);
}

int32 Statics::PhysicalSurfaceTypeToArrayIndex(EPhysicalSurface SurfaceType)
{
	return static_cast<int32>(SurfaceType);
}

EBuildingNetworkType Statics::ArrayIndexToBuildingNetworkType(int32 ArrayIndex)
{
	return static_cast<EBuildingNetworkType>(ArrayIndex + 1);
}

int32 Statics::BuildingNetworkTypeToArrayIndex(EBuildingNetworkType NetworkType)
{
	return static_cast<int32>(NetworkType) - 1;
}

EEditorPlaySkippingOption Statics::ArrayIndexToEditorPlaySkippingOption(int32 ArrayIndex)
{
	return static_cast<EEditorPlaySkippingOption>(ArrayIndex);
}

EInvalidOwnerIndexAction Statics::ArrayIndexToPIESeshInvalidOwnerRule(int32 ArrayIndex)
{
	return static_cast<EInvalidOwnerIndexAction>(ArrayIndex);
}


