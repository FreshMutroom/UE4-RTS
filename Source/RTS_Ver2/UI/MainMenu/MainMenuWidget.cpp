// Fill out your copyright notice in the Description page of Project Settings.

#include "MainMenuWidget.h"
#include "Components/Button.h"

#include "GameFramework/RTSGameInstance.h"
#include "GameFramework/RTSPlayerState.h"
#include "Settings/RTSGameUserSettings.h"
#include "UI/MainMenu/NicknameEntryWidget.h"

bool UMainMenuWidget::Setup()
{
	if (Super::Setup())
	{
		return true;
	}

	SetupButtonFunctionality();

	return false;
}

void UMainMenuWidget::SetupButtonFunctionality()
{
	if (IsWidgetBound(Button_Play))
	{
		Button_Play->OnClicked.AddDynamic(this, &UMainMenuWidget::UIBinding_OnPlayButtonClicked);
	}
	if (IsWidgetBound(Button_CreateSingleplayer))
	{
		Button_CreateSingleplayer->OnClicked.AddDynamic(this, &UMainMenuWidget::UIBinding_OnCreateSingleplayerButtonClicked);
	}
	if (IsWidgetBound(Button_CreateMultiplayer))
	{
		Button_CreateMultiplayer->OnClicked.AddDynamic(this, &UMainMenuWidget::UIBinding_OnCreateMultiplayerButtonClicked);
	}
	if (IsWidgetBound(Button_FindMultiplayer))
	{
		Button_FindMultiplayer->OnClicked.AddDynamic(this, &UMainMenuWidget::UIBinding_OnFindMultiplayerButtonClicked);
	}
	if (IsWidgetBound(Button_Settings))
	{
		Button_Settings->OnClicked.AddDynamic(this, &UMainMenuWidget::UIBinding_OnSettingsButtonClicked);
	}
	if (IsWidgetBound(Button_ExitToOS))
	{
		Button_ExitToOS->OnClicked.AddDynamic(this, &UMainMenuWidget::UIBinding_OnExitToOSButtonClicked);
	}
	if (IsWidgetBound(Button_Return))
	{
		Button_Return->OnClicked.AddDynamic(this, &UMainMenuWidget::UIBinding_OnReturnButtonClicked);
	}
}

void UMainMenuWidget::UIBinding_OnPlayButtonClicked()
{
	GI->ShowWidget(EWidgetType::PlayMenu, ESlateVisibility::Collapsed);
}

void UMainMenuWidget::UIBinding_OnCreateSingleplayerButtonClicked()
{
	GI->CreateSingleplayerLobby();
}

void UMainMenuWidget::UIBinding_OnCreateMultiplayerButtonClicked()
{
	URTSGameUserSettings * Settings = CastChecked<URTSGameUserSettings>(UGameUserSettings::GetGameUserSettings());

	if (Settings->HasSeenProfile())
	{
		GI->ShowWidget(EWidgetType::LobbyCreationScreen, ESlateVisibility::Collapsed);
	}
	else
	{
		Settings->ChangeHasSeenProfile(true);

		/* Redirect player to enter a nickname before continuing */
		UMainMenuWidgetBase * Widget = GI->GetWidget(EWidgetType::NicknameEntryScreen);
		UNicknameEntryWidget * NameEntryWidget = CastChecked<UNicknameEntryWidget>(Widget);
		NameEntryWidget->SetNextWidget(EWidgetType::LobbyCreationScreen);
		GI->ShowWidget(EWidgetType::NicknameEntryScreen, ESlateVisibility::Collapsed);
	}
}

void UMainMenuWidget::UIBinding_OnFindMultiplayerButtonClicked()
{
	URTSGameUserSettings * Settings = CastChecked<URTSGameUserSettings>(UGameUserSettings::GetGameUserSettings());

	if (Settings->HasSeenProfile())
	{
		GI->ShowWidget(EWidgetType::LobbyBrowser, ESlateVisibility::Collapsed);
	}
	else
	{
		Settings->ChangeHasSeenProfile(true);

		/* Redirect player to enter a nickname before continuing */
		UMainMenuWidgetBase * Widget = GI->GetWidget(EWidgetType::NicknameEntryScreen);
		UNicknameEntryWidget * NameEntryWidget = CastChecked<UNicknameEntryWidget>(Widget);
		NameEntryWidget->SetNextWidget(EWidgetType::LobbyBrowser);
		GI->ShowWidget(EWidgetType::NicknameEntryScreen, ESlateVisibility::Collapsed);
	}
}

void UMainMenuWidget::UIBinding_OnSettingsButtonClicked()
{
	GI->ShowWidget(EWidgetType::Settings, ESlateVisibility::Collapsed);
}

void UMainMenuWidget::UIBinding_OnExitToOSButtonClicked()
{
	// Either show confirm widget if set via editor or exit straight away 
	if (GI->IsWidgetBlueprintSet(EWidgetType::ConfirmExitToOS))
	{
		GI->ShowWidget(EWidgetType::ConfirmExitToOS, ESlateVisibility::HitTestInvisible);
	}
	else
	{
		GI->OnQuitGameInitiated();
	}
}

void UMainMenuWidget::UIBinding_OnReturnButtonClicked()
{
	GI->ShowPreviousWidget();
}
