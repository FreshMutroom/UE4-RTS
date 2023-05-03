// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/MainMenu/MainMenuWidgetBase.h"

#include "Statics/OtherEnums.h"
#include "NicknameEntryWidget.generated.h"

class ARTSPlayerState;
class UEditableText;
class UButton;

/**
 *	A widget for the player to enter a nickname into
 */	
UCLASS(Abstract)
class RTS_VER2_API UNicknameEntryWidget : public UMainMenuWidgetBase
{
	GENERATED_BODY()

public:

	virtual bool Setup() override;

protected:

	/* Reference to player state */
	UPROPERTY()
	ARTSPlayerState * PS;

	/* The widget the continue button will navigate to */
	EWidgetType NextWidget;

	/* Text for player to enter the alias they want into */
	UPROPERTY(meta = (BindWidgetOptional))
	UEditableText * Text_Name;

	/* Button to continue to menu user wanted before this screen popped up */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton * Button_Continue;

	/* Button to return to previous menu */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton * Button_Return;

	UFUNCTION()
	void UIBinding_OnNameTextChanged(const FText & NewText);

	UFUNCTION()
	void UIBinding_OnNameTextCommitted(const FText & Text, ETextCommit::Type CommitMethod);

	UFUNCTION()
	void UIBinding_OnContinueButtonClicked();

	UFUNCTION()
	void UIBinding_OnReturnButtonClicked();

public:

	/* Sets the widget the continue button will navigate to */
	virtual void SetNextWidget(EWidgetType NextWidgetType);
	
};
