// Fill out your copyright notice in the Description page of Project Settings.

#include "GhostBuilding.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

#include "Statics/DevelopmentStatics.h"
#include "GameFramework/RTSGameInstance.h"


// Sets default values
AGhostBuilding::AGhostBuilding()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Bounds = CreateDefaultSubobject<UBoxComponent>(TEXT("Root Component"));
	SetRootComponent(Bounds);
	Bounds->SetCollisionProfileName(FName("NoCollision"));
	Bounds->SetCanEverAffectNavigation(false);
	Bounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Bounds->SetGenerateOverlapEvents(false);

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Bounds);
	Mesh->SetCollisionProfileName(FName("NoCollision"));
	Mesh->SetCanEverAffectNavigation(false);
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->CanCharacterStepUpOn = ECB_No;
	// Disable casting a shadow with this mesh. Annnd it does not appear to work. Could be 
	// that BP hasn't updated bug. Don't have time to test right now FIXME
	Mesh->CastShadow = false;

#if WITH_EDITORONLY_DATA
	/* Well I hope this excludes this mesh component completely from packaged builds. Concern 
	is that comp is already created while in editor and is now a part of blueprint FIXME */
	EditorOnlyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EditorOnlyStaticMesh"));
	if (EditorOnlyMesh != nullptr)
	{
		EditorOnlyMesh->SetupAttachment(Bounds);
		// Hide it while editing BP
		EditorOnlyMesh->SetVisibility(false);
		// Set mesh as engine default cube
		auto CubeMesh = ConstructorHelpers::FObjectFinder<UStaticMesh>(TEXT("StaticMesh'/Engine/BasicShapes/Cube.Cube'"));
		if (CubeMesh.Object != nullptr)
		{
			EditorOnlyMesh->SetStaticMesh(CubeMesh.Object);
			EditorOnlyMesh->RelativeLocation = FVector(0.f, 0.f, 80.f);
		}

		EditorOnlyMesh->SetCanEverAffectNavigation(false);
		EditorOnlyMesh->SetGenerateOverlapEvents(false);
		EditorOnlyMesh->CanCharacterStepUpOn = ECB_No;
		EditorOnlyMesh->SetCollisionProfileName(FName("NoCollision"));
	}
#endif

	bCanBeDamaged = false;

	bReplicates = false;
	bReplicateMovement = false;
	bOnlyRelevantToOwner = true;

	/* By default do not start off red */
	bIsAtBuildableLocation = true;
}

// Called when the game starts or when spawned
void AGhostBuilding::BeginPlay()
{
	Super::BeginPlay();

#if WITH_EDITORONLY_DATA
	/* If user has not set their own skeletal mesh then use the default cube mesh. It was 
	previously hidden during design time so unhide it now */
	if (Mesh->SkeletalMesh == nullptr)
	{
		EditorOnlyMesh->SetVisibility(true);
	}
#endif

	URTSGameInstance * GameInst = CastChecked<URTSGameInstance>(GetWorld()->GetGameInstance());

	const TArray < UMaterialInterface * > & MaterialArray = Mesh->GetMaterials();

	// TODO do this for CACs too
	/* Create dynamic material instances for the mesh's materials and store original param values */
	DynamicMaterials.Reserve(MaterialArray.Num());
	OriginalBadLocationParamValues.Reserve(MaterialArray.Num());
	for (int32 i = 0; i < MaterialArray.Num(); ++i)
	{
		UMaterialInterface * Elem = MaterialArray[i];

		UMaterialInstanceDynamic * Material = UMaterialInstanceDynamic::Create(Elem, this);
		Mesh->SetMaterial(i, Material); // This line is important, otherwise changes aren't seen
		DynamicMaterials.Emplace(Material);

		/* By default these variables contain overridden values, so set them back to non-overrides
		if desired */
		if (!bOverrideBadLocationParamName)
		{
			BadLocationParamName = GameInst->GetGhostBadLocationParamName();
		}
		if (!bOverrideBadLocationParamValue)
		{
			BadLocationParamValue = GameInst->GetGhostBadLocationParamValue();
		}

		FMaterialParameterInfo ParamInfo = FMaterialParameterInfo(BadLocationParamName);
		FLinearColor OriginalValue = FLinearColor();
		const bool bSuccess = Material->GetVectorParameterValue(ParamInfo, OriginalValue);

		/* Will actually get crash here if material was set to null in editor */
		UE_CLOG(!bSuccess, RTSLOG, Log, TEXT("In regards to ghost building %s: this building's mesh "
			"material %s does not have vector param named %s and therefore cannot change it "
			"based on suitable build locations"),
			*GetName(), *Elem->GetName(), *BadLocationParamName.ToString());

		OriginalBadLocationParamValues.Emplace(OriginalValue);
	}
}

void AGhostBuilding::SetRotationToDefault()
{
	/* Reset rotation to default for next time. Using the default camera rotation of PC */
	SetActorRotation(FRotator(0.f, PlayersDefaultYawRot, 0.f));
}

void AGhostBuilding::FellOutOfWorld(const UDamageType & dmgType)
{
	/* Essentially do nothing. Since this is pooled we don't want to destroy it and if it raises
	above threshold again then it should be visible */
}

void AGhostBuilding::SetInitialValues(const FBuildingInfo & BuildingInfo, float DefaultYawRotation)
{
#if WITH_EDITOR
	/* Make the cube the same size as the building's bounds to make it more accurately 
	represent the dimensions of the building. 
	Divide by 100 because the default cube mesh is 100x100x100 */
	EditorOnlyMesh->SetRelativeScale3D(BuildingInfo.GetScaledBoundsExtent() / 100.f);
#endif 

	SetDefaultRotation(DefaultYawRotation);
}

void AGhostBuilding::SetDefaultRotation(float DefaultYawRotation)
{
	PlayersDefaultYawRot = DefaultYawRotation;
}

void AGhostBuilding::OnEnterPool()
{
	SetActorHiddenInGame(true);

	// Reset rotation back to default here since may have been rotated before being placed
	SetRotationToDefault();

	/* Make it so initialally not red when leaving pool */
	bIsAtBuildableLocation = false;
	TrySetAppearanceForBuildLocation(true);
}

void AGhostBuilding::OnExitPool(EBuildingType InBuildingType)
{
	Type = InBuildingType;

	SetActorHiddenInGame(false);
}

void AGhostBuilding::TrySetAppearanceForBuildLocation(bool bIsBuildableLocation)
{
	/* A material parameter collection is an option, but I think it will change the materials
	on all the hidden pooled ghosts too if they share the same name which may be worse for
	performance. Because only one ghost is visible at a time I think dynamic material instances
	will be ok performance wise */

	/* Only change appearance if state has changed from buildable to non or vice versa */
	if (bIsBuildableLocation && !bIsAtBuildableLocation)
	{
		bIsAtBuildableLocation = true;

		for (int32 i = 0; i < DynamicMaterials.Num(); ++i)
		{
			DynamicMaterials[i]->SetVectorParameterValue(BadLocationParamName,
				OriginalBadLocationParamValues[i]);
		}
	}
	else if (!bIsBuildableLocation && bIsAtBuildableLocation)
	{
		bIsAtBuildableLocation = false;

		for (int32 i = 0; i < DynamicMaterials.Num(); ++i)
		{
			DynamicMaterials[i]->SetVectorParameterValue(BadLocationParamName,
				BadLocationParamValue);
		}
	}
}

void AGhostBuilding::SetRotationUsingOffset(float YawOffsetAmount)
{
	const FRotator NewRot = FRotator(0.f, PlayersDefaultYawRot - YawOffsetAmount, 0.f);
	
	SetActorRotation(NewRot);
}

EBuildingType AGhostBuilding::GetType() const
{
	return Type;
}

