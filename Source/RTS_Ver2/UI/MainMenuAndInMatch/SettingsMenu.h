// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SettingsMenu.generated.h"

class URTSGameInstance;
class ARTSPlayerController;
class UKeyBindingsWidget;
class UAudioSettingsWidget;
class UControlSettingsWidget;
class UWidgetSwitcher;
class UMyButton;
class USettingsWidget;
class UGameSettingsWidget;
class UVideoSettingsWidget;


/* What instigated the closing of the widget */
enum class ECloseInstigation : uint8
{
	/* Player pressed the cancel key which is ESC by default */
	EscapeRequest, 
	
	/* Player clicked on a UI button */
	ButtonWidget,
};


/** 
 *	What to do with unsaved changes when the cancel key is pressed to close the settings menu. 
 *	Currently key bindings are saved to disk the moment they are changed I think so those won't 
 *	be affected by this 
 */
UENUM()
enum class ECloseRequestBehavior : uint8
{
	/* Load from disk and apply those, so any unsaved changes are not saved */
	DoNotSave, 
	
	/* Apply unsaved changes and save them to disk */
	Save, 
	
	/* Ask the user what to do */
	Ask,
};


UENUM()
enum class ESettingsSubmenuType : uint8
{
	VideoSettings, 
	AudioSettings, 
	ControlSettings, 
	KeyBindings, 
	GameSettings
};


/** 
 *	Base class for widgets that are a submenu of the settings menu.
 *	Example of submenus: video settings, control settings 
 */
UCLASS(Abstract)
class RTS_VER2_API USettingsSubmenuBase : public UUserWidget
{
	GENERATED_BODY()

public:
	
	/* Update the appearance of the widget to the currently applied values */
	virtual void UpdateAppearanceForCurrentValues() PURE_VIRTUAL(USettingsSubmenuBase::UpdateAppearanceForCurrentValues, );

protected:

	virtual void NativeTick(const FGeometry & MyGeometry, float InDeltaTime) override;

	static bool IsWidgetBound(UWidget * Widget) { return Widget != nullptr; }
};


/** 
 *	Widget that asks player "are you sure?" when deciding whether to apply or discard changes 
 *
 *	This gets displayed overtop the settings menu. Make sure something on this blocks the
 *	settings menu from getting mouse events.
 */
UCLASS(Abstract)
class RTS_VER2_API USettingsConfirmationWidget_Exit : public UUserWidget
{
	GENERATED_BODY()

public:

	void InitialSetup(URTSGameInstance * GameInst, ARTSPlayerController * PlayCon, 
		USettingsWidget * InSettingsMenu);

protected:

	virtual void NativeTick(const FGeometry & MyGeometry, float InDeltaTime) override;

	static bool IsWidgetBound(UWidget * Widget) { return Widget != nullptr; }

	void UIBinding_OnConfirmButtonLeftMousePress();
	void UIBinding_OnConfirmButtonLeftMouseRelease();
	void UIBinding_OnConfirmButtonRightMousePress();
	void UIBinding_OnConfirmButtonRightMouseRelease();

	void UIBinding_OnDiscardButtonLeftMousePress();
	void UIBinding_OnDiscardButtonLeftMouseRelease();
	void UIBinding_OnDiscardButtonRightMousePress();
	void UIBinding_OnDiscardButtonRightMouseRelease();

	void UIBinding_OnCancelButtonLeftMousePress();
	void UIBinding_OnCancelButtonLeftMouseRelease();
	void UIBinding_OnCancelButtonRightMousePress();
	void UIBinding_OnCancelButtonRightMouseRelease();

public:

	void OnSaveButtonClicked();
	void OnDiscardButtonClicked();
	void OnCancelButtonClicked();

	void OnRequestToShow();
	void OnEscapeRequest();

protected:

	//----------------------------------------------------------
	//	Data
	//----------------------------------------------------------

	ARTSPlayerController * PC;
	// Widget this widget is on
	USettingsWidget * SettingsMenu;

	/* Button to save changes */
	UPROPERTY(meta = (BindWidgetOptional))
	UMyButton * Button_Save;

	/* Button to discard changes */
	UPROPERTY(meta = (BindWidgetOptional))
	UMyButton * Button_DoNotSave;

	/* Button to close this confirmation widget and go back to the settings menu */
	UPROPERTY(meta = (BindWidgetOptional))
	UMyButton * Button_Cancel;
};


/** 
 *	Widget that asks the player if they are sure they want to reset settings to default. 
 *	
 *	This gets displayed overtop the settings menu. Make sure something on this blocks the 
 *	settings menu from getting mouse events. 
 */
UCLASS(Abstract)
class RTS_VER2_API USettingsConfirmationWidget_ResetToDefaults : public UUserWidget
{
	GENERATED_BODY()

public:

	void InitialSetup(URTSGameInstance * GameInst, ARTSPlayerController * PlayCon,
		USettingsWidget * InSettingsMenu);

protected:

	virtual void NativeTick(const FGeometry & MyGeometry, float InDeltaTime) override;

	static bool IsWidgetBound(UWidget * Widget) { return Widget != nullptr; }

public:

	void OnRequestToShow();
	void OnEscapeRequest();

protected:

	void UIBinding_OnYesButtonLeftMousePress();
	void UIBinding_OnYesButtonLeftMouseRelease();
	void UIBinding_OnYesButtonRightMousePress();
	void UIBinding_OnYesButtonRightMouseRelease();

	void UIBinding_OnNoButtonLeftMousePress();
	void UIBinding_OnNoButtonLeftMouseRelease();
	void UIBinding_OnNoButtonRightMousePress();
	void UIBinding_OnNoButtonRightMouseRelease();

public:

	void OnYesButtonClicked();
	void OnNoButtonClicked();

protected:

	//----------------------------------------------------------------
	//	Data
	//----------------------------------------------------------------

	ARTSPlayerController * PC;
	// Widget this widget is on
	USettingsWidget * SettingsMenu;

	/* I may use this user widget for control settings, video settings etc so this text 
	can display the correct settings type e.g. "Are you sure you want to reset all 
	<Settings name here> to default?" */
	//UPROPERTY(meta = (BindWidgetOptional))
	//UTextBlock * Text_Message;

	/* Button to go ahead and reset settings back to defaults */
	UPROPERTY(meta = (BindWidgetOptional))
	UMyButton * Button_Yes;

	/* Button to go back */
	UPROPERTY(meta = (BindWidgetOptional))
	UMyButton * Button_No;
};


/* The widget that shows settings like video settings, key mappings etc */
UCLASS(Abstract)
class RTS_VER2_API USettingsWidget : public UUserWidget
{
	GENERATED_BODY()

	static constexpr int32 RESET_TO_DEFAULTS_CONFIRMATION_WIDGET_Z_ORDER = 1000;
	static constexpr int32 EXIT_CONFIRMATION_WIDGET_Z_ORDER = 2000;

public:

	USettingsWidget(const FObjectInitializer & ObjectInitializer);

	void InitialSetup(URTSGameInstance * GameInst, ARTSPlayerController * PlayCon);

protected:

	void SetupConfirmationWidgets();

	/* Whether showing a exit confirmation widget is a possibility */
	bool UsingExitConfirmationWidget() const;

	/* Whether to show a confirmation widget when resetting settings back to defaults */
	bool UsingResetToDefaultsConfirmationWidget() const;

	virtual void NativeTick(const FGeometry & MyGeometry, float InDeltaTime) override;

public:

	bool IsShowingOrPlayingShowAnim() const;

	void Show();

	/* Called when the player has pressed the cancel key and the request should be handled 
	by this menu */
	void OnEscapeRequest();

	void ApplyAndSaveChangesAndInitiateClose(ECloseInstigation CloseInstigation);
	void DiscardChangesAndInitiateClose(ECloseInstigation CloseInstigation);

protected:

	void ShowAskSaveChangesWidget();

	void ShowConfirmResetToDefaultsWidget();

	/* Starts closing the settings menu */
	void InitiateClose(ECloseInstigation CloseInstigation);

	bool IsExitConfirmationWidgetShowingOrPlayingShowAnim() const;
	bool IsResetToDefaultsConfirmationWidgetShowingOrPlayingShowAnim() const;

	void UIBinding_OnSaveChangesAndReturnButtonLeftMousePress();
	void UIBinding_OnSaveChangesAndReturnButtonLeftMouseRelease();
	void UIBinding_OnSaveChangesAndReturnButtonRightMousePress();
	void UIBinding_OnSaveChangesAndReturnButtonRightMouseRelease();

	void UIBinding_OnDiscardChangesAndReturnButtonLeftMousePress();
	void UIBinding_OnDiscardChangesAndReturnButtonLeftMouseRelease();
	void UIBinding_OnDiscardChangesAndReturnButtonRightMousePress();
	void UIBinding_OnDiscardChangesAndReturnButtonRightMouseRelease();

	void UIBinding_OnResetToDefaultsButtonLeftMousePress();
	void UIBinding_OnResetToDefaultsButtonLeftMouseRelease();
	void UIBinding_OnResetToDefaultsButtonRightMousePress();
	void UIBinding_OnResetToDefaultsButtonRightMouseRelease();

	void UIBinding_OnSwitchToVideoSubmenuButtonLeftMousePress();
	void UIBinding_OnSwitchToVideoSubmenuButtonLeftMouseRelease();
	void UIBinding_OnSwitchToVideoSubmenuButtonRightMousePress();
	void UIBinding_OnSwitchToVideoSubmenuButtonRightMouseRelease();

	void UIBinding_OnSwitchToAudioSubmenuButtonLeftMousePress();
	void UIBinding_OnSwitchToAudioSubmenuButtonLeftMouseRelease();
	void UIBinding_OnSwitchToAudioSubmenuButtonRightMousePress();
	void UIBinding_OnSwitchToAudioSubmenuButtonRightMouseRelease();
	
	void UIBinding_OnSwitchToControlsSubmenuButtonLeftMousePress();
	void UIBinding_OnSwitchToControlsSubmenuButtonLeftMouseRelease();
	void UIBinding_OnSwitchToControlsSubmenuButtonRightMousePress();
	void UIBinding_OnSwitchToControlsSubmenuButtonRightMouseRelease();
	
	void UIBinding_OnSwitchToKeyBindingsSubmenuButtonLeftMousePress();
	void UIBinding_OnSwitchToKeyBindingsSubmenuButtonLeftMouseRelease();
	void UIBinding_OnSwitchToKeyBindingsSubmenuButtonRightMousePress();
	void UIBinding_OnSwitchToKeyBindingsSubmenuButtonRightMouseRelease();

	void UIBinding_OnSwitchToGameSubmenuButtonLeftMousePress();
	void UIBinding_OnSwitchToGameSubmenuButtonLeftMouseRelease();
	void UIBinding_OnSwitchToGameSubmenuButtonRightMousePress();
	void UIBinding_OnSwitchToGameSubmenuButtonRightMouseRelease();

public:

	void OnSwitchToSubmenuButtonClicked(UWidget * WidgetToSwitchTo);
	void OnSaveChangesAndReturnButtonClicked();
	void OnDiscardChangesAndReturnButtonClicked();
	void OnResetToDefaultsButtonClicked();
	void ResetAllSettingsToDefaults();

protected:

	static bool IsWidgetBound(UWidget * Widget) { return Widget != nullptr; }

	//------------------------------------------------------------
	//	Data
	//------------------------------------------------------------

	URTSGameInstance * GI;
	ARTSPlayerController * PC;

	/* Button to apply/save changes and return to the previous menu */
	UPROPERTY(meta = (BindWidgetOptional))
	UMyButton * Button_SaveChangesAndReturn;

	/* Button to not save changes and return to the previous menu */
	UPROPERTY(meta = (BindWidgetOptional))
	UMyButton * Button_DiscardChangesAndReturn;

	/* Button that changes all settings back to defaults */
	UPROPERTY(meta = (BindWidgetOptional))
	UMyButton * Button_ResetToDefaults;

	/* Widget switcher to put each submenu on */
	UPROPERTY(meta = (BindWidgetOptional))
	UWidgetSwitcher * WidgetSwitcher_Submenus;

	/** Button to switch to the video settings submenu */
	UPROPERTY(meta = (BindWidgetOptional))
	UMyButton * Button_Video;

	UPROPERTY(meta = (BindWidgetOptional))
	UVideoSettingsWidget * Widget_Video;

	/* Button to switch to the audio submenu */
	UPROPERTY(meta = (BindWidgetOptional))
	UMyButton * Button_Audio;

	UPROPERTY(meta = (BindWidgetOptional))
	UAudioSettingsWidget * Widget_Audio;

	/* Button to switch to the controls submenu */
	UPROPERTY(meta = (BindWidgetOptional))
	UMyButton * Button_Controls;

	UPROPERTY(meta = (BindWidgetOptional))
	UControlSettingsWidget * Widget_Controls;

	/* Button to switch to the key bindings submenu */
	UPROPERTY(meta = (BindWidgetOptional))
	UMyButton * Button_KeyBindings;

	UPROPERTY(meta = (BindWidgetOptional))
	UKeyBindingsWidget * Widget_KeyBindingsMenu;

	/* Button to switch to the game settings submenu */
	UPROPERTY(meta = (BindWidgetOptional))
	UMyButton * Button_Game;

	UPROPERTY(meta = (BindWidgetOptional))
	UGameSettingsWidget * Widget_Game;

	/** The individual menu to be on when the settings menu is opened */
	UPROPERTY(EditAnywhere, Category = "RTS")
	ESettingsSubmenuType DefaultSubmenu;

	/** 
	 *	Whether to remember which individual menu was open last time 
	 *	My notes: this may or may not persistest through map changes. If it does not considier 
	 *	storing a variable on the GI
	 */
	UPROPERTY(EditAnywhere, Category = "RTS")
	bool bRememberSubmenu;

	/* What to do with unsaved changes when the player closes the menu by pressing the cancel key */
	UPROPERTY(EditAnywhere, Category = "RTS")
	ECloseRequestBehavior UnsavedChangesBehavior_CloseFromCancelKey;

	/* Widget to show when this widget is shown */
	UWidget * SubmenuToShowOnNextOpen;

	/* Widget that asks player whether to apply and save unsaved changes when closing the widget */
	UPROPERTY(EditAnywhere, Category = "RTS", meta = (DisplayName = "Exit Confirmation Widget", EditCondition = bCanEditConfirmationWidget))
	TSubclassOf<USettingsConfirmationWidget_Exit> ExitConfirmationWidget_BP;

	USettingsConfirmationWidget_Exit * ConfirmationWidget_Exit;

	/* Widget that asks the player "are you sure you want to reset all settings to defaults?" */
	UPROPERTY(EditAnywhere, Category = "RTS", meta = (DisplayName = "Reset to Defaults Confirmation Widget"))
	TSubclassOf<USettingsConfirmationWidget_ResetToDefaults> DefaultsConfirmationWidget_BP;

	USettingsConfirmationWidget_ResetToDefaults * ConfirmationWidget_ResetToDefaults;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	bool bCanEditConfirmationWidget;
#endif

#if WITH_EDITOR
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent) override;
#endif
};
