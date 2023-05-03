// Fill out your copyright notice in the Description page of Project Settings.

#include "VideoSettingsMenu.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Blueprint/WidgetTree.h"

#include "UI/InMatchWidgets/Components/MyButton.h"
#include "Settings/RTSGameUserSettings.h"
#include "GameFramework/RTSPlayerController.h"
#include "Statics/DevelopmentStatics.h"
#include "GameFramework/RTSGameInstance.h"


//---------------------------------------------------------------------------------------------
//=============================================================================================
//	------- Single Video Setting Widget -------
//=============================================================================================
//---------------------------------------------------------------------------------------------

void USingleVideoSettingWidget::InitialSetup(EVideoSettingType InSettingType, URTSGameInstance * GameInst, ARTSPlayerController * PlayCon,
	URTSGameUserSettings * GameUserSettings, UVideoSettingsWidget * InOwningWidget, bool bUpdateAppearanceToCurrentValues)
{
	Type = InSettingType;
	InitialSetup(GameInst, PlayCon, GameUserSettings, InOwningWidget, bUpdateAppearanceToCurrentValues);
}

void USingleVideoSettingWidget::InitialSetup(URTSGameInstance * GameInst, ARTSPlayerController * PlayCon,
	URTSGameUserSettings * GameUserSettings, UVideoSettingsWidget * InOwningWidget, bool bUpdateAppearanceToCurrentValues)
{
	PC = PlayCon;
	VideoSettingsWidget = InOwningWidget;

	if (IsWidgetBound(Button_AdjustLeft))
	{
		AdjustLeftButtonOriginalOpacity = Button_AdjustLeft->GetRenderOpacity();

		Button_AdjustLeft->SetPC(PlayCon);
		Button_AdjustLeft->SetOwningWidget();
		Button_AdjustLeft->SetPurpose(EUIElementType::NoTooltipRequired, nullptr);
		Button_AdjustLeft->GetOnLeftMouseButtonPressDelegate().BindUObject(this, &USingleVideoSettingWidget::UIBinding_OnAdjustLeftButtonLeftMousePress);
		Button_AdjustLeft->GetOnLeftMouseButtonReleasedDelegate().BindUObject(this, &USingleVideoSettingWidget::UIBinding_OnAdjustLeftButtonLeftMouseRelease);
		Button_AdjustLeft->GetOnRightMouseButtonPressDelegate().BindUObject(this, &USingleVideoSettingWidget::UIBinding_OnAdjustLeftButtonRightMousePress);
		Button_AdjustLeft->GetOnRightMouseButtonReleasedDelegate().BindUObject(this, &USingleVideoSettingWidget::UIBinding_OnAdjustLeftButtonRightMouseRelease);
		Button_AdjustLeft->SetImagesAndSounds(GameInst->GetUnifiedButtonAssetFlags_Menus(), GameInst->GetUnifiedButtonAssets_Menus());
	}

	if (IsWidgetBound(Button_AdjustRight))
	{
		AdjustRightButtonOriginalOpacity = Button_AdjustRight->GetRenderOpacity();

		Button_AdjustRight->SetPC(PlayCon);
		Button_AdjustRight->SetOwningWidget();
		Button_AdjustRight->SetPurpose(EUIElementType::NoTooltipRequired, nullptr);
		Button_AdjustRight->GetOnLeftMouseButtonPressDelegate().BindUObject(this, &USingleVideoSettingWidget::UIBinding_OnAdjustRightButtonLeftMousePress);
		Button_AdjustRight->GetOnLeftMouseButtonReleasedDelegate().BindUObject(this, &USingleVideoSettingWidget::UIBinding_OnAdjustRightButtonLeftMouseRelease);
		Button_AdjustRight->GetOnRightMouseButtonPressDelegate().BindUObject(this, &USingleVideoSettingWidget::UIBinding_OnAdjustRightButtonRightMousePress);
		Button_AdjustRight->GetOnRightMouseButtonReleasedDelegate().BindUObject(this, &USingleVideoSettingWidget::UIBinding_OnAdjustRightButtonRightMouseRelease);
		Button_AdjustRight->SetImagesAndSounds(GameInst->GetUnifiedButtonAssetFlags_Menus(), GameInst->GetUnifiedButtonAssets_Menus());
	}

	// Set SettingInfo
	switch (Type)
	{
	case EVideoSettingType::WindowMode:
		SettingInfo = &UVideoSettingsWidget::SettingInfo_WindowMode;
		break;
	case EVideoSettingType::Resolution:
		SettingInfo = &UVideoSettingsWidget::SettingInfo_Resolution;
		break;
	case EVideoSettingType::OverallQuality:
		SettingInfo = &UVideoSettingsWidget::SettingInfo_OverallQuality;
		break;
	case EVideoSettingType::FoliageQuality:
		SettingInfo = &UVideoSettingsWidget::SettingInfo_FoliageQuality;
		break;
	case EVideoSettingType::FrameRateLimit:
		SettingInfo = &UVideoSettingsWidget::SettingInfo_FramerateLimit;
		break;
	case EVideoSettingType::AntiAliasingQuality:
		SettingInfo = &UVideoSettingsWidget::SettingInfo_AntiAliasingQuality;
		break;
	case EVideoSettingType::ShadowQuality:
		SettingInfo = &UVideoSettingsWidget::SettingInfo_ShadowQuality;
		break;
	case EVideoSettingType::ViewDistanceQuality:
		SettingInfo = &UVideoSettingsWidget::SettingInfo_ViewDistanceQuality;
		break;
	case EVideoSettingType::TextureQuality:
		SettingInfo = &UVideoSettingsWidget::SettingInfo_TextureQuality;
		break;
	case EVideoSettingType::VisualEffectQuality:
		SettingInfo = &UVideoSettingsWidget::SettingInfo_VisualEffectQuality;
		break;
	case EVideoSettingType::PostProcessingQuality:
		SettingInfo = &UVideoSettingsWidget::SettingInfo_PostProcessingQuality;
		break;
	case EVideoSettingType::VSync:
		SettingInfo = &UVideoSettingsWidget::SettingInfo_VSync;
		break;
	default:
		UE_LOG(RTSLOG, Fatal, TEXT("Unaccounted for type: [%s]"), TO_STRING(EVideoSettingType, Type));
		break;
	}

	if (IsWidgetBound(Text_Name))
	{
		Text_Name->SetText(SettingInfo->GetDisplayName());
	}

	if (bUpdateAppearanceToCurrentValues)
	{
		UpdateAppearanceForCurrentValue(GameUserSettings);
	}
}

void USingleVideoSettingWidget::NativeTick(const FGeometry & MyGeometry, float InDeltaTime)
{
}

void USingleVideoSettingWidget::UIBinding_OnAdjustLeftButtonLeftMousePress()
{
	PC->OnLMBPressed_AdjustVideoSettingLeft(Button_AdjustLeft);
}

void USingleVideoSettingWidget::UIBinding_OnAdjustLeftButtonLeftMouseRelease()
{
	PC->OnLMBReleased_AdjustVideoSettingLeft(Button_AdjustLeft, this);
}

void USingleVideoSettingWidget::UIBinding_OnAdjustLeftButtonRightMousePress()
{
	PC->OnRMBPressed_AdjustVideoSettingLeft(Button_AdjustLeft);
}

void USingleVideoSettingWidget::UIBinding_OnAdjustLeftButtonRightMouseRelease()
{
	PC->OnRMBReleased_AdjustVideoSettingLeft(Button_AdjustLeft);
}

void USingleVideoSettingWidget::UIBinding_OnAdjustRightButtonLeftMousePress()
{
	PC->OnLMBPressed_AdjustVideoSettingRight(Button_AdjustRight);
}

void USingleVideoSettingWidget::UIBinding_OnAdjustRightButtonLeftMouseRelease()
{
	PC->OnLMBReleased_AdjustVideoSettingRight(Button_AdjustRight, this);
}

void USingleVideoSettingWidget::UIBinding_OnAdjustRightButtonRightMousePress()
{
	PC->OnRMBPressed_AdjustVideoSettingRight(Button_AdjustRight);
}

void USingleVideoSettingWidget::UIBinding_OnAdjustRightButtonRightMouseRelease()
{
	PC->OnRMBReleased_AdjustVideoSettingRight(Button_AdjustRight);
}

void USingleVideoSettingWidget::UpdateAppearanceForCurrentValue(URTSGameUserSettings * GameUserSettings)
{
	if (IsWidgetBound(Text_Value))
	{
		Text_Value->SetText(SettingInfo->GetDisplayText(GameUserSettings));
	}
	if (IsWidgetBound(ProgressBar_Value))
	{
		ProgressBar_Value->SetPercent(SettingInfo->GetDisplayPercentage(GameUserSettings));
	}
	if (IsWidgetBound(Button_AdjustLeft))
	{
		const bool bDisabled = !SettingInfo->CanValueWrap() && SettingInfo->IsAtLowestValue(GameUserSettings);
		if (bDisabled)
		{
			Button_AdjustLeft->SetVisibility(VideoSettingsWidget->GetObsoleteAdjustButtonVisibility());
			Button_AdjustLeft->SetRenderOpacity(VideoSettingsWidget->GetObsoleteAdjustButtonRenderOpacityMultiplier() * AdjustLeftButtonOriginalOpacity);
		}
		else
		{
			Button_AdjustLeft->SetRenderOpacity(AdjustLeftButtonOriginalOpacity);
			Button_AdjustLeft->SetVisibility(ESlateVisibility::Visible);
		}
	}
	if (IsWidgetBound(Button_AdjustRight))
	{
		const bool bDisabled = !SettingInfo->CanValueWrap() && SettingInfo->IsAtHighestValue(GameUserSettings);
		if (bDisabled)
		{
			Button_AdjustRight->SetVisibility(VideoSettingsWidget->GetObsoleteAdjustButtonVisibility());
			Button_AdjustRight->SetRenderOpacity(VideoSettingsWidget->GetObsoleteAdjustButtonRenderOpacityMultiplier() * AdjustRightButtonOriginalOpacity);
		}
		else
		{
			Button_AdjustRight->SetRenderOpacity(AdjustRightButtonOriginalOpacity);
			Button_AdjustRight->SetVisibility(ESlateVisibility::Visible);
		}
	}
}

void USingleVideoSettingWidget::OnAdjustLeftButtonClicked()
{
	URTSGameUserSettings * GameUserSettings = CastChecked<URTSGameUserSettings>(UGameUserSettings::GetGameUserSettings());
	SettingInfo->TryAdjustLeft(GameUserSettings, this);
}

void USingleVideoSettingWidget::OnAdjustRightButtonClicked()
{
	URTSGameUserSettings * GameUserSettings = CastChecked<URTSGameUserSettings>(UGameUserSettings::GetGameUserSettings());
	SettingInfo->TryAdjustRight(GameUserSettings, this);
}


//---------------------------------------------------------------------------------------------
//=============================================================================================
//	------- Main Widget -------
//=============================================================================================
//---------------------------------------------------------------------------------------------

FVideoSetting_WindowMode			UVideoSettingsWidget::SettingInfo_WindowMode = FVideoSetting_WindowMode(NSLOCTEXT("Video Settings", "Window Mode", "Window Mode"));
FVideoSetting_Resolution			UVideoSettingsWidget::SettingInfo_Resolution = FVideoSetting_Resolution(NSLOCTEXT("Video Settings", "Resolution", "Resolution"));
FVideoSetting_OverallQuality		UVideoSettingsWidget::SettingInfo_OverallQuality = FVideoSetting_OverallQuality(NSLOCTEXT("Video Settings", "Overall Quality", "Overall Quality"));
FVideoSetting_FoliageQuality		UVideoSettingsWidget::SettingInfo_FoliageQuality = FVideoSetting_FoliageQuality(NSLOCTEXT("Video Settings", "Foliage Quality", "Foliage Quality"));
FVideoSetting_FrameRateLimit		UVideoSettingsWidget::SettingInfo_FramerateLimit = FVideoSetting_FrameRateLimit(NSLOCTEXT("Video Settings", "Frame Rate Limit", "Frame Rate Limit"));
FVideoSetting_AntiAliasingQuality	UVideoSettingsWidget::SettingInfo_AntiAliasingQuality = FVideoSetting_AntiAliasingQuality(NSLOCTEXT("Video Settings", "Anti-Aliasing", "Anti-Aliasing"));
FVideoSetting_ShadowQuality			UVideoSettingsWidget::SettingInfo_ShadowQuality = FVideoSetting_ShadowQuality(NSLOCTEXT("Video Settings", "Shadow Quality", "Shadow Quality"));
FVideoSetting_ViewDistanceQuality	UVideoSettingsWidget::SettingInfo_ViewDistanceQuality = FVideoSetting_ViewDistanceQuality(NSLOCTEXT("Video Settings", "View Distance", "View Distance"));
FVideoSetting_TextureQuality		UVideoSettingsWidget::SettingInfo_TextureQuality = FVideoSetting_TextureQuality(NSLOCTEXT("Video Settings", "Texture Quality", "Texture Quality"));
FVideoSetting_VisualEffectQuality	UVideoSettingsWidget::SettingInfo_VisualEffectQuality = FVideoSetting_VisualEffectQuality(NSLOCTEXT("Video Settings", "Visual Effects", "Visual Effects"));
FVideoSetting_PostProcessingQuality	UVideoSettingsWidget::SettingInfo_PostProcessingQuality = FVideoSetting_PostProcessingQuality(NSLOCTEXT("Video Settings", "Post Processing", "Post Processing"));
FVideoSetting_VSync					UVideoSettingsWidget::SettingInfo_VSync = FVideoSetting_VSync(NSLOCTEXT("Video Settings", "VSync", "VSync"));

UVideoSettingsWidget::UVideoSettingsWidget(const FObjectInitializer & ObjectInitializer)
	: Super(ObjectInitializer)
{
	ObsoleteAdjustButtonRenderOpacityMultiplier = 0.5f;
	ObsoleteAdjustButtonVisibility = ESlateVisibility::Hidden;

#if WITH_EDITOR
	RunPostEditLogic();
#endif
}

void UVideoSettingsWidget::InitialSetup(URTSGameInstance * GameInst, ARTSPlayerController * PlayCon,
	URTSGameUserSettings * GameUserSettings, bool bUpdateAppearanceToCurrentValues)
{
	static bool bHasSetupStaticInfoStructs = false;
	if (bHasSetupStaticInfoStructs == false)
	{
		bHasSetupStaticInfoStructs = true;

		SettingInfo_WindowMode.InitialSetup(GameUserSettings);
		SettingInfo_Resolution.InitialSetup(GameUserSettings);
		SettingInfo_OverallQuality.InitialSetup(GameUserSettings);
		SettingInfo_FoliageQuality.InitialSetup(GameUserSettings);
		SettingInfo_FramerateLimit.InitialSetup(GameUserSettings);
		SettingInfo_AntiAliasingQuality.InitialSetup(GameUserSettings);
		SettingInfo_ShadowQuality.InitialSetup(GameUserSettings);
		SettingInfo_ViewDistanceQuality.InitialSetup(GameUserSettings);
		SettingInfo_TextureQuality.InitialSetup(GameUserSettings);
		SettingInfo_VisualEffectQuality.InitialSetup(GameUserSettings);
		SettingInfo_PostProcessingQuality.InitialSetup(GameUserSettings);
		SettingInfo_VSync.InitialSetup(GameUserSettings);
	}
	
	const int32 NumVideoSettingTypes = GetNumVideoSettingTypes();

	if (bAutoPopulate)
	{
		UE_CLOG(IsWidgetBound(Panel_Settings) == false, RTSLOG, Fatal, TEXT("[%s]: has bAutoPopulate "
			"set to true however no widget named \"Panel_Settings\" exists"), *GetClass()->GetName());
		
		UE_CLOG(SingleSettingWidget_BP == nullptr, RTSLOG, Fatal, TEXT("[%s]: has bAutoPopulate set "
			"to true however Single Setting Widget is null"), *GetClass()->GetName());

		SingleSettingWidgets.Init(nullptr, NumVideoSettingTypes);
		/* Note down any widgets already added to the panel. We'll keep them there and 
		just auto create the ones the user has not included */
		TArray <UWidget *> AllChildren;
		WidgetTree->GetAllWidgets(AllChildren);
		for (UWidget * Widg : AllChildren)
		{
			USingleVideoSettingWidget * Widget = Cast<USingleVideoSettingWidget>(Widg);
			if (Widget != nullptr)
			{
				const int32 ArrayIndex = VideoSettingTypeToArrayIndex(Widget->GetVideoSettingType());
				if (SingleSettingWidgets[ArrayIndex] != nullptr)
				{
					UE_LOG(RTSLOG, Warning, TEXT("Video settings widget [%s] has a duplicate of a "
						"video setting type added to it. Duplicate type is: [%s]"), *GetClass()->GetName(),
						TO_STRING(EVideoSettingType, Widget->GetVideoSettingType()));

					Widget->RemoveFromViewport();
				}
				else
				{
					SingleSettingWidgets[ArrayIndex] = Widget;
					Widget->InitialSetup(GameInst, PlayCon, GameUserSettings, this, bUpdateAppearanceToCurrentValues);
				}
			}
		}
		
		/* Create widgets */
		for (int32 i = 0; i < NumVideoSettingTypes; ++i)
		{
			const EVideoSettingType SettingType = ArrayIndexToVideoSettingType(i);
			if (SingleSettingWidgets[i] == nullptr)
			{
				USingleVideoSettingWidget * Widget = CreateWidget<USingleVideoSettingWidget>(PlayCon, SingleSettingWidget_BP);
				Panel_Settings->AddChild(Widget);
				SingleSettingWidgets[i] = Widget;
				Widget->InitialSetup(SettingType, GameInst, PlayCon, GameUserSettings, this, bUpdateAppearanceToCurrentValues);
			}
		}
	}
	else
	{
		SingleSettingWidgets.Init(nullptr, NumVideoSettingTypes);
		TArray <UWidget *> AllChildren;
		WidgetTree->GetAllWidgets(AllChildren);
		for (UWidget * Widg : AllChildren)
		{
			USingleVideoSettingWidget * Widget = Cast<USingleVideoSettingWidget>(Widg);
			if (Widget != nullptr)
			{
				SingleSettingWidgets[VideoSettingTypeToArrayIndex(Widget->GetVideoSettingType())] = Widget;
				Widget->InitialSetup(GameInst, PlayCon, GameUserSettings, this, bUpdateAppearanceToCurrentValues);
			}
		}
	}
}

void UVideoSettingsWidget::UpdateAppearanceForCurrentValues()
{
	URTSGameUserSettings * GameUserSettings = CastChecked<URTSGameUserSettings>(UGameUserSettings::GetGameUserSettings());

	for (const auto & Widget : SingleSettingWidgets)
	{
		Widget->UpdateAppearanceForCurrentValue(GameUserSettings);
	}
}

int32 UVideoSettingsWidget::GetNumVideoSettingTypes()
{
	return static_cast<int32>(EVideoSettingType::z_ALWAYS_LAST_IN_ENUM) - 1;
}

EVideoSettingType UVideoSettingsWidget::ArrayIndexToVideoSettingType(int32 ArrayIndex)
{
	return static_cast<EVideoSettingType>(ArrayIndex + 1);
}

int32 UVideoSettingsWidget::VideoSettingTypeToArrayIndex(EVideoSettingType SettingType)
{
	return static_cast<int32>(SettingType) - 1;
}

USingleVideoSettingWidget * UVideoSettingsWidget::GetSingleSettingWidget(EVideoSettingType VideoSettingType) const
{
	return SingleSettingWidgets[VideoSettingTypeToArrayIndex(VideoSettingType)];
}

#if WITH_EDITOR
void UVideoSettingsWidget::PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	RunPostEditLogic();
}

void UVideoSettingsWidget::RunPostEditLogic()
{
	bCanEditObsoleteAdjustButtonRenderOpacityMultiplier =
		ObsoleteAdjustButtonVisibility == ESlateVisibility::SelfHitTestInvisible
		|| ObsoleteAdjustButtonVisibility == ESlateVisibility::HitTestInvisible
		|| ObsoleteAdjustButtonVisibility == ESlateVisibility::Visible;
}
#endif


//---------------------------------------------------------------------------------------------
//=============================================================================================
//	------- Setting Infos -------
//=============================================================================================
//---------------------------------------------------------------------------------------------

FText FVideoSettingInfoBase::GetTextForZeroToFourRange(int32 Value)
{
	// Remember the names of these in order they appear in editor are actually: 
	// low, medium, high, epic, cinematic
	switch (Value)
	{
	case 0:
		return FText::FromString("Very Low");
	case 1:
		return FText::FromString("Low");
	case 2:
		return FText::FromString("Medium");
	case 3:
		return FText::FromString("High");
	case 4:
		return FText::FromString("Very High");
	default:
		assert(0);
		return FText::FromString("Error");
	}
}

float FVideoSettingInfoBase::GetDisplayPercentageForZeroToFourRange(int32 Value)
{
	return Value * (1.f / 4.f);
}

FText FVideoSetting_WindowMode::GetDisplayText(URTSGameUserSettings * GameUserSettings) const
{
	const EWindowMode::Type WindowMode = GameUserSettings->GetFullscreenMode();
	return WindowMode == EWindowMode::Fullscreen
		? FText::FromString("Fullscreen")
		: WindowMode == EWindowMode::WindowedFullscreen
		? FText::FromString("Windowed Fullscreen")
		: FText::FromString("Windowed");
}

float FVideoSetting_WindowMode::GetDisplayPercentage(URTSGameUserSettings * GameUserSettings) const
{
	const EWindowMode::Type WindowMode = GameUserSettings->GetFullscreenMode();
	return WindowMode == EWindowMode::Fullscreen
		? 1.f
		: WindowMode == EWindowMode::WindowedFullscreen
		? 0.5f
		: 0.f;
}

bool FVideoSetting_WindowMode::TryAdjustLeft(URTSGameUserSettings * GameUserSettings, USingleVideoSettingWidget * SingleSettingWidget)
{
	if (CanValueWrap())
	{
		const int32 CurrentWindowModeAsInt = static_cast<int32>(GameUserSettings->GetFullscreenMode());
		GameUserSettings->SetFullscreenMode(EWindowMode::ConvertIntToWindowMode((CurrentWindowModeAsInt + 1) % 3));
		
		// Update UI
		SingleSettingWidget->UpdateAppearanceForCurrentValue(GameUserSettings);
		
		return true;
	}
	else
	{
		const int32 CurrentWindowModeAsInt = static_cast<int32>(GameUserSettings->GetFullscreenMode());
		if (CurrentWindowModeAsInt <= 1)
		{
			// Adjusting left means going Fullscreen --> windowed fullscreen --> windowed
			GameUserSettings->SetFullscreenMode(EWindowMode::ConvertIntToWindowMode(CurrentWindowModeAsInt + 1));
			
			// Update UI
			SingleSettingWidget->UpdateAppearanceForCurrentValue(GameUserSettings);
			
			return true;
		}
		else
		{
			return false;
		}
	}
}

bool FVideoSetting_WindowMode::TryAdjustRight(URTSGameUserSettings * GameUserSettings, USingleVideoSettingWidget * SingleSettingWidget)
{
	if (CanValueWrap())
	{
		const int32 CurrentWindowModeAsInt = static_cast<int32>(GameUserSettings->GetFullscreenMode());
		GameUserSettings->SetFullscreenMode(EWindowMode::ConvertIntToWindowMode((CurrentWindowModeAsInt - 1 + 3) % 3));
		
		// Update UI
		SingleSettingWidget->UpdateAppearanceForCurrentValue(GameUserSettings);
		
		return true;
	}
	else
	{
		const int32 CurrentWindowModeAsInt = static_cast<int32>(GameUserSettings->GetFullscreenMode());
		if (CurrentWindowModeAsInt > 0)
		{
			// Adjusting right means going from Windowed --> fullscreen windowed --> fullscreen
			GameUserSettings->SetFullscreenMode(EWindowMode::ConvertIntToWindowMode(CurrentWindowModeAsInt - 1));
			
			// Update UI
			SingleSettingWidget->UpdateAppearanceForCurrentValue(GameUserSettings);
			
			return true;
		}
		else
		{
			return false;
		}
	}
}

bool FVideoSetting_WindowMode::IsAtLowestValue(URTSGameUserSettings * GameUserSettings) const
{
	return GameUserSettings->GetFullscreenMode() == EWindowMode::Windowed;
}

bool FVideoSetting_WindowMode::IsAtHighestValue(URTSGameUserSettings * GameUserSettings) const
{
	return GameUserSettings->GetFullscreenMode() == EWindowMode::Fullscreen;
}


void FVideoSetting_Resolution::InitialSetup(URTSGameUserSettings * GameUserSettings)
{
	// Populate ResolutionsArray with all available resolutions
	RHIGetAvailableResolutions(ResolutionsArray, true);

	const FIntPoint CurrentResolution = GameUserSettings->GetScreenResolution();
	FScreenResolutionRHI ResStruct;
	ResStruct.Width = CurrentResolution.X;
	ResStruct.Height = CurrentResolution.Y;
	CurrentResolutionIndex = ResolutionsArray.IndexOfByPredicate([&ResStruct](const FScreenResolutionRHI & Other) { return ResStruct.Width == Other.Width && ResStruct.Height == Other.Height; });
}

FText FVideoSetting_Resolution::GetDisplayText(URTSGameUserSettings * GameUserSettings) const
{
	const FIntPoint CurrentResolution = GameUserSettings->GetScreenResolution();

	FString String;
	String += FString::FromInt(CurrentResolution.X);
	String += "x";
	String += FString::FromInt(CurrentResolution.Y);

	return FText::FromString(String);
}

float FVideoSetting_Resolution::GetDisplayPercentage(URTSGameUserSettings * GameUserSettings) const
{
	/* Assuming here that the array is ordered from lowest to highest resolution */
	return (float)(CurrentResolutionIndex + 1) / ResolutionsArray.Num();
}

bool FVideoSetting_Resolution::TryAdjustLeft(URTSGameUserSettings * GameUserSettings, USingleVideoSettingWidget * SingleSettingWidget)
{
	if (CanValueWrap())
	{
		CurrentResolutionIndex = (CurrentResolutionIndex - 1 + ResolutionsArray.Num()) % ResolutionsArray.Num();
		const FScreenResolutionRHI & Elem = ResolutionsArray[CurrentResolutionIndex];
		GameUserSettings->SetScreenResolution(FIntPoint(Elem.Width, Elem.Height));
		
		// Update UI
		SingleSettingWidget->UpdateAppearanceForCurrentValue(GameUserSettings);
		
		return true;
	}
	else
	{
		if (IsAtLowestValue(GameUserSettings))
		{
			return false;
		}
		else
		{
			CurrentResolutionIndex--;
			const FScreenResolutionRHI & Elem = ResolutionsArray[CurrentResolutionIndex];
			GameUserSettings->SetScreenResolution(FIntPoint(Elem.Width, Elem.Height));
			
			// Update UI
			SingleSettingWidget->UpdateAppearanceForCurrentValue(GameUserSettings);
			
			return true;
		}
	}
}

bool FVideoSetting_Resolution::TryAdjustRight(URTSGameUserSettings * GameUserSettings, USingleVideoSettingWidget * SingleSettingWidget)
{
	if (CanValueWrap())
	{
		CurrentResolutionIndex = (CurrentResolutionIndex + 1) % ResolutionsArray.Num();
		const FScreenResolutionRHI & Elem = ResolutionsArray[CurrentResolutionIndex];
		GameUserSettings->SetScreenResolution(FIntPoint(Elem.Width, Elem.Height));
		
		// Update UI
		SingleSettingWidget->UpdateAppearanceForCurrentValue(GameUserSettings);
		
		return true;
	}
	else
	{
		if (IsAtHighestValue(GameUserSettings))
		{
			return false;
		}
		else
		{
			CurrentResolutionIndex++;
			const FScreenResolutionRHI & Elem = ResolutionsArray[CurrentResolutionIndex];
			GameUserSettings->SetScreenResolution(FIntPoint(Elem.Width, Elem.Height));
			
			// Update UI
			SingleSettingWidget->UpdateAppearanceForCurrentValue(GameUserSettings);
			
			return true;
		}
	}
}

bool FVideoSetting_Resolution::IsAtLowestValue(URTSGameUserSettings * GameUserSettings) const
{
	return CurrentResolutionIndex == 0;
}

bool FVideoSetting_Resolution::IsAtHighestValue(URTSGameUserSettings * GameUserSettings) const
{
	return CurrentResolutionIndex == ResolutionsArray.Num() - 1;
}

void FVideoSetting_OverallQuality::InitialSetup(URTSGameUserSettings * GameUserSettings)
{
	ValueBeforeSwitchingToCustom = GameUserSettings->GetOverallScalabilityLevel();
	if (ValueBeforeSwitchingToCustom == -1)
	{
		/* If it's already custom just set it to medium */
		ValueBeforeSwitchingToCustom = 2;
	}
}

FText FVideoSetting_OverallQuality::GetDisplayText(URTSGameUserSettings * GameUserSettings) const
{
	const int32 Value = GameUserSettings->GetOverallScalabilityLevel();
	if (Value == -1)
	{
		return FText::FromString("Custom");
	}
	else
	{
		return GetTextForZeroToFourRange(Value);
	}
}

float FVideoSetting_OverallQuality::GetDisplayPercentage(URTSGameUserSettings * GameUserSettings) const
{
	const int32 Value = GameUserSettings->GetOverallScalabilityLevel();
	if (Value == -1)
	{
		return 1.f;
	}
	else
	{
		return GetDisplayPercentageForZeroToFourRange(Value);
	}
}

bool FVideoSetting_OverallQuality::TryAdjustLeft(URTSGameUserSettings * GameUserSettings, USingleVideoSettingWidget * SingleSettingWidget)
{
	const int32 CurrentValue = GameUserSettings->GetOverallScalabilityLevel();
	if (CurrentValue == -1)
	{
		// Set to what it was before switching to custom
		GameUserSettings->SetOverallScalabilityLevel(ValueBeforeSwitchingToCustom);
		
		// Update UI
		OnChangeUpdateUI(GameUserSettings, SingleSettingWidget, CurrentValue, GameUserSettings->GetOverallScalabilityLevel());
		
		return true;
	}
	else
	{
		if (CanValueWrap())
		{
			const int32 NewValue = (CurrentValue - 1 + 5) % 5;
			ValueBeforeSwitchingToCustom = NewValue;
			GameUserSettings->SetOverallScalabilityLevel(NewValue);
			
			// Update UI
			OnChangeUpdateUI(GameUserSettings, SingleSettingWidget, CurrentValue, GameUserSettings->GetOverallScalabilityLevel());

			return true;
		}
		else
		{
			if (IsAtLowestValue(GameUserSettings))
			{
				return false;
			}
			else
			{
				const int32 NewValue = CurrentValue - 1;
				ValueBeforeSwitchingToCustom = NewValue;
				GameUserSettings->SetOverallScalabilityLevel(NewValue);
				
				// Update UI
				OnChangeUpdateUI(GameUserSettings, SingleSettingWidget, CurrentValue, GameUserSettings->GetOverallScalabilityLevel());
				
				return true;
			}
		}
	}
}

bool FVideoSetting_OverallQuality::TryAdjustRight(URTSGameUserSettings * GameUserSettings, USingleVideoSettingWidget * SingleSettingWidget)
{
	const int32 CurrentValue = GameUserSettings->GetOverallScalabilityLevel();
	if (CurrentValue == -1)
	{
		// Set to what it was before switching to custom
		GameUserSettings->SetOverallScalabilityLevel(ValueBeforeSwitchingToCustom);
		
		// Update UI
		OnChangeUpdateUI(GameUserSettings, SingleSettingWidget, CurrentValue, GameUserSettings->GetOverallScalabilityLevel());
		
		return true;
	}
	else
	{
		if (CanValueWrap())
		{
			const int32 NewValue = (CurrentValue + 1) % 5;
			ValueBeforeSwitchingToCustom = NewValue;
			GameUserSettings->SetOverallScalabilityLevel(NewValue);
			
			// Update UI
			OnChangeUpdateUI(GameUserSettings, SingleSettingWidget, CurrentValue, GameUserSettings->GetOverallScalabilityLevel());
			
			return true;
		}
		else
		{
			if (IsAtHighestValue(GameUserSettings))
			{
				return false;
			}
			else
			{
				const int32 NewValue = CurrentValue + 1;
				ValueBeforeSwitchingToCustom = NewValue;
				GameUserSettings->SetOverallScalabilityLevel(NewValue);
				
				// Update UI
				OnChangeUpdateUI(GameUserSettings, SingleSettingWidget, CurrentValue, GameUserSettings->GetOverallScalabilityLevel());
				
				return true;
			}
		}
	}
}

void FVideoSetting_OverallQuality::OnChangeUpdateUI(URTSGameUserSettings * GameUserSettings, 
	USingleVideoSettingWidget * SingleSettingWidget, int32 OldValue, int32 NewValue)
{
	if (OldValue == -1)
	{
		if (NewValue != -1)
		{
			/* Update every single setting widget */
			const TArray<USingleVideoSettingWidget*> & AllSingleWidgets = SingleSettingWidget->GetVideoSettingsWidget()->GetAllSingleSettingWidgets();
			for (const auto & Widget : AllSingleWidgets)
			{
				if (Widget != nullptr)
				{
					Widget->UpdateAppearanceForCurrentValue(GameUserSettings);
				}
			}
		}
	}
	else
	{
		if (NewValue == -1)
		{
			/* Just update the text to now show "Custom" */
			SingleSettingWidget->UpdateAppearanceForCurrentValue(GameUserSettings);
		}
		else if (OldValue != NewValue)
		{
			/* Update every single setting widget */
			const TArray<USingleVideoSettingWidget*> & AllSingleWidgets = SingleSettingWidget->GetVideoSettingsWidget()->GetAllSingleSettingWidgets();
			for (const auto & Widget : AllSingleWidgets)
			{
				if (Widget != nullptr)
				{
					Widget->UpdateAppearanceForCurrentValue(GameUserSettings);
				}
			}
		}
	}
}

bool FVideoSetting_OverallQuality::IsAtLowestValue(URTSGameUserSettings * GameUserSettings) const
{
	return GameUserSettings->GetOverallScalabilityLevel() == 0;
}

bool FVideoSetting_OverallQuality::IsAtHighestValue(URTSGameUserSettings * GameUserSettings) const
{
	return GameUserSettings->GetOverallScalabilityLevel() == 4;
}

FText FVideoSetting_FoliageQuality::GetDisplayText(URTSGameUserSettings * GameUserSettings) const
{
	return GetTextForZeroToFourRange(GameUserSettings->GetFoliageQuality());
}

float FVideoSetting_FoliageQuality::GetDisplayPercentage(URTSGameUserSettings * GameUserSettings) const
{
	return GetDisplayPercentageForZeroToFourRange(GameUserSettings->GetFoliageQuality());
}

bool FVideoSetting_FoliageQuality::TryAdjustLeft(URTSGameUserSettings * GameUserSettings, USingleVideoSettingWidget * SingleSettingWidget)
{
	TRY_ADJUST_SETTING_LEFT(&UGameUserSettings::GetFoliageQuality, &UGameUserSettings::SetFoliageQuality, 5);
}

bool FVideoSetting_FoliageQuality::TryAdjustRight(URTSGameUserSettings * GameUserSettings, USingleVideoSettingWidget * SingleSettingWidget)
{
	TRY_ADJUST_SETTING_RIGHT(&UGameUserSettings::GetFoliageQuality, &UGameUserSettings::SetFoliageQuality, 5);
}

bool FVideoSetting_FoliageQuality::IsAtLowestValue(URTSGameUserSettings * GameUserSettings) const
{
	return GameUserSettings->GetFoliageQuality() == 0;
}

bool FVideoSetting_FoliageQuality::IsAtHighestValue(URTSGameUserSettings * GameUserSettings) const
{
	return GameUserSettings->GetFoliageQuality() == 4;
}

FText FVideoSetting_FrameRateLimit::GetDisplayText(URTSGameUserSettings * GameUserSettings) const
{
	const float FramerateLimit = GameUserSettings->GetFrameRateLimit();
	if (FramerateLimit == 0.f)
	{
		return FText::FromString("Unlimited");
	}
	else
	{
		return FText::AsNumber(FramerateLimit);
	}
}

float FVideoSetting_FrameRateLimit::GetDisplayPercentage(URTSGameUserSettings * GameUserSettings) const
{
	const float FramerateLimit = GameUserSettings->GetFrameRateLimit();
	if (FramerateLimit == 0.f)
	{
		return 1.f;
	}
	else
	{
		return (FramerateLimit - MIN) / (MAX - MIN);
	}
}

bool FVideoSetting_FrameRateLimit::TryAdjustLeft(URTSGameUserSettings * GameUserSettings, USingleVideoSettingWidget * SingleSettingWidget)
{
	if (CanValueWrap())
	{
		const int32 FramerateLimitAsInt = FramerateAsFloatToInt(GameUserSettings->GetFrameRateLimit());
		if (FramerateLimitAsInt == 0)
		{
			GameUserSettings->SetFrameRateLimit(MAX);
		}
		else if (FramerateLimitAsInt == MIN)
		{
			GameUserSettings->SetFrameRateLimit(0);
		}
		else
		{
			GameUserSettings->SetFrameRateLimit(FramerateLimitAsInt - STEP_SIZE);
		}

		// Update UI
		SingleSettingWidget->UpdateAppearanceForCurrentValue(GameUserSettings);
		USingleVideoSettingWidget * OverallScalabilityWidget = SingleSettingWidget->GetVideoSettingsWidget()->GetSingleSettingWidget(EVideoSettingType::OverallQuality);
		if (OverallScalabilityWidget != nullptr)
		{
			OverallScalabilityWidget->UpdateAppearanceForCurrentValue(GameUserSettings);
		}

		return true;
	}
	else
	{
		if (IsAtLowestValue(GameUserSettings))
		{
			return false;
		}
		else
		{
			const int32 FramerateLimitAsInt = FramerateAsFloatToInt(GameUserSettings->GetFrameRateLimit());
			if (FramerateLimitAsInt == 0)
			{
				GameUserSettings->SetFrameRateLimit(MAX);
			}
			else
			{
				GameUserSettings->SetFrameRateLimit(FramerateLimitAsInt - STEP_SIZE);
			}

			// Update UI
			SingleSettingWidget->UpdateAppearanceForCurrentValue(GameUserSettings);
			USingleVideoSettingWidget * OverallScalabilityWidget = SingleSettingWidget->GetVideoSettingsWidget()->GetSingleSettingWidget(EVideoSettingType::OverallQuality);
			if (OverallScalabilityWidget != nullptr)
			{
				OverallScalabilityWidget->UpdateAppearanceForCurrentValue(GameUserSettings);
			}

			return true;
		}
	}
}

bool FVideoSetting_FrameRateLimit::TryAdjustRight(URTSGameUserSettings * GameUserSettings, USingleVideoSettingWidget * SingleSettingWidget)
{
	if (CanValueWrap())
	{
		const int32 FramerateLimitAsInt = FramerateAsFloatToInt(GameUserSettings->GetFrameRateLimit());
		if (FramerateLimitAsInt == 0)
		{
			GameUserSettings->SetFrameRateLimit(MIN);
		}
		else if (FramerateLimitAsInt == MAX)
		{
			GameUserSettings->SetFrameRateLimit(0);
		}
		else
		{
			GameUserSettings->SetFrameRateLimit(FramerateLimitAsInt + STEP_SIZE);
		}

		// Update UI
		SingleSettingWidget->UpdateAppearanceForCurrentValue(GameUserSettings);
		USingleVideoSettingWidget * OverallScalabilityWidget = SingleSettingWidget->GetVideoSettingsWidget()->GetSingleSettingWidget(EVideoSettingType::OverallQuality);
		if (OverallScalabilityWidget != nullptr)
		{
			OverallScalabilityWidget->UpdateAppearanceForCurrentValue(GameUserSettings);
		}

		return true;
	}
	else
	{
		if (IsAtHighestValue(GameUserSettings))
		{
			return false;
		}
		else
		{
			const int32 FramerateLimitAsInt = FramerateAsFloatToInt(GameUserSettings->GetFrameRateLimit());
			if (FramerateLimitAsInt == MAX)
			{
				GameUserSettings->SetFrameRateLimit(0);
			}
			else
			{
				GameUserSettings->SetFrameRateLimit(FramerateLimitAsInt + STEP_SIZE);
			}
			
			// Update UI
			SingleSettingWidget->UpdateAppearanceForCurrentValue(GameUserSettings);
			USingleVideoSettingWidget * OverallScalabilityWidget = SingleSettingWidget->GetVideoSettingsWidget()->GetSingleSettingWidget(EVideoSettingType::OverallQuality);
			if (OverallScalabilityWidget != nullptr)
			{
				OverallScalabilityWidget->UpdateAppearanceForCurrentValue(GameUserSettings);
			}

			return true;
		}
	}
}

bool FVideoSetting_FrameRateLimit::IsAtLowestValue(URTSGameUserSettings * GameUserSettings) const
{
	return FramerateAsFloatToInt(GameUserSettings->GetFrameRateLimit()) == MIN;
}

bool FVideoSetting_FrameRateLimit::IsAtHighestValue(URTSGameUserSettings * GameUserSettings) const
{
	// 0 means unlimited
	return GameUserSettings->GetFrameRateLimit() == 0.f;
}

FText FVideoSetting_AntiAliasingQuality::GetDisplayText(URTSGameUserSettings * GameUserSettings) const
{
	const EAntiAliasingMethod Type = GameUserSettings->GetAntiAliasingMethod();
	if (Type == AAM_None)
	{
		return FText::FromString("Off");
	}
	else if (Type == AAM_FXAA)
	{
		const int32 AntiAliasingLevel = GameUserSettings->GetAntiAliasingQuality();
		switch (AntiAliasingLevel)
		{
		case 0:
			return FText::FromString("FXAA Very Low");
		case 1:
			return FText::FromString("FXAA Low");
		case 2: 
			return FText::FromString("FXAA Medium");
		case 3:
			return FText::FromString("FXAA High");
		case 4:
			return FText::FromString("FXAA Very High");
		default:
			assert(0);
			return FText::FromString("Error");
		}
	}
	else if (Type == AAM_TemporalAA)
	{
		const int32 AntiAliasingLevel = GameUserSettings->GetAntiAliasingQuality();
		switch (AntiAliasingLevel)
		{
		case 0:
			return FText::FromString("TAA Very Low");
		case 1:
			return FText::FromString("TAA Low");
		case 2:
			return FText::FromString("TAA Medium");
		case 3:
			return FText::FromString("TAA High");
		case 4:
			return FText::FromString("TAA Very High");
		default:
			assert(0);
			return FText::FromString("Error");
		}
	}
	else
	{
		assert(0);
		return FText::FromString("Unexpected Type");
	}
}

float FVideoSetting_AntiAliasingQuality::GetDisplayPercentage(URTSGameUserSettings * GameUserSettings) const
{
	const EAntiAliasingMethod Type = GameUserSettings->GetAntiAliasingMethod();
	if (Type == AAM_None)
	{
		return 0.f;
	}
	else if (Type == AAM_FXAA)
	{
		const int32 AntiAliasingLevel = GameUserSettings->GetAntiAliasingQuality();
		return (AntiAliasingLevel + 1) * (1.f / 10.f);
	}
	else if (Type == AAM_TemporalAA)
	{
		const int32 AntiAliasingLevel = GameUserSettings->GetAntiAliasingQuality();
		return (AntiAliasingLevel + 6) * (1.f / 10.f);
	}
	else
	{
		assert(0);
		return -1.f;
	}
}

bool FVideoSetting_AntiAliasingQuality::TryAdjustLeft(URTSGameUserSettings * GameUserSettings, USingleVideoSettingWidget * SingleSettingWidget)
{
	if (CanValueWrap())
	{
		const EAntiAliasingMethod Method = GameUserSettings->GetAntiAliasingMethod();
		if (Method == AAM_None)
		{
			GameUserSettings->SetAntiAliasingQuality(4);
			// Set method to TAA
			GameUserSettings->SetAntiAliasingMethod(SingleSettingWidget->GetWorld(), AAM_TemporalAA);

			// Update UI
			SingleSettingWidget->UpdateAppearanceForCurrentValue(GameUserSettings);
			USingleVideoSettingWidget * OverallScalabilityWidget = SingleSettingWidget->GetVideoSettingsWidget()->GetSingleSettingWidget(EVideoSettingType::OverallQuality);
			if (OverallScalabilityWidget != nullptr)
			{
				OverallScalabilityWidget->UpdateAppearanceForCurrentValue(GameUserSettings);
			}

			return true;
		}
		else if (Method == AAM_FXAA)
		{
			const int32 AntiAliasingLevel = GameUserSettings->GetAntiAliasingQuality();
			if (AntiAliasingLevel == 0)
			{
				// Turn off anti aliasing
				GameUserSettings->SetAntiAliasingMethod(SingleSettingWidget->GetWorld(), AAM_None);

				// Update UI
				SingleSettingWidget->UpdateAppearanceForCurrentValue(GameUserSettings);
				USingleVideoSettingWidget * OverallScalabilityWidget = SingleSettingWidget->GetVideoSettingsWidget()->GetSingleSettingWidget(EVideoSettingType::OverallQuality);
				if (OverallScalabilityWidget != nullptr)
				{
					OverallScalabilityWidget->UpdateAppearanceForCurrentValue(GameUserSettings);
				}

				return true;
			}
			else
			{
				GameUserSettings->SetAntiAliasingQuality(AntiAliasingLevel - 1);

				// Update UI
				SingleSettingWidget->UpdateAppearanceForCurrentValue(GameUserSettings);
				USingleVideoSettingWidget * OverallScalabilityWidget = SingleSettingWidget->GetVideoSettingsWidget()->GetSingleSettingWidget(EVideoSettingType::OverallQuality);
				if (OverallScalabilityWidget != nullptr)
				{
					OverallScalabilityWidget->UpdateAppearanceForCurrentValue(GameUserSettings);
				}

				return true;
			}
		}
		else if (Method == AAM_TemporalAA)
		{
			const int32 AntiAliasingLevel = GameUserSettings->GetAntiAliasingQuality();
			if (AntiAliasingLevel == 0)
			{
				GameUserSettings->SetAntiAliasingQuality(4);
				// Change method to FXAA
				GameUserSettings->SetAntiAliasingMethod(SingleSettingWidget->GetWorld(), AAM_FXAA);

				// Update UI
				SingleSettingWidget->UpdateAppearanceForCurrentValue(GameUserSettings);
				USingleVideoSettingWidget * OverallScalabilityWidget = SingleSettingWidget->GetVideoSettingsWidget()->GetSingleSettingWidget(EVideoSettingType::OverallQuality);
				if (OverallScalabilityWidget != nullptr)
				{
					OverallScalabilityWidget->UpdateAppearanceForCurrentValue(GameUserSettings);
				}

				return true;
			}
			else
			{
				GameUserSettings->SetAntiAliasingQuality(AntiAliasingLevel - 1);

				// Update UI
				SingleSettingWidget->UpdateAppearanceForCurrentValue(GameUserSettings);
				USingleVideoSettingWidget * OverallScalabilityWidget = SingleSettingWidget->GetVideoSettingsWidget()->GetSingleSettingWidget(EVideoSettingType::OverallQuality);
				if (OverallScalabilityWidget != nullptr)
				{
					OverallScalabilityWidget->UpdateAppearanceForCurrentValue(GameUserSettings);
				}

				return true;
			}
		}
		else
		{
			UE_LOG(RTSLOG, Fatal, TEXT("Unaccounted for anti-aliasing method: [%s] "),
				TO_STRING(EAntiAliasingMethod, Method));
			return false;
		}
	}
	else
	{
		if (IsAtLowestValue(GameUserSettings))
		{
			return false;
		}
		else
		{
			const EAntiAliasingMethod Method = GameUserSettings->GetAntiAliasingMethod();
			if (Method == AAM_FXAA)
			{
				const int32 AntiAliasingLevel = GameUserSettings->GetAntiAliasingQuality();
				if (AntiAliasingLevel == 0)
				{
					// Turn off anti-aliasing
					GameUserSettings->SetAntiAliasingMethod(SingleSettingWidget->GetWorld(), AAM_None);

					// Update UI
					SingleSettingWidget->UpdateAppearanceForCurrentValue(GameUserSettings);
					USingleVideoSettingWidget * OverallScalabilityWidget = SingleSettingWidget->GetVideoSettingsWidget()->GetSingleSettingWidget(EVideoSettingType::OverallQuality);
					if (OverallScalabilityWidget != nullptr)
					{
						OverallScalabilityWidget->UpdateAppearanceForCurrentValue(GameUserSettings);
					}

					return true;
				}
				else
				{
					GameUserSettings->SetAntiAliasingQuality(AntiAliasingLevel - 1);

					// Update UI
					SingleSettingWidget->UpdateAppearanceForCurrentValue(GameUserSettings);
					USingleVideoSettingWidget * OverallScalabilityWidget = SingleSettingWidget->GetVideoSettingsWidget()->GetSingleSettingWidget(EVideoSettingType::OverallQuality);
					if (OverallScalabilityWidget != nullptr)
					{
						OverallScalabilityWidget->UpdateAppearanceForCurrentValue(GameUserSettings);
					}

					return true;
				}
			}
			else if (Method == AAM_TemporalAA)
			{
				const int32 AntiAliasingLevel = GameUserSettings->GetAntiAliasingQuality();
				if (AntiAliasingLevel == 0)
				{
					GameUserSettings->SetAntiAliasingQuality(4);
					// Change method to FXAA
					GameUserSettings->SetAntiAliasingMethod(SingleSettingWidget->GetWorld(), AAM_FXAA);

					// Update UI
					SingleSettingWidget->UpdateAppearanceForCurrentValue(GameUserSettings);
					USingleVideoSettingWidget * OverallScalabilityWidget = SingleSettingWidget->GetVideoSettingsWidget()->GetSingleSettingWidget(EVideoSettingType::OverallQuality);
					if (OverallScalabilityWidget != nullptr)
					{
						OverallScalabilityWidget->UpdateAppearanceForCurrentValue(GameUserSettings);
					}

					return true;
				}
				else
				{
					GameUserSettings->SetAntiAliasingQuality(AntiAliasingLevel - 1);

					// Update UI
					SingleSettingWidget->UpdateAppearanceForCurrentValue(GameUserSettings);
					USingleVideoSettingWidget * OverallScalabilityWidget = SingleSettingWidget->GetVideoSettingsWidget()->GetSingleSettingWidget(EVideoSettingType::OverallQuality);
					if (OverallScalabilityWidget != nullptr)
					{
						OverallScalabilityWidget->UpdateAppearanceForCurrentValue(GameUserSettings);
					}

					return true;
				}
			}
			else
			{
				UE_LOG(RTSLOG, Fatal, TEXT("Unaccounted for anti-aliasing method: [%s] "),
					TO_STRING(EAntiAliasingMethod, Method));
				return false;
			}
		}
	}
}

bool FVideoSetting_AntiAliasingQuality::TryAdjustRight(URTSGameUserSettings * GameUserSettings, USingleVideoSettingWidget * SingleSettingWidget)
{
	if (CanValueWrap())
	{
		const EAntiAliasingMethod Method = GameUserSettings->GetAntiAliasingMethod();
		if (Method == AAM_None)
		{
			GameUserSettings->SetAntiAliasingQuality(0);
			// Set method to FXAA
			GameUserSettings->SetAntiAliasingMethod(SingleSettingWidget->GetWorld(), AAM_FXAA);

			// Update UI
			SingleSettingWidget->UpdateAppearanceForCurrentValue(GameUserSettings);
			USingleVideoSettingWidget * OverallScalabilityWidget = SingleSettingWidget->GetVideoSettingsWidget()->GetSingleSettingWidget(EVideoSettingType::OverallQuality);
			if (OverallScalabilityWidget != nullptr)
			{
				OverallScalabilityWidget->UpdateAppearanceForCurrentValue(GameUserSettings);
			}

			return true;
		}
		else if (Method == AAM_FXAA)
		{
			const int32 AntiAliasingLevel = GameUserSettings->GetAntiAliasingQuality();
			if (AntiAliasingLevel == 4)
			{
				GameUserSettings->SetAntiAliasingQuality(0);
				// Set method to TAA
				GameUserSettings->SetAntiAliasingMethod(SingleSettingWidget->GetWorld(), AAM_TemporalAA);

				// Update UI
				SingleSettingWidget->UpdateAppearanceForCurrentValue(GameUserSettings);
				USingleVideoSettingWidget * OverallScalabilityWidget = SingleSettingWidget->GetVideoSettingsWidget()->GetSingleSettingWidget(EVideoSettingType::OverallQuality);
				if (OverallScalabilityWidget != nullptr)
				{
					OverallScalabilityWidget->UpdateAppearanceForCurrentValue(GameUserSettings);
				}

				return true;
			}
			else
			{
				GameUserSettings->SetAntiAliasingQuality(AntiAliasingLevel + 1);

				// Update UI
				SingleSettingWidget->UpdateAppearanceForCurrentValue(GameUserSettings);
				USingleVideoSettingWidget * OverallScalabilityWidget = SingleSettingWidget->GetVideoSettingsWidget()->GetSingleSettingWidget(EVideoSettingType::OverallQuality);
				if (OverallScalabilityWidget != nullptr)
				{
					OverallScalabilityWidget->UpdateAppearanceForCurrentValue(GameUserSettings);
				}

				return true;
			}
		}
		else if (Method == AAM_TemporalAA)
		{
			const int32 AntiAliasingLevel = GameUserSettings->GetAntiAliasingQuality();
			if (AntiAliasingLevel == 4)
			{
				// Change method to None
				GameUserSettings->SetAntiAliasingMethod(SingleSettingWidget->GetWorld(), AAM_None);

				// Update UI
				SingleSettingWidget->UpdateAppearanceForCurrentValue(GameUserSettings);
				USingleVideoSettingWidget * OverallScalabilityWidget = SingleSettingWidget->GetVideoSettingsWidget()->GetSingleSettingWidget(EVideoSettingType::OverallQuality);
				if (OverallScalabilityWidget != nullptr)
				{
					OverallScalabilityWidget->UpdateAppearanceForCurrentValue(GameUserSettings);
				}

				return true;
			}
			else
			{
				GameUserSettings->SetAntiAliasingQuality(AntiAliasingLevel + 1);

				// Update UI
				SingleSettingWidget->UpdateAppearanceForCurrentValue(GameUserSettings);
				USingleVideoSettingWidget * OverallScalabilityWidget = SingleSettingWidget->GetVideoSettingsWidget()->GetSingleSettingWidget(EVideoSettingType::OverallQuality);
				if (OverallScalabilityWidget != nullptr)
				{
					OverallScalabilityWidget->UpdateAppearanceForCurrentValue(GameUserSettings);
				}

				return true;
			}
		}
		else
		{
			UE_LOG(RTSLOG, Fatal, TEXT("Unaccounted for anti-aliasing method: [%s] "),
				TO_STRING(EAntiAliasingMethod, Method));
			return false;
		}
	}
	else
	{
		if (IsAtHighestValue(GameUserSettings))
		{
			return false;
		}
		else
		{
			const EAntiAliasingMethod Method = GameUserSettings->GetAntiAliasingMethod();
			if (Method == AAM_FXAA)
			{
				const int32 AntiAliasingLevel = GameUserSettings->GetAntiAliasingQuality();
				if (AntiAliasingLevel == 4)
				{
					GameUserSettings->SetAntiAliasingQuality(0);
					// Change method to TAA
					GameUserSettings->SetAntiAliasingMethod(SingleSettingWidget->GetWorld(), AAM_TemporalAA);

					// Update UI
					SingleSettingWidget->UpdateAppearanceForCurrentValue(GameUserSettings);
					USingleVideoSettingWidget * OverallScalabilityWidget = SingleSettingWidget->GetVideoSettingsWidget()->GetSingleSettingWidget(EVideoSettingType::OverallQuality);
					if (OverallScalabilityWidget != nullptr)
					{
						OverallScalabilityWidget->UpdateAppearanceForCurrentValue(GameUserSettings);
					}

					return true;
				}
				else
				{
					GameUserSettings->SetAntiAliasingQuality(AntiAliasingLevel + 1);

					// Update UI
					SingleSettingWidget->UpdateAppearanceForCurrentValue(GameUserSettings);
					USingleVideoSettingWidget * OverallScalabilityWidget = SingleSettingWidget->GetVideoSettingsWidget()->GetSingleSettingWidget(EVideoSettingType::OverallQuality);
					if (OverallScalabilityWidget != nullptr)
					{
						OverallScalabilityWidget->UpdateAppearanceForCurrentValue(GameUserSettings);
					}

					return true;
				}
			}
			else if (Method == AAM_TemporalAA)
			{
				const int32 AntiAliasingLevel = GameUserSettings->GetAntiAliasingQuality();
				assert(AntiAliasingLevel < 4);
				GameUserSettings->SetAntiAliasingQuality(AntiAliasingLevel + 1);

				// Update UI
				SingleSettingWidget->UpdateAppearanceForCurrentValue(GameUserSettings);
				USingleVideoSettingWidget * OverallScalabilityWidget = SingleSettingWidget->GetVideoSettingsWidget()->GetSingleSettingWidget(EVideoSettingType::OverallQuality);
				if (OverallScalabilityWidget != nullptr)
				{
					OverallScalabilityWidget->UpdateAppearanceForCurrentValue(GameUserSettings);
				}

				return true;
			}
			else if (Method == AAM_None)
			{
				GameUserSettings->SetAntiAliasingQuality(0);
				// Change method to FXAA
				GameUserSettings->SetAntiAliasingMethod(SingleSettingWidget->GetWorld(), AAM_FXAA);

				// Update UI
				SingleSettingWidget->UpdateAppearanceForCurrentValue(GameUserSettings);
				USingleVideoSettingWidget * OverallScalabilityWidget = SingleSettingWidget->GetVideoSettingsWidget()->GetSingleSettingWidget(EVideoSettingType::OverallQuality);
				if (OverallScalabilityWidget != nullptr)
				{
					OverallScalabilityWidget->UpdateAppearanceForCurrentValue(GameUserSettings);
				}

				return true;
			}
			else
			{
				UE_LOG(RTSLOG, Fatal, TEXT("Unaccounted for anti-aliasing method: [%s] "),
					TO_STRING(EAntiAliasingMethod, Method));
				return false;
			}
		}
	}
}

bool FVideoSetting_AntiAliasingQuality::IsAtLowestValue(URTSGameUserSettings * GameUserSettings) const
{
	const EAntiAliasingMethod Method = GameUserSettings->GetAntiAliasingMethod();
	return (Method == AAM_None);
}

bool FVideoSetting_AntiAliasingQuality::IsAtHighestValue(URTSGameUserSettings * GameUserSettings) const
{
	if (GameUserSettings->GetAntiAliasingQuality() == 4)
	{
		return GameUserSettings->GetAntiAliasingMethod() == AAM_TemporalAA;
	}
	else
	{
		return false;
	}
}

FText FVideoSetting_ShadowQuality::GetDisplayText(URTSGameUserSettings * GameUserSettings) const
{
	return GetTextForZeroToFourRange(GameUserSettings->GetShadowQuality());
}

float FVideoSetting_ShadowQuality::GetDisplayPercentage(URTSGameUserSettings * GameUserSettings) const
{
	return GetDisplayPercentageForZeroToFourRange(GameUserSettings->GetShadowQuality());
}

bool FVideoSetting_ShadowQuality::TryAdjustLeft(URTSGameUserSettings * GameUserSettings, USingleVideoSettingWidget * SingleSettingWidget)
{
	TRY_ADJUST_SETTING_LEFT(&UGameUserSettings::GetShadowQuality, &UGameUserSettings::SetShadowQuality, 5);
}

bool FVideoSetting_ShadowQuality::TryAdjustRight(URTSGameUserSettings * GameUserSettings, USingleVideoSettingWidget * SingleSettingWidget)
{
	TRY_ADJUST_SETTING_RIGHT(&UGameUserSettings::GetShadowQuality, &UGameUserSettings::SetShadowQuality, 5);
}

bool FVideoSetting_ShadowQuality::IsAtLowestValue(URTSGameUserSettings * GameUserSettings) const
{
	return GameUserSettings->GetShadowQuality() == 0;
}

bool FVideoSetting_ShadowQuality::IsAtHighestValue(URTSGameUserSettings * GameUserSettings) const
{
	return GameUserSettings->GetShadowQuality() == 4;
}

FText FVideoSetting_ViewDistanceQuality::GetDisplayText(URTSGameUserSettings * GameUserSettings) const
{
	return GetTextForZeroToFourRange(GameUserSettings->GetViewDistanceQuality());
}

float FVideoSetting_ViewDistanceQuality::GetDisplayPercentage(URTSGameUserSettings * GameUserSettings) const
{
	return GetDisplayPercentageForZeroToFourRange(GameUserSettings->GetViewDistanceQuality());
}

bool FVideoSetting_ViewDistanceQuality::TryAdjustLeft(URTSGameUserSettings * GameUserSettings, USingleVideoSettingWidget * SingleSettingWidget)
{
	TRY_ADJUST_SETTING_LEFT(&UGameUserSettings::GetViewDistanceQuality, &UGameUserSettings::SetViewDistanceQuality, 5);
}

bool FVideoSetting_ViewDistanceQuality::TryAdjustRight(URTSGameUserSettings * GameUserSettings, USingleVideoSettingWidget * SingleSettingWidget)
{
	TRY_ADJUST_SETTING_RIGHT(&UGameUserSettings::GetViewDistanceQuality, &UGameUserSettings::SetViewDistanceQuality, 5);
}

bool FVideoSetting_ViewDistanceQuality::IsAtLowestValue(URTSGameUserSettings * GameUserSettings) const
{
	return GameUserSettings->GetViewDistanceQuality() == 0;
}

bool FVideoSetting_ViewDistanceQuality::IsAtHighestValue(URTSGameUserSettings * GameUserSettings) const
{
	return GameUserSettings->GetViewDistanceQuality() == 4;
}

FText FVideoSetting_TextureQuality::GetDisplayText(URTSGameUserSettings * GameUserSettings) const
{
	return GetTextForZeroToFourRange(GameUserSettings->GetTextureQuality());
}

float FVideoSetting_TextureQuality::GetDisplayPercentage(URTSGameUserSettings * GameUserSettings) const
{
	return GetDisplayPercentageForZeroToFourRange(GameUserSettings->GetTextureQuality());
}

bool FVideoSetting_TextureQuality::TryAdjustLeft(URTSGameUserSettings * GameUserSettings, USingleVideoSettingWidget * SingleSettingWidget)
{
	TRY_ADJUST_SETTING_LEFT(&UGameUserSettings::GetTextureQuality, &UGameUserSettings::SetTextureQuality, 5);
}

bool FVideoSetting_TextureQuality::TryAdjustRight(URTSGameUserSettings * GameUserSettings, USingleVideoSettingWidget * SingleSettingWidget)
{
	TRY_ADJUST_SETTING_RIGHT(&UGameUserSettings::GetTextureQuality, &UGameUserSettings::SetTextureQuality, 5);
}

bool FVideoSetting_TextureQuality::IsAtLowestValue(URTSGameUserSettings * GameUserSettings) const
{
	return GameUserSettings->GetTextureQuality() == 0;
}

bool FVideoSetting_TextureQuality::IsAtHighestValue(URTSGameUserSettings * GameUserSettings) const
{
	return GameUserSettings->GetTextureQuality() == 4;
}

FText FVideoSetting_VisualEffectQuality::GetDisplayText(URTSGameUserSettings * GameUserSettings) const
{
	return GetTextForZeroToFourRange(GameUserSettings->GetVisualEffectQuality());
}

float FVideoSetting_VisualEffectQuality::GetDisplayPercentage(URTSGameUserSettings * GameUserSettings) const
{
	return GetDisplayPercentageForZeroToFourRange(GameUserSettings->GetVisualEffectQuality());
}

bool FVideoSetting_VisualEffectQuality::TryAdjustLeft(URTSGameUserSettings * GameUserSettings, USingleVideoSettingWidget * SingleSettingWidget)
{
	TRY_ADJUST_SETTING_LEFT(&UGameUserSettings::GetVisualEffectQuality, &UGameUserSettings::SetVisualEffectQuality, 5);
}

bool FVideoSetting_VisualEffectQuality::TryAdjustRight(URTSGameUserSettings * GameUserSettings, USingleVideoSettingWidget * SingleSettingWidget)
{
	TRY_ADJUST_SETTING_RIGHT(&UGameUserSettings::GetVisualEffectQuality, &UGameUserSettings::SetVisualEffectQuality, 5);
}

bool FVideoSetting_VisualEffectQuality::IsAtLowestValue(URTSGameUserSettings * GameUserSettings) const
{
	return GameUserSettings->GetVisualEffectQuality() == 0;
}

bool FVideoSetting_VisualEffectQuality::IsAtHighestValue(URTSGameUserSettings * GameUserSettings) const
{
	return GameUserSettings->GetVisualEffectQuality() == 4;
}

FText FVideoSetting_PostProcessingQuality::GetDisplayText(URTSGameUserSettings * GameUserSettings) const
{
	return GetTextForZeroToFourRange(GameUserSettings->GetPostProcessingQuality());
}

float FVideoSetting_PostProcessingQuality::GetDisplayPercentage(URTSGameUserSettings * GameUserSettings) const
{
	return GetDisplayPercentageForZeroToFourRange(GameUserSettings->GetPostProcessingQuality());
}

bool FVideoSetting_PostProcessingQuality::TryAdjustLeft(URTSGameUserSettings * GameUserSettings, USingleVideoSettingWidget * SingleSettingWidget)
{
	TRY_ADJUST_SETTING_LEFT(&UGameUserSettings::GetPostProcessingQuality, &UGameUserSettings::SetPostProcessingQuality, 5);
}

bool FVideoSetting_PostProcessingQuality::TryAdjustRight(URTSGameUserSettings * GameUserSettings, USingleVideoSettingWidget * SingleSettingWidget)
{
	TRY_ADJUST_SETTING_RIGHT(&UGameUserSettings::GetPostProcessingQuality, &UGameUserSettings::SetPostProcessingQuality, 5);
}

bool FVideoSetting_PostProcessingQuality::IsAtLowestValue(URTSGameUserSettings * GameUserSettings) const
{
	return GameUserSettings->GetPostProcessingQuality() == 0;
}

bool FVideoSetting_PostProcessingQuality::IsAtHighestValue(URTSGameUserSettings * GameUserSettings) const
{
	return GameUserSettings->GetPostProcessingQuality() == 4;
}

FText FVideoSetting_VSync::GetDisplayText(URTSGameUserSettings * GameUserSettings) const
{
	return GameUserSettings->IsVSyncEnabled() ? FText::FromString("Enabled") : FText::FromString("Disabled");
}

float FVideoSetting_VSync::GetDisplayPercentage(URTSGameUserSettings * GameUserSettings) const
{
	return GameUserSettings->IsVSyncEnabled() ? 1.f : 0.f;
}

bool FVideoSetting_VSync::TryAdjustLeft(URTSGameUserSettings * GameUserSettings, USingleVideoSettingWidget * SingleSettingWidget)
{
	if (CanValueWrap())
	{
		GameUserSettings->SetVSyncEnabled(!GameUserSettings->IsVSyncEnabled());

		// Update UI
		SingleSettingWidget->UpdateAppearanceForCurrentValue(GameUserSettings);

		return true;
	}
	else
	{
		if (IsAtLowestValue(GameUserSettings))
		{
			return false;
		}
		else
		{
			GameUserSettings->SetVSyncEnabled(true);

			// Update UI
			SingleSettingWidget->UpdateAppearanceForCurrentValue(GameUserSettings);

			return true;
		}
	}
}

bool FVideoSetting_VSync::TryAdjustRight(URTSGameUserSettings * GameUserSettings, USingleVideoSettingWidget * SingleSettingWidget)
{
	if (CanValueWrap())
	{
		GameUserSettings->SetVSyncEnabled(!GameUserSettings->IsVSyncEnabled());

		// Update UI
		SingleSettingWidget->UpdateAppearanceForCurrentValue(GameUserSettings);

		return true;
	}
	else
	{
		if (IsAtHighestValue(GameUserSettings))
		{
			return false;
		}
		else
		{
			GameUserSettings->SetVSyncEnabled(false);

			// Update UI
			SingleSettingWidget->UpdateAppearanceForCurrentValue(GameUserSettings);

			return true;
		}
	}
}

bool FVideoSetting_VSync::IsAtLowestValue(URTSGameUserSettings * GameUserSettings) const
{
	return GameUserSettings->IsVSyncEnabled() == false;
}

bool FVideoSetting_VSync::IsAtHighestValue(URTSGameUserSettings * GameUserSettings) const
{
	return GameUserSettings->IsVSyncEnabled() == true;
}
