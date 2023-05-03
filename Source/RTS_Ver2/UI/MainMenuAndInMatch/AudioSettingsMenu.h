// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/MainMenuAndInMatch/SettingsMenu.h"
#include "AudioSettingsMenu.generated.h"

class URTSGameInstance;
class ARTSPlayerController;
class USlider;
class UTextBlock;
class UProgressBar;
class UMyButton;
class USoundClass;
class URTSGameUserSettings;
class UAudioSettings;


/* A widget do display info about and allow adjusting of a single sound class */
UCLASS(Abstract)
class RTS_VER2_API USingleAudioClassWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/* @param bUpdateWidgetsToReflectAppliedValues - if true then the widgets will be updated to reflect the applied 
	values */
	void InitialSetup(URTSGameInstance * GameInst, ARTSPlayerController * PlayCon, 
		URTSGameUserSettings * Settings, bool bUpdateWidgetsToReflectAppliedValues);

	/* Update the appearance of all the widgets to reflect the current values. By current we 
	mean the values currently on the game user settings */
	void UpdateAppearanceForCurrentValues(URTSGameUserSettings * Settings);

protected:

	/* Convert a volume to the text that should be displayed for it */
	static FText VolumeToText(float Volume);

	virtual void NativeTick(const FGeometry & MyGeometry, float InDeltaTime) override;

	void UIBinding_OnDecreaseVolumeButtonLeftMousePress();
	void UIBinding_OnDecreaseVolumeButtonLeftMouseRelease();
	void UIBinding_OnDecreaseVolumeButtonRightMousePress();
	void UIBinding_OnDecreaseVolumeButtonRightMouseRelease();

	void UIBinding_OnIncreaseVolumeButtonLeftMousePress();
	void UIBinding_OnIncreaseVolumeButtonLeftMouseRelease();
	void UIBinding_OnIncreaseVolumeButtonRightMousePress();
	void UIBinding_OnIncreaseVolumeButtonRightMouseRelease();

public:

	void OnDecreaseVolumeButtonClicked();
	void OnIncreaseVolumeButtonClicked();

protected:

	UFUNCTION()
	void UIBinding_OnVolumeSliderValueChanged(float NewValue);

	static bool IsWidgetBound(UWidget * Widget) { return Widget != nullptr; }

	//-------------------------------------------------------
	//	Data
	//-------------------------------------------------------

	URTSGameInstance * GI;
	ARTSPlayerController * PC;

	/** 
	 *	The name of the sound class this widget is for. 
	 *	If you start a PIE you can check the output log in editor for the sound classes found 
	 *	during setup. 
	 *	Should probably change this to an FName for performance 
	 */
	UPROPERTY(EditAnywhere, Category = "RTS")
	FString SoundClassName;

	/* Text to show the volume */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_Volume;

	/* A progress bar to show the volume */
	UPROPERTY(meta = (BindWidgetOptional))
	UProgressBar * ProgressBar_Volume;

	/* A button to decrease the volume */
	UPROPERTY(meta = (BindWidgetOptional))
	UMyButton * Button_DecreaseVolume;

	/* A button to increase the volume */
	UPROPERTY(meta = (BindWidgetOptional))
	UMyButton * Button_IncreaseVolume;

	/* A slider to adjust the volume */
	UPROPERTY(meta = (BindWidgetOptional))
	USlider * Slider_Volume;
};


/**
 *	Widget that allows player to change their audio settings. 
 *	
 *	Note if you want to adjust audio quality levels then you need to add entries to project settings 
 *	under Audio ---> Quality Levels. The widget assumes that better audio quality levels are at
 *	higher indices in the Quality Levels array.
 */
UCLASS(Abstract)
class RTS_VER2_API UAudioSettingsWidget : public USettingsSubmenuBase
{
	GENERATED_BODY()

public:

	void InitialSetup(URTSGameInstance * GameInst, ARTSPlayerController * PlayCon, 
		URTSGameUserSettings * Settings, bool bUpdateWidgetsToReflectAppliedValues);

	//~ USettingsSubmenuBase override
	virtual void UpdateAppearanceForCurrentValues() override;

protected:

	void UpdateAudioQualityWidgetsAppearanceForCurrentValues(URTSGameUserSettings * GameUserSettings);

	static FText GetAudioQualityText(const UAudioSettings * AudioSettings, int32 AudioQualityLevel);

	void UIBinding_OnDecreaseAudioQualityButtonLeftMousePress();
	void UIBinding_OnDecreaseAudioQualityButtonLeftMouseRelease();
	void UIBinding_OnDecreaseAudioQualityButtonRightMousePress();
	void UIBinding_OnDecreaseAudioQualityButtonRightMouseRelease();

	void UIBinding_OnIncreaseAudioQualityButtonLeftMousePress();
	void UIBinding_OnIncreaseAudioQualityButtonLeftMouseRelease();
	void UIBinding_OnIncreaseAudioQualityButtonRightMousePress();
	void UIBinding_OnIncreaseAudioQualityButtonRightMouseRelease();

public:

	void OnDecreaseAudioQualityButtonClicked();
	void OnIncreaseAudioQualityButtonClicked();

protected:

	//-------------------------------------------------------
	//	Data
	//-------------------------------------------------------

	ARTSPlayerController * PC;

	TArray<USingleAudioClassWidget*> SingleAudioClassWidgets;

	/** Text to display the audio quality level */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_AudioQuality;

	/** Progress bar to display the audio quality level */
	UPROPERTY(meta = (BindWidgetOptional))
	UProgressBar * ProgressBar_AudioQuality;

	/** Button to decrease audio quality level */
	UPROPERTY(meta = (BindWidgetOptional))
	UMyButton * Button_DecreaseAudioQuality;

	/** Button to increase audio quality level */
	UPROPERTY(meta = (BindWidgetOptional))
	UMyButton * Button_IncreaseAudioQuality;
};
