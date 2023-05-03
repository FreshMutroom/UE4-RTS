// Fill out your copyright notice in the Description page of Project Settings.

#include "ControlSettingsMenu.h"

#include "UI/InMatchWidgets/Components/MyButton.h"
#include "Components/CheckBox.h"


//---------------------------------------------------------------------------------------------
//=============================================================================================
//	------- Base Single Widget -------
//=============================================================================================
//---------------------------------------------------------------------------------------------

USingleControlSettingWidgetBase::USingleControlSettingWidgetBase(const FObjectInitializer & ObjectInitializer)
	: Super(ObjectInitializer)
{
	SettingType = EControlSettingType::None;
}

void USingleControlSettingWidgetBase::InitialSetup(URTSGameInstance * GameInst, ARTSPlayerController * PlayCon, 
	URTSGameUserSettings * Settings, UControlSettingsWidget * InControlSettingsWidget, bool bUpdateAppearancesForCurrentValues)
{
	UE_CLOG(SettingType == EControlSettingType::None, RTSLOG, Fatal, TEXT("Widget [%s] has its "
		"control setting type set to None "), *GetClass()->GetName());

	PC = PlayCon;
	ControlSettingsWidget = InControlSettingsWidget;

	SettingInfo = &Settings->GetControlSettingInfo(SettingType);

	SetTextNameText();

	InitialSetupInner(GameInst, PlayCon, Settings, InControlSettingsWidget, bUpdateAppearancesForCurrentValues);
}

void USingleControlSettingWidgetBase::InitialSetup(const EControlSettingType InSettingType, 
	const FControlSettingInfo * InSettingInfo, URTSGameInstance * GameInst, ARTSPlayerController * PlayCon, 
	URTSGameUserSettings * Settings, UControlSettingsWidget * InControlSettingsWidget, bool bUpdateAppearancesForCurrentValues)
{
	PC = PlayCon;
	ControlSettingsWidget = InControlSettingsWidget;

	SettingType = InSettingType;
	SettingInfo = InSettingInfo;

	SetTextNameText();

	InitialSetupInner(GameInst, PlayCon, Settings, InControlSettingsWidget, bUpdateAppearancesForCurrentValues);
}

void USingleControlSettingWidgetBase::SetTextNameText()
{
	if (IsWidgetBound(Text_Name))
	{
		Text_Name->SetText(SettingInfo->GetDisplayName());
	}
}

void USingleControlSettingWidgetBase::NativeTick(const FGeometry & MyGeometry, float InDeltaTime)
{
	GInitRunaway();
}


//---------------------------------------------------------------------------------------------
//=============================================================================================
//	------- Boolean Widget -------
//=============================================================================================
//---------------------------------------------------------------------------------------------

void USingleBoolControlSettingWidget::InitialSetupInner(URTSGameInstance * GameInst, ARTSPlayerController * PlayCon, URTSGameUserSettings * Settings, UControlSettingsWidget * InControlSettingsWidget, bool bUpdateAppearancesForCurrentValues)
{
	if (IsWidgetBound(Button_AdjustLeft))
	{
		AdjustLeftButtonOriginalOpacity = Button_AdjustLeft->GetRenderOpacity();
		
		Button_AdjustLeft->SetPC(PlayCon);
		Button_AdjustLeft->SetOwningWidget();
		Button_AdjustLeft->SetPurpose(EUIElementType::NoTooltipRequired, nullptr);
		Button_AdjustLeft->GetOnLeftMouseButtonPressDelegate().BindUObject(this, &USingleBoolControlSettingWidget::UIBinding_OnAdjustLeftButtonLeftMousePress);
		Button_AdjustLeft->GetOnLeftMouseButtonReleasedDelegate().BindUObject(this, &USingleBoolControlSettingWidget::UIBinding_OnAdjustLeftButtonLeftMouseRelease);
		Button_AdjustLeft->GetOnRightMouseButtonPressDelegate().BindUObject(this, &USingleBoolControlSettingWidget::UIBinding_OnAdjustLeftButtonRightMousePress);
		Button_AdjustLeft->GetOnRightMouseButtonReleasedDelegate().BindUObject(this, &USingleBoolControlSettingWidget::UIBinding_OnAdjustLeftButtonRightMouseRelease);
		Button_AdjustLeft->SetImagesAndSounds(GameInst->GetUnifiedButtonAssetFlags_Menus(), GameInst->GetUnifiedButtonAssets_Menus());
	}

	if (IsWidgetBound(Button_AdjustRight))
	{
		AdjustRightButtonOriginalOpacity = Button_AdjustRight->GetRenderOpacity();

		Button_AdjustRight->SetPC(PlayCon);
		Button_AdjustRight->SetOwningWidget();
		Button_AdjustRight->SetPurpose(EUIElementType::NoTooltipRequired, nullptr);
		Button_AdjustRight->GetOnLeftMouseButtonPressDelegate().BindUObject(this, &USingleBoolControlSettingWidget::UIBinding_OnAdjustRightButtonLeftMousePress);
		Button_AdjustRight->GetOnLeftMouseButtonReleasedDelegate().BindUObject(this, &USingleBoolControlSettingWidget::UIBinding_OnAdjustRightButtonLeftMouseRelease);
		Button_AdjustRight->GetOnRightMouseButtonPressDelegate().BindUObject(this, &USingleBoolControlSettingWidget::UIBinding_OnAdjustRightButtonRightMousePress);
		Button_AdjustRight->GetOnRightMouseButtonReleasedDelegate().BindUObject(this, &USingleBoolControlSettingWidget::UIBinding_OnAdjustRightButtonRightMouseRelease);
		Button_AdjustRight->SetImagesAndSounds(GameInst->GetUnifiedButtonAssetFlags_Menus(), GameInst->GetUnifiedButtonAssets_Menus());
	}

	if (IsWidgetBound(CheckBox_CheckBox))
	{
		CheckBox_CheckBox->OnCheckStateChanged.AddDynamic(this, &USingleBoolControlSettingWidget::UIBinding_OnCheckBoxStateChanged);
	}

	if (bUpdateAppearancesForCurrentValues)
	{
		UpdateAppearanceForCurrentValue();
	}
}

void USingleBoolControlSettingWidget::UpdateAppearanceForCurrentValue()
{	
	const bool bIsSettingEnabled = SettingInfo->GetValue();
	UpdateTextValueForCurrentValues(bIsSettingEnabled);
	UpdateButtonsForCurrentValues(bIsSettingEnabled);
	UpdateCheckBoxForCurrentValues(bIsSettingEnabled);
}

void USingleBoolControlSettingWidget::UpdateTextValueForCurrentValues(bool bIsSettingEnabled)
{
	if (IsWidgetBound(Text_Value))
	{
		Text_Value->SetText(bIsSettingEnabled ? FText::FromString("Enabled") : FText::FromString("Disabled"));
	}
}

void USingleBoolControlSettingWidget::UpdateButtonsForCurrentValues(bool bIsSettingEnabled)
{
	if (IsWidgetBound(Button_AdjustLeft))
	{
		Button_AdjustLeft->SetVisibility(bIsSettingEnabled ? ESlateVisibility::Visible : ControlSettingsWidget->GetObsoleteAdjustButtonVisibility());
		Button_AdjustLeft->SetRenderOpacity(bIsSettingEnabled ? AdjustLeftButtonOriginalOpacity : AdjustLeftButtonOriginalOpacity * ControlSettingsWidget->GetObsoleteAdjustButtonRenderOpacityMultiplier());
	}
	if (IsWidgetBound(Button_AdjustRight))
	{
		Button_AdjustRight->SetVisibility(bIsSettingEnabled ? ControlSettingsWidget->GetObsoleteAdjustButtonVisibility() : ESlateVisibility::Visible);
		Button_AdjustRight->SetRenderOpacity(bIsSettingEnabled ? AdjustRightButtonOriginalOpacity * ControlSettingsWidget->GetObsoleteAdjustButtonRenderOpacityMultiplier() : AdjustRightButtonOriginalOpacity);
	}
}

void USingleBoolControlSettingWidget::UpdateCheckBoxForCurrentValues(bool bIsSettingEnabled)
{
	if (IsWidgetBound(CheckBox_CheckBox))
	{
		CheckBox_CheckBox->SetIsChecked(bIsSettingEnabled);
	}
}

void USingleBoolControlSettingWidget::UIBinding_OnAdjustLeftButtonLeftMousePress()
{
	PC->OnLMBPressed_AdjustBoolControlSettingLeft(Button_AdjustLeft);
}

void USingleBoolControlSettingWidget::UIBinding_OnAdjustLeftButtonLeftMouseRelease()
{
	PC->OnLMBReleased_AdjustBoolControlSettingLeft(Button_AdjustLeft, this);
}

void USingleBoolControlSettingWidget::UIBinding_OnAdjustLeftButtonRightMousePress()
{
	PC->OnRMBPressed_AdjustBoolControlSettingLeft(Button_AdjustLeft);
}

void USingleBoolControlSettingWidget::UIBinding_OnAdjustLeftButtonRightMouseRelease()
{
	PC->OnRMBReleased_AdjustBoolControlSettingLeft(Button_AdjustLeft);
}

void USingleBoolControlSettingWidget::UIBinding_OnAdjustRightButtonLeftMousePress()
{
	PC->OnLMBPressed_AdjustBoolControlSettingRight(Button_AdjustRight);
}

void USingleBoolControlSettingWidget::UIBinding_OnAdjustRightButtonLeftMouseRelease()
{
	PC->OnLMBReleased_AdjustBoolControlSettingRight(Button_AdjustRight, this);
}

void USingleBoolControlSettingWidget::UIBinding_OnAdjustRightButtonRightMousePress()
{
	PC->OnRMBPressed_AdjustBoolControlSettingRight(Button_AdjustRight);
}

void USingleBoolControlSettingWidget::UIBinding_OnAdjustRightButtonRightMouseRelease()
{
	PC->OnRMBReleased_AdjustBoolControlSettingRight(Button_AdjustRight);
}

void USingleBoolControlSettingWidget::UIBinding_OnCheckBoxStateChanged(bool bNewState)
{	
	SettingInfo->SetValue(bNewState);
	SettingInfo->ApplyValue(PC);

	UpdateTextValueForCurrentValues(bNewState);
	UpdateButtonsForCurrentValues(bNewState);
}

void USingleBoolControlSettingWidget::OnAdjustLeftButtonClicked()
{
	if (bAllowValueToWrap)
	{
		SettingInfo->AdjustValue(-1);
		SettingInfo->ApplyValue(PC);
		UpdateAppearanceForCurrentValue();
	}
	else if (SettingInfo->GetValue())
	{
		SettingInfo->SetValue(0);
		SettingInfo->ApplyValue(PC);
		UpdateAppearanceForCurrentValue();
	}
	
	// Remember that ApplyValue is currently a NOOP so will need to apply all control 
	// settings when closing the whole setting menu or something
}

void USingleBoolControlSettingWidget::OnAdjustRightButtonClicked()
{
	if (bAllowValueToWrap)
	{
		SettingInfo->AdjustValue(1);
		SettingInfo->ApplyValue(PC);
		UpdateAppearanceForCurrentValue();
	}
	else if (!SettingInfo->GetValue())
	{
		SettingInfo->SetValue(1);
		SettingInfo->ApplyValue(PC);
		UpdateAppearanceForCurrentValue();
	}
}


//---------------------------------------------------------------------------------------------
//=============================================================================================
//	------- Float Widget -------
//=============================================================================================
//---------------------------------------------------------------------------------------------

void USingleFloatControlSettingWidget::InitialSetupInner(URTSGameInstance * GameInst, ARTSPlayerController * PlayCon, URTSGameUserSettings * Settings,
	UControlSettingsWidget * InControlSettingsWidget, bool bUpdateAppearancesForCurrentValues)
{
	if (IsWidgetBound(Button_Decrease))
	{
		DecreaseButtonOriginalOpacity = Button_Decrease->GetRenderOpacity();
		
		Button_Decrease->SetPC(PlayCon);
		Button_Decrease->SetOwningWidget();
		Button_Decrease->SetPurpose(EUIElementType::NoTooltipRequired, nullptr);
		Button_Decrease->GetOnLeftMouseButtonPressDelegate().BindUObject(this, &USingleFloatControlSettingWidget::UIBinding_OnDecreaseButtonLeftMousePress);
		Button_Decrease->GetOnLeftMouseButtonReleasedDelegate().BindUObject(this, &USingleFloatControlSettingWidget::UIBinding_OnDecreaseButtonLeftMouseRelease);
		Button_Decrease->GetOnRightMouseButtonPressDelegate().BindUObject(this, &USingleFloatControlSettingWidget::UIBinding_OnDecreaseButtonRightMousePress);
		Button_Decrease->GetOnRightMouseButtonReleasedDelegate().BindUObject(this, &USingleFloatControlSettingWidget::UIBinding_OnDecreaseButtonRightMouseRelease);
		Button_Decrease->SetImagesAndSounds(GameInst->GetUnifiedButtonAssetFlags_Menus(), GameInst->GetUnifiedButtonAssets_Menus());
	}

	if (IsWidgetBound(Button_Increase))
	{
		IncreaseButtonOriginalOpacity = Button_Increase->GetRenderOpacity();
		
		Button_Increase->SetPC(PlayCon);
		Button_Increase->SetOwningWidget();
		Button_Increase->SetPurpose(EUIElementType::NoTooltipRequired, nullptr);
		Button_Increase->GetOnLeftMouseButtonPressDelegate().BindUObject(this, &USingleFloatControlSettingWidget::UIBinding_OnIncreaseButtonLeftMousePress);
		Button_Increase->GetOnLeftMouseButtonReleasedDelegate().BindUObject(this, &USingleFloatControlSettingWidget::UIBinding_OnIncreaseButtonLeftMouseRelease);
		Button_Increase->GetOnRightMouseButtonPressDelegate().BindUObject(this, &USingleFloatControlSettingWidget::UIBinding_OnIncreaseButtonRightMousePress);
		Button_Increase->GetOnRightMouseButtonReleasedDelegate().BindUObject(this, &USingleFloatControlSettingWidget::UIBinding_OnIncreaseButtonRightMouseRelease);
		Button_Increase->SetImagesAndSounds(GameInst->GetUnifiedButtonAssetFlags_Menus(), GameInst->GetUnifiedButtonAssets_Menus());
	}

	if (bUpdateAppearancesForCurrentValues)
	{
		UpdateAppearanceForCurrentValue();
	}
}

void USingleFloatControlSettingWidget::UpdateAppearanceForCurrentValue()
{	
	const float Value = SettingInfo->GetValue();
	const int16 StepValue = SettingInfo->GetStep();
	const int16 NumSteps = SettingInfo->GetNumSteps();
	if (IsWidgetBound(Text_Value))
	{
		// Just showing the step index + 1. Maybe want to allow the user to use custom values here instead
		Text_Value->SetText(FText::AsNumber(StepValue + 1));
	}
	if (IsWidgetBound(ProgressBar_Value))
	{
		ProgressBar_Value->SetPercent((float)(StepValue + 1) / NumSteps);
	}
	if (IsWidgetBound(Button_Decrease))
	{
		if (StepValue == 0)
		{
			Button_Decrease->SetVisibility(ControlSettingsWidget->GetObsoleteAdjustButtonVisibility());
			Button_Decrease->SetRenderOpacity(DecreaseButtonOriginalOpacity * ControlSettingsWidget->GetObsoleteAdjustButtonRenderOpacityMultiplier());
		}
		else
		{
			Button_Decrease->SetVisibility(ESlateVisibility::Visible);
			Button_Decrease->SetRenderOpacity(DecreaseButtonOriginalOpacity);
		}
	}
	if (IsWidgetBound(Button_Increase))
	{
		if (StepValue == NumSteps - 1)
		{
			Button_Increase->SetVisibility(ControlSettingsWidget->GetObsoleteAdjustButtonVisibility());
			Button_Increase->SetRenderOpacity(DecreaseButtonOriginalOpacity * ControlSettingsWidget->GetObsoleteAdjustButtonRenderOpacityMultiplier());
		}
		else
		{
			Button_Increase->SetVisibility(ESlateVisibility::Visible);
			Button_Increase->SetRenderOpacity(DecreaseButtonOriginalOpacity);
		}
	}
}

void USingleFloatControlSettingWidget::UIBinding_OnDecreaseButtonLeftMousePress()
{
	PC->OnLMBPressed_DecreaseControlSetting_Float(Button_Decrease);
}

void USingleFloatControlSettingWidget::UIBinding_OnDecreaseButtonLeftMouseRelease()
{
	PC->OnLMBReleased_DecreaseControlSetting_Float(Button_Decrease, this);
}

void USingleFloatControlSettingWidget::UIBinding_OnDecreaseButtonRightMousePress()
{
	PC->OnLMBPressed_DecreaseControlSetting_Float(Button_Decrease);
}

void USingleFloatControlSettingWidget::UIBinding_OnDecreaseButtonRightMouseRelease()
{
	PC->OnRMBReleased_DecreaseControlSetting_Float(Button_Decrease);
}

void USingleFloatControlSettingWidget::UIBinding_OnIncreaseButtonLeftMousePress()
{
	PC->OnLMBPressed_IncreaseControlSetting_Float(Button_Increase);
}

void USingleFloatControlSettingWidget::UIBinding_OnIncreaseButtonLeftMouseRelease()
{
	PC->OnLMBReleased_IncreaseControlSetting_Float(Button_Increase, this);
}

void USingleFloatControlSettingWidget::UIBinding_OnIncreaseButtonRightMousePress()
{
	PC->OnRMBPressed_IncreaseControlSetting_Float(Button_Increase);
}

void USingleFloatControlSettingWidget::UIBinding_OnIncreaseButtonRightMouseRelease()
{
	PC->OnRMBReleased_IncreaseControlSetting_Float(Button_Increase);
}

void USingleFloatControlSettingWidget::OnDecreaseButtonClicked()
{
	SettingInfo->AdjustValue(-1);
	SettingInfo->ApplyValue(PC);
	UpdateAppearanceForCurrentValue();
}

void USingleFloatControlSettingWidget::OnIncreaseButtonClicked()
{
	SettingInfo->AdjustValue(1);
	SettingInfo->ApplyValue(PC);
	UpdateAppearanceForCurrentValue();
}


//---------------------------------------------------------------------------------------------
//=============================================================================================
//	------- Main Widget -------
//=============================================================================================
//---------------------------------------------------------------------------------------------

UControlSettingsWidget::UControlSettingsWidget(const FObjectInitializer & ObjectInitializer)
	: Super(ObjectInitializer)
{
	ObsoleteAdjustButtonRenderOpacityMultiplier = 0.5f;
	ObsoleteAdjustButtonVisibility = ESlateVisibility::Hidden;

#if WITH_EDITOR
	PostEditLogic();
#endif
}

void UControlSettingsWidget::InitialSetup(URTSGameInstance * GameInst, ARTSPlayerController * PlayCon, 
	URTSGameUserSettings * Settings, bool bUpdateAppearancesForCurrentValues)
{
	if (bAutoPopulate)
	{
		UE_CLOG(Panel_Settings == nullptr, RTSLOG, Fatal, TEXT("[%s]: a panel widget named "
			"\"Panel_Settings\" was not on widget"), *GetClass()->GetName());
		
		SingleSettingWidgets.Reserve(Statics::NUM_CONTROL_SETTING_TYPES);
		// Create widgets and set them up and add them to panel
		for (int32 i = 0; i < Statics::NUM_CONTROL_SETTING_TYPES; ++i)
		{
			const EControlSettingType SettingType = Statics::ArrayIndexToControlSettingType(i);
			const FControlSettingInfo * SettingInfo = &Settings->GetControlSettingInfo(SettingType);
			if (SettingInfo->GetVariableType() == EVariableType::Float)
			{
				UE_CLOG(SingleFloatSettingWidget_BP == nullptr, RTSLOG, Fatal, TEXT("[%s]: single float "
					"setting widget blueprint is null"), *GetClass()->GetName());

				USingleFloatControlSettingWidget * Widget = CreateWidget<USingleFloatControlSettingWidget>(PlayCon, SingleFloatSettingWidget_BP);
				Panel_Settings->AddChild(Widget);
				
				Widget->InitialSetup(SettingType, SettingInfo, GameInst, PlayCon, Settings, this, bUpdateAppearancesForCurrentValues);
				SingleSettingWidgets.Emplace(Widget);
			}
			else
			{
				UE_CLOG(SingleBoolSettingWidget_BP == nullptr, RTSLOG, Fatal, TEXT("[%s]: single bool "
					"setting widget blueprint is null"), *GetClass()->GetName());

				USingleBoolControlSettingWidget * Widget = CreateWidget<USingleBoolControlSettingWidget>(PlayCon, SingleBoolSettingWidget_BP);
				Panel_Settings->AddChild(Widget);

				Widget->InitialSetup(SettingType, SettingInfo, GameInst, PlayCon, Settings, this, bUpdateAppearancesForCurrentValues);
				SingleSettingWidgets.Emplace(Widget);
			}
		}
	}
	else
	{
		/* Set up each individial setting widget */
		TArray <UWidget *> AllChildren;
		WidgetTree->GetAllWidgets(AllChildren);
		for (UWidget * Widg : AllChildren)
		{
			USingleControlSettingWidgetBase * Widget = Cast<USingleControlSettingWidgetBase>(Widg);
			if (Widget != nullptr)
			{
				Widget->InitialSetup(GameInst, PlayCon, Settings, this, bUpdateAppearancesForCurrentValues);
				SingleSettingWidgets.Emplace(Widget);
			}
		}
	}
}

void UControlSettingsWidget::UpdateAppearanceForCurrentValues()
{
	for (const auto & Widget : SingleSettingWidgets)
	{
		Widget->UpdateAppearanceForCurrentValue();
	}
}

#if WITH_EDITOR
void UControlSettingsWidget::PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);
	PostEditLogic();
}

void UControlSettingsWidget::PostEditLogic()
{
	bCanEditObsoleteAdjustButtonRenderOpacityMultiplier =
		(ObsoleteAdjustButtonVisibility == ESlateVisibility::Visible)
		|| (ObsoleteAdjustButtonVisibility == ESlateVisibility::HitTestInvisible)
		|| (ObsoleteAdjustButtonVisibility == ESlateVisibility::SelfHitTestInvisible);
}
#endif


