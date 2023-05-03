// Fill out your copyright notice in the Description page of Project Settings.

#include "StartingGrid.h"
#include "Components/StaticMeshComponent.h"
#include "Components/ArrowComponent.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"

#include "Statics/Statics.h"
#include "MapElements/Building.h"
#include "MapElements/Infantry.h"
#include "Miscellaneous/StartingGrid/StartingGridComponent.h"
#include "GameFramework/RTSPlayerState.h"
#include "Statics/DevelopmentStatics.h"
#include "GameFramework/FactionInfo.h"


AStartingGrid::AStartingGrid()
{
	DummyRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Dummy Root"));
	SetRootComponent(DummyRoot);
	DummyRoot->PrimaryComponentTick.bCanEverTick = false;

#if WITH_EDITORONLY_DATA

	Arrow = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	if (Arrow != nullptr)
	{
		Arrow->SetupAttachment(DummyRoot);
		Arrow->SetWorldScale3D(FVector(2.f));
		Arrow->AddRelativeLocation(FVector(0.f, 0.f, 40.f));
	}
	
	Floor = CreateEditorOnlyDefaultSubobject<UStaticMeshComponent>(TEXT("Floor Mesh"));
	if (Floor != nullptr)
	{
		Floor->SetupAttachment(DummyRoot);

		/* Set mesh as cube and scale it */
		const auto CubeMesh = ConstructorHelpers::FObjectFinder<UStaticMesh>(TEXT("StaticMesh'/Engine/BasicShapes/Cube.Cube'"));
		if (CubeMesh.Object != nullptr)
		{
			Floor->SetStaticMesh(CubeMesh.Object);
			Floor->SetWorldScale3D(FVector(20.f, 20.f, 0.2f));
			Floor->AddRelativeLocation(FVector(0.f, 0.f, -11.f));
		}
		Floor->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Floor->SetCanEverAffectNavigation(false);
		Floor->CanCharacterStepUpOn = ECB_No;
		Floor->bReceivesDecals = false;
		Floor->PrimaryComponentTick.bCanEverTick = false;
	}

#endif

	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	bReplicates = false;
}

void AStartingGrid::PostLoad()
{
	Super::PostLoad();

	bStartedInMap = true;
}

void AStartingGrid::BeginPlay()
{
	Super::BeginPlay();

	if (bStartedInMap)
	{
		/* In this case GM should handle spawning the selectables */
		return;
	}

	/* GetOwner() should refer to the player state */
	PS = CastChecked<ARTSPlayerState>(GetOwner());
	ARTSGameState * GameState = CastChecked<ARTSGameState>(GetWorld()->GetGameState());

	/* Spawn buildings/units */
	for (const auto & Elem : GridComponents)
	{
		const TSubclassOf <AActor> ActorBP = Elem->GetSelectableBP();

		if (ActorBP->IsChildOf(ABuilding::StaticClass()))
		{
			ABuilding * Building = SpawnStartingBuilding(GameState, GetWorld(), Elem->GetComponentTransform(),
				PS->GetFI()->GetBuildingInfoSlow(ActorBP), PS);

			SpawnedBuildingTypes.Emplace(Building->GetType());
		}
		else if (ActorBP->IsChildOf(AInfantry::StaticClass()))
		{
			AInfantry * Infantry = SpawnStartingInfantry(GameState, GetWorld(), Elem->GetComponentTransform(),
				ActorBP, PS);

			SpawnedUnitTypes.Emplace(Infantry->GetType());
		}
	}
}

FVector AStartingGrid::GetSpawnLocation(UWorld * World, const FTransform & DesiredTransform)
{
	/* How far above param location to go when tracing downwards looking for the ground */
	const float TraceHalfHeight = 10000.f;

	/* Possible grid component was actually placed in editor below/above the floor. Do a line
	trace against the environment looking for the floor but start from a quite high up height */
	FHitResult HitResult;
	const FVector StartLoc = DesiredTransform.GetLocation() + FVector(0.f, 0.f, TraceHalfHeight);
	const FVector EndLoc = DesiredTransform.GetLocation() + FVector(0.f, 0.f, -TraceHalfHeight);
	//const bool bResult = World->LineTraceSingleByChannel(HitResult, StartLoc, EndLoc,
	//	GROUND_CHANNEL);
	const bool bResult = World->SweepSingleByChannel(HitResult, StartLoc, EndLoc, FQuat::Identity, 
		GROUND_CHANNEL, FCollisionShape::MakeSphere(0.01f));
	/* Did sphere sweep instead of line trace. Uggg, line trace most definently does not work 
	sometimes. See:  
	https://answers.unrealengine.com/questions/133871/single-line-trace-does-not-always-hit-the-object-l.html 
	Perhaps try increasing the thickness of the landscape from 16?
	The only issue with using sphere is that the buildings will be slightly elevated above the 
	ground although the sphere's radius really small seems to work fine. Actually because I'm 
	using HitResult.ImpactPoint we should be fine anyways no matter the sphere's radius. 
	Yet another insanely verbose comment.  
	*/

	UE_CLOG(!bResult, RTSLOG, Fatal, TEXT("Starting grid: Trace did not hit anything. Start location: [%s], "
		"end location: [%s]"), *StartLoc.ToString(), *EndLoc.ToString());

	return HitResult.ImpactPoint;
}

FRotator AStartingGrid::GetSpawnRotation(UWorld * World, const FTransform & DesiredTransform)
{
	return DesiredTransform.GetRotation().Rotator();
}

#if WITH_EDITOR
bool AStartingGrid::PIE_IsForCPUPlayer() const
{
	return PIE_bUseCPUPlayerOwnerIndex;
}

int32 AStartingGrid::PIE_GetCPUOwnerIndex() const
{
	return PIE_CPUPlayerOwnerIndex;
}

int32 AStartingGrid::PIE_GetHumanOwnerIndex() const
{
	return PIE_HumanPlayerOwnerIndex;
}

void AStartingGrid::PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	bCanEditHumanOwnerIndex = !PIE_bUseCPUPlayerOwnerIndex;
}
#endif // WITH_EDITOR

ABuilding * AStartingGrid::SpawnStartingBuilding(ARTSGameState * GameState, UWorld * World, 
	const FTransform & DesiredTransform, const FBuildingInfo * BuildingInfo, ARTSPlayerState * BuildingOwner)
{
	/* For now assumptions are made, like that the ground where we want to spawn things on is
	completely flat */

	const FVector SpawnLoc = GetSpawnLocation(World, DesiredTransform);
	const FRotator SpawnRot = GetSpawnRotation(World, DesiredTransform);

	/* Last param signals building is being spawned as a starting selectable and it should
	start already constructed */
	return Statics::SpawnBuilding(BuildingInfo, SpawnLoc, SpawnRot, BuildingOwner, GameState, World, 
		ESelectableCreationMethod::StartingSelectable);
}

AInfantry * AStartingGrid::SpawnStartingInfantry(ARTSGameState * GameState, UWorld * World, 
	const FTransform & DesiredTransform, TSubclassOf<AActor> UnitBP, ARTSPlayerState * UnitOwner)
{
	const FVector SpawnLoc = GetSpawnLocation(World, DesiredTransform);
	const FRotator SpawnRot = GetSpawnRotation(World, DesiredTransform);

	/* Know this will crash in future when being called by GM */
	return CastChecked<AInfantry>(Statics::SpawnUnit(UnitBP, SpawnLoc, SpawnRot, UnitOwner, 
		GameState, World, ESelectableCreationMethod::StartingSelectable, nullptr));
}

const TArray<EBuildingType>& AStartingGrid::GetSpawnedBuildingTypes() const
{
	return SpawnedBuildingTypes;
}

const TArray<EUnitType>& AStartingGrid::GetSpawnedUnitTypes() const
{
	return SpawnedUnitTypes;
}

void AStartingGrid::RegisterGridComponent(UStartingGridComponent * GridComponent)
{
	assert(GridComponent != nullptr);
	GridComponents.Emplace(GridComponent);
}
