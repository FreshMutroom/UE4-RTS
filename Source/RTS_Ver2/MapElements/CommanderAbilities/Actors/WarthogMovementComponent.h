// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/MovementComponent.h"
#include "WarthogMovementComponent.generated.h"

class AWarthog;


/**
 * 
 */
UCLASS()
class RTS_VER2_API UWarthogMovementComponent : public UMovementComponent
{
	GENERATED_BODY()
	
public:

	UWarthogMovementComponent();

protected:

	virtual void InitializeComponent() override;

	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction * ThisTickFunction) override;

	// GetOwner() casted to AWarthog
	AWarthog * GetOwnerAsWarthog() const;

public:

	void OnOwningAbilityUsed(const FVector & TargetLocation);

protected:

	//----------------------------------------------------------------
	//	Data
	//----------------------------------------------------------------

	FVector OriginalVelocity;

	/** 
	 *	How long it takes to go from flying straight to flying at the decent angle 
	 *	specified on AWarthog. 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = "0.0001"))
	float DescentTransitionTime;

	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = "0.0001"))
	float GetawayTransitionTime;
};
