// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "MySphereComponent.generated.h"

/**
 *	Just a USphereComponent expect UPrimitiveComponent::OverlappingComponents has a getter. 
 */
UCLASS()
class RTS_VER2_API UMySphereComponent : public USphereComponent
{
	GENERATED_BODY()
	
public:

	virtual void OnComponentCreated() override;

	// @return - UPrimitiveComponent::OverlappingComponents
	const TArray < FOverlapInfo > & GetOverlaps() const;
};
