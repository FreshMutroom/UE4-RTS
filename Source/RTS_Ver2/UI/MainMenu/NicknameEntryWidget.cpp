// Fill out your copyright notice in the Description page of Project Settings.

#include "NicknameEntryWidget.h"
#include "Components/EditableText.h"
#include "GameFramework/PlayerState.h"

#include "UI/MainMenu/Menus.h"
#include "Statics/Statics.h"
#include "Statics/DevelopmentStatics.h"
#include "GameFramework/RTSPlayerState.h"
#include "GameFramework/RTSGameInstance.h"
#include "Settings/RTSGameUserSettings.h"

bool UNicknameEntryWidget::Setup()
{
	if (Super::Setup())
	{
		return true;
	}

	PS = CastChecked<ARTSPlayerState>(GetWorld()->GetFirstPlayerController()->PlayerState);

	if (IsWidgetBound(Text_Name))
	{
		Text_Name->OnTextChanged.AddDynamic(this, &UNicknameEntryWidget::UIBinding_OnNameTextChanged);
		Text_Name->OnTextCommitted.AddDynamic(this, &UNicknameEntryWidget::UIBinding_OnNameTextCommitted);

		URTSGameUserSettings * Settings = CastChecked<URTSGameUserSettings>(UGameUserSettings::GetGameUserSettings());

		/* Set default profile name and concate it to max length */
		FText DefaultName = FText::FromString(Settings->GetPlayerAlias());
		DefaultName = Statics::ConcateText(DefaultName, PlayerOptions::MAX_ALIAS_LENGTH);

		Text_Name->SetText(DefaultName);
	}
	if (IsWidgetBound(Button_Continue))
	{
		Button_Continue->OnClicked.AddDynamic(this, &UNicknameEntryWidget::UIBinding_OnContinueButtonClicked);
	}
	if (IsWidgetBound(Button_Return))
	{
		Button_Return->OnClicked.AddDynamic(this, &UNicknameEntryWidget::UIBinding_OnReturnButtonClicked);
	}

	return false;
}

void UNicknameEntryWidget::UIBinding_OnNameTextChanged(const FText & NewText)
{
	Text_Name->SetText(Statics::ConcateText(NewText, PlayerOptions::MAX_ALIAS_LENGTH));
}

void UNicknameEntryWidget::UIBinding_OnNameTextCommitted(const FText & Text, ETextCommit::Type CommitMethod)
{
	URTSGameUserSettings * Settings = CastChecked<URTSGameUserSettings>(UGameUserSettings::GetGameUserSettings());

	// TODO does pressing ESC return text to what it was before box got focus?
	if (CommitMethod != ETextCommit::OnCleared)
	{
		Settings->ChangePlayerAlias(Text.ToString());

		/* Write the settings to permanent storage. Note: this will write all user settings such
		as video, controls etc, not just profile settings */
		Settings->ApplyAllSettings(GI, GetWorld()->GetFirstPlayerController());
	}
	else
	{
		/* Revert to what was in box before */
		Text_Name->SetText(FText::FromString(Settings->GetPlayerAlias()));
	}
}

void UNicknameEntryWidget::UIBinding_OnContinueButtonClicked()
{
	/* Ensure that if text in profile name text box hasn't been committed that we will still
	use it */
	UIBinding_OnNameTextCommitted(Text_Name->GetText(), ETextCommit::OnEnter);

	GI->ShowWidget(NextWidget, ESlateVisibility::Collapsed);

	/* Prevent this widget from being navigated to with ShowPreviousWidget */
	GI->RemoveFromWidgetHistory(EWidgetType::Profile);
}

void UNicknameEntryWidget::UIBinding_OnReturnButtonClicked()
{
	GI->ShowPreviousWidget();
}

void UNicknameEntryWidget::SetNextWidget(EWidgetType NextWidgetType)
{
	NextWidget = NextWidgetType;
}
