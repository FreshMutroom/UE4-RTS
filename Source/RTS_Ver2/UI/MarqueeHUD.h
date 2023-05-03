// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"

#include "Statics/OtherEnums.h" // For EMarqueeBoxDrawMethod
#include "MarqueeHUD.generated.h"

class ARTSPlayerController;
class ISelectable;

/**
 *	This class is responsible for drawing the marquee selection
 *  rectangle on screen and selecting units in the marquee box
 */
UCLASS()
class RTS_VER2_API AMarqueeHUD : public AHUD
{
	GENERATED_BODY()

	AMarqueeHUD();

	/* For 4.20 moved logic from BeginPlay to here because DrawHUD was getting PC == null */
	virtual void PostInitializeComponents() override;

	/* Sort of like Tick() for HUDs. Calls to draw things like
	rectangles etc must be put in this function */
	void DrawHUD() override;

	/* 2D coordinates for where LMB was pressed */
	FVector2D ClickLoc;

	/* 2D coords of current mouse location */
	FVector2D MouseLoc;

	/* Copies of these 4 variables are in GI */
	EMarqueeBoxDrawMethod MarqueeBoxDrawMethod;
	FLinearColor MarqueeBoxRectangleColor;
	FLinearColor MarqueeBoxBorderColor;
	float MarqueeBorderLineThickness;

	/* True if a marquee selection should be performed when DrawHUD
	is called */
	bool bPerformMarqueeASAP;

	/* Array of all units that are inside a pending marquee box. If the player
	lets go of the LMB they will be selected */
	UPROPERTY()
	TArray <TScriptInterface <ISelectable>> InsideBox;

	/* Reference to player controller */
	UPROPERTY()
	ARTSPlayerController * PC;

	/* Draw the marquee selection box on screen */
	void DrawSelectionBox();

	void DrawSelectionFilledRectangle(FVector2D Size);
	void DrawSelectionBorder(FVector2D Size);

	/* Clear InsideBox, making sure to remove their pending selection decals */
	void ClearHighlightedSelectables();

	void HighlightUnitsInsideBox(float ExtentMultiplier = 0.125f);

	/* Selects all units in marquee box puts them in OutActors,
	or if no units were in the marquee box then does not modify
	OutActors. OutActors should not be cleared before calling
	this. Also puts the unit who should have their context menu
	shown into OutActors[0]
	@param FirstPoint - A corner of the rectangle
	@param SecondPoint - Another corner of the rectangle
	@param OutActors - The array to add the units to
	@param ExtentMultiplier - What to multiply the bounds by to make
	it harder to select units partially inside the selection box
	@return - true if the selection changed from what was selected before */
	bool MakeMarqueeSelection(const FVector2D & FirstPoint, const FVector2D & SecondPoint, TArray<class AActor *> & OutActors, float ExtentMultiplier = 0.125f);

public:

	void SetClickLocation(FVector2D NewCoords);
	void SetMouseLoc(FVector2D NewCoords);
	void SetMarqueeSettingsValues(EMarqueeBoxDrawMethod InDrawMethod, const FLinearColor & InRectangleColor, 
		const FLinearColor & InBorderColor, float InBorderLineThickness);

	/* Set bPerfromMarqueeASAP which if true will perform a marquee
	selection on next tick and put actors in PC->Selected */
	void SetPerformMarqueeASAP(bool bNewValue);

	void SetPC(ARTSPlayerController * PlayerController);
};
