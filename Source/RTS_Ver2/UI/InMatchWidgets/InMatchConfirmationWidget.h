// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/InGameWidgetBase.h"
#include "InMatchConfirmationWidget.generated.h"

class UMyButton;
class ARTSPlayerController;
class URTSGameInstance;
class URTSHUD;


enum class EConfirmationWidgetPurpose : uint8
{
	ExitToMainMenu, 
	ExitToOperatingSystem,
};


/**
 *	Widgets that appear that say 'are you sure?'
 *
 *	TODO add a container and populate in ctor all the BindWidget widgest just like UInGameWidgetBase
 */
UCLASS(Abstract)
class RTS_VER2_API UInMatchConfirmationWidget : public UInGameWidgetBase
{
	GENERATED_BODY()
	
public:

	void InitialSetup(URTSGameInstance * GameInst, ARTSPlayerController * InPlayerController, 
		EConfirmationWidgetPurpose InPurpose);

public:

	void OnRequestToBeShown(URTSHUD * HUD);

	/* Called when the "no" button is clicked */
	void OnNoButtonClicked(URTSHUD * HUD);

	virtual bool RespondToEscapeRequest(URTSHUD * HUD) override;

	EConfirmationWidgetPurpose GetPurpose() const { return Purpose; }

protected:

	// switch on Purpose with these to decide what to do. Still need to add Purpose variable to this class
	void UIBinding_OnYesButtonLeftMousePress();
	void UIBinding_OnYesButtonLeftMouseReleased();
	void UIBinding_OnYesButtonRightMousePress();
	void UIBinding_OnYesButtonRightMouseReleased();
	void UIBinding_OnNoButtonLeftMousePress();
	void UIBinding_OnNoButtonLeftMouseReleased();
	void UIBinding_OnNoButtonRightMousePress();
	void UIBinding_OnNoButtonRightMouseReleased();

	static bool IsWidgetBound(UWidget * Widget) { return Widget != nullptr; }

	//-------------------------------------------------------
	//	Data
	//-------------------------------------------------------

	EConfirmationWidgetPurpose Purpose;

	/* The yes button */
	UPROPERTY(meta = (BindWidget))
	UMyButton * Button_Yes;

	/* The no button */
	UPROPERTY(meta = (BindWidget))
	UMyButton * Button_No;
};
