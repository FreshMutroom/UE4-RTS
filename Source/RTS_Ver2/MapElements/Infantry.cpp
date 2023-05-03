// Fill out your copyright notice in the Description page of Project Settings.

#include "Infantry.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/SkeletalMeshComponent.h"
#include "Public/TimerManager.h"
#include "Animation/AnimInstance.h"
#include "UnrealNetwork.h"
#include "Components/StaticMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "GameFramework/WorldSettings.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Public/AssetRegistryModule.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Public/KismetCompilerModule.h"

#include "MapElements/AIControllers/InfantryController.h"
#include "Statics/Statics.h"
#include "GameFramework/FactionInfo.h"
#include "GameFramework/RTSPlayerState.h"
#include "GameFramework/RTSGameState.h"
#include "GameFramework/RTSGameInstance.h"
#include "GameFramework/RTSPlayerController.h"
#include "MapElements/Abilities/AbilityBase.h"
#include "MapElements/Projectiles/ProjectileBase.h"
#include "Managers/ObjectPoolingManager.h"
#include "Managers/UpgradeManager.h"
#include "Statics/DevelopmentStatics.h"
#include "UI/RTSHUD.h"
#include "UI/WorldWidgets/WorldWidget.h"
#include "UI/WorldWidgets/SelectableWidgetComponent.h"
#include "MapElements/Building.h" // For SetOnSpawnValues
#include "Miscellaneous/UpgradeEffect.h"
#include "MapElements/Animation/InfantryAnimInstance.h"
#include "Audio/FogObeyingAudioComponent.h"


void FInfantryGarrisonStatus::SetEnteredSelectable(ISelectable * EnteredSelectable)
{
	GarrisonOwnerID = EnteredSelectable->GetOwnersID();
	GarrisonSelectableID = EnteredSelectable->GetSelectableID();
}

void FInfantryGarrisonStatus::ClearEnteredSelectable()
{
	GarrisonSelectableID = 0;
}


AInfantry::AInfantry()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->bReceivesDecals = false;
	GetCapsuleComponent()->SetGenerateOverlapEvents(false);
	GetCapsuleComponent()->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	GetCapsuleComponent()->SetCollisionProfileName(FName("Infantry"));

	GetMesh()->bReceivesDecals = false;
	GetMesh()->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	/* Leave collision to capsule */
	GetMesh()->SetCollisionProfileName(FName("NoCollision"));
	GetMesh()->SetGenerateOverlapEvents(false);

	HeldResourceMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Held Resources Mesh"));
	HeldResourceMesh->SetCollisionProfileName(FName("NoCollision"));
	HeldResourceMesh->SetGenerateOverlapEvents(false);
	HeldResourceMesh->SetCanEverAffectNavigation(false);
	HeldResourceMesh->bReceivesDecals = false;
	HeldResourceMesh->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	HeldResourceMesh->SetupAttachment(GetMesh());

	SelectionWidgetComponent = CreateDefaultSubobject<USelectableWidgetComponent>(TEXT("Selection Widget"));
	SelectionWidgetComponent->SetupAttachment(GetCapsuleComponent());

	PersistentWidgetComponent = CreateDefaultSubobject<USelectableWidgetComponent>(TEXT("Persistent Widget"));
	PersistentWidgetComponent->SetupAttachment(GetCapsuleComponent());

	SelectionDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("Selection Decal"));
	SelectionDecal->SetupAttachment(GetCapsuleComponent());
	SelectionDecal->bVisible = false;
	// So decal will not rotate when character rotates
	SelectionDecal->bAbsoluteRotation = true;
	SelectionDecal->bDestroyOwnerAfterFade = false;
	SelectionDecal->SetRelativeLocation(FVector(0.f, 0.f, -88.f));
	SelectionDecal->RelativeRotation = FRotator(-90.f, 0.f, 0.f);
	SelectionDecal->RelativeScale3D = FVector(0.1f, 0.3f, 0.3f);

	SelectionParticles = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("Selection Particles"));
	SelectionParticles->SetupAttachment(GetCapsuleComponent());
	SelectionParticles->bAutoActivate = false;
	SelectionParticles->bAbsoluteRotation = true;
	SelectionParticles->SetCollisionProfileName(FName("NoCollision"));
	SelectionParticles->SetGenerateOverlapEvents(false);
	SelectionParticles->SetCanEverAffectNavigation(false);
	SelectionParticles->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	SelectionParticles->bReceivesDecals = false;

	InitMovementDefaults();

	InitAnimationsForConstructor();

	bAlwaysRelevant = true;

	AIControllerClass = AInfantryController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	bInStealthMode = false;

	CreationMethod = ESelectableCreationMethod::Uninitialized;
	GameTickCountOnCreation = UINT8_MAX;

	Attributes.SetupBasicTypeInfo(ESelectableType::Unit, EBuildingType::NotBuilding, Type);

#if EXPERIENCE_ENABLED_GAME
	Rank = LevelingUpOptions::STARTING_LEVEL;
#endif
	ExperienceTowardsNextRank = 0.f;
	HeldResourceType = EResourceType::None;
	PreviousHeldResourceType = HeldResourceType;

	MoveAnimPlayRate = 1.f;

	bHasAttack = true;
	AttackFacingRotationRequirement = 10.f;
	bAllowRotationStraightAfterAttack = false;
}

void AInfantry::PostLoad()
{
	Super::PostLoad();

	/* Signal that game mode should call Setup() when our owner is known */
	bStartedInMap = true;
}

void AInfantry::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	InitStealthingVariables();
#if WITH_EDITOR
	if (GetWorld()->WorldType != EWorldType::EditorPreview)
#endif
	{
		/* Always spawn hidden and only consider revealing when we get a rep notify
		for player state */
		SetUnitInitialVisibility(false);
	}

	/* Held resources should be invisible on spawn since we aren't carrying any */
	HeldResourceMesh->SetStaticMesh(nullptr);

	SetupWeaponAttachment();
	InitContextCooldowns();

	// In case set to true in editor
	SelectionDecal->SetVisibility(false);
}

// Called when the game starts or when spawned
void AInfantry::BeginPlay()
{
	UE_CLOG(Type == EUnitType::None, RTSLOG, Fatal, TEXT("%s type is \"None\". Must change this "
		"to your own custom type"), *GetClass()->GetName());
	
	/* Game might actually run fine but not really expected behavior */
	assert(!Attributes.ResourceGatheringProperties.CanGatherResource(EResourceType::None));

	Super::BeginPlay();

	/* On clients Setup() will be called when owner (player state) reps */
	if (!GetWorld()->IsServer())
	{
		return;
	}

	GI = CastChecked<URTSGameInstance>(GetWorld()->GetGameInstance());

	/* Stop here if just spawning for faction info */
	if (GI->IsInitializingFactionInfo())
	{
		/* Pretty sure actor will never tick - is destroyed right after spawned */
		//SetActorTickEnabled(false);
		return;
	}

	/* If placed in map via editor then stop here. Usually true if using PIE and this was placed
	in map before PIE */
	if (bStartedInMap)
	{
		SetActorTickEnabled(false);
		return;
	}

	Setup();
}

void AInfantry::OnRep_ID()
{
	// See if we can setup now
	Attributes.IncrementNumReppedVariablesReceived();
	if (Attributes.CanSetup())
	{
		Setup();
	}
}

void AInfantry::OnRep_CreationMethod()
{
	// Assign duplicate variable
	Attributes.SetCreationMethod(CreationMethod);

	// See if we can setup now
	Attributes.IncrementNumReppedVariablesReceived();
	if (Attributes.CanSetup())
	{
		Setup();
	}
}

void AInfantry::OnRep_GameTickCountOnCreation()
{
	Attributes.IncrementNumReppedVariablesReceived();
	if (Attributes.CanSetup())
	{
		Setup();
	}
}

void AInfantry::OnRep_Owner()
{
	/* When testing in PIE with 2+ players this can actually get called before begin play. */

	/* if bStartedInMap then this not expected to be called since we destroy selectables already
	placed in map and respawn them */

	assert(!GetWorld()->IsServer());
	assert(GetOwner() != nullptr);
	/* Check we haven't called this before */
	assert(PS == nullptr);

	Super::OnRep_Owner();

	GI = CastChecked<URTSGameInstance>(GetWorld()->GetGameInstance());

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

void AInfantry::Setup()
{
	assert(CreationMethod != ESelectableCreationMethod::Uninitialized);

	SetupSelectionInfo();
	
	if (CreationMethod == ESelectableCreationMethod::StartingSelectable 
		|| CreationMethod == ESelectableCreationMethod::Spawned)
	{
		if (GetWorld()->IsServer() || Attributes.GetAffiliation() == EAffiliation::Owned)
		{
			/* Since queues handle incrementing housing consumption and we weren't built by a queue 
			we have to adjust his ourselves. */
			PS->AddHousingResourceConsumption(Type);
		}
	}

	if (!GetWorld()->IsServer())
	{
		// Already did on server, but wouldn't be wrong to do it anyway
		PS->Client_RegisterSelectableID(ID, this);
	}

	/* Important this happens before AdjustForUpgrades */
	SetDamageValues();

	/* Attributes only kept up to date for owning players. Stats should be
	starting stats for non-owners */
	/* TODO: Then
	once all this is done check upgrades are applying both client and server side
	for both already spawned and newly spawned units. Also try and figure out why
	upgrade progress does not show on clients */
	AdjustForUpgrades();

	SetupWidgetComponents();

	GS->OnInfantryBuilt(this, Attributes.GetTeam(), GetWorld()->IsServer());
	PS->OnUnitBuilt(this, Type, CreationMethod);

#if EXPERIENCE_ENABLED_GAME
	PreviousRank = Rank;
#endif

	/* Setup AI controller references and start behaviour */
	if (GetWorld()->IsServer())
	{
		Control->SetReferences(PS, GS, FI, &GS->GetTeamVisibilityInfo(Attributes.GetTeam()));
		Control->StartBehavior(BuildingSpawnedFrom);
	}
}

void AInfantry::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime); 

#if TICKABLE_BUFF_AND_DEBUFF_ENABLED_GAME

	/* This code should be fairly fast, but may want to look at the UI updated code see if 
	can make it faster */

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

	/* Might need to update the HUD durations on becoming selected. Should test this to see if it 
	is required or not. The logic could actually probably go in URTSHUD::OnPlayerCurrentSelectedChanged */

#endif // TICKABLE_BUFF_AND_DEBUFF_ENABLED_GAME
}

void AInfantry::SetupSelectionInfo()
{
	assert(Tags.Num() == 0);

	/* Assign owning player state reference */
	PS = CastChecked<ARTSPlayerState>(GetOwner());

	/* Assign game state reference */
	GS = CastChecked<ARTSGameState>(GetWorld()->GetGameState());

	/* Get local player controller, and get local player state after */
	APlayerController * const PlayerController = GetWorld()->GetFirstPlayerController();
	PC = CastChecked<ARTSPlayerController>(PlayerController);

	FI = GI->GetFactionInfo(PS->GetFaction());
	assert(FI != nullptr);

	ARTSPlayerState * const LocalPlayerState = PC->GetPS();
	/* Sometimes throws in PIE 2+ players when infantry are already placed on map. Reason is
	basically our own player state hasn't repped yet */
	assert(LocalPlayerState != nullptr);

	assert(PS->GetPlayerID() != FName());

	/* Add several tags to help identify what kind of unit this is. Some of this stuff can
	be moved to post.edit/ctor */
	Tags.Reserve(Statics::NUM_ACTOR_TAGS);
	Tags.Emplace(PS->GetPlayerID());
	Tags.Emplace(PS->GetTeamTag());
	Tags.Emplace(Statics::GetTargetingType(Attributes.GetTargetingType()));
	Tags.Emplace(Statics::NotAirTag);
	Tags.Emplace(Statics::UnitTag);
	Tags.Emplace(bHasAttack ? Statics::HasAttackTag : Statics::NotHasAttackTag);
	Tags.Emplace(Statics::AboveZeroHealthTag); 
	Tags.Emplace(Attributes.GetInventory().GetCapacity() > 0 ? Statics::HasInventoryWithCapacityGreaterThanZeroTag : Statics::HasZeroCapacityInventoryTag);
	assert(Tags.Num() == Statics::NUM_ACTOR_TAGS); // Make sure I did not forget one

	Attributes.SetupSelectionInfo(PS->GetPlayerID(), PS->GetTeam());

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
	else if (false /* TODO neutral */) { }
	else /* Assumed hostile */
	{
		Attributes.SetAffiliation(EAffiliation::Hostile);
	}

	PutAbilitiesOnInitialCooldown();

	SetupSelectionDecal(PC->GetFactionInfo());

	SetColorOnMaterials();

	/* TODO check: Make sure to set correct stealth material values. Assumed
	unit spawned with 'in stealth' values set */
	/* Set visibility */
	if (Attributes.GetAffiliation() == EAffiliation::Owned || Attributes.GetAffiliation() == EAffiliation::Allied
		|| Attributes.GetAffiliation() == EAffiliation::Observed)
	{
		SetUnitVisibility(true);
		Attributes.SetVisionState(EFogStatus::Revealed);
	}

	/* Set collision response to teams channel */
	const ECollisionChannel TeamCollisionChannel = GS->GetTeamCollisionChannel(Attributes.GetTeam());
	GetCapsuleComponent()->SetCollisionObjectType(TeamCollisionChannel);
}

void AInfantry::PossessedBy(AController * NewController)
{
	Super::PossessedBy(NewController);

	AssignController(NewController);
}

void AInfantry::PreReplication(IRepChangedPropertyTracker & ChangedPropertyTracker)
{
	Super::PreReplication(ChangedPropertyTracker);

	/* Only consider for replication if we have the ability to gather resources. This will be
	static but will it actually improve performance cause we're just checking delta of a uint8? 
	Eh it should because it's likely 1 if check vs NumConnections if checks */
	DOREPLIFETIME_ACTIVE_OVERRIDE(AInfantry, HeldResourceType,
		Attributes.ResourceGatheringProperties.IsCollector());
}

void AInfantry::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	/* When testing in PIE with 2+ players and having selectables start on map, both COND_Initial
	and COND_OwnerOnly don't want to work, so just repping all the time while with editor. RPC
	would be a more efficient alternative. BTW COND_OwnerOnly is not an option since
	now abilities use these */
#if WITH_EDITOR
	DOREPLIFETIME(AInfantry, ID);
#else
	DOREPLIFETIME_CONDITION(AInfantry, ID, COND_InitialOnly);
#endif

	DOREPLIFETIME_CONDITION(AInfantry, CreationMethod, COND_InitialOnly);
	/* Might have to make this COND_None just like ID if it isn't being sent */
	DOREPLIFETIME_CONDITION(AInfantry, GameTickCountOnCreation, COND_InitialOnly);
#if EXPERIENCE_ENABLED_GAME
	/* Would ideally like to make this variable never exist in the first place but UHT will not 
	allow that */
	DOREPLIFETIME_CONDITION(AInfantry, ExperienceTowardsNextRank, COND_OwnerOnly);
#endif
	DOREPLIFETIME_CONDITION(AInfantry, HeldResourceType, COND_Custom);
	DOREPLIFETIME(AInfantry, Health);
	DOREPLIFETIME(AInfantry, bInStealthMode);
	DOREPLIFETIME(AInfantry, AnimationState);
	DOREPLIFETIME(AInfantry, GarrisonStatus);
}

void AInfantry::InitContextCooldowns()
{
	for (const auto & Button : Attributes.GetContextMenu().GetButtonsArray())
	{
		ContextCooldowns.Emplace(Button, FTimerHandle());
	}
}

void AInfantry::OnCooldownFinished()
{
	/* Do nothing */
}

void AInfantry::ShowSelectionDecal()
{
	if (Attributes.GetSelectionDecalSetup() == ESelectionDecalSetup::UsesDynamicMaterial)
	{
		/* Set param value back to original */
		const FSelectionDecalInfo & DecalInfo = FI->GetSelectionDecalInfo(Attributes.GetAffiliation());
		if (true/*DecalInfo.IsParamNameValid()*/)
		{
			/* Was wrapped in if because I originally thought game would crash if param name
			isn't valid, but that is not the case */
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

void AInfantry::HideSelectionDecal()
{
	if (Attributes.GetSelectionDecalSetup() == ESelectionDecalSetup::UsesNonDynamicMaterial 
		|| Attributes.GetSelectionDecalSetup() == ESelectionDecalSetup::UsesDynamicMaterial)
	{
		SelectionDecal->SetVisibility(false);
	}
}

void AInfantry::ShowHoverDecal()
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

void AInfantry::PutAbilitiesOnInitialCooldown()
{
	// Only server or owner does this although nothing wrong with everyone doing it
	if (GetWorld()->IsServer() || Attributes.GetAffiliation() == EAffiliation::Owned)
	{
		for (const auto & Button : Attributes.GetContextMenu().GetButtonsArray())
		{
			const FContextButtonInfo & AbilityInfo = GI->GetContextInfo(Button.GetButtonType());
			if (AbilityInfo.GetInitialCooldown() > 0.f)
			{
				// Put on cooldown
				Delay(ContextCooldowns[Button], &AInfantry::OnCooldownFinished, AbilityInfo.GetInitialCooldown());
			}
		}
	}
}

void AInfantry::SetupSelectionDecal(AFactionInfo * LocalPlayersFactionInfo)
{
	/* Set selection decal */

	const FSelectionDecalInfo * DecalInfo;

	if (Attributes.GetAffiliation() == EAffiliation::Observed)
	{
		/* Get a decal from game instance since observers aren't part of a faction */
		DecalInfo = &GI->GetObserverSelectionDecal();
	}
	else
	{
		DecalInfo = &LocalPlayersFactionInfo->GetSelectionDecalInfo(Attributes.GetAffiliation());
	}

	SelectionDecal->SetDecalMaterial(DecalInfo->GetDecal());

	/* Null check because calling CreateDynamicMaterialInstance() will still do something even 
	if the set material is null */
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

void AInfantry::SetColorOnMaterials()
{
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

	/* Check if dynamic material instances have already been created due to this unit being
	a stealth unit. */
	if (DynamicMaterialInstances.Num() > 0)
	{
		for (const auto & Elem : DynamicMaterialInstances)
		{
			Elem->SetVectorParameterValue(SelectableColoringOptions::ParamName, Color);
		}
	}
	else
	{
		/* Need to create dynamic material instances. But in this case we can actually get
		away with UMaterialInstanceConstant instead, but they must be created when
		WITH_EDITOR. Since buildings cannot go stealth I will likely do this for buildings
		so check their code for how it is done */

		/* Trying to do it similar to how InitStealthingVariables does it. */

		const int32 NumMeshMaterials = GetMesh()->GetNumMaterials();
		const int32 NumWeaponMaterials = GetNumWeaponMeshMaterials();
		DynamicMaterialInstances.Reserve(NumMeshMaterials + NumWeaponMaterials);
		for (int32 i = 0; i < NumMeshMaterials; ++i)
		{
			UMaterialInstanceDynamic * MID = UMaterialInstanceDynamic::Create(GetMesh()->GetMaterial(i), GetMesh());

			// Don't forget to actually set team color
			MID->SetVectorParameterValue(SelectableColoringOptions::ParamName, Color);

			GetMesh()->SetMaterial(i, MID);
			DynamicMaterialInstances.Emplace(MID);
		}
		for (int32 i = 0; i < NumWeaponMaterials; ++i)
		{
			/* Commenting so compile */
			//UMaterialInstanceDynamic * MID = UMaterialInstanceDynamic::Create(WeaponMesh->GetMaterial(i), WeaponMesh);
			//
			//// Don't forget to actually set team color
			//MID->SetVectorParameterValue(SelectableColoringOptions::ParamName, Color);
			//
			//WeaponMesh->SetMaterial(i, MID);
			//DynamicMaterialInstances.Emplace(MID);
		}
	}
}

void AInfantry::GainExperience(float EnemyBounty, bool bApplyRandomness)
{
#if EXPERIENCE_ENABLED_GAME

	SERVER_CHECK;
	assert(EnemyBounty >= 0.f);

	if (CanClassGainExperience() == false)
	{
		return;
	}

	/* If max level do nothing */
	if (Rank == LevelingUpOptions::MAX_LEVEL)
	{
		return;
	}

	const float RandomMultiplier = bApplyRandomness ? FMath::RandRange(1.f - LevelingUpOptions::SELECTABLE_EXPERIENCE_GAIN_RANDOM_FACTOR, 1.f + LevelingUpOptions::SELECTABLE_EXPERIENCE_GAIN_RANDOM_FACTOR) : 1.f;
	const float ExperienceGain = (EnemyBounty * Attributes.GetExperienceGainMultiplier() * RandomMultiplier);

	ExperienceTowardsNextRank += ExperienceGain;
	/* Figure out how many levels we gained */
	while (true)
	{
		const float ExpReqForNextRank = GetTotalExperienceRequiredForNextLevel();
		if (ExperienceTowardsNextRank >= ExpReqForNextRank)
		{
			/* Have leveled up */
			Rank++;
			ExperienceTowardsNextRank -= ExpReqForNextRank;
		}
		else
		{
			break;
		}

		if (Rank == LevelingUpOptions::MAX_LEVEL)
		{
			break;
		}
	}

	const uint8 NumLevelsGained = Rank - PreviousRank;
	if (NumLevelsGained > 0)
	{
		/* Apply bonuses for each level up making sure not to skip one if leveled up
		more than one level */
		for (uint8 i = PreviousRank + 1; i <= Rank; ++i)
		{
			ApplyLevelUpBonuses(i);

			/* Increase how much experience we are worth to the enemy because we're a higher level 
			now */
			Attributes.SetExperienceBounty(Attributes.GetExperienceBounty() * GI->GetExperienceBountyMultiplierPerLevel());
		}

		const FRankInfo & NewLevelsInfo = FI->GetLevelUpInfo(Rank);

		/* Play particle system */
		PlayLevelUpParticles(NewLevelsInfo);

		/* Play sound */
		PlayLevelUpSound(NewLevelsInfo);

		/* Optional on server */
		PreviousRank = Rank;

		// Tell HUD about ranking up
		if (Attributes.IsSelected())
		{
			PC->GetHUDWidget()->Selected_OnRankChanged(this, ExperienceTowardsNextRank, Rank,
				GetTotalExperienceRequiredForNextLevel(), Attributes.IsPrimarySelected());
		}

		GS->Multicast_OnSelectableLevelUp(GetSelectableID(), GetOwnersID(), Rank);
	}
	else
	{	
		// Tell HUD about EXP gain
		if (Attributes.IsSelected())
		{
			PC->GetHUDWidget()->Selected_OnCurrentRankExperienceChanged(this, ExperienceTowardsNextRank,
				Rank, GetTotalExperienceRequiredForNextLevel(), Attributes.IsPrimarySelected());
		}
	}

#endif // EXPERIENCE_ENABLED_GAME
}

uint8 AInfantry::GetOwnersID() const
{
	return PS->GetPlayerIDAsInt();
}

void AInfantry::RegenSelectableResourcesFromGameTicks(uint8 NumGameTicksWorth, URTSHUD * HUDWidget, 
	ARTSPlayerController * LocalPlayerController)
{
	assert(Attributes.HasASelectableResourceThatRegens());

	Attributes.GetSelectableResource_1().RegenFromGameTicks(NumGameTicksWorth, HUDWidget, this,
		LocalPlayerController, PersistentWidgetComponent, SelectionWidgetComponent);
}

bool AInfantry::HasEnoughSelectableResources(const FContextButtonInfo & AbilityInfo) const
{
	return Attributes.GetSelectableResource_1().GetAmount() >= AbilityInfo.GetSelectableResourceCost_1();
}

void AInfantry::ApplyLevelUpBonuses(uint8 Level)
{
	/* Apply bonus from leveling up if any */
	UUpgradeEffect * Bonus = FI->GetLevelUpBonus(Level);
	if (Bonus != nullptr)
	{
		Bonus->ApplyEffect_Infantry(this, false);
	}
}

void AInfantry::PlayLevelUpParticles(const FRankInfo & RankInfo)
{
	/* TODO: Make sure particle is affected by fog of war */
	/* Spawn particle effect just at actor location for now if any */
	if (RankInfo.GetParticles() != nullptr)
	{
		Statics::SpawnFogParticles(RankInfo.GetParticles(), GS, GetActorLocation(), GetActorRotation(),
			FVector::OneVector);
	}
}

void AInfantry::PlayLevelUpSound(const FRankInfo & RankInfo)
{
	if (RankInfo.GetSound() != nullptr)
	{
		Statics::SpawnSoundAttached(RankInfo.GetSound(), GetRootComponent(), ESoundFogRules::InstigatingTeamOnly);
	}
}

void AInfantry::EnterStealthMode()
{
	SERVER_CHECK;
	assert(!bInStealthMode);

	bInStealthMode = true;
	OnRep_bInStealthMode();

	// Tell AI controller about change
	Control->OnUnitEnterStealthMode();
}

void AInfantry::OnRep_bInStealthMode()
{
	/* Can be called when coming out of stealth so things like sounds shouldn't be
	played in here without first checking if that was the case */

	if (bInStealthMode)
	{
		/* Have entered stealth mode */

		/* Change transparency */
		for (int32 i = 0; i < DynamicMaterialInstances.Num(); ++i)
		{
			UMaterialInstanceDynamic * MID = DynamicMaterialInstances[i];
			MID->SetScalarParameterValue(Attributes.StealthMaterialParamName,
				Attributes.GetInStealthParamValue());
		}

		if (Attributes.GetAffiliation() == EAffiliation::Hostile)
		{
			if (Attributes.GetVisionState() == EFogStatus::Hidden
				|| Attributes.GetVisionState() == EFogStatus::Revealed)
			{
				/* Want to avoid setting the visibility of the unit if it is inside a garrison. 
				Actually setting vis to false here probably makes no difference since it's 
				probably already not visible - it's the true case that matters */
				if (IsInsideGarrison() == false)
				{
					SetUnitVisibility(false);
				}
			}
		}
	}
	else
	{
		/* Have exited stealth mode */

		/* Change transparency */
		for (int32 i = 0; i < DynamicMaterialInstances.Num(); ++i)
		{
			UMaterialInstanceDynamic * MID = DynamicMaterialInstances[i];
			MID->SetScalarParameterValue(Attributes.StealthMaterialParamName,
				OutOfStealthParamValues[i]);
		}

		if (Attributes.GetAffiliation() == EAffiliation::Hostile)
		{
			// If Revealed or StealthRevealed
			if (static_cast<uint8>(Attributes.GetVisionState()) & 0x01)
			{
				/* Want to avoid setting the visibility of the unit if it is inside a garrison */
				if (IsInsideGarrison() == false)
				{
					SetUnitVisibility(true);
				}
			}
		}
	}
}

void AInfantry::OnRep_GarrisonStatus()
{
	/* Just checking this. By rules of replication it should hold but if it doesn't then 
	you can remove it, and optionally for performance maybe make the first if statement: 
	if (ClientPreviousGarrisonStatus.IsInsideGarrison() && ClientPreviousGarrisonStatus != GarrisonStatus) */
	assert(ClientPreviousGarrisonStatus != GarrisonStatus);

	/* Check if we had unit down as being inside something before this update came in */
	if (ClientPreviousGarrisonStatus.IsInsideGarrison())
	{
		/* Remove the unit from whatever we had it in previously */
		AActor * PreviouslyEnteredActor = GS->GetPlayerFromID(ClientPreviousGarrisonStatus.GarrisonOwnerID)->GetSelectableFromID(ClientPreviousGarrisonStatus.GarrisonSelectableID);
		if (Statics::IsValid(PreviouslyEnteredActor))
		{
			ABuilding * PreviouslyEnteredBuilding = CastChecked<ABuilding>(PreviouslyEnteredActor);
			PreviouslyEnteredBuilding->GetBuildingAttributesModifiable()->GetGarrisonAttributes().ClientOnUnitExited(GS, PreviouslyEnteredBuilding, this, PC->GetHUDWidget());
		}
	}

	/* Now process GarrisonStatus */
	if (GarrisonStatus.IsInsideGarrison()) 
	{
		SetUnitCollisionEnabled(false);
		SetUnitVisibility(false);
		GetMesh()->SetComponentTickEnabled(false);

		AActor * EnteredBuildingActor = GS->GetPlayerFromID(GarrisonStatus.GarrisonOwnerID)->GetSelectableFromID(GarrisonStatus.GarrisonSelectableID);
		if (Statics::IsValid(EnteredBuildingActor))
		{
			ABuilding * EnteredBuilding = CastChecked<ABuilding>(EnteredBuildingActor);
			EnteredBuilding->GetBuildingAttributesModifiable()->GetGarrisonAttributes().ClientOnUnitEntered(GS, EnteredBuilding, this, PC->GetHUDWidget());
		}

		/* This should amoung possibly other things deselect the unit if it is selected */
		PC->NotifyOfInfantryEnteringGarrison(this, Attributes.IsSelected());
	}
	else 
	{
		GetMesh()->SetComponentTickEnabled(true);
		SetUnitCollisionEnabled(true);
		{
			/* The code in this block is pretty much optional. We can just let the fog manager
			reveal the unit on it's next tick if we like */

			/* Set the unit's fog status based on the visibility status of the tile they are now on */
			Attributes.SetVisionState(Statics::GetLocationVisionStatusLocally(GetActorLocation(), GS->GetFogManager()));
			if (Attributes.GetVisionState() == EFogStatus::StealthRevealed
				|| (Attributes.GetVisionState() == EFogStatus::Revealed && bInStealthMode == false))
			{
				SetUnitVisibility(true);
			}
		}
	}

	ClientPreviousGarrisonStatus = GarrisonStatus;

	/* This is just a general message. Make sure to make sure fog manager takes into 
	account infantry inside garrisons, and that the correct vision radius for the 
	garrisons is used */
}

void AInfantry::OnRep_HeldResourceType()
{
	// TODO need to reveal/hide these with fog of war

	/* Show/hide held resource mesh */
	if (HeldResourceType != EResourceType::None)
	{
		const FGatheredResourceMeshInfo & MeshInfo = Attributes.ResourceGatheringProperties.GetGatheredMeshProperties(HeldResourceType);
		if (MeshInfo.GetMesh() != nullptr)
		{
			FAttachmentTransformRules AttachmentRules = FAttachmentTransformRules(MeshInfo.GetAttachmentRule(),
				false);

			HeldResourceMesh->AttachToComponent(GetMesh(), AttachmentRules, MeshInfo.GetSocketName());
			HeldResourceMesh->SetRelativeLocationAndRotation(MeshInfo.GetAttachTransform().GetLocation(),
				MeshInfo.GetAttachTransform().GetRotation());
			HeldResourceMesh->SetStaticMesh(MeshInfo.GetMesh());
		}
		else
		{
			/* No mesh assigned for this resource type. Show no mesh */
			HeldResourceMesh->SetStaticMesh(nullptr);
		}
	}
	else
	{
		/* Not holding a resource. Show no mesh */
		HeldResourceMesh->SetStaticMesh(nullptr);
	}

	/* Set movement speed. 3 different situations here:
	- going from none to something
	- going from something to none
	- going from something to something else */
	if (PreviousHeldResourceType == EResourceType::None)
	{
		ApplyTempMoveSpeedMultiplier(GetMoveSpeedMultiplierForHoldingResources());
	}
	else if (HeldResourceType == EResourceType::None)
	{
		RemoveMoveSpeedEffectOfResources();
	}
	else
	{
		/* Changing resource type. No need to do anything */
	}

	PreviousHeldResourceType = HeldResourceType;
}

void AInfantry::OnRep_ExperienceTowardsNextRank()
{
#if EXPERIENCE_ENABLED_GAME
	/* If we ranked up too then we'll double up on HUD updates but oh well */
	if (Attributes.IsSelected())
	{
		PC->GetHUDWidget()->Selected_OnCurrentRankExperienceChanged(this, ExperienceTowardsNextRank,
			Rank, GetTotalExperienceRequiredForNextLevel(), Attributes.IsPrimarySelected());
	}
#else
	UE_LOG(RTSLOG, Fatal, TEXT("Gained experience even though EXPERIENCE_ENABLED_GAME is set to false"));
#endif
}

void AInfantry::SetUnitInitialVisibility(bool bInitialVisibility)
{
	GetCapsuleComponent()->SetVisibility(bInitialVisibility);
	GetMesh()->SetVisibility(bInitialVisibility);
	PersistentWidgetComponent->SetVisibility(bInitialVisibility);
	SelectionWidgetComponent->SetVisibility(bInitialVisibility);
	HeldResourceMesh->SetVisibility(bInitialVisibility);
}

void AInfantry::SetUnitVisibility(bool bNewVisibility)
{
	/* Root component (capsule) is not visible in the first place but is
	its bVisible is used to determine if it is in fog of war or not hence why its visibility
	is changed */
	GetCapsuleComponent()->SetVisibility(bNewVisibility);
	GetMesh()->SetVisibility(bNewVisibility);
	PersistentWidgetComponent->SetVisibility(bNewVisibility);
	// SelectionWidgetComponent too?
	HeldResourceMesh->SetVisibility(bNewVisibility);
}

void AInfantry::SetUnitCollisionEnabled(bool bCollisionEnabled)
{
	GetCapsuleComponent()->SetCollisionEnabled(bCollisionEnabled ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
}

void AInfantry::OnEnterFogOfWar()
{	
	// Make unselectable
	GetCapsuleComponent()->SetVisibility(false);

	if (Attributes.IsSelected())
	{
		OnDeselect();

		/* Because owned/allied units are always visible this can only be called on hostile units.
		Won't bother updating the players Selected or CurrentSelected. Instead just tell HUD
		directly. Also because only one hostile selectable can be selected max then this func
		call is valid */
		PC->GetHUDWidget()->OnPlayerNoSelection();
	}

	GetMesh()->SetVisibility(false);
	PersistentWidgetComponent->SetVisibility(false);
	HeldResourceMesh->SetVisibility(false);

	// Attached particles
	for (const auto & Elem : Attributes.AttachedParticles)
	{
		Elem.GetPSC()->SetVisibility(false);
	}
}

void AInfantry::OnExitFogOfWar()
{
	GetMesh()->SetVisibility(true);
	PersistentWidgetComponent->SetVisibility(true);
	HeldResourceMesh->SetVisibility(true);

	// Attached particles
	for (const auto & Elem : Attributes.AttachedParticles)
	{
		Elem.GetPSC()->SetVisibility(true);
	}

	// Make selectable
	GetCapsuleComponent()->SetVisibility(true);
}

void AInfantry::AssignController(AController * NewController)
{
	Control = CastChecked<AInfantryController>(NewController);
}

void AInfantry::InitStealthingVariables()
{
	if (Attributes.bCanEverEnterStealth)
	{
		const bool bStartInStealthMode = Attributes.ShouldSpawnStealthed();
		
		/* Assuming that just the body mesh and weapon mesh are the only two visible
		mesh components for this. If upgrades add more meshes then their materials
		will need to be iterated over here aswell */
		OutOfStealthParamValues.Reserve(GetMesh()->GetNumMaterials() + GetNumWeaponMeshMaterials());
		DynamicMaterialInstances.Reserve(GetMesh()->GetNumMaterials() + GetNumWeaponMeshMaterials());
		for (int32 i = 0; i < GetMesh()->GetNumMaterials(); ++i)
		{
			UMaterialInterface * Material = GetMesh()->GetMaterial(i);

			/* Cache original value of param. Assuming here the user left the material's state in 
			editor as the 'out of stealth' version. */
			float OriginalParamValue;
			Material->GetScalarParameterValue(Attributes.StealthMaterialParamName, OriginalParamValue);
			OutOfStealthParamValues.Emplace(OriginalParamValue);

			/* Create dynamic material */
			UMaterialInstanceDynamic * MID = UMaterialInstanceDynamic::Create(Material, this);
			
			/* Set MID param value to stealth value if required */
			if (bStartInStealthMode)
			{
				MID->SetScalarParameterValue(Attributes.StealthMaterialParamName, Attributes.InStealthParamValue);
			}
			
			GetMesh()->SetMaterial(i, MID);
			DynamicMaterialInstances.Emplace(MID);
		}
		for (int32 i = 0; i < GetNumWeaponMeshMaterials(); ++i)
		{
			/* Commenting so compile */
			//UMaterialInterface * Material = WeaponMesh->GetMaterial(i);
			//
			///* Cache original value of param. Assuming here the user left the material's state in
			//editor as the 'out of stealth' version. */
			//float OriginalParamValue;
			//Material->GetScalarParameterValue(Attributes.StealthMaterialParamName, OriginalParamValue);
			//OutOfStealthParamValues.Emplace(OriginalParamValue);
			//
			///* Create dynamic material */
			//UMaterialInstanceDynamic * MID = UMaterialInstanceDynamic::Create(Material, this);
			//
			///* Set MID param value to stealth value if required */
			//if (bStartInStealthMode)
			//{
			//	MID->SetScalarParameterValue(Attributes.StealthMaterialParamName, Attributes.InStealthParamValue);
			//}
			//
			//WeaponMesh->SetMaterial(i, MID);
			//DynamicMaterialInstances.Emplace(MID);
		}
	}
}

void AInfantry::SetupWeaponAttachment()
{
	//WeaponMesh->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform,
	//	WeaponSocket);
}

void AInfantry::SetupWidgetComponents()
{
	SelectionWidgetComponent->SetWidgetClassAndSpawn(this, FI->GetSelectableSelectionWorldWidget(),
		Health, Attributes, GI);
	PersistentWidgetComponent->SetWidgetClassAndSpawn(this, FI->GetSelectablePersistentWorldWidget(),
		Health, Attributes, GI);
}

void AInfantry::OnRep_AnimationState()
{
	/* Cancel the 'Attack preparation' sound if it is playing */
	if (AttackPreparationAudioComp != nullptr)
	{
		AttackPreparationAudioComp->Stop();
	}
	
	UAnimMontage * Montage = Animations[AnimationState.AnimationType];

	if (Montage != nullptr)
	{
		/* We do not handle very well the case where the anim's start time is actually in the
		future - we just stat playing it now if that's the case */
		float StartTime = GS->GetServerWorldTimeSeconds() - AnimationState.StartTime;
		StartTime = FMath::Fmod(StartTime, Montage->SequenceLength); 

		/* Interesting fact: Montage_play will clamp the start time to >= 0 and <= Montage->SequenceLength. 
		However if the start time is clamped to Montage->SequenceLength then the montage will just 
		be frozen on a single frame. Maybe the fact that it is looping has something to do with it. 
		A "solution" is to clamp it before passing it in to Montage_Play and subtract a really 
		small amount from it if it does end up being Montage->SequenceLength. I assume that MAYBE 
		this means our animation plays one frame behind (MAYBE, would have to check engine source) */
		GetMesh()->GetAnimInstance()->Montage_Play(Montage, GetAnimPlayRate(AnimationState.AnimationType),
			EMontagePlayReturnType::MontageLength, StartTime);
	}
}

void AInfantry::OnInventoryItemUseCooldownFinished(uint8 ServerInventorySlotIndex)
{
	if (Attributes.IsSelected())
	{
		const uint8 DisplayIndex = GetInventory()->GetLocalIndexFromServerIndex(ServerInventorySlotIndex);
		
		/* Let HUD know */
		PC->GetHUDWidget()->Selected_OnInventoryItemUseCooldownFinished(this, DisplayIndex,
			Attributes.IsPrimarySelected());
	}
}

int32 AInfantry::GetNumWeaponMeshMaterials() const
{
	// 0 cause I removed the weapon mesh
	return 0; //WeaponMesh->GetNumMaterials();
}

void AInfantry::ServerEnterBuildingGarrison(ABuilding * Building, FBuildingGarrisonAttributes & GarrisonAttributes)
{
	SERVER_CHECK;
	assert(GarrisonAttributes.HasEnoughCapacityToAcceptUnit(this));

	SetUnitCollisionEnabled(false);
	SetUnitVisibility(false);
	GetMesh()->SetComponentTickEnabled(false);
	/* Setting actor location so the AI controller will sweep from the correct location. 
	If I ever change how sweep location is decided on the AI controller i.e. instead of 
	always doing GetActorLocation() it does if (Unit->IsInGarrison()) SomeLocation else... 
	then this may not be required */
	SetActorLocation(GetActorLocationForGarrison(Building));

	GarrisonStatus.SetEnteredSelectable(Building);
	GarrisonAttributes.ServerOnUnitEntered(GS, Building, this, PC->GetHUDWidget());

	/* Set as 'not visible' for all the team's visibility info. Reason for doing this is 
	well the unit kind of is I guess garrisons have windows though. Second reason is that 
	it will now make AI controllers fail their IsUnitAttackable tests making them unaquire 
	this unit. 
	If this just isn't good enough then instead each infantry might have to keep track of 
	everything that is targeting it and make them all unaquire it when it enters a garrison */
	const uint8 IndexToAvoid = Statics::TeamToArrayIndex(Attributes.GetTeam());
	TArray<FVisibilityInfo> & VisInfos = GS->GetAllTeamsVisibilityInfo();
	for (int32 i = 0; i < VisInfos.Num(); ++i)
	{
		/* Skip our own team cause we don't put our own units into it and therefore will get a
		TMap::operator[] crash */
		if (i != IndexToAvoid)
		{
			VisInfos[i].SetVisibility(this, false); 
		}
	}
	
	PC->NotifyOfInfantryEnteringGarrison(this, Attributes.IsSelected());
}

FVector AInfantry::GetActorLocationForGarrison(ABuilding * GarrisonedBuilding) const
{
	return GarrisonedBuilding->GetActorLocation();
}

void AInfantry::ServerEvacuateGarrison(const FVector & EvacLocation, const FQuat & EvacRotation)
{
	GarrisonStatus.ClearEnteredSelectable();
	SetActorLocationAndRotation(EvacLocation, EvacRotation);
	GetMesh()->SetComponentTickEnabled(true);
	SetUnitCollisionEnabled(true);
	{
		/* The code in this block is pretty much optional. We can just let the fog manager 
		reveal the unit on it's next tick if we like */
		
		/* Set the unit's fog status based on the visibility status of the tile they are now on */
		Attributes.SetVisionState(Statics::GetLocationVisionStatusLocally(EvacLocation, GS->GetFogManager()));
		if (Attributes.GetVisionState() == EFogStatus::StealthRevealed
			|| (Attributes.GetVisionState() == EFogStatus::Revealed && bInStealthMode == false))
		{
			SetUnitVisibility(true);
			/* I don't do anything with TeamVisibilityInfo here. Perhaps I should. We have a
			timeframe where the unit is visible but the vis info says it might not be which 
			is inconsistent. But it would only be the local player's TeamVisibilityInfo, cbf 
			doing it for all teams, let fog manager do that on it's next tick */
		}
	}

	Control->OnUnitExitGarrison();
}

void AInfantry::Delay(FTimerHandle & TimerHandle, void(AInfantry::* Function)(), float Delay)
{
	assert(Delay > 0.f);
	GetWorldTimerManager().SetTimer(TimerHandle, this, Function, Delay, false);
}

void AInfantry::Delay(void(AInfantry::* Function)(), float Delay)
{
	FTimerHandle TimerHandle_Dummy;
	this->Delay(TimerHandle_Dummy, Function, Delay);
}

void AInfantry::InitAnimationsForConstructor()
{
	Animations.Reserve(Statics::NUM_ANIMATIONS);
	for (int32 i = 0; i < Statics::NUM_ANIMATIONS; ++i)
	{
		EAnimation AnimationType = Statics::ArrayIndexToAnimation(i);
		Animations.Add(AnimationType, nullptr);
	}
}

void AInfantry::InitMovementDefaults()
{
	// Code to handle facing direction

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	// Rate unit rotates
	GetCharacterMovement()->RotationRate = FRotator(360.f, 360.f, 360.f);
	GetCharacterMovement()->bOrientRotationToMovement = true;

	/* All the code I use to make AI avoid bumping into each other */

	GetCharacterMovement()->bUseRVOAvoidance = true;
	GetCharacterMovement()->AvoidanceWeight = 0.5f;
	// This makes quite a difference
	GetCharacterMovement()->AvoidanceConsiderationRadius = 200.f;
	GetCharacterMovement()->DefaultLandMovementMode = EMovementMode::MOVE_NavWalking;
}

#if WITH_EDITOR
void AInfantry::PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	AtkAttr.OnPostEdit();

	Attributes.OnPostEdit(this);

	bInStealthMode = Attributes.bCanEverEnterStealth && Attributes.ShouldSpawnStealthed();

	bCanEditHumanOwnerIndex = !PIE_bIsForCPUPlayer;

	/* If have edited move speed make sure default move speed variable updates too */
	Internal_SetNewDefaultMoveSpeed(GetMoveSpeed());

	// Set our starting move speed
	GetCharacterMovement()->MaxSwimSpeed = GetMoveSpeed();

	const FName EditedPropertyName = PropertyChangedEvent.GetPropertyName();
	if (EditedPropertyName == FName("Mesh"))
	{
		/* All this blueprint creation code works, but right now I'm still just 
		going to use montages so have commented it */

		//if (GetMesh()->GetAnimationMode() == EAnimationMode::AnimationBlueprint)
		//{
		//	/* The mesh has changed. Set anim instance. I will set it to one of my auto-generated animBPs */
		//	
		//	/* This code here will either find the anim blueprint asset or create it  */
		//	UClass * BlueprintClass = nullptr;
		//	UClass * BlueprintGeneratedClass = nullptr;
		//	IKismetCompilerInterface & KismetCompilerModule = FModuleManager::LoadModuleChecked<IKismetCompilerInterface>("KismetCompiler");
		//	KismetCompilerModule.GetBlueprintTypesForClass(UInfantryAnimInstance::StaticClass(), BlueprintClass, BlueprintGeneratedClass);
		//	const FString BlueprintName = (GetMesh()->SkeletalMesh->Skeleton->GetName() + "_AnimBP_AutoGenerated");
		//	FString UserPackageName = FString("/Game/AutoGeneratedRTSAssets/AnimationBlueprints/") + BlueprintName;
		//	FName BPName(*FPackageName::GetLongPackageAssetName(UserPackageName));
		//	UPackage * Package = CreatePackage(NULL, *UserPackageName);
		//	/* Check if the blueprint already exists */
		//	UBlueprint * AnimBlueprint = FindObject<UBlueprint>(Package, *BPName.ToString());
		//	if (AnimBlueprint == nullptr)
		//	{
		//		AnimBlueprint = FKismetEditorUtilities::CreateBlueprint(UInfantryAnimInstance::StaticClass(), Package, BPName, BPTYPE_Normal, BlueprintClass, BlueprintGeneratedClass, FName("LevelEditorActions"));
		//		assert(AnimBlueprint != nullptr);
		//		// Notify the asset registry
		//		FAssetRegistryModule::AssetCreated(AnimBlueprint);
		//		// Mark the package dirty...
		//		Package->MarkPackageDirty();
		//	}
		//
		//	/* Set the blueprint */
		//	GetMesh()->SetAnimInstanceClass(CastChecked<UClass>(AnimBlueprint->GeneratedClass));
		//
		//	/* TODO at some point before packaging make sure to erase unused autogenerated animBPs.
		//	so packaged game isn't unneccessarily larger than it needs to be. Not sure how I will
		//	do that. */
		//}
	}

	/* TODO: Bear in mind this only fires on a struct edit, not non-struct edits. May need to revise
	if that still gets the job done or not but I think it's ok */
}
#endif

void AInfantry::SetDamageValues()
{
	const FUnitInfo * InfoAboutUs = FI->GetUnitInfo(Type);

	AtkAttr.SetDamageValues(InfoAboutUs);
}

void AInfantry::AdjustForUpgrades()
{
	PS->GetUpgradeManager()->ApplyAllCompletedUpgrades(this);

	/* Make sure health starts at max if max was changed. Users are free to modify max health 
	and leave current health unchanged so that is why this is here */
	Health = Attributes.GetMaxHealth();
}

//#if WITH_EDITOR
EUnitType AInfantry::GetType() const
{
	return Type;
}
//#endif

bool AInfantry::IsStartingSelectable() const
{
	return (CreationMethod == ESelectableCreationMethod::StartingSelectable);
}

bool AInfantry::CheckAnimationProperties() const
{
#if UE_BUILD_SHIPPING
	return true;
#else
	const bool bAnimInstanceNull = (GetMesh()->GetAnimInstance() == nullptr);
	if (bAnimInstanceNull)
	{
		UE_LOG(RTSLOG, Fatal, TEXT("Unit \"%s\" does not have an animation blueprint set"),
			*GetClass()->GetName());
		
		return false;
	}
	
	const bool bAnimInstanceClassOk = GetMesh()->GetAnimInstance()->IsA(UInfantryAnimInstance::StaticClass());

	UE_CLOG(!bAnimInstanceClassOk, RTSLOG, Fatal, TEXT("Unit \"%s\" has anim instance \"%s\". This "
		"anim instance needs to be reparented to a UInfantryAnimInstance otherwise the RTS anim "
		"notifies will not trigger"), *GetClass()->GetName(), *GetMesh()->GetAnimInstance()->GetName());

	// Check all anim notifies we expect to be on the montages are there
	FString ErrorMessage;
	bool AllAnimNotifiesPresent = true;
	for (const auto & Elem : Animations)
	{
		// A lot of animations won't be used by some infantry so null check
		if (Elem.Value != nullptr)
		{
			AllAnimNotifiesPresent = VerifyAllAnimNotifiesArePresentOnMontage(Elem.Key, Elem.Value, ErrorMessage);
			if (AllAnimNotifiesPresent == false)
			{
				UE_LOG(RTSLOG, Fatal, TEXT("Unit [%s] with anim montage [%s] is missing anim notify [%s]. "
					"Please add this notify to the montage."), *GetClass()->GetName(),
					*Elem.Value->GetName(), *ErrorMessage);
				break;
			}
		}
	}

	return bAnimInstanceClassOk && AllAnimNotifiesPresent;
#endif
}

bool AInfantry::IsAnimationAssigned(EAnimation AnimType) const
{
	return Animations[AnimType] != nullptr;
}

#if !UE_BUILD_SHIPPING
bool AInfantry::VerifyAllAnimNotifiesArePresentOnMontage(EAnimation AnimType, UAnimMontage * Montage, FString & OutErrorMessage)
{
	switch (AnimType)
	{
		// Animation types that don't need anim notifies
		case EAnimation::Idle:
		case EAnimation::Moving:
		case EAnimation::MovingWithResources:
		case EAnimation::GatheringResources:
		case EAnimation::ConstructBuilding:
		{
			return true;
		}
	
		case EAnimation::Attack:
		{
			FString FireWeaponFuncName = GET_FUNCTION_NAME_STRING_CHECKED(UInfantryAnimInstance, AnimNotify_FireWeapon);
			// Remove the "AnimNotify_" part at the start
			FireWeaponFuncName = FireWeaponFuncName.RightChop(11);
			static const FName FireWeaponName = FName(*FireWeaponFuncName);

			FString AnimFinishedFuncName = GET_FUNCTION_NAME_STRING_CHECKED(UInfantryAnimInstance, AnimNotify_OnAttackAnimationFinished);
			// Remove the "AnimNotify_" part at the start
			AnimFinishedFuncName = AnimFinishedFuncName.RightChop(11);
			static const FName AnimFinishedName = FName(*AnimFinishedFuncName);

			// What about AnimNotify_PlayAttackPreparationSound? Is that required?

			bool bFoundFireWeaponNotify = false;
			bool bFoundAnimFinishedNotify = false;
			for (const auto & Elem : Montage->Notifies)
			{
				if (Elem.NotifyName == FireWeaponName)
				{
					bFoundFireWeaponNotify = true;
				}
				if (Elem.NotifyName == AnimFinishedName)
				{
					bFoundAnimFinishedNotify = true;
				}
			}

			if (bFoundFireWeaponNotify && bFoundAnimFinishedNotify)
			{
				return true;
			}
			else
			{
				if (bFoundFireWeaponNotify)
				{
					OutErrorMessage = AnimFinishedFuncName;
				}
				else if (bFoundAnimFinishedNotify)
				{
					OutErrorMessage = FireWeaponFuncName;
				}
				else
				{
					OutErrorMessage = FireWeaponFuncName + " and " + AnimFinishedFuncName;
				}
				
				return false;
			}
		}

		case EAnimation::Destroyed:
		{
			FString NotifyFuncName = GET_FUNCTION_NAME_STRING_CHECKED(UInfantryAnimInstance, AnimNotify_OnZeroHealthAnimationFinished);
			// Remove the "AnimNotify_" part at the start
			NotifyFuncName = NotifyFuncName.RightChop(11);
			static const FName NotifyName = FName(*NotifyFuncName);
			for (const auto & Elem : Montage->Notifies)
			{
				if (Elem.NotifyName == NotifyName)
				{
					return true;
				}
			}
			
			// If here then we did not find the anim notify
			
			OutErrorMessage = NotifyFuncName;
			return false;
		}

		case EAnimation::PickingUpInventoryItem:
		{
			FString NotifyString = GET_FUNCTION_NAME_STRING_CHECKED(UInfantryAnimInstance, AnimNotify_PickUpInventoryItemOffGround);
			// Remove the "AnimNotify_" part at the start
			NotifyString = NotifyString.RightChop(11);
			static const FName NotifyName = FName(*NotifyString);

			for (const auto & Notify : Montage->Notifies)
			{
				if (Notify.NotifyName == NotifyName)
				{
					return true;
				}
			}

			OutErrorMessage = NotifyString;
			return false;
		}

		case EAnimation::DropOffResources:
		{
			FString NotifyString = GET_FUNCTION_NAME_STRING_CHECKED(UInfantryAnimInstance, AnimNotify_DropOffResources);
			// Remove the "AnimNotify_" part at the start
			NotifyString = NotifyString.RightChop(11);
			static const FName NotifyName = FName(*NotifyString);

			for (const auto & Notify : Montage->Notifies)
			{
				if (Notify.NotifyName == NotifyName)
				{
					return true;
				}
			}

			OutErrorMessage = NotifyString;
			return false;
		}
		
		/* Default case are all animations added by users. We will treat them as if they are 
		animations for context actions */
		default:
		{
			FString ExecuteActionString = GET_FUNCTION_NAME_STRING_CHECKED(UInfantryAnimInstance, AnimNotify_ExecuteContextAction);
			// Remove the "AnimNotify_" part at the start
			ExecuteActionString = ExecuteActionString.RightChop(11);
			static const FName ExecuteName = FName(*ExecuteActionString);

			FString FinishedString = GET_FUNCTION_NAME_STRING_CHECKED(UInfantryAnimInstance, AnimNotify_OnContextAnimationFinished);
			// Remove the "AnimNotify_" part at the start
			FinishedString = FinishedString.RightChop(11);
			static const FName FinishedName = FName(*FinishedString);

			bool bFoundExecute = false;
			bool bFoundFinished = false;
			for (const auto & Notify : Montage->Notifies)
			{
				if (Notify.NotifyName == ExecuteName)
				{
					bFoundExecute = true;
				}

				if (Notify.NotifyName == FinishedName)
				{
					bFoundFinished = true;
				}
			}

			if (bFoundExecute && bFoundFinished)
			{
				return true;
			}
			else
			{
				if (bFoundExecute)
				{
					OutErrorMessage = FinishedString;
				}
				else if (bFoundFinished)
				{
					OutErrorMessage = ExecuteActionString;
				}
				else
				{
					OutErrorMessage = ExecuteActionString + " and " + FinishedString;
				}
				
				return false;
			}
		}
	}
}
#endif

void AInfantry::PlayAnimation(EAnimation AnimType, bool bAllowReplaySameType)
{
	SERVER_CHECK;
	
	UE_CLOG(AnimType == EAnimation::None, RTSLOG, Fatal, TEXT("Trying to play \"None\" animation"));

	UAnimMontage * Montage = Animations[AnimType];

#if WITH_EDITOR || UE_BUILD_DEBUG
	
	if (Montage == nullptr)
	{
		/* These animations should have important anim notifies on them. If they do not play 
		then things will go wrong. So for that reason if they are not set then we call the 
		anim notify directly right now. 
		Users will likely get a shock when they package for development/shipping and find that 
		they start crashing here because these anims aren't set, but we gotta check at some point 
		so I think > DEBUG is good. Actually if logging doesn't work in Development build then 
		should change preprocessor to WITH_EDITOR only */
		if (AnimType == EAnimation::Attack)
		{
			AnimNotify_FireWeapon();
			AnimNotify_OnAttackAnimationFinished();
		}
		else if (AnimType == EAnimation::Destroyed)
		{
			/* Give it a delay because why not */
			Delay(&AInfantry::FinalizeDestruction, 1.f);
		}
		else if (false/*Context actions anims, or maybe just who cares, too many to put here*/)
		{
			
		}
	}

	/* Because we now just call the anim notifies explicity above the types Attack and Destroyed 
	don't really need to be logged here */
	UE_CLOG(Montage == nullptr && Statics::IsShouldBeSetAnim(AnimType), RTSLOG, Warning,
		TEXT("%s tried to play animation of type %s but no animation of that type is set"),
		*GetName(), TO_STRING(EAnimation, AnimType));

#endif // WITH_EDITOR || UE_BUILD_DEBUG

	if (bAllowReplaySameType)
	{
		/* Cancel the 'Attack preparation' sound if it is playing. */
		if (AttackPreparationAudioComp != nullptr)
		{
			AttackPreparationAudioComp->Stop();
		}
		
		GetMesh()->GetAnimInstance()->Montage_Play(Montage, GetAnimPlayRate(AnimType));

		/* OnRep notify will update animation on clients */
		AnimationState.AnimationType = AnimType;
		AnimationState.StartTime = GS->GetServerWorldTimeSeconds();
	}
	else
	{
		/* Only play montage if it is of a different type than the one
		currently playing */
		//if (GetMesh()->GetAnimInstance()->GetCurrentActiveMontage() != Montage)
		if (AnimationState.AnimationType != AnimType)
		{
			/* Cancel the 'Attack preparation' sound if it is playing. */
			if (AttackPreparationAudioComp != nullptr)
			{
				AttackPreparationAudioComp->Stop();
			}
			
			GetMesh()->GetAnimInstance()->Montage_Play(Montage, GetAnimPlayRate(AnimType));

			/* OnRep notify will update animation on clients */
			AnimationState.AnimationType = AnimType;
			AnimationState.StartTime = GS->GetServerWorldTimeSeconds();
		}
	}
}

float AInfantry::GetAnimPlayRate(EAnimation AnimType) const
{
	return (AnimType == EAnimation::Moving || AnimType == EAnimation::MovingWithResources) ? MoveAnimPlayRate : 1.f;
}

float AInfantry::GetHealthPercentage() const
{
	return Health / Attributes.GetMaxHealth();
}

bool AInfantry::IsInsideGarrison() const
{
	/* TODO consider adding another enum value EFogStatus::InsideBuildingGarrison. Then would 
	have to set that at some point and would query Attributes.GetVisionState() instead of this. 
	This is just for possibly better cache locallity, although at some points this implementation 
	might actually be faster anyways so perhaps have two funcs 
	IsInsideGarrison_QueryFogStatus and IsInsideGarrison_QueryGarrisonStatus */
	return GarrisonStatus.IsInsideGarrison();
}

UBoxComponent * AInfantry::GetBounds() const
{
	return nullptr;
}

FSelectableRootComponent2DShapeInfo AInfantry::GetRootComponent2DCollisionInfo() const
{
	return FSelectableRootComponent2DShapeInfo(GetCapsuleComponent()->GetScaledCapsuleRadius());
}

void AInfantry::OnRightClickCommand(const FVector & Location)
{
	/* Trying this as an assert instead of a if then return. PC should really check this stuff 
	before issuing these */
	assert(!Statics::HasZeroHealth(this));

	Control->OnRightClickCommand(Location);
}

void AInfantry::OnRightClickCommand(ISelectable * TargetAsSelectable, const FSelectableAttributesBase & TargetInfo)
{
	/* Trying this as an assert instead of a if then return. PC should really check this stuff
	before issuing these */
	assert(!Statics::HasZeroHealth(this));

	Control->OnRightClickCommand(TargetAsSelectable, TargetInfo);
}

void AInfantry::IssueCommand_MoveTo(const FVector & Location)
{
	assert(Statics::HasZeroHealth(this) == false);

	assert(0); // TODO just moving to location
}

void AInfantry::IssueCommand_PickUpItem(const FVector & SomeLocation, AInventoryItem * TargetItem)
{
	assert(Statics::HasZeroHealth(this) == false);

	Control->OnPickUpInventoryItemCommand(SomeLocation, TargetItem);
}

void AInfantry::IssueCommand_RightClickOnResourceSpot(AResourceSpot * ResourceSpot)
{
	assert(Statics::HasZeroHealth(this) == false);

	Control->OnRightClickOnResourceSpotCommand(ResourceSpot);
}

void AInfantry::OnSingleSelect()
{
	Attributes.SetIsSelected(true);
	Attributes.SetIsPrimarySelected(false);

	// Show decal
	ShowSelectionDecal();

	// Show selection widget
	SelectionWidgetComponent->SetVisibility(true);

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

EUnitType AInfantry::OnMarqueeSelect(uint8 & SelectableID)
{
	/* For now this is exactly the same as single select */
	OnSingleSelect();

	SelectableID = GetSelectableID();
	return Type;
}

EUnitType AInfantry::OnCtrlGroupSelect(uint8 & OutSelectableID)
{
	/* For now this is exactly the same as single select */
	OnSingleSelect();
	
	OutSelectableID = GetSelectableID();
	return Type;
}

uint8 AInfantry::OnDeselect()
{
	Attributes.SetIsSelected(false);
	Attributes.SetIsPrimarySelected(false);

	HideSelectionDecal();

	/* Stop selection/right-click particles */
	SelectionParticles->ResetParticles(true);
	//SelectionParticles->DeactivateSystem();
	//SelectionParticles->KillParticlesForced();

	// Hide selection widget
	SelectionWidgetComponent->SetVisibility(false);

	return GetSelectableID();
}

uint8 AInfantry::GetSelectableID() const
{
	return ID;
}

bool AInfantry::Security_CanBeClickedOn(ETeam ClickersTeam, const FName & ClickersTeamTag, ARTSGameState * GameState) const
{
	/* Just returning true for now. Naivly trusting all clients */
	return true;
	//return Statics::IsOutsideFog(this, ClickersTeam, ClickersTeamTag, GameState);
}

void AInfantry::OnRightClick()
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

	// TODO Show click decal if any
}

void AInfantry::OnMouseHover(ARTSPlayerController * LocalPlayCon)
{
	/* Do not overwrite fully selected decal if this is selected */
	if (Attributes.IsSelected())
	{
		return;
	}

	ShowHoverDecal();
}

void AInfantry::OnMouseUnhover(ARTSPlayerController * LocalPlayCon)
{
	/* Do nothing if selected */
	if (Attributes.IsSelected())
	{
		return;
	}

	HideSelectionDecal();
}

void AInfantry::OnEnterMarqueeBox(ARTSPlayerController * LocalPlayCon)
{
	// Same as mouse hover right now
	OnMouseHover(LocalPlayCon);
}

void AInfantry::OnExitMarqueeBox(ARTSPlayerController * LocalPlayCon)
{
	// Same as mouse unhover right now
	OnMouseUnhover(LocalPlayCon);
}

ETeam AInfantry::GetTeam() const
{
	return Attributes.GetTeam();
}

uint8 AInfantry::GetTeamIndex() const
{
	return Statics::TeamToArrayIndex(Attributes.GetTeam());
}

const FContextMenuButtons * AInfantry::GetContextMenu() const
{
	return &Attributes.GetContextMenu();
}

void AInfantry::GetInfo(TMap < EResourceType, int32 > & OutCosts, float & OutBuildTime, FContextMenuButtons & OutContextMenu) const
{
	OutCosts = this->Attributes.GetCosts();
	OutBuildTime = this->Attributes.GetBuildTime();
	OutContextMenu = this->Attributes.GetContextMenu();
}

void AInfantry::OnLayFoundationCommand(EBuildingType BuildingType, const FVector & Location, const FRotator & Rotation)
{
#if !UE_BUILD_SHIPPING
	assert(GetWorld()->IsServer());

	const FBuildingInfo * BuildingInfo = FI->GetBuildingInfo(BuildingType);
	const EBuildingBuildMethod BuildMethod = BuildingInfo->GetBuildingBuildMethod();

	/* Only these build methods should issue this command */
	assert(BuildMethod == EBuildingBuildMethod::LayFoundationsWhenAtLocation
		|| BuildMethod == EBuildingBuildMethod::Protoss);
#endif

	Control->OnLayFoundationCommand(BuildingType, Location, Rotation);
}

void AInfantry::OnContextCommand(const FContextButtonInfo & AbilityInfo, const FVector & ClickLoc,
	ISelectable * Target, const FSelectableAttributesBase & TargetInfo)
{
	/* Define what happens for certain context button pressess. Having
	behaviour defined in this function is not enough for the action
	to take place - the action must also be added to ContextMenu
	through the editor */

	SERVER_CHECK;
	assert(Target != nullptr); // Should also be valid too

	const EContextButton Command = AbilityInfo.GetButtonType();

	const FContextButton Button = FContextButton(Command);
	/* Since PC now does not issue unless it is on context menu this assert is correct */
	assert(Attributes.GetContextMenu().HasButton(Button));

	switch (Command)
	{
		case EContextButton::AttackMove:
		{	
			/* Legit not sure if we can ever get here. 
			Abilities need another bool like bCanHaveTargets that AttackMove could use because 
			attack moves can have targets but do not require them - they can also target a 
			world location */
			
			Control->OnAttackMoveCommand(ClickLoc, Target, &TargetInfo);
			break;
		}
		default:
		{
			/* All the custom user defined abilities will reach here */
			Control->OnTargetedContextCommand(Command, Target, TargetInfo);
			break;
		}
	}
}
void AInfantry::OnContextCommand(const FContextButtonInfo & AbilityInfo, const FVector & ClickLoc)
{
	SERVER_CHECK;

	const EContextButton Command = AbilityInfo.GetButtonType();

	const FContextButton Button = FContextButton(Command);

	/* Since PC now does not issue unless it is on context menu this assert is correct */
	assert(Attributes.GetContextMenu().HasButton(Button));
	
	switch (Command)
	{
		case EContextButton::AttackMove:
		{
			Control->OnAttackMoveCommand(ClickLoc, nullptr, nullptr);
			break;
		}
		default:
		{
			Control->OnTargetedContextCommand(Command, ClickLoc);
			break;
		}
	}	
}

void AInfantry::OnContextCommand(const FContextButton & Button)
{
	/* Assumed already checked for if off cooldown */

	const EContextButton Command = Button.GetButtonType();

	if (Attributes.GetContextMenu().HasButton(Button))
	{
		switch (Command)
		{
		case EContextButton::BuildBuilding:
		case EContextButton::Upgrade:
		case EContextButton::Train:
		{
			break;
		}
		case EContextButton::HoldPosition:
		{
			Control->OnHoldPositionCommand();
			break;
		}
		default:
		{
			/* Here is for any action that doesn't have a specific implementation. If you
			would like to add any custom behavior you can do so by adding a case for it above */
			Control->OnInstantContextCommand(Button);
			break;
		}
		}
	}
	else
	{
		/* Action not one this unit can carry out. If this message is seen it means PC thinks this
		unit supports the ability even though it does not and therefore PC logic needs correcting.
		But this will have no affect of gameplay */
		UE_LOG(RTSLOG, Warning, TEXT("Unit issued instant context command when they do not support "
			"the action. Unit was %s, action was %s"), *GetName(),
			TO_STRING(EContextButton, Command));
	}
}

void AInfantry::IssueCommand_UseInventoryItem(uint8 InventorySlotIndex, 
	EInventoryItem ItemType, const FContextButtonInfo & AbilityInfo)
{
	Control->OnInstantUseInventoryItemCommand(InventorySlotIndex, ItemType, AbilityInfo);
}

void AInfantry::IssueCommand_UseInventoryItem(uint8 InventorySlotIndex, 
	EInventoryItem ItemType, const FContextButtonInfo & AbilityInfo, const FVector & Location)
{
	Control->OnLocationTargetingUseInventoryItemCommand(InventorySlotIndex, ItemType, AbilityInfo, Location);
}

void AInfantry::IssueCommand_UseInventoryItem(uint8 InventorySlotIndex, 
	EInventoryItem ItemType, const FContextButtonInfo & AbilityInfo, ISelectable * Target)
{
	Control->OnSelectableTargetingUseInventoryItemCommand(InventorySlotIndex, ItemType, AbilityInfo, Target);
}

void AInfantry::IssueCommand_SpecialBuildingTargetingAbility(ABuilding * TargetBuilding, const FBuildingTargetingAbilityPerSelectableInfo * AbilityInfo)
{
	Control->OnSpecialBuildingTargetingAbilityCommand(TargetBuilding, AbilityInfo);
}

void AInfantry::IssueCommand_EnterGarrison(ABuilding * TargetBuilding, const FBuildingGarrisonAttributes & GarrisonAttributes)
{
	Control->OnEnterGarrisonCommand(TargetBuilding, GarrisonAttributes);
}

void AInfantry::OnContextMenuPlaceBuildingResult(ABuilding * PlacedBuilding, const FBuildingInfo & BuildingInfo, 
	EBuildingBuildMethod BuildMethod)
{
	assert(GetWorld()->IsServer());

	if (Statics::HasZeroHealth(this))
	{
		return;
	}

	Control->OnContextMenuPlaceBuildingResult(PlacedBuilding, BuildingInfo, BuildMethod);
}

void AInfantry::OnWorkedOnBuildingConstructionComplete()
{
	assert(GetWorld()->IsServer());

	if (Statics::HasZeroHealth(this))
	{
		return;
	}

	Control->OnWorkedOnBuildingConstructionComplete();
}

float AInfantry::GetBoundsLength() const
{
	return GetCapsuleComponent()->GetScaledCapsuleRadius();
}

float AInfantry::GetSightRadius() const
{
	return Attributes.GetSightRange();
}

void AInfantry::OnUpgradeComplete(EUpgradeType UpgradeType, uint8 UpgradeLevel)
{
}

void AInfantry::OnEnemyDestroyed(const FSelectableAttributesBasic & EnemyAttributes)
{
	GainExperience(EnemyAttributes.GetExperienceBounty(), true);

	PS->GainExperience(EnemyAttributes.GetExperienceBounty(), true);

	// Notify AI controller
	Control->OnControlledPawnKilledSomething(EnemyAttributes);
}

void AInfantry::Client_OnLevelGained(uint8 NewLevel)
{
#if EXPERIENCE_ENABLED_GAME

	CLIENT_CHECK;
	
	/* All for visuals only */

	Rank = NewLevel;

	/* Adjust attributes, making sure not to skip a level if we leveled up more than one level */
	for (uint8 i = PreviousRank + 1; i <= Rank; ++i)
	{
		ApplyLevelUpBonuses(i);

		/* Increase how much experience we are worth to enemy because we're a higher level now */
		Attributes.SetExperienceBounty(Attributes.GetExperienceBounty() * GI->GetExperienceBountyMultiplierPerLevel());
	}

	const FRankInfo & NewLevelsInfo = FI->GetLevelUpInfo(Rank);

	/* Play particle system */
	PlayLevelUpParticles(NewLevelsInfo);
	
	/* Play sound */
	PlayLevelUpSound(NewLevelsInfo);

	PreviousRank = Rank;

	// Tell HUD about EXP change and ranking up
	PC->GetHUDWidget()->Selected_OnRankChanged(this, ExperienceTowardsNextRank, Rank, 
		GetTotalExperienceRequiredForNextLevel(), Attributes.IsPrimarySelected());

#else

	UE_LOG(RTSLOG, Fatal, TEXT("AInfantry::Client_OnLevelGained called even though EXPERIENCE_ENABLED_GAME is false"));

#endif
}

void AInfantry::GetVisionInfo(float & SightRadius, float & StealthRevealRadius) const
{
	SightRadius = Attributes.GetSightRange();
	StealthRevealRadius = Attributes.GetStealthRevealRange();
}

bool AInfantry::UpdateFogStatus(EFogStatus NewFogStatus)
{
	/* EFogStatus::StealthDetected should never be assigned to VisionState.
	Assign EFogStatus::Hidden instead */

	/* This is only called on units that are not owned/allied */

	/* Check if state has changed */
	if (Attributes.GetVisionState() == NewFogStatus)
	{
		return GetCapsuleComponent()->bVisible;
	}

	Attributes.SetVisionState(NewFogStatus);

	bool bCanBeSeen;

	if (bInStealthMode)
	{
		if (NewFogStatus == EFogStatus::StealthRevealed)
		{
			SetUnitVisibility(true);
			bCanBeSeen = true;
		}
		else
		{
			SetUnitVisibility(false);
			bCanBeSeen = false;
		}
	}
	else
	{
		/* I think this can happen when going from stealthed to unstealthed so OnEnter/ExitFogOfWar
		isn't 100% true. */

		if (static_cast<uint8>(NewFogStatus) & 0x01) // If Revealed/StealthRevealed
		{
			OnExitFogOfWar();

			bCanBeSeen = true;
		}
		else // Hidden
		{
			OnEnterFogOfWar();

			bCanBeSeen = false;
		}
	}

	return bCanBeSeen;
}

bool AInfantry::IsInStealthMode() const
{
	return bInStealthMode;
}

TSubclassOf<AProjectileBase> AInfantry::GetProjectileBP() const
{
	return AtkAttr.GetProjectileBP();
}

void AInfantry::SetupBuildInfo(FUnitInfo & OutInfo, const AFactionInfo * FactionInfo) const
{
	OutInfo.SetupCostsArray(Attributes.GetCosts());
	OutInfo.SetupHousingCostsArray(Attributes.GetHousingCosts());
	OutInfo.SetTrainTime(Attributes.GetBuildTime());
	OutInfo.SetUnitType(Type);
	OutInfo.SetContextMenu(Attributes.GetContextMenu());
	OutInfo.SetName(Attributes.GetName());
	OutInfo.SetHUDImage(Attributes.GetHUDImage_Normal());
	OutInfo.SetHUDHoveredImage(Attributes.GetHUDImage_Hovered());
	OutInfo.SetHUDPressedImage(Attributes.GetHUDImage_Pressed());
	OutInfo.SetDescription(Attributes.GetDescription());
	OutInfo.SetSelectableType(ESelectableType::Unit);
	OutInfo.SetBuildingType(EBuildingType::NotBuilding);
	OutInfo.SetPrerequisites(Attributes.GetPrerequisites(), Attributes.GetUpgradePrerequisites());
	OutInfo.SetHUDPersistentTabType(Attributes.GetHUDPersistentTabCategory());
	OutInfo.SetHUDPersistentTabOrdering(Attributes.GetHUDPersistentTabButtonOrdering());
	OutInfo.SetJustBuiltSound(Attributes.GetJustBuiltSound());
	OutInfo.SetAnnounceToAllWhenBuilt(Attributes.AnnounceToAllWhenBuilt());
	
	// Set whether unit is a worker type 
	OutInfo.SetIsAWorkerType(GetInfantryAttributes()->bCanBuildBuildings);
	
	// Set whether unit is a collector type
	OutInfo.SetIsACollectorType(Attributes.ResourceGatheringProperties.IsCollector());

	// Set whether unit is an army unit type
	OutInfo.SetIsAAttackingType(!OutInfo.IsAWorkerType() && !OutInfo.IsACollectorType());

	// Set the damage values 
	OutInfo.SetDamageValues(GetWorld(), AtkAttr);
}

float AInfantry::GetHealth() const
{
	return Health;
}

const FAttachInfo * AInfantry::GetBodyLocationInfo(ESelectableBodySocket BodyLocationType) const
{
	return &Attributes.GetBodyAttachPointInfo(BodyLocationType);
}

void AInfantry::SetOnSpawnValues(ESelectableCreationMethod InCreationMethod, ABuilding * BuilderBuilding)
{
	assert(GetWorld()->IsServer());

	Attributes.SetCreationMethod(InCreationMethod);
	CreationMethod = InCreationMethod;

	BuildingSpawnedFrom = BuilderBuilding;
	
	/* Can be null if this unit is a starting selectable for one. Possibly also if the building gets 
	destroyed very quickly after this unit is built. This might actually just be flat out impossible. */
	if (BuilderBuilding != nullptr)
	{
		BarracksRallyLoc = BuilderBuilding->GetRallyPointLocation();
	}
}

void AInfantry::SetSelectableIDAndGameTickCount(uint8 InID, uint8 GameTickCounter)
{
	ID = InID;
	GameTickCountOnCreation = GameTickCounter;
}

bool AInfantry::CanClassGainExperience() const
{
	return Attributes.CanEverGainExperience();
}

#if EXPERIENCE_ENABLED_GAME
float AInfantry::GetCurrentRankExperience() const
{
	return ExperienceTowardsNextRank;
}

float AInfantry::GetTotalExperienceRequiredForNextLevel() const
{
	return GetTotalExperienceRequiredForLevel(Rank + 1);
}

float AInfantry::GetTotalExperienceRequiredForLevel(FSelectableRankInt Level) const
{
	if (Level.Level <= LevelingUpOptions::MAX_LEVEL)
	{
		return Attributes.GetTotalExperienceRequirementForLevel(Level);
	}
	else
	{
		return -1.f;
	}
}
#endif // EXPERIENCE_ENABLED_GAME

float AInfantry::GetCooldownRemaining(const FContextButton & Button)
{
	assert(GetWorld() != nullptr);

	if (!Attributes.GetContextMenu().HasButton(Button))
	{
		return -2.f;
	}

	/* Commented since TMap is filled with blank THs on setup. */
	//if (!ContextCooldowns.Contains(Button))
	//{
	//	FTimerHandle DummyTimerHandle;
	//	ContextCooldowns.Emplace(Button, DummyTimerHandle);
	//
	//	return 0.f;
	//}

	/* Should return -1.f if handle does not exist */
	return GetWorldTimerManager().GetTimerRemaining(ContextCooldowns[Button]);
}

const TMap<FContextButton, FTimerHandle>* AInfantry::GetContextCooldowns() const
{
	return &ContextCooldowns;
}

void AInfantry::OnAbilityUse(const FContextButtonInfo & AbilityInfo, uint8 ServerTickCountAtTimeOfAbility)
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
			PC->GetCurrentSelected() == this);
	}
	
	/* Start cooldown timer handle. I guess we'll only do this for server and owning clients 
	but there's nothing wrong with doing it for everybody */
	if (AbilityInfo.GetCooldown() > 0.f)
	{
		if (GetWorld()->IsServer() || Attributes.GetAffiliation() == EAffiliation::Owned)
		{
			const FContextButton Button(AbilityInfo.GetButtonType());
			Delay(ContextCooldowns[Button], &AInfantry::OnCooldownFinished, AbilityInfo.GetCooldown());

			/* If the HUD wasn't polling each action button each tick then this is where we would
			tell the HUD that it is now on cooldown. So when I change to event driven "telling HUD
			we're on cooldown" then this is where it should be done */
		}
	}
}

void AInfantry::OnInventoryItemUse(uint8 ServerInventorySlotIndex, const FContextButtonInfo & AbilityInfo, 
	uint8 ServerTickCountAtTimeOfAbility)
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

	/* Start cooldown timer handle and decrement an item charge */
	GetInventoryModifiable()->OnItemUsed(GetWorld(), this, Attributes.GetAffiliation(), 
		ServerInventorySlotIndex, AbilityInfo, GI, PC->GetHUDWidget());
}

void AInfantry::StartInventoryItemUseCooldownTimerHandle(FTimerHandle & TimerHandle, uint8 ServerInventorySlotIndex,
	float Duration)
{
	// Timer handle with params

	FTimerDelegate TimerDel;
	TimerDel.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(AInfantry, OnInventoryItemUseCooldownFinished),
		ServerInventorySlotIndex);

	GetWorldTimerManager().SetTimer(TimerHandle, TimerDel, Duration, false);
}

URTSGameInstance * AInfantry::Selectable_GetGI() const
{
	return GI;
}

ARTSPlayerState * AInfantry::Selectable_GetPS() const
{
	return PS;
}

bool AInfantry::HasZeroHealth() const
{
	return Statics::HasZeroHealth(this);
}

bool AInfantry::CanClassAcceptBuffsAndDebuffs() const
{
	return true;
}

const TArray<FStaticBuffOrDebuffInstanceInfo>* AInfantry::GetStaticBuffArray() const
{
	return &Attributes.GetStaticBuffs();
}

const TArray<FTickableBuffOrDebuffInstanceInfo>* AInfantry::GetTickableBuffArray() const
{
	return &Attributes.GetTickableBuffs();
}

const TArray<FStaticBuffOrDebuffInstanceInfo>* AInfantry::GetStaticDebuffArray() const
{
	return &Attributes.GetStaticDebuffs();
}

const TArray<FTickableBuffOrDebuffInstanceInfo>* AInfantry::GetTickableDebuffArray() const
{
	return &Attributes.GetTickableDebuffs();
}

FStaticBuffOrDebuffInstanceInfo * AInfantry::GetBuffState(EStaticBuffAndDebuffType BuffType)
{
#if !UE_BUILD_SHIPPING

	/* Make sure user defined this as a buff and not a debuff */
	
	const FStaticBuffOrDebuffInfo * Info = GI->GetBuffOrDebuffInfo(BuffType);

	UE_CLOG(Info->IsDebuff(), RTSLOG, Fatal, TEXT("Called GetBuffState for type %s but this is actually a "
		"debuff, replace the call of GetBuffState with GetDebuffState instead"),
		TO_STRING(EStaticBuffAndDebuffType, BuffType));

#endif
	
	/* O(n) */
	for (auto & Elem : Attributes.GetStaticBuffs())
	{
		if (Elem.GetSpecificType() == BuffType)
		{
			return &Elem;
		}
	}

	/* Buff not applied to us */
	return nullptr;
}

FStaticBuffOrDebuffInstanceInfo * AInfantry::GetDebuffState(EStaticBuffAndDebuffType DebuffType)
{
#if !UE_BUILD_SHIPPING

	/* Make sure user defined this as a debuff not a buff */

	const FStaticBuffOrDebuffInfo * Info = GI->GetBuffOrDebuffInfo(DebuffType);

	UE_CLOG(Info->IsBuff(), RTSLOG, Fatal, TEXT("Called GetDebuffState for type %s but this is actually a "
		"buff, replace the call of GetDebuffState with GetBuffState instead"),
		TO_STRING(EStaticBuffAndDebuffType, DebuffType));

#endif 
	
	/* O(n) */
	for (auto & Elem : Attributes.GetStaticDebuffs())
	{
		if (Elem.GetSpecificType() == DebuffType)
		{
			return &Elem;
		}
	}
	
	/* Debuff not applied to us */
	return nullptr;
}

FTickableBuffOrDebuffInstanceInfo * AInfantry::GetBuffState(ETickableBuffAndDebuffType BuffType)
{
#if !UE_BUILD_SHIPPING

	/* Make sure user defined this as a buff and not a debuff */

	const FTickableBuffOrDebuffInfo * Info = GI->GetBuffOrDebuffInfo(BuffType);

	UE_CLOG(Info->IsDebuff(), RTSLOG, Fatal, TEXT("Called GetBuffState for type %s but this is actually a "
		"debuff, replace the call of GetBuffState with GetDebuffState instead"),
		TO_STRING(ETickableBuffAndDebuffType, BuffType));

#endif
	
	/* O(n) */
	for (auto & Elem : Attributes.GetTickableBuffs())
	{
		if (Elem.GetSpecificType() == BuffType)
		{
			return &Elem;
		}
	}

	/* Buff not applied to us */
	return nullptr;
}

FTickableBuffOrDebuffInstanceInfo * AInfantry::GetDebuffState(ETickableBuffAndDebuffType DebuffType)
{
#if !UE_BUILD_SHIPPING

	/* Make sure user defined this as a debuff and not a buff */

	const FTickableBuffOrDebuffInfo * Info = GI->GetBuffOrDebuffInfo(DebuffType);

	UE_CLOG(Info->IsBuff(), RTSLOG, Fatal, TEXT("Called GetDebuffState for type %s but this is actually a "
		"buff, replace the call of GetDebbuffState with GetBuffState instead"), 
		TO_STRING(ETickableBuffAndDebuffType, DebuffType));

#endif
	
	/* O(n) */
	for (auto & Elem : Attributes.GetTickableDebuffs())
	{
		if (Elem.GetSpecificType() == DebuffType)
		{
			return &Elem;
		}
	}

	return nullptr;
}

void AInfantry::RegisterDebuff(EStaticBuffAndDebuffType DebuffType, AActor * DebuffInstigator, 
	ISelectable * InstigatorAsSelectable)
{
	const FStaticBuffOrDebuffInfo * Info = GI->GetBuffOrDebuffInfo(DebuffType);

#if !UE_BUILD_SHIPPING

	assert(Info->IsDebuff());

	/* Make sure this debuff isn't already applied */
	for (const auto & Elem : Attributes.GetStaticDebuffs())
	{
		if (Elem.GetSpecificType() == DebuffType)
		{
			UE_LOG(RTSLOG, Fatal, TEXT("Trying to register debuff %s on %s but infantry already "
				"has this buff"),
				TO_STRING(EStaticBuffAndDebuffType, DebuffType), *GetName()); 
		}
	}

#endif

	Attributes.StaticDebuffs.Emplace(FStaticBuffOrDebuffInstanceInfo(Info, DebuffInstigator,
		InstigatorAsSelectable));

	/* Make sure HUD knows about this */
	if (Attributes.IsSelected())
	{
		PC->GetHUDWidget()->Selected_OnDebuffApplied(this, Attributes.StaticDebuffs.Last(), 
			PC->GetCurrentSelected() == this);
	}
}

void AInfantry::RegisterBuff(ETickableBuffAndDebuffType BuffType, AActor * BuffInstigator, 
	ISelectable * InstigatorAsSelectable)
{
	const FTickableBuffOrDebuffInfo * Info = GI->GetBuffOrDebuffInfo(BuffType);

#if !UE_BUILD_SHIPPING
	
	/* Make sure this is a buff type and not a debuff type */
	assert(Info->IsBuff());

	/* Make sure this buff isn't already applied */
	for (const auto & Elem : Attributes.GetTickableBuffs())
	{
		if (Elem.GetSpecificType() == BuffType)
		{
			UE_LOG(RTSLOG, Fatal, TEXT("Trying to register buff %s on %s but infantry already "
				"has this buff"),
				TO_STRING(ETickableBuffAndDebuffType, BuffType), *GetName());
		}
	}

#endif
	
	Attributes.TickableBuffs.Emplace(FTickableBuffOrDebuffInstanceInfo(Info, BuffInstigator,
		InstigatorAsSelectable));

	/* Make sure UI knows about this */
	if (Attributes.IsSelected())
	{
		PC->GetHUDWidget()->Selected_OnBuffApplied(this, Attributes.TickableBuffs.Last(), 
			PC->GetCurrentSelected() == this);
	}
}

void AInfantry::RegisterDebuff(ETickableBuffAndDebuffType DebuffType, AActor * DebuffInstigator, 
	ISelectable * InstigatorAsSelectable)
{
	const FTickableBuffOrDebuffInfo * Info = GI->GetBuffOrDebuffInfo(DebuffType);

#if !UE_BUILD_SHIPPING

	assert(Info->IsDebuff());

	/* Make sure debuff isn't already applied */
	for (const auto & Elem : Attributes.GetTickableDebuffs())
	{
		if (Elem.GetSpecificType() == DebuffType)
		{
			UE_LOG(RTSLOG, Fatal, TEXT("Trying to register debuff %s on %s but infantry already "
				"has this debuff "), 
				TO_STRING(ETickableBuffAndDebuffType, DebuffType), *GetName());
		}
	}

#endif

	Attributes.TickableDebuffs.Emplace(FTickableBuffOrDebuffInstanceInfo(Info, DebuffInstigator,
		InstigatorAsSelectable));

	/* Make sure HUD knows about this */
	if (Attributes.IsSelected())
	{
		PC->GetHUDWidget()->Selected_OnDebuffApplied(this, Attributes.TickableDebuffs.Last(), 
			PC->GetCurrentSelected() == this);
	}
}

EBuffOrDebuffRemovalOutcome AInfantry::RemoveBuff(EStaticBuffAndDebuffType BuffType, 
	AActor * RemovalInstigator, ISelectable * InstigatorAsSelectable, EBuffAndDebuffRemovalReason RemovalReason)
{
	// If thrown then should call RemoveDebuff instead
	assert(GI->IsBuff(BuffType));
	
	for (int32 i = Attributes.GetStaticBuffs().Num() - 1; i >= 0; --i)
	{
		FStaticBuffOrDebuffInstanceInfo & Elem = Attributes.GetStaticBuffs()[i];

		if (Elem.GetSpecificType() == BuffType)
		{
			const FStaticBuffOrDebuffInfo * Info = Elem.GetInfoStruct();
			
			Attributes.GetStaticBuffs().RemoveAt(i, 1, false);

			const EBuffOrDebuffRemovalOutcome Result = Info->ExecuteOnRemovedBehavior(this, Elem, 
				RemovalInstigator, InstigatorAsSelectable, RemovalReason);

			/* Update HUD. For now will not update world widgets but can easily do that
			by using similar logic to HUD */
			if (Attributes.IsSelected())
			{
				PC->GetHUDWidget()->Selected_OnStaticBuffRemoved(this, i, RemovalReason, 
					PC->GetCurrentSelected() == this);
			}

			return Result;
		}
	}

	// Implies did not actually have the buff. No biggie really 
	assert(0);

	return EBuffOrDebuffRemovalOutcome::NotPresent;
}

EBuffOrDebuffRemovalOutcome AInfantry::RemoveDebuff(EStaticBuffAndDebuffType DebuffType, AActor * RemovalInstigator,
	ISelectable * InstigatorAsSelectable, EBuffAndDebuffRemovalReason RemovalReason)
{
	// If thrown then should call RemoveBuff instead
	assert(GI->IsDebuff(DebuffType));

	for (int32 i = Attributes.GetStaticDebuffs().Num() - 1; i >= 0; --i)
	{
		FStaticBuffOrDebuffInstanceInfo & Elem = Attributes.GetStaticDebuffs()[i];

		if (Elem.GetSpecificType() == DebuffType)
		{
			const FStaticBuffOrDebuffInfo * Info = Elem.GetInfoStruct();

			Attributes.GetStaticDebuffs().RemoveAt(i, 1, false);

			const EBuffOrDebuffRemovalOutcome Result = Info->ExecuteOnRemovedBehavior(this, Elem, 
				RemovalInstigator, InstigatorAsSelectable, RemovalReason);

			if (Attributes.IsSelected())
			{
				PC->GetHUDWidget()->Selected_OnStaticDebuffRemoved(this, i, RemovalReason,
					PC->GetCurrentSelected() == this);
			}

			return Result;
		}
	}

	// Implies did not actually have the debuff. No biggie really 
	assert(0);

	return EBuffOrDebuffRemovalOutcome::NotPresent;
}

EBuffOrDebuffRemovalOutcome AInfantry::RemoveBuff(ETickableBuffAndDebuffType BuffType, AActor * RemovalInstigator,
	ISelectable * InstigatorAsSelectable, EBuffAndDebuffRemovalReason RemovalReason)
{
	// If thrown then should call RemoveDebuff instead
	assert(GI->IsBuff(BuffType));

	for (int32 i = Attributes.GetTickableBuffs().Num() - 1; i >= 0; --i)
	{
		FTickableBuffOrDebuffInstanceInfo & Elem = Attributes.GetTickableBuffs()[i];

		if (Elem.GetSpecificType() == BuffType)
		{
			const FTickableBuffOrDebuffInfo * Info = Elem.GetInfoStruct();

			Attributes.GetTickableBuffs().RemoveAt(i, 1, false);

			const EBuffOrDebuffRemovalOutcome Result = Info->ExecuteOnRemovedBehavior(this, Elem, 
				RemovalInstigator, InstigatorAsSelectable, RemovalReason);

			if (Attributes.IsSelected())
			{
				PC->GetHUDWidget()->Selected_OnTickableBuffRemoved(this, i, RemovalReason,
					PC->GetCurrentSelected() == this);
			}

			return Result;
		}
	}

	return EBuffOrDebuffRemovalOutcome::NotPresent;
}

EBuffOrDebuffRemovalOutcome AInfantry::RemoveDebuff(ETickableBuffAndDebuffType DebuffType, AActor * RemovalInstigator,
	ISelectable * InstigatorAsSelectable, EBuffAndDebuffRemovalReason RemovalReason)
{
	// If thrown then should call RemoveBuff instead
	assert(GI->IsDebuff(DebuffType));

	for (int32 i = Attributes.GetTickableDebuffs().Num() - 1; i >= 0; --i)
	{
		FTickableBuffOrDebuffInstanceInfo & Elem = Attributes.GetTickableDebuffs()[i];

		if (Elem.GetSpecificType() == DebuffType)
		{
			const FTickableBuffOrDebuffInfo * Info = Elem.GetInfoStruct();

			Attributes.GetTickableDebuffs().RemoveAt(i, 1, false);

			const EBuffOrDebuffRemovalOutcome Result = Info->ExecuteOnRemovedBehavior(this, Elem, 
				RemovalInstigator, InstigatorAsSelectable, RemovalReason);

			if (Attributes.IsSelected())
			{
				PC->GetHUDWidget()->Selected_OnTickableDebuffRemoved(this, i, RemovalReason,
					PC->GetCurrentSelected() == this);
			}

			return Result;
		}
	}

	return EBuffOrDebuffRemovalOutcome::NotPresent;
}

uint8 AInfantry::GetAppliedGameTickCount() const
{
	// I think this is right
	return GameTickCountOnCreation;
}

FVector AInfantry::GetActorLocationSelectable() const
{
	return GetActorLocation();
}

float AInfantry::GetDistanceFromAnotherForAbilitySquared(const FContextButtonInfo & AbilityInfo, ISelectable * Target) const
{
	/* Maybe coud be faster if we pass 'this' pointer into func aswell, saves Control having 
	to use its Unit pointer. 
	Crash here? Possibly calling this on client but Control only exists on server */
	return FMath::Square(Control->GetDistanceFromAnotherSelectableForAbility(AbilityInfo, Target));
}

float AInfantry::GetDistanceFromLocationForAbilitySquared(const FContextButtonInfo & AbilityInfo, const FVector & Location) const
{
	/* Maybe coud be faster if we pass 'this' pointer into func aswell, saves Control having
	to use its Unit pointer */
	return FMath::Square(Control->GetDistanceFromLocationForAbility(AbilityInfo, Location));
}

ARTSPlayerController * AInfantry::GetLocalPC() const
{
	return PC;
}

bool AInfantry::HasAttack() const
{
	return bHasAttack;
}

const FShopInfo * AInfantry::GetShopAttributes() const
{
	return nullptr;
}

const FInventory * AInfantry::GetInventory() const
{
	return &Attributes.GetInventory();
}

FInventory * AInfantry::GetInventoryModifiable()
{
	return &Attributes.GetInventory();
}

float & AInfantry::GetHealthRef()
{
	return Health;
}

void AInfantry::ShowTooltip(URTSHUD * HUDWidget) const
{
	// NOOP. Infantry don't show tooltips
}

USelectableWidgetComponent * AInfantry::GetPersistentWorldWidget() const
{
	return PersistentWidgetComponent;
}

USelectableWidgetComponent * AInfantry::GetSelectedWorldWidget() const
{
	return SelectionWidgetComponent;
}

void AInfantry::AdjustPersistentWorldWidgetForNewCameraZoomAmount(float CameraZoomAmount, float CameraPitchAmount)
{
	// NOOP. See ABuilding's impl
}

void AInfantry::AdjustSelectedWorldWidgetForNewCameraZoomAmount(float CameraZoomAmount, float CameraPitchAmount)
{
	// NOOP. See ABuilding's impl
}

void AInfantry::AttachParticles(const FContextButtonInfo & AbilityInfo, UParticleSystem * Template,
	ESelectableBodySocket AttachLocation, uint32 ParticleIndex)
{
	const FAttachInfo * AttachInfo = GetBodyLocationInfo(AttachLocation);
	UParticleSystemComponent * PSC = UGameplayStatics::SpawnEmitterAttached(Template,
		AttachInfo->GetComponent(), AttachInfo->GetSocketName(),
		AttachInfo->GetAttachTransform().GetLocation(),
		AttachInfo->GetAttachTransform().GetRotation().Rotator(),
		AttachInfo->GetAttachTransform().GetScale3D());
	Attributes.AttachedParticles.Emplace(FAttachedParticleInfo(PSC, AbilityInfo, ParticleIndex));
}

void AInfantry::AttachParticles(const FTickableBuffOrDebuffInfo & BuffOrDebuffInfo)
{
	if (BuffOrDebuffInfo.GetParticlesTemplate() != nullptr)
	{
		const FAttachInfo * AttachInfo = GetBodyLocationInfo(BuffOrDebuffInfo.GetParticlesAttachPoint());
		UParticleSystemComponent * PSC = UGameplayStatics::SpawnEmitterAttached(BuffOrDebuffInfo.GetParticlesTemplate(),
			AttachInfo->GetComponent(), AttachInfo->GetSocketName(),
			AttachInfo->GetAttachTransform().GetLocation(),
			AttachInfo->GetAttachTransform().GetRotation().Rotator(),
			AttachInfo->GetAttachTransform().GetScale3D());
		Attributes.AttachedParticles.Emplace(FAttachedParticleInfo(PSC, BuffOrDebuffInfo));
	}
}

void AInfantry::AttachParticles(const FStaticBuffOrDebuffInfo & BuffOrDebuffInfo)
{
	if (BuffOrDebuffInfo.GetParticlesTemplate() != nullptr)
	{
		const FAttachInfo * AttachInfo = GetBodyLocationInfo(BuffOrDebuffInfo.GetParticlesAttachPoint());
		UParticleSystemComponent * PSC = UGameplayStatics::SpawnEmitterAttached(BuffOrDebuffInfo.GetParticlesTemplate(),
			AttachInfo->GetComponent(), AttachInfo->GetSocketName(),
			AttachInfo->GetAttachTransform().GetLocation(),
			AttachInfo->GetAttachTransform().GetRotation().Rotator(),
			AttachInfo->GetAttachTransform().GetScale3D());
		Attributes.AttachedParticles.Emplace(FAttachedParticleInfo(PSC, BuffOrDebuffInfo));
	}
}

void AInfantry::RemoveAttachedParticles(const FTickableBuffOrDebuffInfo & BuffOrDebuffInfo)
{
	if (BuffOrDebuffInfo.GetParticlesTemplate() != nullptr)
	{
		/* Array will probably have like 4 entries max or something. RemoveSingle may be faster */
		const int32 Index = Attributes.AttachedParticles.Find(FAttachedParticleInfo(BuffOrDebuffInfo));
		UParticleSystemComponent * PSC = Attributes.AttachedParticles[Index].GetPSC(); 
		PSC->DeactivateSystem();
		//PSC->DestroyComponent(); // From my testing don't need this; is done automatically by DeactivateSystem

		Attributes.AttachedParticles.RemoveAtSwap(Index, 1, false);
	}
}

void AInfantry::RemoveAttachedParticles(const FStaticBuffOrDebuffInfo & BuffOrDebuffInfo)
{
	if (BuffOrDebuffInfo.GetParticlesTemplate() != nullptr)
	{
		/* Array will probably have like 4 entries max or something. RemoveSingle may be faster */
		const int32 Index = Attributes.AttachedParticles.Find(FAttachedParticleInfo(BuffOrDebuffInfo));
		UParticleSystemComponent * PSC = Attributes.AttachedParticles[Index].GetPSC();
		PSC->DeactivateSystem();
		//PSC->DestroyComponent(); // From my testing don't need this; is done automatically by DeactivateSystem

		Attributes.AttachedParticles.RemoveAtSwap(Index, 1, false);
	}
}

FVector AInfantry::GetMuzzleLocation() const
{
	return GetMesh()->GetSocketLocation(AtkAttr.GetMuzzleSocket());
}

bool AInfantry::CanAquireTarget(AActor * PotentialTargetAsActor, ISelectable * PotentialTarget) const
{
	return Statics::CanTypeBeTargeted(PotentialTargetAsActor, AtkAttr.GetAcceptableTargetTypes())
		&& (AtkAttr.CanAttackAir() || !Statics::IsAirUnit(PotentialTargetAsActor))
		&& Statics::IsHostile(PotentialTargetAsActor, PS->GetTeamTag())
		&& Statics::CanBeSelected(PotentialTargetAsActor)
		&& !Statics::HasZeroHealth(PotentialTargetAsActor);
}

const FBuildingTargetingAbilityPerSelectableInfo * AInfantry::GetSpecialRightClickActionTowardsBuildingInfo(ABuilding * Building, EFaction BuildingsFaction, EBuildingType BuildingsType, EAffiliation BuildingsAffiliation) const
{
	return Attributes.BuildingTargetingAbilityAttributes.GetAbilityInfo(BuildingsFaction, BuildingsType, BuildingsAffiliation);
}

const FCursorInfo * AInfantry::GetSelectedMouseCursor_CanAttackHostileUnit(URTSGameInstance * GameInst, AActor * HostileAsActor, ISelectable * HostileUnit) const
{
	if (Attributes.CanAttackHostileMouseCursor != EMouseCursorType::None)
	{
		/* Querying GI here but this is only called a maximum of once per frame so it's ok */
		const FCursorInfo & Cursor = GameInst->GetMouseCursorInfo(Attributes.CanAttackHostileMouseCursor);
		return Cursor.ContainsCustomCursor() ? &Cursor : nullptr;
	}
	else
	{
		return nullptr;
	}
}

const FCursorInfo * AInfantry::GetSelectedMouseCursor_CanAttackFriendlyUnit(URTSGameInstance * GameInst, AActor * FriendlyAsActor, ISelectable * FriendlyUnit, EAffiliation UnitsAffiliation) const
{
	if (Attributes.CanAttackFriendlyMouseCursor != EMouseCursorType::None)
	{
		/* Querying GI here but this is only called a maximum of once per frame so it's ok */
		const FCursorInfo & Cursor = GameInst->GetMouseCursorInfo(Attributes.CanAttackFriendlyMouseCursor);
		return Cursor.ContainsCustomCursor() ? &Cursor : nullptr;
	}
	else
	{
		return nullptr;
	}
}

const FCursorInfo * AInfantry::GetSelectedMouseCursor_CanAttackHostileBuilding(URTSGameInstance * GameInst, ABuilding * HostileBuilding) const
{
	/* Just using the exact same cursor the unit version uses */
	return GetSelectedMouseCursor_CanAttackHostileUnit(GameInst, HostileBuilding, HostileBuilding);
}

const FCursorInfo * AInfantry::GetSelectedMouseCursor_CanAttackFriendlyBuilding(URTSGameInstance * GameInst, ABuilding * FriendlyBuilding, EAffiliation BuildingsAffiliation) const
{
	/* Just using the exact same cursor the unit version uses */
	return GetSelectedMouseCursor_CanAttackFriendlyUnit(GameInst, FriendlyBuilding, FriendlyBuilding, BuildingsAffiliation);
}

#if WITH_EDITOR
bool AInfantry::PIE_IsForCPUPlayer() const
{
	return PIE_bIsForCPUPlayer;
}

int32 AInfantry::PIE_GetHumanOwnerIndex() const
{
	return PIE_HumanOwnerIndex;
}

int32 AInfantry::PIE_GetCPUOwnerIndex() const
{
	return PIE_CPUOwnerIndex;
}
#endif // WITH_EDITOR

void AInfantry::FellOutOfWorld(const UDamageType & dmgType)
{
	UE_LOG(RTSLOG, Error, TEXT("%s fell out of world"), *GetName());

	// TODO call some stuff like when destroyed but don't really expect this to happen

	Super::FellOutOfWorld(dmgType);
}

float AInfantry::TakeDamage(float DamageAmount, FDamageEvent const & DamageEvent, AController * EventInstigator, AActor * DamageCauser)
{
	if (!GetWorld()->IsServer())
	{
		return 0.f;
	}

	if (DamageAmount == 0.f || Statics::HasZeroHealth(this))
	{
		return 0.f;
	}

	/* Appears to just broadcast event */
	//Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	/* Get multiplier for damage type and armour type pairing */
	const float ArmourTypeMultiplier = GI->GetDamageMultiplier(DamageEvent.DamageTypeClass,
		Attributes.GetArmourType());
	const float FinalDamageAmount = DamageAmount * ArmourTypeMultiplier * Attributes.GetDefenseMultiplier();

	if ((FinalDamageAmount > 0.f) && (!bCanBeDamaged))
	{
		return 0.f;
	}

	Health -= FinalDamageAmount;
	if (Health <= 0.f)
	{
		/* Set Health to 0. Important since OnRep_Health will interpret values less than 
		0 to mean other things */
		Health = 0.f;
		Control->OnPossessedUnitDestroyed(DamageCauser);

		/* TODO: start shrinking sightradius gradually */
	}
	else if (FinalDamageAmount > 0.f)
	{
		Control->OnUnitTakeDamage(DamageCauser, FinalDamageAmount);
	}
	else if (FinalDamageAmount < 0.f)
	{
		/* Ensure no overhealing */
		if (Health > Attributes.GetMaxHealth())
		{
			Health = Attributes.GetMaxHealth();
		}
	}

	OnRep_Health();

	return FinalDamageAmount;
}

float AInfantry::TakeDamageSelectable(float DamageAmount, FDamageEvent const & DamageEvent, AController * EventInstigator, AActor * DamageCauser)
{
	return TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

void AInfantry::Consume_BuildingTargetingAbilityInstigator()
{
	/* Set Health to -1. OnRep_Health will interpret this that the unit is being consumed since 
	they used a building targeting ability */
	Health = -1.f;

	Control->OnPossessedUnitConsumed(this);

	OnRep_Health();
}

void AInfantry::OnRep_Health()
{
	if (Health == 0.f)
	{
		OnZeroHealth();
	}
	/* -1.f means consumed due to being the instigator of a consuming type building targeting 
	ability e.g. C&C engineers */
	else if (Health == -1.f)
	{
		OnZeroHealth();
		SetUnitVisibility(false);
		SetUnitCollisionEnabled(false);
		/* Ok it looks like this delay is needed. Without it the server will just send the 
		signal that AActor::Destroy has been called and not send the Health update */
		Delay(&AInfantry::FinalizeDestruction, 10.f);
	}
	else
	{
		/* Update the persistent and selection widgets */
		PersistentWidgetComponent->OnHealthChanged(Health);
		SelectionWidgetComponent->OnHealthChanged(Health);
		
		/* Notify HUD if selected. Probably only really needs to be done if current selected
		actually since only showing info for current selected at the moment */
		if (Attributes.IsSelected())
		{
			PC->GetHUDWidget()->Selected_OnHealthChanged(this, Health, Attributes.GetMaxHealth(),
				Attributes.IsPrimarySelected());
		}
	}
}

void AInfantry::OnZeroHealth()
{
	Tags[Statics::GetZeroHealthTagIndex()] = Statics::HasZeroHealthTag;

	/* Make unselectable */
	GetCapsuleComponent()->SetVisibility(false);

	if (IsInsideGarrison())
	{
		ABuilding * BuildingGarrisonedInsideOf = CastChecked<ABuilding>(GS->GetPlayerFromID(GarrisonStatus.GarrisonOwnerID)->GetSelectableFromID(GarrisonStatus.GarrisonSelectableID));
		/* Validity check mainly for if client */
		if (Statics::IsValid(BuildingGarrisonedInsideOf))
		{
			const int32 Index = BuildingGarrisonedInsideOf->GetBuildingAttributesModifiable()->GetGarrisonAttributes().OnGarrisonedUnitZeroHealth(BuildingGarrisonedInsideOf, this);
			/* PC->NotifyOfInfantryReachingZeroHealth will potentially do another HUD update... 
			probably wanna just do 1 for performance */
			PC->GetHUDWidget()->OnGarrisonedUnitZeroHealth(BuildingGarrisonedInsideOf, Index);
		}
	}

	PC->NotifyOfInfantryReachingZeroHealth(this, Attributes.GetZeroHealthSound(), Attributes.IsSelected());

	if (Attributes.IsSelected())
	{
		OnDeselect();
	}

	/* Since we can't be selected anymore just need to update the persistent widget */
	PersistentWidgetComponent->OnZeroHealth();

	GS->OnInfantryZeroHealth(this, GetWorld()->IsServer());
	/* As long as a selectable can never be damaged while its owner has not been figured
	out yet then this is ok, but if we can be damaged before owner has been figured out
	then calling this here is not technically safe */
	PS->OnUnitDestroyed(this);

	// Probably lots of stuff that needs to be done if PS->bIsABot

	// Legacy comment
	/* Will need to notify HUD if we are the current selected that we now have zero health.
	Same for PersistentWidgetComponent and SelectionWidgetComponent. Perhaps call a func like
	OnZeroHealth instead of OnHealthChanged. Maybe leave all this up to the PC doing it
	though since their current selected should change when this unit hits zero health. At
	least that's the behavior that should happen */
}

void AInfantry::FinalizeDestruction()
{
	if (!GetWorld()->IsServer())
	{
		return;
	}

	/* Disable revealing fog of war with this unit. Actually because we removed it from the
	PS::Units array on zero health fog got revealing stopped then */
	GS->OnSelectableDestroyed(this, Attributes.GetTeam(), true);

	Destroy();
}

void AInfantry::AnimNotify_ExitStealthMode()
{
	if (!GetWorld()->IsServer())
	{
		return;
	}

	/* 0 is code for 'never leave stealth' similar to ghosts or dark templars in SCII */
	if (Attributes.StealthRevealTime != 0.f && bInStealthMode)
	{
		bInStealthMode = false;
		/* Call manually because we're on server */
		OnRep_bInStealthMode();
	}

	/* The bInToggleStealthMode check guards against this notify being in an animation for a 
	unit that does not actually want to enter stealth */
	if (Attributes.bInToggleStealthMode && Attributes.StealthRevealTime > 0.f)
	{
		Delay(TimerHandle_ReenterStealth, &AInfantry::EnterStealthMode, Attributes.StealthRevealTime);
	}

	/* Negative StealRevealTime implies stealth will not be automatically applied and unit will
	need to be explicitly told to re-enter stealth */

	// Tell AI controller about change
	Control->OnUnitExitStealthMode();
}

void AInfantry::AnimNotify_PlayAttackPreparationSound()
{
	if (AtkAttr.GetPreparationSound() != nullptr)
	{
		// Spawn the audio comp if it has not been spawned already, otherwise just play sound on it
		if (AttackPreparationAudioComp == nullptr)
		{
			AttackPreparationAudioComp = Statics::SpawnSoundAtLocation(GS, Attributes.GetTeam(),
				AtkAttr.GetPreparationSound(), GetMesh()->GetSocketLocation(AtkAttr.GetMuzzleSocket()),
				ESoundFogRules::DynamicExceptForInstigatorsTeam);
		}
		else
		{
			AttackPreparationAudioComp->PlaySound(GS, AtkAttr.GetPreparationSound(), 0.f,
				Attributes.GetTeam(), ESoundFogRules::DynamicExceptForInstigatorsTeam);
		}
	}
}

void AInfantry::AnimNotify_FireWeapon()
{
	if (!GetWorld()->IsServer())
	{
		return;
	}

	Control->AnimNotify_OnWeaponFired();
}

void AInfantry::AnimNotify_OnAttackAnimationFinished()
{
	if (!GetWorld()->IsServer())
	{
		return;
	}

	Control->AnimNotify_OnAttackAnimationFinished();
}

void AInfantry::AnimNotify_ExecuteContextAction()
{
	if (!GetWorld()->IsServer())
	{
		return;
	}

	Control->AnimNotify_OnContextActionExecuted();
}

void AInfantry::AnimNotify_OnContextAnimationFinished()
{
	if (!GetWorld()->IsServer())
	{
		return;
	}

	Control->AnimNotify_OnContextAnimationFinished();
}

void AInfantry::AnimNotify_DropOffResources()
{
	if (!GetWorld()->IsServer())
	{
		return;
	}

	Control->AnimNotify_OnResourcesDroppedOff();
}

void AInfantry::AnimNotify_TryPickUpInventoryItemOffGround()
{
	// An alternative way of doing this is to say (Control == nullptr) since it is never assigned 
	// on clients. Perhaps just double check that and if it is then make sure to not do that
	if (!GetWorld()->IsServer())
	{
		return;
	}

	Control->AnimNotify_TryPickUpInventoryItemOffGround();
}

void AInfantry::AnimNotify_OnZeroHealthAnimationFinished()
{
	FinalizeDestruction();
}

bool AInfantry::CanBuildBuildings() const
{
	return Attributes.bCanBuildBuildings;
}

bool AInfantry::IsACollector() const
{
	return Attributes.ResourceGatheringProperties.IsCollector();
}

bool AInfantry::CanCollectResource(EResourceType ResourceType) const
{
	return Attributes.ResourceGatheringProperties.CanGatherResource(ResourceType);
}

const FSelectableAttributesBase & AInfantry::GetAttributesBase() const
{
	return Attributes;
}

FSelectableAttributesBase & AInfantry::GetAttributesBase()
{
	return Attributes;
}

const FSelectableAttributesBasic * AInfantry::GetAttributes() const
{
	return &Attributes;
}

const FInfantryAttributes * AInfantry::GetInfantryAttributes() const
{
	return &Attributes;
}

const FAttackAttributes * AInfantry::GetAttackAttributes() const
{
	return &AtkAttr;
}

FSelectableAttributesBasic * AInfantry::GetAttributesModifiable()
{
	return &Attributes;
}

FInfantryAttributes * AInfantry::GetInfantryAttributesModifiable()
{
	return &Attributes;
}

FAttackAttributes * AInfantry::GetAttackAttributesModifiable()
{
	return &AtkAttr;
}

void AInfantry::SetHasAttack(bool bCanNowAttack)
{
	bHasAttack = bCanNowAttack;

	// Update actor tags
	const int32 TagsIndex = Statics::GetHasAttackTagIndex();
	assert(Tags[TagsIndex] == Statics::HasAttackTag || Tags[TagsIndex] == Statics::NotHasAttackTag);
	Tags[TagsIndex] = bCanNowAttack ? Statics::HasAttackTag : Statics::NotHasAttackTag;

	/* Update AI controller behavior */
	Control->OnUnitHasAttackChanged(bCanNowAttack);
}

uint8 AInfantry::GetRank() const
{
#if EXPERIENCE_ENABLED_GAME
	return Rank;
#else
	return 255;
#endif
}

EFaction AInfantry::GetFaction() const
{
	return PS->GetFaction();
}

const FName & AInfantry::GetTeamTag() const
{
	return Tags[Statics::GetTeamTagIndex()];
}

void AInfantry::SetMoveAnimPlayRate()
{
	// Calculate play rate. This is based on movement speed
	MoveAnimPlayRate = GetMoveSpeed() / GetStartingMoveSpeed();

	// If the movement animation is playing right now then make sure this change affects it
	UAnimInstance * AnimInst = GetMesh()->GetAnimInstance();
	if (FAnimMontageInstance * Montage = AnimInst->GetActiveInstanceForMontage(Animations[EAnimation::Moving]))
	{
		Montage->SetPlayRate(MoveAnimPlayRate);
	}
	// Check if the 'MovingWithResources' anim is playing instead
	else if (FAnimMontageInstance * Montage2 = AnimInst->GetActiveInstanceForMontage(Animations[EAnimation::MovingWithResources]))
	{
		Montage2->SetPlayRate(MoveAnimPlayRate);
	}
}

float AInfantry::GetMoveSpeed() const
{
	return GetCharacterMovement()->MaxWalkSpeed;
}

float AInfantry::GetDefaultMoveSpeed() const
{
	/* I use MaxWalkSpeedCrouched to represent default move speed since I likely won't use it 
	and it avoids having to introduce another variable. Also it's right next to MaxWalkSpeed 
	for cache locality. I know it's bad */
	return GetCharacterMovement()->MaxWalkSpeedCrouched;
}

float AInfantry::GetStartingMoveSpeed() const
{
	/* Really this should be a value on the FUnitInfo struct instead of using move comp's 
	swim speed (likely this is exposed to editor too so users can change it - not good) */
	return GetCharacterMovement()->MaxSwimSpeed;
}

void AInfantry::Internal_SetNewDefaultMoveSpeed(float NewValue)
{
	assert(NewValue >= 0.f);
	// @See GetDefaultMoveSpeed as to why using MaxWalkSpeedCrouched
	GetCharacterMovement()->MaxWalkSpeedCrouched = NewValue;
}

float AInfantry::SetNewDefaultMoveSpeed(float NewMoveSpeed)
{
	if (Attributes.GetNumTempMoveSpeedModifiers() > 0)
	{
		const float PercentageDiff = NewMoveSpeed / GetDefaultMoveSpeed();

		// Apply now
		GetCharacterMovement()->MaxWalkSpeed *= PercentageDiff;
	}
	else
	{
		// Apply now
		GetCharacterMovement()->MaxWalkSpeed = NewMoveSpeed;
	}
	
	// Adjust move anim play rate
	SetMoveAnimPlayRate();

	Internal_SetNewDefaultMoveSpeed(NewMoveSpeed);

	return GetMoveSpeed();
}

float AInfantry::SetNewDefaultMoveSpeedViaMultiplier(float Multiplier)
{
	if (Attributes.GetNumTempMoveSpeedModifiers() > 0)
	{
		Internal_SetNewDefaultMoveSpeed(GetDefaultMoveSpeed() * Multiplier);

		// Apply now
		GetCharacterMovement()->MaxWalkSpeed *= Multiplier;
	}
	else
	{
		Internal_SetNewDefaultMoveSpeed(GetDefaultMoveSpeed() * Multiplier);

		// Apply now
		GetCharacterMovement()->MaxWalkSpeed = GetDefaultMoveSpeed();
	}

	// Adjust move anim play rate
	SetMoveAnimPlayRate();

	return GetMoveSpeed();
}

float AInfantry::ApplyTempMoveSpeedMultiplier(float Multiplier)
{
	/* Important to check if the buff type is not already applied before calling this */
	
	assert(Multiplier >= 0.f);

	Attributes.NumTempMoveSpeedModifiers++;

	GetCharacterMovement()->MaxWalkSpeed *= Multiplier;
	
	// Adjust move anim play rate
	SetMoveAnimPlayRate();

	return GetMoveSpeed();
}

float AInfantry::RemoveTempMoveSpeedMultiplier(float Multiplier)
{
	/* Important to check if the buff type is already applied before calling this */

	assert(Multiplier >= 0.f);

	Attributes.NumTempMoveSpeedModifiers--;
	if (Attributes.GetNumTempMoveSpeedModifiers() > 0)
	{
		GetCharacterMovement()->MaxWalkSpeed *= Multiplier;
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = GetDefaultMoveSpeed();
	}

	// Adjust move anim play rate
	SetMoveAnimPlayRate();

	return GetMoveSpeed();
}

float AInfantry::RemoveMoveSpeedEffectOfResources()
{
	Attributes.NumTempMoveSpeedModifiers--;

	if (Attributes.GetNumTempMoveSpeedModifiers() > 0)
	{
		GetCharacterMovement()->MaxWalkSpeed /= GetMoveSpeedMultiplierForHoldingResources();
	}
	else
	{
		/* Value will likely be close to this anyway but need to do this because floats */
		GetCharacterMovement()->MaxWalkSpeed = GetDefaultMoveSpeed();
	}

	// Adjust move anim play rate
	SetMoveAnimPlayRate();

	return GetMoveSpeed();
}

bool AInfantry::ApplyTempStealthModeEffect()
{
	if (GetWorld()->IsServer())
	{
		Attributes.NumTempStealthModeEffects++;

		if (!bInStealthMode)
		{
			bInStealthMode = true;
			OnRep_bInStealthMode();
		}
	}
	
	return bInStealthMode;
}

bool AInfantry::RemoveTempStealthModeEffect()
{
	if (GetWorld()->IsServer())
	{
		Attributes.NumTempStealthModeEffects--;
		assert(Attributes.NumTempStealthModeEffects >= 0);

		if (Attributes.NumTempStealthModeEffects == 0)
		{
			/* Only push us out of stealth if we're not a permanently invis type e.g. if a temp 
			invis buff fades from a dark templar we still stay invis since we're always ment to 
			be invis */
			if ((Attributes.ShouldSpawnStealthed() == false) && (bInStealthMode == true))
			{
				bInStealthMode = false;
				OnRep_bInStealthMode();
			}
		}
	}
	
	return bInStealthMode;
}

const TMap<EResourceType, FResourceCollectionAttribute>& AInfantry::GetResourceGatheringProperties() const
{
	return Attributes.ResourceGatheringProperties.GetTMap();
}

float AInfantry::GetMoveSpeedMultiplierForHoldingResources() const
{
	return Attributes.ResourceGatheringProperties.GetMoveSpeedPenaltyForHoldingResources();
}

bool AInfantry::IsHoldingResources() const
{
	return HeldResourceType != EResourceType::None;
}

EResourceType AInfantry::GetHeldResourceType() const
{
	return HeldResourceType;
}

void AInfantry::SetHeldResource(EResourceType NewHeldResource, uint32 Amount)
{
	/* Maybe this will fire. The move speed logic in OnRep_HeldResource which is called soon 
	after this relies on this assuption being true for it to work so it will need changing if 
	this assert is ever removed */
	assert(HeldResourceType != NewHeldResource);
	
	HeldResourceType = NewHeldResource;
	HeldResourceAmount = Amount;
}

uint32 AInfantry::GetHeldResourceAmount() const
{
	return HeldResourceAmount;
}

float AInfantry::GetResourceGatherRate(EResourceType ResourceType) const
{
	return Attributes.ResourceGatheringProperties.GetGatherTime(ResourceType);
}

uint32 AInfantry::GetCapacityForResource(EResourceType ResourceType) const
{
	return Attributes.ResourceGatheringProperties.GetCapacity(ResourceType);
}

AInfantryController * AInfantry::GetAIController() const
{
	return Control;
}

#if WITH_EDITOR
void AInfantry::DisplayAIControllerOnScreenDebugInfo()
{
	Control->DisplayOnScreenDebugInfo();
}
#endif

void AInfantry::Multicast_OnAttackMade_Implementation(AActor * Target)
{
	const FVector MuzzleLoc = GetMesh()->GetSocketLocation(AtkAttr.GetMuzzleSocket());

	if (GetWorld()->IsServer())
	{
		assert(AtkAttr.GetProjectileBP() != nullptr);
		assert(Target != nullptr);

		/* Fire projectile. Both here and for client we use 0 as the roll rotation param. 
		Perhaps it could be more varied but will leave it how it is. The reason for this 
		is perhaps the flight of the projectile depends on roll and if they are significantly 
		different between clients then their flights may end up being way off between clients too */
		PC->GetPoolingManager()->Server_FireProjectileAtTarget(this, AtkAttr.GetProjectileBP(), 
			AtkAttr.GetDamageInfo(), AtkAttr.GetAttackRange(), Attributes.GetTeam(), MuzzleLoc, Target, 0.f);
	}
	else
	{
		/* Check because networking */
		if (Statics::IsValid(Target))
		{
			assert(AtkAttr.GetProjectileBP() != nullptr);
			PC->GetPoolingManager()->Client_FireProjectileAtTarget(AtkAttr.GetProjectileBP(), 
				AtkAttr.GetDamageInfo(), AtkAttr.GetAttackRange(), Attributes.GetTeam(), MuzzleLoc, Target, 0.f);
		}
	}

	/* Play sound if any */
	if (AtkAttr.GetAttackMadeSound() != nullptr)
	{
		Statics::SpawnSoundAtLocation(GS, Attributes.GetTeam(), AtkAttr.GetAttackMadeSound(), 
			MuzzleLoc, ESoundFogRules::DynamicExceptForInstigatorsTeam);
	}

	/* Play muzzle camera shake if any */
	if (AtkAttr.GetMuzzleCameraShakeBP() != nullptr)
	{
		PC->PlayCameraShake(AtkAttr.GetMuzzleCameraShakeBP(), MuzzleLoc, AtkAttr.GetMuzzleShakeRadius(),
			AtkAttr.GetMuzzleShakeFalloff());
	}

	/* Spawn muzzle particles if any */
	if (AtkAttr.GetMuzzleParticles() != nullptr)
	{
		Statics::SpawnFogParticles(AtkAttr.GetMuzzleParticles(), GS, MuzzleLoc,
			GetMesh()->GetSocketRotation(AtkAttr.GetMuzzleSocket()), FVector::OneVector);
	}
}
