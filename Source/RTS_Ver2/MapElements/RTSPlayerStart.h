// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerStart.h"
#include "RTSPlayerStart.generated.h"

/**
 *	RTS player start implementation
 *
 *	Haven't thought about this fully but could probably use the default player start class since 
 *	it provides a ID tag in the form of an FName
 */
UCLASS(NotBlueprintable)
class RTS_VER2_API ARTSPlayerStart : public APlayerStart
{
	GENERATED_BODY()
	
public:
	
	ARTSPlayerStart(const FObjectInitializer & ObjectInitializer);

protected:

	virtual void BeginPlay() override;

public:

#if WITH_EDITOR
	
	virtual void OnConstruction(const FTransform & Transform) override;
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent & PropertyChangedEvent) override;
	virtual void PostEditChangeChainProperty(struct FPropertyChangedChainEvent & PropertyChangedEvent) override;
	virtual void PostEditMove(bool bFinished) override;

protected:

	/* Tell the level volume to update the map info */
	void TellLevelVolumeToUpdateMapInfo();

	/* Unique ID to easily identify */
	uint8 UniqueID;

public:

	void SetUniqueID(uint8 InID);
	uint8 GetUniquePlayerStartID() const;

#endif // WITH_EDITOR
};
