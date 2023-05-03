// Fill out your copyright notice in the Description page of Project Settings.

#include "RTSHUD.h"
#include "Components/MultiLineEditableTextBox.h"
#include "Components/MultiLineEditableText.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Blueprint/WidgetTree.h"
#include "Components/WidgetSwitcher.h"
#include "Public/TimerManager.h"
#include "Components/Overlay.h"
#include "Components/ButtonSlot.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h" 
#include "Components/BrushComponent.h"
#include "Public/EngineUtils.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Animation/WidgetAnimation.h"
#include "Widgets/SViewport.h"
#include "Components/EditableTextBox.h"
#include "Engine/Texture2D.h"
#include "Kismet/GameplayStatics.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"

#include "GameFramework/RTSPlayerController.h"
#include "GameFramework/RTSGameInstance.h"
#include "GameFramework/Selectable.h"
#include "GameFramework/RTSPlayerState.h"
#include "GameFramework/FactionInfo.h"
#include "Managers/ObjectPoolingManager.h"
#include "MapElements/Building.h"
#include "Managers/UpgradeManager.h"
#include "Statics/Statics.h"
#include "Statics/DevelopmentStatics.h"
#include "MapElements/Invisible/RTSLevelVolume.h"
#include "GameFramework/RTSGameState.h"
#include "Settings/ProjectSettings.h"
#include "UI/BuffAndDebuffWidgets.h"
#include "MapElements/Infantry.h" // Because compile from editor, try removing and seeing if compile
#include "UI/InMatchWidgets/Components/MyButton.h"
#include "UI/InMatchWidgets/Components/MyButtonSlot.h"
#include "MapElements/InventoryItem.h"
#include "UI/InMatchWidgets/GlobalSkillsPanel.h"
#include "UI/UIUtilities.h"
#include "UI/InMatchWidgets/CommanderSkillTreeWidget.h"
#include "UI/InMatchWidgets/PlayerTargetingPanel.h"
#include "UI/InMatchWidgets/PauseMenu.h"
#include "UI/InMatchWidgets/GlobalSkillsPanelButton.h"
#include "UI/InMatchWidgets/CommanderSkillTreeNodeWidget.h"
#include "UI/MainMenuAndInMatch/MenuOutputWidget.h"


//==============================================================================================
//	HUD
//==============================================================================================

const FText URTSHUD::PALETTE_CATEGORY = NSLOCTEXT("HUD", "Palette Category", "RTS");
const FText URTSHUD::BLANK_TEXT = FText::GetEmpty();

//-------------------------------------------------------------------
//	Widget anim FNames
//-------------------------------------------------------------------
const FName URTSHUD::SkillTreeWantsOpeningAnimName = FName("SkillTreeWantsOpeningAnim_INST");
//-------------------------------------------------------------------

URTSHUD::URTSHUD(const FObjectInitializer & ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Default value bottom right corner
	InventoryItemTooltipWidgetAnchor = FAnchors(1.f, 1.f, 1.f, 1.f);
	ActionBarAndPersistentPanelTooltipAnchor = FAnchors(1.f, 1.f, 1.f, 1.f);
	GlobalSkillPanelButtonTooltipAnchor = FAnchors(1.f, 1.f, 1.f, 1.f);
	CommandersSkillTreeNodeTooltipAnchor = FAnchors(1.f, 1.f, 1.f, 1.f);
	CommanderSkillTreeAnimPlayRule = ECommanderSkillTreeAnimationPlayRule::CanAffordAbilityAtStartOfMatchOrLevelUp;
}

bool URTSHUD::SetupWidget(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController)
{
	if (Super::SetupWidget(InGameInstance, InPlayerController))
	{
		return true;
	}

	SetupBindings();

	SetupBoundWidgets();

	if (GI->UsingAtLeastOneUnifiedMouseFocusImage())
	{
		/* Create a UImage widget we use to give our mouse hover/pressed highlight effects */
		SpawnMouseHoverAndPressedWidget();
	}

	SetupCommanderSkillTreeWidget();
	
	SetupPlayerTargetingWidget();

	SetupPauseMenu();

	SetupMenuOutputWidget();

	SpawnTooltipWidgets();

	return false;
}

void URTSHUD::SetupMinimap(ARTSLevelVolume * FogVolume)
{
	assert(FogVolume != nullptr);

	if (IsWidgetBound(Widget_Minimap))
	{
		Widget_Minimap->Setup(FogVolume);
	}
}

void URTSHUD::NativeTick(const FGeometry & MyGeometry, float InDeltaTime)
{
	/* Bear in mind this does not call UUserWidget::NativeTick but UInGameWidgetBase::NativeTick
	which essentialy does nothing. This means widget animations and latent actions are disabled.
	This is all for performance. */
	Super::NativeTick(MyGeometry, InDeltaTime);
	
	/* Need this because I have an animation SkillTreeWantsOpeningAnim */
	TickActionsAndAnimation(MyGeometry, InDeltaTime);

	/* The behavior here is no widget does anything on its native tick and instead this HUD widget
	will tell them to tick. There's really no reason to do this - each subwidget could just
	override their native tick and have their logic in there. Advantage is tick order can be
	defined but I have nothing dependent on order anyway */

	/* Tick all subwidgets that need ticking */
	if (IsWidgetBound(Widget_ContextMenu))
	{
		Widget_ContextMenu->OnTick(InDeltaTime);
	}
	if (IsWidgetBound(Widget_Resources))
	{
		Widget_Resources->OnTick(InDeltaTime);
	}
	if (IsWidgetBound(Widget_PersistentPanel))
	{
		Widget_PersistentPanel->OnTick(InDeltaTime);
	}
}

void URTSHUD::SetupBindings()
{
	/* Setup button functionality */
	if (IsWidgetBound(Button_PauseMenu)) 
	{
		Button_PauseMenu->SetPC(PC);
		Button_PauseMenu->SetOwningWidget();
		Button_PauseMenu->SetPurpose(EUIElementType::NoTooltipRequired, nullptr);
		Button_PauseMenu->GetOnLeftMouseButtonPressDelegate().BindUObject(this, &URTSHUD::UIBinding_OnPauseMenuButtonLeftMousePress);
		Button_PauseMenu->GetOnLeftMouseButtonReleasedDelegate().BindUObject(this, &URTSHUD::UIBinding_OnPauseMenuButtonLeftMouseReleased);
		Button_PauseMenu->GetOnRightMouseButtonPressDelegate().BindUObject(this, &URTSHUD::UIBinding_OnPauseMenuButtonRightMousePress);
		Button_PauseMenu->GetOnRightMouseButtonReleasedDelegate().BindUObject(this, &URTSHUD::UIBinding_OnPauseMenuButtonRightMouseReleased);
	}
	if (IsWidgetBound(Button_CommanderSkillTree))
	{
		Button_CommanderSkillTree->SetPC(PC);
		Button_CommanderSkillTree->SetOwningWidget();
		Button_CommanderSkillTree->SetPurpose(EUIElementType::NoTooltipRequired, nullptr);
		Button_CommanderSkillTree->GetOnLeftMouseButtonPressDelegate().BindUObject(this, &URTSHUD::UIBinding_OnCommanderSkillTreeButtonLeftMousePress);
		Button_CommanderSkillTree->GetOnLeftMouseButtonReleasedDelegate().BindUObject(this, &URTSHUD::UIBinding_OnCommanderSkillTreeButtonLeftMouseReleased);
		Button_CommanderSkillTree->GetOnRightMouseButtonPressDelegate().BindUObject(this, &URTSHUD::UIBinding_OnCommanderSkillTreeButtonRightMousePress);
		Button_CommanderSkillTree->GetOnRightMouseButtonReleasedDelegate().BindUObject(this, &URTSHUD::UIBinding_OnCommanderSkillTreeButtonRightMouseReleased);
	}
}

void URTSHUD::SetupBoundWidgets()
{
	if (IsWidgetBound(Widget_Resources))
	{
		Widget_Resources->SetupWidget(GI, PC);
	}
	if (IsWidgetBound(Widget_Minimap))
	{
		Widget_Minimap->SetupWidget(GI, PC);
	}
	if (IsWidgetBound(Widget_ChatInput))
	{
		Widget_ChatInput->SetupWidget(GI, PC);
	}
	if (IsWidgetBound(Widget_ChatOutput))
	{
		Widget_ChatOutput->SetupWidget(GI, PC);
	}
	if (IsWidgetBound(Widget_GameOutput))
	{
		Widget_GameOutput->SetupWidget(GI, PC);
	}
	if (IsWidgetBound(Widget_ContextMenu))
	{
		Widget_ContextMenu->SetupWidget(GI, PC);
	}
	if (IsWidgetBound(Widget_SelectedGarrisonInfo))
	{
		Widget_SelectedGarrisonInfo->SetupWidget(GI, PC);
	}
	if (IsWidgetBound(Widget_PersistentPanel))
	{
		Widget_PersistentPanel->SetupWidget(GI, PC);
	}
	if (IsWidgetBound(Widget_GlobalSkillsPanel))
	{
		Widget_GlobalSkillsPanel->SetupWidget(GI, PC);
	}

	/* Set values on these because it's likely they weren't set to the right values in editor */
	if (IsWidgetBound(Text_CommandersRank))
	{
		Text_CommandersRank->SetText(FText::AsNumber(PS->GetRank()));
	}
	if (IsWidgetBound(Text_CommandersUnspentSkillPoints))
	{
		Text_CommandersUnspentSkillPoints->SetText(FText::AsNumber(PS->GetNumUnspentSkillPoints()));
	}
	if (IsWidgetBound(Text_CommandersExperienceProgress))
	{
		Text_CommandersExperienceProgress->SetText(GetPlayerExperienceAquiredAsText(0.f));
	}
	if (IsWidgetBound(Text_CommandersExperienceRequired))
	{
		if (PS->GetRank() < LevelingUpOptions::MAX_COMMANDER_LEVEL)
		{
			Text_CommandersExperienceRequired->SetText(GetPlayerExperienceRequiredAsText(FI->GetCommanderLevelUpInfo(PS->GetRank() + 1).GetExperienceRequired()));
		}
	}
	if (IsWidgetBound(ProgressBar_CommandersExperienceProgress))
	{
		ProgressBar_CommandersExperienceProgress->SetPercent(0.f);
	}

	OnPlayerNoSelection();
}

void URTSHUD::SpawnMouseHoverAndPressedWidget()
{
	MouseFocusImage = WidgetTree->ConstructWidget<UImage>();
	// Might need more here, will see
	
	// Assumption that root widget is a canvas panel
	MouseFocusImageSlot = CastChecked<UCanvasPanel>(GetRootWidget())->AddChildToCanvas(MouseFocusImage);

	MouseFocusImage->SetVisibility(ESlateVisibility::Hidden);
}

void URTSHUD::SetupCommanderSkillTreeWidget()
{
	TSubclassOf<UCommanderSkillTreeWidget> WidgetBlueprint = nullptr;
	if (FI->GetMatchWidgetBP(EMatchWidgetType::CommanderSkillTree_Ver2) != nullptr)
	{
		WidgetBlueprint = FI->GetMatchWidgetBP(EMatchWidgetType::CommanderSkillTree_Ver2);
	}
	else if (GI->IsMatchWidgetBlueprintSet(EMatchWidgetType::CommanderSkillTree_Ver2))
	{
		WidgetBlueprint = GI->GetMatchWidgetBP(EMatchWidgetType::CommanderSkillTree_Ver2);
	}

	if (WidgetBlueprint != nullptr)
	{
		if (CommanderSkillTreeAnimPlayRule != ECommanderSkillTreeAnimationPlayRule::Never)
		{
			// Find the widget anim 
			SkillTreeWantsOpeningAnim = UIUtilities::FindWidgetAnim(this, SkillTreeWantsOpeningAnimName);
		}
		
		if (SkillTreeWantsOpeningAnim == nullptr)
		{
			// Set this since there is no anim 
			CommanderSkillTreeAnimPlayRule = ECommanderSkillTreeAnimationPlayRule::Never;
		}

		CommanderSkillTreeWidget = CreateWidget<UCommanderSkillTreeWidget>(PC, WidgetBlueprint);
		CommanderSkillTreeWidget->SetVisibility(ESlateVisibility::Hidden); // Just copied what I did before, check if needed
		CommanderSkillTreeWidget->SetupWidget(GI, PC);
		
		UCanvasPanelSlot * CanvasSlot = CastChecked<UCanvasPanel>(GetRootWidget())->AddChildToCanvas(CommanderSkillTreeWidget);
		CanvasSlot->SetZOrder(HUDStatics::COMMANDER_SKILL_TREE_Z_ORDER);
		const bool bShouldPlayOpenAnim = CommanderSkillTreeWidget->MoreSetup(PS, FI, CommanderSkillTreeAnimPlayRule);
 
		if (bShouldPlayOpenAnim)
		{
			PlayAnimation(SkillTreeWantsOpeningAnim, 0.f, 0); // Looping
		}
	}
}

void URTSHUD::SetupPlayerTargetingWidget()
{
	if (PlayerTargetingPanel_BP != nullptr)
	{
		PlayerTargetingPanel = CreateWidget<UPlayerTargetingPanel>(PC, PlayerTargetingPanel_BP);
		PlayerTargetingPanel->SetVisibility(ESlateVisibility::Hidden);
		PlayerTargetingPanel->SetupWidget(GI, PC);

		UCanvasPanelSlot * CanvasSlot = CastChecked<UCanvasPanel>(GetRootWidget())->AddChildToCanvas(PlayerTargetingPanel);
		CanvasSlot->SetZOrder(HUDStatics::PLAYER_TARGETING_PANEL_Z_ORDER);
		
		/* Note: this gets it part of the way set up but there's still some more setup that 
		will happen later: PerformPlayerTargetingPanelFinalSetup will be called later */
	}
}

void URTSHUD::PerformPlayerTargetingPanelFinalSetup()
{
	if (PlayerTargetingPanel != nullptr)
	{
		PlayerTargetingPanel->MoreSetup();
	}
}

void URTSHUD::SetupPauseMenu()
{
	TSubclassOf<UInGameWidgetBase> Blueprint = nullptr;
	if (FI->GetMatchWidgetBP(EMatchWidgetType::PauseMenu) != nullptr)
	{
		Blueprint = FI->GetMatchWidgetBP(EMatchWidgetType::PauseMenu);
	}
	else if (GI->IsMatchWidgetBlueprintSet(EMatchWidgetType::PauseMenu))
	{
		Blueprint = GI->GetMatchWidgetBP(EMatchWidgetType::PauseMenu);
	}

	if (Blueprint != nullptr)
	{
		PauseMenu = CreateWidget<UPauseMenu>(PC, Blueprint);
		PauseMenu->SetVisibility(ESlateVisibility::Hidden);

		UCanvasPanelSlot * CanvasSlot = CastChecked<UCanvasPanel>(GetRootWidget())->AddChildToCanvas(PauseMenu);
		CanvasSlot->SetZOrder(HUDStatics::PAUSE_MENU_Z_ORDER);
		PauseMenu->SetupWidget(GI, PC);
	}
}

void URTSHUD::SetupMenuOutputWidget()
{
	if (MenuOutputWidget_BP != nullptr)
	{
		MenuOutputWidget = CreateWidget<UMenuOutputWidget>(PC, MenuOutputWidget_BP);
		MenuOutputWidget->SetVisibility(ESlateVisibility::Hidden);
		UCanvasPanelSlot * CanvasSlot = CastChecked<UCanvasPanel>(GetRootWidget())->AddChildToCanvas(MenuOutputWidget);
		CanvasSlot->SetZOrder(HUDStatics::MENU_OUTPUT_Z_ORDER);
		MenuOutputWidget->InitialSetup(PC);
	}
}

void URTSHUD::SpawnTooltipWidgets()
{
	if (TooltipWidget_InventoryItem_BP != nullptr)
	{
		TooltipWidget_InventoryItem = CreateWidget<UTooltipWidget_InventoryItem>(PC, TooltipWidget_InventoryItem_BP);
		TooltipWidget_InventoryItem->SetVisibility(ESlateVisibility::Hidden);

		TooltipWidget_InventoryItem->InitialSetup();

		UCanvasPanelSlot * CanvasSlot = CastChecked<UCanvasPanel>(GetRootWidget())->AddChildToCanvas(TooltipWidget_InventoryItem);
		CanvasSlot->SetZOrder(HUDStatics::TOOLTIP_Z_ORDER);
		CanvasSlot->SetAnchors(InventoryItemTooltipWidgetAnchor);
	}

	if (TooltipWidget_Abilities_BP != nullptr)
	{
		TooltipWidget_Abilities = CreateWidget<UTooltipWidget_Ability>(PC, TooltipWidget_Abilities_BP);
		TooltipWidget_Abilities->SetVisibility(ESlateVisibility::Hidden);

		UCanvasPanelSlot * CanvasSlot = CastChecked<UCanvasPanel>(GetRootWidget())->AddChildToCanvas(TooltipWidget_Abilities);
		CanvasSlot->SetZOrder(HUDStatics::TOOLTIP_Z_ORDER);
		CanvasSlot->SetAnchors(ActionBarAndPersistentPanelTooltipAnchor);
	}

	if (TooltipWidget_BuildBuilding_BP != nullptr)
	{
		TooltipWidget_BuildBuilding = CreateWidget<UTooltipWidget_Production>(PC, TooltipWidget_BuildBuilding_BP);
		TooltipWidget_BuildBuilding->SetVisibility(ESlateVisibility::Hidden);

		UCanvasPanelSlot * CanvasSlot = CastChecked<UCanvasPanel>(GetRootWidget())->AddChildToCanvas(TooltipWidget_BuildBuilding);
		CanvasSlot->SetZOrder(HUDStatics::TOOLTIP_Z_ORDER);
		CanvasSlot->SetAnchors(ActionBarAndPersistentPanelTooltipAnchor);
	}

	if (TooltipWidget_TrainUnit_BP != nullptr)
	{
		TooltipWidget_TrainUnit = CreateWidget<UTooltipWidget_Production>(PC, TooltipWidget_TrainUnit_BP);
		TooltipWidget_TrainUnit->SetVisibility(ESlateVisibility::Hidden);

		UCanvasPanelSlot * CanvasSlot = CastChecked<UCanvasPanel>(GetRootWidget())->AddChildToCanvas(TooltipWidget_TrainUnit);
		CanvasSlot->SetZOrder(HUDStatics::TOOLTIP_Z_ORDER);
		CanvasSlot->SetAnchors(ActionBarAndPersistentPanelTooltipAnchor);
	}

	if (TooltipWidget_ResearchUpgrade_BP != nullptr)
	{
		TooltipWidget_ResearchUpgrade = CreateWidget<UTooltipWidget_Production>(PC, TooltipWidget_ResearchUpgrade_BP);
		TooltipWidget_ResearchUpgrade->SetVisibility(ESlateVisibility::Hidden);

		UCanvasPanelSlot * CanvasSlot = CastChecked<UCanvasPanel>(GetRootWidget())->AddChildToCanvas(TooltipWidget_ResearchUpgrade);
		CanvasSlot->SetZOrder(HUDStatics::TOOLTIP_Z_ORDER);
		CanvasSlot->SetAnchors(ActionBarAndPersistentPanelTooltipAnchor);
	}

	if (TooltipWidget_GlobalSkillPanelButton_BP != nullptr)
	{
		TooltipWidget_GlobalSkillsPanelButton = CreateWidget<UTooltipWidget_GlobalSkillsPanelButton>(PC, TooltipWidget_GlobalSkillPanelButton_BP);
		TooltipWidget_GlobalSkillsPanelButton->SetVisibility(ESlateVisibility::Hidden);

		UCanvasPanelSlot * CanvasSlot = CastChecked<UCanvasPanel>(GetRootWidget())->AddChildToCanvas(TooltipWidget_GlobalSkillsPanelButton);
		CanvasSlot->SetZOrder(HUDStatics::TOOLTIP_Z_ORDER);
		CanvasSlot->SetAnchors(GlobalSkillPanelButtonTooltipAnchor);
	}

	if (TooltipWidget_CommanderSkillTreeNode_BP != nullptr)
	{
		TooltipWidget_CommanderSkillTreeNode = CreateWidget<UTooltipWidget_CommanderSkillTreeNode>(PC, TooltipWidget_CommanderSkillTreeNode_BP);
		TooltipWidget_CommanderSkillTreeNode->SetVisibility(ESlateVisibility::Hidden);
		
		TooltipWidget_CommanderSkillTreeNode->InitialSetup(PS);

		UCanvasPanelSlot * CanvasSlot = CastChecked<UCanvasPanel>(GetRootWidget())->AddChildToCanvas(TooltipWidget_CommanderSkillTreeNode);
		CanvasSlot->SetZOrder(HUDStatics::TOOLTIP_Z_ORDER);
		CanvasSlot->SetAnchors(CommandersSkillTreeNodeTooltipAnchor);
	}
}

FText URTSHUD::GetPlayerExperienceAquiredAsText(float ExperienceAquired)
{
	FloatToText(ExperienceAquired, UIOptions::PlayerExperienceGainedDisplayMethod);
}

FText URTSHUD::GetPlayerExperienceRequiredAsText(float ExperienceRequired)
{
	FloatToText(ExperienceRequired, UIOptions::PlayerExperienceRequiredDisplayMethod);
}

void URTSHUD::UIBinding_OnPauseMenuButtonLeftMousePress()
{
	PC->OnLMBPressed_PauseGame(Button_PauseMenu);
}

void URTSHUD::UIBinding_OnPauseMenuButtonLeftMouseReleased()
{
	PC->OnLMBReleased_PauseGame(Button_PauseMenu);
}

void URTSHUD::UIBinding_OnPauseMenuButtonRightMousePress()
{
	PC->OnRMBPressed_PauseGame(Button_PauseMenu);
}

void URTSHUD::UIBinding_OnPauseMenuButtonRightMouseReleased()
{
	PC->OnRMBReleased_PauseGame(Button_PauseMenu);
}

void URTSHUD::UIBinding_OnCommanderSkillTreeButtonLeftMousePress()
{
	PC->OnLMBPressed_CommanderSkillTreeShowButton(Button_CommanderSkillTree);
}

void URTSHUD::UIBinding_OnCommanderSkillTreeButtonLeftMouseReleased()
{
	PC->OnLMBReleased_CommanderSkillTreeShowButton(Button_CommanderSkillTree);
}

void URTSHUD::UIBinding_OnCommanderSkillTreeButtonRightMousePress()
{
	PC->OnRMBPressed_CommanderSkillTreeShowButton(Button_CommanderSkillTree);
}

void URTSHUD::UIBinding_OnCommanderSkillTreeButtonRightMouseReleased()
{
	PC->OnRMBReleased_CommanderSkillTreeShowButton(Button_CommanderSkillTree);
}

void URTSHUD::TellPlayerTargetingPanelAboutButtonAction_LMBPressed(UMyButton * Button, ARTSPlayerController * LocalPlayCon)
{
	PlayerTargetingPanel->OnPlayerTargetingButtonEvent_LMBPressed(Button, LocalPlayCon);
}

void URTSHUD::TellPlayerTargetingPanelAboutButtonAction_LMBReleased(UMyButton * Button, ARTSPlayerController * LocalPlayCon)
{
	PlayerTargetingPanel->OnPlayerTargetingButtonEvent_LMBReleased(Button, LocalPlayCon);
}

void URTSHUD::TellPlayerTargetingPanelAboutButtonAction_RMBPressed(UMyButton * Button, ARTSPlayerController * LocalPlayCon)
{
	PlayerTargetingPanel->OnPlayerTargetingButtonEvent_RMBPressed(Button, LocalPlayCon);
}

void URTSHUD::TellPlayerTargetingPanelAboutButtonAction_RMBReleased(UMyButton * Button, ARTSPlayerController * LocalPlayCon)
{
	PlayerTargetingPanel->OnPlayerTargetingButtonEvent_RMBReleased(Button, LocalPlayCon);
}

void URTSHUD::OnPlayerCurrentSelectedChanged(const TScriptInterface < ISelectable > NewCurrentSelected)
{
	CurrentSelected = NewCurrentSelected;

	if (IsWidgetBound(Widget_ContextMenu))
	{
		Widget_ContextMenu->OnPlayerSelectionChanged(NewCurrentSelected);
	}
	if (IsWidgetBound(Widget_SelectedGarrisonInfo))
	{
		Widget_SelectedGarrisonInfo->OnPlayerPrimarySelectedChanged(NewCurrentSelected);
	}
}

void URTSHUD::OnPlayerNoSelection()
{
	CurrentSelected = nullptr;

	if (IsWidgetBound(Widget_ContextMenu))
	{
		Widget_ContextMenu->OnPlayerNoSelection();
	}
	if (IsWidgetBound(Widget_SelectedGarrisonInfo))
	{
		Widget_SelectedGarrisonInfo->OnPlayerNoSelection();
	}
}

void URTSHUD::TryShowCommanderSkillTree()
{
	if (CommanderSkillTreeWidget != nullptr)
	{
		if (CommanderSkillTreeWidget->IsShowingOrPlayingShowAnimation() == false)
		{
			CommanderSkillTreeWidget->OnRequestToBeShown();
			AddToEscapeRequestResponableWidgets(CommanderSkillTreeWidget);

			// The player opened the skill tree so this animation can stop bothering them now
			StopAnimation(SkillTreeWantsOpeningAnim);
		}
	}
}

void URTSHUD::TryHideCommanderSkillTree()
{
	if (CommanderSkillTreeWidget != nullptr)
	{
		if (CommanderSkillTreeWidget->IsShowingOrPlayingShowAnimation())
		{
			CommanderSkillTreeWidget->OnRequestToBeHidden();
			RemoveFromEscapeRequestResponableWidgets(CommanderSkillTreeWidget);
		}
	}
}

void URTSHUD::OnToggleCommanderSkillTreeButtonClicked()
{
	// Kinda odd having the toggle visibility button on the HUD but no actual skill tree widget
	if (CommanderSkillTreeWidget != nullptr)
	{
		if (CommanderSkillTreeWidget->IsShowingOrPlayingShowAnimation())
		{
			CommanderSkillTreeWidget->OnRequestToBeHidden();
			RemoveFromEscapeRequestResponableWidgets(CommanderSkillTreeWidget);
		}
		else
		{
			CommanderSkillTreeWidget->OnRequestToBeShown();
			AddToEscapeRequestResponableWidgets(CommanderSkillTreeWidget);

			// The player opened the skill tree so this animation can stop bothering them now
			StopAnimation(SkillTreeWantsOpeningAnim);
		}
	}
}

void URTSHUD::OnToggleCommanderSkillTreeVisibilityKeyPressed()
{
	if (CommanderSkillTreeWidget != nullptr)
	{
		if (CommanderSkillTreeWidget->IsShowingOrPlayingShowAnimation())
		{
			CommanderSkillTreeWidget->OnRequestToBeHidden();
			RemoveFromEscapeRequestResponableWidgets(CommanderSkillTreeWidget);
		}
		else
		{
			CommanderSkillTreeWidget->OnRequestToBeShown();
			AddToEscapeRequestResponableWidgets(CommanderSkillTreeWidget);

			// The player opened the skill tree so this animation can stop bothering them now
			StopAnimation(SkillTreeWantsOpeningAnim);
		}
	}
}

void URTSHUD::ShowPlayerTargetingPanel(const FCommanderAbilityInfo * AbilityInfo)
{
	UE_CLOG(PlayerTargetingPanel == nullptr, RTSLOG, Fatal, TEXT("Request to show player targeting "
		"panel but panel is null. Set its blueprint in the HUD blueprint"));

	assert(PlayerTargetingPanel->IsShowingOrPlayingShowAnimation() == false);
	PlayerTargetingPanel->OnRequestToBeShown(AbilityInfo);

	AddToEscapeRequestResponableWidgets(PlayerTargetingPanel);
}

void URTSHUD::HidePlayerTargetingPanel()
{
	if (PlayerTargetingPanel->IsShowingOrPlayingShowAnimation())
	{
		PlayerTargetingPanel->OnRequestToBeHidden();

		RemoveFromEscapeRequestResponableWidgets(PlayerTargetingPanel);
	}
}

void URTSHUD::OnCommanderExperienceGained(float ExperienceTowardsNextRank, float ExperienceRequiredForNextRank)
{
	if (IsWidgetBound(Text_CommandersExperienceProgress))
	{
		Text_CommandersExperienceProgress->SetText(GetPlayerExperienceAquiredAsText(ExperienceTowardsNextRank));
	}
	if (IsWidgetBound(ProgressBar_CommandersExperienceProgress))
	{
		ProgressBar_CommandersExperienceProgress->SetPercent(ExperienceTowardsNextRank / ExperienceRequiredForNextRank);
	}

	// Update the commander skill tree widget
	if (CommanderSkillTreeWidget != nullptr)
	{
		CommanderSkillTreeWidget->OnExperienceGained(ExperienceTowardsNextRank, ExperienceRequiredForNextRank);
	}
}

void URTSHUD::OnCommanderRankGained_LastForEvent(uint8 OldRank, uint8 NewRank, int32 UnspentSkillPoints, 
	float ExperienceTowardsNextRank, float ExperienceRequiredForNextRank)
{
	if (IsWidgetBound(Text_CommandersRank))
	{
		Text_CommandersRank->SetText(FText::AsNumber(NewRank));
	}
	if (IsWidgetBound(Text_CommandersUnspentSkillPoints))
	{
		Text_CommandersUnspentSkillPoints->SetText(FText::AsNumber(UnspentSkillPoints));
	}
	if (IsWidgetBound(Text_CommandersExperienceProgress))
	{
		Text_CommandersExperienceProgress->SetText(GetPlayerExperienceAquiredAsText(ExperienceTowardsNextRank));
	}
	if (IsWidgetBound(Text_CommandersExperienceRequired))
	{
		Text_CommandersExperienceRequired->SetText(GetPlayerExperienceRequiredAsText(ExperienceRequiredForNextRank));
	}
	if (IsWidgetBound(ProgressBar_CommandersExperienceProgress))
	{
		ProgressBar_CommandersExperienceProgress->SetPercent(ExperienceTowardsNextRank / ExperienceRequiredForNextRank);
	}

	/* Update the skill tree widget. Null check first. If faction does not use skill tree 
	then it can be null */
	if (CommanderSkillTreeWidget != nullptr)
	{
		/* If the anim is already playing or the skill tree is already open/opening then we do not 
		need to play it so pass in "Never" */
		const ECommanderSkillTreeAnimationPlayRule AnimRule = IsAnimationPlaying(SkillTreeWantsOpeningAnim) || CommanderSkillTreeWidget->IsShowingOrPlayingShowAnimation()
			? ECommanderSkillTreeAnimationPlayRule::Never
			: CommanderSkillTreeAnimPlayRule;

		const bool bShouldPlayOpenAnim = CommanderSkillTreeWidget->OnLevelUp_LastForEvent(OldRank, NewRank, UnspentSkillPoints,
			ExperienceTowardsNextRank, ExperienceRequiredForNextRank, AnimRule);

		// Decide if should play widget anim
		if (bShouldPlayOpenAnim)
		{
			/* During setup if the anim was null then we set the rule to Never so this should hold */
			assert(SkillTreeWantsOpeningAnim != nullptr);
			assert(IsAnimationPlaying(SkillTreeWantsOpeningAnim) == false);
			PlayAnimation(SkillTreeWantsOpeningAnim, 0.f, 0); // Looping
		}
	}
}

void URTSHUD::OnCommanderSkillAquired(FAquiredCommanderAbilityState * AquiredAbilityState, 
	const FCommanderAbilityTreeNodeInfo * NodeInfo, int32 NumUnspentSkillPoints)
{
	if (IsWidgetBound(Text_CommandersUnspentSkillPoints))
	{
		Text_CommandersUnspentSkillPoints->SetText(FText::AsNumber(NumUnspentSkillPoints));
	}

	if (CommanderSkillTreeWidget != nullptr)
	{
		CommanderSkillTreeWidget->OnNewAbilityRankAquired(NodeInfo, AquiredAbilityState->GetAquiredRank(), 
			NumUnspentSkillPoints);
	}

	/* OnlyExecuteOnAquired abilities shouldn't go on the global skills panel */
	if (NodeInfo->OnlyExecuteOnAquired() == false)
	{
		if (IsWidgetBound(Widget_GlobalSkillsPanel))
		{
			/* Check if this was the first rank of the ability that was aquired */
			if (AquiredAbilityState->GetAquiredRank() == 0)
			{
				Widget_GlobalSkillsPanel->OnCommanderSkillAquired_FirstRank(AquiredAbilityState);
			}
			else
			{
				Widget_GlobalSkillsPanel->OnCommanderSkillAquired_NotFirstRank(AquiredAbilityState->GetGlobalSkillsPanelButtonIndex());
			}
		}
	}

	/* Update the tooltip widget if it is showing and the node it is showing info for is the 
	one that just got modified */
	if (TooltipWidget_CommanderSkillTreeNode != nullptr 
		&& TooltipWidget_CommanderSkillTreeNode->GetDisplayedNodeInfo() == NodeInfo
		&& TooltipWidget_CommanderSkillTreeNode->GetVisibility() == HUDStatics::TOOLTIP_VISIBILITY)
	{
		TooltipWidget_CommanderSkillTreeNode->OnNewRankAquired(NodeInfo);
	}

	/* Although unlikely that it is showing, try update the global skills panel tooltip 
	widget if it is showing */
	if (TooltipWidget_GlobalSkillsPanelButton != nullptr
		&& TooltipWidget_GlobalSkillsPanelButton->GetDisplayedInfo() == AquiredAbilityState
		&& TooltipWidget_GlobalSkillsPanelButton->GetVisibility() == HUDStatics::TOOLTIP_VISIBILITY)
	{
		TooltipWidget_GlobalSkillsPanelButton->OnNewRankAquired();
	}
}

void URTSHUD::OnCommanderAbilityUsed(const FCommanderAbilityInfo & UsedAbility, int32 GlobalSkillsPanelIndex, bool bWasLastCharge)
{
	Widget_GlobalSkillsPanel->OnCommanderSkillUsed(UsedAbility.GetCooldown(), GlobalSkillsPanelIndex, bWasLastCharge);
}

void URTSHUD::OnCommanderAbilityCooledDown(const FCommanderAbilityInfo & AbilityInfo, int32 GlobalSkillsPanelIndex)
{
	Widget_GlobalSkillsPanel->OnCommanderAbilityCooledDown(GlobalSkillsPanelIndex);
}

void URTSHUD::Selected_OnHealthChanged(ISelectable * InSelectable, float InNewAmount, float MaxHealth, bool bIsCurrentSelected)
{
	// Only update context menu if change was for current selected
	if (bIsCurrentSelected && IsWidgetBound(Widget_ContextMenu))
	{
		Widget_ContextMenu->Selected_OnHealthChanged(InNewAmount, MaxHealth);
	}
}

void URTSHUD::Selected_OnMaxHealthChanged(ISelectable * InSelectable, float NewMaxHealth, float CurrentHealth, bool bIsCurrentSelected)
{
	if (bIsCurrentSelected && IsWidgetBound(Widget_ContextMenu))
	{
		Widget_ContextMenu->Selected_OnMaxHealthChanged(NewMaxHealth, CurrentHealth);
	}
}

void URTSHUD::Selected_OnCurrentAndMaxHealthChanged(ISelectable * InSelectable, float NewMaxHealth, float CurrentHealth, bool bIsCurrentSelected)
{
	if (bIsCurrentSelected && IsWidgetBound(Widget_ContextMenu))
	{
		Widget_ContextMenu->Selected_OnCurrentAndMaxHealthChanged(NewMaxHealth, CurrentHealth);
	}
}

void URTSHUD::Selected_OnSelectableResourceCurrentAmountChanged(ISelectable * InSelectable, 
	float CurrentAmount, int32 MaxAmount, bool bIsCurrentSelected)
{
	if (bIsCurrentSelected && IsWidgetBound(Widget_ContextMenu))
	{
		Widget_ContextMenu->Selected_OnSelectableResourceCurrentAmountChanged(CurrentAmount, MaxAmount);
	}
}

void URTSHUD::Selected_OnSelectableResourceMaxAmountChanged(ISelectable * InSelectable, 
	float CurrentAmount, int32 MaxAmount, bool bIsCurrentSelected)
{
	if (bIsCurrentSelected && IsWidgetBound(Widget_ContextMenu))
	{
		Widget_ContextMenu->Selected_OnSelectableResourceMaxAmountChanged(CurrentAmount, MaxAmount);
	}
}

void URTSHUD::Selected_OnSelectableResourceCurrentAndMaxAmountsChanged(ISelectable * InSelectable, 
	float CurrentAmount, int32 MaxAmount, bool bIsCurrentSelected)
{
	if (bIsCurrentSelected && IsWidgetBound(Widget_ContextMenu))
	{
		Widget_ContextMenu->Selected_OnSelectableResourceCurrentAndMaxAmountsChanged(CurrentAmount, MaxAmount);
	}
}

void URTSHUD::Selected_OnSelectableResourceRegenRateChanged(ISelectable * InSelectable, 
	float PerSecondRegenRate, bool bIsCurrentSelected)
{
	if (bIsCurrentSelected && IsWidgetBound(Widget_ContextMenu))
	{
		Widget_ContextMenu->Selected_OnSelectableResourceRegenRateChanged(PerSecondRegenRate);
	}
}

void URTSHUD::Selected_OnRankChanged(ISelectable * InSelectable, float ExperienceTowardsNextRank, 
	uint8 InNewRank, float ExperienceRequired, bool bIsCurrentSelected)
{
	assert(InNewRank <= LevelingUpOptions::MAX_LEVEL);
	
	if (bIsCurrentSelected && IsWidgetBound(Widget_ContextMenu))
	{
		Widget_ContextMenu->Selected_OnRankChanged(ExperienceTowardsNextRank, InNewRank, ExperienceRequired);
	}
}

void URTSHUD::Selected_OnCurrentRankExperienceChanged(ISelectable * InSelectable, float InNewAmount, 
	uint8 CurrentRank, float ExperienceRequired, bool bIsCurrentSelected)
{
	assert(CurrentRank <= LevelingUpOptions::MAX_LEVEL);
	
	if (bIsCurrentSelected && IsWidgetBound(Widget_ContextMenu))
	{
		Widget_ContextMenu->Selected_OnCurrentRankExperienceChanged(InNewAmount, CurrentRank, ExperienceRequired);
	}
}

void URTSHUD::Selected_OnImpactDamageChanged(ISelectable * InSelectable, float InNewAmount, bool bIsCurrentSelected)
{
	if (bIsCurrentSelected && IsWidgetBound(Widget_ContextMenu))
	{
		Widget_ContextMenu->Selected_OnImpactDamageChanged(InNewAmount);
	}
}

void URTSHUD::Selected_OnAttackRateChanged(ISelectable * InSelectable, float AttackRate, bool bIsCurrentSelected)
{
	if (bIsCurrentSelected && IsWidgetBound(Widget_ContextMenu))
	{
		Widget_ContextMenu->Selected_OnAttackRateChanged(AttackRate);
	}
}

void URTSHUD::Selected_OnAttackRangeChanged(ISelectable * InSelectable, float AttackRange, bool bIsCurrentSelected)
{
	if (bIsCurrentSelected && IsWidgetBound(Widget_ContextMenu))
	{
		Widget_ContextMenu->Selected_OnAttackRangeChanged(AttackRange);
	}
}

void URTSHUD::Selected_OnDefenseMultiplierChanged(ISelectable * InSelectable, float NewDefenseMultiplier, bool bIsCurrentSelected)
{
	if (bIsCurrentSelected && IsWidgetBound(Widget_ContextMenu))
	{
		Widget_ContextMenu->Selected_OnDefenseMultiplierChanged(NewDefenseMultiplier);
	}
}

void URTSHUD::Selected_OnItemAddedToInventory(ISelectable * InSelectable, int32 DisplayIndex, const FInventorySlotState & UpdatedSlot, bool bIsCurrentSelected)
{
	if (bIsCurrentSelected && IsWidgetBound(Widget_ContextMenu))
	{
		Widget_ContextMenu->Selected_OnItemAddedToInventory(DisplayIndex, UpdatedSlot);
	}
}

void URTSHUD::Selected_OnItemsRemovedFromInventory(ISelectable * InSelectable, const FInventory * Inventory, 
	const TMap <uint8, FInventoryItemQuantityPair> & RemovedItems, bool bIsCurrentSelected)
{
	if (bIsCurrentSelected && IsWidgetBound(Widget_ContextMenu))
	{
		Widget_ContextMenu->Selected_OnItemsRemovedFromInventory(Inventory, RemovedItems);
	}
}

void URTSHUD::Selected_OnInventoryPositionsSwapped(ISelectable * InSelectable, uint8 DisplayIndex1, uint8 DisplayIndex2, bool bIsCurrentSelected)
{
	if (bIsCurrentSelected && IsWidgetBound(Widget_ContextMenu))
	{
		Widget_ContextMenu->Selected_OnInventoryPositionsSwapped(DisplayIndex1, DisplayIndex2);
	}
}

void URTSHUD::Selected_OnInventoryItemUsed(ISelectable * InSelectable, uint8 DisplayIndex, float UseAbilityTotalCooldown, bool bIsCurrentSelected)
{
	if (bIsCurrentSelected && IsWidgetBound(Widget_ContextMenu))
	{
		Widget_ContextMenu->Selected_OnInventoryItemUsed(DisplayIndex, UseAbilityTotalCooldown);
	}
}

void URTSHUD::Selected_OnInventoryItemUseCooldownFinished(ISelectable * InSelectable, uint8 InventorySlotDisplayIndex, bool bIsCurrentSelected)
{
	if (bIsCurrentSelected && IsWidgetBound(Widget_ContextMenu))
	{
		Widget_ContextMenu->Selected_OnInventoryItemUseCooldownFinished(InventorySlotDisplayIndex);
	}
}

void URTSHUD::Selected_OnItemPurchasedFromShop(ISelectable * InSelectable, uint8 ShopSlotIndex, bool bIsCurrentSelected)
{
	if (bIsCurrentSelected && IsWidgetBound(Widget_ContextMenu))
	{
		Widget_ContextMenu->Selected_OnItemPurchasedFromShop(ShopSlotIndex);
	}
}

void URTSHUD::Selected_OnInventoryItemSold(ISelectable * InSelectable, uint8 InvDisplayIndex, bool bIsCurrentSelected)
{
	if (bIsCurrentSelected && IsWidgetBound(Widget_ContextMenu))
	{
		Widget_ContextMenu->Selected_OnInventoryItemSold(InvDisplayIndex);
	}
}

void URTSHUD::Selected_OnBuffApplied(ISelectable * InSelectable, const FTickableBuffOrDebuffInstanceInfo & BuffInfo, bool bIsCurrentSelected)
{
	if (bIsCurrentSelected && IsWidgetBound(Widget_ContextMenu))
	{
		Widget_ContextMenu->Selected_OnBuffApplied(BuffInfo);
	}
}

void URTSHUD::Selected_OnBuffApplied(ISelectable * InSelectable, const FStaticBuffOrDebuffInstanceInfo & DebuffInfo, bool bIsCurrentSelected)
{
	if (bIsCurrentSelected && IsWidgetBound(Widget_ContextMenu))
	{
		Widget_ContextMenu->Selected_OnBuffApplied(DebuffInfo);
	}
}

void URTSHUD::Selected_OnDebuffApplied(ISelectable * InSelectable, const FTickableBuffOrDebuffInstanceInfo & DebuffInfo, bool bIsCurrentSelected)
{
	if (bIsCurrentSelected && IsWidgetBound(Widget_ContextMenu))
	{
		Widget_ContextMenu->Selected_OnDebuffApplied(DebuffInfo);
	}
}

void URTSHUD::Selected_OnDebuffApplied(ISelectable * InSelectable, const FStaticBuffOrDebuffInstanceInfo & DebuffInfo, bool bIsCurrentSelected)
{
	if (bIsCurrentSelected && IsWidgetBound(Widget_ContextMenu))
	{
		Widget_ContextMenu->Selected_OnDebuffApplied(DebuffInfo);
	}
}

void URTSHUD::Selected_OnTickableBuffRemoved(ISelectable * InSelectable, int32 ArrayIndex, 
	EBuffAndDebuffRemovalReason RemovalReason, bool bIsCurrentSelected)
{
	if (bIsCurrentSelected && IsWidgetBound(Widget_ContextMenu))
	{
		Widget_ContextMenu->Selected_OnTickableBuffRemoved(ArrayIndex, RemovalReason);
	}
}

void URTSHUD::Selected_OnTickableDebuffRemoved(ISelectable * InSelectable, int32 ArrayIndex,
	EBuffAndDebuffRemovalReason RemovalReason, bool bIsCurrentSelected)
{
	if (bIsCurrentSelected && IsWidgetBound(Widget_ContextMenu))
	{
		Widget_ContextMenu->Selected_OnTickableDebuffRemoved(ArrayIndex, RemovalReason);
	}
}

void URTSHUD::Selected_OnStaticBuffRemoved(ISelectable * InSelectable, int32 ArrayIndex, 
	EBuffAndDebuffRemovalReason RemovalReason, bool bIsCurrentSelected)
{
	if (bIsCurrentSelected && IsWidgetBound(Widget_ContextMenu))
	{
		Widget_ContextMenu->Selected_OnStaticBuffRemoved(ArrayIndex, RemovalReason);
	}
}

void URTSHUD::Selected_OnStaticDebuffRemoved(ISelectable * InSelectable, int32 ArrayIndex, 
	EBuffAndDebuffRemovalReason RemovalReason, bool bIsCurrentSelected)
{
	if (bIsCurrentSelected && IsWidgetBound(Widget_ContextMenu))
	{
		Widget_ContextMenu->Selected_OnStaticDebuffRemoved(ArrayIndex, RemovalReason);
	}
}

void URTSHUD::Selected_UpdateTickableBuffDuration(ISelectable * InSelectable, int32 ArrayIndex, 
	float DurationRemaining, bool bIsCurrentSelected)
{
	if (bIsCurrentSelected && IsWidgetBound(Widget_ContextMenu))
	{
		Widget_ContextMenu->Selected_UpdateTickableBuffDuration(ArrayIndex, DurationRemaining);
	}
}

void URTSHUD::Selected_UpdateTickableDebuffDuration(ISelectable * InSelectable, int32 ArrayIndex,
	float DurationRemaining, bool bIsCurrentSelected)
{
	if (bIsCurrentSelected && IsWidgetBound(Widget_ContextMenu))
	{
		Widget_ContextMenu->Selected_UpdateTickableDebuffDuration(ArrayIndex, DurationRemaining);
	}
}

void URTSHUD::OnUnitEnteredBuildingGarrison(ABuilding * Building, const FBuildingGarrisonAttributes & BuildingsGarrisonAttributes, AInfantry * UnitThatEnteredGarrison, int32 ContainerIndex)
{
	if (IsWidgetBound(Widget_SelectedGarrisonInfo))
	{
		Widget_SelectedGarrisonInfo->OnBuildingGarrisonedUnitGained(Building, UnitThatEnteredGarrison, ContainerIndex);
	}
}

void URTSHUD::OnUnitExitedBuildingGarrison(ABuilding * Building, const FBuildingGarrisonAttributes & BuildingsGarrisonAttributes, int32 LostUnitsContainerIndex)
{
	if (IsWidgetBound(Widget_SelectedGarrisonInfo))
	{
		Widget_SelectedGarrisonInfo->OnBuildingGarrisonedUnitLost(Building, LostUnitsContainerIndex);
	}
}

void URTSHUD::OnBuildingGarrisonMultipleUnitsEnteredOrExited(ABuilding * Building, const FBuildingGarrisonAttributes & BuildingsGarrisonAttributes, const TArray<TRawPtr(AActor)> & GarrisonedUnitsContainer)
{
	if (IsWidgetBound(Widget_SelectedGarrisonInfo))
	{
		Widget_SelectedGarrisonInfo->OnBuildingPotentiallyManyGarrisonedUnitsLost(Building, GarrisonedUnitsContainer);
	}
}

void URTSHUD::OnGarrisonedUnitZeroHealth(ABuilding * Building, int32 LostUnitsContainerIndex)
{
	if (IsWidgetBound(Widget_SelectedGarrisonInfo))
	{
		Widget_SelectedGarrisonInfo->OnBuildingGarrisonedUnitLost(Building, LostUnitsContainerIndex);
	}
}

bool URTSHUD::IsChatBoxShowing() const
{
	if (IsWidgetBound(Widget_ChatInput))
	{
		return Widget_ChatInput->GetVisibility() == ESlateVisibility::Visible;
	}

	return false;
}

void URTSHUD::OpenChatBox(EMessageRecipients MessageRecipients)
{
	if (IsWidgetBound(Widget_ChatInput))
	{	
		Widget_ChatInput->Open(MessageRecipients);

		AddToEscapeRequestResponableWidgets(Widget_ChatInput);
	}
}

void URTSHUD::CloseChatBox()
{
	Widget_ChatInput->Close();
}

void URTSHUD::OnGameMessageReceived(const FText & Message, EMessageType MessageType)
{
	if (IsWidgetBound(Widget_GameOutput))
	{
		switch (MessageType)
		{
			case EMessageType::GameWarning:
			{
				Widget_GameOutput->OnGameWarningReceived(Message.ToString());
				break;
			}
			case EMessageType::GameNotification:
			{
				Widget_GameOutput->OnGameNotificationReceived(Message.ToString());
				break;
			}
			default:
			{
				UE_LOG(RTSLOG, Fatal, TEXT("Unexpected message type %s"),
					TO_STRING(EMessageType, MessageType));
				break;
			}
		}
	}
}

void URTSHUD::OnChatMessageReceived(const FString & SendersName, const FString & Message,
	EMessageRecipients MessageType)
{
	/* Currently do nothing with MessageType param */

	if (IsWidgetBound(Widget_ChatOutput))
	{
		Widget_ChatOutput->OnChatMessageReceived(SendersName, Message, MessageType);
	}
}

void URTSHUD::OnMenuWarningHappened(EGameWarning WarningType)
{
	if (MenuOutputWidget != nullptr)
	{
		MenuOutputWidget->ShowWarningMessageAndPlaySound(GI, WarningType);
	}
}

void URTSHUD::SendChatMessage()
{
	Widget_ChatInput->SendChatMessageAndClose();
}

bool URTSHUD::IsPauseMenuShowingOrPlayingShowAnimation() const
{
	return PauseMenu->IsShowingOrPlayingShowAnimation();
}

void URTSHUD::ShowPauseMenu()
{
	/* Pretty sure this will fail if net mode is not standalone which is what we want. Unsure
	of behavior if player hosts multiplayer game but only plays against CPU players */
	ARTSPlayerController::SetGamePaused(GetWorld(), true);
	
	TimeWhenPauseMenuWasShown = GetWorld()->GetRealTimeSeconds();

	// Visible not SelfHitTestInvisible so it will block clicks onto action bar, persistent panel, etc. 
	PauseMenu->SetVisibility(ESlateVisibility::Visible);
	AddToEscapeRequestResponableWidgets(PauseMenu);
}

void URTSHUD::HidePauseMenu()
{
	// Unpause game if it is paused
	ARTSPlayerController::SetGamePaused(GetWorld(), false);

	PauseMenu->SetVisibility(ESlateVisibility::Hidden);
	RemoveFromEscapeRequestResponableWidgets(PauseMenu);
}

bool URTSHUD::TryCloseEscapableWidget()
{
	if (EscapeRequestResponableWidgets.Num() > 0)
	{
		const bool bResult = EscapeRequestResponableWidgets.Last()->RespondToEscapeRequest(this);
		if (bResult)
		{
			EscapeRequestResponableWidgets.Pop(false);
		}

		return true;
	}
	
	return false;
}

void URTSHUD::SetInitialResourceAmounts(const TArray<int32>& ResourceArray, const TArray<FHousingResourceState>& HousingResourceArray)
{
	if (IsWidgetBound(Widget_Resources))
	{
		Widget_Resources->SetInitialResourceAmounts(ResourceArray, HousingResourceArray);
	}
}

void URTSHUD::OnResourceChanged(EResourceType ResourceType, int32 PreviousAmount, int32 NewAmount)
{
	if (IsWidgetBound(Widget_Resources))
	{
		Widget_Resources->OnResourcesChanged(ResourceType, PreviousAmount, NewAmount);
	}
}

void URTSHUD::OnHousingResourceConsumptionChanged(const TArray<FHousingResourceState>& ResourceArray)
{
	if (IsWidgetBound(Widget_Resources))
	{
		Widget_Resources->OnHousingResourceConsumptionChanged(ResourceArray);
	}
}

void URTSHUD::OnHousingResourceProvisionsChanged(const TArray<FHousingResourceState>& ResourceArray)
{
	if (IsWidgetBound(Widget_Resources))
	{
		Widget_Resources->OnHousingResourceProvisionsChanged(ResourceArray);
	}
}

void URTSHUD::OnUpgradeComplete(EUpgradeType UpgradeType, bool bUpgradePrereqsNowMetForSomething)
{
	/* Both these widgets only care if something potentially went from being unproducable to 
	producable */
	if (bUpgradePrereqsNowMetForSomething)
	{
		if (IsWidgetBound(Widget_ContextMenu))
		{
			Widget_ContextMenu->OnUpgradeComplete(UpgradeType);
		}

		if (IsWidgetBound(Widget_PersistentPanel))
		{
			Widget_PersistentPanel->OnUpgradeComplete(UpgradeType);
		}
	}
}

void URTSHUD::OnItemAddedToProductionQueue(const FTrainingInfo & Item, const FProductionQueue & Queue, AActor * Producer)
{
	assert(Producer != nullptr);

	/* Only tell context menu widget if it is the current selected that is producing something */
	if (IsWidgetBound(Widget_ContextMenu) && Producer == CurrentSelected.GetObject())
	{
		Widget_ContextMenu->OnItemAddedToProductionQueue(Item, Queue, Producer);
	}

	if (IsWidgetBound(Widget_PersistentPanel))
	{
		Widget_PersistentPanel->OnItemAddedToProductionQueue(Item, Queue, Producer);
	}
}

void URTSHUD::OnItemAddedAndProductionStarted(const FTrainingInfo & Item, const FProductionQueue & Queue, AActor * Producer)
{
	assert(Producer != nullptr);
	assert(Queue.GetType() != EProductionQueueType::None);

	bool bIsBuildsInTabBuilding = false;

	if (Item.IsProductionForBuilding())
	{
		const FBuildingInfo * BuildingInfo = FI->GetBuildingInfo(Item.GetBuildingType());
		const EBuildingBuildMethod BuildMethod = BuildingInfo->GetBuildingBuildMethod();

		if (BuildMethod == EBuildingBuildMethod::BuildsInTab)
		{
			bIsBuildsInTabBuilding = true;
		}
	}

	if (bIsBuildsInTabBuilding)
	{
		if (IsWidgetBound(Widget_PersistentPanel))
		{
			Widget_PersistentPanel->OnBuildsInTabProductionStarted(Item, Queue, Producer);
		}
	}
	else
	{
		/* Only tell context menu widget if it is the current selected that is producing something */
		if (IsWidgetBound(Widget_ContextMenu) && Producer == CurrentSelected.GetObject())
		{
			Widget_ContextMenu->OnItemAddedAndProductionStarted(Item, Queue, Producer);
		}

		if (IsWidgetBound(Widget_PersistentPanel))
		{
			Widget_PersistentPanel->OnItemAddedAndProductionStarted(Item, Queue, Producer);
		}
	}
}

void URTSHUD::OnProductionComplete(const FTrainingInfo & Item, const FProductionQueue & Queue, uint8 NumRemoved, AActor * Producer)
{
	assert(Producer != nullptr);

	/* Only tell context menu widget if it is the current selected that is producing something */
	if (IsWidgetBound(Widget_ContextMenu) && Producer == CurrentSelected.GetObject())
	{
		Widget_ContextMenu->OnProductionComplete(Item, Queue, NumRemoved, Producer);
	}

	if (IsWidgetBound(Widget_PersistentPanel))
	{
		/* Check if item is displayed in a persistent tab. If not then persistent panel does not
		need to know about it */
		if (FI->GetHUDPersistentTab(Item) != EHUDPersistentTabType::None)
		{
			Widget_PersistentPanel->OnProductionComplete(Item, Queue, NumRemoved, Producer);
		}
	}
}

void URTSHUD::OnBuildsInTabProductionComplete(EBuildingType BuildingType)
{
	if (IsWidgetBound(Widget_PersistentPanel))
	{
		Widget_PersistentPanel->OnBuildsInTabProductionComplete(BuildingType);
	}
}

void URTSHUD::OnBuildsInTabBuildingPlaced(const FTrainingInfo & Item)
{
	if (IsWidgetBound(Widget_PersistentPanel))
	{
		Widget_PersistentPanel->OnBuildsInTabBuildingPlaced(Item);
	}
}

void URTSHUD::OnBuildingConstructed(EBuildingType BuildingType, bool bFirstOfItsType)
{
	if (IsWidgetBound(Widget_PersistentPanel))
	{
		Widget_PersistentPanel->OnBuildingConstructed(BuildingType, bFirstOfItsType);
	}

	if (IsWidgetBound(Widget_ContextMenu))
	{
		Widget_ContextMenu->OnBuildingConstructed(BuildingType, bFirstOfItsType);
	}
}

void URTSHUD::OnBuildingDestroyed(EBuildingType BuildingType, bool bLastOfItsType)
{
	if (IsWidgetBound(Widget_PersistentPanel))
	{
		Widget_PersistentPanel->OnBuildingDestroyed(BuildingType, bLastOfItsType);
	}

	if (IsWidgetBound(Widget_ContextMenu))
	{
		Widget_ContextMenu->OnBuildingDestroyed(BuildingType, bLastOfItsType);
	}
}

void URTSHUD::OnAnotherPlayerDefeated(ARTSPlayerState * DefeatedPlayer)
{
	if (PlayerTargetingPanel != nullptr)
	{
		PlayerTargetingPanel->OnAnotherPlayerDefeated(DefeatedPlayer);
	}
}

UImage * URTSHUD::GetMouseFocusWidget() const
{
	assert(MouseFocusImage != nullptr);
	return MouseFocusImage;
}

UCanvasPanelSlot * URTSHUD::GetMouseFocusWidgetSlot() const
{
	return MouseFocusImageSlot;
}

float URTSHUD::GetTimeWhenPauseMenuWasShown() const
{
	return TimeWhenPauseMenuWasShown;
}

#if !UE_BUILD_SHIPPING
bool URTSHUD::IsTooltipWidgetShowing() const
{
	return GetNumTooltipWidgetsShowing() > 0;
	
	// Currently just considers the inventory item tooltip widget
	return TooltipWidget_InventoryItem->GetVisibility() != ESlateVisibility::Hidden;
}

int32 URTSHUD::GetNumTooltipWidgetsShowing() const
{
	int32 Num = 0;

	if (TooltipWidget_InventoryItem != nullptr && TooltipWidget_InventoryItem->GetVisibility() != ESlateVisibility::Hidden)
	{
		Num++;
	}
	if (TooltipWidget_Abilities != nullptr && TooltipWidget_Abilities->GetVisibility() != ESlateVisibility::Hidden)
	{
		Num++;
	}
	if (TooltipWidget_BuildBuilding != nullptr && TooltipWidget_BuildBuilding->GetVisibility() != ESlateVisibility::Hidden)
	{
		Num++;
	}
	if (TooltipWidget_TrainUnit != nullptr && TooltipWidget_TrainUnit->GetVisibility() != ESlateVisibility::Hidden)
	{
		Num++;
	}
	if (TooltipWidget_ResearchUpgrade != nullptr && TooltipWidget_ResearchUpgrade->GetVisibility() != ESlateVisibility::Hidden)
	{
		Num++;
	}

	return Num;
}
#endif

void URTSHUD::ShowTooltipWidget(UWidget * Button, EUIElementType ButtonType)
{
	/* It's possible to briefly show 2 tooltips at the same time if moving mouse fast between 
	an inventory item in world and a UI element, so could hide the inv item tooltip here first 
	before showing new tooltip, or do we care? If I choose yes then make sure that the 
	inv item show tooltip func hides a UI tooltip if it is showing too */
	
	/* This could very well happen for any UMyButton that isn't for one of the values in 
	EUIElementType. May want to actually just return instead. Do same for 
	HideTooltipWidget too */
	UE_CLOG(ButtonType == EUIElementType::None, RTSLOG, Fatal, TEXT("Did not set button type for "
		"button %s"), *Button->GetName());
	
	if (ButtonType == EUIElementType::SelectablesActionBar)
	{
		UContextActionButton * ActionButton = static_cast<UContextActionButton*>(Button);
		const FContextButton * ActionButtonInfo = ActionButton->GetButtonType();

		if (ActionButtonInfo->IsForBuildBuilding())
		{
			ShowTooltipWidget_SelectableActionBar_BuildBuilding(*FI->GetBuildingInfo(ActionButtonInfo->GetBuildingType()));
		}
		else if (ActionButtonInfo->IsForTrainUnit())
		{
			ShowTooltipWidget_SelectableActionBar_TrainUnit(*FI->GetUnitInfo(ActionButtonInfo->GetUnitType()));
		}
		else if (ActionButtonInfo->IsForResearchUpgrade())
		{
			ShowTooltipWidget_SelectableActionBar_ResearchUpgrade(FI->GetUpgradeInfoChecked(ActionButtonInfo->GetUpgradeType()));
		}
		else // Assumed for ability
		{
			ShowTooltipWidget_SelectableActionBar_Ability(GI->GetContextInfo(ActionButtonInfo->GetButtonType()));
		}
	}
	else if (ButtonType == EUIElementType::PersistentPanel)
	{
		UHUDPersistentTabButton * ProductionButton = static_cast<UHUDPersistentTabButton*>(Button);
		const FContextButton * ButtonInfo = ProductionButton->GetButtonType();

		if (ButtonInfo->IsForBuildBuilding())
		{
			ShowTooltipWidget_PersistentPanel_BuildBuilding(*FI->GetBuildingInfo(ButtonInfo->GetBuildingType()));
		}
		else if (ButtonInfo->IsForTrainUnit())
		{
			ShowTooltipWidget_PersistentPanel_TrainUnit(*FI->GetUnitInfo(ButtonInfo->GetUnitType()));
		}
		else // Assumed for research upgrade. Are upgrades even possible on persistent panel though?
		{
			assert(ButtonInfo->IsForResearchUpgrade());
			ShowTooltipWidget_PersistentPanel_ResearchUpgrade(FI->GetUpgradeInfoChecked(ButtonInfo->GetUpgradeType()));
		}
	}
	else if (ButtonType == EUIElementType::ProductionQueueSlot)
	{
		UProductionQueueButton * ProductionButton = CastChecked<UProductionQueueButton>(Button);
		const FContextButton * ButtonInfo = ProductionButton->GetButtonType();

		if (ButtonInfo->IsForBuildBuilding())
		{
			ShowTooltipWidget_ProductionQueueSlot_BuildBuilding(*FI->GetBuildingInfo(ButtonInfo->GetBuildingType()));
		}
		else if (ButtonInfo->IsForTrainUnit())
		{
			ShowTooltipWidget_ProductionQueueSlot_TrainUnit(*FI->GetUnitInfo(ButtonInfo->GetUnitType()));
		}
		else // Assumed for research upgrade
		{
			assert(ButtonInfo->IsForResearchUpgrade());
			ShowTooltipWidget_ProductionQueueSlot_ResearchUpgrade(FI->GetUpgradeInfoChecked(ButtonInfo->GetUpgradeType()));
		}
	}
	else if (ButtonType == EUIElementType::InventorySlot)
	{
		UInventoryItemButton * InvButton = static_cast<UInventoryItemButton*>(Button);

		// Only show tooltip for inventory slots with items in them
		if (InvButton->GetInventorySlot()->HasItem())
		{
			ShowTooltipWidget_InventorySlot(*InvButton->GetInventorySlot(), *InvButton->GetItemInfo());
		}
	}
	else if (ButtonType == EUIElementType::ShopSlot)
	{
		UItemOnDisplayInShopButton * ShopButton = static_cast<UItemOnDisplayInShopButton*>(Button);
		ShowTooltipWidget_ShopSlot(*ShopButton->GetSlotStateInfo(), *ShopButton->GetItemInfo());
	}
	else if (ButtonType == EUIElementType::GlobalSkillsPanelButton)
	{
		UGlobalSkillsPanelButton * PanelButton = CastChecked<UGlobalSkillsPanelButton>(Button);
		ShowTooltipWidget_GlobalSkillsPanelButton(PanelButton->GetAbilityState());
	}
	else if (ButtonType == EUIElementType::CommanderSkillTreeNode)
	{
		UCommanderSkillTreeNodeWidget * NodeWidget = CastChecked<UCommanderSkillTreeNodeWidget>(Button);
		ShowTooltipWidget_CommanderSkillTreeNode(NodeWidget->GetNodeInfo());
	}
	else // Assumed NoTooltipRequired
	{
		assert(ButtonType == EUIElementType::NoTooltipRequired);
	}
}

void URTSHUD::ShowTooltipWidget_SelectableActionBar_Ability(const FContextButtonInfo & ButtonInfo)
{
	if (TooltipWidget_Abilities != nullptr)
	{
		TooltipWidget_Abilities->SetupFor(ButtonInfo);
		TooltipWidget_Abilities->SetVisibility(HUDStatics::TOOLTIP_VISIBILITY);
	}
}

void URTSHUD::ShowTooltipWidget_SelectableActionBar_BuildBuilding(const FDisplayInfoBase & BuildingInfo)
{
	if (TooltipWidget_BuildBuilding != nullptr)
	{
		TooltipWidget_BuildBuilding->SetupFor(BuildingInfo);
		TooltipWidget_BuildBuilding->SetVisibility(HUDStatics::TOOLTIP_VISIBILITY);
	}
}

void URTSHUD::ShowTooltipWidget_SelectableActionBar_TrainUnit(const FDisplayInfoBase & UnitInfo)
{
	if (TooltipWidget_TrainUnit != nullptr)
	{
		TooltipWidget_TrainUnit->SetupFor(UnitInfo);
		TooltipWidget_TrainUnit->SetVisibility(HUDStatics::TOOLTIP_VISIBILITY);
	}
}

void URTSHUD::ShowTooltipWidget_SelectableActionBar_ResearchUpgrade(const FDisplayInfoBase & UpgradeInfo)
{
	if (TooltipWidget_ResearchUpgrade != nullptr)
	{
		TooltipWidget_ResearchUpgrade->SetupFor(UpgradeInfo);
		TooltipWidget_ResearchUpgrade->SetVisibility(HUDStatics::TOOLTIP_VISIBILITY);
	}
}

void URTSHUD::ShowTooltipWidget_PersistentPanel_BuildBuilding(const FDisplayInfoBase & BuildingInfo)
{
	if (TooltipWidget_BuildBuilding != nullptr)
	{
		TooltipWidget_BuildBuilding->SetupFor(BuildingInfo);
		TooltipWidget_BuildBuilding->SetVisibility(HUDStatics::TOOLTIP_VISIBILITY);
	}
}

void URTSHUD::ShowTooltipWidget_PersistentPanel_TrainUnit(const FDisplayInfoBase & UnitInfo)
{
	if (TooltipWidget_TrainUnit != nullptr)
	{
		TooltipWidget_TrainUnit->SetupFor(UnitInfo);
		TooltipWidget_TrainUnit->SetVisibility(HUDStatics::TOOLTIP_VISIBILITY);
	}
}

void URTSHUD::ShowTooltipWidget_PersistentPanel_ResearchUpgrade(const FDisplayInfoBase & UpgradeInfo)
{
	if (TooltipWidget_ResearchUpgrade != nullptr)
	{
		TooltipWidget_ResearchUpgrade->SetupFor(UpgradeInfo);
		TooltipWidget_ResearchUpgrade->SetVisibility(HUDStatics::TOOLTIP_VISIBILITY);
	}
}

void URTSHUD::ShowTooltipWidget_ProductionQueueSlot_BuildBuilding(const FDisplayInfoBase & BuildingInfo)
{
	if (TooltipWidget_BuildBuilding != nullptr)
	{
		TooltipWidget_BuildBuilding->SetupFor(BuildingInfo);
		TooltipWidget_BuildBuilding->SetVisibility(HUDStatics::TOOLTIP_VISIBILITY);
	}
}

void URTSHUD::ShowTooltipWidget_ProductionQueueSlot_TrainUnit(const FDisplayInfoBase & UnitInfo)
{
	if (TooltipWidget_TrainUnit != nullptr)
	{
		TooltipWidget_TrainUnit->SetupFor(UnitInfo);
		TooltipWidget_TrainUnit->SetVisibility(HUDStatics::TOOLTIP_VISIBILITY);
	}
}

void URTSHUD::ShowTooltipWidget_ProductionQueueSlot_ResearchUpgrade(const FDisplayInfoBase & UpgradeInfo)
{
	if (TooltipWidget_ResearchUpgrade != nullptr)
	{
		TooltipWidget_ResearchUpgrade->SetupFor(UpgradeInfo);
		TooltipWidget_ResearchUpgrade->SetVisibility(HUDStatics::TOOLTIP_VISIBILITY);
	}
}

void URTSHUD::ShowTooltipWidget_InventorySlot(const FInventorySlotState & InvSlot, const FInventoryItemInfo & ItemsInfo)
{
	if (TooltipWidget_InventoryItem != nullptr)
	{
		TooltipWidget_InventoryItem->SetupFor(InvSlot, ItemsInfo);
		TooltipWidget_InventoryItem->SetVisibility(HUDStatics::TOOLTIP_VISIBILITY);
	}
}

void URTSHUD::ShowTooltipWidget_ShopSlot(const FItemOnDisplayInShopSlot & ShopSlot, const FInventoryItemInfo & ItemsInfo)
{
	if (TooltipWidget_InventoryItem != nullptr) 
	{
		TooltipWidget_InventoryItem->SetupFor(ShopSlot, ItemsInfo);
		TooltipWidget_InventoryItem->SetVisibility(HUDStatics::TOOLTIP_VISIBILITY);
	}
}

void URTSHUD::ShowTooltipWidget_InventoryItemInWorld(const AInventoryItem * InventoryItemActor)
{
	// Can be null if user did not set a BP for it. Assuming that they do not want tooltip widgets for inv items
	if (TooltipWidget_InventoryItem != nullptr)
	{
		TooltipWidget_InventoryItem->SetupFor(InventoryItemActor);
		TooltipWidget_InventoryItem->SetVisibility(HUDStatics::TOOLTIP_VISIBILITY);
	}
}

void URTSHUD::ShowTooltipWidget_GlobalSkillsPanelButton(const FAquiredCommanderAbilityState * AbilityState)
{
	if (TooltipWidget_GlobalSkillsPanelButton != nullptr)
	{
		TooltipWidget_GlobalSkillsPanelButton->SetupFor(AbilityState, false);
		TooltipWidget_GlobalSkillsPanelButton->SetVisibility(HUDStatics::TOOLTIP_VISIBILITY);
	}
}

void URTSHUD::ShowTooltipWidget_CommanderSkillTreeNode(const FCommanderAbilityTreeNodeInfo * ButtonNodeInfo)
{
	if (TooltipWidget_CommanderSkillTreeNode != nullptr)
	{
		TooltipWidget_CommanderSkillTreeNode->SetupFor(ButtonNodeInfo, false);
		TooltipWidget_CommanderSkillTreeNode->SetVisibility(HUDStatics::TOOLTIP_VISIBILITY);
	}
}

void URTSHUD::HideTooltipWidget(UWidget * Button, EUIElementType ButtonType)
{
	/* Alternative way to do this if only one tooltip can be shown at a time: 
	- whenever a tooltip widget is shown store it in some variable like CurrentTooltipWidget. 
	Then just call CurrentTooltipWidget->SetVisibility(ESlateVisibility::Hidden) */

	if (ButtonType == EUIElementType::SelectablesActionBar)
	{
		UContextActionButton * ActionButton = static_cast<UContextActionButton*>(Button);
		const FContextButton * ActionButtonInfo = ActionButton->GetButtonType();

		if (ActionButtonInfo->IsForBuildBuilding())
		{
			if (TooltipWidget_BuildBuilding != nullptr)
			{
				TooltipWidget_BuildBuilding->SetVisibility(ESlateVisibility::Hidden);
			}
		}
		else if (ActionButtonInfo->IsForTrainUnit())
		{
			if (TooltipWidget_TrainUnit != nullptr)
			{
				TooltipWidget_TrainUnit->SetVisibility(ESlateVisibility::Hidden);
			}
		}
		else if (ActionButtonInfo->IsForResearchUpgrade())
		{
			if (TooltipWidget_ResearchUpgrade != nullptr)
			{
				TooltipWidget_ResearchUpgrade->SetVisibility(ESlateVisibility::Hidden);
			}
		}
		else // Assumed for ability
		{
			if (TooltipWidget_Abilities != nullptr)
			{
				TooltipWidget_Abilities->SetVisibility(ESlateVisibility::Hidden);
			}
		}
	}
	else if (ButtonType == EUIElementType::PersistentPanel)
	{
		UHUDPersistentTabButton * ProductionButton = static_cast<UHUDPersistentTabButton*>(Button);
		const FContextButton * ButtonInfo = ProductionButton->GetButtonType();

		if (ButtonInfo->IsForBuildBuilding())
		{
			if (TooltipWidget_BuildBuilding != nullptr)
			{
				TooltipWidget_BuildBuilding->SetVisibility(ESlateVisibility::Hidden);
			}
		}
		else if (ButtonInfo->IsForTrainUnit())
		{
			if (TooltipWidget_TrainUnit != nullptr)
			{
				TooltipWidget_TrainUnit->SetVisibility(ESlateVisibility::Hidden);
			}
		}
		else // Assumed for research upgrade.
		{
			assert(ButtonInfo->IsForResearchUpgrade());
			
			if (TooltipWidget_ResearchUpgrade != nullptr)
			{
				TooltipWidget_ResearchUpgrade->SetVisibility(ESlateVisibility::Hidden);
			}
		}
	}
	else if (ButtonType == EUIElementType::ProductionQueueSlot)
	{
		UProductionQueueButton * ProductionButton = CastChecked<UProductionQueueButton>(Button);
		const FContextButton * ButtonInfo = ProductionButton->GetButtonType();
	
		if (ButtonInfo->IsForBuildBuilding())
		{
			if (TooltipWidget_BuildBuilding != nullptr)
			{
				TooltipWidget_BuildBuilding->SetVisibility(ESlateVisibility::Hidden);
			}
		}
		else if (ButtonInfo->IsForTrainUnit())
		{
			if (TooltipWidget_TrainUnit != nullptr)
			{
				TooltipWidget_TrainUnit->SetVisibility(ESlateVisibility::Hidden);
			}
		}
		else // Assumed for research upgrade.
		{
			assert(ButtonInfo->IsForResearchUpgrade());
	
			if (TooltipWidget_ResearchUpgrade != nullptr)
			{
				TooltipWidget_ResearchUpgrade->SetVisibility(ESlateVisibility::Hidden);
			}
		}
	}
	else if (ButtonType == EUIElementType::InventorySlot)
	{
		if (TooltipWidget_InventoryItem != nullptr)
		{
			TooltipWidget_InventoryItem->SetVisibility(ESlateVisibility::Hidden);
		}
	}
	else if (ButtonType == EUIElementType::ShopSlot)
	{
		if (TooltipWidget_InventoryItem != nullptr)
		{
			TooltipWidget_InventoryItem->SetVisibility(ESlateVisibility::Hidden);
		}
	}
	else if (ButtonType == EUIElementType::GlobalSkillsPanelButton)
	{
		if (TooltipWidget_GlobalSkillsPanelButton != nullptr)
		{
			TooltipWidget_GlobalSkillsPanelButton->SetVisibility(ESlateVisibility::Hidden);
		}
	}
	else if (ButtonType == EUIElementType::CommanderSkillTreeNode)
	{
		if (TooltipWidget_CommanderSkillTreeNode != nullptr)
		{
			TooltipWidget_CommanderSkillTreeNode->SetVisibility(ESlateVisibility::Hidden);
		}
	}
	else // Assumed NoTooltipRequired
	{
		assert(ButtonType == EUIElementType::NoTooltipRequired);
	}
}

void URTSHUD::HideTooltipWidget_InventoryItemInWorld(AInventoryItem * InventoryItem)
{
	if (TooltipWidget_InventoryItem != nullptr)
	{
		/* Fun fact: if you hover your mouse over an inventory item in the world and then 
		manage to move your mouse over a inventory slot without a gap between the two then 
		the tooltip widget for the inventory slot will be hidden when really it should be 
		shown. This is probably because the order of events is something like:
		- UI button becomes hovered. We show its tooltip 
		- inv item in world becomes unhovered. Behavior is to hide the inventory tooltip 
		So to solve this we have to check if the tooltip widget contains the info for the 
		unhovered world inv item. If not then we do nothing */
		
		if (TooltipWidget_InventoryItem->IsSetupFor(InventoryItem))
		{
			TooltipWidget_InventoryItem->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void URTSHUD::AddToEscapeRequestResponableWidgets(UInGameWidgetBase * Widget)
{
	/* If this is thrown then a possibility you've not checked a widget is showing before showing 
	it e.g. chat input is showing and another request to show chat input comes through and 
	you try show it again */
	assert(EscapeRequestResponableWidgets.Contains(Widget) == false);
	EscapeRequestResponableWidgets.Emplace(Widget);
}

void URTSHUD::RemoveFromEscapeRequestResponableWidgets(UInGameWidgetBase * Widget)
{
	/* Usually widgets that want to remove themselves from here are last or near last 
	in the array so iterate backwards */

	for (int32 i = EscapeRequestResponableWidgets.Num() - 1; i >= 0; --i)
	{
		if (EscapeRequestResponableWidgets[i] == Widget)
		{
			EscapeRequestResponableWidgets.RemoveAt(i, 1, false);
			return;
		}
	}

	/* If here then did not find widget in array. It is best to check whether the widget is 
	actually showing first before calling this func to avoid this ever happening */
	UE_LOG(RTSLOG, Fatal, TEXT("Failed to find widget [%s] in container "), *Widget->GetClass()->GetName());
}


//####################################################
//----------------------------------------------------
//	Context Menu
//----------------------------------------------------
//####################################################

bool USelectableContextMenu::SetupWidget(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController)
{
	if (Super::SetupWidget(InGameInstance, InPlayerController))
	{
		return true;
	}

	/* Setup child widgets */
	if (IsWidgetBound(Widget_Info))
	{
		Widget_Info->SetupWidget(InGameInstance, InPlayerController);
	}
	if (IsWidgetBound(Widget_Actions))
	{
		Widget_Actions->SetupWidget(InGameInstance, InPlayerController);
	}

	return false;
}

void USelectableContextMenu::OnTick(float InDeltaTime)
{
	if (IsWidgetBound(Widget_Info))
	{
		Widget_Info->OnTick(InDeltaTime);
	}
	if (IsWidgetBound(Widget_Actions))
	{
		Widget_Actions->OnTick(InDeltaTime);
	}
}

void USelectableContextMenu::OnPlayerSelectionChanged(const TScriptInterface < ISelectable > NewCurrentSelected)
{
	CurrentSelected = NewCurrentSelected;

	const FSelectableAttributesBase & Attributes = NewCurrentSelected->GetAttributesBase();

	if (IsWidgetBound(Widget_Info))
	{
		Widget_Info->OnPlayerSelectionChanged(NewCurrentSelected, Attributes);
	}
	if (IsWidgetBound(Widget_Actions))
	{
		Widget_Actions->OnPlayerSelectionChanged(NewCurrentSelected, Attributes);
	}

	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void USelectableContextMenu::OnPlayerNoSelection()
{
	CurrentSelected = nullptr;

	if (IsWidgetBound(Widget_Info))
	{
		Widget_Info->OnNoPlayerSelection();
	}
	if (IsWidgetBound(Widget_Actions))
	{
		Widget_Actions->OnNoPlayerSelection();
	}

	SetVisibility(ESlateVisibility::Collapsed);
}

void USelectableContextMenu::Selected_OnHealthChanged(float InNewAmount, float MaxHealth)
{
	if (IsWidgetBound(Widget_Info))
	{
		Widget_Info->Selected_OnHealthChanged(InNewAmount, MaxHealth);
	}
}

void USelectableContextMenu::Selected_OnMaxHealthChanged(float NewMaxHealth, float CurrentHealth)
{
	if (IsWidgetBound(Widget_Info))
	{
		Widget_Info->Selected_OnMaxHealthChanged(NewMaxHealth, CurrentHealth);
	}
}

void USelectableContextMenu::Selected_OnCurrentAndMaxHealthChanged(float NewMaxHealth, float CurrentHealth)
{
	if (IsWidgetBound(Widget_Info))
	{
		Widget_Info->Selected_OnCurrentAndMaxHealthChanged(NewMaxHealth, CurrentHealth);
	}
}

void USelectableContextMenu::Selected_OnSelectableResourceCurrentAmountChanged(float CurrentAmount, int32 MaxAmount)
{
	if (IsWidgetBound(Widget_Info))
	{
		Widget_Info->Selected_OnSelectableResourceCurrentAmountChanged(CurrentAmount, MaxAmount);
	}
}

void USelectableContextMenu::Selected_OnSelectableResourceMaxAmountChanged(float CurrentAmount, int32 MaxAmount)
{
	if (IsWidgetBound(Widget_Info))
	{
		Widget_Info->Selected_OnSelectableResourceMaxAmountChanged(CurrentAmount, MaxAmount);
	}
}

void USelectableContextMenu::Selected_OnSelectableResourceCurrentAndMaxAmountsChanged(float CurrentAmount, int32 MaxAmount)
{
	if (IsWidgetBound(Widget_Info))
	{
		Widget_Info->Selected_OnSelectableResourceCurrentAndMaxAmountsChanged(CurrentAmount, MaxAmount);
	}
}

void USelectableContextMenu::Selected_OnSelectableResourceRegenRateChanged(float PerSecondRegenRate)
{
	if (IsWidgetBound(Widget_Info))
	{
		Widget_Info->Selected_OnSelectableResourceRegenRateChanged(PerSecondRegenRate);
	}
}

void USelectableContextMenu::Selected_OnRankChanged(float ExperienceTowardsNextRank, uint8 InNewRank, float ExperienceRequired)
{
	if (IsWidgetBound(Widget_Info))
	{
		Widget_Info->Selected_OnRankChanged(ExperienceTowardsNextRank, InNewRank, ExperienceRequired);
	}
}

void USelectableContextMenu::Selected_OnCurrentRankExperienceChanged(float InNewAmount, uint8 CurrentRank, float ExperienceRequired)
{
	if (IsWidgetBound(Widget_Info))
	{
		Widget_Info->Selected_OnCurrentRankExperienceChanged(InNewAmount, CurrentRank, ExperienceRequired);
	}
}

void USelectableContextMenu::Selected_OnImpactDamageChanged(float InNewAmount)
{
	if (IsWidgetBound(Widget_Info))
	{
		Widget_Info->Selected_OnImpactDamageChanged(InNewAmount);
	}
}

void USelectableContextMenu::Selected_OnAttackRateChanged(float AttackRate)
{
	if (IsWidgetBound(Widget_Info))
	{
		Widget_Info->Selected_OnAttackRateChanged(AttackRate);
	}
}

void USelectableContextMenu::Selected_OnAttackRangeChanged(float AttackRange)
{
	if (IsWidgetBound(Widget_Info))
	{
		Widget_Info->Selected_OnAttackRangeChanged(AttackRange);
	}
}

void USelectableContextMenu::Selected_OnDefenseMultiplierChanged(float NewDefenseMultiplier)
{
	if (IsWidgetBound(Widget_Info))
	{
		Widget_Info->Selected_OnDefenseMultiplierChanged(NewDefenseMultiplier);
	}
}

void USelectableContextMenu::Selected_OnBuffApplied(const FStaticBuffOrDebuffInstanceInfo & BuffInfo)
{
	if (IsWidgetBound(Widget_Info))
	{
		Widget_Info->Selected_OnBuffApplied(BuffInfo);
	}
}

void USelectableContextMenu::Selected_OnDebuffApplied(const FStaticBuffOrDebuffInstanceInfo & DebuffInfo)
{
	if (IsWidgetBound(Widget_Info))
	{
		Widget_Info->Selected_OnDebuffApplied(DebuffInfo);
	}
}

void USelectableContextMenu::Selected_OnBuffApplied(const FTickableBuffOrDebuffInstanceInfo & BuffInfo)
{
	if (IsWidgetBound(Widget_Info))
	{
		Widget_Info->Selected_OnBuffApplied(BuffInfo);
	}
}

void USelectableContextMenu::Selected_OnDebuffApplied(const FTickableBuffOrDebuffInstanceInfo & DebuffInfo)
{
	if (IsWidgetBound(Widget_Info))
	{
		Widget_Info->Selected_OnDebuffApplied(DebuffInfo);
	}
}

void USelectableContextMenu::Selected_OnTickableBuffRemoved(int32 ArrayIndex, EBuffAndDebuffRemovalReason RemovalReason)
{
	if (IsWidgetBound(Widget_Info))
	{
		Widget_Info->Selected_OnTickableBuffRemoved(ArrayIndex, RemovalReason);
	}
}

void USelectableContextMenu::Selected_OnTickableDebuffRemoved(int32 ArrayIndex, EBuffAndDebuffRemovalReason RemovalReason)
{
	if (IsWidgetBound(Widget_Info))
	{
		Widget_Info->Selected_OnTickableDebuffRemoved(ArrayIndex, RemovalReason);
	}
}

void USelectableContextMenu::Selected_OnStaticBuffRemoved(int32 ArrayIndex, EBuffAndDebuffRemovalReason RemovalReason)
{
	if (IsWidgetBound(Widget_Info))
	{
		Widget_Info->Selected_OnStaticBuffRemoved(ArrayIndex, RemovalReason);
	}
}

void USelectableContextMenu::Selected_OnStaticDebuffRemoved(int32 ArrayIndex, EBuffAndDebuffRemovalReason RemovalReason)
{
	if (IsWidgetBound(Widget_Info))
	{
		Widget_Info->Selected_OnStaticDebuffRemoved(ArrayIndex, RemovalReason);
	}
}

void USelectableContextMenu::Selected_UpdateTickableBuffDuration(int32 ArrayIndex, float DurationRemaining)
{
	if (IsWidgetBound(Widget_Info))
	{
		Widget_Info->Selected_UpdateTickableBuffDuration(ArrayIndex, DurationRemaining);
	}
}

void USelectableContextMenu::Selected_UpdateTickableDebuffDuration(int32 ArrayIndex, float DurationRemaining)
{
	if (IsWidgetBound(Widget_Info))
	{
		Widget_Info->Selected_UpdateTickableDebuffDuration(ArrayIndex, DurationRemaining);
	}
}

void USelectableContextMenu::Selected_OnItemAddedToInventory(int32 SlotIndex, const FInventorySlotState & UpdatedSlot)
{
	if (IsWidgetBound(Widget_Info))
	{
		Widget_Info->Selected_OnItemAddedToInventory(SlotIndex, UpdatedSlot);
	}
}

void USelectableContextMenu::Selected_OnItemsRemovedFromInventory(const FInventory * Inventory, const TMap<uint8, FInventoryItemQuantityPair>& RemovedItems)
{
	if (IsWidgetBound(Widget_Info))
	{
		Widget_Info->Selected_OnItemsRemovedFromInventory(Inventory, RemovedItems);
	}
}

void USelectableContextMenu::Selected_OnInventoryPositionsSwapped(uint8 DisplayIndex1, uint8 DisplayIndex2)
{
	if (IsWidgetBound(Widget_Info))
	{
		Widget_Info->Selected_OnInventoryPositionsSwapped(DisplayIndex1, DisplayIndex2);
	}
}

void USelectableContextMenu::Selected_OnInventoryItemUsed(uint8 DisplayIndex, float UseAbilityTotalCooldown)
{
	if (IsWidgetBound(Widget_Info))
	{
		Widget_Info->Selected_OnInventoryItemUsed(DisplayIndex, UseAbilityTotalCooldown);
	}
}

void USelectableContextMenu::Selected_OnInventoryItemUseCooldownFinished(uint8 InventorySlotDisplayIndex)
{
	if (IsWidgetBound(Widget_Info))
	{
		Widget_Info->Selected_OnInventoryItemUseCooldownFinished(InventorySlotDisplayIndex);
	}
}

void USelectableContextMenu::Selected_OnItemPurchasedFromShop(uint8 ShopSlotIndex)
{
	if (IsWidgetBound(Widget_Info))
	{
		Widget_Info->Selected_OnItemPurchasedFromShop(ShopSlotIndex);
	}
}

void USelectableContextMenu::Selected_OnInventoryItemSold(uint8 InvDisplayIndex)
{
	if (IsWidgetBound(Widget_Info))
	{
		Widget_Info->Selected_OnInventoryItemSold(InvDisplayIndex);
	}
}

void USelectableContextMenu::OnBuildingConstructed(EBuildingType BuildingType, bool bFirstOfItsType)
{
	if (IsWidgetBound(Widget_Actions))
	{
		Widget_Actions->OnBuildingConstructed(BuildingType, bFirstOfItsType);
	}
}

void USelectableContextMenu::OnBuildingDestroyed(EBuildingType BuildingType, bool bLastOfItsType)
{
	if (IsWidgetBound(Widget_Actions))
	{
		Widget_Actions->OnBuildingDestroyed(BuildingType, bLastOfItsType);
	}
}

void USelectableContextMenu::OnUpgradeComplete(EUpgradeType UpgradeType)
{
	if (IsWidgetBound(Widget_Actions))
	{
		Widget_Actions->OnUpgradeComplete(UpgradeType);
	}
}

void USelectableContextMenu::OnItemAddedToProductionQueue(const FTrainingInfo & Item, const FProductionQueue & Queue, AActor * Producer)
{
	if (IsWidgetBound(Widget_Info))
	{
		Widget_Info->OnItemAddedToProductionQueue(Item, Queue, Producer);
	}
}

void USelectableContextMenu::OnItemAddedAndProductionStarted(const FTrainingInfo & Item, const FProductionQueue & Queue, AActor * Producer)
{
	if (IsWidgetBound(Widget_Info))
	{
		Widget_Info->OnItemAddedAndProductionStarted(Item, Queue, Producer);
	}
}

void USelectableContextMenu::OnProductionComplete(const FTrainingInfo & Item, const FProductionQueue & Queue, 
	uint8 NumRemoved, AActor * Producer)
{
	if (IsWidgetBound(Widget_Info))
	{
		Widget_Info->OnProductionComplete(Item, Queue, NumRemoved, Producer);
	}

	if (IsWidgetBound(Widget_Actions))
	{
		Widget_Actions->OnProductionComplete(Item, Queue, NumRemoved, Producer);
	}
}


//===============================================================================================
//	Selectable Info
//===============================================================================================

USelectableInfo::USelectableInfo(const FObjectInitializer & ObjectInitializer)
	: Super(ObjectInitializer)
{
	LastSelectableResourceType = ESelectableResourceType::None;
}

bool USelectableInfo::SetupWidget(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController)
{
	if (Super::SetupWidget(InGameInstance, InPlayerController))
	{
		return true;
	}

	/* Popualate production queue buttons array. Hoping order during iteration is same order
	they were placed in editor */

	TArray < UWidget * > AllChildren;
	WidgetTree->GetAllWidgets(AllChildren);

	for (const auto & Widget : AllChildren)
	{
		if (Widget->IsA(UProductionQueueButton::StaticClass()))
		{
			UProductionQueueButton * Button = CastChecked<UProductionQueueButton>(Widget);

			Button->Setup(InGameInstance, InPlayerController, FI);

			// Treat button as if it does not have any item from production queue in it
			Button->Unassign();

			ProductionQueueSlots.Emplace(Button);
		}
	}

	UE_CLOG(FI->GetLargestProductionQueueCapacity() > ProductionQueueSlots.Num(), RTSLOG, Fatal, 
		TEXT("Faction \"%s\" has a maximum queue size of %d but only has %d production queue "
		"slot buttons on its HUD"), TO_STRING(EFaction, FI->GetFaction()), FI->GetLargestProductionQueueCapacity(),
			ProductionQueueSlots.Num());

	if (IsWidgetBound(Panel_ItemsOnDisplay))
	{	
		const int32 MaxItemsOnDisplayListSize = GI->GetLargestShopCatalogueSize();
		
		/* Setup the widget that shows what inventory items are on display and usually for sale */

		ItemOnDisplayInShopButtons.Reserve(MaxItemsOnDisplayListSize);

		if (bAutoCreateItemForSaleWidgets)
		{
			UE_CLOG(ItemForSaleWidget_BP == nullptr, RTSLOG, Fatal, TEXT("Trying to auto populate "
				"the items for sale widget but no widgetblueprint is set. Either set ItemForSaleWidget_BP "
				"or set bAutoCreateItemForSaleWidgets to false and add your own buttons in editor"));

			for (int32 i = 0; i < MaxItemsOnDisplayListSize; ++i)
			{
				UItemOnDisplayInShopButton * Widget = CreateWidget<UItemOnDisplayInShopButton>(PC, ItemForSaleWidget_BP);

				/* Add the newly created widget to the panel */
				UPanelSlot * PanelSlot = Panel_ItemsOnDisplay->AddChild(Widget);
				OnItemOnDisplayButtonAddedToPanel(Panel_ItemsOnDisplay, PanelSlot, Widget);

				Widget->SetupWidget(InGameInstance, InPlayerController);
				Widget->SetupMore(this);

				ItemOnDisplayInShopButtons.Emplace(Widget);
			}
		}
		else
		{
			/* Iterating all children again... could do this in the loop above if it's better
			for performance. For readability it is like it is now */
			for (const auto & Elem : AllChildren)
			{
				if (Elem->IsA(UItemOnDisplayInShopButton::StaticClass()))
				{
					UItemOnDisplayInShopButton * ButtonWidget = CastChecked<UItemOnDisplayInShopButton>(Elem);

					ButtonWidget->SetupWidget(InGameInstance, InPlayerController);
					ButtonWidget->SetupMore(this);

					ItemOnDisplayInShopButtons.Emplace(ButtonWidget);
				}
			}

			UE_CLOG(ItemOnDisplayInShopButtons.Num() < MaxItemsOnDisplayListSize, RTSLOG, Fatal,
				TEXT("Not enough buttons added to Panel_ItemsOnDisplay. Need %d more"), 
				MaxItemsOnDisplayListSize - ItemOnDisplayInShopButtons.Num());

			UE_CLOG(ItemOnDisplayInShopButtons.Num() > MaxItemsOnDisplayListSize, RTSLOG, Warning,
				TEXT("Too many buttons added to Panel_ItemsOnDisplay. There are %d too many. For "
					"performance's sake it is best to remove these before shipping build."),
				ItemOnDisplayInShopButtons.Num() - MaxItemsOnDisplayListSize);
		}
	}

	if (IsWidgetBound(Panel_Inventory))
	{
		const int32 MaxInventoryCapacity = GI->GetLargestInventoryCapacity();
		
		/* Setup inventory stuff */

		InventoryButtons.Reserve(MaxInventoryCapacity);

		if (bAutoCreateInventoryItemWidgets)
		{
			UE_CLOG(InventoryItemWidget_BP == nullptr, RTSLOG, Fatal, TEXT("Trying to auto populate "
				"inventory panel but no button blueprint is set. Either set InventoryItemWidget_BP or "
				"set bAutoCreateInventoryItemWidgets to false and add your own buttons in editor"));
			
			for (int32 i = 0; i < MaxInventoryCapacity; ++i)
			{
				UInventoryItemButton * ItemWidget = CreateWidget<UInventoryItemButton>(PC, InventoryItemWidget_BP);

				/* Add the newly created widget to the panel */
				UPanelSlot * PanelSlot = Panel_Inventory->AddChild(ItemWidget);
				OnInventoryItemButtonAddedToPanel(Panel_Inventory, PanelSlot, ItemWidget);

				ItemWidget->SetupWidget(InGameInstance, InPlayerController);

				InventoryButtons.Emplace(ItemWidget);
			}
		}
		else
		{
			/* Iterating all children again... could do this in the loop above if it's better 
			for performance. For readability it is like it is now */
			for (const auto & Elem : AllChildren)
			{
				if (Elem->IsA(UInventoryItemButton::StaticClass()))
				{
					UInventoryItemButton * AsItemButton = CastChecked<UInventoryItemButton>(Elem);

					AsItemButton->SetupWidget(InGameInstance, InPlayerController);

					InventoryButtons.Emplace(AsItemButton);
				}
			}

			UE_CLOG(InventoryButtons.Num() < MaxInventoryCapacity, RTSLOG, Fatal, 
				TEXT("Not enough inventory buttons added to Panel_Inventory to accomodate every "
					"selectable. Add %d more."), MaxInventoryCapacity - InventoryButtons.Num());

			UE_CLOG(InventoryButtons.Num() > MaxInventoryCapacity, RTSLOG, Warning,
				TEXT("Too many buttons added to Panel_Inventory. %d can be removed. For performance's "
					"sake it is best to remove these before shipping build"), 
					InventoryButtons.Num() - MaxInventoryCapacity);
		}
	}

	return false;
}

void USelectableInfo::OnTick(float InDeltaTime)
{
	if (Statics::IsValid(CurrentSelected) && ProductionQueue != nullptr)
	{
		/* Update fill percent of production queue progress bar */
		UpdateProgressBarProductionQueue(CurrentSelected);
	}

	const FTimerManager & TimerManager = GI->GetTimerManager();

	/* Might need a null check here for CurrentSelected because we'll essentially be updating 
	text and progress bars that are hidden if we don't have a primary selected */
	// Update the inventory items cooldowns
	if (CurrentSelected != nullptr)
	{
		for (const auto & Elem : CoolingDownInventorySlots)
		{
			Elem->UpdateCooldown(InDeltaTime, TimerManager);
		}
	}
}

USingleBuffOrDebuffWidget * USelectableInfo::GetStaticBuffWidget()
{
	UE_CLOG(SingleStaticBuffWidget_BP == nullptr, RTSLOG, Fatal, TEXT("Widget blueprint is null, "
		"need to set it in editor"));
	
	USingleBuffOrDebuffWidget * Widget = CreateWidget<USingleBuffOrDebuffWidget>(PC, SingleStaticBuffWidget_BP);
	assert(Widget != nullptr);

	return Widget;
}

USingleTickableBuffOrDebuffWidget * USelectableInfo::GetTickableBuffWidget()
{
	UE_CLOG(SingleTickableBuffWidget_BP == nullptr, RTSLOG, Fatal, TEXT("Buff widget is null, need to "
		"set one in editor"));
	
	USingleTickableBuffOrDebuffWidget * Widget = CreateWidget<USingleTickableBuffOrDebuffWidget>(PC, SingleTickableBuffWidget_BP);
	assert(Widget != nullptr);

	return Widget;
}

USingleBuffOrDebuffWidget * USelectableInfo::GetStaticDebuffWidget()
{
	UE_CLOG(SingleStaticDebuffWidget_BP == nullptr, RTSLOG, Fatal, TEXT("Widget bluprint is null, "
		"need to set one in editor"));
	
	USingleBuffOrDebuffWidget * Widget = CreateWidget<USingleBuffOrDebuffWidget>(PC, SingleStaticDebuffWidget_BP);
	assert(Widget != nullptr);

	return Widget;
}

USingleTickableBuffOrDebuffWidget * USelectableInfo::GetTickableDebuffWidget()
{
	UE_CLOG(SingleTickableDebuffWidget_BP == nullptr, RTSLOG, Fatal, TEXT("Widget blueprint is null, "
		"need to set one in editor"));

	USingleTickableBuffOrDebuffWidget * Widget = CreateWidget<USingleTickableBuffOrDebuffWidget>(PC, SingleTickableDebuffWidget_BP);
	assert(Widget != nullptr);

	return Widget;
}

void USelectableInfo::OnItemOnDisplayButtonAddedToPanel(UPanelWidget * PanelWidget, UPanelSlot * PanelSlot, 
	UItemOnDisplayInShopButton * JustAdded)
{
}

void USelectableInfo::OnInventoryItemButtonAddedToPanel(UPanelWidget * Panel, UPanelSlot * PanelSlot, UInventoryItemButton * JustAdded)
{
}

void USelectableInfo::SetInfoVisibilities(const TScriptInterface < ISelectable > & Selectable, const FSelectableAttributesBase & Attributes)
{
	/* If a neutral then do not show health info */
	if (IsWidgetBound(Text_Health))
	{
		const ESlateVisibility NewVis = (Attributes.GetAffiliation() == EAffiliation::Neutral)
			? ESlateVisibility::Collapsed : ESlateVisibility::Visible;
		Text_Health->SetVisibility(NewVis);
	}
	if (IsWidgetBound(ProgressBar_Health))
	{
		const ESlateVisibility NewVis = (Attributes.GetAffiliation() == EAffiliation::Neutral)
			? ESlateVisibility::Collapsed : ESlateVisibility::Visible;
		ProgressBar_Health->SetVisibility(NewVis);
	}

	const FSelectableAttributesBasic * AttriBasic = Selectable->GetAttributes();
	const ESelectableResourceType CurrentSelectedsSelectableResourceType = (AttriBasic != nullptr)
		? AttriBasic->GetSelectableResourceType() : ESelectableResourceType::None;
	const ESlateVisibility SelectableResourceWidgetVis = (CurrentSelectedsSelectableResourceType != ESelectableResourceType::None)
		? ESlateVisibility::HitTestInvisible : HUDStatics::HIDDEN_VISIBILITY;
	
	/* Check if user is using a panel widget. If so just make that visible */
	if (IsWidgetBound(Panel_SelectableResource))
	{
		// Set progress bar color
		if (SelectableResourceWidgetVis == ESlateVisibility::HitTestInvisible
			&& LastSelectableResourceType != CurrentSelectedsSelectableResourceType)
		{
			const FSelectableResourceColorInfo & ResourceInfo = GI->GetSelectableResourceInfo(CurrentSelectedsSelectableResourceType);
			ProgressBar_SelectableResource->SetFillColorAndOpacity(ResourceInfo.GetPBarFillColor());
		}

		// Set panel vis
		Panel_SelectableResource->SetVisibility(SelectableResourceWidgetVis);
	}
	else // Case where user isn't using a panel but may still have individual widgets on there so check each one
	{
		if (IsWidgetBound(Text_SelectableResource))
		{
			Text_SelectableResource->SetVisibility(SelectableResourceWidgetVis);
		}
		if (IsWidgetBound(Text_SelectableResourceMax))
		{
			Text_SelectableResourceMax->SetVisibility(SelectableResourceWidgetVis);
		}
		if (IsWidgetBound(ProgressBar_SelectableResource))
		{
			ProgressBar_SelectableResource->SetVisibility(SelectableResourceWidgetVis);

			/* Set the color of the progress bar. We will check whether the color has actually
			changed because UProgressBar::SetFillColorAndOpacity will auto invalidate layout. Actually
			it would be perhaps better to check the actual FLinearColor instead of the selectable
			resource type because different resource types could use the same color. If that is the
			case then LastSelectableResourceType can be removed */
			if (SelectableResourceWidgetVis == ESlateVisibility::HitTestInvisible
				&& LastSelectableResourceType != CurrentSelectedsSelectableResourceType)
			{
				const FSelectableResourceColorInfo & ResourceInfo = GI->GetSelectableResourceInfo(CurrentSelectedsSelectableResourceType);
				ProgressBar_SelectableResource->SetFillColorAndOpacity(ResourceInfo.GetPBarFillColor());
			}
		}
		if (IsWidgetBound(Text_SelectableResourceRegenRate))
		{
			Text_SelectableResourceRegenRate->SetVisibility(SelectableResourceWidgetVis);
		}
	}
	LastSelectableResourceType = CurrentSelectedsSelectableResourceType;

	/* White image shown if texture is null. We don't want that so hide widget if that is the case */
	if (IsWidgetBound(Image_Icon))
	{
		const ESlateVisibility ImageVis = (Attributes.GetHUDImage_Normal().GetResourceObject() == nullptr)
			? ESlateVisibility::Collapsed : ESlateVisibility::Visible;
		Image_Icon->SetVisibility(ImageVis);
	}

	const bool bIsOwned = (Attributes.GetAffiliation() == EAffiliation::Owned);
	const bool bCanGainExperience = EXPERIENCE_ENABLED_GAME && Selectable->CanClassGainExperience();

	if (IsWidgetBound(Text_Level))
	{
		const ESlateVisibility NewVis = (bCanGainExperience)
			? ESlateVisibility::HitTestInvisible : HUDStatics::HIDDEN_VISIBILITY;
		Text_Level->SetVisibility(NewVis);
	}
	if (IsWidgetBound(Image_Level))
	{
		const ESlateVisibility NewVis = (bCanGainExperience)
			? ESlateVisibility::HitTestInvisible : HUDStatics::HIDDEN_VISIBILITY;
		Image_Level->SetVisibility(NewVis);
	}

	const ESlateVisibility ExperienceRelatedWidgetNewVis = (bIsOwned && bCanGainExperience) 
		? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
	if (IsWidgetBound(Panel_Experience))
	{
		Panel_Experience->SetVisibility(ExperienceRelatedWidgetNewVis);
	}
	else
	{
		if (IsWidgetBound(Text_ExperienceTowardsNextLevel))
		{
			Text_ExperienceTowardsNextLevel->SetVisibility(ExperienceRelatedWidgetNewVis);
		}
		if (IsWidgetBound(Text_ExperienceRequired))
		{
			Text_ExperienceRequired->SetVisibility(ExperienceRelatedWidgetNewVis);
		}
		if (IsWidgetBound(ProgressBar_Experience))
		{
			ProgressBar_Experience->SetVisibility(ExperienceRelatedWidgetNewVis);
		}
	}

	const bool bShowShowAttackAttributes = ShouldShowAttackAttributes(Selectable);

	if (IsWidgetBound(Text_ImpactDamage))
	{
		const ESlateVisibility NewVis = bShowShowAttackAttributes ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden;
		Text_ImpactDamage->SetVisibility(NewVis);
	}
	if (IsWidgetBound(Text_AttackRate))
	{
		const ESlateVisibility NewVis = bShowShowAttackAttributes ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden;
		Text_AttackRate->SetVisibility(NewVis);
	}
	if (IsWidgetBound(Text_AttackRange))
	{
		const ESlateVisibility NewVis = bShowShowAttackAttributes ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden;
		Text_AttackRange->SetVisibility(NewVis);
	}

	const ESlateVisibility QueueVis = ShouldShowProductionQueueInfo(Selectable)
		? ESlateVisibility::Visible : ESlateVisibility::Collapsed;

	if (IsWidgetBound(ProgressBar_ProductionQueue))
	{
		ProgressBar_ProductionQueue->SetVisibility(QueueVis);
	}
	if (IsWidgetBound(Panel_ProductionQueueInfo))
	{
		Panel_ProductionQueueInfo->SetVisibility(QueueVis);
	}

	if (IsWidgetBound(Panel_ItemsOnDisplay))
	{
		const FShopInfo * ShopAttributes = Selectable->GetShopAttributes();
		
		/* If not null then we know it sells something */
		if (ShopAttributes != nullptr)
		{
			/* Check if our affiliation is good enough to shop here */
			if (ShopAttributes->CanShopHere(Attributes.GetAffiliation()))
			{
				const FUnifiedImageAndSoundFlags UnifiedAssetFlags = GI->GetUnifiedButtonAssetFlags_Shop();

				const int32 NumItemsOnDisplay = ShopAttributes->GetItems().Num();

				/* Show items in buttons */
				for (int32 i = 0; i < NumItemsOnDisplay; ++i)
				{
					UItemOnDisplayInShopButton * Widget = ItemOnDisplayInShopButtons[i].Get();

					Widget->MakeActive(i, ShopAttributes->GetItems()[i], UnifiedAssetFlags);

					Widget->SetVisibility(ESlateVisibility::Visible);
				}

				/* The rest of the buttons on the panel (if any) do not need to be used so make them
				hidden */
				for (int32 i = NumItemsOnDisplay; i < ItemOnDisplayInShopButtons.Num(); ++i)
				{
					ItemOnDisplayInShopButtons[i]->SetVisibility(ESlateVisibility::Hidden);
				}

				Panel_ItemsOnDisplay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			}
			else
			{
				Panel_ItemsOnDisplay->SetVisibility(ESlateVisibility::Hidden);
			}
		}
		else
		{
			Panel_ItemsOnDisplay->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	if (IsWidgetBound(Panel_Inventory))
	{
		/* Check if selectable's affiliation means we can show its inventory */
		if (Attributes.GetAffiliation() <= InventoryItemOptions::WorstAffiliationThatCanSeeInventory)
		{
			const FInventory * Inventory = Selectable->GetInventory();

			/* Null implies they have no inventory */
			if (Inventory != nullptr)
			{
				CoolingDownInventorySlots.Reset();
				
				FTimerManager & WorldTimerManager = GI->GetTimerManager();

				/* Make the correct number of buttons visible and show the items in them if any */
				for (int32 i = 0; i < Inventory->GetCapacity(); ++i)
				{
					UInventoryItemButton * ButtonWidget = InventoryButtons[i].Get();

					const FInventorySlotState & InvSlot = Inventory->GetSlotForDisplayAtIndex(i);

					if (InvSlot.HasItem())
					{
						const FInventoryItemInfo * ItemsInfo = GI->GetInventoryItemInfo(InvSlot.GetItemType());
						
						/* If I get index out of bounds here then I need to populate the selectable's
						inventory TArray in post edit of the FInfantryAttributes. Fill it with "empty"
						slots */
						if (ButtonWidget->MakeActive(InvSlot, i, WorldTimerManager, ItemsInfo))
						{
							CoolingDownInventorySlots.Emplace(ButtonWidget);
						}
					}
					else
					{
						ButtonWidget->MakeActive(Inventory->GetSlotForDisplayAtIndex(i), i,
							WorldTimerManager, nullptr);
					}

					// if (VisIsDifferent()) for performance maybe?
					ButtonWidget->SetVisibility(ESlateVisibility::Visible);
				}

				/* Make the rest hidden */
				for (int32 i = Inventory->GetCapacity(); i < InventoryButtons.Num(); ++i)
				{
					// if (VisIsDifferent()) for performance maybe?
					InventoryButtons[i]->SetVisibility(ESlateVisibility::Hidden);
				}

				Panel_Inventory->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			}
			else
			{
				Panel_Inventory->SetVisibility(ESlateVisibility::Hidden);
			}
		}
		else
		{
			Panel_Inventory->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void USelectableInfo::UpdateTextName(const FSelectableAttributesBase & Attributes)
{
	if (IsWidgetBound(Text_Name))
	{
		Text_Name->SetText(Attributes.GetName());
	}
}

void USelectableInfo::UpdateTextDescription(const FSelectableAttributesBase & Attributes)
{
	if (IsWidgetBound(Text_Description))
	{
		Text_Description->SetText(Attributes.GetDescription());
	}
}

void USelectableInfo::UpdateImageIcon(const FSelectableAttributesBase & Attributes)
{
	if (IsWidgetBound(Image_Icon))
	{
		Image_Icon->SetBrush(Attributes.GetHUDImage_Normal());
	}
}

void USelectableInfo::UpdateTextLevel(uint8 InLevel)
{
	if (IsWidgetBound(Text_Level))
	{
		Text_Level->SetText(FText::AsNumber(InLevel));
	}
}

void USelectableInfo::UpdateImageLevel(const FRankInfo & RankInfo)
{
	if (IsWidgetBound(Image_Level))
	{
		Image_Level->SetBrushFromTexture(RankInfo.GetIcon());
	}
}

void USelectableInfo::UpdateTextExperienceRequired(uint8 Rank, float ExperienceRequired)
{
	if (IsWidgetBound(Text_ExperienceRequired))
	{
		if (Rank == LevelingUpOptions::MAX_LEVEL)
		{
			/* We're at max level. Show the text "INF" for infinity */
			Text_ExperienceRequired->SetText(FText::FromString("INF"));
		}
		else
		{
			Text_ExperienceRequired->SetText(ExperienceRequiredFloatToText(ExperienceRequired));
		}
	}
}

void USelectableInfo::UpdateTextExperienceTowardsNextLevel(float CurrentEXP, uint8 CurrentRank)
{
	if (IsWidgetBound(Text_ExperienceTowardsNextLevel))
	{
		if (CurrentRank == LevelingUpOptions::MAX_LEVEL)
		{
			/* We're at max level. Show infinity text */
			Text_ExperienceTowardsNextLevel->SetText(FText::FromString("INF"));
		}
		else
		{
			Text_ExperienceTowardsNextLevel->SetText(ExperienceGainedFloatToText(CurrentEXP));
		}
	}
}

void USelectableInfo::UpdateProgressBarExperience(float CurrentEXP, uint8 Rank, float ExperienceRequired)
{
	if (IsWidgetBound(ProgressBar_Experience))
	{	
		float NewPercent;
		if (Rank == LevelingUpOptions::MAX_LEVEL)
		{
			/* We are at max level. Behavior I choose is to show a full experience bar */
			NewPercent = 1.f;
		}
		else
		{
			NewPercent = CurrentEXP / ExperienceRequired;
		}

		ProgressBar_Experience->SetPercent(NewPercent);
	}
}

void USelectableInfo::UpdateTextHealth(float NewHealth)
{
	if (IsWidgetBound(Text_Health))
	{
		Text_Health->SetText(CurrentHealthFloatToText(NewHealth));
	}
}

void USelectableInfo::UpdateTextMaxHealth(float NewMaxHealth)
{
	if (IsWidgetBound(Text_MaxHealth))
	{
		Text_MaxHealth->SetText(MaxHealthFloatToText(NewMaxHealth));
	}
}

void USelectableInfo::UpdateProgressBarHealth(float CurrentHealth, float MaxHealth)
{
	if (IsWidgetBound(ProgressBar_Health))
	{
		ProgressBar_Health->SetPercent(CurrentHealth / MaxHealth);
	}
}

void USelectableInfo::UpdateTextSelectableResource(float NewAmount)
{
	if (IsWidgetBound(Text_SelectableResource))
	{
		Text_SelectableResource->SetText(SelectableResourceCurrentAmountFloatToText(NewAmount));
	}
}

void USelectableInfo::UpdateTextSelectableResourceMax(int32 NewAmount)
{
	if (IsWidgetBound(Text_SelectableResourceMax))
	{
		Text_SelectableResourceMax->SetText(FText::AsNumber(NewAmount));
	}
}

void USelectableInfo::UpdateProgressBarSelectableResource(float CurrentAmount, int32 MaxAmount)
{
	if (IsWidgetBound(ProgressBar_SelectableResource))
	{
		ProgressBar_SelectableResource->SetPercent(CurrentAmount / MaxAmount);
	}
}

void USelectableInfo::UpdateTextSelectableResourceRegenRate(float PerSecondRegenRate)
{
	if (IsWidgetBound(Text_SelectableResourceRegenRate))
	{
		Text_SelectableResourceRegenRate->SetText(SelectableResourceRegenRateFloatToText(PerSecondRegenRate));
	}
}

void USelectableInfo::UpdateTextImpactDamage(float Damage)
{
	if (IsWidgetBound(Text_ImpactDamage))
	{
		Text_ImpactDamage->SetText(DamageFloatToText(Damage));
	}
}

void USelectableInfo::UpdateTextAttackRate(float AttackRate)
{
	if (IsWidgetBound(Text_AttackRate))
	{
		Text_AttackRate->SetText(AttackRateFloatToText(AttackRate));
	}
}

void USelectableInfo::UpdateTextAttackRange(float AttackRange)
{
	if (IsWidgetBound(Text_AttackRange))
	{
		Text_AttackRange->SetText(AttackRangeFloatToText(AttackRange));
	}
}

void USelectableInfo::UpdateTextDefenseMultiplier(float DefenseMultiplier)
{
	if (IsWidgetBound(Text_DefenseMultiplier))
	{
		Text_DefenseMultiplier->SetText(DefenseMultiplierFloatToText(DefenseMultiplier));
	}
}

void USelectableInfo::UpdateProgressBarProductionQueue(const TScriptInterface < ISelectable > & Selectable)
{
	if (IsWidgetBound(ProgressBar_ProductionQueue))
	{
		if (ProgressBar_ProductionQueue->GetVisibility() != ESlateVisibility::Collapsed)
		{
			const float QueuePercentage = ProductionQueue->GetPercentageCompleteForUI(GetWorld());

			ProgressBar_ProductionQueue->SetPercent(QueuePercentage);
		}
	}
}

void USelectableInfo::UpdateBuffsAndDebuffs(const TScriptInterface<ISelectable>& Selectable)
{
	/* Ordering on the panel widgets children is statics first followed by tickables */

	if (IsWidgetBound(Panel_Buffs))
	{
		/* Clear any entries left by what was previously selected */
		Panel_Buffs->ClearChildren();
		StaticBuffWidgets.Reset();
		TickableBuffWidgets.Reset();

		if (Selectable->CanClassAcceptBuffsAndDebuffs())
		{
			/* Add any buffs applied to the selectable */
			
			const TArray < FStaticBuffOrDebuffInstanceInfo > * SelectablesStaticBuffArray = Selectable->GetStaticBuffArray();

			for (const auto & Elem : (*SelectablesStaticBuffArray))
			{
				USingleBuffOrDebuffWidget * Widget = GetStaticBuffWidget();
				Widget->SetupFor(Elem, GI);

				StaticBuffWidgets.Emplace(Widget);

				Panel_Buffs->AddChild(Widget);
			}

#if TICKABLE_BUFF_AND_DEBUFF_ENABLED_GAME

			const TArray < FTickableBuffOrDebuffInstanceInfo > * SelectablesTickableBuffArray = Selectable->GetTickableBuffArray();

			for (const auto & Elem : (*SelectablesTickableBuffArray))
			{
				USingleTickableBuffOrDebuffWidget * Widget = GetTickableBuffWidget();
				Widget->SetupFor(Elem, GI);

				TickableBuffWidgets.Emplace(Widget);

				Panel_Buffs->AddChild(Widget);
			}

#endif // TICKABLE_BUFF_AND_DEBUFF_ENABLED_GAME
		}
	}

	if (IsWidgetBound(Panel_Debuffs))
	{
		/* Clear any entries left by what was previously selected */
		Panel_Debuffs->ClearChildren();
		StaticDebuffWidgets.Reset();
		TickableDebuffWidgets.Reset();

		if (Selectable->CanClassAcceptBuffsAndDebuffs())
		{
			/* Add any debuffs applied to the selectable */
			
			const TArray < FStaticBuffOrDebuffInstanceInfo > * SelectablesStaticDebuffArray = Selectable->GetStaticDebuffArray();

			for (const auto & Elem : (*SelectablesStaticDebuffArray))
			{
				USingleBuffOrDebuffWidget * Widget = GetStaticDebuffWidget();
				Widget->SetupFor(Elem, GI);

				StaticDebuffWidgets.Emplace(Widget);

				Panel_Debuffs->AddChild(Widget);
			}

#if TICKABLE_BUFF_AND_DEBUFF_ENABLED_GAME

			const TArray < FTickableBuffOrDebuffInstanceInfo > * SelectablesTickableDebuffArray = Selectable->GetTickableDebuffArray();

			for (const auto & Elem : (*SelectablesTickableDebuffArray))
			{
				USingleTickableBuffOrDebuffWidget * Widget = GetTickableDebuffWidget();
				Widget->SetupFor(Elem, GI);

				TickableDebuffWidgets.Emplace(Widget);

				Panel_Debuffs->AddChild(Widget);
			}

#endif // TICKABLE_BUFF_AND_DEBUFF_ENABLED_GAME
		}
	}
}

void USelectableInfo::UpdateProductionQueueContents()
{
	if (IsWidgetBound(Panel_ProductionQueueInfo))
	{
		if (Panel_ProductionQueueInfo->GetVisibility() != ESlateVisibility::Collapsed)
		{
			if (ProductionQueue != nullptr)
			{
				const FUnifiedImageAndSoundFlags UnifiedAssetFlags = GI->GetUnifiedButtonAssetFlags_ActionBar();
				
				/* Fill slots with what is in queue */
				for (int32 i = 0; i < ProductionQueue->Num(); ++i)
				{
					UProductionQueueButton * Button = ProductionQueueSlots[i];
					Button->Assign((*ProductionQueue)[i], UnifiedAssetFlags);
				}

				/* Make remaining buttons unactive */
				for (int32 i = ProductionQueue->Num(); i < QueueNum; ++i)
				{
					UProductionQueueButton * Button = ProductionQueueSlots[i];
					Button->Unassign();
				}

				QueueNum = ProductionQueue->Num();
			}
		}
	}
}

bool USelectableInfo::ShouldShowProductionQueueInfo(const TScriptInterface<ISelectable> & Selectable) const
{
	/* If a building that can produce stuff and is controlled by us */
	return Selectable->GetAttributesBase().GetSelectableType() == ESelectableType::Building 
		&& Selectable->GetAttributesBase().GetBuildingType() != EBuildingType::ResourceSpot
		&& Selectable->GetBuildingAttributes()->CanProduce()
		&& Selectable->GetBuildingAttributes()->GetAffiliation() == EAffiliation::Owned;
}

bool USelectableInfo::ShouldShowAttackAttributes(const TScriptInterface<ISelectable>& Selectable) const
{
	return Selectable->HasAttack();
}

FText USelectableInfo::DamageFloatToText(float DamageValue)
{
	FloatToText(DamageValue, UIOptions::DamageDisplayMethod);
}

FText USelectableInfo::AttackRateFloatToText(float AttackRate)
{
	FloatToText(AttackRate, UIOptions::AttackRateDisplayMethod);
}

FText USelectableInfo::AttackRangeFloatToText(float AttackRange)
{
	FloatToText(AttackRange, UIOptions::AttackRangeDisplayMethod);
}

FText USelectableInfo::CurrentHealthFloatToText(float HealthValue)
{
	FloatToText(HealthValue, UIOptions::CurrentHealthDisplayMethod);
}

FText USelectableInfo::MaxHealthFloatToText(float MaxHealthValue)
{
	FloatToText(MaxHealthValue, UIOptions::MaxHealthDisplayMethod);
}

FText USelectableInfo::SelectableResourceCurrentAmountFloatToText(float Amount)
{
	FloatToText(Amount, UIOptions::SelectableResourceAmountDisplayMethod);
}

FText USelectableInfo::SelectableResourceRegenRateFloatToText(float RegenRateFloat)
{
	FloatToText(RegenRateFloat, UIOptions::SelectableResourceRegenRateDisplayMethod);
}

FText USelectableInfo::DefenseMultiplierFloatToText(float DefenseMultiplier)
{	
	FText Text = DefenseMultiplierFloatToTextInner(DefenseMultiplier);

	/* Tag on the "%" symbol if required */
	if (UIOptions::DefenseMultiplierSpecificDisplayMethod == EMultiplierDisplayMethod::Percentage_WithPercentageSymbol
		|| UIOptions::DefenseMultiplierSpecificDisplayMethod == EMultiplierDisplayMethod::Percentage_WithPercentageSymbol)
	{
		FString AsString = Text.ToString();
		AsString += "%";

		return FText::FromString(AsString);
	}
	
	return Text;
}

FText USelectableInfo::DefenseMultiplierFloatToTextInner(float DefenseMultiplier)
{
	if (UIOptions::DefenseMultiplierSpecificDisplayMethod == EMultiplierDisplayMethod::Percentage_WithPercentageSymbol
		|| UIOptions::DefenseMultiplierSpecificDisplayMethod == EMultiplierDisplayMethod::Percentage_NoPercentageSymbol)
	{
		FloatToText(DefenseMultiplier * 100.f, UIOptions::DefenseMultiplierDisplayMethod);
	}
	else // Assuming InversePercentage_WithPercentageSymbol or InversePercentage_NoPercentageSymbol
	{
		assert(UIOptions::DefenseMultiplierSpecificDisplayMethod == EMultiplierDisplayMethod::InversePercentage_WithPercentageSymbol
			|| UIOptions::DefenseMultiplierSpecificDisplayMethod == EMultiplierDisplayMethod::InversePercentage_NoPercentageSymbol);

		FloatToText((1.f - DefenseMultiplier) * 100.f, UIOptions::DefenseMultiplierDisplayMethod);
	}
}

FText USelectableInfo::ExperienceGainedFloatToText(float ExperienceAmount)
{
	FloatToText(ExperienceAmount, UIOptions::ExperienceGainedDisplayMethod);
}

FText USelectableInfo::ExperienceRequiredFloatToText(float ExperienceAmount)
{
	FloatToText(ExperienceAmount, UIOptions::ExperienceRequiredDisplayMethod);
}

void USelectableInfo::OnPlayerSelectionChanged(const TScriptInterface < ISelectable > NewCurrentSelected, const FSelectableAttributesBase & AttributesBase)
{
	CurrentSelected = NewCurrentSelected;
	ProductionQueue = NewCurrentSelected->GetProductionQueue();

	// Do we need to assign FI here?

	/* Update all the info shown. First change visibility of widgets based on new current
	selected's affiliation */

	SetInfoVisibilities(NewCurrentSelected, AttributesBase);

	UpdateTextName(AttributesBase);
	UpdateTextDescription(AttributesBase);
	UpdateImageIcon(AttributesBase);

#if EXPERIENCE_ENABLED_GAME
	if (NewCurrentSelected->CanClassGainExperience())
	{
		const uint8 CurrentRank = NewCurrentSelected->GetRank();
		const float ExperienceTowardsNextRank = NewCurrentSelected->GetCurrentRankExperience();
		const float ExperienceRequired = NewCurrentSelected->GetTotalExperienceRequiredForNextLevel();
		const FRankInfo & LevelsInfo = FI->GetLevelUpInfo(CurrentRank);

		UpdateTextLevel(CurrentRank);
		UpdateImageLevel(LevelsInfo);
		UpdateTextExperienceRequired(CurrentRank, ExperienceRequired);
		UpdateTextExperienceTowardsNextLevel(ExperienceTowardsNextRank, CurrentRank);
		UpdateProgressBarExperience(ExperienceTowardsNextRank, CurrentRank, ExperienceRequired);
	}
#endif

	const FSelectableAttributesBasic * AttriBasic = NewCurrentSelected->GetAttributes();

	/* Everything from here on out is stuff that not all selectables have.
	Note I've probably got some things not 100% correct in SetInfoVisibilities like showing 
	health based on whether selectable is neutral or not. 
	So all the stuff set from here on should have been hidden if we don't have basic attributes 
	or something */
	if (AttriBasic != nullptr) 
	{
		const float Health = NewCurrentSelected->GetHealth();
		const float MaxHealth = AttriBasic->GetMaxHealth();
		UpdateTextHealth(Health);
		UpdateTextMaxHealth(MaxHealth);
		UpdateProgressBarHealth(Health, MaxHealth);

		/* Check here if we're actually showing the selectable resource widgets */
		if (LastSelectableResourceType != ESelectableResourceType::None)
		{
			const float SelectableResourceCurrentAmount = AttriBasic->GetSelectableResource_1().GetAmountAsFloatForDisplay();
			const int32 SelectableResourceMaxAmount = AttriBasic->GetSelectableResource_1().GetMaxAmount();
			UpdateTextSelectableResource(SelectableResourceCurrentAmount);
			UpdateTextSelectableResourceMax(SelectableResourceMaxAmount);
			UpdateProgressBarSelectableResource(SelectableResourceCurrentAmount, SelectableResourceMaxAmount);
			UpdateTextSelectableResourceRegenRate(AttriBasic->GetSelectableResource_1().GetRegenRatePerSecondForDisplay());
		}

		UpdateTextDefenseMultiplier(AttriBasic->GetDefenseMultiplier());

		if (ShouldShowAttackAttributes(NewCurrentSelected))
		{
			const FAttackAttributes * AttackAttributes = NewCurrentSelected->GetAttackAttributes();

			UpdateTextImpactDamage(AttackAttributes->GetImpactDamage());
			UpdateTextAttackRate(AttackAttributes->GetAttackRate());
			UpdateTextAttackRange(AttackAttributes->GetAttackRange());
		}

		UpdateProgressBarProductionQueue(NewCurrentSelected);

		UpdateProductionQueueContents();

		UpdateBuffsAndDebuffs(NewCurrentSelected);
	}
}

void USelectableInfo::OnNoPlayerSelection()
{
	CurrentSelected = nullptr;
	ProductionQueue = nullptr;
}

void USelectableInfo::Selected_OnHealthChanged(float InNewAmount, float MaxHealth)
{
	UpdateTextHealth(InNewAmount);
	UpdateProgressBarHealth(InNewAmount, MaxHealth);
}

void USelectableInfo::Selected_OnMaxHealthChanged(float NewMaxHealth, float CurrentHealth)
{
	UpdateTextMaxHealth(NewMaxHealth);
	UpdateProgressBarHealth(CurrentHealth, NewMaxHealth);
}

void USelectableInfo::Selected_OnCurrentAndMaxHealthChanged(float NewMaxHealth, float CurrentHealth)
{
	UpdateTextHealth(CurrentHealth);
	UpdateTextMaxHealth(NewMaxHealth);
	UpdateProgressBarHealth(CurrentHealth, NewMaxHealth);
}

void USelectableInfo::Selected_OnSelectableResourceCurrentAmountChanged(float CurrentAmount, int32 MaxAmount)
{
	UpdateTextSelectableResource(CurrentAmount);
	UpdateProgressBarSelectableResource(CurrentAmount, MaxAmount);
}

void USelectableInfo::Selected_OnSelectableResourceMaxAmountChanged(float CurrentAmount, int32 MaxAmount)
{
	UpdateTextSelectableResourceMax(MaxAmount);
	UpdateProgressBarSelectableResource(CurrentAmount, MaxAmount);
}

void USelectableInfo::Selected_OnSelectableResourceCurrentAndMaxAmountsChanged(float CurrentAmount, int32 MaxAmount)
{
	UpdateTextSelectableResource(CurrentAmount);
	UpdateTextSelectableResourceMax(MaxAmount);
	UpdateProgressBarSelectableResource(CurrentAmount, MaxAmount);
}

void USelectableInfo::Selected_OnSelectableResourceRegenRateChanged(float PerSecondRegenRate)
{
	UpdateTextSelectableResourceRegenRate(PerSecondRegenRate);
}

void USelectableInfo::Selected_OnRankChanged(float ExperienceTowardsNextRank, uint8 InNewRank, float ExperienceRequired)
{
	const FRankInfo & LevelsInfo = FI->GetLevelUpInfo(InNewRank);
	
	UpdateTextLevel(InNewRank);
	UpdateImageLevel(LevelsInfo);
	UpdateTextExperienceRequired(InNewRank, ExperienceRequired);
	UpdateTextExperienceTowardsNextLevel(ExperienceTowardsNextRank, InNewRank);
	UpdateProgressBarExperience(ExperienceTowardsNextRank, InNewRank, ExperienceRequired);
}

void USelectableInfo::Selected_OnCurrentRankExperienceChanged(float InNewAmount, uint8 Rank, float ExperienceRequired)
{
	UpdateTextExperienceTowardsNextLevel(InNewAmount, Rank);
	UpdateProgressBarExperience(InNewAmount, Rank, ExperienceRequired);
}

void USelectableInfo::Selected_OnImpactDamageChanged(float Damage)
{
	UpdateTextImpactDamage(Damage);
}

void USelectableInfo::Selected_OnAttackRateChanged(float AttackRate)
{
	UpdateTextAttackRate(AttackRate);
}

void USelectableInfo::Selected_OnAttackRangeChanged(float AttackRange)
{
	UpdateTextAttackRange(AttackRange);
}

void USelectableInfo::Selected_OnDefenseMultiplierChanged(float NewDefenseMultiplier)
{
	UpdateTextDefenseMultiplier(NewDefenseMultiplier);
}

void USelectableInfo::Selected_OnBuffApplied(const FStaticBuffOrDebuffInstanceInfo & BuffInfo)
{
	if (IsWidgetBound(Panel_Buffs))
	{
		USingleBuffOrDebuffWidget * BuffWidget = GetStaticBuffWidget();
		BuffWidget->SetupFor(BuffInfo, GI);

		StaticBuffWidgets.Emplace(BuffWidget);

		/* Emulate the behavior of Panel_Buffs->InsertChildAt(StaticBuffWidgets.Num(), BuffWidget) */
		Panel_Buffs->ClearChildren();
		for (int32 i = 0; i < StaticBuffWidgets.Num(); ++i)
		{
			Panel_Buffs->AddChild(StaticBuffWidgets[i]);
		}
#if TICKABLE_BUFF_AND_DEBUFF_ENABLED_GAME
		for (int32 i = 0; i < TickableBuffWidgets.Num(); ++i)
		{
			Panel_Buffs->AddChild(TickableBuffWidgets[i]);
		}
#endif // TICKABLE_BUFF_AND_DEBUFF_ENABLED_GAME
	}
}

void USelectableInfo::Selected_OnDebuffApplied(const FStaticBuffOrDebuffInstanceInfo & DebuffInfo)
{
	if (IsWidgetBound(Panel_Debuffs))
	{
		USingleBuffOrDebuffWidget * DebuffWidget = GetStaticDebuffWidget();
		DebuffWidget->SetupFor(DebuffInfo, GI);

		StaticDebuffWidgets.Emplace(DebuffWidget);

		/* Well from my testing this doesn't do what I expect which is put this widget before 
		any tickable widgets. I thought that's how InsertChildAt works. Does the ordering of the 
		slots array not matter? The implication is that debuffs just appear in the order they 
		are applied, but after deselecting then reselecting the selectable the debuffs will be 
		in the correct order which is statics followed by tickables 
		
		Found the answer: 
		"InsertChildAt had to go away. It wasn't intended to be used by user code, it's something 
		the designer uses to rearrange elements because it is about to completely rebuild the UI. 
		The function wouldn't work in game code because it can't insert at a specific index after 
		construction and the widget is live. So you may as well just use AddChild, or 
		AddChild....container specific version, since you clearly know the order to add things. 
		Rather than keep a seemingly buggy piece of API callable by the user, we made it editor 
		only" 
		
		Well that answers that. Because when debuffs are removed order matters we need to 
		emulate its behavior.

		/* This code here emulates the behavior of Panel_Debuffs->InsertChildAt(StaticDebuffWidgets.Num(), DebuffWidget). 
		Maybe able to do this faster by shuffling the children round, don't know if that's 
		even possible, so just doing it like this for now. 
		Also can't help but feel ClearChildren should iterate backwards instead of forwards. There 
		must be a reason for this. Actually I just realized it removes the widget at index 0 
		every loop iteration */
		Panel_Debuffs->ClearChildren();
		for (int32 i = 0; i < StaticDebuffWidgets.Num(); ++i)
		{
			Panel_Debuffs->AddChild(StaticDebuffWidgets[i]);
		}
#if TICKABLE_BUFF_AND_DEBUFF_ENABLED_GAME
		for (int32 i = 0; i < TickableDebuffWidgets.Num(); ++i)
		{
			Panel_Debuffs->AddChild(TickableDebuffWidgets[i]);
		}
#endif // TICKABLE_BUFF_AND_DEBUFF_ENABLED_GAME
	}
}

void USelectableInfo::Selected_OnBuffApplied(const FTickableBuffOrDebuffInstanceInfo & BuffInfo)
{
	if (IsWidgetBound(Panel_Buffs))
	{
		/* Could choose to pool these instead but for now just always creating */
		USingleTickableBuffOrDebuffWidget * BuffWidget = GetTickableBuffWidget(); 
		BuffWidget->SetupFor(BuffInfo, GI);

		TickableBuffWidgets.Emplace(BuffWidget);

		Panel_Buffs->AddChild(BuffWidget);
	}
}

void USelectableInfo::Selected_OnDebuffApplied(const FTickableBuffOrDebuffInstanceInfo & DebuffInfo)
{
	if (IsWidgetBound(Panel_Debuffs))
	{
		USingleTickableBuffOrDebuffWidget * DebuffWidget = GetTickableDebuffWidget();
		DebuffWidget->SetupFor(DebuffInfo, GI);

		TickableDebuffWidgets.Emplace(DebuffWidget);

		Panel_Debuffs->AddChild(DebuffWidget);
	}
}

void USelectableInfo::Selected_OnTickableBuffRemoved(int32 ArrayIndex, EBuffAndDebuffRemovalReason RemovalReason)
{
	if (IsWidgetBound(Panel_Buffs))
	{
		const int32 ChildIndex = StaticBuffWidgets.Num() + ArrayIndex;

		TickableBuffWidgets.RemoveAt(ArrayIndex, 1, false);

		Panel_Buffs->RemoveChildAt(ChildIndex);
	}
}

void USelectableInfo::Selected_OnTickableDebuffRemoved(int32 ArrayIndex, EBuffAndDebuffRemovalReason RemovalReason)
{
	if (IsWidgetBound(Panel_Debuffs))
	{
		const int32 ChildIndex = StaticDebuffWidgets.Num() + ArrayIndex;

		TickableDebuffWidgets.RemoveAt(ArrayIndex, 1, false);

		Panel_Debuffs->RemoveChildAt(ChildIndex);
	}
}

void USelectableInfo::Selected_OnStaticBuffRemoved(int32 ArrayIndex, EBuffAndDebuffRemovalReason RemovalReason)
{
	if (IsWidgetBound(Panel_Buffs))
	{
		const int32 ChildIndex = ArrayIndex;

		StaticBuffWidgets.RemoveAt(ArrayIndex, 1, false);

		Panel_Buffs->RemoveChildAt(ChildIndex);
	}
}

void USelectableInfo::Selected_OnStaticDebuffRemoved(int32 ArrayIndex, EBuffAndDebuffRemovalReason RemovalReason)
{
	if (IsWidgetBound(Panel_Debuffs))
	{
		const int32 ChildIndex = ArrayIndex;

		StaticDebuffWidgets.RemoveAt(ArrayIndex, 1, false);

		Panel_Debuffs->RemoveChildAt(ChildIndex);
	}
}

void USelectableInfo::Selected_UpdateTickableBuffDuration(int32 ArrayIndex, float DurationRemaining)
{
	if (IsWidgetBound(Panel_Buffs))
	{
		USingleTickableBuffOrDebuffWidget * BuffWidget = TickableBuffWidgets[ArrayIndex];

		BuffWidget->SetDurationRemaining(DurationRemaining);
	}
}

void USelectableInfo::Selected_UpdateTickableDebuffDuration(int32 ArrayIndex, float DurationRemaining)
{
	if (IsWidgetBound(Panel_Debuffs))
	{
		USingleTickableBuffOrDebuffWidget * DebuffWidget = TickableDebuffWidgets[ArrayIndex];

		DebuffWidget->SetDurationRemaining(DurationRemaining);
	}
}

void USelectableInfo::Selected_OnItemAddedToInventory(int32 SlotIndex, const FInventorySlotState & UpdatedSlot)
{
	if (IsWidgetBound(Panel_Inventory))
	{
		/* The correct index should have already been passed into this func so no need to 
		look it up or anything */
		InventoryButtons[SlotIndex]->OnItemAdded(UpdatedSlot);
	}
}

void USelectableInfo::Selected_OnItemsRemovedFromInventory(const FInventory * Inventory, const TMap<uint8, FInventoryItemQuantityPair>& RemovedItems)
{
	if (IsWidgetBound(Panel_Inventory))
	{
		FTimerManager & TimerManager = GI->GetTimerManager();

		for (const auto & Elem : RemovedItems)
		{
			UInventoryItemButton * InvSlotWidget = InventoryButtons[Inventory->GetLocalIndexFromServerIndex(Elem.Key)].Get();
			
			/* Check if the stack is now empty because of the remove */
			if (InvSlotWidget->GetInventorySlot()->HasItem() == false)
			{
				/* Check if the item has a use ability cooling down. If so then it is definently/likely 
				in cooling down items array and we need to remove it. Removing items does nothing 
				to the timer handle so we can still check it, though it might be good to stop that 
				timer handle for performance, but it's like 1 TH who cares */
				const bool bItemUseWasCoolingDown = InvSlotWidget->GetInventorySlot()->IsOnCooldown(TimerManager);
				if (bItemUseWasCoolingDown)
				{
					const int32 NumRemoved = CoolingDownInventorySlots.RemoveSingleSwap(InvSlotWidget);
					assert(NumRemoved == 1);
				}
			}

			InvSlotWidget->OnAmountRemoved();
		}
	}
}

void USelectableInfo::Selected_OnInventoryPositionsSwapped(uint8 DisplayIndex1, uint8 DisplayIndex2)
{
	if (IsWidgetBound(Panel_Inventory))
	{
		FTimerManager & WorldTimerManager = GI->GetTimerManager();
		
		UInventoryItemButton * Widget_1 = InventoryButtons[DisplayIndex1].Get();
		UInventoryItemButton * Widget_2 = InventoryButtons[DisplayIndex2].Get();

		// Swap. 
		// These 2 ServerIndex_ can probably be made params of this func instead for performance. No biggie though
		const uint8 ServerIndex_1 = Widget_1->GetServerSlotIndex();
		const uint8 ServerIndex_2 = Widget_2->GetServerSlotIndex();
		const FInventorySlotState * TempSlot = Widget_1->GetInventorySlot();
		const FInventoryItemInfo * TempItemInfo = Widget_1->GetItemInfo();
		Widget_1->MakeActive(*Widget_2->GetInventorySlot(), ServerIndex_2, WorldTimerManager, Widget_2->GetItemInfo());
		Widget_2->MakeActive(*TempSlot, ServerIndex_1, WorldTimerManager, TempItemInfo);
	}
}

void USelectableInfo::Selected_OnInventoryItemUsed(uint8 DisplayIndex, float TotalUseCooldown)
{
	if (IsWidgetBound(Panel_Inventory))
	{	
		UInventoryItemButton * InvSlotWidget = InventoryButtons[DisplayIndex].Get();
		
		InvSlotWidget->OnUsed(GI->GetTimerManager());

		/* Check if use ability has a cooldown. This func requires passing in TotalUseCooldown == 0.f 
		as a way of saying 'has no cooldown' */
		if (TotalUseCooldown > 0.f)
		{
			/* Register this widget as one that is cooling down so we know to update the progress 
			bar each tick */
			assert(CoolingDownInventorySlots.Contains(InvSlotWidget) == false);
			CoolingDownInventorySlots.Emplace(InvSlotWidget);
		}
	}
}

void USelectableInfo::Selected_OnInventoryItemUseCooldownFinished(uint8 InventorySlotDisplayIndex)
{
	/* You know an alternative way of handling cooldowns is to never call this function and
	instead each tick check using the timer handle whether the cooldown has finished, and 
	call UInventorySlotButton::OnUseCooldownFinished then instead. This makes tick performance 
	slightly worse but feels like the better way to do it */
	
	UInventoryItemButton * InvSlotWidget = InventoryButtons[InventorySlotDisplayIndex].Get();
	
	const int32 NumRemoved = CoolingDownInventorySlots.RemoveSingleSwap(InvSlotWidget);
	assert(NumRemoved == 1);

	InvSlotWidget->OnUseCooldownFinished();
}

void USelectableInfo::Selected_OnItemPurchasedFromShop(uint8 ShopSlotIndex)
{
	if (IsWidgetBound(Panel_ItemsOnDisplay))
	{
		ItemOnDisplayInShopButtons[ShopSlotIndex]->OnPurchaseFromHere();
	}
}

void USelectableInfo::Selected_OnInventoryItemSold(uint8 InvDisplayIndex)
{
	if (IsWidgetBound(Panel_Inventory))
	{
		UInventoryItemButton * InvSlotWidget = InventoryButtons[InvDisplayIndex].Get();
		
		/* Check if the stack is now empty because of the remove */
		if (InvSlotWidget->GetInventorySlot()->HasItem() == false)
		{
			// Check if item has use ability that is cooling down
			const bool bItemUseWasCoolingDown = InvSlotWidget->GetInventorySlot()->IsOnCooldown(GI->GetTimerManager());
			if (bItemUseWasCoolingDown)
			{
				const int32 NumRemoved = CoolingDownInventorySlots.RemoveSingleSwap(InvSlotWidget);
				assert(NumRemoved == 1);
			}
		}

		InvSlotWidget->OnAmountRemoved();
	}
}

void USelectableInfo::OnItemAddedToProductionQueue(const FTrainingInfo & Item, const FProductionQueue & Queue, AActor * Producer)
{
	// TODO call ShouldShowProductionQueueInfo and set visibility based on return value

	if (Statics::IsValid(CurrentSelected))
	{
		/* Assumed if calling this then already something in queue, if not then would have called
		OnItemAddedAndProductionStarted */
		assert(QueueNum >= 1);

		/* If null then the current selected is producing something but its GetProductionQueue func is
		returning null when it should be returning the queue */
		assert(ProductionQueue != nullptr);

		/* Add an entry to the production queue panel. 
		A crash here could mean not enough production queue buttons have been added to your 
		HUD widget to support the capacity of the queue. You should add enough buttons equal to 
		the max queue size out of all buildings */
		UProductionQueueButton * Button = ProductionQueueSlots[ProductionQueue->Num() - 1];
		Button->Assign(Item, GI->GetUnifiedButtonAssetFlags_ActionBar());

		QueueNum++;
	}
	else
	{
		// Perhaps erase queue
	}
}

void USelectableInfo::OnItemAddedAndProductionStarted(const FTrainingInfo & Item, const FProductionQueue & Queue, AActor * Producer)
{
	// TODO call ShouldShowProductionQueueInfo and set visibility based on return value

	if (Queue.GetType() == EProductionQueueType::Persistent)
	{
		assert(Item.IsProductionForBuilding());
		// As long as persistent queue capacities are 1 this is true
		assert(FI->GetBuildingInfo(Item.GetBuildingType())->GetBuildingBuildMethod()
			== EBuildingBuildMethod::BuildsItself);

		/* Do nothing. Could show persistent queue's progress. You would need another progress
		bar for it though because player can decide to build units from context queue at the same
		time. Could just show the progress on the button. */
	}
	else
	{
		// Add an entry to the production queue panel

		assert(ProductionQueue != nullptr);

		if (Statics::IsValid(CurrentSelected))
		{
			/* Assuming item added was added to a previously empty queue */
			assert(ProductionQueue->Num() == 1);

			// Add an entry to the production queue panel
			UProductionQueueButton * Button = ProductionQueueSlots[ProductionQueue->Num() - 1];
			Button->Assign(Item, GI->GetUnifiedButtonAssetFlags_ActionBar());

			QueueNum++;
		}
	}
}

void USelectableInfo::OnProductionComplete(const FTrainingInfo & Item, const FProductionQueue & Queue, 
	uint8 NumRemoved, AActor * Producer)
{
	// TODO call ShouldShowProductionQueueInfo and set visibility based on return value

	/* If a persistent queue then we don't show its progress in the context menu so just return.
	If you decide to change this behavior make sure OnItemAddedAndProductionStarted() does
	something too when its a persistent queue */
	if (Queue.GetType() == EProductionQueueType::Persistent)
	{
		return;
	}

	const FUnifiedImageAndSoundFlags UnifiedAssetFlags = GI->GetUnifiedButtonAssetFlags_ActionBar();

	/* Just assign items to buttons */
	for (int32 i = 0; i < Queue.Num(); ++i)
	{
		UProductionQueueButton * Button = ProductionQueueSlots[i];
		Button->Assign(Queue[i], UnifiedAssetFlags);
	}

	/* Deactivate the remaining buttons that were previously activated */
	for (uint8 i = Queue.Num(); i < QueueNum; ++i)
	{
		UProductionQueueButton * Button = ProductionQueueSlots[i];
		Button->Unassign();
	}

	QueueNum -= NumRemoved;
	assert(QueueNum >= 0);
	assert(QueueNum == Queue.Num());

	/* Perhaps hide queue */
	if (ShouldShowProductionQueueInfo(CurrentSelected))
	{
		/* Crash here probably means Panel_ProductionQueueInfo is not bound */
		if (Panel_ProductionQueueInfo->GetVisibility() != ESlateVisibility::Visible)
		{
			Panel_ProductionQueueInfo->SetVisibility(ESlateVisibility::Visible);
		}
	}
	else
	{
		if (Panel_ProductionQueueInfo->GetVisibility() != ESlateVisibility::Collapsed)
		{
			Panel_ProductionQueueInfo->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}


//==========================================================================================
//	 Production Queue Button
//==========================================================================================

UProductionQueueButton::UProductionQueueButton(const FObjectInitializer & ObjectInitializer)
	: Super(ObjectInitializer)
{
	//IsFocusable = false; // Old UButton variable
	Purpose = EUIElementType::ProductionQueueSlot;
}

void UProductionQueueButton::Setup(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController, 
	AFactionInfo * InFactionInfo)
{
	assert(InPlayerController != nullptr && InFactionInfo != nullptr);
	PC = InPlayerController;
	FI = InFactionInfo;

	SetPC(InPlayerController);
	SetOwningWidget();

	/* Setup on clicked functionality */
	GetOnLeftMouseButtonPressDelegate().BindUObject(this, &UProductionQueueButton::UIBinding_OnLeftMouseButtonPress);
	GetOnLeftMouseButtonReleasedDelegate().BindUObject(this, &UProductionQueueButton::UIBinding_OnLeftMouseButtonReleased);
	GetOnRightMouseButtonPressDelegate().BindUObject(this, &UProductionQueueButton::UIBinding_OnRightMouseButtonPress);
	GetOnRightMouseButtonReleasedDelegate().BindUObject(this, &UProductionQueueButton::UIBinding_OnRightMouseButtonReleased);

	const FUnifiedImageAndSoundFlags UnifiedAssetFlags = InGameInstance->GetUnifiedButtonAssetFlags_ActionBar();
	SetUnifiedImages_ExcludeNormalAndHighlightedImage(UnifiedAssetFlags,
		&InGameInstance->GetUnifiedHoverBrush_ActionBar().GetBrush(),
		&InGameInstance->GetUnifiedPressedBrush_ActionBar().GetBrush());
	if (UnifiedAssetFlags.bUsingUnifiedHoverSound)
	{
		SetHoveredSound(InGameInstance->GetUnifiedHoverSound_ActionBar().GetSound());
	}
	if (UnifiedAssetFlags.bUsingUnifiedLMBPressSound)
	{
		SetPressedByLMBSound(InGameInstance->GetUnifiedLMBPressedSound_ActionBar().GetSound());
	}
	if (UnifiedAssetFlags.bUsingUnifiedRMBPressSound)
	{
		SetPressedByRMBSound(InGameInstance->GetUnifiedRMBPressedSound_ActionBar().GetSound());
	}
}

/* These 4 mouse funcs... I had never implemented any behavior for cancelling items in 
production queue so they will be close to NOOPs */
void UProductionQueueButton::UIBinding_OnLeftMouseButtonPress()
{
	PC->OnLMBPressed_PrimarySelected_ProductionQueueSlot(this);
}

void UProductionQueueButton::UIBinding_OnLeftMouseButtonReleased()
{
	PC->OnLMBReleased_PrimarySelected_ProductionQueueSlot(this);
}

void UProductionQueueButton::UIBinding_OnRightMouseButtonPress()
{
	PC->OnRMBPressed_PrimarySelected_ProductionQueueSlot(this);
}

void UProductionQueueButton::UIBinding_OnRightMouseButtonReleased()
{
	PC->OnRMBReleased_PrimarySelected_ProductionQueueSlot(this);
}

const FContextButton * UProductionQueueButton::GetButtonType() const
{
	return &ButtonType;
}

void UProductionQueueButton::OnPlayerSelectionChanged(const TScriptInterface<ISelectable> NewCurrentSelected)
{
}

void UProductionQueueButton::Assign(const FTrainingInfo & Item, FUnifiedImageAndSoundFlags InUnifiedAssetFlags)
{	
	ButtonType = FContextButton(Item);

	/* Set image */
	const FDisplayInfoBase * Info = FI->GetDisplayInfo(ButtonType);
	SetImages_ExcludeHighlightedImage(InUnifiedAssetFlags,
		&Info->GetHUDImage(),
		&Info->GetHoveredHUDImage(),
		&Info->GetPressedHUDImage());
	if (InUnifiedAssetFlags.bUsingUnifiedHoverSound == false)
	{
		SetHoveredSound(Info->GetHoveredButtonSound());
	}
	if (InUnifiedAssetFlags.bUsingUnifiedLMBPressSound == false)
	{
		SetPressedByLMBSound(Info->GetPressedByLMBSound());
	}
	if (InUnifiedAssetFlags.bUsingUnifiedRMBPressSound == false)
	{
		SetPressedByRMBSound(Info->GetPressedByRMBSound());
	}

	SetVisibility(ESlateVisibility::Visible);
}

void UProductionQueueButton::Unassign()
{
	/* Just making invisible, not changing ButtonType or anything else */
	SetVisibility(ESlateVisibility::Collapsed);
}

#if WITH_EDITOR
void UProductionQueueButton::OnCreationFromPalette()
{
	Super::OnCreationFromPalette();	
}

const FText UProductionQueueButton::GetPaletteCategory()
{
	return URTSHUD::PALETTE_CATEGORY;
}
#endif


//===============================================================================================
//	Button That Goes In A Shop
//===============================================================================================

UItemOnDisplayInShopButton::UItemOnDisplayInShopButton(const FObjectInitializer & ObjectInitializer)
	: Super(ObjectInitializer)
{
}

bool UItemOnDisplayInShopButton::SetupWidget(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController)
{
	if (Super::SetupWidget(InGameInstance, InPlayerController))
	{
		return true;
	}

	OriginalOpacity = GetRenderOpacity();

	if (IsWidgetBound(Button_Button))
	{
		Button_Button->SetPurpose(EUIElementType::ShopSlot, this);
		Button_Button->SetPC(InPlayerController);
		Button_Button->SetOwningWidget();
		Button_Button->GetOnLeftMouseButtonPressDelegate().BindUObject(this, &UItemOnDisplayInShopButton::UIBinding_OnLeftMouseButtonPress);
		Button_Button->GetOnLeftMouseButtonReleasedDelegate().BindUObject(this, &UItemOnDisplayInShopButton::UIBinding_OnLeftMouseButtonReleased);
		Button_Button->GetOnRightMouseButtonPressDelegate().BindUObject(this, &UItemOnDisplayInShopButton::UIBinding_OnRightMouseButtonPress);
		Button_Button->GetOnRightMouseButtonReleasedDelegate().BindUObject(this, &UItemOnDisplayInShopButton::UIBinding_OnRightMouseButtonReleased);

		Button_Button->SetUnifiedImages_ExcludeNormalAndHighlightedImage(InGameInstance->GetUnifiedButtonAssetFlags_Shop(),
			&InGameInstance->GetUnifiedHoverBrush_Shop().GetBrush(),
			&InGameInstance->GetUnifiedPressedBrush_Shop().GetBrush());
		if (InGameInstance->GetUnifiedButtonAssetFlags_Shop().bUsingUnifiedHoverSound)
		{
			Button_Button->SetHoveredSound(InGameInstance->GetUnifiedHoverSound_Shop().GetSound());
		}
		if (InGameInstance->GetUnifiedButtonAssetFlags_Shop().bUsingUnifiedLMBPressSound)
		{
			Button_Button->SetPressedByLMBSound(InGameInstance->GetUnifiedLMBPressedSound_Shop().GetSound());
		}
		if (InGameInstance->GetUnifiedButtonAssetFlags_Shop().bUsingUnifiedRMBPressSound)
		{
			Button_Button->SetPressedByRMBSound(InGameInstance->GetUnifiedRMBPressedSound_Shop().GetSound());
		}
	}

	return false;
}

void UItemOnDisplayInShopButton::SetupMore(USelectableInfo * InOwningWidget)
{
	OwningWidget = InOwningWidget;
}

FText UItemOnDisplayInShopButton::GetPurchasesRemainingText() const
{
	return SlotStateInfo->HasUnlimitedPurchases() ? FText::GetEmpty() : FText::AsNumber(SlotStateInfo->GetPurchasesRemaining());
}

float UItemOnDisplayInShopButton::GetSoldOutRenderOpacity() const
{
	return OriginalOpacity * HUDOptions::SHOP_SLOT_OUT_OF_PURCHASES_OPACITY_MULTIPLIER;
}

void UItemOnDisplayInShopButton::UIBinding_OnLeftMouseButtonPress()
{
	PC->OnLMBPressed_PrimarySelected_ShopButton(Button_Button);
}

void UItemOnDisplayInShopButton::UIBinding_OnLeftMouseButtonReleased()
{
	PC->OnLMBReleased_PrimarySelected_ShopButton(Button_Button, this);
}

void UItemOnDisplayInShopButton::UIBinding_OnRightMouseButtonPress()
{
	PC->OnRMBPressed_PrimarySelected_ShopButton(Button_Button);
}

void UItemOnDisplayInShopButton::UIBinding_OnRightMouseButtonReleased()
{
	PC->OnRMBReleased_PrimarySelected_ShopButton(Button_Button, this);
}

uint8 UItemOnDisplayInShopButton::GetShopSlotIndex() const
{
	return ShopSlotIndex;
}

const FItemOnDisplayInShopSlot * UItemOnDisplayInShopButton::GetSlotStateInfo() const
{
	return SlotStateInfo;
}

const FInventoryItemInfo * UItemOnDisplayInShopButton::GetItemInfo() const
{
	return ItemInfo;
}

void UItemOnDisplayInShopButton::MakeActive(uint8 InShopSlotIndex, const FItemOnDisplayInShopSlot & SlotInfo, 
	FUnifiedImageAndSoundFlags UnifiedAssetFlags)
{
	ShopSlotIndex = InShopSlotIndex;
	SlotStateInfo = &SlotInfo;
	ItemInfo = GI->GetInventoryItemInfo(SlotInfo.GetItemType());

	/* Set render opacity  */
	if (SlotInfo.GetPurchasesRemaining() != 0)
	{
		if (GetRenderOpacity() != OriginalOpacity)
		{
			SetRenderOpacity(OriginalOpacity);
		}
	}
	else // Item has sold out
	{
		if (GetRenderOpacity() != GetSoldOutRenderOpacity())
		{
			SetRenderOpacity(GetSoldOutRenderOpacity());
		}
	}

	/* Set image etc */
	if (IsWidgetBound(Button_Button))
	{
		Button_Button->SetImages_ExcludeHighlightedImage(UnifiedAssetFlags,
			&ItemInfo->GetDisplayImage(),
			&ItemInfo->GetDisplayImage_Hovered(),
			&ItemInfo->GetDisplayImage_Pressed());
		if (UnifiedAssetFlags.bUsingUnifiedHoverSound == false)
		{
			Button_Button->SetHoveredSound(ItemInfo->GetUISound_Hovered());
		}
		if (UnifiedAssetFlags.bUsingUnifiedLMBPressSound == false)
		{
			Button_Button->SetPressedByLMBSound(ItemInfo->GetUISound_PressedByLMB());
		}
		if (UnifiedAssetFlags.bUsingUnifiedRMBPressSound == false)
		{
			Button_Button->SetPressedByRMBSound(ItemInfo->GetUISound_PressedByRMB());
		}
	}
	if (IsWidgetBound(Text_Name))
	{
		Text_Name->SetText(ItemInfo->GetDisplayName());
	}
	if (IsWidgetBound(Text_PurchasesRemaining))
	{
		Text_PurchasesRemaining->SetText(GetPurchasesRemainingText());
	}
}

void UItemOnDisplayInShopButton::OnPurchaseFromHere()
{
	if (SlotStateInfo->HasUnlimitedPurchases() == false)
	{
		if (IsWidgetBound(Text_PurchasesRemaining))
		{
			Text_PurchasesRemaining->SetText(GetPurchasesRemainingText());
		}

		if (SlotStateInfo->GetPurchasesRemaining() == 0)
		{
			/* That was the last item that could be bought from this slot. Grey out this widget */
			SetRenderOpacity(GetSoldOutRenderOpacity());
		}
	}
}


//===============================================================================================
//	Inventory Slot
//===============================================================================================

UInventoryItemButton::UInventoryItemButton(const FObjectInitializer & ObjectInitializer)
	: Super(ObjectInitializer)
{
	StackQuantityOrNumChargesShown = -2;
}

bool UInventoryItemButton::SetupWidget(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController)
{
	if (Super::SetupWidget(InGameInstance, InPlayerController))
	{
		return true;
	}

	if (IsWidgetBound(Button_Button))
	{	
		Button_Button->SetPurpose(EUIElementType::InventorySlot, this);
		
		Button_Button->OnLeftMouseButtonPressed().BindUObject(this, &UInventoryItemButton::UIBinding_OnButtonLeftMouseButtonPressed);
		Button_Button->OnLeftMouseButtonReleased().BindUObject(this, &UInventoryItemButton::UIBinding_OnButtonLeftMouseButtonReleased);
		Button_Button->OnRightMouseButtonPressed().BindUObject(this, &UInventoryItemButton::UIBinding_OnButtonRightMouseButtonPressed);
		Button_Button->OnRightMouseButtonReleased().BindUObject(this, &UInventoryItemButton::UIBinding_OnButtonRightMouseButtonReleased);
		//Button_Button->OnClicked.AddDynamic(this, &UInventoryItemButton::UIBinding_OnButtonClicked); // Old code

		UnifiedBrushAndSoundFlags = InGameInstance->GetUnifiedButtonAssetFlags_InventoryItems();

		/* Set unified brushs/sounds if required */
		Button_Button->SetImagesAndUnifiedImageFlags_ExcludeNormalAndHighlightedImage(UnifiedBrushAndSoundFlags,
			&InGameInstance->GetUnifiedHoverBrush_Inventory().GetBrush(),
			&InGameInstance->GetUnifiedPressedBrush_Inventory().GetBrush());
		if (InGameInstance->GetUnifiedHoverSound_Inventory().UsingSound())
		{
			Button_Button->SetHoveredSound(InGameInstance->GetUnifiedHoverSound_Inventory().GetSound());
		}
		if (InGameInstance->GetUnifiedLMBPressedSound_Inventory().UsingSound())
		{
			Button_Button->SetPressedByLMBSound(InGameInstance->GetUnifiedLMBPressedSound_Inventory().GetSound());
		}
		if (InGameInstance->GetUnifiedRMBPressedSound_Inventory().UsingSound())
		{
			Button_Button->SetPressedByRMBSound(InGameInstance->GetUnifiedRMBPressedSound_Inventory().GetSound());
		}

		Button_Button->SetPC(InPlayerController);
		Button_Button->SetOwningWidget(); 
	}

	if (IsWidgetBound(Text_UseCooldown))
	{
		// Remove text in case it has been set to something in editor
		Text_UseCooldown->SetText(FText::GetEmpty());
	}
	if (IsWidgetBound(ProgressBar_UseCooldown))
	{
		// Set to 0 in case it has been set to something else in editor
		ProgressBar_UseCooldown->SetPercent(0.f);
	}

	return false;
}

void UInventoryItemButton::SetAppearanceForEmptySlot(FUnifiedImageAndSoundFlags UnifiedAssetFlags)
{
	// Possibly make button HitTestInvisible so no clicking on it? Need to obviously make it 
	// visible again when button becomes active. For now will leave it like it is meaning 
	// clicks can happen on empty slots

	if (IsWidgetBound(Button_Button))
	{
		Button_Button->SetImages_ExcludeHighlightedImage(UnifiedAssetFlags,
			&GI->GetEmptyInventorySlotImage_Normal(),
			&GI->GetEmptyInventorySlotImage_Hovered(),
			&GI->GetEmptyInventorySlotImage_Pressed());
		
		if (UnifiedAssetFlags.bUsingUnifiedHoverSound == false)
		{
			Button_Button->SetHoveredSound(GI->GetEmptyInventorySlotSound_Hovered());
		}
		if (UnifiedAssetFlags.bUsingUnifiedLMBPressSound == false)
		{
			Button_Button->SetPressedByLMBSound(GI->GetEmptyInventorySlotSound_PressedByLMB());
		}
		if (UnifiedAssetFlags.bUsingUnifiedRMBPressSound == false)
		{
			Button_Button->SetPressedByRMBSound(GI->GetEmptyInventorySlotSound_PressedByRMB());
		}
	}

	if (IsWidgetBound(Text_QuantityOrNumCharges))
	{
		if (StackQuantityOrNumChargesShown != -1)
		{
			Text_QuantityOrNumCharges->SetText(FText::GetEmpty());
		}
	}

	if (IsWidgetBound(Text_Name))
	{
		// Essentially checking if it is non-empty text
		if (StackQuantityOrNumChargesShown != -1)
		{
			Text_Name->SetText(FText::GetEmpty());
		}
	}

	StackQuantityOrNumChargesShown = -1;

	if (IsWidgetBound(Text_UseCooldown))
	{
		// Make it empty to effectively make it hidden
		Text_UseCooldown->SetText(FText::GetEmpty());
	}
	if (IsWidgetBound(ProgressBar_UseCooldown))
	{
		// Set 0 percent to effectively make progress bar hidden
		ProgressBar_UseCooldown->SetPercent(0.f);
	}
}

FText UInventoryItemButton::ItemUseCooldownFloatToText(float Cooldown)
{
	FloatToText(Cooldown, UIOptions::ItemUseCooldownDisplayMethod);
}

void UInventoryItemButton::UIBinding_OnButtonLeftMouseButtonPressed()
{
	PC->OnLMBPressed_PrimarySelected_InventoryButton(Button_Button);
}

void UInventoryItemButton::UIBinding_OnButtonLeftMouseButtonReleased()
{
	PC->OnLMBReleased_PrimarySelected_InventoryButton(Button_Button, this);
}

void UInventoryItemButton::UIBinding_OnButtonRightMouseButtonPressed()
{
	PC->OnRMBPressed_PrimarySelected_InventoryButton(Button_Button);
}

void UInventoryItemButton::UIBinding_OnButtonRightMouseButtonReleased()
{
	PC->OnRMBReleased_PrimarySelected_InventoryButton(Button_Button, this);
}

const FInventorySlotState * UInventoryItemButton::GetInventorySlot() const
{
	return InventorySlot;
}

const FInventoryItemInfo * UInventoryItemButton::GetItemInfo() const
{
	return SlotsItemInfo;
}

uint8 UInventoryItemButton::GetServerSlotIndex() const
{
	return ServerSlotIndex;
}

bool UInventoryItemButton::MakeActive(const FInventorySlotState & SlotInfo, uint8 InServerSlotIndex,
	FTimerManager & WorldTimerManager, const FInventoryItemInfo * ItemsInfo)
{	
	InventorySlot = &SlotInfo;
	SlotsItemInfo = ItemsInfo;
	ServerSlotIndex = InServerSlotIndex;

	if (SlotInfo.GetItemType() == EInventoryItem::None)
	{
		SetAppearanceForEmptySlot(UnifiedBrushAndSoundFlags);
		DisplayedItemType = SlotInfo.GetItemType();
		return false;
	}
	else
	{
		if (IsWidgetBound(Button_Button))
		{
			Button_Button->SetImages(UnifiedBrushAndSoundFlags,
				&ItemsInfo->GetDisplayImage(),
				&ItemsInfo->GetDisplayImage_Hovered(),
				&ItemsInfo->GetDisplayImage_Pressed(),
				&ItemsInfo->GetDisplayImage_Highlighted());
			if (UnifiedBrushAndSoundFlags.bUsingUnifiedHoverSound == false)
			{
				Button_Button->SetHoveredSound(ItemsInfo->GetUISound_Hovered());
			}
			if (UnifiedBrushAndSoundFlags.bUsingUnifiedLMBPressSound == false)
			{
				Button_Button->SetPressedByLMBSound(ItemsInfo->GetUISound_PressedByLMB());
			}
			if (UnifiedBrushAndSoundFlags.bUsingUnifiedRMBPressSound == false)
			{
				Button_Button->SetPressedByRMBSound(ItemsInfo->GetUISound_PressedByRMB());
			}
		}

		if (IsWidgetBound(Text_QuantityOrNumCharges))
		{
			/* Here we choose to show a number in this text block if either:
			- the item can stack
			- it has a finite number of charges

			If neither of those are the case then we make the text blank to pseudo hide it which
			might be cheaper than doing SetVisibility */

			if (SlotInfo.CanItemStack() == true)
			{
				if (StackQuantityOrNumChargesShown != SlotInfo.GetNumInStack())
				{
					StackQuantityOrNumChargesShown = SlotInfo.GetNumInStack();
					Text_QuantityOrNumCharges->SetText(FText::AsNumber(SlotInfo.GetNumInStack()));
				}
			}
			else if (SlotInfo.IsItemUsable() && SlotInfo.HasUnlimitedCharges() == false)
			{
				if (StackQuantityOrNumChargesShown != SlotInfo.GetNumCharges())
				{
					StackQuantityOrNumChargesShown = SlotInfo.GetNumCharges();
					Text_QuantityOrNumCharges->SetText(FText::AsNumber(SlotInfo.GetNumCharges()));
				}
			}
			else
			{
				if (StackQuantityOrNumChargesShown != -1)
				{
					StackQuantityOrNumChargesShown = -1;

					// Make text blank is my way of hiding it
					Text_QuantityOrNumCharges->SetText(FText::GetEmpty());
				}
			}
		}

		if (IsWidgetBound(Text_Name))
		{
			Text_Name->SetText(ItemsInfo->GetDisplayName());
		}

		const float CooldownRemaining = SlotInfo.GetUseCooldownRemaining(WorldTimerManager);
		
		/* We will assign this 0 if the item is not usable */
		ItemUseTotalCooldown = SlotInfo.IsItemUsable() ? ItemsInfo->GetUseAbilityInfo()->GetCooldown() : 0.f;

		// Set cooldown remaining text value
		if (IsWidgetBound(Text_UseCooldown))
		{ 
			/* Check if item is both usable and has a cooldown */
			if (ItemUseTotalCooldown > 0.f)
			{
				Text_UseCooldown->SetText(ItemUseCooldownFloatToText(CooldownRemaining));
			}
			else
			{
				// Make text blank is my way of hiding it
				Text_UseCooldown->SetText(FText::GetEmpty());
			}
		}

		// Set cooldown remaining progress bar percentage
		if (IsWidgetBound(ProgressBar_UseCooldown))
		{
			/* Check if item is both usable and has a cooldown */
			if (ItemUseTotalCooldown > 0.f)
			{
				ProgressBar_UseCooldown->SetPercent(CooldownRemaining / ItemUseTotalCooldown);
			}
			else
			{
				// Set 0 to effectively make the progress bar hidden
				ProgressBar_UseCooldown->SetPercent(0.f);
			}
		}

		DisplayedItemType = SlotInfo.GetItemType();

		return CooldownRemaining > 0.f;
	}
}

void UInventoryItemButton::OnItemAdded(const FInventorySlotState & UpdatedSlot)
{
	// Param irrelevant, InventorySlot == UpdatedSlot 

	// Check whether just quantity changed or if the item type changed too. 
	if (UpdatedSlot.GetItemType() == DisplayedItemType)
	{ 
		// Just need to update quantity 
		if (IsWidgetBound(Text_QuantityOrNumCharges))
		{
			StackQuantityOrNumChargesShown = UpdatedSlot.GetNumInStack();
			Text_QuantityOrNumCharges->SetText(FText::AsNumber(UpdatedSlot.GetNumInStack()));
		}
	}
	else
	{
		SlotsItemInfo = GI->GetInventoryItemInfo(UpdatedSlot.GetItemType());

		if (IsWidgetBound(Button_Button))
		{
			Button_Button->SetImages(UnifiedBrushAndSoundFlags,
				&SlotsItemInfo->GetDisplayImage(),
				&SlotsItemInfo->GetDisplayImage_Hovered(),
				&SlotsItemInfo->GetDisplayImage_Pressed(),
				&SlotsItemInfo->GetDisplayImage_Highlighted());
			if (UnifiedBrushAndSoundFlags.bUsingUnifiedHoverImage == false)
			{
				Button_Button->SetHoveredSound(SlotsItemInfo->GetUISound_Hovered());
			}
			if (UnifiedBrushAndSoundFlags.bUsingUnifiedLMBPressSound == false)
			{
				Button_Button->SetPressedByLMBSound(SlotsItemInfo->GetUISound_PressedByLMB());
			}
			if (UnifiedBrushAndSoundFlags.bUsingUnifiedRMBPressSound == false)
			{
				Button_Button->SetPressedByRMBSound(SlotsItemInfo->GetUISound_PressedByRMB());
			}
		}

		if (IsWidgetBound(Text_QuantityOrNumCharges))
		{
			/* Here we choose to show a number in this text block if either:
			- the item can stack
			- it has a finite number of charges

			If neither of those are the case then we make the text blank to pseudo hide it which
			might be cheaper than doing SetVisibility */

			if (UpdatedSlot.CanItemStack())
			{
				if (StackQuantityOrNumChargesShown != UpdatedSlot.GetNumInStack())
				{
					StackQuantityOrNumChargesShown = UpdatedSlot.GetNumInStack();
					Text_QuantityOrNumCharges->SetText(FText::AsNumber(UpdatedSlot.GetNumInStack()));
				}
			}
			else if (SlotsItemInfo->IsUsable() && SlotsItemInfo->HasUnlimitedNumberOfUsesChecked() == false)
			{
				if (StackQuantityOrNumChargesShown != UpdatedSlot.GetNumCharges())
				{
					StackQuantityOrNumChargesShown = UpdatedSlot.GetNumCharges();
					Text_QuantityOrNumCharges->SetText(FText::AsNumber(UpdatedSlot.GetNumCharges()));
				}
			}
			else
			{
				if (StackQuantityOrNumChargesShown != -1)
				{
					StackQuantityOrNumChargesShown = -1;

					// Make text blank to simulate hiding it
					Text_QuantityOrNumCharges->SetText(FText::GetEmpty());
				}
			}
		}

		if (IsWidgetBound(Text_Name))
		{
			Text_Name->SetText(SlotsItemInfo->GetDisplayName());
		}
	}

	//InventorySlot = &UpdatedSlot; // Probably redundent
	DisplayedItemType = UpdatedSlot.GetItemType();
}

void UInventoryItemButton::OnAmountRemoved()
{
	// Trying HasItem here instead. GetNumInStack will always return 1 for non-stackables
	if (/*InventorySlot->GetNumInStack() == 0 */ InventorySlot->HasItem() == false)
	{
		/* Now the stack is empty */

		SetAppearanceForEmptySlot(UnifiedBrushAndSoundFlags);
		SlotsItemInfo = nullptr;
		DisplayedItemType = EInventoryItem::None;
	}
	else
	{
		// Just change quantity text
		if (IsWidgetBound(Text_QuantityOrNumCharges))
		{
			Text_QuantityOrNumCharges->SetText(FText::AsNumber(InventorySlot->GetNumInStack()));
		}
	}
}

void UInventoryItemButton::OnUsed(FTimerManager & WorldTimerManager)
{
	const bool bIsSlotEmptyNow = (InventorySlot->HasItem() == false);
	if (bIsSlotEmptyNow)
	{
		SetAppearanceForEmptySlot(UnifiedBrushAndSoundFlags);
		SlotsItemInfo = nullptr;
		DisplayedItemType = EInventoryItem::None;
	}
	else
	{
		/* Check if use ability has cooldown. "Don't actually need this check" is what I 
		wrote but we divide by this later on so I think we do */
		if (ItemUseTotalCooldown > 0.f)
		{
			const float CooldownRemaining = InventorySlot->GetUseCooldownRemainingChecked(WorldTimerManager);

			if (IsWidgetBound(Text_UseCooldown))
			{
				Text_UseCooldown->SetText(ItemUseCooldownFloatToText(CooldownRemaining));
			}

			if (IsWidgetBound(ProgressBar_UseCooldown))
			{
				/* Set the progress bar percentage based on the ability's cooldown. Although -1.f should
				never be set here even if it is it's ok visuals wise. Tbh it should always be 1 anyway*/
				ProgressBar_UseCooldown->SetPercent(CooldownRemaining / ItemUseTotalCooldown);
			}
		}

		/* Quite possible that number of charges is 1 less than before so try update it */
		if (IsWidgetBound(Text_QuantityOrNumCharges))
		{
			/* This is probably going to fail if... ? */
			if (StackQuantityOrNumChargesShown != InventorySlot->GetNumCharges())
			{
				StackQuantityOrNumChargesShown = InventorySlot->GetNumCharges();
				Text_QuantityOrNumCharges->SetText(FText::AsNumber(InventorySlot->GetNumCharges()));
			}
		}
	}
}

void UInventoryItemButton::UpdateCooldown(float DeltaTime, const FTimerManager & WorldTimerManager)
{
	const float CooldownRemaining = InventorySlot->GetUseCooldownRemainingChecked(WorldTimerManager);
	
	if (IsWidgetBound(Text_UseCooldown))
	{
		Text_UseCooldown->SetText(ItemUseCooldownFloatToText(CooldownRemaining));
	}
	
	if (IsWidgetBound(ProgressBar_UseCooldown))
	{
		ProgressBar_UseCooldown->SetPercent(CooldownRemaining / ItemUseTotalCooldown);
	}
}

void UInventoryItemButton::OnUseCooldownFinished()
{
	/* Could play a little flash or something now to show the item is now off cooldown */
	
	/* Make these two widgets pseudo-hidden by setting empty text and 0 percent */
	if (IsWidgetBound(Text_UseCooldown))
	{
		Text_UseCooldown->SetText(FText::GetEmpty());
	}
	if (IsWidgetBound(ProgressBar_UseCooldown))
	{
		ProgressBar_UseCooldown->SetPercent(0.f);
	}
}


//===============================================================================================
//	Context Menu Action Bar
//===============================================================================================

USelectableActionBar::USelectableActionBar(const FObjectInitializer & ObjectInitializer)
	: Super(ObjectInitializer)
{
	UnclickableButtonAlpha = 0.5f;
}

bool USelectableActionBar::SetupWidget(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController)
{
	/* This should be done before match starts - kind of performance intensive */

	if (Super::SetupWidget(InGameInstance, InPlayerController))
	{
		return true;
	}

	TArray < UWidget * > AllChildren;
	WidgetTree->GetAllWidgets(AllChildren);

	/* Assuming that the order they appear in editor is the same order we get them by iterating */
	for (const auto & Elem : AllChildren)
	{
		if (Elem->IsA(UContextActionButton::StaticClass()))
		{
			UContextActionButton * const AsButton = CastChecked<UContextActionButton>(Elem);

			AsButton->Setup(GI, PC);

			ActionButtons.Emplace(AsButton);
		}
	}

	// Check that the user has added enough button widgets to support the largest context menu. 
	// Only checking our own faction, but maybe if context buttons show on unowned selectables 
	// too then we might want to check that too?
	UE_CLOG(FI->GetLargestContextMenu() > ActionButtons.Num(), RTSLOG, Fatal, TEXT("For faction \"%s\": "
		"Not enough context menu buttons on HUD's context menu action bar. Amount required: %d, "
		"actual amount: %d"), TO_STRING(EFaction, FI->GetFaction()), FI->GetLargestContextMenu(), 
			ActionButtons.Num());

	return false;
}

void USelectableActionBar::OnTick(float InDeltaTime)
{
	if (GetVisibility() != ESlateVisibility::Collapsed && Statics::IsValid(CurrentSelected))
	{
		UpdateButtonCooldowns();
	}
}

void USelectableActionBar::OnPlayerSelectionChanged(const TScriptInterface <ISelectable> NewCurrentSelected, const FSelectableAttributesBase & Attributes)
{
	// TODO set is usuable based on prerequisites
	
	CurrentSelected = NewCurrentSelected;

	if (Attributes.GetAffiliation() == EAffiliation::Owned)
	{
		SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		const TMap < FContextButton, FTimerHandle > * SelectedContextCooldowns = NewCurrentSelected->GetContextCooldowns();
		const FBuildingAttributes * BuildingAttr = NewCurrentSelected->GetBuildingAttributes();

		const TArray < FContextButton > & Buttons = Attributes.GetContextMenu().GetButtonsArray();

		NumButtonsInMenu = Buttons.Num();

		/* These are used in the loop below. My goal with these is to hopefully make the 
		compiler hoist these out of the loop. I should check assembly if I actually knew 
		where to look...
		Whenever we call this func we do a lot in that single frame so making this as fast 
		as possible is good. So check that asm.  
		I mean a sure fire way would be to make these unified flags as preprocessors instead 
		of variables inside GI. */
		const FUnifiedImageAndSoundFlags UnifiedAssetFlags = GI->GetUnifiedButtonAssetFlags_ActionBar();

		ActionButtonsMap.Reset();

		FTimerManager & TimerManager = GetWorld()->GetTimerManager();

		for (int32 i = 0; i < NumButtonsInMenu; ++i)
		{
			const FContextButton & Button = Buttons[i];

			UContextActionButton * ActionButton = ActionButtons[i];

			ActionButtonsMap.Emplace(Button, ActionButtons[i]);

			if (Button.IsForTrainUnit() || Button.IsForResearchUpgrade())
			{
				const FDisplayInfoBase * DisplayInfo = FI->GetDisplayInfo(Button);

				/* Assuming here that it is a building doing the producing because either 
				Build/Train/Upgrade type button */

				ActionButton->MakeActive(&Button, DisplayInfo, *BuildingAttr, UnifiedAssetFlags, 
					UnclickableButtonAlpha);
			}
			else if (Button.IsForBuildBuilding())
			{
				const FBuildingInfo * BuildingInfo = FI->GetBuildingInfo(Button.GetBuildingType());
				// Null implies current selected is not a building
				if (BuildingAttr == nullptr)
				{
					const EBuildingBuildMethod BuildMethod = BuildingInfo->GetBuildingBuildMethod();
					
					/* Units shouldn't really be able to build a building using one of these
					methods. If they want to build self-constructing buildings then use Protoss
					build method */
					UE_CLOG(BuildMethod == EBuildingBuildMethod::BuildsInTab
						|| BuildMethod == EBuildingBuildMethod::BuildsItself, RTSLOG, Fatal,
						TEXT("Unit type \"%s\" for faction \"%s\" has a BuildBuilding button for building type \"%s\" on its "
							"context menu that uses the build method %s. Buildings with this build method "
							"are not really allowed on context menus. Either remove the button from the "
							"context menu or change the building's build method"),
						TO_STRING(EUnitType, NewCurrentSelected->GetAttributesBase().GetUnitType()),
						TO_STRING(EFaction, FI->GetFaction()),
						TO_STRING(EBuildingType, Button.GetBuildingType()),
						TO_STRING(EBuildingBuildMethod, BuildMethod));
				}

				ActionButton->MakeActive(&Button, BuildingInfo, BuildingAttr, UnifiedAssetFlags, 
					UnclickableButtonAlpha);
			}
			else // Ability
			{
				//bIsForProductionAction = false; // Variable removed

				const FContextButtonInfo & ContextInfo = GI->GetContextInfo(Button.GetButtonType());

				assert(SelectedContextCooldowns != nullptr);

				// Set timer handle if a non upgrade/train/build button
				const FTimerHandle * TimerHandle = &(*SelectedContextCooldowns)[Button];

				/* This logic should really be made similar to UpdateCooldownProgressBarAndText */

				ActionButton->MakeActive(&Button, ContextInfo, TimerHandle, TimerManager, UnifiedAssetFlags);
			}
		}

		// Make remaining buttons invisible 
		for (int32 i = NumButtonsInMenu; i < ActionButtons.Num(); ++i)
		{
			UContextActionButton * ActionButton = ActionButtons[i];

			ActionButton->Disable();
		}
	}
	else // Selectable not controlled by us - make bar invisible
	{
		SetVisibility(ESlateVisibility::Collapsed);
	}
}

void USelectableActionBar::OnNoPlayerSelection()
{
	CurrentSelected = nullptr;
}

void USelectableActionBar::UpdateButtonCooldowns()
{
	if (Statics::IsValid(CurrentSelected))
	{
		for (const auto & Elem : ActionButtons)
		{
			Elem->UpdateCooldownProgressBarAndText();
		}
	}
}

void USelectableActionBar::OnProductionComplete(const FTrainingInfo & Item, const FProductionQueue & Queue, 
	uint8 NumRemoved, AActor * Producer)
{
	if (Item.IsForUpgrade())
	{
		/* Grey out the button since we cannot research this upgrade anymore. This assumes that 
		upgrades can only be researched once. */
		ActionButtonsMap[FContextButton(Item)]->OnUpgradeResearchComplete();
	}
}

void USelectableActionBar::OnUpgradeComplete(EUpgradeType UpgradeType)
{
	if (!Statics::IsValid(CurrentSelected))
	{
		return;
	}

	/* Check if any of the button's prerequisites are now met */
	for (int32 i = 0; i < NumButtonsInMenu; ++i)
	{
		UContextActionButton * Button = ActionButtons[i];

		/* Check if state has changed */
		if (!Button->ArePrequisitesMet() && PS->ArePrerequisitesMet(*Button->GetButtonType(), false))
		{
			Button->OnPrerequisitesMet();
		}
	}
}

void USelectableActionBar::OnBuildingConstructed(EBuildingType BuildingType, bool bFirstOfItsType)
{
	assert(PS != nullptr);

	if (!Statics::IsValid(CurrentSelected))
	{
		return;
	}

	/* Basically if you're using a build method that is for a persistent tab then you should check
	PS::GetNumPersistentQueues also, but for now we're just assuming it is a non-persistent type
	of build method the buildings in the context menu have */
	for (int32 i = 0; i < NumButtonsInMenu; ++i)
	{
		UContextActionButton * Button = ActionButtons[i];

		/* Check if state has changed */
		if (!Button->ArePrequisitesMet() && PS->ArePrerequisitesMet(*Button->GetButtonType(), false))
		{
			Button->OnPrerequisitesMet();
		}
	}
}

void USelectableActionBar::OnBuildingDestroyed(EBuildingType BuildingType, bool bLastOfItsType)
{
	if (!Statics::IsValid(CurrentSelected))
	{
		return;
	}

	for (int32 i = 0; i < NumButtonsInMenu; ++i)
	{
		UContextActionButton * Button = ActionButtons[i];

		/* Check if state has changed */
		if (Button->ArePrequisitesMet() && !PS->ArePrerequisitesMet(*Button->GetButtonType(), false))
		{
			Button->OnPrerequisiteLost(UnclickableButtonAlpha);
		}
	}
}


//==========================================================================================
//	 Context Menu Action Bar Button
//==========================================================================================

UContextActionButton::UContextActionButton(const FObjectInitializer & ObjectInitializer)
	: Super(ObjectInitializer)
{
	Purpose = EUIElementType::SelectablesActionBar;
	
	/* Remove focus so user cannot click button, grab focus then press enter to click button */
	//IsFocusable = false; // UButton variable, not here after switch to UMyButton
}

void UContextActionButton::Setup(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController)
{
	OriginalAlpha = GetRenderOpacity();

	GI = InGameInstance;
	PC = InPlayerController;
	PS = PC->GetPS();
	FI = InPlayerController->GetFactionInfo();

	SetImageAndProgressBarReferences(this, CooldownProgressBar, DescriptionText, ExtraText, 
		true, CooldownRemainingText);

	/* Crashing here is because UWidget::RebuildWidget is not being called until later on, 
	not sure when exactly. I think in AddToViewport is when RebuildWidget is called so that 
	would explain it */
	OnLeftMouseButtonPressed().BindUObject(this, &UContextActionButton::UIBinding_OnLMBPressed);
	OnLeftMouseButtonReleased().BindUObject(this, &UContextActionButton::UIBinding_OnLMBReleased);
	OnRightMouseButtonPressed().BindUObject(this, &UContextActionButton::UIBinding_OnRMBPressed);
	OnRightMouseButtonReleased().BindUObject(this, &UContextActionButton::UIBinding_OnRMBReleased);

	/* Set unified brushs/sounds if required */
	const FUnifiedImageAndSoundFlags UnifiedAssetFlags = InGameInstance->GetUnifiedButtonAssetFlags_ActionBar();
	SetUnifiedImages_ExcludeNormalAndHighlightedImage(UnifiedAssetFlags,
		&InGameInstance->GetUnifiedHoverBrush_ActionBar().GetBrush(),
		&InGameInstance->GetUnifiedPressedBrush_ActionBar().GetBrush());
	if (InGameInstance->GetUnifiedHoverSound_ActionBar().UsingSound())
	{
		SetHoveredSound(InGameInstance->GetUnifiedHoverSound_ActionBar().GetSound());
	}
	if (InGameInstance->GetUnifiedLMBPressedSound_ActionBar().UsingSound())
	{
		SetPressedByLMBSound(InGameInstance->GetUnifiedLMBPressedSound_ActionBar().GetSound());
	}
	if (InGameInstance->GetUnifiedRMBPressedSound_ActionBar().UsingSound())
	{
		SetPressedByRMBSound(InGameInstance->GetUnifiedRMBPressedSound_ActionBar().GetSound());
	}

	SetPC(InPlayerController);
	SetOwningWidget();
}

#if WITH_EDITOR
void UContextActionButton::OnCreationFromPalette()
{
	UContextActionButton::OnButtonCreationFromPalette(this, true);
}
#endif

void UContextActionButton::OnButtonCreationFromPalette(UMyButton * Button, bool bCreateCooldownText)
{
	/* Construct the extra parts of the button, specifically the overlay, image and progress bar */

	// First get reference to widget tree of user widget this was just placed on

	UWidgetTree * WidgetTree = nullptr;

	UObject * Outer = Button->GetOuter();
	while (Outer != nullptr)
	{
		if (Outer->IsA(UWidgetTree::StaticClass()))
		{
			WidgetTree = CastChecked<UWidgetTree>(Outer);
			break;
		}
		else
		{
			Outer = Outer->GetOuter();
		}
	}

	/* Add overlay to allow image and progress bar to be stacked on top each other. Names are
	not really important */
	const FName OverlayFName = *(Button->GetFName().ToString() + "_Overlay");
	UOverlay * Overlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), OverlayFName);
	Button->AddChild(Overlay);

	/* Set some common default values */
	UMyButtonSlot * const ButtonSlot = CastChecked<UMyButtonSlot>(Overlay->Slot);
	ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
	ButtonSlot->SetVerticalAlignment(VAlign_Fill);

	/* Create text box */
	const FName TextName = *(Button->GetFName().ToString() + "_Text");
	UMultiLineEditableText * Text = WidgetTree->ConstructWidget<UMultiLineEditableText>(UMultiLineEditableText::StaticClass(), TextName);
	Overlay->AddChild(Text);

	/* Set some common default values */
	UOverlaySlot * const SlotForText = CastChecked<UOverlaySlot>(Text->Slot);
	SlotForText->SetHorizontalAlignment(HAlign_Fill);
	SlotForText->SetVerticalAlignment(VAlign_Bottom);
	Text->SetIsReadOnly(true);
	Text->WidgetStyle.SetColorAndOpacity(FSlateColor(FLinearColor(0.f, 0.f, 0.f, 1.f)));
	Text->SetText(FText::FromString("Sample text"));
	// Protected, gotta do through editor 
	//Text->Justification = ETextJustify::Center;

	/* Create another text box. This name is actually important. Will use it later in
	SetImageAndProgressBarReferences and therefore should not be changed */
	const FName ExtraTextName = *(Button->GetFName().ToString() + "_ExtraText");
	Text = WidgetTree->ConstructWidget<UMultiLineEditableText>(UMultiLineEditableText::StaticClass(), ExtraTextName);
	Overlay->AddChild(Text);

	/* Set some default values */
	UOverlaySlot * const SlotForExtraText = CastChecked<UOverlaySlot>(Text->Slot);
	SlotForExtraText->SetHorizontalAlignment(HAlign_Fill);
	SlotForExtraText->SetVerticalAlignment(VAlign_Fill);
	Text->SetIsReadOnly(true);
	Text->WidgetStyle.SetColorAndOpacity(FSlateColor(FLinearColor(0.f, 0.f, 0.f, 1.f)));
	Text->SetText(FText::FromString(""));

	/* Create progress bar, child it to overlay and save reference */
	const FName BarFName = *(Button->GetFName().ToString() + "_CooldownBar");
	UProgressBar * const ProgressBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), BarFName);
	Overlay->AddChild(ProgressBar);

	/* Set some common default values */
	UOverlaySlot * const SlotForPBar = CastChecked<UOverlaySlot>(ProgressBar->Slot);
	SlotForPBar->SetHorizontalAlignment(HAlign_Fill);
	SlotForPBar->SetVerticalAlignment(VAlign_Fill);
	// Hide background so image can be seen 
	ProgressBar->WidgetStyle.BackgroundImage.DrawAs = ESlateBrushDrawType::NoDrawType;
	// Lower fill image alpha so image can still be seen through it 
	ProgressBar->WidgetStyle.FillImage.TintColor = FSlateColor(FLinearColor(0.1f, 0.1f, 0.1f, 0.5f));

	if (bCreateCooldownText)
	{
		/* Create a text block for the purpose of showing cooldown, child it to overlay and 
		save reference */
		const FName CooldownTextFName = *(Button->GetName() + "_CooldownText");
		UTextBlock * const DurationTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), CooldownTextFName);
		Overlay->AddChild(DurationTextBlock);

		/* Set some common default values */
		UOverlaySlot * const SlotForCooldownText = CastChecked<UOverlaySlot>(DurationTextBlock->Slot);
		SlotForCooldownText->SetHorizontalAlignment(HAlign_Center);
		SlotForCooldownText->SetVerticalAlignment(VAlign_Center);
		DurationTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
		DurationTextBlock->SetText(FText::FromString("Num"));
	}
}

void UContextActionButton::SetImageAndProgressBarReferences(UMyButton * Caller,
	UProgressBar *& InProgressBar, UMultiLineEditableText *& InDescriptionText, 
	UMultiLineEditableText *& InExtraText, bool bFindCooldownRemainingText, 
	UTextBlock *& InCooldownRemainingText)
{
	assert(InProgressBar == nullptr && InDescriptionText == nullptr 
		&& InExtraText == nullptr && InCooldownRemainingText == nullptr);

	/* First get ref to overlay widget */

	UOverlay * Overlay = nullptr;

	for (int32 i = 0; i < Caller->GetChildrenCount(); ++i)
	{
		UWidget * Widget = Caller->GetChildAt(i);
		if (Widget->IsA(UOverlay::StaticClass()))
		{
			Overlay = CastChecked<UOverlay>(Widget);
			break;
		}
	}

	UE_CLOG(Overlay == nullptr, RTSLOG, Fatal, TEXT("No overlay widget found as a direct child, "
		"errors will occur when selecting something during a match"));

	// Now iterate overlays direct children searching for image and progress bar
	int32 NumFound = 0;
	int32 NumRequiredToFind = bFindCooldownRemainingText ? 4 : 3;
	for (int32 i = 0; i < Overlay->GetChildrenCount(); ++i)
	{
		UWidget * const Widg = Overlay->GetChildAt(i);
		if (Widg->IsA(UProgressBar::StaticClass()))
		{
			InProgressBar = CastChecked<UProgressBar>(Widg);
			NumFound++;
		}
		else if (Widg->IsA(UMultiLineEditableText::StaticClass()))
		{
			/* There are two kinds of text we are searching for. Going by their name to distunguish
			between them */
			const FString Name = *Widg->GetName();
			if (Name.Right(10) == "_ExtraText")
			{
				/* If thrown there may be two UMultilineEditableText with a name ending in
				"_ExtraText" */
				assert(InExtraText == nullptr);
				InExtraText = CastChecked<UMultiLineEditableText>(Widg);
			}
			else
			{
				/* If thrown it is likely there is no UMultilineEditableText on button with a name
				ending in "_ExtraText" */
				assert(InDescriptionText == nullptr);
				InDescriptionText = CastChecked<UMultiLineEditableText>(Widg);
			}

			NumFound++;
		}
		else if (bFindCooldownRemainingText && Widg->IsA(UTextBlock::StaticClass()))
		{
			/* Since user may want to add multiple text blocks we check the name to determine 
			if we should use it or not */
			if (Widg->GetName().Right(13) == "_CooldownText")
			{
				/* If thrown there may be two UTextBlock with a name ending in "_CooldownText" */
				assert(InCooldownRemainingText == nullptr);
				InCooldownRemainingText = CastChecked<UTextBlock>(Widg);
				NumFound++;
			}
		}

		if (NumFound == NumRequiredToFind)
		{
			// Can stop now; found everything
			break;
		}
	}

	assert(NumFound == NumRequiredToFind);
}

FText UContextActionButton::GetDurationRemainingText(float Duration)
{
	FloatToText(Duration, UIOptions::AbilityCooldownDisplayMethod);
}

void UContextActionButton::SetAppearanceForFullyResearchedUpgrade()
{
	/* The behavior we take here is we will make the button hidden */
	SetVisibility(HUDStatics::HIDDEN_VISIBILITY);
}

void UContextActionButton::UIBinding_OnLMBPressed()
{
	PC->OnLMBPressed_PrimarySelected_ActionBar(this);
}

void UContextActionButton::UIBinding_OnLMBReleased()
{
	PC->OnLMBReleased_PrimarySelected_ActionBar(this);
}

void UContextActionButton::UIBinding_OnRMBPressed()
{
	PC->OnRMBPressed_PrimarySelected_ActionBar(this);
}

void UContextActionButton::UIBinding_OnRMBReleased()
{
	PC->OnRMBReleased_PrimarySelected_ActionBar(this);
}

void UContextActionButton::MakeActive(const FContextButton * InButtonType, const FDisplayInfoBase * DisplayInfo, 
	const FBuildingAttributes & PrimarySelectedBuildingAttributes, FUnifiedImageAndSoundFlags UnifiedAssetFlags, 
	float InUnclickableButtonAlpha)
{
	ButtonType = InButtonType;
	ProductionQueue = &PrimarySelectedBuildingAttributes.ProductionQueue;
	bIsForProductionAction = true;

	/* I think this was being set as null from previous MakeActive big function impl, but I don't 
	know if that's right. Should test this */
	TimerHandle_ProgressBar = nullptr;

	/* Just copying how UpdateCooldownProgressBarAndText does it. But perhaps some of these
	checks don't need to happen like the Peek() == ButtonType - this may have already
	been checked during the function that called MakeActive */
	float PBarPercent = 0.f;
	if (ProductionQueue->Num() > 0 && ProductionQueue->Peek() == *InButtonType)
	{
		const float PercentageComplete = ProductionQueue->GetPercentageCompleteForUI(GetWorld());
		PBarPercent = (PercentageComplete == 0.f) ? (0.f) : (1.f - PercentageComplete);
	}

	CooldownProgressBar->SetPercent(PBarPercent);
	/* Do not show duration text for production buttons */
	bIsCooldownTextEmpty = true;
	CooldownRemainingText->SetText(FText::GetEmpty());

	/* Set images/sounds */
	SetImages_ExcludeHighlightedImage(UnifiedAssetFlags,
		&DisplayInfo->GetHUDImage(),
		&DisplayInfo->GetHoveredHUDImage(),
		&DisplayInfo->GetPressedHUDImage());
	if (UnifiedAssetFlags.bUsingUnifiedHoverSound == false)
	{
		SetHoveredSound(DisplayInfo->GetHoveredButtonSound());
	}
	if (UnifiedAssetFlags.bUsingUnifiedLMBPressSound == false)
	{
		SetPressedByLMBSound(DisplayInfo->GetPressedByLMBSound());
	}
	if (UnifiedAssetFlags.bUsingUnifiedRMBPressSound == false)
	{
		SetPressedByRMBSound(DisplayInfo->GetPressedByRMBSound());
	}

	DescriptionText->SetText(DisplayInfo->GetName());

	/* Need to check if prereqs are met or not since if they changed while button was inactive
	it has nothing to go on since it didn't have ButtonType assigned */
	if (InButtonType->IsForResearchUpgrade())
	{
		if (PS->GetUpgradeManager()->HasBeenFullyResearched(InButtonType->GetUpgradeType(), false) == true)
		{
			SetAppearanceForFullyResearchedUpgrade();
		}
		else
		{
			if (PS->ArePrerequisitesMet(*InButtonType, false) == true)
			{
				OnPrerequisitesMet();
			}
			else
			{
				OnPrerequisiteLost(InUnclickableButtonAlpha);
			}
		}
	}
	else
	{
		if (PS->ArePrerequisitesMet(*InButtonType, false) == true)
		{
			OnPrerequisitesMet();
		}
		else
		{
			OnPrerequisiteLost(InUnclickableButtonAlpha);
		}
	}

	SetVisibility(ESlateVisibility::Visible);
}

void UContextActionButton::MakeActive(const FContextButton * InButtonType, const FBuildingInfo * BuildingInfo,
	const FBuildingAttributes * PrimarySelectedBuildingAttributes, 
	FUnifiedImageAndSoundFlags UnifiedAssetFlags, float InUnclickableButtonAlpha)
{
	// 23/12/18: created this func. Might want to check previous MakeActive code if this turns out to cause issues
	
	ButtonType = InButtonType;

	// Null implies current selected is not a building
	if (PrimarySelectedBuildingAttributes == nullptr)
	{
		// Just copying what I had before in the one hude MakeActive func. What I wrote was:
		/* The build method doesn't require a progress bar on the button */
		bIsForProductionAction = false;
		ProductionQueue = nullptr;

		// This is my way of hiding them
		CooldownProgressBar->SetPercent(0.f);
		bIsCooldownTextEmpty = true;
		CooldownRemainingText->SetText(FText::GetEmpty());
	}
	else
	{
		const EBuildingBuildMethod BuildMethod = BuildingInfo->GetBuildingBuildMethod();
		if (BuildMethod == EBuildingBuildMethod::BuildsInTab
			|| BuildMethod == EBuildingBuildMethod::BuildsItself)
		{
			bIsForProductionAction = true;
			
			/* 23/12/18: changed from ProductionQueue to PersistentProductionQueue, surely that 
			is what I ment */
			ProductionQueue = &PrimarySelectedBuildingAttributes->PersistentProductionQueue;

			/* Just copying how UpdateCooldownProgressBarAndText does it. But perhaps some of these
			checks don't need to happen like the Peek() == ButtonType - this may have already
			been checked during the function that called MakeActive */
			float PBarPercent = 0.f;
			if (ProductionQueue->Num() > 0 && ProductionQueue->Peek() == *InButtonType)
			{
				const float PercentageComplete = ProductionQueue->GetPercentageCompleteForUI(GetWorld());
				PBarPercent = (PercentageComplete == 0.f) ? (0.f) : (1.f - PercentageComplete);
			}

			CooldownProgressBar->SetPercent(PBarPercent);
			/* Do not show duration text for production buttons */
			bIsCooldownTextEmpty = true;
			CooldownRemainingText->SetText(FText::GetEmpty());
		}
		else
		{
			bIsForProductionAction = false;
			ProductionQueue = nullptr;

			// This is my way of hiding them
			CooldownProgressBar->SetPercent(0.f);
			bIsCooldownTextEmpty = true;
			CooldownRemainingText->SetText(FText::GetEmpty());
		}
	}

	/* Set images/sounds */
	SetImages(UnifiedAssetFlags,
		&BuildingInfo->GetHUDImage(),
		&BuildingInfo->GetHoveredHUDImage(),
		&BuildingInfo->GetPressedHUDImage(),
		&BuildingInfo->GetHighlightedHUDImage());
	if (UnifiedAssetFlags.bUsingUnifiedHoverSound == false)
	{
		SetHoveredSound(BuildingInfo->GetHoveredButtonSound());
	}
	if (UnifiedAssetFlags.bUsingUnifiedLMBPressSound == false)
	{
		SetPressedByLMBSound(BuildingInfo->GetPressedByLMBSound());
	}
	if (UnifiedAssetFlags.bUsingUnifiedRMBPressSound == false)
	{
		SetPressedByRMBSound(BuildingInfo->GetPressedByRMBSound());
	}

	// Set text
	DescriptionText->SetText(BuildingInfo->GetName());

	/* Need to check if prereqs are met or not since if they changed while button was inactive
	it has nothing to go on since it didn't have ButtonType assigned */
	if (InButtonType->IsForResearchUpgrade())
	{
		if (PS->GetUpgradeManager()->HasBeenFullyResearched(InButtonType->GetUpgradeType(), false) == true)
		{
			SetAppearanceForFullyResearchedUpgrade();
		}
		else
		{
			if (PS->ArePrerequisitesMet(*InButtonType, false) == true)
			{
				OnPrerequisitesMet();
			}
			else
			{
				OnPrerequisiteLost(InUnclickableButtonAlpha);
			}
		}
	}
	else
	{
		if (PS->ArePrerequisitesMet(*InButtonType, false) == true)
		{
			OnPrerequisitesMet();
		}
		else
		{
			OnPrerequisiteLost(InUnclickableButtonAlpha);
		}
	}

	SetVisibility(ESlateVisibility::Visible);
}

void UContextActionButton::MakeActive(const FContextButton * InButtonType, const FContextButtonInfo & AbilityInfo,
	const FTimerHandle * CooldownTimerHandle, FTimerManager & WorldTimerManager, FUnifiedImageAndSoundFlags UnifiedAssetFlags)
{
	ButtonType = InButtonType;
	ProductionQueue = nullptr;
	bIsForProductionAction = false;

	TimerHandle_ProgressBar = CooldownTimerHandle;
	
	const bool bOnCooldown = WorldTimerManager.IsTimerActive(*CooldownTimerHandle);
	if (bOnCooldown)
	{
		/* Work out percentage of progress bar */
		const float CooldownRemaining = WorldTimerManager.GetTimerRemaining(*CooldownTimerHandle);
		const float TotalCooldownDuration = CooldownRemaining + WorldTimerManager.GetTimerElapsed(*CooldownTimerHandle);
		
		CooldownProgressBar->SetPercent(CooldownRemaining / TotalCooldownDuration);
		bIsCooldownTextEmpty = false;
		CooldownRemainingText->SetText(GetDurationRemainingText(CooldownRemaining));
	}
	else
	{
		CooldownProgressBar->SetPercent(0.f);
		bIsCooldownTextEmpty = true;
		CooldownRemainingText->SetText(FText::GetEmpty());
	}

	/* Set images/sounds */
	SetImages(UnifiedAssetFlags,
		&AbilityInfo.GetImage(),
		&AbilityInfo.GetHoveredImage(),
		&AbilityInfo.GetPressedImage(),
		&AbilityInfo.GetHighlightedImage());
	if (UnifiedAssetFlags.bUsingUnifiedHoverSound == false)
	{
		SetHoveredSound(AbilityInfo.GetHoveredSound());
	}
	if (UnifiedAssetFlags.bUsingUnifiedLMBPressSound == false)
	{
		SetPressedByLMBSound(AbilityInfo.GetPressedByLMBSound());
	}
	if (UnifiedAssetFlags.bUsingUnifiedRMBPressSound == false)
	{
		SetPressedByRMBSound(AbilityInfo.GetPressedByRMBSound());
	}
	
	DescriptionText->SetText(AbilityInfo.GetName());

	/* Prereqs always met for abilities. Actually just copying code from previous MakeActive 
	implementation */
	OnPrerequisitesMet();

	SetVisibility(ESlateVisibility::Visible);
}

void UContextActionButton::UpdateCooldownProgressBarAndText()
{
	float FillPercentage = 0.f;

	if (bIsForProductionAction)
	{
		/* FIXME optimization: use OnItemAdded...Queue functions for this bar and on those
		events keep track of which buttons require PBar updates, then only call this func
		on those buttons */
		// This check sees if this button is the one being produced right now by queue
		if (ProductionQueue->Num() > 0 && ProductionQueue->Peek() == *ButtonType)
		{
			/* If producing something fill inverse of % complete, otherwise fill with 0 */
			const float PercentageComplete = ProductionQueue->GetPercentageCompleteForUI(GetWorld());
			FillPercentage = (PercentageComplete == 0.f) ? (0.f) : (1.f - PercentageComplete);
		}

		/* Since we don't show duration for production items we don't need to set the text since 
		it would have been set to empty in MakeActive and we just leave it like that */
	}
	else
	{
		if (TimerHandle_ProgressBar != nullptr)
		{
			if (GetWorld()->GetTimerManager().IsTimerActive(*TimerHandle_ProgressBar))
			{
				const float TimeElapsed = GetWorld()->GetTimerManager().GetTimerElapsed(*TimerHandle_ProgressBar);
				const float TimeRemaining = GetWorld()->GetTimerManager().GetTimerRemaining(*TimerHandle_ProgressBar);

				FillPercentage = TimeRemaining / (TimeRemaining + TimeElapsed);

				bIsCooldownTextEmpty = false;
				CooldownRemainingText->SetText(GetDurationRemainingText(TimeRemaining));
			}
			else
			{
				/* Adding this check here to prevent setting text each frame and invalidating 
				layout/volatility */
				if (!bIsCooldownTextEmpty)
				{
					bIsCooldownTextEmpty = true;
					CooldownRemainingText->SetText(FText::GetEmpty());
				}
			}
		}
	}

	CooldownProgressBar->SetPercent(FillPercentage);
}

void UContextActionButton::Disable()
{
	SetVisibility(HUDStatics::HIDDEN_VISIBILITY);
}

bool UContextActionButton::ArePrequisitesMet() const
{
	return bPrerequisitesMet;
}

const FContextButton * UContextActionButton::GetButtonType() const
{
	return ButtonType;
}

void UContextActionButton::OnPrerequisitesMet()
{
	bPrerequisitesMet = true;

	// Change alpha 
	SetRenderOpacity(OriginalAlpha);
}

void UContextActionButton::OnPrerequisiteLost(float NewAlpha)
{
	bPrerequisitesMet = false;

	// Change alpha
	SetRenderOpacity(NewAlpha);
}

void UContextActionButton::OnUpgradeResearchComplete()
{
	SetAppearanceForFullyResearchedUpgrade();
}

#if WITH_EDITOR
const FText UContextActionButton::GetPaletteCategory()
{
	return URTSHUD::PALETTE_CATEGORY;
}
#endif


//===============================================================================================
//	Garrison Garrisoned Unit Info Widget
//===============================================================================================

bool UGarrisonedUnitInfo::SetupWidget(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController)
{
	if (Super::SetupWidget(InGameInstance, InPlayerController))
	{
		return true;
	}

	if (IsWidgetBound(Button_IconAndUnload))
	{
		Button_IconAndUnload->SetPC(InPlayerController);
		Button_IconAndUnload->SetOwningWidget();
		Button_IconAndUnload->SetPurpose(EUIElementType::NoTooltipRequired, nullptr);
		Button_IconAndUnload->GetOnLeftMouseButtonPressDelegate().BindUObject(this, &UGarrisonedUnitInfo::UIBinding_OnUnloadButtonLMBPressed);
		Button_IconAndUnload->GetOnLeftMouseButtonReleasedDelegate().BindUObject(this, &UGarrisonedUnitInfo::UIBinding_OnUnloadButtonLMBReleased);
		Button_IconAndUnload->GetOnRightMouseButtonPressDelegate().BindUObject(this, &UGarrisonedUnitInfo::UIBinding_OnUnloadButtonRMBPressed);
		Button_IconAndUnload->GetOnRightMouseButtonReleasedDelegate().BindUObject(this, &UGarrisonedUnitInfo::UIBinding_OnUnloadButtonRMBReleased);
		// TODO Unified images/sounds?
	}

	return false;
}

void UGarrisonedUnitInfo::SetAppearanceFor(AActor * Selectable)
{
	GarrisonedSelectable = Selectable;
	ISelectable * AsSelectable = CastChecked<ISelectable>(Selectable);

	if (IsWidgetBound(Button_IconAndUnload))
	{
		Button_IconAndUnload->SetImages_ExcludeHighlightedImageAndIgnoreUnifiedFlags(&AsSelectable->GetAttributesBase().GetHUDImage_Normal(),
			&AsSelectable->GetAttributesBase().GetHUDImage_Hovered(),
			&AsSelectable->GetAttributesBase().GetHUDImage_Pressed());
		// Sounds too?
	}
}

void UGarrisonedUnitInfo::UIBinding_OnUnloadButtonLMBPressed()
{
	PC->OnLMBPressed_UnloadSingleGarrisonUnit(Button_IconAndUnload);
}

void UGarrisonedUnitInfo::UIBinding_OnUnloadButtonLMBReleased()
{
	PC->OnLMBReleased_UnloadSingleGarrisonUnit(Button_IconAndUnload, this);
}

void UGarrisonedUnitInfo::UIBinding_OnUnloadButtonRMBPressed()
{
	PC->OnRMBPressed_UnloadSingleGarrisonUnit(Button_IconAndUnload);
}

void UGarrisonedUnitInfo::UIBinding_OnUnloadButtonRMBReleased()
{
	PC->OnRMBReleased_UnloadSingleGarrisonUnit(Button_IconAndUnload);
}

void UGarrisonedUnitInfo::OnUnloadButtonClicked(ARTSPlayerController * PlayCon)
{
	if (GetWorld()->IsServer())
	{
		PlayCon->ServerUnloadSingleUnitFromGarrison(CastChecked<ABuilding>(PlayCon->GetCurrentSelected().GetObject()), GarrisonedSelectable);
	}
	else
	{
		PlayCon->Server_RequestUnloadSingleUnitFromGarrison(CastChecked<ABuilding>(PlayCon->GetCurrentSelected().GetObject()), GarrisonedSelectable);
	}
}


//===============================================================================================
//	Garrison Info Widget
//===============================================================================================

bool UGarrisonInfo::SetupWidget(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController)
{
	if (Super::SetupWidget(InGameInstance, InPlayerController))
	{
		return true;
	}

	UE_CLOG(SingleUnitInfoWidget_BP == nullptr, RTSLOG, Fatal, TEXT("[%s]: SingleUnitInfoWidget_BP is null"),
		*GetClass()->GetName());

	/* Create single unit info widgets. Create enough for worst case which is that 1 slot = 1 unit 
	(e.g. bunker in SCII has 4 marines in it as opposed to only putting 2 marauders in it) even 
	though worst case may never happen. Why am I so aggressive with avoiding creating widgets 
	during matches? */
	for (int32 i = 0; i < FI->GetLargestBuildingGarrisonSlotCapacity(); ++i)
	{
		UGarrisonedUnitInfo * Widget = CreateWidget<UGarrisonedUnitInfo>(PC, SingleUnitInfoWidget_BP);
		Panel_Units->AddChild(Widget);
		Widget->SetupWidget(InGameInstance, InPlayerController);
	}

	/* Setup the unload button */
	if (IsWidgetBound(Button_UnloadGarrison))
	{
		Button_UnloadGarrison->SetPC(InPlayerController);
		Button_UnloadGarrison->SetOwningWidget();
		Button_UnloadGarrison->SetPurpose(EUIElementType::NoTooltipRequired, nullptr);
		Button_UnloadGarrison->GetOnLeftMouseButtonPressDelegate().BindUObject(this, &UGarrisonInfo::UIBinding_OnUnloadGarrisonButtonLeftMousePress);
		Button_UnloadGarrison->GetOnLeftMouseButtonReleasedDelegate().BindUObject(this, &UGarrisonInfo::UIBinding_OnUnloadGarrisonButtonLeftMouseReleased);
		Button_UnloadGarrison->GetOnRightMouseButtonPressDelegate().BindUObject(this, &UGarrisonInfo::UIBinding_OnUnloadGarrisonButtonRightMousePress);
		Button_UnloadGarrison->GetOnRightMouseButtonReleasedDelegate().BindUObject(this, &UGarrisonInfo::UIBinding_OnUnloadGarrisonButtonRightMouseReleased);
		// TODO Unified images/sounds?
	}

	/* Start invis cause the player probably has no selection */
	SetVisibility(ESlateVisibility::Hidden);

	return false;
}

void UGarrisonInfo::OnPlayerPrimarySelectedChanged(const TScriptInterface<ISelectable>& NewPrimarySelected)
{
	CurrentSelected = NewPrimarySelected;
	PrimarySelectedsGarrisonNetworkType = NewPrimarySelected->GetBuildingAttributes() != nullptr ? NewPrimarySelected->GetBuildingAttributes()->GetGarrisonAttributes().GetGarrisonNetworkType() : EBuildingNetworkType::None;

	/* Check if the affiliation of the selected lets us see inside the garrison. 
	98% sure I haven't taken care of this already */
	const EAffiliation PrimarySelectedsAffiliation = NewPrimarySelected->GetAttributesBase().GetAffiliation();
	if (PrimarySelectedsAffiliation <= GarrisonOptions::HighestAffiliationAllowedToSeeInsideOf)
	{
		const FBuildingAttributes * BuildingAttributes = NewPrimarySelected->GetBuildingAttributes();
		if (BuildingAttributes != nullptr) 
		{
			if (BuildingAttributes->GetGarrisonAttributes().IsPartOfGarrisonNetwork() 
				|| BuildingAttributes->GetGarrisonAttributes().GetTotalNumGarrisonSlots() > 0)
			{
				const TArray<TRawPtr(AActor)> & GarrisonedUnitsContainer = BuildingAttributes->GetGarrisonAttributes().GetGarrisonedUnitsContainerTakingIntoAccountNetworkType();
				for (int32 i = 0; i < GarrisonedUnitsContainer.Num(); ++i)
				{
					UGarrisonedUnitInfo * Widget = CastChecked<UGarrisonedUnitInfo>(Panel_Units->GetChildAt(i));
					Widget->SetAppearanceFor(GarrisonedUnitsContainer[i].Get());
					Widget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
				}

				/* Make remaining slot widgets hidden */
				for (int32 i = GarrisonedUnitsContainer.Num(); i < Panel_Units->GetChildrenCount(); ++i)
				{
					Panel_Units->GetChildAt(i)->SetVisibility(ESlateVisibility::Hidden);
				}

				/* Set unload all button visibility */
				if (PrimarySelectedsAffiliation == EAffiliation::Owned)
				{
					Button_UnloadGarrison->SetVisibility(ESlateVisibility::Visible);
				}
				else
				{
					Button_UnloadGarrison->SetVisibility(ESlateVisibility::Hidden);
				}

				SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			}
			else
			{
				SetVisibility(ESlateVisibility::Hidden);
			}
		}
		else
		{
			SetVisibility(ESlateVisibility::Hidden);
		}
	}
	else /* Primary selected's affiliation means we're not allowed to see what's inside garrison */
	{
		SetVisibility(ESlateVisibility::Hidden);
	}
}

void UGarrisonInfo::OnPlayerNoSelection()
{
	CurrentSelected = nullptr;
	SetVisibility(ESlateVisibility::Hidden);
}

void UGarrisonInfo::OnBuildingGarrisonedUnitGained(ABuilding * Building, AInfantry * GainedUnit, int32 ContainerIndex)
{
	if (CurrentSelected != nullptr)
	{
		/* Check affiliation of primary selected */
		if (CurrentSelected->GetAttributesBase().GetAffiliation() <= GarrisonOptions::HighestAffiliationAllowedToSeeInsideOf)
		{
			/* Check if either the building is
			- the primary selected
			- shares garrison network type with the primary selected */
			const EBuildingNetworkType BuildingsNetworkType = Building->GetBuildingAttributes()->GetGarrisonAttributes().GetGarrisonNetworkType();
			if (Building == CurrentSelected.GetObject() || (BuildingsNetworkType != EBuildingNetworkType::None && BuildingsNetworkType == PrimarySelectedsGarrisonNetworkType))
			{
				UGarrisonedUnitInfo * Widget = CastChecked<UGarrisonedUnitInfo>(Panel_Units->GetChildAt(ContainerIndex));
				Widget->SetAppearanceFor(GainedUnit);
				Widget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			}
		}
	}
}

void UGarrisonInfo::OnBuildingGarrisonedUnitLost(ABuilding * Building, int32 ContainerIndex)
{
	if (CurrentSelected != nullptr)
	{
		/* Check affiliation of primary selected */
		if (CurrentSelected->GetAttributesBase().GetAffiliation() <= GarrisonOptions::HighestAffiliationAllowedToSeeInsideOf)
		{
			/* Check if either the building is
			   - the primary selected
			   - shares garrison network type with the primary selected */
			const EBuildingNetworkType BuildingsNetworkType = Building->GetBuildingAttributes()->GetGarrisonAttributes().GetGarrisonNetworkType();
			if (Building == CurrentSelected.GetObject() || (BuildingsNetworkType != EBuildingNetworkType::None && BuildingsNetworkType == PrimarySelectedsGarrisonNetworkType))
			{ 
				const TArray<TRawPtr(AActor)> & GarrisonedUnitsContainer = Building->GetBuildingAttributes()->GetGarrisonAttributes().GetGarrisonedUnitsContainerTakingIntoAccountNetworkType();
				/* Shuffle some down 1. Understand that GarrisonedUnitsContainer has already had the unit 
				removed from it */
				for (int32 i = ContainerIndex; i < GarrisonedUnitsContainer.Num(); ++i)
				{
					CastChecked<UGarrisonedUnitInfo>(Panel_Units->GetChildAt(i))->SetAppearanceFor(GarrisonedUnitsContainer[i].Get());
				}
				/* Make the last widget hidden */
				CastChecked<UGarrisonedUnitInfo>(Panel_Units->GetChildAt(GarrisonedUnitsContainer.Num()))->SetVisibility(ESlateVisibility::Hidden);
			}
		}
	}
}

void UGarrisonInfo::OnBuildingPotentiallyManyGarrisonedUnitsLost(ABuilding * Building, const TArray<TRawPtr(AActor)>& GarrisonedUnitsContainer)
{
	if (CurrentSelected != nullptr)
	{
		/* Check affiliation of primary selected */
		if (CurrentSelected->GetAttributesBase().GetAffiliation() <= GarrisonOptions::HighestAffiliationAllowedToSeeInsideOf)
		{
			/* Check if either the building is
			   - the primary selected
			   - shares garrison network type with the primary selected */
			const EBuildingNetworkType BuildingsNetworkType = Building->GetBuildingAttributes()->GetGarrisonAttributes().GetGarrisonNetworkType();
			if (Building == CurrentSelected.GetObject() || (BuildingsNetworkType != EBuildingNetworkType::None && BuildingsNetworkType == PrimarySelectedsGarrisonNetworkType))
			{
				for (int32 i = 0; i < GarrisonedUnitsContainer.Num(); ++i)
				{
					UGarrisonedUnitInfo * Widget = CastChecked<UGarrisonedUnitInfo>(Panel_Units->GetChildAt(i));
					Widget->SetAppearanceFor(GarrisonedUnitsContainer[i].Get());
					Widget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
				}

				/* Make remaining slot widgets hidden */
				for (int32 i = GarrisonedUnitsContainer.Num(); i < Panel_Units->GetChildrenCount(); ++i)
				{
					Panel_Units->GetChildAt(i)->SetVisibility(ESlateVisibility::Hidden);
				}
			}
		}
	}
}

void UGarrisonInfo::UIBinding_OnUnloadGarrisonButtonLeftMousePress()
{
	PC->OnLMBPressed_UnloadGarrisonButton(Button_UnloadGarrison);
}

void UGarrisonInfo::UIBinding_OnUnloadGarrisonButtonLeftMouseReleased()
{
	PC->OnLMBReleased_UnloadGarrisonButton(Button_UnloadGarrison, this);
}

void UGarrisonInfo::UIBinding_OnUnloadGarrisonButtonRightMousePress()
{
	PC->OnRMBPressed_UnloadGarrisonButton(Button_UnloadGarrison);
}

void UGarrisonInfo::UIBinding_OnUnloadGarrisonButtonRightMouseReleased()
{
	PC->OnRMBReleased_UnloadGarrisonButton(Button_UnloadGarrison);
}

void UGarrisonInfo::OnUnloadGarrisonButtonClicked()
{
	if (GetWorld()->IsServer())
	{
		PC->ServerUnloadGarrison(CastChecked<ABuilding>(CurrentSelected.GetObject()));
	}
	else
	{
		PC->Server_RequestUnloadGarrison(CastChecked<ABuilding>(CurrentSelected.GetObject()));
	}
}


//===============================================================================================
//	Resources Widget
//===============================================================================================

bool UHUDResourcesWidget::SetupWidget(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController)
{
	if (Super::SetupWidget(InGameInstance, InPlayerController))
	{
		return true;
	}

	HousingResourceWidgets.Init(nullptr, Statics::NUM_HOUSING_RESOURCE_TYPES);

	TArray<UWidget *> AllChildren;
	WidgetTree->GetAllWidgets(AllChildren);

	for (const auto & Elem : AllChildren)
	{
		if (Elem->IsA(UHUDSingleResourcesWidget::StaticClass()))
		{
			UHUDSingleResourcesWidget * ResourceWidget = CastChecked<UHUDSingleResourcesWidget>(Elem);
			ResourceWidget->SetupWidget(InGameInstance, InPlayerController);

			const EResourceType ResourceType = ResourceWidget->GetResourceType();

			/* If thrown then more than one widget is updating a resource */
			assert(!ResourceWidgets.Contains(ResourceType));

			ResourceWidgets.Emplace(ResourceType, ResourceWidget);
		}
		else if (Elem->IsA(UHUDSingleHousingResourceWidget::StaticClass()))
		{
			UHUDSingleHousingResourceWidget * ResourceWidget = CastChecked<UHUDSingleHousingResourceWidget>(Elem);
			ResourceWidget->SetupResourceWidget();

			const int32 Index = Statics::HousingResourceTypeToArrayIndex(ResourceWidget->GetResourceType());
			
			/* Would be pretty easy to allow this though. Would just need to store array of pointers
			instead of single pointer */
			UE_CLOG(HousingResourceWidgets[Index] != nullptr, RTSLOG, Fatal, TEXT("More than "
				"one housing resource widget for type \"%s\" has been added to resource widget. This is "
				"not allowed"), TO_STRING(EHousingResourceType, ResourceWidget->GetResourceType()));
			
			HousingResourceWidgets[Index] = ResourceWidget;
		}
	}

	return false;
}

void UHUDResourcesWidget::OnTick(float InDeltaTime)
{
	// Iterating over hashmap here. Good idea to change it to an array
	for (const auto & Elem : ResourceWidgets)
	{
		Elem.Value->OnTick(InDeltaTime);
	}
}

void UHUDResourcesWidget::SetInitialResourceAmounts(const TArray<int32>& InInitialValueArray, 
	const TArray < FHousingResourceState > & InInitialHousingProvided)
{
	for (int32 i = 0; i < InInitialValueArray.Num(); ++i)
	{
		const EResourceType Type = Statics::ArrayIndexToResourceType(i);

#if WITH_EDITOR

		/* While with editor we allow widgets to be missing */
		if (ResourceWidgets.Contains(Type))
		{
			ResourceWidgets[Type]->SetInitialValue(InInitialValueArray[i]);
		}
		else
		{
			UE_LOG(RTSLOG, Warning, TEXT("No single resource widget for \"%s\" is present on HUD "
				"resource widget. This will need to be resolved in order for packaged game to not crash"),
				TO_STRING(EResourceType, Type));
		}

#else
		
		ResourceWidgets[Type]->SetInitialValue(InInitialValueArray[i]);

#endif // !WITH_EDITOR
	}

	for (int32 i = 0; i < InInitialHousingProvided.Num(); ++i)
	{
		UHUDSingleHousingResourceWidget * Widget = HousingResourceWidgets[i];

#if WITH_EDITOR

		/* While with editor we allow widgets to be missing */
		if (Widget != nullptr)
		{
			Widget->SetInitialValue(InInitialHousingProvided[i].GetAmountConsumed(), InInitialHousingProvided[i].GetAmountProvidedClamped());
		}
		else
		{
			UE_LOG(RTSLOG, Warning, TEXT("No single housing resource widget for resource \"%s\" is present on HUD "
				"resource widget. This will need to be resolved in order for packaged game to not crash"),
				TO_STRING(EHousingResourceType, Statics::ArrayIndexToHousingResourceType(i)));
		}

#else

		Widget->SetInitialValue(InInitialHousingProvided[i].GetAmountConsumed(), InInitialHousingProvided[i].GetAmountProvidedClamped());

#endif // !WITH_EDITOR
	}
}

void UHUDResourcesWidget::OnResourcesChanged(EResourceType ResourceType, int32 PreviousAmount, int32 NewAmount)
{
#if WITH_EDITOR

	if (ResourceWidgets.Contains(ResourceType))
	{
		UHUDSingleResourcesWidget * SingleWidget = ResourceWidgets[ResourceType];
		SingleWidget->OnResourcesChanged(PreviousAmount, NewAmount);
	}

#else

	UHUDSingleResourcesWidget * SingleWidget = ResourceWidgets[ResourceType];
	SingleWidget->OnResourcesChanged(PreviousAmount, NewAmount);

#endif // !WITH_EDITOR
}

void UHUDResourcesWidget::OnHousingResourceConsumptionChanged(const TArray<FHousingResourceState>& HousingResourceArray)
{
	assert(HousingResourceArray.Num() == Statics::NUM_HOUSING_RESOURCE_TYPES);
	
	/* Pretty inefficent that we update ALL single resource widgets, should probably single out 
	which resource changed */

	for (uint8 i = 0; i < Statics::NUM_HOUSING_RESOURCE_TYPES; ++i)
	{
		UHUDSingleHousingResourceWidget * Widget = HousingResourceWidgets[i];

#if WITH_EDITOR
		if (Widget == nullptr)
		{
			continue;
		}
#endif

		Widget->OnAmountConsumedChanged(HousingResourceArray[i].GetAmountConsumed(), 
			HousingResourceArray[i].GetAmountProvidedClamped());
	}
}

void UHUDResourcesWidget::OnHousingResourceProvisionsChanged(const TArray<FHousingResourceState>& HousingResourceArray)
{
	assert(HousingResourceArray.Num() == Statics::NUM_HOUSING_RESOURCE_TYPES);
	
	/* Pretty inefficent that we update ALL single resource widgets, should probably single out
	which resource changed */

	for (uint8 i = 0; i < Statics::NUM_HOUSING_RESOURCE_TYPES; ++i)
	{
		UHUDSingleHousingResourceWidget * Widget = HousingResourceWidgets[i];

#if WITH_EDITOR
		if (Widget == nullptr)
		{
			continue;
		}
#endif

		Widget->OnAmountProvidedChanged(HousingResourceArray[i].GetAmountConsumed(), 
			HousingResourceArray[i].GetAmountProvidedClamped());
	}
}


//==============================================================================================
//	Single Resources Widget
//==============================================================================================

bool UHUDSingleResourcesWidget::SetupWidget(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController)
{
	if (Super::SetupWidget(InGameInstance, InPlayerController))
	{
		return true;
	}

	CheckUpdateCurve();

	/* Set image */
	if (IsWidgetBound(Image_Icon))
	{
		UTexture2D * Image = FI->HasOverriddenResourceImage(ResourceType)
			? FI->GetResourceImage(ResourceType) : GI->GetResourceInfo(ResourceType).GetIcon();

		Image_Icon->SetBrushFromTexture(Image);
	}

	return false;
}

void UHUDSingleResourcesWidget::OnTick(float InDeltaTime)
{
	/* Gradually change displayed value based on curve. If statement only true when UpdateCurve
	is set */
	if (CurrentAmount != TargetAmount)
	{
		CurrentCurveTime += InDeltaTime;

		float TimelinePercentThrough = CurrentCurveTime / UpdateCurveDuration;
		if (TimelinePercentThrough > 1.f)
		{
			TimelinePercentThrough = 1.f;
		}

		/* Rounded to nearest integer. Some casting ints to float here */
		const int32 ToShow = FMath::RoundHalfFromZero((float)Range * UpdateCurve->GetFloatValue(CurrentCurveTime) + (float)StartAmount);

		UpdateResourceDisplayAmount(ToShow, true);
	}
}

void UHUDSingleResourcesWidget::CheckUpdateCurve()
{
	if (UpdateCurve != nullptr)
	{
		float MinX, MaxX, MinY, MaxY;
		UpdateCurve->GetTimeRange(MinX, MaxX);
		UpdateCurve->GetValueRange(MinY, MaxY);

		/* If curve either has no points or is a single vertical/horizontal line then do not use it */
		if (MinX == MaxX || MinY == MaxY)
		{
			UE_LOG(RTSLOG, Warning, TEXT("Float curve %s on widget %s either has no points or is a "
				"single horizontal/vertical line and therefore will not be used"), 
				*UpdateCurve->GetName(), *GetName());
			
			UpdateCurve = nullptr;
		}
		else
		{
			UpdateCurveDuration = MaxX - MinX;
		}
	}
}

void UHUDSingleResourcesWidget::UpdateResourceDisplayAmount(int32 NewAmount, bool bUsingFloatCurve)
{
	if (IsWidgetBound(Text_Amount))
	{
		Text_Amount->SetText(FText::AsNumber(NewAmount));
		CurrentAmount = NewAmount;

		/* This stops tick from trying to update display when not using curve */
		if (!bUsingFloatCurve)
		{
			TargetAmount = NewAmount;
		}
	}
}

EResourceType UHUDSingleResourcesWidget::GetResourceType() const
{
	return ResourceType;
}

void UHUDSingleResourcesWidget::SetInitialValue(int32 InitialValue)
{
	UpdateResourceDisplayAmount(InitialValue, false);
}

void UHUDSingleResourcesWidget::OnResourcesChanged(int32 PreviousAmount, int32 NewAmount)
{
	/* If curve is set make display gradually change to new value */
	if (UpdateCurve != nullptr)
	{
		/* Use the actual previous amount and not the one currently displayed because using the
		displayed amount can make some changes really slow */

		/* Cast to float so result is a float */
		float PercentageToStartAt = (float)(PreviousAmount - CurrentAmount) / (PreviousAmount - NewAmount);

		if (PercentageToStartAt < 0.f)
		{
			PercentageToStartAt = 0.f;
			StartAmount = CurrentAmount;
		}
		else
		{
			StartAmount = PreviousAmount;
		}

		CurrentCurveTime = PercentageToStartAt * UpdateCurveDuration;

		Range = NewAmount - StartAmount;

		TargetAmount = NewAmount;
	}
	else
	{
		UpdateResourceDisplayAmount(NewAmount, false);
	}
}


//==============================================================================================
//	Single Housing Resource Widget
//==============================================================================================

UHUDSingleHousingResourceWidget::UHUDSingleHousingResourceWidget(const FObjectInitializer & ObjectInitializer)
	: Super(ObjectInitializer)
{
	Consumed_OverCapColor = FLinearColor::Red;
	Provided_OverCapColor = Consumed_OverCapColor;
}

void UHUDSingleHousingResourceWidget::SetupResourceWidget()
{
	// Cache the text widget's colors
	if (IsWidgetBound(Text_AmountConsumed))
	{
		Consumed_OriginalColor = Text_AmountConsumed->ColorAndOpacity.GetSpecifiedColor();
	}
	if (IsWidgetBound(Text_AmountProvided))
	{
		Provided_OriginalColor = Text_AmountProvided->ColorAndOpacity.GetSpecifiedColor();
	}
}

void UHUDSingleHousingResourceWidget::CheckIfStateHasChanged(int32 AmountConsumed, int32 AmountProvided)
{
	if (!bIsOverCap && (AmountConsumed > AmountProvided))
	{
		bIsOverCap = true;
		OnGoneOverCap();
	}
	else if (bIsOverCap && (AmountConsumed <= AmountProvided))
	{
		bIsOverCap = false;
		OnReturnedToWithinCap();
	}
}

void UHUDSingleHousingResourceWidget::OnGoneOverCap()
{
	/* Change the color of the texts */
	if (IsWidgetBound(Text_AmountConsumed))
	{
		Text_AmountConsumed->SetColorAndOpacity(Consumed_OverCapColor);
	}
	if (IsWidgetBound(Text_AmountProvided))
	{
		Text_AmountProvided->SetColorAndOpacity(Provided_OverCapColor);
	}
}

void UHUDSingleHousingResourceWidget::OnReturnedToWithinCap()
{
	/* Change color of texts back to their original color */
	if (IsWidgetBound(Text_AmountConsumed))
	{
		Text_AmountConsumed->SetColorAndOpacity(Consumed_OriginalColor);
	}
	if (IsWidgetBound(Text_AmountProvided))
	{
		Text_AmountProvided->SetColorAndOpacity(Provided_OriginalColor);
	}
}

EHousingResourceType UHUDSingleHousingResourceWidget::GetResourceType() const
{
	return ResourceType;
}

void UHUDSingleHousingResourceWidget::SetInitialValue(int32 InitialAmountConsumed, int32 InitialAmountProvided)
{
	if (IsWidgetBound(Text_AmountConsumed))
	{
		Text_AmountConsumed->SetText(FText::AsNumber(InitialAmountConsumed));
	}

	if (IsWidgetBound(Text_AmountProvided))
	{
		Text_AmountProvided->SetText(FText::AsNumber(InitialAmountProvided));
	}

	/* Check in case user wants some factions to start with negative amount provided */
	CheckIfStateHasChanged(InitialAmountConsumed, InitialAmountProvided);
}

void UHUDSingleHousingResourceWidget::OnAmountConsumedChanged(int32 NewAmountConsumed, int32 AmountProvided)
{
	if (IsWidgetBound(Text_AmountConsumed))
	{
		Text_AmountConsumed->SetText(FText::AsNumber(NewAmountConsumed));
	}

	CheckIfStateHasChanged(NewAmountConsumed, AmountProvided);
}

void UHUDSingleHousingResourceWidget::OnAmountProvidedChanged(int32 AmountConsumed, int32 NewAmountProvided)
{
	if (IsWidgetBound(Text_AmountProvided))
	{
		Text_AmountProvided->SetText(FText::AsNumber(NewAmountProvided));
	}

	CheckIfStateHasChanged(AmountConsumed, NewAmountProvided);
}


//-------------------------------------------------
//	Chat Input Widget
//-------------------------------------------------

UHUDChatInputWidget::UHUDChatInputWidget(const FObjectInitializer & ObjectInitializer)
	: Super(ObjectInitializer)
{
	for (uint8 i = 0; i < Statics::NUM_MESSAGE_RECIPIENT_TYPES; ++i)
	{
		const EMessageRecipients RecipientType = Statics::ArrayIndexToMessageRecipientType(i);

		MessageStarts.Emplace(RecipientType,
			"[" + ENUM_TO_STRING(EMessageRecipients, RecipientType) + "]: ");
	}

	MaxMessageLength = 200;
}

bool UHUDChatInputWidget::SetupWidget(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController)
{
	if (Super::SetupWidget(InGameInstance, InPlayerController))
	{
		return true;
	}

	/* Start hidden */
	SetVisibility(ESlateVisibility::Hidden);

	if (IsWidgetBound(Text_Input))
	{
		Text_Input->SetText(URTSHUD::BLANK_TEXT);
	}

	SetupBindings();

	return false;
}

void UHUDChatInputWidget::SetupBindings()
{
	if (IsWidgetBound(Text_Input))
	{
		Text_Input->OnTextChanged.AddUniqueDynamic(this, &UHUDChatInputWidget::UIBinding_OnInputTextChanged);
		Text_Input->OnTextCommitted.AddUniqueDynamic(this, &UHUDChatInputWidget::UIBinding_OnInputTextCommitted);
	}
}

void UHUDChatInputWidget::RemoveFocus()
{
	// Does nothing
	//UWidgetBlueprintLibrary::SetFocusToGameViewport();

	/* This is a hacky way to restore focus back to game the way I want it which is:
	- allow game input
	- allow UI interaction without having to click somewhere first
	A possibly more performant solution is below */
	//PC->SetInputMode(FInputModeGameOnly());
	//FInputModeGameAndUI InputMode;
	//InputMode.SetHideCursorDuringCapture(false);
	//PC->SetInputMode(InputMode);

	/* Another solution. Most of this is taken from APlayerController::SetInputMode */

	UGameViewportClient* GameViewportClient = GetWorld()->GetGameViewport();
	ULocalPlayer* LocalPlayer = Cast< ULocalPlayer >(PC->Player);
	if (GameViewportClient && LocalPlayer)
	{
		FReply & SlateOperations = LocalPlayer->GetSlateOperations();

		TSharedPtr<SViewport> ViewportWidget = GameViewportClient->GetGameViewportWidget();
		if (ViewportWidget.IsValid())
		{
			TSharedRef<SViewport> ViewportWidgetRef = ViewportWidget.ToSharedRef();

			// This is taken from FInputModeGameOnly
			SlateOperations.SetUserFocus(ViewportWidgetRef);
			//SlateOperations.LockMouseToWidget(ViewportWidgetRef);

			// This is taken from FInputModeGameAndUI, but I hardcoded some params
			GameViewportClient->SetMouseLockMode(EMouseLockMode::DoNotLock);
			GameViewportClient->SetIgnoreInput(false);
			GameViewportClient->SetHideCursorDuringCapture(false);
			GameViewportClient->SetCaptureMouseOnClick(EMouseCaptureMode::CaptureDuringMouseDown);
		}
	}
}

FString UHUDChatInputWidget::GetMessageToSend() const
{
	// RightChop: Do not send the part that user did not type in themself
	return Text_Input->GetText().ToString().RightChop(NumUndeletableCharacters);
}

bool UHUDChatInputWidget::SendMessage()
{
	const FString & ToSend = GetMessageToSend();

	if (ToSend.Len() > 0)
	{
		if (MessageRecipients == EMessageRecipients::Everyone)
		{
			PC->Server_SendInMatchChatMessageToEveryone(ToSend);
		}
		else // Assuminging MessageRecipients == EMessageRecipients::TeamOnly
		{
			assert(MessageRecipients == EMessageRecipients::TeamOnly);

			PC->Server_SendInMatchChatMessageToTeam(ToSend);
		}

		return true;
	}

	return false;
}

void UHUDChatInputWidget::UIBinding_OnInputTextChanged(const FText & NewText)
{
	// Need to check if user has removed characters they were not ment to and restore them if so
	FString ToShow = NewText.ToString();
	if (ToShow.Len() < NumUndeletableCharacters)
	{
		ToShow = MessageStarts[MessageRecipients];
	}
	// Limit message length
	else if (ToShow.Len() - NumUndeletableCharacters > MaxMessageLength)
	{
		ToShow = ToShow.Left(NumUndeletableCharacters + MaxMessageLength);
	}

	Text_Input->SetText(FText::FromString(ToShow));
}

void UHUDChatInputWidget::UIBinding_OnInputTextCommitted(const FText & Text, ETextCommit::Type CommitMethod)
{
	// Take focus off this widget
	RemoveFocus();

	if (CommitMethod == ETextCommit::OnEnter)
	{
		// Send message and hide this widget 
		SendChatMessageAndClose();
	}
	else if (CommitMethod == ETextCommit::OnCleared)
	{
		/* Hide widget. This will do nothing when testing with standalone because the "P"
		key is what I use instead of ESC, and it may still do nothing in packaged game will see */
		Close();
	}
}

void UHUDChatInputWidget::Open(EMessageRecipients InMessageRecipients)
{
	// Open with the correct message start text

	MessageRecipients = InMessageRecipients;

	// Remember how many at start should never be deleted
	NumUndeletableCharacters = MessageStarts[InMessageRecipients].Len();

	SetVisibility(ESlateVisibility::Visible);

	Text_Input->SetText(FText::FromString(MessageStarts[MessageRecipients]));

	// Set focus on input box
	Text_Input->SetKeyboardFocus();
}

void UHUDChatInputWidget::Close()
{
	SetVisibility(ESlateVisibility::Hidden);
}

bool UHUDChatInputWidget::RespondToEscapeRequest(URTSHUD * HUD)
{
	Close();
	return true;
}

void UHUDChatInputWidget::SendChatMessageAndClose()
{
	SendMessage();
	Close();
}


//####################################################
//----------------------------------------------------
//	Chat output
//----------------------------------------------------
//####################################################

// Don't need the '_INST' part in editor
const FName UHUDChatOutputWidget::MessageReceivedAnimName = FName("MessageReceivedAnim_INST");

UHUDChatOutputWidget::UHUDChatOutputWidget(const FObjectInitializer & ObjectInitializer)
	: Super(ObjectInitializer)
{
	ShowDuration = 10.f;
	NumMaxMessages = 20;
	MessageStarts.Emplace(EMessageRecipients::Everyone, "[");
	MessageStarts.Emplace(EMessageRecipients::TeamOnly, "[TEAM][");
	MessageMiddle = "]: ";
}

bool UHUDChatOutputWidget::SetupWidget(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController)
{
	if (Super::SetupWidget(InGameInstance, InPlayerController))
	{
		return true;
	}

	MessageLengths.Init(-1, NumMaxMessages);

	SetVisibility(ESlateVisibility::Hidden);

	if (IsWidgetBound(Text_Output))
	{
		Text_Output->SetIsReadOnly(true);
		Text_Output->SetText(URTSHUD::BLANK_TEXT);
	}

	OnMessageReceivedAnim = UIUtilities::FindWidgetAnim(this, MessageReceivedAnimName);

	return false;
}

void UHUDChatOutputWidget::NativeTick(const FGeometry & MyGeometry, float InDeltaTime)
{
	// Call UUserWidget version not Super to allow for widget anim ticking 
	UUserWidget::NativeTick(MyGeometry, InDeltaTime);
}

int32 UHUDChatOutputWidget::GetNextArrayIndex(int32 CurrentIndex) const
{
	return (CurrentIndex + 1) % NumMaxMessages;
}

void UHUDChatOutputWidget::HideWidget()
{
	SetVisibility(ESlateVisibility::Hidden);
}

void UHUDChatOutputWidget::Delay(FTimerHandle & TimerHandle, void(UHUDChatOutputWidget::* Function)(), float Delay)
{
	assert(Delay > 0.f);
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, Function, Delay, false);
}

void UHUDChatOutputWidget::OnChatMessageReceived(const FString & SendersName, const FString & Message,
	EMessageRecipients MessageType)
{
	if (IsWidgetBound(Text_Output))
	{
		// Create message with senders name tagged on
		const FString & FullNewMessage = MessageStarts[MessageType] + SendersName + MessageMiddle + Message;

		FString ToShow = Text_Output->GetText().ToString();

		NumMessages++;
		if (NumMessages > NumMaxMessages)
		{
			// Delete oldest message

			const int32 IndexInStringToChopAt = MessageLengths[FrontOfMessageIndices];

			// Default initialized value; should not be referencing it
			assert(IndexInStringToChopAt != -1);

			// +1 is to take into account newline char... seems to work
			ToShow = ToShow.RightChop(IndexInStringToChopAt + 1);

			NumMessages--;
		}

		// Put new entry in array
		MessageLengths[FrontOfMessageIndices] = FullNewMessage.Len();

		if (NumMessages > 1)
		{
			// This is not first/only message so append a newline char before message
			ToShow += "\n";
		}

		FrontOfMessageIndices = GetNextArrayIndex(FrontOfMessageIndices);

		ToShow += FullNewMessage;

		Text_Output->SetText(FText::FromString(ToShow));
	}

	if (OnMessageReceivedAnim != nullptr)
	{
		float StartTime = GetAnimationCurrentTime(OnMessageReceivedAnim);

		if (StartTime > AdditionalMessageAnimStartTime)
		{
			StartTime = AdditionalMessageAnimStartTime;
		}

		// Anim's responsibility to set visibility both initial vis and then hide later
		PlayAnimation(OnMessageReceivedAnim, StartTime);
	}
	else
	{
		// HitTestInvisible not Visible otherwise when hovering over box mouse cursor will change
		SetVisibility(ESlateVisibility::HitTestInvisible);

		// Hide widget after a while
		Delay(TimerHandle_Hide, &UHUDChatOutputWidget::HideWidget, ShowDuration);
	}
}


//####################################################
//----------------------------------------------------
//	Game output
//----------------------------------------------------
//####################################################

//======================================================
//	Message attributes
//======================================================

int32 FGameMessageAttributes::GetNextArrayIndex(int32 CurrentIndex) const
{
	return (CurrentIndex + 1) % MaxNumMessages;
}

int32 FGameMessageAttributes::GetPreviousArrayIndex(int32 CurrentIndex) const
{
	return (CurrentIndex - 1 + MaxNumMessages) % MaxNumMessages;
}

void FGameMessageAttributes::SetMessageReceivedAnimName(const FName & InName)
{
	MessageReceivedAnimName = InName;
}

void FGameMessageAttributes::Init(UHUDGameOutputWidget * InWidget, FunctionPtrType InFunctionPtr)
{
	MessageLengths.Init(-1, MaxNumMessages);

	OwningWidget = InWidget;
	HidePanelFunctionPtr = InFunctionPtr;

	MessageReceivedAnim = UIUtilities::FindWidgetAnim(OwningWidget, *FString(MessageReceivedAnimName.ToString() + "_INST"));

	if (bNewMessagesAboveOlder)
	{
		FrontOfMessageIndices = MaxNumMessages - 1;
	}
	else
	{
		FrontOfMessageIndices = 0;
	}
}

void FGameMessageAttributes::OnMessageReceived(const FString & Message, UPanelWidget * PanelWidget,
	UMultiLineEditableText * TextWidget)
{
	NumMessages++;

	FString ToShow = TextWidget->GetText().ToString();

	// Update text
	if (bNewMessagesAboveOlder)
	{
		if (NumMessages > MaxNumMessages)
		{
			const int32 IndexInStringToChopAt = MessageLengths[FrontOfMessageIndices];

			// -1 = default initialized value; should not be referencing it
			assert(IndexInStringToChopAt != -1);

			ToShow = ToShow.LeftChop(IndexInStringToChopAt);

			NumMessages--;
		}

		int32 MessageLength;

		// Add on new message and add newline char in between if not only message
		if (NumMessages > 1)
		{
			ToShow = Message + "\n" + ToShow;
			MessageLength = Message.Len() + 1;
		}
		else
		{
			ToShow = Message + ToShow;
			MessageLength = Message.Len();
		}

		// Store new message length in array
		MessageLengths[FrontOfMessageIndices] = MessageLength;

		TextWidget->SetText(FText::FromString(ToShow));

		// Play anim or show message for certain amount of time
		if (MessageReceivedAnim != nullptr)
		{
			float StartTime = OwningWidget->GetAnimationCurrentTime(MessageReceivedAnim);

			if (StartTime > AdditionalMessageAnimStartTime)
			{
				StartTime = AdditionalMessageAnimStartTime;
			}

			// Anim's responsibility to set visibility both initial vis and then hide later
			OwningWidget->PlayAnimation(MessageReceivedAnim, StartTime);
		}
		else
		{
			PanelWidget->SetVisibility(ESlateVisibility::HitTestInvisible);

			// Hide panel widget after a while
			OwningWidget->Delay(TimerHandle_HidePanel, HidePanelFunctionPtr, ShowDuration);
		}
	}
	else
	{
		if (NumMessages > MaxNumMessages)
		{
			const int32 IndexInStringToChopAt = MessageLengths[FrontOfMessageIndices];

			// Default initialized value; should not be referencing it
			assert(IndexInStringToChopAt != -1);

			// +1 is to take into account newline char... seems to work
			ToShow = ToShow.RightChop(IndexInStringToChopAt + 1);

			NumMessages--;
		}

		// Store new message length in array
		MessageLengths[FrontOfMessageIndices] = Message.Len();

		// Update "linked list" front index reference
		FrontOfMessageIndices = GetNextArrayIndex(FrontOfMessageIndices);

		if (NumMessages > 1)
		{
			// This is not first/only message so append a newline char before message
			ToShow += "\n";
		}

		ToShow += Message;

		TextWidget->SetText(FText::FromString(ToShow));

		// Play anim or show message for certain amount of time
		if (MessageReceivedAnim != nullptr)
		{
			float StartTime = OwningWidget->GetAnimationCurrentTime(MessageReceivedAnim);

			if (StartTime > AdditionalMessageAnimStartTime)
			{
				StartTime = AdditionalMessageAnimStartTime;
			}

			// Anim's responsibility to set visibility both initial vis and then hide later
			OwningWidget->PlayAnimation(MessageReceivedAnim, StartTime);
		}
		else
		{
			PanelWidget->SetVisibility(ESlateVisibility::HitTestInvisible);

			// Hide panel widget after a while
			OwningWidget->Delay(TimerHandle_HidePanel, HidePanelFunctionPtr, ShowDuration);
		}
	}
}

void FGameMessageAttributes::ClearText(UMultiLineEditableText * TextWidget)
{
	TextWidget->SetText(URTSHUD::BLANK_TEXT);

	// Update state variables to reflect no text

	NumMessages = 0;

	if (bNewMessagesAboveOlder)
	{
		// Assignment not necessary - should always equal this
		//FrontOfMessageIndices = MaxNumMessages - 1;
	}
	else
	{
		FrontOfMessageIndices = 0;
	}
}

UHUDGameOutputWidget::UHUDGameOutputWidget(const FObjectInitializer & ObjectInitializer)
	: Super(ObjectInitializer)
{
	WarningAttributes.SetMessageReceivedAnimName(FName("WarningMessageReceivedAnim"));
	NotificationAttributes.SetMessageReceivedAnimName(FName("NotificationMessageReceivedAnim"));
}

bool UHUDGameOutputWidget::SetupWidget(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController)
{
	if (Super::SetupWidget(InGameInstance, InPlayerController))
	{
		return true;
	}

	/* Wouldn't be too hard to allow just a text widget and no panel if that is what user wants */
	if (IsWidgetBound(Panel_Warnings) && IsWidgetBound(Text_Warnings))
	{
		WarningAttributes.Init(this, &UHUDGameOutputWidget::HideWarningPanel);

		Panel_Warnings->SetVisibility(ESlateVisibility::Hidden);

		Text_Warnings->SetIsReadOnly(true);
		Text_Warnings->SetText(URTSHUD::BLANK_TEXT);
	}
	if (IsWidgetBound(Panel_Notifications) && IsWidgetBound(Text_Notifications))
	{
		NotificationAttributes.Init(this, &UHUDGameOutputWidget::HideNotificationPanel);

		Panel_Notifications->SetVisibility(ESlateVisibility::Hidden);

		Text_Notifications->SetIsReadOnly(true);
		Text_Notifications->SetText(URTSHUD::BLANK_TEXT);
	}

	// Set Z order
	CastChecked<UCanvasPanelSlot>(Slot)->SetZOrder(HUDStatics::GAME_OUTPUT_Z_ORDER);
	
	return false;
}

void UHUDGameOutputWidget::NativeTick(const FGeometry & MyGeometry, float InDeltaTime)
{
	// Call UUserWidget version not Super to allow for widget anim ticking 
	UUserWidget::NativeTick(MyGeometry, InDeltaTime);
}

void UHUDGameOutputWidget::HideWarningPanel()
{
	Panel_Warnings->SetVisibility(ESlateVisibility::Hidden);
	WarningAttributes.ClearText(Text_Warnings);
}

void UHUDGameOutputWidget::HideNotificationPanel()
{
	Panel_Notifications->SetVisibility(ESlateVisibility::Hidden);
	NotificationAttributes.ClearText(Text_Notifications);
}

void UHUDGameOutputWidget::OnGameWarningReceived(const FString & Message)
{
	if (IsWidgetBound(Panel_Warnings) && IsWidgetBound(Text_Warnings))
	{
		WarningAttributes.OnMessageReceived(Message, Panel_Warnings, Text_Warnings);
	}
}

void UHUDGameOutputWidget::OnGameNotificationReceived(const FString & Message)
{
	if (IsWidgetBound(Panel_Notifications) && IsWidgetBound(Text_Notifications))
	{
		NotificationAttributes.OnMessageReceived(Message, Panel_Notifications, Text_Notifications);
	}
}

void UHUDGameOutputWidget::Delay(FTimerHandle & TimerHandle, void(UHUDGameOutputWidget::* Function)(), float Delay)
{
	assert(Delay > 0.f);
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, Function, Delay, false);
}


//==============================================================================================
//----------------------------------------------------------------------------------------------
//	C&C like Persistent Panel
//----------------------------------------------------------------------------------------------
//==============================================================================================

bool UHUDPersistentPanel::SetupWidget(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController)
{
	if (Super::SetupWidget(InGameInstance, InPlayerController))
	{
		return true;
	}

	/* Setup all tabs and tab switching buttons */

	/* To make sure 2 tab switching buttons don't share the same type */
	TSet <EHUDPersistentTabType> NoDuplicates;

	TArray <UWidget *> AllChildren;
	WidgetTree->GetAllWidgets(AllChildren);

	for (int32 i = 0; i < AllChildren.Num(); ++i)
	{
		UWidget * Widget = AllChildren[i];

		if (Widget->IsA(UHUDPersistentTab::StaticClass()))
		{
			UHUDPersistentTab * Tab = CastChecked<UHUDPersistentTab>(Widget);

			Tab->SetShowTextLabelsOnButtons(bShowTextLabelsOnButtons);
			Tab->SetUnclickableButtonOpacity(UnclickableButtonOpacity);
			Tab->SetupWidget(GI, PC);

			Tabs.Emplace(Tab);
			TabsTMap.Emplace(Tab->GetType(), Tab);
		}
		else if (Widget->IsA(UHUDPersistentTabSwitchingButton::StaticClass()))
		{
			UHUDPersistentTabSwitchingButton * Button = CastChecked<UHUDPersistentTabSwitchingButton>(Widget);

#if !UE_BUILD_SHIPPING
			const EHUDPersistentTabType ButtonType = Button->GetTabType();

			assert(ButtonType != EHUDPersistentTabType::None);

			if (NoDuplicates.Contains(ButtonType))
			{
				/* Two buttons share the same tab type. Actually game will still run fine if this
				is the case */
				UE_LOG(RTSLOG, Warning, TEXT("On the HUD persistent panel at least two tab switching "
					"buttons share the same category %s"),
					TO_STRING(EHUDPersistentTabType, ButtonType));
			}
			else
			{
				NoDuplicates.Emplace(ButtonType);
			}
#endif // !UE_BUILD_SHIPPING

			Button->Setup(this, InPlayerController, InGameInstance);
		}
	}

	/* Added this recently. Pretty sure it needs to happen otherwise the enum to int conversion 
	funcs will not work correctly. Actually this doesn't need to happen at all. I now use a TMap 
	to get the widget given a tab type */
	Tabs.Sort();

	// Show default active tab
	SwitchToTab(DefaultTab);

	return false;
}

void UHUDPersistentPanel::OnTick(float InDeltaTime)
{
	if (ActiveTab != nullptr /* <-- more perf option. IsWidgetBound(Switcher_Tabs) is alternative though */)
	{	
		/* Just ticking the active tab not all of them */
		ActiveTab->OnTick(InDeltaTime);
	}
}

void UHUDPersistentPanel::SwitchToTab(EHUDPersistentTabType TabType)
{
	assert(TabType != EHUDPersistentTabType::None);
	
	UE_CLOG(!TabsTMap.Contains(TabType), RTSLOG, Fatal, TEXT("In regards to HUD persistent panel: "
		"tried to switch to tab type \"%s\" but it is not on the panel"),
		TO_STRING(EHUDPersistentTabType, TabType));

	/* The more expensive SetActiveWidget will have to be used since the indices in Tabs 
	don't necessarily corrispond to the child indices of Switcher_Tabs. */
	//Switcher_Tabs->SetActiveWidgetIndex(TabTypeToInt(TabType));
	Switcher_Tabs->SetActiveWidget(TabsTMap[TabType]);

	ActiveTab = CastChecked<UHUDPersistentTab>(Switcher_Tabs->GetActiveWidget());
}

void UHUDPersistentPanel::OnUpgradeComplete(EUpgradeType UpgradeType)
{
	/* This is currently a NOOP. It may need to run logic similar to OnBuildingConstructed 
	and make buttons usable that now have their prereqs met. Right now I'm too busy with 
	other stuff so I'm not going to do it TODO */
}

void UHUDPersistentPanel::OnBuildingConstructed(EBuildingType BuildingType, bool bFirstOfItsType)
{
	if (bFirstOfItsType)
	{
		for (const auto & Tab : Tabs)
		{
			Tab->OnBuildingConstructed(BuildingType, bFirstOfItsType);
		}
	}
}

void UHUDPersistentPanel::OnBuildingDestroyed(EBuildingType BuildingType, bool bLastOfItsType)
{
	if (bLastOfItsType)
	{
		for (const auto & Tab : Tabs)
		{
			Tab->OnBuildingDestroyed(BuildingType, bLastOfItsType);
		}
	}
}

void UHUDPersistentPanel::OnItemAddedToProductionQueue(const FTrainingInfo & Item, const FProductionQueue & Queue, AActor * Producer)
{
	/* Assumed not a BuildBuilding command in a persistent queue since it would build automatically
	and therefore OnItemAddedAndProductionStarted should have been called (all because queue
	capacity is capped at 1 right now) */

	const EHUDPersistentTabType TabType = FI->GetHUDPersistentTab(Item);

	if (TabType == EHUDPersistentTabType::None)
	{
		UE_LOG(RTSLOG, VeryVerbose, TEXT("Tab type for button was None"));
		return;
	}

	TabsTMap[TabType]->OnItemAddedToProductionQueue(Item, Queue, Producer);
}

void UHUDPersistentPanel::OnItemAddedAndProductionStarted(const FTrainingInfo & Item, const FProductionQueue & Queue, AActor * Producer)
{
	/* If for building then assuming it for from a persistent queue and therefore all buttons need
	to know */
	if (Item.IsProductionForBuilding())
	{
		for (const auto & Tab : Tabs)
		{
			Tab->OnItemAddedAndProductionStarted(Item, Queue, Producer);
		}
	}
	else
	{
		const EHUDPersistentTabType TabType = FI->GetHUDPersistentTab(Item);

		/* If true implies the button used None and therefore does not want to be on a persistent tab */
		if (TabType == EHUDPersistentTabType::None)
		{
			UE_LOG(RTSLOG, VeryVerbose, TEXT("Tab type for button was None"));
		}
		else
		{
			UE_CLOG(TabsTMap.Contains(TabType) == false, RTSLOG, Fatal, TEXT("Tab not part of HUD "
				"persistent panel: [%s], item was: [%s]"), TO_STRING(EHUDPersistentTabType, TabType),
				*Item.ToString());
			
			/* A crash here may indicate the selectable you were trying to build belongs to a persistent
			tab category that isn't a part of the panel. Make sure you have a tab widget added to the
			panel that uses this selectable's tab category, or change the selectable's category */
			TabsTMap[TabType]->OnItemAddedAndProductionStarted(Item, Queue, Producer);
		}
	}
}

void UHUDPersistentPanel::OnProductionComplete(const FTrainingInfo & Item, const FProductionQueue & Queue, uint8 NumRemoved, AActor * Producer)
{
	/* If persistent queue then all tabs need to know. If not then only one button needs to
	know (I think) */
	if (Queue.GetType() == EProductionQueueType::Persistent)
	{
		for (const auto & Tab : Tabs)
		{
			Tab->OnProductionComplete(Item, Queue, NumRemoved, Producer);
		}
	}
	else
	{
		const EHUDPersistentTabType TabToNotify = FI->GetHUDPersistentTab(Item);
		TabsTMap[TabToNotify]->OnProductionComplete(Item, Queue, NumRemoved, Producer);
	}
}

void UHUDPersistentPanel::OnBuildsInTabProductionStarted(const FTrainingInfo & Item, const FProductionQueue & InQueue, AActor * Producer)
{
	/* If we knew which tabs had BuildBuilding buttons we could narrow down the number of tabs
	we call this on because we only need to grey out BuildBuilding buttons while this new building
	builds */
	for (const auto & Tab : Tabs)
	{
		Tab->OnBuildsInTabProductionStarted(Item, InQueue, Producer);
	}
}

void UHUDPersistentPanel::OnBuildsInTabProductionComplete(const FTrainingInfo & Item)
{
	/* Get the tab this should affect and call appropriate function */
	const EHUDPersistentTabType TabType = FI->GetHUDPersistentTab(Item);
	TabsTMap[TabType]->OnBuildsInTabProductionComplete(Item);
}

void UHUDPersistentPanel::OnBuildsInTabBuildingPlaced(const FTrainingInfo & Item)
{
	for (const auto & Tab : Tabs)
	{
		Tab->OnBuildsInTabBuildingPlaced(Item);
	}
}


//=============================================================================================
//	HUD Persistent Tab Switching Button
//=============================================================================================

UHUDPersistentTabSwitchingButton::UHUDPersistentTabSwitchingButton(const FObjectInitializer & ObjectInitializer)
	: Super(ObjectInitializer)
{
	TabType = EHUDPersistentTabType::None;
	//IsFocusable = false; // UButton variable
	Purpose = EUIElementType::NoTooltipRequired;
}

void UHUDPersistentTabSwitchingButton::Setup(UHUDPersistentPanel * InPersistentPanel, ARTSPlayerController * LocalPlayCon, 
	URTSGameInstance * InGameInstance)
{
	PersistentPanel = InPersistentPanel;
	PC = LocalPlayCon;

	SetPC(LocalPlayCon);
	SetOwningWidget();

	OnLeftMouseButtonPressed().BindUObject(this, &UHUDPersistentTabSwitchingButton::UIBinding_OnLeftMouseButtonPress);
	OnLeftMouseButtonReleased().BindUObject(this, &UHUDPersistentTabSwitchingButton::UIBinding_OnLeftMouseButtonReleased);
	OnRightMouseButtonPressed().BindUObject(this, &UHUDPersistentTabSwitchingButton::UIBinding_OnRightMouseButtonPress);
	OnRightMouseButtonReleased().BindUObject(this, &UHUDPersistentTabSwitchingButton::UIBinding_OnRightMouseButtonReleased);

	// Guess we'll use action bar flags. Could make our own category in GI if needed though
	const FUnifiedImageAndSoundFlags UnifiedAssetFlags = InGameInstance->GetUnifiedButtonAssetFlags_ActionBar();
	if (UnifiedAssetFlags.bUsingUnifiedHoverSound)
	{
		SetHoveredSound(InGameInstance->GetUnifiedHoverSound_ActionBar().GetSound());
	}
	if (UnifiedAssetFlags.bUsingUnifiedLMBPressSound)
	{
		SetPressedByLMBSound(InGameInstance->GetUnifiedLMBPressedSound_ActionBar().GetSound());
	}
	if (UnifiedAssetFlags.bUsingUnifiedRMBPressSound)
	{
		SetPressedByRMBSound(InGameInstance->GetUnifiedRMBPressedSound_ActionBar().GetSound());
	}
}

void UHUDPersistentTabSwitchingButton::UIBinding_OnLeftMouseButtonPress()
{
	PC->OnLMBPressed_HUDPersistentPanel_SwitchTab(this);
}

void UHUDPersistentTabSwitchingButton::UIBinding_OnLeftMouseButtonReleased()
{
	PC->OnLMBReleased_HUDPersistentPanel_SwitchTab(this);
}

void UHUDPersistentTabSwitchingButton::UIBinding_OnRightMouseButtonPress()
{
	PC->OnRMBPressed_HUDPersistentPanel_SwitchTab(this);
}

void UHUDPersistentTabSwitchingButton::UIBinding_OnRightMouseButtonReleased()
{
	PC->OnRMBReleased_HUDPersistentPanel_SwitchTab(this);
}

void UHUDPersistentTabSwitchingButton::OnClicked()
{
	PersistentPanel->SwitchToTab(TabType);
}

EHUDPersistentTabType UHUDPersistentTabSwitchingButton::GetTabType() const
{
	return TabType;
}

#if WITH_EDITOR
const FText UHUDPersistentTabSwitchingButton::GetPaletteCategory()
{
	return URTSHUD::PALETTE_CATEGORY;
}
#endif


//=============================================================================================
//	HUD Persistent Tab
//=============================================================================================

//=============================================================================================
//	HUD Persistent Tab Structs
//=============================================================================================

FButtonClickabilityInfo::FButtonClickabilityInfo()
	: ButtonPtr(nullptr)
	, bArePrerequisitesMet(false)
	, bHasQueueSupport(false)
{
	// Because never expected to be called
	// Commented because crash here on startup after migrating to 4.21
	//assert(0);
}

FButtonClickabilityInfo::FButtonClickabilityInfo(const FContextButton * InButton)
	: ButtonPtr(InButton)
	, bArePrerequisitesMet(false)
	, bHasQueueSupport(false)
{
}

FButtonClickabilityInfo::FButtonClickabilityInfo(const FContextButton * InButton, bool InArePrereqsMet, bool bInQueueSupport)
	: ButtonPtr(InButton)
	, bArePrerequisitesMet(InArePrereqsMet)
	, bHasQueueSupport(bInQueueSupport)
{
}

const FContextButton * FButtonClickabilityInfo::GetButtonPtr() const
{
	return ButtonPtr;
}

bool FButtonClickabilityInfo::ArePrerequisitesMet() const
{
	return bArePrerequisitesMet;
}

bool FButtonClickabilityInfo::HasQueueSupport() const
{
	return bHasQueueSupport;
}

void FButtonClickabilityInfo::SetArePrerequisitesMet(bool bNewValue)
{
	bArePrerequisitesMet = bNewValue;
}

void FButtonClickabilityInfo::SetHasQueueSupport(bool bNewValue)
{
	bHasQueueSupport = bNewValue;
}

bool FButtonClickabilityInfo::CanBeActive() const
{
	return ArePrerequisitesMet() && HasQueueSupport();
}


FActiveButtonStateData::FActiveButtonStateData() 
	: Queue(nullptr)
	, QueueOwner(nullptr) 
	, NumItemsInQueue(0) 
	, bIsAnotherButtonsPersistentQueueProducing(false)
	, CannotFunctionReason(EGameWarning::None)
{
}

const FProductionQueue * FActiveButtonStateData::GetQueue() const
{
	return Queue;
}

AActor * FActiveButtonStateData::GetQueueOwner() const
{
	return QueueOwner;
}

int32 FActiveButtonStateData::GetNumItemsInQueue() const
{
	return NumItemsInQueue;
}

bool FActiveButtonStateData::IsAnotherButtonsPersistentQueueProducing() const
{
	return bIsAnotherButtonsPersistentQueueProducing;
}

EGameWarning FActiveButtonStateData::GetCannotFunctionReason() const
{
	return CannotFunctionReason;
}

void FActiveButtonStateData::SetQueue(const FProductionQueue * InQueue)
{
	Queue = InQueue;
}

void FActiveButtonStateData::SetQueueOwner(AActor * InQueueOwner)
{
	QueueOwner = InQueueOwner;
}

void FActiveButtonStateData::SetNumItemsInQueue(int32 NewValue)
{
	NumItemsInQueue = NewValue;

	assert(NumItemsInQueue >= 0);
}

void FActiveButtonStateData::SetIsAnotherButtonsPersistentQueueProducing(bool bNewValue)
{
	bIsAnotherButtonsPersistentQueueProducing = bNewValue;
}

void FActiveButtonStateData::SetCannotFunctionReason(EGameWarning InReason)
{
	CannotFunctionReason = InReason;
}

FString FActiveButtonStateData::ToString() const
{
	FString String;
	String += "Owner: ";
	String += Statics::IsValid(QueueOwner) ? QueueOwner->GetName() : "Invalid";
	return String;
}


//=============================================================================================
//	Tab Widget
//=============================================================================================

UHUDPersistentPanel::UHUDPersistentPanel(const FObjectInitializer & ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Could change this to something like None + 1
	DefaultTab = EHUDPersistentTabType::None;
	UnclickableButtonOpacity = 0.5f;
	bShowTextLabelsOnButtons = true;
}

bool UHUDPersistentTab::SetupWidget(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController)
{
	if (Super::SetupWidget(InGameInstance, InPlayerController))
	{
		return true;
	}

#if !UE_BUILD_SHIPPING

	if (TabCategory == EHUDPersistentTabType::None)
	{
		UE_LOG(RTSLOG, Warning, TEXT("HUD persistent panel tab widget %s has type set to None. This "
			"is not allowed. Either change the tab type to something different or remove the widget "
			"via editor"), *GetName());
		return false;
	}
	else
	{
		const TMap < EHUDPersistentTabType, FButtonArray > & Map = FI->GetHUDPersistentTabButtons();
		if (Map[TabCategory].GetButtons().Num() == 0)
		{
			UE_LOG(RTSLOG, Warning, TEXT("The HUD widget for faction %s has a persistent panel "
				"with tab category %s but no selectables use that category. \nYou can override "
				"the default HUD widget in faction info to use a different widget."),
				TO_STRING(EFaction, FI->GetFaction()),
				TO_STRING(EHUDPersistentTabType, TabCategory));
			
			/* No return here - need to hide buttons. May get another log message though */
		}
	}

#endif // !UE_BUILD_SHIPPING

	TArray<UWidget *> AllChildren;
	WidgetTree->GetAllWidgets(AllChildren);

	/* For logging */
	int32 NumUnusedButtons = 0;

	/* Assuming that the order they appear in editor is the same order we get them by iterating */
	int32 ButtonCount = 0;
	for (int32 i = 0; i < AllChildren.Num(); ++i)
	{
		UWidget * Elem = AllChildren[i];

		if (Elem->IsA(UHUDPersistentTabButton::StaticClass()))
		{
			UHUDPersistentTabButton * AsButton = CastChecked<UHUDPersistentTabButton>(Elem);

			AsButton->SetupButton(this, GI, PC, PS, UnclickableButtonOpacity);

			/* Set what context the button will have. Based on each selectables
			Attributes.HUDPersistentTabButtonOrdering */
			const FContextButton * ButtonType = FI->GetHUDPersistentTabButton(TabCategory, ButtonCount);
			if (ButtonType == nullptr)
			{
				NumUnusedButtons++;

				AsButton->SetVisibility(HUDStatics::HIDDEN_VISIBILITY);

				/* Note: button is never added to Buttons array since it is expected to never
				be used */

				/* Do not return here - let all remaining buttons be made unactive */
			}
			else
			{
				if (HUDOptions::PersistentTabButtonDisplayRule == EHUDPersistentTabButtonDisplayRule::NoShuffling_HideUnavailables
					|| HUDOptions::PersistentTabButtonDisplayRule == EHUDPersistentTabButtonDisplayRule::NoShuffling_FadeUnavailables)
				{
					AsButton->SetPermanentButtonType(ButtonType, bShowTextLabelsOnButtons);
				}

				ButtonWidgets.Emplace(AsButton);
		
				/* Make all buttons unactive by default. Assuming here that buildings have not 
				been spawned yet */
				UnactiveButtons.Emplace(FButtonClickabilityInfo(ButtonType));
				AsButton->MakeUnactive();

				/* This is used to know what button requested production when the confirmation
				comes back from server */
				const FTrainingInfo TrainingInfo = FTrainingInfo(*ButtonType);
				assert(!TypeMap.Contains(TrainingInfo));
				TypeMap.Emplace(TrainingInfo, AsButton);

				ButtonCount++;
			}
		}
	}

	/* This Reserve is extra important not only for performance but to prevent the pointers 
	to the TMap values from becoming invalidated as more pairs are added */
	ButtonStates.Reserve(ButtonWidgets.Num());
	for (const auto & Elem : ButtonWidgets)
	{
		FActiveButtonStateData & Value = ButtonStates.Emplace(Elem->GetButtonType(), FActiveButtonStateData());
			
		if (HUDOptions::PersistentTabButtonDisplayRule == EHUDPersistentTabButtonDisplayRule::NoShuffling_HideUnavailables
			|| HUDOptions::PersistentTabButtonDisplayRule == EHUDPersistentTabButtonDisplayRule::NoShuffling_FadeUnavailables)
		{
			Elem->SetPeramentStateInfo(&Value);
		}
	}

	/* Initially set active any button widgets that should be. This is in case buildings were
	build before this function was run but usually this will do nothing (in fact it
	will ALWAYS do nothing). Leaving this out */
	//for (int32 i = 0; i < ButtonWidgets.Num(); ++i)
	//{
	//	
	//}
	
	/* Check if we either added not enough buttons or too many, and log something if so */
	const int32 NumButtonsMissing = FI->GetHUDPersistentTabButtons()[TabCategory].GetButtons().Num() - ButtonCount;
	if (NumButtonsMissing > 0)
	{
		UE_LOG(RTSLOG, Warning, TEXT("Faction %s does not have enough buttons for the tab %s to "
			"support every selectable. It needs %d more buttons for everything to be buildable "
			"from persistent panel"),
			TO_STRING(EFaction, FI->GetFaction()),
			TO_STRING(EHUDPersistentTabType, TabCategory),
			NumButtonsMissing);

		// If thrown then not enough buttons we added in editor to support. Same as message above
		verboseAssert(0);
	}
	else if (NumUnusedButtons > 0)
	{
		UE_LOG(RTSLOG, Warning, TEXT("%d buttons on persistent HUD tab %s were placed in editor but faction %s does "
			"not need this many buttons in this tab - button will be made hidden"),
			NumUnusedButtons,
			TO_STRING(EHUDPersistentTabType, TabCategory),
			TO_STRING(EFaction, FI->GetFaction()));
	}

	return false;
}

void UHUDPersistentTab::OnTick(float InDeltaTime)
{
	// UHUDPersistentPanel::OnTick... checks to see if the widget is the active widget before 
	// calling this tick func, so the GetVisibility() != HUDStatics::HIDDEN_VISIBILITY is likely 
	// irrelevant. Should test this 
	
	if (GetVisibility() != HUDStatics::HIDDEN_VISIBILITY)
	{
		/* Update progress bars for each button */
		for (const auto & ButtonWidget : ButtonWidgets)
		{
			assert(Statics::IsValid(ButtonWidget));
			
			// TODO update progress bars of buttons
			ButtonWidget->OnTick(InDeltaTime);
		}
	}
}

int32 UHUDPersistentTab::GetNumActiveButtons() const
{
	/* The NoShuffling methods do not keep this array up to date */
	assert(HUDOptions::PersistentTabButtonDisplayRule == EHUDPersistentTabButtonDisplayRule::SameEveryTime
		|| HUDOptions::PersistentTabButtonDisplayRule == EHUDPersistentTabButtonDisplayRule::OrderTheyBecomeAvailable)

	return ButtonIndices.Num();
}

EHUDPersistentTabType UHUDPersistentTab::GetType() const
{
	return TabCategory;
}

void UHUDPersistentTab::OnBuildingConstructed(EBuildingType BuildingType, bool bFirstOfItsType)
{
	/* This + OnBuildingDestroyed code *may* be able to run faster */
	
	// bFirstOfItsType has already been checked to be true by this point, double check
	assert(bFirstOfItsType);

	if (HUDOptions::PersistentTabButtonDisplayRule == EHUDPersistentTabButtonDisplayRule::NoShuffling_HideUnavailables 
		|| HUDOptions::PersistentTabButtonDisplayRule == EHUDPersistentTabButtonDisplayRule::NoShuffling_FadeUnavailables)
	{
		/* Need to check every button and see if its prereqs are now met. Also need to check if 
		the building we just built has any of them on its context menu implying it can produce 
		them */
		for (int32 i = UnactiveButtons.Num() - 1; i >= 0; --i)
		{
			FButtonClickabilityInfo & ButtonStruct = UnactiveButtons[i];

			// Check if prereqs are now fulfilled because of new building
			if (ButtonStruct.ArePrerequisitesMet() == false
				&& PS->ArePrerequisitesMet(*ButtonStruct.GetButtonPtr(), false) == true)
			{
				ButtonStruct.SetArePrerequisitesMet(true);
			}

			/* Check if new building is the first one to be able to produce what the button wants. 
			There is an alternative way of doing this. Remove this whole statement from this loop, 
			then after this for loop make another for loop that loops through the building's context menu array. Use 
			TypeMap[FTrainingInfo(Elem)] to get the widget and then check on that widget. 
			With the current method we are checking on all widgets - the alternative method narrows 
			the scope. */
			if (ButtonStruct.HasQueueSupport() == false
				&& PS->HasQueueSupport(*ButtonStruct.GetButtonPtr(), false) == true)
			{
				ButtonStruct.SetHasQueueSupport(true);
			}

			/* Check if we can make this button active now */
			if (ButtonStruct.CanBeActive())
			{
				UHUDPersistentTabButton * ButtonWidget = TypeMap[FTrainingInfo(*ButtonStruct.GetButtonPtr())];

				ButtonWidget->MakeActive();

				UnactiveButtons.RemoveAtSwap(i, 1, false);
			}
		}
	}
	else if (HUDOptions::PersistentTabButtonDisplayRule == EHUDPersistentTabButtonDisplayRule::SameEveryTime)
	{
		const int32 NumUnactiveAtStartOfFunction = UnactiveButtons.Num();
		
		for (int32 i = 0; i < UnactiveButtons.Num(); ++i)
		{
			FButtonClickabilityInfo & ButtonStruct = UnactiveButtons[i];

			// Check if prereq status has changed because of new building just built
			if (ButtonStruct.ArePrerequisitesMet() == false
				&& PS->ArePrerequisitesMet(*ButtonStruct.GetButtonPtr(), false) == true)
			{
				ButtonStruct.SetArePrerequisitesMet(true);
			}

			// Check if queue support status has changed because of new building just built
			if (ButtonStruct.HasQueueSupport() == false
				&& PS->HasQueueSupport(*ButtonStruct.GetButtonPtr(), false) == true)
			{
				ButtonStruct.SetHasQueueSupport(true);
			}

			// Check if we can now active this button because all requirements are now fulfilled
			if (ButtonStruct.CanBeActive())
			{
				/* Slot the button into the right position by finding the correct index 
				in the array and inserting it there O(n).
				Binary search is an option instead but technically still keeps this O(n) 
				because we have to insert into array after finding right index. Since these 
				arrays are expected to be quite small ( < 50 ) will continue to use linear search */
				int32 IndexToInsertAt = 0;
				for (int32 k = 0; k < ButtonIndices.Num(); ++k)
				{
					const FContextButton * Elem = ButtonIndices[k];

					if (ButtonStruct.GetButtonPtr()->GetHUDPersistentTabButtonOrdering() > Elem->GetHUDPersistentTabButtonOrdering())
					{
						IndexToInsertAt++;
					}
					else
					{
						break;
					} 
				}
				
				ButtonIndices.Insert(ButtonStruct.GetButtonPtr(), IndexToInsertAt);

				UnactiveButtons.RemoveAtSwap(i, 1, false);
			}
		}

		const bool bDirtyedTab = (NumUnactiveAtStartOfFunction != UnactiveButtons.Num());

		if (bDirtyedTab)
		{
			const FUnifiedImageAndSoundFlags UnifiedAssetFlags = GI->GetUnifiedButtonAssetFlags_ActionBar();
			
			/* Now update all the button widgets */
			for (int32 i = 0; i < ButtonIndices.Num(); ++i)
			{
				ButtonWidgets[i]->MakeActive(ButtonIndices[i], &ButtonStates[ButtonIndices[i]], 
					bShowTextLabelsOnButtons, UnifiedAssetFlags);
			}
		}
	}
	else // Assuming OrderTheyBecomeAvailable
	{
		assert(HUDOptions::PersistentTabButtonDisplayRule == EHUDPersistentTabButtonDisplayRule::OrderTheyBecomeAvailable);

		int32 NumButtonsToActivateThisFunctionCall = 0;

		for (int32 i = UnactiveButtons.Num() - 1; i >= 0; --i)
		{
			FButtonClickabilityInfo & ButtonStruct = UnactiveButtons[i];

			// Check if prereq status changed because of building just built
			if (ButtonStruct.ArePrerequisitesMet() == false
				&& PS->ArePrerequisitesMet(*ButtonStruct.GetButtonPtr(), false) == true)
			{
				ButtonStruct.SetArePrerequisitesMet(true);
			}

			// Check if queue support status has changed because of building just built
			if (ButtonStruct.HasQueueSupport() == false
				&& PS->HasQueueSupport(*ButtonStruct.GetButtonPtr(), false) == true)
			{
				ButtonStruct.SetHasQueueSupport(true);
			}

			// Check if we can now activate this button because all requirements are now fulfilled
			if (ButtonStruct.CanBeActive())
			{
				if (NumButtonsToActivateThisFunctionCall == 0)
				{
					// First element we are adding, we know it goes at the end
					ButtonIndices.Emplace(ButtonStruct.GetButtonPtr());
				}
				else
				{
					/* Find right index in ButtonIndices and insert there. The rest of the array is
					already sorted. We only need to consider the elements we've added previously
					during this func */
					const int32 StartIndex = GetNumActiveButtons() - NumButtonsToActivateThisFunctionCall;
					int32 IndexToInsertAt = StartIndex;
					for (int32 k = StartIndex; k < ButtonIndices.Num(); ++k)
					{
						const FContextButton * Elem = ButtonIndices[k];

						if (ButtonStruct.GetButtonPtr()->GetHUDPersistentTabButtonOrdering() > Elem->GetHUDPersistentTabButtonOrdering())
						{
							IndexToInsertAt++;
						}
						else
						{
							break;
						}
					}

					ButtonIndices.Insert(ButtonStruct.GetButtonPtr(), IndexToInsertAt);
				}
				
				NumButtonsToActivateThisFunctionCall++;

				UnactiveButtons.RemoveAtSwap(i, 1, false);
			}
		}

		const bool bDirtyedTab = (NumButtonsToActivateThisFunctionCall > 0);

		/* Check if UI was dirtyed */
		if (bDirtyedTab)
		{
			const FUnifiedImageAndSoundFlags UnifiedAssetFlags = GI->GetUnifiedButtonAssetFlags_ActionBar();
			
			/* Now update all the button widgets */
			for (int32 i = 0; i < ButtonIndices.Num(); ++i)
			{
				ButtonWidgets[i]->MakeActive(ButtonIndices[i], &ButtonStates[ButtonIndices[i]], 
					bShowTextLabelsOnButtons, UnifiedAssetFlags);
			}
		}
	}
}

void UHUDPersistentTab::OnBuildingDestroyed(EBuildingType BuildingType, bool bLastOfItsType)
{
	if (HUDOptions::PersistentTabButtonDisplayRule == EHUDPersistentTabButtonDisplayRule::NoShuffling_HideUnavailables
		|| HUDOptions::PersistentTabButtonDisplayRule == EHUDPersistentTabButtonDisplayRule::NoShuffling_FadeUnavailables)
	{
		for (int32 i = ButtonIndices.Num() - 1; i >= 0; --i)
		{
			const FContextButton * Button = ButtonIndices[i];
			
			// Check if prereqs are now not fulfilled because of building being destroyed
			const bool bPrereqsMet = PS->ArePrerequisitesMet(*Button, false);

			// Check if queue support is gone now that building has been destroyed
			const bool bQueueSupport = PS->HasQueueSupport(*Button, false);

			if (!bPrereqsMet || !bQueueSupport)
			{
				UHUDPersistentTabButton * ButtonWidget = ButtonWidgets[i];
				
				ButtonWidget->MakeUnactive();
				UnactiveButtons.Emplace(FButtonClickabilityInfo(Button, bPrereqsMet, bQueueSupport));
			}
		}
	}
	else if (HUDOptions::PersistentTabButtonDisplayRule == EHUDPersistentTabButtonDisplayRule::SameEveryTime)
	{
		assert(ButtonWidgets.Num() == ButtonIndices.Num());
		
		const int32 NumActiveButtonsOnFunctionStart = ButtonIndices.Num();

		for (int32 i = ButtonIndices.Num() - 1; i >= 0; --i)
		{
			const FContextButton * Button = ButtonIndices[i];
			
			// Check if prereqs are now not fulfilled because of building being destroyed
			const bool bPrereqsMet = PS->ArePrerequisitesMet(*Button, false);

			// Check if queue support is gone now that building has been destroyed
			const bool bQueueSupport = PS->HasQueueSupport(*Button, false);

			if (!bPrereqsMet || !bQueueSupport)
			{
				ButtonIndices.RemoveAtSwap(i, 1, false);
			}
		}

		const int32 NumMadeUnactive = NumActiveButtonsOnFunctionStart - ButtonIndices.Num();
		const bool bDirtyedTab = (NumMadeUnactive > 0);
		 
		if (bDirtyedTab)
		{
			const FUnifiedImageAndSoundFlags UnifiedAssetFlags = GI->GetUnifiedButtonAssetFlags_ActionBar();
			
			// Update the widgets
			const int32 ArrayNum = ButtonIndices.Num();
			for (int32 i = 0; i < ArrayNum; ++i)
			{
				ButtonWidgets[i]->MakeActive(ButtonIndices[i], &ButtonStates[ButtonIndices[i]], 
					bShowTextLabelsOnButtons, UnifiedAssetFlags);
			}

			// Make unactive the buttons at the end
			for (int32 i = ArrayNum; i < NumMadeUnactive; ++i)
			{
				ButtonWidgets[i]->MakeUnactive();
			}
		}
	}
	else // Assumed OrderTheyBecomeAvailable
	{
		assert(HUDOptions::PersistentTabButtonDisplayRule == EHUDPersistentTabButtonDisplayRule::OrderTheyBecomeAvailable);

		assert(ButtonWidgets.Num() == ButtonIndices.Num());

		// Currently just same as EHUDPersistentTabButtonDisplayRule::SameEveryTime

		const int32 NumActiveButtonsOnFunctionStart = ButtonIndices.Num();

		for (int32 i = ButtonIndices.Num() - 1; i >= 0; --i)
		{
			const FContextButton * Button = ButtonIndices[i];

			// Check if prereqs are now not fulfilled because of building being destroyed
			const bool bPrereqsMet = PS->ArePrerequisitesMet(*Button, false);

			// Check if queue support is gone now that building has been destroyed
			const bool bQueueSupport = PS->HasQueueSupport(*Button, false);

			if (!bPrereqsMet || !bQueueSupport)
			{
				ButtonIndices.RemoveAtSwap(i, 1, false);
			}
		}

		const int32 NumMadeUnactive = NumActiveButtonsOnFunctionStart - ButtonIndices.Num();
		const bool bDirtyedTab = (NumMadeUnactive > 0);

		if (bDirtyedTab)
		{
			const FUnifiedImageAndSoundFlags UnifiedAssetFlags = GI->GetUnifiedButtonAssetFlags_ActionBar();
			
			// Update the widgets
			const int32 ArrayNum = ButtonIndices.Num();
			for (int32 i = 0; i < ArrayNum; ++i)
			{
				ButtonWidgets[i]->MakeActive(ButtonIndices[i], &ButtonStates[ButtonIndices[i]], 
					bShowTextLabelsOnButtons, UnifiedAssetFlags);
			}

			// Make unactive the buttons at the end
			for (int32 i = ArrayNum; i < NumMadeUnactive; ++i)
			{
				ButtonWidgets[i]->MakeUnactive();
			}
		}
	}
}

void UHUDPersistentTab::SetUnclickableButtonOpacity(float InOpacity)
{
	UnclickableButtonOpacity = InOpacity;
}

void UHUDPersistentTab::SetShowTextLabelsOnButtons(bool bNewValue)
{
	bShowTextLabelsOnButtons = bNewValue;
}

void UHUDPersistentTab::OnItemAddedToProductionQueue(const FTrainingInfo & Item, const FProductionQueue & Queue, AActor * Producer)
{
	/* Only need to notify button that is relevant */
	TypeMap[Item]->OnItemAddedToProductionQueue(Item, Queue, Producer);
}

void UHUDPersistentTab::OnItemAddedAndProductionStarted(const FTrainingInfo & Item, const FProductionQueue & Queue, AActor * Producer)
{
	/* This will be interesting. Will need to grey out all BuildBuildings buttons if it is a
	BuildBuilding command. If not then do nothing */

	/* If a persistent queue build command then we need to notify each button so it can turn grey
	while the item builds */
	if (Queue.GetType() == EProductionQueueType::Persistent)
	{
		assert(Item.IsProductionForBuilding());

		for (const auto & Button : ButtonWidgets)
		{
			// Null means button widget is unactive i.e. no button is assigned to it and it 
			// shouldn't even be visible
			if (Button->GetButtonType() != nullptr)
			{
				const bool bForThisButton = (Item == FTrainingInfo(*Button->GetButtonType()));

				Button->OnItemAddedAndProductionStarted(Item, Queue, Producer, bForThisButton);
			}
		}
	}
	else
	{
		TypeMap[Item]->OnItemAddedAndProductionStarted(Item, Queue, Producer, true);
	}
}

void UHUDPersistentTab::OnProductionComplete(const FTrainingInfo & Item, const FProductionQueue & Queue, uint8 NumRemoved, AActor * Producer)
{
	if (Queue.GetType() == EProductionQueueType::Persistent)
	{
		assert(Item.IsProductionForBuilding());

		for (const auto & ButtonWidget : ButtonWidgets)
		{
			/* These null checks i'm doing with GetButtonType() isn't the best for scaling. It 
			means as the potential number of buttons that can be on the persistent panel grows 
			we spend more time doing these checks. An alternative is to use ButtonIndices instead 
			but we would have to store the pointers to the UHUDPersistentButton that they each 
			corrispond to */
			if (ButtonWidget->GetButtonType() != nullptr)
			{
				ButtonWidget->OnProductionComplete(Item, Queue, NumRemoved, Producer);
			}
		}
	}
	else
	{
		TypeMap[Item]->OnProductionComplete(Item, Queue, NumRemoved, Producer);
	}
}

void UHUDPersistentTab::OnBuildsInTabProductionStarted(const FTrainingInfo & Item, const FProductionQueue & InQueue, AActor * Producer)
{
	for (const auto & Button : ButtonWidgets)
	{
		if (Button->GetButtonType()->IsForBuildBuilding())
		{
			Button->OnBuildsInTabProductionStarted(Item, InQueue, Producer);
		}
	}
}

void UHUDPersistentTab::OnBuildsInTabProductionComplete(const FTrainingInfo & Item)
{
	/* Call on the button this should affect */
	TypeMap[Item]->OnBuildsInTabProductionComplete(Item);
}

void UHUDPersistentTab::OnBuildsInTabBuildingPlaced(const FTrainingInfo & Item)
{
	for (const auto & Button : ButtonWidgets)
	{
		if (Button->GetButtonType()->IsForBuildBuilding())
		{
			Button->OnBuildsInTabBuildingPlaced(Item);
		}
	}
}

bool operator<(const UHUDPersistentTab & Widget1, const UHUDPersistentTab & Widget2)
{
	return Widget1.TabCategory < Widget2.TabCategory;
}


//==============================================================================================
//	HUD Persistent Tab Button
//==============================================================================================

UHUDPersistentTabButton::UHUDPersistentTabButton(const FObjectInitializer & ObjectInitializer)
	: Super(ObjectInitializer)
{
	Purpose = EUIElementType::PersistentPanel;
	
	// Possibly no longer needed after inheriting from UMyButton
	//IsFocusable = false;
}

void UHUDPersistentTabButton::SetupButton(UHUDPersistentTab * InOwningTab, URTSGameInstance * InGameInstance,
	ARTSPlayerController * InPlayerController, ARTSPlayerState * InPlayerState, float InUnclickableAlpha)
{
	assert(InPlayerController != nullptr);

	OriginalAlpha = GetRenderOpacity();
	UnclickableAlpha = InUnclickableAlpha;

	OwningTab = InOwningTab;
	GI = InGameInstance;
	PC = InPlayerController;
	SetPC(InPlayerController);
	SetOwningWidget();
	PS = InPlayerState;
	FI = InPlayerController->GetFactionInfo();

	UTextBlock * DummyPtr = nullptr;
	UContextActionButton::SetImageAndProgressBarReferences(this, ProductionProgressBar,
		DescriptionText, ExtraText, false, DummyPtr);

	OnLeftMouseButtonPressed().BindUObject(this, &UHUDPersistentTabButton::UIBinding_OnLMBPress);
	OnLeftMouseButtonReleased().BindUObject(this, &UHUDPersistentTabButton::UIBinding_OnLMBReleased);
	OnRightMouseButtonPressed().BindUObject(this, &UHUDPersistentTabButton::UIBinding_OnRMBPress);
	OnRightMouseButtonReleased().BindUObject(this, &UHUDPersistentTabButton::UIBinding_OnRMBReleased);
	
	const FUnifiedImageAndSoundFlags UnifiedAssetFlags = InGameInstance->GetUnifiedButtonAssetFlags_ActionBar();
	SetImagesAndUnifiedImageFlags_ExcludeNormalAndHighlightedImage(UnifiedAssetFlags,
		&InGameInstance->GetUnifiedHoverBrush_ActionBar().GetBrush(),
		&InGameInstance->GetUnifiedPressedBrush_ActionBar().GetBrush());
	if (UnifiedAssetFlags.bUsingUnifiedHoverSound)
	{
		SetHoveredSound(InGameInstance->GetUnifiedHoverSound_ActionBar().GetSound());
	}
	if (UnifiedAssetFlags.bUsingUnifiedLMBPressSound)
	{
		SetPressedByLMBSound(InGameInstance->GetUnifiedLMBPressedSound_ActionBar().GetSound());
	}
	if (UnifiedAssetFlags.bUsingUnifiedRMBPressSound)
	{
		SetPressedByRMBSound(InGameInstance->GetUnifiedRMBPressedSound_ActionBar().GetSound());
	}

	Button = nullptr;
	StateInfo = nullptr;
}

void UHUDPersistentTabButton::SetPermanentButtonType(const FContextButton * InButton, bool bShowTextLabel)
{
	/* Only really expected to be called for one of these 2 methods */
	assert(HUDOptions::PersistentTabButtonDisplayRule == EHUDPersistentTabButtonDisplayRule::NoShuffling_HideUnavailables
		|| HUDOptions::PersistentTabButtonDisplayRule == EHUDPersistentTabButtonDisplayRule::NoShuffling_FadeUnavailables);
	
	Button = InButton;

	const FUnifiedImageAndSoundFlags UnifiedAssetFlags = GI->GetUnifiedButtonAssetFlags_ActionBar();

	if (InButton->IsForResearchUpgrade())
	{
		const FUpgradeInfo & UpgradeInfo = FI->GetUpgradeInfoChecked(InButton->GetUpgradeType());
		
		/* Set image */
		/* Setting these we're assuming this func is called after SetupButton */
		SetImages_ExcludeHighlightedImage(UnifiedAssetFlags,
			&UpgradeInfo.GetHUDImage(),
			&UpgradeInfo.GetHoveredHUDImage(),
			&UpgradeInfo.GetPressedHUDImage());
		if (UnifiedAssetFlags.bUsingUnifiedHoverSound == false)
		{
			SetHoveredSound(UpgradeInfo.GetHoveredButtonSound());
		}
		if (UnifiedAssetFlags.bUsingUnifiedLMBPressSound == false)
		{
			SetPressedByLMBSound(UpgradeInfo.GetPressedByLMBSound());
		}
		if (UnifiedAssetFlags.bUsingUnifiedRMBPressSound == false)
		{
			SetPressedByRMBSound(UpgradeInfo.GetPressedByRMBSound());
		}

		/* Set button name text */
		DescriptionText->SetText(bShowTextLabel ? UpgradeInfo.GetName() : FText::GetEmpty());
	}
	else // Assumed for train unit or build building
	{
		assert(InButton->IsForBuildBuilding() || InButton->IsForTrainUnit());

		const FBuildInfo * BuildInfo = FI->GetBuildInfo(*InButton);

		/* Set image */
		/* Setting these we're assuming this func is called after SetupButton */
		SetImages_ExcludeHighlightedImage(UnifiedAssetFlags,
			&BuildInfo->GetHUDImage(),
			&BuildInfo->GetHoveredHUDImage(),
			&BuildInfo->GetPressedHUDImage());
		if (UnifiedAssetFlags.bUsingUnifiedHoverSound == false)
		{
			SetHoveredSound(BuildInfo->GetHoveredButtonSound());
		}
		if (UnifiedAssetFlags.bUsingUnifiedLMBPressSound == false)
		{
			SetPressedByLMBSound(BuildInfo->GetPressedByLMBSound());
		}
		if (UnifiedAssetFlags.bUsingUnifiedRMBPressSound == false)
		{
			SetPressedByRMBSound(BuildInfo->GetPressedByRMBSound());
		}

		/* Set button name text */
		DescriptionText->SetText(bShowTextLabel ? BuildInfo->GetName() : FText::GetEmpty());
	}
	
	// Do these in case they were edited in editor
	ExtraText->SetText(FText::GetEmpty());
	ProductionProgressBar->SetPercent(0.f);
}

void UHUDPersistentTabButton::SetPeramentStateInfo(FActiveButtonStateData * InStateInfo)
{
	StateInfo = InStateInfo;
}

void UHUDPersistentTabButton::MakeActive()
{
	if (HUDOptions::PersistentTabButtonDisplayRule == EHUDPersistentTabButtonDisplayRule::NoShuffling_HideUnavailables)
	{
		// Not logical to call this if we weren't already hidden
		assert(GetVisibility() == HUDStatics::HIDDEN_VISIBILITY);

		SetVisibility(ESlateVisibility::Visible);
	}
	else if (HUDOptions::PersistentTabButtonDisplayRule == EHUDPersistentTabButtonDisplayRule::NoShuffling_FadeUnavailables)
	{
		SetRenderOpacity(OriginalAlpha);
	}
	else
	{
		UE_LOG(RTSLOG, Fatal, TEXT("Logic error"));
	}
}

void UHUDPersistentTabButton::MakeActive(const FContextButton * InButton, FActiveButtonStateData * InStateInfo, 
	bool bShowLabelText, FUnifiedImageAndSoundFlags UnifiedAssetFlags)
{
	/* These methods should not need to call this function */
	assert(HUDOptions::PersistentTabButtonDisplayRule != EHUDPersistentTabButtonDisplayRule::NoShuffling_HideUnavailables
		&& HUDOptions::PersistentTabButtonDisplayRule != EHUDPersistentTabButtonDisplayRule::NoShuffling_FadeUnavailables);

	// Use MakeUnactive() if you want to deactivate the button
	assert(InButton != nullptr);

	/* If not changing button type then return */
	if (Button == InButton)
	{
		return;
	}

	Button = InButton;
	StateInfo = InStateInfo;

	if (InButton->IsForResearchUpgrade())
	{
		const FUpgradeInfo & UpgradeInfo = FI->GetUpgradeInfoChecked(InButton->GetUpgradeType());

		/* Set image */
		SetImages_ExcludeHighlightedImage(UnifiedAssetFlags,
			&UpgradeInfo.GetHUDImage(),
			&UpgradeInfo.GetHoveredHUDImage(),
			&UpgradeInfo.GetPressedHUDImage());
		if (UnifiedAssetFlags.bUsingUnifiedHoverSound == false)
		{
			SetHoveredSound(UpgradeInfo.GetHoveredButtonSound());
		}
		if (UnifiedAssetFlags.bUsingUnifiedLMBPressSound == false)
		{
			SetPressedByLMBSound(UpgradeInfo.GetPressedByLMBSound());
		}
		if (UnifiedAssetFlags.bUsingUnifiedRMBPressSound == false)
		{
			SetPressedByRMBSound(UpgradeInfo.GetPressedByRMBSound());
		}

		/* Set button name text */
		DescriptionText->SetText(bShowLabelText ? UpgradeInfo.GetName() : URTSHUD::BLANK_TEXT);
	}
	else // Assumed for train unit or build building
	{
		assert(InButton->IsForBuildBuilding() || InButton->IsForTrainUnit());

		const FBuildInfo * BuildInfo = FI->GetBuildInfo(*InButton);

		/* Set image */
		SetImages_ExcludeHighlightedImage(UnifiedAssetFlags,
			&BuildInfo->GetHUDImage(),
			&BuildInfo->GetHoveredHUDImage(),
			&BuildInfo->GetPressedHUDImage());
		if (UnifiedAssetFlags.bUsingUnifiedHoverSound == false)
		{
			SetHoveredSound(BuildInfo->GetHoveredButtonSound());
		}
		if (UnifiedAssetFlags.bUsingUnifiedLMBPressSound == false)
		{
			SetPressedByLMBSound(BuildInfo->GetPressedByLMBSound());
		}
		if (UnifiedAssetFlags.bUsingUnifiedRMBPressSound == false)
		{
			SetPressedByRMBSound(BuildInfo->GetPressedByRMBSound());
		}

		/* Set button name text */
		DescriptionText->SetText(bShowLabelText ? BuildInfo->GetName() : URTSHUD::BLANK_TEXT);
	}

	/* Set quantity text */
	ExtraText->SetText(GetQuantityText());
	
	/* Set PBar percent */
	float PBarPercent;
	if (InButton->IsForBuildBuilding())
	{
		if (InStateInfo->IsAnotherButtonsPersistentQueueProducing())
		{
			PBarPercent = 1.f;
		}
		else
		{
			PBarPercent = 1.f - InStateInfo->GetQueue()->GetPercentageCompleteForUI(GetWorld());
		}
	}
	else
	{
		if (InStateInfo->GetQueue() != nullptr)
		{
			PBarPercent = 1.f - InStateInfo->GetQueue()->GetPercentageCompleteForUI(GetWorld());
		}
		else
		{
			PBarPercent = 0.f;
		}
	}
	ProductionProgressBar->SetPercent(PBarPercent);

	// These 3 rules set the button hidden when it is made unactive
	if (HUDOptions::PersistentTabButtonDisplayRule == EHUDPersistentTabButtonDisplayRule::NoShuffling_HideUnavailables
		|| HUDOptions::PersistentTabButtonDisplayRule == EHUDPersistentTabButtonDisplayRule::SameEveryTime
		|| HUDOptions::PersistentTabButtonDisplayRule == EHUDPersistentTabButtonDisplayRule::OrderTheyBecomeAvailable)
	{
		SetVisibility(ESlateVisibility::Visible);
	}
}

void UHUDPersistentTabButton::MakeUnactive()
{
	if (HUDOptions::PersistentTabButtonDisplayRule == EHUDPersistentTabButtonDisplayRule::NoShuffling_HideUnavailables)
	{
		SetVisibility(HUDStatics::HIDDEN_VISIBILITY);
	}
	else if (HUDOptions::PersistentTabButtonDisplayRule == EHUDPersistentTabButtonDisplayRule::NoShuffling_FadeUnavailables)
	{
		/* We could optionally make the button flat out unclickalbe. If so then we can remove the 
		checks for queue support and prereqs in the OnButtonClick func. We would also need to 
		do SetVisibility(Visible) in MakeActive() */

		SetRenderOpacity(UnclickableAlpha);
	}
	else // Assumed SameEveryTime or OrderTheyBecomeAvailable
	{
		assert(HUDOptions::PersistentTabButtonDisplayRule == EHUDPersistentTabButtonDisplayRule::SameEveryTime
			|| HUDOptions::PersistentTabButtonDisplayRule == EHUDPersistentTabButtonDisplayRule::OrderTheyBecomeAvailable);

		SetVisibility(HUDStatics::HIDDEN_VISIBILITY);
	}
}

#if WITH_EDITOR
void UHUDPersistentTabButton::OnCreationFromPalette()
{
	UContextActionButton::OnButtonCreationFromPalette(this, false);
}
#endif // WITH_EDITOR

void UHUDPersistentTabButton::OnTick(float InDeltaTime)
{
	assert(StateInfo != nullptr);
	
	/* Recently changed this to set percentage to 0 if Queue was null, but this is can't happen 
	anymore, so may need to adjust some logic elsewhere to get PBar fill correct */
	
	/* Set fill percentage of progress bar */
	if (StateInfo->GetQueue() != nullptr)
	{
		assert(Statics::IsValid(GetWorld()));
		assert(Statics::IsValid(ProductionProgressBar));
		
		float FillPercentage = 0.f;
		
		FillPercentage = 1.f - StateInfo->GetQueue()->GetPercentageCompleteForUI(GetWorld());

		ProductionProgressBar->SetPercent(FillPercentage);
	}
}

void UHUDPersistentTabButton::OnAnotherPersistentQueueProductionStarted()
{
	StateInfo->SetIsAnotherButtonsPersistentQueueProducing(true);
	StateInfo->SetCannotFunctionReason(EGameWarning::BuildingInProgress);

	// We should fill the progress bar 100% while the other building builds
	ProductionProgressBar->SetPercent(1.f);
}

void UHUDPersistentTabButton::OnAnotherPersistentQueueProductionFinished()
{
	StateInfo->SetIsAnotherButtonsPersistentQueueProducing(false);
	
	/* Currently the only reason this variable can get assigned is either "BuildingInProgress" 
	or "None". If more options become possible then assigning None now may not be correct. 
	Could just ditch this variable altogether though */
	StateInfo->SetCannotFunctionReason(EGameWarning::None);

	// Remove the 100% fill of progress bar
	ProductionProgressBar->SetPercent(0.f);
}

FText UHUDPersistentTabButton::GetQuantityText() const
{
	// Only show number if there is more than 1 item in the queue
	return StateInfo->GetNumItemsInQueue() > 1 ? FText::AsNumber(StateInfo->GetNumItemsInQueue()) : FText::GetEmpty();
}

void UHUDPersistentTabButton::UIBinding_OnLMBPress()
{
	PC->OnLMBPressed_HUDPersistentPanel_Build(this);
}

void UHUDPersistentTabButton::UIBinding_OnLMBReleased()
{
	PC->OnLMBReleased_HUDPersistentPanel_Build(this);
}

void UHUDPersistentTabButton::UIBinding_OnRMBPress()
{
	PC->OnRMBPressed_HUDPersistentPanel_Build(this);
}

void UHUDPersistentTabButton::UIBinding_OnRMBReleased()
{
	PC->OnRMBReleased_HUDPersistentPanel_Build(this);
}

const FContextButton * UHUDPersistentTabButton::GetButtonType() const
{
	return Button;
}

FActiveButtonStateData * UHUDPersistentTabButton::GetStateInfo() const
{
	return StateInfo;
}

bool UHUDPersistentTabButton::IsProducingSomething() const
{
	return Statics::IsValid(StateInfo->GetQueueOwner()) && StateInfo->GetQueue() != nullptr
		&& StateInfo->GetQueue()->Num() > 0 && !StateInfo->GetQueue()->HasCompletedBuildsInTab();
}

bool UHUDPersistentTabButton::HasProductionCompleted() const
{
	return Statics::IsValid(StateInfo->GetQueueOwner()) && StateInfo->GetQueue() != nullptr
		&& StateInfo->GetQueue()->Num() > 0 && StateInfo->GetQueue()->HasCompletedBuildsInTab();
}

void UHUDPersistentTabButton::OnItemAddedToProductionQueue(const FTrainingInfo & Item, const FProductionQueue & InQueue, AActor * Producer)
{
	/* Not expected to be called with persistent queues while their max capacity is limited to 1 */
	assert(InQueue.GetType() != EProductionQueueType::Persistent);

	/* Call OnBuildsInTabProductionStarted instead */
	assert(!Item.IsProductionForBuilding());

	/* There is an assumption here that the owning tab only calls this on the button corrisponding
	to Item. Therefore this must be for our button. 
	TODO But different button clicks could result in different barracks getting the production requests,
	so these asserts will not hold in those cases */
	assert(StateInfo->GetQueue() == &InQueue);
	assert(StateInfo->GetQueueOwner() == Producer);

	// Increment num items in queue
	StateInfo->SetNumItemsInQueue(StateInfo->GetNumItemsInQueue() + 1);

	ExtraText->SetText(FText::AsNumber(StateInfo->GetNumItemsInQueue()));
}

void UHUDPersistentTabButton::OnItemAddedAndProductionStarted(const FTrainingInfo & Item, const FProductionQueue & InQueue, AActor * Producer, bool bIsForThisButton)
{
	/* This function can be called whether the item corrisponds to this button or not */

	if (bIsForThisButton)
	{
		assert(StateInfo->GetNumItemsInQueue() == 0);

		StateInfo->SetQueue(&InQueue);
		StateInfo->SetQueueOwner(Producer);

		// Increment num items in queue
		StateInfo->SetNumItemsInQueue(StateInfo->GetNumItemsInQueue() + 1);

		/* Only show number in queue when there is more than 1, but we know there's only 1 */
		ExtraText->SetText(FText::GetEmpty());
	}
	else
	{
		// If new item is a building and this button is for a building too...
		if (Button->IsForBuildBuilding() && Item.IsProductionForBuilding())
		{
			OnAnotherPersistentQueueProductionStarted();
		}
	}
}

void UHUDPersistentTabButton::OnProductionComplete(const FTrainingInfo & Item, const FProductionQueue & InQueue, uint8 NumRemoved, AActor * Producer)
{
	/* If the queue type is persistent then update that we may be clickable again because the item
	has finished */
	if (Item.IsProductionForBuilding())
	{
		assert(InQueue.GetType() == EProductionQueueType::Persistent);
		
		if (Button->IsForBuildBuilding())
		{
			if (Item.GetBuildingType() != Button->GetBuildingType())
			{
				OnAnotherPersistentQueueProductionFinished();
			}
			else // The item that completed was for this button
			{
				assert(StateInfo->GetQueue() == &InQueue);
				assert(StateInfo->GetQueueOwner() == Producer);

				/* If this was the last item in queue we can unlink with the queue so we do not monitor its
				progress each tick */
				if (InQueue.Num() == 0)
				{
					StateInfo->SetQueue(nullptr);
					StateInfo->SetQueueOwner(nullptr);
				}

				StateInfo->SetNumItemsInQueue(StateInfo->GetNumItemsInQueue() - NumRemoved);

				ExtraText->SetText(GetQuantityText());
			}
		}
		else
		{
			// Do nothing I think
		}
	}	
	/* I *think* this else is only executed for non-building production and the item is for 
	this button */
	else
	{
		StateInfo->SetNumItemsInQueue(StateInfo->GetNumItemsInQueue() - 1);

		// Update the quantity text
		ExtraText->SetText(GetQuantityText());

		if (StateInfo->GetNumItemsInQueue() == 0)
		{
			ProductionProgressBar->SetPercent(0.f);
			StateInfo->SetQueue(nullptr);
		}
	}
}

void UHUDPersistentTabButton::OnBuildsInTabProductionStarted(const FTrainingInfo & Item, const FProductionQueue & InQueue, AActor * Producer)
{
	/* Check if the item being produced corrisponds to our button. If it does we will set our queue
	and producer. If not then we will grey out our button since only one item can be produced from
	persistent queue at a time */
	if (Item.GetBuildingType() == Button->GetBuildingType())
	{
		assert(StateInfo->GetNumItemsInQueue() == 0);
		assert(ExtraText->GetText().EqualTo(URTSHUD::BLANK_TEXT));

		StateInfo->SetQueue(&InQueue);
		StateInfo->SetQueueOwner(Producer);
		StateInfo->SetNumItemsInQueue(StateInfo->GetNumItemsInQueue() + 1);
	}
	else
	{
		// Check here whether out button is a BuildBuilding button? Have we checked previously?
		OnAnotherPersistentQueueProductionStarted();
	}
}

void UHUDPersistentTabButton::OnBuildsInTabProductionComplete(const FTrainingInfo & Item)
{
	/* Expected to only be called if the item corrisponds to this button */
	assert(Item.GetBuildingType() == Button->GetBuildingType());

	ExtraText->SetText(HUDOptions::TEXT_READY_TO_PLACE);
}

void UHUDPersistentTabButton::OnBuildsInTabBuildingPlaced(const FTrainingInfo & Item)
{
	/* If item corrisponds to this button remove text. Otherwise ungrey button */
	if (Item.GetBuildingType() == Button->GetBuildingType())
	{
		StateInfo->SetQueue(nullptr);
		StateInfo->SetQueueOwner(nullptr);

		StateInfo->SetNumItemsInQueue(StateInfo->GetNumItemsInQueue() - 1);
		assert(StateInfo->GetNumItemsInQueue() == 0);

		/* Remove the "Ready to place" text */
		ExtraText->SetText(URTSHUD::BLANK_TEXT);
	}
	else
	{
		OnAnotherPersistentQueueProductionFinished();
	}
}

#if WITH_EDITOR
const FText UHUDPersistentTabButton::GetPaletteCategory()
{
	return URTSHUD::PALETTE_CATEGORY;
}
#endif


//===============================================================================================
//	Minimap
//===============================================================================================

/* Ideas:
-  Have fog of war manager as it iterates through every unit convert their world coords or (maybe
fog coords) to minimap coords. */

UMinimap::UMinimap(const FObjectInitializer & ObjectInitializer) 
	: Super(ObjectInitializer)
{
	SelectableColors.Reserve(Statics::NUM_AFFILIATIONS);
	SelectableColors.Emplace(EAffiliation::Owned, FLinearColor::Green);
	SelectableColors.Emplace(EAffiliation::Allied, FLinearColor::Blue);
	SelectableColors.Emplace(EAffiliation::Neutral, FLinearColor::Gray);
	SelectableColors.Emplace(EAffiliation::Hostile, FLinearColor::Red);
}

bool UMinimap::SetupWidget(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController)
{
	if (Super::SetupWidget(InGameInstance, InPlayerController))
	{
		return true;
	}

	// Put any logic here

	return false;
}

void UMinimap::Setup(ARTSLevelVolume * FogVolume)
{
	if (IsWidgetBound(Image_Minimap))
	{
		/* Could check image dimensions are good enough for texture and possibly resize the
		image if they aren't POSSIBLE TODO */

		// A lot of this copied from fog manager's initialize

		const FBoxSphereBounds Bounds = FogVolume->GetMapBounds();

		MapCenter = FVector2D(Bounds.Origin.Y, Bounds.Origin.X);

		const float X = Bounds.BoxExtent.X * 2;
		const float Y = Bounds.BoxExtent.Y * 2;

		/* Divide now to avoid division each tick */
		MapDimensionsInverse = FVector2D(1.f / Y, 1.f / X);

		UTexture2D * BackgroundTexture = FogVolume->GetMinimapTexture();

		// Set the texture to use as the map layer
		Image_Minimap->SetBrushFromTexture(BackgroundTexture);

		/* Image appears brighter than expected but can be changed by lowering the RBG values
		of the image brush in widget editor */

		ImageSizeInPixels = FIntPoint(BackgroundTexture->GetSizeX(), BackgroundTexture->GetSizeY());

		int32 ViewportX, ViewportY;
		PC->GetViewportSize(ViewportX, ViewportY);
		ViewportSizeInPixels = FIntPoint(ViewportX, ViewportY);
	}
	else
	{
		UE_LOG(RTSLOG, Warning, TEXT("Using a minimap widget but no image named \"Image_Minimap\" "
			"exists on it, therefore minimap data cannot be shown"));
	}
}

int32 UMinimap::NativePaint(const FPaintArgs & Args, const FGeometry & AllottedGeometry,
	const FSlateRect & MyCullingRect, FSlateWindowElementList & OutDrawElements, int32 LayerId,
	const FWidgetStyle & InWidgetStyle, bool bParentEnabled) const
{
	/* Leaving out Super and instead copying some of it because I cannot find any other way
	of getting a reference to a FPaintContext */

	FPaintContext Context(AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

#if MINIMAP_CALCULATED_ON_GAME_THREAD

	if (IsWidgetBound(Image_Minimap))
	{
		DrawSelectables(Context);
		DrawFogOfWar(Context);
	}

#endif // MINIMAP_CALCULATED_ON_GAME_THREAD

	return FMath::Max(LayerId, Context.MaxLayer);
}

void UMinimap::DrawSelectables(FPaintContext & PaintContext) const
{
	const ETeam LocalPlayersTeam = PS->GetTeam();
	const FName LocalPlayersTeamTag = PS->GetTeamTag();

	const FGeometry & CachedGeometry = Image_Minimap->GetCachedGeometry();

	// For each player...
	for (const auto & PlayerState : GS->GetPlayerStates())
	{
		const EAffiliation PlayerAffiliation = PlayerState->GetAffiliation();

		/* Coordinates of where to draw selectable */
		TArray < FVector2D > Points;
		Points.Reserve(PlayerState->GetBuildings().Num() + PlayerState->GetUnits().Num());

		for (const auto & Building : PlayerState->GetBuildings())
		{
			bool bDraw = true;

			// Need to check if in fog and if so then don't draw
			if (PlayerAffiliation == EAffiliation::Hostile)
			{
				bDraw = Statics::IsOutsideFog(Building, Building, LocalPlayersTeam, LocalPlayersTeamTag, GS);
			}

			if (bDraw)
			{
				Points.Emplace(WorldCoordsToMinimapCoords(Building->GetActorLocation(),
					CachedGeometry));
			}
		}
		for (const auto & Unit : PlayerState->GetUnits())
		{
			bool bDraw = true;

			// Need to check if in fog and if so then don't draw
			if (PlayerAffiliation == EAffiliation::Hostile)
			{
				bDraw = Statics::IsOutsideFog(Unit, Unit, LocalPlayersTeam, LocalPlayersTeamTag, GS);
			}

			if (bDraw)
			{
				Points.Emplace(WorldCoordsToMinimapCoords(Unit->GetActorLocation(),
					CachedGeometry));
			}
		}

		/* Not sure if better to call these DrawLine in the loops above or in this loop
		seperately... doing sperately for now */
		for (const auto & Pos : Points)
		{
			UWidgetBlueprintLibrary::DrawLine(PaintContext, Pos, Pos + 20, // TODO remove + 20. for testing only
				SelectableColors[PlayerAffiliation], false);
		}
	}

	/* Do neutral selectables */

	TArray < FVector2D > NeutralPoints;
	NeutralPoints.Reserve(GS->GetNeutrals().Num());

	for (const auto & Elem : GS->GetNeutrals())
	{
		/* Maybe add an actor tag that defines if neutral should alwasy be drawn even if in fog
		and check that here first */
		if (Statics::IsOutsideFog(Elem, CastChecked<ISelectable>(Elem), LocalPlayersTeam, LocalPlayersTeamTag, GS))
		{
			NeutralPoints.Emplace(WorldCoordsToMinimapCoords(Elem->GetActorLocation(),
				CachedGeometry));
		}
	}

	// Draw selectables on minimap
	for (const auto & Pos : NeutralPoints)
	{
		UWidgetBlueprintLibrary::DrawLine(PaintContext, Pos, Pos,
			SelectableColors[EAffiliation::Neutral], false);
	}

	// These don't have to be equal, just shouldn't be constantly always increasing each frame. 
	// If they are then the AddLines() Calls may need to not happen each tick and instead be in 
	// some kind of setup func
	//DEBUG_MESSAGE("Number of elements to be drawn", PaintContext.OutDrawElements.GetElementCount());
	//DEBUG_MESSAGE("Number of elements in some array", PaintContext.OutDrawElements.GetDrawElements().Num());
}

void UMinimap::DrawFogOfWar(FPaintContext & InContext) const
{
}

FVector2D UMinimap::WorldCoordsToMinimapCoords(const FVector & WorldLocation,
	const FGeometry & Geometry) const
{
	/* Note world coord X is Y and world coord Y is X here. This assumes X is forward in map */
	const FVector2D Coords = FVector2D(WorldLocation.Y, WorldLocation.X);

	FVector2D Vector = Coords - MapCenter;
	Vector *= MapDimensionsInverse;
	Vector += FVector2D(0.5f, 0.5f);
	Vector *= FVector2D(ImageSizeInPixels.X, -ImageSizeInPixels.Y);

	/* Geometry.GetAbsolutePosition() returns where the topleft corner of the image is on
	the actual screen i.e. on the actual deskptop resolution */

	return FVector2D(FMath::FloorToInt(Vector.X + Geometry.GetAbsolutePosition().X + ViewportSizeInPixels.X),
		FMath::FloorToInt(Vector.Y + Geometry.GetAbsolutePosition().Y + ViewportSizeInPixels.Y));
}


//============================================================================================
//	Inventory item tooltip widget
//============================================================================================

UTooltipWidget_InventoryItem::UTooltipWidget_InventoryItem(const FObjectInitializer & ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UTooltipWidget_InventoryItem::InitialSetup()
{
	if (IsWidgetBound(Text_QuantityOrNumCharges))
	{
		Text_QuantityOrNumCharges->SetText(FText::GetEmpty());

		// -1 for blank text
		DisplayedNumInStackOrNumCharges = -1;
	}
}

void UTooltipWidget_InventoryItem::SetupForInner(int16 InStackQuantityOrNumChargesToDisplay,
	const FInventoryItemInfo & ItemsInfo)
{
	if (DisplayedItemInfo != &ItemsInfo)
	{
		DisplayedItemInfo = &ItemsInfo;

		if (IsWidgetBound(Text_Name))
		{
			Text_Name->SetText(ItemsInfo.GetDisplayName());
		}

		if (IsWidgetBound(Text_Description))
		{
			Text_Description->SetText(ItemsInfo.GetDescription());
		}

		if (IsWidgetBound(Text_StatChanges))
		{
			Text_StatChanges->SetText(ItemsInfo.GetStatChangeText());
		}

		if (IsWidgetBound(Text_Flavor))
		{
			Text_Flavor->SetText(ItemsInfo.GetFlavorText());
		}

		if (IsWidgetBound(Image_Image))
		{
			Image_Image->SetBrush(ItemsInfo.GetDisplayImage());
		}
	}

	if (InStackQuantityOrNumChargesToDisplay != DisplayedNumInStackOrNumCharges)
	{
		DisplayedNumInStackOrNumCharges = InStackQuantityOrNumChargesToDisplay;

		if (IsWidgetBound(Text_QuantityOrNumCharges))
		{
			Text_QuantityOrNumCharges->SetText(InStackQuantityOrNumChargesToDisplay == -1 ? FText::GetEmpty() : FText::AsNumber(InStackQuantityOrNumChargesToDisplay));
		}
	}
}

void UTooltipWidget_InventoryItem::SetupFor(const AInventoryItem * InventoryItemActor)
{
	const FInventoryItemInfo * ItemsInfo = InventoryItemActor->GetItemInfo();

	// Here we pass in -1 to say 'don't show quantity or num charges' 
	const int16 DisplayAmount = ItemsInfo->CanStack()
		? InventoryItemActor->GetItemQuantity() : ItemsInfo->IsUsable() && !ItemsInfo->HasUnlimitedNumberOfUses()
		? InventoryItemActor->GetNumItemCharges()
		: -1;
	
	SetupForInner(DisplayAmount, *ItemsInfo);

	DisplayedInventoryItem = InventoryItemActor;
}

void UTooltipWidget_InventoryItem::SetupFor(const FInventorySlotState & InvSlot, const FInventoryItemInfo & ItemsInfo)
{
	// Here we pass in -1 to say 'don't show quantity or num charges' 
	const int16 DisplayAmount = ItemsInfo.CanStack()
		? InvSlot.GetNumInStack() : InvSlot.IsItemUsable() && !InvSlot.HasUnlimitedCharges()
		? InvSlot.GetNumCharges()
		: -1;

	SetupForInner(DisplayAmount, ItemsInfo);

	DisplayedInventoryItem = nullptr;
}

void UTooltipWidget_InventoryItem::SetupFor(const FItemOnDisplayInShopSlot & ShopSlot, const FInventoryItemInfo & ItemsInfo)
{
	// Here we pass in -1 to say 'don't show quantity'. We will pass in the number of item 
	// bought per shop purchase. This fills the UTextBlock for num charges or item quanitity. 
	// Could look wrong, Consider not showing at all
	const int16 DisplayAmount = ShopSlot.GetQuantityBoughtPerPurchase();

	SetupForInner(DisplayAmount, ItemsInfo);

	DisplayedInventoryItem = nullptr;
}

bool UTooltipWidget_InventoryItem::IsSetupFor(AInventoryItem * InventoryItem) const
{
	/* Note that because inventory item actors are pooled so this could return a false positive.
	Be aware of that when calling this */
	return (DisplayedInventoryItem == InventoryItem);
}


//============================================================================================
//	Selectable's action bar tooltip widget for abilities
//============================================================================================

void UTooltipWidget_Ability::SetupFor(const FContextButtonInfo & InButtonInfo)
{
	if (DisplayedButtonType != &InButtonInfo)
	{
		DisplayedButtonType = &InButtonInfo;

		if (IsWidgetBound(Text_Name))
		{
			Text_Name->SetText(InButtonInfo.GetName());
		}

		if (IsWidgetBound(Text_Description))
		{
			Text_Description->SetText(InButtonInfo.GetDescription());
		}
	}
}


//============================================================================================
//	Selectable's action bar tooltip widget for production
//============================================================================================

FText UTooltipWidget_Production::GetProductionTimeText(float ProductionTime)
{
	FloatToText(ProductionTime, TooltipOptions::ProductionTimeDisplayMethod);
}

void UTooltipWidget_Production::SetupFor(const FDisplayInfoBase & InInfo)
{
	if (DisplayedInfo != &InInfo)
	{
		DisplayedInfo = &InInfo;

		if (IsWidgetBound(Text_Name))
		{
			Text_Name->SetText(InInfo.GetName());
		}

		if (IsWidgetBound(Text_Description))
		{
			Text_Description->SetText(InInfo.GetDescription());
		}

		if (IsWidgetBound(Text_BuildTime))
		{
			Text_BuildTime->SetText(GetProductionTimeText(InInfo.GetTrainTime()));
		}

		if (IsWidgetBound(Text_Prerequisites))
		{
			Text_Prerequisites->SetText(InInfo.GetPrerequisitesText());
		}
	}
}


//============================================================================================
//	Global skill panel button tooltip widget
//============================================================================================

void UTooltipWidget_GlobalSkillsPanelButton::SetupFor(const FAquiredCommanderAbilityState * AbilityState, bool bForceUpdate)
{
	if (bForceUpdate || DisplayedAbilityInfoStruct != AbilityState->GetAbilityInfo())
	{
		DisplayedAbilityState = AbilityState;
		DisplayedAbilityInfoStruct = AbilityState->GetAbilityInfo();

		if (IsWidgetBound(Text_Name))
		{
			Text_Name->SetText(DisplayedAbilityInfoStruct->GetName());
		}
		if (IsWidgetBound(Text_Description))
		{
			Text_Description->SetText(DisplayedAbilityInfoStruct->GetDescription());
		}
	}
}

void UTooltipWidget_GlobalSkillsPanelButton::OnNewRankAquired()
{
	SetupFor(DisplayedAbilityState, true);
}

const FAquiredCommanderAbilityState * UTooltipWidget_GlobalSkillsPanelButton::GetDisplayedInfo() const
{
	return DisplayedAbilityState;
}


//============================================================================================
//	Commander's skill tree node tooltip widget
//============================================================================================

UTooltipWidget_CommanderSkillTreeNode::UTooltipWidget_CommanderSkillTreeNode(const FObjectInitializer & ObjectInitializer)
	: Super(ObjectInitializer)
{
	CannotAffordCostTextColor = FSlateColor(FLinearColor(1.f, 0.f, 0.f, 1.f));
}

void UTooltipWidget_CommanderSkillTreeNode::InitialSetup(ARTSPlayerState * PlayerState)
{
	PS = PlayerState;
	
	if (IsWidgetBound(Text_NextRank_Cost))
	{
		CostTextOriginalColor = Text_NextRank_Cost->ColorAndOpacity;
	}
}

void UTooltipWidget_CommanderSkillTreeNode::SetupFor(const FCommanderAbilityTreeNodeInfo * NodeInfo, bool bForceUpdate)
{
	/* Only do stuff if the node is different than what is already being displaying OR we 
	have just gained another rank of the ability */
	if (bForceUpdate || DisplayedNodeInfo != NodeInfo)
	{
		DisplayedNodeInfo = NodeInfo;
		
		const int32 AbilityRank = PS->GetCommanderAbilityObtainedRank(*DisplayedNodeInfo);
		const bool bHasAquiredAbility = (AbilityRank > -1);
		const bool bAnotherRankExists = DisplayedNodeInfo->IsRankValid(AbilityRank + 1);
		if (bHasAquiredAbility)
		{
			const FCommanderAbilityTreeNodeSingleRankInfo * AquiredRankAbilityInfo = &DisplayedNodeInfo->GetAbilityInfo(AbilityRank);
			const FCommanderAbilityInfo * AquiredRankAbilityInfoStruct = AquiredRankAbilityInfo->GetAbilityInfo();

			//-----------------------------------------------------------
			//	Set text/images for aquired rank of ability
			//-----------------------------------------------------------
			
			if (IsWidgetBound(Text_AquiredRank_Name))
			{
				Text_AquiredRank_Name->SetText(AquiredRankAbilityInfoStruct->GetName());
			}
			if (IsWidgetBound(Image_AquiredRank_Image))
			{
				Image_AquiredRank_Image->SetBrush(AquiredRankAbilityInfoStruct->GetNormalImage());
			}
			if (IsWidgetBound(Text_AquiredRank_Description))
			{
				Text_AquiredRank_Description->SetText(AquiredRankAbilityInfoStruct->GetDescription());
			}

			if (IsWidgetBound(Panel_AquiredRank))
			{
				Panel_AquiredRank->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
		}
		else
		{
			if (IsWidgetBound(Panel_AquiredRank))
			{
				Panel_AquiredRank->SetVisibility(ESlateVisibility::Collapsed);
			}
		}

		if (bAnotherRankExists)
		{
			const FCommanderAbilityTreeNodeSingleRankInfo * NextAbilityRankInfo = &DisplayedNodeInfo->GetAbilityInfo(AbilityRank + 1);
			const FCommanderAbilityInfo * NextAbilityRankInfoStruct = NextAbilityRankInfo->GetAbilityInfo();

			//-----------------------------------------------------------
			//	Set text/images for next rank of ability
			//-----------------------------------------------------------

			if (IsWidgetBound(Text_NextRank_Name))
			{
				Text_NextRank_Name->SetText(NextAbilityRankInfoStruct->GetName());
			}
			if (IsWidgetBound(Text_NextRank_Cost))
			{
				Text_NextRank_Cost->SetText(FText::AsNumber(NextAbilityRankInfo->GetCost()));

				const bool bCanPlayerAfford = PS->GetNumUnspentSkillPoints() >= NextAbilityRankInfo->GetCost();
				// Set color
				if (bCanPlayerAfford)
				{
					Text_NextRank_Cost->SetColorAndOpacity(CostTextOriginalColor);
				}
				else
				{
					Text_NextRank_Cost->SetColorAndOpacity(CannotAffordCostTextColor);
				}
			}
			if (IsWidgetBound(Text_NextRank_MissingRequirements))
			{
				Text_NextRank_MissingRequirements->SetText(GetMissingRequirementsText(DisplayedNodeInfo,
					NextAbilityRankInfo));
			}
			if (IsWidgetBound(Image_NextRank_Image))
			{
				Image_NextRank_Image->SetBrush(NextAbilityRankInfoStruct->GetNormalImage());
			}
			if (IsWidgetBound(Text_NextRank_Description))
			{
				Text_NextRank_Description->SetText(NextAbilityRankInfoStruct->GetDescription());
			}

			if (IsWidgetBound(Panel_NextRank))
			{
				Panel_NextRank->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
		}
		else
		{
			if (IsWidgetBound(Panel_NextRank))
			{
				Panel_NextRank->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}
}

void UTooltipWidget_CommanderSkillTreeNode::OnNewRankAquired(const FCommanderAbilityTreeNodeInfo * NodeInfo)
{
	SetupFor(NodeInfo, true);
}

const FCommanderAbilityTreeNodeInfo * UTooltipWidget_CommanderSkillTreeNode::GetDisplayedNodeInfo() const
{
	return DisplayedNodeInfo;
}

FText UTooltipWidget_CommanderSkillTreeNode::GetMissingRequirementsText(const FCommanderAbilityTreeNodeInfo * NodeInfo, 
	const FCommanderAbilityTreeNodeSingleRankInfo * AbilityInfo)
{
	// TODO
	return FText::FromString("TODO");
	
	//const bool bIsCommanderRankHighEnough = AbilityInfo->GetUnlockRank() <= NodeButton->GetCommandersRank();
	//const bool bHasEnoughSkillPointsToPurchase = AbilityInfo->GetCost() <= NodeButton->GetCommandersUnspentSkillPoints();
	//
	//FText Text;
	//
	//if (!bIsCommanderRankHighEnough)
	//{
	//	Text = NSLOCTEXT("CommanderSkillTreeTooltip", "MissingRequirements", "Rank not high enough");
	//}
	//
	//// Prerequisites too
	//for (int32 i = 0; i < NodeInfo->GetPrerequisites().Num(); ++i)
	//{
	//	const FCommanderSkillTreePrerequisiteArrayEntry & Elem = NodeInfo->GetPrerequisites()[i];
	//	
	//	if (NodeButton->HasAquiredSkill(Elem.GetPrerequisiteType()) == false)
	//	{
	//
	//	}
	//}
}


