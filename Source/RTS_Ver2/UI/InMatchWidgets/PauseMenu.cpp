// Fill out your copyright notice in the Description page of Project Settings.

#include "PauseMenu.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"

#include "UI/MainMenuAndInMatch/SettingsMenu.h"


bool UPauseMenu::SetupWidget(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController)
{
	if (Super::SetupWidget(InGameInstance, InPlayerController))
	{
		return true;
	}

	SetupBoundWidgets();

	SpawnConfirmationWidgets();

	SpawnSettingsWidget();

	return false;
}

void UPauseMenu::SetupBoundWidgets()
{	
	if (IsWidgetBound(Button_Resume))
	{
		Button_Resume->SetPC(PC);
		Button_Resume->SetOwningWidget();
		Button_Resume->SetPurpose(EUIElementType::NoTooltipRequired, nullptr);
		Button_Resume->GetOnLeftMouseButtonPressDelegate().BindUObject(this, &UPauseMenu::UIBinding_OnResumeButtonLeftMouseButtonPress);
		Button_Resume->GetOnLeftMouseButtonReleasedDelegate().BindUObject(this, &UPauseMenu::UIBinding_OnResumeButtonLeftMouseButtonReleased);
		Button_Resume->GetOnRightMouseButtonPressDelegate().BindUObject(this, &UPauseMenu::UIBinding_OnResumeButtonRightMouseButtonPress);
		Button_Resume->GetOnRightMouseButtonReleasedDelegate().BindUObject(this, &UPauseMenu::UIBinding_OnResumeButtonRightMouseButtonReleased);
		Button_Resume->SetImagesAndSounds(GI->GetUnifiedButtonAssetFlags_Menus(), GI->GetUnifiedButtonAssets_Menus());
	}

	if (IsWidgetBound(Button_SettingsMenu))
	{
		Button_SettingsMenu->SetPC(PC);
		Button_SettingsMenu->SetOwningWidget();
		Button_SettingsMenu->SetPurpose(EUIElementType::NoTooltipRequired, nullptr);
		Button_SettingsMenu->GetOnLeftMouseButtonPressDelegate().BindUObject(this, &UPauseMenu::UIBinding_OnSettingsButtonLeftMouseButtonPress);
		Button_SettingsMenu->GetOnLeftMouseButtonReleasedDelegate().BindUObject(this, &UPauseMenu::UIBinding_OnSettingsButtonLeftMouseButtonReleased);
		Button_SettingsMenu->GetOnRightMouseButtonPressDelegate().BindUObject(this, &UPauseMenu::UIBinding_OnSettingsButtonRightMouseButtonPress);
		Button_SettingsMenu->GetOnRightMouseButtonReleasedDelegate().BindUObject(this, &UPauseMenu::UIBinding_OnSettingsButtonRightMouseButtonReleased);
		Button_SettingsMenu->SetImagesAndSounds(GI->GetUnifiedButtonAssetFlags_Menus(), GI->GetUnifiedButtonAssets_Menus());
	}

	if (IsWidgetBound(Button_ReturnToMainMenu))
	{
		Button_ReturnToMainMenu->SetPC(PC);
		Button_ReturnToMainMenu->SetOwningWidget();
		Button_ReturnToMainMenu->SetPurpose(EUIElementType::NoTooltipRequired, nullptr);
		Button_ReturnToMainMenu->GetOnLeftMouseButtonPressDelegate().BindUObject(this, &UPauseMenu::UIBinding_OnReturnToMainMenuLeftMouseButtonPress);
		Button_ReturnToMainMenu->GetOnLeftMouseButtonReleasedDelegate().BindUObject(this, &UPauseMenu::UIBinding_OnReturnToMainMenuLeftMouseButtonReleased);
		Button_ReturnToMainMenu->GetOnRightMouseButtonPressDelegate().BindUObject(this, &UPauseMenu::UIBinding_OnReturnToMainMenuRightMouseButtonPress);
		Button_ReturnToMainMenu->GetOnRightMouseButtonReleasedDelegate().BindUObject(this, &UPauseMenu::UIBinding_OnReturnToMainMenuRightMouseButtonReleased);
		Button_ReturnToMainMenu->SetImagesAndSounds(GI->GetUnifiedButtonAssetFlags_Menus(), GI->GetUnifiedButtonAssets_Menus());
	}

	if (IsWidgetBound(Button_ReturnToOperatingSystem))
	{
		Button_ReturnToOperatingSystem->SetPC(PC);
		Button_ReturnToOperatingSystem->SetOwningWidget();
		Button_ReturnToOperatingSystem->SetPurpose(EUIElementType::NoTooltipRequired, nullptr);
		Button_ReturnToOperatingSystem->GetOnLeftMouseButtonPressDelegate().BindUObject(this, &UPauseMenu::UIBinding_OnReturnToOSLeftMouseButtonPress);
		Button_ReturnToOperatingSystem->GetOnLeftMouseButtonReleasedDelegate().BindUObject(this, &UPauseMenu::UIBinding_OnReturnToOSLeftMouseButtonReleased);
		Button_ReturnToOperatingSystem->GetOnRightMouseButtonPressDelegate().BindUObject(this, &UPauseMenu::UIBinding_OnReturnToOSRightMouseButtonPress);
		Button_ReturnToOperatingSystem->GetOnRightMouseButtonReleasedDelegate().BindUObject(this, &UPauseMenu::UIBinding_OnReturnToOSRightMouseButtonReleased);
		Button_ReturnToOperatingSystem->SetImagesAndSounds(GI->GetUnifiedButtonAssetFlags_Menus(), GI->GetUnifiedButtonAssets_Menus());
	}
}

void UPauseMenu::SpawnConfirmationWidgets()
{
	if (ConfirmExitToMainMenuWidget_BP != nullptr)
	{
		// Double check all 5 lines here are needed
		
		ConfirmExitToMainMenuWidget = CreateWidget<UInMatchConfirmationWidget>(PC, ConfirmExitToMainMenuWidget_BP);
		ConfirmExitToMainMenuWidget->SetVisibility(ESlateVisibility::Hidden);

		UCanvasPanelSlot * CanvasSlot = CastChecked<UCanvasPanel>(GetRootWidget())->AddChildToCanvas(ConfirmExitToMainMenuWidget);
		CanvasSlot->SetZOrder(CONFIRMATION_WIDGET_Z_ORDER);
		ConfirmExitToMainMenuWidget->InitialSetup(GI, PC, EConfirmationWidgetPurpose::ExitToMainMenu);
	}

	if (ConfirmExitToOperatingSystemWidget_BP != nullptr)
	{
		ConfirmExitToOperatingSystemWidget = CreateWidget<UInMatchConfirmationWidget>(PC, ConfirmExitToOperatingSystemWidget_BP);
		ConfirmExitToOperatingSystemWidget->SetVisibility(ESlateVisibility::Hidden);

		UCanvasPanelSlot * CanvasSlot = CastChecked<UCanvasPanel>(GetRootWidget())->AddChildToCanvas(ConfirmExitToOperatingSystemWidget);
		CanvasSlot->SetZOrder(CONFIRMATION_WIDGET_Z_ORDER);
		ConfirmExitToOperatingSystemWidget->InitialSetup(GI, PC, EConfirmationWidgetPurpose::ExitToOperatingSystem);
	}
}

void UPauseMenu::SpawnSettingsWidget()
{
	if (SettingsWidget_BP != nullptr)
	{
		SettingsWidget = CreateWidget<USettingsWidget>(PC, SettingsWidget_BP);
		SettingsWidget->SetVisibility(ESlateVisibility::Hidden);

		UCanvasPanelSlot * CanvasSlot = CastChecked<UCanvasPanel>(GetRootWidget())->AddChildToCanvas(SettingsWidget);
		CanvasSlot->SetZOrder(SETTINGS_WIDGET_Z_ORDER);
		SettingsWidget->InitialSetup(GI, PC);
	}
}

bool UPauseMenu::HasConfirmExitToMainMenuWidget() const
{
	return (ConfirmExitToMainMenuWidget != nullptr);
}

void UPauseMenu::ShowConfirmExitToMainMenuWidget(URTSHUD * HUD)
{
	ConfirmExitToMainMenuWidget->OnRequestToBeShown(HUD);
}

bool UPauseMenu::HasConfirmExitToOperatingSystemWidget() const
{
	return (ConfirmExitToOperatingSystemWidget != nullptr);
}

void UPauseMenu::ShowConfirmExitToOperatingSystemWidget(URTSHUD * HUD)
{
	ConfirmExitToOperatingSystemWidget->OnRequestToBeShown(HUD);
}

bool UPauseMenu::IsShowingOrPlayingShowAnimation() const
{
	/* Because right now I do not have any animations on this widget this is ok */
	return GetVisibility() == ESlateVisibility::Visible;
}

bool UPauseMenu::RespondToEscapeRequest(URTSHUD * HUD)
{
	/* Check if the settings menu is open. If so then close it */
	if (SettingsWidget->IsShowingOrPlayingShowAnim())
	{
		SettingsWidget->OnEscapeRequest();
		return false; // Return false so PauseMenu isn't popped from URTSHUD::EscapeRequestResponableWidgets
					 // Should probably change the return value to an enum like 
					// Handled, HandledButDontPop, NotHandled
	}
	/* Check if some time has passed since the last time the player pressed the ESC key. 
	The idea here is if the player spams their ESC key to get to the pause menu then we don't 
	want them to toggle it rapidly while doing this */
	else if (GetWorld()->GetRealTimeSeconds() - HUD->GetTimeWhenPauseMenuWasShown() > 0.6f)
	{
		ARTSPlayerController::SetGamePaused(GetWorld(), false);
		SetVisibility(ESlateVisibility::Hidden);
		return true;
	}

	return false;
}

void UPauseMenu::ShowSettingsMenu()
{
	UE_CLOG(SettingsWidget == nullptr, RTSLOG, Fatal, TEXT("Request to show settings menu but it is null. "
		"It is likely no blueprint is set for it in the pause menu"));

	SettingsWidget->Show();
}

void UPauseMenu::UIBinding_OnResumeButtonLeftMouseButtonPress()
{
	PC->OnLMBPressed_PauseMenu_Resume(Button_Resume);
}

void UPauseMenu::UIBinding_OnResumeButtonLeftMouseButtonReleased()
{
	PC->OnLMBReleased_PauseMenu_Resume(Button_Resume);
}

void UPauseMenu::UIBinding_OnResumeButtonRightMouseButtonPress()
{
	PC->OnRMBPressed_PauseMenu_Resume(Button_Resume);
}

void UPauseMenu::UIBinding_OnResumeButtonRightMouseButtonReleased()
{
	PC->OnRMBReleased_PauseMenu_Resume(Button_Resume);
}

void UPauseMenu::UIBinding_OnSettingsButtonLeftMouseButtonPress()
{
	PC->OnLMBPressed_PauseMenu_ShowSettingsMenu(Button_SettingsMenu);
}

void UPauseMenu::UIBinding_OnSettingsButtonLeftMouseButtonReleased()
{
	PC->OnLMBReleased_PauseMenu_ShowSettingsMenu(Button_SettingsMenu, this);
}

void UPauseMenu::UIBinding_OnSettingsButtonRightMouseButtonPress()
{
	PC->OnRMBPressed_PauseMenu_ShowSettingsMenu(Button_SettingsMenu);
}

void UPauseMenu::UIBinding_OnSettingsButtonRightMouseButtonReleased()
{
	PC->OnRMBReleased_PauseMenu_ShowSettingsMenu(Button_SettingsMenu);
}

void UPauseMenu::UIBinding_OnReturnToMainMenuLeftMouseButtonPress()
{
	PC->OnLMBPressed_PauseMenu_ReturnToMainMenu(Button_ReturnToMainMenu);
}

void UPauseMenu::UIBinding_OnReturnToMainMenuLeftMouseButtonReleased()
{
	PC->OnLMBReleased_PauseMenu_ReturnToMainMenu(Button_ReturnToMainMenu, this);
}

void UPauseMenu::UIBinding_OnReturnToMainMenuRightMouseButtonPress()
{
	PC->OnRMBPressed_PauseMenu_ReturnToMainMenu(Button_ReturnToMainMenu);
}

void UPauseMenu::UIBinding_OnReturnToMainMenuRightMouseButtonReleased()
{
	PC->OnRMBReleased_PauseMenu_ReturnToMainMenu(Button_ReturnToMainMenu);
}

void UPauseMenu::UIBinding_OnReturnToOSLeftMouseButtonPress()
{
	PC->OnLMBPressed_PauseMenu_ReturnToOperatingSystem(Button_ReturnToOperatingSystem);
}

void UPauseMenu::UIBinding_OnReturnToOSLeftMouseButtonReleased()
{
	PC->OnLMBReleased_PauseMenu_ReturnToOperatingSystem(Button_ReturnToOperatingSystem, this);
}

void UPauseMenu::UIBinding_OnReturnToOSRightMouseButtonPress()
{
	PC->OnRMBPressed_PauseMenu_ReturnToOperatingSystem(Button_ReturnToOperatingSystem);
}

void UPauseMenu::UIBinding_OnReturnToOSRightMouseButtonReleased()
{
	PC->OnRMBReleased_PauseMenu_ReturnToOperatingSystem(Button_ReturnToOperatingSystem);
}
