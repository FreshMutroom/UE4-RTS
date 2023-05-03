// Fill out your copyright notice in the Description page of Project Settings.

#include "StartingGridComponent.h"
#include "Engine/World.h"

#include "Miscellaneous/StartingGrid/StartingGrid.h"

void UStartingGridComponent::OnRegister()
{
	/* This doesn't evaluate true if grid is placed in editor map but I expect that it should,
	and as a consequence when tesing in PIE the grids selectables will not be spawned because
	they were never spawned in the first place when placed in editor map */
	if (GetWorld()->WorldType == EWorldType::EditorPreview
		|| GetWorld()->WorldType == EWorldType::GamePreview)
	{
		/* Allows seeing preview of what spawn grid will look like in editor */
		Super::OnRegister();
	}
	else
	{
		/* Skip UChildActorComponent::OnRegister because we don't want to spawn the child actor */
		USceneComponent::OnRegister();

		AStartingGrid * const Grid = CastChecked<AStartingGrid>(GetOwner());
		Grid->RegisterGridComponent(this);
	}
}

TSubclassOf<AActor> UStartingGridComponent::GetSelectableBP() const
{
	return GetChildActorClass();
}
