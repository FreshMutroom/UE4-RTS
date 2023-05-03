// Fill out your copyright notice in the Description page of Project Settings.

#include "AudioSettingsMenu.h"
#include "Components/Slider.h"
#include "Components/ProgressBar.h"
#include "Sound/AudioSettings.h"

#include "UI/InMatchWidgets/Components/MyButton.h"


//----------------------------------------------------------------------------------------------
//==============================================================================================
//	------- Single Audio Class Widget -------
//==============================================================================================
//----------------------------------------------------------------------------------------------

void USingleAudioClassWidget::InitialSetup(URTSGameInstance * GameInst, ARTSPlayerController * PlayCon, 
	URTSGameUserSettings * Settings, bool bUpdateWidgetsToReflectAppliedValues)
{
	GI = GameInst;
	PC = PlayCon;

	if (IsWidgetBound(Button_DecreaseVolume))
	{
		Button_DecreaseVolume->SetPC(PlayCon);
		Button_DecreaseVolume->SetOwningWidget();
		Button_DecreaseVolume->SetPurpose(EUIElementType::NoTooltipRequired, nullptr);
		Button_DecreaseVolume->GetOnLeftMouseButtonPressDelegate().BindUObject(this, &USingleAudioClassWidget::UIBinding_OnDecreaseVolumeButtonLeftMousePress);
		Button_DecreaseVolume->GetOnLeftMouseButtonReleasedDelegate().BindUObject(this, &USingleAudioClassWidget::UIBinding_OnDecreaseVolumeButtonLeftMouseRelease);
		Button_DecreaseVolume->GetOnRightMouseButtonPressDelegate().BindUObject(this, &USingleAudioClassWidget::UIBinding_OnDecreaseVolumeButtonRightMousePress);
		Button_DecreaseVolume->GetOnRightMouseButtonReleasedDelegate().BindUObject(this, &USingleAudioClassWidget::UIBinding_OnDecreaseVolumeButtonRightMouseRelease);
		Button_DecreaseVolume->SetImagesAndSounds(GameInst->GetUnifiedButtonAssetFlags_Menus(), GameInst->GetUnifiedButtonAssets_Menus());
	}
	if (IsWidgetBound(Button_IncreaseVolume))
	{
		Button_IncreaseVolume->SetPC(PlayCon);
		Button_IncreaseVolume->SetOwningWidget();
		Button_IncreaseVolume->SetPurpose(EUIElementType::NoTooltipRequired, nullptr);
		Button_IncreaseVolume->GetOnLeftMouseButtonPressDelegate().BindUObject(this, &USingleAudioClassWidget::UIBinding_OnIncreaseVolumeButtonLeftMousePress);
		Button_IncreaseVolume->GetOnLeftMouseButtonReleasedDelegate().BindUObject(this, &USingleAudioClassWidget::UIBinding_OnIncreaseVolumeButtonLeftMouseRelease);
		Button_IncreaseVolume->GetOnRightMouseButtonPressDelegate().BindUObject(this, &USingleAudioClassWidget::UIBinding_OnIncreaseVolumeButtonRightMousePress);
		Button_IncreaseVolume->GetOnRightMouseButtonReleasedDelegate().BindUObject(this, &USingleAudioClassWidget::UIBinding_OnIncreaseVolumeButtonRightMouseRelease);
		Button_IncreaseVolume->SetImagesAndSounds(GameInst->GetUnifiedButtonAssetFlags_Menus(), GameInst->GetUnifiedButtonAssets_Menus());
	}
	if (IsWidgetBound(Slider_Volume))
	{
		/* Set up step size */
		Slider_Volume->SetStepSize(1.f / (URTSGameUserSettings::NUM_VOLUME_STEPS - 1));

		Slider_Volume->OnValueChanged.AddDynamic(this, &USingleAudioClassWidget::UIBinding_OnVolumeSliderValueChanged);
	}

	if (bUpdateWidgetsToReflectAppliedValues)
	{
		UpdateAppearanceForCurrentValues(Settings);
	}
}

void USingleAudioClassWidget::UpdateAppearanceForCurrentValues(URTSGameUserSettings * Settings)
{
	const float CurrentVolume = Settings->GetVolume(SoundClassName);
	const int16 CurrentStepCount = Settings->GetStepCount(SoundClassName);

	if (IsWidgetBound(Text_Volume))
	{
		Text_Volume->SetText(VolumeToText(CurrentVolume));
	}
	if (IsWidgetBound(ProgressBar_Volume))
	{
		ProgressBar_Volume->SetPercent((float)CurrentStepCount / (URTSGameUserSettings::NUM_VOLUME_STEPS - 1));
	}
	if (IsWidgetBound(Slider_Volume))
	{
		Slider_Volume->SetValue((float)CurrentStepCount / (URTSGameUserSettings::NUM_VOLUME_STEPS - 1));
	}
}

FText USingleAudioClassWidget::VolumeToText(float Volume)
{
	// Map Volume from range [VOLUME_MIN, VOLUME_MAX] to range [0, 100]
	const float ToShow = FMath::GetMappedRangeValueUnclamped(FVector2D(URTSGameUserSettings::VOLUME_MIN, URTSGameUserSettings::VOLUME_MAX), FVector2D(0.f, 100.f), Volume);

	FloatToText(ToShow, UIOptions::AudioVolumeDisplayMethod);
}

void USingleAudioClassWidget::NativeTick(const FGeometry & MyGeometry, float InDeltaTime)
{
	GInitRunaway();
}

void USingleAudioClassWidget::UIBinding_OnDecreaseVolumeButtonLeftMousePress()
{
	PC->OnLMBPressed_DecreaseVolumeButton(Button_DecreaseVolume);
}

void USingleAudioClassWidget::UIBinding_OnDecreaseVolumeButtonLeftMouseRelease()
{
	PC->OnLMBReleased_DecreaseVolumeButton(Button_DecreaseVolume, this);
}

void USingleAudioClassWidget::UIBinding_OnDecreaseVolumeButtonRightMousePress()
{
	PC->OnRMBPressed_DecreaseVolumeButton(Button_DecreaseVolume);
}

void USingleAudioClassWidget::UIBinding_OnDecreaseVolumeButtonRightMouseRelease()
{
	PC->OnRMBReleased_DecreaseVolumeButton(Button_DecreaseVolume);
}

void USingleAudioClassWidget::UIBinding_OnIncreaseVolumeButtonLeftMousePress()
{
	PC->OnLMBPressed_IncreaseVolumeButton(Button_IncreaseVolume);
}

void USingleAudioClassWidget::UIBinding_OnIncreaseVolumeButtonLeftMouseRelease()
{
	PC->OnLMBReleased_IncreaseVolumeButton(Button_IncreaseVolume, this);
}

void USingleAudioClassWidget::UIBinding_OnIncreaseVolumeButtonRightMousePress()
{
	PC->OnRMBPressed_IncreaseVolumeButton(Button_IncreaseVolume);
}

void USingleAudioClassWidget::UIBinding_OnIncreaseVolumeButtonRightMouseRelease()
{
	PC->OnRMBReleased_IncreaseVolumeButton(Button_IncreaseVolume);
}

void USingleAudioClassWidget::OnDecreaseVolumeButtonClicked()
{
	URTSGameUserSettings * Settings = CastChecked<URTSGameUserSettings>(UGameUserSettings::GetGameUserSettings());
	Settings->AdjustVolume(GI, SoundClassName, -2);

	UpdateAppearanceForCurrentValues(Settings);

	// Change has been applied but not written to disk. Will want to do that at some point, 
	// probably when the whole settings widget closes
}

void USingleAudioClassWidget::OnIncreaseVolumeButtonClicked()
{
	URTSGameUserSettings * Settings = CastChecked<URTSGameUserSettings>(UGameUserSettings::GetGameUserSettings());
	Settings->AdjustVolume(GI, SoundClassName, 2);

	UpdateAppearanceForCurrentValues(Settings);
}

void USingleAudioClassWidget::UIBinding_OnVolumeSliderValueChanged(float NewValue)
{
	URTSGameUserSettings * Settings = CastChecked<URTSGameUserSettings>(UGameUserSettings::GetGameUserSettings());
	
	// Convert NewValue to a step number 
	const int16 StepNumber = FMath::RoundHalfToZero(NewValue * (URTSGameUserSettings::NUM_VOLUME_STEPS - 1));
	
	/* If this throws then may have to clamp the above round to within these values. 
	It gets clamped anyway later on though */
	assert(StepNumber >= 0 && StepNumber <= URTSGameUserSettings::NUM_VOLUME_STEPS - 1);
	
	Settings->SetVolume(GI, SoundClassName, StepNumber);

	const float CurrentVolume = Settings->GetVolume(SoundClassName);
	const int16 CurrentStepCount = Settings->GetStepCount(SoundClassName);

	// Update the text and progress bar to the new volume
	if (IsWidgetBound(Text_Volume))
	{
		Text_Volume->SetText(VolumeToText(CurrentVolume));
	}
	if (IsWidgetBound(ProgressBar_Volume))
	{
		ProgressBar_Volume->SetPercent((float)CurrentStepCount / (URTSGameUserSettings::NUM_VOLUME_STEPS - 1));
	}
}


//----------------------------------------------------------------------------------------------
//==============================================================================================
//	------- Main Widget -------
//==============================================================================================
//----------------------------------------------------------------------------------------------

void UAudioSettingsWidget::InitialSetup(URTSGameInstance * GameInst, ARTSPlayerController * PlayCon,
	URTSGameUserSettings * Settings, bool bUpdateWidgetsToReflectAppliedValues)
{
	PC = PlayCon;

	/* Find and setup all single audio class widgets */
	TArray<UWidget *> AllChildren;
	WidgetTree->GetAllWidgets(AllChildren);
	for (UWidget * Widg : AllChildren)
	{
		USingleAudioClassWidget * Widget = Cast<USingleAudioClassWidget>(Widg);
		if (Widget != nullptr)
		{
			Widget->InitialSetup(GameInst, PlayCon, Settings, bUpdateWidgetsToReflectAppliedValues);
			SingleAudioClassWidgets.Emplace(Widget);
		}
	}

	/* Setup the widgets for audio quality */
	if (IsWidgetBound(Button_DecreaseAudioQuality))
	{
		Button_DecreaseAudioQuality->SetPC(PlayCon);
		Button_DecreaseAudioQuality->SetOwningWidget();
		Button_DecreaseAudioQuality->SetPurpose(EUIElementType::NoTooltipRequired, nullptr);
		Button_DecreaseAudioQuality->GetOnLeftMouseButtonPressDelegate().BindUObject(this, &UAudioSettingsWidget::UIBinding_OnDecreaseAudioQualityButtonLeftMousePress);
		Button_DecreaseAudioQuality->GetOnLeftMouseButtonReleasedDelegate().BindUObject(this, &UAudioSettingsWidget::UIBinding_OnDecreaseAudioQualityButtonLeftMouseRelease);
		Button_DecreaseAudioQuality->GetOnRightMouseButtonPressDelegate().BindUObject(this, &UAudioSettingsWidget::UIBinding_OnDecreaseAudioQualityButtonRightMousePress);
		Button_DecreaseAudioQuality->GetOnRightMouseButtonReleasedDelegate().BindUObject(this, &UAudioSettingsWidget::UIBinding_OnDecreaseAudioQualityButtonRightMouseRelease);
		Button_DecreaseAudioQuality->SetImagesAndSounds(GameInst->GetUnifiedButtonAssetFlags_Menus(), GameInst->GetUnifiedButtonAssets_Menus());
	}
	if (IsWidgetBound(Button_IncreaseAudioQuality))
	{
		Button_IncreaseAudioQuality->SetPC(PlayCon);
		Button_IncreaseAudioQuality->SetOwningWidget();
		Button_IncreaseAudioQuality->SetPurpose(EUIElementType::NoTooltipRequired, nullptr);
		Button_IncreaseAudioQuality->GetOnLeftMouseButtonPressDelegate().BindUObject(this, &UAudioSettingsWidget::UIBinding_OnIncreaseAudioQualityButtonLeftMousePress);
		Button_IncreaseAudioQuality->GetOnLeftMouseButtonReleasedDelegate().BindUObject(this, &UAudioSettingsWidget::UIBinding_OnIncreaseAudioQualityButtonLeftMouseRelease);
		Button_IncreaseAudioQuality->GetOnRightMouseButtonPressDelegate().BindUObject(this, &UAudioSettingsWidget::UIBinding_OnIncreaseAudioQualityButtonRightMousePress);
		Button_IncreaseAudioQuality->GetOnRightMouseButtonReleasedDelegate().BindUObject(this, &UAudioSettingsWidget::UIBinding_OnIncreaseAudioQualityButtonRightMouseRelease);
		Button_IncreaseAudioQuality->SetImagesAndSounds(GameInst->GetUnifiedButtonAssetFlags_Menus(), GameInst->GetUnifiedButtonAssets_Menus());
	}
}

void UAudioSettingsWidget::UpdateAppearanceForCurrentValues()
{
	URTSGameUserSettings * GameUserSettings = CastChecked<URTSGameUserSettings>(UGameUserSettings::GetGameUserSettings());

	for (const auto & Elem : SingleAudioClassWidgets)
	{
		Elem->UpdateAppearanceForCurrentValues(GameUserSettings);
	}

	UpdateAudioQualityWidgetsAppearanceForCurrentValues(GameUserSettings);
}

void UAudioSettingsWidget::UpdateAudioQualityWidgetsAppearanceForCurrentValues(URTSGameUserSettings * GameUserSettings)
{
	const int32 AudioQualityLevel = GameUserSettings->GetAudioQualityLevel();
	const UAudioSettings * AudioSettings = GetDefault<UAudioSettings>();
	if (IsWidgetBound(Text_AudioQuality))
	{
		Text_AudioQuality->SetText(GetAudioQualityText(AudioSettings, AudioQualityLevel));
	}
	if (IsWidgetBound(ProgressBar_AudioQuality))
	{
		float Percent;
		if (AudioQualityLevel == 0)
		{
			if (AudioSettings->QualityLevels.Num() == 0)
			{
				Percent = 1.f;
			}
			else
			{
				Percent = 0.f;
			}
		}
		else
		{
			Percent = (float)(AudioQualityLevel) / (AudioSettings->QualityLevels.Num() - 1);
		}
		
		ProgressBar_AudioQuality->SetPercent(Percent);
	}
	// Maybe wanna adjust visibility and/or opacity of Button_DecreaseAudioQuality and 
	// Button_IncreaseAudioQuality here if a max or min quality level
}

FText UAudioSettingsWidget::GetAudioQualityText(const UAudioSettings * AudioSettings, int32 AudioQualityLevel)
{
	return AudioSettings->GetQualityLevelSettings(AudioQualityLevel).DisplayName;
}

void UAudioSettingsWidget::UIBinding_OnDecreaseAudioQualityButtonLeftMousePress()
{
	PC->OnLMBPressed_DecreaseAudioQuality(Button_DecreaseAudioQuality);
}

void UAudioSettingsWidget::UIBinding_OnDecreaseAudioQualityButtonLeftMouseRelease()
{
	PC->OnLMBReleased_DecreaseAudioQuality(Button_DecreaseAudioQuality, this);
}

void UAudioSettingsWidget::UIBinding_OnDecreaseAudioQualityButtonRightMousePress()
{
	PC->OnRMBPressed_DecreaseAudioQuality(Button_DecreaseAudioQuality);
}

void UAudioSettingsWidget::UIBinding_OnDecreaseAudioQualityButtonRightMouseRelease()
{
	PC->OnRMBReleased_DecreaseAudioQuality(Button_DecreaseAudioQuality);
}

void UAudioSettingsWidget::UIBinding_OnIncreaseAudioQualityButtonLeftMousePress()
{
	PC->OnLMBPressed_IncreaseAudioQuality(Button_IncreaseAudioQuality);
}

void UAudioSettingsWidget::UIBinding_OnIncreaseAudioQualityButtonLeftMouseRelease()
{
	PC->OnLMBReleased_IncreaseAudioQuality(Button_IncreaseAudioQuality, this);
}

void UAudioSettingsWidget::UIBinding_OnIncreaseAudioQualityButtonRightMousePress()
{
	PC->OnRMBPressed_IncreaseAudioQuality(Button_IncreaseAudioQuality);
}

void UAudioSettingsWidget::UIBinding_OnIncreaseAudioQualityButtonRightMouseRelease()
{
	PC->OnRMBReleased_IncreaseAudioQuality(Button_IncreaseAudioQuality);
}

void UAudioSettingsWidget::OnDecreaseAudioQualityButtonClicked()
{
	URTSGameUserSettings * GameUserSettings = CastChecked<URTSGameUserSettings>(UGameUserSettings::GetGameUserSettings());
	const int32 AudioQualityLevel = GameUserSettings->GetAudioQualityLevel();
	// Check if there is a lower audio quality level
	if (AudioQualityLevel > 0)
	{
		GameUserSettings->SetAudioQualityLevel(AudioQualityLevel - 1);
		UpdateAudioQualityWidgetsAppearanceForCurrentValues(GameUserSettings);
	}
}

void UAudioSettingsWidget::OnIncreaseAudioQualityButtonClicked()
{
	URTSGameUserSettings * GameUserSettings = CastChecked<URTSGameUserSettings>(UGameUserSettings::GetGameUserSettings());
	const int32 AudioQualityLevel = GameUserSettings->GetAudioQualityLevel();
	const UAudioSettings * AudioSettings = GetDefault<UAudioSettings>();
	/* Check if there is a better audio quality level */
	if (AudioQualityLevel < AudioSettings->QualityLevels.Num() - 1)
	{
		GameUserSettings->SetAudioQualityLevel(AudioQualityLevel + 1);
		UpdateAudioQualityWidgetsAppearanceForCurrentValues(GameUserSettings);
	}
}
