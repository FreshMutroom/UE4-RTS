// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/MainMenuAndInMatch/SettingsMenu.h"
#include "GameSettingsMenu.generated.h"

class UEditableTextBox;
class URTSGameUserSettings;


/**
 *	Game settings are things like whether to show healthbars. I have also put profily 
 *	type settings on here such as the player's alias.
 */
UCLASS(Abstract)
class RTS_VER2_API UGameSettingsWidget : public USettingsSubmenuBase
{
	GENERATED_BODY()

public:

	void InitialSetup(URTSGameInstance * GameInst, ARTSPlayerController * PlayCon, 
		URTSGameUserSettings * GameUserSettings, bool bUpdateAppearanceForCurrentValues);

	//~ Begin USettingsSubmenuBase interface
	virtual void UpdateAppearanceForCurrentValues() override;
	//~ End USettingsSubmenuBase interface

protected:

	UFUNCTION()
	void UIBinding_OnAliasTextChanged(const FText & Text);

	UFUNCTION()
	void UIBinding_OnAliasTextCommitted(const FText & Text, ETextCommit::Type CommitMethod);

	bool IsPlayerAllowedToChangeAlias(URTSGameInstance * GameInst) const;

	//-------------------------------------------------------------
	//	Data
	//-------------------------------------------------------------

	URTSGameInstance * GI;

	/* Text box for player to enter their alias into */
	UPROPERTY(meta = (BindWidgetOptional))
	UEditableTextBox * TextBox_PlayerAlias;

	float TextBoxPlayerAliasOriginalOpacity;
};
