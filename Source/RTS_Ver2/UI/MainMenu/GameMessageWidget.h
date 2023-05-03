// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/MainMenu/MainMenuWidgetBase.h"
#include "GameMessageWidget.generated.h"

class UTextBlock;
class UButton;


/**
 *	A widget that displays a message like "lobby is full"
 */
UCLASS()
class RTS_VER2_API UGameMessageWidget : public UMainMenuWidgetBase
{
	GENERATED_BODY()
	
public:

	virtual bool Setup() override;

protected:

	/* Text widget that displays the message */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_Message;

	/* Button that returns player to previous widget */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton * Button_Return;

	/* The color of Text_Message set in editor */
	FSlateColor OriginalTextColor;

	UFUNCTION()
	void UIBinding_OnReturnButtonClicked();

public:

	/**
	 *	Set the text to be displayed 
	 *	@param InMessage - text to display with widget 
	 *	@param bSetColor - if true then 3rd param can be specified to set color of text
	 */
	void SetMessage(const FText & InMessage, bool bSetColor = false, 
		FLinearColor TextColor = FLinearColor::Black);
};
