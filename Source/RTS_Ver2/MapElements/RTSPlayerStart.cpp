// Fill out your copyright notice in the Description page of Project Settings.

#include "RTSPlayerStart.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"

#include "MapElements/Invisible/RTSLevelVolume.h"

ARTSPlayerStart::ARTSPlayerStart(const FObjectInitializer & ObjectInitializer)
	: Super(ObjectInitializer)
{
	GetCapsuleComponent()->SetCanEverAffectNavigation(false);
	GetCapsuleComponent()->CanCharacterStepUpOn = ECB_No;
	GetCapsuleComponent()->SetGenerateOverlapEvents(false);

	bCanBeDamaged = false;

	/* Transform will be saved to disk so this is ok */
	bIsEditorOnlyActor = true;
}

void ARTSPlayerStart::BeginPlay()
{
	Super::BeginPlay();
	
	// Not needed once match has started. Maybe could call this even earlier than BeginPlay
	Destroy();
}

#if WITH_EDITOR
void ARTSPlayerStart::OnConstruction(const FTransform & Transform)
{
	Super::OnConstruction(Transform);

	/* Leaving this out because it will happen when playing PIE/Standalone and don't want that */
	//TellLevelVolumeToUpdateMapInfo();
}

void ARTSPlayerStart::PostEditChangeProperty(FPropertyChangedEvent & PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	TellLevelVolumeToUpdateMapInfo();
}

void ARTSPlayerStart::PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	TellLevelVolumeToUpdateMapInfo();
}

void ARTSPlayerStart::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);

	if (bFinished)
	{
		TellLevelVolumeToUpdateMapInfo();
	}
}

void ARTSPlayerStart::TellLevelVolumeToUpdateMapInfo()
{
	/* Check if there is a RTS level volume actor already placed in map and make them update 
	the maps info. If not then no biggie */
	TArray < AActor * > LevelVolumes;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARTSLevelVolume::StaticClass(), LevelVolumes);

	if (LevelVolumes.Num() > 0)
	{
		/* Use first one found. Not expecting multiple level volumes on same map */
		ARTSLevelVolume * LevelVolume = CastChecked<ARTSLevelVolume>(LevelVolumes[0]);
		LevelVolume->StoreMapInfo();
	}
}

void ARTSPlayerStart::SetUniqueID(uint8 InID)
{
	UniqueID = InID;
}

uint8 ARTSPlayerStart::GetUniquePlayerStartID() const
{
	return UniqueID;
}
#endif // WITH_EDITOR
