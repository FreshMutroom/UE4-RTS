// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/MainMenu/MainMenuWidgetBase.h"
#include "MapSelection.generated.h"

class UButton;
class UImage;
class UTextBlock;
class UMultiLineEditableTextBox;
class UMapSelectionWidget;
struct FMapInfo;


/**
 *	A widget that is placed on the map image. It represents a player start location. It can be 
 *	clicked on by players to secure where they will start in match
 *
 *	Notes: 
 *	- Might help if anchored to center of screen (legit don't know though)
 *
 *	FIXME:
 *	- Player starts do not draw in the correct location on minimap 
 *	@See SetMap SetMinimapLocationValues 
 */
UCLASS(Abstract)
class RTS_VER2_API UPlayerStartWidget : public UMainMenuWidgetBase
{
	GENERATED_BODY()

public:

	UPlayerStartWidget(const FObjectInitializer & ObjectInitializer);
	
	// Make sure the owning widget explicitly calls this
	virtual bool Setup() override;

	/**
	 *	Sets up widget for a player start location
	 *	@param InPlayerStartUniqueID - ID of player start to assign to this widget 
	 *	@param ScreenLocation - location on screen this widget should be positioned at 
	 *	@param bIsClickable - if true then button click functionality is activated
	 */
	void SetupForPlayerSpot(uint8 InPlayerStartUniqueID, FVector2D ScreenLocation, bool bIsClickable);

protected:

	// Unique ID of the player start assigned to this widget
	uint8 PlayerStartUniqueID;

	/* Slot in lobby that is assigned to this spot. -1 = no one assigned */
	int16 AssignedLobbySlot;

	/**
	 *	Text that shows the index of the player assigned to this spot. 
	 *	1 = player in first lobby slot, 2 = player in second lobby slot, etc
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_PlayerIndex;

	/**
	 *	Button player can click to try and assign themselves to this player start
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton * Button_Button;

	UFUNCTION()
	void UIBinding_OnButtonClicked();

	/* Returns true if the local player is host */
	bool IsHost() const;

public:

	/* Set the lobby slot that should be assigned to this spot. -1 means assign to no one */
	void SetAssignedPlayer(int16 PlayerIndex);
	
	/* Get the lobby slot that should be assigned to this spot. Returns -1 if no player assigned */
	int16 GetAssignedPlayer() const;
};


/** 
 *	A widget that shows information about a single map. 
 */
UCLASS(Abstract)
class RTS_VER2_API UMapInfoWidget : public UMainMenuWidgetBase
{
	GENERATED_BODY()

public:

	UMapInfoWidget(const FObjectInitializer & ObjectInitializer);

	virtual bool Setup() override;

	void SetRefToOwningWidget(UMapSelectionWidget * ParentWidget);

	/* Set image, description etc given some map info 
	@param InMapInfo - map info of  the map to show 
	@param PlayerStartRule - whether to show the player start widgets on the map */
	virtual void SetMap(const FMapInfo * InMapInfo, EPlayerStartDisplay PlayerStartRule);

	/* Get the display name of the map this widget holds info for */
	const FText & GetSetMap() const;

	/* Get the ID of the map this widget holds info for */
	uint8 GetSetMapID() const;

	/* Get map info this widget holds info for or null if none */
	const FMapInfo * GetSetMapInfo() const;

protected:

	/* Reference to map selection widget this widget belongs to if any. Will be set by owning
	map selection widget */
	UPROPERTY()
	UMapSelectionWidget * MapSelectionWidget;

	/* The map this slot is for */
	const FMapInfo * MapInfo;

	/* Widget to display at a player start location on the map */
	UPROPERTY(EditAnywhere, Category = "RTS", meta = (DisplayName = "Player Start Widget"))
	TSubclassOf < UPlayerStartWidget > PlayerStartWidget_BP;

	/* If true then this widget is displaying player start widgets overtop the map image */
	bool bIsDisplayingPlayerStarts;

	/* Array of widgets used to represent a player start point. 
	Key = player start unique ID */
	UPROPERTY()
	TArray < UPlayerStartWidget * > PlayerStartWidgets;

	/* The screen location of Image_Minimap updated on post edit */
	FVector2D MinimapImageScreenLoc;

	/* The size of Image_Minimap updated on post edit */
	FVector2D MinimapImageScreenDim;

	void SetMinimapLocationValues();

	//==========================================================================================
	//	BindWidgetOptional Widgets
	//==========================================================================================

	/** 
	 *	The button that selects this map. If this widget is placed on the lobby widget then this 
	 *	button does not need to be there 
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton * Button_Select;

	/** Text to show map name */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_Name;

	/* Image to show an image set by user */
	UPROPERTY(meta = (BindWidgetOptional))
	UImage * Image_Custom;

	/* Image to show minimap. */
	UPROPERTY(meta = (BindWidgetOptional))
	UImage * Image_Minimap;

	/* Text to show map description */
	UPROPERTY(meta = (BindWidgetOptional))
	UMultiLineEditableTextBox * Text_Description;

	/* Text to show max number of players allowed on map */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_MaxPlayers;

	virtual void SetupBindings();

	//==========================================================================================
	//	UI Bindings
	//==========================================================================================

	/* Should set the selected map to this map */
	UFUNCTION()
	void UIBinding_OnSelectButtonClicked();

public:

	/* Sets each player start widget to the corrisponding player */
	void UpdatePlayerStartAssignments(const TArray < int16 > & PlayerStartArray);

	/* Get array of all starting spots. For array key = Starting spot unique ID, entries = 
	lobby slot index of player assigned to spot so 0 would be host usually. -1 means no 
	player assigned to spot */
	void GetStartingSpots(TArray < int16 > & OutStartingSpots);

#if WITH_EDITOR
	// Appears none of these functions are called when editing Image_Minimap
	//virtual void PostEditChangeProperty(struct FPropertyChangedEvent & PropertyChangedEvent) override;
	//virtual void OnDesignerChanged(const FDesignerChangedEventArgs & EventArgs) override;
	//virtual void SynchronizeProperties() override;
	//virtual void NativePreConstruct() override;
	
	// MAYBE this one is good enough but no way 100% sure
	virtual void OnWidgetRebuilt() override;
#endif
};


/**
 *	The widget that appears when choosing which map to play on, either from lobby creation
 *	screen or lobby. It will generally contain a list of all the available maps.
 *
 *	When you add a new map to your map pool in game instance you should add an extra slot
 *	widget to this in editor. The slot can be named whatever you want
 */
UCLASS(Abstract)
class RTS_VER2_API UMapSelectionWidget : public UMainMenuWidgetBase
{
	GENERATED_BODY()

public:

	UMapSelectionWidget(const FObjectInitializer & ObjectInitializer);
	
	virtual bool Setup() override;

	void SetupMapList();

protected:

	virtual void SetupBindings();

	/* Map info for selected map */
	const FMapInfo * CurrentMapInfo;

	/* True if child map info widgets have been created and/or setup. Similar to 
	UMainMenuWidgetBase::bHasBeenSetup */
	bool bHasSetupMapInfoList;

	/** 
	 *	If true this widget widget will automatically create map info widgets at runtime and 
	 *	add them to the widget Panel_MapList. Settings this to true basically means whenever 
	 *	you create a new map you do not need to modify this widget. 
	 *
	 *	Widgets are added using UPanelWidget::AddChild and UMapSelectionWidget::OnMapInfoAddedToPanel 
	 *	will be called when each widget is added
	 */
	UPROPERTY(EditAnywhere, Category = "RTS")
	bool bAutoPopulateMapList;

	/** 
	 *	Only relevant if bAutoPopulateMapList is true. The widget to use to show a map's info
	 */
	UPROPERTY(EditAnywhere, Category = "RTS", meta = (EditCondition = bAutoPopulateMapList, DisplayName = "Map Info Widget"))
	TSubclassOf < UMapInfoWidget > MapInfoWidget_BP;

	/* Function called when auto-generated map info widgets are created and added to panel 
	widget Panel_MapList */
	virtual void OnMapInfoAddedToPanel(UMapInfoWidget * WidgetJustAdded);

	/* Reference to widget this widget should update with map info */
	UPROPERTY()
	UMainMenuWidgetBase * UpdatedWidget;

	/** 
	 *	Whether to show player starts or not - both on the current selected map and the maps 
	 *	in the list. Depending what widget this widget is a part of they may or may not be 
	 *	clickable.
	 */
	UPROPERTY(EditAnywhere, Category = "RTS")
	bool bShowPlayerStartsOnMinimaps;

	/** 
	 *	This is only relevant if bAutoPopulateMapList is true. The panel widget to add map info 
	 *	widgets to automatically at runtime
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	UPanelWidget * Panel_MapList;

	/**
	 *	Widget for displaying info about the current selected map. When new map is selected
	 *	from list this will be updated. This should be a different widget BP than the ones used
	 *	in the list and should have no need for Button_Select being assigned 
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	UMapInfoWidget * Widget_CurrentSelectedMap;

	/** Button to return to previous menu */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton * Button_Return;

	//======================================================================================
	//	UI Bindings
	//======================================================================================

	UFUNCTION()
	void UIBinding_OnReturnButtonClicked();

public:

	/* Set the map to use for match */
	void SetCurrentMap(const FMapInfo * MapInfo);

	/* Set the widget this will update when the return button is pressed */
	void SetRefToUpdatedWidget(UMainMenuWidgetBase * OwningWidget);
};
