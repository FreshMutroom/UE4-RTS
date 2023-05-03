// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/MainMenuAndInMatch/SettingsMenu.h"
#include "ControlSettingsMenu.generated.h"

class UTextBlock;
class UMyButton;
class UCheckBox;
class UProgressBar;
class URTSGameInstance;
class ARTSPlayerController;
class URTSGameUserSettings;
enum class EControlSettingType : uint8;
struct FControlSettingInfo;
class UControlSettingsWidget;


/* Base widget to display a single control setting */
UCLASS(Abstract)
class RTS_VER2_API USingleControlSettingWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:

	USingleControlSettingWidgetBase(const FObjectInitializer & ObjectInitializer);

	void InitialSetup(URTSGameInstance * GameInst, ARTSPlayerController * PlayCon, URTSGameUserSettings * Settings,
		UControlSettingsWidget * InControlSettingsWidget, bool bUpdateAppearancesForCurrentValues);

	/* Version when we are telling it what control setting type it should have */
	void InitialSetup(const EControlSettingType InSettingType, const FControlSettingInfo * InSettingInfo, 
		URTSGameInstance * GameInst, ARTSPlayerController * PlayCon, URTSGameUserSettings * Settings,
		UControlSettingsWidget * InControlSettingsWidget, bool bUpdateAppearancesForCurrentValues);

	virtual void UpdateAppearanceForCurrentValue() PURE_VIRTUAL(USingleControlSettingWidgetBase::UpdateAppearanceForCurrentValue, );

private:

	/* Set the text on Text_Name */
	void SetTextNameText();

	virtual void InitialSetupInner(URTSGameInstance * GameInst, ARTSPlayerController * PlayCon, URTSGameUserSettings * Settings,
		UControlSettingsWidget * InControlSettingsWidget, bool bUpdateAppearancesForCurrentValues) PURE_VIRTUAL(USingleControlSettingWidgetBase::InitialSetupInner, );

protected:

	virtual void NativeTick(const FGeometry & MyGeometry, float InDeltaTime) override;

	static bool IsWidgetBound(UWidget * Widget) { return Widget != nullptr; }

	//------------------------------------------------------
	//	Data
	//------------------------------------------------------

	ARTSPlayerController * PC;

	/* The main control settings widget */
	UControlSettingsWidget * ControlSettingsWidget;

	/* The setting this widget is for */
	UPROPERTY(EditAnywhere, Category = "RTS")
	EControlSettingType SettingType;

	// Info struct for SettingType
	const FControlSettingInfo * SettingInfo;

	/* Text to display the name of the setting */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_Name;
};


/* Widget to display a control setting that is just a bool e.g. whether to invert Y axis */
UCLASS(Abstract)
class RTS_VER2_API USingleBoolControlSettingWidget : public USingleControlSettingWidgetBase
{
	GENERATED_BODY()

protected:

	virtual void InitialSetupInner(URTSGameInstance * GameInst, ARTSPlayerController * PlayCon, URTSGameUserSettings * Settings,
		UControlSettingsWidget * InControlSettingsWidget, bool bUpdateAppearancesForCurrentValues) override;

	virtual void UpdateAppearanceForCurrentValue() override;

	void UpdateTextValueForCurrentValues(bool bIsSettingEnabled);
	void UpdateButtonsForCurrentValues(bool bIsSettingEnabled);
	void UpdateCheckBoxForCurrentValues(bool bIsSettingEnabled);

	void UIBinding_OnAdjustLeftButtonLeftMousePress();
	void UIBinding_OnAdjustLeftButtonLeftMouseRelease();
	void UIBinding_OnAdjustLeftButtonRightMousePress();
	void UIBinding_OnAdjustLeftButtonRightMouseRelease();
	void UIBinding_OnAdjustRightButtonLeftMousePress();
	void UIBinding_OnAdjustRightButtonLeftMouseRelease();
	void UIBinding_OnAdjustRightButtonRightMousePress();
	void UIBinding_OnAdjustRightButtonRightMouseRelease();

	 UFUNCTION()
	void UIBinding_OnCheckBoxStateChanged(bool bNewState);

public:

	 void OnAdjustLeftButtonClicked();
	 void OnAdjustRightButtonClicked();

protected:

	//-----------------------------------------------------
	//	Data
	//-----------------------------------------------------

	/* Text to show whether the setting is enabled or not */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_Value;

	/* Button to toggle the value of the variable */
	UPROPERTY(meta = (BindWidgetOptional))
	UMyButton * Button_AdjustLeft;

	float AdjustLeftButtonOriginalOpacity;
	float AdjustRightButtonOriginalOpacity;

	/* Another button to toggle the value of the variable */
	UPROPERTY(meta = (BindWidgetOptional))
	UMyButton * Button_AdjustRight;

	/* Check box to toggle the state of the variable */
	UPROPERTY(meta = (BindWidgetOptional))
	UCheckBox * CheckBox_CheckBox;

	/** 
	 *	If setting is true and you press the adjust right button it will be changed to false.
	 *	If setting is false and you press the adjust left button it will be changed to true.
	 *	That is what wrapping is 
	 */
	UPROPERTY(EditAnywhere, Category = "RTS")
	bool bAllowValueToWrap;
};


/* Widget to display a single control setting that uses a float e.g. camera move speed */
UCLASS(Abstract)
class RTS_VER2_API USingleFloatControlSettingWidget : public USingleControlSettingWidgetBase
{
	GENERATED_BODY()

protected:

	virtual void InitialSetupInner(URTSGameInstance * GameInst, ARTSPlayerController * PlayCon, URTSGameUserSettings * Settings,
		UControlSettingsWidget * InControlSettingsWidget, bool bUpdateAppearancesForCurrentValues) override;

	virtual void UpdateAppearanceForCurrentValue() override;

	void UIBinding_OnDecreaseButtonLeftMousePress();
	void UIBinding_OnDecreaseButtonLeftMouseRelease();
	void UIBinding_OnDecreaseButtonRightMousePress();
	void UIBinding_OnDecreaseButtonRightMouseRelease();
	void UIBinding_OnIncreaseButtonLeftMousePress();
	void UIBinding_OnIncreaseButtonLeftMouseRelease();
	void UIBinding_OnIncreaseButtonRightMousePress();
	void UIBinding_OnIncreaseButtonRightMouseRelease();

public:

	/* Not very good names. Better names are adjust left and adjust right */
	void OnDecreaseButtonClicked();
	void OnIncreaseButtonClicked();

protected:

	//-----------------------------------------------------
	//	Data
	//-----------------------------------------------------

	/* Text to show the current value of the setting */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_Value;
	
	/* Progress bar to show the value of the setting */
	UPROPERTY(meta = (BindWidgetOptional))
	UProgressBar * ProgressBar_Value;

	/* Button to lower the value */
	UPROPERTY(meta = (BindWidgetOptional))
	UMyButton * Button_Decrease;

	float DecreaseButtonOriginalOpacity;
	float IncreaseButtonOriginalOpacity;

	/* Button to increase the value */
	UPROPERTY(meta = (BindWidgetOptional))
	UMyButton * Button_Increase;
};


/* Shows control settings like camera move speed, camera zoom rate, etc */
UCLASS(Abstract)
class RTS_VER2_API UControlSettingsWidget : public USettingsSubmenuBase
{
	GENERATED_BODY()

public:

	UControlSettingsWidget(const FObjectInitializer & ObjectInitializer);
	
	void InitialSetup(URTSGameInstance * GameInst, ARTSPlayerController * PlayCon, URTSGameUserSettings * Settings, 
		bool bUpdateAppearancesForCurrentValues);

	float GetObsoleteAdjustButtonRenderOpacityMultiplier() const { return ObsoleteAdjustButtonRenderOpacityMultiplier; }
	ESlateVisibility GetObsoleteAdjustButtonVisibility() const { return ObsoleteAdjustButtonVisibility; }

	//~ Begin USettingsSubmenuBase interface
	virtual void UpdateAppearanceForCurrentValues() override;
	//~ End USettingsSubmenuBase interface

protected:

	//---------------------------------------------------------
	//	Data
	//---------------------------------------------------------

	/* Every single setting widget */
	TArray<USingleControlSettingWidgetBase*> SingleSettingWidgets;

	/* If bAutoPopulate is true then this will be populated with every control setting */
	UPROPERTY(meta = (BindWidgetOptional))
	UPanelWidget * Panel_Settings;

	/* Widget to use for bool settings if bAutoPopulate is true */
	UPROPERTY(EditAnywhere, Category = "RTS", meta = (DisplayName = "Single Bool Setting Widget", EditCondition = bAutoPopulate))
	TSubclassOf<USingleBoolControlSettingWidget> SingleBoolSettingWidget_BP;

	/* Widget to use for float settings if bAutoPopulate is true */
	UPROPERTY(EditAnywhere, Category = "RTS", meta = (DisplayName = "Single Float Setting Widget", EditCondition = bAutoPopulate))
	TSubclassOf<USingleFloatControlSettingWidget> SingleFloatSettingWidget_BP;

	/** Render opacity multiplier for adjust buttons when the value is at it's minimum/maximum */
	UPROPERTY(EditAnywhere, Category = "RTS", meta = (EditCondition = bCanEditObsoleteAdjustButtonRenderOpacityMultiplier))
	float ObsoleteAdjustButtonRenderOpacityMultiplier;

	/**
	 *	Visibility to set adjustment buttons when the control setting is at its limit 
	 *	e.g. player has their camera move speed set to 5000 which is the max value. Since it 
	 *	cannot go any higher apply this visibility to the increase button 
	 */
	UPROPERTY(EditAnywhere, Category = "RTS")
	ESlateVisibility ObsoleteAdjustButtonVisibility;

	/* If true then Panel_Settings will be populated with every control setting */
	UPROPERTY(EditAnywhere, Category = "RTS")
	bool bAutoPopulate;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	bool bCanEditObsoleteAdjustButtonRenderOpacityMultiplier;
#endif

#if WITH_EDITOR
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent) override;
	void PostEditLogic();
#endif
};
