// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/InGameWidgetBase.h"

#include "Statics/OtherEnums.h"
#include "EndOfMatchWidget.generated.h"

class UButton;

/**
 *	The widget to appear when the match has ended
 */
UCLASS(Abstract)
class RTS_VER2_API UEndOfMatchWidget : public UInGameWidgetBase
{
	GENERATED_BODY()

public:

	virtual bool SetupWidget(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController) override;

	/* Setup for match result */
	virtual void SetupForResult(EMatchResult MatchResult);

	/* Override tick to allow widget animations */
	virtual void NativeTick(const FGeometry & MyGeometry, float InDeltaTime) override;

protected:

	/* Button to keep spectating even though match has ended */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton * Button_Spectate;

	/* Button to return to main menu */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton * Button_ReturnToMainMenu;

	/* Panel widgets, one for each result. Put whatever you want on them for example an image
	representing victory or defeat */

	/* Widget to show for victory result */
	UPROPERTY(meta = (BindWidgetOptional))
	UPanelWidget * Panel_Victory;

	/* Widget to show for draw result */
	UPROPERTY(meta = (BindWidgetOptional))
	UPanelWidget * Panel_Draw;

	/* Widget to show for defeat result */
	UPROPERTY(meta = (BindWidgetOptional))
	UPanelWidget * Panel_Defeat;

	UFUNCTION()
	void UIBinding_OnSpectateButtonClicked();

	UFUNCTION()
	void UIBinding_OnReturnToMainMenuButtonClicked();
};
