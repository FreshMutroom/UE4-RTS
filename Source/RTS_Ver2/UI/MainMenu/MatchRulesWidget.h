// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/MainMenu/MainMenuWidgetBase.h"

#include "Statics/CommonEnums.h"
#include "MatchRulesWidget.generated.h"

class UComboBoxString;
class UTextBlock;

/**
 *	Widget for the rules of the match like for example the defeat condition, maybe if crates
 *	spawn, etc. Only for in-match rules but can optionally be displayed when browsing lobbies and
 *	will be displayed while in a lobby.
 *	
 *	The combo box is optional. If you do not want your game to allow the host to change match settings
 *	after a lobby has been created then a simple way to do this is to not add a combo box to this
 *	widget that belongs to the lobby widget 
 */
UCLASS(Abstract)
class RTS_VER2_API UMatchRulesWidget : public UMainMenuWidgetBase
{
	GENERATED_BODY()

public:

	/* If this widget is on the lobby widget then it should call this */
	void SetIsForLobby(bool bNewValue);

	virtual bool Setup() override;

protected:

	bool bIsForLobby;

	/* Whether this widget is placed in a lobby widget */
	bool IsForLobby() const;

	/* Right now the only things I'm going to show in this widget is starting resources and the
	defeat condition */

	/* Combo box to choose starting resources */
	UPROPERTY(meta = (BindWidgetOptional))
	UComboBoxString * ComboBox_StartingResources;

	/* Text to show starting resources */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_StartingResources;

	/* Combo box to choose the defeat condition */
	UPROPERTY(meta = (BindWidgetOptional))
	UComboBoxString * ComboBox_DefeatCondition;

	/* Text to show the defeat condition */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_DefeatCondition;

	UFUNCTION()
	void UIBinding_OnStartingResourcesSelectionChanged(FString Item, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void UIBinding_OnDefeatConditionSelectionChanged(FString Item, ESelectInfo::Type SelectionType);

public:

	EStartingResourceAmount GetStartingResources() const;

	void SetStartingResources(EStartingResourceAmount NewAmount);

	EDefeatCondition GetDefeatCondition() const;

	void SetDefeatCondition(EDefeatCondition NewCondition);
};
