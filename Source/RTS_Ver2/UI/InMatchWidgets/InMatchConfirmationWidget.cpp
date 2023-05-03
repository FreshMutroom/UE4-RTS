// Fill out your copyright notice in the Description page of Project Settings.

#include "InMatchConfirmationWidget.h"


void UInMatchConfirmationWidget::InitialSetup(URTSGameInstance * GameInst, ARTSPlayerController * InPlayerController, 
	EConfirmationWidgetPurpose InPurpose)
{
	assert(InPlayerController != nullptr);
	
	PC = InPlayerController;
	Purpose = InPurpose;
	
	/* The yes and no buttons are pretty pivotal to this user widget so no IsWidgetBound checks here. 
	Instead we assume they are bound */
	Button_Yes->SetPC(InPlayerController);
	Button_Yes->SetOwningWidget();
	Button_Yes->SetPurpose(EUIElementType::NoTooltipRequired, nullptr);
	Button_Yes->GetOnLeftMouseButtonPressDelegate().BindUObject(this, &UInMatchConfirmationWidget::UIBinding_OnYesButtonLeftMousePress);
	Button_Yes->GetOnLeftMouseButtonReleasedDelegate().BindUObject(this, &UInMatchConfirmationWidget::UIBinding_OnYesButtonLeftMouseReleased);
	Button_Yes->GetOnRightMouseButtonPressDelegate().BindUObject(this, &UInMatchConfirmationWidget::UIBinding_OnYesButtonRightMousePress);
	Button_Yes->GetOnRightMouseButtonReleasedDelegate().BindUObject(this, &UInMatchConfirmationWidget::UIBinding_OnYesButtonRightMouseReleased);
	
	Button_Yes->SetImagesAndSounds(GameInst->GetUnifiedButtonAssetFlags_Menus(),
		GameInst->GetUnifiedButtonAssets_Menus());

	Button_No->SetPC(InPlayerController);
	Button_No->SetOwningWidget();
	Button_No->SetPurpose(EUIElementType::NoTooltipRequired, nullptr);
	Button_No->GetOnLeftMouseButtonPressDelegate().BindUObject(this, &UInMatchConfirmationWidget::UIBinding_OnNoButtonLeftMousePress);
	Button_No->GetOnLeftMouseButtonReleasedDelegate().BindUObject(this, &UInMatchConfirmationWidget::UIBinding_OnNoButtonLeftMouseReleased);
	Button_No->GetOnRightMouseButtonPressDelegate().BindUObject(this, &UInMatchConfirmationWidget::UIBinding_OnNoButtonRightMousePress);
	Button_No->GetOnRightMouseButtonReleasedDelegate().BindUObject(this, &UInMatchConfirmationWidget::UIBinding_OnNoButtonRightMouseReleased);

	Button_No->SetImagesAndSounds(GameInst->GetUnifiedButtonAssetFlags_Menus(),
		GameInst->GetUnifiedButtonAssets_Menus());
}

void UInMatchConfirmationWidget::OnRequestToBeShown(URTSHUD * HUD)
{
	SetVisibility(ESlateVisibility::Visible);
	HUD->AddToEscapeRequestResponableWidgets(this);
}

void UInMatchConfirmationWidget::OnNoButtonClicked(URTSHUD * HUD)
{
	SetVisibility(ESlateVisibility::Hidden);
	HUD->RemoveFromEscapeRequestResponableWidgets(this);
}

bool UInMatchConfirmationWidget::RespondToEscapeRequest(URTSHUD * HUD)
{
	SetVisibility(ESlateVisibility::Hidden);
	return true;
}

void UInMatchConfirmationWidget::UIBinding_OnYesButtonLeftMousePress()
{
	PC->OnLMBPressed_ConfirmationWidgetYesButton(Button_Yes);
}

void UInMatchConfirmationWidget::UIBinding_OnYesButtonLeftMouseReleased()
{
	PC->OnLMBReleased_ConfirmationWidgetYesButton(Button_Yes, this);
}

void UInMatchConfirmationWidget::UIBinding_OnYesButtonRightMousePress()
{
	PC->OnRMBPressed_ConfirmationWidgetYesButton(Button_Yes);
}

void UInMatchConfirmationWidget::UIBinding_OnYesButtonRightMouseReleased()
{
	PC->OnRMBReleased_ConfirmationWidgetYesButton(Button_Yes);
}

void UInMatchConfirmationWidget::UIBinding_OnNoButtonLeftMousePress()
{
	PC->OnLMBPressed_ConfirmationWidgetNoButton(Button_No);
}

void UInMatchConfirmationWidget::UIBinding_OnNoButtonLeftMouseReleased()
{
	PC->OnLMBReleased_ConfirmationWidgetNoButton(Button_No, this);
}

void UInMatchConfirmationWidget::UIBinding_OnNoButtonRightMousePress()
{
	PC->OnRMBPressed_ConfirmationWidgetNoButton(Button_No);
}

void UInMatchConfirmationWidget::UIBinding_OnNoButtonRightMouseReleased()
{
	PC->OnRMBReleased_ConfirmationWidgetNoButton(Button_No);
}
