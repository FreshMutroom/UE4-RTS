// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/InGameWidgetBase.h"

#include "Statics/Structs_1.h" // All for FTrainingInfo
#include "Statics/Structs_3.h"
#include "UI/InMatchWidgets/Components/MyButton.h"
#include "RTSHUD.generated.h"

class UMultiLineEditableTextBox;
class UMultiLineEditableText;
class UHUDResourcesWidget;
class UHUDChatInputWidget;
class UTextBlock;
class UWidgetSwitcher;
class USelectableContextMenu;
class UProgressBar;
class ISelectable;
struct FContextMenuButtons;
class URTSGameInstance;
struct FContextButton;
class UEditableText;
class USelectableInfo;
class USelectableActionBar;
class UImage;
class UContextActionButton;
class UHUDPersistentTab;
class UHUDSingleResourcesWidget;
class ARTSPlayerController;
struct FSelectableAttributesBasic;
class UHUDPersistentTabButton;
class UHUDPersistentPanel;
class AFactionInfo;
struct FProductionQueue;
struct FTrainingInfo;
struct FDisplayInfoBase;
class UProductionQueueButton;
class UMinimap;
class UHUDChatOutputWidget;
class UEditableTextBox;
class ARTSLevelVolume;
class UHUDGameOutputWidget;
class ARTSPlayerState;
class USingleBuffOrDebuffWidget;
class USingleTickableBuffOrDebuffWidget;
class UHUDSingleHousingResourceWidget;
class UInventoryItemButton;
class UItemOnDisplayInShopButton;
class UMyButton;
class UCanvasPanelSlot;
class UTooltipWidget_InventoryItem;
class UTooltipWidget_Production;
class UTooltipWidget_Ability;
struct FRankInfo;
class UCommanderSkillTreeNodeWidget;
class UTooltipWidget_CommanderSkillTreeNode;
class UCommanderSkillTreeWidget;
class UGlobalSkillsPanel;
class UTooltipWidget_GlobalSkillsPanelButton;
class UPlayerTargetingPanel;
class UPauseMenu;
class UMenuOutputWidget;
struct FBuildingGarrisonAttributes;


/* Rule that says when the commander skill tree widget "open me" anim should play */
UENUM()
enum class ECommanderSkillTreeAnimationPlayRule : uint8
{
	/* Never play the anim. An alternative is to just never name the anim correctly in editor */
	Never,
	/* This Never value also has its use in code though. We can pass it in as a param when
	we don't care whether the anim should play likely because it is already playing */

	/**
	 *	The anim will play:
	 *	- at the start of the match if the player can afford at least one ability on the skill tree
	 *	- if you level up and can afford at least one ability on the skill tree
	 */
	CanAffordAbilityAtStartOfMatchOrLevelUp,
};


namespace HUDStatics
{
	/* Visibility that is set on widgets when we want to hide them. Can't decide between 
	Collapsed and Hidden hence this variable. 
	
	Note: Should update existing code to use this variable */
	constexpr ESlateVisibility HIDDEN_VISIBILITY = ESlateVisibility::Hidden;

	/* Visibility for tooltip widgets */
	constexpr ESlateVisibility TOOLTIP_VISIBILITY = ESlateVisibility::HitTestInvisible;

	//---------------------------------------------------------
	//	Z Orderings
	//---------------------------------------------------------

	/* Z order of the commander skill tree widget on the HUD use widget */
	constexpr int32 COMMANDER_SKILL_TREE_Z_ORDER = 100;

	/* Z order of the player targeting panel */
	constexpr int32 PLAYER_TARGETING_PANEL_Z_ORDER = 150;

	/* Z order of tooltip widget on the HUD user widget */
	constexpr int32 TOOLTIP_Z_ORDER = 200;

	constexpr int32 GAME_OUTPUT_Z_ORDER = 300;

	constexpr int32 PAUSE_MENU_Z_ORDER = 400;

	constexpr int32 MENU_OUTPUT_Z_ORDER = 500;
}


/**
 *	In-game HUD. Each faction can use its own HUD.
 *
 *	Some things:
 *	- if you want a UI element to not allow mouse pressed to go through it then make sure 
 *	to create bindings for OnMouseButtonDown and OnMouseButtonUp and return FReplay::Handled.
 *	Currently you have to do this in the editor which means it will be a blueprint binding. 
 *	/Begin verbose comment
 *	For performance I would like to move this to C++ in the future. If all UI elements 
 *	should block by default (which I think is what all RTSs do anyway) then I could make 
 *	sure during setup to iterate every widget of the HUD and add a NOOP binding to both 
 *	OnMouseButtonDown and OnMouseButtonUp. If users want to pick and choose which should 
 *	and shouldn't block then the only way I think I can do that is by creating my own 
 *	SWidgets/UWidgets for each widget type... which would be a lot of effort.
 */
UCLASS(Abstract)
class RTS_VER2_API URTSHUD : public UInGameWidgetBase
{
	GENERATED_BODY()

public:

	/* Category to appear in the palette in user widget blueprints */
	static const FText PALETTE_CATEGORY;

	/* Text that has nothing in it */
	static const FText BLANK_TEXT;

	//=====================================================================================
	//	Setup functions
	//=====================================================================================

	URTSHUD(const FObjectInitializer & ObjectInitializer);

	virtual bool SetupWidget(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController) override;

	/* Setup the minimap
	@param FogVolume - fog volume for current map */
	void SetupMinimap(ARTSLevelVolume * FogVolume);

protected:

	// TODO instead of UUserWidget some that are never reused could just be UPanelWidget

	virtual void NativeTick(const FGeometry & MyGeometry, float InDeltaTime) override;

	void SetupBindings();

	/* Call SetupWidget on all the BindWidget widgets */
	void SetupBoundWidgets();

	/* Spawns the widget we use to make mouse hover/pressed effects */
	void SpawnMouseHoverAndPressedWidget();

	void SetupCommanderSkillTreeWidget();

	void SetupPlayerTargetingWidget();

public:

	void PerformPlayerTargetingPanelFinalSetup();

protected:

	void SetupPauseMenu();

	void SetupMenuOutputWidget();

	void SpawnTooltipWidgets();

	//==========================================================================================
	//	BindWidgetOptional Widgets
	//==========================================================================================

	/* Widget that displays resources */
	UPROPERTY(meta = (BindWidgetOptional))
	UHUDResourcesWidget * Widget_Resources;

	/* Widget for the minimap */
	UPROPERTY(meta = (BindWidgetOptional))
	UMinimap * Widget_Minimap;

	/* Widget that appears when entering chat message */
	UPROPERTY(meta = (BindWidgetOptional))
	UHUDChatInputWidget * Widget_ChatInput;

	/* Text box that holds chat messages received */
	UPROPERTY(meta = (BindWidgetOptional))
	UHUDChatOutputWidget * Widget_ChatOutput;

	/* Widget that will show game messages like "not enough resources" */
	UPROPERTY(meta = (BindWidgetOptional))
	UHUDGameOutputWidget * Widget_GameOutput;

	/* Button to bring up the pause menu */
	UPROPERTY(meta = (BindWidgetOptional))
	UMyButton * Button_PauseMenu;

	//-------------------------------------------------------------------------------------
	//	------- Commander rank variables -------
	//-------------------------------------------------------------------------------------

	/** Button to toggle the commander's skill tree */
	UPROPERTY(meta = (BindWidgetOptional))
	UMyButton * Button_CommanderSkillTree;

	/* Displays the commander's rank */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_CommandersRank;

	/* Displays the commander's unspent skill points */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_CommandersUnspentSkillPoints;

	/** 
	 *	Displays the commander's progress towards their next rank. This is not total experience 
	 *	gained but how much as been gained that goes towards the next rank. 
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_CommandersExperienceProgress;

	/**
	 *	Displays how much experience is required to reach the next rank. This is not cumulative 
	 *	and does not change as experience is gained 
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_CommandersExperienceRequired;

	/** Displays commander's progress towards next rank */
	UPROPERTY(meta = (BindWidgetOptional))
	UProgressBar * ProgressBar_CommandersExperienceProgress;

	//------------------------ End commander rank stuff -----------------------------

	//-------------------------------------------------------------------------------------
	//	------- Commander skill tree variables -------
	//-------------------------------------------------------------------------------------

	/* Can be null if the local player's faction does not use a skill tree */
	UCommanderSkillTreeWidget * CommanderSkillTreeWidget;

	//------------------------ End commander skill tree stuff -----------------------------

	//-------------------------------------------------------------------------------------
	//	------- Player targeting panel variables -------
	//-------------------------------------------------------------------------------------

	/** Widget that appears when an ability requires a player as a target */
	UPROPERTY(EditAnywhere, Category = "RTS", meta = (DisplayName = "Player Targeting Widget"))
	TSubclassOf<UPlayerTargetingPanel> PlayerTargetingPanel_BP;

	UPlayerTargetingPanel * PlayerTargetingPanel;

	//------------------------ End player targeting panel variables -----------------------------

	/* Current selected context menu */
	UPROPERTY(meta = (BindWidgetOptional))
	USelectableContextMenu * Widget_ContextMenu;

	/* Displays info about units that are inside a garrison for primary selected */
	UPROPERTY(meta = (BindWidgetOptional))
	UGarrisonInfo * Widget_SelectedGarrisonInfo;

	/* Persistent panel like in C&C where you can build stuff from */
	UPROPERTY(meta = (BindWidgetOptional))
	UHUDPersistentPanel * Widget_PersistentPanel;

	/* Panel that usually has commander abilities on it such as fuel air bomb */
	UPROPERTY(meta = (BindWidgetOptional))
	UGlobalSkillsPanel * Widget_GlobalSkillsPanel;

	/* Widget we use to create mouse hover/pressed highlights */
	UPROPERTY()
	UImage * MouseFocusImage;

	/* The slot that MouseFocusImage belongs to */
	UPROPERTY()
	UCanvasPanelSlot * MouseFocusImageSlot;

	/* Widgets that will close when the player presses the ESC key. The last one to be opened 
	is at .Last() in the array */
	TArray<UInGameWidgetBase *> EscapeRequestResponableWidgets;

	UPauseMenu * PauseMenu;

	UMenuOutputWidget * MenuOutputWidget;

	/* Widget that displays messages from the menu e.g. "cannot remap that */
	UPROPERTY(EditAnywhere, Category = "RTS", meta = (DisplayName = "Menu Output Widget"))
	TSubclassOf<UMenuOutputWidget> MenuOutputWidget_BP;

	//======================================================================================
	//	Tooltip Widgets Data
	//======================================================================================

	/* Tooltip widget for inventory items. Leave null to not use an inventory tooltip widget */
	UPROPERTY()
	UTooltipWidget_InventoryItem * TooltipWidget_InventoryItem;

	UPROPERTY()
	UTooltipWidget_Ability * TooltipWidget_Abilities;

	/* Tool tip widget for buildings */
	UPROPERTY()
	UTooltipWidget_Production *	TooltipWidget_BuildBuilding;

	/* Tooltip widget for units */
	UPROPERTY()
	UTooltipWidget_Production *	TooltipWidget_TrainUnit;

	/* Tooltip widget for upgrades */
	UPROPERTY()
	UTooltipWidget_Production *	TooltipWidget_ResearchUpgrade;

	/* Tooltip widget for buttons that appear on the global skills panel */
	UPROPERTY()
	UTooltipWidget_GlobalSkillsPanelButton * TooltipWidget_GlobalSkillsPanelButton;

	/* Tooltip widget for commander skill tree nodes */
	UPROPERTY()
	UTooltipWidget_CommanderSkillTreeNode * TooltipWidget_CommanderSkillTreeNode;

	/* Widget to use as a tooltip for inventory items */
	UPROPERTY(EditAnywhere, Category = "RTS", meta = (DisplayName = "Inventory Item Tooltip Widget"))
	TSubclassOf <UTooltipWidget_InventoryItem> TooltipWidget_InventoryItem_BP;

	/* Tooltip widget for abilities */
	UPROPERTY(EditAnywhere, Category = "RTS", meta = (DisplayName = "Abilities Tooltip Widget"))
	TSubclassOf <UTooltipWidget_Ability> TooltipWidget_Abilities_BP;

	/* Tooltip widget for buildings. This will be used when hovering over a button that builds 
	a building, either in a selectable's context menu or the HUD persistent panel */
	UPROPERTY(EditAnywhere, Category = "RTS", meta = (DisplayName = "Building Tooltip Widget"))
	TSubclassOf <UTooltipWidget_Production> TooltipWidget_BuildBuilding_BP;

	/* Tooltip widget for units */
	UPROPERTY(EditAnywhere, Category = "RTS", meta = (DisplayName = "Unit Tooltip Widget"))
	TSubclassOf <UTooltipWidget_Production> TooltipWidget_TrainUnit_BP;

	/* Tooltip widget for upgrades */
	UPROPERTY(EditAnywhere, Category = "RTS", meta = (DisplayName = "Upgrade Tooltip Widget"))
	TSubclassOf <UTooltipWidget_Production> TooltipWidget_ResearchUpgrade_BP;

	/* Tooltip for the abilities that appear on the global skills panel */
	UPROPERTY(EditAnywhere, Category = "RTS", meta = (DisplayName = "Global Skills Panel Tooltip Widget"))
	TSubclassOf <UTooltipWidget_GlobalSkillsPanelButton> TooltipWidget_GlobalSkillPanelButton_BP;

	/* Tooltip widget for commander skill tree nodes */
	UPROPERTY(EditAnywhere, Category = "RTS", meta = (DisplayName = "Commander Skill Tree Node Tooltip Widget"))
	TSubclassOf <UTooltipWidget_CommanderSkillTreeNode> TooltipWidget_CommanderSkillTreeNode_BP;

	/* Inventory item tooltip widget anchor. */
	UPROPERTY(EditAnywhere, Category = "RTS")
	FAnchors InventoryItemTooltipWidgetAnchor;

	/* Anchor for tooltips for selectable's action bar and the persistent panel */
	UPROPERTY(EditAnywhere, Category = "RTS")
	FAnchors ActionBarAndPersistentPanelTooltipAnchor;

	/* Anchor for tooltips for abilities that appear on the global skills panel */
	UPROPERTY(EditAnywhere, Category = "RTS")
	FAnchors GlobalSkillPanelButtonTooltipAnchor;

	/* Anchor for tooltips for commander's skill tree nodes */
	UPROPERTY(EditAnywhere, Category = "RTS")
	FAnchors CommandersSkillTreeNodeTooltipAnchor;

	// UWorld::GetRealTimeSeconds() when the pause menu was shown
	float TimeWhenPauseMenuWasShown;

	static FText GetPlayerExperienceAquiredAsText(float ExperienceAquired);
	static FText GetPlayerExperienceRequiredAsText(float ExperienceRequired);

	//==========================================================================================
	//	------- Widget Animations -------
	//==========================================================================================

	//------------------------ "Open me" anim for commander skill tree -------------------------

#if WITH_EDITORONLY_DATA
	/** 
	 *	The "you have commander skill points to spend" animation. If you name a widget anim this
	 *	then it will play in accordance with the CommanderSkillTreeAnimPlayRule.
	 *	I added this because in C&C generals when you rank up the button to open the skill tree
	 *	flashes stars
	 *
	 *	The animation will loop. You can change this easiy by changing the two 
	 *	PlayAnimation(SkillTreeWantsOpeningAnim) calls 
	 */
	UPROPERTY(VisibleAnywhere, Category = "RTS", meta = (DisplayName = "SkillTreeWantsOpeningAnim"))
	FString SkillTreeWantsOpeningAnim_FString;
#endif

	static const FName SkillTreeWantsOpeningAnimName;

	UWidgetAnimation * SkillTreeWantsOpeningAnim;

	/** When to play the "open me" anim for the commander skill tree */
	UPROPERTY(EditAnywhere, Category = "RTS")
	ECommanderSkillTreeAnimationPlayRule CommanderSkillTreeAnimPlayRule;

	//------------------------------------------------------------------------------------------

	//==========================================================================================
	//	UI Bindings
	//==========================================================================================

	void UIBinding_OnPauseMenuButtonLeftMousePress();
	void UIBinding_OnPauseMenuButtonLeftMouseReleased();
	void UIBinding_OnPauseMenuButtonRightMousePress();
	void UIBinding_OnPauseMenuButtonRightMouseReleased();

	void UIBinding_OnCommanderSkillTreeButtonLeftMousePress();
	void UIBinding_OnCommanderSkillTreeButtonLeftMouseReleased();
	void UIBinding_OnCommanderSkillTreeButtonRightMousePress();
	void UIBinding_OnCommanderSkillTreeButtonRightMouseReleased();

public:

	void TellPlayerTargetingPanelAboutButtonAction_LMBPressed(UMyButton * Button, ARTSPlayerController * LocalPlayCon);
	void TellPlayerTargetingPanelAboutButtonAction_LMBReleased(UMyButton * Button, ARTSPlayerController * LocalPlayCon);
	void TellPlayerTargetingPanelAboutButtonAction_RMBPressed(UMyButton * Button, ARTSPlayerController * LocalPlayCon);
	void TellPlayerTargetingPanelAboutButtonAction_RMBReleased(UMyButton * Button, ARTSPlayerController * LocalPlayCon);

	//==========================================================================================
	//	More Functions
	//==========================================================================================

public:

	// Called when the player changes their CurrentSelected 
	void OnPlayerCurrentSelectedChanged(const TScriptInterface<ISelectable> NewCurrentSelected);

	// Called when the player has no selection
	void OnPlayerNoSelection();

	/* Will show the commander skill tree if it is not already showing or in the process of 
	being shown */
	void TryShowCommanderSkillTree();
	
	/* Will hide the commander skill tree if it is not already hidden or in the process of
	being hidden */
	void TryHideCommanderSkillTree();

	void OnToggleCommanderSkillTreeButtonClicked();

	void OnToggleCommanderSkillTreeVisibilityKeyPressed();

	void ShowPlayerTargetingPanel(const FCommanderAbilityInfo * AbilityInfo);
	void HidePlayerTargetingPanel();

	/* Called when the player gains experience but does not level up as a result of it */
	void OnCommanderExperienceGained(float ExperienceTowardsNextRank, float ExperienceRequiredForNextRank);

	/**
	 *	Called when the player ranks up but only for the last level gained if multplie levels 
	 *	were gained at a time 
	 */
	void OnCommanderRankGained_LastForEvent(uint8 OldRank, uint8 NewRank, int32 UnspentSkillPoints, 
		float ExperienceTowardsNextRank, float ExperienceRequiredForNextRank);

	/** 
	 *	Called when the player aquires a commander skill, either a new one entirely or a new rank 
	 *	of one. 
	 *	
	 *	@param NodeWidget - the node that was aquired 
	 *	@param NumUnspentSkillPoints - number of unspent skill points the player has 
	 */
	void OnCommanderSkillAquired(FAquiredCommanderAbilityState * AquiredAbilityState, 
		const FCommanderAbilityTreeNodeInfo * NodeInfo, int32 NumUnspentSkillPoints);

	/** 
	 *	Called when a commander ability is used 
	 *	
	 *	@param UsedAbility - ability info for the ability that was used 
	 *	@param GlobalSkillsPanelIndex - index in the global skills panel's AllButtons array that 
	 *	the ability is. 
	 *	@param bWasLastCharge - whether that was the last time the ability can be used if it is 
	 *	a limited use ability
	 */
	void OnCommanderAbilityUsed(const FCommanderAbilityInfo & UsedAbility, int32 GlobalSkillsPanelIndex, bool bWasLastCharge);

	/** 
	 *	Called when a commander ability comes off cooldown. This could be either the initial 
	 *	cooldown or the regular cooldown. 
	 *	
	 *	@param AbilityInfo - info struct for the ability that has come off cooldown 
	 */
	void OnCommanderAbilityCooledDown(const FCommanderAbilityInfo & AbilityInfo, int32 GlobalSkillsPanelIndex);

	// Functions for when stats of a selected selectable changes
	void Selected_OnHealthChanged(ISelectable * InSelectable, float InNewAmount, float MaxHealth, bool bIsCurrentSelected);
	void Selected_OnMaxHealthChanged(ISelectable * InSelectable, float NewMaxHealth, float CurrentHealth, bool bIsCurrentSelected);
	void Selected_OnCurrentAndMaxHealthChanged(ISelectable * InSelectable, float NewMaxHealth, float CurrentHealth, bool bIsCurrentSelected);
	void Selected_OnSelectableResourceCurrentAmountChanged(ISelectable * InSelectable, float CurrentAmount, int32 MaxAmount, bool bIsCurrentSelected);
	void Selected_OnSelectableResourceMaxAmountChanged(ISelectable * InSelectable, float CurrentAmount, int32 MaxAmount, bool bIsCurrentSelected);
	void Selected_OnSelectableResourceCurrentAndMaxAmountsChanged(ISelectable * InSelectable, float CurrentAmount, int32 MaxAmount, bool bIsCurrentSelected);
	void Selected_OnSelectableResourceRegenRateChanged(ISelectable * InSelectable, float PerSecondRegenRate, bool bIsCurrentSelected);
	void Selected_OnRankChanged(ISelectable * InSelectable, float ExperienceTowardsNextRank, uint8 InNewRank, float ExperienceRequired, bool bIsCurrentSelected);
	void Selected_OnCurrentRankExperienceChanged(ISelectable * InSelectable, float InNewAmount, uint8 CurrentRank, float ExperienceRequired, bool bIsCurrentSelected);
	void Selected_OnImpactDamageChanged(ISelectable * InSelectable, float InNewAmount, bool bIsCurrentSelected);
	void Selected_OnAttackRateChanged(ISelectable * InSelectable, float AttackRate, bool bIsCurrentSelected);
	void Selected_OnAttackRangeChanged(ISelectable * InSelectable, float AttackRange, bool bIsCurrentSelected);
	void Selected_OnDefenseMultiplierChanged(ISelectable * InSelectable, float NewDefenseMultiplier, bool bIsCurrentSelected);
	void Selected_OnItemAddedToInventory(ISelectable * InSelectable, int32 DisplayIndex, const FInventorySlotState & UpdatedSlot, bool bIsCurrentSelected);
	void Selected_OnItemsRemovedFromInventory(ISelectable * InSelectable, const FInventory * Inventory, const TMap <uint8, FInventoryItemQuantityPair> & RemovedItems, bool bIsCurrentSelected);
	void Selected_OnInventoryPositionsSwapped(ISelectable * InSelectable, uint8 DisplayIndex1, uint8 DisplayIndex2, bool bIsCurrentSelected);
	// @param UseAbilityTotalCooldown - pass in 0 to say 'has no cooldown'
	void Selected_OnInventoryItemUsed(ISelectable * InSelectable, uint8 DisplayIndex, float UseAbilityTotalCooldown, bool bIsCurrentSelected);
	void Selected_OnInventoryItemUseCooldownFinished(ISelectable * InSelectable, uint8 InventorySlotDisplayIndex, bool bIsCurrentSelected);
	void Selected_OnItemPurchasedFromShop(ISelectable * InSelectable, uint8 ShopSlotIndex, bool bIsCurrentSelected);
	void Selected_OnInventoryItemSold(ISelectable * InSelectable, uint8 InvDisplayIndex, bool bIsCurrentSelected);

	/** 
	 *	Called when a buff/debuff is applied 
	 */
	void Selected_OnBuffApplied(ISelectable * InSelectable, const FTickableBuffOrDebuffInstanceInfo & BuffInfo,
		bool bIsCurrentSelected);
	void Selected_OnBuffApplied(ISelectable * InSelectable, const FStaticBuffOrDebuffInstanceInfo & DebuffInfo,
		bool bIsCurrentSelected);
	void Selected_OnDebuffApplied(ISelectable * InSelectable, const FTickableBuffOrDebuffInstanceInfo & DebuffInfo,
		bool bIsCurrentSelected);
	void Selected_OnDebuffApplied(ISelectable * InSelectable, const FStaticBuffOrDebuffInstanceInfo & DebuffInfo,
		bool bIsCurrentSelected);

	/** 
	 *	Called when a tickable buff is removed for whatever reason 
	 *	
	 *	@param ArrayIndex - index buff was in the selectable's tickable buff array 
	 */
	void Selected_OnTickableBuffRemoved(ISelectable * InSelectable, int32 ArrayIndex,
		EBuffAndDebuffRemovalReason RemovalReason, bool bIsCurrentSelected);
	/* Debuff version */
	void Selected_OnTickableDebuffRemoved(ISelectable * InSelectable, int32 ArrayIndex,
		EBuffAndDebuffRemovalReason RemovalReason, bool bIsCurrentSelected);
	void Selected_OnStaticBuffRemoved(ISelectable * InSelectable, int32 ArrayIndex,
		EBuffAndDebuffRemovalReason RemovalReason, bool bIsCurrentSelected);
	void Selected_OnStaticDebuffRemoved(ISelectable * InSelectable, int32 ArrayIndex,
		EBuffAndDebuffRemovalReason RemovalReason, bool bIsCurrentSelected);

	/** 
	 *	Called to update the duration left of a buff.
	 *	
	 *	@param ArrayIndex - index of buff in the selectable's tickable buff array 
	 *	@param DurationRemaining - how much is left for buff
	 */
	void Selected_UpdateTickableBuffDuration(ISelectable * InSelectable, int32 ArrayIndex,
		float DurationRemaining, bool bIsCurrentSelected);
	/* Debuff version */
	void Selected_UpdateTickableDebuffDuration(ISelectable * InSelectable, int32 ArrayIndex,
		float DurationRemaining, bool bIsCurrentSelected);


	//=========================================================================================
	//	More functions 
	//=========================================================================================

	/** 
	 *	Called whenever a unit enters a building garrison, regardless of whether the building 
	 *	is selected or not 
	 */
	void OnUnitEnteredBuildingGarrison(ABuilding * Building, const FBuildingGarrisonAttributes & BuildingsGarrisonAttributes, AInfantry * UnitThatEnteredGarrison, int32 ContainerIndex);

	/** 
	 *	Called whenever a unit exits a building garrison, regardless of whether the building 
	 *	is selected or not 
	 */
	void OnUnitExitedBuildingGarrison(ABuilding * Building, const FBuildingGarrisonAttributes & BuildingsGarrisonAttributes, int32 LostUnitsContainerIndex);

	/** 
	 *	Called when multiple changes to a building garrison happen at once, regardless of whether the building 
	 *	is selected or not. This is here for 
	 *	performance - calling OnUnitEnteredBuildingGarrison or OnUnitEnteredBuildingGarrison 
	 *	many times should have the same effect 
	 */
	void OnBuildingGarrisonMultipleUnitsEnteredOrExited(ABuilding * Building, const FBuildingGarrisonAttributes & BuildingsGarrisonAttributes, const TArray<TRawPtr(AActor)> & GarrisonedUnitsContainer);

	/** 
	 *	Called when a unit reaches zero health and that unit was garrisoned inside a building
	 *	regardless of whether the building is selected or not. 
	 */
	void OnGarrisonedUnitZeroHealth(ABuilding * Building, int32 LostUnitsContainerIndex);

	bool IsChatBoxShowing() const;
	void OpenChatBox(EMessageRecipients MessageRecipients);
	void CloseChatBox();

	/* Called when a game message is received like "Not enough resources" */
	void OnGameMessageReceived(const FText & Message, EMessageType MessageType);

	/* Called when a in-match chat mesage is received
	@param SendersName - name of player that sent message
	@param Message - what the player typed in
	@prarm MessageType - who the message was sent to */
	void OnChatMessageReceived(const FString & SendersName, const FString & Message,
		EMessageRecipients MessageType);

	void OnMenuWarningHappened(EGameWarning WarningType);

	/* Send what is typed into chat input. This is called BY PC when they press enter but
	focus is not on the chat box */
	void SendChatMessage();

	bool IsPauseMenuShowingOrPlayingShowAnimation() const;

	void ShowPauseMenu();
	void HidePauseMenu();

	/** 
	 *	If there are any menus that should be closed when the player presses ESC then this 
	 *	should close one. 
	 *	
	 *	@return - true if a menu was closed 
	 */
	bool TryCloseEscapableWidget();

	/** 
	 *	Set the initial amount of resources to show. Only call at the start of a match and not
	 *	during one - use OnResourcesChanged instead 
	 */
	void SetInitialResourceAmounts(const TArray < int32 > & ResourceArray, 
		const TArray <FHousingResourceState> & HousingResourceArray);

	/** 
	 *	Call to update HUD when a single resource value changes
	 *	
	 *	@param ResourceType - the type of resource tp update
	 *	@param PreviousAmount - amount resource was at before change happened
	 *	@param NewAmount - new amount resource should be at 
	 */
	void OnResourceChanged(EResourceType ResourceType, int32 PreviousAmount, int32 NewAmount);

	/**
	 *	Call to update HUD when potentially many resource values change
	 *	
	 *	@param PreviousAmounts - resource array before change
	 *	@param NewAmounts - resource array after change 
	 */
	void OnResourcesChanged(const TArray < int32 > & PreviousAmounts, const TArray < int32 > & NewAmounts);

	/* Call when housing resources change. This will update all housing resources */
	void OnHousingResourceConsumptionChanged(const TArray < FHousingResourceState > & ResourceArray);
	void OnHousingResourceProvisionsChanged(const TArray < FHousingResourceState > & ResourceArray);

	/** 
	 *	Called when the local player completes researching an upgrade 
	 *	
	 *	@param bUpgradePrereqsNowMetForSomething - whether a selectable previously required a prereq 
	 *	that was an upgrade but now that this upgrade has been researched it has all its upgrade 
	 *	prereqs fulfilled.
	 */
	void OnUpgradeComplete(EUpgradeType UpgradeType, bool bUpgradePrereqsNowMetForSomething);

	/* Production queue functions */

	/* When an item is added to the production queue but not started immediately */
	void OnItemAddedToProductionQueue(const FTrainingInfo & Item, const FProductionQueue & Queue, AActor * Producer);

	/* When an item is added to a production queue and is started immediately */
	void OnItemAddedAndProductionStarted(const FTrainingInfo & Item, const FProductionQueue & Queue, AActor * Producer);
	// TODO change some classes that call this func to use new one below

	/* When the front of a production queue completes. Assumed next item starts immediately.
	Should be called after the producer has removed what has needed to be removed
	@param Item - item that was just produced
	@param Queue - production queue that just produced something
	@param NumRemoved - number of items removed from the queue because it could be more than 1 if
	some of the other items did not meet prerequisites
	@param Producer - actor that the queue is a part of */
	void OnProductionComplete(const FTrainingInfo & Item, const FProductionQueue & Queue, uint8 NumRemoved, AActor * Producer);

	void OnBuildsInTabProductionComplete(EBuildingType BuildingType);

	void OnBuildsInTabBuildingPlaced(const FTrainingInfo & Item);

	/* More functions */

	/* Functions called when a building is built/destroyed
	@param BuildingType - the type of building that was built
	@param bFirstOfItsType - whether this was the first building of its type for prerequisite
	purposes */
	void OnBuildingConstructed(EBuildingType BuildingType, bool bFirstOfItsType);
	void OnBuildingDestroyed(EBuildingType BuildingType, bool bLastOfItsType);

	/* Called when a player excluding ourselves is considered defeated */
	void OnAnotherPlayerDefeated(ARTSPlayerState * DefeatedPlayer);

	/* Get the image to use for creating button 'on hovered' and 'on pressed' effects */
	UImage * GetMouseFocusWidget() const;

	/* Get mouse focus image's slot */
	UCanvasPanelSlot * GetMouseFocusWidgetSlot() const;

	// GetWorld()->GetRealTimeSeconds() when the pause menu was shown
	float GetTimeWhenPauseMenuWasShown() const;

	//======================================================================================
	//	Tooltip Widgets
	//======================================================================================

#if !UE_BUILD_SHIPPING
	/* Wrapped in preproc cause they are slowish */
	
	// Return true if at least 1 tooltip widget is showing
	bool IsTooltipWidgetShowing() const;

	// Kinda a slow function, but currently only called in non-shipping builds anyway
	int32 GetNumTooltipWidgetsShowing() const;
#endif

	/* @param Button - the button to show a tooltip for. It will either point to a UMyButton 
	or a UUserWidget */
	void ShowTooltipWidget(UWidget * Button, EUIElementType ButtonType);

	// Functions to show tooltip widget. Should really be protected. Call ShowTooltipWidget instead 
	// except for InventoryItemInWorld
	void ShowTooltipWidget_SelectableActionBar_Ability(const FContextButtonInfo & ButtonInfo);
	void ShowTooltipWidget_SelectableActionBar_BuildBuilding(const FDisplayInfoBase & BuildingInfo);
	void ShowTooltipWidget_SelectableActionBar_TrainUnit(const FDisplayInfoBase & UnitInfo);
	void ShowTooltipWidget_SelectableActionBar_ResearchUpgrade(const FDisplayInfoBase & UpgradeInfo);
	void ShowTooltipWidget_PersistentPanel_BuildBuilding(const FDisplayInfoBase & BuildingInfo);
	void ShowTooltipWidget_PersistentPanel_TrainUnit(const FDisplayInfoBase & UnitInfo);
	void ShowTooltipWidget_PersistentPanel_ResearchUpgrade(const FDisplayInfoBase & UpgradeInfo);
	void ShowTooltipWidget_ProductionQueueSlot_BuildBuilding(const FDisplayInfoBase & BuildingInfo);
	void ShowTooltipWidget_ProductionQueueSlot_TrainUnit(const FDisplayInfoBase & UnitInfo);
	void ShowTooltipWidget_ProductionQueueSlot_ResearchUpgrade(const FDisplayInfoBase & UpgradeInfo);
	void ShowTooltipWidget_InventorySlot(const FInventorySlotState & InvSlot, const FInventoryItemInfo & ItemsInfo);
	void ShowTooltipWidget_ShopSlot(const FItemOnDisplayInShopSlot & ShopSlot, const FInventoryItemInfo & ItemsInfo);
	void ShowTooltipWidget_InventoryItemInWorld(const AInventoryItem * InventoryItemActor);
	void ShowTooltipWidget_GlobalSkillsPanelButton(const FAquiredCommanderAbilityState * AbilityState);
	void ShowTooltipWidget_CommanderSkillTreeNode(const FCommanderAbilityTreeNodeInfo * ButtonNodeInfo);

	/* @param Button - the button to hide a tooltip for. It will either point to a UMyButton
	or a UUserWidget */
	void HideTooltipWidget(UWidget * Button, EUIElementType ButtonType);

	/* @param InventoryItem - inv item actor that we want to hide the tooltip for */
	void HideTooltipWidget_InventoryItemInWorld(AInventoryItem * InventoryItem);


public:

	// Try make these protected. I only use them outside this class when showing those confirmation 
	// widgets but can easily change that
	void AddToEscapeRequestResponableWidgets(UInGameWidgetBase * Widget);
	void RemoveFromEscapeRequestResponableWidgets(UInGameWidgetBase * Widget);
};


//============================================================================================
//----------------------------------------------------
//	Context Menu
//----------------------------------------------------
//============================================================================================

/* Widget that appears when clicking on a selectable. Can really show anything you want about
selectable here */
UCLASS(Abstract)
class RTS_VER2_API USelectableContextMenu : public UInGameWidgetBase
{
	// TODO consider changing inheriting class because Initialize assigns GI and GS refs

	GENERATED_BODY()

public:

	virtual bool SetupWidget(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController) override;

	/* Called by the HUD widgets NativeTick */
	virtual void OnTick(float InDeltaTime);

protected:

	/* Holds whatever info you want to be displayed about the selectable such as experience,
	health etc */
	UPROPERTY(meta = (BindWidgetOptional))
	USelectableInfo * Widget_Info;

	/* Actions the selectable can carry out like 'attack move' */
	UPROPERTY(meta = (BindWidgetOptional))
	USelectableActionBar * Widget_Actions;

public:

	void OnPlayerSelectionChanged(const TScriptInterface < ISelectable > NewCurrentSelected);
	void OnPlayerNoSelection();

	/* Functions for when stats of a selected selectable changes. It is assumed that if these are
	called then it is the current selected that needs updating */
	void Selected_OnHealthChanged(float InNewAmount, float MaxHealth);
	void Selected_OnMaxHealthChanged(float NewMaxHealth, float CurrentHealth);
	void Selected_OnCurrentAndMaxHealthChanged(float NewMaxHealth, float CurrentHealth);
	void Selected_OnSelectableResourceCurrentAmountChanged(float CurrentAmount, int32 MaxAmount);
	void Selected_OnSelectableResourceMaxAmountChanged(float CurrentAmount, int32 MaxAmount);
	void Selected_OnSelectableResourceCurrentAndMaxAmountsChanged(float CurrentAmount, int32 MaxAmount);
	void Selected_OnSelectableResourceRegenRateChanged(float PerSecondRegenRate);
	void Selected_OnRankChanged(float ExperienceTowardsNextRank, uint8 InNewRank, float ExperienceRequired);
	void Selected_OnCurrentRankExperienceChanged(float InNewAmount, uint8 CurrentRank, float ExperienceRequired);
	void Selected_OnImpactDamageChanged(float InNewAmount);
	void Selected_OnAttackRateChanged(float AttackRate);
	void Selected_OnAttackRangeChanged(float AttackRange);
	void Selected_OnDefenseMultiplierChanged(float NewDefenseMultiplier);
	void Selected_OnBuffApplied(const FStaticBuffOrDebuffInstanceInfo & BuffInfo);
	void Selected_OnDebuffApplied(const FStaticBuffOrDebuffInstanceInfo & DebuffInfo);
	void Selected_OnBuffApplied(const FTickableBuffOrDebuffInstanceInfo & BuffInfo);
	void Selected_OnDebuffApplied(const FTickableBuffOrDebuffInstanceInfo & DebuffInfo);
	void Selected_OnTickableBuffRemoved(int32 ArrayIndex, EBuffAndDebuffRemovalReason RemovalReason);
	void Selected_OnTickableDebuffRemoved(int32 ArrayIndex, EBuffAndDebuffRemovalReason RemovalReason);
	void Selected_OnStaticBuffRemoved(int32 ArrayIndex, EBuffAndDebuffRemovalReason RemovalReason);
	void Selected_OnStaticDebuffRemoved(int32 ArrayIndex, EBuffAndDebuffRemovalReason RemovalReason);
	void Selected_UpdateTickableBuffDuration(int32 ArrayIndex, float DurationRemaining);
	void Selected_UpdateTickableDebuffDuration(int32 ArrayIndex, float DurationRemaining);
	void Selected_OnItemAddedToInventory(int32 SlotIndex, const FInventorySlotState & UpdatedSlot);
	void Selected_OnItemsRemovedFromInventory(const FInventory * Inventory, const TMap <uint8, FInventoryItemQuantityPair> & RemovedItems);
	void Selected_OnInventoryPositionsSwapped(uint8 DisplayIndex1, uint8 DisplayIndex2);
	void Selected_OnInventoryItemUsed(uint8 DisplayIndex, float UseAbilityTotalCooldown);
	void Selected_OnInventoryItemUseCooldownFinished(uint8 InventorySlotDisplayIndex);
	void Selected_OnItemPurchasedFromShop(uint8 ShopSlotIndex);
	void Selected_OnInventoryItemSold(uint8 InvDisplayIndex);

	/* Functions called when a building is built/destroyed
	@param BuildingType - the type of building that was built
	@param bFirstOfItsType - whether this was the first building of its type for prerequisite
	purposes */
	void OnBuildingConstructed(EBuildingType BuildingType, bool bFirstOfItsType);
	void OnBuildingDestroyed(EBuildingType BuildingType, bool bLastOfItsType);

	void OnUpgradeComplete(EUpgradeType UpgradeType);

	/* Production queue functions */

	/* When an item is added to the production queue but not started immediately */
	void OnItemAddedToProductionQueue(const FTrainingInfo & Item, const FProductionQueue & Queue, AActor * Producer);

	/* When an item is added to a production queue and is started immediately */
	void OnItemAddedAndProductionStarted(const FTrainingInfo & Item, const FProductionQueue & Queue, AActor * Producer);

	/* When the front of a production queue completes. Assumed next item starts immediately
	@param Item - the item that was just produced
	@param Queue - production queue that just produced something
	@param Producer - actor that the queue is a part of */
	void OnProductionComplete(const FTrainingInfo & Item, const FProductionQueue & Queue, 
		uint8 NumRemoved, AActor * Producer);
};


/* Holds info about a selectable. Can virtually hold anything such as experience, icon, health etc */
UCLASS(Abstract)
class RTS_VER2_API USelectableInfo : public UInGameWidgetBase
{
	GENERATED_BODY()

public:

	USelectableInfo(const FObjectInitializer & ObjectInitializer);

	virtual bool SetupWidget(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController) override;

	void OnTick(float InDeltaTime);

protected:

	/* Text to show selectables name */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_Name;

	/* Text to show a description of the selectable */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_Description;

	/* Icon of selectable */
	UPROPERTY(meta = (BindWidgetOptional))
	UImage * Image_Icon;

	/* Current rank/level */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_Level;

	/* Icon for current rank/level */
	UPROPERTY(meta = (BindWidgetOptional))
	UImage * Image_Level;

	/* Optional panel that contains experience related widgets */
	UPROPERTY(meta = (BindWidgetOptional))
	UPanelWidget * Panel_Experience;

	/** 
	 *	Shows how much experience is required to reach the next rank/level. Does not take into 
	 *	account how much current EXP we have so this can only change when we rank up.
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_ExperienceRequired;

	/* Shows how much experience we have towards our next rank/level */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_ExperienceTowardsNextLevel;

	/* Progress bar for experience towards next level */
	UPROPERTY(meta = (BindWidgetOptional))
	UProgressBar * ProgressBar_Experience;

	/* Text that shows health */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_Health;

	/* Text that shows max health */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_MaxHealth;

	/* Progress bar for health */
	UPROPERTY(meta = (BindWidgetOptional))
	UProgressBar * ProgressBar_Health;

	/* An optional panel to put selectable resource related widgets on. It will be made visible 
	if the primary selected uses a selectable resource */
	UPROPERTY(meta = (BindWidgetOptional))
	UPanelWidget * Panel_SelectableResource;

	/* Shows how much of their selectable resource the selectable has. Selectable resources 
	are things like mana, energy */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_SelectableResource;

	/* Shows max amount of selectable resource */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_SelectableResourceMax;

	/* Progress bar to show selectable resource */
	UPROPERTY(meta = (BindWidgetOptional))
	UProgressBar * ProgressBar_SelectableResource;

	/* Text that shows the regeneration rate of the selectable resource. By default the 
	rate is amount regenerated per second */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_SelectableResourceRegenRate;

	/* How much damage selectable deals. Will be made hidden if selectable does not have an 
	attack. This shows impact damage. I haven't added a widget to show AoE damage. I should do 
	that */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_ImpactDamage;

	/* Shows the unit's attack rate. Will be made hidden if selectable does not have an attack */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_AttackRate;

	/* Shows the unit's attack range. Will be made hidden if selectable does not have an attack */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_AttackRange;

	/** 
	 *	Shows the defense multiplier. 
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_DefenseMultiplier;

	/* The panel widget that contains everything about the production queue. This will be hidden
	based on the func ShouldShowProductionQueueInfo. It is assumed ProgressBar_ProductionQueue and
	any production queue buttons added to the user widget will be children of this panel widget
	for easy hidden/visible toggling. */
	UPROPERTY(meta = (BindWidgetOptional))
	UPanelWidget * Panel_ProductionQueueInfo;

	/* To show percentage complete of production */
	UPROPERTY(meta = (BindWidgetOptional))
	UProgressBar * ProgressBar_ProductionQueue;

	/* Array populated in SetupWidget. Holds references to images that will show what is in the
	production queue. */
	UPROPERTY()
	TArray < UProductionQueueButton * > ProductionQueueSlots;

	/* A tally to keep track of how many buttons are activeated in ProductionQueueSlots so we can
	avoid calling SetVisibility needlessly */
	int32 QueueNum;

	/* Queue of current selected, or null if it has none. As long as check if CurrentSelected
	is valid then should be ok dereferncing this I think */
	const FProductionQueue * ProductionQueue;

	/* The selectable resource type of the last selected selectable. This is used for 
	optimization. It is used to know whether we need to change the PBar color for selectable 
	resource */
	ESelectableResourceType LastSelectableResourceType;

	/* Panel for displaying buffs applied */
	UPROPERTY(meta = (BindWidgetOptional))
	UPanelWidget * Panel_Buffs;

	/* The widget to use to display info about each static buff on selectable */
	UPROPERTY(EditAnywhere, Category = "RTS")
	TSubclassOf < USingleBuffOrDebuffWidget > SingleStaticBuffWidget_BP;

	/* Array that mimics Panel_Buffs's children */
	UPROPERTY()
	TArray < USingleBuffOrDebuffWidget * > StaticBuffWidgets;

	/* The widget to use to display info about each tickable buff on selectable */
	UPROPERTY(EditAnywhere, Category = "RTS")
	TSubclassOf < USingleTickableBuffOrDebuffWidget > SingleTickableBuffWidget_BP;

	UPROPERTY()
	TArray < USingleTickableBuffOrDebuffWidget * > TickableBuffWidgets;

	/* Panel for displaying debuffs applied */
	UPROPERTY(meta = (BindWidgetOptional))
	UPanelWidget * Panel_Debuffs;

	/* Widget to display static type debuffs */
	UPROPERTY(EditAnywhere, Category = "RTS")
	TSubclassOf < USingleBuffOrDebuffWidget > SingleStaticDebuffWidget_BP;

	/* This array is the first entries of Panel_Debuffs */
	UPROPERTY()
	TArray < USingleBuffOrDebuffWidget * > StaticDebuffWidgets;

	/* Widget to display tickable debuffs */
	UPROPERTY(EditAnywhere, Category = "RTS")
	TSubclassOf < USingleTickableBuffOrDebuffWidget > SingleTickableDebuffWidget_BP;

	/* This array is the entries after statics in Panel_Debuffs */
	UPROPERTY()
	TArray < USingleTickableBuffOrDebuffWidget * > TickableDebuffWidgets;

	/* Getters that will either get the widget from a pool or create one. Here for when I 
	implement pooling */
	USingleBuffOrDebuffWidget * GetStaticBuffWidget();
	USingleTickableBuffOrDebuffWidget * GetTickableBuffWidget();
	USingleBuffOrDebuffWidget * GetStaticDebuffWidget();
	USingleTickableBuffOrDebuffWidget * GetTickableDebuffWidget();

	/* A panel that shows what items are on display for a shop. Usually they are on sale */
	UPROPERTY(meta = (BindWidgetOptional))
	UPanelWidget * Panel_ItemsOnDisplay;

	/* @See bAutoCreateInventoryItemWidgets */
	UPROPERTY(EditAnywhere, Category = "RTS")
	bool bAutoCreateItemForSaleWidgets;

	/* Widget to use if bAutoCreateItemForSaleWidgets is true */
	UPROPERTY(EditAnywhere, Category = "RTS", meta = (EditCondition = bAutoCreateItemForSaleWidgets))
	TSubclassOf < UItemOnDisplayInShopButton > ItemForSaleWidget_BP;

	/** 
	 *	Called right after a UItemOnDisplayInShopButton widget is added to Panel_ItemsOnDisplay. 
	 *	
	 *	@param PanelWidget - Panel_ItemsOnDisplay 
	 *	@param PanelSlot - slot that JustAdded uses 
	 *	@param JustAdded - widget that was just added to Panel_ItemsOnDisplay 
	 */
	void OnItemOnDisplayButtonAddedToPanel(UPanelWidget * PanelWidget, UPanelSlot * PanelSlot, UItemOnDisplayInShopButton * JustAdded);

	/* Array of widgets that represent an item on display in a shop. If I find these 
	are getting GCed then this array may need to be changed to a UPROPERTY with raw pointers */
	TArray < TWeakObjectPtr<UItemOnDisplayInShopButton> > ItemOnDisplayInShopButtons;

	/* A panel that shows the inventory of a selectable */
	UPROPERTY(meta = (BindWidgetOptional))
	UPanelWidget * Panel_Inventory;

	/* Array of inventory slots with items that are coolding down */
	TArray < UInventoryItemButton * > CoolingDownInventorySlots;

	/** 
	 *	If true then the buttons that represent an item in the inventory will be automatically 
	 *	created during HUD initialization at runtime. You will just need to specify a widget 
	 *	blueprint to use for each button. After each button has been added to the panel the 
	 *	function OnInventoryItemButtonAddedToPanel will be called giving you a chance to tweek 
	 *	exactly how the widget is positioned if needed. Also you should remove any 
	 *	UInventoryItemButton widgets that you have added via editor since they will be ignored.
	 *	
	 *	If false then you can just add the button widgets in the editor. Make sure you add an 
	 *	amount of buttons equal to the max size inventory of all selectables e.g. if the most 
	 *	number of items a selectable can carry is 6 then make sure you add 6 buttons. Generally the 
	 *	first button you add will be considered the first inventory slot, the 2nd you add will be 
	 *	the 2nd inventory slot, etc. 
	 */
	UPROPERTY(EditAnywhere, Category = "RTS")
	bool bAutoCreateInventoryItemWidgets;

	/** 
	 *	If choosing to auto populate the inventory panel then this is the button widget to use 
	 *	for it. 
	 */
	UPROPERTY(EditAnywhere, Category = "RTS", meta = (EditCondition = bAutoCreateInventoryItemWidgets))
	TSubclassOf < UInventoryItemButton > InventoryItemWidget_BP;

	/**
	 *	Called right after a UInventoryItemButton widget is added to Panel_Inventory 
	 *	
	 *	@param Panel - Panel_Inventory 
	 *	@param PanelSlot - slot that JustAdded uses 
	 *	@param JustAdded - widget that was just added as a child of Panel_Inventory
	 */
	void OnInventoryItemButtonAddedToPanel(UPanelWidget * Panel, UPanelSlot * PanelSlot, UInventoryItemButton * JustAdded);

	/* Array of the widgets that represent an item in a selectable's inventory. If I find these 
	are getting GCed then this array may need to be changed to a UPROPERTY with raw pointers */
	TArray < TWeakObjectPtr <UInventoryItemButton> > InventoryButtons;

	void SetInfoVisibilities(const TScriptInterface < ISelectable > & Selectable, const FSelectableAttributesBase & Attributes);

	/** 
	 *	Functions to update what is shown the the player selection changes
	 *	
	 *	@param Selectable - selectable to show info on
	 *	@param Attributes - selectables attributes here for convenience 
	 */
	void UpdateTextName(const FSelectableAttributesBase & Attributes);
	void UpdateTextDescription(const FSelectableAttributesBase & Attributes);
	void UpdateImageIcon(const FSelectableAttributesBase & Attributes);
	void UpdateTextLevel(uint8 InLevel);
	void UpdateImageLevel(const FRankInfo & RankInfo);
	void UpdateTextExperienceRequired(uint8 Rank, float ExperienceRequired);
	void UpdateTextExperienceTowardsNextLevel(float CurrentEXP, uint8 CurrentRank);
	void UpdateProgressBarExperience(float CurrentEXP, uint8 Rank, float ExperienceRequired);
	void UpdateTextHealth(float NewHealth);
	void UpdateTextMaxHealth(float NewMaxHealth);
	void UpdateProgressBarHealth(float CurrentHealth, float MaxHealth);
	void UpdateTextSelectableResource(float NewAmount);
	void UpdateTextSelectableResourceMax(int32 NewAmount);
	void UpdateProgressBarSelectableResource(float CurrentAmount, int32 MaxAmount);
	void UpdateTextSelectableResourceRegenRate(float PerSecondRegenRate);
	void UpdateTextImpactDamage(float Damage);
	void UpdateTextAttackRate(float AttackRate);
	void UpdateTextAttackRange(float AttackRange);
	void UpdateTextDefenseMultiplier(float DefenseMultiplier);
	/* Called each tick. Calls ShouldShowProductionProgressBar and either update progress bar
	for production or hide it. Should be called when player selection changes and on tick */
	void UpdateProgressBarProductionQueue(const TScriptInterface < ISelectable > & Selectable);
	void UpdateBuffsAndDebuffs(const TScriptInterface < ISelectable > & Selectable);

	/* Update what shows in the production queue */
	void UpdateProductionQueueContents();

	/* Whether a certain selectable should have its production progress bar shown or not.
	Buildings that can produce things are a yes, infantry probably not */
	bool ShouldShowProductionQueueInfo(const TScriptInterface < ISelectable > & Selectable) const;

	/* Given a selectable return whether we should show the attack related attributes like 
	attack damage, attack range etc for it */
	bool ShouldShowAttackAttributes(const TScriptInterface < ISelectable > & Selectable) const;

	/* Convert a selectable's attack damage from float to FText */
	static FText DamageFloatToText(float DamageValue);

	/* Convert a selectable's attack rate from float to FText */
	static FText AttackRateFloatToText(float AttackRate);

	/* Convert a selectable's attack range from float to FText */
	static FText AttackRangeFloatToText(float AttackRange);

	/* Convert a float value that represents current health to an FText */
	static FText CurrentHealthFloatToText(float HealthValue);

	/* Convert a float value that represents max health to an FText */
	static FText MaxHealthFloatToText(float MaxHealthValue);

	/* Convert a float value that represents current amount of a selectable resource to an FText */
	static FText SelectableResourceCurrentAmountFloatToText(float Amount);

	/* Convert a float that displays the selectable resource regenerated rate to an FText */
	static FText SelectableResourceRegenRateFloatToText(float RegenRateFloat);

	/* Convert a defense multiplier to some text for it to be displayed on the HUD */
	static FText DefenseMultiplierFloatToText(float DefenseMultiplier);

	/* This is actually only here because my FloatToText macro can't handle assigning values 
	to FText in function bodies */
	static FText DefenseMultiplierFloatToTextInner(float DefenseMultiplier);

	/* Convert a float that represents a experience amount gained towards next rank to an FText */
	static FText ExperienceGainedFloatToText(float ExperienceAmount);

	/* Convert a float that represents a experience amount required to rank up to an FText */
	static FText ExperienceRequiredFloatToText(float ExperienceAmount);

public:

	void OnPlayerSelectionChanged(const TScriptInterface < ISelectable > NewCurrentSelected, const FSelectableAttributesBase & AttributesBase);
	void OnNoPlayerSelection();

	// Functions for when stats of the current selected changes
	void Selected_OnHealthChanged(float InNewAmount, float MaxHealth);
	void Selected_OnMaxHealthChanged(float NewMaxHealth, float CurrentHealth);
	void Selected_OnCurrentAndMaxHealthChanged(float NewMaxHealth, float CurrentHealth);
	void Selected_OnSelectableResourceCurrentAmountChanged(float CurrentAmount, int32 MaxAmount);
	void Selected_OnSelectableResourceMaxAmountChanged(float CurrentAmount, int32 MaxAmount);
	void Selected_OnSelectableResourceCurrentAndMaxAmountsChanged(float CurrentAmount, int32 MaxAmount);
	void Selected_OnSelectableResourceRegenRateChanged(float PerSecondRegenRate);
	void Selected_OnRankChanged(float ExperienceTowardsNextRank, uint8 InNewRank, float ExperienceRequired);
	void Selected_OnCurrentRankExperienceChanged(float InNewAmount, uint8 Rank, float ExperienceRequired);
	void Selected_OnImpactDamageChanged(float Damage);
	void Selected_OnAttackRateChanged(float AttackRate);
	void Selected_OnAttackRangeChanged(float AttackRange);
	void Selected_OnDefenseMultiplierChanged(float NewDefenseMultiplier);
	void Selected_OnBuffApplied(const FStaticBuffOrDebuffInstanceInfo & BuffInfo);
	void Selected_OnDebuffApplied(const FStaticBuffOrDebuffInstanceInfo & DebuffInfo);
	void Selected_OnBuffApplied(const FTickableBuffOrDebuffInstanceInfo & BuffInfo);
	void Selected_OnDebuffApplied(const FTickableBuffOrDebuffInstanceInfo & DebuffInfo);
	void Selected_OnTickableBuffRemoved(int32 ArrayIndex, EBuffAndDebuffRemovalReason RemovalReason);
	void Selected_OnTickableDebuffRemoved(int32 ArrayIndex, EBuffAndDebuffRemovalReason RemovalReason);
	void Selected_OnStaticBuffRemoved(int32 ArrayIndex, EBuffAndDebuffRemovalReason RemovalReason);
	void Selected_OnStaticDebuffRemoved(int32 ArrayIndex, EBuffAndDebuffRemovalReason RemovalReason);
	void Selected_UpdateTickableBuffDuration(int32 ArrayIndex, float DurationRemaining);
	void Selected_UpdateTickableDebuffDuration(int32 ArrayIndex, float DurationRemaining);
	void Selected_OnItemAddedToInventory(int32 SlotIndex, const FInventorySlotState & UpdatedSlot);
	void Selected_OnItemsRemovedFromInventory(const FInventory * Inventory, const TMap <uint8, FInventoryItemQuantityPair> & RemovedItems);
	void Selected_OnInventoryPositionsSwapped(uint8 DisplayIndex1, uint8 DisplayIndex2);
	// @param TotalUseCooldown - pass in 0 to say 'has no cooldown'
	void Selected_OnInventoryItemUsed(uint8 DisplayIndex, float TotalUseCooldown);
	void Selected_OnInventoryItemUseCooldownFinished(uint8 InventorySlotDisplayIndex);
	void Selected_OnItemPurchasedFromShop(uint8 ShopSlotIndex);
	void Selected_OnInventoryItemSold(uint8 InvDisplayIndex);

	//===================================================================================
	/* Production queue functions */
	//===================================================================================
	
	/* When an item is added to the production queue but not started immediately */
	void OnItemAddedToProductionQueue(const FTrainingInfo & Item, const FProductionQueue & Queue, AActor * Producer);

	/* When an item is added to a production queue and is started immediately */
	void OnItemAddedAndProductionStarted(const FTrainingInfo & Item, const FProductionQueue & Queue, AActor * Producer);

	/* When the front of a production queue completes. Assumed next item starts immediately
	@param Queue - production queue that just produced something
	@param Producer - actor that the queue is a part of */
	void OnProductionComplete(const FTrainingInfo & Item, const FProductionQueue & Queue, 
		uint8 NumRemoved, AActor * Producer);
};


/** 
 *	A button for showing what is in the production queue. It contains a button to enable 
 *	cancelling items in queue but this functionality has not been implemented yet
 */
UCLASS(meta = (DisplayName = "HUD Context Menu: Production Queue Slot"))
class RTS_VER2_API UProductionQueueButton : public UMyButton
{
	GENERATED_BODY()

public:

	UProductionQueueButton(const FObjectInitializer & ObjectInitializer);

	/* Assign reference to image and setup clicked functionality */
	void Setup(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController, 
		AFactionInfo * InFactionInfo);

protected:

	/* Reference to local player controller */
	UPROPERTY()
	ARTSPlayerController * PC;

	/* Reference to faction info of local player */
	UPROPERTY()
	AFactionInfo * FI;

	void UIBinding_OnLeftMouseButtonPress();
	void UIBinding_OnLeftMouseButtonReleased();
	void UIBinding_OnRightMouseButtonPress();
	void UIBinding_OnRightMouseButtonReleased();

	/* The info this button is for */
	FContextButton ButtonType;

public:

	const FContextButton * GetButtonType() const;

	void OnPlayerSelectionChanged(const TScriptInterface < ISelectable > NewCurrentSelected);

	/* Make this button show the image for a certain item in production queue */
	void Assign(const FTrainingInfo & Item, FUnifiedImageAndSoundFlags InUnifiedAssetFlags);

	/* When slot no longer has queue item in it */
	void Unassign();

#if WITH_EDITOR
	/* Called when created from palette in editor */
	virtual void OnCreationFromPalette() override;

	virtual const FText GetPaletteCategory() override;
#endif // WITH_EDITOR
};


/** 
 *	A widget that represents an item that is on display in a shop. Usually these items can be 
 *	purchased. 
 *
 *	Better name perhaps = UShopWindowSlotButton
 */
UCLASS(Abstract)
class RTS_VER2_API UItemOnDisplayInShopButton : public UInGameWidgetBase
{
	GENERATED_BODY()

public:

	enum class NotFunctionalReason : uint8
	{
		// Fully functional
		NoReason,

		/* Item has a limited quantity and has sold out */
		SoldOut,
	};

	UItemOnDisplayInShopButton(const FObjectInitializer & ObjectInitializer);

	virtual bool SetupWidget(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController) override;

	// Do more setup stuff
	void SetupMore(USelectableInfo * InOwningWidget);

protected:

	/* Widget this is a child of */
	const USelectableInfo * OwningWidget;

	/* The slot in the shop this button should show info for. If this info is ever contained 
	in FItemOnDisplayInShopSlot then this can be removed */
	uint8 ShopSlotIndex;

	/* State info for what this button is selling */
	const FItemOnDisplayInShopSlot * SlotStateInfo;

	/* Info struct for ItemTypeThisIsFor */
	const FInventoryItemInfo * ItemInfo;

	float OriginalOpacity;

	/* Button to handle clicks */
	UPROPERTY(meta = (BindWidget))
	UMyButton * Button_Button;

	/* Text to show the name of the item */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_Name;

	/* Text to show the number of purchases remaining */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_PurchasesRemaining;

	FText GetPurchasesRemainingText() const;

	/* Get the render opacity for this widget when the item it represents has sold out */
	float GetSoldOutRenderOpacity() const;

	void UIBinding_OnLeftMouseButtonPress();
	void UIBinding_OnLeftMouseButtonReleased();
	void UIBinding_OnRightMouseButtonPress();
	void UIBinding_OnRightMouseButtonReleased();

public:

	uint8 GetShopSlotIndex() const;

	const FItemOnDisplayInShopSlot * GetSlotStateInfo() const;

	const FInventoryItemInfo * GetItemInfo() const;

	/**
	 *	Set up this widget for a certain slot in a shop
	 *
	 *	@param InShopSlotIndex - what button in the shop this is for 
	 *	@param SlotInfo - contains info about the slot 
	 */
	void MakeActive(uint8 InShopSlotIndex, const FItemOnDisplayInShopSlot & SlotInfo, 
		FUnifiedImageAndSoundFlags UnifiedAssetFlags);

	/* Called when a purchase is made from this slot */
	void OnPurchaseFromHere();
};


/** 
 *	A button that represents an item in a selectable's inventory. 
 *	
 *	Tip: to achieve displaying an image an text overtop the button put an overlay on it, then 
 *	add the image and text widgets as childs to the overlay. Actually UButton can display an 
 *	image on it anyway so I can completely do away with the image widget and set the image on 
 *	the button instead.
 *
 *	Better name = UInventorySlotButton 
 */
UCLASS(Abstract)
class RTS_VER2_API UInventoryItemButton : public UInGameWidgetBase
{
	GENERATED_BODY()

public:

	UInventoryItemButton(const FObjectInitializer & ObjectInitializer);

	virtual bool SetupWidget(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController) override;

protected:

	/* Inventory slot this widget is displaying info for */
	const FInventorySlotState * InventorySlot;

	/* Pointer to info struct for what is in this slot. If this widget is representing an 
	empty inventory slot then this will be null. This may not actually be needed */
	const FInventoryItemInfo * SlotsItemInfo;

	/* The button that will handle things like using the item if it is a usable item */
	UPROPERTY(meta = (BindWidgetOptional))
	UMyButton * Button_Button;

	/* The index in FInventory::SlotsArray this widget represents. Should probably be moved 
	closer to InventorySlot */
	uint8 ServerSlotIndex;

	/* [Optimization] The item type that the widget is displaying. Here to try avoid dirtying 
	widgets when they don't actually change what they display */
	EInventoryItem DisplayedItemType;

	/* [Optimization] Because UTextBlock::SetText does not check if text has changed */
	int16 StackQuantityOrNumChargesShown;

	// Duplicate of what's already stored in GI; don't really need it
	FUnifiedImageAndSoundFlags UnifiedBrushAndSoundFlags;

	/** 
	 *	Text that shows either: 
	 *	- how many of the item are in the stack 
	 *	- how many charges the item has 
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_QuantityOrNumCharges;

	/* Text to show the name of the item */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_Name;

	/* Text that shows the cooldown remaining on the item's 'use' ability if any */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_UseCooldown;

	/* Progress bar that shows the cooldown remaining on the item's 'use' ability if any */
	UPROPERTY(meta = (BindWidgetOptional))
	UProgressBar * ProgressBar_UseCooldown;

	/* The cooldown of the use ability of the item this widget represents. Irrelevant if the 
	item does not have a use ability. If it has no cooldown then this will be 0. 
	This data is also stored on the FContextButtonInfo struct so a pointer to that in place 
	of this variable is an option */
	float ItemUseTotalCooldown;

	/* Often the param is just UnifiedBrushAndSoundFlags */
	void SetAppearanceForEmptySlot(FUnifiedImageAndSoundFlags UnifiedAssetFlags);

	static FText ItemUseCooldownFloatToText(float Cooldown);

	void UIBinding_OnButtonLeftMouseButtonPressed();
	void UIBinding_OnButtonLeftMouseButtonReleased();
	void UIBinding_OnButtonRightMouseButtonPressed();
	void UIBinding_OnButtonRightMouseButtonReleased();

public:

	/* Get the inventory slot this widget is displaying info for */
	const FInventorySlotState * GetInventorySlot() const;

	/* Get the item info struct for the item this slot represents. Will be null if this slot 
	represents an empty slot */
	const FInventoryItemInfo * GetItemInfo() const;

	/* Get which index in FInventory::SlotsArray this widget is displaying info for */
	uint8 GetServerSlotIndex() const;

	/*
	 *	Makes this button show info for a certain stack of items. It is valid for an "empty" to 
	 *	be passed into this in which case that means the slot does not have any item in it 
	 *	
	 *	@param ItemsInfo - item info for inv slot. Can be null if slot has nothing in it 
	 *	@return - true if the item that was assigned to this slot has its use ability coolding down 
	 */
	bool MakeActive(const FInventorySlotState & SlotInfo, uint8 InServerSlotIndex, 
		FTimerManager & WorldTimerManager, const FInventoryItemInfo * ItemsInfo);

	void OnItemAdded(const FInventorySlotState & UpdatedSlot);
	
	/* Possibly this should only be called for stack amounts changing and not for charges 
	being consumed */
	void OnAmountRemoved();

	/** 
	 *	Call when the item is used
	 */
	void OnUsed(FTimerManager & WorldTimerManager);

	/* Update the cooldown on tick */
	void UpdateCooldown(float DeltaTime, const FTimerManager & WorldTimerManager);

	/* Called when the cooldown for the item this widget represents is finished */
	void OnUseCooldownFinished();
};


/* Holds the actions a selectable can do like 'attack move' */
UCLASS(Abstract)
class RTS_VER2_API USelectableActionBar : public UInGameWidgetBase
{
	GENERATED_BODY()

public:

	USelectableActionBar(const FObjectInitializer & ObjectInitializer);

	virtual bool SetupWidget(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController) override;

	void OnTick(float InDeltaTime);

protected:

	/* Alpha for buttons when they are not clickable because prerequisites are not met */
	UPROPERTY(EditAnywhere, Category = "RTS", meta = (ClampMin = "0.0"))
	float UnclickableButtonAlpha;

	/* Array of buttons for each action slot */
	UPROPERTY()
	TArray < UContextActionButton * > ActionButtons;

	/* Maps a button to the widget assigned to that button */
	UPROPERTY()
	TMap < FContextButton, UContextActionButton * > ActionButtonsMap;

	/* Number of buttons active in ActionButtons i.e. the number of context buttons on the current
	selected's context menu (including whether they have prereqs met or not) */
	int32 NumButtonsInMenu;

public:

	void OnPlayerSelectionChanged(const TScriptInterface < ISelectable > NewCurrentSelected, const FSelectableAttributesBase & Attributes);

	/* Call when player changes to no selection */
	void OnNoPlayerSelection();

	/* Update the cooldowns for each button */
	void UpdateButtonCooldowns();

	//===================================================================================
	/* Production queue functions */
	//===================================================================================

	void OnProductionComplete(const FTrainingInfo & Item, const FProductionQueue & Queue,
		uint8 NumRemoved, AActor * Producer);

	//===================================================================================
	//	Upgrade Functions
	//===================================================================================

	void OnUpgradeComplete(EUpgradeType UpgradeType);

	//===================================================================================
	//	Building Created/Destroyed Functions
	//===================================================================================

	/* Functions called when a building is built/destroyed
	@param BuildingType - the type of building that was built
	@param bFirstOfItsType - whether this was the first building of its type for prerequisite
	purposes */
	void OnBuildingConstructed(EBuildingType BuildingType, bool bFirstOfItsType);
	void OnBuildingDestroyed(EBuildingType BuildingType, bool bLastOfItsType);
};


/** 
 *	A single button for a context action. It will construct extra widgets when placed in editor. 
 *	A few assumptions are made when using these:
 *	- That the overlay widget is a direct child of this
 *	- That the image and progress bar widgets are direct children of the overlay widget 
 */
UCLASS(meta = (DisplayName = "HUD Context Menu: Action Button"))
class RTS_VER2_API UContextActionButton : public UMyButton
{
	GENERATED_BODY()

public:

	UContextActionButton(const FObjectInitializer & ObjectInitializer);

	void Setup(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController);

#if WITH_EDITOR
	/* Called when created from palette in editor */
	virtual void OnCreationFromPalette() override;
#endif

	/* Made public and static to allow UHUDPersistentTabButton to use it 
	
	@param bCreateCooldownText - whether to create a text block for the purpose of displaying 
	the cooldown remaining */
	static void OnButtonCreationFromPalette(UMyButton * Button, bool bCreateCooldownText);

	/* Set params to point to first image and progress bar found that are children. Made static so
	UHUDPersistentTabButton can reuse it 
	
	@param bFindCooldownRemainingText - whether to find a text block for displaying a duration 
	value. If false then the param InCooldownRemainingText will not be touched. If this param is 
	true then it is likely OnButtonCreationFromPalette with bCreateCooldownText as true was called 
	to setup the widget */
	static void SetImageAndProgressBarReferences(UMyButton * Caller,
		UProgressBar *& InProgressBar, UMultiLineEditableText *& InDescriptionText,
		UMultiLineEditableText *& InExtraText, bool bFindCooldownRemainingText,
		UTextBlock *& InCooldownRemainingText);

protected:

	/* Reference to the progress bar that shows the cooldown remaining */
	UPROPERTY()
	UProgressBar * CooldownProgressBar;

	/* Reference to the text block that shows the ability's cooldown remaining. Could also 
	show the time left on construction progress too I guess */
	UPROPERTY()
	UTextBlock * CooldownRemainingText;

	/* Whether CooldownRemainingText has its text set as empty or not. This is here because
	we're not fully event driven on abilities coming off cooldown and this should give a
	performance boost by not setting the text block's text to empty every frame. */
	bool bIsCooldownTextEmpty;

	/* The text that shows on the button describing what the button is for. Really just 1 or 2
	words - it's not the actual description of what the button does */
	UPROPERTY()
	UMultiLineEditableText * DescriptionText;

	/* More text intended to show whatever, perhaps the remaining cooldown of button */
	UPROPERTY()
	UMultiLineEditableText * ExtraText;

	/* Whether this button is for a Train/Upgrade type button or a BuildBuilding button that
	is either BuildsInTab or BuildsItself */
	bool bIsForProductionAction;

	/* Production queue if using this button for a production action */
	const FProductionQueue * ProductionQueue;

	/* Timer handle for updating progress bar. Ignored if using for production action */
	const FTimerHandle * TimerHandle_ProgressBar;

	/* Type of action this button is for. Will change as players current selected changes */
	const FContextButton * ButtonType;

	/* Reference to game instance */
	UPROPERTY()
	URTSGameInstance * GI;

	/* Reference to player controller */
	UPROPERTY()
	ARTSPlayerController * PC;

	/* Reference to player state that uses this */
	UPROPERTY()
	ARTSPlayerState * PS;

	/* Reference to faction info of local player */
	UPROPERTY()
	AFactionInfo * FI;

	bool bPrerequisitesMet;

	/* Original alpha of button cached so it can be restored when prerequisites are met */
	float OriginalAlpha;

	/* Convert a float value to a FText. */
	static FText GetDurationRemainingText(float Duration);

	/* Set the appearance of this button given that we know it is for an upgrade and that the
	upgrade cannot be researched anymore */
	void SetAppearanceForFullyResearchedUpgrade();

	void UIBinding_OnLMBPressed();
	void UIBinding_OnLMBReleased();
	void UIBinding_OnRMBPressed();
	void UIBinding_OnRMBReleased();

public:

	/* Version for TrainUnit/Upgrade */
	void MakeActive(const FContextButton * InButtonType, const FDisplayInfoBase * DisplayInfo, 
		const FBuildingAttributes & PrimarySelectedBuildingAttributes, 
		FUnifiedImageAndSoundFlags UnifiedAssetFlags, float InUnclickableButtonAlpha);

	/** 
	 *	Version for BuildBuilding 
	 *	
	 *	@param BuildingInfo - info struct for building button represents, not primary selected 
	 *	@param PrimarySelectedBuildingAttributes - FBuildingAttributes for the player's primary 
	 *	selected. Can be null if they are not a building. 
	 */
	void MakeActive(const FContextButton * InButtonType, const FBuildingInfo * BuildingInfo, 
		const FBuildingAttributes * PrimarySelectedBuildingAttributes, 
		FUnifiedImageAndSoundFlags UnifiedAssetFlags, float InUnclickableButtonAlpha);

	/* Version for abilities */
	void MakeActive(const FContextButton * InButtonType, const FContextButtonInfo & AbilityInfo,
		const FTimerHandle * CooldownTimerHandle, FTimerManager & WorldTimerManager, 
		FUnifiedImageAndSoundFlags UnifiedAssetFlags);

	/* Update the cooldown progress bar and text */
	void UpdateCooldownProgressBarAndText();

	/* Makes widget invisible */
	void Disable();

	/* Return true if the button is currently setup for use */
	bool ArePrequisitesMet() const;

	/* Get the button functionality for this button */
	const FContextButton * GetButtonType() const;

	template <typename T> 
	T * GetDisplayedInfo() const
	{
		return static_cast<T*>(DisplayedInfo);
	}

	void OnPrerequisitesMet();
	void OnPrerequisiteLost(float NewAlpha);

	/* Called if this button is for an upgrade and the upgrade completed */
	void OnUpgradeResearchComplete();

#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif
};


/** Info about a single garrisoned unit */
UCLASS(Abstract)
class RTS_VER2_API UGarrisonedUnitInfo : public UInGameWidgetBase
{
	GENERATED_BODY()

public:

	virtual bool SetupWidget(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController) override;

	void SetAppearanceFor(AActor * Selectable);

protected:

	void UIBinding_OnUnloadButtonLMBPressed();
	void UIBinding_OnUnloadButtonLMBReleased();
	void UIBinding_OnUnloadButtonRMBPressed();
	void UIBinding_OnUnloadButtonRMBReleased();

public:

	void OnUnloadButtonClicked(ARTSPlayerController * PlayCon);

	AActor * GetGarrisonedSelectable() const { return GarrisonedSelectable; }

protected:

	//--------------------------------------------------------------
	//	Data
	//--------------------------------------------------------------

	/** Button to display image of unit and when clicked to unload the unit */
	UPROPERTY(meta = (BindWidgetOptional))
	UMyButton * Button_IconAndUnload;

	/**
	 *	The selectable garrisoned inside the building. This may be stale if the widget hides 
	 *	this button but should not be an issue because the button is hidden and therefore not 
	 *	clickable! 
	 */
	AActor * GarrisonedSelectable;
};


/** This widget was added to show what units are inside garrisons */
UCLASS(Abstract, meta = (DisplayName = "Garrison Info Widget"))
class RTS_VER2_API UGarrisonInfo : public UInGameWidgetBase
{
	GENERATED_BODY()

public:

	virtual bool SetupWidget(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController) override;

	void OnPlayerPrimarySelectedChanged(const TScriptInterface<ISelectable> & NewPrimarySelected);
	void OnPlayerNoSelection();
	void OnBuildingGarrisonedUnitGained(ABuilding * Building, AInfantry * GainedUnit, int32 ContainerIndex);
	void OnBuildingGarrisonedUnitLost(ABuilding * Building, int32 ContainerIndex);
	void OnBuildingPotentiallyManyGarrisonedUnitsLost(ABuilding * Building, const TArray<TRawPtr(AActor)> & GarrisonedUnitsContainer);

protected:

	void UIBinding_OnUnloadGarrisonButtonLeftMousePress();
	void UIBinding_OnUnloadGarrisonButtonLeftMouseReleased();
	void UIBinding_OnUnloadGarrisonButtonRightMousePress();
	void UIBinding_OnUnloadGarrisonButtonRightMouseReleased();

public:

	void OnUnloadGarrisonButtonClicked();

protected:

	//------------------------------------------------------------------
	//	Data
	//------------------------------------------------------------------

	/* The building garrison network type for the primary selected. If primary selected is not a 
	building then this will likely be "None" */
	EBuildingNetworkType PrimarySelectedsGarrisonNetworkType;

	/* Widget that will display info about garrisoned unit */
	UPROPERTY(EditAnywhere, Category = "RTS", meta = (DisplayName = "Single Unit Info Widget"))
	TSubclassOf<UGarrisonedUnitInfo> SingleUnitInfoWidget_BP;

	/* Panel that will show all the units */
	UPROPERTY(meta = (BindWidget))
	UPanelWidget * Panel_Units;

	/* A button that when clicked will unload all units inside the garrison */
	UPROPERTY(meta = (BindWidgetOptional))
	UMyButton * Button_UnloadGarrison;
};


//==============================================================================================
//----------------------------------------------------------------------------------------------
//	Resources Widget
//----------------------------------------------------------------------------------------------
//==============================================================================================

/* Widget that holds single resource widgets. */
UCLASS(Abstract)
class RTS_VER2_API UHUDResourcesWidget : public UInGameWidgetBase
{
	GENERATED_BODY()

	// TODO: consider changing parent 

public:

	virtual bool SetupWidget(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController) override;

	void OnTick(float DeltaTime);

protected:

	/* Maps resource type to its panel widget */
	UPROPERTY()
	TMap < EResourceType, UHUDSingleResourcesWidget * > ResourceWidgets;

	/* Array that holds all the single housing resource widgets. 
	Key = Statics::HousingResourceTypeToArrayIndex */
	UPROPERTY()
	TArray < UHUDSingleHousingResourceWidget * > HousingResourceWidgets;

public:

	/** 
	 *	Set the starting values. Only call once at start of the match 
	 *	
	 *	@param InInitialValueArray - array that holds how much of each resource we started match with 
	 *	@param InInitialHousingProvided - array that holds how much of eaqch housing resource we 
	 *	started the match with 
	 */
	void SetInitialResourceAmounts(const TArray < int32 > & InInitialValueArray, 
		const TArray < FHousingResourceState > & InInitialHousingProvided);

	void OnResourcesChanged(EResourceType ResourceType, int32 PreviousAmount, int32 NewAmount);

	void OnHousingResourceConsumptionChanged(const TArray < FHousingResourceState > & HousingResourceArray);
	void OnHousingResourceProvisionsChanged(const TArray < FHousingResourceState > & HousingResourceArray);
};


/** 
 *	A widget that shows how much of a single resource type the player has. 
 *	
 *	Not used for housing resource types 
 */
UCLASS(Abstract)
class RTS_VER2_API UHUDSingleResourcesWidget : public UInGameWidgetBase
{
	GENERATED_BODY()

public:

	virtual bool SetupWidget(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController) override;

	void OnTick(float InDeltaTime);

protected:

	/* Check if UpdateCurve is usable and null it if it is not */
	void CheckUpdateCurve();

	/* Resource type this is for */
	UPROPERTY(EditAnywhere, Category = "RTS")
	EResourceType ResourceType;

	/* Curve for playing the decreasing money over time tick down/up effect like in SCII. If no
	curve is set then resources are updated instantly. Also resources are always subtracted
	instantly in player state - this is just for visuals only and doesn't mean if the curve is
	slow you can spend those resources again.

	Curve should be increasing.
	X axis = duration to go from current value to new value
	Y axis = percentage towards new value (should have a range from 0 to 1).

	My notes: If resources changes and the value displayed is already part way towards the new
	value then the update time will not be the full length of curve. If the displayed value hasn't
	caught up to the actual value and a change causes the new value to go even further away from
	the displayed value then it will take the full curve duration but not any longer */
	UPROPERTY(EditAnywhere, Category = "RTS")
	UCurveFloat * UpdateCurve;

	//~ For update curve only
	int32 StartAmount;
	int32 CurrentAmount;
	int32 TargetAmount;
	float UpdateCurveDuration;
	float CurrentCurveTime;
	int32 Range;

	/* Resource icon */
	UPROPERTY(meta = (BindWidgetOptional))
	UImage * Image_Icon;

	/* Amount of resource */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_Amount;

	/* Update what is shown in widget
	@param NewAmount - new amount to change to
	@param bUsingFloatCurve - true if UpdateCurve is being used */
	void UpdateResourceDisplayAmount(int32 NewAmount, bool bUsingFloatCurve);

public:

	EResourceType GetResourceType() const;

	/* Only call once at the start of the match */
	void SetInitialValue(int32 InitialValue);

	void OnResourcesChanged(int32 PreviousAmount, int32 NewAmount);
};


/** 
 *	A widget that shows how much of a single housing resource the player is consuming and 
 *	providing.
 *
 *	The values do not update using curves
 */
UCLASS(Abstract)
class RTS_VER2_API UHUDSingleHousingResourceWidget : public UInGameWidgetBase
{
	GENERATED_BODY()

public:

	UHUDSingleHousingResourceWidget(const FObjectInitializer & ObjectInitializer);

	/* I named this func like this in case I decide to change the parent class and accidentally 
	override a virtual */
	void SetupResourceWidget();

protected:

	/* The housing resource this widget is for */
	UPROPERTY(EditAnywhere, Category = "RTS")
	EHousingResourceType ResourceType;

	FLinearColor Consumed_OriginalColor;
	FLinearColor Provided_OriginalColor;

	/* The color to make the consumed text when the amount consumed goes over the amount 
	provided */
	UPROPERTY(EditAnywhere, Category = "RTS")
	FLinearColor Consumed_OverCapColor;

	/* The color to make the provided text when the amount consumed goes over the amount
	provided */
	UPROPERTY(EditAnywhere, Category = "RTS")
	FLinearColor Provided_OverCapColor;

	/* If true then this widget is displaying as if the player is consuming more than they are 
	producing */
	bool bIsOverCap;

	/* Text to show the amount of the resource being consumed */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_AmountConsumed;

	/* Text to show the amount of the resource being provided */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_AmountProvided;

	/* Check if the state has gone from over cap to within cap or vice versa and call appropriate 
	function if so */
	void CheckIfStateHasChanged(int32 AmountConsumed, int32 AmountProvided);

	/* Called when the player starts consuming more than they are providing */
	void OnGoneOverCap();
	
	/* Called when the player stops consuming more than they are providing */
	void OnReturnedToWithinCap();

public:

	EHousingResourceType GetResourceType() const;

	void SetInitialValue(int32 InitialAmountConsumed, int32 InitialAmountProvided);

	void OnAmountConsumedChanged(int32 NewAmountConsumed, int32 AmountProvided);
	void OnAmountProvidedChanged(int32 AmountConsumed, int32 NewAmountProvided);
};


//####################################################
//----------------------------------------------------
//	Chat Input
//----------------------------------------------------
//####################################################

/* Widget that shows when wanting to enter chat message */
UCLASS(Abstract)
class RTS_VER2_API UHUDChatInputWidget : public UInGameWidgetBase
{
	GENERATED_BODY()

public:

	UHUDChatInputWidget(const FObjectInitializer & ObjectInitializer);

	virtual bool SetupWidget(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController) override;

protected:

	/* What to put onto the start of the text when typing out a for these groups. This will not
	be sent with the message */
	UPROPERTY(EditAnywhere, Category = "RTS")
	TMap < EMessageRecipients, FString > MessageStarts;

	/* Max length of message excluding any MessageStarts */
	UPROPERTY(EditAnywhere, Category = "RTS", meta = (ClampMin = 0))
	int32 MaxMessageLength;

	virtual void SetupBindings();

	/* Text box to type message into. Interesting... in PIE ESC will lose focus on a
	UEditableTextBox but not a UEditableText */
	UPROPERTY(meta = (BindWidgetOptional))
	UEditableTextBox * Text_Input;

	/* Who will receive the message if sent */
	EMessageRecipients MessageRecipients;

	/* How many characters inside the input box cannot be deleted. This prevents the things
	like [TEAM ONLY]: part of the text never being deleted */
	int32 NumUndeletableCharacters;

	/* Removes focus from this widget and puts it back on game */
	void RemoveFocus();

	FString GetMessageToSend() const;

	bool SendMessage();

	UFUNCTION()
	void UIBinding_OnInputTextChanged(const FText & NewText);

	UFUNCTION()
	void UIBinding_OnInputTextCommitted(const FText & Text, ETextCommit::Type CommitMethod);

public:

	// Called when player wants to type in message
	void Open(EMessageRecipients InMessageRecipients);

	// Hide
	void Close();

	virtual bool RespondToEscapeRequest(URTSHUD * HUD) override;

	void SendChatMessageAndClose();
};


//####################################################
//----------------------------------------------------
//	Chat output
//----------------------------------------------------
//####################################################

UCLASS(Abstract)
class RTS_VER2_API UHUDChatOutputWidget : public UInGameWidgetBase
{
	GENERATED_BODY()

	// Name of widget to play message received anim
	static const FName MessageReceivedAnimName;

public:

	UHUDChatOutputWidget(const FObjectInitializer & ObjectInitializer);

	virtual bool SetupWidget(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController) override;

protected:

	virtual void NativeTick(const FGeometry & MyGeometry, float InDeltaTime) override;

	// Messages received
	UPROPERTY(meta = (BindWidgetOptional))
	UMultiLineEditableText * Text_Output;

	/* List of how long each message is. Used kind of like a linked list */
	UPROPERTY()
	TArray < int32 > MessageLengths;

	/* Index in MessageIndices that is considered the front of it cause using it like a
	linked list */
	int32 FrontOfMessageIndices;

	/* Get what is considered next in array MessageLengths */
	int32 GetNextArrayIndex(int32 CurrentIndex) const;

	/* Number of messages currently in chat log */
	int32 NumMessages;

	/* Animation that plays when a message is received. Will probably want to make widget hit
	test invis at start and hidden/collapsed at end */
	UPROPERTY()
	UWidgetAnimation * OnMessageReceivedAnim;

	/* If not using a widget anim, the amount of time after receiving a message before
	hiding widget */
	UPROPERTY(EditAnywhere, Category = "RTS", meta = (ClampMin = "0.01"))
	float ShowDuration;

	/* Relevant if:
	- using a widget anim for when message is received
	- the animation has some kind of start effect to it as opposed to just showing widget
	straight away e.g. it takes 1 second to fade in.
	The amount of time to reset anim to when a message is received while the anim is still
	playing, but never moves anim forward, will only move anim start time backwards */
	UPROPERTY(EditAnywhere, Category = "RTS", meta = (ClampMin = 0))
	float AdditionalMessageAnimStartTime;

	/* Max number of messages allowed in log before it will start deleting the oldest ones */
	UPROPERTY(EditAnywhere, Category = "RTS", meta = (ClampMin = 1, ClampMax = 500))
	int32 NumMaxMessages;

	/* What to put very first in the message */
	UPROPERTY(EditAnywhere, Category = "RTS")
	TMap < EMessageRecipients, FString > MessageStarts;

	/* What to put after the senders name but before what they typed. There's probably better
	names for these.

	So to clarify the whole message will be:
	MessageStarts + senders name + MessageMiddle + what they typed, with new lines added
	automatically */
	UPROPERTY(EditAnywhere, Category = "RTS")
	FString MessageMiddle;

	// Timer handle for hiding widget
	FTimerHandle TimerHandle_Hide;

	void HideWidget();

	void Delay(FTimerHandle & TimerHandle, void(UHUDChatOutputWidget::* Function)(), float Delay);

public:

	void OnChatMessageReceived(const FString & SendersName, const FString & Message,
		EMessageRecipients MessageType);
};


//####################################################
//----------------------------------------------------
//	Game output
//----------------------------------------------------
//####################################################

/**
 *	Attributes for a game message category
 */
USTRUCT()
struct FGameMessageAttributes
{
	GENERATED_BODY()

protected:

	/**
	 *	If not using a widget anim, the amount of time after receiving a message before hiding widget
	 */
	UPROPERTY(EditAnywhere, Category = "RTS", meta = (ClampMin = "0.01"))
	float ShowDuration;

	/**
	 *	Max number of messages that can be shown before old messages start getting deleted
	 *	as new ones arrive
	 */
	UPROPERTY(EditAnywhere, Category = "RTS", meta = (ClampMin = 1, ClampMax = 20))
	int32 MaxNumMessages;

	/** Whether new messages should appear above or below older messages */
	UPROPERTY(EditAnywhere, Category = "RTS")
	bool bNewMessagesAboveOlder;

	/* List of how long each message is. Used kind of like a linked list */
	UPROPERTY()
	TArray < int32 > MessageLengths;

	FTimerHandle TimerHandle_HidePanel;

	/* Index in MessageIndices that is considered the front of it cause using it like a
	linked list */
	int32 FrontOfMessageIndices;

	/* Number of messages currently in chat log */
	int32 NumMessages;

	/**
	 *	If you name a widget animation this then it will be played every time a message is received
	 */
	UPROPERTY(VisibleAnywhere)
	FName MessageReceivedAnimName;

	/** @See UHUDChatOutputWidget::AdditionalMessageAnimStartTime */
	UPROPERTY(EditAnywhere, Category = "RTS", meta = (ClampMin = 0))
	float AdditionalMessageAnimStartTime;

	/* Anim to play when message received. Whole panel animations may not suit this widget
	very well */
	UPROPERTY()
	UWidgetAnimation * MessageReceivedAnim;

	// Widget these attributes are a part of
	UHUDGameOutputWidget * OwningWidget;

	typedef void (UHUDGameOutputWidget:: *FunctionPtrType)(void);
	/* Pointer to function to call that hides a panel widget... more code I will look at later
	and go "what the heck does that do?" */
	FunctionPtrType HidePanelFunctionPtr;

	int32 GetNextArrayIndex(int32 CurrentIndex) const;
	int32 GetPreviousArrayIndex(int32 CurrentIndex) const;

public:

	FGameMessageAttributes()
	{
		ShowDuration = 5.f;
		MaxNumMessages = 3;
		bNewMessagesAboveOlder = false;
		AdditionalMessageAnimStartTime = 0.f;
	}

	void SetMessageReceivedAnimName(const FName & InName);
	void Init(UHUDGameOutputWidget * InWidget, FunctionPtrType InFunctionPtr);

	/* Update text widget with new message and show panel */
	void OnMessageReceived(const FString & Message, UPanelWidget * PanelWidget,
		UMultiLineEditableText * TextWidget);

	/* Remove text from widget and update attributes to reflect no text */
	void ClearText(UMultiLineEditableText * TextWidget);
};


/**
*	Widget that displays messages about the game such as "not enough resources" or "player defeated"
*/
UCLASS(Abstract)
class RTS_VER2_API UHUDGameOutputWidget : public UInGameWidgetBase
{
	GENERATED_BODY()

public:

	UHUDGameOutputWidget(const FObjectInitializer & ObjectInitializer);

	virtual bool SetupWidget(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController) override;

protected:

	/* Override to allow widget anims */
	virtual void NativeTick(const FGeometry & MyGeometry, float InDeltaTime) override;

	/* Attributes for warning type messages */
	UPROPERTY(EditAnywhere, Category = "RTS")
	FGameMessageAttributes WarningAttributes;

	/* Attributes for notification type messages */
	UPROPERTY(EditAnywhere, Category = "RTS")
	FGameMessageAttributes NotificationAttributes;

	/* Panel that should have a text widget as a child to display game warnings */
	UPROPERTY(meta = (BindWidgetOptional))
	UPanelWidget * Panel_Warnings;

	/* Panel that should have a text widget as a child to display game notifications */
	UPROPERTY(meta = (BindWidgetOptional))
	UPanelWidget * Panel_Notifications;

	/* Text widget to display warnings in. Should be child of Panel_Warnings */
	UPROPERTY(meta = (BindWidgetOptional))
	UMultiLineEditableText * Text_Warnings;

	/* Text widget to display notifications in. Should be child of Panel_Notifications */
	UPROPERTY(meta = (BindWidgetOptional))
	UMultiLineEditableText * Text_Notifications;

	// Set vis to hidden
	void HideWarningPanel();
	void HideNotificationPanel();

public:

	/* Called when a game message is received like "Not enough resources" */
	void OnGameWarningReceived(const FString & Message);

	/* Called when a game message is received like "Resources depleted" */
	void OnGameNotificationReceived(const FString & Message);

	void Delay(FTimerHandle & TimerHandle, void(UHUDGameOutputWidget::* Function)(), float Delay);
};


//===============================================================================================
//-----------------------------------------------------------------------------------------------
//	C&C like Persistent Panel
//-----------------------------------------------------------------------------------------------
//===============================================================================================

/** 
 *	Widget that is the panel that appears on the right in C&C 
 *	
 *	The panel may contain two types of widgets: 
 *	- tabs 
 *	- buttons to switch between the tabs 
 *	
 *	Example of the different tabs in RA2: 
 *	- buildings, defense buildings, infantry, vehicles 
 *	
 *	Of course you do not have to use multiple tabs. You can put everything in one tab if you choose. 
 *
 *	Each building/unit should have a variable that lets you choose the tab they appear in, or 
 *	you can assign the value "None" to exclude them from appearing in any tab at all. There is 
 *	also a variable something like HUDPersistentTabOrdering that defines the order they appear 
 *	in the tab (e.g. in C&C generally power plants appear first, while something like a super 
 *	weapon type building will usually appear last)
 */
UCLASS(Abstract)
class RTS_VER2_API UHUDPersistentPanel : public UInGameWidgetBase
{
	GENERATED_BODY()

public:

	UHUDPersistentPanel(const FObjectInitializer & ObjectInitializer);

	virtual bool SetupWidget(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController) override;

	/* Tick function called by HUD widget */
	void OnTick(float InDeltaTime);

protected:

	/* Tab to start on at beginning of match. Do not use "None" */
	UPROPERTY(EditAnywhere, Category = "RTS")
	EHUDPersistentTabType DefaultTab;

	/** 
	 *	Render opacity for buttons that are not active, usually because prereqs are not met 
	 *	or no building can produce it. 
	 *	This is only relevant if HUDOptions::PersistentTabButtonDisplayRule in ProjectSettings.h 
	 *	is set to NoShuffling_FadeUnavailables
	 */
	UPROPERTY(EditAnywhere, Category = "RTS")
	float UnclickableButtonOpacity;

	/** 
	 *	In regards to the buttons on each tab, whether to display the names of buttons or not
	 *	(like in C&C they will say "Power Plant" or "Sniper" for example). 
	 */
	UPROPERTY(EditAnywhere, Category = "RTS")
	bool bShowTextLabelsOnButtons;

	/* Widget switcher to switch between different tabs like buildings and infantry */
	UPROPERTY(meta = (BindWidgetOptional))
	UWidgetSwitcher * Switcher_Tabs;

	/* Holds different panel types. Indices do not corrispond to Switcher_Tabs children */
	UPROPERTY()
	TArray < UHUDPersistentTab * > Tabs;

	/* Maps tab type to the tab widget for that type. Using an array AND Tmap for fast 
	iteration and quick lookups */
	UPROPERTY()
	TMap < EHUDPersistentTabType, UHUDPersistentTab * > TabsTMap;

	/* The tab that is active in Switcher_Tabs */
	UHUDPersistentTab * ActiveTab;

public:

	void SwitchToTab(EHUDPersistentTabType TabType);

	/* Called when the local player finishes researching an upgrade */
	void OnUpgradeComplete(EUpgradeType UpgradeType);

	/* Functions called when a building is built/destroyed
	@param BuildingType - the type of building that was built
	@param bFirstOfItsType - whether this was the first building of its type for prerequisite
	purposes */
	void OnBuildingConstructed(EBuildingType BuildingType, bool bFirstOfItsType);
	void OnBuildingDestroyed(EBuildingType BuildingType, bool bLastOfItsType);

	//=========================================================================================
	//	Production queue functions
	//=========================================================================================

	/* When an item is added to the production queue but not started immediately */
	void OnItemAddedToProductionQueue(const FTrainingInfo & Item, const FProductionQueue & Queue, AActor * Producer);

	/* When an item is added to a production queue and is started immediately */
	void OnItemAddedAndProductionStarted(const FTrainingInfo & Item, const FProductionQueue & Queue, AActor * Producer);

	/* When the front of a production queue completes. Assumed next item starts immediately
	@param Item - item that was just produced
	@param Queue - production queue that just produced something
	@param Producer - actor that the queue is a part of */
	void OnProductionComplete(const FTrainingInfo & Item, const FProductionQueue & Queue, uint8 NumRemoved, AActor * Producer);

	/* When a building is added to a production queue and it uses BuildsInTab build method
	@param Item - item that is being produced
	@param InQueue - production queue that is producing item
	@param Producer - actor the queue belongs to */
	void OnBuildsInTabProductionStarted(const FTrainingInfo & Item, const FProductionQueue & InQueue, AActor * Producer);

	void OnBuildsInTabProductionComplete(const FTrainingInfo & Item);

	void OnBuildsInTabBuildingPlaced(const FTrainingInfo & Item);
};


/* Button to switch between persistent tabs */
UCLASS(meta = (DisplayName = "HUD Persistent Panel: Tab Switching Button"))
class RTS_VER2_API UHUDPersistentTabSwitchingButton : public UMyButton
{
	GENERATED_BODY()

public:

	UHUDPersistentTabSwitchingButton(const FObjectInitializer & ObjectInitializer);

	void Setup(UHUDPersistentPanel * InPersistentPanel, ARTSPlayerController * LocalPlayCon, 
		URTSGameInstance * InGameInstance);

protected:

	UPROPERTY()
	ARTSPlayerController * PC;

	/* Reference to persistent tab this is a part of */
	UPROPERTY()
	UHUDPersistentPanel * PersistentPanel;

	/* Tab this button opens */
	UPROPERTY(EditAnywhere, Category = "RTS")
	EHUDPersistentTabType TabType;

	void UIBinding_OnLeftMouseButtonPress();
	void UIBinding_OnLeftMouseButtonReleased();
	void UIBinding_OnRightMouseButtonPress();
	void UIBinding_OnRightMouseButtonReleased();

public:

	void OnClicked();

	/* Get the tab pressing this button should switch to */
	EHUDPersistentTabType GetTabType() const;

#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif
};


/* Struct that represents a button and stores whether it can be made activated or not */
USTRUCT()
struct FButtonClickabilityInfo
{
	GENERATED_BODY()

protected:

	const FContextButton * ButtonPtr;

	bool bArePrerequisitesMet;
	bool bHasQueueSupport;

public:

	// Constructors
	FButtonClickabilityInfo();
	explicit FButtonClickabilityInfo(const FContextButton * InButton);
	explicit FButtonClickabilityInfo(const FContextButton * InButton, bool InArePrereqsMet, bool bInQueueSupport);
	
	// Trivial Getters and setters

	const FContextButton * GetButtonPtr() const;
	bool ArePrerequisitesMet() const;
	bool HasQueueSupport() const;
	void SetArePrerequisitesMet(bool bNewValue);
	void SetHasQueueSupport(bool bNewValue);

	bool CanBeActive() const;
};


/* Contains state data that an active button needs */
USTRUCT()
struct FActiveButtonStateData
{
	GENERATED_BODY()

protected:

	/* Queue that this button should monitor for progress bar purposes */
	const FProductionQueue * Queue;

	/* Actor that owns the queue this button is linked to */
	UPROPERTY()
	AActor * QueueOwner;

	/* The number of items in the queue this button is linked to. Here to avoid string to int
	conversion by reading value from what is in text block */
	int32 NumItemsInQueue;

	/* Whether a persistent queue is producing something. Only relevant if button is for
	BuildBuilding */
	bool bIsAnotherButtonsPersistentQueueProducing;

	/* The reason the button cannot carry out what is requested. Button is never made hit test
	invis. Instead we allow user to click it and will show a message if the action cannot be
	carried out.

	Only some reasons will ever be assigned to this - those that are event driven. Currently
	the only valid reason this variable can get is that another building is producing */
	EGameWarning CannotFunctionReason;

public:

	FActiveButtonStateData();

	// Trivial getters and setters 

	const FProductionQueue * GetQueue() const;
	AActor * GetQueueOwner() const;
	int32 GetNumItemsInQueue() const;
	bool IsAnotherButtonsPersistentQueueProducing() const;
	EGameWarning GetCannotFunctionReason() const;

	void SetQueue(const FProductionQueue * InQueue);
	void SetQueueOwner(AActor * InQueueOwner);
	void SetNumItemsInQueue(int32 NewValue);
	void SetIsAnotherButtonsPersistentQueueProducing(bool bNewValue);
	void SetCannotFunctionReason(EGameWarning InReason);

	FString ToString() const;
};


/** 
 *	A single tab for a single category. 
 *	
 *	Add as many UHUDPersistentTabButton to it as you need 
 */
UCLASS(Abstract)
class RTS_VER2_API UHUDPersistentTab : public UInGameWidgetBase
{
	GENERATED_BODY()

public:

	virtual bool SetupWidget(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController) override;

	void OnTick(float InDeltaTime);

protected:

	/* Type this is for. Do not use "None" */
	UPROPERTY(EditAnywhere, Category = "RTS")
	EHUDPersistentTabType TabCategory;

	/* Array of buttons. These buttons are never added/removed from any panel. To emulate 
	shifting a button we basically just update all the buttons that would be affected */
	UPROPERTY()
	TArray < UHUDPersistentTabButton * > ButtonWidgets;

	/* This array closly mirrors ButtonWidgets. It update is updated right before updating 
	the button widgets. It exists for performance only */
	TArray < const FContextButton * > ButtonIndices;

	/* Maps button type to its state. This container contains all the state info for a button */
	TMap < const FContextButton *, FActiveButtonStateData > ButtonStates;

	/* Array of buttons that are inactive. Depending on the method used these buttons will 
	either be completely hidden (and therefore unclickable) or they will have reduced 
	opacity and will still be clickable but will show a warning when the player tries to 
	click them */
	TArray < FButtonClickabilityInfo > UnactiveButtons;

	/* The number of buttons that are "active" in the tab. For the two 
	EHUDPersistentTabButtonDisplayRule NoShuffling methods this isn't really needed but I may 
	choose to keep it updated for no reason whatsoever */
	//int32 NumActiveButtons;

	/* Maps button type to the persistent tab button */
	UPROPERTY()
	TMap < FTrainingInfo, UHUDPersistentTabButton * > TypeMap;

	float UnclickableButtonOpacity;

	bool bShowTextLabelsOnButtons;

	int32 GetNumActiveButtons() const;

public:

	EHUDPersistentTabType GetType() const;

	/** 
	 *	Functions called when a building is built/destroyed
	 *	
	 *	@param BuildingType - the type of building that was built
	 *	@param bFirstOfItsType - whether this was the first building of its type 
	 */
	void OnBuildingConstructed(EBuildingType BuildingType, bool bFirstOfItsType);
	void OnBuildingDestroyed(EBuildingType BuildingType, bool bLastOfItsType);

	void SetUnclickableButtonOpacity(float InOpacity);

	void SetShowTextLabelsOnButtons(bool bNewValue);

	//===========================================================================================
	//	Production queue functions
	//===========================================================================================

	/* When an item is added to the production queue but not started immediately */
	void OnItemAddedToProductionQueue(const FTrainingInfo & Item, const FProductionQueue & Queue, AActor * Producer);

	/* When an item is added to a production queue and is started immediately */
	void OnItemAddedAndProductionStarted(const FTrainingInfo & Item, const FProductionQueue & Queue, AActor * Producer);

	/* When the front of a production queue completes. Assumed next item starts immediately
	@param Item - item that was just produced
	@param Queue - production queue that just produced something
	@param NumRemoved - number of items removed from the queue
	@param Producer - actor that the queue is a part of */
	void OnProductionComplete(const FTrainingInfo & Item, const FProductionQueue & Queue, uint8 NumRemoved, AActor * Producer);

	/* When a building is added to a production queue and it uses BuildsInTab build method
	@param Item - item that is being produced
	@param InQueue - production queue that is producing item
	@param Producer - actor the queue belongs to */
	void OnBuildsInTabProductionStarted(const FTrainingInfo & Item, const FProductionQueue & InQueue, AActor * Producer);

	/* Call when a building that is using BuildsInTab build method becomes ready to place */
	void OnBuildsInTabProductionComplete(const FTrainingInfo & Item);

	void OnBuildsInTabBuildingPlaced(const FTrainingInfo & Item);

	/* For sorting UHUDPersistentPanel::Tabs */
	friend bool operator<(const UHUDPersistentTab & Widget1, const UHUDPersistentTab & Widget2);
};


/* Button on a persistent tab. Very similar to UContextActionButton and may eventually become a
child/parent of it or share the same parent. */
UCLASS(meta = (DisplayName = "HUD Persistent Tab: Button"))
class RTS_VER2_API UHUDPersistentTabButton : public UMyButton
{
	GENERATED_BODY()

public:

	UHUDPersistentTabButton(const FObjectInitializer & ObjectInitializer);

	void SetupButton(UHUDPersistentTab * InOwningTab, URTSGameInstance * InGameInstance,
		ARTSPlayerController * InPlayerController, ARTSPlayerState * InPlayerState,
		float InUnclickableAlpha);

	/* This is for the 2 NoShuffling methods only. Link a button type with this widget */
	void SetPermanentButtonType(const FContextButton * InButton, bool bShowTextLabel);
	/* Set pointer to state info */
	void SetPeramentStateInfo(FActiveButtonStateData * InStateInfo);

	/* This version is intended to be called only for the NoShuffling methods */
	void MakeActive();

	/* This version is intended to be called for the non-NoShuffling methods */
	void MakeActive(const FContextButton * InButton, FActiveButtonStateData * InStateInfo,
		bool bShowLabelText, FUnifiedImageAndSoundFlags UnifiedAssetFlags);

	void MakeUnactive();

#if WITH_EDITOR
	virtual void OnCreationFromPalette() override;
#endif

	void OnTick(float InDeltaTime);

protected:

	UPROPERTY()
	UProgressBar * ProductionProgressBar;

	/* The text that shows on the button describing what the button is for */
	UPROPERTY()
	UMultiLineEditableText * DescriptionText;

	/* More text. This text will show something like "Ready" when a building finishes production
	using the BuildsInTab build method letting the user know it is ready to place, or could show
	the number of units queued for other Train button types */
	UPROPERTY()
	UMultiLineEditableText * ExtraText;

	/* Reference to tab that owns this button */
	UPROPERTY()
	UHUDPersistentTab * OwningTab;

	/* Reference to game instance */
	UPROPERTY()
	URTSGameInstance * GI;

	/* Reference to player controller. Doubling up on PC because SMyButton has one too */
	UPROPERTY()
	ARTSPlayerController * PC;

	/* Reference to player state */
	UPROPERTY()
	ARTSPlayerState * PS;

	/* Faction info of player state */
	UPROPERTY()
	AFactionInfo * FI;

	/* Button type this button represents */
	const FContextButton * Button;

	/* Pointer to info struct that holds all the state for this button */
	FActiveButtonStateData * StateInfo;

	float OriginalAlpha;
	float UnclickableAlpha;

	void OnAnotherPersistentQueueProductionStarted();
	void OnAnotherPersistentQueueProductionFinished();

	/* Get the text that should be displayed on ExtraText that is ment to show how many of the 
	item are queued for production. */
	FText GetQuantityText() const;

	void UIBinding_OnLMBPress();
	void UIBinding_OnLMBReleased();
	void UIBinding_OnRMBPress();
	void UIBinding_OnRMBReleased();

public:

	/* Get the context button this widget represents */
	const FContextButton * GetButtonType() const;

	FActiveButtonStateData * GetStateInfo() const;

	//===========================================================================================
	//	Production queue functions
	//===========================================================================================

	/* Check if button */
	bool IsProducingSomething() const;

	/* Only for build method BuildsInTab. Check if production has completed */
	bool HasProductionCompleted() const;

	/* When an item is added to the production queue but not started immediately. Only called by
	owning tab on button it affects */
	void OnItemAddedToProductionQueue(const FTrainingInfo & Item, const FProductionQueue & InQueue, AActor * Producer);

	/* When an item is added to a production queue and is started immediately. Only called by
	owning tab on button it affects */
	void OnItemAddedAndProductionStarted(const FTrainingInfo & Item, const FProductionQueue & InQueue, AActor * Producer, bool bIsForThisButton);

	/* When the front of a production queue completes. Assumed next item starts immediately. Only
	called by owning tab on button it affects
	@param Item - the item that was just produced
	@param Queue - production queue that just produced something
	@param NumRemoved - number of items removed from the queue
	@param Producer - actor that the queue is a part of */
	void OnProductionComplete(const FTrainingInfo & Item, const FProductionQueue & InQueue, uint8 NumRemoved, AActor * Producer);

	/* When a building is added to a production queue and it uses BuildsInTab build method
	@param Item - item that is being produced
	@param InQueue - production queue that is producing item
	@param Producer - actor the queue belongs to */
	void OnBuildsInTabProductionStarted(const FTrainingInfo & Item, const FProductionQueue & InQueue, AActor * Producer);

	/* Call when a building that is using BuildsInTab build method becomes ready to place. Label
	the button as 'ready to place' */
	void OnBuildsInTabProductionComplete(const FTrainingInfo & Item);

	void OnBuildsInTabBuildingPlaced(const FTrainingInfo & Item);

#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif
};


//####################################################
//----------------------------------------------------
//	Minimap
//----------------------------------------------------
//####################################################

/* Widget for the minimap */
UCLASS(Abstract)
class RTS_VER2_API UMinimap : public UInGameWidgetBase
{
	GENERATED_BODY()

public:

	UMinimap(const FObjectInitializer & ObjectInitializer);

	virtual bool SetupWidget(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController) override;

	void Setup(ARTSLevelVolume * BackgroundTexture);

protected:

	virtual int32 NativePaint(const FPaintArgs & Args, const FGeometry & AllottedGeometry, const FSlateRect & MyCullingRect,
		FSlateWindowElementList & OutDrawElements, int32 LayerId, const FWidgetStyle & InWidgetStyle,
		bool bParentEnabled) const override;

	/* Color to draw selectables */
	UPROPERTY(EditAnywhere, EditFixedSize, Category = "RTS")
	TMap < EAffiliation, FLinearColor > SelectableColors;

	/* Image of minimap. Set image is landscape and extra parts like selectables and fog of war
	are drawn overtop */
	UPROPERTY(meta = (BindWidgetOptional))
	UImage * Image_Minimap;

	/* 2D coords for center of map, but flipped so X is Y and Y is X */
	FVector2D MapCenter;

	/* 1.f / MapDimensions.  MapDimesnions = length and width of map in unreal units */
	FVector2D MapDimensionsInverse;

	/* Dimensions of the minimap image in pixels */
	FIntPoint ImageSizeInPixels;

	/* This needs to be updated whenever resolution changes */
	FIntPoint ViewportSizeInPixels;

	/* Draw selectables on minimap */
	void DrawSelectables(FPaintContext & PaintContext) const;

	/* Draw fog of war on minimap */
	void DrawFogOfWar(FPaintContext & InContext) const;

public:

	/* Returns the pixel on minimap some world coordinates corrispond to
	@param WorldLocation - some world coords to be converted to minimap pixel coords
	@param Geometry - Image_Minimap geometry
	@return - minimap pixel coords */
	FVector2D WorldCoordsToMinimapCoords(const FVector & WorldLocation,
		const FGeometry & Geometry) const;

	static FVector2D MinimapCoordsToWorldCoords(const FIntPoint InMinimapCoords);
};



//=============================================================================================
//---------------------------------------------------------------------------------------------
//	Tooltip widgets
//---------------------------------------------------------------------------------------------
//=============================================================================================

/* Tooltip widget for an inventory item.

Some notes:
- Mess around with each individual widget's anchors if you can't see them 
- You will likely want to enable text wrapping for the text blocks */
UCLASS(Abstract, meta = (DisableNativeTick))
class RTS_VER2_API UTooltipWidget_InventoryItem : public UInGameWidgetBase
{
	GENERATED_BODY()

public:

	UTooltipWidget_InventoryItem(const FObjectInitializer & ObjectInitializer);

	void InitialSetup();

protected:

	/* The inventory item actor this widget is displaying info for. If this widget is not 
	displaying info for an inventory item actor but instead something else like say an 
	inventory slot then this will be null */
	const AInventoryItem * DisplayedInventoryItem;

	// Pointer to the inventory iteminfo  this widget is displaying
	const FInventoryItemInfo * DisplayedItemInfo;

	// Number in stack or num charges this widget is displaying. -1 means blank text
	int16 DisplayedNumInStackOrNumCharges;

	/* Text that shows the name of the item */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_Name;

	/* Text to show the description of the item */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_Description;

	/* Text to show the stat changes from the item */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_StatChanges;

	/* Flavor text */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_Flavor;

	/* Icon of item */
	UPROPERTY(meta = (BindWidgetOptional))
	UImage * Image_Image;

	/* Text to show either the number in the stack or number of charges */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_QuantityOrNumCharges;

	/**
	 *	Setup the widgets to show info about an item. Can be in an inventory slot, shop slot
	 *	or an item in the world or more.
	 *
	 *	@param StackQuantityOrNumCharges - pass in -1 to say 'do not show'
	 */
	void SetupForInner(int16 InStackQuantityOrNumCharges, const FInventoryItemInfo & ItemsInfo);

public:

	/* Version for inventory item actors */
	void SetupFor(const AInventoryItem * InventoryItemActor);
	/* Version for inventory slots */
	void SetupFor(const FInventorySlotState & InvSlot, const FInventoryItemInfo & ItemsInfo);
	/* Version for shop slots */
	void SetupFor(const FItemOnDisplayInShopSlot & ShopSlot, const FInventoryItemInfo & ItemsInfo);

	/* Return whether this widget contains the info for a particular inventory item */
	bool IsSetupFor(AInventoryItem * InventoryItem) const;
};


/* Tooltip widget for a selectable's action bar */
UCLASS(Abstract, meta = (DisableNativeTick))
class RTS_VER2_API UTooltipWidget_Ability : public UInGameWidgetBase
{
	GENERATED_BODY()

protected:

	/* The button that this widget is displaying info for */
	const FContextButtonInfo * DisplayedButtonType;

	/* Text to show the name of the ability */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_Name;

	/* Text to show the description of the ability */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_Description;

public:

	/* Sets up the appearance of the widget for a button type */
	void SetupFor(const FContextButtonInfo & InButtonInfo);
};


/** 
 *	Tooltip widget for a buildings/units/upgrades. 
 *	
 *	Has 5 different text fields for: 
 *	- Name 
 *	- Description 
 *	- Cost (TODO) 
 *	- Build time 
 *	- Prerequisites 
 */
UCLASS(Abstract, meta = (DisableNativeTick))
class RTS_VER2_API UTooltipWidget_Production : public UInGameWidgetBase
{
	GENERATED_BODY()

protected:

	/* Info struct for what this widget is displaying info for */
	const FDisplayInfoBase * DisplayedInfo;

	/* Text to show the name of the thing */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_Name;

	/* Text to show the description of the thing */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_Description;

	/* Text to show the build time for the thing */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_BuildTime;

	/* Text to show the prerequisites for the thing. Just shows all prerequisites and does not 
	differentiate between those already obtained or not */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_Prerequisites;

	/* Convert production time as a float to text */
	static FText GetProductionTimeText(float ProductionTime);

public:

	void SetupFor(const FDisplayInfoBase & InInfo);
};


/** Tooltip for a button on the global skills panel */
UCLASS(Abstract, meta = (DisableNativeTick))
class RTS_VER2_API UTooltipWidget_GlobalSkillsPanelButton : public UInGameWidgetBase
{
	GENERATED_BODY()

public:

	void SetupFor(const FAquiredCommanderAbilityState * AbilityState, bool bForceUpdate);

	/* Called when a new rank of an ability is aquired and this tooltip 
	is showing and the info it is showing is for that ability */
	void OnNewRankAquired();

	const FAquiredCommanderAbilityState * GetDisplayedInfo() const;

protected:

	//--------------------------------------------------
	//	Data
	//--------------------------------------------------

	const FAquiredCommanderAbilityState * DisplayedAbilityState;
	const FCommanderAbilityInfo * DisplayedAbilityInfoStruct;

	/** Text to show the name of the ability */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_Name;

	/* Text to show the description of the ability */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_Description;
};


/* Tooltip widget for a node in the commander's skill tree */
UCLASS(Abstract, meta = (DisableNativeTick))
class RTS_VER2_API UTooltipWidget_CommanderSkillTreeNode : public UInGameWidgetBase
{
	GENERATED_BODY()

public:

	UTooltipWidget_CommanderSkillTreeNode(const FObjectInitializer & ObjectInitializer);

	/* @param PlayerState - player state of the local player */
	void InitialSetup(ARTSPlayerState * PlayerState);

	void SetupFor(const FCommanderAbilityTreeNodeInfo * NodeInfo, bool bForceUpdate);

	/* Called when the tooltip widget is showing */
	void OnNewRankAquired(const FCommanderAbilityTreeNodeInfo * NodeInfo);

	/* Get the node this tooltip is displaying info for */
	const FCommanderAbilityTreeNodeInfo * GetDisplayedNodeInfo() const;

protected:

	/* Get the text that should go in the missing requirements text block */
	static FText GetMissingRequirementsText(const FCommanderAbilityTreeNodeInfo * NodeInfo, 
		const FCommanderAbilityTreeNodeSingleRankInfo * AbilityInfo);

	//--------------------------------------------------
	//	Data
	//--------------------------------------------------

	const FCommanderAbilityTreeNodeInfo * DisplayedNodeInfo;

	//-----------------------------------------------------------------------
	//	Widgets to show info for the current aquired rank of the ability 
	//-----------------------------------------------------------------------

	/* Panel widget to put all the text and image widget's for the current aquried rank on. 
	If you use any of the Aquired widgets then they will have to be children of this */
	UPROPERTY(meta = (BindWidgetOptional))
	UPanelWidget * Panel_AquiredRank;

	/** Name of the ability */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_AquiredRank_Name;

	/** Image for ability */
	UPROPERTY(meta = (BindWidgetOptional))
	UImage * Image_AquiredRank_Image;

	/** Text to show the description */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_AquiredRank_Description;

	//-----------------------------------------------------------------------
	//	Widgets to show info for the next rank of the ability if one exists
	//-----------------------------------------------------------------------
	
	/* Panel widget to put all the text and image widget's for the next rank on.
	If you use any of the Next widgets then they will have to be children of this */
	UPROPERTY(meta = (BindWidgetOptional))
	UPanelWidget * Panel_NextRank;

	/** Cost of next rank of ability */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_NextRank_Cost;

	/**
	 *	This is the color to color the next rank skill point cost text when the player does not have
	 *	enough skill points to purchase the skill.
	 */
	UPROPERTY(EditAnywhere, Category = "RTS")
	FSlateColor CannotAffordCostTextColor;

	FSlateColor CostTextOriginalColor;

	/* Text that shows missing requirements of next rank of ability */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_NextRank_MissingRequirements;

	/** Name of the next rank of the ability */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_NextRank_Name;

	/** Image for next rank of the ability */
	UPROPERTY(meta = (BindWidgetOptional))
	UImage * Image_NextRank_Image;

	/** Text to show the description for the next rank of the ability */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_NextRank_Description;
};
