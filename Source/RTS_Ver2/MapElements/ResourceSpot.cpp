// Fill out your copyright notice in the Description page of Project Settings.

#include "ResourceSpot.h"
#include "Engine/World.h"
#include "Public/TimerManager.h"
#include "Components/StaticMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/DecalComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

#include "Statics/DevelopmentStatics.h"
#include "GameFramework/RTSGameState.h"
#include "Statics/Statics.h"
#include "MapElements/Infantry.h"
#include "MapElements/AIControllers/InfantryController.h"
#include "GameFramework/RTSPlayerController.h"
#include "GameFramework/FactionInfo.h"
#include "GameFramework/RTSGameInstance.h"
#include "UI/WorldWidgets/SelectableWidgetComponent.h"

// Sets default values
AResourceSpot::AResourceSpot()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->bReceivesDecals = false;
	Mesh->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->SetCollisionProfileName(FName("ResourceSpot"));

	SelectionWidgetComponent = CreateDefaultSubobject<USelectableWidgetComponent>(TEXT("Selection Widget"));
	SelectionWidgetComponent->SetupAttachment(Mesh);

	PersistentWidgetComponent = CreateDefaultSubobject<USelectableWidgetComponent>(TEXT("Persistent Widget"));
	PersistentWidgetComponent->SetupAttachment(Mesh);

	SelectionDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("Selection Decal"));
	SelectionDecal->SetupAttachment(Mesh);
	SelectionDecal->bVisible = false;
	SelectionDecal->bAbsoluteRotation = true;
	SelectionDecal->bDestroyOwnerAfterFade = false;
	// Do this because it starts off sideways
	SelectionDecal->RelativeRotation = FRotator(-90.f, 0.f, 0.f);
	// Lower height to lower rendering cost
	SelectionDecal->RelativeScale3D = FVector(0.1f, 1.f, 1.f);

	SelectionParticles = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("Selection Particles"));
	SelectionParticles->SetupAttachment(Mesh);
	SelectionParticles->bAutoActivate = false;
	SelectionParticles->bAbsoluteRotation = true;
	SelectionParticles->SetCollisionProfileName(FName("NoCollision"));
	SelectionParticles->SetGenerateOverlapEvents(false);
	SelectionParticles->SetCanEverAffectNavigation(false);
	SelectionParticles->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	SelectionParticles->bReceivesDecals = false;

	bReplicates = false;
	bReplicateMovement = false;

	Attributes.SetupBasicTypeInfo(ESelectableType::Building, EBuildingType::ResourceSpot, EUnitType::NotUnit);
	Attributes.SetupSelectionInfo(Statics::NeutralID, ETeam::Neutral);
	Attributes.SetAffiliation(EAffiliation::Neutral);

	Capacity = 20000;
}

// Called when the game starts or when spawned
void AResourceSpot::BeginPlay()
{
	UE_CLOG(Type == EResourceType::None, RTSLOG, Fatal, TEXT("Resource spot %s's type is \"None\". "
		"Need to assign user defined type to it"), *GetClass()->GetName());

	Super::BeginPlay();

	SetupSelectionInfo();

	assert(Tags.Num() == 0);
	/* Some of these tags may need revising */
	Tags.Reserve(Statics::NUM_ACTOR_TAGS);
	Tags.Emplace(Statics::NeutralID);
	Tags.Emplace(Statics::NEUTRAL_TEAM_TAG);
	Tags.Emplace(Statics::UNTARGETABLE_TAG);
	Tags.Emplace(Statics::NotAirTag);
	Tags.Emplace(Statics::BuildingTag);
	Tags.Emplace(Statics::NotHasAttackTag);
	Tags.Emplace(Statics::HasZeroHealthTag);
	Tags.Emplace(Statics::NotHasInventoryTag);
	assert(Tags.Num() == Statics::NUM_ACTOR_TAGS);

	CurrentAmount = Capacity;

	/* Leave it up to game mode to call Setup() */
}

void AResourceSpot::SetupForLocalPlayer(URTSGameInstance * InGameInstance, AFactionInfo * InFactionInfo,
	ETeam LocalPlayersTeam)
{
	// InFactionInfo can be null if player is an observer
	FI = InFactionInfo;
	GI = InGameInstance;
	bIsLocalPlayerObserver = (LocalPlayersTeam == ETeam::Observer);

	// Setup selection decal

	if (bIsLocalPlayerObserver)
	{
		// Update this to stay consistent
		Attributes.SetAffiliation(EAffiliation::Observed);

		SelectionDecalInfo = &GI->GetObserverSelectionDecal();
	}
	else
	{
		SelectionDecalInfo = &FI->GetSelectionDecalInfo(Attributes.GetAffiliation());
	}

	SelectionDecal->SetDecalMaterial(SelectionDecalInfo->GetDecal());

	if (SelectionDecal->GetDecalMaterial() != nullptr)
	{
		if (SelectionDecalInfo->RequiresCreatingMaterialInstanceDynamic())
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

	SelectionDecal->SetVisibility(false);

	// Setup widget components
	if (bIsLocalPlayerObserver)
	{
		SelectionWidgetComponent->SetWidgetClassAndSpawn(this, GI->GetObserverResourceSpotSelectionWorldWidget(),
			0.f, Attributes, GI);
		PersistentWidgetComponent->SetWidgetClassAndSpawn(this, GI->GetObserverResourceSpotPersistentWorldWidget(),
			0.f, Attributes, GI);
	}
	else
	{
		SelectionWidgetComponent->SetWidgetClassAndSpawn(this, FI->GetResourceSpotSelectionWorldWidget(),
			0.f, Attributes, GI);
		PersistentWidgetComponent->SetWidgetClassAndSpawn(this, FI->GetResourceSpotPersistentWorldWidget(),
			0.f, Attributes, GI);
	}

	SelectionWidgetComponent->SetVisibility(false);
}

void AResourceSpot::Setup()
{
	// Pretty sure this only gets called on the server

	Init();

	if (GetWorld()->IsServer())
	{
		GS->Server_RegisterNeutralSelectable(this);
	}
}

void AResourceSpot::Init()
{
	GS = CastChecked<ARTSGameState>(GetWorld()->GetGameState());

	GS->AddToResourceSpots(Type, this);
}

void AResourceSpot::SetupSelectionInfo()
{
	// NOOP, already done in ctor
}

AInfantry * AResourceSpot::GetQueueFront() const
{
	return Queue.Last();
}

void AResourceSpot::OnResourcesDepleted(ARTSPlayerState * GatherersPlayerState)
{
	assert(GetWorld()->IsServer());

	// Perhaps figure out who should receive a notification and send it

	/* Also CurrentAmount could be replicated if we want to track amount spot has */
}

void AResourceSpot::Delay(FTimerHandle & TimerHandle, void(AResourceSpot::* Function)(), float Delay)
{
	assert(Delay > 0.f);
	GetWorldTimerManager().SetTimer(TimerHandle, this, Function, Delay, false);
}

#if WITH_EDITOR
void AResourceSpot::PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	Attributes.SetName(Name);
	Attributes.SetHUDImage_Normal(HUDImage);
}
#endif

EResourceType AResourceSpot::GetType() const
{
	return Type;
}

float AResourceSpot::GetCollectionAcceptanceRadius() const
{
	return CollectionAcceptanceRadius;
}

bool AResourceSpot::IsDepleted() const
{
	return CurrentAmount == 0;
}

uint32 AResourceSpot::GetCurrentResources() const
{
	return CurrentAmount;
}

bool AResourceSpot::Enqueue(AInfantry * Collector)
{
	assert(Collector != nullptr);

	/* Add to back of queue */
	Queue.Insert(Collector, 0);

	/* If first to arrive then start collecting immediately */
	if (Queue.Num() == 1)
	{
		Collector->GetAIController()->StartCollectingResources();
		return true;
	}
#if !UE_BUILD_SHIPPING
	else
	{
		/* Make sure all collectors in queue still want to be there. They should remove themselves
		if they do not */
		int32 NumCollectorsInWaitingState = 0;
		for (int32 i = Queue.Num() - 1; i >= 0; --i)
		{
			if (Queue[i]->GetAIController()->IsWaitingToCollectResources())
			{
				NumCollectorsInWaitingState++;
			}
		}

		// Minus 2 because the collecter that just got here has not set UnitState = WaitingToGatherResources
		// It will when this returns though and the front of queue is collecting
		if (Queue.Num() > 1)
		{
			assert(NumCollectorsInWaitingState == Queue.Num() - 2);
		}
		else
		{
			assert(NumCollectorsInWaitingState == 0);
		}
	}
#endif

	return false;
}

uint32 AResourceSpot::TakeResourcesFrom(uint32 GathererCapacity, ARTSPlayerState * GatherersPlayerState)
{
	uint32 AmountTaken;

	if (CurrentAmount <= GathererCapacity)
	{
		AmountTaken = CurrentAmount;
		CurrentAmount = 0;
		OnResourcesDepleted(GatherersPlayerState);
	}
	else
	{
		AmountTaken = GathererCapacity;
		CurrentAmount -= GathererCapacity;
	}

	return AmountTaken;
}

void AResourceSpot::OnCollectorLeaveSpot(AInfantry * Collector)
{
	assert(Queue.Num() > 0);

	/* If they were currently collecting then let the next in queue collect from here */
	if (GetQueueFront() == Collector)
	{
		Queue.Pop(false);

		/* If they took the last chunk of resources then do nothing more */
		if (IsDepleted())
		{
			Queue.Empty();
			return;
		}

		if (Queue.Num() > 0)
		{
			/* Make next in queue start collecting resources. The queue should be 100% up to date.
			Any collector that doesn't want to collect from this spot should have removed
			themselves */
			AInfantry * NextInQueue = Queue.Last();
			assert(Statics::IsValid(NextInQueue) && !Statics::HasZeroHealth(NextInQueue)
				&& NextInQueue->GetAIController()->IsWaitingToCollectResources()
				&& NextInQueue->GetAIController()->IsAtResourceSpot());

			NextInQueue->GetAIController()->StartCollectingResources();
		}
	}
	else
	{
		const int32 NumRemoved = Queue.RemoveSingle(Collector);
		assert(NumRemoved == 1);
	}
}

ABuilding * AResourceSpot::GetClosestDepot(ARTSPlayerState * PlayerState) const
{
	assert(PlayerState != nullptr);

	/* TMap::Find() crashes */
	if (ClosestDepots.Contains(PlayerState))
	{
		return ClosestDepots[PlayerState];
	}
	else
	{
		return nullptr;
	}
}

void AResourceSpot::SetClosestDepot(ARTSPlayerState * PlayerState, ABuilding * Depot)
{
	assert(PlayerState != nullptr);

	ClosestDepots.Emplace(PlayerState, Depot);
}

float AResourceSpot::AICon_GetBaseBuildRadius() const
{
	/* Will have to see how well this does */
	return Mesh->Bounds.SphereRadius * 1.5f;
}

UBoxComponent * AResourceSpot::GetBounds() const
{
	return nullptr;
}

float AResourceSpot::GetBoundsLength() const
{
	// Not really that good
	return Mesh->Bounds.SphereRadius * Mesh->RelativeScale3D.X * 2.5f;
}

void AResourceSpot::OnSingleSelect()
{
	Attributes.SetIsSelected(true);
	Attributes.SetIsPrimarySelected(false);

	ShowSelectionDecal();

	SelectionWidgetComponent->SetVisibility(true);
}

uint8 AResourceSpot::GetSelectableID() const
{
	/* Any garbage should do. Key point is that the player controller picks up that this isn't
	owned by us and therefore doesn't let us issue command on it */
	return UINT8_MAX;
}

uint8 AResourceSpot::OnDeselect()
{
	Attributes.SetIsSelected(false);
	Attributes.SetIsPrimarySelected(false);

	if (bIsMouseHovering)
	{
		ShowHoverDecal();
	}
	else
	{
		HideSelectionDecal();
	}

	SelectionWidgetComponent->SetVisibility(false);

	return GetSelectableID();
}

void AResourceSpot::OnRightClick()
{
	/* Show right-click particles. Note: Attributes currently not editable in editor so
	particle size cannot be set */

	UParticleSystem * Template = nullptr;

	// Can probably set particle reference in initialization
	if (bIsLocalPlayerObserver)
	{
		//Template = GI->GetObserverPlayersRightClickParticlesIfTheyEvenHaveAny();
	}
	else
	{
		Template = FI->GetRightClickParticles(Attributes.GetAffiliation(), Attributes.GetParticleSize());
	}

	if (SelectionParticles->Template != Template)
	{
		SelectionParticles->SetTemplate(Template);
	}

	// Reset particles to play from start
	SelectionParticles->ResetParticles(true);
	SelectionParticles->ActivateSystem(false);

	// TODO show click decal if any
}

void AResourceSpot::ShowTooltip(URTSHUD * HUDWidget) const
{
	// NOOP. No tooltips for resource spots. See AInventoryItem::OnMouseHover and 
	// AInventoryItem::ShowTooltip if want to change this
}

void AResourceSpot::HideSelectionDecal()
{
	if (Attributes.GetSelectionDecalSetup() == ESelectionDecalSetup::UsesNonDynamicMaterial
		|| Attributes.GetSelectionDecalSetup() == ESelectionDecalSetup::UsesDynamicMaterial)
	{
		SelectionDecal->SetVisibility(false);
	}
}

void AResourceSpot::ShowSelectionDecal()
{
	if (Attributes.GetSelectionDecalSetup() == ESelectionDecalSetup::UsesDynamicMaterial)
	{
		// Set decal material param to value ofr not hovering
		Attributes.GetSelectionDecalMID()->SetScalarParameterValue(SelectionDecalInfo->GetParamName(),
			SelectionDecalInfo->GetOriginalParamValue());

		// Show decal
		SelectionDecal->SetVisibility(true);
	}
	else if (Attributes.GetSelectionDecalSetup() == ESelectionDecalSetup::UsesNonDynamicMaterial)
	{
		SelectionDecal->SetVisibility(true);
	}
}

void AResourceSpot::ShowHoverDecal()
{
	if (Attributes.GetSelectionDecalSetup() == ESelectionDecalSetup::UsesDynamicMaterial)
	{
		// Set decal material param to value for hover
		Attributes.GetSelectionDecalMID()->SetScalarParameterValue(SelectionDecalInfo->GetParamName(),
			SelectionDecalInfo->GetMouseoverParamValue());

		// Show decal
		SelectionDecal->SetVisibility(true);
	}
	else if (Attributes.GetSelectionDecalSetup() == ESelectionDecalSetup::UsesNonDynamicMaterial)
	{
		SelectionDecal->SetVisibility(true);
	}
}

bool AResourceSpot::Security_CanBeClickedOn(ETeam ClickersTeam, const FName & ClickersTeamTag, ARTSGameState * GameState) const
{
	// Resource spots can always be clicked on
	return true;
}

void AResourceSpot::OnMouseHover(ARTSPlayerController * LocalPlayCon)
{
	bIsMouseHovering = true;

	/* Do not overwrite fully selected decal if this is selected */
	if (Attributes.IsSelected())
	{
		return;
	}

	ShowHoverDecal();
}

void AResourceSpot::OnMouseUnhover(ARTSPlayerController * LocalPlayCon)
{
	bIsMouseHovering = false;

	/* Do nothing if selected */
	if (Attributes.IsSelected())
	{
		return;
	}

	HideSelectionDecal();
}

const FSelectableAttributesBase & AResourceSpot::GetAttributesBase() const
{
	return Attributes;
}

FSelectableAttributesBase & AResourceSpot::GetAttributesBase()
{
	return Attributes;
}

const FSelectableAttributesBasic * AResourceSpot::GetAttributes() const
{
	/* Even though the attributes struct for this selectable is a FSelectableAttributeBasic 
	I just return null here because it should not be. TODO change Attributes to 
	FSelectableAttributesBase */
	return nullptr;
}

bool AResourceSpot::HasAttack() const
{
	return false;
}

bool AResourceSpot::CanClassGainExperience() const
{
	return false;
}

const FInventory * AResourceSpot::GetInventory() const
{
	return nullptr;
}

void AResourceSpot::AdjustPersistentWorldWidgetForNewCameraZoomAmount(float CameraZoomAmount, float CameraPitchAmount)
{
	// NOOP see ABuilding's impl
}

void AResourceSpot::AdjustSelectedWorldWidgetForNewCameraZoomAmount(float CameraZoomAmount, float CameraPitchAmount)
{
	// NOOP see ABuilding's impl
}

#if WITH_EDITOR
bool AResourceSpot::PIE_IsForCPUPlayer() const
{
	return false;
}

int32 AResourceSpot::PIE_GetHumanOwnerIndex() const
{
	return -2;
}

int32 AResourceSpot::PIE_GetCPUOwnerIndex() const
{
	return -2;
}
#endif // WITH_EDITOR

float AResourceSpot::TakeDamage(float DamageAmount, FDamageEvent const & DamageEvent, AController * EventInstigator, AActor * DamageCauser)
{
	// Resource spots don't take damage so do nothing
	return 0.0f;
}
