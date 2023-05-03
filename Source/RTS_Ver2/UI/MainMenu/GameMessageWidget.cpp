// Fill out your copyright notice in the Description page of Project Settings.

#include "GameMessageWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"

#include "GameFramework/RTSGameInstance.h"

bool UGameMessageWidget::Setup()
{
	if (Super::Setup())
	{
		return true;
	}

	if (IsWidgetBound(Text_Message))
	{
		OriginalTextColor = Text_Message->ColorAndOpacity;
	}

	if (IsWidgetBound(Button_Return))
	{
		Button_Return->OnClicked.AddDynamic(this, &UGameMessageWidget::UIBinding_OnReturnButtonClicked);
	}

	return false;
}

void UGameMessageWidget::UIBinding_OnReturnButtonClicked()
{
	GI->ShowPreviousWidget();
}

void UGameMessageWidget::SetMessage(const FText & InMessage, bool bSetColor, FLinearColor TextColor)
{
	if (IsWidgetBound(Text_Message))
	{
		Text_Message->SetText(InMessage);
		
		if (bSetColor)
		{
			Text_Message->SetColorAndOpacity(TextColor);
		}
		else
		{
			// Use original color that was set by user in editor
			Text_Message->SetColorAndOpacity(OriginalTextColor);
		}
	}
}
