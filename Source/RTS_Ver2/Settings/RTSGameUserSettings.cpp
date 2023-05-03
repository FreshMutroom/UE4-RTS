// Fill out your copyright notice in the Description page of Project Settings.

#include "RTSGameUserSettings.h"
#include "Sound/SoundClass.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/InputSettings.h"
#include "Kismet/GameplayStatics.h"

#include "GameFramework/RTSGameInstance.h"
#include "Statics/DevelopmentStatics.h"
#include "GameFramework/RTSPlayerController.h"
#include "Statics/Statics.h"
#include "Settings/KeyMappings.h"


//===============================================================================================
//	FControlSettingDependencyInfo
//===============================================================================================

FControlSettingDependencyInfo::FControlSettingDependencyInfo()
{
	// Commented because crash here on startup after migrating to 4.21
	//assert(0);
}

FControlSettingDependencyInfo::FControlSettingDependencyInfo(EControlSettingType InDependency, 
	bool bInRequiredValue)
{
	Dependency = InDependency;
	DependencyFullfilledOperator = EDependencyRequirementType::Bool;
	bRequiredValue = bInRequiredValue;
}

FControlSettingDependencyInfo::FControlSettingDependencyInfo(EControlSettingType InDependency, 
	EDependencyRequirementType InDependencyFullfilledOperator, float InRequiredValue)
{
	Dependency = InDependency;
	DependencyFullfilledOperator = InDependencyFullfilledOperator;
	RequiredValue = InRequiredValue;
}


//===============================================================================================
//	FControlSettingInfo
//===============================================================================================

FControlSettingInfo::FControlSettingInfo() 
	: VariablePtr(nullptr)
	//, ApplyFuncPtr_Float(nullptr) 
	//, ApplyFuncPtr_Bool(nullptr)
	, DefaultValue(0.f) 
	, UIDependencies(TArray<FControlSettingDependencyInfo>())
{
}

FControlSettingInfo::FControlSettingInfo(const FString & InDisplayName, int16 * InVariablePtr, ApplyControlSettingFuncPtr_Float ApplyFunc, float InDefaultValue,
	const TArray<float>& InSteps, const TArray <FControlSettingDependencyInfo> & InUIDependencies)
{
	DisplayName = FText::FromString(InDisplayName);
	
	VariablePtr = InVariablePtr;
	VariableType = EVariableType::Float;
	//ApplyFuncPtr_Float = ApplyFunc;
	//ApplyFuncPtr_Bool = nullptr;
	DefaultValue = InDefaultValue;

	Steps = InSteps;
	UIDependencies = InUIDependencies;
}

FControlSettingInfo::FControlSettingInfo(const FString & InDisplayName, int16 * InVariablePtr, ApplyControlSettingFuncPtr_Float ApplyFunc, float InDefaultValue,
	const FControlSettingDependencyInfo & UIDependency, const TArray<float>& InSteps)
{
	DisplayName = FText::FromString(InDisplayName);
	
	VariablePtr = InVariablePtr;
	VariableType = EVariableType::Float;
	//ApplyFuncPtr_Float = ApplyFunc;
	//ApplyFuncPtr_Bool = nullptr;
	DefaultValue = InDefaultValue;

	Steps = InSteps;
	UIDependencies.Emplace(UIDependency);
}

FControlSettingInfo::FControlSettingInfo(const FString & InDisplayName, int16 * InVariablePtr, 
	ApplyControlSettingFuncPtr_Bool ApplyFunc, bool InDefaultValue, const TArray<FControlSettingDependencyInfo>& InUIDependencies)
{
	DisplayName = FText::FromString(InDisplayName);
	VariablePtr = InVariablePtr;
	VariableType = EVariableType::Bool;
	//ApplyFuncPtr_Float = nullptr;
	//ApplyFuncPtr_Bool = ApplyFunc;
	// It's important we have 1.f for true and 0.f for false. Maybe static_cast<bool> will do this dunno but too busy to test
	DefaultValue = (InDefaultValue == true) ? 1.f : 0.f;
	Steps = { 0.f, 1.f };
	UIDependencies = InUIDependencies;
}

FControlSettingInfo::FControlSettingInfo(const FString & InDisplayName, int16 * InVariablePtr,
	ApplyControlSettingFuncPtr_Bool ApplyFunc, bool InDefaultValue, const FControlSettingDependencyInfo & UIDependency)
{
	DisplayName = FText::FromString(InDisplayName);
	VariablePtr = InVariablePtr;
	VariableType = EVariableType::Bool;
	//ApplyFuncPtr_Float = nullptr;
	//ApplyFuncPtr_Bool = ApplyFunc;
	DefaultValue = (InDefaultValue == true) ? 1.f : 0.f;
	Steps = { 0.f, 1.f };
	UIDependencies.Emplace(UIDependency);
}

float FControlSettingInfo::GetValue() const
{
	// -1 means default value
	if (*VariablePtr == -1)
	{
		return DefaultValue;
	}
	else
	{
		return Steps[*VariablePtr];
	}
}

int16 FControlSettingInfo::GetStep() const
{
	return (*VariablePtr == -1) ? DefaultValueStep : *VariablePtr;
}

int16 FControlSettingInfo::GetNumSteps() const
{
	return Steps.Num();
}

void FControlSettingInfo::SetValue(int16 NewStepsIndex) const
{
	*VariablePtr = FMath::Clamp<int16>(NewStepsIndex, 0, Steps.Num() - 1);
}

void FControlSettingInfo::AdjustValue(int16 StepAdjustAmount) const
{
	if (*VariablePtr == -1)
	{
		*VariablePtr = FMath::Clamp<int16>(DefaultValueStep + StepAdjustAmount, 0, Steps.Num() - 1);
	}
	else
	{
		*VariablePtr = FMath::Clamp<int16>(*VariablePtr + StepAdjustAmount, 0, Steps.Num() - 1);
	}
}

void FControlSettingInfo::ClampValue() const
{
	*VariablePtr = FMath::Clamp<int16>(*VariablePtr, -1, Steps.Num() - 1);
}

void FControlSettingInfo::ResetToDefault()
{
	*VariablePtr = -1;
}

float FControlSettingInfo::GetDefaultValue() const
{
	return DefaultValue;
}

void FControlSettingInfo::ApplyValue(ARTSPlayerController * LocalPlayerController) const
{
	/* Because function pointers don't like me this does nothing. 
	So unfortunately settings cannot be applied one at a time as the player changes them. 
	I'll have to make sure then that all control settings apply when the user closes the 
	settings menu */
	
	//if (ApplyFuncPtr_Bool != nullptr)
	//{
	//	(LocalPlayerController->* (ApplyFuncPtr_Bool))(*BoolValue);
	//}
	//else
	//{
	//	(LocalPlayerController->* (ApplyFuncPtr_Float))(*FloatValue);
	//}
}


//===============================================================================================
//	URTSGameUserSettings
//===============================================================================================

const float URTSGameUserSettings::MIN_CAMERA_ZOOM_AMOUNT = 200.f;
const float URTSGameUserSettings::MAX_CAMERA_ZOOM_AMOUNT = 3500.f;
const float URTSGameUserSettings::MIN_CAMERA_PITCH = 10.f;
const float URTSGameUserSettings::MAX_CAMERA_PITCH = 89.9f; 
const float URTSGameUserSettings::MOUSE_DIAGONAL_MOVEMENT_SPEED_MULTIPLIER = 1.05f;
const float URTSGameUserSettings::KEYBOARD_DIAGONAL_MOVEMENT_SPEED_MULTIPLIER = 1.05f;

URTSGameUserSettings::URTSGameUserSettings(const FObjectInitializer & ObjectInitializer)
	: Super(ObjectInitializer)
{
	/* Ok this ctor is called before config UPROPERTIES are loaded from .ini file, meaning
	setting a value to those will just be overridden. So basically if user wants to set a
	default value for these they will have to do it by modifying their GameUserSettings.ini
	file */

	/* This is called when starting standalone but not PIE I think */

	GameStartType = EGameStartType::HasNotStartedGameBefore;

	/* Just looked at source. After NewObject is called SetToDefaults then LoadSettings 
	is called so my whole CheckIfStartingGameForryFirstTime might be irrelevant, although 
	it doesn't look like they are applied so my calling ApplyAllSettings might still 
	be useful */
}

void URTSGameUserSettings::PostInitProperties()
{
	Super::PostInitProperties(); 

	assert(ControlSettings.Num() == 0);
	ControlSettings.Reserve(64);
	
	//=========================================================================================
	//	Define control settings here
	//	Here define the display name, default value and adjustment amounts for settings menu
	//	The adjustments amounts array is optional, but for a shipping game you should 
	//	make sure to define one that's to your liking 
	//=========================================================================================

	/* Well that's interesting... need to restart editor for default values to take effect.
	Don't know why. TODO fix this. */

	ControlSettings.Emplace(EControlSettingType::CameraPanSpeed_Keyboard,
		FControlSettingInfo("Camera Key Move Speed", &CameraPanSpeed_Keyboard, &ARTSPlayerController::SetCameraKeyboardMoveSpeed,
			0.85f));

	ControlSettings.Emplace(EControlSettingType::CameraPanSpeed_Mouse,
		FControlSettingInfo("Camera Screen Edge Move Speed", &CameraPanSpeed_Mouse, &ARTSPlayerController::SetCameraMouseMoveSpeed,
			100.f));

	ControlSettings.Emplace(EControlSettingType::CameraMaxSpeed,
		FControlSettingInfo("Camera Max Speed", &CameraMaxSpeed, &ARTSPlayerController::SetCameraMaxSpeed,
			10000.f));

	ControlSettings.Emplace(EControlSettingType::CameraAcceleration,
		FControlSettingInfo("Camera Acceleration", &CameraAcceleration, &ARTSPlayerController::SetCameraAcceleration,
			32000.f));

	ControlSettings.Emplace(EControlSettingType::bEnableCameraMovementLag,
		FControlSettingInfo("Enable Camera Movement Lag", &bEnableCameraMovementLag, &ARTSPlayerController::SetEnableCameraMovementLag,
			false));

	ControlSettings.Emplace(EControlSettingType::CameraMovementLagSpeed,
		FControlSettingInfo("Camera Movement Lag Speed", &CameraMovementLagSpeed, &ARTSPlayerController::SetCameraMovementLagSpeed,
			10.f,
			FControlSettingDependencyInfo(EControlSettingType::bEnableCameraMovementLag, true)));

	ControlSettings.Emplace(EControlSettingType::CameraTurningBoost,
		FControlSettingInfo("Camera Turning Boost", &CameraTurningBoost, &ARTSPlayerController::SetCameraTurningBoost,
			32.f));

	ControlSettings.Emplace(EControlSettingType::CameraDeceleration,
		FControlSettingInfo("Camera Deceleration", &CameraDeceleration, &ARTSPlayerController::SetCameraDeceleration,
			128000.f));

	ControlSettings.Emplace(EControlSettingType::CameraEdgeMovementThreshold,
		FControlSettingInfo("Camera Edge Movement Threshold", &CameraEdgeMovementThreshold, &ARTSPlayerController::SetCameraEdgeMovementThreshold,
			0.01f));

	ControlSettings.Emplace(EControlSettingType::CameraZoomIncrementalAmount,
		FControlSettingInfo("Camera Zoom Amount", &CameraZoomIncrementalAmount, &ARTSPlayerController::SetCameraZoomIncrementalAmount,
			100.f, { 25.f, 50.f, 75.f, 100.f, 125.f, 150.f, 175.f, 200.f }));

	ControlSettings.Emplace(EControlSettingType::CameraZoomSpeed,
		FControlSettingInfo("Camera Zoom Speed", &CameraZoomSpeed, &ARTSPlayerController::SetCameraZoomSpeed,
			2.5f, { 0.5f, 1.f, 1.5f, 2.f, 2.5f, 3.f, 3.5f, 4.f, 4.5f, 5.f, 5.5f, 6.f, 6.5f, 7.f, 7.5f, 8.f }));

	ControlSettings.Emplace(EControlSettingType::MMBLookYawSensitivity,
		FControlSettingInfo("Camera Free Look Yaw Sensitivity", &MMBYawSensitivity, &ARTSPlayerController::SetMMBLookYawSensitivity,
			2.5f, { 0.5f, 1.f, 1.5f, 2.f, 2.5f, 3.f, 3.5f, 4.f, 4.5f, 5.f, 5.5f, 6.f, 6.5f, 7.f, 7.5f, 8.f, 8.5f, 9.f, 9.5f, 10.f }));

	ControlSettings.Emplace(EControlSettingType::MMBLookPitchSensitivity,
		FControlSettingInfo("Camera Free Look Pitch Sensitivity", &MMBPitchSensitivity, &ARTSPlayerController::SetMMBLookPitchSensitivity,
			2.5f, { 0.1f, 0.3f, 0.5f, 0.7f, 0.9f, 1.1f, 1.3f, 1.5f, 1.7f, 1.9f, 2.1f, 2.3f, 2.5f, 2.7f, 2.9f, 3.1f }));

	ControlSettings.Emplace(EControlSettingType::bInvertMMBLookYaw,
		FControlSettingInfo("Invert Camera Free Look X Axis", &bInvertMMBYaw, &ARTSPlayerController::SetInvertMMBYawLook,
			true));

	ControlSettings.Emplace(EControlSettingType::bInvertMMBLookPitch,
		FControlSettingInfo("Invert Camera Free Look Y Axis", &bInvertMMBPitch, &ARTSPlayerController::SetInvertMMBPitchLook,
			true));

	ControlSettings.Emplace(EControlSettingType::bEnableMMBLookLag,
		FControlSettingInfo("Enable Camera Free Look Lag", &bEnableMMBLookLag, &ARTSPlayerController::SetEnableMMBLookLag,
			false));

	ControlSettings.Emplace(EControlSettingType::MMBLookLagAmount,
		FControlSettingInfo("Camera Free Look Lag Amount", &MMBLookLagAmount, &ARTSPlayerController::SetMMBLookLagAmount,
			10.f, { 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f, 13.f, 14.f, 15.f, 17.f, 20.f, 25.f, 30.f, 35.f }));

	ControlSettings.Emplace(EControlSettingType::DefaultCameraPitch,
		FControlSettingInfo("Default Camera Pitch", &DefaultCameraPitch, &ARTSPlayerController::SetDefaultCameraPitch,
			60.f, { 10.f, 15.f, 20.f, 25.f, 30.f, 35.f, 40.f, 45.f, 50.f, 55.f, 60.f, 65.f, 70.f, 75.f, 80.f, 85.f, 89.9f }));

	ControlSettings.Emplace(EControlSettingType::DefaultCameraZoomAmount,
		FControlSettingInfo("Default Camera Zoom", &DefaultCameraZoomAmount, &ARTSPlayerController::SetDefaultCameraZoomAmount,
			2000.f, { 200.f, 400.f, 600.f, 800.f, 1000.f, 1200.f, 1400.f, 1600.f, 1800.f, 2000.f, 2200.f, 2400.f, 2600.f, 2800.f, 3000.f, 3500.f, 4000.f }));

	ControlSettings.Emplace(EControlSettingType::ResetCameraToDefaultSpeed,
		FControlSettingInfo("Reset Camera Speed", &ResetCameraToDefaultSpeed, &ARTSPlayerController::SetResetCameraToDefaultRate,
			2.8f, { 0.8f, 1.3f, 1.8f, 2.3f, 2.8f, 3.3f, 3.8f, 4.5f, 5.5f, 7.f, 10.f }));

	/* Here default value is whatever is defined in project settings */
	ControlSettings.Emplace(EControlSettingType::DoubleClickTime,
		FControlSettingInfo("Double Click Time", &DoubleClickTime, &ARTSPlayerController::SetDoubleClickTime,
			GetDefault<UInputSettings>()->DoubleClickTime, { 0.025f, 0.05f, 0.075f, 0.1f, 0.125f, 0.15f, 0.175f, 0.2f, 0.225f, 0.25f, 0.275f, 0.3f, 0.325f, 0.35f, 0.375f, 0.4f, 0.45f, 0.5f }));

	ControlSettings.Emplace(EControlSettingType::MouseMovementThreshold,
		FControlSettingInfo("Click Threshold", &MouseMovementThreshold, &ARTSPlayerController::SetMouseMovementThreshold,
			0.5f, { 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.f, 1.2f, 1.4f, 1.6f, 1.8f, 2.f } ));

	//==========================================================================================
	//	Ghost Building Settings

	ControlSettings.Emplace(EControlSettingType::GhostRotationRadius,
		FControlSettingInfo("Ghost Building Rotation Radius", &GhostRotationRadius, &ARTSPlayerController::SetGhostRotationRadius,
			15.f, { 0.f, 1.f, 2.f, 3.f, 5.f, 10.f, 15.f, 20.f, 25.f, 30.f, 40.f, 50.f, 60.f, 70.f, 80.f, 90.f, 100.f }));

	ControlSettings.Emplace(EControlSettingType::GhostRotationSpeed,
		FControlSettingInfo("Ghost Building Rotation Speed", &GhostRotationSpeed, &ARTSPlayerController::SetGhostRotationSpeed,
			100.f, { 20.f, 40.f, 60.f, 80.f, 100.f, 120.f, 140.f, 160.f, 180.f, 200.f, 250.f, 400.f, 450.f, 500.f }));


	//========================================================================================
	//	Figuring out closest step for default values or generating step values
	//========================================================================================

	SetupControlSettingStepArrays();
}

void URTSGameUserSettings::SetupControlSettingStepArrays()
{
	for (auto & Pair : ControlSettings)
	{
		TArray<float> & Steps = Pair.Value.Steps;

		if (Steps.Num() > 0)
		{
			if (Steps.Num() > 1)
			{
#if !UE_BUILD_SHIPPING
				/* Check that the array is always strictly decreasing/increasing */
				
				if (Steps[0] < Steps[1])
				{
					float PreviousValue = Steps[0];
					for (int32 i = 1; i < Steps.Num(); ++i)
					{
						UE_CLOG(PreviousValue >= Steps[i], RTSLOG, Fatal, TEXT("Control setting [%s]: "
							"Steps array is not strictly decreasing/increasing. Error with indices [%d] "
							"and [%d]. Values were [%f] and [%f]"), TO_STRING(EControlSettingType, Pair.Key),
							i - 1, i, Steps[i - 1], Steps[i]);

						PreviousValue = Steps[i];
					}
				}
				else
				{
					float PreviousValue = Steps[0];
					for (int32 i = 1; i < Steps.Num(); ++i)
					{
						UE_CLOG(PreviousValue <= Steps[i], RTSLOG, Fatal, TEXT("Control setting [%s]: "
							"Steps array is not strictly decreasing/increasing. Error with indices [%d] "
							"and [%d]. Values were [%f] and [%f]"), TO_STRING(EControlSettingType, Pair.Key),
							i - 1, i, Steps[i - 1], Steps[i]);

						PreviousValue = Steps[i];
					}
				}
#endif

				/* Binary search for closest step number. Given these arrays will probably be quite
				small (20-40 entries) a linear search may be faster. */

				const float DefaultValue = Pair.Value.GetDefaultValue();
				/* Figure out if the values are increasing or decreasing */
				if (Steps[0] < Steps[1])
				{
					int32 Index = -1;
					int32 Low = 0;
					int32 High = Steps.Num() - 1;
					while (Low <= High)
					{
						const int32 Mid = (Low + High) / 2;
						if (Steps[Mid] < DefaultValue)
						{
							Low = Mid + 1;
						}
						else if (Steps[Mid] > DefaultValue)
						{
							High = Mid - 1;
						}
						else
						{
							assert(Steps[Mid] == DefaultValue);
							Index = Mid;
							break;
						}
					}

					if (Index == -1)
					{
						/* The exact default value was not one of the entries in Steps.
						For a shipping game this is not good cause it means the player will
						never be able to set the value back to the default value by adjusting
						the value. The only way it can be done is with some reset to defaults
						button */
						UE_LOG(RTSLOG, Warning, TEXT("Control setting [%s] has default value "
							"[%f]. However this is not in its steps array"),
							TO_STRING(EControlSettingType, Pair.Key), DefaultValue);

						/* If this isn't true then I may want to rewrite the line below */
						assert(Low == High);
						/* Use the closest step */
						Index = Low;
					}

					Pair.Value.DefaultValueStep = Index;
				}
				else // Binary search for decreasing array
				{
					int32 Index = -1;
					int32 Low = 0;
					int32 High = Steps.Num() - 1;
					while (Low <= High)
					{
						const int32 Mid = (Low + High) / 2;
						if (Steps[Mid] > DefaultValue)
						{
							Low = Mid + 1;
						}
						else if (Steps[Mid] < DefaultValue)
						{
							High = Mid - 1;
						}
						else
						{
							assert(Steps[Mid] == DefaultValue);
							Index = Mid;
							break;
						}
					}

					if (Index == -1)
					{
						UE_LOG(RTSLOG, Warning, TEXT("Control setting [%s] has default value "
							"[%f]. However this is not in its steps array "),
							TO_STRING(EControlSettingType, Pair.Key), DefaultValue);

						Index = Low;
					}

					Pair.Value.DefaultValueStep = Index;
				}
			}
			else
			{
				Pair.Value.DefaultValueStep = 0;
			}
		}
		else
		{
			/* Generate some steps. Note these values may not be remotely close to what you had in
			mind. This is really just so things can get up and working asap. For a shipping
			game you're better off hardcoding your own ones via the FControlSettingInfo ctor.

			Total of 20 entries.
			DefaultValue is near the middle */

			const float DefaultValue = Pair.Value.GetDefaultValue();
			Pair.Value.Steps.Reserve(20);
			// 1st value
			Steps.Emplace(0.f);
			// 15 more values
			for (int32 i = -9; i < 6; ++i)
			{
				Steps.Emplace(DefaultValue + (DefaultValue * i * 0.1f));
			}
			// Last 4 values
			Steps.Emplace(DefaultValue * 2.f);
			Steps.Emplace(DefaultValue * 3.f);
			Steps.Emplace(DefaultValue * 4.f);
			Steps.Emplace(DefaultValue * 5.f);

			Pair.Value.DefaultValueStep = 10;
		}
	}
}

bool URTSGameUserSettings::UseCPlusPlusDefaults() const
{
#if WITH_EDITOR
	return !bUseIniValues;
#else
	return GameStartType == EGameStartType::HasNotStartedGameBefore;
#endif
}

void URTSGameUserSettings::SetToDefaults()
{
	if (UseCPlusPlusDefaults())
	{	
		ResetProfileSettingsToDefaults();
		ResetControlSettingsToDefaults();
		ResetKeyMappingsToDefault();
		ResetAudioSettingsToDefaults();
		ResetCustomVideoSettingsToDefaults();

		Super::SetToDefaults();
	}
}

void URTSGameUserSettings::SetToDefaultsNoFail()
{
	ResetProfileSettingsToDefaults();
	ResetControlSettingsToDefaults();
	ResetKeyMappingsToDefault();
	ResetAudioSettingsToDefaults();
	ResetCustomVideoSettingsToDefaults();

	Super::SetToDefaults();
}

void URTSGameUserSettings::LoadSettings(bool bForceReload)
{	
	if (UseCPlusPlusDefaults() == false)
	{
		LoadSettingsInner(bForceReload);
	}
}

void URTSGameUserSettings::LoadSettingsNoFail(bool bForceReload)
{
	LoadSettingsInner(bForceReload);
}

void URTSGameUserSettings::LoadSettingsInner(bool bForceReload)
{
	Super::LoadSettings(bForceReload);

	KeyToAction.Reset();
	KeyToAction.Reserve(ActionToKey.Num());
	for (const auto & Elem : ActionToKey)
	{
		KeyToAction.Emplace(Elem.Value, Elem.Key);
	}

	KeyToAxis.Reset();
	KeyToAxis.Reserve(AxisToKey.Num());
	for (const auto & Elem : AxisToKey)
	{
		KeyToAxis.Emplace(Elem.Value, Elem.Key);
	}
}

void URTSGameUserSettings::InitialSetup(URTSGameInstance * GameInstance)
{
	/* Assuming this is called after UGameUserSettings::PostInitProperties() (should be) */

	// Maybe these can happen earlier. Calling ApplyAllKeyBindingSettings without having 
	// called this first will cause nothing
	//KeyMappings::CreateDefaultActionInfos();
	//AxisMappings::CreateDefaultAxisInfos();

	/* Populate SoundClassVolumes. The only reason this needs to be done is because the user
	may have deleted a sound class entry from their .ini file although perhaps it gets
	set to default value if that is the case */
	PopulateSoundClassVolumes(GameInstance);

	if (UseCPlusPlusDefaults())
	{
		/* Set all values to default values.
		SetToDefaults has already done it all except it would have missed out on the sound
		settings so just do them */
		ResetAudioSettingsToDefaults();
	}
	else
	{
#if WITH_EDITOR
		/* Doing this now so that changes to the .ini file while the editor is open take effect
		on the next PIE session.
		Probably has a slight hit on load times */
		LoadSettings(true);

		if (UseCPlusPlusDefaults())
		{
			/* The user changed bUseIniValues to false since last time */
			SetToDefaults();
		}
#endif
	}
}

void URTSGameUserSettings::ApplyAllSettings(URTSGameInstance * GameInstance, APlayerController * LocalPlayCon)
{
	ApplyProfileSettings(LocalPlayCon->PlayerState);
	ApplySoundSettings(GameInstance);
	ApplyControlSettings(LocalPlayCon);
	ApplyKeyMappingSettings(true);
	ApplyCustomVideoSettings(LocalPlayCon->GetWorld());
	ApplySettings(true);
}

void URTSGameUserSettings::ApplySettings(bool bCheckForCommandLineOverrides)
{
	/* Applying audio settings cannot be done here - no world context and setting a GI var for
	this class will still be null when this func is called, so need a another function to call
	manually from GI */

	Super::ApplySettings(bCheckForCommandLineOverrides);
}

bool URTSGameUserSettings::HasSeenProfile() const
{
	return bHasSeenProfile;
}

const FString & URTSGameUserSettings::GetPlayerAlias() const
{
	return PlayerAlias;
}

EFaction URTSGameUserSettings::GetDefaultFaction() const
{
	return DefaultFaction;
}

void URTSGameUserSettings::ChangeHasSeenProfile(bool bNewValue)
{
	bHasSeenProfile = bNewValue;
}

void URTSGameUserSettings::ChangePlayerAlias(const FString & NewName)
{
	PlayerAlias = NewName;

	/* Assumed changing alias while in a lobby or match is not allowed. Otherwise we 
	need to update the player state's name and make sure UI is updated and that other 
	players know about the change. APlayerState::OnRep_PlayerName is probably going to 
	be useful for that */
}

void URTSGameUserSettings::ChangeDefaultFaction(EFaction NewFaction)
{
	DefaultFaction = NewFaction;
}

void URTSGameUserSettings::ResetProfileSettingsToDefaults()
{
	bHasSeenProfile = false;

	/* The deal with this is: this default value is relevant only if ChangeProfileName has
	been called. */
	PlayerAlias = "Player";

	// EFaction() == EFaction::None which is not allowed
	DefaultFaction = Statics::ArrayIndexToFaction(0);
}

void URTSGameUserSettings::ApplyProfileSettings(APlayerState * LocalPlayerState)
{
	/* Set name in player state so clients can derive the players name using the replicated
	variable APlayerState::PlayerNamePrivate although a new player state is spawned when joining
	sessions so it actually does not matter. Player name needs to be passed in via
	GetGameLoginOptions() */

	/* Just this, the others "apply" automatically when they are changed. Actually when creating
	lobby game state will actually query the variable stored here instead anyway */
	LocalPlayerState->SetPlayerName(PlayerAlias);
}

const FControlSettingInfo & URTSGameUserSettings::GetControlSettingInfo(EControlSettingType SettingType) const
{
	return ControlSettings[SettingType];
}

float URTSGameUserSettings::GetControlSettingValue(EControlSettingType SettingType) const
{
	return ControlSettings[SettingType].GetValue();
}

void URTSGameUserSettings::ResetControlSettingsToDefaults()
{
	for (auto & Elem : ControlSettings)
	{
		// Set all values to defaults defined in ctor
		Elem.Value.ResetToDefault();
	}
}

void URTSGameUserSettings::ApplyControlSettings(APlayerController * InPlayerController)
{
	ARTSPlayerController * PlayCon = CastChecked<ARTSPlayerController>(InPlayerController);

	for (const auto & Elem : ControlSettings)
	{
		// Clamp value. Assumed only needed if loading from ini file
		Elem.Value.ClampValue();

		// Apply
		//Elem.Value.ApplyValue(PlayCon);  
	}

	// Apply

	PlayCon->SetCameraKeyboardMoveSpeed(GetControlSettingValue(EControlSettingType::CameraPanSpeed_Keyboard));
	PlayCon->SetCameraMouseMoveSpeed(GetControlSettingValue(EControlSettingType::CameraPanSpeed_Mouse));
	PlayCon->SetCameraMaxSpeed(GetControlSettingValue(EControlSettingType::CameraMaxSpeed));
	PlayCon->SetCameraAcceleration(GetControlSettingValue(EControlSettingType::CameraAcceleration));
	PlayCon->SetEnableCameraMovementLag(GetControlSettingValue(EControlSettingType::bEnableCameraMovementLag));
	PlayCon->SetCameraMovementLagSpeed(GetControlSettingValue(EControlSettingType::CameraMovementLagSpeed));
	PlayCon->SetCameraTurningBoost(GetControlSettingValue(EControlSettingType::CameraTurningBoost));
	PlayCon->SetCameraDeceleration(GetControlSettingValue(EControlSettingType::CameraDeceleration));
	PlayCon->SetCameraEdgeMovementThreshold(GetControlSettingValue(EControlSettingType::CameraEdgeMovementThreshold));
	PlayCon->SetCameraZoomIncrementalAmount(GetControlSettingValue(EControlSettingType::CameraZoomIncrementalAmount));
	PlayCon->SetCameraZoomSpeed(GetControlSettingValue(EControlSettingType::CameraZoomSpeed));
	PlayCon->SetMMBLookYawSensitivity(GetControlSettingValue(EControlSettingType::MMBLookYawSensitivity));
	PlayCon->SetMMBLookPitchSensitivity(GetControlSettingValue(EControlSettingType::MMBLookPitchSensitivity));

	// Important these happen after MMB sensitivities are set
	PlayCon->SetInvertMMBYawLook(GetControlSettingValue(EControlSettingType::bInvertMMBLookYaw));
	PlayCon->SetInvertMMBPitchLook(GetControlSettingValue(EControlSettingType::bInvertMMBLookPitch));

	PlayCon->SetEnableMMBLookLag(GetControlSettingValue(EControlSettingType::bEnableMMBLookLag));
	PlayCon->SetMMBLookLagAmount(GetControlSettingValue(EControlSettingType::MMBLookLagAmount));
	PlayCon->SetDefaultCameraPitch(GetControlSettingValue(EControlSettingType::DefaultCameraPitch));
	PlayCon->SetDefaultCameraZoomAmount(GetControlSettingValue(EControlSettingType::DefaultCameraZoomAmount));

	PlayCon->SetGhostRotationRadius(GetControlSettingValue(EControlSettingType::GhostRotationRadius));
	PlayCon->SetGhostRotationSpeed(GetControlSettingValue(EControlSettingType::GhostRotationSpeed));

	PlayCon->SetResetCameraToDefaultRate(GetControlSettingValue(EControlSettingType::ResetCameraToDefaultSpeed));

	PlayCon->SetDoubleClickTime(GetControlSettingValue(EControlSettingType::DoubleClickTime));

	PlayCon->SetMouseMovementThreshold(GetControlSettingValue(EControlSettingType::MouseMovementThreshold));
}


//--------------------------------------
//	Audio settings
//--------------------------------------

/* 
For VOLUME_MIN you have two options: 
- Set it to 0. Remember though that if a sound's volume hits 0 then it will stop 
playing unless you enable virtualization in the sound wav file (search "slient") so you'll 
probably want to do this for all your sound files. 
- Otherwise if you don't want to do that then a value like 0.001 might work ok here, but I 
would go with the tedius first option */
const float URTSGameUserSettings::VOLUME_MIN = 0.0f;
const float URTSGameUserSettings::VOLUME_MAX = 1.f;
const int16 URTSGameUserSettings::VOLUME_DEFAULT_STEP = 32;

const float URTSGameUserSettings::VOLUME_STEPS[NUM_VOLUME_STEPS] = {
	0.025f * 0,		0.025f * 1,		0.025f * 2,		0.025f * 3,		0.025f * 4, 
	0.025f * 5,		0.025f * 6,		0.025f * 7,		0.025f * 8,		0.025f * 9,		
	0.025f * 10,	0.025f * 11,	0.025f * 12,	0.025f * 13,	0.025f * 14, 
	0.025f * 15,	0.025f * 16,	0.025f * 17,	0.025f * 18,	0.025f * 19, 
	0.025f * 20,	0.025f * 21,	0.025f * 22,	0.025f * 23,	0.025f * 24,	
	0.025f * 25,	0.025f * 26,	0.025f * 27,	0.025f * 28,	0.025f * 29,	
	0.025f * 30,	0.025f * 31,	0.025f * 32,	0.025f * 33,	0.025f * 34, 
	0.025f * 35,	0.025f * 36,	0.025f * 37,	0.025f * 38,	0.025f * 39,	
	0.025f * 40
};

float URTSGameUserSettings::GetVolume(const FString & SoundClassName) const
{
	return VOLUME_STEPS[SoundClassVolumes[SoundClassName]];
}

int16 URTSGameUserSettings::GetStepCount(const FString & SoundClassName) const
{
	return SoundClassVolumes[SoundClassName];
}

void URTSGameUserSettings::SetVolume(URTSGameInstance * GameInst, const FString & SoundClassName, int16 NewVolumeStep)
{
	const int16 ClampedStep = FMath::Clamp<int16>(NewVolumeStep, 0, NUM_VOLUME_STEPS - 1);
	const float NewVolume = FMath::Clamp<float>(VOLUME_STEPS[ClampedStep], VOLUME_MIN, VOLUME_MAX);
	SoundClassVolumes[SoundClassName] = ClampedStep;

	USoundClass * SoundClass = GameInst->GetSoundClass(SoundClassName);

	// Actually apply the change but don't write to disk
	UGameplayStatics::SetSoundMixClassOverride(GameInst->GetWorld(), GameInst->GetSoundMix(),
		SoundClass, NewVolume, 1.f, 0.f);
}

void URTSGameUserSettings::AdjustVolume(URTSGameInstance * GameInst, const FString & SoundClassName, int16 StepAdjustAmount)
{
	const int16 CurrentVolumeStep = SoundClassVolumes[SoundClassName];
	const int16 NewStep = FMath::Clamp<int16>(CurrentVolumeStep + StepAdjustAmount, 0, NUM_VOLUME_STEPS - 1);
	const float NewVolume = FMath::Clamp<float>(VOLUME_STEPS[NewStep], VOLUME_MIN, VOLUME_MAX);
	SoundClassVolumes[SoundClassName] = NewStep;

	USoundClass * SoundClass = GameInst->GetSoundClass(SoundClassName);

	// Actually apply the change but don't write to disk
	UGameplayStatics::SetSoundMixClassOverride(GameInst->GetWorld(), GameInst->GetSoundMix(),
		SoundClass, NewVolume, 1.f, 0.f);
}

void URTSGameUserSettings::PopulateSoundClassVolumes(URTSGameInstance * GameInst)
{
	for (const auto & Elem : GameInst->GetSoundClasses())
	{
		const FString & SoundClassName = Elem.Key;
		USoundClass * SoundClass = Elem.Value;

		if (SoundClassVolumes.Contains(SoundClassName) == false)
		{
			const int16 DefaultStep = FMath::Clamp<int16>(VOLUME_DEFAULT_STEP, 0, NUM_VOLUME_STEPS - 1);
			SoundClassVolumes.Emplace(SoundClassName, DefaultStep);
		}
	}
}

void URTSGameUserSettings::ResetAudioSettingsToDefaults()
{
	// Make sure default value is within limits
	static const int16 StepToSet = FMath::Clamp<int16>(VOLUME_DEFAULT_STEP, 0, NUM_VOLUME_STEPS - 1);
	
	for (auto & Pair : SoundClassVolumes)
	{
		Pair.Value = StepToSet;
	}
}

void URTSGameUserSettings::ApplySoundSettings(URTSGameInstance * GameInstance)
{
	/* I have no idea why but this needs its own function, cannot do in ApplySettings even if
	setting a GI variable before call. MAYBE it was that editor needed restarting, I *think* I
	checked that but maybe try once more */

	/* Set volume for each sound class */
	for (const auto & Elem : GameInstance->GetSoundClasses())
	{
		const FString & SoundClassName = Elem.Key;
		USoundClass * SoundClass = Elem.Value;

		// Clamp value. Maybe log if outside limits before clamp, probably because .ini edited
		SoundClassVolumes[SoundClassName] = FMath::Clamp<int16>(SoundClassVolumes[SoundClassName], 0, NUM_VOLUME_STEPS - 1);

		const float VolumeToSet = FMath::Clamp<float>(VOLUME_STEPS[SoundClassVolumes[SoundClassName]], VOLUME_MIN, VOLUME_MAX);

		// Change the volume
		UGameplayStatics::SetSoundMixClassOverride(GameInstance->GetWorld(), GameInstance->GetSoundMix(),
			SoundClass, VolumeToSet, 1.f, 0.f);
	}
}

void URTSGameUserSettings::ResetKeyMappingsToDefault()
{
	// Reset action mappings
	ActionToKey.Reset();
	ActionToKey.Reserve(KeyMappings::NUM_ACTIONS);
	KeyToAction.Reset();
	KeyToAction.Reserve(KeyMappings::NUM_ACTIONS);
	for (const auto & Elem : KeyMappings::ActionInfos)
	{
		ActionToKey.Emplace(Elem.ActionAsEnum, Elem.KeyWithModifiers);
		KeyToAction.Emplace(Elem.KeyWithModifiers, Elem.ActionAsEnum);
	}

	// Reset axis mappings
	AxisToKey.Reset();
	AxisToKey.Reserve(AxisMappings::NUM_AXIS);
	KeyToAxis.Reset();
	KeyToAxis.Reserve(AxisMappings::NUM_AXIS);
	for (const auto & Elem : AxisMappings::AxisInfos)
	{
		AxisToKey.Emplace(Elem.AxisAsEnum, Elem.KeyWithModifiers.Key);
		KeyToAxis.Emplace(Elem.KeyWithModifiers.Key, Elem.AxisAsEnum);
	}
}

void URTSGameUserSettings::ApplyKeyMappingSettings(bool bSaveToDisk)
{	
	UInputSettings * InputSettings = UInputSettings::GetInputSettings();

	/* Quite a lot of TArray/TMap iterating here. Perhaps it can be done faster? */

	/* Does this get rid of the stuff like ESC to quit PIE? I think it does not */
	InputSettings->ActionMappings.Reset();
	
	for (const auto & Elem : KeyMappings::ActionInfos)
	{
		const FKeyWithModifiers & Key = ActionToKey[Elem.ActionAsEnum];
		InputSettings->AddActionMapping(FInputActionKeyMapping(Elem.ActionName, Key.Key,
			Key.HasSHIFTModifier(), Key.HasCTRLModifier(), Key.HasALTModifier(), Key.HasCMDModifier()), false);
	}

	InputSettings->AxisMappings.Reset();

	for (const auto & Elem : AxisMappings::AxisInfos)
	{
		const FKey & Key = AxisToKey[Elem.AxisAsEnum];
		InputSettings->AddAxisMapping(FInputAxisKeyMapping(Elem.ActionName, Key, Elem.Scale), false);
	}

	InputSettings->ForceRebuildKeymaps();

	if (bSaveToDisk)
	{
		/* I think this writes to /Saved/Config/Windows/Input.ini which I think should also 
		update the project settings */
		InputSettings->SaveKeyMappings();
	}
}

bool URTSGameUserSettings::RemapKeyBinding(EKeyMappingAction ActionToBind, const FKeyWithModifiers & Key, bool bForce, FTryBindActionResult & OutResult)
{
	UInputSettings * InputSettings = UInputSettings::GetInputSettings();

	const EKeyMappingAction ActionToUnbind = GetBoundAction(Key);
	if (ActionToUnbind != EKeyMappingAction::None)
	{	
		if (ActionToUnbind != ActionToBind)
		{	
			const FInputActionInfo & ToUnbindInfo = KeyMappings::ActionInfos[KeyMappings::ActionTypeToArrayIndex(ActionToUnbind)];
			if (ToUnbindInfo.bCanBeRemapped)
			{	
				OutResult.AlreadyBoundToKey_Action[0] = ActionToUnbind;
				if (!bForce)
				{
					return false;
				}
				
				// Using the param key. Assuming that that was the key that is bound to it
				InputSettings->RemoveActionMapping(FInputActionKeyMapping(ToUnbindInfo.ActionName, Key.Key, Key.HasSHIFTModifier(), Key.HasCTRLModifier(), Key.HasALTModifier()), false);

				ActionToKey[ActionToUnbind] = FKeyWithModifiers();
				KeyToAction[Key] = EKeyMappingAction::None;
			}
			else
			{
				OutResult.Warning = EGameWarning::WouldUnbindUnremappableAction;
				return false;
			}
		}
		else // Trying to bind action to the key it's already bound to. Don't need to do anything
		{
			return true;
		}
	}
	else
	{
		const FKeyWithModifiers KeyWithoutModifiers = FKeyWithModifiers(Key.Key, EKeyModifiers::None);
		const EKeyMappingAxis AxisToUnbind = GetBoundAxis(KeyWithoutModifiers);
		if (AxisToUnbind != EKeyMappingAxis::None)
		{
			const FInputAxisInfo & ToUnbindInfo = AxisMappings::AxisInfos[AxisMappings::AxisTypeToArrayIndex(AxisToUnbind)];
			if (ToUnbindInfo.bCanBeRemapped)
			{
				OutResult.AlreadyBoundToKey_Axis = AxisToUnbind;
				if (!bForce)
				{
					return false;
				}
				
				InputSettings->RemoveAxisMapping(FInputAxisKeyMapping(ToUnbindInfo.ActionName, Key.Key, ToUnbindInfo.Scale), false);

				AxisToKey[AxisToUnbind] = EKeys::Invalid;
				KeyToAxis[Key.Key] = EKeyMappingAxis::None;
			}
			else
			{
				OutResult.Warning = EGameWarning::WouldUnbindUnremappableAction;
				return false;
			}
		}
	}

	const FInputActionInfo & ToBindInfo = KeyMappings::ActionInfos[KeyMappings::ActionTypeToArrayIndex(ActionToBind)];
	const FKeyWithModifiers & CurrentKey = ActionToKey[ActionToBind];
	InputSettings->RemoveActionMapping(FInputActionKeyMapping(ToBindInfo.ActionName, CurrentKey.Key, CurrentKey.HasSHIFTModifier(), CurrentKey.HasCTRLModifier(), CurrentKey.HasALTModifier()), false);
	InputSettings->AddActionMapping(FInputActionKeyMapping(ToBindInfo.ActionName, Key.Key, Key.HasSHIFTModifier(), Key.HasCTRLModifier(), Key.HasALTModifier()), true);

	KeyToAction.Remove(CurrentKey);
	KeyToAction.Emplace(Key, ActionToBind);
	ActionToKey[ActionToBind] = Key;

	return true;
}

bool URTSGameUserSettings::RemapKeyBinding(EKeyMappingAxis ActionToBind, const FKey & Key, bool bForce, FTryBindActionResult & OutResult)
{
	UInputSettings * InputSettings = UInputSettings::GetInputSettings();

	const EKeyMappingAxis AxisToUnbind = GetBoundAxis(Key);
	if (AxisToUnbind != EKeyMappingAxis::None)
	{
		if (AxisToUnbind != ActionToBind)
		{
			const FInputAxisInfo & ToUnbindInfo = AxisMappings::AxisInfos[AxisMappings::AxisTypeToArrayIndex(AxisToUnbind)];
			if (ToUnbindInfo.bCanBeRemapped)
			{
				OutResult.AlreadyBoundToKey_Axis = AxisToUnbind;
				if (!bForce)
				{
					return false;
				}
				
				InputSettings->RemoveAxisMapping(FInputAxisKeyMapping(ToUnbindInfo.ActionName, Key, ToUnbindInfo.Scale), false);

				AxisToKey[AxisToUnbind] = EKeys::Invalid;
				KeyToAxis[Key] = EKeyMappingAxis::None;
			}
			else
			{
				OutResult.Warning = EGameWarning::WouldUnbindUnremappableAction;
				return false;
			}
		}
		else // Trying to bind action to the key it's already bound to. Don't need to do anything
		{
			return true;
		}
	}
	else
	{
		/* Unbind every action that is bound to the param Key even if it has a modifier variation 
		(if bForce is true) */
		int32 NumCollidingActions = 0;
		for (uint8 i = 0; i < 8; ++i)
		{
			const EKeyModifiers Modifiers = static_cast<EKeyModifiers>(i);
			const FKeyWithModifiers ActionKey = FKeyWithModifiers(Key, Modifiers);
			const EKeyMappingAction ActionToUnbind = GetBoundAction(ActionKey);
			if (ActionToUnbind != EKeyMappingAction::None)
			{
				const FInputActionInfo & ToUnbindInfo = KeyMappings::ActionInfos[KeyMappings::ActionTypeToArrayIndex(ActionToUnbind)];
				if (ToUnbindInfo.bCanBeRemapped)
				{
					OutResult.AlreadyBoundToKey_Action[NumCollidingActions++] = ActionToUnbind;
					if (bForce)
					{
						InputSettings->RemoveActionMapping(FInputActionKeyMapping(ToUnbindInfo.ActionName, ActionKey.Key, ActionKey.HasSHIFTModifier(), ActionKey.HasCTRLModifier(), ActionKey.HasALTModifier(), ActionKey.HasCMDModifier()), false);

						ActionToKey[ActionToUnbind] = FKeyWithModifiers();
						KeyToAction[ActionKey] = EKeyMappingAction::None;
					}
				}
				else
				{
					OutResult.Warning = EGameWarning::WouldUnbindUnremappableAction;
					return false;
				}
			}
		}

		if (NumCollidingActions > 0 && !bForce)
		{
			return false;
		}
	}

	const FInputAxisInfo & ToBindInfo = AxisMappings::AxisInfos[AxisMappings::AxisTypeToArrayIndex(ActionToBind)];
	const FKey & CurrentKey = AxisToKey[ActionToBind];
	InputSettings->RemoveAxisMapping(FInputAxisKeyMapping(ToBindInfo.ActionName, CurrentKey, ToBindInfo.Scale), false);
	InputSettings->AddAxisMapping(FInputAxisKeyMapping(ToBindInfo.ActionName, Key, ToBindInfo.Scale), true);

	KeyToAxis.Remove(CurrentKey);
	KeyToAxis.Emplace(Key, ActionToBind);
	AxisToKey[ActionToBind] = Key;

	return true;
}

EKeyMappingAction URTSGameUserSettings::GetBoundAction(const FKeyWithModifiers & Key) const
{
	if (KeyToAction.Contains(Key))
	{
		return KeyToAction[Key];
	}
	else
	{
		return EKeyMappingAction::None;
	}
}

EKeyMappingAction URTSGameUserSettings::GetBoundActionIgnoringModifiers(const FKey & Key) const
{
	if (KeyToAction.Contains(FKeyWithModifiers(Key, EKeyModifiers::None)))
	{
		return KeyToAction[FKeyWithModifiers(Key, EKeyModifiers::None)];
	}
	else if (KeyToAction.Contains(FKeyWithModifiers(Key, EKeyModifiers::CTRL)))
	{
		return KeyToAction[FKeyWithModifiers(Key, EKeyModifiers::CTRL)];
	}
	else if (KeyToAction.Contains(FKeyWithModifiers(Key, EKeyModifiers::ALT)))
	{
		return KeyToAction[FKeyWithModifiers(Key, EKeyModifiers::ALT)];
	}
	else if (KeyToAction.Contains(FKeyWithModifiers(Key, EKeyModifiers::CTRL_ALT)))
	{
		return KeyToAction[FKeyWithModifiers(Key, EKeyModifiers::CTRL_ALT)];
	}
	else if (KeyToAction.Contains(FKeyWithModifiers(Key, EKeyModifiers::SHIFT)))
	{
		return KeyToAction[FKeyWithModifiers(Key, EKeyModifiers::SHIFT)];
	}
	else if (KeyToAction.Contains(FKeyWithModifiers(Key, EKeyModifiers::CTRL_SHIFT)))
	{
		return KeyToAction[FKeyWithModifiers(Key, EKeyModifiers::CTRL_SHIFT)];
	}
	else if (KeyToAction.Contains(FKeyWithModifiers(Key, EKeyModifiers::ALT_SHIFT)))
	{
		return KeyToAction[FKeyWithModifiers(Key, EKeyModifiers::ALT_SHIFT)];
	}
	else if (KeyToAction.Contains(FKeyWithModifiers(Key, EKeyModifiers::CTRL_ALT_SHIFT)))
	{
		return KeyToAction[FKeyWithModifiers(Key, EKeyModifiers::CTRL_ALT_SHIFT)];
	}
	else
	{
		return EKeyMappingAction::None;
	}
}

EKeyMappingAxis URTSGameUserSettings::GetBoundAxis(const FKeyWithModifiers & Key) const
{
	/* I have a rule that only one action can be bound to a key. 
	With axis mappings they ignore modifiers so it violates that rule to let something 
	be bound to a key that is bound to an axis action if it differs only by modifiers. 
	e.g. moving the camera is an axis mapping. If it is bound to W and I want to bind 
	PauseGame to CTRL + W that is now allowed.
	Bottom line: ignore modifiers when returning result here */
	return GetBoundAxis(Key.Key);
}

EKeyMappingAxis URTSGameUserSettings::GetBoundAxis(const FKey & Key) const
{
	if (KeyToAxis.Contains(Key))
	{
		return KeyToAxis[Key];
	}
	else
	{
		return EKeyMappingAxis::None;
	}
}

void URTSGameUserSettings::ResetCustomVideoSettingsToDefaults()
{
	AntiAliasingMethod = AAM_TemporalAA;
}

void URTSGameUserSettings::ApplyCustomVideoSettings(UWorld * World)
{
	SetAntiAliasingMethod(World, AntiAliasingMethod);
}

int32 URTSGameUserSettings::GetOverallScalabilityLevel() const
{
	const int32 SuperValue = Super::GetOverallScalabilityLevel();
	if (SuperValue == -1)
	{
		return SuperValue;
	}
	else
	{
		// Take into account anti-aliasing method
		// Values are same as in SetOverallScalabilityLevel. If modifying make sure to modify this func here too
		if (SuperValue == 0)
		{
			// Lowest setting uses no AA
			if (AntiAliasingMethod == AAM_None)
			{
				return 0;
			}
			else
			{
				return -1;
			}
		}
		else
		{
			// Everything else uses TAA
			if (AntiAliasingMethod == AAM_TemporalAA)
			{
				return SuperValue;
			}
			else
			{
				return -1;
			}
		}
	}
}

void URTSGameUserSettings::SetOverallScalabilityLevel(int32 Value)
{
	/* Set the anti-aliasing method */
	if (Value == 0)
	{
		SetAntiAliasingMethod(GetWorld(), AAM_None);
	}
	else if (Value == 1 || Value == 2 || Value == 3 || Value == 4)
	{
		SetAntiAliasingMethod(GetWorld(), AAM_TemporalAA);
	}

	Super::SetOverallScalabilityLevel(Value);
}

EAntiAliasingMethod URTSGameUserSettings::GetAntiAliasingMethod() const
{
	return AntiAliasingMethod;
	
	/* The code below will get the actual applied value */
	//IConsoleVariable * CVarDefaultAntiAliasing = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DefaultFeature.AntiAliasing"));
	//return (EAntiAliasingMethod)CVarDefaultAntiAliasing->GetInt();
}

void URTSGameUserSettings::SetAntiAliasingMethod(UWorld * World, EAntiAliasingMethod Method)
{
	AntiAliasingMethod = Method;

	switch (Method)
	{
	case AAM_None:
		GEngine->Exec(World, TEXT("r.DefaultFeature.AntiAliasing 0"));
		break;
	case AAM_FXAA:
		GEngine->Exec(World, TEXT("r.DefaultFeature.AntiAliasing 1"));
		break;
	case AAM_TemporalAA:
		GEngine->Exec(World, TEXT("r.DefaultFeature.AntiAliasing 2"));
		break;
	default:
		UE_LOG(RTSLOG, Fatal, TEXT("Unaccounted for anti-aliasing method: [%s]"),
			TO_STRING(EAntiAliasingMethod, Method));
		break;
	}
}
