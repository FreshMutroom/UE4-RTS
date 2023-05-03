// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

/*------------------------------------------------------------------------------------------------
	File for the key bindings menu - the menu that lets the player change what actions are bound 
	to what keys. 
------------------------------------------------------------------------------------------------*/


#include "CoreMinimal.h"
#include "UI/MainMenuAndInMatch/SettingsMenu.h"

#include "Statics/Structs/Structs_8.h"
#include "KeyMappingMenu.generated.h"

class URTSGameUserSettings;
class UWidget;
enum class EKeyMappingAction : uint16;
enum class EKeyMappingAxis : uint16;
class UTextBlock;
struct FInputInfoBase;
class UBorder;
class URTSGameInstance;
class UMyButton;
struct FKeyWithModifiers;
struct FKeyInfo;
enum class EInputKeyDisplayMethod : uint8;
class ARTSPlayerController;
class UKeyBindingsWidget;
struct FTryBindActionResult;


/**
 *	A widget that displays the key binding for a single game action.
 *	
 *	There is the option to display the assinged key with a text block 
 *	Additionally you can show an image to represent the binding. This can be done in two ways:
 *	- you supply an image of the key 
 *	- you supply an image of the key and we'll put the text on it for you
 *
 *	If you have images for keys + their modifiers then congratulations: I actually didn't 
 *	implement anything for that. But just chuck a struct into game instance like 
 *	TMap<FKeyWithModifiers, FKeyWithModifiersInfo> and you can easily get your image 
 *	that way.
 */
UCLASS(Abstract)
class RTS_VER2_API USingleKeyBindingWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	USingleKeyBindingWidget(const FObjectInitializer & ObjectInitializer);

	void InitialSetup(URTSGameInstance * GameInst, ARTSPlayerController * InPlayerController, 
		UKeyBindingsWidget * InKeyBindingsWidget);

	void SetTypes(EKeyMappingAction InActionType, EKeyMappingAxis InAxisType);

protected:

	// Overridden probably because we wanna turn tick off for performance
	virtual void NativeTick(const FGeometry & MyGeometry, float InDeltaTime) override;

public:

	void UpdateAppearanceForCurrentBindingValue(URTSGameInstance * GameInst, URTSGameUserSettings * GameUserSettings);

	/* Return true if for action mapping, false if for axis mapping */
	bool IsForActionMapping() const;
	EKeyMappingAction GetActionType() const;
	EKeyMappingAxis GetAxisType() const;

	/* Return the text to display that represents a key + modifiers. This is the text for 
	Text_Action */
	static FText GetDisplayText(const FKeyWithModifiers & Key, const FKeyInfo & KeyInfo);

protected:

	static void SetBrushAndText_Modifier(const FSlateBrush & ModifierBrush, UBorder * Border, UTextBlock * TextBlock);
	static void SetBrushAndText_PlusSymbol(const FSlateBrush & InBrush, UBorder * Border, UTextBlock * TextBlock);

	/* Set the image on the border to represent a key. Optionally set the text on a text 
	block too if required. This is for non-modifier non-plus symbol border/text combos */
	static void SetBrushAndText_Key(const FKeyInfo & KeyInfo, UBorder * Border, UTextBlock * TextBlock);

	/* e.g. if index = 2 hide Border_KeyElement3, Border_KeyElement4 ... Border_KeyElement7 */
	void HideKeyElementWidgetsFromIndex(int32 Index);

	/* Same as HideKeyElementWidgetsFromIndex but checks for null before dereferencing pointers */
	void HideKeyElementWidgetsFromIndexCheckIfNull(int32 Index);

	void UIBinding_OnRemapButtonLeftMousePress();
	void UIBinding_OnRemapButtonLeftMouseRelease();
	void UIBinding_OnRemapButtonRightMousePress();
	void UIBinding_OnRemapButtonRightMouseRelease();

public:

	void OnClicked();

protected:

	static bool IsWidgetBound(UWidget * Widget) { return Widget != nullptr; }

#if WITH_EDITOR
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent) override;

	void RunOnPostEditLogic();
#endif

protected:

	//----------------------------------------------------
	//	Data
	//----------------------------------------------------

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	bool bCanEditActionType;

	UPROPERTY()
	bool bCanEditAxisType;
#endif

	// Local player controller 
	ARTSPlayerController * PC;

	// The widget this widget belongs to
	UKeyBindingsWidget * OwningWidget;

	/* Pointer to static info struct */
	const FInputInfoBase * BindingInfo;

	/* The game action this widget is for. Leave as None to use the axis mapping instead */
	UPROPERTY(EditAnywhere, Category = "RTS", meta = (EditCondition = bCanEditActionType))
	EKeyMappingAction ActionType;

	/* Leave as None to use te action mapping instead */
	UPROPERTY(EditAnywhere, Category = "RTS", meta = (EditCondition = bCanEditAxisType))
	EKeyMappingAxis AxisType;

	/* The text that displays the action name e.g. toggle skill tree */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_Action;

	/* The button to click to bind the key to something else */
	UPROPERTY(meta = (BindWidgetOptional))
	UMyButton * Button_Remap;

	/* The text that that displays the key name e.g. left mouse button, shift + Q */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_Key;

	/* Number of Image_KeyElementX widgets that are bound */
	uint8 NumKeyElementWidgetsBound;

	/* 7 images for CTRL + SHIFT + ALT + Key. Did not take into account CMD.
	One for the CTRL, one for the plus symbol, etc.
	You should bind the lower numbered ones first */
	UPROPERTY(meta = (BindWidgetOptional))
	UBorder * Border_KeyElement1;
	UPROPERTY(meta = (BindWidgetOptional))
	UBorder * Border_KeyElement2;
	UPROPERTY(meta = (BindWidgetOptional))
	UBorder * Border_KeyElement3;
	UPROPERTY(meta = (BindWidgetOptional))
	UBorder * Border_KeyElement4;
	UPROPERTY(meta = (BindWidgetOptional))
	UBorder * Border_KeyElement5;
	UPROPERTY(meta = (BindWidgetOptional))
	UBorder * Border_KeyElement6;
	UPROPERTY(meta = (BindWidgetOptional))
	UBorder * Border_KeyElement7;

	// Index 0 = Border_KeyElement1, index 1 = Border_KeyElement2, etc. 
	UBorder * BorderKeyElements[7];

	/* The 7 texts to go in the borders. Bind widgets to these if you want to display the 
	key using a font instead of an image with the text already on it. The option for this 
	will be in GI. 
	It is assumed that these are children of the corrisponing Border_KeyElementX widget */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_KeyElement1;
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_KeyElement2;
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_KeyElement3;
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_KeyElement4;
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_KeyElement5;
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_KeyElement6;
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_KeyElement7;
};


/** 
 *	The widget the shows up to say "Press any key to rebind it". 
 */
UCLASS(Abstract)
class RTS_VER2_API UPressAnyKeyWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	// Overridden probably because we wanna turn tick off for performance
	virtual void NativeTick(const FGeometry & MyGeometry, float InDeltaTime) override;

	static bool IsWidgetBound(UWidget * Widget) { return Widget != nullptr; }

public:

	void SetupForAboutToBeShown(URTSGameInstance * GameInst);

protected:

	//----------------------------------------------------------
	//	Data
	//----------------------------------------------------------

	/* The message that is like "Press any key to rebind it. Hold <cancel key's name> to cancel" */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_Message;
};


/**
 *	The widget that shows up when the player tries to bind an action to a key that's already 
 *	bound to another action. 
 */
UCLASS(Abstract)
class RTS_VER2_API URebindingCollisionWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	void InitialSetup(URTSGameInstance * GameInst, ARTSPlayerController * InPlayerController, UKeyBindingsWidget * InKeyBindingsWidget);

protected:

	// Overridden probably because we wanna turn tick off for performance
	virtual void NativeTick(const FGeometry & MyGeometry, float InDeltaTime) override;

public:

	/* Setup the appearance of this widget for a result from trying to bind key */
	void SetupFor(const FTryBindActionResult & Result);

	void OnCancelButtonClicked(ARTSPlayerController * PlayCon);

protected:

	void UIBinding_OnConfirmButtonLeftMousePress();
	void UIBinding_OnConfirmButtonLeftMouseRelease();
	void UIBinding_OnConfirmButtonRightMousePress();
	void UIBinding_OnConfirmButtonRightMouseRelease();

	void UIBinding_OnCancelButtonLeftMousePress();
	void UIBinding_OnCancelButtonLeftMouseRelease();
	void UIBinding_OnCancelButtonRightMousePress();
	void UIBinding_OnCancelButtonRightMouseRelease();

	static bool IsWidgetBound(UWidget * Widget) { return Widget != nullptr; }

	//-------------------------------------------------------
	//	Data
	//-------------------------------------------------------

	ARTSPlayerController * PC;
	UKeyBindingsWidget * KeyBindingsWidget;

	/* The text that shows what action is already bound to the key */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_AlreadyBoundAction;

	/* Button to confirm you want to do the remapping */
	UPROPERTY(meta = (BindWidgetOptional))
	UMyButton * Button_Confirm;

	/* Button to cancel doing the remapping */
	UPROPERTY(meta = (BindWidgetOptional))
	UMyButton * Button_Cancel;
};


/**
 *	The menu that lets the player change their key bindings. 
 *	If I already have a widget for this in SettingsWidgets.h then ignore that one - this 
 *	one's probably better.
 */
UCLASS(Abstract)
class RTS_VER2_API UKeyBindingsWidget : public USettingsSubmenuBase
{
	GENERATED_BODY()

	static const int32 PRESS_ANY_KEY_WIDGET_Z_ORDER = 1000;
	static const int32 REBINDING_COLLISION_WIDGET_Z_ORDER = 2000;

public:
 
	void InitialSetup(URTSGameInstance * GameInst, ARTSPlayerController * InPlayerController, 
		bool bUpdateWidgetsToReflectAppliedValues);

	//~ Begin USettingsSubmenuBase interface
	virtual void UpdateAppearanceForCurrentValues() override;
	//~ End USettingsSubmenuBase interface

	bool IsShowingOrPlayingShowAnim() const;

	void OnRequestToBeToggled();

	void OnSingleBindingWidgetClicked(USingleKeyBindingWidget * ClickedWidget);

	/**
	 *	Called after the player inputs a key to try and rebind an action 
	 *	
	 *	@param bSuccess - if the binding was changed 
	 *	@return - true if the widget will show a screen asking for confirmation to rebind the key 
	 */
	bool OnKeyBindAttempt(URTSGameInstance * GameInst, ARTSPlayerController * PlayCon, bool bSuccess, const FTryBindActionResult & Result);

	void OnPendingKeyBindCancelled();

	void OnPendingKeyBindCancelledViaCancelKey();

	bool IsCollisionConfirmationWidgetShowingOrPlayingShowAnim() const;

	/* Get the key that is trying to be rebound. May return garbage if no binding change is pending */
	const FKeyWithModifiers & GetPendingKey() const;

protected:

	void OnOpened();

	void UIBinding_OnResetToDefaultsButtonLeftMousePress();
	void UIBinding_OnResetToDefaultsButtonLeftMouseRelease();
	void UIBinding_OnResetToDefaultsButtonRightMousePress();
	void UIBinding_OnResetToDefaultsButtonRightMouseRelease();

	int32 AssignWidgetsArrayIndex(USingleKeyBindingWidget * Widget) const;
	int32 GetBindingWidgetsArrayIndex(EKeyMappingAction ActionType) const;
	int32 GetBindingWidgetsArrayIndex(EKeyMappingAxis AxisType) const;

	//------------------------------------------------------
	//	Data
	//------------------------------------------------------

	URTSGameInstance * GI;

	/* Local player controller */
	ARTSPlayerController * PC;

	/* Array of all the single key binding widgets on this widget. 
	Index depends on the action/axis of the widget. 
	@See GetBindingWidgetsArrayIndex */
	TArray<USingleKeyBindingWidget *> BindingWidgets;

	UPressAnyKeyWidget * PressAnyKeyWidget;
	URebindingCollisionWidget * RebindingCollisionWidget;

	/* The widget that is waiting for the player to press a key so its action can be rebound. 
	Will be null if the player isn't trying to rebind a key */
	USingleKeyBindingWidget * PendingRebindInstigator;

	/* The key that we're trying to setup a binding for but need a confirmation from the player 
	to continue doing it */
	FKeyWithModifiers PendingKey;

	/** Resets all the bindings back to defaults */
	UPROPERTY(meta = (BindWidgetOptional))
	UMyButton * Button_ResetToDefaults;

	/** 
	 *	The widget that is shown when the player clicks a button to rebind an action. 
	 *	Cannot think of a better name 
	 */
	UPROPERTY(EditAnywhere, Category = "RTS")
	TSubclassOf<UPressAnyKeyWidget> PressAnyKeyWidget_BP;

	/* The widget that shows up when rebinding will cause a collision with another action */
	UPROPERTY(EditAnywhere, Category = "RTS")
	TSubclassOf<URebindingCollisionWidget> RebindingCollisionWidget_BP;

	/** 
	 *	If you bind a widget to this and set bAutoPopulatePanel to true then it will be automatically 
	 *	populated with each single setting widget. 
	 *	This was added to speed up development 
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	UPanelWidget * Panel_BindingWidgets;

	/** 
	 *	If true and you have a widget bound to Panel_BindingWidgets then it will be automatically 
	 *	populated with each action. Any USingleKeyBindingWidget you have already added to the widget 
	 *	will be destroyed.
	 *	This was added to speed up development 
	 */
	UPROPERTY(EditAnywhere, Category = "RTS")
	bool bAutoPopulatePanel;

	/* The widget to use if bAutoPopulatePanel is true */
	UPROPERTY(EditAnywhere, Category = "RTS", meta = (DisplayName = "Single Binding Widget", EditCondition = bAutoPopulatePanel))
	TSubclassOf<USingleKeyBindingWidget> SingleKeyBindingWidget_BP;
};
