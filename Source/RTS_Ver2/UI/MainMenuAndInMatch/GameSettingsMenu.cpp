// Fill out your copyright notice in the Description page of Project Settings.

#include "GameSettingsMenu.h"
#include "Components/EditableTextBox.h"


void UGameSettingsWidget::InitialSetup(URTSGameInstance * GameInst, ARTSPlayerController * PlayCon,
	URTSGameUserSettings * GameUserSettings, bool bUpdateAppearanceForCurrentValues)
{
	GI = GameInst;

	if (IsWidgetBound(TextBox_PlayerAlias))
	{
		TextBoxPlayerAliasOriginalOpacity = TextBox_PlayerAlias->GetRenderOpacity();

		TextBox_PlayerAlias->OnTextChanged.AddDynamic(this, &UGameSettingsWidget::UIBinding_OnAliasTextChanged);
		TextBox_PlayerAlias->OnTextCommitted.AddDynamic(this, &UGameSettingsWidget::UIBinding_OnAliasTextCommitted);
	}

	if (bUpdateAppearanceForCurrentValues)
	{
		UpdateAppearanceForCurrentValues();
	}
}

void UGameSettingsWidget::UpdateAppearanceForCurrentValues()
{
	URTSGameUserSettings * GameUserSettings = CastChecked<URTSGameUserSettings>(UGameUserSettings::GetGameUserSettings());

	if (IsWidgetBound(TextBox_PlayerAlias))
	{
		if (IsPlayerAllowedToChangeAlias(GI))
		{
			TextBox_PlayerAlias->SetVisibility(ESlateVisibility::Visible);
			TextBox_PlayerAlias->SetRenderOpacity(TextBoxPlayerAliasOriginalOpacity);
			TextBox_PlayerAlias->SetIsReadOnly(false);
		}
		else
		{
			/* Make widget read only. Just doing SetIsReadOnly doesn't stop it from being 
			clicked on so have to set vis too */
			
			/* Wow that is interesting. SelfHitTestInvis and HitTestInvis behave
			different for the editable text box. Need to use HitTestInvis if you want it
			to not be able to be clicked on, even though I thought that's what SelfHitTestInvis
			does though too */
			TextBox_PlayerAlias->SetVisibility(ESlateVisibility::HitTestInvisible);
			TextBox_PlayerAlias->SetRenderOpacity(TextBoxPlayerAliasOriginalOpacity * 0.6f);
			/* Do this too cause the widget has a different image for if it is in read only state */
			TextBox_PlayerAlias->SetIsReadOnly(true);
		}

		TextBox_PlayerAlias->SetText(FText::FromString(GameUserSettings->GetPlayerAlias()));
	}
}

void UGameSettingsWidget::UIBinding_OnAliasTextChanged(const FText & Text)
{
	// Make sure name does not go over character limit
	const FString String = Text.ToString();
	if (String.Len() > PlayerOptions::MAX_ALIAS_LENGTH)
	{
		/* Note: SetText will end up firing the OnTextChanged delegate which will call this whole func 
		again, but the branch above will not be taken */
		TextBox_PlayerAlias->SetText(FText::FromString(String.LeftChop(String.Len() - PlayerOptions::MAX_ALIAS_LENGTH)));
	}
}

void UGameSettingsWidget::UIBinding_OnAliasTextCommitted(const FText & Text, ETextCommit::Type CommitMethod)
{
	assert(IsPlayerAllowedToChangeAlias(GI));

	/* Interesting... pressing ESC will call OnEnter then OnCleared */

	URTSGameUserSettings * GameUserSettings = CastChecked<URTSGameUserSettings>(UGameUserSettings::GetGameUserSettings());

	if (CommitMethod == ETextCommit::OnEnter || CommitMethod == ETextCommit::OnUserMovedFocus)
	{
		GameUserSettings->ChangePlayerAlias(Text.ToString());
	}
	else if (CommitMethod == ETextCommit::OnCleared)
	{
		// Do not accept what was typed
		TextBox_PlayerAlias->SetText(FText::FromString(GameUserSettings->GetPlayerAlias()));
	}
}

bool UGameSettingsWidget::IsPlayerAllowedToChangeAlias(URTSGameInstance * GameInst) const
{
	/* Now allowed to change name if in lobby or match */
	return GameInst->IsInMainMenuMap();
}
