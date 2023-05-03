// Fill out your copyright notice in the Description page of Project Settings.

#include "SettingsMenu.h"
#include "Components/WidgetSwitcher.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/CanvasPanel.h"

#include "UI/InMatchWidgets/Components/MyButton.h"
#include "UI/MainMenuAndInMatch/VideoSettingsMenu.h"
#include "Statics/OtherEnums.h"
#include "GameFramework/RTSGameInstance.h"
#include "GameFramework/RTSPlayerController.h"
#include "Settings/RTSGameUserSettings.h"
#include "UI/MainMenuAndInMatch/AudioSettingsMenu.h"
#include "UI/MainMenuAndInMatch/ControlSettingsMenu.h"
#include "UI/MainMenuAndInMatch/KeyMappingMenu.h"
#include "UI/MainMenuAndInMatch/GameSettingsMenu.h"
#include "Statics/DevelopmentStatics.h"


//----------------------------------------------------------------------------------------------
//==============================================================================================
//	------- Submenu Base Widget -------
//==============================================================================================
//----------------------------------------------------------------------------------------------

void USettingsSubmenuBase::NativeTick(const FGeometry & MyGeometry, float InDeltaTime)
{
}


//----------------------------------------------------------------------------------------------
//==============================================================================================
//	------- Apply Settings on Exit Confirmation Widget -------
//==============================================================================================
//----------------------------------------------------------------------------------------------

void USettingsConfirmationWidget_Exit::InitialSetup(URTSGameInstance * GameInst, ARTSPlayerController * PlayCon, 
	USettingsWidget * InSettingsMenu)
{
	PC = PlayCon;
	SettingsMenu = InSettingsMenu;

	if (IsWidgetBound(Button_Save))
	{
		Button_Save->SetPC(PlayCon);
		Button_Save->SetOwningWidget();
		Button_Save->SetPurpose(EUIElementType::NoTooltipRequired, nullptr);
		Button_Save->GetOnLeftMouseButtonPressDelegate().BindUObject(this, &USettingsConfirmationWidget_Exit::UIBinding_OnConfirmButtonLeftMousePress);
		Button_Save->GetOnLeftMouseButtonReleasedDelegate().BindUObject(this, &USettingsConfirmationWidget_Exit::UIBinding_OnConfirmButtonLeftMouseRelease);
		Button_Save->GetOnRightMouseButtonPressDelegate().BindUObject(this, &USettingsConfirmationWidget_Exit::UIBinding_OnConfirmButtonRightMousePress);
		Button_Save->GetOnRightMouseButtonReleasedDelegate().BindUObject(this, &USettingsConfirmationWidget_Exit::UIBinding_OnConfirmButtonRightMouseRelease);
		Button_Save->SetImagesAndSounds(GameInst->GetUnifiedButtonAssetFlags_Menus(), GameInst->GetUnifiedButtonAssets_Menus());
	}

	if (IsWidgetBound(Button_DoNotSave))
	{
		Button_DoNotSave->SetPC(PlayCon);
		Button_DoNotSave->SetOwningWidget();
		Button_DoNotSave->SetPurpose(EUIElementType::NoTooltipRequired, nullptr);
		Button_DoNotSave->GetOnLeftMouseButtonPressDelegate().BindUObject(this, &USettingsConfirmationWidget_Exit::UIBinding_OnDiscardButtonLeftMousePress);
		Button_DoNotSave->GetOnLeftMouseButtonReleasedDelegate().BindUObject(this, &USettingsConfirmationWidget_Exit::UIBinding_OnDiscardButtonLeftMouseRelease);
		Button_DoNotSave->GetOnRightMouseButtonPressDelegate().BindUObject(this, &USettingsConfirmationWidget_Exit::UIBinding_OnDiscardButtonRightMousePress);
		Button_DoNotSave->GetOnRightMouseButtonReleasedDelegate().BindUObject(this, &USettingsConfirmationWidget_Exit::UIBinding_OnDiscardButtonRightMouseRelease);
		Button_DoNotSave->SetImagesAndSounds(GameInst->GetUnifiedButtonAssetFlags_Menus(), GameInst->GetUnifiedButtonAssets_Menus());
	}

	if (IsWidgetBound(Button_Cancel))
	{
		Button_Cancel->SetPC(PlayCon);
		Button_Cancel->SetOwningWidget();
		Button_Cancel->SetPurpose(EUIElementType::NoTooltipRequired, nullptr);
		Button_Cancel->GetOnLeftMouseButtonPressDelegate().BindUObject(this, &USettingsConfirmationWidget_Exit::UIBinding_OnCancelButtonLeftMousePress);
		Button_Cancel->GetOnLeftMouseButtonReleasedDelegate().BindUObject(this, &USettingsConfirmationWidget_Exit::UIBinding_OnCancelButtonLeftMouseRelease);
		Button_Cancel->GetOnRightMouseButtonPressDelegate().BindUObject(this, &USettingsConfirmationWidget_Exit::UIBinding_OnCancelButtonRightMousePress);
		Button_Cancel->GetOnRightMouseButtonReleasedDelegate().BindUObject(this, &USettingsConfirmationWidget_Exit::UIBinding_OnCancelButtonRightMouseRelease);
		Button_Cancel->SetImagesAndSounds(GameInst->GetUnifiedButtonAssetFlags_Menus(), GameInst->GetUnifiedButtonAssets_Menus());
	}
}

void USettingsConfirmationWidget_Exit::NativeTick(const FGeometry & MyGeometry, float InDeltaTime)
{
}

void USettingsConfirmationWidget_Exit::UIBinding_OnConfirmButtonLeftMousePress()
{
	PC->OnLMBPressed_SettingsConfirmationWidget_Confirm(Button_Save);
}

void USettingsConfirmationWidget_Exit::UIBinding_OnConfirmButtonLeftMouseRelease()
{
	PC->OnLMBReleased_SettingsConfirmationWidget_Confirm(Button_Save, this);
}

void USettingsConfirmationWidget_Exit::UIBinding_OnConfirmButtonRightMousePress()
{
	PC->OnRMBPressed_SettingsConfirmationWidget_Confirm(Button_Save);
}

void USettingsConfirmationWidget_Exit::UIBinding_OnConfirmButtonRightMouseRelease()
{
	PC->OnRMBReleased_SettingsConfirmationWidget_Confirm(Button_Save);
}

void USettingsConfirmationWidget_Exit::UIBinding_OnDiscardButtonLeftMousePress()
{
	PC->OnLMBPressed_SettingsConfirmationWidget_Discard(Button_DoNotSave);
}

void USettingsConfirmationWidget_Exit::UIBinding_OnDiscardButtonLeftMouseRelease()
{
	PC->OnLMBReleased_SettingsConfirmationWidget_Discard(Button_DoNotSave, this);
}

void USettingsConfirmationWidget_Exit::UIBinding_OnDiscardButtonRightMousePress()
{
	PC->OnRMBPressed_SettingsConfirmationWidget_Discard(Button_DoNotSave);
}

void USettingsConfirmationWidget_Exit::UIBinding_OnDiscardButtonRightMouseRelease()
{
	PC->OnRMBReleased_SettingsConfirmationWidget_Discard(Button_DoNotSave);
}

void USettingsConfirmationWidget_Exit::UIBinding_OnCancelButtonLeftMousePress()
{
	PC->OnLMBPressed_SettingsConfirmationWidget_Cancel(Button_Cancel);
}

void USettingsConfirmationWidget_Exit::UIBinding_OnCancelButtonLeftMouseRelease()
{
	PC->OnLMBReleased_SettingsConfirmationWidget_Cancel(Button_Cancel, this);
}

void USettingsConfirmationWidget_Exit::UIBinding_OnCancelButtonRightMousePress()
{
	PC->OnRMBPressed_SettingsConfirmationWidget_Cancel(Button_Cancel);
}

void USettingsConfirmationWidget_Exit::UIBinding_OnCancelButtonRightMouseRelease()
{
	PC->OnRMBReleased_SettingsConfirmationWidget_Cancel(Button_Cancel);
}

void USettingsConfirmationWidget_Exit::OnSaveButtonClicked()
{
	SetVisibility(ESlateVisibility::Hidden);
	/* Assumed EscapeRequest because I don't have a UI button that's like "Button_AskIfSaveThenReturn" */
	SettingsMenu->ApplyAndSaveChangesAndInitiateClose(ECloseInstigation::EscapeRequest);
}

void USettingsConfirmationWidget_Exit::OnDiscardButtonClicked()
{
	SetVisibility(ESlateVisibility::Hidden);
	/* Assumed EscapeRequest because I don't have a UI button that's like "Button_AskIfSaveThenReturn" */
	SettingsMenu->DiscardChangesAndInitiateClose(ECloseInstigation::EscapeRequest); 
}

void USettingsConfirmationWidget_Exit::OnCancelButtonClicked()
{
	SetVisibility(ESlateVisibility::Hidden);
}

void USettingsConfirmationWidget_Exit::OnRequestToShow()
{
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void USettingsConfirmationWidget_Exit::OnEscapeRequest()
{
	SetVisibility(ESlateVisibility::Hidden);
}


//----------------------------------------------------------------------------------------------
//==============================================================================================
//	------- Reset to Defaults Confirmation Widget -------
//==============================================================================================
//----------------------------------------------------------------------------------------------

void USettingsConfirmationWidget_ResetToDefaults::InitialSetup(URTSGameInstance * GameInst, ARTSPlayerController * PlayCon,
	USettingsWidget * InSettingsMenu)
{
	PC = PlayCon;
	SettingsMenu = InSettingsMenu;

	if (IsWidgetBound(Button_Yes))
	{
		Button_Yes->SetPC(PlayCon);
		Button_Yes->SetOwningWidget();
		Button_Yes->SetPurpose(EUIElementType::NoTooltipRequired, nullptr);
		Button_Yes->GetOnLeftMouseButtonPressDelegate().BindUObject(this, &USettingsConfirmationWidget_ResetToDefaults::UIBinding_OnYesButtonLeftMousePress);
		Button_Yes->GetOnLeftMouseButtonReleasedDelegate().BindUObject(this, &USettingsConfirmationWidget_ResetToDefaults::UIBinding_OnYesButtonLeftMouseRelease);
		Button_Yes->GetOnRightMouseButtonPressDelegate().BindUObject(this, &USettingsConfirmationWidget_ResetToDefaults::UIBinding_OnYesButtonRightMousePress);
		Button_Yes->GetOnRightMouseButtonReleasedDelegate().BindUObject(this, &USettingsConfirmationWidget_ResetToDefaults::UIBinding_OnYesButtonRightMouseRelease);
		Button_Yes->SetImagesAndSounds(GameInst->GetUnifiedButtonAssetFlags_Menus(), GameInst->GetUnifiedButtonAssets_Menus());
	}

	if (IsWidgetBound(Button_No))
	{
		Button_No->SetPC(PlayCon);
		Button_No->SetOwningWidget();
		Button_No->SetPurpose(EUIElementType::NoTooltipRequired, nullptr);
		Button_No->GetOnLeftMouseButtonPressDelegate().BindUObject(this, &USettingsConfirmationWidget_ResetToDefaults::UIBinding_OnNoButtonLeftMousePress);
		Button_No->GetOnLeftMouseButtonReleasedDelegate().BindUObject(this, &USettingsConfirmationWidget_ResetToDefaults::UIBinding_OnNoButtonLeftMouseRelease);
		Button_No->GetOnRightMouseButtonPressDelegate().BindUObject(this, &USettingsConfirmationWidget_ResetToDefaults::UIBinding_OnNoButtonRightMousePress);
		Button_No->GetOnRightMouseButtonReleasedDelegate().BindUObject(this, &USettingsConfirmationWidget_ResetToDefaults::UIBinding_OnNoButtonRightMouseRelease);
		Button_No->SetImagesAndSounds(GameInst->GetUnifiedButtonAssetFlags_Menus(), GameInst->GetUnifiedButtonAssets_Menus());
	}
}

void USettingsConfirmationWidget_ResetToDefaults::NativeTick(const FGeometry & MyGeometry, float InDeltaTime)
{
}

void USettingsConfirmationWidget_ResetToDefaults::OnRequestToShow()
{
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void USettingsConfirmationWidget_ResetToDefaults::OnEscapeRequest()
{
	SetVisibility(ESlateVisibility::Hidden);
}

void USettingsConfirmationWidget_ResetToDefaults::UIBinding_OnYesButtonLeftMousePress()
{
	PC->OnLMBPressed_SettingsMenu_ConfirmResetToDefaults(Button_Yes);
}

void USettingsConfirmationWidget_ResetToDefaults::UIBinding_OnYesButtonLeftMouseRelease()
{
	PC->OnLMBReleased_SettingsMenu_ConfirmResetToDefaults(Button_Yes, this);
}

void USettingsConfirmationWidget_ResetToDefaults::UIBinding_OnYesButtonRightMousePress()
{
	PC->OnRMBPressed_SettingsMenu_ConfirmResetToDefaults(Button_Yes);
}

void USettingsConfirmationWidget_ResetToDefaults::UIBinding_OnYesButtonRightMouseRelease()
{
	PC->OnRMBReleased_SettingsMenu_ConfirmResetToDefaults(Button_Yes);
}

void USettingsConfirmationWidget_ResetToDefaults::UIBinding_OnNoButtonLeftMousePress()
{
	PC->OnLMBPressed_SettingsMenu_CancelResetToDefaults(Button_No);
}

void USettingsConfirmationWidget_ResetToDefaults::UIBinding_OnNoButtonLeftMouseRelease()
{
	PC->OnLMBReleased_SettingsMenu_CancelResetToDefaults(Button_No, this);
}

void USettingsConfirmationWidget_ResetToDefaults::UIBinding_OnNoButtonRightMousePress()
{
	PC->OnRMBPressed_SettingsMenu_CancelResetToDefaults(Button_No);
}

void USettingsConfirmationWidget_ResetToDefaults::UIBinding_OnNoButtonRightMouseRelease()
{
	PC->OnRMBReleased_SettingsMenu_CancelResetToDefaults(Button_No);
}

void USettingsConfirmationWidget_ResetToDefaults::OnYesButtonClicked()
{
	SetVisibility(ESlateVisibility::Hidden);
	SettingsMenu->ResetAllSettingsToDefaults();
}

void USettingsConfirmationWidget_ResetToDefaults::OnNoButtonClicked()
{
	SetVisibility(ESlateVisibility::Hidden);
}


//----------------------------------------------------------------------------------------------
//==============================================================================================
//	------- Main Widget -------
//==============================================================================================
//----------------------------------------------------------------------------------------------

USettingsWidget::USettingsWidget(const FObjectInitializer & ObjectInitializer)
	: Super(ObjectInitializer)
{
	DefaultSubmenu = ESettingsSubmenuType::GameSettings;
	bRememberSubmenu = false;
	UnsavedChangesBehavior_CloseFromCancelKey = ECloseRequestBehavior::Save;
}

void USettingsWidget::InitialSetup(URTSGameInstance * GameInst, ARTSPlayerController * PlayCon)
{
	GI = GameInst;
	PC = PlayCon;

	if (IsWidgetBound(Button_SaveChangesAndReturn))
	{
		Button_SaveChangesAndReturn->SetPC(PlayCon);
		Button_SaveChangesAndReturn->SetOwningWidget();
		Button_SaveChangesAndReturn->SetPurpose(EUIElementType::NoTooltipRequired, nullptr);
		Button_SaveChangesAndReturn->GetOnLeftMouseButtonPressDelegate().BindUObject(this, &USettingsWidget::UIBinding_OnSaveChangesAndReturnButtonLeftMousePress);
		Button_SaveChangesAndReturn->GetOnLeftMouseButtonReleasedDelegate().BindUObject(this, &USettingsWidget::UIBinding_OnSaveChangesAndReturnButtonLeftMouseRelease);
		Button_SaveChangesAndReturn->GetOnRightMouseButtonPressDelegate().BindUObject(this, &USettingsWidget::UIBinding_OnSaveChangesAndReturnButtonRightMousePress);
		Button_SaveChangesAndReturn->GetOnRightMouseButtonReleasedDelegate().BindUObject(this, &USettingsWidget::UIBinding_OnSaveChangesAndReturnButtonRightMouseRelease);
		Button_SaveChangesAndReturn->SetImagesAndSounds(GameInst->GetUnifiedButtonAssetFlags_Menus(), GameInst->GetUnifiedButtonAssets_Menus());
	}

	if (IsWidgetBound(Button_DiscardChangesAndReturn))
	{
		Button_DiscardChangesAndReturn->SetPC(PlayCon);
		Button_DiscardChangesAndReturn->SetOwningWidget();
		Button_DiscardChangesAndReturn->SetPurpose(EUIElementType::NoTooltipRequired, nullptr);
		Button_DiscardChangesAndReturn->GetOnLeftMouseButtonPressDelegate().BindUObject(this, &USettingsWidget::UIBinding_OnDiscardChangesAndReturnButtonLeftMousePress);
		Button_DiscardChangesAndReturn->GetOnLeftMouseButtonReleasedDelegate().BindUObject(this, &USettingsWidget::UIBinding_OnDiscardChangesAndReturnButtonLeftMouseRelease);
		Button_DiscardChangesAndReturn->GetOnRightMouseButtonPressDelegate().BindUObject(this, &USettingsWidget::UIBinding_OnDiscardChangesAndReturnButtonRightMousePress);
		Button_DiscardChangesAndReturn->GetOnRightMouseButtonReleasedDelegate().BindUObject(this, &USettingsWidget::UIBinding_OnDiscardChangesAndReturnButtonRightMouseRelease);
		Button_DiscardChangesAndReturn->SetImagesAndSounds(GameInst->GetUnifiedButtonAssetFlags_Menus(), GameInst->GetUnifiedButtonAssets_Menus());
	}

	if (IsWidgetBound(Button_ResetToDefaults))
	{
		Button_ResetToDefaults->SetPC(PlayCon);
		Button_ResetToDefaults->SetOwningWidget();
		Button_ResetToDefaults->SetPurpose(EUIElementType::NoTooltipRequired, nullptr);
		Button_ResetToDefaults->GetOnLeftMouseButtonPressDelegate().BindUObject(this, &USettingsWidget::UIBinding_OnResetToDefaultsButtonLeftMousePress);
		Button_ResetToDefaults->GetOnLeftMouseButtonReleasedDelegate().BindUObject(this, &USettingsWidget::UIBinding_OnResetToDefaultsButtonLeftMouseRelease);
		Button_ResetToDefaults->GetOnRightMouseButtonPressDelegate().BindUObject(this, &USettingsWidget::UIBinding_OnResetToDefaultsButtonRightMousePress);
		Button_ResetToDefaults->GetOnRightMouseButtonReleasedDelegate().BindUObject(this, &USettingsWidget::UIBinding_OnResetToDefaultsButtonRightMouseRelease);
		Button_ResetToDefaults->SetImagesAndSounds(GameInst->GetUnifiedButtonAssetFlags_Menus(), GameInst->GetUnifiedButtonAssets_Menus());
	}

	URTSGameUserSettings * Settings = CastChecked<URTSGameUserSettings>(UGameUserSettings::GetGameUserSettings());
	const bool bUpdateAppearancesForCurrentValues = false;

	if (IsWidgetBound(Button_Video))
	{
		Button_Video->SetPC(PlayCon);
		Button_Video->SetOwningWidget();
		Button_Video->SetPurpose(EUIElementType::NoTooltipRequired, nullptr);
		Button_Video->GetOnLeftMouseButtonPressDelegate().BindUObject(this, &USettingsWidget::UIBinding_OnSwitchToVideoSubmenuButtonLeftMousePress);
		Button_Video->GetOnLeftMouseButtonReleasedDelegate().BindUObject(this, &USettingsWidget::UIBinding_OnSwitchToVideoSubmenuButtonLeftMouseRelease);
		Button_Video->GetOnRightMouseButtonPressDelegate().BindUObject(this, &USettingsWidget::UIBinding_OnSwitchToVideoSubmenuButtonRightMousePress);
		Button_Video->GetOnRightMouseButtonReleasedDelegate().BindUObject(this, &USettingsWidget::UIBinding_OnSwitchToVideoSubmenuButtonRightMouseRelease);
		Button_Video->SetImagesAndSounds(GameInst->GetUnifiedButtonAssetFlags_Menus(), GameInst->GetUnifiedButtonAssets_Menus());
	}

	if (IsWidgetBound(Widget_Video))
	{
		Widget_Video->InitialSetup(GameInst, PlayCon, Settings, bUpdateAppearancesForCurrentValues);
	}

	if (IsWidgetBound(Button_Audio))
	{
		Button_Audio->SetPC(PlayCon);
		Button_Audio->SetOwningWidget();
		Button_Audio->SetPurpose(EUIElementType::NoTooltipRequired, nullptr);
		Button_Audio->GetOnLeftMouseButtonPressDelegate().BindUObject(this, &USettingsWidget::UIBinding_OnSwitchToAudioSubmenuButtonLeftMousePress);
		Button_Audio->GetOnLeftMouseButtonReleasedDelegate().BindUObject(this, &USettingsWidget::UIBinding_OnSwitchToAudioSubmenuButtonLeftMouseRelease);
		Button_Audio->GetOnRightMouseButtonPressDelegate().BindUObject(this, &USettingsWidget::UIBinding_OnSwitchToAudioSubmenuButtonRightMousePress);
		Button_Audio->GetOnRightMouseButtonReleasedDelegate().BindUObject(this, &USettingsWidget::UIBinding_OnSwitchToAudioSubmenuButtonRightMouseRelease);
		Button_Audio->SetImagesAndSounds(GameInst->GetUnifiedButtonAssetFlags_Menus(), GameInst->GetUnifiedButtonAssets_Menus());
	}

	if (IsWidgetBound(Widget_Audio))
	{
		Widget_Audio->InitialSetup(GameInst, PlayCon, Settings, bUpdateAppearancesForCurrentValues);
	}

	if (IsWidgetBound(Button_Controls))
	{
		Button_Controls->SetPC(PlayCon);
		Button_Controls->SetOwningWidget();
		Button_Controls->SetPurpose(EUIElementType::NoTooltipRequired, nullptr);
		Button_Controls->GetOnLeftMouseButtonPressDelegate().BindUObject(this, &USettingsWidget::UIBinding_OnSwitchToControlsSubmenuButtonLeftMousePress);
		Button_Controls->GetOnLeftMouseButtonReleasedDelegate().BindUObject(this, &USettingsWidget::UIBinding_OnSwitchToControlsSubmenuButtonLeftMouseRelease);
		Button_Controls->GetOnRightMouseButtonPressDelegate().BindUObject(this, &USettingsWidget::UIBinding_OnSwitchToControlsSubmenuButtonRightMousePress);
		Button_Controls->GetOnRightMouseButtonReleasedDelegate().BindUObject(this, &USettingsWidget::UIBinding_OnSwitchToControlsSubmenuButtonRightMouseRelease);
		Button_Controls->SetImagesAndSounds(GameInst->GetUnifiedButtonAssetFlags_Menus(), GameInst->GetUnifiedButtonAssets_Menus());
	}

	if (IsWidgetBound(Widget_Controls))
	{
		Widget_Controls->InitialSetup(GameInst, PlayCon, Settings, bUpdateAppearancesForCurrentValues);
	}

	if (IsWidgetBound(Button_KeyBindings))
	{
		Button_KeyBindings->SetPC(PlayCon);
		Button_KeyBindings->SetOwningWidget();
		Button_KeyBindings->SetPurpose(EUIElementType::NoTooltipRequired, nullptr);
		Button_KeyBindings->GetOnLeftMouseButtonPressDelegate().BindUObject(this, &USettingsWidget::UIBinding_OnSwitchToKeyBindingsSubmenuButtonLeftMousePress);
		Button_KeyBindings->GetOnLeftMouseButtonReleasedDelegate().BindUObject(this, &USettingsWidget::UIBinding_OnSwitchToKeyBindingsSubmenuButtonLeftMouseRelease);
		Button_KeyBindings->GetOnRightMouseButtonPressDelegate().BindUObject(this, &USettingsWidget::UIBinding_OnSwitchToKeyBindingsSubmenuButtonRightMousePress);
		Button_KeyBindings->GetOnRightMouseButtonReleasedDelegate().BindUObject(this, &USettingsWidget::UIBinding_OnSwitchToKeyBindingsSubmenuButtonRightMouseRelease);
		Button_KeyBindings->SetImagesAndSounds(GameInst->GetUnifiedButtonAssetFlags_Menus(), GameInst->GetUnifiedButtonAssets_Menus());
	}

	if (IsWidgetBound(Widget_KeyBindingsMenu))
	{
		Widget_KeyBindingsMenu->InitialSetup(GameInst, PlayCon, bUpdateAppearancesForCurrentValues);
	}

	if (IsWidgetBound(Button_Game))
	{
		Button_Game->SetPC(PlayCon);
		Button_Game->SetOwningWidget();
		Button_Game->SetPurpose(EUIElementType::NoTooltipRequired, nullptr);
		Button_Game->GetOnLeftMouseButtonPressDelegate().BindUObject(this, &USettingsWidget::UIBinding_OnSwitchToGameSubmenuButtonLeftMousePress);
		Button_Game->GetOnLeftMouseButtonReleasedDelegate().BindUObject(this, &USettingsWidget::UIBinding_OnSwitchToGameSubmenuButtonLeftMouseRelease);
		Button_Game->GetOnRightMouseButtonPressDelegate().BindUObject(this, &USettingsWidget::UIBinding_OnSwitchToGameSubmenuButtonRightMousePress);
		Button_Game->GetOnRightMouseButtonReleasedDelegate().BindUObject(this, &USettingsWidget::UIBinding_OnSwitchToGameSubmenuButtonRightMouseRelease);
		Button_Game->SetImagesAndSounds(GameInst->GetUnifiedButtonAssetFlags_Menus(), GameInst->GetUnifiedButtonAssets_Menus());
	}

	if (IsWidgetBound(Widget_Game))
	{
		Widget_Game->InitialSetup(GameInst, PlayCon, Settings, bUpdateAppearancesForCurrentValues);
	}

	switch (DefaultSubmenu)
	{
	case ESettingsSubmenuType::VideoSettings:
		SubmenuToShowOnNextOpen = Widget_Video;
		break;
	case ESettingsSubmenuType::AudioSettings:
		SubmenuToShowOnNextOpen = Widget_Audio;
		break;
	case ESettingsSubmenuType::ControlSettings:
		SubmenuToShowOnNextOpen = Widget_Controls;
		break;
	case ESettingsSubmenuType::KeyBindings:
		SubmenuToShowOnNextOpen = Widget_KeyBindingsMenu;
		break;
	case ESettingsSubmenuType::GameSettings:
		SubmenuToShowOnNextOpen = Widget_Game; 
		break;
	default:
		assert(0);
		break;
	}

	SetupConfirmationWidgets();
}

void USettingsWidget::SetupConfirmationWidgets()
{
	if (UsingExitConfirmationWidget())
	{
		ConfirmationWidget_Exit = CreateWidget<USettingsConfirmationWidget_Exit>(PC, ExitConfirmationWidget_BP);
		ConfirmationWidget_Exit->SetVisibility(ESlateVisibility::Hidden);
		UCanvasPanelSlot * CanvasSlot = CastChecked<UCanvasPanel>(GetRootWidget())->AddChildToCanvas(ConfirmationWidget_Exit);
		CanvasSlot->SetZOrder(EXIT_CONFIRMATION_WIDGET_Z_ORDER);
		ConfirmationWidget_Exit->InitialSetup(GI, PC, this);
	}

	if (UsingResetToDefaultsConfirmationWidget())
	{
		ConfirmationWidget_ResetToDefaults = CreateWidget<USettingsConfirmationWidget_ResetToDefaults>(PC, DefaultsConfirmationWidget_BP);
		ConfirmationWidget_ResetToDefaults->SetVisibility(ESlateVisibility::Hidden);
		UCanvasPanelSlot * CanvasSlot = CastChecked<UCanvasPanel>(GetRootWidget())->AddChildToCanvas(ConfirmationWidget_ResetToDefaults);
		CanvasSlot->SetZOrder(RESET_TO_DEFAULTS_CONFIRMATION_WIDGET_Z_ORDER);
		ConfirmationWidget_ResetToDefaults->InitialSetup(GI, PC, this);
	}
}

bool USettingsWidget::UsingExitConfirmationWidget() const
{
	return UnsavedChangesBehavior_CloseFromCancelKey == ECloseRequestBehavior::Ask
		&& ExitConfirmationWidget_BP != nullptr;
}

bool USettingsWidget::UsingResetToDefaultsConfirmationWidget() const
{
	return DefaultsConfirmationWidget_BP != nullptr;
}

void USettingsWidget::NativeTick(const FGeometry & MyGeometry, float InDeltaTime)
{
}

bool USettingsWidget::IsShowingOrPlayingShowAnim() const
{
	return GetVisibility() == ESlateVisibility::SelfHitTestInvisible;
}

void USettingsWidget::Show()
{
	if (IsWidgetBound(WidgetSwitcher_Submenus))
	{
		/* Make sure submenu we're about to show's appearance is for the currently applied settings */
		CastChecked<USettingsSubmenuBase>(SubmenuToShowOnNextOpen)->UpdateAppearanceForCurrentValues();

		// Switch to that submenu
		WidgetSwitcher_Submenus->SetActiveWidget(SubmenuToShowOnNextOpen);
	}

	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void USettingsWidget::OnEscapeRequest()
{
	/* Check if a confirmation widget is showing. If yes then execute it's 'cancelly' type 
	behavior and close it */
	if (IsExitConfirmationWidgetShowingOrPlayingShowAnim())
	{
		ConfirmationWidget_Exit->OnEscapeRequest();
	}
	else if (IsResetToDefaultsConfirmationWidgetShowingOrPlayingShowAnim())
	{
		ConfirmationWidget_ResetToDefaults->OnEscapeRequest();
	}
	else
	{
		if (UnsavedChangesBehavior_CloseFromCancelKey == ECloseRequestBehavior::DoNotSave)
		{
			DiscardChangesAndInitiateClose(ECloseInstigation::EscapeRequest);
		}
		else if (UnsavedChangesBehavior_CloseFromCancelKey == ECloseRequestBehavior::Save)
		{
			ApplyAndSaveChangesAndInitiateClose(ECloseInstigation::EscapeRequest);
		}
		else // Assumed Ask
		{
			assert(UnsavedChangesBehavior_CloseFromCancelKey == ECloseRequestBehavior::Ask);

			ShowAskSaveChangesWidget();
		}
	}
}

void USettingsWidget::ApplyAndSaveChangesAndInitiateClose(ECloseInstigation CloseInstigation)
{
	URTSGameUserSettings * Settings = CastChecked<URTSGameUserSettings>(UGameUserSettings::GetGameUserSettings());
	
	/* Apply all settings may be too much. I try to apply everything as they change so just 
	calling SaveSettings might be enough (although right now control settings do not apply 
	on change because function pointer misery) */
	Settings->ApplyAllSettings(GI, PC);
	InitiateClose(CloseInstigation);
}

void USettingsWidget::DiscardChangesAndInitiateClose(ECloseInstigation CloseInstigation)
{
	URTSGameUserSettings * Settings = CastChecked<URTSGameUserSettings>(UGameUserSettings::GetGameUserSettings());
	Settings->LoadSettingsNoFail(true); // Param true, false?
	/* ApplyAllSettings will save to disk but don't actually need to do that since
	we obviously just loaded from ini file */
	Settings->ApplyAllSettings(GI, PC);
	InitiateClose(CloseInstigation);
}

void USettingsWidget::ShowAskSaveChangesWidget()
{
	ConfirmationWidget_Exit->OnRequestToShow();
}

void USettingsWidget::ShowConfirmResetToDefaultsWidget()
{
	ConfirmationWidget_ResetToDefaults->OnRequestToShow();
}

void USettingsWidget::InitiateClose(ECloseInstigation CloseInstigation)
{
	SetVisibility(ESlateVisibility::Hidden);

	if (bRememberSubmenu)
	{
		if (IsWidgetBound(WidgetSwitcher_Submenus))
		{
			SubmenuToShowOnNextOpen = WidgetSwitcher_Submenus->GetActiveWidget();
		}
	}

	if (CloseInstigation == ECloseInstigation::ButtonWidget)
	{
		/* Actually nothing needs to be done here for the in match case. Setting vis above 
		to hidden is good enough because the pause menu will query IsShowingOrPlayingShowAnim() 
		to decide whether it should close this widget on escape request. 
		The InMainMenu case *might* need something. I havent implemented escape reqeusts for 
		main menu widgets yet */
		//if (GI->IsInMainMenu())
		//{
		//	// TODO let something know
		//}
		//else
		//{
		//	PauseMenuRefSomehow->
		//}
	}
}

bool USettingsWidget::IsExitConfirmationWidgetShowingOrPlayingShowAnim() const
{
	return ConfirmationWidget_Exit != nullptr && ConfirmationWidget_Exit->GetVisibility() != ESlateVisibility::Hidden;
}

bool USettingsWidget::IsResetToDefaultsConfirmationWidgetShowingOrPlayingShowAnim() const
{
	return ConfirmationWidget_ResetToDefaults != nullptr && ConfirmationWidget_ResetToDefaults->GetVisibility() != ESlateVisibility::Hidden;
}

void USettingsWidget::UIBinding_OnSaveChangesAndReturnButtonLeftMousePress()
{
	PC->OnLMBPressed_SettingsMenu_SaveChangesAndReturnButton(Button_SaveChangesAndReturn);
}

void USettingsWidget::UIBinding_OnSaveChangesAndReturnButtonLeftMouseRelease()
{
	PC->OnLMBReleased_SettingsMenu_SaveChangesAndReturnButton(Button_SaveChangesAndReturn, this);
}

void USettingsWidget::UIBinding_OnSaveChangesAndReturnButtonRightMousePress()
{
	PC->OnRMBPressed_SettingsMenu_SaveChangesAndReturnButton(Button_SaveChangesAndReturn);
}

void USettingsWidget::UIBinding_OnSaveChangesAndReturnButtonRightMouseRelease()
{
	PC->OnRMBReleased_SettingsMenu_SaveChangesAndReturnButton(Button_SaveChangesAndReturn);
}

void USettingsWidget::UIBinding_OnDiscardChangesAndReturnButtonLeftMousePress()
{
	PC->OnLMBPressed_SettingsMenu_DiscardChangesAndReturnButton(Button_DiscardChangesAndReturn);
}

void USettingsWidget::UIBinding_OnDiscardChangesAndReturnButtonLeftMouseRelease()
{
	PC->OnLMBReleased_SettingsMenu_DiscardChangesAndReturnButton(Button_DiscardChangesAndReturn, this);
}

void USettingsWidget::UIBinding_OnDiscardChangesAndReturnButtonRightMousePress()
{
	PC->OnRMBPressed_SettingsMenu_DiscardChangesAndReturnButton(Button_DiscardChangesAndReturn);
}

void USettingsWidget::UIBinding_OnDiscardChangesAndReturnButtonRightMouseRelease()
{
	PC->OnRMBReleased_SettingsMenu_DiscardChangesAndReturnButton(Button_DiscardChangesAndReturn);
}

void USettingsWidget::UIBinding_OnResetToDefaultsButtonLeftMousePress()
{
	PC->OnLMBPressed_SettingsMenu_ResetToDefaults(Button_ResetToDefaults);
}

void USettingsWidget::UIBinding_OnResetToDefaultsButtonLeftMouseRelease()
{
	PC->OnLMBReleased_SettingsMenu_ResetToDefaults(Button_ResetToDefaults, this);
}

void USettingsWidget::UIBinding_OnResetToDefaultsButtonRightMousePress()
{
	PC->OnRMBPressed_SettingsMenu_ResetToDefaults(Button_ResetToDefaults);
}

void USettingsWidget::UIBinding_OnResetToDefaultsButtonRightMouseRelease()
{
	PC->OnRMBReleased_SettingsMenu_ResetToDefaults(Button_ResetToDefaults);
}

void USettingsWidget::UIBinding_OnSwitchToVideoSubmenuButtonLeftMousePress()
{
	PC->OnLMBPressed_SwitchSettingsSubmenu(Button_Video);
}
void USettingsWidget::UIBinding_OnSwitchToVideoSubmenuButtonLeftMouseRelease()
{
	PC->OnLMBReleased_SwitchSettingsSubmenu(Button_Video, this, Widget_Video);
}

void USettingsWidget::UIBinding_OnSwitchToVideoSubmenuButtonRightMousePress()
{
	PC->OnRMBPressed_SwitchSettingsSubmenu(Button_Video);
}

void USettingsWidget::UIBinding_OnSwitchToVideoSubmenuButtonRightMouseRelease()
{
	PC->OnRMBReleased_SwitchSettingsSubmenu(Button_Video);
}

void USettingsWidget::UIBinding_OnSwitchToAudioSubmenuButtonLeftMousePress()
{
	PC->OnLMBPressed_SwitchSettingsSubmenu(Button_Audio);
}

void USettingsWidget::UIBinding_OnSwitchToAudioSubmenuButtonLeftMouseRelease()
{
	PC->OnLMBReleased_SwitchSettingsSubmenu(Button_Audio, this, Widget_Audio);
}

void USettingsWidget::UIBinding_OnSwitchToAudioSubmenuButtonRightMousePress()
{
	PC->OnRMBPressed_SwitchSettingsSubmenu(Button_Audio);
}

void USettingsWidget::UIBinding_OnSwitchToAudioSubmenuButtonRightMouseRelease()
{
	PC->OnRMBReleased_SwitchSettingsSubmenu(Button_Audio);
}

void USettingsWidget::UIBinding_OnSwitchToControlsSubmenuButtonLeftMousePress()
{
	PC->OnLMBPressed_SwitchSettingsSubmenu(Button_Controls);
}

void USettingsWidget::UIBinding_OnSwitchToControlsSubmenuButtonLeftMouseRelease()
{
	PC->OnLMBReleased_SwitchSettingsSubmenu(Button_Controls, this, Widget_Controls);
}

void USettingsWidget::UIBinding_OnSwitchToControlsSubmenuButtonRightMousePress()
{
	PC->OnRMBPressed_SwitchSettingsSubmenu(Button_Controls);
}

void USettingsWidget::UIBinding_OnSwitchToControlsSubmenuButtonRightMouseRelease()
{
	PC->OnRMBReleased_SwitchSettingsSubmenu(Button_Controls);
}

void USettingsWidget::UIBinding_OnSwitchToKeyBindingsSubmenuButtonLeftMousePress()
{
	PC->OnLMBPressed_SwitchSettingsSubmenu(Button_KeyBindings);
}

void USettingsWidget::UIBinding_OnSwitchToKeyBindingsSubmenuButtonLeftMouseRelease()
{
	PC->OnLMBReleased_SwitchSettingsSubmenu(Button_KeyBindings, this, Widget_KeyBindingsMenu);
}

void USettingsWidget::UIBinding_OnSwitchToKeyBindingsSubmenuButtonRightMousePress()
{
	PC->OnRMBPressed_SwitchSettingsSubmenu(Button_KeyBindings);
}

void USettingsWidget::UIBinding_OnSwitchToKeyBindingsSubmenuButtonRightMouseRelease()
{
	PC->OnRMBReleased_SwitchSettingsSubmenu(Button_KeyBindings);
}

void USettingsWidget::UIBinding_OnSwitchToGameSubmenuButtonLeftMousePress()
{
	PC->OnLMBPressed_SwitchSettingsSubmenu(Button_Game);
}

void USettingsWidget::UIBinding_OnSwitchToGameSubmenuButtonLeftMouseRelease()
{
	PC->OnLMBReleased_SwitchSettingsSubmenu(Button_Game, this, Widget_Game);
}

void USettingsWidget::UIBinding_OnSwitchToGameSubmenuButtonRightMousePress()
{
	PC->OnRMBPressed_SwitchSettingsSubmenu(Button_Game);
}

void USettingsWidget::UIBinding_OnSwitchToGameSubmenuButtonRightMouseRelease()
{
	PC->OnRMBReleased_SwitchSettingsSubmenu(Button_Game);
}

void USettingsWidget::OnSwitchToSubmenuButtonClicked(UWidget * WidgetToSwitchTo)
{
	if (IsWidgetBound(WidgetSwitcher_Submenus))
	{
		/* Make sure submenu we're about to show's appearance is for the currently applied settings. 
		Could perhaps just do this once each time the whole settings menu is opened e.g. if 
		switching between video settings then to audio then back to video we don't need to 
		do this for the 2nd switch back to video settings. 
		The reason I'm not doing it like that is perhaps settings in one submenu can affect 
		settings in another submenu. It's probably pretty unlikely but if that's the case then 
		we always need to do this. If there's noticable delay loading submenus then should 
		probably implement that branch here something like if (bHasVisitedThisTimeSettingsMenuIsOpen == false). 
		Will need to add that variable.
		Actually the Button_ResetToDefaults will mess this method up, although after resetting 
		to defaults we could update all widgets instead of just the one showing and problem 
		solved */
		CastChecked<USettingsSubmenuBase>(WidgetToSwitchTo)->UpdateAppearanceForCurrentValues();

		WidgetSwitcher_Submenus->SetActiveWidget(WidgetToSwitchTo);
	}
}

void USettingsWidget::OnSaveChangesAndReturnButtonClicked()
{
	ApplyAndSaveChangesAndInitiateClose(ECloseInstigation::ButtonWidget);
}

void USettingsWidget::OnDiscardChangesAndReturnButtonClicked()
{
	DiscardChangesAndInitiateClose(ECloseInstigation::ButtonWidget);
}

void USettingsWidget::OnResetToDefaultsButtonClicked()
{
	if (UsingResetToDefaultsConfirmationWidget())
	{
		ShowConfirmResetToDefaultsWidget();
	}
	else
	{
		ResetAllSettingsToDefaults();
	}
}

void USettingsWidget::ResetAllSettingsToDefaults()
{
	URTSGameUserSettings * GameUserSettings = CastChecked<URTSGameUserSettings>(UGameUserSettings::GetGameUserSettings());

	GameUserSettings->SetToDefaultsNoFail();
	GameUserSettings->ApplyAllSettings(GI, PC);

	if (IsWidgetBound(WidgetSwitcher_Submenus))
	{
		// Update appearance of currently showing widget - it is likely it changed as a result of resetting everything back to defaults
		CastChecked<USettingsSubmenuBase>(WidgetSwitcher_Submenus->GetActiveWidget())->UpdateAppearanceForCurrentValues();
	}
}

#if WITH_EDITOR
void USettingsWidget::PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	bCanEditConfirmationWidget = (UnsavedChangesBehavior_CloseFromCancelKey == ECloseRequestBehavior::Ask);
}
#endif

