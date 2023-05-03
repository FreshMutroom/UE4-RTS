// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/MainMenuAndInMatch/SettingsMenu.h"
#include "VideoSettingsMenu.generated.h"

class URTSGameUserSettings;
class UTextBlock;
class UProgressBar;
class UVideoSettingsWidget;
class USingleVideoSettingWidget;


UENUM()
enum class EVideoSettingType : uint8
{
	/* Not a value that should ever be used */
	None    UMETA(Hidden),
	
	WindowMode,
	Resolution,
	FrameRateLimit,
	VSync,
	OverallQuality, 
	ShadowQuality,
	TextureQuality,
	AntiAliasingQuality, 
	ViewDistanceQuality, 
	VisualEffectQuality, 
	PostProcessingQuality,  
	FoliageQuality,

	z_ALWAYS_LAST_IN_ENUM    UMETA(Hidden)
};


struct FVideoSettingInfoBase
{
	FVideoSettingInfoBase(const FText & InDisplayName)
		: DisplayName(InDisplayName)
	{}

	virtual void InitialSetup(URTSGameUserSettings * GameUserSettings) { };

	/* Return the text that represents the name of this setting */
	const FText & GetDisplayName() const { return DisplayName; }

	/* Get the FText to show for the value this setting currently is */
	virtual FText GetDisplayText(URTSGameUserSettings * GameUserSettings) const = 0;
	
	/* Get the normalized percentage the value is at */
	virtual float GetDisplayPercentage(URTSGameUserSettings * GameUserSettings) const = 0;
	
	/** 
	 *	Returns whether the value is allowed to wrap around when adjusting it e.g. if your
	 *	texture quality is set to max and you press the adjust right button is it allowed to
	 *	change down to the lowest? 
	 */
	virtual bool CanValueWrap() const { return false; }

	/** 
	 *	This function is also in charge of updating the UI if something changes 
	 *	
	 *	@param SingleSettingWidget 0 widget corrisponding to this setting
	 *	@return - true if something changed 
	 */
	virtual bool TryAdjustLeft(URTSGameUserSettings * GameUserSettings, USingleVideoSettingWidget * SingleSettingWidget) = 0;
	virtual bool TryAdjustRight(URTSGameUserSettings * GameUserSettings, USingleVideoSettingWidget * SingleSettingWidget) = 0;

	/* @return - true if the setting is at what is considered the lowest or worst setting */
	virtual bool IsAtLowestValue(URTSGameUserSettings * GameUserSettings) const = 0;
	/* @return - true if the setting is at what is considered the highest or best setting */
	virtual bool IsAtHighestValue(URTSGameUserSettings * GameUserSettings) const = 0;

	virtual ~FVideoSettingInfoBase() { }

protected:

	/* Return the text to display for a setting whose value goes from 0 to 4 */
	static FText GetTextForZeroToFourRange(int32 Value);
	/* Return the normalized percentage for a setting that goes from 0 to 4 */
	static float GetDisplayPercentageForZeroToFourRange(int32 Value);

	//-------------------------------------------------------------------
	//	Data
	//-------------------------------------------------------------------

	/* Name of setting */
	const FText DisplayName;
};


/**
 *	Adjust a video setting to the left. This will probably only work for the settings in
 *	UGameUserSettings that range from values 0 to 4
 *	@param NumValues - number of values the setting can take on
 */
#define TRY_ADJUST_SETTING_LEFT(Getter, Setter, NumValues)										\
if (CanValueWrap())																				\
{																								\
	const int32 CurrentValue = (GameUserSettings->* (Getter))();								\
	const int32 NewValue = (CurrentValue - 1 + NumValues) % NumValues;							\
	(GameUserSettings->* (Setter))(NewValue);													\
																								\
	SingleSettingWidget->UpdateAppearanceForCurrentValue(GameUserSettings);						\
	USingleVideoSettingWidget * OverallQualityWidget = SingleSettingWidget->GetVideoSettingsWidget()->GetSingleSettingWidget(EVideoSettingType::OverallQuality); \
	if (OverallQualityWidget != nullptr)														\
	{																							\
		OverallQualityWidget->UpdateAppearanceForCurrentValue(GameUserSettings);				\
	}																							\
																								\
	return true;																				\
}																								\
else																							\
{																								\
	if (IsAtLowestValue(GameUserSettings))														\
	{																							\
		return false;																			\
	}																							\
	else																						\
	{																							\
		const int32 CurrentValue = (GameUserSettings->* (Getter))();							\
		const int32 NewValue = CurrentValue - 1;												\
		(GameUserSettings->* (Setter))(NewValue);												\
																								\
		SingleSettingWidget->UpdateAppearanceForCurrentValue(GameUserSettings);					\
		USingleVideoSettingWidget * OverallQualityWidget = SingleSettingWidget->GetVideoSettingsWidget()->GetSingleSettingWidget(EVideoSettingType::OverallQuality); \
		if (OverallQualityWidget != nullptr)													\
		{																						\
			OverallQualityWidget->UpdateAppearanceForCurrentValue(GameUserSettings);			\
		}																						\
																								\
		return true;																			\
	}																							\
}


#define TRY_ADJUST_SETTING_RIGHT(Getter, Setter, NumValues)										\
if (CanValueWrap())																				\
{																								\
	const int32 CurrentValue = (GameUserSettings->* (Getter))();								\
	const int32 NewValue = (CurrentValue + 1) % NumValues;										\
	(GameUserSettings->* (Setter))(NewValue);													\
																								\
	/* Update UI */																				\
	SingleSettingWidget->UpdateAppearanceForCurrentValue(GameUserSettings);						\
	USingleVideoSettingWidget * OverallQualityWidget = SingleSettingWidget->GetVideoSettingsWidget()->GetSingleSettingWidget(EVideoSettingType::OverallQuality); \
	if (OverallQualityWidget != nullptr)														\
	{																							\
		OverallQualityWidget->UpdateAppearanceForCurrentValue(GameUserSettings);				\
	}																							\
																								\
	return true;																				\
}																								\
else																							\
{																								\
	if (IsAtHighestValue(GameUserSettings))														\
	{																							\
		return false;																			\
	}																							\
	else																						\
	{																							\
		const int32 CurrentValue = (GameUserSettings->* (Getter))();							\
		const int32 NewValue = CurrentValue + 1;												\
		(GameUserSettings->* (Setter))(NewValue);												\
																								\
		SingleSettingWidget->UpdateAppearanceForCurrentValue(GameUserSettings);					\
		USingleVideoSettingWidget * OverallQualityWidget = SingleSettingWidget->GetVideoSettingsWidget()->GetSingleSettingWidget(EVideoSettingType::OverallQuality); \
		if (OverallQualityWidget != nullptr)													\
		{																						\
			OverallQualityWidget->UpdateAppearanceForCurrentValue(GameUserSettings);			\
		}																						\
																								\
		return true;																			\
	}																							\
}


struct FVideoSetting_WindowMode : public FVideoSettingInfoBase
{
	FVideoSetting_WindowMode(const FText & InDisplayName)
		: FVideoSettingInfoBase(InDisplayName)
	{}
	
	virtual FText GetDisplayText(URTSGameUserSettings * GameUserSettings) const override;
	virtual float GetDisplayPercentage(URTSGameUserSettings * GameUserSettings) const override;
	virtual bool TryAdjustLeft(URTSGameUserSettings * GameUserSettings, USingleVideoSettingWidget * SingleSettingWidget) override;
	virtual bool TryAdjustRight(URTSGameUserSettings * GameUserSettings, USingleVideoSettingWidget * SingleSettingWidget) override;
	virtual bool IsAtLowestValue(URTSGameUserSettings * GameUserSettings) const override;
	virtual bool IsAtHighestValue(URTSGameUserSettings * GameUserSettings) const override;
};

struct FVideoSetting_Resolution : public FVideoSettingInfoBase
{
	FVideoSetting_Resolution(const FText & InDisplayName)
		: FVideoSettingInfoBase(InDisplayName)
	{}

	virtual void InitialSetup(URTSGameUserSettings * GameUserSettings) override;
	virtual FText GetDisplayText(URTSGameUserSettings * GameUserSettings) const override;
	virtual float GetDisplayPercentage(URTSGameUserSettings * GameUserSettings) const override;
	virtual bool TryAdjustLeft(URTSGameUserSettings * GameUserSettings, USingleVideoSettingWidget * SingleSettingWidget) override;
	virtual bool TryAdjustRight(URTSGameUserSettings * GameUserSettings, USingleVideoSettingWidget * SingleSettingWidget) override;
	virtual bool IsAtLowestValue(URTSGameUserSettings * GameUserSettings) const override;
	virtual bool IsAtHighestValue(URTSGameUserSettings * GameUserSettings) const override;

	//------------------------------------------------------------
	//	Data
	//------------------------------------------------------------

	/* Array of all resolutions the monitor can handle */
	FScreenResolutionArray ResolutionsArray;

	/* Index in ResolutionsArray that the current resolution is */
	int32 CurrentResolutionIndex; 
};

struct FVideoSetting_OverallQuality : public FVideoSettingInfoBase
{
	FVideoSetting_OverallQuality(const FText & InDisplayName)
		: FVideoSettingInfoBase(InDisplayName)
	{}

	virtual void InitialSetup(URTSGameUserSettings * GameUserSettings) override;
	virtual FText GetDisplayText(URTSGameUserSettings * GameUserSettings) const override;
	virtual float GetDisplayPercentage(URTSGameUserSettings * GameUserSettings) const override;
	virtual bool TryAdjustLeft(URTSGameUserSettings * GameUserSettings, USingleVideoSettingWidget * SingleSettingWidget) override;
	virtual bool TryAdjustRight(URTSGameUserSettings * GameUserSettings, USingleVideoSettingWidget * SingleSettingWidget) override;
	void OnChangeUpdateUI(URTSGameUserSettings * GameUserSettings, USingleVideoSettingWidget * SingleSettingWidget, int32 OldValue, int32 NewValue);
	virtual bool IsAtLowestValue(URTSGameUserSettings * GameUserSettings) const override;
	virtual bool IsAtHighestValue(URTSGameUserSettings * GameUserSettings) const override;

	//-------------------------------------------------------------
	//	Data
	//-------------------------------------------------------------

	/* What the overall quality level was at before switching to custom */
	int32 ValueBeforeSwitchingToCustom;
};

struct FVideoSetting_FoliageQuality : public FVideoSettingInfoBase
{
	FVideoSetting_FoliageQuality(const FText & InDisplayName)
		: FVideoSettingInfoBase(InDisplayName)
	{}

	virtual FText GetDisplayText(URTSGameUserSettings * GameUserSettings) const override;
	virtual float GetDisplayPercentage(URTSGameUserSettings * GameUserSettings) const override;
	virtual bool TryAdjustLeft(URTSGameUserSettings * GameUserSettings, USingleVideoSettingWidget * SingleSettingWidget) override;
	virtual bool TryAdjustRight(URTSGameUserSettings * GameUserSettings, USingleVideoSettingWidget * SingleSettingWidget) override;
	virtual bool IsAtLowestValue(URTSGameUserSettings * GameUserSettings) const override;
	virtual bool IsAtHighestValue(URTSGameUserSettings * GameUserSettings) const override;
};

struct FVideoSetting_FrameRateLimit : public FVideoSettingInfoBase
{
	FVideoSetting_FrameRateLimit(const FText & InDisplayName)
		: FVideoSettingInfoBase(InDisplayName)
	{}

	virtual FText GetDisplayText(URTSGameUserSettings * GameUserSettings) const override;
	virtual float GetDisplayPercentage(URTSGameUserSettings * GameUserSettings) const override;
	virtual bool TryAdjustLeft(URTSGameUserSettings * GameUserSettings, USingleVideoSettingWidget * SingleSettingWidget) override;
	virtual bool TryAdjustRight(URTSGameUserSettings * GameUserSettings, USingleVideoSettingWidget * SingleSettingWidget) override;
	virtual bool IsAtLowestValue(URTSGameUserSettings * GameUserSettings) const override;
	virtual bool IsAtHighestValue(URTSGameUserSettings * GameUserSettings) const override;

	static int32 FramerateAsFloatToInt(float FramerateAsFloat) { return FramerateAsFloat + 0.5f; }

	const int32 MIN = 5;
	const int32 MAX = 250;
	/* 1 is low. Pressing the buttons to change framerate limit is very tedious */
	const int32 STEP_SIZE = 1;
};

struct FVideoSetting_AntiAliasingQuality : public FVideoSettingInfoBase
{
	FVideoSetting_AntiAliasingQuality(const FText & InDisplayName)
		: FVideoSettingInfoBase(InDisplayName)
	{}

	virtual FText GetDisplayText(URTSGameUserSettings * GameUserSettings) const override;
	virtual float GetDisplayPercentage(URTSGameUserSettings * GameUserSettings) const override;
	virtual bool TryAdjustLeft(URTSGameUserSettings * GameUserSettings, USingleVideoSettingWidget * SingleSettingWidget) override;
	virtual bool TryAdjustRight(URTSGameUserSettings * GameUserSettings, USingleVideoSettingWidget * SingleSettingWidget) override;
	virtual bool IsAtLowestValue(URTSGameUserSettings * GameUserSettings) const override;
	virtual bool IsAtHighestValue(URTSGameUserSettings * GameUserSettings) const override;

	/* 11 different levels of AA: None, FXAA very low, low, medium, high, very high 
	and TAA very low, low, medium, high, very high
	
	I have written this class's functions with the assumption that 
	- the console command r.DefaultFeature.AntiAliasing 0 = none 1 = FXAA 2 = TemporalAA will set the method 
	- UGameUserSettings::SetAntiAliasingQuality affects how strong the anti-aliasing method is 
	and it's range is [0, 4] */
};

struct FVideoSetting_ShadowQuality : public FVideoSettingInfoBase
{
	FVideoSetting_ShadowQuality(const FText & InDisplayName)
		: FVideoSettingInfoBase(InDisplayName)
	{}

	virtual FText GetDisplayText(URTSGameUserSettings * GameUserSettings) const override;
	virtual float GetDisplayPercentage(URTSGameUserSettings * GameUserSettings) const override;
	virtual bool TryAdjustLeft(URTSGameUserSettings * GameUserSettings, USingleVideoSettingWidget * SingleSettingWidget) override;
	virtual bool TryAdjustRight(URTSGameUserSettings * GameUserSettings, USingleVideoSettingWidget * SingleSettingWidget) override;
	virtual bool IsAtLowestValue(URTSGameUserSettings * GameUserSettings) const override;
	virtual bool IsAtHighestValue(URTSGameUserSettings * GameUserSettings) const override;
};

struct FVideoSetting_ViewDistanceQuality : public FVideoSettingInfoBase
{
	FVideoSetting_ViewDistanceQuality(const FText & InDisplayName)
		: FVideoSettingInfoBase(InDisplayName)
	{}

	virtual FText GetDisplayText(URTSGameUserSettings * GameUserSettings) const override;
	virtual float GetDisplayPercentage(URTSGameUserSettings * GameUserSettings) const override;
	virtual bool TryAdjustLeft(URTSGameUserSettings * GameUserSettings, USingleVideoSettingWidget * SingleSettingWidget) override;
	virtual bool TryAdjustRight(URTSGameUserSettings * GameUserSettings, USingleVideoSettingWidget * SingleSettingWidget) override;
	virtual bool IsAtLowestValue(URTSGameUserSettings * GameUserSettings) const override;
	virtual bool IsAtHighestValue(URTSGameUserSettings * GameUserSettings) const override;
};

struct FVideoSetting_TextureQuality : public FVideoSettingInfoBase
{
	FVideoSetting_TextureQuality(const FText & InDisplayName)
		: FVideoSettingInfoBase(InDisplayName)
	{}

	virtual FText GetDisplayText(URTSGameUserSettings * GameUserSettings) const override;
	virtual float GetDisplayPercentage(URTSGameUserSettings * GameUserSettings) const override;
	virtual bool TryAdjustLeft(URTSGameUserSettings * GameUserSettings, USingleVideoSettingWidget * SingleSettingWidget) override;
	virtual bool TryAdjustRight(URTSGameUserSettings * GameUserSettings, USingleVideoSettingWidget * SingleSettingWidget) override;
	virtual bool IsAtLowestValue(URTSGameUserSettings * GameUserSettings) const override;
	virtual bool IsAtHighestValue(URTSGameUserSettings * GameUserSettings) const override;
};

struct FVideoSetting_VisualEffectQuality : public FVideoSettingInfoBase
{
	FVideoSetting_VisualEffectQuality(const FText & InDisplayName)
		: FVideoSettingInfoBase(InDisplayName)
	{}

	virtual FText GetDisplayText(URTSGameUserSettings * GameUserSettings) const override;
	virtual float GetDisplayPercentage(URTSGameUserSettings * GameUserSettings) const override;
	virtual bool TryAdjustLeft(URTSGameUserSettings * GameUserSettings, USingleVideoSettingWidget * SingleSettingWidget) override;
	virtual bool TryAdjustRight(URTSGameUserSettings * GameUserSettings, USingleVideoSettingWidget * SingleSettingWidget) override;
	virtual bool IsAtLowestValue(URTSGameUserSettings * GameUserSettings) const override;
	virtual bool IsAtHighestValue(URTSGameUserSettings * GameUserSettings) const override;
};

struct FVideoSetting_PostProcessingQuality : public FVideoSettingInfoBase
{
	FVideoSetting_PostProcessingQuality(const FText & InDisplayName)
		: FVideoSettingInfoBase(InDisplayName)
	{}

	virtual FText GetDisplayText(URTSGameUserSettings * GameUserSettings) const override;
	virtual float GetDisplayPercentage(URTSGameUserSettings * GameUserSettings) const override;
	virtual bool TryAdjustLeft(URTSGameUserSettings * GameUserSettings, USingleVideoSettingWidget * SingleSettingWidget) override;
	virtual bool TryAdjustRight(URTSGameUserSettings * GameUserSettings, USingleVideoSettingWidget * SingleSettingWidget) override;
	virtual bool IsAtLowestValue(URTSGameUserSettings * GameUserSettings) const override;
	virtual bool IsAtHighestValue(URTSGameUserSettings * GameUserSettings) const override;
};

struct FVideoSetting_VSync : public FVideoSettingInfoBase
{
	FVideoSetting_VSync(const FText & InDisplayName)
		: FVideoSettingInfoBase(InDisplayName)
	{}

	virtual FText GetDisplayText(URTSGameUserSettings * GameUserSettings) const override;
	virtual float GetDisplayPercentage(URTSGameUserSettings * GameUserSettings) const override;
	virtual bool TryAdjustLeft(URTSGameUserSettings * GameUserSettings, USingleVideoSettingWidget * SingleSettingWidget) override;
	virtual bool TryAdjustRight(URTSGameUserSettings * GameUserSettings, USingleVideoSettingWidget * SingleSettingWidget) override;
	virtual bool IsAtLowestValue(URTSGameUserSettings * GameUserSettings) const override;
	virtual bool IsAtHighestValue(URTSGameUserSettings * GameUserSettings) const override;
};


//-----------------------------------------------------------------------------------------------
//===============================================================================================
//	------- Widgets -------
//===============================================================================================
//-----------------------------------------------------------------------------------------------

/* Widget for a single video setting */
UCLASS(Abstract)
class RTS_VER2_API USingleVideoSettingWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	void InitialSetup(EVideoSettingType InSettingType, URTSGameInstance * GameInst, ARTSPlayerController * PlayCon,
		URTSGameUserSettings * GameUserSettings, UVideoSettingsWidget * InOwningWidget, bool bUpdateAppearanceToCurrentValues);

	void InitialSetup(URTSGameInstance * GameInst, ARTSPlayerController * PlayCon,
		URTSGameUserSettings * GameUserSettings, UVideoSettingsWidget * InOwningWidget, bool bUpdateAppearanceToCurrentValues);

protected:

	virtual void NativeTick(const FGeometry & MyGeometry, float InDeltaTime) override;

	void UIBinding_OnAdjustLeftButtonLeftMousePress();
	void UIBinding_OnAdjustLeftButtonLeftMouseRelease();
	void UIBinding_OnAdjustLeftButtonRightMousePress();
	void UIBinding_OnAdjustLeftButtonRightMouseRelease();

	void UIBinding_OnAdjustRightButtonLeftMousePress();
	void UIBinding_OnAdjustRightButtonLeftMouseRelease();
	void UIBinding_OnAdjustRightButtonRightMousePress();
	void UIBinding_OnAdjustRightButtonRightMouseRelease();

public:

	void UpdateAppearanceForCurrentValue(URTSGameUserSettings * GameUserSettings);
	void OnAdjustLeftButtonClicked();
	void OnAdjustRightButtonClicked();

	EVideoSettingType GetVideoSettingType() const { return Type; }
	
	/* Get the main video setting widget - the widget this widget belongs to */
	UVideoSettingsWidget * GetVideoSettingsWidget() const { return VideoSettingsWidget; }

protected:

	static bool IsWidgetBound(UWidget * Widget) { return Widget != nullptr; }

	//--------------------------------------------------------
	//	Data
	//--------------------------------------------------------

	ARTSPlayerController * PC;
	UVideoSettingsWidget * VideoSettingsWidget;

	/* Text that displays the name of the setting */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_Name;

	/* Text to display the value of the setting */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_Value;

	/** Progress bar to display the value of the setting */
	UPROPERTY(meta = (BindWidgetOptional))
	UProgressBar * ProgressBar_Value;

	/* Button to adjust setting to the left */
	UPROPERTY(meta = (BindWidgetOptional))
	UMyButton * Button_AdjustLeft;

	float AdjustLeftButtonOriginalOpacity;
	float AdjustRightButtonOriginalOpacity;

	/* Button to adjust setting to the right */
	UPROPERTY(meta = (BindWidgetOptional))
	UMyButton * Button_AdjustRight;

	FVideoSettingInfoBase * SettingInfo;

	/* Type of setting this widget is for */
	UPROPERTY(EditAnywhere, Category = "RTS")
	EVideoSettingType Type;
};


/* Widget that shows and allows player to change their video settings */
UCLASS(Abstract)
class RTS_VER2_API UVideoSettingsWidget : public USettingsSubmenuBase
{
	GENERATED_BODY()

public:

	UVideoSettingsWidget(const FObjectInitializer & ObjectInitializer);

	void InitialSetup(URTSGameInstance * GameInst, ARTSPlayerController * PlayCon,
		URTSGameUserSettings * GameUserSettings, bool bUpdateAppearanceToCurrentValues);

	//~ Begin USettingsSubmenuBase interface
	virtual void UpdateAppearanceForCurrentValues() override;
	//~ End USettingsSubmenuBase interface

protected:

	static int32 GetNumVideoSettingTypes();
	static EVideoSettingType ArrayIndexToVideoSettingType(int32 ArrayIndex);
	static int32 VideoSettingTypeToArrayIndex(EVideoSettingType SettingType);

public:

	float GetObsoleteAdjustButtonRenderOpacityMultiplier() const { return ObsoleteAdjustButtonRenderOpacityMultiplier; }
	ESlateVisibility GetObsoleteAdjustButtonVisibility() const { return ObsoleteAdjustButtonVisibility; }
	const TArray<USingleVideoSettingWidget*> & GetAllSingleSettingWidgets() const { return SingleSettingWidgets; }

	/* Can return null if the widget doesn't exist */
	USingleVideoSettingWidget * GetSingleSettingWidget(EVideoSettingType VideoSettingType) const;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	bool bCanEditObsoleteAdjustButtonRenderOpacityMultiplier;
#endif

#if WITH_EDITOR
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent) override;
	void RunPostEditLogic();
#endif

protected:

	//----------------------------------------------------------
	//	Data
	//----------------------------------------------------------

	/* Array of every single setting widget. Entries can be null. Ordering matters. */
	TArray<USingleVideoSettingWidget*> SingleSettingWidgets;

	/** 
	 *	If true then the setings menu will be auto populated. This was mainly added here for 
	 *	development 
	 */
	UPROPERTY(EditAnywhere, Category = "RTS")
	bool bAutoPopulate;

	/** If bAutoPopulate is true then this is the widget that is spawned for each single setting type */
	UPROPERTY(EditAnywhere, Category = "RTS", meta = (DisplayName = "Single Setting Widget", EditCondition = bAutoPopulate))
	TSubclassOf<USingleVideoSettingWidget> SingleSettingWidget_BP;

	/* If bAutoPopulate is true then this is the panel they are added to */
	UPROPERTY(meta = (BindWidgetOptional))
	UPanelWidget * Panel_Settings;

	/** Render opacity multiplier for adjust buttons when the value is at it's lowest/highest */
	UPROPERTY(EditAnywhere, Category = "RTS", meta = (EditCondition = bCanEditObsoleteAdjustButtonRenderOpacityMultiplier))
	float ObsoleteAdjustButtonRenderOpacityMultiplier;

	/**
	 *	Visibility to set adjustment buttons when the control setting is at its limit
	 */
	UPROPERTY(EditAnywhere, Category = "RTS")
	ESlateVisibility ObsoleteAdjustButtonVisibility;

public:

	/* Struct for each of the different types of settings */
	static FVideoSetting_WindowMode				SettingInfo_WindowMode;
	static FVideoSetting_Resolution				SettingInfo_Resolution;
	static FVideoSetting_OverallQuality			SettingInfo_OverallQuality;
	static FVideoSetting_FoliageQuality			SettingInfo_FoliageQuality;
	static FVideoSetting_FrameRateLimit			SettingInfo_FramerateLimit;
	static FVideoSetting_AntiAliasingQuality	SettingInfo_AntiAliasingQuality;
	static FVideoSetting_ShadowQuality			SettingInfo_ShadowQuality;
	static FVideoSetting_ViewDistanceQuality	SettingInfo_ViewDistanceQuality;
	static FVideoSetting_TextureQuality			SettingInfo_TextureQuality;
	static FVideoSetting_VisualEffectQuality	SettingInfo_VisualEffectQuality;
	static FVideoSetting_PostProcessingQuality	SettingInfo_PostProcessingQuality;
	static FVideoSetting_VSync					SettingInfo_VSync;
};
