// Fill out your copyright notice in the Description page of Project Settings.

#include "MySphereComponent.h"


void UMySphereComponent::OnComponentCreated()
{
	Super::OnComponentCreated();

#if WITH_EDITOR

	// Hide ourselves in editor viewport. Too many circles. 
	// Would be even cooler if when selected the building actually showed this, similar to decal components
	if (GetWorld()->WorldType == EWorldType::Editor)
	{
		SetVisibility(false);
	}

#endif
}

const TArray < FOverlapInfo > & UMySphereComponent::GetOverlaps() const
{
	return OverlappingComponents;
}
