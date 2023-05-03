// Fill out your copyright notice in the Description page of Project Settings.

#include "Building.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/Character.h"
#include "Components/DecalComponent.h"
#include "Public/TimerManager.h"
#include "UnrealNetwork.h"
#include "Animation/AnimInstance.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/BillboardComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/Texture2D.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/WorldSettings.h"

#include "Statics/DevelopmentStatics.h"
#include "Statics/Statics.h"
#include "GameFramework/RTSPlayerController.h"
#include "GameFramework/FactionInfo.h"
#include "GameFramework/RTSPlayerState.h"
#include "GameFramework/RTSGameState.h"
#include "GameFramework/RTSGameMode.h"
#include "GameFramework/RTSGameInstance.h"
#include "Managers/UpgradeManager.h"
#include "UI/RTSHUD.h"
#include "UI/WorldWidgets/SelectableWidgetComponent.h"
#include "Managers/FogOfWarManager.h"
#include "MapElements/GhostBuilding.h"
#include "Networking/RTSReplicationGraph.h"
#include "Audio/FogObeyingAudioComponent.h"
#include "MapElements/ResourceSpot.h"
#include "MapElements/Animation/BuildingAnimInstance.h"
#include "Miscellaneous/CPUPlayerAIController.h"
#include "Managers/HeavyTaskManager.h"
#include "MapElements/BuildingComponents/BuildingAttackComp_Turret.h"
#include "MapElements/BuildingComponents/BuildingAttackComp_TurretsBase.h"


const float ABuilding::TimeToSpendSinking = 5.f;

ABuilding::ABuilding()
{
	PrimaryActorTick.bCanEverTick = true;

	Bounds = CreateDefaultSubobject<UBoxComponent>(TEXT("Bounds"));
	SetRootComponent(Bounds);

	Bounds->bVisible = true;
	Bounds->bHiddenInGame = true;
	Bounds->SetCollisionProfileName(FName("BuildingBounds"));
	Bounds->SetGenerateOverlapEvents(false);
	Bounds->SetCanEverAffectNavigation(true);
	Bounds->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	Bounds->bReceivesDecals = false;

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Bounds);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	/* TODO: review if changing profile is needed */
	Mesh->SetCollisionProfileName(FName("BuildingMesh"));
	Mesh->SetCanEverAffectNavigation(true);
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	Mesh->bReceivesDecals = false;

	SelectionWidgetComponent = CreateDefaultSubobject<USelectableWidgetComponent>(TEXT("Selection Widget"));
	SelectionWidgetComponent->SetupAttachment(Bounds);

	PersistentWidgetComponent = CreateDefaultSubobject<USelectableWidgetComponent>(TEXT("Persistent Widget"));
	PersistentWidgetComponent->SetupAttachment(Bounds);

	SelectionDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("Selection Decal"));
	SelectionDecal->SetupAttachment(RootComponent);
	SelectionDecal->bVisible = false;
	// So decal will not rotate when building rotates
	SelectionDecal->bAbsoluteRotation = true;
	SelectionDecal->bDestroyOwnerAfterFade = false;
	SelectionDecal->RelativeRotation = FRotator(-90.f, 0.f, 0.f);
	SelectionDecal->RelativeScale3D = FVector(0.1f, 1.f, 1.f);
	SelectionDecal->RelativeLocation = FVector(0.f, 0.f, -Bounds->GetScaledBoxExtent().Z);

	SelectionParticles = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("Selection Particles"));
	SelectionParticles->SetupAttachment(Bounds);
	SelectionParticles->bAutoActivate = false;
	SelectionParticles->bAbsoluteRotation = true;
	SelectionParticles->SetCollisionProfileName(FName("NoCollision"));
	SelectionParticles->SetGenerateOverlapEvents(false);
	SelectionParticles->SetCanEverAffectNavigation(false);
	SelectionParticles->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	SelectionParticles->bReceivesDecals = false;

	UnitSpawnLoc = CreateDefaultSubobject<USceneComponent>(TEXT("Unit Spawn Location"));
	UnitSpawnLoc->SetupAttachment(Mesh);
	UnitSpawnLoc->SetCanEverAffectNavigation(false);

	UnitFirstMoveLoc = CreateDefaultSubobject<USceneComponent>(TEXT("Unit First Move Loc"));
	UnitFirstMoveLoc->SetupAttachment(Mesh);
	UnitFirstMoveLoc->SetCanEverAffectNavigation(false);

	UnitRallyPoint = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Unit Rally Loc"));
	UnitRallyPoint->SetupAttachment(Mesh);
	UnitRallyPoint->SetGenerateOverlapEvents(false);
	UnitRallyPoint->SetCanEverAffectNavigation(false);
	UnitRallyPoint->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	UnitRallyPoint->bReceivesDecals = false;
	UnitRallyPoint->SetCollisionProfileName(FName("NoCollision"));

#if WITH_EDITORONLY_DATA

	UnitSpawnLocArrow = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("Unit Spawn Loc Arrow"));
	if (UnitSpawnLocArrow != nullptr)
	{
		UnitSpawnLocArrow->SetupAttachment(UnitSpawnLoc);
	}

	// Use target point sprite
	auto Sprite = ConstructorHelpers::FObjectFinder<UTexture2D>(TEXT("/Engine/EditorMaterials/TargetIcon"));

	UnitFirstMoveSprite = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("First Move Spirite"));
	if (UnitFirstMoveSprite != nullptr)
	{
		UnitFirstMoveSprite->SetupAttachment(UnitFirstMoveLoc);
		if (Sprite.Object != nullptr)
		{
			UnitFirstMoveSprite->Sprite = Sprite.Object;
		}
	}

	UnitRallySprite = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Default Rally Sprite"));
	if (UnitRallySprite != nullptr)
	{
		UnitRallySprite->SetupAttachment(UnitRallyPoint);
		if (Sprite.Object != nullptr)
		{
			UnitRallySprite->Sprite = Sprite.Object;
		}
	}

#endif // WITH_EDITORONLY_DATA

	/* For 'works out of the box' functionality assign the default ghost. Will likely have a
	cube or something for its mesh or have no mesh at all since there are no engine default 
	skeletal meshes */
	Ghost_BP = AGhostBuilding::StaticClass();

	CreationMethod = ESelectableCreationMethod::Uninitialized;
	GameTickCountOnConstructionComplete = UINT8_MAX;

	Attributes.SetupBasicTypeInfo(ESelectableType::Building, Type, EUnitType::NotUnit);

	/* Replication defaults */

	bReplicates = true;
	bReplicateMovement = false;
	bAlwaysRelevant = true; // MAYBE don't need this, but true for now
}

void ABuilding::PostLoad()
{
	Super::PostLoad();

	bStartedInMap = true;
}

void ABuilding::PreInitializeComponents()
{
	Super::PreInitializeComponents();
}

void ABuilding::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (Attributes.CanProduce())
	{
		/* "Detach" rally point mesh from building location wise. Ideally should be true all along
		for game world but false for editor worlds */
		UnitRallyPoint->bAbsoluteLocation = true;
	}
	else
	{
		/* Destroy UnitRallyPoint if possible since not needed. Ehhh who cares. Probably no
		persistent runtime cost since won't be rendered and won't move once building is
		constructed. But do it anyway */
		UnitRallyPoint->DestroyComponent();
	}

#if WITH_EDITOR
	if (GetWorld()->WorldType != EWorldType::EditorPreview)
#endif
	{
		GI = CastChecked<URTSGameInstance>(GetGameInstance());

		SetInitialBuildingVisibilityV2();
	}
}

void ABuilding::RunStartOfBeginPlayLogic()
{
	bHasRunStartOfBeginPlay = true;

	GS = CastChecked<ARTSGameState>(GetWorld()->GetGameState());

	/* Populate AttackComponents_Turrets and
	AttackComponents_TurretsBases and call SetHasRotatingBase on any applicable attack components.
	Tried doing this during post edit but the component pointers sometimes end up null.
	Maybe it's something to do with TArray::Empty() just not working for me in post edit.
	Could revisit this later */
	assert(Attributes.AttackComponents_Turrets.Num() == 0);
	assert(Attributes.AttackComponents_TurretsBases.Num() == 0);
	uint8 TurretAttackCompUniqueID = 0;
	for (UActorComponent * Comp : BlueprintCreatedComponents)
	{
		IBuildingAttackComp_Turret * AttackComp = Cast<IBuildingAttackComp_Turret>(Comp);
		if (AttackComp != nullptr)
		{
			Attributes.AttackComponents_Turrets.Emplace(CastChecked<UMeshComponent>(AttackComp));

			/* Check if the turret has a rotating base */
			IBuildingAttackComp_TurretsBase * AttackCompBase = Cast<IBuildingAttackComp_TurretsBase>(AttackComp->GetAsMeshComponent()->GetAttachParent());
			if (AttackCompBase != nullptr)
			{
				AttackComp->SetupAttackComp(this, GS, GI->GetPoolingManager(), TurretAttackCompUniqueID++, AttackCompBase);
				AttackCompBase->SetupComp(this, Comp);
				Attributes.AttackComponents_TurretsBases.Emplace(CastChecked<UMeshComponent>(AttackCompBase));
			}
			else
			{
				AttackComp->SetupAttackComp(this, GS, GI->GetPoolingManager(), TurretAttackCompUniqueID++, AttackCompBase);
			}
		}
	}

	// Start hidden. Only when owner is known is it possible to be revealed. Already done in PostInitializeComponents
	//SetInitialBuildingVisibility();
	Attributes.SetVisionState(EFogStatus::Hidden); // I don't do this in ranged inf. Does it need it?
}

void ABuilding::BeginPlay()
{
	/* Note the OnRep_ functions can be called before BeginPlay */
	
	/* If placed in map via editor then stop here. Usually true if using PIE and this was placed
	in map before PIE session started */
	if (bStartedInMap)
	{
		SetActorTickEnabled(false);
		return;
	}

	if (bHasRunStartOfBeginPlay == false)
	{
		RunStartOfBeginPlayLogic();
	}
	
	Super::BeginPlay();

	/* On clients do all this when owner (player state) reps */
	if (GetWorld()->IsServer() == false)
	{
		return;
	}

	/* Stop here if just spawning for faction info */
	if (GI->IsInitializingFactionInfo())
	{
		SetActorTickEnabled(false); // Precaution, probably not needed
		return;
	}

	Setup();
}

void ABuilding::OnRep_ID()
{
	/* TODO: override OnRep_Owner. When that and this have been called for server then
	this can initialize itself. Also when owner reps will need to get their faction info
	from GI to determine building build method. ID only reps to the owning player though,
	if building is hostile then player will not have it rep to them */

	// See if we can setup now
	Attributes.IncrementNumReppedVariablesReceived();
	if (Attributes.CanSetup())
	{
		Setup();
	}
}

void ABuilding::OnRep_CreationMethod()
{
	/* This along with ID and owner need to come through before we can start setup. Do not need
	to assign anything*/

	// Duplicate copy of this variable, may as well update it
	Attributes.SetCreationMethod(CreationMethod);

	// See if we can setup now
	Attributes.IncrementNumReppedVariablesReceived();
	if (Attributes.CanSetup())
	{
		Setup();
	}
}

void ABuilding::OnRep_Owner()
{
	/* The reason this isn't called when testing in PIE is because replication is paused for
	actors that aren't visible. And we're going to have a problem maybe when actor attacks
	because owner is not set */
	
	assert(!GetWorld()->IsServer());
	assert(GetOwner() != nullptr);
	/* Check we haven't called this before */
	assert(PS == nullptr);
	assert(GI != nullptr);

	Super::OnRep_Owner();

	/* Check if just spawning to get information. If we are then we can stop here */
	if (GI->IsInitializingFactionInfo())
	{
		return;
	}

	// See if we can setup now
	Attributes.IncrementNumReppedVariablesReceived();
	if (Attributes.CanSetup())
	{
		Setup();
	}
}

void ABuilding::OnRep_GameTickCountOnConstructionComplete()
{
	/* There is no need to send this data if the building is a starting selectable. I don't 
	know why I have this here but I'm pretty sure this is not the case. Well actually if 
	all the starting selectables are spawned at the same time then I guess it would be possible 
	to know what the count is but I hadn't thought of that when I wrote this. Don't know what 
	I was thinking */
	//assert(IsStartingSelectable() == false);

	/* If have not called Setup then do nothing. */
	if (Attributes.GetAffiliation() == EAffiliation::Unknown || bHasRunOnConstructionCompleteLogic)
	{
		return;
	}

	/* We do this to halt trying to progress construction in tick. */
	ConstructionProgressInfo.LastTimeWhenConstructionStarted = -1.f;

	if (Attributes.IsAStartingSelectable())
	{
		Client_FinalizeConstructionComplete();
	}
	else
	{
		Client_OnConstructionComplete();
	}
}

void ABuilding::OnRep_AnimationState()
{
	MESSAGE("OnRep_AnimationState");
	
	// Check if building has been completely destroyed
	if (HasBeenCompletelyDestroyed())
	{
		OnZeroHealth(EOnZeroHealthCallSite::Other);
		ClientCleanUp(false);
		return;
	}
	
	// Special case for the destroyed animation
	if (AnimationState.AnimationType == EBuildingAnimation::Destroyed)
	{
		// Possibly don't need this line
		//PausedAnimInfo.AnimationType = EBuildingAnimation::None;

		if (AreAnimationsPausedLocally())
		{
			PausedAnimInfo.AnimationType = AnimationState.AnimationType;
			PausedAnimInfo.StartTime = AnimationState.StartTime;
		}
		else
		{
			// Not sure if this code is solid. I cannot force the order OnReps fire so it is hard to test in editor
			
			/* Run this logic. It will do nothing if we have already run it */
			OnZeroHealth(EOnZeroHealthCallSite::Other);

			bHasLocalPlayerSeenBuildingAtZeroHealth = true;

			const float TimeSinceBuildingReachedZeroHealth = GS->GetServerWorldTimeSeconds() - AnimationState.StartTime;

			// Check if the Destroyed anim has progressed to the sinking into ground portion
			if (bSinkingIntoGround_Logically || bSinkingIntoGround_Visually)
			{
				TimeSpentSinking = TimeSinceBuildingReachedZeroHealth - TimeIntoZeroHealthAnimThatAnimNotifyIs;

				// Resume from right location
				const FVector Loc = GetActorLocation();
				SetActorLocation(FVector(Loc.X, Loc.Y, GetFinalLocation().Z - (TimeSpentSinking * BoundsHeight * (1.f / TimeToSpendSinking))));

				return;
			}

			/* Check if enough time has passed to trigger the anim notify OnZeroHealthAnimationFinished */
			if (TimeSinceBuildingReachedZeroHealth >= TimeIntoZeroHealthAnimThatAnimNotifyIs)
			{
				UAnimMontage * DestroyedMontage = Attributes.GetAnimation(EBuildingAnimation::Destroyed);
				assert(DestroyedMontage != nullptr);

				/* Set animation montage to the Destroyed montage.
				Set start time to be where the anim notify is. Anim is frozen here later. It could be
				off a bit from where it would freeze if the player was actually witnessing the
				destroyed anim outside fog.
				I don't think this code works though. Need to find a way to freeze anim */
				Mesh->GetAnimInstance()->Montage_Play(DestroyedMontage, 1.f, EMontagePlayReturnType::MontageLength,
					TimeIntoZeroHealthAnimThatAnimNotifyIs);

				// My attempt at making skeletal mesh update to the anim I just set above
				//Mesh->TickComponent(0.1f, LEVELTICK_All, nullptr);
				//Mesh->TickPose(0.1f, false);
				//Mesh->TickAnimation(0.1f, false);

				RunOnZeroHealthAnimFinishedLogic();
			}
			else
			{
				const float TimeTillAnimNotify = TimeIntoZeroHealthAnimThatAnimNotifyIs - TimeSinceBuildingReachedZeroHealth;
				Delay(&ABuilding::OnZeroHealthAnimFinished, TimeTillAnimNotify);

				Mesh->bNoSkeletonUpdate = false;
				Mesh->bPauseAnims = false;

				PlayAnimInternal();
			}
		}

		return;
	}
	// Special case for SinkIntoGround
	else if (AnimationState.AnimationType == EBuildingAnimation::SinkIntoGround)
	{
		/* None means building has begun to sink into the ground on server. We use  
		AnimationState.StartTime to know when it started */

		// Do nothing if already sinking
		if (bSinkingIntoGround_Visually)
		{
			// For clients these should always be equal
			assert(bSinkingIntoGround_Logically == bSinkingIntoGround_Visually);
			return;
		}

		if (IsFullyVisibleLocally())
		{
			// bHasLocalPlayerSeenBuildingAtZeroHealth= IsFullyVisibleLocally(); Can probably do this and remove the if statement because that was the last anim rep we'll ever get
			bHasLocalPlayerSeenBuildingAtZeroHealth = true; 
		}

		// Pause animation. Look weird when building sinks into ground while something like idle anim is playing
		Mesh->bNoSkeletonUpdate = true;
		Mesh->bPauseAnims = true;

		TimeSpentSinking = GS->GetServerWorldTimeSeconds() - AnimationState.StartTime;
		// Set position for how far through sinking building is
		AddActorLocalOffset(FVector(0.f, 0.f, -TimeSpentSinking * BoundsHeight * (1.f / TimeToSpendSinking)));
		BeginSinkingIntoGround();

		return;
	}

	if (AreAnimationsPausedLocally())
	{
		PausedAnimInfo.AnimationType = AnimationState.AnimationType;
		PausedAnimInfo.StartTime = AnimationState.StartTime;
	}
	else
	{
		assert(PausedAnimInfo.AnimationType == EBuildingAnimation::None);

		PlayAnimInternal();
	}
}

void ABuilding::PlayAnimInternal()
{	
	UAnimMontage * Montage = Attributes.GetAnimation(AnimationState.AnimationType);
	assert(Montage != nullptr); // Server should have already checked this

	// StartTime calculated just like infantry... which isn't very good in the first place
	float StartTime = GS->GetServerWorldTimeSeconds() - AnimationState.StartTime;
	StartTime = FMath::Fmod(StartTime, Montage->SequenceLength);

	GetMesh()->GetAnimInstance()->Montage_Play(Montage, 1.f,
		EMontagePlayReturnType::MontageLength, StartTime);
}

void ABuilding::Setup()
{
	if (bHasRunStartOfBeginPlay == false)
	{
		RunStartOfBeginPlayLogic();
	}
	
	SetupSelectionInfo();

	if (!GetWorld()->IsServer())
	{
		// Already did on server, but wouldn't be wrong to do it anyway
		PS->Client_RegisterSelectableID(ID, this);
	}

	AdjustForUpgrades();

	SetupWidgetComponents();

	SetupBuildingAttackComponents();

	InitProductionQueues();

	// Set rally point location if a unit producing building
	if (Attributes.CanProduce())
	{
		/* Gotta do this if on server and let location rep.
		Originally was doing this for if either owner or server. I get interesting results with
		the line traces. The client line trace hits some location near the origin that means
		nothing while the server one hits the correct location. So basically cannot avoid sending
		vector update across wire on setup. Can reinvestigate this later */
		if (GetWorld()->IsServer())// && !bProductionIsForAirUnits)
		{
			/* Gonna have to ray trace down against environment channel searching for ground */

			// Make it a decent height since could be near cliffs
			const float TraceHeight = 8192.f;

			FHitResult HitResult;
			const FVector StartLoc = UnitRallyPoint->GetComponentLocation() + FVector(0.f, 0.f, TraceHeight / 2);
			const FVector EndLoc = UnitRallyPoint->GetComponentLocation() + FVector(0.f, 0.f, -TraceHeight / 2);
			if (GetWorld()->LineTraceSingleByChannel(HitResult, StartLoc, EndLoc, GROUND_CHANNEL))
			{
				RallyPointLocation = HitResult.ImpactPoint;

				OnRep_RallyPointLocation();
			}
			else
			{
				/* Uh-oh. Didn't hit ground. Rally point will most likely be somewhere below ground.
				Maybe do another trace but make height massive this time? */

				UE_LOG(RTSLOG, Warning, TEXT("Trace for unit rally point initial location did not hit "
					"environment. Trace start: %s, trace end: %s"), *StartLoc.ToString(), *EndLoc.ToString());
			}
		}
	}

	Attributes.GetGarrisonAttributes().OnOwningBuildingSetup(PS);

	GS->OnBuildingPlaced(this, Attributes.GetTeam(), GetWorld()->IsServer());
	PS->OnBuildingPlaced(this);

	/* Setting Attributes.BuildMethod means we can turn off replication for 2 floats so 
	do that now. Actually I don't do that anymore */
	const FBuildingInfo * BuildInfo = FI->GetBuildingInfo(Type);
	Attributes.SetBuildMethod(BuildInfo->GetBuildingBuildMethod());

	TimeIntoZeroHealthAnimThatAnimNotifyIs = BuildInfo->GetTimeIntoZeroHealthAnimThatAnimNotifyIs();

	/* Check if this is being spawned as one of the starting buildings in the match. If so then
	place it already constructed */
	assert(CreationMethod == Attributes.GetCreationMethod());
	if (GetWorld()->IsServer())
	{
		if (Attributes.IsAStartingSelectable())
		{
			SpawnLocation = GetActorLocation();
			Server_OnConstructionComplete();
		}
		else
		{
			SetupInitialConstructionValues(BuildInfo);

			PlayJustPlacedParticles(); 
			PlayJustPlacedSound();
		}
	}
	else
	{
		if (Attributes.IsAStartingSelectable())
		{
			SpawnLocation = GetActorLocation();
			
			/* Here we're basically checking if the HasCompleteConstruction variable has repped 
			yet */
			if (IsConstructionComplete())
			{
				Client_OnConstructionComplete();
			}
			else
			{
				Client_SetAppearanceForConstructionComplete();
			}
		}
		else
		{
			if (IsConstructionComplete())
			{
				SpawnLocation = GetActorLocation();
				Client_OnConstructionComplete();
			}
			else
			{
				SetupInitialConstructionValues(BuildInfo);
			}
			
			PlayJustPlacedParticles();
			// Possibly we'll want to play it part way through because it's possible for 
			// Setup() to be called long after building is placed 
			PlayJustPlacedSound();
		}
	}
}

void ABuilding::SetupSelectionInfo()
{
	PS = CastChecked<ARTSPlayerState>(GetOwner());

	assert(PS->GetPlayerID() != FName());

	/* Set faction info ref */
	FI = GI->GetFactionInfo(PS->GetFaction());

	assert(Tags.Num() == 0);
	Tags.Reserve(Statics::NUM_ACTOR_TAGS);
	Tags.Emplace(PS->GetPlayerID());
	Tags.Emplace(PS->GetTeamTag());
	Tags.Emplace(Statics::GetTargetingType(Attributes.GetTargetingType()));
	Tags.Emplace(Statics::NotAirTag);
	Tags.Emplace(Statics::BuildingTag);
	Tags.Emplace(Attributes.IsDefenseBuilding() ? Statics::HasAttackTag : Statics::NotHasAttackTag);
	Tags.Emplace(Statics::AboveZeroHealthTag);
	Tags.Emplace(GetShopAttributes() != nullptr && GetShopAttributes()->AcceptsRefunds() ? Statics::IsShopThatAcceptsRefundsTag : Statics::NotHasInventoryTag);
	assert(Tags.Num() == Statics::NUM_ACTOR_TAGS); // Make sure I did not forget one

	Attributes.SetupSelectionInfo(PS->GetPlayerID(), PS->GetTeam());

	/* Assign PC and get local player state too */
	APlayerController * const LocalPlayerController = GetWorld()->GetFirstPlayerController();
	PC = CastChecked<ARTSPlayerController>(LocalPlayerController);
	ARTSPlayerState * const LocalPlayerState = PC->GetPS();
	
	if (LocalPlayerState->GetPlayerID() == PS->GetPlayerID())
	{
		Attributes.SetAffiliation(EAffiliation::Owned);
	}
	else if (LocalPlayerState->IsAllied(PS))
	{
		Attributes.SetAffiliation(EAffiliation::Allied);
	}
	else if (PC->IsObserver())
	{
		Attributes.SetAffiliation(EAffiliation::Observed);
	}
	else if (false /* TODO: if neutral*/)
	{
		Attributes.SetAffiliation(EAffiliation::Neutral);
	}
	else /* Assumed hostile */
	{
		Attributes.SetAffiliation(EAffiliation::Hostile);
	}

	PutAbilitiesOnInitialCooldown();

	/* Set selection decal */
	SetupSelectionDecal(PC->GetFactionInfo());

	SetMeshColors();

	/* BoundsHeight is used for both rising from the ground and sinking into the ground */
	BoundsHeight = CalculateBoundsHeight(); 

	/* FIXED: Set visibility */
	if (Attributes.GetAffiliation() == EAffiliation::Hostile)
	{
		// Building already starts hidden, no need to do anything
		//SetBuildingVisibility(false);
		//Attributes.VisionState = EFogStatus::Hidden;
	}
	else // Assumed owned, allied or observed
	{
		SetBuildingVisibilityV2(true);
		Attributes.SetVisionState(EFogStatus::Revealed);
		bHasLocalPlayerSeenBuildingAtLeastOnce = true;
	}

	/* Setup collision. Bounds may not be the best choice since it is a box. Maybe set it up on
	the mesh instead which would allow for some more accurate collision. Bounds could maybe be
	used for traces for selection but navigation etc would use the mesh (have changed it to Mesh 
	now) */
	const ECollisionChannel TeamCollisionChannel = GS->GetTeamCollisionChannel(Attributes.GetTeam());
	Mesh->SetCollisionObjectType(TeamCollisionChannel);
	for (UMeshComponent * AttackComp : Attributes.AttackComponents_Turrets)
	{
		AttackComp->SetCollisionObjectType(TeamCollisionChannel);
	}
}

void ABuilding::AdjustForUpgrades()
{
	PS->GetUpgradeManager()->ApplyAllCompletedUpgrades(this);
}

void ABuilding::PutAbilitiesOnInitialCooldown()
{
	// Only server or owner does this although nothing wrong with everyone doing it
	if (GetWorld()->IsServer() || Attributes.GetAffiliation() == EAffiliation::Owned)
	{
		for (const auto & Button : Attributes.GetContextMenu().GetButtonsArray())
		{
			const FContextButtonInfo & AbilityInfo = GI->GetContextInfo(Button.GetButtonType());
			if (AbilityInfo.GetInitialCooldown() > 0.f)
			{
				// If ability has a preparation part to it then start that timer handle
				if (AbilityInfo.UsesPreparationAnimation())
				{
					CallAbilityPreparationLogicAfterDelay(AbilityInfo, AbilityInfo.GetInitialCooldown());
				}
				
				// Put on cooldown
				Delay(ContextActionCooldowns[Button], &ABuilding::OnAbilityCooldownFinished, AbilityInfo.GetInitialCooldown());
			}
		}
	}
}

void ABuilding::SetupSelectionDecal(AFactionInfo * LocalPlayersFactionInfo)
{
	const FSelectionDecalInfo * DecalInfo;

	if (Attributes.GetAffiliation() == EAffiliation::Observed)
	{
		DecalInfo = &GI->GetObserverSelectionDecal();
	}
	else
	{
		DecalInfo = &LocalPlayersFactionInfo->GetSelectionDecalInfo(Attributes.GetAffiliation());
	}

	SelectionDecal->SetDecalMaterial(DecalInfo->GetDecal());

	if (SelectionDecal->GetDecalMaterial() != nullptr)
	{
		if (DecalInfo->RequiresCreatingMaterialInstanceDynamic())
		{
			Attributes.SetSelectionDecalMaterial(SelectionDecal->CreateDynamicMaterialInstance());
			Attributes.SetSelectionDecalSetup(ESelectionDecalSetup::UsesDynamicMaterial);
		}
		else
		{
			Attributes.SetSelectionDecalSetup(ESelectionDecalSetup::UsesNonDynamicMaterial);
		}
	}
	else
	{
		Attributes.SetSelectionDecalSetup(ESelectionDecalSetup::DoesNotUse);
	}
}

void ABuilding::SetMeshColors()
{
	/* Here I'm creating dynamic material instances. Constant material instances could be used 
	but just would require more work in post edit. Basically the material instance constant would 
	have to be created sometime during WITH_EDITOR and written to disk. One would need to be 
	created for each team. Then at this point we would find the material that corrisponds to 
	the owner's team and just set that as the material. I'm not sure if this would give performance 
	gains over using dynamic material instances but it is worth a shot. 
	The reason I don't want to do this is that it will be hard to maintain in post edits because 
	if the color array in ProjectSettings.h SelectableColoringOptions changes then all the 
	materials would need to be changed too at some point while WITH_EDITOR. Also it would be 
	hard to know that the array was actually changed in C++ (actually could write the values 
	to file, then after every compile check if they have changed, but this is just too much 
	effort for now). For that reason I will not do it that way but would like to revisit 
	it later */

	if (SelectableColoringOptions::Rule == ESelectableColoringRule::NoChange)
	{
		return;
	}

	FLinearColor Color;
	if (SelectableColoringOptions::Rule == ESelectableColoringRule::Player)
	{
		Color = Statics::GetPlayerColor(PS);
	}
	else if (SelectableColoringOptions::Rule == ESelectableColoringRule::Team)
	{
		Color = Statics::GetTeamColor(Attributes.GetTeam());
	}
	else
	{
		UE_LOG(RTSLOG, Fatal, TEXT("Unimplemented selectable coloring rule"));
	}

	// Create dynamic material instances and apply color 
	const int32 NumMaterials = Mesh->GetNumMaterials();
	for (int32 i = 0; i < NumMaterials; ++i)
	{
		UMaterialInstanceDynamic * MID = UMaterialInstanceDynamic::Create(Mesh->GetMaterial(i), Mesh);
		
		MID->SetVectorParameterValue(SelectableColoringOptions::ParamName, Color);
		
		Mesh->SetMaterial(i, MID);

		/* Because don't use stealth at the moment on buildings we have no reason to actually 
		reference these dynamic materials instances again so no need to add to any array */
	}
}

void ABuilding::SetupWidgetComponents()
{	
	/* Note: not setting health here. If inital health values on widgets are incorrect then 
	will need to do something about it. */
	SelectionWidgetComponent->SetWidgetClassAndSpawn(this, FI->GetSelectableSelectionWorldWidget(), 
		Attributes, GI);
	PersistentWidgetComponent->SetWidgetClassAndSpawn(this, FI->GetSelectablePersistentWorldWidget(), 
		Attributes, GI);
}

void ABuilding::SetupBuildingAttackComponents()
{
	if (GetWorld()->IsServer())
	{
		const FCollisionObjectQueryParams QueryParams = GS->GetAllEnemiesQueryParams(Attributes.GetTeam());
		for (const auto & Elem : Attributes.AttackComponents_Turrets)
		{
			CastChecked<IBuildingAttackComp_Turret>(Elem)->ServerSetupAttackCompMore(Attributes.GetTeam(), &GS->GetTeamVisibilityInfo(Attributes.GetTeam()), QueryParams);
		}
	}
	else
	{
		for (const auto & Elem : Attributes.AttackComponents_Turrets)
		{
			CastChecked<IBuildingAttackComp_Turret>(Elem)->ClientSetupAttackCompMore(Attributes.GetTeam());
		}
	}
}

void ABuilding::SetupInitialConstructionValues(const FBuildingInfo * BuildingInfo)
{
	/* Need to determine how to start given our build method and whether a build animation should
	be used or not. Assuming already placed in correct location */

	// TODO make sure to set in build info during FI creation
	bRiseFromGround = BuildingInfo->ShouldBuildingRiseFromGround();

	if (Attributes.GetBuildMethod() == EBuildingBuildMethod::BuildsInTab
		|| Attributes.GetBuildMethod() == EBuildingBuildMethod::BuildsItself
		|| Attributes.GetBuildMethod() == EBuildingBuildMethod::Protoss)
	{
		if (bRiseFromGround)
		{
			SpawnLocation = GetActorLocation();
		}
		else
		{
			if (Attributes.IsAnimationValid(EBuildingAnimation::Construction))
			{
				PlayAnimation(EBuildingAnimation::Construction, false);
			}
		}

		if (GetWorld()->IsServer())
		{
			/* Set this so we know when the building was placed. It will rep to clients. */
			// True because building builds itself
			Server_SetBeingBuilt(true);
		}
	}
	else // A method that requires a worker building it
	{
		/* I don't really know the animation behavior if choosing to not rise from the ground, so
		really this should always be true */
		if (bRiseFromGround)
		{
			SpawnLocation = GetActorLocation();
		}
	}

	/* Set initial health and some values to use during tick */

	HealthGainRate = Attributes.GetMaxHealth() * (1.f - GI->GetBuildingStartHealthPercentage());

	/* This should always evaluate true if its a self building build method and we are the server. 
	Otherwise there is no guarantees */
	if (ConstructionProgressInfo.LastTimeWhenConstructionStarted >= 0.f)
	{
		const float BuildTime = (Attributes.GetBuildMethod() == EBuildingBuildMethod::BuildsInTab)
			? Attributes.GetConstructTime() : Attributes.GetBuildTime();
		
		TimeSpentBuilding = ConstructionProgressInfo.PercentageCompleteWhenConstructionLastStopped * BuildTime;
		TimeSpentBuilding += GS->GetServerWorldTimeSeconds() - ConstructionProgressInfo.LastTimeWhenConstructionStarted;

		// Set correct health amount
		HealthWhileUnderConstruction = Attributes.GetMaxHealth() * GI->GetBuildingStartHealthPercentage() + (HealthGainRate * (TimeSpentBuilding / BuildTime));
		HealthWhileUnderConstruction -= DamageTakenWhileUnderConstruction;

		/* Possibly want to update persistent and selection widget now that health is final */
		SelectionWidgetComponent->OnHealthChanged(HealthWhileUnderConstruction);
		PersistentWidgetComponent->OnHealthChanged(HealthWhileUnderConstruction);
	}
}

void ABuilding::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// TODO disable tick until Owner and ID rep ?

#if TICKABLE_BUFF_AND_DEBUFF_ENABLED_GAME && ALLOW_BUFFS_AND_DEBUFFS_ON_BUILDINGS
	
	/* Basically just copied and pasted this code from AInfantry. Should be fine but just keep 
	that in mind */

	const bool bIsCurrentSelected = Attributes.IsPrimarySelected();

	/* Update all the buffs applied to us */
	for (int32 i = Attributes.GetTickableBuffs().Num() - 1; i >= 0; --i)
	{
		FTickableBuffOrDebuffInstanceInfo & Elem = Attributes.GetTickableBuffs()[i];

		Elem.DecrementDeltaTime(DeltaTime);

		const FTickableBuffOrDebuffInfo * Info = Elem.GetInfoStruct();
		const float TickInterval = Info->GetTickInterval();

		/* Going to while statement here instead of just if statement so we can do multiple ticks
		for extremly fast tick rate buffs/debuffs */
		uint8 NumTicksToExecute = 0;
		while (Elem.GetTimeRemainingTillNextTick() <= 0.f)
		{
			NumTicksToExecute++;
			Elem.IncreaseTimeRemainingTillNextTick(TickInterval);
		}

		const bool bExpiresOverTime = Info->ExpiresOverTime();
		bool bUpdatedUI = false;

		for (uint8 k = 0; k < NumTicksToExecute; ++k)
		{
			Elem.IncrementTickCount();

			Info->ExecuteDoTickBehavior(this, Elem);

			/* Check if buff/debuff has 'ticked out' */
			if (bExpiresOverTime && Elem.GetTickCount() == Info->GetNumberOfTicks())
			{
				/* Will avoid RemoveAtSwap to preserve ordering since it will look strange
				if buffs/debuffs are swapping location on HUD */
				Attributes.GetTickableBuffs().RemoveAt(i, 1, false);

				Info->ExecuteOnRemovedBehavior(this, Elem, nullptr, nullptr,
					EBuffAndDebuffRemovalReason::Expired);

				/* Update HUD. For now will not update world widgets but can easily do that
				by using similar logic to HUD */
				if (Attributes.IsSelected())
				{
					PC->GetHUDWidget()->Selected_OnTickableBuffRemoved(this, i,
						EBuffAndDebuffRemovalReason::Expired, bIsCurrentSelected);
				}

				bUpdatedUI = true;

				break;
			}
		}

		/* Going to do this as we iterate loop. Could maybe batch them together in their own
		loop could see if it's faster */
		if (Attributes.IsSelected() && bExpiresOverTime && !bUpdatedUI)
		{
			PC->GetHUDWidget()->Selected_UpdateTickableBuffDuration(this, i,
				Elem.CalculateDurationRemaining(), bIsCurrentSelected);
		}
	}

	/* Do exactly the same for debuffs */
	for (int32 i = Attributes.GetTickableDebuffs().Num() - 1; i >= 0; --i)
	{
		FTickableBuffOrDebuffInstanceInfo & Elem = Attributes.GetTickableDebuffs()[i];

		Elem.DecrementDeltaTime(DeltaTime);

		const FTickableBuffOrDebuffInfo * Info = Elem.GetInfoStruct();
		const float TickInterval = Info->GetTickInterval();

		/* Going to while statement here instead of just if statement so we can do multiple ticks
		for extremly fast tick rate buffs/debuffs */
		uint8 NumTicksToExecute = 0;
		while (Elem.GetTimeRemainingTillNextTick() <= 0.f)
		{
			NumTicksToExecute++;
			Elem.IncreaseTimeRemainingTillNextTick(TickInterval);
		}

		const bool bExpiresOverTime = Info->ExpiresOverTime();
		bool bUpdatedUI = false;

		for (uint8 k = 0; k < NumTicksToExecute; ++k)
		{
			Elem.IncrementTickCount();

			Info->ExecuteDoTickBehavior(this, Elem);

			/* Check if buff/debuff has 'ticked out' */
			if (bExpiresOverTime && Elem.GetTickCount() == Info->GetNumberOfTicks())
			{
				/* Will avoid RemoveAtSwap to preserve ordering since it will look strange
				if buffs/debuffs are swapping location on HUD */
				Attributes.GetTickableDebuffs().RemoveAt(i, 1, false);

				Info->ExecuteOnRemovedBehavior(this, Elem, nullptr, nullptr,
					EBuffAndDebuffRemovalReason::Expired);

				/* Update HUD. For now will not update world widgets but can easily do that
				by using similar logic to HUD */
				if (Attributes.IsSelected())
				{
					PC->GetHUDWidget()->Selected_OnTickableDebuffRemoved(this, i,
						EBuffAndDebuffRemovalReason::Expired, bIsCurrentSelected);
				}

				bUpdatedUI = true;

				break;
			}
		}

		/* Going to do this as we iterate loop. Could maybe batch them together in their own
		loop could see if it's faster */
		if (Attributes.IsSelected() && bExpiresOverTime && !bUpdatedUI)
		{
			PC->GetHUDWidget()->Selected_UpdateTickableDebuffDuration(this, i,
				Elem.CalculateDurationRemaining(), bIsCurrentSelected);
		}
	}

#endif // TICKABLE_BUFF_AND_DEBUFF_ENABLED_GAME && ALLOW_BUFFS_AND_DEBUFFS_ON_BUILDINGS

	if (IsBeingBuilt())
	{
		ProgressConstruction(DeltaTime);
	}
	else if (bSinkingIntoGround_Logically)
	{
		ProgressSinkingIntoGround(DeltaTime);
	}

	/* Also TODO: make Dfense comps hidden if in fog when spawned */
}

void ABuilding::ProgressSinkingIntoGround(float DeltaTime)
{
	TimeSpentSinking += DeltaTime;
	if (TimeSpentSinking >= TimeToSpendSinking)
	{
		OnSinkingIntoGroundComplete();

		//bSinkingIntoGround = false;
	}
	else
	{
		// Only update appearance if player knows the building is sinking into ground
		if (bSinkingIntoGround_Visually)
		{
			AddActorLocalOffset(FVector(0.f, 0.f, -DeltaTime * BoundsHeight * (1.f / TimeToSpendSinking)));
		}
	}
}

void ABuilding::OnRep_RallyPointLocation()
{
	// Move mesh to new location
	UnitRallyPoint->SetWorldLocation(RallyPointLocation);
}

void ABuilding::OnRep_ConstructionProgressInfo()
{
	/* If it's impossible for this to not throw then we may just need to make this func just 
	return if this is a starting selectable */
	assert(Attributes.IsAStartingSelectable() == false);
	
	/* Check if hasn't setup yet */
	if (Attributes.GetAffiliation() == EAffiliation::Unknown)
	{
		return;
	}
	
	if (IsConstructionComplete())
	{
		return;
	}

	if (ConstructionProgressInfo.LastTimeWhenConstructionStarted < 0.f)
	{
		/* Building not being worked on */

		/* We do not have to differentiate between Attributes.GetConstructTime() and GetBuildTime() 
		because the auto building build methods never stop building */
		TimeSpentBuilding = ConstructionProgressInfo.PercentageCompleteWhenConstructionLastStopped * Attributes.GetBuildTime();

		/* Work out health. This may not be accurate enough. May need to end up making
		DamageTakenWhileUnderConstruction replicated while this is constructing */
		const float BaseHealth = GI->GetBuildingStartHealthPercentage() * Attributes.GetMaxHealth();
		HealthWhileUnderConstruction = BaseHealth + (TimeSpentBuilding * HealthGainRate) - DamageTakenWhileUnderConstruction;

		if (bIsBeingBuiltParticlesRunning)
		{
			StopBeingConstructedParticles();
			StopBeingConstructedSound();
		}
	}
	else
	{
		// Building is being worked on

		/* This can actually be called by self building build methods so need to use the correct 
		build time */
		const float BuildTime = (Attributes.GetBuildMethod() == EBuildingBuildMethod::BuildsInTab)
			? Attributes.GetConstructTime() : Attributes.GetBuildTime();

		/* Update TimeSpentBuilding using the replicated variables */
		TimeSpentBuilding = ConstructionProgressInfo.PercentageCompleteWhenConstructionLastStopped * BuildTime;
		TimeSpentBuilding += GS->GetServerWorldTimeSeconds() - ConstructionProgressInfo.LastTimeWhenConstructionStarted;

		// Make at least 0. Replicated server time not precise (as expected). Will have to think of what to do about this 
		TimeSpentBuilding = FMath::Max(0.f, TimeSpentBuilding);

		/* Work out health. This may not be accurate enough. May need to end up making 
		DamageTakenWhileUnderConstruction replicated while this is constructing */
		const float BaseHealth = GI->GetBuildingStartHealthPercentage() * Attributes.GetMaxHealth();
		HealthWhileUnderConstruction = BaseHealth + (TimeSpentBuilding * HealthGainRate) - DamageTakenWhileUnderConstruction;

		const float PercentageComplete = TimeSpentBuilding * (1.f / BuildTime);

		if (bRiseFromGround)
		{
			/* Set right height */
			AddActorLocalOffset(FVector(0.f, 0.f, PercentageComplete * BoundsHeight));
		}
		else
		{
			// Set animation time? If anim state it replicated then we might not need to
		}

		/* Tell widgets about health and construction progress. Or could just let the next tick do 
		it. */
	
		if (!bIsBeingBuiltParticlesRunning)
		{
			PlayBeingConstructedParticles();
			PlayBeingConstructedSound(0.f); // Perhaps we want a random start time instead of 0?
		}
	}
}

void ABuilding::ProgressConstruction(float DeltaTime)
{
	assert(Attributes.IsAStartingSelectable() == false); // I think this can be here
	assert(IsConstructionComplete() == false);
	assert(IsBeingBuilt() == true);

	TimeSpentBuilding += DeltaTime;
		
	/* Attributes.GetBuildMethod() will return the proper value taking into account the 
	faction's default build method too */
	const float BuildTime = (Attributes.GetBuildMethod() == EBuildingBuildMethod::BuildsInTab)
		? Attributes.GetConstructTime() : Attributes.GetBuildTime();

	if (TimeSpentBuilding >= BuildTime)
	{
		if (GetWorld()->IsServer())
		{
			Server_OnConstructionComplete();
		}
		else
		{
			/* We have completed construction on our side but the server has not sent the 
			replicated variable to confirm this so just max out the UI to 100% construction 
			complete while we wait */
			
			/* Check if we haven't already done all this */
			if (GetActorLocation() != GetFinalLocation())
			{
				/* Set location to final location */
				SetActorLocation(GetFinalLocation());

				/* Set health. Set construction progress to 100% */
				SelectionWidgetComponent->OnHealthChanged(Attributes.GetMaxHealth() - DamageTakenWhileUnderConstruction);
				SelectionWidgetComponent->OnConstructionProgressChanged(1.f);
				PersistentWidgetComponent->OnHealthChanged(Attributes.GetMaxHealth() - DamageTakenWhileUnderConstruction);
				PersistentWidgetComponent->OnConstructionProgressChanged(1.f);
			}
		}
	}
	else
	{
		if (bRiseFromGround)
		{
			/* Move upwards */
			AddActorLocalOffset(FVector(0.f, 0.f, DeltaTime * BoundsHeight * (1.f / BuildTime)));
		}

		/* Increase health */
		HealthWhileUnderConstruction += DeltaTime * HealthGainRate * (1.f / BuildTime);

		const float PercentageComplete = TimeSpentBuilding * (1.f / BuildTime);

		// Tell widget about health change and construction progress
		SelectionWidgetComponent->OnHealthChanged(HealthWhileUnderConstruction);
		SelectionWidgetComponent->OnConstructionProgressChanged(PercentageComplete);

		// Tell widget about health change and construction progress
		PersistentWidgetComponent->OnHealthChanged(HealthWhileUnderConstruction);
		PersistentWidgetComponent->OnConstructionProgressChanged(PercentageComplete);

		if (Attributes.IsSelected())
		{
			/* Tell HUD about health change */
			PC->GetHUDWidget()->Selected_OnHealthChanged(this, HealthWhileUnderConstruction,
				Attributes.GetMaxHealth(), true);
		}
	}
}

void ABuilding::Server_OnConstructionComplete()
{
	SERVER_CHECK;
	
	/* TODO make sure anim notifies on construct animations call this when they finish */
	
	if (IsStartingSelectable())
	{
		SetActorLocation(GetActorLocation() + FVector(0.f, 0.f, BoundsHeight / 2));
	}
	else
	{
		/* Set location in case off by a bit from constantly adding floats in tick */
		SetActorLocation(GetFinalLocation());
	}

	/* Adjust health because of tick/floats it can be off slighty when construction is complete.
	Let it rep to client. */
	Health = Attributes.GetMaxHealth() - DamageTakenWhileUnderConstruction;
	
	SelectionWidgetComponent->OnConstructionComplete(Health);
	PersistentWidgetComponent->OnConstructionComplete(Health);

	if (Attributes.IsSelected())
	{
		PC->GetHUDWidget()->Selected_OnHealthChanged(this, Health, Attributes.GetMaxHealth(), true);
	}

	GS->OnBuildingConstructionCompleted(this, Attributes.GetTeam(), true);
	PS->OnBuildingBuilt(this, Type, CreationMethod);

	/* Setting this variable is basically how we signal that construction is complete.
	It will rep to clients */
	GameTickCountOnConstructionComplete = GS->GetGameTickCounter();
		
	/* Set this to stop trying to progress construction in tick and it also represents that 
	the building is not being worked on. I don't know if it will rep to clients because 
	replication of this variable is stopped when construction completes. Clients will set 
	it to -1 though when GameTickCountOnConstructionComplete reps */
	ConstructionProgressInfo.LastTimeWhenConstructionStarted = -1.f;

	ActivateAttackComponents();

	if (Attributes.IsAnimationValid(EBuildingAnimation::Idle))
	{
		PlayAnimation(EBuildingAnimation::Idle, false);
	}

	/* Notify any builders working on this that construction has completed */
	for (const auto & Elem : Builders)
	{
		if (Statics::IsValid(Elem))
		{
			ISelectable * AsSelectable = CastChecked<ISelectable>(Elem);
			AsSelectable->OnWorkedOnBuildingConstructionComplete();
		}
	}
	Builders.Reset();

	// Check if they're actually running first if it's more efficient?
	StopBeingConstructedParticles();
	StopBeingConstructedSound();
	
	/* If selected then make sure rally point is now visible if it wasn't previously */
	if (Attributes.IsSelected())
	{
		UnitRallyPoint->SetVisibility(true);
	}
}

void ABuilding::Client_SetAppearanceForConstructionComplete()
{
	CLIENT_CHECK;
	
	// No reason to call this if not a starting selectable
	assert(IsStartingSelectable() && !Attributes.IsSelected());

	SetActorLocation(GetActorLocation() + FVector(0.f, 0.f, BoundsHeight / 2));

	// To hide the construction progress bar
	SelectionWidgetComponent->OnConstructionComplete();
	PersistentWidgetComponent->OnConstructionComplete();
}

void ABuilding::Client_FinalizeConstructionComplete()
{
	CLIENT_CHECK;
	
	// No reason to call this if not a starting selectable
	assert(IsStartingSelectable());
	
	GS->OnBuildingConstructionCompleted(this, Attributes.GetTeam(), false);
	PS->OnBuildingBuilt(this, Type, CreationMethod);

	/* If selected then make sure rally point is now visible if it wasn't previously */
	if (Attributes.IsSelected())
	{
		UnitRallyPoint->SetVisibility(true);
	}
}

void ABuilding::Client_OnConstructionComplete()
{
	CLIENT_CHECK;

	if (IsStartingSelectable())
	{
		SetActorLocation(GetActorLocation()/* + FVector(0.f, 0.f, BoundsHeight / 2)*/);
	}
	else
	{
		SetActorLocation(GetFinalLocation());

		/* If selected then make sure rally point is now visible if it wasn't previously */
		if (Attributes.IsSelected())
		{
			UnitRallyPoint->SetVisibility(true);
		}
	}

	// To hide the construction progress bar
	SelectionWidgetComponent->OnConstructionComplete();
	PersistentWidgetComponent->OnConstructionComplete();

	GS->OnBuildingConstructionCompleted(this, Attributes.GetTeam(), false);
	PS->OnBuildingBuilt(this, Type, CreationMethod);

	// Could be more efficient without the if statement maybe
	if (bIsBeingBuiltParticlesRunning)
	{
		StopBeingConstructedParticles();
		StopBeingConstructedSound();
	}

	bHasRunOnConstructionCompleteLogic = true;
}

void ABuilding::SetInitialBuildingVisibility()
{
	const bool bInitialVisibility = false;

	Bounds->SetVisibility(bInitialVisibility);
	Mesh->SetVisibility(bInitialVisibility);
	PersistentWidgetComponent->SetVisibility(bInitialVisibility);
	SelectionWidgetComponent->SetVisibility(bInitialVisibility);
	UnitRallyPoint->SetVisibility(bInitialVisibility);
	SelectionDecal->SetVisibility(bInitialVisibility);
	for (const auto & AttackComp : Attributes.AttackComponents_Turrets)
	{
		AttackComp->SetVisibility(bInitialVisibility);
	}
	for (const auto & AttackComp : Attributes.AttackComponents_TurretsBases)
	{
		AttackComp->SetVisibility(bInitialVisibility);
	}
}

void ABuilding::SetInitialBuildingVisibilityV2()
{
	const bool bInitialVisibility = false;

	Bounds->SetVisibility(bInitialVisibility);
	Mesh->SetVisibility(bInitialVisibility);
	PersistentWidgetComponent->SetVisibility(bInitialVisibility);
	SelectionWidgetComponent->SetVisibility(bInitialVisibility);
	UnitRallyPoint->SetVisibility(bInitialVisibility);
	SelectionDecal->SetVisibility(bInitialVisibility);
	/* Hide the attack components. This just assumes every blueprint created component is 
	a building attack component */
	for (const auto & Elem : BlueprintCreatedComponents)
	{
		CastChecked<UMeshComponent>(Elem)->SetVisibility(false);
	}
}

void ABuilding::SetBuildingVisibility(bool bNewVisibility)
{
	// Currently this is only called during Setup(). Assuming wasn't selectable
	assert(Statics::CanBeSelected(this) == false);

	Bounds->SetVisibility(bNewVisibility);
	Mesh->SetVisibility(bNewVisibility);
	PersistentWidgetComponent->SetVisibility(bNewVisibility);
	for (const auto & AttackComp : Attributes.AttackComponents_Turrets)
	{
		AttackComp->SetVisibility(bNewVisibility);
	}
	for (const auto & AttackComp : Attributes.AttackComponents_TurretsBases)
	{
		AttackComp->SetVisibility(bNewVisibility);
	}
}

void ABuilding::SetBuildingVisibilityV2(bool bNewVisibility)
{
	// Currently this is only called during Setup(). Assuming wasn't selectable
	assert(Statics::CanBeSelected(this) == false);

	Bounds->SetVisibility(bNewVisibility);
	Mesh->SetVisibility(bNewVisibility);
	PersistentWidgetComponent->SetVisibility(bNewVisibility);
	/* Hide the attack components. This just assumes every blueprint created component is
	a building attack component */
	for (const auto & Elem : BlueprintCreatedComponents)
	{
		CastChecked<UMeshComponent>(Elem)->SetVisibility(bNewVisibility);
	}
}

void ABuilding::HideBuilding()
{
	Mesh->SetVisibility(false);
	PersistentWidgetComponent->SetVisibility(false);
	if (UnitRallyPoint != nullptr)
	{
		UnitRallyPoint->SetVisibility(false);
	}
	for (const auto & AttackComp : Attributes.AttackComponents_Turrets)
	{
		AttackComp->SetVisibility(false); 
	}
	for (const auto & AttackComp : Attributes.AttackComponents_TurretsBases)
	{
		AttackComp->SetVisibility(false);
	}
}

void ABuilding::OnEnterFogOfWar()
{
	if (Attributes.IsSelected())
	{
		OnDeselect();

		/* Tell HUD. Because assuming only one building selected at a time this func is correct
		to call */
		PC->GetHUDWidget()->OnPlayerNoSelection();
	}

	// Make unclickable
	Bounds->SetVisibility(false);
	PersistentWidgetComponent->SetVisibility(false);
	// Do not change mesh visibility

	/* If defense components get their movement predicted client-side then might have to
	force them to stop them rotating on clients otherwise they can continuously rotate while
	in fog due to client-side prediction. A bool tht turns off client-side prediction would
	be nice. Look for that first when implementing this */

	// Pause animation for Mesh. Note this doesn't do the attack components because they are handled below
	PauseAnimation();

	const bool bOnServer = GetWorld()->IsServer();
	for (const auto & AttackComp : Attributes.AttackComponents_Turrets)
	{
		/* This function should pause anims if it's an SK comp and we're not on server. Otherwise 
		it'll probably be a NOOP for everything else including the SM version.
		Calling a virtual in a loop here, but it's expected that these arrays will be small 
		and that maybe they're all the same class anyway. But maybe want to do something about this. 
		Same deal for the loop of the bases below too */
		CastChecked<IBuildingAttackComp_Turret>(AttackComp)->OnParentBuildingEnterFogOfWar(bOnServer);
	}
	for (const auto & AttackComp : Attributes.AttackComponents_TurretsBases)
	{
		CastChecked<IBuildingAttackComp_TurretsBase>(AttackComp)->OnParentBuildingEnterFogOfWar(bOnServer);
	}
}

void ABuilding::OnExitFogOfWar()
{
	/* Check if the building has been completely destroyed */
	if (HasBeenCompletelyDestroyed())
	{
		PS->RemoveBuildingFromBuildingsContainer(this);

		/* Server shouldn't destroy the building yet. From my testing calling TearOff followed
		by Destroy in the same frame or even with a small 0.01 delay between what will
		happen is the destroy will actually happen on clients too which is what we don't
		want. We know it has called TearOff() but we don't know how long ago it was. 
		So delay Destroy call by 5 seconds */
		if (OnServer())
		{
			// Make building hidden. What about collision too?
			HideBuilding();
			if (GetWorldTimerManager().IsTimerActive(TimerHandle_Destroy) == false)
			{
				Delay(TimerHandle_Destroy, &ABuilding::DestroyBuilding, 5.f);
			}
		}
		else
		{
			OnZeroHealth(EOnZeroHealthCallSite::Other);
			ClientCleanUp(true);
		}

		return;
	}

	UnpauseAnimation();

	const bool bOnServer = GetWorld()->IsServer();
	if (bHasLocalPlayerSeenBuildingAtLeastOnce == false)
	{
		bHasLocalPlayerSeenBuildingAtLeastOnce = true;

		// Make mesh visible
		Mesh->SetVisibility(true);

		for (const auto & AttackComp : Attributes.AttackComponents_Turrets)
		{
			CastChecked<IBuildingAttackComp_Turret>(AttackComp)->OnParentBuildingExitFogOfWar(bOnServer);
			AttackComp->SetVisibility(true);
		}
		for (const auto & AttackComp : Attributes.AttackComponents_TurretsBases)
		{
			CastChecked<IBuildingAttackComp_TurretsBase>(AttackComp)->OnParentBuildingExitFogOfWar();
			AttackComp->SetVisibility(true);
		}
	}
	else
	{
		for (const auto & AttackComp : Attributes.AttackComponents_Turrets)
		{
			CastChecked<IBuildingAttackComp_Turret>(AttackComp)->OnParentBuildingExitFogOfWar(bOnServer);
		}
		for (const auto & AttackComp : Attributes.AttackComponents_TurretsBases)
		{
			CastChecked<IBuildingAttackComp_TurretsBase>(AttackComp)->OnParentBuildingExitFogOfWar();
		}
	}

	PersistentWidgetComponent->SetVisibility(true);

	/* Defense components need something done to them maybe? Dunno */

	// Make clickable
	Bounds->SetVisibility(true);
}

void ABuilding::PlayJustPlacedParticles()
{
	const FParticleInfo & ParticleInfo = Attributes.GetPlacedParticles();
	
	if (ParticleInfo.GetTemplate() != nullptr)
	{
		Statics::SpawnFogParticles(ParticleInfo.GetTemplate(), GS, 
			GetFinalLocation() + ParticleInfo.GetTransform().GetLocation(),
			GetActorRotation() + ParticleInfo.GetTransform().GetRotation().Rotator(), 
			ParticleInfo.GetTransform().GetScale3D());
	}
}

void ABuilding::PlayJustPlacedSound()
{
	Statics::SpawnSoundAtLocation(GS, Attributes.GetTeam(), Attributes.GetJustPlacedSound(), 
		GetActorLocation(), ESoundFogRules::Dynamic);
}

void ABuilding::InitProductionQueues()
{
	/* Capacities should have already been set OnPostEdit but setting them again here in case
	a vanilla unedited BP is made */

	// if statement possibly optional - can do it always if you want
	if (Attributes.GetAffiliation() == EAffiliation::Owned || GetWorld()->IsServer())
	{
		if (PS->bIsABot)
		{
			Attributes.ProductionQueue.SetupForCPUOwner(EProductionQueueType::Context,
				Attributes.GetProductionQueueLimit());

			Attributes.PersistentProductionQueue.SetupForCPUOwner(EProductionQueueType::Persistent, 
				Attributes.GetNumPersistentBuildSlots());
		}
		else
		{
			Attributes.ProductionQueue.SetupForHumanOwner(EProductionQueueType::Context,
				Attributes.GetProductionQueueLimit());

			Attributes.PersistentProductionQueue.SetupForHumanOwner(EProductionQueueType::Persistent,
				Attributes.GetNumPersistentBuildSlots());
		}
	}
}

void ABuilding::Server_SetBeingBuilt(bool bBeingBuilt)
{	
	SERVER_CHECK;
	
	// Assert this is a toggle
	assert(IsBeingBuilt() != bBeingBuilt);
	
	if (bBeingBuilt)
	{
		ConstructionProgressInfo.LastTimeWhenConstructionStarted = GS->GetServerWorldTimeSeconds();
	}
	else
	{
		ConstructionProgressInfo.PercentageCompleteWhenConstructionLastStopped = TimeSpentBuilding / Attributes.GetBuildTime();

		// Negative value basically means it's not being worked on
		ConstructionProgressInfo.LastTimeWhenConstructionStarted = -1.f;
	}

	if (bBeingBuilt)
	{
		PlayBeingConstructedParticles();
		PlayBeingConstructedSound(0.f); // Perhaps we want a random start time instead of 0? 
	}
	else
	{
		StopBeingConstructedParticles();
		StopBeingConstructedSound();
	}
}

void ABuilding::PlayBeingConstructedParticles()
{
	const FParticleInfo & ParticleInfo = Attributes.GetAdvancingConstructionParticles();
	
	if (ParticleInfo.GetTemplate() != nullptr)
	{
		BeingBuiltParticleComponent = Statics::SpawnFogParticles(ParticleInfo.GetTemplate(), GS, 
			GetFinalLocation() + ParticleInfo.GetTransform().GetLocation(), 
			GetActorRotation() + ParticleInfo.GetTransform().GetRotation().Rotator(), 
			ParticleInfo.GetTransform().GetScale3D());

		bIsBeingBuiltParticlesRunning = true;
	}
}

void ABuilding::StopBeingConstructedParticles()
{
	if (BeingBuiltParticleComponent.IsValid())
	{
		BeingBuiltParticleComponent->DeactivateSystem();

		bIsBeingBuiltParticlesRunning = false;
	}
}

void ABuilding::PlayBeingConstructedSound(float AudioStartTime)
{
	if (Attributes.GetBeingWorkedOnSound() != nullptr)
	{
		// Check if need to spawn audio comp or if already spawned
		if (BeingWorkedOnAudioComp.IsValid() == false)
		{
			// Save reference to audio comp since we need to stop it later on
			BeingWorkedOnAudioComp = Statics::SpawnSoundAtLocation(GS, Attributes.GetTeam(),
				Attributes.GetBeingWorkedOnSound(), GetActorLocation(), ESoundFogRules::DynamicExceptForInstigatorsTeam,
				FRotator::ZeroRotator, 1.f, 1.f, AudioStartTime);
		}
		else
		{
			BeingWorkedOnAudioComp->PlaySound(GS, Attributes.GetBeingWorkedOnSound(), AudioStartTime,
				Attributes.GetTeam(), ESoundFogRules::DynamicExceptForInstigatorsTeam);
		}
	}
}

void ABuilding::StopBeingConstructedSound()
{
	if (BeingWorkedOnAudioComp.IsValid())
	{
		BeingWorkedOnAudioComp->Stop();
	}
}

void ABuilding::ActivateAttackComponents()
{
	SERVER_CHECK;

	UHeavyTaskManager * TaskManager = GI->GetHeavyTaskManager();
	for (UMeshComponent * AttackComp : Attributes.AttackComponents_Turrets)
	{
		IBuildingAttackComp_Turret * Turret = CastChecked<IBuildingAttackComp_Turret>(AttackComp);
		TaskManager->RegisterBuildingAttackComponent(Turret);
	}
}

void ABuilding::PlayAnimation(EBuildingAnimation AnimType, bool bAllowReplaySameType)
{
	/* Assuming here that the open door anim will complete before it needs to open again */

	if (GetWorld()->IsServer() == false)
	{
		return;
	}

	UE_CLOG(AnimType == EBuildingAnimation::None, RTSLOG, Fatal, TEXT("Building [%s] is trying to "
		"to play \"None\" animation. "), *GetName());

	UAnimMontage * Montage = Attributes.GetAnimation(AnimType);
	assert(Montage != nullptr); // Not 100% required

	if (bAllowReplaySameType)
	{
		if (AreAnimationsPausedLocally() == false)
		{
			assert(PausedAnimInfo.AnimationType == EBuildingAnimation::None);
			
			GetMesh()->GetAnimInstance()->Montage_Play(Montage);
		}
		else
		{
			/* Do not play anim, but note down the time now so we can resume playing it later if needed */
			PausedAnimInfo.AnimationType = AnimType;
			PausedAnimInfo.StartTime = GS->GetServerWorldTimeSeconds();
		}

		/* Regardless of whether we are locally visible or not we need to set these so they rep 
		to clients */
		AnimationState.AnimationType = AnimType;
		AnimationState.StartTime = GS->GetServerWorldTimeSeconds();
	}
	else
	{
		/* Only play montage if it is different from the one currently playing */
		if (AnimationState.AnimationType != AnimType)
		{
			if (AreAnimationsPausedLocally() == false)
			{
				assert(PausedAnimInfo.AnimationType == EBuildingAnimation::None);
				
				GetMesh()->GetAnimInstance()->Montage_Play(Montage);
			}
			else
			{
				/* Do not play anim, but note down the time now so we can resume playing it later if needed */
				PausedAnimInfo.AnimationType = AnimType;
				PausedAnimInfo.StartTime = GS->GetServerWorldTimeSeconds();
			}

			/* Regardless of whether we are locally visible or not we need to set these so they rep
			to clients */
			AnimationState.AnimationType = AnimType;
			AnimationState.StartTime = GS->GetServerWorldTimeSeconds();
		}
	}
}

void ABuilding::PauseAnimation()
{
	/* Note down where the animation was at so we can resume it at the right location later */
	PausedAnimInfo.AnimationType = AnimationState.AnimationType;
	PausedAnimInfo.StartTime = AnimationState.StartTime;
		
	// Stop animation. Not sure if setting both of these are required but will do both for good measure
	Mesh->bNoSkeletonUpdate = true;
	Mesh->bPauseAnims = true;
}

void ABuilding::UnpauseAnimation()
{
	if (PausedAnimInfo.AnimationType != EBuildingAnimation::None)
	{
		assert(AnimationState.AnimationType == PausedAnimInfo.AnimationType);
		assert(AnimationState.StartTime == PausedAnimInfo.StartTime);
		
		// Destroyed anim is a special case
		if (PausedAnimInfo.AnimationType == EBuildingAnimation::Destroyed)
		{
			// Given Destroyed anim it is implied OnZeroHealth has already run
			assert(Statics::HasZeroHealth(this));

			// Might need to change this to an if (bHasRunCleanUp) return;
			assert(!bHasRunCleanUp);

			bHasLocalPlayerSeenBuildingAtZeroHealth = true;

			const float TimeSinceBuildingReachedZeroHealth = GS->GetServerWorldTimeSeconds() - PausedAnimInfo.StartTime;

			// Check if the Destroyed anim has progressed to the sinking into ground portion
			if (bSinkingIntoGround_Logically)
			{
				if (!bSinkingIntoGround_Visually)
				{
					TimeSpentSinking = TimeSinceBuildingReachedZeroHealth - TimeIntoZeroHealthAnimThatAnimNotifyIs;

					// Sink building based on progress time so far
					const FVector Loc = GetActorLocation();
					SetActorLocation(FVector(Loc.X, Loc.Y, GetActorLocation().Z - (TimeSpentSinking * BoundsHeight * (1.f / TimeToSpendSinking))));

					bSinkingIntoGround_Visually = true;
				}
				
				return;
			}

			/* Check if enough time has passed to trigger the anim notify OnZeroHealthAnimationFinished */
			if (TimeSinceBuildingReachedZeroHealth >= TimeIntoZeroHealthAnimThatAnimNotifyIs)
			{
				UAnimMontage * DestroyedMontage = Attributes.GetAnimation(EBuildingAnimation::Destroyed);
				assert(DestroyedMontage != nullptr);

				/* Set animation montage to the Destroyed montage.
				Set start time to be where the anim notify is. Anim is frozen here later. It could be
				off a bit from where it would freeze if the player was actually witnessing the
				destroyed anim outside fog. 
				I don't think this code works though. Need to find a way to freeze anim. 
				Will need to do the same with similar code in OnRep_AnimationState */
				Mesh->GetAnimInstance()->Montage_Play(DestroyedMontage, 1.f, EMontagePlayReturnType::MontageLength,
					TimeIntoZeroHealthAnimThatAnimNotifyIs);
				
				// My attempt at making skeletal mesh update to the anim I just set above
				//Mesh->TickComponent(0.1f, LEVELTICK_All, nullptr);
				//Mesh->TickPose(0.1f, false);
				//Mesh->TickAnimation(0.1f, false);

				RunOnZeroHealthAnimFinishedLogic();
			}
			else
			{
				const float TimeTillAnimNotify = TimeIntoZeroHealthAnimThatAnimNotifyIs - TimeSinceBuildingReachedZeroHealth;
				Delay(&ABuilding::OnZeroHealthAnimFinished, TimeTillAnimNotify);

				Mesh->bNoSkeletonUpdate = false;
				Mesh->bPauseAnims = false;

				UAnimMontage * Montage = Attributes.GetAnimation(PausedAnimInfo.AnimationType);
				const float MontageLength = Montage->SequenceLength;

				const float StartTime = FMath::Fmod(GS->GetServerWorldTimeSeconds() - PausedAnimInfo.StartTime, MontageLength);
				Mesh->GetAnimInstance()->Montage_Play(Montage, 1.f, EMontagePlayReturnType::Duration, StartTime);

				PausedAnimInfo.AnimationType = EBuildingAnimation::None;
			}
		}
		// Special case for sinking into ground
		else if (PausedAnimInfo.AnimationType == EBuildingAnimation::SinkIntoGround)
		{
			// Given SinkIntoGround anim it is implied OnZeroHealth has already run
			assert(Statics::HasZeroHealth(this));

			// The logic that follows has already been run if either of these are true
			if (bHasLocalPlayerSeenBuildingAtZeroHealth)
			{
				return;
			}

			bHasLocalPlayerSeenBuildingAtZeroHealth = true;

			if (bSinkingIntoGround_Logically)
			{
				if (!bSinkingIntoGround_Visually)
				{
					TimeSpentSinking = GS->GetServerWorldTimeSeconds() - PausedAnimInfo.StartTime;

					// Sink building based on progress time so far
					const FVector Loc = GetActorLocation();
					SetActorLocation(FVector(Loc.X, Loc.Y, GetActorLocation().Z - (TimeSpentSinking * BoundsHeight * (1.f / TimeToSpendSinking))));

					bSinkingIntoGround_Visually = true;
				}
				
				return;
			}
			else
			{
				// Resume from where it should be
				TimeSpentSinking = GS->GetServerWorldTimeSeconds() - PausedAnimInfo.StartTime;
				AddActorLocalOffset(FVector(0.f, 0.f, -TimeSpentSinking * BoundsHeight * (1.f / TimeToSpendSinking)));
				BeginSinkingIntoGround();
			}
		}
		else
		{
			Mesh->bNoSkeletonUpdate = false;
			Mesh->bPauseAnims = false;
			
			UAnimMontage * Montage = Attributes.GetAnimation(PausedAnimInfo.AnimationType);
			const float MontageLength = Montage->SequenceLength;

			const float StartTime = FMath::Fmod(GS->GetServerWorldTimeSeconds() - PausedAnimInfo.StartTime, MontageLength);
			// With blending this doesn't look good
			Mesh->GetAnimInstance()->Montage_Play(Montage, 1.f, EMontagePlayReturnType::Duration, StartTime);

			PausedAnimInfo.AnimationType = EBuildingAnimation::None;
		}
	}
}

void ABuilding::DoNothing()
{
}

bool ABuilding::IsSelectableSelected() const
{
	return Attributes.IsSelected();
}

void ABuilding::ShowSelectionDecal() const
{
	if (Attributes.GetSelectionDecalSetup() == ESelectionDecalSetup::UsesDynamicMaterial)
	{
		/* Set param value back to original */
		const FSelectionDecalInfo & DecalInfo = FI->GetSelectionDecalInfo(Attributes.GetAffiliation());
		if (true/*DecalInfo.IsParamNameValid()*/)
		{
			Attributes.GetSelectionDecalMID()->SetScalarParameterValue(DecalInfo.GetParamName(),
				DecalInfo.GetOriginalParamValue());
		}

		SelectionDecal->SetVisibility(true);
	}
	else if (Attributes.GetSelectionDecalSetup() == ESelectionDecalSetup::UsesNonDynamicMaterial)
	{
		SelectionDecal->SetVisibility(true);
	}
}

void ABuilding::ShowHoverDecal()
{
	if (Attributes.GetSelectionDecalSetup() == ESelectionDecalSetup::UsesDynamicMaterial)
	{
		const FSelectionDecalInfo & DecalInfo = FI->GetSelectionDecalInfo(Attributes.GetAffiliation());
		if (true/*DecalInfo.IsParamNameValid()*/)
		{
			Attributes.GetSelectionDecalMID()->SetScalarParameterValue(DecalInfo.GetParamName(),
				DecalInfo.GetMouseoverParamValue());
		}

		SelectionDecal->SetVisibility(true);
	}
	else if (Attributes.GetSelectionDecalSetup() == ESelectionDecalSetup::UsesNonDynamicMaterial)
	{
		SelectionDecal->SetVisibility(true);
	}
}

void ABuilding::HideSelectionDecal() const
{
	if (Attributes.GetSelectionDecalSetup() == ESelectionDecalSetup::UsesNonDynamicMaterial 
		|| Attributes.GetSelectionDecalSetup() == ESelectionDecalSetup::UsesDynamicMaterial)
	{
		SelectionDecal->SetVisibility(false);
	}
}

bool ABuilding::IsStartingSelectable() const
{
	assert(CreationMethod != ESelectableCreationMethod::Uninitialized);
	assert(CreationMethod == Attributes.GetCreationMethod());

	return (CreationMethod == ESelectableCreationMethod::StartingSelectable);
}

bool ABuilding::IsFullyVisibleLocally() const
{
	return Attributes.GetVisionState() == EFogStatus::Revealed 
		|| Attributes.GetVisionState() == EFogStatus::StealthRevealed;
}

bool ABuilding::OnServer() const
{
	return GetWorld()->IsServer();
}

bool ABuilding::OnClient() const
{
	return !OnServer();
}

bool ABuilding::AreAnimationsPausedLocally() const
{
	/* If I ever want to play animations even while the building is inside fog something like 
	return Mesh->bNoSkeletonUpdate == true; could be used */
	return IsFullyVisibleLocally() == false;
}

void ABuilding::PreReplication(IRepChangedPropertyTracker & ChangedPropertyTracker)
{
	Super::PreReplication(ChangedPropertyTracker);

	/* The docs mention that you should not toggle this condition often. I wonder what the 
	performance impact is for toggling this variable. 
	
	If the build method is also one of the ones 
	that autobuilds the building then we can even not replicate half of this struct 
	(the PercentageCompleteWhenConstructionLastStopped part) but since I need both variables 
	to always arive at the same time that is not an option. */
	DOREPLIFETIME_ACTIVE_OVERRIDE(ABuilding, ConstructionProgressInfo, IsConstructionComplete() == false);
}

void ABuilding::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// @See ARangedInfantry for why this is done
#if WITH_EDITOR
	DOREPLIFETIME(ABuilding, ID);
#else
	DOREPLIFETIME_CONDITION(ABuilding, ID, COND_InitialOnly);
#endif

	DOREPLIFETIME_CONDITION(ABuilding, CreationMethod, COND_InitialOnly);
	// TODO Probably want a building base class that cannot produce units so no unnecessary cost of using this
	DOREPLIFETIME_CONDITION(ABuilding, RallyPointLocation, COND_OwnerOnly);
	DOREPLIFETIME(ABuilding, Health);
	DOREPLIFETIME(ABuilding, AnimationState);
	DOREPLIFETIME_CONDITION(ABuilding, ConstructionProgressInfo, COND_Custom);
	
	/* This will only need to be sent when production completes. I don't know what guarantees 
	UE has with toggling replication and sending changes. If I shut off replication of this 
	variable will UE try to send any final changes before or will it just totally not care 
	if at that time of toggle clients do not have the most up to date values? If the latter 
	then a "solution" is to turn off replication after a little while say like 15secs, and 
	we assume that the change makes it to clients in that time. 
	So bottom line is: we could set this to COND_Custom and turn it aff after about 15 secs 
	after production completes (make it random between 15 - 25 sec. Docs state toggling 
	the condition variable is not good. We could potentially spawn a lot of buildings at the 
	start of the match and do not want them all toggling this variable's replication off on the 
	same frame). */
	DOREPLIFETIME(ABuilding, GameTickCountOnConstructionComplete);
}

void ABuilding::Delay(FTimerHandle & TimerHandle, void(ABuilding::* Function)(), float Delay)
{
	assert(Delay > 0.f);
	GetWorldTimerManager().SetTimer(TimerHandle, this, Function, Delay, false);
}

void ABuilding::Delay(void(ABuilding::* Function)(), float DelayAmount)
{
	FTimerHandle TimerHandle_Dummy;
	Delay(TimerHandle_Dummy, Function, DelayAmount);
}

#if WITH_EDITOR
void ABuilding::PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	Attributes.OnPostEdit(this); 

	// Fill ContextActionCooldowns
	ContextActionCooldowns.Reset(); // Reset() cause Empty() is broken in post edit
	for (const auto & Button : Attributes.GetContextMenu().GetButtonsArray())
	{
		ContextActionCooldowns.Emplace(Button, FTimerHandle());
	}

	bCanEditHumanOwnerIndex = !PIE_bIsForCPUPlayer;
	
	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(ABuilding, Bounds))
	{
		/* If editing scale not extent then I have run into times where fog locations were
		incorrect and needed to edit extent by say 0.01 then change it back to get post edit
		to fire. Could be more times where this is an issue */

		/* Fill the fog locations array. Avoids having to do this when building is built. Still
		have to add on spawn location + account for rotation when spawned though */

		const FVector Extent = Bounds->GetScaledBoxExtent();

		//-----------------------------------------------------------------------
		//	Quick detour

		/* Also move the selection decal to match the bottom of the bounds */
		const FVector DecalLoc = SelectionDecal->GetRelativeTransform().GetLocation();
		SelectionDecal->SetRelativeLocation(FVector(DecalLoc.X, DecalLoc.Y, -Extent.Z));

		//-------------------------------------------------------------------------

		/* Can use any value really, does not have to be fog managers tile size, but makes sense
		to use a number not lower than tile size. Larger number means better performance when
		checking if a building can be seen, but means more of it has to be revealed for it to
		be seen. Whenever editing this you will need to make an edit to Bounds to cause
		post edit to fire */
		const float StepSize = FogOfWarOptions::FOG_TILE_SIZE * 2;
		assert(StepSize >= FogOfWarOptions::FOG_TILE_SIZE);

		/* Each double for loop is for a single quadrant. Each works its way out from the
		center. Do it this way instead of starting at edge because then one edge can have
		an easier time being revealed than others */

		TArray <FVector2D> Corners;
		TArray <FVector2D> Edges;
		TArray <FVector2D> Other;

		{
			int32 i = 0.f;
			while (i <= Extent.X)
			{
				const bool bAtEdge = (i + StepSize > Extent.X);

				int32 j = i == 0.f ? StepSize : 0.f;
				while (j <= Extent.Y)
				{
					const bool bAtSomeOtherEdge = (j + StepSize > Extent.Y);

					if (bAtEdge && bAtSomeOtherEdge)
					{
						// Corner point 
						Corners.Emplace(FVector2D(i, j));
					}
					else if (bAtEdge || bAtSomeOtherEdge)
					{
						// Edge
						Edges.Emplace(FVector2D(i, j));
					}
					else
					{
						// Not edge
						Other.Emplace(FVector2D(i, j));
					}

					j += StepSize;
				}

				i += StepSize;
			}
		}

		UE_CLOG(Corners.Num() > 1, RTSLOG, Fatal, TEXT("Too many corners: %d"), Corners.Num());

		{
			int32 i = -StepSize;
			while (i >= -Extent.X)
			{
				const bool bAtEdge = (i - StepSize < -Extent.X);

				int32 j = 0.f;
				while (j <= Extent.Y)
				{
					const bool bAtSomeOtherEdge = (j + StepSize > Extent.Y);

					if (bAtEdge && bAtSomeOtherEdge)
					{
						// Corner point 
						Corners.Emplace(FVector2D(i, j));
					}
					else if (bAtEdge || bAtSomeOtherEdge)
					{
						// Edge
						Edges.Emplace(FVector2D(i, j));
					}
					else
					{
						// Not edge
						Other.Emplace(FVector2D(i, j));
					}

					j += StepSize;
				}

				i -= StepSize;
			}
		}

		UE_CLOG(Corners.Num() > 2, RTSLOG, Fatal, TEXT("Too many corners: %d"), Corners.Num());

		{
			int32 i = 0.f;
			while (i <= Extent.X)
			{
				const bool bAtEdge = (i + StepSize > Extent.X);

				int32 j = -StepSize;
				while (j >= -Extent.Y)
				{
					const bool bAtSomeOtherEdge = (j - StepSize < -Extent.Y);

					if (bAtEdge && bAtSomeOtherEdge)
					{
						// Corner point 
						Corners.Emplace(FVector2D(i, j));
					}
					else if (bAtEdge || bAtSomeOtherEdge)
					{
						// Edge
						Edges.Emplace(FVector2D(i, j));
					}
					else
					{
						// Not edge
						Other.Emplace(FVector2D(i, j));
					}

					j -= StepSize;
				}

				i += StepSize;
			}
		}

		UE_CLOG(Corners.Num() > 3, RTSLOG, Fatal, TEXT("Too many corners: %d"), Corners.Num());

		{
			int32 i = -StepSize;
			while (i >= -Extent.X)
			{
				const bool bAtEdge = (i - StepSize < -Extent.X);

				int32 j = -StepSize;
				while (j >= -Extent.Y)
				{
					const bool bAtSomeOtherEdge = (j - StepSize > Extent.Y);

					if (bAtEdge && bAtSomeOtherEdge)
					{
						// Corner point 
						Corners.Emplace(FVector2D(i, j));
					}
					else if (bAtEdge || bAtSomeOtherEdge)
					{
						// Edge
						Edges.Emplace(FVector2D(i, j));
					}
					else
					{
						// Not edge
						Other.Emplace(FVector2D(i, j));
					}

					j -= StepSize;
				}

				i -= StepSize;
			}
		}

		if (Corners.Num() > 4)
		{
			FString ErrorMessage;
			for (const auto & Elem : Corners)
			{
				ErrorMessage += Elem.ToString() + "  ||  ";
			}
			
			UE_LOG(RTSLOG, Fatal, TEXT("Too many corners: %d. Corners are: %s "), Corners.Num(), *ErrorMessage);
		}

		/* Add points to array in the order corners -> edges -> other. This aids fog manager
		because once it finds one revealed point it considers the whole building revealed and
		will stop iterating the array. Corners and edges are usually discovered first when
		scouting so that is why they are added first */

		FogLocations.Empty();

		for (const auto & Elem : Corners)
		{
			FogLocations.Emplace(Elem);
		}
		for (const auto & Elem : Edges)
		{
			FogLocations.Emplace(Elem);
		}
		for (const auto & Elem : Other)
		{
			FogLocations.Emplace(Elem);
		}

		// Add center point
		FogLocations.Emplace(FVector2D(0.f, 0.f));

		// Make sure no duplicate entries
		TSet < FVector2D > CheckForDuplicates;
		CheckForDuplicates.Reserve(FogLocations.Num());
		for (const auto & Elem : FogLocations)
		{
			if (CheckForDuplicates.Contains(Elem))
			{
				UE_LOG(RTSLOG, Fatal, TEXT("Duplicate point found: %s"), *Elem.ToString());
			}

			CheckForDuplicates.Emplace(Elem);
		}
	}
}
#endif // WITH_EDITOR

//#if WITH_EDITOR
EBuildingType ABuilding::GetType() const
{
	return Type;
}
//#endif

EFaction ABuilding::GetFaction() const
{
	return PS->GetFaction();
}

bool ABuilding::IsInsideFogOfWar() const
{
	return RootComponent->bVisible == false;
}

float ABuilding::GetRemainingBuildTime() const
{
	return Attributes.GetBuildTime() - TimeSpentBuilding;
}

FVector ABuilding::GetFinalLocation() const
{
	return SpawnLocation + FVector(0.f, 0.f, BoundsHeight);
}

bool ABuilding::IsConstructionComplete() const
{
	return (GameTickCountOnConstructionComplete != UINT8_MAX);
}

bool ABuilding::IsBeingBuilt() const
{
	/* Recently added this. To avoid making uneccessary changes to replicated variables this 
	assert has been added. Should be able to avoid it. If we cannot then this function needs to 
	be rewritten because it assumes that construction has not completed. Actually this has 
	been commented. Anytime I set construction to being complete we also set 
	LastTimeWhenConstructionStarted to less than 0 */
	//assert(IsConstructionComplete() == false);
	
	return (ConstructionProgressInfo.LastTimeWhenConstructionStarted >= 0.f);
}

bool ABuilding::CanAcceptBuilder() const
{
	const bool bBuildMethodAcceptable = (Attributes.GetBuildMethod() == EBuildingBuildMethod::LayFoundationsInstantly
		|| Attributes.GetBuildMethod() == EBuildingBuildMethod::LayFoundationsWhenAtLocation);
	
	return bBuildMethodAcceptable && CanAcceptBuilderChecked();
}

bool ABuilding::CanAcceptBuilderChecked() const
{
	return !IsConstructionComplete() && !IsBeingBuilt();
}

void ABuilding::OnWorkerGained(AActor * NewWorker)
{
	SERVER_CHECK;
	assert(CanAcceptBuilder());
	assert(Statics::IsValid(NewWorker));

	assert(Builders.Contains(NewWorker) == false);
	Builders.Emplace(NewWorker);

	Server_SetBeingBuilt(true);
}

void ABuilding::OnWorkerLost(AActor * LostWorker)
{
	SERVER_CHECK;
	assert(Statics::IsValid(LostWorker));

	assert(Builders.Contains(LostWorker) == true);
	Builders.Remove(LostWorker);

	Server_SetBeingBuilt(false);
}

bool ABuilding::IsDropPointFor(EResourceType ResourceType) const
{
	assert(ResourceType != EResourceType::None);
	return Attributes.GetResourceCollectionTypes().Contains(ResourceType);
}

FVector ABuilding::GetInitialMoveLocation() const
{
	return UnitFirstMoveLoc->GetComponentLocation();
}

FVector ABuilding::GetRallyPointLocation() const
{
	return UnitRallyPoint->GetComponentLocation();
}

EUnitInitialSpawnBehavior ABuilding::GetProducedUnitInitialBehavior() const
{
	return Attributes.GetProducedUnitInitialBehavior();
}

bool ABuilding::CanChangeRallyPoint() const
{
	// Component will be null or is just irrelevant if cannot produce
	if (Attributes.CanProduce())
	{
		if (GI->GetBuildingRallyPointDisplayRule() == EBuildingRallyPointDisplayRule::OnlyWhenFullyConstructed
			&& IsConstructionComplete() == false)
		{
			return false;
		}
		else
		{
			return true;
		}
	}

	return false;
}

AResourceSpot * ABuilding::GetClosestResourceSpot(const FResourceGatheringProperties & ResourceGatheringAttributes) const
{
	/* Going to be slow brute force here. I currently save the closest depot for each resource 
	spot but not the other way round. */

	// About to iterate TMap. Good to have done this quick check first
	assert(ResourceGatheringAttributes.IsCollector());
	
	AResourceSpot * BestResourceSpot = nullptr;
	float BestDistanceSqr = FLT_MAX;

	for (const auto & Pair : ResourceGatheringAttributes.GetTMap())
	{
		const EResourceType ResourceType = Pair.Key;

		/* Slow - imagine if user wants like 5000 trees each its own resource spot similar to 
		warcraft III. Should be more event driven. When a building is built then figure it 
		out then. Also need to consider resource spots becoming depleted though */
		for (const auto & ResourceSpot : GS->GetResourceSpots(ResourceType))
		{
			// Skip resource spots with nothing left
			if (ResourceSpot->IsDepleted() == false)
			{
				const float DistanceSqr = Statics::GetPathDistanceSqr(this, ResourceSpot);
				if (DistanceSqr < BestDistanceSqr)
				{
					BestDistanceSqr = DistanceSqr;
					BestResourceSpot = ResourceSpot;
				}
			}
		}
	}

	return BestResourceSpot;
}

void ABuilding::Server_ChangeRallyPointLocation_Implementation(const FVector_NetQuantize & NewLocation)
{
	if (CanChangeRallyPoint())
	{
		RallyPointLocation = NewLocation;

		OnRep_RallyPointLocation();
	}
}

bool ABuilding::Server_ChangeRallyPointLocation_Validate(const FVector_NetQuantize & NewLocation)
{
	return true;
}

void ABuilding::ChangeRallyPointLocation(const FVector_NetQuantize & NewLocation)
{
	// Assumed these have already been checked before calling this
	SERVER_CHECK;
	assert(CanChangeRallyPoint());

	RallyPointLocation = NewLocation;

	OnRep_RallyPointLocation();
}

ARTSPlayerState * ABuilding::GetPS() const
{
	return PS;
}

ARTSGameState * ABuilding::GetGS() const
{
	return GS;
}

ARTSPlayerController * ABuilding::GetPC() const
{
	return PC;
}

TSubclassOf<class AGhostBuilding> ABuilding::GetGhostBP() const
{
	return Ghost_BP;
}

const TArray<FVector2D>& ABuilding::GetFogLocations() const
{
	return FogLocations;
}

bool ABuilding::CheckAnimationProperties() const
{
	const bool bAnimInstanceNull = (GetMesh()->GetAnimInstance() == nullptr);

	if (bAnimInstanceNull)
	{
		UE_LOG(RTSLOG, Fatal, TEXT("Building \"%s\" does not have an anim instance set or maybe it "
			"isn't for the right skeleton"), *GetClass()->GetName());
		
		return false;
	}
	
	const bool bAnimInstanceClassOk = (GetMesh()->GetAnimInstance()->IsA(UBuildingAnimInstance::StaticClass()));

	UE_CLOG(!bAnimInstanceClassOk, RTSLOG, Fatal, TEXT("Building \"%s\" has anim instance \"%s\". This "
		"anim instance needs to be reparented to a UBuildingAnimInstance otherwise the RTS anim "
		"notifies will not trigger"), *GetName(), *GetMesh()->GetAnimInstance()->GetName());

	// Check all anim notifies we expect to be on the montages are there
	FString ErrorMessage;
	bool AllAnimNotifiesPresent = true;
	for (const auto & Elem : Attributes.GetAllAnimations())
	{
		// A lot of animations won't be used by some buildings so null check
		if (Elem.Value != nullptr)
		{
			AllAnimNotifiesPresent = VerifyAllAnimNotifiesArePresentOnMontage(Elem.Key, Elem.Value, ErrorMessage);
			if (AllAnimNotifiesPresent == false)
			{
				UE_LOG(RTSLOG, Fatal, TEXT("Building [%s] with anim montage [%s] is missing anim notify [%s]. "
					"Please add this notify to the montage."), *GetClass()->GetName(),
					*Elem.Value->GetName(), *ErrorMessage);
				break;
			}
		}
	}

	return bAnimInstanceClassOk;
}

#if !UE_BUILD_SHIPPING
bool ABuilding::VerifyAllAnimNotifiesArePresentOnMontage(EBuildingAnimation AnimType,
	UAnimMontage * Montage, FString & OutErrorMessage) const
{
	switch (AnimType)
	{
	// Animation types that don't need any notifies
	case EBuildingAnimation::Idle:
	case EBuildingAnimation::Construction:
	{
		return true;
	}
	case EBuildingAnimation::Destroyed:
	{
		FString FuncName = GET_FUNCTION_NAME_STRING_CHECKED(UBuildingAnimInstance, AnimNotify_OnZeroHealthAnimationFinished);
		// Remove the "AnimNotify_" part at the start
		FuncName = FuncName.RightChop(11);
		static const FName FuncFName = FName(*FuncName);

		for (const auto & Elem : Montage->Notifies)
		{
			if (Elem.NotifyName == FuncFName)
			{
				return true;
			}
		}

		// If here then montage not found 

		OutErrorMessage = FuncName;
		return false;
	}
	case EBuildingAnimation::OpenDoor:
	{
		FString FuncName = GET_FUNCTION_NAME_STRING_CHECKED(UBuildingAnimInstance, AnimNotify_OnDoorFullyOpen);
		// Remove the "AnimNotify_" part at the start
		FuncName = FuncName.RightChop(11);
		static const FName FuncFName = FName(*FuncName);

		for (const auto & Elem : Montage->Notifies)
		{
			if (Elem.NotifyName == FuncFName)
			{
				return true;
			}
		}

		// If here then montage not found 

		OutErrorMessage = FuncName;
		return false;
	}
	default:
	{
		return true;
	}
	}
}
#endif

const FProductionQueue * ABuilding::GetPersistentProductionQueue() const
{
	return &Attributes.PersistentProductionQueue;
}

FProductionQueue * ABuilding::GetPersistentProductionQueue()
{
	return &Attributes.PersistentProductionQueue;
}

const FProductionQueue & ABuilding::GetContextProductionQueue() const
{
	return Attributes.ProductionQueue;
}

void ABuilding::Server_OnProductionRequestBuilding_Implementation(EBuildingType BuildingType, EProductionQueueType QueueType)
{
	const FContextButton Button = FContextButton(BuildingType);
	Server_OnProductionRequest(Button, QueueType, true, nullptr);
}

bool ABuilding::Server_OnProductionRequestBuilding_Validate(EBuildingType BuildingType, EProductionQueueType QueueType)
{
	return true;
}

void ABuilding::Server_OnProductionRequestUnit_Implementation(EUnitType UnitType, EProductionQueueType QueueType)
{
	const FContextButton Button = FContextButton(UnitType);
	Server_OnProductionRequest(Button, QueueType, true, nullptr);
}

bool ABuilding::Server_OnProductionRequestUnit_Validate(EUnitType UnitType, EProductionQueueType QueueType)
{
	return true;
}

void ABuilding::Server_OnProductionRequestUpgrade_Implementation(EUpgradeType UpgradeType, EProductionQueueType QueueType)
{
	const FContextButton Button = FContextButton(UpgradeType);
	Server_OnProductionRequest(Button, QueueType, true, nullptr);
}

bool ABuilding::Server_OnProductionRequestUpgrade_Validate(EUpgradeType UpgradeType, EProductionQueueType QueueType)
{
	return true;
}

void ABuilding::Server_OnProductionRequest(const FContextButton & Button, EProductionQueueType QueueType, 
	bool bPerformChecks, ABuilding * InBuildingWeAreProducing)
{
	SERVER_CHECK;

	const bool bShowHUDWarning = true;
	const FTrainingInfo TrainingInfo = FTrainingInfo(Button);

	FProductionQueue & Queue = (QueueType == EProductionQueueType::Persistent)
		? Attributes.PersistentProductionQueue : Attributes.ProductionQueue;

	/* Perform all the relevant checks first */
	if (bPerformChecks)
	{
		if (Queue.HasRoom(PS, bShowHUDWarning) == false)
		{
			return;
		}
		if (TrainingInfo.IsForUpgrade())
		{
			/* Check if it has been researched or not */
			if (PS->GetUpgradeManager()->HasBeenFullyResearched(TrainingInfo.GetUpgradeType(), bShowHUDWarning) == true)
			{
				return;
			}
			/* Check if already researching the upgrade */
			else if (PS->GetUpgradeManager()->IsUpgradeQueued(TrainingInfo.GetUpgradeType(), bShowHUDWarning) == true)
			{
				return;
			}
		}
		else
		{
			if (PS->IsAtSelectableCap(true, bShowHUDWarning) == true
				|| PS->IsAtUniqueSelectableCap(TrainingInfo, true, bShowHUDWarning) == true)
			{
				return;
			}

			if (TrainingInfo.IsForUnit())
			{
				if (PS->HasEnoughHousingResources(TrainingInfo.GetUnitType(), bShowHUDWarning) == false)
				{
					return;
				}
			}
		}
		if (PS->ArePrerequisitesMet(TrainingInfo, bShowHUDWarning) == false)
		{
			return;
		}
		if (PS->HasEnoughResources(Button, bShowHUDWarning) == false)
		{
			return;
		}
	}
	
	/* If here then all checks have passed */

	/* Deduct cost */
	const FUpgradeInfo * UpgradeInfo = nullptr;
	const FBuildInfo * BuildInfo = nullptr;
	if (TrainingInfo.IsForUpgrade())
	{
		UpgradeInfo = &FI->GetUpgradeInfoChecked(TrainingInfo.GetUpgradeType());

		PS->SpendResources(UpgradeInfo->GetCosts());
	}
	else
	{
		BuildInfo = FI->GetBuildInfo(Button);

		PS->SpendResources(BuildInfo->GetCosts());
	}

	/* Put item at back of queue. Remember: queue is reversed so index 0 is the back */
	Queue.Insert(TrainingInfo, 0);
	const bool bFirstInQueue = (Queue.Num() == 1);

#if !UE_BUILD_SHIPPING

	/* Make sure if we are producing a building that uses the BuildsItself build method that 
	we passed in a valid pointer to it */
	if (TrainingInfo.IsProductionForBuilding())
	{
		const EBuildingBuildMethod BuildMethod = FI->GetBuildingInfo(TrainingInfo.GetBuildingType())->GetBuildingBuildMethod();

		if (BuildMethod == EBuildingBuildMethod::BuildsItself)
		{
			assert(InBuildingWeAreProducing != nullptr);
		}
	}
	
#endif

	/* Only really need to do this if it is a building we are producing and its build method 
	is BuildsItself. Make InBuildingWeAreProducing a 3rd param with default value nullptr. Well 
	eventually do that, but first make it a non-default param so we can catch all the places 
	it is called */
	Queue.SetBuildingBeingProduced(InBuildingWeAreProducing);

	PS->OnItemAddedToAProductionQueue(Button);

	if (bFirstInQueue)
	{
		/* Start production if queue was previously empty */
		
		float ProductionTime;
		if (TrainingInfo.IsForUpgrade())
		{
			ProductionTime = UpgradeInfo->GetTrainTime();

			PS->GetUpgradeManager()->OnUpgradePutInProductionQueue(TrainingInfo.GetUpgradeType(), this);
			PS->GetUpgradeManager()->OnUpgradeProductionStarted(TrainingInfo.GetUpgradeType(), this);
		}
		else
		{
			ProductionTime = BuildInfo->GetTrainTime();
		}

		if (QueueType == EProductionQueueType::Persistent)
		{
			Delay(Queue.GetTimerHandle(), &ABuilding::Server_OnProductionComplete_Persistent, ProductionTime);
		}
		else
		{
			Delay(Queue.GetTimerHandle(), &ABuilding::Server_OnProductionComplete_Context, ProductionTime);
		}

		// Notify client
		if (Button.IsForBuildBuilding())
		{
			Client_StartProductionBuilding(Button.GetBuildingType(), QueueType);
		}
		else if (Button.IsForTrainUnit())
		{
			Client_StartProductionUnit(Button.GetUnitType(), QueueType);
		}
		else // Assuming for upgrade
		{
			assert(Button.IsForResearchUpgrade());

			Client_StartProductionUpgrade(Button.GetUpgradeType(), QueueType);
		}
	}
	else
	{
		// Notify client
		if (Button.IsForBuildBuilding())
		{
			Client_OnBuildingAddedToProductionQueue(Button.GetBuildingType(), QueueType);
		}
		else if (Button.IsForTrainUnit())
		{
			Client_OnUnitAddedToProductionQueue(Button.GetUnitType(), QueueType);
		}
		else // Assuming for upgrade
		{
			assert(Button.IsForResearchUpgrade());

			PS->GetUpgradeManager()->OnUpgradePutInProductionQueue(TrainingInfo.GetUpgradeType(), this);

			Client_OnUpgradeAddedToProductionQueue(Button.GetUpgradeType(), QueueType);
		}
	}

	/* Update our own HUD since the client functions just return if on server but only if we
	own the building */
	if (Attributes.GetAffiliation() == EAffiliation::Owned)
	{
		if (bFirstInQueue)
		{
			PC->GetHUDWidget()->OnItemAddedAndProductionStarted(TrainingInfo, Queue, this);
		}
		else
		{
			PC->GetHUDWidget()->OnItemAddedToProductionQueue(TrainingInfo, Queue, this);
		}
	}
}

void ABuilding::Client_StartProductionBuilding_Implementation(EBuildingType BuildingType, EProductionQueueType QueueType)
{
	const FTrainingInfo Info = FTrainingInfo(BuildingType);
	Client_StartProduction(Info, QueueType);
}

void ABuilding::Client_StartProductionUnit_Implementation(EUnitType UnitType, EProductionQueueType QueueType)
{
	const FTrainingInfo Info = FTrainingInfo(UnitType);
	Client_StartProduction(Info, QueueType);
}

void ABuilding::Client_StartProductionUpgrade_Implementation(EUpgradeType UpgradeType, EProductionQueueType QueueType)
{
	const FTrainingInfo Info = FTrainingInfo(UpgradeType);
	Client_StartProduction(Info, QueueType);
}

void ABuilding::Client_StartProduction(const FTrainingInfo & InTrainingInfo, EProductionQueueType QueueType)
{
	/* Calling this func implies a success. A failure will be dealt with automatically if choosing
	to display HUD messages via param */

	/* Already did stuff on server */
	if (OnServer())
	{
		return;
	}

	// Assume persistent if not context
	FProductionQueue & QueueToUse = (QueueType == EProductionQueueType::Persistent)
		? Attributes.PersistentProductionQueue : Attributes.ProductionQueue;

	QueueToUse.AddToQueue(InTrainingInfo);

	PS->OnItemAddedToAProductionQueue(InTrainingInfo);

	const float ProductionTime = FI->GetProductionTime(InTrainingInfo);

	if (QueueType == EProductionQueueType::Persistent)
	{
		Delay(QueueToUse.GetTimerHandle(),
			&ABuilding::Client_OnProductionQueueTimerHandleFinished_Persistent, ProductionTime);
	}
	else // Assumed context
	{
		if (Attributes.GetAffiliation() == EAffiliation::Owned && InTrainingInfo.IsForUpgrade())
		{
			PS->GetUpgradeManager()->OnUpgradePutInProductionQueue(InTrainingInfo.GetUpgradeType(), this);
			PS->GetUpgradeManager()->OnUpgradeProductionStarted(InTrainingInfo.GetUpgradeType(), this);
		}

		assert(QueueType == EProductionQueueType::Context);
		Delay(QueueToUse.GetTimerHandle(),
			&ABuilding::Client_OnProductionQueueTimerHandleFinished_Context, ProductionTime);
	}

	/* This is where we need to tell HUD so it can monitor timer handle */
	PC->GetHUDWidget()->OnItemAddedAndProductionStarted(InTrainingInfo,
		QueueToUse, this);
}

void ABuilding::Client_OnBuildingAddedToProductionQueue_Implementation(EBuildingType BuildingType, EProductionQueueType QueueType)
{
	Client_OnItemAddedToProductionQueue(FTrainingInfo(BuildingType), QueueType);
}

void ABuilding::Client_OnUnitAddedToProductionQueue_Implementation(EUnitType UnitType, EProductionQueueType QueueType)
{
	Client_OnItemAddedToProductionQueue(FTrainingInfo(UnitType), QueueType);
}

void ABuilding::Client_OnUpgradeAddedToProductionQueue_Implementation(EUpgradeType UpgradeType, EProductionQueueType QueueType)
{
	Client_OnItemAddedToProductionQueue(FTrainingInfo(UpgradeType), QueueType);
}

void ABuilding::Client_OnItemAddedToProductionQueue(const FTrainingInfo & InInfo, EProductionQueueType QueueType)
{
	/* Already did stuff on server */
	if (OnServer())
	{
		return;
	}

	/* Update num in queue */
	FProductionQueue & QueueToUse = (QueueType == EProductionQueueType::Persistent)
		? Attributes.PersistentProductionQueue
		: Attributes.ProductionQueue; // Assumed context queue
	QueueToUse.AddToQueue(InInfo);

	PS->OnItemAddedToAProductionQueue(InInfo);

	if (Attributes.GetAffiliation() == EAffiliation::Owned && InInfo.IsForUpgrade())
	{
		PS->GetUpgradeManager()->OnUpgradePutInProductionQueue(InInfo.GetUpgradeType(), this);
	}

	PC->GetHUDWidget()->OnItemAddedToProductionQueue(InInfo, Attributes.ProductionQueue, this);
}

void ABuilding::Server_OnProductionComplete_Context()
{
	// Because not actually RPC
	SERVER_CHECK;

	Server_OnProductionComplete_Inner(Attributes.ProductionQueue);
}

void ABuilding::Server_OnProductionComplete_Persistent()
{
	// Because not actually RPC
	SERVER_CHECK;
	
	// Will in turn call Client_OnProductionComplete
	Server_OnProductionComplete_Inner(Attributes.PersistentProductionQueue);
}

void ABuilding::Server_OnProductionComplete_Inner(FProductionQueue & Queue)
{
	assert(Queue.Num() > 0);

	/* Check what was completed - different behavior for buildings, units and upgrades */
	const FTrainingInfo Front = Queue.Peek();
	if (Front.IsProductionForBuilding())
	{
		/* Depending on the building build method behavior will vary */

		const FBuildingInfo * BuildingInfo = FI->GetBuildingInfo(Front.GetBuildingType());
		const EBuildingBuildMethod BuildMethod = BuildingInfo->GetBuildingBuildMethod();

		if (BuildMethod == EBuildingBuildMethod::BuildsInTab)
		{
			/* Show in HUD that building is complete and ready for placement and update button
			to allow placing building in world */

			Queue.SetHasCompletedBuildsInTab(true);

			PS->OnBuildsInTabProductionComplete(this);
		}
		else if (BuildMethod == EBuildingBuildMethod::BuildsItself)
		{
			/* Flag this as null. Don't really need to wrap it in an if, can probably do it right 
			after the if (Front.IsProductionForBuilding()) check */
			Queue.SetBuildingBeingProduced(nullptr);
		}
		else
		{
			// This indicates a problem with code
			UE_LOG(RTSLOG, Fatal, TEXT("Building building with queue but not using a supported "
				"build method, need to use BuildsInTab or BuildsItself but build method is %s"),
				TO_STRING(EBuildingBuildMethod, BuildMethod));
		}
	}
	else if (Front.IsForUnit())
	{
		OnUnitProductionComplete(Front.GetUnitType());

		/* We stop here and do not remove item from queue yet because there may be a open door
		anim that plays for the building. Once we confirm unit has been spawned
		OnProductionCompletePart2() will be called */
		return;
	}
	else // Assumed for upgrade
	{
		assert(Front.IsForUpgrade());

		GS->Multicast_OnUpgradeComplete(PS->GetPlayerIDAsInt(), Front.GetUpgradeType());
	}

	Queue.GetType() == EProductionQueueType::Persistent
		? Server_OnProductionCompletePart2_Persistent()
		: Server_OnProductionCompletePart2_Context();
}

void ABuilding::OnUnitProductionComplete(EUnitType UnitType)
{
	SERVER_CHECK;
	
	/* Check if should play 'open door' anim. Buildings that use an open door anim are robbed 
	of some time. Basically when production completes the door anim starts and when the door 
	anim completes then the unit spawns. Without a door anim unit would just spawn instantly 
	when production completes */
	if (Attributes.IsAnimationValid(EBuildingAnimation::OpenDoor))
	{
		/* This won't look good if the open door anim is already playing. Will play from start.
		Anim notify AnimNotify_OnDoorFullyOpen will handle spawning the unit */
		PlayAnimation(EBuildingAnimation::OpenDoor, true);
	}
	else
	{
		SpawnUnit(UnitType);
	}
}

void ABuilding::AnimNotify_OnDoorFullyOpen()
{
	SERVER_CHECK;

	// Need to distinguish here between regular queue and AICon queue
	SpawnUnit(Attributes.ProductionQueue.GetUnitAtFront(IsOwnedByCPUPlayer()));
}

void ABuilding::SpawnUnit(EUnitType UnitType)
{
	SERVER_CHECK;

	const FUnitInfo * const Info = FI->GetUnitInfo(UnitType);
	const TSubclassOf <AActor> UnitBP = Info->GetSelectableBP();
	assert(UnitBP != nullptr);

	// Figure out spawn location and rotation
	const FVector SpawnLoc = UnitSpawnLoc->GetComponentLocation();
	const FRotator SpawnRot = UnitSpawnLoc->GetComponentRotation();

	// Spawn unit
	AActor * Unit = Statics::SpawnUnit(UnitBP, SpawnLoc, SpawnRot, PS, GS, GetWorld(), 
		ESelectableCreationMethod::Production, this);
	/* Assert fires if spawn location is blocked */
	assert(Unit != nullptr);

	/* Notify queue the unit has been spawned and it can continue with production if there is
	more stuff in queue. Always use context version here since only context queue can 
	produce units */
	Server_OnProductionCompletePart2_Context();
}

void ABuilding::Server_OnProductionCompletePart2_Persistent()
{
	Server_OnProductionCompletePart2_Inner(Attributes.PersistentProductionQueue);
}

void ABuilding::Server_OnProductionCompletePart2_Context()
{
	Server_OnProductionCompletePart2_Inner(Attributes.ProductionQueue);
}

void ABuilding::Server_OnProductionCompletePart2_Inner(FProductionQueue & Queue)
{
	SERVER_CHECK;

	if (PS->bIsABot)
	{
		Queue.GetType() == EProductionQueueType::Persistent
			? AICon_OnProductionCompletePart2_Persistent()
			: AICon_OnProductionCompletePart2_Context();
		
		return;
	}

	const FTrainingInfo Front = Queue.Peek();

	/* Tally to keep track of how many places in queue to remove client-side - we're only sending
	one message to client to say how many items to remove from their queue */
	uint8 NumRemoved = 0;

	if (Queue.HasCompletedBuildsInTab())
	{
		// Do not need to do anything here 
	}
	else
	{
		Queue.Pop();
		NumRemoved++;

		PS->OnItemRemovedFromAProductionQueue(Front); 

		/* Do nothing with housing resources because the unit will be spawned. But we could 
		remove consumption here if we wanted I guess and then add consumption in the unit's 
		Setup() behavior. But we will choose to do nothing */

		/* Now start producing the next thing in queue but only if certain conditions are met 
		such as prereqs */
		while (Queue.Num() > 0)
		{
			const FTrainingInfo NextInfo = Queue.Peek();

			if (NextInfo.IsForUpgrade())
			{
				const FUpgradeInfo & UpgradeInfo = FI->GetUpgradeInfoChecked(NextInfo.GetUpgradeType());

				const bool bPassedChecks = PS->ArePrerequisitesMet(UpgradeInfo, NextInfo.GetUpgradeType(), false);

				if (!bPassedChecks)
				{
					/* Refund resources */
					PS->GainResources(UpgradeInfo.GetCosts());

					/* Remove from queue */
					Queue.Pop();
					NumRemoved++;
					// NextInfo is an alias, is this OK? Changed it, but didn't test to see if it needed it
					PS->OnItemRemovedFromAProductionQueue(NextInfo);
					continue;
				}
				else
				{
					// Start production
					const float ProductionTime = UpgradeInfo.GetTrainTime();
					Queue.GetType() == EProductionQueueType::Persistent
						? Delay(Queue.GetTimerHandle(), &ABuilding::Server_OnProductionComplete_Persistent, ProductionTime)
						: Delay(Queue.GetTimerHandle(), &ABuilding::Server_OnProductionComplete_Context, ProductionTime);
					
					break;
				}
			}
			else // Assuming for unit since persistent queues have a limit of 1
			{
				assert(NextInfo.IsForUnit());

				const FUnitInfo * UnitInfo = FI->GetUnitInfo(NextInfo.GetUnitType());

				/* If we fail the checks then the item just gets removed and refunded from queue. 
				However if we still have the prereqs but are missing the housing then we may want 
				to halt production instead. Consider implementing this some time */
				const bool bPassedChecks = PS->ArePrerequisitesMet(NextInfo.GetUnitType(), false) 
					&& PS->HasEnoughHousingResources(UnitInfo->GetHousingCosts(), false);

				if (!bPassedChecks)
				{
					/* Refund resources */
					PS->GainResources(UnitInfo->GetCosts());
					PS->RemoveHousingResourceConsumption(UnitInfo->GetHousingCosts());

					/* Remove from queue */
					Queue.Pop();
					NumRemoved++;
					PS->OnItemRemovedFromAProductionQueue(NextInfo);
					continue;
				}
				else
				{
					// Start production
					const float ProductionTime = UnitInfo->GetTrainTime();
					Queue.GetType() == EProductionQueueType::Persistent
						? Delay(Queue.GetTimerHandle(), &ABuilding::Server_OnProductionComplete_Persistent, ProductionTime)
						: Delay(Queue.GetTimerHandle(), &ABuilding::Server_OnProductionComplete_Context, ProductionTime);

					break;
				}
			}
		}
	}
	
	// If the server player's queue
	if (PC == GetWorld()->GetFirstPlayerController())
	{
		// Update our HUD
		if (Queue.HasCompletedBuildsInTab())
		{
			PC->GetHUDWidget()->OnBuildsInTabProductionComplete(Front.GetBuildingType());
		}
		else
		{
			PC->GetHUDWidget()->OnProductionComplete(Front, Queue, NumRemoved, this);
		}
	}
	// Remote client
	else
	{
		/* Tell client how many items were removed from queue so they can update their visuals */
		Client_OnProductionComplete(NumRemoved, Queue.GetType());
	}
}

void ABuilding::Client_OnProductionComplete_Implementation(uint8 NumRemoved, EProductionQueueType QueueType)
{
	// For remote clients only
	CLIENT_CHECK;

	/* Update queue for visuals */

	FProductionQueue & QueueToUse = (QueueType == EProductionQueueType::Persistent)
		? Attributes.PersistentProductionQueue : Attributes.ProductionQueue;

	/* Clear the timer handle that was for visuals only if still running */
	GetWorldTimerManager().ClearTimer(QueueToUse.GetTimerHandle());

	if (NumRemoved == 0)
	{
		/* This is the case where we are using BuildsInTab build method. Show some kind of
		"Ready to place" text */

		PS->OnBuildsInTabProductionComplete(this);

		PC->GetHUDWidget()->OnBuildsInTabProductionComplete(QueueToUse.Peek().GetBuildingType());
	}
	else
	{
		const FTrainingInfo Front = QueueToUse.Peek();

		/* Remove from queue however many items server removed */
		for (uint8 i = 0; i < NumRemoved; ++i)
		{
			const FTrainingInfo Item = QueueToUse.Peek();
			
			QueueToUse.Pop();

			// Notify player state
			PS->OnItemRemovedFromAProductionQueue(Item);

			if (Item.IsForUnit())
			{
				const FUnitInfo * UnitInfo = FI->GetUnitInfo(Item.GetUnitType());
				PS->RemoveHousingResourceConsumption(UnitInfo->GetHousingCosts());
			}
		}

		/* Assuming that we just automatically start producing the next item in queue. Change how
		this is assigned in the future if needed */
		const bool bContinueProduction = true;

		if (bContinueProduction)
		{
			// If something in queue...
			if (QueueToUse.Num() > 0)
			{
				const FTrainingInfo & NextInfo = QueueToUse.Peek();
				const float ProductionTime = FI->GetProductionTime(NextInfo);

				/* Start timer handle for visuals only */
				QueueType == EProductionQueueType::Persistent 
					? Delay(QueueToUse.GetTimerHandle(), &ABuilding::Client_OnProductionQueueTimerHandleFinished_Persistent, ProductionTime) 
					: Delay(QueueToUse.GetTimerHandle(), &ABuilding::Client_OnProductionQueueTimerHandleFinished_Context, ProductionTime);
			}
		}

		PC->GetHUDWidget()->OnProductionComplete(Front, QueueToUse, NumRemoved, this);
	}

	/* For UI progress bar purposes */
	QueueToUse.SetHasCompletedEarly(false);
}

void ABuilding::Client_OnProductionQueueTimerHandleFinished_Context()
{
	CLIENT_CHECK;

	Attributes.ProductionQueue.Client_OnProductionComplete();
}

void ABuilding::Client_OnProductionQueueTimerHandleFinished_Persistent()
{
	CLIENT_CHECK;

	Attributes.PersistentProductionQueue.Client_OnProductionComplete();
}

void ABuilding::AICon_QueueProduction(const FCPUPlayerTrainingInfo & ItemWeWantToProduce)
{
	// Buildings go to persistent queue, units and upgrades go to context queue 
	if (ItemWeWantToProduce.IsProductionForBuilding())
	{
		AICon_QueueProductionInner(Attributes.PersistentProductionQueue, ItemWeWantToProduce);
	}
	else
	{
		AICon_QueueProductionInner(Attributes.ProductionQueue, ItemWeWantToProduce);
	}
}

void ABuilding::AICon_QueueProduction(const FCPUPlayerTrainingInfo & ItemWeWantToProduce, 
	ABuilding * BuildingWeAreProducing)
{
	/* Check the 1 item per persistent queue rule is being followed */
	assert(GetPersistentProductionQueue()->Num() == 0);
	assert(BuildingWeAreProducing != nullptr);

	Attributes.PersistentProductionQueue.SetBuildingBeingProduced(BuildingWeAreProducing);

	AICon_QueueProductionInner(Attributes.PersistentProductionQueue, ItemWeWantToProduce);
}

void ABuilding::AICon_QueueProductionInner(FProductionQueue & Queue, const FCPUPlayerTrainingInfo & ItemWeWantToProduce)
{
	assert(Queue.AICon_HasRoom());

	/* Put item at back of queue */
	Queue.AICon_Insert(ItemWeWantToProduce, 0);

	/* Should probably deduct resources now since I don't think the AIController does it at 
	any point */

	/* Check if first item in queue. In that case will need to start production */
	if (Queue.AICon_Num() == 1)
	{
		// Figure out how long it takes to build
		float ProductionTime;
		if (ItemWeWantToProduce.IsProductionForBuilding())
		{
			const FBuildingInfo * BuildingInfo = FI->GetBuildingInfo(ItemWeWantToProduce.GetBuildingType());

			ProductionTime = BuildingInfo->GetTrainTime();
		}
		else if (ItemWeWantToProduce.IsForUnit())
		{
			const FUnitInfo * UnitInfo = FI->GetUnitInfo(ItemWeWantToProduce.GetUnitType());

			ProductionTime = UnitInfo->GetTrainTime();
		}
		else // Asummed upgrade
		{
			assert(ItemWeWantToProduce.IsForUpgrade());

			const FUpgradeInfo & UpgradeInfo = FI->GetUpgradeInfoChecked(ItemWeWantToProduce.GetUpgradeType());

			ProductionTime = UpgradeInfo.GetTrainTime();
		}

		/* Start production */
		Queue.GetType() == EProductionQueueType::Persistent
			? Delay(Queue.GetTimerHandle(), &ABuilding::AICon_OnQueueProductionComplete_Persistent, ProductionTime)
			: Delay(Queue.GetTimerHandle(), &ABuilding::AICon_OnQueueProductionComplete_Context, ProductionTime);
	}
}

void ABuilding::AICon_OnQueueProductionComplete_Persistent()
{
	AICon_OnQueueProductionComplete_Inner(Attributes.PersistentProductionQueue);
}

void ABuilding::AICon_OnQueueProductionComplete_Context()
{
	AICon_OnQueueProductionComplete_Inner(Attributes.ProductionQueue);
}

void ABuilding::AICon_OnQueueProductionComplete_Inner(FProductionQueue & Queue)
{
	const FCPUPlayerTrainingInfo Front = Queue.AICon_Last();

	/* Spawn unit, apply upgrade etc */
	if (Front.IsProductionForBuilding())
	{
		// Do not need to do anything here
	}
	else if (Front.IsForUnit())
	{
		OnUnitProductionComplete(Front.GetUnitType());

		/* Do not remove from queue yet because open door anim might need to know what was
		completed */
		return;
	}
	else // Assumed upgrade
	{
		assert(Front.IsForUpgrade());
		GS->Multicast_OnUpgradeComplete(PS->GetPlayerIDAsInt(), Front.GetUpgradeType());
	}

	//~~~

	Queue.GetType() == EProductionQueueType::Persistent
		? AICon_OnProductionCompletePart2_Persistent()
		: AICon_OnProductionCompletePart2_Context();
}

void ABuilding::AICon_OnProductionCompletePart2_Persistent()
{
	AICon_OnProductionCompletePart2_Inner(Attributes.PersistentProductionQueue);
}

void ABuilding::AICon_OnProductionCompletePart2_Context()
{
	AICon_OnProductionCompletePart2_Inner(Attributes.ProductionQueue);
}

void ABuilding::AICon_OnProductionCompletePart2_Inner(FProductionQueue & Queue)
{
	const FCPUPlayerTrainingInfo & Front = Queue.AICon_Last();

	Queue.AICon_Pop();

	Queue.SetBuildingBeingProduced(nullptr);

	/* Array of items removed from queue because they can no longer be produced */
	TArray < FCPUPlayerTrainingInfo > RemovedButNotBuilt;

	// Need to make sure next in queue's prereqs are met just like human players
	for (int32 i = Queue.AICon_Num() - 1; i >= 0; --i)
	{
		// AICon_GetItemInQueue = Bracket operator on AICon_Queue
		const FCPUPlayerTrainingInfo & Item = Queue.AICon_BracketOperator(i);

		/* Remember right now persistent queues have max capacity of 1 so next in queue has to
		either be a unit or upgrade */
		assert(!Item.IsProductionForBuilding());

		if (Item.IsForUnit())
		{
			const FUnitInfo * UnitInfo = FI->GetUnitInfo(Item.GetUnitType());

			const bool bPassedChecks = PS->ArePrerequisitesMet(Item.GetUnitType(), false);

			if (bPassedChecks)
			{
				// Good to produce
				break;
			}
			else
			{
				/* Refund resources */
				PS->GainResources(UnitInfo->GetCosts());

				/* Remove from queue and put in array */
				RemovedButNotBuilt.Emplace(Queue.AICon_Pop());
			}
		}
		else // Assumed for upgrade
		{
			assert(Item.IsForUpgrade());

			const FUpgradeInfo & UpgradeInfo = FI->GetUpgradeInfoChecked(Item.GetUpgradeType());

			/* Check upgrade has not already been researched already. Actually don't do this
			with human version so won't do it here either */

			const bool bPassedChecks = PS->ArePrerequisitesMet(UpgradeInfo, Item.GetUpgradeType(), false);

			if (bPassedChecks)
			{
				// Good to produce
				break;
			}
			else
			{
				/* Refund resources */
				PS->GainResources(UpgradeInfo.GetCosts());

				/* Remove from queue and put in array */
				RemovedButNotBuilt.Emplace(Queue.AICon_Pop());
			}
		}
	}

	// Start producing next thing in queue if not empty
	if (Queue.AICon_Num() > 0)
	{
		const FTrainingInfo & NextInQueue = Queue.AICon_Last();

		// Figure out how long it takes to build
		float ProductionTime;
		if (NextInQueue.IsProductionForBuilding())
		{
			const FBuildingInfo * BuildingInfo = FI->GetBuildingInfo(NextInQueue.GetBuildingType());

			ProductionTime = BuildingInfo->GetTrainTime();
		}
		else if (NextInQueue.IsForUnit())
		{
			const FUnitInfo * UnitInfo = FI->GetUnitInfo(NextInQueue.GetUnitType());

			ProductionTime = UnitInfo->GetTrainTime();
		}
		else // Asummed upgrade
		{
			assert(NextInQueue.IsForUpgrade());

			const FUpgradeInfo & UpgradeInfo = FI->GetUpgradeInfoChecked(NextInQueue.GetUpgradeType());

			ProductionTime = UpgradeInfo.GetTrainTime();
		}

		/* Start production */
		Queue.GetType() == EProductionQueueType::Persistent
			? Delay(Queue.GetTimerHandle(), &ABuilding::AICon_OnQueueProductionComplete_Persistent, ProductionTime)
			: Delay(Queue.GetTimerHandle(), &ABuilding::AICon_OnQueueProductionComplete_Context, ProductionTime);
	}

	//~~~

	/* Tell AI controller about everything that happened */
	PS->GetAIController()->OnQueueProductionComplete(this, &Queue, Front, RemovedButNotBuilt);
}

UBoxComponent * ABuilding::GetBounds() const
{
	return Bounds;
}

void ABuilding::OnSingleSelect()
{
	assert(SelectionWidgetComponent != nullptr);
	
	Attributes.SetIsSelected(true);
	Attributes.SetIsPrimarySelected(false);

	ShowSelectionDecal();
	SelectionWidgetComponent->SetVisibility(true);

	// If we own this building and it can produce then possibly show the rally point
	if (Attributes.GetAffiliation() == EAffiliation::Owned && UnitRallyPoint != nullptr)
	{
		bool bShowRallyPoint = false;

		if (GI->GetBuildingRallyPointDisplayRule() == EBuildingRallyPointDisplayRule::Always)
		{
			bShowRallyPoint = true;
		}
		else // Assumed OnlyWhenFullyConstructed
		{
			if (IsConstructionComplete())
			{
				bShowRallyPoint = true;
			}
		}

		if (bShowRallyPoint)
		{	
			UnitRallyPoint->SetVisibility(true);
		}
	}

	// Show selection particles 
	UParticleSystem * NewTemplate = FI->GetSelectionParticles(Attributes.GetAffiliation(),
		Attributes.GetParticleSize());

	// Thought source would return straight away if template is the same... doesn't look like it though
	if (SelectionParticles->Template != NewTemplate)
	{
		SelectionParticles->SetTemplate(NewTemplate);
	}

	// Reset particles to play from start
	SelectionParticles->ResetParticles(true);
	SelectionParticles->ActivateSystem(false);
}

EUnitType ABuilding::OnMarqueeSelect(uint8 & SelectableID)
{
	// Buildings are not expected to be marquee selected
	assert(0);

	SelectableID = GetSelectableID();

	return EUnitType::NotUnit;
}

uint8 ABuilding::OnDeselect()
{
	Attributes.SetIsSelected(false);
	Attributes.SetIsPrimarySelected(false);

	HideSelectionDecal();

	// Hide rally point (only does stuff if building owned by us)
	if (UnitRallyPoint != nullptr)
	{
		UnitRallyPoint->SetVisibility(false);
	}

	/* Stop selection/right-click particles */
	SelectionParticles->ResetParticles(true);

	// Hide selection widget
	SelectionWidgetComponent->SetVisibility(false);

	return ID;
}

uint8 ABuilding::GetSelectableID() const
{
	return ID;
}

bool ABuilding::Security_CanBeClickedOn(ETeam ClickersTeam, const FName & ClickersTeamTag, ARTSGameState * GameState) const
{
	return true;
}

void ABuilding::OnRightClick()
{
	// Show click particles if any
	UParticleSystem * NewTemplate = FI->GetRightClickParticles(Attributes.GetAffiliation(),
		Attributes.GetParticleSize());

	if (SelectionParticles->Template != NewTemplate)
	{
		SelectionParticles->SetTemplate(NewTemplate);
	}

	// Reset particles to play from start
	SelectionParticles->ResetParticles(true);
	SelectionParticles->ActivateSystem(false);
}

void ABuilding::OnMouseHover(ARTSPlayerController * LocalPlayCon)
{
	/* Do not overwrite fully selected decal if this is selected */
	if (IsSelectableSelected())
	{
		return;
	}

	ShowHoverDecal();
}

void ABuilding::OnMouseUnhover(ARTSPlayerController * LocalPlayCon)
{
	/* Do nothing if selected */
	if (IsSelectableSelected())
	{
		return;
	}

	HideSelectionDecal();
}

ETeam ABuilding::GetTeam() const
{
	return Attributes.GetTeam();
}

uint8 ABuilding::GetTeamIndex() const
{
	return Statics::TeamToArrayIndex(Attributes.GetTeam());
}

const FContextMenuButtons * ABuilding::GetContextMenu() const
{
	return &Attributes.GetContextMenu();
}

void ABuilding::GetInfo(TMap < EResourceType, int32 > & OutCosts, float & OutBuildTime, FContextMenuButtons & OutContextMenu) const
{
	OutCosts = this->Attributes.GetCosts();
	OutBuildTime = this->Attributes.GetBuildTime();
	OutContextMenu = this->Attributes.GetContextMenu();
}

TArray<EBuildingType> ABuilding::GetPrerequisites() const
{
	return Attributes.GetPrerequisites();
}

float ABuilding::GetCooldownRemaining(const FContextButton & Button)
{
	if (!Attributes.GetContextMenu().HasButton(Button))
	{
		return -2.f;
	}

	return GetWorldTimerManager().GetTimerRemaining(ContextActionCooldowns[Button]);
}

void ABuilding::OnContextCommand(const FContextButton & Button)
{
	assert(GetWorld()->IsServer());

	assert(Attributes.GetContextMenu().HasButton(Button));

	const EContextButton Command = Button.GetButtonType();

	switch (Command)
	{
	case EContextButton::Train:
	case EContextButton::Upgrade:
	/* Not sure this should be here since we know it's the context queue. MAYBE some of the 3 
	context build methods will work. But the PC::OnContextButtonPress code hijacks build type 
	buttons and always spawns a ghost so I don't think this case is ever true */
	case EContextButton::BuildBuilding:
	{
		// Aggressive assert, for now just assuming we cannot produce buildings using context queue
		assert(Command != EContextButton::BuildBuilding);
		
		/* The 3rd param says whether to perform checks */
		Server_OnProductionRequest(Button, EProductionQueueType::Context, true, nullptr);
		break;
	}
	default:
	{
		// Put functionality for different buttons here

		break;
	}
	}
}

void ABuilding::OnContextCommand(const FContextButtonInfo & AbilityInfo, const FVector & ClickLoc)
{
	SERVER_CHECK;

	/* Play animation */
	if (AbilityInfo.UsesAnimation())
	{
		PlayAnimation(AbilityInfo.GetBuildingAnimationType(), true);
	}

	/* Spawn the ability. No waiting for a anim notify. 
	It is good to check range and maybe other stuff too here. But for
	now we just assume it is all ok. I only assume this because the only ability I use right
	now is a nuke which has unlimited range */
	GS->Server_CreateAbilityEffect(EAbilityUsageType::SelectablesActionBar, 0, AbilityInfo,
		this, this, GetTeam(), ClickLoc, this, this);
}

void ABuilding::OnAbilityUse(const FContextButtonInfo & AbilityInfo, uint8 ServerTickCountAtTimeOfAbility)
{
#if !UE_BUILD_SHIPPING
	if (GetWorld()->IsServer())
	{
		assert(ServerTickCountAtTimeOfAbility == GS->GetGameTickCounter());
	}
#endif 

	// Spend selectable resource cost (like mana cost)
	if (AbilityInfo.GetSelectableResourceCost_1() != 0)
	{
		Attributes.GetSelectableResource_1().AdjustAmount(-AbilityInfo.GetSelectableResourceCost_1(),
			GS->GetGameTickCounter(), ServerTickCountAtTimeOfAbility, true, PC->GetHUDWidget(), this,
			PersistentWidgetComponent, SelectionWidgetComponent, Attributes.IsSelected(),
			Attributes.IsPrimarySelected());
	}

	/* Start cooldown timer handle. I guess we'll only do this for server and owning clients
	but there's nothing wrong with doing it for everybody */
	// Ummm... don't I need to check if Cooldown > 0 here also?
	if (GetWorld()->IsServer() || Attributes.GetAffiliation() == EAffiliation::Owned)
	{
		if (AbilityInfo.UsesPreparationAnimation())
		{
			CallAbilityPreparationLogicAfterDelay(AbilityInfo, AbilityInfo.GetCooldown());
		}

		const FContextButton Button(AbilityInfo.GetButtonType());
		Delay(ContextActionCooldowns[Button], &ABuilding::OnAbilityCooldownFinished, AbilityInfo.GetCooldown());

		/* If the HUD wasn't polling each action button each tick then this is where we would
		tell the HUD that it is now on cooldown. So when I change to event driven "telling HUD
		we're on cooldown" then this is where it should be done */
	}
}

const FAttachInfo * ABuilding::GetBodyLocationInfo(ESelectableBodySocket BodyLocationType) const
{
	// Just copied AInfantry's version
	return &Attributes.GetBodyAttachPointInfo(BodyLocationType);
}

bool ABuilding::UpdateFogStatus(EFogStatus FogStatus)
{
	/* For multithreaded fog, as long as some class does the whole 'add all selectables
	and their locs to a service queue, then have fog thread calc stuff' then in Setup()
	where PS->OnBuildingPlaced is called doesn't need reordering, otherwise it may need
	to go at the very end of Setup() */

	/* Assuming that fog of war manager only ever passes in EFogStatus::Hidden or
	EFogStatus::Revealed since buildings do not support stealth at the moment */

	bool bCanBeSeen;

	if (FogStatus == EFogStatus::Hidden)
	{
		// Check if status has changed
		if (Bounds->bVisible == true)
		{
			OnEnterFogOfWar();
		}

		bCanBeSeen = false;
	}
	else
	{
		assert(FogStatus == EFogStatus::Revealed);

		// Check if status has changed 
		if (Bounds->bVisible == false)
		{
			OnExitFogOfWar();
		}

		bCanBeSeen = true;
	}

	Attributes.SetVisionState(FogStatus);

	return bCanBeSeen;
}

void ABuilding::GetVisionInfo(float & SightRadius, float & StealthRevealRadius) const
{
	SightRadius = Attributes.GetEffectiveSightRange();
	StealthRevealRadius = Attributes.GetEffectiveStealthRevealRange();
}

const FProductionQueue * ABuilding::GetProductionQueue() const
{
	return &Attributes.ProductionQueue;
}

void ABuilding::SetOnSpawnValues(ESelectableCreationMethod InCreationMethod, ABuilding * BuilderBuilding)
{
	Attributes.SetCreationMethod(InCreationMethod);
	CreationMethod = InCreationMethod;
}

void ABuilding::SetSelectableIDAndGameTickCount(uint8 InID, uint8 GameTickCounter)
{
	ID = InID;
	
	/* This variable is gone. We now have GameTickCountOnConstructionComplete which does not 
	get set now */
	//GameTickCountOnCreation = GameTickCounter;
}

const FSelectableAttributesBase & ABuilding::GetAttributesBase() const
{
	return Attributes;
}

FSelectableAttributesBase & ABuilding::GetAttributesBase()
{
	return Attributes;
}

const FSelectableAttributesBasic * ABuilding::GetAttributes() const
{
	return &Attributes;
}

const FBuildingAttributes * ABuilding::GetBuildingAttributes() const
{
	return &Attributes;
}

FBuildingAttributes * ABuilding::GetBuildingAttributesModifiable()
{
	return &Attributes;
}

float ABuilding::GetHealth() const
{
	return IsConstructionComplete() ? Health : HealthWhileUnderConstruction;
}

float ABuilding::GetBoundsLength() const
{
	/* This by no means is very good. For a long time this just returned 0. This 
	is a quick solution and not very good. Should look through code and see where and 
	why I'm calling this. For all the times I call this I should ask myself: 
	'Do we want to query the box comp or the mesh?' Cause right now I have it querying 
	the mesh */
	return Mesh->Bounds.GetSphere().W;
}

TSubclassOf<AProjectileBase> ABuilding::GetProjectileBP() const
{
	// Returns null but may want to consider defense components
	return nullptr;
}

bool ABuilding::HasAttack() const
{
	return Attributes.IsDefenseBuilding();
}

bool ABuilding::CanClassGainExperience() const
{
	return false;
}

uint8 ABuilding::GetRank() const
{
	return UINT8_MAX;
}

float ABuilding::GetCurrentRankExperience() const
{
	return 0.0f;
}

const TMap<FContextButton, FTimerHandle>* ABuilding::GetContextCooldowns() const
{
	return &ContextActionCooldowns;
}

const FShopInfo * ABuilding::GetShopAttributes() const
{
	/* The function description says return null if we don't display or sell anything so that's 
	what we'll do */
	
	if (Attributes.GetShoppingInfo().GetItems().Num() > 0)
	{
		return &Attributes.GetShoppingInfo();
	}
	else
	{
		return nullptr;
	}
}

const FInventory * ABuilding::GetInventory() const
{
	return nullptr; // Null because buildings don't have inventories
}

float ABuilding::GetTotalExperienceRequiredForNextLevel() const
{
	return -1.0f; // Garbage
}

ARTSPlayerController * ABuilding::GetLocalPC() const
{
	return PC;
}

void ABuilding::OnItemPurchasedFromHere(int32 ShopSlotIndex, ISelectable * ItemRecipient, bool bTryUpdateHUD)
{
	Attributes.GetShoppingInfo().OnPurchase(ShopSlotIndex);

	if (bTryUpdateHUD)
	{
		if (Attributes.IsSelected())
		{
			// This func should end up calling UItemOnDisplayInShopButton::OnPurchaseFromHere
			PC->GetHUDWidget()->Selected_OnItemPurchasedFromShop(this, ShopSlotIndex, Attributes.IsPrimarySelected());
		}
	}
}

uint8 ABuilding::GetOwnersID() const
{
	return PS->GetPlayerIDAsInt();
}

FVector ABuilding::GetActorLocationSelectable() const
{
	return GetActorLocation();
}

bool ABuilding::IsInStealthMode() const
{
	// False because buildings cannot enter stealth
	return false;
}

bool ABuilding::HasEnoughSelectableResources(const FContextButtonInfo & AbilityInfo) const
{
	return Attributes.GetSelectableResource_1().GetAmount() >= AbilityInfo.GetSelectableResourceCost_1();
}

void ABuilding::ShowTooltip(URTSHUD * HUDWidget) const
{
	// NOOP. Don't show tooltips for buildings
}

void ABuilding::OnEnemyDestroyed(const FSelectableAttributesBasic & EnemyAttributes)
{
	PS->GainExperience(EnemyAttributes.GetExperienceBounty(), true);
}

void ABuilding::AdjustPersistentWorldWidgetForNewCameraZoomAmount(float CameraZoomAmount, float CameraPitchAmount)
{
	// todo

	//const FVector CurrentLocation = PersistentWidgetComponent->RelativeLocation;
	//const FVector NewLocation = FVector(CurrentLocation.X, CurrentLocation.Y + Attributes.PersistentWorldWidgetOffset.X,
	//	(1000000.f / CameraZoomAmount) + Attributes.PersistentWorldWidgetOffset.Y);
	//
	//PersistentWidgetComponent->SetRelativeLocation(NewLocation);
}

void ABuilding::AdjustSelectedWorldWidgetForNewCameraZoomAmount(float CameraZoomAmount, float CameraPitchAmount)
{
	//const FVector CurrentLocation = SelectionWidgetComponent->RelativeLocation;
	//const FVector NewLocation = FVector(CurrentLocation.X, CurrentLocation.Y + Attributes.SelectionWorldWidgetOffset.X,
	//	(1000000.f / CameraZoomAmount) + Attributes.SelectionWorldWidgetOffset.Y);
	//
	//SelectionWidgetComponent->SetRelativeLocation(NewLocation);
}

ARTSPlayerState * ABuilding::Selectable_GetPS() const
{
	return PS;
}

const FBuildingTargetingAbilityPerSelectableInfo * ABuilding::GetSpecialRightClickActionTowardsBuildingInfo(ABuilding * Building, EFaction BuildingsFaction, EBuildingType BuildingsType, EAffiliation BuildingsAffiliation) const
{
	/* Buildings don't currently have any special building targeting abilities */
	return nullptr;
}

bool ABuilding::CanAquireTarget(AActor * PotentialTargetAsActor, ISelectable * PotentialTarget) const
{
	/* For now this just returns false. But it may want to consider all the building 
	attack components */
	return false;
}

URTSGameInstance * ABuilding::Selectable_GetGI() const
{
	return GI;
}

bool ABuilding::CanClassAcceptBuffsAndDebuffs() const
{
	return ALLOW_BUFFS_AND_DEBUFFS_ON_BUILDINGS;
}

#if ALLOW_BUFFS_AND_DEBUFFS_ON_BUILDINGS

const TArray<FStaticBuffOrDebuffInstanceInfo>* ABuilding::GetStaticBuffArray() const
{
	return &Attributes.GetStaticBuffs();
}

const TArray<FStaticBuffOrDebuffInstanceInfo>* ABuilding::GetStaticDebuffArray() const
{
	return &Attributes.GetStaticDebuffs();
}

#if TICKABLE_BUFF_AND_DEBUFF_ENABLED_GAME

const TArray<FTickableBuffOrDebuffInstanceInfo>* ABuilding::GetTickableBuffArray() const
{
	return &Attributes.GetTickableBuffs();
}

const TArray<FTickableBuffOrDebuffInstanceInfo>* ABuilding::GetTickableDebuffArray() const
{
	return &Attributes.GetTickableDebuffs();
}

#endif // TICKABLE_BUFF_AND_DEBUFF_ENABLED_GAME

#endif // ALLOW_BUFFS_AND_DEBUFFS_ON_BUILDINGS

#if WITH_EDITOR
bool ABuilding::PIE_IsForCPUPlayer() const
{
	return PIE_bIsForCPUPlayer;
}

int32 ABuilding::PIE_GetHumanOwnerIndex() const
{
	return PIE_HumanOwnerIndex;
}

int32 ABuilding::PIE_GetCPUOwnerIndex() const
{
	return PIE_CPUOwnerIndex;
}
#endif

void ABuilding::SetupBuildingInfo(FBuildingInfo & OutInfo, const AFactionInfo * FactionInfo) const
{
	OutInfo.SetupCostsArray(Attributes.GetCosts());
	OutInfo.SetTrainTime(Attributes.GetBuildTime());
	OutInfo.SetBuildingType(Type);
	OutInfo.SetContextMenu(Attributes.GetContextMenu());
	OutInfo.SetName(Attributes.GetName());
	OutInfo.SetHUDImage(Attributes.GetHUDImage_Normal());
	OutInfo.SetHUDHoveredImage(Attributes.GetHUDImage_Hovered());
	OutInfo.SetHUDPressedImage(Attributes.GetHUDImage_Pressed());
	OutInfo.SetHUDHighlightedImage(Attributes.GetHUDImage_Highlighted());
	OutInfo.SetDescription(Attributes.GetDescription());
	OutInfo.SetSelectableType(ESelectableType::Building);
	OutInfo.SetUnitType(EUnitType::NotUnit);
	OutInfo.SetPrerequisites(Attributes.GetPrerequisites(), Attributes.GetUpgradePrerequisites());
	OutInfo.SetBoundsHeight(CalculateBoundsHeight());
	OutInfo.SetScaledBoxExtent(Bounds->GetScaledBoxExtent());
	if (Attributes.OverridesFactionBuildMethod())
	{
		OutInfo.SetBuildingBuildMethod(Attributes.GetBuildMethod());
	}
	else
	{
		OutInfo.SetBuildingBuildMethod(FactionInfo->GetDefaultBuildingBuildMethod());
	}
	// Set rises from ground true if no construction animation
	OutInfo.SetBuildingRisesFromGround(!Attributes.IsAnimationValid(EBuildingAnimation::Construction));
	// Set how far building has to be from another building to be built
	const float ProximityRange = Attributes.OverrideFactionBuildProximity() ?
		Attributes.GetBuildProximityRange() : FactionInfo->GetBuildingProximityRange();
	OutInfo.SetBuildProximityDistance(ProximityRange);
	OutInfo.SetFoundationRadius(Attributes.GetFoundationsRadius());
	OutInfo.SetHUDPersistentTabType(Attributes.GetHUDPersistentTabCategory());
	OutInfo.SetHUDPersistentTabOrdering(Attributes.GetHUDPersistentTabButtonOrdering());
	OutInfo.SetJustBuiltSound(Attributes.GetJustBuiltSound());
	OutInfo.SetAnnounceToAllWhenBuilt(Attributes.AnnounceToAllWhenBuilt());
	OutInfo.SetNumPersistentQueues(Attributes.GetNumPersistentBuildSlots());

	/* Set whether building is a barracks type. This assumes that unit info has been setup first */
	OutInfo.SetIsBarracksType(false); // Set default value false
	for (const auto & Button : GetContextMenu()->GetButtonsArray())
	{	
		if (Button.IsForTrainUnit() 
			&& FactionInfo->GetUnitInfo(Button.GetUnitType())->IsAArmyUnitType())
		{
			OutInfo.SetIsBarracksType(true);
			break;
		}
	}

	/* Set whether building is a base defense type */
	OutInfo.SetIsBaseDefenseType(Attributes.IsDefenseBuilding());
	OutInfo.SetTimeIntoZeroHealthAnimThatAnimNotifyIs(Attributes.GetTimeIntoZeroHealthAnimThatAnimNotifyIsSlow());
}

bool ABuilding::IsOwnedByCPUPlayer() const
{
	return PS->bIsABot;
}

float ABuilding::TakeDamage(float DamageAmount, FDamageEvent const & DamageEvent, AController * EventInstigator, AActor * DamageCauser)
{
	if (GetWorld()->IsServer() == false)
	{
		return 0.f;
	}
	
	if (DamageAmount == 0.f || Statics::HasZeroHealth(this))
	{
		return 0.f;
	}

	// Leaving out super because it appears to just broadcast event
	//Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	const float ArmourTypeMultiplier = GI->GetDamageMultiplier(DamageEvent.DamageTypeClass,
		Attributes.GetArmourType());
	const float FinalDamage = DamageAmount * ArmourTypeMultiplier * Attributes.GetDefenseMultiplier();

	/* I use (bCanBeDamaged == false) to mean invulnerable. Note we had to wait to after the 
	multipliers because a damage type could have a negative multiplier and could heal us instead 
	of damage us which we want to allow happen */
	if ((FinalDamage > 0.f) && (bCanBeDamaged == false))
	{
		return 0.f;
	}

	if (IsConstructionComplete() == false)
	{
		DamageTakenWhileUnderConstruction += FinalDamage;
		Health = HealthWhileUnderConstruction = HealthWhileUnderConstruction - FinalDamage;
	}
	else
	{
		Health -= FinalDamage;
	}

	if (Health <= 0.f)
	{
		Health = 0.f;
		OnZeroHealth(EOnZeroHealthCallSite::FromTakeDamage);
	}
	else
	{
		PersistentWidgetComponent->OnHealthChanged(Health);
		SelectionWidgetComponent->OnHealthChanged(Health);
		
		if (Attributes.IsSelected())
		{
			/* Update HUD */
			PC->GetHUDWidget()->Selected_OnHealthChanged(this, Health, Attributes.GetMaxHealth(),
				Attributes.IsPrimarySelected());
		}
	}

	return FinalDamage;
}

void ABuilding::OnZeroHealthAnimFinished()
{
	MESSAGE("OnZeroHealthAnimFinished()");

	assert(Attributes.IsAnimationValid(EBuildingAnimation::Destroyed));
	
	RunOnZeroHealthAnimFinishedLogic();
}

void ABuilding::RunOnZeroHealthAnimFinishedLogic()
{
	assert(Attributes.IsAnimationValid(EBuildingAnimation::Destroyed));

	// Might need to change to if then return
	assert(!bHasRunCleanUp);
	assert(!bSinkingIntoGround_Logically);

	/* Freeze animation at this point. For this to work though the anim notify needs to
	be slightly away from the end. Kinda lame */
	Mesh->bNoSkeletonUpdate = true;
	Mesh->bPauseAnims = true;
	bHasDestroyedAnimFinished = true;

	// Set this. Only relevant for else case below
	TimeSpentSinking = GS->GetServerWorldTimeSeconds() - AnimationState.StartTime - TimeIntoZeroHealthAnimThatAnimNotifyIs;

	/* Check if enough time has passed that we could have finished sinking into ground */
	if (TimeSpentSinking >= TimeToSpendSinking)
	{
		GetWorld()->IsServer() ? ServerCleanUp() : ClientCleanUp(false);
	}
	else
	{
		// Set position for how far through sinking building is. Possibly will double up on SetActorLocation calls this frame
		const FVector Loc = GetActorLocation();
		SetActorLocation(FVector(Loc.X, Loc.Y, Loc.Z - (TimeSpentSinking * BoundsHeight * (1.f / TimeToSpendSinking))));
		
		BeginSinkingIntoGround();
	}
}

void ABuilding::OnSinkingIntoGroundComplete()
{
	GetWorld()->IsServer() ? ServerCleanUp() : ClientCleanUp(false);
}

bool ABuilding::HasBeenCompletelyDestroyed() const
{
	return GetTearOff();
}

void ABuilding::ServerCleanUp()
{
	SERVER_CHECK;

	if (bHasRunCleanUp)
	{
		return;
	}
	bHasRunCleanUp = true;

	GS->OnSelectableDestroyed(this, Attributes.GetTeam(), GetWorld()->IsServer());
	PS->OnBuildingDestroyed(this);

	if (bHasLocalPlayerSeenBuildingAtZeroHealth)
	{
		PS->RemoveBuildingFromBuildingsContainer(this);
	}

	/* Well this is interesting:
	In my testing calling TearOff then Destroy in the same frame will still cause the destroy
	to be replicated to clients. However calling them with a delay in between will not.
	The amount of the delay matters though. Small delays like 0.01 don't work.
	So because of this I'm going to do a 10sec delay between calling
	TearOff and Destroy. I don't really like using delays as bandaids like this, would
	like to figure out exactly why this happens but it may be code in the actor channel
	that I don't think I can touch without a custom engine build. Then again adding a delay
	is the solution to everything */

	/* Tear off here so Destroy call later does not destroy the building on clients too.
	Even if the building isn't net relevant to clients I still get the building being destroyed
	for clients which I don't want because it could be in fog and we want players to discover
	it is gone only when they reveal the fog over its location */
	TearOff();

	/* Notify the replication graph. It will move this building onto an always relevant
	to all connections node. We really want all clients to know about its destruction.
	Ya know what. If calling TearOff does it anyway (i.e. the actor is replicated to
	all clients even if they're not relevant) then we don't really need to do anything here */
	UNetDriver * NetDriver = GetNetDriver();
	if (NetDriver != nullptr)
	{
		URTSReplicationGraph * ReplicationGraph = NetDriver->GetReplicationDriver<URTSReplicationGraph>();
#if WITH_EDITOR
		if (ReplicationGraph != nullptr)
#endif
		{
			ReplicationGraph->NotifyOfBuildingDestroyed(this);
		}
	}

	if (bHasLocalPlayerSeenBuildingAtZeroHealth)
	{
		// Make building hidden. What about collision too?
		HideBuilding();

		Delay(TimerHandle_Destroy, &ABuilding::DestroyBuilding, 5.f);
	}
	else
	{
		/* Building not visible to local player. Do nothing now. When they reveal the fog
		over its location it only then will we make it hidden and destroy it */
	}
}

void ABuilding::ClientCleanUp(bool bForceDestroy)
{
	CLIENT_CHECK;

	if (bHasRunCleanUp)
	{
		return;
	}
	bHasRunCleanUp = true;

	GS->OnSelectableDestroyed(this, Attributes.GetTeam(), GetWorld()->IsServer());
	PS->OnBuildingDestroyed(this);

	if (bHasLocalPlayerSeenBuildingAtZeroHealth || bForceDestroy)
	{
		PS->RemoveBuildingFromBuildingsContainer(this);

		Destroy();
	}
	else
	{
		/* Building not visible to local player. Do nothing now. When they reveal the fog
		over its location it only then will we make it hidden and destroy it */
	}
}

void ABuilding::TornOff()
{
	/* Being torn off is the server's way of telling us 'building can be destroyed' */

	Super::TornOff();

	/* This gets spammed between the time TearOff and Destroy is called. I do not know why. 
	With UBasicReplicationGraph it does not happen. However when I create my own rep graph class, 
	child it to UReplicationGraph and set it up exactly like UBasicReplication graph I still 
	get TornOff spam. Given that I don't really know where to go from there */

	MESSAGE("TornOff");
}

void ABuilding::DestroyBuilding()
{
	Destroy();
}

void ABuilding::CallAbilityPreparationLogicAfterDelay(const FContextButtonInfo & AbilityInfo, float CooldownAmount)
{
	/* Given we just end up playing an animation only the server really needs to do this */
	
	FTimerDelegate TimerDel;
	FTimerHandle TimerHandle;

	float DelayAmount = CooldownAmount - AbilityInfo.GetPreparationAnimPlayPoint();
	
	/* Could let this one slide and just call the function now. If this throws it could mean 
	your ability's preparation point time is greater than either the cooldown, initial cooldown 
	or both */
	assert(DelayAmount > 0.f);

	//Binding the function with specific values
	TimerDel.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ABuilding, OnAbilityPreparationCooldownFinished), AbilityInfo);
	GetWorldTimerManager().SetTimer(TimerHandle, TimerDel, DelayAmount, false);
}

void ABuilding::OnAbilityPreparationCooldownFinished(const FContextButtonInfo & AbilityInfo)
{
	// If destroyed then do not play the preparation animation 
	if (Statics::HasZeroHealth(this) == false)
	{
		PlayAnimation(AbilityInfo.GetBuildingPreparationAnimationType(), true);
	}
}

void ABuilding::OnAbilityCooldownFinished()
{
	// Do nothing
}

void ABuilding::OnRep_Health()
{
	CLIENT_CHECK;

	MESSAGE("OnRep_Health");

	// Check if building has been completely destroyed
	if (HasBeenCompletelyDestroyed())
	{
		OnZeroHealth(EOnZeroHealthCallSite::Other);
		ClientCleanUp(false);
		return;
	}

	if (Health <= 0.f)
	{
		OnZeroHealth(EOnZeroHealthCallSite::FromOnRepHealth);
	}
	else
	{
		if (IsConstructionComplete() == false)
		{
			/* This calculation is kind of an approximation. As usual clients always have an 
			approximation of the server state, it's just whether this approximation will be 
			acceptable enough */
			DamageTakenWhileUnderConstruction += HealthWhileUnderConstruction - Health;
			
			HealthWhileUnderConstruction = Health;
		}
		
		/* Update the persistent and selection widgets */
		PersistentWidgetComponent->OnHealthChanged(Health);
		SelectionWidgetComponent->OnHealthChanged(Health);

		if (Attributes.IsSelected())
		{
			/* Update HUD */
			PC->GetHUDWidget()->Selected_OnHealthChanged(this, Health, Attributes.GetMaxHealth(),
				Attributes.IsPrimarySelected());
		}
	}
}

void ABuilding::OnZeroHealth(EOnZeroHealthCallSite CalledFrom)
{
	/* Check if we've already run zero health logic. This check is mainly for clients. */
	if (Tags[Statics::GetZeroHealthTagIndex()] == Statics::HasZeroHealthTag)
	{
		return;
	}
	
	// Set this tag
	Tags[Statics::GetZeroHealthTagIndex()] = Statics::HasZeroHealthTag;

	/* Make unselectable */
	Bounds->SetVisibility(false);
	
	// bIsBeingBuilt = bIsBeingConstructed = false ?

	/* Since we can't be selected anymore just need to update the persistent widget */
	PersistentWidgetComponent->OnZeroHealth();

	PC->NotifyOfBuildingReachingZeroHealth(this, Attributes.GetZeroHealthSound(), Attributes.IsSelected());

	if (Attributes.IsSelected())
	{
		OnDeselect();
	}

	GS->OnBuildingZeroHealth(this, GetWorld()->IsServer());
	PS->OnBuildingZeroHealth(this);

	bool bPlayedDestroyedAnim = false;
	/* Play an animation if set. Otherwise just sink into the ground */
	if (Attributes.IsAnimationValid(EBuildingAnimation::Destroyed))
	{
		/* Will rely on an anim notify to tell us when it has finished */
		PlayAnimation(EBuildingAnimation::Destroyed, false);
		bPlayedDestroyedAnim = true;
	}
	else
	{
		if (GetWorld()->IsServer())
		{
			assert(TimeSpentSinking == 0.f);
			ServerInitiateSinkingIntoGround();
		}
	}

	/* Stop queue production and refund everything in them */
	if (GetWorld()->IsServer())
	{
		// AI con uses different queue so distinuish between that
		if (PS->bIsABot)
		{
			GetWorldTimerManager().ClearTimer(Attributes.PersistentProductionQueue.GetTimerHandle());
			GetWorldTimerManager().ClearTimer(Attributes.ProductionQueue.GetTimerHandle());

			for (int32 i = Attributes.PersistentProductionQueue.Num() - 1; i >= 0; --i)
			{
				const FCPUPlayerTrainingInfo & Elem = Attributes.PersistentProductionQueue.AICon_BracketOperator(i);

				// Refund resources. Assuming only buildings in persistent queue
				const FBuildingInfo * BuildingInfo = FI->GetBuildingInfo(Elem.GetBuildingType());
				PS->GainResources(BuildingInfo->GetCosts());
			}

			if (Attributes.PersistentProductionQueue.GetBuildingBeingProduced().IsValid())
			{
				// Need to cancel the production of the building we are producing
				Attributes.PersistentProductionQueue.GetBuildingBeingProduced()->OnProducerBuildingZeroHealth();
			}

			for (int32 i = Attributes.ProductionQueue.Num() - 1; i >= 0; --i)
			{
				const FCPUPlayerTrainingInfo & Elem = Attributes.ProductionQueue.AICon_BracketOperator(i);

				if (Elem.IsForUnit())
				{
					// Refund resources
					const FUnitInfo * UnitInfo = FI->GetUnitInfo(Elem.GetUnitType());
					PS->GainResources(UnitInfo->GetCosts());

					// Refund housing resource consumption
					PS->RemoveHousingResourceConsumption(UnitInfo->GetHousingCosts());
				}
				else // Assumed upgrade
				{
					assert(Elem.IsForUpgrade());

					// Refund resources
					const FUpgradeInfo & UpgradeInfo = FI->GetUpgradeInfoChecked(Elem.GetUpgradeType());
					PS->GainResources(UpgradeInfo.GetCosts());

					// Upgrade manager needs to know about this, especially if it was first in queue
					PS->GetUpgradeManager()->OnUpgradeCancelledFromProductionQueue(Elem.GetUpgradeType());
				}
			}
		}
		else
		{
			// Cancel and refund everything in the queue.
			// Timer handle clearing possibly done automatically but we'll do it anyway
			GetWorldTimerManager().ClearTimer(Attributes.PersistentProductionQueue.GetTimerHandle());
			for (int32 i = Attributes.PersistentProductionQueue.Num() - 1; i >= 0; --i)
			{
				const FTrainingInfo & Elem = Attributes.PersistentProductionQueue[i];

				// Refund resources. Assuming only buildings in persistent queue
				const FBuildingInfo * BuildingInfo = FI->GetBuildingInfo(Elem.GetBuildingType());
				PS->GainResources(BuildingInfo->GetCosts());
			}

			if (Attributes.PersistentProductionQueue.GetBuildingBeingProduced().IsValid())
			{
				// Need to cancel the production of the building we are producing
				Attributes.PersistentProductionQueue.GetBuildingBeingProduced()->OnProducerBuildingZeroHealth();
			}

			// Cancel and refund everything in the queue
			// Timer handle clearing possibly done automatically but we'll do it anyway
			GetWorldTimerManager().ClearTimer(Attributes.ProductionQueue.GetTimerHandle());
			for (int32 i = Attributes.ProductionQueue.Num() - 1; i >= 0; --i)
			{
				const FTrainingInfo & Elem = Attributes.ProductionQueue[i];

				if (Elem.IsForUnit())
				{
					// Refund resources
					const FUnitInfo * UnitInfo = FI->GetUnitInfo(Elem.GetUnitType());
					PS->GainResources(UnitInfo->GetCosts());

					// Refund housing resource consumption
					PS->RemoveHousingResourceConsumption(UnitInfo->GetHousingCosts());
				}
				else // Assumed upgrade
				{
					assert(Elem.IsForUpgrade());

					// Refund resources
					const FUpgradeInfo & UpgradeInfo = FI->GetUpgradeInfoChecked(Elem.GetUpgradeType());
					PS->GainResources(UpgradeInfo.GetCosts());

					// Upgrade manager needs to know about this, especially if it was first in queue
					PS->GetUpgradeManager()->OnUpgradeCancelledFromProductionQueue(Elem.GetUpgradeType());
				}
			}
		}

		// Disable the turret attack components
		// I haven't implement the bases yet but I may want to disable their ticks here too so they stop rotating or something
		UHeavyTaskManager * HeavyTaskManager = GI->GetHeavyTaskManager();
		for (UMeshComponent * AttackComp : Attributes.AttackComponents_Turrets)
		{
			// Unregister them with tasks manager. This will disable their sweeps from happening
			HeavyTaskManager->UnregisterBuildingAttackComponent(CastChecked<IBuildingAttackComp_Turret>(AttackComp));

			// Disable tick to stop them from attacking anything
			AttackComp->SetComponentTickEnabled(false);
		}
	}
	else
	{
		if (Attributes.GetAffiliation() == EAffiliation::Owned)
		{
			GetWorldTimerManager().ClearTimer(Attributes.PersistentProductionQueue.GetTimerHandle());
			GetWorldTimerManager().ClearTimer(Attributes.ProductionQueue.GetTimerHandle());

			for (int32 i = Attributes.ProductionQueue.Num() - 1; i >= 0; --i)
			{
				const FTrainingInfo & Elem = Attributes.ProductionQueue[i];

				if (Elem.IsForUpgrade())
				{
					PS->GetUpgradeManager()->OnUpgradeCancelledFromProductionQueue(Elem.GetUpgradeType());
				}
			}
		}
	}
	
	if (bPlayedDestroyedAnim && GetWorld()->IsServer())
	{
		if (TimeIntoZeroHealthAnimThatAnimNotifyIs > 0.f)
		{
			Delay(&ABuilding::OnZeroHealthAnimFinished, TimeIntoZeroHealthAnimThatAnimNotifyIs);
		}
		else
		{
			// Do it, do it now
			OnZeroHealthAnimFinished();

			/* Likely 1 of 3 things is true: 
			1. there is no OnZeroHealthAnimationFinished anim notify on the Destroyed anim
			2. Post edit isn't running correctly (my bad, need to fix this) 
			3. the user has placed the anim notify at the start of the anim. This is fine but 
			unexpected since the moment the notify fires the actor is destroyed. You can ignore 
			this warning if this is the case */
			UE_LOG(RTSLOG, Warning, TEXT("Animation montage \"%s\" does not have an anim notify "
				"OnZeroHealthAnimationFinished"), *Attributes.GetAnimation(EBuildingAnimation::Destroyed)->GetName());
		}
	}

	if (IsFullyVisibleLocally()) 
	{
		bHasLocalPlayerSeenBuildingAtZeroHealth = true;
	}

	/* Make sure BuildsInTab buildings that are complete are cancelled and refunded */
	/* Make sure if this building is producing another building with BuildsItself then it 
	is cancelled. 
	Side note: PC needs to make sure the BuildingBeingProduced ref inside FProductionQueue 
	is actually assigned a value whenever BuildsItself production happens */

	// Make sure to null production queue TH
}

void ABuilding::ServerInitiateSinkingIntoGround()
{
	// Pause animation. Look weird when building sinks into ground while something like idle anim is playing
	Mesh->bNoSkeletonUpdate = true;
	Mesh->bPauseAnims = true;
	
	/* Use the anim state replicated variable to broadcast the sinking into ground */
	AnimationState.AnimationType = EBuildingAnimation::SinkIntoGround;
	AnimationState.StartTime = GS->GetServerWorldTimeSeconds();
	// Set these too for server when it reveals building from fog. 
	PausedAnimInfo.AnimationType = EBuildingAnimation::SinkIntoGround;
	PausedAnimInfo.StartTime = GS->GetServerWorldTimeSeconds();

	//if (IsFullyVisibleLocally())
	BeginSinkingIntoGround();
}

void ABuilding::BeginSinkingIntoGround()
{
	assert(bSinkingIntoGround_Logically == false);

	bSinkingIntoGround_Logically = true;
	bSinkingIntoGround_Visually = bHasLocalPlayerSeenBuildingAtZeroHealth;
}

void ABuilding::OnProducerBuildingZeroHealth()
{
	SERVER_CHECK;
	
	/* This will trigger OnRep_Health for clients */
	Health = 0.f;

	/* OnRep_Health not called automatically on server so do this. Also note that OnZeroHealth 
	will do some things we know don't need to happen like clear out the production queue so 
	we could make a seperate function for the sake of performance */
	OnZeroHealth(EOnZeroHealthCallSite::Other);
}

float ABuilding::CalculateBoundsHeight() const
{
	// Old way which includes all components including rally points
	//const FBox BoundingBox = GetComponentsBoundingBox(true);
	//return (BoundingBox.Max.Z - BoundingBox.Min.Z);

	return Bounds->GetScaledBoxExtent().Z * 2;
}

void ABuilding::Multicast_OnBuildingAttackComponentAttackMade_Implementation(uint8 AttackCompID, AActor * AttackTarget)
{
	if (GetWorld()->IsServer())
	{
		// Already did stuff on server
		return;
	}

	IBuildingAttackComp_Turret * AttackComp = GetBuildingAttackComp_Turret(AttackCompID);
	AttackComp->ClientDoAttack(AttackTarget);
}

IBuildingAttackComp_Turret * ABuilding::GetBuildingAttackComp_Turret(uint8 CompUniqueID) const
{
	return CastChecked<IBuildingAttackComp_Turret>(Attributes.AttackComponents_Turrets[CompUniqueID]);
}

bool operator<(const ABuilding & Building1, const ABuilding & Building2)
{
	return Building1.GetType() < Building2.GetType();
}
