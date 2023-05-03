// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/InGameWidgetBase.h"
#include "PauseMenu.generated.h"

class UMyButton;
class UInMatchConfirmationWidget;
class URTSHUD;
class USettingsWidget;


/**
 *	In match pause menu
 *
 *	Have not implemented animations for this or the confirmation widgets. If I do make 
 *	sure to modify IsShowingOrPlayingShowAnimation() to take into account animations. 
 *
 *	Side note and this goes for I think all UUserWidget that get added to the HUD: 
 *	The anchors in the top left corner seem to be the way to go. If you anchor them full screen 
 *	then they do not show correctly. But just like the tooltip widgets I think I can get 
 *	them full screen.
 */
UCLASS(Abstract)
class RTS_VER2_API UPauseMenu : public UInGameWidgetBase
{
	GENERATED_BODY()

	static constexpr int32 SETTINGS_WIDGET_Z_ORDER = 1000;
	static constexpr int32 CONFIRMATION_WIDGET_Z_ORDER = 2000;
	
public:

	virtual bool SetupWidget(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController) override;

protected:

	void SetupBoundWidgets();
	void SpawnConfirmationWidgets();
	void SpawnSettingsWidget();

public:

	/* Return true if this widget will show a confirm exit to main menu widget */
	bool HasConfirmExitToMainMenuWidget() const;

	void ShowConfirmExitToMainMenuWidget(URTSHUD * HUD);

	/* Return true if this widget will show a confirm exit to operating system widget */
	bool HasConfirmExitToOperatingSystemWidget() const;

	void ShowConfirmExitToOperatingSystemWidget(URTSHUD * HUD);

	bool IsShowingOrPlayingShowAnimation() const;

	virtual bool RespondToEscapeRequest(URTSHUD * HUD) override;

	void ShowSettingsMenu();

protected:

	//-------------------------------------------------------
	//	Data
	//-------------------------------------------------------

	/* Button to close the pause menu and resume playing */
	UPROPERTY(meta = (BindWidgetOptional))
	UMyButton * Button_Resume;

	/* Button to show the settings menu */
	UPROPERTY(meta = (BindWidgetOptional))
	UMyButton * Button_SettingsMenu;

	USettingsWidget * SettingsWidget;

	/* The widget to use for the settings menu */
	UPROPERTY(EditAnywhere, Category = "RTS", meta = (DisplayName = "Settings Menu"))
	TSubclassOf<USettingsWidget> SettingsWidget_BP;

	/* Button to exit the match and return to the main menu */
	UPROPERTY(meta = (BindWidgetOptional))
	UMyButton * Button_ReturnToMainMenu;

	UInMatchConfirmationWidget * ConfirmExitToMainMenuWidget;

	/* Optional widget to appear when the player clicks the 'return to main menu' button */
	UPROPERTY(EditAnywhere, Category = "RTS", meta = (DisplayName = "Confirm Exit To Main Menu Widget"))
	TSubclassOf<UInMatchConfirmationWidget> ConfirmExitToMainMenuWidget_BP;

	/* Button to exit the match and return to the operating system */
	UPROPERTY(meta = (BindWidgetOptional))
	UMyButton * Button_ReturnToOperatingSystem;

	UInMatchConfirmationWidget * ConfirmExitToOperatingSystemWidget;

	/* Optional widget to appear when the player clicks the 'return to operating system' button */
	UPROPERTY(EditAnywhere, Category = "RTS", meta = (DisplayName = "Confirm Exit To Operating System Widget"))
	TSubclassOf<UInMatchConfirmationWidget> ConfirmExitToOperatingSystemWidget_BP;

	void UIBinding_OnResumeButtonLeftMouseButtonPress();
	void UIBinding_OnResumeButtonLeftMouseButtonReleased();
	void UIBinding_OnResumeButtonRightMouseButtonPress();
	void UIBinding_OnResumeButtonRightMouseButtonReleased();

	void UIBinding_OnSettingsButtonLeftMouseButtonPress();
	void UIBinding_OnSettingsButtonLeftMouseButtonReleased();
	void UIBinding_OnSettingsButtonRightMouseButtonPress();
	void UIBinding_OnSettingsButtonRightMouseButtonReleased();

	void UIBinding_OnReturnToMainMenuLeftMouseButtonPress();
	void UIBinding_OnReturnToMainMenuLeftMouseButtonReleased();
	void UIBinding_OnReturnToMainMenuRightMouseButtonPress();
	void UIBinding_OnReturnToMainMenuRightMouseButtonReleased();

	void UIBinding_OnReturnToOSLeftMouseButtonPress();
	void UIBinding_OnReturnToOSLeftMouseButtonReleased();
	void UIBinding_OnReturnToOSRightMouseButtonPress();
	void UIBinding_OnReturnToOSRightMouseButtonReleased();
};
