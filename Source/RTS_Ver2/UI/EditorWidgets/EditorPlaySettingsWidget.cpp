// Fill out your copyright notice in the Description page of Project Settings.


#include "EditorPlaySettingsWidget.h"
#include "Components/ComboBoxString.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/CheckBox.h"
#include "Components/CanvasPanel.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/EditableTextBox.h"
#include "Engine/Canvas.h"
#include "Components/ButtonSlot.h"
#include "Components/GridPanel.h"
#include "Components/Spacer.h"
#include "Components/GridSlot.h"

#include "Statics/CommonEnums.h"
#include "Statics/DevelopmentStatics.h"
#include "Settings/EnumStringRepresentations.h"
#include "UI/InMatchDeveloperWidget.h"
#include "Settings/ProjectSettings.h"
#include "Statics/Statics.h"


//-----------------------------------------------------------------------
//=======================================================================
//	------- Resource Widget -------
//=======================================================================
//-----------------------------------------------------------------------

TSharedRef<SWidget> UEditorPlayStartingResourcesWidget::RebuildWidget()
{
	return Super::RebuildWidget();
}


//-----------------------------------------------------------------------
//=======================================================================
//	------- Player Info Widgets -------
//=======================================================================
//-----------------------------------------------------------------------

void UPIEHumanPlayerInfoProxy::UIBinding_OnTeamComboBoxSelectionChanged(FString Item, ESelectInfo::Type SelectionType)
{
	PlayerInfoStatics::SetTeamFromString(EnumStringObject, PlayerInfo.Team, Item);
	
	PlaySettingsWidget->HumanPlayerInfoCopy[PlayerIndex].Team = PlayerInfo.Team;
}

void UPIEHumanPlayerInfoProxy::UIBinding_OnFactionComboBoxSelectionChanged(FString Item, ESelectInfo::Type SelectionType)
{
	PlayerInfoStatics::SetFactionFromString(EnumStringObject, PlayerInfo.Faction, Item);
	
	PlaySettingsWidget->HumanPlayerInfoCopy[PlayerIndex].Faction = PlayerInfo.Faction;
}

void UPIEHumanPlayerInfoProxy::UIBinding_OnStartingSpotTextChanged(const FText & Text)
{
}

void UPIEHumanPlayerInfoProxy::UIBinding_OnStartingSpotTextCommitted(const FText & Text, ETextCommit::Type CommitMethod)
{
	PlayerInfoStatics::SetStartingSpotFromString(PlayerInfo.StartingSpot, Text.ToString());

	/* Set the text to reflect the set spot */
	Text_StartingSpot->SetText(FText::FromString(PlayerInfoStatics::GetString_StartingSpot(PlayerInfo.GetStartingSpot())));

	PlaySettingsWidget->HumanPlayerInfoCopy[PlayerIndex].StartingSpot = PlayerInfo.StartingSpot;
}

void UPIECPUPlayerInfoProxy::UIBinding_OnTeamComboBoxSelectionChanged(FString Item, ESelectInfo::Type SelectionType)
{
	PlayerInfoStatics::SetTeamFromString(EnumStringObject, PlayerInfo.Team, Item);

	PlaySettingsWidget->CPUPlayerInfoCopy[PlayerIndex].Team = PlayerInfo.Team;
}

void UPIECPUPlayerInfoProxy::UIBinding_OnFactionComboBoxSelectionChanged(FString Item, ESelectInfo::Type SelectionType)
{
	PlayerInfoStatics::SetFactionFromString(EnumStringObject, PlayerInfo.Faction, Item);

	PlaySettingsWidget->CPUPlayerInfoCopy[PlayerIndex].Faction = PlayerInfo.Faction;
}

void UPIECPUPlayerInfoProxy::UIBinding_OnStartingSpotTextChanged(const FText & Text)
{
}

void UPIECPUPlayerInfoProxy::UIBinding_OnStartingSpotTextCommitted(const FText & Text, ETextCommit::Type CommitMethod)
{
	PlayerInfoStatics::SetStartingSpotFromString(PlayerInfo.StartingSpot, Text.ToString());

	/* Set the text to reflect the set spot */
	Text_StartingSpot->SetText(FText::FromString(PlayerInfoStatics::GetString_StartingSpot(PlayerInfo.GetStartingSpot())));

	PlaySettingsWidget->CPUPlayerInfoCopy[PlayerIndex].StartingSpot = PlayerInfo.StartingSpot;
}

void UPIECPUPlayerInfoProxy::UIBinding_OnCPUDifficultyComboBoxSelectionChanged(FString Item, ESelectInfo::Type SelectionType)
{
	PlayerInfoStatics::SetCPUDifficultyFromString(EnumStringObject, PlayerInfo.Difficulty, Item);

	PlaySettingsWidget->CPUPlayerInfoCopy[PlayerIndex].Difficulty = PlayerInfo.GetDifficulty();
}


bool PlayerInfoStatics::SetTeamFromString(const UEnumStringRepresentations * EnumStringObject, ETeam & OutTeam, const FString & String)
{
	return EnumStringObject->GetEnumValueFromStringSlow_Team(String, OutTeam);
}

bool PlayerInfoStatics::SetFactionFromString(const UEnumStringRepresentations * EnumStringObject, EFaction & OutFaction, const FString & String)
{
	return EnumStringObject->GetEnumValueFromStringSlow_Faction(String, OutFaction);
}

bool PlayerInfoStatics::SetStartingSpotFromString(int16 & OutStartingSpot, const FString & String)
{
	if (String.Equals("Random", ESearchCase::IgnoreCase))
	{
		/* -1 means random */
		OutStartingSpot = -1;
		return true;
	}

	const int16 LocalStartingSpot = OutStartingSpot;
	OutStartingSpot = FCString::Atoi(*String);
	if (OutStartingSpot < -1 || OutStartingSpot > INT16_MAX)
	{
		OutStartingSpot = LocalStartingSpot;
		return false;
	}
	return true;
}

bool PlayerInfoStatics::SetCPUDifficultyFromString(const UEnumStringRepresentations * EnumStringObject, ECPUDifficulty & OutDifficulty, const FString & String)
{
	return EnumStringObject->GetEnumValueFromStringSlow_CPUDifficulty(String, OutDifficulty);
}

FString PlayerInfoStatics::GetString_StartingSpot(int16 StartingSpot)
{
	return StartingSpot == -1 ? "Random" : FString::FromInt(StartingSpot);
}


//-----------------------------------------------------------------------
//=======================================================================
//	------- Main Widget -------
//=======================================================================
//-----------------------------------------------------------------------

UEditorPlaySettingsWidget::UEditorPlaySettingsWidget(const FObjectInitializer & ObjectInitializer)
	: Super(ObjectInitializer)
{
	/* This variable is EditDefaultsOnly. Widget blueprints need EditAnywhere to actually
	edit variables in editor (probably bug) so this variable is not editable via editor
	but might have intended to be by the programmer who wrote it. So in the future
	if/when that changes make sure to not allow it to be edited, HideCategory
	meta would do it. The reason for this is that my URTSUnrealEdEngine class handles
	creating the widget every time the editor starts anyway */
	bAlwaysReregisterWithWindowsMenu = false;
}

bool UEditorPlaySettingsWidget::ShouldSkipOpeningCutscene() const
{
	return SkippingOption == EEditorPlaySkippingOption::SkipOpeningCutsceneOnly 
		|| SkippingOption == EEditorPlaySkippingOption::SkipMainMenu;
}

bool UEditorPlaySettingsWidget::ShouldSkipMainMenu() const
{
	return SkippingOption == EEditorPlaySkippingOption::SkipMainMenu;
}

EDefeatCondition UEditorPlaySettingsWidget::GetDefeatCondition() const
{
	return DefeatCondition;
}

const FStartingResourceConfig & UEditorPlaySettingsWidget::GetStartingResourceConfig() const
{
	return StartingResourceConfig;
}

bool UEditorPlaySettingsWidget::IsCheatWidgetBPSet() const
{
	return CheatWidget_BP != nullptr;
}

bool UEditorPlaySettingsWidget::ShouldInitiallyShowCheatWidget() const
{
	return bInitiallyShowCheatWidget;
}

const TSubclassOf<UInMatchDeveloperWidget>& UEditorPlaySettingsWidget::GetCheatWidgetBP() const
{
	return CheatWidget_BP;
}

const TArray<FPIEPlayerInfo>& UEditorPlaySettingsWidget::GetHumanPlayerInfo() const
{
	return HumanPlayerInfoCopy;
}

int32 UEditorPlaySettingsWidget::GetNumCPUPlayers() const
{
	return NumCPUPlayers;
}

const TArray<FPIECPUPlayerInfo>& UEditorPlaySettingsWidget::GetCPUPlayerInfo() const
{
	return CPUPlayerInfoCopy;
}

EInvalidOwnerIndexAction UEditorPlaySettingsWidget::GetInvalidHumanOwnerRule() const
{
	return InvalidHumanOwnerRule;
}

EInvalidOwnerIndexAction UEditorPlaySettingsWidget::GetInvalidCPUOwnerRule() const
{
	return InvalidCPUOwnerRule;
}

TSharedRef<SWidget> UEditorPlaySettingsWidget::RebuildWidget()
{
	TSharedRef<SWidget> Ref = Super::RebuildWidget();

	EnumStringObject = UEnumStringRepresentations::Get();
	assert(EnumStringObject != nullptr);

	if (GetClass() != StaticClass()) // TODO change this from != to ==. It is != for testing only
	{
		//-------------------------------------------------------------------------------------
		//=====================================================================================
		//	------- Creation of widgets for default implementation -------
		//=====================================================================================
		//-------------------------------------------------------------------------------------

		/* This is not a blueprint created by user. Create widgets to give it a default
		'works out of the box' implementation */

		//-----------------------------------------------------------------------------
		//	Root widget
		//-----------------------------------------------------------------------------

		/* Actually can probably assert this - I added this conditional for testing only */
		UCanvasPanel * RootWidget;
		if (GetRootWidget() == nullptr)
		{
			RootWidget = WidgetTree->ConstructWidget<UCanvasPanel>();
			WidgetTree->RootWidget = RootWidget;
		}
		else
		{
			RootWidget = CastChecked<UCanvasPanel>(GetRootWidget());
		}

		//-----------------------------------------------------------------------------
		//	Skipping Combo box
		//-----------------------------------------------------------------------------

		ComboBox_Skipping = WidgetTree->ConstructWidget<UComboBoxString>();
		RootWidget->AddChild(ComboBox_Skipping);
		UCanvasPanelSlot * Slot_ComboBox_Skipping = CastChecked<UCanvasPanelSlot>(ComboBox_Skipping->Slot);
		Slot_ComboBox_Skipping->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
		Slot_ComboBox_Skipping->SetPosition(FVector2D(300.f, 300.f));
		Slot_ComboBox_Skipping->SetSize(FVector2D(280.f, 50.f));
		Slot_ComboBox_Skipping->SetAlignment(FVector2D(0.0f, 0.0f));

		//-----------------------------------------------------------------------------
		//	Text above Skipping Combo box saying what it does
		//-----------------------------------------------------------------------------

		/* Text above the combo box saying what it does */
		UTextBlock * SkippingText = WidgetTree->ConstructWidget<UTextBlock>();
		RootWidget->AddChild(SkippingText);
		SkippingText->SetText(FText::FromString("Menu Skipping"));
		UCanvasPanelSlot * Slot_SkippingText = CastChecked<UCanvasPanelSlot>(SkippingText->Slot);
		Slot_SkippingText->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
		Slot_SkippingText->SetPosition(Slot_ComboBox_Skipping->GetPosition() + FVector2D(0.f, -50.f));
		Slot_SkippingText->SetSize(Slot_ComboBox_Skipping->GetSize());
		Slot_SkippingText->SetAlignment(FVector2D(0.0f, 0.0f));

		//-----------------------------------------------------------------------------
		//	Player configuration panels
		//-----------------------------------------------------------------------------

		/* Number of CPU players left button */
		Button_DecreaseNumCPUPlayers = WidgetTree->ConstructWidget<UButton>();
		RootWidget->AddChild(Button_DecreaseNumCPUPlayers);
		UCanvasPanelSlot * Slot_Button_DecreaseNumCPUPlayers = CastChecked<UCanvasPanelSlot>(Button_DecreaseNumCPUPlayers->Slot);
		Slot_Button_DecreaseNumCPUPlayers->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
		Slot_Button_DecreaseNumCPUPlayers->SetPosition(Slot_ComboBox_Skipping->GetPosition() + FVector2D(0.f, Slot_ComboBox_Skipping->GetSize().Y + 50.f));
		Slot_Button_DecreaseNumCPUPlayers->SetSize(FVector2D(50.f, 50.f));
		Slot_Button_DecreaseNumCPUPlayers->SetAlignment(FVector2D(0.0f, 0.0f));
		/* Text to go on 'left' button */
		UTextBlock * NumCPUPlayersMinusSymbolText = WidgetTree->ConstructWidget<UTextBlock>();
		Button_DecreaseNumCPUPlayers->AddChild(NumCPUPlayersMinusSymbolText);
		NumCPUPlayersMinusSymbolText->SetText(FText::FromString("-"));
		NumCPUPlayersMinusSymbolText->SetJustification(ETextJustify::Center);
		UButtonSlot * Slot_NumCPUPlayersMinusSymbolText = CastChecked<UButtonSlot>(NumCPUPlayersMinusSymbolText->Slot);
		Slot_NumCPUPlayersMinusSymbolText->SetVerticalAlignment(VAlign_Center);
		Slot_NumCPUPlayersMinusSymbolText->SetHorizontalAlignment(HAlign_Center);
		/* Text that shows number of CPU players */
		Text_NumCPUPlayers = WidgetTree->ConstructWidget<UTextBlock>();
		RootWidget->AddChild(Text_NumCPUPlayers);
		Text_NumCPUPlayers->SetJustification(ETextJustify::Center);
		UCanvasPanelSlot * Slot_Text_NumCPUPlayers = CastChecked<UCanvasPanelSlot>(Text_NumCPUPlayers->Slot);
		Slot_Text_NumCPUPlayers->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
		Slot_Text_NumCPUPlayers->SetPosition(Slot_Button_DecreaseNumCPUPlayers->GetPosition() + FVector2D(Slot_Button_DecreaseNumCPUPlayers->GetSize().X, 0.f));
		Slot_Text_NumCPUPlayers->SetSize(FVector2D(50.f, 50.f));
		Slot_Text_NumCPUPlayers->SetAlignment(FVector2D(0.0f, 0.0f));
		/* Number of CPU players right button */
		Button_IncreaseNumCPUPlayers = WidgetTree->ConstructWidget<UButton>();
		RootWidget->AddChild(Button_IncreaseNumCPUPlayers);
		UCanvasPanelSlot * Slot_Button_IncreaseNumCPUPlayers = CastChecked<UCanvasPanelSlot>(Button_IncreaseNumCPUPlayers->Slot);
		Slot_Button_IncreaseNumCPUPlayers->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
		Slot_Button_IncreaseNumCPUPlayers->SetPosition(Slot_Text_NumCPUPlayers->GetPosition() + FVector2D(Slot_Text_NumCPUPlayers->GetSize().X, 0.f));
		Slot_Button_IncreaseNumCPUPlayers->SetSize(FVector2D(50.f, 50.f));
		Slot_Button_IncreaseNumCPUPlayers->SetAlignment(FVector2D(0.0f, 0.0f));
		/* Text to go on 'right' button */
		UTextBlock * NumCPUPlayersPlusSymbolText = WidgetTree->ConstructWidget<UTextBlock>();
		Button_IncreaseNumCPUPlayers->AddChild(NumCPUPlayersPlusSymbolText);
		NumCPUPlayersPlusSymbolText->SetText(FText::FromString("+"));
		NumCPUPlayersPlusSymbolText->SetJustification(ETextJustify::Center);
		UButtonSlot * Slot_NumCPUPlayersPlusSymbolText = CastChecked<UButtonSlot>(NumCPUPlayersPlusSymbolText->Slot);
		Slot_NumCPUPlayersPlusSymbolText->SetVerticalAlignment(VAlign_Center);
		Slot_NumCPUPlayersPlusSymbolText->SetHorizontalAlignment(HAlign_Center);
		/* Text just saying "number of CPU players" */
		UTextBlock * NumCPUPlayersText = WidgetTree->ConstructWidget<UTextBlock>();
		RootWidget->AddChild(NumCPUPlayersText);
		NumCPUPlayersText->SetText(FText::FromString("Num CPU Players"));
		NumCPUPlayersText->SetJustification(ETextJustify::Center);
		UCanvasPanelSlot * Slot_NumCPUPlayersText = CastChecked<UCanvasPanelSlot>(NumCPUPlayersText->Slot);
		Slot_NumCPUPlayersText->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
		Slot_NumCPUPlayersText->SetPosition(Slot_Text_NumCPUPlayers->GetPosition() + FVector2D(-100.f, -Slot_Text_NumCPUPlayers->GetSize().Y));
		Slot_NumCPUPlayersText->SetAutoSize(true);
		Slot_NumCPUPlayersText->SetAlignment(FVector2D(0.0f, 0.0f));
		
		/* Text above human player grid panel */
		UTextBlock * HumanPanelText = WidgetTree->ConstructWidget<UTextBlock>();
		RootWidget->AddChild(HumanPanelText);
		HumanPanelText->SetJustification(ETextJustify::Center);
		HumanPanelText->SetText(FText::FromString("Human Players"));
		UCanvasPanelSlot * Slot_HumanPanelText = CastChecked<UCanvasPanelSlot>(HumanPanelText->Slot);
		Slot_HumanPanelText->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
		Slot_HumanPanelText->SetPosition(Slot_Button_DecreaseNumCPUPlayers->GetPosition() + FVector2D(0.f, Slot_Button_DecreaseNumCPUPlayers->GetSize().Y + 50.f));
		Slot_HumanPanelText->SetAutoSize(true);
		Slot_HumanPanelText->SetAlignment(FVector2D(0.0f, 0.0f));

		/* Human players panel */
		Grid_HumanPlayers = WidgetTree->ConstructWidget<UGridPanel>();
		RootWidget->AddChild(Grid_HumanPlayers);
		UCanvasPanelSlot * Slot_Panel_HumanPlayers = CastChecked<UCanvasPanelSlot>(Grid_HumanPlayers->Slot);
		Slot_Panel_HumanPlayers->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
		Slot_Panel_HumanPlayers->SetPosition(Slot_HumanPanelText->GetPosition() + FVector2D(0.f, Slot_HumanPanelText->GetSize().Y + 25.f));
		Slot_Panel_HumanPlayers->SetSize(FVector2D(500.f, 500.f));
		Slot_Panel_HumanPlayers->SetAlignment(FVector2D(0.0f, 0.0f));
		{
			/* First add a spacer cause the first part is the column that is the row labels */
			USpacer * Spacer = WidgetTree->ConstructWidget<USpacer>();
			Grid_HumanPlayers->AddChildToGrid(Spacer);
			UGridSlot * Slot_Spacer = CastChecked<UGridSlot>(Spacer->Slot);
			Slot_Spacer->SetColumn(0);
			/* Add text to label the 'team' column */
			UTextBlock * TeamText = WidgetTree->ConstructWidget<UTextBlock>();
			Grid_HumanPlayers->AddChildToGrid(TeamText);
			TeamText->SetJustification(ETextJustify::Center);
			TeamText->SetText(FText::FromString("Team"));
			UGridSlot * Slot_TeamText = CastChecked<UGridSlot>(TeamText->Slot);
			Slot_TeamText->SetColumn(1);
			/* Add text to label the 'faction' column */
			UTextBlock * FactionText = WidgetTree->ConstructWidget<UTextBlock>();
			Grid_HumanPlayers->AddChildToGrid(FactionText);
			FactionText->SetJustification(ETextJustify::Center);
			FactionText->SetText(FText::FromString("Faction"));
			UGridSlot * Slot_FactionText = CastChecked<UGridSlot>(FactionText->Slot);
			Slot_FactionText->SetColumn(2);
			/* Add text to label the 'starting spot' column */
			UTextBlock * StartingSpotText = WidgetTree->ConstructWidget<UTextBlock>();
			Grid_HumanPlayers->AddChildToGrid(StartingSpotText);
			StartingSpotText->SetJustification(ETextJustify::Center);
			StartingSpotText->SetText(FText::FromString("Starting Spot"));
			UGridSlot * Slot_StartingSpotText = CastChecked<UGridSlot>(StartingSpotText->Slot);
			Slot_StartingSpotText->SetColumn(3);
		}
		
		/* Text above CPU player grid panel */
		UTextBlock * CPUPanelText = WidgetTree->ConstructWidget<UTextBlock>();
		RootWidget->AddChild(CPUPanelText);
		CPUPanelText->SetJustification(ETextJustify::Center);
		CPUPanelText->SetText(FText::FromString("CPU Players"));
		UCanvasPanelSlot * Slot_CPUPanelText = CastChecked<UCanvasPanelSlot>(CPUPanelText->Slot);
		Slot_CPUPanelText->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
		Slot_CPUPanelText->SetPosition(Slot_HumanPanelText->GetPosition() + FVector2D(Slot_Panel_HumanPlayers->GetSize().X + 50.f, 0.f));
		Slot_CPUPanelText->SetAutoSize(true);
		Slot_CPUPanelText->SetAlignment(FVector2D(0.0f, 0.0f));

		/* CPU players panel */
		Grid_CPUPlayers = WidgetTree->ConstructWidget<UGridPanel>();
		RootWidget->AddChild(Grid_CPUPlayers);
		UCanvasPanelSlot * Slot_Panel_CPUPlayers = CastChecked<UCanvasPanelSlot>(Grid_CPUPlayers->Slot);
		Slot_Panel_CPUPlayers->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
		Slot_Panel_CPUPlayers->SetPosition(Slot_CPUPanelText->GetPosition() + FVector2D(0.f, Slot_CPUPanelText->GetSize().Y + 50.f));
		Slot_Panel_CPUPlayers->SetSize(FVector2D(500.f, 500.f));
		Slot_Panel_CPUPlayers->SetAlignment(FVector2D(0.0f, 0.0f));
		{
			/* First add a spacer cause the first part is the column that is the row labels */
			USpacer * Spacer = WidgetTree->ConstructWidget<USpacer>();
			Grid_CPUPlayers->AddChildToGrid(Spacer);
			UGridSlot * Slot_Spacer = CastChecked<UGridSlot>(Spacer->Slot);
			Slot_Spacer->SetColumn(0);
			/* Add text to label the 'team' column */
			UTextBlock * TeamText = WidgetTree->ConstructWidget<UTextBlock>();
			Grid_CPUPlayers->AddChildToGrid(TeamText);
			TeamText->SetJustification(ETextJustify::Center);
			TeamText->SetText(FText::FromString("Team"));
			UGridSlot * Slot_TeamText = CastChecked<UGridSlot>(TeamText->Slot);
			Slot_TeamText->SetColumn(1);
			/* Add text to label the 'faction' column */
			UTextBlock * FactionText = WidgetTree->ConstructWidget<UTextBlock>();
			Grid_CPUPlayers->AddChildToGrid(FactionText);
			FactionText->SetJustification(ETextJustify::Center);
			FactionText->SetText(FText::FromString("Faction"));
			UGridSlot * Slot_FactionText = CastChecked<UGridSlot>(FactionText->Slot);
			Slot_FactionText->SetColumn(2);
			/* Add text to label the 'starting spot' column */
			UTextBlock * StartingSpotText = WidgetTree->ConstructWidget<UTextBlock>();
			Grid_CPUPlayers->AddChildToGrid(StartingSpotText);
			StartingSpotText->SetJustification(ETextJustify::Center);
			StartingSpotText->SetText(FText::FromString("Starting Spot"));
			UGridSlot * Slot_StartingSpotText = CastChecked<UGridSlot>(StartingSpotText->Slot);
			Slot_StartingSpotText->SetColumn(3);
			/* Add text to label the CPU difficulty column */
			UTextBlock * CPUDifficultyText = WidgetTree->ConstructWidget<UTextBlock>();
			Grid_CPUPlayers->AddChildToGrid(CPUDifficultyText);
			CPUDifficultyText->SetJustification(ETextJustify::Center);
			CPUDifficultyText->SetText(FText::FromString("CPU Difficulty"));
			UGridSlot * Slot_CPUDifficultyText = CastChecked<UGridSlot>(CPUDifficultyText->Slot);
			Slot_CPUDifficultyText->SetColumn(4);
		}

		//-----------------------------------------------------------------------------
		//	Defeat condition Combo box
		//-----------------------------------------------------------------------------

		ComboBox_DefeatCondition = WidgetTree->ConstructWidget<UComboBoxString>();
		RootWidget->AddChild(ComboBox_DefeatCondition);
		UCanvasPanelSlot * Slot_ComboBox_DefeatCondition = CastChecked<UCanvasPanelSlot>(ComboBox_DefeatCondition->Slot);
		Slot_ComboBox_DefeatCondition->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
		Slot_ComboBox_DefeatCondition->SetPosition(Slot_ComboBox_Skipping->GetPosition() + FVector2D(Slot_ComboBox_Skipping->GetSize().X, 0.f));
		Slot_ComboBox_DefeatCondition->SetSize(FVector2D(300.f, 50.f));
		Slot_ComboBox_DefeatCondition->SetAlignment(FVector2D(0.0f, 0.0f));

		//-----------------------------------------------------------------------------
		//	Text above Defeat condition Combo box
		//-----------------------------------------------------------------------------

		UTextBlock * DefeatCondishText = WidgetTree->ConstructWidget<UTextBlock>();
		RootWidget->AddChild(DefeatCondishText);
		DefeatCondishText->SetText(FText::FromString("Defeat Condition"));
		UCanvasPanelSlot * Slot_DefeatCondishText = CastChecked<UCanvasPanelSlot>(DefeatCondishText->Slot);
		Slot_DefeatCondishText->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
		Slot_DefeatCondishText->SetPosition(Slot_ComboBox_DefeatCondition->GetPosition() + FVector2D(0.f, -50.f));
		Slot_DefeatCondishText->SetSize(Slot_ComboBox_DefeatCondition->GetSize());
		Slot_DefeatCondishText->SetAlignment(FVector2D(0.0f, 0.0f));

		//-----------------------------------------------------------------------------
		//	Cheat widget check box
		//-----------------------------------------------------------------------------

		CheckBox_CheatWidget = WidgetTree->ConstructWidget<UCheckBox>();
		RootWidget->AddChild(CheckBox_CheatWidget);
		CheckBox_CheatWidget->SetRenderScale(FVector2D(3.f, 3.f));
		UCanvasPanelSlot * Slot_CheckBox_CheatWidget = CastChecked<UCanvasPanelSlot>(CheckBox_CheatWidget->Slot);
		Slot_CheckBox_CheatWidget->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
		Slot_CheckBox_CheatWidget->SetPosition(Slot_ComboBox_DefeatCondition->GetPosition() + FVector2D(Slot_ComboBox_DefeatCondition->GetSize().X + 40.f, 20.f));
		Slot_CheckBox_CheatWidget->SetAutoSize(true);
		Slot_CheckBox_CheatWidget->SetAlignment(FVector2D(0.0f, 0.0f));

		//-----------------------------------------------------------------------------
		//	Cheat widget check box text
		//-----------------------------------------------------------------------------

		UTextBlock * CheatWidgetText = WidgetTree->ConstructWidget<UTextBlock>();
		RootWidget->AddChild(CheatWidgetText);
		CheatWidgetText->SetText(FText::FromString("Show Cheat Widget"));
		UCanvasPanelSlot * Slot_CheatWidgetText = CastChecked<UCanvasPanelSlot>(CheatWidgetText->Slot);
		Slot_CheatWidgetText->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
		Slot_CheatWidgetText->SetPosition(Slot_CheckBox_CheatWidget->GetPosition() + FVector2D(0.f, -70.f));
		Slot_CheatWidgetText->SetSize(Slot_CheckBox_CheatWidget->GetSize());
		Slot_CheatWidgetText->SetAlignment(FVector2D(0.0f, 0.0f));

		//-----------------------------------------------------------------------------
		//	Cheat widget combo box
		//-----------------------------------------------------------------------------

		ComboBox_CheatWidget = WidgetTree->ConstructWidget<UComboBoxString>();
		RootWidget->AddChild(ComboBox_CheatWidget);
		UCanvasPanelSlot * Slot_ComboBox_CheatWidget = CastChecked<UCanvasPanelSlot>(ComboBox_CheatWidget->Slot);
		Slot_ComboBox_CheatWidget->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
		Slot_ComboBox_CheatWidget->SetPosition(Slot_CheckBox_CheatWidget->GetPosition() + FVector2D(Slot_CheckBox_CheatWidget->GetSize().X + 300.f, 0.f));
		Slot_ComboBox_CheatWidget->SetSize(FVector2D(200.f, 50.f));
		Slot_ComboBox_CheatWidget->SetAlignment(FVector2D(0.0f, 0.0f));
	}

	//-------------------------------------------------------------------------------------
	//=====================================================================================
	//	------- Setting values -------
	//=====================================================================================
	//-------------------------------------------------------------------------------------

	if (true /* TODO replace true with this func and implement it somehow IsFirstTimeEverOpeningProject()*/)
	{
		/* These values are only used the first time ever starting your project.
		In future whatever you had when the editor closes/crashes will be remembered */
		SkippingOption = EEditorPlaySkippingOption::SkipMainMenu;
		DefeatCondition = EDefeatCondition::NoCondition;
		NumCPUPlayers = 0;
		bInitiallyShowCheatWidget = false;
		CheatWidget_BP = UInMatchDeveloperWidget::StaticClass();
		InvalidHumanOwnerRule = EInvalidOwnerIndexAction::AssignToServerPlayer;
		InvalidCPUOwnerRule = EInvalidOwnerIndexAction::DoNotSpawn;
	}
	else
	{
		/* TODO Should remember what the settings were last time player played and use those */
	}


	//-------------------------------------------------------------------------------------
	//=====================================================================================
	//	------- Setting up each widget -------
	//=====================================================================================
	//-------------------------------------------------------------------------------------

	//------------------------------------------------------------------
	//	Skipping
	//------------------------------------------------------------------

	if (IsWidgetBound(ComboBox_Skipping))
	{
		/* Add all options */
		const int32 MaxEnumValue = static_cast<int32>(EEditorPlaySkippingOption::z_ALWAYS_LAST_IN_ENUM);
		for (int32 i = 0; i < MaxEnumValue; ++i)
		{
			ComboBox_Skipping->AddOption(GetString(static_cast<EEditorPlaySkippingOption>(i)));
		}

		/* Set option */
		ComboBox_Skipping->SetSelectedOption(GetString(SkippingOption));

		/* On option changed binding */
		ComboBox_Skipping->OnSelectionChanged.AddDynamic(this, &UEditorPlaySettingsWidget::UIBinding_OnSkippingComboBoxOptionChanged);
	}
	if (IsWidgetBound(Text_SkippingOption))
	{
		Text_SkippingOption->SetText(GetText(SkippingOption));
	}
	if (IsWidgetBound(Button_AdjustSkippingLeft))
	{
		Button_AdjustSkippingLeft->OnClicked.AddDynamic(this, &UEditorPlaySettingsWidget::UIBinding_OnAdjustSkippingOptionLeftButtonClicked);
	}
	if (IsWidgetBound(Button_AdjustSkippingRight))
	{
		Button_AdjustSkippingLeft->OnClicked.AddDynamic(this, &UEditorPlaySettingsWidget::UIBinding_OnAdjustSkippingOptionRightButtonClicked);
	}


	//------------------------------------------------------------------
	//	Player Configuration
	//------------------------------------------------------------------

	UWorld * World = GEditor->GetEditorWorldContext().World();

	if (IsWidgetBound(Grid_HumanPlayers))
	{
		HumanPlayerInfo.Reserve(ProjectSettings::MAX_NUM_PLAYERS);
		HumanPlayerInfoCopy.Reserve(ProjectSettings::MAX_NUM_PLAYERS);
		/* Populate Panel_HumanPlayers */
		for (int32 i = 0; i < ProjectSettings::MAX_NUM_PLAYERS; ++i)
		{
			CreateSingleHumanPlayerConfigurationWidget(i);
		}
	}
	if (IsWidgetBound(Grid_CPUPlayers))
	{
		CPUPlayerInfo.Reserve(ProjectSettings::MAX_NUM_PLAYERS);
		CPUPlayerInfoCopy.Reserve(ProjectSettings::MAX_NUM_PLAYERS);
		/* Populate Panel_CPUPlayers */
		for (int32 i = 0; i < ProjectSettings::MAX_NUM_PLAYERS; ++i)
		{
			CreateSingleCPUPlayerConfigurationWidget(i);
		}

		/* Set position of CPU player panel. Cause it's position is relative to the human 
		player panel we do this now after the human panel has been populated with everything 
		it will have */
		UCanvasPanelSlot * Slot_Panel_HumanPlayers = CastChecked<UCanvasPanelSlot>(Grid_HumanPlayers->Slot);
		UCanvasPanelSlot * Slot_Panel_CPUPlayers = CastChecked<UCanvasPanelSlot>(Grid_CPUPlayers->Slot);
		Slot_Panel_CPUPlayers->SetPosition(Slot_Panel_HumanPlayers->GetPosition() + FVector2D(Slot_Panel_HumanPlayers->GetSize().X, 0.f));
	}
	if (IsWidgetBound(ComboBox_NumCPUPlayers))
	{
		/* Add options */
		for (int32 i = 0; i <= ProjectSettings::MAX_NUM_PLAYERS; ++i)
		{
			ComboBox_NumCPUPlayers->AddOption(FString::FromInt(i));
		}

		/* Set selected option */
		ComboBox_NumCPUPlayers->SetSelectedOption(FString::FromInt(NumCPUPlayers));

		/* Binding */
		ComboBox_NumCPUPlayers->OnSelectionChanged.AddDynamic(this, &UEditorPlaySettingsWidget::UIBinding_OnNumCPUPlayersComboBoxOptionChanged);
	}
	if (IsWidgetBound(Text_NumCPUPlayers))
	{
		Text_NumCPUPlayers->SetText(FText::AsNumber(NumCPUPlayers));
	}
	if (IsWidgetBound(Button_DecreaseNumCPUPlayers))
	{
		Button_DecreaseNumCPUPlayers->OnClicked.AddDynamic(this, &UEditorPlaySettingsWidget::UIBinding_OnDecreaseNumCPUPlayersButtonClicked);
	}
	if (IsWidgetBound(Button_IncreaseNumCPUPlayers))
	{
		Button_IncreaseNumCPUPlayers->OnClicked.AddDynamic(this, &UEditorPlaySettingsWidget::UIBinding_OnIncreaseNumCPUPlayersButtonClicked);
	}

	//------------------------------------------------------------------
	//	Starting Resources Configuration
	//------------------------------------------------------------------

	// TODO


	//------------------------------------------------------------------
	//	Defeat Condition 
	//------------------------------------------------------------------

	if (IsWidgetBound(ComboBox_DefeatCondition))
	{
		/* Add all options */
		const int32 MaxEnumValue = static_cast<int32>(EDefeatCondition::z_ALWAYS_LAST_IN_ENUM) - 1;
		for (int32 i = 0; i < MaxEnumValue; ++i)
		{
			ComboBox_DefeatCondition->AddOption(GetString(static_cast<EDefeatCondition>(i + 1)));
		}

		/* Set selected option */
		ComboBox_DefeatCondition->SetSelectedOption(GetString(DefeatCondition));

		/* On option changed binding */
		ComboBox_DefeatCondition->OnSelectionChanged.AddDynamic(this, &UEditorPlaySettingsWidget::UIBinding_OnDefeatConditionComboBoxOptionChanged);
	}
	if (IsWidgetBound(Text_DefeatCondition))
	{
		Text_DefeatCondition->SetText(GetText(DefeatCondition));
	}
	if (IsWidgetBound(Button_AdjustDefeatConditionLeft))
	{
		Button_AdjustDefeatConditionLeft->OnClicked.AddDynamic(this, &UEditorPlaySettingsWidget::UIBinding_OnAdjustDefeatConditionLeftButtonClicked);
	}
	if (IsWidgetBound(Button_AdjustDefeatConditionRight))
	{
		Button_AdjustDefeatConditionRight->OnClicked.AddDynamic(this, &UEditorPlaySettingsWidget::UIBinding_OnAdjustDefeatConditionRightButtonClicked);
	}


	//------------------------------------------------------------------
	//	Cheat Widget 
	//------------------------------------------------------------------

	if (IsWidgetBound(CheckBox_CheatWidget))
	{
		CheckBox_CheatWidget->SetIsChecked(bInitiallyShowCheatWidget);

		CheckBox_CheatWidget->OnCheckStateChanged.AddDynamic(this, &UEditorPlaySettingsWidget::UIBinding_OnShowCheatWidgetCheckBoxChanged);
	}
	if (IsWidgetBound(ComboBox_CheatWidget))
	{
		/* Add options. Just a note: took this method from 
		https://forums.unrealengine.com/development-discussion/c-gameplay-programming/13263-possible-to-loop-through-all-blueprint-sub-classes 
		It says something about 'they may not be loaded into memory". If that's an issue then 
		do something about it.
		Another side note: I'm balls deep trying to get code to compile right now so I cannot test 
		it but the ctor for TObjectIterator has some params that may be able to get the class 
		default object without me having to check with the if statement below. If this is the case 
		then is it better performing than the loop I have written right now? Or is it the same? */
		for (TObjectIterator<UInMatchDeveloperWidget> Iter; Iter; ++Iter)
		{
			if (Iter->HasAnyFlags(RF_ClassDefaultObject))
			{
				UClass * Class = Iter->GetClass();
				ComboBox_CheatWidget->AddOption(CreateHumanReadableNameForUClassName(Class->GetName()));
			}
		}
		
		/* Set current option */
		ComboBox_CheatWidget->SetSelectedOption(CreateHumanReadableNameForUClassName(CheatWidget_BP->GetClass()->GetName()));

		/* Setup binding */
		ComboBox_CheatWidget->OnSelectionChanged.AddDynamic(this, &UEditorPlaySettingsWidget::UIBinding_OnCheatWidgetComboBoxOptionChanged);
	}


	//------------------------------------------------------------------
	//	Invalid Owner Widgets 
	//------------------------------------------------------------------

	if (IsWidgetBound(ComboBox_InvalidHumanOwnerRule))
	{
		/* Add options */
		for (uint8 i = 0; i < Statics::NUM_PIE_SESSION_INVALID_OWNER_RULES; ++i)
		{
			ComboBox_InvalidHumanOwnerRule->AddOption(GetString(Statics::ArrayIndexToPIESeshInvalidOwnerRule(i)));
		}

		/* Set selected option */
		ComboBox_InvalidHumanOwnerRule->SetSelectedOption(GetString(InvalidHumanOwnerRule));

		/* Binding */
		ComboBox_InvalidHumanOwnerRule->OnSelectionChanged.AddDynamic(this, &UEditorPlaySettingsWidget::UIBinding_OnInvalidHumanOwnerRuleComboxBoxOptionChanged);
	}

	if (IsWidgetBound(ComboBox_InvalidCPUOwnerRule))
	{
		/* Add options */
		for (uint8 i = 0; i < Statics::NUM_PIE_SESSION_INVALID_OWNER_RULES; ++i)
		{
			ComboBox_InvalidCPUOwnerRule->AddOption(GetString(Statics::ArrayIndexToPIESeshInvalidOwnerRule(i)));
		}

		/* Set selected option */
		ComboBox_InvalidCPUOwnerRule->SetSelectedOption(GetString(InvalidCPUOwnerRule));

		/* Binding */
		ComboBox_InvalidCPUOwnerRule->OnSelectionChanged.AddDynamic(this, &UEditorPlaySettingsWidget::UIBinding_OnInvalidCPUOwnerRuleComboxBoxOptionChanged);
	}

	return Ref;
}

void UEditorPlaySettingsWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
}

void UEditorPlaySettingsWidget::CreateSingleHumanPlayerConfigurationWidget(int32 PlayerIndex)
{
	//--------------------------------------------------------------
	/* Create the 'proxy' UObject for the bindings */
	//--------------------------------------------------------------
	UPIEHumanPlayerInfoProxy * ProxyObject = NewObject<UPIEHumanPlayerInfoProxy>();
	ProxyObject->PlayerIndex = PlayerIndex;
	ProxyObject->EnumStringObject = EnumStringObject;
	ProxyObject->PlaySettingsWidget = this;
	HumanPlayerInfo.Emplace(ProxyObject);
	HumanPlayerInfoCopy.Emplace(ProxyObject->PlayerInfo);

	//--------------------------------------------------------------
	/* Create widgets */
	//--------------------------------------------------------------

	/* Text to show the player's index */
	ProxyObject->Text_PlayerIndex = WidgetTree->ConstructWidget<UTextBlock>();
	Grid_HumanPlayers->AddChild(ProxyObject->Text_PlayerIndex);
	ProxyObject->Text_PlayerIndex->SetText(FText::AsNumber(PlayerIndex + 1));
	UGridSlot * Slot_Text_PlayerIndex = CastChecked<UGridSlot>(ProxyObject->Text_PlayerIndex->Slot);
	/* +1 cause the first row labels each column */
	Slot_Text_PlayerIndex->SetRow(PlayerIndex + 1);
	Slot_Text_PlayerIndex->SetColumn(0);
	Slot_Text_PlayerIndex->SetPadding(FMargin(10.f, 3.f));

	/* Combo box to show/select team */
	ProxyObject->ComboBox_Team = WidgetTree->ConstructWidget<UComboBoxString>();
	Grid_HumanPlayers->AddChild(ProxyObject->ComboBox_Team);
	UGridSlot * Slot_ComboBox_Team = CastChecked<UGridSlot>(ProxyObject->ComboBox_Team->Slot);
	Slot_ComboBox_Team->SetRow(PlayerIndex + 1);
	Slot_ComboBox_Team->SetColumn(1);
	Slot_ComboBox_Team->SetPadding(FMargin(10.f, 3.f));

	/* Combo box to show/select faction */
	ProxyObject->ComboBox_Faction = WidgetTree->ConstructWidget<UComboBoxString>();
	Grid_HumanPlayers->AddChild(ProxyObject->ComboBox_Faction);
	UGridSlot * Slot_ComboBox_Faction = CastChecked<UGridSlot>(ProxyObject->ComboBox_Faction->Slot);
	Slot_ComboBox_Faction->SetRow(PlayerIndex + 1);
	Slot_ComboBox_Faction->SetColumn(2);
	Slot_ComboBox_Faction->SetPadding(FMargin(10.f, 3.f));

	/* Text for starting spot */
	ProxyObject->Text_StartingSpot = WidgetTree->ConstructWidget<UEditableTextBox>();
	Grid_HumanPlayers->AddChild(ProxyObject->Text_StartingSpot);
	ProxyObject->Text_StartingSpot->WidgetStyle.SetFont(FSlateFontInfo("Roboto", 24));
	UGridSlot * Slot_Text_StartingSpot = CastChecked<UGridSlot>(ProxyObject->Text_StartingSpot->Slot);
	Slot_Text_StartingSpot->SetRow(PlayerIndex + 1);
	Slot_Text_StartingSpot->SetColumn(3);
	Slot_Text_StartingSpot->SetPadding(FMargin(10.f, 3.f));

	//--------------------------------------------------------------
	/* Setting values on widgets + bindings */
	//--------------------------------------------------------------

	/* Probably want to have loaded the disk variables by now if this isn't the first time
	starting game (by that I mean ProxyObject->PlayerInfo and ProxyObject->PlayerIndex) */

	{
		//------------------------------------------------------------
		//	ComboBox_Team
		//------------------------------------------------------------

		/* Add options */
		for (int32 i = 0; i < ProjectSettings::MAX_NUM_TEAMS; ++i)
		{
			ProxyObject->ComboBox_Team->AddOption(GetString(Statics::ArrayIndexToTeam(i)));
		}
		ProxyObject->ComboBox_Team->AddOption(FString(GetString(ETeam::Observer)));

		/* Set selected option */
		ProxyObject->ComboBox_Team->SetSelectedOption(GetString(ProxyObject->PlayerInfo.GetTeam()));

		/* Binding */
		ProxyObject->ComboBox_Team->OnSelectionChanged.AddDynamic(ProxyObject, &UPIEHumanPlayerInfoProxy::UIBinding_OnTeamComboBoxSelectionChanged);
	}
	
	{
		//------------------------------------------------------------
		//	ComboBox_Faction
		//------------------------------------------------------------
		
		/* Add options */
		for (int32 i = 0; i < Statics::NUM_FACTIONS; ++i)
		{
			ProxyObject->ComboBox_Faction->AddOption(GetString(Statics::ArrayIndexToFaction(i)));
		}

		/* Set selected option - the first faction defined by user. Will just do nothing
		if there is none */
		ProxyObject->ComboBox_Faction->SetSelectedOption(GetString(ProxyObject->PlayerInfo.GetFaction()));

		/* Binding */
		ProxyObject->ComboBox_Faction->OnSelectionChanged.AddDynamic(ProxyObject, &UPIEHumanPlayerInfoProxy::UIBinding_OnFactionComboBoxSelectionChanged);
	}
	
	{
		//------------------------------------------------------------
		//	Text_StartingSpot
		//------------------------------------------------------------
		
		/* Set default starting spot */
		ProxyObject->Text_StartingSpot->SetText(FText::FromString(GetString_StartingSpot(ProxyObject->PlayerInfo.GetStartingSpot())));

		/* Binding */
		ProxyObject->Text_StartingSpot->OnTextChanged.AddDynamic(ProxyObject, &UPIEHumanPlayerInfoProxy::UIBinding_OnStartingSpotTextChanged);
		ProxyObject->Text_StartingSpot->OnTextCommitted.AddDynamic(ProxyObject, &UPIEHumanPlayerInfoProxy::UIBinding_OnStartingSpotTextCommitted);
	}
}

void UEditorPlaySettingsWidget::CreateSingleCPUPlayerConfigurationWidget(int32 PlayerIndex)
{
	//--------------------------------------------------------------
	/* Create the 'proxy' UObject for the bindings */
	//--------------------------------------------------------------
	UPIECPUPlayerInfoProxy * ProxyObject = NewObject<UPIECPUPlayerInfoProxy>();
	ProxyObject->PlayerIndex = PlayerIndex;
	ProxyObject->EnumStringObject = EnumStringObject;
	ProxyObject->PlaySettingsWidget = this;
	CPUPlayerInfo.Emplace(ProxyObject);
	CPUPlayerInfoCopy.Emplace(ProxyObject->PlayerInfo);

	//--------------------------------------------------------------
	/* Create widgets */
	//--------------------------------------------------------------

	/* Text to show the player's index */
	ProxyObject->Text_PlayerIndex = WidgetTree->ConstructWidget<UTextBlock>();
	Grid_CPUPlayers->AddChild(ProxyObject->Text_PlayerIndex);
	ProxyObject->Text_PlayerIndex->SetText(FText::AsNumber(PlayerIndex + 1));
	UGridSlot * Slot_Text_PlayerIndex = CastChecked<UGridSlot>(ProxyObject->Text_PlayerIndex->Slot);
	/* +1 cause the first row labels each column */
	Slot_Text_PlayerIndex->SetRow(PlayerIndex + 1);
	Slot_Text_PlayerIndex->SetColumn(0);
	Slot_Text_PlayerIndex->SetPadding(FMargin(10.f, 3.f));

	/* Combo box to show/select team */
	ProxyObject->ComboBox_Team = WidgetTree->ConstructWidget<UComboBoxString>();
	Grid_CPUPlayers->AddChild(ProxyObject->ComboBox_Team);
	UGridSlot * Slot_ComboBox_Team = CastChecked<UGridSlot>(ProxyObject->ComboBox_Team->Slot);
	Slot_ComboBox_Team->SetRow(PlayerIndex + 1);
	Slot_ComboBox_Team->SetColumn(1);
	Slot_ComboBox_Team->SetPadding(FMargin(10.f, 3.f));

	/* Combo box to show/select faction */
	ProxyObject->ComboBox_Faction = WidgetTree->ConstructWidget<UComboBoxString>();
	Grid_CPUPlayers->AddChild(ProxyObject->ComboBox_Faction);
	UGridSlot * Slot_ComboBox_Faction = CastChecked<UGridSlot>(ProxyObject->ComboBox_Faction->Slot);
	Slot_ComboBox_Faction->SetRow(PlayerIndex + 1);
	Slot_ComboBox_Faction->SetColumn(2);
	Slot_ComboBox_Faction->SetPadding(FMargin(10.f, 3.f));

	/* Text for starting spot */
	ProxyObject->Text_StartingSpot = WidgetTree->ConstructWidget<UEditableTextBox>();
	Grid_CPUPlayers->AddChild(ProxyObject->Text_StartingSpot);
	ProxyObject->Text_StartingSpot->WidgetStyle.SetFont(FSlateFontInfo("Roboto", 24));
	UGridSlot * Slot_Text_StartingSpot = CastChecked<UGridSlot>(ProxyObject->Text_StartingSpot->Slot);
	Slot_Text_StartingSpot->SetRow(PlayerIndex + 1);
	Slot_Text_StartingSpot->SetColumn(3);
	Slot_Text_StartingSpot->SetPadding(FMargin(10.f, 3.f));

	/* Combo box to show/select CPU difficulty */
	ProxyObject->ComboBox_CPUDifficulty = WidgetTree->ConstructWidget<UComboBoxString>();
	Grid_CPUPlayers->AddChild(ProxyObject->ComboBox_CPUDifficulty);
	UGridSlot * Slot_ComboBox_CPUDifficulty = CastChecked<UGridSlot>(ProxyObject->ComboBox_CPUDifficulty->Slot);
	Slot_ComboBox_CPUDifficulty->SetRow(PlayerIndex + 1);
	Slot_ComboBox_CPUDifficulty->SetColumn(4);
	Slot_ComboBox_CPUDifficulty->SetPadding(FMargin(10.f, 3.f));

	//--------------------------------------------------------------
	/* Setting values on widgets + bindings */
	//--------------------------------------------------------------

	/* Probably want to have loaded the disk variables by now if this isn't the first time
	starting game (by that I mean ProxyObject->PlayerInfo and ProxyObject->PlayerIndex) */

	{
		//------------------------------------------------------------
		//	ComboBox_Team
		//------------------------------------------------------------

		/* Add options */
		for (int32 i = 0; i < ProjectSettings::MAX_NUM_TEAMS; ++i)
		{
			ProxyObject->ComboBox_Team->AddOption(GetString(Statics::ArrayIndexToTeam(i)));
		}
		ProxyObject->ComboBox_Team->AddOption(FString(GetString(ETeam::Observer)));

		/* Set selected option */
		ProxyObject->ComboBox_Team->SetSelectedOption(GetString(ProxyObject->PlayerInfo.GetTeam()));

		/* Binding */
		ProxyObject->ComboBox_Team->OnSelectionChanged.AddDynamic(ProxyObject, &UPIECPUPlayerInfoProxy::UIBinding_OnTeamComboBoxSelectionChanged);
	}

	{
		//------------------------------------------------------------
		//	ComboBox_Faction
		//------------------------------------------------------------

		/* Add options */
		for (int32 i = 0; i < Statics::NUM_FACTIONS; ++i)
		{
			ProxyObject->ComboBox_Faction->AddOption(GetString(Statics::ArrayIndexToFaction(i)));
		}

		/* Set selected option - the first faction defined by user. Will just do nothing
		if there is none */
		ProxyObject->ComboBox_Faction->SetSelectedOption(GetString(ProxyObject->PlayerInfo.GetFaction()));

		/* Binding */
		ProxyObject->ComboBox_Faction->OnSelectionChanged.AddDynamic(ProxyObject, &UPIECPUPlayerInfoProxy::UIBinding_OnFactionComboBoxSelectionChanged);
	}

	{
		//------------------------------------------------------------
		//	Text_StartingSpot
		//------------------------------------------------------------

		/* Set default starting spot */
		ProxyObject->Text_StartingSpot->SetText(FText::FromString(GetString_StartingSpot(ProxyObject->PlayerInfo.GetStartingSpot())));

		/* Binding */
		ProxyObject->Text_StartingSpot->OnTextChanged.AddDynamic(ProxyObject, &UPIECPUPlayerInfoProxy::UIBinding_OnStartingSpotTextChanged);
		ProxyObject->Text_StartingSpot->OnTextCommitted.AddDynamic(ProxyObject, &UPIECPUPlayerInfoProxy::UIBinding_OnStartingSpotTextCommitted);
	}

	{
		//------------------------------------------------------------
		//	ComboBox_CPUDifficulty
		//------------------------------------------------------------

		/* Add options to ComboBox_CPUDifficulty */
		ProxyObject->ComboBox_CPUDifficulty->AddOption(GetString(ECPUDifficulty::DoesNothing));
		for (int32 i = 0; i < Statics::NUM_CPU_DIFFICULTIES; ++i)
		{
			const ECPUDifficulty EnumValue = Statics::ArrayIndexToCPUDifficulty(i);
			ProxyObject->ComboBox_CPUDifficulty->AddOption(GetString(EnumValue));
		}
		ProxyObject->ComboBox_CPUDifficulty->SetSelectedOption(GetString(ProxyObject->PlayerInfo.GetDifficulty()));

		ProxyObject->ComboBox_CPUDifficulty->OnSelectionChanged.AddDynamic(ProxyObject, &UPIECPUPlayerInfoProxy::UIBinding_OnCPUDifficultyComboBoxSelectionChanged);
	}
}

void UEditorPlaySettingsWidget::GetTextSize(const UFont * Font, float ScaleX, float ScaleY, int32 & XL, int32 & YL, const TCHAR * Text)
{
	int32 XLi, YLi;
	UCanvas::ClippedStrLen(Font, ScaleX, ScaleY, XLi, YLi, Text);
	XL = XLi;
	YL = YLi;
}

FText UEditorPlaySettingsWidget::GetText(EEditorPlaySkippingOption InSkippingOption) const
{
	return FText::FromString(GetString(InSkippingOption));
}

FString UEditorPlaySettingsWidget::GetString(EEditorPlaySkippingOption InSkippingOption) const
{
	return EnumStringObject->GetString(InSkippingOption);
}

FText UEditorPlaySettingsWidget::GetText(EDefeatCondition InDefeatCondition) const
{
	return FText::FromString(GetString(InDefeatCondition));
}

FString UEditorPlaySettingsWidget::GetString(EDefeatCondition InDefeatCondition) const
{
	return EnumStringObject->GetString(InDefeatCondition);
}

FText UEditorPlaySettingsWidget::GetText(EInvalidOwnerIndexAction InRule) const
{
	return FText::FromString(GetString(InRule));
}

FString UEditorPlaySettingsWidget::GetString(EInvalidOwnerIndexAction InRule) const
{
	return EnumStringObject->GetString(InRule);
}

FString UEditorPlaySettingsWidget::GetString(ETeam InTeam) const
{
	return EnumStringObject->GetString(InTeam);
}

FString UEditorPlaySettingsWidget::GetString(EFaction InFaction) const
{
	return EnumStringObject->GetString(InFaction);
}

FString UEditorPlaySettingsWidget::GetString_StartingSpot(int16 InStartingSpot) const
{
	if (InStartingSpot == -1)
	{
		return "Random";
	}

	return FString::FromInt(InStartingSpot);
}

FString UEditorPlaySettingsWidget::GetString(ECPUDifficulty InCPUDifficulty) const
{
	return EnumStringObject->GetString(InCPUDifficulty);
}

bool UEditorPlaySettingsWidget::SetSkippingOptionFromString(const FString & String)
{
	assert(EnumStringObject != nullptr);
	return EnumStringObject->GetEnumValueFromStringSlow_EditorPlaySkippingOption(String, SkippingOption);
}

bool UEditorPlaySettingsWidget::SetDefeatConditionFromString(const FString & String)
{
	return EnumStringObject->GetEnumValueFromStringSlow_DefeatCondition(String, DefeatCondition);
}

bool UEditorPlaySettingsWidget::SetNumCPUPlayersFromString(const FString & String)
{
	const int32 LocalNumCPUPlayers = FCString::Atoi(*String);
	if (LocalNumCPUPlayers >= 0 && LocalNumCPUPlayers <= ProjectSettings::MAX_NUM_PLAYERS)
	{
		NumCPUPlayers = LocalNumCPUPlayers;
		return true;
	}
	 
	return false;
}

bool UEditorPlaySettingsWidget::SetCheatWidgetBPFromString(const FString & String)
{
	UInMatchDeveloperWidget * DefaultObject = FindObject<UInMatchDeveloperWidget>(ANY_PACKAGE, *CreateUClassNameFromHumanReadableName(String));
	if (DefaultObject != nullptr)
	{
		CheatWidget_BP = DefaultObject->GetClass();
		return true;
	}

	return false;
}

bool UEditorPlaySettingsWidget::SetInvalidHumanOwnerRuleFromString(const FString & String)
{
	return EnumStringObject->GetEnumValueFromStringSlow_PIEPlayInvalidOwnerRule(String, InvalidHumanOwnerRule);
}

bool UEditorPlaySettingsWidget::SetInvalidCPUOwnerRuleFromString(const FString & String)
{
	return EnumStringObject->GetEnumValueFromStringSlow_PIEPlayInvalidOwnerRule(String, InvalidCPUOwnerRule);
}

FString UEditorPlaySettingsWidget::CreateHumanReadableNameForUClassName(const FString & UClassGetNameResult)
{
	/* Remove the "_C" part */
	return UClassGetNameResult.LeftChop(2);
}

FString UEditorPlaySettingsWidget::CreateUClassNameFromHumanReadableName(const FString & HumanReadableName)
{
	return HumanReadableName + "_C";
}

void UEditorPlaySettingsWidget::SetSlotAnchors(UCanvasPanelSlot * Slot)
{
	Slot->SetAnchors(FAnchors(0.f, 0.f, 0.f, 0.f));
}

void UEditorPlaySettingsWidget::SetSlotAlignment(UCanvasPanelSlot * Slot)
{
	Slot->SetAlignment(FVector2D(0.f, 0.f));
}

void UEditorPlaySettingsWidget::UIBinding_OnSkippingComboBoxOptionChanged(FString Item, ESelectInfo::Type SelectionType)
{
	/* Set SkippingOption */
	SetSkippingOptionFromString(Item);

	/* Update rest of UI */
	if (IsWidgetBound(Text_SkippingOption))
	{
		Text_SkippingOption->SetText(GetText(SkippingOption));
	}
}

void UEditorPlaySettingsWidget::UIBinding_OnAdjustSkippingOptionLeftButtonClicked()
{
	/* Check if not already at minimum value */
	if (static_cast<int32>(SkippingOption) > 0)
	{
		/* Set SkippingOption */
		DecrementEnum(SkippingOption);

		/* Update rest of UI */
		if (IsWidgetBound(Text_SkippingOption))
		{
			Text_SkippingOption->SetText(GetText(SkippingOption));
		}
		if (IsWidgetBound(ComboBox_Skipping))
		{
			/* Doing this might call UIBinding_OnSkippingComboBoxOptionChanged. Doesn't matter though */
			ComboBox_Skipping->SetSelectedOption(GetString(SkippingOption));
		}
	}
}

void UEditorPlaySettingsWidget::UIBinding_OnAdjustSkippingOptionRightButtonClicked()
{
	/* Check if not already at maximum value */
	if (static_cast<int32>(SkippingOption) < static_cast<int32>(EEditorPlaySkippingOption::z_ALWAYS_LAST_IN_ENUM) - 1)
	{
		/* Set SkippingOption */
		IncrementEnum(SkippingOption);

		/* Update rest of UI */
		if (IsWidgetBound(Text_SkippingOption))
		{
			Text_SkippingOption->SetText(GetText(SkippingOption));
		}
		if (IsWidgetBound(ComboBox_Skipping))
		{
			/* Doing this might call UIBinding_OnSkippingComboBoxOptionChanged. Doesn't matter though */
			ComboBox_Skipping->SetSelectedOption(GetString(SkippingOption));
		}
	}
}

void UEditorPlaySettingsWidget::UIBinding_OnDefeatConditionComboBoxOptionChanged(FString Item, ESelectInfo::Type SelectionType)
{
	/* Set DefeatCondition */
	SetDefeatConditionFromString(Item);

	/* Update rest of UI */
	if (IsWidgetBound(Text_DefeatCondition))
	{
		Text_DefeatCondition->SetText(GetText(DefeatCondition));
	}
}

void UEditorPlaySettingsWidget::UIBinding_OnAdjustDefeatConditionLeftButtonClicked()
{
	/* Check if not already at minimum value */
	if (static_cast<int32>(DefeatCondition) > 1)
	{
		/* Set DefeatCondition */
		DecrementEnum(DefeatCondition);

		/* Update rest of UI */
		if (IsWidgetBound(Text_DefeatCondition))
		{
			Text_DefeatCondition->SetText(GetText(DefeatCondition));
		}
		if (IsWidgetBound(ComboBox_DefeatCondition))
		{
			ComboBox_DefeatCondition->SetSelectedOption(GetString(DefeatCondition));
		}
	}
}

void UEditorPlaySettingsWidget::UIBinding_OnAdjustDefeatConditionRightButtonClicked()
{	
	/* Check if not already at maximum value */
	if (static_cast<int32>(DefeatCondition) < static_cast<int32>(EDefeatCondition::z_ALWAYS_LAST_IN_ENUM) - 1)
	{
		/* Set DefeatCondition */
		IncrementEnum(DefeatCondition);

		/* Update rest of UI */
		if (IsWidgetBound(Text_DefeatCondition))
		{
			Text_DefeatCondition->SetText(GetText(DefeatCondition));
		}
		if (IsWidgetBound(ComboBox_DefeatCondition))
		{
			ComboBox_DefeatCondition->SetSelectedOption(GetString(DefeatCondition));
		}
	}
}

void UEditorPlaySettingsWidget::UIBinding_OnNumCPUPlayersComboBoxOptionChanged(FString Item, ESelectInfo::Type SelectionType)
{
	SetNumCPUPlayersFromString(Item);

	/* Update rest of UI */
	if (IsWidgetBound(Text_NumCPUPlayers))
	{
		Text_NumCPUPlayers->SetText(FText::AsNumber(NumCPUPlayers));
	}
}

void UEditorPlaySettingsWidget::UIBinding_OnDecreaseNumCPUPlayersButtonClicked()
{
	NumCPUPlayers = FMath::Clamp<int32>(NumCPUPlayers - 1, 0, ProjectSettings::MAX_NUM_PLAYERS);

	/* Update UI */
	if (IsWidgetBound(Text_NumCPUPlayers))
	{
		Text_NumCPUPlayers->SetText(FText::AsNumber(NumCPUPlayers));
	}
	if (IsWidgetBound(ComboBox_NumCPUPlayers))
	{
		ComboBox_NumCPUPlayers->SetSelectedOption(FString::FromInt(NumCPUPlayers));
	}
}

void UEditorPlaySettingsWidget::UIBinding_OnIncreaseNumCPUPlayersButtonClicked()
{
	NumCPUPlayers = FMath::Clamp<int32>(NumCPUPlayers + 1, 0, ProjectSettings::MAX_NUM_PLAYERS);

	/* Update UI */
	if (IsWidgetBound(Text_NumCPUPlayers))
	{
		Text_NumCPUPlayers->SetText(FText::AsNumber(NumCPUPlayers));
	}
	if (IsWidgetBound(ComboBox_NumCPUPlayers))
	{
		ComboBox_NumCPUPlayers->SetSelectedOption(FString::FromInt(NumCPUPlayers));
	}
}

void UEditorPlaySettingsWidget::UIBinding_OnShowCheatWidgetCheckBoxChanged(bool bIsChecked)
{
	bInitiallyShowCheatWidget = CheckBox_CheatWidget->IsChecked();
}

void UEditorPlaySettingsWidget::UIBinding_OnCheatWidgetComboBoxOptionChanged(FString Item, ESelectInfo::Type SelectionType)
{
	/* Set CheatWidget_BP from string */
	SetCheatWidgetBPFromString(Item);
}

void UEditorPlaySettingsWidget::UIBinding_OnInvalidHumanOwnerRuleComboxBoxOptionChanged(FString Item, ESelectInfo::Type SelectionType)
{
	SetInvalidHumanOwnerRuleFromString(Item);
}

void UEditorPlaySettingsWidget::UIBinding_OnInvalidCPUOwnerRuleComboxBoxOptionChanged(FString Item, ESelectInfo::Type SelectionType)
{
	SetInvalidCPUOwnerRuleFromString(Item);
}


