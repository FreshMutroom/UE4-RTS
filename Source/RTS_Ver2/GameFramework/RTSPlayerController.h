// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Components/SlateWrapperTypes.h" // For ESlateVisibility

#include "Statics/Structs_1.h"
#include "Statics/OtherEnums.h"
#include "Settings/ProjectSettings.h"
#include "Statics/Structs/Structs_8.h"
#include "RTSPlayerController.generated.h"

class USpringArmComponent;
class AFogOfWarManager;
class ARTSGameState;
class AObjectPoolingManager;
class AGhostBuilding;
class UInGameWidgetBase;
class URTSHUD;
class AMarqueeHUD;
class ARTSPlayerState;
class URTSGameInstance;
class AFactionInfo;
class UDecalComponent;
class UParticleSystem;
class UParticleSystemComponent;
class ARTSLevelVolume;
class ISelectable;
class APlayerCamera;
class UInMatchDeveloperWidget;
class AInventoryItem;
class AInfantry;
class UMyButton;
class SMyButton;
class UHUDPersistentTabButton;
class UContextActionButton;
class UInventoryItemButton;
class UWidget;
class UItemOnDisplayInShopButton;
class UProductionQueueButton;
class UHUDPersistentTabSwitchingButton;
class UCommanderSkillTreeNodeWidget;
class UGlobalSkillsPanelButton;
class UPauseMenu;
class UInMatchConfirmationWidget;
class USingleKeyBindingWidget;
enum class EKeyMappingAction : uint16;
enum class EKeyMappingAxis : uint16;
class UKeyBindingsWidget;
struct FKeyWithModifiers;
class URebindingCollisionWidget;
enum class EKeyModifiers : uint8;
class USingleAudioClassWidget;
class USingleBoolControlSettingWidget;
class USingleFloatControlSettingWidget;
class USettingsWidget;
class USettingsConfirmationWidget_Exit;
class USettingsConfirmationWidget_ResetToDefaults;
class USingleVideoSettingWidget;
class UAudioSettingsWidget;
class UGarrisonInfo;
class UGarrisonedUnitInfo;


//=============================================================================================
//	Enums
//=============================================================================================

UENUM()
enum class EPlayerType : uint8
{
	/* Has not been specified yet */
	Unknown,

	/* A player participating in a match */
	Player,

	/* A player watching the match */
	Observer
};


/* Direction of rotation. Used when rotating ghost */
UENUM()
enum class ERotationDirection : uint8
{
	NoDirectionEstablished,
	Clockwise,
	CounterClockwise
};


//=============================================================================================
//	Structs
//=============================================================================================

/* Array that holds what selectables are in a ctrl group */
USTRUCT()
struct FCtrlGroupList
{
	GENERATED_BODY()

protected:

	/* Holds the selectable in the ctrl group. Not quite there yet in using selectable IDs 
	because it is hard to know if an actor was destroyed, then their ID was assigned to a 
	newly built selectable and now that selectable will be in the ctrl group even though it 
	shouldn't be */
	UPROPERTY()
	TArray < AActor * > List;

public:

	TArray < AActor * > & GetArray();

	// Get number of selectables in control group
	int32 GetNum() const;
};


/* The result of trying to change a key binding */
struct FTryBindActionResult
{
	explicit FTryBindActionResult(const FKeyWithModifiers & InKey);

	FKeyWithModifiers KeyWeAreTryingToAssign;

	/* If this != None then an axis is already bound to key. Also if binding was successful then 
	the axis that gets unbinded can be stored here, or it will be None if no axis was unbinded. 
	This will be None if we're just changing the key for the same axis e.g. MoveCameraForward 
	is changing from W to T and there's nothing becoming unbound */
	EKeyMappingAxis AlreadyBoundToKey_Axis;
	/* Same as above except for action mappings instead. If it was an axis binding we 
	were trying to bind then it will unmap every modifier combination with that key so there 
	can be up to 8 actions unmapped from a rebinding */
	EKeyMappingAction AlreadyBoundToKey_Action[8];
	/* If this != None then something else was the problem */
	EGameWarning Warning;
};


//=============================================================================================
//	RTSPlayerController implementation
//=============================================================================================

/**
*	All input handled through this class, well at least non-widget input. Both players and obsevers
*	use this class
*/
UCLASS()
class RTS_VER2_API ARTSPlayerController : public APlayerController
{
	GENERATED_BODY()

public:

	ARTSPlayerController();

protected:

	virtual void SetPlayer(UPlayer * InPlayer) override;

	virtual void BeginPlay() override;

public:

	virtual void Tick(float DeltaTime) override;

private:

	virtual void SetupInputComponent() override;

	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const override;

	/* Reference to ViewTarget's sprint arm */
	UPROPERTY()
	USpringArmComponent * SpringArm;

	/* Team this player is on in FName form */
	FName TeamTag;

	/* Team this player is on */
	ETeam Team;

	/* What type of player this is; either player participating in match or an observer */
	EPlayerType Type;

	/* Particle system that will be drawn on world when right clicking on world or selectable.
	Use UStatics::CommandTargetTypeToArrayIndex to get correct array index */
	UPROPERTY()
	TArray < UParticleSystem * > RightClickParticles;

	/* The last spawned right click particle so it can be hidden when a new right click happens
	because we only show one max */
	UPROPERTY()
	UParticleSystemComponent * PreviousRightClickParticle;

	// A reference to owning game instance
	UPROPERTY()
	URTSGameInstance * GI;

	// A reference to owning player state
	UPROPERTY()
	ARTSPlayerState * PS;

	/* Reference to game state */
	UPROPERTY()
	ARTSGameState * GS;

	/* Reference to object pooling manager for handling creating projectiles.
	One for each client */
	UPROPERTY()
	AObjectPoolingManager * PoolingManager;

	// A reference to the HUD widget
	UPROPERTY()
	URTSHUD * HUD;

	/** Widget the mouse is hovered over */
	UMyButton * HoveredUIElement;

	/** The button on the UI that is 'highlighted'. Null if no button is highlighted */
	UMyButton * HighlightedButton;

#if !INSTANT_SHOWING_UI_ELEMENT_TOOLTIPS
	
	UInGameWidgetBase * HoveredUserWidget;

	/* Time spent hovering a UI element */
	float AccumulatedTimeSpentHoveringUIElement;

#if USING_UI_ELEMENT_HOVER_TIME_DECAY_DELAY
	/* Time spent not hovering a UI element */
	float TimeSpentNotHoveringUIElement;
#endif

	/* Whether a tooltip for a UI element is showing */
	uint8 bShowingUIElementTooltip : 1;

#endif // !INSTANT_SHOWING_UI_ELEMENT_TOOLTIPS 

#if !INSTANT_SHOWING_SELECTABLE_TOOLTIPS

	uint8 bShowingSelectablesTooltip : 1;

	/* Time spent hovering a selectable */
	float AccumulatedTimeSpentHoveringSelectable;

#if USING_SELECTABLE_HOVER_TIME_DECAY_DELAY
	/* Time spent not hovering a selectable */
	float TimeSpentNotHoveringSelectable;
#endif

#endif // !INSTANT_SHOWING_SELECTABLE_TOOLTIPS

	/* A reference to the marquee box drawing HUD */
	UPROPERTY()
	AMarqueeHUD * MarqueeHUD;

	/* A reference to the faction info for the faction this player
	is currently controlling. Is just used for visual things currently
	like appearance of ghost building. */
	UPROPERTY()
	AFactionInfo * FactionInfo;

	/* Faction this player controller is commanding */
	EFaction Faction;

	/* Pool of ghost buildings for our faction. To avoid spawn actor during match */
	UPROPERTY()
	TMap < EBuildingType, AGhostBuilding * > GhostPool;

public:

	//-----------------------------------------------------------------------------------------
	//	Input callbacks for UI elements receiving mouse events
	//	
	//	Just an FYI: the UI elements have their mouse events called before the PC's input 
	//	functions setup during SetupInputComponent.
	//------------------------------------------------------------------------------------------

	// Mouse events for trying to build something from HUD persistet panel
	void OnLMBPressed_HUDPersistentPanel_Build(UHUDPersistentTabButton * ButtonWidget);
	void OnLMBReleased_HUDPersistentPanel_Build(UHUDPersistentTabButton * ButtonWidget);
	void OnRMBPressed_HUDPersistentPanel_Build(UHUDPersistentTabButton * ButtonWidget);
	void OnRMBReleased_HUDPersistentPanel_Build(UHUDPersistentTabButton * ButtonWidget);

	// Mouse events for the buttons on the HUD persistent panel to switch between tabs
	void OnLMBPressed_HUDPersistentPanel_SwitchTab(UHUDPersistentTabSwitchingButton * ButtonWidget);
	void OnLMBReleased_HUDPersistentPanel_SwitchTab(UHUDPersistentTabSwitchingButton * ButtonWidget);
	void OnRMBPressed_HUDPersistentPanel_SwitchTab(UHUDPersistentTabSwitchingButton * ButtonWidget);
	void OnRMBReleased_HUDPersistentPanel_SwitchTab(UHUDPersistentTabSwitchingButton * ButtonWidget);

	/* Mouse events for primary selected's action bar */
	void OnLMBPressed_PrimarySelected_ActionBar(UContextActionButton * ButtonWidget);
	void OnLMBReleased_PrimarySelected_ActionBar(UContextActionButton * ButtonWidget);
	void OnRMBPressed_PrimarySelected_ActionBar(UContextActionButton * ButtonWidget);
	void OnRMBReleased_PrimarySelected_ActionBar(UContextActionButton * ButtonWidget);

	/*  Mouse events for inventory of primary selected */
	void OnLMBPressed_PrimarySelected_InventoryButton(UMyButton * ButtonWidget);
	void OnLMBReleased_PrimarySelected_InventoryButton(UMyButton * ButtonWidget, UInventoryItemButton * ButtonUserWidget);
	void OnRMBPressed_PrimarySelected_InventoryButton(UMyButton * ButtonWidget);
	void OnRMBReleased_PrimarySelected_InventoryButton(UMyButton * ButtonWidget, UInventoryItemButton * ButtonUserWidget);

	/* Mouse events for items in shop */
	void OnLMBPressed_PrimarySelected_ShopButton(UMyButton * ButtonWidget);
	void OnLMBReleased_PrimarySelected_ShopButton(UMyButton * ButtonWidget, UItemOnDisplayInShopButton * ButtonUserWidget);
	void OnRMBPressed_PrimarySelected_ShopButton(UMyButton * ButtonWidget);
	void OnRMBReleased_PrimarySelected_ShopButton(UMyButton * ButtonWidget, UItemOnDisplayInShopButton * ButtonUserWidget);

	/* Mouse events for a selectable's production queue */
	void OnLMBPressed_PrimarySelected_ProductionQueueSlot(UProductionQueueButton * ButtonWidget);
	void OnLMBReleased_PrimarySelected_ProductionQueueSlot(UProductionQueueButton * ButtonWidget);
	void OnRMBPressed_PrimarySelected_ProductionQueueSlot(UProductionQueueButton * ButtonWidget);
	void OnRMBReleased_PrimarySelected_ProductionQueueSlot(UProductionQueueButton * ButtonWidget);

	/* Pause game button */
	void OnLMBPressed_PauseGame(UMyButton * ButtonWidget);
	void OnLMBReleased_PauseGame(UMyButton * ButtonWidget);
	void OnRMBPressed_PauseGame(UMyButton * ButtonWidget);
	void OnRMBReleased_PauseGame(UMyButton * ButtonWidget);

	/* Button that shows/hides commander skill tree */
	void OnLMBPressed_CommanderSkillTreeShowButton(UMyButton * ButtonWidget);
	void OnLMBReleased_CommanderSkillTreeShowButton(UMyButton * ButtonWidget);
	void OnRMBPressed_CommanderSkillTreeShowButton(UMyButton * ButtonWidget);
	void OnRMBReleased_CommanderSkillTreeShowButton(UMyButton * ButtonWidget);

	/* Buttons on the global skills panel */
	void OnLMBPressed_GlobalSkillsPanelButton(UMyButton * ButtonWidget);
	void OnLMBReleased_GlobalSkillsPanelButton(UMyButton * ButtonWidget, UGlobalSkillsPanelButton * UserWidget);
	void OnRMBPressed_GlobalSkillsPanelButton(UMyButton * ButtonWidget);
	void OnRMBReleased_GlobalSkillsPanelButton(UMyButton * ButtonWidget, UGlobalSkillsPanelButton * UserWidget);

	/* Buttons on the player targeting panel */
	void OnLMBPressed_PlayerTargetingPanelButton(UMyButton * ButtonWidget);
	void OnLMBReleased_PlayerTargetingPanelButton(UMyButton * ButtonWidget, ARTSPlayerState * AbilityTarget);
	void OnRMBPressed_PlayerTargetingPanelButton(UMyButton * ButtonWidget);
	void OnRMBReleased_PlayerTargetingPanelButton(UMyButton * ButtonWidget);

	/* Button that hides the player targeting panel effectively cancelling the ability */
	void OnLMBPressed_HidePlayerTargetingPanel(UMyButton * ButtonWidget);
	void OnLMBReleased_HidePlayerTargetingPanel(UMyButton * ButtonWidget);
	void OnRMBPressed_HidePlayerTargetingPanel(UMyButton * ButtonWidget);
	void OnRMBReleased_HidePlayerTargetingPanel(UMyButton * ButtonWidget);

	/* Buttons for abilities on the commander's skill tree */
	void OnLMBPressed_CommanderSkillTreeNode(UCommanderSkillTreeNodeWidget * ButtonWidget);
	void OnLMBReleased_CommanderSkillTreeNode(UCommanderSkillTreeNodeWidget * ButtonWidget);
	void OnRMBPressed_CommanderSkillTreeNode(UCommanderSkillTreeNodeWidget * ButtonWidget);
	void OnRMBReleased_CommanderSkillTreeNode(UCommanderSkillTreeNodeWidget * ButtonWidget);

	/* Button to unload a single unit in a garrison */
	void OnLMBPressed_UnloadSingleGarrisonUnit(UMyButton * ButtonWidget);
	void OnLMBReleased_UnloadSingleGarrisonUnit(UMyButton * ButtonWidget, UGarrisonedUnitInfo * SingleGarrisonInfoWidget);
	void OnRMBPressed_UnloadSingleGarrisonUnit(UMyButton * ButtonWidget);
	void OnRMBReleased_UnloadSingleGarrisonUnit(UMyButton * ButtonWidget);

	/* Button for unload all units in garrison */
	void OnLMBPressed_UnloadGarrisonButton(UMyButton * ButtonWidget);
	void OnLMBReleased_UnloadGarrisonButton(UMyButton * ButtonWidget, UGarrisonInfo * GarrisonInfoWidget);
	void OnRMBPressed_UnloadGarrisonButton(UMyButton * ButtonWidget);
	void OnRMBReleased_UnloadGarrisonButton(UMyButton * ButtonWidget);

	/* "Resume play" button on pause menu */
	void OnLMBPressed_PauseMenu_Resume(UMyButton * ButtonWidget);
	void OnLMBReleased_PauseMenu_Resume(UMyButton * ButtonWidget);
	void OnRMBPressed_PauseMenu_Resume(UMyButton * ButtonWidget);
	void OnRMBReleased_PauseMenu_Resume(UMyButton * ButtonWidget);

	/* Show settings menu button on pause menu */
	void OnLMBPressed_PauseMenu_ShowSettingsMenu(UMyButton * ButtonWidget);
	void OnLMBReleased_PauseMenu_ShowSettingsMenu(UMyButton * ButtonWidget, UPauseMenu * PauseMenuWidget);
	void OnRMBPressed_PauseMenu_ShowSettingsMenu(UMyButton * ButtonWidget);
	void OnRMBReleased_PauseMenu_ShowSettingsMenu(UMyButton * ButtonWidget);

	/* "Return to main menu" button on pause menu */
	void OnLMBPressed_PauseMenu_ReturnToMainMenu(UMyButton * ButtonWidget);
	void OnLMBReleased_PauseMenu_ReturnToMainMenu(UMyButton * ButtonWidget, UPauseMenu * PauseMenuWidget);
	void OnRMBPressed_PauseMenu_ReturnToMainMenu(UMyButton * ButtonWidget);
	void OnRMBReleased_PauseMenu_ReturnToMainMenu(UMyButton * ButtonWidget);

	/* "Return to operating system" button on pause menu */
	void OnLMBPressed_PauseMenu_ReturnToOperatingSystem(UMyButton * ButtonWidget);
	void OnLMBReleased_PauseMenu_ReturnToOperatingSystem(UMyButton * ButtonWidget, UPauseMenu * PauseMenuWidget);
	void OnRMBPressed_PauseMenu_ReturnToOperatingSystem(UMyButton * ButtonWidget);
	void OnRMBReleased_PauseMenu_ReturnToOperatingSystem(UMyButton * ButtonWidget);

	/* Confirmation widgets */
	void OnLMBPressed_ConfirmationWidgetYesButton(UMyButton * ButtonWidget);
	void OnLMBReleased_ConfirmationWidgetYesButton(UMyButton * ButtonWidget, UInMatchConfirmationWidget  * ConfirmationWidget);
	void OnRMBPressed_ConfirmationWidgetYesButton(UMyButton * ButtonWidget);
	void OnRMBReleased_ConfirmationWidgetYesButton(UMyButton * ButtonWidget);
	void OnLMBPressed_ConfirmationWidgetNoButton(UMyButton * ButtonWidget);
	void OnLMBReleased_ConfirmationWidgetNoButton(UMyButton * ButtonWidget, UInMatchConfirmationWidget  * ConfirmationWidget);
	void OnRMBPressed_ConfirmationWidgetNoButton(UMyButton * ButtonWidget);
	void OnRMBReleased_ConfirmationWidgetNoButton(UMyButton * ButtonWidget);

	/* Buttons to leave the settings menu */
	void OnLMBPressed_SettingsMenu_SaveChangesAndReturnButton(UMyButton * ButtonWidget);
	void OnLMBReleased_SettingsMenu_SaveChangesAndReturnButton(UMyButton * ButtonWidget, USettingsWidget * SettingsWidget);
	void OnRMBPressed_SettingsMenu_SaveChangesAndReturnButton(UMyButton * ButtonWidget);
	void OnRMBReleased_SettingsMenu_SaveChangesAndReturnButton(UMyButton * ButtonWidget);
	void OnLMBPressed_SettingsMenu_DiscardChangesAndReturnButton(UMyButton * ButtonWidget);
	void OnLMBReleased_SettingsMenu_DiscardChangesAndReturnButton(UMyButton * ButtonWidget, USettingsWidget * SettingsWidget);
	void OnRMBPressed_SettingsMenu_DiscardChangesAndReturnButton(UMyButton * ButtonWidget);
	void OnRMBReleased_SettingsMenu_DiscardChangesAndReturnButton(UMyButton * ButtonWidget);

	/* Button to reset all settings back to defaults */
	void OnLMBPressed_SettingsMenu_ResetToDefaults(UMyButton * ButtonWidget);
	void OnLMBReleased_SettingsMenu_ResetToDefaults(UMyButton * ButtonWidget, USettingsWidget * SettingsMenu);
	void OnRMBPressed_SettingsMenu_ResetToDefaults(UMyButton * ButtonWidget);
	void OnRMBReleased_SettingsMenu_ResetToDefaults(UMyButton * ButtonWidget);

	/* Settings menu confirmation widget asking whether to reset to defaults */
	void OnLMBPressed_SettingsMenu_ConfirmResetToDefaults(UMyButton * ButtonWidget);
	void OnLMBReleased_SettingsMenu_ConfirmResetToDefaults(UMyButton * ButtonWidget, USettingsConfirmationWidget_ResetToDefaults * ConfirmationWidget);
	void OnRMBPressed_SettingsMenu_ConfirmResetToDefaults(UMyButton * ButtonWidget);
	void OnRMBReleased_SettingsMenu_ConfirmResetToDefaults(UMyButton * ButtonWidget);
	void OnLMBPressed_SettingsMenu_CancelResetToDefaults(UMyButton * ButtonWidget);
	void OnLMBReleased_SettingsMenu_CancelResetToDefaults(UMyButton * ButtonWidget, USettingsConfirmationWidget_ResetToDefaults * ConfirmationWidget);
	void OnRMBPressed_SettingsMenu_CancelResetToDefaults(UMyButton * ButtonWidget);
	void OnRMBReleased_SettingsMenu_CancelResetToDefaults(UMyButton * ButtonWidget);

	/* The confirmation widget on the settings menu asking whether to save or discard changes */
	void OnLMBPressed_SettingsConfirmationWidget_Confirm(UMyButton * ButtonWidget);
	void OnLMBReleased_SettingsConfirmationWidget_Confirm(UMyButton * ButtonWidget, USettingsConfirmationWidget_Exit * ConfirmationWidget);
	void OnRMBPressed_SettingsConfirmationWidget_Confirm(UMyButton * ButtonWidget);
	void OnRMBReleased_SettingsConfirmationWidget_Confirm(UMyButton * ButtonWidget);
	void OnLMBPressed_SettingsConfirmationWidget_Discard(UMyButton * ButtonWidget);
	void OnLMBReleased_SettingsConfirmationWidget_Discard(UMyButton * ButtonWidget, USettingsConfirmationWidget_Exit * ConfirmationWidget);
	void OnRMBPressed_SettingsConfirmationWidget_Discard(UMyButton * ButtonWidget);
	void OnRMBReleased_SettingsConfirmationWidget_Discard(UMyButton * ButtonWidget);
	void OnLMBPressed_SettingsConfirmationWidget_Cancel(UMyButton * ButtonWidget);
	void OnLMBReleased_SettingsConfirmationWidget_Cancel(UMyButton * ButtonWidget, USettingsConfirmationWidget_Exit * ConfirmationWidget);
	void OnRMBPressed_SettingsConfirmationWidget_Cancel(UMyButton * ButtonWidget);
	void OnRMBReleased_SettingsConfirmationWidget_Cancel(UMyButton * ButtonWidget);

	/* Buttons to switch between submenus in the settings menu */
	void OnLMBPressed_SwitchSettingsSubmenu(UMyButton * ButtonWidget);
	void OnLMBReleased_SwitchSettingsSubmenu(UMyButton * ButtonWidget, USettingsWidget * SettingsWidget, UWidget * WidgetToSwitchTo);
	void OnRMBPressed_SwitchSettingsSubmenu(UMyButton * ButtonWidget);
	void OnRMBReleased_SwitchSettingsSubmenu(UMyButton * ButtonWidget);

	/* Buttons to adjust video settings */
	void OnLMBPressed_AdjustVideoSettingLeft(UMyButton * ButtonWidget);
	void OnLMBReleased_AdjustVideoSettingLeft(UMyButton * ButtonWidget, USingleVideoSettingWidget * SettingWidget);
	void OnRMBPressed_AdjustVideoSettingLeft(UMyButton * ButtonWidget);
	void OnRMBReleased_AdjustVideoSettingLeft(UMyButton * ButtonWidget);
	void OnLMBPressed_AdjustVideoSettingRight(UMyButton * ButtonWidget);
	void OnLMBReleased_AdjustVideoSettingRight(UMyButton * ButtonWidget, USingleVideoSettingWidget * SettingWidget);
	void OnRMBPressed_AdjustVideoSettingRight(UMyButton * ButtonWidget);
	void OnRMBReleased_AdjustVideoSettingRight(UMyButton * ButtonWidget);

	/* Buttons to change the audio quality */
	void OnLMBPressed_DecreaseAudioQuality(UMyButton * ButtonWidget);
	void OnLMBReleased_DecreaseAudioQuality(UMyButton * ButtonWidget, UAudioSettingsWidget * AudioWidget);
	void OnRMBPressed_DecreaseAudioQuality(UMyButton * ButtonWidget);
	void OnRMBReleased_DecreaseAudioQuality(UMyButton * ButtonWidget);
	void OnLMBPressed_IncreaseAudioQuality(UMyButton * ButtonWidget);
	void OnLMBReleased_IncreaseAudioQuality(UMyButton * ButtonWidget, UAudioSettingsWidget * AudioWidget);
	void OnRMBPressed_IncreaseAudioQuality(UMyButton * ButtonWidget);
	void OnRMBReleased_IncreaseAudioQuality(UMyButton * ButtonWidget);

	/* Buttons to change the volume of a sound class */
	void OnLMBPressed_DecreaseVolumeButton(UMyButton * ButtonWidget);
	void OnLMBReleased_DecreaseVolumeButton(UMyButton * ButtonWidget, USingleAudioClassWidget * AudioWidget);
	void OnRMBPressed_DecreaseVolumeButton(UMyButton * ButtonWidget);
	void OnRMBReleased_DecreaseVolumeButton(UMyButton * ButtonWidget);
	void OnLMBPressed_IncreaseVolumeButton(UMyButton * ButtonWidget);
	void OnLMBReleased_IncreaseVolumeButton(UMyButton * ButtonWidget, USingleAudioClassWidget * AudioWidget);
	void OnRMBPressed_IncreaseVolumeButton(UMyButton * ButtonWidget);
	void OnRMBReleased_IncreaseVolumeButton(UMyButton * ButtonWidget);

	/* Buttons to adjust a boolean control setting */
	void OnLMBPressed_AdjustBoolControlSettingLeft(UMyButton * ButtonWidget);
	void OnLMBReleased_AdjustBoolControlSettingLeft(UMyButton * ButtonWidget, USingleBoolControlSettingWidget * SettingWidget);
	void OnRMBPressed_AdjustBoolControlSettingLeft(UMyButton * ButtonWidget);
	void OnRMBReleased_AdjustBoolControlSettingLeft(UMyButton * ButtonWidget);
	void OnLMBPressed_AdjustBoolControlSettingRight(UMyButton * ButtonWidget);
	void OnLMBReleased_AdjustBoolControlSettingRight(UMyButton * ButtonWidget, USingleBoolControlSettingWidget * SettingWidget);
	void OnRMBPressed_AdjustBoolControlSettingRight(UMyButton * ButtonWidget);
	void OnRMBReleased_AdjustBoolControlSettingRight(UMyButton * ButtonWidget);

	/* Buttons to adjust a float control setting */
	void OnLMBPressed_DecreaseControlSetting_Float(UMyButton * ButtonWidget);
	void OnLMBReleased_DecreaseControlSetting_Float(UMyButton * ButtonWidget, USingleFloatControlSettingWidget * SettingWidget);
	void OnRMBPressed_DecreaseControlSetting_Float(UMyButton * ButtonWidget);
	void OnRMBReleased_DecreaseControlSetting_Float(UMyButton * ButtonWidget);
	void OnLMBPressed_IncreaseControlSetting_Float(UMyButton * ButtonWidget);
	void OnLMBReleased_IncreaseControlSetting_Float(UMyButton * ButtonWidget, USingleFloatControlSettingWidget * SettingWidget);
	void OnRMBPressed_IncreaseControlSetting_Float(UMyButton * ButtonWidget);
	void OnRMBReleased_IncreaseControlSetting_Float(UMyButton * ButtonWidget);

	/* The buttons in the key bindings menu to change the mapping */
	void OnLMBPressed_RemapKey(UMyButton * ButtonWidget);
	void OnLMBReleased_RemapKey(UMyButton * ButtonWidget, USingleKeyBindingWidget * KeyBindingWidget);
	void OnRMBPressed_RemapKey(UMyButton * ButtonWidget);
	void OnRMBReleased_RemapKey(UMyButton * ButtonWidget);

	/* The buttons to confirm the players wants to rebind a key */
	void OnLMBPressed_RebindingCollisionWidgetConfirm(UMyButton * ButtonWidget);
	void OnLMBReleased_RebindingCollisionWidgetConfirm(UMyButton * ButtonWidget, URebindingCollisionWidget * CollisionWidget);
	void OnRMBPressed_RebindingCollisionWidgetConfirm(UMyButton * ButtonWidget);
	void OnRMBReleased_RebindingCollisionWidgetConfirm(UMyButton * ButtonWidget);

	/* The buttons to cancel rebinding a key */
	void OnLMBPressed_RebindingCollisionWidgetCancel(UMyButton * ButtonWidget);
	void OnLMBReleased_RebindingCollisionWidgetCancel(UMyButton * ButtonWidget, URebindingCollisionWidget * CollisionWidget);
	void OnRMBPressed_RebindingCollisionWidgetCancel(UMyButton * ButtonWidget);
	void OnRMBReleased_RebindingCollisionWidgetCancel(UMyButton * ButtonWidget);

	void OnLMBPressed_ResetKeyBindingsToDefaults(UMyButton * ButtonWidget);
	void OnLMBReleased_ResetKeyBindingsToDefaults(UMyButton * ButtonWidget);
	void OnRMBPressed_ResetKeyBindingsToDefaults(UMyButton * ButtonWidget);
	void OnRMBReleased_ResetKeyBindingsToDefaults(UMyButton * ButtonWidget);

	/* Call this to have the next key events rebind an action */
	void ListenForKeyRemappingInputEvents(EKeyMappingAction ActionToRebind, UKeyBindingsWidget * InKeyBindingsWidget);
	void ListenForKeyRemappingInputEvents(EKeyMappingAxis ActionToRebind, UKeyBindingsWidget * InKeyBindingsWidget);

	void CancelPendingKeyRebind();
	void OnCancelPendingKeyRebindButtonHeld();

	/* Handles a key press. Remember it will be called for both the key down and key up events */
	virtual bool InputKey(FKey Key, EInputEvent EventType, float AmountDepressed, bool bGamepad) override;

	/** 
	 *	On key event update what modifiers are being pressed.
	 *	
	 *	@return - true if Key is a modifier key and EventType == IE_Pressed or IE_Released
	 */
	bool PendingKeyRebind_UpdatePressedModifierFlags(FKey Key, EInputEvent EventType);

	/** 
	 *	Attempt to rebind an action to a new key, then notify a UI element if one is set. 
	 *	Will apply the change but does not write it to .ini file. 
	 *	
	 *	@param Key - the key player wants to assign to Action
	 *	@param bForce - if true the key binding will happen for sure even if there is a conflicting 
	 *	binding already set except if it would cause an unrebindable action to become unbinded. 
	 *	The previous action will become unbound. If false then if there is a conflicting mapping 
	 *	already then the change will not take place.
	 *	@return - true if successful 
	 */
	bool TryRebindAction(EKeyMappingAction Action, const FKeyWithModifiers & Key, bool bForce, FTryBindActionResult & OutResult);
	bool TryRebindAction(EKeyMappingAxis Action, const FKeyWithModifiers & Key, bool bForce, FTryBindActionResult & OutResult);

protected:

	void DoOnEveryLMBPress();
	void DoOnEveryLMBRelease();

	/* If one of these are not "None" then the player wants to change one of their key bindings */
	EKeyMappingAction PendingKeyRebind_Action;
	EKeyMappingAxis PendingKeyRebind_Axis;

	/* If true then the game is asking the player "Are you sure you want to rebind this key?" 
	likely because there is a key mapping collision with another action */
	bool PendingKeyRebind_bWaitingForConfirmation;

	/* Flags to keep track of what modifier keys are pressed while trying to rebind a key. 
	Only meaningfull while trying to rebind a key. Do not query these at any other time. 
	Use PlayerInput->IsCtrlPressed() etc instead. 
	There are two sets of flags - one for modifier keys on the left of the keyboard and one 
	for modifier keys on the right side of the keyboard */
	EKeyModifiers PendingKeyRebind_PressedModifierFlags_Left;
	EKeyModifiers PendingKeyRebind_PressedModifierFlags_Right;

	/* The widget for key bindings. Can be null */
	UKeyBindingsWidget * PendingKeyRebind_KeyBindingsWidget;

	/* If >= 0 then the player is holding the cancel key down while a key bind change is pending 
	TODO move this closer to other variables used in tick */
	float PendingKeyRebind_TimeSpentTryingToCancel;

	/* Return what modifier keys are pressed, taking into account both left and right ones. 
	Only call this if a key bind is pending. Otherwise query PlayerInput instead */
	EKeyModifiers PendingKeyRebind_GetAllKeyModifiers() const;

public:

	//---------------------------------------------------------------------------------------
	//	Input events
	//---------------------------------------------------------------------------------------

	/* All of these are made UFUNCTION because of the nasty bug described near the top of
	KeyMappings.h */
	UFUNCTION() void Input_LMBPressed();
	UFUNCTION() void Input_LMBReleased();
	UFUNCTION() void Input_RMBPressed();
	UFUNCTION() void Input_RMBReleased();
	UFUNCTION() void Axis_MoveCameraRight(float Value);
	UFUNCTION() void Axis_MoveCameraForward(float Value);
	UFUNCTION() void Input_ZoomCameraIn();
	UFUNCTION() void Input_ZoomCameraOut();
	UFUNCTION() void Input_EnableCameraFreeLook();
	UFUNCTION() void Input_DisableCameraFreeLook();
	UFUNCTION() void Input_ResetCameraRotationToOriginal();
	UFUNCTION() void Input_ResetCameraZoomToOriginal();
	UFUNCTION() void Input_ResetCameraRotationAndZoomToOriginal();
	UFUNCTION() void Input_OpenTeamChat();
	UFUNCTION() void Input_OpenAllChat();
	UFUNCTION() void Input_Cancel();
	UFUNCTION() void Input_QuitGame();
	UFUNCTION() void Input_ToggleDevelopmentCheatWidget();
	UFUNCTION() void Input_OpenCommanderSkillTree();
	UFUNCTION() void Input_CloseCommanderSkillTree();
	UFUNCTION() void Input_ToggleCommanderSkillTree();
	UFUNCTION() void Input_AssignControlGroupButtonPressed_0();
	UFUNCTION() void Input_AssignControlGroupButtonPressed_1();
	UFUNCTION() void Input_AssignControlGroupButtonPressed_2();
	UFUNCTION() void Input_AssignControlGroupButtonPressed_3();
	UFUNCTION() void Input_AssignControlGroupButtonPressed_4();
	UFUNCTION() void Input_AssignControlGroupButtonPressed_5();
	UFUNCTION() void Input_AssignControlGroupButtonPressed_6();
	UFUNCTION() void Input_AssignControlGroupButtonPressed_7();
	UFUNCTION() void Input_AssignControlGroupButtonPressed_8();
	UFUNCTION() void Input_AssignControlGroupButtonPressed_9();
	UFUNCTION() void Input_SelectControlGroupButtonPressed_0();
	UFUNCTION() void Input_SelectControlGroupButtonPressed_1();
	UFUNCTION() void Input_SelectControlGroupButtonPressed_2();
	UFUNCTION() void Input_SelectControlGroupButtonPressed_3();
	UFUNCTION() void Input_SelectControlGroupButtonPressed_4();
	UFUNCTION() void Input_SelectControlGroupButtonPressed_5();
	UFUNCTION() void Input_SelectControlGroupButtonPressed_6();
	UFUNCTION() void Input_SelectControlGroupButtonPressed_7();
	UFUNCTION() void Input_SelectControlGroupButtonPressed_8();
	UFUNCTION() void Input_SelectControlGroupButtonPressed_9();
	UFUNCTION() void Input_SelectControlGroupButtonReleased_0();
	UFUNCTION() void Input_SelectControlGroupButtonReleased_1();
	UFUNCTION() void Input_SelectControlGroupButtonReleased_2();
	UFUNCTION() void Input_SelectControlGroupButtonReleased_3();
	UFUNCTION() void Input_SelectControlGroupButtonReleased_4();
	UFUNCTION() void Input_SelectControlGroupButtonReleased_5();
	UFUNCTION() void Input_SelectControlGroupButtonReleased_6();
	UFUNCTION() void Input_SelectControlGroupButtonReleased_7();
	UFUNCTION() void Input_SelectControlGroupButtonReleased_8();
	UFUNCTION() void Input_SelectControlGroupButtonReleased_9();
	/* Called when mouse moves left/right */
	UFUNCTION() void On_Mouse_Move_X(float Value);
	/* Called when mouse moves up/down */
	UFUNCTION() void On_Mouse_Move_Y(float Value);

protected:

	/* Get how much to multiply movement due to the zoom amount of the camera */
	float GetCameraMovementMultiplierDueToZoom() const;

	void MoveCameraForward(float Value);
	void MoveCameraRight(float Value);

	/* Check if at edge of screen for camera movement and do movement */
	void MoveIfAtEdgeOfScreen(float DeltaTime, const FCursorInfo *& OutCursorToShowThisFrame);

	/* Functions to say whether on a reset button press whether camera needs to be reset or not */
	bool ShouldResetCameraRotation() const;
	bool ShouldResetCameraZoom() const;

	void OnAssignControlGroupButtonPressed(int32 ControlGroupNumber);

	/* Here cause double press is measured from when button is released to when it is pressed 
	next time */

	void OnSelectControlGroupButtonPressed(int32 ControlGroupNumber);
	void OnSelectControlGroupButtonReleased(int32 ControlGroupNumber);
	void OnSelectControlGroupButtonSinglePress(int32 ControlGroupNumber);
	void OnSelectControlGroupButtonDoublePress(int32 ControlGroupNumber);

	/* Causes the current selection to become a ctrl group, like when ctrl + a number button 
	is pressed */
	void CreateControlGroup(int32 ControlGroupNumber);

	/* Change the selection to that of a ctrl group 
	@return - true if player's selection changed */
	bool SelectControlGroup(int32 ControlGroupNumber);

	/* Sets the camera view to a ctrl group, does not actually select them. Could change it 
	though. 
	@return - true if ctrl group contained at least one selectable that was valid and alive 
	and camera position was set to some location */
	bool SnapViewToControlGroup(int32 ControlGroupNumber);

	/** 
	 *	Return the number of members in a ctrl group, but does not account for whether they 
	 *	are still valid/alive or not 
	 *	@param CtrlGroupNumber - ctrl group to query. Range = 0 ... 9
	 */
	int32 GetNumCtrlGroupMembers(int32 CtrlGroupNumber) const;

	/* Begin play type setup function to add key/value pairs to CtrlGroups. */
	void InitCtrlGroups();

	/*	Ctrl group TArray. Maps button pressed to array of selectables that are part of that
	 *	ctrl group. 
	 *
	 *	Ctrl groups are NOT updated when the selectables they contain become invalid and/or 
	 *	reach zero health. This is because there are 10 groups with potentially a lot of 
	 *	selectables in each one and it would be too much of a hit to performance. Instead you will 
	 *	have to check validity/health of each entry before using it.
	 *
	 *	I don't really know what is best here - either hard pointers, weak pointers or using 
	 *	the selectableIDs in terms of performance, keeping GC in mind 
	 */
	UPROPERTY()
	TArray <FCtrlGroupList> CtrlGroups;

	/* The last number button that was released from being pressed. Here to detect double presses */
	int32 LastSelectControlGroupButtonReleased;

	/* The time the last number button press was released */
	float LastSelectControlGroupButtonReleaseTime;

	bool bWasLastSelectControlGroupButtonPressADoublePress;

	/** 
	 *	Call when pressing a button if you would like to know if it was a double 
	 *	click/press/whatever. Will update fields to store whether future presses are also doubles. 
	 *	Instead of using this func an alternative would be using a custom UPlayerInput class and 
	 *	overridding InputKey but I do not know if setting custom UPlayerInput class is possible.
	 *	Double press time is from the moment button was released to when it is pressed again
	 *	@param ControlGroupButtonPressed - the ctrl group key that was pressed, range: 0 ... 9
	 *	@return - whether it was a double click/press/whatever or not 
	 */
	bool WasDoublePress(int32 ControlGroupButtonPressed);

	// Reference to players camera pawn during a match
	UPROPERTY()
	APlayerCamera * MatchCamera;
	
	// Reference to ghost building
	UPROPERTY()
	AGhostBuilding * GhostBuilding;

	/**
	 *	Here to delay recording the screen location of ghost until tick. Recording it on 
	 *	LMB press may give slightly off value, but haven't tested if this is the case
	 */
	bool bNeedToRecordGhostLocOnNextTick;

	/* True if mouse has moved at least GhostRotationRadius */
	bool bIsGhostRotationActive;

	/* The screen location of the ghost building. Should not change while rotating ghost. 
	Will be updated before calling RotateGhost */
	FVector2D GhostScreenSpaceLoc;

	/* The larger this is the more you have to move the mouse to rotate ghost. Small values  
	mean the ghost building can sometimes rotate undesirably when trying to actually place it */
	float GhostRotationRadius;

	/* Calculate the new rotation of ghost when rotating
	it with LMB held down, then rotate it
	@param DeltaTime - deltatime from tick */
	void RotateGhost(float DeltaTime);

	/* Higher values mean ghost rotates faster */
	float GhostRotationSpeed;

#if GHOST_ROTATION_METHOD == STANDARD_GHOST_ROT_METHOD
	/* Some variables only used by this rotation method */

	// The way the ghost is currently rotating
	ERotationDirection GhostRotationDirection;

	/** 
	 *	This is used to decide if ghost rotation direction should change. 
	 *	When negative mouse movement is in a direction that should cause rotation in the opposite 
	 *	direction then this number will be decreased, while movement in the current direction 
	 *	will increase it (up to a certain limit)
	 */
	float GhostAccumulatedMovementTowardsDirection;

	/* Some functions only used by the standard method. Can be removed and replaced with 
	lambda expressions inside RotateGhost func */

	/* 
	 *	@param ScreenSpaceAngleWithGhost - angle mouse makes with ghost (all screenspace). 
	 *	0 = 3 o'clock, 90 = 12 o'clock, etc
	 */
	float GhostRotStandard_GetAngleFalloffMultiplier(float ScreenSpaceAngleWithGhost) const;
	/* @param Distance2D - screen space distance from ghost to mouse */
	float GhostRotStandard_GetDistanceFalloffMultiplier(float Distance2DFromGhost) const;
	/* Assigns rot when mouse moves in a direction that isn't expected to rotate ghost */
	float GhostRotStandard_AssignNeutralDirectionYawRot(float Distance2D,
		bool bRotatingClockwise, bool bRotatingCounterClockwise, float ScreenSpaceAngleWithGhost, 
		float Distance2DFromGhost);
	/* Function to assign rotation when mouse moves in such a way that it
	is expected to apply rotation in the clockwise direction */
	float GhostRotStandard_AssignYawRotForClockwiseTypeMouseMovement(float Distance2D,
		bool bRotatingClockwise, bool bRotatingCounterClockwise, bool bNotRotating, 
		float Distance2DFromGhost);
	// Opposite of AssignYawRotForClockwiseTypeMouseMovement
	float GhostRotStandard_AssignYawRotForCounterClockwiseTypeMouseMovement(float Distance2D,
		bool bRotatingClockwise, bool bRotatingCounterClockwise, bool bNotRotating, 
		float Distance2DFromGhost);
#endif // GHOST_ROTATION_METHOD == STANDARD_GHOST_ROT_METHOD

	/* True if the player can rotate the camera with mouse movement. By default this is the 
	middle mouse button */
	bool bIsCameraFreeLookEnabled;

	/* Screen space location of mouse in previous frame or when the LMB was pressed */
	FVector2D MouseLocLastFrame;

	/* Screen space location */
	FVector2D MouseLocThisFrame;

	/* Return screen space coords of mouse as a FVector2D
	@return mouse screen space coords */
	FVector2D GetMouseCoords();

public:

	/**
	 *	Spawn a ghost building and attach it to the mouse cursor for placement in the world
	 *	
	 *	@param BuildingType - the type of building to spawn
	 *	@param InstigatorsID - Only relveant if the building is being built from a context menu. The
	 *	selectable ID of the selectable wanting to build this building. Can be ignored if building from
	 *	the HUD persistent panel and 0 will be the signal for that 
	 */
	void SpawnGhostBuilding(EBuildingType BuildingType, uint8 InstigatorsID = 0);

private:

	/**
	 *	Whether the mouse is hovering over a UI element. 
	 *	
	 *	Currently this will only return true if the UI element is a UMyButton. But I would like 
	 *	to change this to include pretty much all of the HUD. But I think this would require making 
	 *	more custom slate widgets 
	 */
	bool IsHoveringUIElement() const;

	/** 
	 *	Call when mouse button is released. Returns true if the mouse
	 *	hasn't moved around the screen enough for it to still be considered
	 *	a click
	 *
	 *	@return - True if a mouse click happened 
	 */
	bool WasMouseClick();

	/* Line trace under mouse and put hit result into HitResult.
	This version should only be called during tick.
	Tbh if it returns false there will be problems
	@param Channel the channel to test for hits on
	@return trace hit something within MaxLineTraceDistance */
	bool LineTraceUnderMouse(ECollisionChannel Channel);

	/* Line trace under mouse and put hit result into HitResult.
	Tbh if it returns false there will be problems
	@param Channel the channel to test for hits on
	@param Hit - the hit result to store the result in
	@return trace hit something within MaxLineTraceDistance */
	bool LineTraceUnderMouse(ECollisionChannel Channel, FHitResult & Hit);

	/* Move ghost building to location of mouse */
	void MoveGhostBuilding();

	/* Check if it's ok to build ghost building. Checks literally everything there is to check
	@param BuildingType - type of building
	@param InstigatorsID - will be 0 if being built from the HUD persistent panel. Otherwise this
	will be the selectable ID of the selectable that wants to build this building
	@param Location - location
	@param Rotation - rotation
	@param bShowHUDMessages - whether to try and show a message on the HUD if any of the conditions
	fail
	@param OutProducer - actor that will be used to produce building, or null if no actors
	queue can be used to produce the desired building. Currently is always a ABuilding but was 
	left as an AActor in case in future actors other than buildings can produce stuff
	@param OutQueueType - type of queue that will be used to produce building
	@return - true if it is ok */
	bool CanPlaceBuilding(EBuildingType BuildingType, uint8 InstigatorsID, const FVector & Location,
		const FRotator & Rotation, bool bShowHUDMessages, ABuilding *& OutProducer,
		EProductionQueueType & OutQueueType) const;

	/* Clients version. Doesn't care about what actor is picked for production since it
	will be checked on server anyway */
	bool CanPlaceBuilding(EBuildingType BuildingType, uint8 InstigatorsID, const FVector & Location,
		const FRotator & Rotation, bool bShowHUDMessages) const;

public:

	/* Remove ghost building and place a real one.
	@param BuildingType - the type of building to start building
	@param Location - the location
	@param Rotation - the rotation
	@param ConstructionInstigatorID - selectable ID of selectable wanting to place this building,
	but only relevant if being placed from a context menu. Should be 0 otherwise */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_PlaceBuilding(EBuildingType BuildingType, const FVector_NetQuantize10 & Location, const FRotator & Rotation, uint8 ConstructionInstigatorID);

private:

	/* Called when the server successfully places a building. If its build method is BuildsInTab
	the second function will remove it from the array of complete queues in PS. 3 functions to
	reduce on bandwidth
	@param ProducingBuilding - only relevant if placing a building using BuildInTab. The building
	that produced the other building
	@param BuildingType - the type of building that was placed, not the producer. Not required
	but helps HUD efficiently find button this action effects */
	UFUNCTION(Client, Reliable)
	void Client_OnPlaceBuildingSuccess();
	UFUNCTION(Client, Reliable)
	void Client_OnBuildsInTabPlaceBuildingSuccess(ABuilding * ProducerBuilding, EBuildingType BuildingType);

	// Cancel placing ghost e.g. by right clicking
	void CancelGhost();

	/* Max distance that GetHitResultUnderCursorByChannel will consider
	it a hit */
	float MaxLineTraceDistance;

	/* Array of widgets currently being displayed (excluding any shown with ShowWidget where 2nd
	param is false, for example HUD). .Last() is the widget being displayed right now */
	UPROPERTY()
	TArray < EMatchWidgetType > WidgetHistory;

	// Holds already created widgets
	UPROPERTY()
	TMap < EMatchWidgetType, UInGameWidgetBase * > MatchWidgets;

	/** 
	 *	Get a reference to the widget of a certain type. May be constructed and setup if one has
	 *	not already been created. Will check if BP is set in faction info before using one in game
	 *	instance 
	 */
	UInGameWidgetBase * GetWidget(EMatchWidgetType WidgetType, bool bCallSetupWidget = true);

public:

	/* Show a match widget. We use AddToViewport and RemoveFromViewport to toggle widget visibility.
	Will construct widget if it has not already been constructed
	@param WidgetType - type of widget to add to viewport
	@param bAddToWidgetHistory - whether to add to WidgetHistory. False is mainly there just
	for HUD
	@param PreviousWidgetVisibility - the visibility to set to the widget last in widget history
	before this was called
	@param bCallSetupWidget - whether to calle UInGameWidgetBase::SetupWidget on the widget to be shown 
	@return - reference to widget */
	UInGameWidgetBase * ShowWidget(EMatchWidgetType WidgetType, bool bAddToWidgetHistory,
		ESlateVisibility PreviousWidgetVisibility = ESlateVisibility::Collapsed, bool bCallSetupWidget = true);

	/* Return to previous widget in widget history by removing current from viewport. Will hide
	current widget if WidgetHistory.Num() == 1. If WidgetHistory.Num() == 0 crash
	@return - widget that becomes the current as aresult of this */
	UInGameWidgetBase * ShowPreviousWidget();

	/* Hide a widget and remove it from widget history */
	void HideWidget(EMatchWidgetType WidgetType, bool bTryRemoveFromWidgetHistory);

	/* Check if an optional widget has its blueprint set from editor so we know whether we
	should try showing it or not */
	bool IsWidgetBlueprintSet(EMatchWidgetType WidgetType) const;

	/* Get the ZOrder for a match widget */
	static int32 GetWidgetZOrder(EMatchWidgetType WidgetType);

private:

	uint8 GetBuildingIndex(EBuildingType BuildingType) const;

	virtual void OnRep_PlayerState() override;

	void SetupReferences();
	void AssignGI();
	void AssignPS();

	/** 
	 *	Call function that returns void after delay
	 *	
	 *	@param TimerHandle - timer handle to use
	 *	@param Function - function to call
	 *	@param Delay - delay before calling function 
	 */
	void Delay(FTimerHandle & TimerHandle, void(ARTSPlayerController:: *Function)(), float Delay);

	// Hit result for trace under mouse on tick
	FHitResult HitResult;

	//=========================================================================================
	//	Mouse press/release stuff
	//=========================================================================================

	/* Object under mouse on tick. Used for example to highlight
	selectables under mouse */
	UPROPERTY()
	TScriptInterface <ISelectable> ObjectUnderMouseOnTick;

	/* The object under the mouse when it is pressed. Needs to
	be the same object under mouse when LMB is released for it
	to get selected. */
	UPROPERTY()
	AActor * ObjectUnderMouseOnLMB;

	/* True if LMB is currently held down */
	bool bIsLMBPressed;

	/* This is used to get an idea of what mouse clicks should
	do. Returns true if a context action is pending e.g. a ghost
	is being placed or an attack move command is about to be
	given. */
	bool IsContextActionPending() const;

	/* Return true if an ability form the global skills panel is pending. This includes abilities 
	that target a selectable, the ground, both or a player */
	bool IsGlobalSkillsPanelAbilityPending() const;

	/* Used in On_Mouse_Move_X/Y to know whether we should consider maequee selection to be 
	active or not */
	bool bWantsMarquee;

	/* True if marquee selection is active */
	bool bIsMarqueeActive;

	/* Sends info for the marquee rectangle to HUD */
	void SendMouseLocToMarqueeHUD();

	/* The button that was pressed/released with the mouse button */
	UMyButton * ButtonPressedOnLMBPressed;
	UMyButton * ButtonPressedOnRMBPressed;

	/* Return whether we think a click has happened on a UI button */
	bool OnLMBReleased_WasUIButtonClicked(UMyButton * ButtonLMBReleasedOn) const;
	bool OnRMBReleased_WasUIButtonClicked(UMyButton * ButtonRMBReleasedOn) const;

	/* How far the mouse has moved since it was clicked */
	float MouseMovement;

	/* How far mouse is can move before it is no longer considered a click. Used by marquee
	selection */
	float MouseMovementThreshold;

	/* Returns true if actor implements Selectable */
	bool IsASelectable(AActor * Actor) const;

	//=======================================================================================
	//	Other stuff
	//=======================================================================================

	/* ID of player. Used to determine what is under control by this
	player. Assigned on PostLogin */
	FName PlayerID;

	void SetSpringArmTargetArmLength(float NewLength);
	
	/* Adjust the location of world widgets because the camera's zoom amount changed */
	void AdjustWorldWidgetLocations();

	/* Called when mouse has been pressed and released near the
	same location. Will select a slectable. Assumes param is valid.
	@param SelectedActor - the object that was under the mouse both when it
	was pressed and released */
	void SingleSelect(AActor * SelectedActor);

	/* Returns whether a selectable selection sound should be played. Usually called when the
	players selection changes */
	bool ShouldPlaySelectionSound(const FSelectableAttributesBase & CurrentSelectedInfo) const;

	/* Plays CurrentSelected's selectable selection sound */
	void PlaySelectablesSelectionSound();

	/* Returns whether to play a commands sound */
	bool ShouldPlayMoveCommandSound(const TScriptInterface < ISelectable > & SelectableToPlaySoundFor) const;

	// Play sound for when selectable is given move command
	void PlayMoveCommandSound(const TScriptInterface < ISelectable > & SelectableToPlaySoundFor);

	// Whether should play context command sound
	bool ShouldPlayCommandSound(const TScriptInterface < ISelectable > & SelectableToPlaySoundFor, EContextButton Command);

	// Play command sound for context commnand
	void PlayCommandSound(const TScriptInterface < ISelectable > & SelectableToPlaySoundFor, EContextButton Command);

	/* Return whether should play sound when setting unit rally point for a building */
	bool ShouldPlayChangeRallyPointSound() const;

	// Play sound for setting unit rally point for building
	void PlayChangeRallyPointSound();

	/* Return whether to play a sound when a chat message is received */
	bool ShouldPlayChatMessageReceivedSound(EMessageRecipients MessageType) const;

	/* Play sound for receiving a chat message
	@param MessageType - type of message that was received e.g. team chat, everyone chat */
	void PlayChatMessageReceivedSound(EMessageRecipients MessageType);
	
	/* Return whether to play a sound when a selectable reaches zero health */
	bool ShouldPlayZeroHealthSound(AActor * Selectable, USoundBase * SoundRequested) const;

	void PlayZeroHealthSound(USoundBase * Sound);

	/* Tell HUD to make marquee selection. Marquee selection will be
	performed when the HUD ticks next (when DrawHUD is called)
	@param bNewValue - True to make it do it on next DrawHUD; false
	to not */
	void SetPerformMarqueeNextTick(bool bNewValue);

	/* Spawned decal to be drawn under mouse cursor. This is not the
	mouse cursor itself but a decal for context actions that have an
	AoE. If this is null then no decal will de drawn. */
	UPROPERTY()
	UDecalComponent * MouseDecal;

	/* Return whether it's possible to use an ability on the global skills panel e.g. it's off 
	cooldown etc. This function will probably assume that the ability has been aquired */
	EGameWarning IsGlobalSkillsPanelAbilityUsable(const FAquiredCommanderAbilityState * AbilityState, bool bCheckCharges) const;

	/* Version for abilities that target another player */
	EGameWarning IsGlobalSkillsPanelAbilityUsable(const FAquiredCommanderAbilityState * AbilityState, ARTSPlayerState * AbilityTarget, bool bCheckCharges) const;

	/* Version for ability that targets a selectable */
	EGameWarning IsGlobalSkillsPanelAbilityUsable(const FAquiredCommanderAbilityState * AbilityState, AActor * TargetAsActor, ISelectable * AbilityTarget, bool bCheckCharges) const;

	/* Version for ability that targets a world location */
	EGameWarning IsGlobalSkillsPanelAbilityUsable(const FAquiredCommanderAbilityState * AbilityState, const FVector & AbilityLocation, bool bCheckCharges) const;

	/**
	 *	Makes a UI element highlighted e.g. if it is a button for an ability that 
	 *	is pending and required a target then it could be highlighted 
	 */
	void HighlightButton(UMyButton * ButtonToHighlight);

	void UnhighlightHighlightedButton();

	/* Changes the mouse appearance based on what context action is
	pending. Will either make the mouse cursor visible and set its
	curosr or will make the mouse cursor invisible and draw a decal
	under it */
	void UpdateMouseAppearance(const FContextButtonInfo & ButtonInfo);

	/* Pointer to mouse cursor that is considered the default cursor. If null then no custom
	cursor has been set. In that case problems will arise if the cursor changes while doing an
	ability because the cursor will have nothing to revert back to, so if using custom cursors
	for abilities then this will need to be non null. */
	const FCursorInfo * DefaultCursor;

	/** 
	 *	I could not think of a better name. This is the cursor that is considered the default 
	 *	given where the mouse is taking into account: 
	 *	- edge scrolling 
	 *	- hovering a UI element
	 *	So if the mouse is at the edge of the screen then this will be set to one of the edge scrolling 
	 *	cursors. 
	 *	This does not take into account abilities 
	 */
	const FCursorInfo * ScreenLocationCurrentCursor;

	/* Pointer to currently set mouse cursor */
	const FCursorInfo * CurrentCursor;

	/**
	 *	Return the mouse cursor to show for hovering the mouse over a selectable. This will only 
	 *	be called if there are no abilities pending. 
	 *	
	 *	@param HoveredActor - hovered selectable as an AActor
	 *	@param HoveredSelectable - selectable that is being hovered by mouse cursor 
	 *	@param HoveredAffiliation - affiliation of the hovered selectable as a convenience 
	 *	@param OutCursor - return value
	 */
	void GetMouseCursor_NoAbilitiesPending(AActor * HoveredActor, ISelectable * HoveredSelectable, EAffiliation HoveredAffiliation, const FCursorInfo *& OutCursor) const;

	/* Changes mouse cursor
	@param MouseCursorInfo - info about the mouse cursor you want to change to
	@param bFirstCall - whether this is the first time calling this and therefore CurrentCursor
	is null */
	void SetMouseCursor(const FCursorInfo & MouseCursorInfo, bool bFirstCall = false);

	/** Set a decal to draw at the mouse location */
	void SetMouseDecal(const FBasicDecalInfo & DecalInfo);

	/** 
	 *	Make a decal appear under the mouse cursor. Uses the "acceptable location" decal of 
	 *	the ability
	 *	
	 *	@param ButtonInfo - the context button info whose decal you want drawn 
	 */
	void SetContextDecal(const FContextButtonInfo & ButtonInfo);

	/** 
	 *	Change the decal for the current pending ability 
	 *	
	 *	@param AbilityInfo - ability to set the decal for
	 *	@param DecalTypeToSet - the type of decal to set. If one has not been assigned for the 
	 *	ability then will silently fail (except maybe the "acceptable location" one, unsure of 
	 *	this)
	 */
	void SetContextDecalType(const FContextButtonInfo & AbilityInfo, EAbilityDecalType DecalTypeToSet);

	/* Set the decal for the current pending commander ability */
	void SetCommanderAbilityDecalType(const FCommanderAbilityInfo & AbilityInfo, EAbilityDecalType DecalTypeToSet);

	/* Change mouse appearance to default cursor and hide any decals
	under it */
	void ResetMouseAppearance();

	/* Stop drawing context decal */
	void HideContextDecal();

	/* Size of decal drawn under mouse for Z axis only. Higher values
	means less performance but the decal will draw up/down slopes
	better. */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	float MouseDecalHeight;

	/* Highlight/unhighlight selectable under mouse on tick */
	void UpdateSelectableUnderMouse(float DeltaTime);

	void UnhoverPreviouslyHoveredSelectable();

	/* Update location of decal under mouse. Call on tick and
	should be done after line trace has happened */
	void UpdateMouseDecalLocation(float DeltaTime);

	//========================================================================================
	//	Pending action variables
	//========================================================================================

	/* The action that requires more than just pressing the
	context button to carry out. Added '_A' onto name for a 
	temporary name change, change it back. 
	All the other pending action variables are irrelevant if this equals "None" */
	EContextButton PendingContextAction;

	/* How the pending context action is being carried out */
	EAbilityUsageType PendingContextActionUseMethod;

	/* [Client] If pending context action is to place ghost building, the ID of the selectable that wants
	to place the building. 0 means no selectable. Will only be non-zero if building is being
	placed from a context menu. This is sent with Server_PlaceBuilding. If building is placed
	successfully then on confirmation RPC unit with this ID will be ordered to go over and start
	working on building */
	uint8 GhostInstigatorID;

	/* Data that is needed by pending context command.
	One time this is used is when using inventory items we store the item type here. 
	May be possible to actually remove GhostInstigatorID and make it use this instead */
	uint8 PendingContextActionAuxilleryData;
	/* For inventory item use I store the inventory slot index here (the index the server likes 
	i.e. the index in FInventory::SlotsArray) */
	uint8 PendingContextActionMoreAuxilleryData;

	/* Whether from the time we made the ability pending the primary selected has changed to 
	something else. I think I have currently coded it so this variable will be set to 
	true if the primary selected is the only selectable selected and it gets removed from 
	Selected */
	bool bHasPrimarySelectedChanged;

	/* Pointer to something that is needed for carrying out the pending context command. 
	Currently this is only used to point to an inventory slot */
	void * PendingContextActionAuxilleryDataPtr;

	/* Pointer to the pending commander ability. Will be null if there is none */
	FAquiredCommanderAbilityState * PendingCommanderAbility;

	//===========================================================================================

	/* Special case of a context command for building buildings. This applies to the build methods
	LayFoundationsWhenAtLocation and Protoss only. Give a command to CurrentSelected to move to a
	location and try lay some foundations there.
	@param BuildingType - building type to lay foundations for when at location
	@param Location - location of foundations
	@param Rotation - rotation of foundations
	@param BuildersID - the selectable ID of the selectable wanting to build this building. Pretty
	sure this is needed provided Selected is not updated on server (which iirc it isn't) */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_GiveLayFoundationCommand(EBuildingType BuildingType, const FVector_NetQuantize10 & Location, const FRotator & Rotation, uint8 BuildersID);

	/** 
	 *	[Client] Called when a two-click context command that requires a selectable as a target is 
	 *	issued 
	 *	
	 *	@param CommandType - the ability to use
	 *	@param ClickLoc - world location where click was made. Some abilities may want this 
	 *	@param ClickTarget - the selectable that was clicked on. Assumed to be valid 
	 */
	void PrepareContextCommandRPC(EContextButton CommandType, const FVector & ClickLoc, AActor * ClickTarget);

	/**
	 *	[Client] Called when a two-click context command that requires a world location as a target 
	 *	is issued 
	 *	
	 *	@param CommandType - the ability to use 
	 *	@param ClickLoc - world location where click was made 
	 */
	void PrepareContextCommandRPC(EContextButton CommandType, const FVector & ClickLoc);

	/* RPC to make server do context command. Do not call from server */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_IssueContextCommand(const FContextCommandWithTarget & Command);

	/* RPC to make server do context command that requires a world location */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_IssueLocationTargetingContextCommand(const FContextCommandWithLocation & Command);

	/* RPC to make server do instant context action. Avoid calling this directly
	from server, use IssueInstantContextCommand instead */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_IssueInstantContextCommand(const FContextCommand & Command);

	/** 
	 *	RPC to make server do instant inventory item use. Avoid calling this from server, use 
	 *	something else instead 
	 *	
	 *	@param SelectableID - selectable ID of the selectable to carry out the item use 
	 *	@param InventorySlotIndex - the slot in their inventory they are using. This must be 
	 *	the index in FInventory::SlotsArray.
	 *	@param ItemType - this is kind of not required. It is sent because the selectable's 
	 *	inventory could have been modified on server and client hasn't been updated yet, so it 
	 *	avoids the case of a client say using a diffusal blade when on their machine they actually 
	 *	clicked on a clarity potion.
	 */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_IssueInstantUseInventoryItemCommand(uint8 SelectableID, uint8 InventorySlotIndex, 
		EInventoryItem ItemType);

	/* RPC to make server do an inventory item use for only the primary selected. 
	Call from clients only */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_IssueLocationTargetingUseInventoryItemCommand(uint8 SelectableID, uint8 InventorySlotIndex,
		EInventoryItem ItemType, const FVector_NetQuantize & Location);
	/* The TargetSelectable param can be changed to an FSelectableIdentifier to make it 2 bytes */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_IssueSelectableTargetingUseInventoryItemCommand(uint8 SelectableID, uint8 InventorySlotIndex,
		EInventoryItem ItemType, AActor * TargetSelectable);

	/** 
	 *	[Server] Issue a context command that requires a target. Assumes everything that PC
	 *	needs to check has been checked so should just issue the command
	 *	
	 *	@param ToIssueTo - the selectables to issue the command to 
	 */
	void IssueTargetRequiredContextCommandChecked(const FContextButtonInfo & AbilityInfo, 
		const FVector & ClickLoc, AActor * ClickTarget, const TArray < ISelectable * > & ToIssueTo);

	/** 
	 *	[Server] Issue a context command that requires a location as a target. Assumes everything 
	 *	that PC needs to check has been checked so should just issue commands 
	 *	
	 *	@param ToIssueTo - the selectables to issue the command to 
	 */
	void IssueLocationRequiredContextCommandChecked(const FContextButtonInfo & AbilityInfo,
		const FVector & Location, const TArray < ISelectable * > & ToIssueTo);

	/**
	 *	[Server] Issue a command to use an inventory item for primary selected. The use ability 
	 *	is an instant type one.
	 *
	 *	@param InventorySlot - inventory slot we are trying to use
	 *	@param ItemsInfo - info struct for the item in InventorySlot 
	 *	@param AbilityInfo - ability info struct for the use of the item
	 */
	void IssueInstantUseInventoryItemCommandChecked(FInventorySlotState & InventorySlot, uint8 InventorySlotIndex,
		const FInventoryItemInfo & ItemsInfo, const FContextButtonInfo & AbilityInfo);

	/* [Server] For item use that targets a world location */
	void IssueUseInventoryItemCommandChecked(FInventorySlotState & InventorySlot, uint8 InventorySlotIndex, 
		const FInventoryItemInfo & ItemsInfo, const FContextButtonInfo & AbilityInfo,
		const FVector & Location);
	/* [Server] For item use that requires another selectable as a target */
	void IssueUseInventoryItemCommandChecked(FInventorySlotState & InventorySlot, uint8 InventorySlotIndex,
		const FInventoryItemInfo & ItemsInfo, const FContextButtonInfo & AbilityInfo,
		ISelectable * Target);

	/**
	 *	Issues a context command to the selected selectables that it applies to. Called if 
	 *	clicked on another selectable. 
	 *
	 *	@param CommandInfo - all the info about the button that was pressed
	 *	@param ClickLoc - the world coords where the mouse was clicked.
	 *	@param Target - the selectable that was clicked on. If no selectable
	 *	was clicked on then this should be null 
	 *	@return - true if at least one selectable was issued a command 
	 */
	bool IssueContextCommand(const FContextButtonInfo & CommandInfo, const FVector & ClickLoc, AActor * Target);

	/** 
	 *	Issues a context command to the selected selectables that it applies to. This version 
	 *	is for abilities that only require a world location as their target 
	 *
	 *	@return - true if at least one selectable was issued a command
	 */
	bool IssueContextCommand(const FContextButtonInfo & AbilityInfo, const FVector & ClickLoc);

	/** 
	 *	Issue instant context commands on server. Handles displaying HUD warnings if no 
	 *	command is issued
	 *	
	 *	@param bFromServerPlayer - whether we are the server player
	 *	@return - true if at least one selectable was given a command 
	 */
	bool IssueInstantContextCommand(const FContextButton & Button, bool bFromServerPlayer);

	/** 
	 *	Carries out a commander ability, puts it on cooldown, etc. Assumes that all checks have 
	 *	been made an it is OK to carry it out.
	 *	
	 *	@param AbilityState - state struct for ability to carry out
	 *	@param AbilityInfo - info struct for AbilityState just as a convenience 
	 */
	void ExecuteCommanderAbility(FAquiredCommanderAbilityState * AbilityState, const FCommanderAbilityInfo & AbilityInfo);
	/** This version is for commander abilities that require targeting a player */
	void ExecuteCommanderAbility(FAquiredCommanderAbilityState * AbilityState, const FCommanderAbilityInfo & AbilityInfo, ARTSPlayerState * AbilityTarget);
	/* This version is for commander abilities that require targeting a selectable */
	void ExecuteCommanderAbility(FAquiredCommanderAbilityState * AbilityState, const FCommanderAbilityInfo & AbilityInfo, AActor * AbilityTarget, ISelectable * TargetAsSelectable);
	/* This version is for commander abilities that reuqire targeting a world location */
	void ExecuteCommanderAbility(FAquiredCommanderAbilityState * AbilityState, const FCommanderAbilityInfo & AbilityInfo, const FVector & AbilityLocation);
	/** 
	 *	This version is for commander abilities that can target either a world location or a selectable. 
	 *	Pass in the AActor* param as null to signal you are targeting the world location 
	 *	
	 *	@param TargetAsSelectable - selectable that is the target for the ability. Pass in null to 
	 *	signal you are targeting a world location instead 
	 */
	void ExecuteCommanderAbility(FAquiredCommanderAbilityState * AbilityState, const FCommanderAbilityInfo & AbilityInfo, 
		const FVector & AbilityLocation, AActor * AbilityTarget, ISelectable * TargetAsSelectable);

	/** RPCs to execute commander abilities */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestExecuteCommanderAbility(ECommanderAbility AbilityType);
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestExecuteCommanderAbility_PlayerTargeting(ECommanderAbility AbilityType, uint8 TargetsPlayerID);
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestExecuteCommanderAbility_SelectableTargeting(ECommanderAbility AbilityType, AActor * SelectableTarget);
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestExecuteCommanderAbility_LocationTargeting(ECommanderAbility AbilityType, const FVector_NetQuantize & AbilityLocation);
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestExecuteCommanderAbility_LocationOrSelectableTargeting_UsingSelectable(ECommanderAbility AbilityType, AActor * SelectableTarget);
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestExecuteCommanderAbility_LocationOrSelectableTargeting_UsingLocation(ECommanderAbility AbilityType, const FVector_NetQuantize & AbilityLocation);

	/* Cancel ability to issue context command that's pending. Do we want to cancel ghosts too? 
	
	I call this a lot right before switching to another context action. For performance consider 
	creating a function like ReplacePendingContextCommander(const FContextButtonInfo & NewAbility)
	+ overloads for inventory items and commander abilities too */
	void CancelPendingContextCommand();

	/* Cancel pending global skills panel ability */
	void CancelPendingGlobalSkillsPanelAbility();

	/* Call OnDeselect() on and remove all selectables in Selected. Clear out SelectedIDs */
	void RemoveSelection();

	/* Call OnDeselect() on all in Selected except param if it is in there,
	then empty Selected. Also completely clears out SelectedIDs
	@param Exclude - the selectable to not remove if it is in Seleceted */
	void RemoveSelection(const ISelectable * Exclude);

	/* World time when last selection of an owned selectable was made */
	float TimeOfLastSelection_Owned;

	/* All selected units/buildings. The selectable whose context menu will
	be shown in in index 0 */
	UPROPERTY()
	TArray <AActor *> Selected;

	/* Selected unit/building whose context menu is showing. This only needs
	to be updated on the client */
	UPROPERTY()
	TScriptInterface <ISelectable> CurrentSelected;

	/* Set of IDs of each selected selectable. Whenever selection changes this
	should be updated. Empty if selection is not owned */
	UPROPERTY()
	TSet <uint8> SelectedIDs;

	/* Array of selectable IDs for selectables that were last given orders. Used to
	reduce size of RPCs for commands by allowing a 'selection has not changed' signal
	to be sent instead of sending all the selectables' IDs */
	UPROPERTY()
	TArray <uint8> CommandIDs;

	/* This is mainly here to speed up performance in tick. It will be populated when 
	a non-issuedInstantly context button is pressed. It contains all the selectables 
	that are selected and have the PendingContextAction in their context menu. If 
	PendingContextAction is only ment to be issued to the CurrentSelected then it should 
	only contain the CurrentSelected (but that behavior has been depreciated). 
	Haven't decided whether we can assume entries are valid or not. Currently this is only 
	updated on a context button click therefore validity of each element will need to 
	be checked whenever accessing them
	This array is updated on clients (not just remote clients) so sending an RPC across the 
	wire that relies on this being up to date will not work. 
	May decide to get rid of this array. Main implication would be that now whenever we have a 
	2 click context command pending we will need to iterate the whole of Selected and check 
	which selectables support the action */
	UPROPERTY()
	TArray < AActor * > SelectedAndSupportPendingContextAction;

	/* Check if any units/buildings are selected
	@return - true if at least one object is selected */
	bool HasSelection() const;

	/* Returns true if multiple units are selected */
	bool HasMultipleSelected() const;

	/* Returns true if the player ID matches this player and
	therefore the selectable is owned by this player */
	bool IsControlledByThis(const FName OtherPlayerID) const;

	/* Returns true if an actor is controlled by this */
	bool IsControlledByThis(const AActor * Actor) const;

	/* Returns true if player has a selection and it is controlled by them */
	bool IsSelectionControlledByThis() const;

	/* Get raw ptr from TScriptInterface ptr. I use this because some funcs I have written 
	require ISelectable* and I cannot be arsed overloading them to allow TScriptInterface 
	aswell */
	ISelectable * ToSelectablePtr(const TScriptInterface<ISelectable> & Obj) const;

	/* Returns true if the player ID matches that of something
	that is neutral */
	bool IsNeutral(FName OtherPlayerID);

	/* Call when issuing a right click command
	@param ClickLoc - world space coords where right click was made
	@param Target - the actor that was right clicked on, or null if none */
	void OnRightClickCommand(const FVector & ClickLoc, AActor * Target);

	/** 
	 *	[Client] Put the right selectable IDs into the command struct 
	 *	
	 *	@param Command - command struct to fill with selectable IDs 
	 */
	void FillCommandWithIDs(FRightClickCommandBase & Command);

	/* @param InPrimarySelected - player's primary selected at the time the command was issued */
	void PlayRightClickCommandParticlesAndSound(const TScriptInterface<ISelectable> InPrimarySelected, 
		const FVector & ClickLoc, AActor * Target);

	/* Sends from client message to issue right click command */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_IssueRightClickCommand(const FRightClickCommandWithTarget & Command);

	/* Sends from client message to issue right click command. Assuming that the actor that 
	was clicked on was an inventory item */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_IssueRightClickCommandOnInventoryItem(const FRightClickCommandOnInventoryItem & Command);

	/* Issues a right click command to all selected. If the actor
	right clicked on is a selectable then pass a reference to them,
	otherwise pass null. All selectables should be replicated so
	while non-selectables may not. If a non-selectable reference is
	passed it will be null on server which is ok while selectable
	references should all be not null on server.
	@param Location - the world coords of the right click
	@param ClickedActor - the actor that was clicked on 
	@return - something not equal to "None" if a command was not issued */
	EGameWarning IssueRightClickCommand(const FVector & Location, AActor * ClickedActor);

	/* Reference to fog of war manager. On clients this only affects rendering.
	On server it affects rendering and also calculates which buildings/units
	can see what */
	UPROPERTY()
	AFogOfWarManager * FogOfWarManager;

	void SetupFogOfWarManager(ARTSLevelVolume * MapFogVolume, uint8 InNumTeams, ETeam InTeam);

	/* Spawn ghost buildings and add them to GhostPool */
	void SetupGhostPool();

	/* Set PC and GS references in player state. Can't find a BeginPlay
	type function in player state so I call this instead */
	void SetupExternalReferences();

	/* Set unique ID for player. APlayerState::PlayerId used to work in PIE but now it's always
	returning 0 */
	void SetupPlayerID(const FName & InPlayerID);

	/* Sets up HUD before match starts */
	void InitHUD(EFaction InFaction, EStartingResourceAmount InStartingResources,
		ARTSLevelVolume * FogVolume);

	/* Set the mouse cursor, either main menu or match one depending on if we are in main menu or
	match */
	void SetupMouseCursorProperties();

	/** 
	 *	Teleport to the location player should start match at. Also as a result sets the default 
	 *	yaw of the player's camera which is the yaw that it will be reset to when they press the 
	 *	'reset camera view' button
	 *
	 *	@param InStartingSpotID - the starting spot unique ID that they should teleport to. If -2 
	 *	then they weren't assigned a starting spot and the game could not find one for them usually 
	 *	because there aren't any left on map. 
	 *	@param MapID - the unique ID of the map we are going to play match on.
	 */
	void SetStartingSpotInMatch(int16 InStartingSpotID, FMapID MapID);

public:

	/* Was called Server_PostLogin for a long time. Sets up everything the controller needs
	to be function in a match. Should be called at some point when transitioning from lobby
	to match. Doesn't setup UI elements  */
	UFUNCTION()
	void Server_SetupForMatch(uint8 InPlayerID, ETeam InTeam, EFaction InFaction,
		EStartingResourceAmount StartingResources, uint8 NumTeams, FMapID InMapID, 
		int16 InStartingSpotID);

protected:

	/* Was called Client_PostLogin for a long time. Sets up everything the controller needs
	to be function in a match. Should be called at some point when transitioning from lobby
	to match. Called by Server_SetupForMatch */
	UFUNCTION(Client, Reliable)
	void Client_SetupForMatch(uint8 InPlayerID, ETeam InTeam, EFaction InFaction, uint8 NumTeams,
		EStartingResourceAmount StartingResources, FMapID InMapID, int16 InStartingSpotID);

public:

	/* Tell game state that Client_SetupForMatch has completed */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_AckSetupForMatchComplete();

	/* Called during game modes begin play */
	void Server_BeginPlay();

	/* Called by game state when PIE starts */
	UFUNCTION(Client, Reliable)
	void Client_OnPIEStarted();

	/* Called when a match from lobby starts. The specific time is actually after the 1
	second black screen ends */
	void OnMatchStarted();

	void NotifyOfBuildingReachingZeroHealth(ABuilding * Building, USoundBase * ZeroHealthSound,
		bool bWasSelected);

	/** 
	 *	Called by a selectable when it reaches zero health 
	 *	
	 *	@param Selectable - selectable that just reached zero health
	 *	@param bWasSelected - whether the selectable was selected by the player when it reached 
	 *	zero health 
	 */
	void NotifyOfInfantryReachingZeroHealth(AInfantry * Infantry, USoundBase * ZeroHealthSound, 
		bool bWasSelected);

	/** 
	 *	Called when an infantry enters a garrison 
	 *	
	 *	@param Infantry - infantry that entered garrison
	 *	@param bWasSelected - if the selectable is selected by this player 
	 */
	void NotifyOfInfantryEnteringGarrison(AInfantry * Infantry, bool bWasSelected);

	/** 
	 *	Called when an inventory item in the world is picked up. 
	 *	
	 *	@param InventoryItem - inventory item that was picked up 
	 *	@param bWasSelected - whether the inventory item was selected by this player controller 
	 */
	void NotifyOfInventoryItemBeingPickedUp(AInventoryItem * InventoryItem, bool bWasSelected);

	/* Unload a single unit from a garrison */
	void ServerUnloadSingleUnitFromGarrison(ABuilding * BuildingGarrison, AActor * SelectableToUnload);

	/* Unload all units from a garrison */
	void ServerUnloadGarrison(ABuilding * BuildingGarrison);

	/** 
	 *	Request unload a single unit from a garrison 
	 *	
	 *	@param SelectableToUnload - unit to remove from the garrison 
	 */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestUnloadSingleUnitFromGarrison(ABuilding * BuildingGarrison, AActor * SelectableToUnload);

	/** Request unload all units from a garrison */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestUnloadGarrison(ABuilding * BuildingGarrison);

protected:

	/* Removes a selectable from Selected. Useful to use when a selectable reaches zero health */
	template <typename TSelectable>
	void RemoveFromSelected(TSelectable * Selectable);

public:

	/** 
	 *	Called when the inventory of the primary selected swaps around indices for any reason, 
	 *	and it is also owned by this.
	 *	
	 *	@param ServerIndex_1 - one of the indices that was swapped 
	 *	@param ServerIndex_2 - one of the indices that was swapped
	 */
	void OnInventoryIndicesOfPrimarySelectedSwapped(uint8 ServerIndex_1, uint8 ServerIndex_2);

	/* Called when multiple the HUD does a marquee select.
	@param bDidSelectionChange - true if the marquee select changed the selection */
	void OnMarqueeSelect(bool bDidSelectionChange);

protected:

	void OnPersistentTabButtonLeftClicked(UHUDPersistentTabButton * Button);
	void OnPersistentTabButtonRightClicked(UHUDPersistentTabButton * Button);

	/** 
	 *	Called when context menu button is clicked.
	 *	
	 *	@param Button - the button that was clicked 
	 */
	void OnContextButtonClick(const FContextButton & Button);

	/** 
	 *	Called when a button on the global skills panel is left clicked 
	 */
	void OnGlobalSkillsPanelButtonLeftClicked(UGlobalSkillsPanelButton * ClickedButtonsUserWidget);

	/** 
	 *	Called when a button on the player targeting panel is left clicked 
	 *	
	 *	@param AbilityTarget - the player that corrisponds to the button. 
	 */
	void OnPlayerTargetingPanelButtonLeftClicked(ARTSPlayerState * AbilityTarget);

	/** 
	 *	Called when a inventory slot button is left clicked. 
	 *	
	 *	@param InventorySlot - inventory slot that was clicked 
	 *	@param SlotsServerIndex - index in FInventory::SlotsArray
	 */
	void OnInventorySlotButtonLeftClicked(FInventorySlotState & InventorySlot, uint8 SlotsServerIndex);

	/** 
	 *	Called when an inventory slot button is right clicked. Will likely be how items are sold 
	 *	back to shops.
	 *	
	 *	My notes: haven't really done anything for this 
	 */
	void OnInventorySlotButtonRightClicked(UInventoryItemButton * ButtonWidget);

	void OnShopSlotButtonLeftClicked(UItemOnDisplayInShopButton * ButtonWidget);

	/* This variable is for the unified mouse hovered/pressed effects. 
	If true then we a re showing the pressed image over some button */
	bool bShowingUIButtonPressedImage;

	void UnhoverHoveredUIElement();

public:
	
	bool ShouldIgnoreMouseHoverEvents() const;
	bool ShouldIgnoreButtonUpOrDownEvents() const;

	/**
	 *	@param UserWidget - if the UMyButton is a button that belongs to another UUserWidget 
	 *	button then this should point to that UUserWidget. Otherwise it should be null. 
	 *	Of course all UWidget are part of a UUserWidget, but that's not what this is refering to 
	 *	@return - true to let the hovered button know it should play a hovered sound 
	 */
	bool OnButtonHovered(UMyButton * HoveredWidget, const FGeometry & HoveredWidgetGeometry,
		const FSlateBrush * HoveredBrush, bool bUsingUnifiedHoverImage, UInGameWidgetBase * UserWidget);

	void OnButtonUnhovered(UMyButton * UnhoveredWidget, bool bUsingUnifiedHoverImage, UInGameWidgetBase * UserWidget);

	void ShowUIButtonHoveredImage(UMyButton * HoveredWidget, const FGeometry & HoveredWidgetGeometry,
		const FSlateBrush * HoveredBrush);

	/* Call to show a 'on pressed' version of a button by drawing an image widget overtop of
	it. This is for the mouse pressing something */
	void ShowUIButtonPressedImage_Mouse(SMyButton * PressedButtonWidget, const FSlateBrush * PressedBrush);
	void HideUIButtonPressedImage_Mouse(UMyButton * ButtonWidget, const FGeometry & HoveredWidgetGeometry, 
		const FSlateBrush * HoveredBrush, bool bUsingUnifiedHoverImage);

	/** 
	 *	Called whenever a menu warning happens. This is intended to be called for menus that 
	 *	can either be used both in the main menu or in a match e.g. settings menu 
	 */
	void OnMenuWarningHappened(EGameWarning WarningType);

protected:

	/** 
	 *	Called every time a menu warning happens. Return whether a message should be displayed 
	 *	on the HUD and a sound should be played
	 */
	bool ShouldShowMenuWarningMessage(EGameWarning WarningType);

public:

	/* My custom function that measures to the camera pawns root component instead
	of to the actual camera. Also excludes Z axis for distance calculations
	@param CameraShakeBP - blueprint for camera shake
	@param Epicenter - location to play camera shake. Z axis will be ignored
	@param OuterRadius - area around Location camera shake will affect
	@param Falloff - Falloff of shakiness */
	UFUNCTION(Client, Unreliable)
	void PlayCameraShake(TSubclassOf <UCameraShake> CameraShakeBP, const FVector & Epicenter, float OuterRadius, float Falloff);

	/** 
	 *	Request buying an item from a shop. 
	 *	
	 *	@param Shop - the shop we're trying to buy an item from 
	 *	@param ShopSlotIndex - what item from the shop we're trying to buy 
	 */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestBuyItemFromShop(AActor * Shop, uint8 ShopSlotIndex);

	/** 
	 *	Request selling item to a shop.
	 *	
	 *	@param SellerID - selectable ID of the selectable that wants to sell the item 
	 *	@param ItemInSlot - the type of item we're trying to sell. This is just here to try and 
	 *	reduce the number of times a client requests to sell something but the item was actually 
	 *	something different on the server.
	 */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SellInventoryItem(uint8 SellerID, uint8 InventorySlotIndex, EInventoryItem ItemInSlot);

protected:

	/* Pause the game if it is a single player game and show the pause menu */
	void PauseGameAndShowPauseMenu();

public:

	/* When going from pause menu back to playing */
	void ResumePlay();

	/* Just calls UGameplayStatics::SetGamePaused */
	static bool SetGamePaused(const UObject * WorldContextObject, bool bPaused);

	/** 
	 *	Called if local player levels up but only called for the last level gained if multiple 
	 *	were gained at once
	 *	
	 *	@param ExperienceRequiredForNextRank - how much experience is required to go from the current 
	 *	rank to the next rank. Does not change as experience is gained/lost 
	 */
	void OnLevelUp_LastForEvent(uint8 OldRank, uint8 NewRank, int32 NumUnspentSkillPoints, 
		float ExperienceTowardsNextRank, float ExperienceRequiredForNextRank);

	/** 
	 *	[Remote clients] Request gaining a rank in a commander skill. Could be the first rank 
	 *	for the ability or the 2nd, 3rd, etc 
	 *	
	 *	@param AllNodesArrayIndex - index in UCommanderSkilTreeWidget::AllNodes that the player 
	 *	wants to try gain. 
	 */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestAquireCommanderSkill(uint8 AllNodesArrayIndex);

public:

	//=============================================================
	//	Getters and setters
	//=============================================================

	/* Return true if placing ghost building */
	bool IsPlacingGhost() const;

	/* True if the marquee box should be drawn */
	bool IsMarqueeActive() const;

	/* Setup reference to spring arm of currently possessed camera */
	void SetupCameraReferences();

	EBuildingType GetGhostType() const;

	ETeam GetTeam() const;

	ARTSPlayerState * GetPS() const;
	void SetPS(ARTSPlayerState * NewPlayerState);

	ARTSGameState * GetGS() const;
	void SetGS(ARTSGameState * InGameState);

	AFactionInfo * GetFactionInfo() const;

	URTSGameInstance * GetGI() const;

	/* Get a reference to Selected */
	TArray <AActor *> & GetSelected();

	/* Get CurrentSelected - the selected who's context menu is showing */
	TScriptInterface <ISelectable> GetCurrentSelected() const;

	AObjectPoolingManager * GetPoolingManager() const;

	AFogOfWarManager * GetFogOfWarManager() const;

	URTSHUD * GetHUDWidget() const;

	/* Return true if ID is in SelectedIDs */
	bool IsSelected(uint8 SelectableID) const;
	void AddToSelectedIDs(uint8 NewID);
	void RemoveFromSelectedIDs(uint8 IDToRemove);

	/* Returns true if this player controller is for a player observing match but not actually
	playing */
	bool IsObserver() const;

	virtual void FailedToSpawnPawn() override;


	//=========================================================================================
	//	End of Match
	//=========================================================================================

public:

	/* Disables HUD input. Usually called when player is defeated or match has ended */
	void DisableHUD();


	//=========================================================================================
	//	In Match Messaging
	//=========================================================================================

protected:

	/* Returns true if the chat input widget is showing */
	bool IsChatInputOpen() const;

	/* Shows the chat input widget
	@param MessageRecipients - the group intended to receive the message */
	void OpenChatInput(EMessageRecipients MessageRecipients);

	/* Hides the chat input widget */
	void CloseChatInput();

	/* What player name to use when we can't figure out who sent the message */
	static const FString UNKNOWN_PLAYER_NAME;

public:

	/* Send message to server to then send to everyone in the match
	@param Message - the message user typed without any extra identifiers tagged on */
	UFUNCTION(Server, Unreliable, WithValidation)
	void Server_SendInMatchChatMessageToEveryone(const FString & Message);

	/* Send message to server to then send to everyone on your team
	@param Message - the message user typed without any extra identifiers tagged on */
	UFUNCTION(Server, Unreliable, WithValidation)
	void Server_SendInMatchChatMessageToTeam(const FString & Message);

	/* When an "all chat" message is received from server */
	UFUNCTION(Client, Reliable)
	void Client_OnAllChatInMatchChatMessageReceived(ARTSPlayerState * Sender, const FString & Message);

	/* When a "team only" chat message is received from server */
	UFUNCTION(Client, Reliable)
	void Client_OnTeamChatInMatchChatMessageReceived(ARTSPlayerState * Sender, const FString & Message);


	//=========================================================================================
	//	Lobby
	//=========================================================================================
	// To reduce bandwidth these RPCs are here, but game state is a good place for them otherwise

protected:

	/* Slot in lobby this player is in */
	uint8 LobbySlotIndex;

public:

	uint8 GetLobbySlotIndex() const;

	/** 
	 *	Called during GM::PostLogin but only on the host's player controller. Sets lobby index 
	 *	to 0
	 */
	void SetLobbySlotIndexForHost();
	
	/**
	 *	This is called during AGameMode::PostLogin when we joined but no open lobby slots were 
	 *	available. This should hopefully never be called
	 */
	UFUNCTION(Client, Reliable)
	void Client_OnTryJoinButLobbyFull();

	/** 
	 *	This is called during AGameMode::PostLogin when we have successfully joined into the 
	 *	lobby. It will allow us to show our lobby widget 
	 */
	UFUNCTION(Client, Reliable)
	void Client_ShowLobbyWidget(uint8 OurLobbySlot, EFaction OurDefaultFaction);

	/* Change our team in lobby */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_ChangeTeamInLobby(ETeam NewTeam);

	/* Change our faction in lobby */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_ChangeFactionInLobby(EFaction NewFaction);

	/**
	 *	Change starting spot on map in lobby. 
	 *	@param PlayerStartID - unique ID of player start being requested to change to. -1 = unassign 
	 */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_ChangeStartingSpotInLobby(int16 PlayerStartID);

	// Note: does not need to be called for server player
	UFUNCTION(Client, Reliable)
	void Client_OnChangeTeamInLobbySuccess(ETeam NewTeam);

	// Note: does not need to be called for server player
	UFUNCTION(Client, Reliable)
	void Client_OnChangeFactionInLobbySuccess(EFaction NewFaction);

	// Note: does not need to be called for server player
	UFUNCTION(Client, Reliable)
	void Client_OnChangeStartingSpotInLobbySuccess(int16 PlayerStartID);

	/* Called when lock slots was in place of server but hadn't repped to client in time, therefore
	they could change their team selection
	@param TeamServerSide - the players team server-side which is what their team will be restored
	to on their client */
	UFUNCTION(Client, Reliable)
	void Client_OnChangeTeamInLobbyFailed(ETeam TeamServerSide);

	/* Called when lock slots was in place on server but hadn't repped to client in time, therefore
	they could change their faction selection */
	UFUNCTION(Client, Reliable)
	void Client_OnChangeFactionInLobbyFailed(EFaction FactionServerSide);

	/* @param PlayerStartID - the player start to have client set. If -1 then do not set a 
	player start point */
	UFUNCTION(Client, Reliable)
	void Client_OnChangeStartingSpotInLobbyFailed(int16 PlayerStartID);

	/* Send chat message in lobby */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SendLobbyChatMessage(const FString & Message);


	//=========================================================================================
	//	Match Initialization
	//=========================================================================================

public:

	/* Here because of RPCs and ownership. Send ack to GS server-side to let them know that a
	player state has setup completely for match */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_AckPSSetupOnClient();

	/* Acknowledge level has streamed in for match */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_AckLevelStreamedIn();


	//=========================================================================================
	//	Controls Settings
	//=========================================================================================

	/* Functions */

protected:

	void SetupCameraCurves();

	// Assign viewport size values
	void AssignViewportValues();

	/* Variables */

protected:

	/* All these variables default values will be defined in URTSGameUserSettings.cpp in
	the constructor */

	/* Camera move speed when using keyboard. Limited by camera's movement component's
	max speed, which I've just set really big */
	float CameraMoveSpeed;

	/* Camera move speed when moving mouse to edge of screen. Limited by camera's movement
	component's max speed, which I've just set really big */
	float CameraEdgeMoveSpeed;

	/* How close to the edge the mouse has to be to allow movement
	0.05 = within 5% of screen edge */
	float CameraEdgeThreshold;

	/* How much  to zoom in/out by when scrolling mouse wheel */
	float CameraZoomIncrementalAmount;

	/* Curve for zooming camera */
	UPROPERTY()
	UCurveFloat * CameraMouseWheelZoomCurve;

	/* Speed camera zooms in/out */
	float CameraZoomSpeed;

	/* Positive means pending zoom out, negative means pending zoom in. 0 means stop zooming */
	int32 NumPendingScrollZooms;

	/* The rotation to set to camera at start of match and also the rotation to return to when
	pressing a 'return to default rotation' button */
	FRotator DefaultCameraRotation;

	/* The zoom amount to start the match at and also the zoom amount to return to when pressing
	a 'return to default zoom' button */
	float DefaultCameraZoomAmount;

	/** 
	 *	Curve to use to transition from current camera rotation/zoom to defaults. Can be left
	 *	null and transition will happen instantly
	 *	X axis = base time to complete zoom. Will be multiplied by ResetCameraToDefaultRate. Good to
	 *	make it at least from 0 to 1 and adjust ResetCameraToDefaultRate to get desired time taken
	 *	Y axis = Lerp amount, range should be from 0 to 1 
	 */
	UCurveFloat * ResetCameraCurve;

	/* Speed camera resets to default rotation/zoom */
	float ResetCameraToDefaultRate;

public:

	/* Set the speed to move the camera when using keyboard */
	void SetCameraKeyboardMoveSpeed(float NewValue);

	/* Set speed to move the camera when moving mouse to edge of screen */
	void SetCameraMouseMoveSpeed(float NewValue);

	/* Set max move speed of camera */
	void SetCameraMaxSpeed(float NewValue);

	/* Set camera acceleration for any movement */
	void SetCameraAcceleration(float NewValue);

	/* Set whether to use camera movement lag or not */
	void SetEnableCameraMovementLag(bool bIsEnabled);

	/* Set the speed the camera lags behind movement */
	void SetCameraMovementLagSpeed(float NewValue);

	/* Set how fast camera changes direction with regards to WASD and mouse edge movement */
	void SetCameraTurningBoost(float NewValue);

	/* Set how fast camera stops */
	void SetCameraDeceleration(float NewValue);

	/* Set how close to edge of screen mouse has to be to move camera */
	void SetCameraEdgeMovementThreshold(float NewValue);

	/* Set how much to zoom out by when scrolling mouse wheel */
	void SetCameraZoomIncrementalAmount(float NewValue);

	/* Set speed camera zooms in/out at */
	void SetCameraZoomSpeed(float NewValue);

	/* Set the sensitivity on X axis when looking around with MMB held */
	void SetMMBLookYawSensitivity(float NewValue);

	/* Set the sensitivity on Y axis when looking around with MMB held */
	void SetMMBLookPitchSensitivity(float NewValue);

	/* Whether to invert the MMB free looks X axis */
	void SetInvertMMBYawLook(bool bInvert);

	/* Whether to invert the MMB free looks Y axis */
	void SetInvertMMBPitchLook(bool bInvert);

	/* Whether MMB look around lags. True = lags, false = updates instantly */
	void SetEnableMMBLookLag(bool bEnableLag);

	/* Set amount of MMB look around lag */
	void SetMMBLookLagAmount(float NewValue);

	void SetMouseMovementThreshold(float NewValue);

protected:

	/* Set what the default camera yaw should be. This will be set on a per-map basis and is
	not something the user can adjust in their in-game settings */
	void SetDefaultCameraYaw(float NewValue);

public:

	/* Set what the default camera pitch should be */
	void SetDefaultCameraPitch(float NewValue);

	/* Set what camera zoom to have at start of match */
	void SetDefaultCameraZoomAmount(float NewValue);

	/* Set speed camera resets to default rotation/zoom */
	void SetResetCameraToDefaultRate(float NewValue);

	void SetDoubleClickTime(float NewValue);

	//=================================================================
	//	Ghost Building

	/* Set how far mouse has to move for rotation is possible */
	void SetGhostRotationRadius(float NewValue);

	/* Set rate ghost rotates */
	void SetGhostRotationSpeed(float NewValue);


	//-------------------------------------------------
	//	Control related but not actual settings
	//-------------------------------------------------

protected:

	/* Some of these variables are because I do not use FTimeline so they're the start and target
	values + accumulated time along X axis */

	// Size of viewport in pixels (I think) but as a float
	float ViewportSize_X, ViewportSize_Y;

	// By default these are the WASD keys but can be changed 
	bool bIsLeftRightCameraMovementKeyPressed, bIsForwardBackwardCameraMovementKeyPressed;

	/* True if 'reset camera rotation' button was pressed recently. Tell mouse movement func
	to ignore changing control rotation */
	bool bIsResettingCameraRotation;

	/* Amount of time through CameraMouseWheelZoomCurve */
	float MouseWheelZoomCurveAccumulatedTime;

	// X axis values I presume
	float MouseWheelZoomCurveMin, MouseWheelZoomCurveMax;

	/* True if 'reset camera zoom' button was pressed recently. Tell mouse wheel func to
	ignore zooming */
	bool bIsResettingCameraZoom;

	/* Amount of time through ResetCameraCurve */
	float ResetCameraCurveAccumulatedTime;

	float ResetCameraCurveMin, ResetCameraCurveMax;

	/* Control rotation when request to reset rotation was made so lerp can work */
	FRotator StartResetRotation;

	/* The rotation camera is trying to reset to. If this equals current control rotation then
	no change needs to be done */
	FRotator TargetResetRotation;

	/* Camera zoom when request to change zoom is initiated. Here os lerp can work */
	float StartZoomAmount;

	/* Same as above but for camera zoom */
	float TargetZoomAmount;


	//==========================================================================================
	//	Development Cheat Widget
	//==========================================================================================

protected:

	//==========================================================================================
	//	Variables

#if WITH_EDITORONLY_DATA

	/* Not sure if WITH_EDITOR or WITH_EDITORONLY_DATA is the right thing to use here. Will see 
	whether stuff compiles and works */
	UPROPERTY()
	UInMatchDeveloperWidget * DevelopmentCheatWidget;

	bool Development_bIsLMBPressed;

	EDevelopmentAction DevelopmentInputIntercept_LMBPress;
	EDevelopmentAction DevelopmentInputIntercept_LMBRelease;
	EDevelopmentAction DevelopmentInputIntercept_RMBPress;

	/* Auxillery data that may be needed by the action e.g. an item type if we are giving an 
	item to a selectable */
	int32 DevelopmentWidgetAuxilleryData;

#endif // WITH_EDITORONLY_DATA

public:

#if WITH_EDITOR
	/* Requests an action to be carried out, usually on the next relevant input */
	void OnDevelopmentWidgetRequest(EDevelopmentAction Action);

	/* This version aslo takes some auxillery data */
	void OnDevelopmentWidgetRequest(EDevelopmentAction Action, int32 InAuxilleryData);

protected:

	/**
	 *	All of these functions run on an input
	 *
	 *	@return - true if an action is carried out
	 */
	bool ExecuteDevelopmentInputInterceptAction_LMBPress();
	bool ExecuteDevelopmentInputInterceptAction_LMBRelease();
	bool ExecuteDevelopmentInputInterceptAction_RMBPress();
#endif

	// Would like to use the preprocessor but UHT does not allow
//#if WITH_EDITORONLY_DATA
	/* Request server deal default damage to a selectable */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_DealDamageToSelectable(AActor * DamageInstigator, AActor * Target, float DamageAmount);

	/* Award experience to a selectable */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_AwardExperience(AActor * ExperienceGainer, float ExperienceAmount);

	/* Award experience to a player */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_AwardExperienceToPlayer();

	/* Give an inventory item to a selectable */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_GiveInventoryItem(AActor * ItemRecipient, EInventoryItem ItemType);
//#endif 

	//	End development cheat widget
	//===========================================================================================
};
