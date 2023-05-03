// Fill out your copyright notice in the Description page of Project Settings.

#include "MarqueeHUD.h"
#include "Public/EngineUtils.h"
#include "Engine/Canvas.h"

#include "GameFramework/RTSPlayerController.h"
#include "GameFramework/RTSPlayerState.h"
#include "Statics/Statics.h"
#include "Statics/DevelopmentStatics.h"
#include "MapElements/Infantry.h"


AMarqueeHUD::AMarqueeHUD()
{
	bShowHUD = false;
}

void AMarqueeHUD::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	PC = CastChecked<ARTSPlayerController>(GetOwningPlayerController());

	bShowHUD = true;
}

void AMarqueeHUD::DrawHUD()
{
	Super::DrawHUD(); // TODO this at bottom of func not top any more responsive?

	/* Remove highlight decals from all units inside box */
	ClearHighlightedSelectables();

	/* Either make marquee select or highlight all units inside box */
	if (bPerformMarqueeASAP)
	{
		bPerformMarqueeASAP = false;

		/* Select units that should be selected */
		const bool bHasSelectionChanged = MakeMarqueeSelection(ClickLoc, MouseLoc, PC->GetSelected());

		/* Let PC know if we selected something */
		PC->OnMarqueeSelect(bHasSelectionChanged);
	}
	else if (PC->IsMarqueeActive())
	{
		DrawSelectionBox();

		/* Highlight units inside box */
		HighlightUnitsInsideBox();
	}
}

void AMarqueeHUD::DrawSelectionBox()
{
	/* Calculate size of marquee box */
	FVector2D Size = FVector2D(MouseLoc.X - ClickLoc.X, MouseLoc.Y - ClickLoc.Y);

	/* Draw marquee selection box */
	if (MarqueeBoxDrawMethod == EMarqueeBoxDrawMethod::BorderOnly)
	{
		DrawSelectionBorder(Size);
	}
	else if (MarqueeBoxDrawMethod == EMarqueeBoxDrawMethod::FilledRectangleOnly)
	{
		DrawSelectionFilledRectangle(Size);
	}
	else // Assuming BorderAndFill
	{
		assert(MarqueeBoxDrawMethod == EMarqueeBoxDrawMethod::BorderAndFill);

		DrawSelectionFilledRectangle(Size);
		DrawSelectionBorder(Size);
	}
}

void AMarqueeHUD::DrawSelectionFilledRectangle(FVector2D Size)
{
	DrawRect(MarqueeBoxRectangleColor, ClickLoc.X, ClickLoc.Y, Size.X, Size.Y);
}

void AMarqueeHUD::DrawSelectionBorder(FVector2D Size)
{
	/* How much to shift the border so it renders in the desired spot */
	FVector2D ShiftAmount;
	
	if (HUDOptions::MarqueeBoxPositioningRule == EMarqueeBoxOutlinePositioningRule::Mine)
	{
		if (MouseLoc.X < ClickLoc.X)
		{
			if (MouseLoc.Y < ClickLoc.Y)
			{
				ShiftAmount = FVector2D(-MarqueeBorderLineThickness / 2, -MarqueeBorderLineThickness / 2);
			}
			else
			{
				ShiftAmount = FVector2D(-MarqueeBorderLineThickness / 2, MarqueeBorderLineThickness / 2);
			}
		}
		else
		{
			if (MouseLoc.Y < ClickLoc.Y)
			{
				ShiftAmount = FVector2D(MarqueeBorderLineThickness / 2, -MarqueeBorderLineThickness / 2);
			}
			else
			{
				ShiftAmount = FVector2D(MarqueeBorderLineThickness / 2, MarqueeBorderLineThickness / 2);
			}
		}
	}
	else
	{
		assert(HUDOptions::MarqueeBoxPositioningRule == EMarqueeBoxOutlinePositioningRule::Standard);
		
		ShiftAmount = FVector2D(0.f, 0.f);
	}

	FCanvasBoxItem RectItem(ClickLoc + ShiftAmount, Size);
	RectItem.SetColor(MarqueeBoxBorderColor);
	RectItem.LineThickness = MarqueeBorderLineThickness;
	Canvas->DrawItem(RectItem);
}

void AMarqueeHUD::ClearHighlightedSelectables()
{
	for (int32 i = InsideBox.Num() - 1; i >= 0; --i)
	{
		InsideBox[i]->OnExitMarqueeBox(PC);
		InsideBox.RemoveAt(i, 1, false);
	}
}

void AMarqueeHUD::HighlightUnitsInsideBox(float ExtentMultiplier)
{
	/* Most of this is borrowed from AHUD::GetActorsInSelectionRectangle() */

	//Create Selection Rectangle from Points
	FBox2D SelectionRectangle(ForceInit);

	//This method ensures that an appropriate rectangle is generated, 
	//		no matter what the coordinates of first and second point actually are.
	SelectionRectangle += ClickLoc;
	SelectionRectangle += MouseLoc;


	//The Actor Bounds Point Mapping
	const FVector BoundsPointMapping[8] =
	{
		FVector(1.f, 1.f, 1.f),
		FVector(1.f, 1.f, -1.f),
		FVector(1.f, -1.f, 1.f),
		FVector(1.f, -1.f, -1.f),
		FVector(-1.f, 1.f, 1.f),
		FVector(-1.f, 1.f, -1.f),
		FVector(-1.f, -1.f, 1.f),
		FVector(-1.f, -1.f, -1.f) };

	//~~~

	const TArray < AInfantry * > & UnitArray = PC->GetPS()->GetUnits();

	//For Each Actor of the Class Filter Type
	for (AInfantry * EachActor : UnitArray)
	{
		/* Null check because networking */
		if (!Statics::IsValid(EachActor))
		{
			continue;
		}

		//Get Actor Bounds				//casting to base class, checked by template in the .h
		const FBox EachActorBounds = EachActor->GetComponentsBoundingBox(false);

		//Center
		const FVector BoxCenter = EachActorBounds.GetCenter();

		//Extents
		const FVector BoxExtents = EachActorBounds.GetExtent() * ExtentMultiplier;

		// Build 2D bounding box of actor in screen space
		FBox2D ActorBox2D(ForceInit);
		for (uint8 BoundsPointItr = 0; BoundsPointItr < 8; BoundsPointItr++)
		{
			// Project vert into screen space.
			const FVector ProjectedWorldLocation = Project(BoxCenter + (BoundsPointMapping[BoundsPointItr] * BoxExtents));
			// Add to 2D bounding box
			ActorBox2D += FVector2D(ProjectedWorldLocation.X, ProjectedWorldLocation.Y);
		}

		/* Toggle different selection criteria:
		.IsInside: selection box must fully enclose actor bounds
		.Intersect: selection box has partial intersection with actor bounds */
		if (SelectionRectangle.IsInside(ActorBox2D))
		{
			EachActor->OnEnterMarqueeBox(PC);

			InsideBox.Emplace(EachActor);
		}
	}
}

bool AMarqueeHUD::MakeMarqueeSelection(const FVector2D & FirstPoint, const FVector2D & SecondPoint, 
	TArray <AActor *> & OutActors, float ExtentMultiplier)
{
	/* TODO: Remove OutActors as param and just replace it with PC->GetSelected() */

	/* This code will not run from anywhere other than DrawHUD().
	My guess is the bounds are not calculated correctly unless
	done in DrawHUD */

	/* This is a big function. Some of it has been borrowed from
	AHUD::GetActorsInSelectionRectangle in UE4 source. */

	/* Keep track of whether the players selection changed at all. Don't really need
	to do this anymore but do need to keep SelectedIDs up to date */
	bool bHasSelectionChanged = false;

	/* Updating IDs only needs to be done for clients, but is done for server too anyway */

	TArray < AActor *, TInlineAllocator< 16 > > Selected;

	/* For checking what to call OnDeselect() on */
	TSet <TScriptInterface<ISelectable>> NewSelected;

	/* Index in Selected of the unit who will have their context menu shown */
	int32 ContextUnitIndex = 0;

	/* To keep track of which context menu to show */
	EUnitType ContextToShow = EUnitType::None;

	/* Setup selection critera */

	//Create Selection Rectangle from Points
	FBox2D SelectionRectangle(ForceInit);

	//This method ensures that an appropriate rectangle is generated, 
	//		no matter what the coordinates of first and second point actually are.
	SelectionRectangle += FirstPoint;
	SelectionRectangle += SecondPoint;

	//The Actor Bounds Point Mapping
	const FVector BoundsPointMapping[8] =
	{
		FVector(1.f, 1.f, 1.f),
		FVector(1.f, 1.f, -1.f),
		FVector(1.f, -1.f, 1.f),
		FVector(1.f, -1.f, -1.f),
		FVector(-1.f, 1.f, 1.f),
		FVector(-1.f, 1.f, -1.f),
		FVector(-1.f, -1.f, 1.f),
		FVector(-1.f, -1.f, -1.f) };


	const TArray < AInfantry * > & UnitArray = PC->GetPS()->GetUnits();

	/* Check every unit this player controls. Units only; buildings cannot be
	selected by marquee select */
	for (AInfantry * Unit : UnitArray)
	{
		/* Null check because networking */
		if (!Statics::IsValid(Unit))
		{
			continue;
		}

		/* Get Actor bounds */
		const FBox Bounds = Unit->GetComponentsBoundingBox(false);

		// Center
		const FVector BoxCenter = Bounds.GetCenter();

		//Extents
		const FVector BoxExtents = Bounds.GetExtent() * ExtentMultiplier;

		// Build 2D bounding box of actor in screen space
		FBox2D ActorBox2D(ForceInit);
		for (uint8 BoundsPointItr = 0; BoundsPointItr < 8; ++BoundsPointItr)
		{
			// Project vert into screen space.
			const FVector ProjectedWorldLocation = Project(BoxCenter + (BoundsPointMapping[BoundsPointItr] * BoxExtents));
			// Add to 2D bounding box
			ActorBox2D += FVector2D(ProjectedWorldLocation.X, ProjectedWorldLocation.Y);
		}

		/* Toggle different selection criteria:
		.IsInside: selection box must fully enclose actor bounds
		.Intersect: selection box has partial intersection with actor bounds */
		if (SelectionRectangle.IsInside(ActorBox2D))
		{
			/* Check if unit is alive */
			if (!Statics::HasZeroHealth(Unit))
			{
				/* If here we know the actor will be counted as selected */

				/* Add actor to array and set */
				Selected.Add(Unit);
				const FSetElementId ID = NewSelected.Emplace(Unit);

				/* Do OnMarqueeSelect on unit and get its type to find out
				what context menu to show. Also get ID to see if selection has changed */
				uint8 SelectableID;
				const EUnitType UnitType = NewSelected[ID]->OnMarqueeSelect(SelectableID);
				assert(SelectableID != 0);

				if (!PC->IsSelected(SelectableID))
				{
					bHasSelectionChanged = true;
					PC->AddToSelectedIDs(SelectableID);
				}

				if (UnitType > ContextToShow)
				{
					/* Save index to know what to swap to index 0 of OutActors */
					ContextUnitIndex = Selected.Num() - 1;
					ContextToShow = UnitType;
				}
			}
		}
	}

	/* If have selected a different amount of selectables than before then selection
	has changed. From here on out bHasSelectionChanged will not change */
	if (Selected.Num() != OutActors.Num())
	{
		bHasSelectionChanged = true;
	}


	/* Modify OutActors and do it in a way that there is no shuffling in
	the TArray. Call deselect on anything that wasn't selected. Can
	probably be done simpler by just iterating along array from start till
	end */

	/* This if is required since the 3rd for loop below will deselect
	current selection if nothing is selected with marquee which is not
	the behaviour I want */
	if (Selected.Num() > 0)
	{
		const int32 CurrentSize = OutActors.Num();
		const int32 NewSize = Selected.Num();

		if (CurrentSize <= NewSize)
		{
			/* Add actors to the end of OutActors */
			for (int32 i = CurrentSize; i < NewSize; ++i)
			{
				/* No cast here required!?!? */
				OutActors.Emplace(Selected[i]);
			}
			/* Add the rest of the actors */
			for (int32 i = 0; i < CurrentSize; ++i)
			{
				if (!NewSelected.Contains(OutActors[i]))
				{
					ISelectable * const AsSelectable = CastChecked<ISelectable>(OutActors[i]);
					const uint8 SelectableID = AsSelectable->OnDeselect();
					PC->RemoveFromSelectedIDs(SelectableID);
				}
				OutActors[i] = Selected[i];
			}
		}
		else
		{
			/* CurrentSize > NewSize */

			/* Iterate backwards through OutActors removing elements */
			for (int32 i = CurrentSize - 1; i >= NewSize; --i)
			{
				if (!NewSelected.Contains(OutActors[i]))
				{
					ISelectable * const AsSelectable = CastChecked<ISelectable>(OutActors[i]);
					const uint8 SelectableID = AsSelectable->OnDeselect();
					PC->RemoveFromSelectedIDs(SelectableID);
				}
				OutActors.RemoveAt(i);
			}
			/* Add the rest of the actors */
			for (int32 i = 0; i < NewSize; ++i)
			{
				if (!NewSelected.Contains(OutActors[i]))
				{
					ISelectable * const AsSelectable = CastChecked<ISelectable>(OutActors[i]);
					const uint8 SelectableID = AsSelectable->OnDeselect();
					PC->RemoveFromSelectedIDs(SelectableID);
				}
				OutActors[i] = Selected[i];
			}
		}

		/* Swap unit whose context menu will be shown into index 0. 
		This commented line was here before. After testing I'm pretty sure it's wrong but 
		leaving it here in case I was wrong about assuming it is wrong */
		//OutActors.SwapMemory(ContextUnitIndex, Selected.Num() - 1);
		OutActors.SwapMemory(0, ContextUnitIndex);
	}

	return bHasSelectionChanged;
}

void AMarqueeHUD::SetClickLocation(FVector2D NewCoords)
{
	ClickLoc = NewCoords;
}

void AMarqueeHUD::SetMouseLoc(FVector2D NewCoords)
{
	MouseLoc = NewCoords;
}

void AMarqueeHUD::SetMarqueeSettingsValues(EMarqueeBoxDrawMethod InDrawMethod, const FLinearColor & InRectangleColor, 
	const FLinearColor & InBorderColor, float InBorderLineThickness)
{
	MarqueeBoxDrawMethod = InDrawMethod;
	MarqueeBoxRectangleColor = InRectangleColor;
	MarqueeBoxBorderColor = InBorderColor;
	MarqueeBorderLineThickness = InBorderLineThickness;
}

void AMarqueeHUD::SetPerformMarqueeASAP(bool bNewValue)
{
	bPerformMarqueeASAP = bNewValue;
}

void AMarqueeHUD::SetPC(ARTSPlayerController * PlayerController)
{
	PC = PlayerController;
}
