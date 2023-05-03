// Fill out your copyright notice in the Description page of Project Settings.

#include "LobbyCreationWidget.h"
#include "Components/EditableText.h"
#include "Components/ComboBoxString.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

//#include "UI/MainMenu/Menus.h"
#include "Statics/Statics.h"
#include "Statics/DevelopmentStatics.h"
#include "GameFramework/RTSGameInstance.h"
#include "GameFramework/RTSGameState.h"
#include "UI/MainMenu/MatchRulesWidget.h"
#include "UI/MainMenu/MapSelection.h"


bool ULobbyCreationWidget::Setup()
{
	if (Super::Setup())
	{
		return true;
	}

	if (IsWidgetBound(Widget_MatchRules))
	{
		Widget_MatchRules->Setup();
	}

	SetupBindings();
	SetDefaultValues();

	return false;
}

void ULobbyCreationWidget::SetupBindings()
{
	if (IsWidgetBound(Text_LobbyName))
	{
		Text_LobbyName->OnTextChanged.AddDynamic(this, &ULobbyCreationWidget::UIBinding_OnLobbyNameTextChanged);
	}
	if (IsWidgetBound(Text_Password))
	{
		Text_Password->OnTextChanged.AddDynamic(this, &ULobbyCreationWidget::UIBinding_OnPasswordTextChanged);
	}
	if (IsWidgetBound(Button_Create))
	{
		Button_Create->OnClicked.AddDynamic(this, &ULobbyCreationWidget::UIBinding_OnCreateButtonClicked);
	}
	if (IsWidgetBound(Button_Return))
	{
		Button_Return->OnClicked.AddDynamic(this, &ULobbyCreationWidget::UIBinding_OnReturnButtonClicked);
	}
	if (IsWidgetBound(Button_NetworkTypeLeft))
	{
		Button_NetworkTypeLeft->OnClicked.AddDynamic(this, &ULobbyCreationWidget::UIBinding_OnNetworkTypeLeftButtonClicked);
	}
	if (IsWidgetBound(Button_NetworkTypeRight))
	{
		Button_NetworkTypeRight->OnClicked.AddDynamic(this, &ULobbyCreationWidget::UIBinding_OnNetworkTypeRightButtonClicked);
	}
	if (IsWidgetBound(Button_DecreaseNumSlots))
	{
		Button_DecreaseNumSlots->OnClicked.AddDynamic(this, &ULobbyCreationWidget::UIBinding_OnDecreaseNumSlotsButtonClicked);
	}
	if (IsWidgetBound(Button_IncreaseNumSlots))
	{
		Button_IncreaseNumSlots->OnClicked.AddDynamic(this, &ULobbyCreationWidget::UIBinding_OnIncreaseNumSlotsButtonClicked);
	}
	if (IsWidgetBound(Button_ChangeMap))
	{
		Button_ChangeMap->OnClicked.AddDynamic(this, &ULobbyCreationWidget::UIBinding_OnChangeMapButtonClicked);
	}
}

void ULobbyCreationWidget::SetDefaultValues()
{
	/* Because game state only holds arrays for the state of lobby and we are in lobby creation
	here the values held by these widgets will be used to create the lobby */

	if (IsWidgetBound(Text_LobbyName))
	{
		Text_LobbyName->SetText(LobbyOptions::DEFAULT_MULTIPLAYER_LOBBY_NAME);
	}
	if (IsWidgetBound(Text_NetworkType))
	{
		Text_NetworkType->SetText(LobbyOptions::DEFAULT_NETWORK_TYPE);
	}
	if (IsWidgetBound(Text_NumSlots))
	{
		Text_NumSlots->SetText(Statics::IntToText(LobbyOptions::DEFAULT_NUM_SLOTS));
	}
	if (IsWidgetBound(Widget_MatchRules))
	{
		Widget_MatchRules->SetStartingResources(LobbyOptions::DEFAULT_STARTING_RESOURCES);
		Widget_MatchRules->SetDefeatCondition(LobbyOptions::DEFAULT_DEFEAT_CONDITION);
	}
	SetMap(&GI->GetMapInfo(LobbyOptions::DEFAULT_MAP_ID));
}

void ULobbyCreationWidget::SetMap(const FMapInfo * MapInfo)
{
	if (IsWidgetBound(Widget_SelectedMap))
	{
		Widget_SelectedMap->SetMap(MapInfo, bTryShowPlayerStartsOnMaps 
			? EPlayerStartDisplay::VisibleButUnclickable : EPlayerStartDisplay::Hidden);
	}
}

void ULobbyCreationWidget::UIBinding_OnCreateButtonClicked()
{
	/* Here use the const default values if the widget isn't bound */
	
	const FText LobbyName = IsWidgetBound(Text_LobbyName) ? Text_LobbyName->GetText() : LobbyOptions::DEFAULT_MULTIPLAYER_LOBBY_NAME;
	const bool bIsLAN = IsWidgetBound(Text_NetworkType) ? Text_NetworkType->GetText().EqualTo(LobbyOptions::LAN_TEXT) : IsTextForLAN(LobbyOptions::DEFAULT_NETWORK_TYPE);
	const FText Password = IsWidgetBound(Text_Password) ? Text_Password->GetText() : LobbyOptions::DEFAULT_PASSWORD;
	const uint32 NumSlots = IsWidgetBound(Text_NumSlots) ? Statics::TextToInt(Text_NumSlots->GetText()) : LobbyOptions::DEFAULT_NUM_SLOTS;
	const uint8 Map = IsWidgetBound(Widget_SelectedMap) ? Widget_SelectedMap->GetSetMapID() : LobbyOptions::DEFAULT_MAP_ID;
	const EStartingResourceAmount StartingResources = IsWidgetBound(Widget_MatchRules) ? Widget_MatchRules->GetStartingResources() : LobbyOptions::DEFAULT_STARTING_RESOURCES;
	const EDefeatCondition DefeatCondition = IsWidgetBound(Widget_MatchRules) ? Widget_MatchRules->GetDefeatCondition() : LobbyOptions::DEFAULT_DEFEAT_CONDITION;
	
	GI->CreateNetworkedSession(LobbyName, bIsLAN, Password, NumSlots, Map, StartingResources,
		DefeatCondition);
}

void ULobbyCreationWidget::UIBinding_OnReturnButtonClicked()
{
	GI->ShowPreviousWidget();
}

void ULobbyCreationWidget::UIBinding_OnLobbyNameTextChanged(const FText & NewText)
{
	Text_LobbyName->SetText(Statics::ConcateText(NewText, LobbyOptions::MAX_NAME_LENGTH));
}

void ULobbyCreationWidget::UIBinding_OnNetworkTypeLeftButtonClicked()
{
	/* Toggle between LAN and online */
	if (IsTextForOnline(Text_NetworkType->GetText()))
	{
		Text_NetworkType->SetText(LobbyOptions::ONLINE_TEXT);
	}
	else if (IsTextForLAN(Text_NetworkType->GetText()))
	{
		Text_NetworkType->SetText(LobbyOptions::LAN_TEXT);
	}
	else
	{
		UE_LOG(RTSLOG, Fatal, TEXT("Text_NetworkType text not expected value, text was: %s"), 
			*Text_NetworkType->GetText().ToString());
	}
}

void ULobbyCreationWidget::UIBinding_OnNetworkTypeRightButtonClicked()
{
	/* With only two network types this is ok */
	UIBinding_OnNetworkTypeLeftButtonClicked();
}

void ULobbyCreationWidget::UIBinding_OnPasswordTextChanged(const FText & NewText)
{
	Text_Password->SetText(Statics::ConcateText(NewText, LobbyOptions::MAX_PASSWORD_LENGTH));
}

void ULobbyCreationWidget::UIBinding_OnDecreaseNumSlotsButtonClicked()
{
	const FText & CurrentText = Text_NumSlots->GetText();
	int32 AsInt = Statics::TextToInt(CurrentText);
	AsInt--;
	AsInt = FMath::Clamp<int32>(AsInt, 2, ProjectSettings::MAX_NUM_PLAYERS);
	Text_NumSlots->SetText(Statics::IntToText(AsInt));
}

void ULobbyCreationWidget::UIBinding_OnIncreaseNumSlotsButtonClicked()
{
	const FText & CurrentText = Text_NumSlots->GetText();
	int32 AsInt = Statics::TextToInt(CurrentText);
	AsInt++;
	AsInt = FMath::Clamp<int32>(AsInt, 2, ProjectSettings::MAX_NUM_PLAYERS);
	Text_NumSlots->SetText(Statics::IntToText(AsInt));
}

void ULobbyCreationWidget::UIBinding_OnChangeMapButtonClicked()
{
	/* Setup map selection widget to make sure it updates our setup map when we exit it */
	UMapSelectionWidget * MapSelectionWidget = GI->GetWidget<UMapSelectionWidget>(EWidgetType::MapSelectionScreen);
	MapSelectionWidget->SetRefToUpdatedWidget(this);
	MapSelectionWidget->SetupMapList();

	if (IsWidgetBound(Widget_SelectedMap))
	{
		const FString & CurrentMapName = Widget_SelectedMap->GetSetMap().ToString();
		MapSelectionWidget->SetCurrentMap(&GI->GetMapInfo(*CurrentMapName));
	}

	GI->ShowWidget(EWidgetType::MapSelectionScreen, ESlateVisibility::Collapsed);
}

bool ULobbyCreationWidget::IsTextForLAN(const FText & InText)
{
	return InText.EqualTo(LobbyOptions::LAN_TEXT);
}

bool ULobbyCreationWidget::IsTextForOnline(const FText & InText)
{
	return InText.EqualTo(LobbyOptions::ONLINE_TEXT);
}

void ULobbyCreationWidget::UpdateMapDisplay(const FMapInfo & MapInfo)
{
	SetMap(&MapInfo);
}


