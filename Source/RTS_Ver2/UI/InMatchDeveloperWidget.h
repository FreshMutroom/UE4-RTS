// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/InGameWidgetBase.h"
#include "InMatchDeveloperWidget.generated.h"

class UButton;
class UImage;
class UDevelopmentPopupWidget;
class UDevelopmentPopupWidgetButton;
struct FInventoryItemInfo;
class UGridSlot;
enum class EDevelopmentAction : uint8;
enum class EInventoryItem : uint8;


/**
 *	A widget that contains cheat actions to be used during a match that can help with speeding 
 *	up development.
 *
 *	This class is not abstract and has a 'works out of the box' implementation.
 *
 *	This widget is only used with editor so like I care about performance in it. 
 *
 *	Also oddly it does not show in PIE but does in standalone. Not sure if/when it changed 
 *	to be like that.
 */
UCLASS()
class RTS_VER2_API UInMatchDeveloperWidget : public UInGameWidgetBase
{
	GENERATED_BODY()

	// The Y position to set for the top most button
	static const float STARTING_Y_POSITION;
	
	// Amount of Y axis space between buttons 
	static const float Y_SPACING_BETWEEN_BUTTONS;

	// Size of each button
	static const FVector2D BUTTON_SIZE;

public:

	UInMatchDeveloperWidget(const FObjectInitializer & ObjectInitializer);

	virtual bool SetupWidget(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController) override;

	virtual bool IsEditorOnly() const override;

protected:
	
	virtual TSharedRef<SWidget> RebuildWidget() override;

	//=========================================================================================
	//	BindWidgetOptional Widgets
	//=========================================================================================

	/* Button to destroy a selectable with a mouse click. More specifically deals 9999999 default 
	damage */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton * Button_DestroySelectable;

	/* Button that deals some damage to a selectable. Deals 20 default damage */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton * Button_DamageSelectable;

	/** 
	 *	Button that tries to award some experience to a selectable. Awards 55% to 65% of the 
	 *	amount needed to reach the next level.  
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton * Button_AwardExperience;

	/** 
	 *	Button that tries to award enough experience to a selectable such that it will level up 
	 *	at least two ranks. 
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton * Button_AwardLotsOfExperience;

	/* Button that awards experience to the local player */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton * Button_AwardExperienceToPlayer;

	/** 
	 *	Button that tries to put a random item in a selectable's inventory 
	 *	
	 *	This button will likely only be visible for the server player and if items have been 
	 *	defined in EInventoryItem enum. 
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton * Button_GiveRandomInventoryItem;

	/** 
	 *	Button that tried to put a specific item in a selectable's inventory. 
	 *	
	 *	This button will likely only be visible for the server player and if items have been 
	 *	defined in EInventoryItem enum. 
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton * Button_GiveSpecificInventoryItem;

	/* Shows debug info for a unit */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton * Button_GetAIControllerInfo;

	//=========================================================================================
	//	Other stuff
	//=========================================================================================

	/* Widget to use when a popup menu is required */
	UPROPERTY(EditAnywhere, NoClear, Category = "RTS", meta = (DisplayName = "Popup Menu Widget"))
	TSubclassOf<UDevelopmentPopupWidget> PopupMenuWidget_BP;

	UPROPERTY()
	UDevelopmentPopupWidget * PopupMenuWidget;

	//=========================================================================================
	//	Button click functions
	//=========================================================================================

	UFUNCTION()
	void UIBinding_OnDestroySelectableButtonClicked();

	UFUNCTION()
	void UIBinding_OnDamageSelectableButtonClicked();

	UFUNCTION()
	void UIBinding_OnAwardExperienceButtonClicked();

	UFUNCTION()
	void UIBinding_OnAwardLotsOfExperienceButtonClicked();

	UFUNCTION()
	void UIBinding_OnAwardExperienceToPlayerButtonClicked();

	UFUNCTION()
	void UIBinding_OnGiveRandomItemButtonClicked();

	UFUNCTION()
	void UIBinding_OnGiveSpecificItemButtonClicked();
	
	UFUNCTION()
	void UIBinding_OnGetUnitAIInfoButtonClicked();
	
public:

	/** Return whether the popup menu is showing on screen */
	bool IsPopupMenuShowing() const;

	/** 
	 *	Show another widget that is ment for selecting specific stuff. If the popup widget is 
	 *	already showing then this will hide it.
	 *	
	 *	@param PopupWidgetsPurpose - why we are showing this popup widget so we can adjust its 
	 *	appearance. 
	 */
	void ShowPopupWidget(EDevelopmentAction PopupWidgetsPurpose);

	void HidePopupWidget();
};


/** 
 *	This is a popup widget used when the action requires being more specific e.g. the action 
 *	is to give an item to a selectable but we would like to specify exactly which one.
 *	
 *	Just like the main widget this class is also has a 'works out of the box' implementation. 
 */
UCLASS()
class RTS_VER2_API UDevelopmentPopupWidget : public UInGameWidgetBase
{
	GENERATED_BODY()

	// How many children per row
	static const int32 NUM_GRID_PANEL_CHILDREN_PER_ROW;

public:

	UDevelopmentPopupWidget(const FObjectInitializer & ObjectInitializer);

	virtual bool IsEditorOnly() const override;

protected:

	virtual TSharedRef < SWidget > RebuildWidget() override;

	/* Panel that holds all the options */
	UPROPERTY(meta = (BindWidgetOptional))
	UPanelWidget * Panel_Selections;

	/* Widget to use for each individual entry */
	UPROPERTY(EditAnywhere, NoClear, Category = "RTS", meta = (DisplayName = "Single Entry Widget"))
	TSubclassOf<UDevelopmentPopupWidgetButton> Button_BP;

	void OnWidgetAddedToSelectionsPanel(UGridSlot * GridSlot, UDevelopmentPopupWidgetButton * JustAdded);

public:

	void SetAppearanceFor(EDevelopmentAction Action);
};


/**
 *	A single button in the popup widget.
 *	
 *	Has default 'works out of the box' implementation. 
 */
UCLASS()
class RTS_VER2_API UDevelopmentPopupWidgetButton : public UInGameWidgetBase
{
	GENERATED_BODY()

	// What button clicks do 
	enum class EButtonClickBehavior : uint8
	{
		Nothing, 
		GiveInventoryItem, 
	};

public:

	UDevelopmentPopupWidgetButton(const FObjectInitializer & ObjectInitializer);

	virtual bool SetupWidget(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController) override;

	virtual bool IsEditorOnly() const override;

protected:

	virtual TSharedRef < SWidget > RebuildWidget() override;

	// Button for click functionality
	UPROPERTY(meta = (BindWidgetOptional))
	UButton * Button_Button;

	// Image to show something
	UPROPERTY(meta = (BindWidgetOptional))
	UImage * Image_Image;

	EButtonClickBehavior ClickFunctionality;
	uint8 ClickAuxilleryInfo;

	UFUNCTION()
	void UIBinding_OnButtonClicked();

public:

	/** 
	 *	Version for inventory items. Set what the appearance and click functionality of this 
	 *	widget should be. 
	 */
	void SetupFor(EInventoryItem ItemType, const FInventoryItemInfo & ItemsInfo);
};
