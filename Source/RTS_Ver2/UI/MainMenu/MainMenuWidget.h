// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/MainMenu/MainMenuWidgetBase.h"
#include "MainMenuWidget.generated.h"

class UButton;
class UMyGameInstance;

/**
 *	The main menu widget. This is the first widget that will appear after the game starts
 */
UCLASS(Abstract)
class RTS_VER2_API UMainMenuWidget : public UMainMenuWidgetBase
{
	GENERATED_BODY()

protected:

	virtual bool Setup() override; 

	/**
	 *	Button to open the play menu. The play menu is just another main menu
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton * Button_Play;

	/* Button to create a singleplayer lobby */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton * Button_CreateSingleplayer;

	/* Button to create a multiplayer lobby */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton * Button_CreateMultiplayer;

	/* Button to search for a multiplayer game to join */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton * Button_FindMultiplayer;

	/* Button to bring up settings menu for video/audio/game/control settings */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton * Button_Settings;

	/* Button to exit to the operating system */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton * Button_ExitToOS;

	/** 
	 *	Button to return to previous menu. Intended to be used if this widget is a play menu 
	 *	and not the main menu
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton * Button_Return;

	void SetupButtonFunctionality();

	//~ Button functionality bindings that will be set automatically if names match in editor 
	//~ but can be called from blueprints if desired

	UFUNCTION()
	void UIBinding_OnPlayButtonClicked();

	UFUNCTION()
	void UIBinding_OnCreateSingleplayerButtonClicked();

	UFUNCTION()
	void UIBinding_OnCreateMultiplayerButtonClicked();

	UFUNCTION()
	void UIBinding_OnFindMultiplayerButtonClicked();

	UFUNCTION()
	void UIBinding_OnSettingsButtonClicked();

	UFUNCTION()
	void UIBinding_OnExitToOSButtonClicked();

	UFUNCTION()
	void UIBinding_OnReturnButtonClicked();
};
