// Fill out your copyright notice in the Description page of Project Settings.

#include "EndOfMatchWidget.h"
#include "Components/PanelWidget.h"
#include "Components/Button.h"

#include "GameFramework/RTSGameInstance.h"
#include "GameFramework/RTSPlayerController.h"

bool UEndOfMatchWidget::SetupWidget(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController)
{
	if (Super::SetupWidget(InGameInstance, InPlayerController))
	{
		return true;
	}

	/* Setup button functionality */
	if (IsWidgetBound(Button_Spectate))
	{
		Button_Spectate->OnClicked.AddDynamic(this, &UEndOfMatchWidget::UIBinding_OnSpectateButtonClicked);
	}
	if (IsWidgetBound(Button_ReturnToMainMenu))
	{
		Button_ReturnToMainMenu->OnClicked.AddDynamic(this, &UEndOfMatchWidget::UIBinding_OnReturnToMainMenuButtonClicked);
	}

	return false;
}

void UEndOfMatchWidget::SetupForResult(EMatchResult MatchResult)
{
	/* Show the panel widget that matches the match result */

	if (IsWidgetBound(Panel_Victory))
	{
		const ESlateVisibility NewVis = (MatchResult == EMatchResult::Victory)
			? ESlateVisibility::Visible : ESlateVisibility::Hidden;

		Panel_Victory->SetVisibility(NewVis);
	}

	if (IsWidgetBound(Panel_Draw))
	{
		const ESlateVisibility NewVis = (MatchResult == EMatchResult::Draw)
			? ESlateVisibility::Visible : ESlateVisibility::Hidden;

		Panel_Draw->SetVisibility(NewVis);
	}

	if (IsWidgetBound(Panel_Defeat))
	{
		const ESlateVisibility NewVis = (MatchResult == EMatchResult::Defeat)
			? ESlateVisibility::Visible : ESlateVisibility::Hidden;

		Panel_Defeat->SetVisibility(NewVis);
	}
}

void UEndOfMatchWidget::NativeTick(const FGeometry & MyGeometry, float InDeltaTime)
{
	UUserWidget::Tick(MyGeometry, InDeltaTime);
}

void UEndOfMatchWidget::UIBinding_OnSpectateButtonClicked()
{
	/* Just hide widget */
	PC->ShowPreviousWidget();
}

void UEndOfMatchWidget::UIBinding_OnReturnToMainMenuButtonClicked()
{
	GI->GoFromMatchToMainMenu();
}
