// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ChildActorComponent.h"
#include "StartingGridComponent.generated.h"

/**
 *	A component to be added to a starting grid that allows an actor to be specified as a 
 *	"starting" selectable
 */
UCLASS(ClassGroup = RTS, meta = (BlueprintSpawnableComponent))
class RTS_VER2_API UStartingGridComponent : public UChildActorComponent
{
	GENERATED_BODY()

public:

	virtual void OnRegister() override;

	TSubclassOf <AActor> GetSelectableBP() const;

};
