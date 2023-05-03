// Fill out your copyright notice in the Description page of Project Settings.

#include "KeyMappings.h"

#include "Statics/DevelopmentStatics.h"
//#include "Settings/RTSGameUserSettings.h"
#include "GameFramework/RTSPlayerController.h"


FInputInfoBase::FInputInfoBase()
	: DisplayName(FText::FromString("Error"))
	, ActionName(NAME_None)
	, KeyWithModifiers(FKeyWithModifiers(EKeys::Invalid))
	, ActionAsEnum(EKeyMappingAction::None)
	, AxisAsEnum(EKeyMappingAxis::None)
	, bExecuteWhenPaused(false) 
	, bCanBeRemapped(true)
	, bIsEditorOnly(false)
	, bIsForPlayer(true)
	, bIsForObserver(true)
{
}

FInputInfoBase::FInputInfoBase(const FString & InDisplayName, EKeyMappingAction ActionEnum, EKeyMappingAxis AxisEnum, const FKey & InDefaultKey, EKeyModifiers KeyModifierFlags, EMappingTarget WhoMappingIsFor, bool bShouldExecuteWhenPaused, bool bIsAllowedToBeRemapped, bool bInIsEditorOnly)
	: DisplayName(FText::FromString(InDisplayName)) 
	, ActionName(*InDisplayName) 
	, KeyWithModifiers(FKeyWithModifiers(InDefaultKey, KeyModifierFlags))
	, ActionAsEnum(ActionEnum) 
	, AxisAsEnum(AxisEnum)
	, bExecuteWhenPaused(bShouldExecuteWhenPaused) 
	, bCanBeRemapped(bIsAllowedToBeRemapped)
	, bIsEditorOnly(bInIsEditorOnly)
	, bIsForPlayer(WhoMappingIsFor == EMappingTarget::PlayerOnly || WhoMappingIsFor == EMappingTarget::PlayerAndObserver)
	, bIsForObserver(WhoMappingIsFor == EMappingTarget::ObserverOnly || WhoMappingIsFor == EMappingTarget::PlayerAndObserver) 
{
}

bool FInputInfoBase::HasCTRLKeyModifier() const
{
	return KeyWithModifiers.HasCTRLModifier();
}

bool FInputInfoBase::HasALTKeyModifier() const
{
	return KeyWithModifiers.HasALTModifier();
}

bool FInputInfoBase::HasShiftKeyModifier() const
{
	return KeyWithModifiers.HasSHIFTModifier();
}


FInputActionInfo::FInputActionInfo(const FString & InDisplayName, EKeyMappingAction ActionEnum, const FKey & InDefaultKey, EKeyModifiers KeyModifierFlags, EMappingTarget WhoMappingIsFor, FName PressedAction, FName ReleasedAction, bool bShouldExecuteWhenPaused, bool bIsAllowedToBeRemapped, bool bIsEditorOnly)
	: FInputInfoBase(InDisplayName, ActionEnum, EKeyMappingAxis::None, InDefaultKey, KeyModifierFlags, WhoMappingIsFor, bShouldExecuteWhenPaused, bIsAllowedToBeRemapped, bIsEditorOnly)
	, OnPressedAction(PressedAction) 
	, OnReleasedAction(ReleasedAction) 
{
}

bool FInputActionInfo::HasActionForKeyPress() const
{
	return OnPressedAction != NAME_None;
}

bool FInputActionInfo::HasActionForKeyRelease() const
{
	return OnReleasedAction != NAME_None;
}


const TArray<FInputActionInfo, TFixedAllocator<KeyMappings::NUM_ACTIONS>> KeyMappings::ActionInfos = CreateDefaultActionInfos();
//FInputActionInfo KeyMappings::ActionInfos[KeyMappings::NUM_ACTIONS];
#if WITH_EDITOR
int32 KeyMappings::NumArrayEntries = 0;
#endif

TArray<FInputActionInfo, TFixedAllocator<KeyMappings::NUM_ACTIONS>> KeyMappings::CreateDefaultActionInfos()
{
	/* Guard against calling this multiple times */
	//static bool bHaveCalledThisAlready = false;
	//if (bHaveCalledThisAlready)
	//{
	//	return;
	//}
	//else
	//{
	//	bHaveCalledThisAlready = true;
	//}

	TArray<FInputActionInfo, TFixedAllocator<KeyMappings::NUM_ACTIONS>> Array;

	Array.Init(FInputActionInfo(), NUM_ACTIONS);
	//Array.SetNumUnsafeInternal(NUM_ACTIONS);

	//-------------------------------------------------------------------------------------------
	//===========================================================================================
	//	------- Add new entries below here -------
	//===========================================================================================
	//-------------------------------------------------------------------------------------------

	//----------------------------------------------------------------------
	//	Note: player controller function must be a UFUNCTION
	//----------------------------------------------------------------------

	// It's possible to remap these 2. It would just be really uncommon
	AddActionInfo(Array, "LMB", EKeyMappingAction::LMB, EKeys::LeftMouseButton, EKeyModifiers::None, EMappingTarget::PlayerAndObserver, PC_FUNC(Input_LMBPressed), PC_FUNC(Input_LMBReleased), false, false);
	AddActionInfo(Array, "RMB", EKeyMappingAction::RMB, EKeys::RightMouseButton, EKeyModifiers::None, EMappingTarget::PlayerAndObserver, PC_FUNC(Input_RMBPressed), PC_FUNC(Input_RMBReleased), false, false);

	//------------------------------------------------------------------------------------
	//	Camera 
	//------------------------------------------------------------------------------------
	AddActionInfo(Array, "Zoom Camera In", EKeyMappingAction::ZoomCameraIn, EKeys::MouseScrollUp, EKeyModifiers::None, EMappingTarget::PlayerAndObserver, PC_FUNC(Input_ZoomCameraIn));
	AddActionInfo(Array, "Zoom Camera Out", EKeyMappingAction::ZoomCameraOut, EKeys::MouseScrollDown, EKeyModifiers::None, EMappingTarget::PlayerAndObserver, PC_FUNC(Input_ZoomCameraOut));
	AddActionInfo(Array, "Enable Camera Look Around", EKeyMappingAction::EnableCameraLookAround, EKeys::MiddleMouseButton, EKeyModifiers::None, EMappingTarget::PlayerAndObserver, PC_FUNC(Input_EnableCameraFreeLook), PC_FUNC(Input_DisableCameraFreeLook));
	AddActionInfo(Array, "Reset Camera Rotation to Default", EKeyMappingAction::ResetCameraRotationToOriginal, EKeys::Invalid, EKeyModifiers::None, EMappingTarget::PlayerAndObserver, PC_FUNC(Input_ResetCameraRotationToOriginal));
	AddActionInfo(Array, "Reset Camera Zoom to Default", EKeyMappingAction::ResetCameraZoomToOriginal, EKeys::Invalid, EKeyModifiers::None, EMappingTarget::PlayerAndObserver, PC_FUNC(Input_ResetCameraZoomToOriginal));
	AddActionInfo(Array, "Reset Camera Rotation and Zoom to Default", EKeyMappingAction::ResetCameraRotationAndZoomToOriginal, EKeys::End, EKeyModifiers::None, EMappingTarget::PlayerAndObserver, PC_FUNC(Input_ResetCameraRotationAndZoomToOriginal));
	
	//------------------------------------------------------------------------------------
	//	Stuff
	//------------------------------------------------------------------------------------
	AddActionInfo(Array, "Open Team Chat", EKeyMappingAction::OpenTeamChat, EKeys::Enter, EKeyModifiers::None, EMappingTarget::PlayerAndObserver, PC_FUNC(Input_OpenTeamChat));
	AddActionInfo(Array, "Open All Chat", EKeyMappingAction::OpenAllChat, EKeys::Enter, EKeyModifiers::SHIFT, EMappingTarget::PlayerAndObserver,	PC_FUNC(Input_OpenAllChat));
#if WITH_EDITOR
	/* Usually can't use ESC while with editor so use P instead */
	AddActionInfo(Array, "Open Pause Menu / Cancel", EKeyMappingAction::OpenPauseMenuSlashCancel, EKeys::P, EKeyModifiers::None, EMappingTarget::PlayerAndObserver, PC_FUNC(Input_Cancel), NAME_None, true);
#else
	AddActionInfo(Array, "Open Pause Menu / Cancel", EKeyMappingAction::OpenPauseMenuSlashCancel, EKeys::Escape, EKeyModifiers::None, EMappingTarget::PlayerAndObserver, PC_FUNC(Input_Cancel), NAME_None, true);
#endif
	AddActionInfo(Array, "Open Commander Skill Tree", EKeyMappingAction::OpenCommanderSkillTree, EKeys::Invalid, EKeyModifiers::None, EMappingTarget::PlayerOnly,	PC_FUNC(Input_OpenCommanderSkillTree));
	AddActionInfo(Array, "Close Commander Skill Tree", EKeyMappingAction::CloseCommanderSkillTree, EKeys::Invalid, EKeyModifiers::None, EMappingTarget::PlayerOnly, PC_FUNC(Input_CloseCommanderSkillTree));
	AddActionInfo(Array, "Toggle Commander Skill Tree", EKeyMappingAction::ToggleCommanderSkillTree, EKeys::Tab, EKeyModifiers::None, EMappingTarget::PlayerOnly,	PC_FUNC(Input_ToggleCommanderSkillTree));

	//------------------------------------------------------------------------------------
	//	Editor only
	//------------------------------------------------------------------------------------

	AddActionInfo(Array, "Insta Quit Game", EKeyMappingAction::InstaQuitGame, EKeys::Q, EKeyModifiers::None, EMappingTarget::PlayerAndObserver, PC_FUNC(Input_QuitGame), NAME_None, true, true, true);
	AddActionInfo(Array, "Show Development Cheat Widget", EKeyMappingAction::ShowDevelopmentCheatWidget, EKeys::Equals, EKeyModifiers::None, EMappingTarget::PlayerAndObserver, PC_FUNC(Input_ToggleDevelopmentCheatWidget), NAME_None, false, true, true);
	
	//------------------------------------------------------------------------------------
	//	Control groups
	//------------------------------------------------------------------------------------
	AddActionInfo(Array, "Assign Control Group 0", EKeyMappingAction::AssignControlGroup0, EKeys::Zero, EKeyModifiers::CTRL, EMappingTarget::PlayerOnly,	PC_FUNC(Input_AssignControlGroupButtonPressed_0));
	AddActionInfo(Array, "Assign Control Group 1", EKeyMappingAction::AssignControlGroup1, EKeys::One, EKeyModifiers::CTRL, EMappingTarget::PlayerOnly,		PC_FUNC(Input_AssignControlGroupButtonPressed_1));
	AddActionInfo(Array, "Assign Control Group 2", EKeyMappingAction::AssignControlGroup2, EKeys::Two, EKeyModifiers::CTRL, EMappingTarget::PlayerOnly,		PC_FUNC(Input_AssignControlGroupButtonPressed_2));
	AddActionInfo(Array, "Assign Control Group 3", EKeyMappingAction::AssignControlGroup3, EKeys::Three, EKeyModifiers::CTRL, EMappingTarget::PlayerOnly,	PC_FUNC(Input_AssignControlGroupButtonPressed_3));
	AddActionInfo(Array, "Assign Control Group 4", EKeyMappingAction::AssignControlGroup4, EKeys::Four, EKeyModifiers::CTRL, EMappingTarget::PlayerOnly,	PC_FUNC(Input_AssignControlGroupButtonPressed_4));
	AddActionInfo(Array, "Assign Control Group 5", EKeyMappingAction::AssignControlGroup5, EKeys::Five, EKeyModifiers::CTRL, EMappingTarget::PlayerOnly,	PC_FUNC(Input_AssignControlGroupButtonPressed_5));
	AddActionInfo(Array, "Assign Control Group 6", EKeyMappingAction::AssignControlGroup6, EKeys::Six, EKeyModifiers::CTRL, EMappingTarget::PlayerOnly,		PC_FUNC(Input_AssignControlGroupButtonPressed_6));
	AddActionInfo(Array, "Assign Control Group 7", EKeyMappingAction::AssignControlGroup7, EKeys::Seven, EKeyModifiers::CTRL, EMappingTarget::PlayerOnly,	PC_FUNC(Input_AssignControlGroupButtonPressed_7));
	AddActionInfo(Array, "Assign Control Group 8", EKeyMappingAction::AssignControlGroup8, EKeys::Eight, EKeyModifiers::CTRL, EMappingTarget::PlayerOnly,	PC_FUNC(Input_AssignControlGroupButtonPressed_8));
	AddActionInfo(Array, "Assign Control Group 9", EKeyMappingAction::AssignControlGroup9, EKeys::Nine, EKeyModifiers::CTRL, EMappingTarget::PlayerOnly,	PC_FUNC(Input_AssignControlGroupButtonPressed_9));
	AddActionInfo(Array, "Select Control Group 0", EKeyMappingAction::SelectControlGroup0, EKeys::Zero, EKeyModifiers::None, EMappingTarget::PlayerOnly,	PC_FUNC(Input_SelectControlGroupButtonPressed_0), PC_FUNC(Input_SelectControlGroupButtonReleased_0));
	AddActionInfo(Array, "Select Control Group 1", EKeyMappingAction::SelectControlGroup1, EKeys::One, EKeyModifiers::None, EMappingTarget::PlayerOnly,		PC_FUNC(Input_SelectControlGroupButtonPressed_1), PC_FUNC(Input_SelectControlGroupButtonReleased_1));
	AddActionInfo(Array, "Select Control Group 2", EKeyMappingAction::SelectControlGroup2, EKeys::Two, EKeyModifiers::None, EMappingTarget::PlayerOnly,		PC_FUNC(Input_SelectControlGroupButtonPressed_2), PC_FUNC(Input_SelectControlGroupButtonReleased_2));
	AddActionInfo(Array, "Select Control Group 3", EKeyMappingAction::SelectControlGroup3, EKeys::Three, EKeyModifiers::None, EMappingTarget::PlayerOnly,	PC_FUNC(Input_SelectControlGroupButtonPressed_3), PC_FUNC(Input_SelectControlGroupButtonReleased_3));
	AddActionInfo(Array, "Select Control Group 4", EKeyMappingAction::SelectControlGroup4, EKeys::Four, EKeyModifiers::None, EMappingTarget::PlayerOnly,	PC_FUNC(Input_SelectControlGroupButtonPressed_4), PC_FUNC(Input_SelectControlGroupButtonReleased_4));
	AddActionInfo(Array, "Select Control Group 5", EKeyMappingAction::SelectControlGroup5, EKeys::Five, EKeyModifiers::None, EMappingTarget::PlayerOnly,	PC_FUNC(Input_SelectControlGroupButtonPressed_5), PC_FUNC(Input_SelectControlGroupButtonReleased_5));
	AddActionInfo(Array, "Select Control Group 6", EKeyMappingAction::SelectControlGroup6, EKeys::Six, EKeyModifiers::None, EMappingTarget::PlayerOnly,		PC_FUNC(Input_SelectControlGroupButtonPressed_6), PC_FUNC(Input_SelectControlGroupButtonReleased_6));
	AddActionInfo(Array, "Select Control Group 7", EKeyMappingAction::SelectControlGroup7, EKeys::Seven, EKeyModifiers::None, EMappingTarget::PlayerOnly,	PC_FUNC(Input_SelectControlGroupButtonPressed_7), PC_FUNC(Input_SelectControlGroupButtonReleased_7));
	AddActionInfo(Array, "Select Control Group 8", EKeyMappingAction::SelectControlGroup8, EKeys::Eight, EKeyModifiers::None, EMappingTarget::PlayerOnly,	PC_FUNC(Input_SelectControlGroupButtonPressed_8), PC_FUNC(Input_SelectControlGroupButtonReleased_8));
	AddActionInfo(Array, "Select Control Group 9", EKeyMappingAction::SelectControlGroup9, EKeys::Nine, EKeyModifiers::None, EMappingTarget::PlayerOnly,	PC_FUNC(Input_SelectControlGroupButtonPressed_9), PC_FUNC(Input_SelectControlGroupButtonReleased_9));

	//-------------------------------------------------------------------------------------------
	//===========================================================================================
	//	------- Add new entries above here -------
	//===========================================================================================
	//-------------------------------------------------------------------------------------------

#if !UE_BUILD_SHIPPING
	/* Check every value defined in EKeyMappingAction has an entry for it */
	if (NUM_ACTIONS != NumArrayEntries)
	{
		/* If this throws you've added too many entries too Array. 
		OR you've called this func somewhere else in code */
		assert(NumArrayEntries < NUM_ACTIONS);

		/* Find the missing actions so the crash log message is more helpful */
		FString String;
		for (int32 i = 0; i < NUM_ACTIONS; ++i)
		{
			if (Array[i].ActionAsEnum != ArrayIndexToActionType(i))
			{
				if (String.Len() > 0)
				{
					String += ", ";
				}
				String += ENUM_TO_STRING(EKeyMappingAction, ArrayIndexToActionType(i));
			}
		}

		UE_LOG(RTSLOG, Fatal, TEXT("Not all values in EKeyMappingAction have been added to array. "
			"Missing: [%s] "), *String);
	}
#endif

	return Array;
}

void KeyMappings::AddActionInfo(TArray<FInputActionInfo, TFixedAllocator<NUM_ACTIONS>>& OutArray, const FString & DisplayName, EKeyMappingAction KeyMappingAction, const FKey & Key, EKeyModifiers Modifiers, EMappingTarget WhoItIsFor, FName PressedAction, FName ReleasedAction, bool bShouldExecuteWhenPaused, bool bIsAllowedToBeRemapped, bool bIsEditorOnly)
{
	const int32 ArrayIndex = ActionTypeToArrayIndex(KeyMappingAction);

	// Make sure the same action type hasn't been used mistakingly
	assert(OutArray[ArrayIndex].ActionAsEnum == EKeyMappingAction::None);
	
	OutArray[ArrayIndex] = FInputActionInfo(DisplayName, KeyMappingAction, Key, Modifiers, WhoItIsFor,
		PressedAction, ReleasedAction, bShouldExecuteWhenPaused, bIsAllowedToBeRemapped, bIsEditorOnly);

#if WITH_EDITOR
	NumArrayEntries++;
#endif
}

void KeyMappings::AddActionInfo(FInputActionInfo * OutArray, const FString & DisplayName, EKeyMappingAction KeyMappingAction, const FKey & Key, EKeyModifiers Modifiers, EMappingTarget WhoItIsFor, FName PressedAction, FName ReleasedAction, bool bShouldExecuteWhenPaused, bool bIsAllowedToBeRemapped, bool bIsEditorOnly)
{
	const int32 ArrayIndex = ActionTypeToArrayIndex(KeyMappingAction);

	// Make sure the same action type hasn't been used mistakingly
	assert(OutArray[ArrayIndex].ActionAsEnum == EKeyMappingAction::None);
	
	OutArray[ArrayIndex] = FInputActionInfo(DisplayName, KeyMappingAction, Key, Modifiers, WhoItIsFor,
		PressedAction, ReleasedAction, bShouldExecuteWhenPaused, bIsAllowedToBeRemapped, bIsEditorOnly);

#if WITH_EDITOR
	NumArrayEntries++;
#endif
}

int32 KeyMappings::ActionTypeToArrayIndex(EKeyMappingAction KeyMappingAction)
{
	return static_cast<int32>(KeyMappingAction) - 1;
}

EKeyMappingAction KeyMappings::ArrayIndexToActionType(int32 ArrayIndex)
{
	return static_cast<EKeyMappingAction>(ArrayIndex + 1);
}


FInputAxisInfo::FInputAxisInfo(const FString & InName, EKeyMappingAxis AxisEnum, const FKey & InKey, float InScale, EMappingTarget WhoMappingIsFor, FName InFunctionPtr, bool bShouldExecuteWhenPaused, bool bIsAllowedToBeRemapped, bool bIsEditorOnly)
	: FInputInfoBase(InName, EKeyMappingAction::None, AxisEnum, InKey, EKeyModifiers::None, WhoMappingIsFor, bShouldExecuteWhenPaused, bIsAllowedToBeRemapped, bIsEditorOnly)
	, FunctionPtr(InFunctionPtr) 
	, Scale(InScale)
{
}


const TArray<FInputAxisInfo, TFixedAllocator<AxisMappings::NUM_AXIS>> AxisMappings::AxisInfos = CreateDefaultAxisInfos();
//FInputAxisInfo AxisMappings::AxisInfos[AxisMappings::NUM_AXIS];
#if WITH_EDITOR
int32 AxisMappings::NumArrayEntries = 0;
#endif

TArray<FInputAxisInfo, TFixedAllocator<AxisMappings::NUM_AXIS>> AxisMappings::CreateDefaultAxisInfos()
{
	/* Guard against calling this multiple times */
	//static bool bHaveCalledThisAlready = false;
	//if (bHaveCalledThisAlready)
	//{
	//	return;
	//}
	//else
	//{
	//	bHaveCalledThisAlready = true;
	//}

	TArray<FInputAxisInfo, TFixedAllocator<AxisMappings::NUM_AXIS>> Array;

	Array.Init(FInputAxisInfo(), NUM_AXIS);
	//Array.SetNumUnsafeInternal(NUM_AXIS);

	//-------------------------------------------------------------------------------------------
	//===========================================================================================
	//	------- Add new entries below here -------
	//===========================================================================================
	//-------------------------------------------------------------------------------------------

	//----------------------------------------------------------------------
	//	Note: player controller function must be a UFUNCTION
	//----------------------------------------------------------------------

	AddAxisInfo(Array, "MouseMove_X", EKeyMappingAxis::MouseMoveX, EKeys::MouseX, 1.f, EMappingTarget::PlayerAndObserver, PC_FUNC(On_Mouse_Move_X), false, false);
	AddAxisInfo(Array, "MouseMove_Y", EKeyMappingAxis::MouseMoveY, EKeys::MouseY, 1.f, EMappingTarget::PlayerAndObserver, PC_FUNC(On_Mouse_Move_Y), false, false);

	//--------------------------------------------------------------------------------------
	//	Camera
	//--------------------------------------------------------------------------------------
	AddAxisInfo(Array, "Move Camera Left", EKeyMappingAxis::MoveCameraLeft, EKeys::A, -1.f, EMappingTarget::PlayerAndObserver,			PC_FUNC(Axis_MoveCameraRight));
	AddAxisInfo(Array, "Move Camera Right", EKeyMappingAxis::MoveCameraRight, EKeys::D, 1.f, EMappingTarget::PlayerAndObserver,			PC_FUNC(Axis_MoveCameraRight));
	AddAxisInfo(Array, "Move Camera Forward", EKeyMappingAxis::MoveCameraForward, EKeys::W, 1.f, EMappingTarget::PlayerAndObserver,		PC_FUNC(Axis_MoveCameraForward));
	AddAxisInfo(Array, "Move Camera Backward", EKeyMappingAxis::MoveCameraBackward, EKeys::S, -1.f, EMappingTarget::PlayerAndObserver,	PC_FUNC(Axis_MoveCameraForward));

	//-------------------------------------------------------------------------------------------
	//===========================================================================================
	//	------- Add new entries above here -------
	//===========================================================================================
	//-------------------------------------------------------------------------------------------

#if !UE_BUILD_SHIPPING
	/* Check every value defined in EKeyMappingAxis has an entry for it */
	if (NUM_AXIS != NumArrayEntries)
	{
		// If thrown means we've somehow added too many 
		assert(NumArrayEntries < NUM_AXIS);

		/* Find the missing actions so the crash log message is more helpful */
		FString String;
		for (int32 i = 0; i < NUM_AXIS; ++i)
		{
			if (Array[i].AxisAsEnum != ArrayIndexToAxisType(i))
			{
				if (String.Len() > 0)
				{
					String += ", ";
				}
				String += ENUM_TO_STRING(EKeyMappingAxis, ArrayIndexToAxisType(i));
			}
		}

		UE_LOG(RTSLOG, Fatal, TEXT("Not all values in EKeyMappingAxis have been added to array. "
			"Missing: [%s] "), *String);
	}
#endif

	return Array;
}

void AxisMappings::AddAxisInfo(TArray<FInputAxisInfo, TFixedAllocator<NUM_AXIS>>& OutArray, const FString & InName, EKeyMappingAxis AxisEnum, const FKey & InKey, float InScale, EMappingTarget WhoMappingIsFor, FName InFunctionPtr, bool bShouldExecuteWhenPaused, bool bIsAllowedToBeRemapped, bool bIsEditorOnly)
{
	const int32 ArrayIndex = AxisTypeToArrayIndex(AxisEnum);
	
	// Make sure the same action type hasn't been used mistakingly
	assert(OutArray[ArrayIndex].AxisAsEnum == EKeyMappingAxis::None);
	
	OutArray[ArrayIndex] = FInputAxisInfo(InName, AxisEnum, InKey, InScale, WhoMappingIsFor, 
		InFunctionPtr, bShouldExecuteWhenPaused, bIsAllowedToBeRemapped, bIsEditorOnly);

#if WITH_EDITOR
	NumArrayEntries++;
#endif
}

void AxisMappings::AddAxisInfo(FInputAxisInfo * OutArray, const FString & InName, EKeyMappingAxis AxisEnum, const FKey & InKey, float InScale, EMappingTarget WhoMappingIsFor, FName InFunctionPtr, bool bShouldExecuteWhenPaused, bool bIsAllowedToBeRemapped, bool bIsEditorOnly)
{
	const int32 ArrayIndex = AxisTypeToArrayIndex(AxisEnum);

	// Make sure the same action type hasn't been used mistakingly
	assert(OutArray[ArrayIndex].AxisAsEnum == EKeyMappingAxis::None);

	OutArray[ArrayIndex] = FInputAxisInfo(InName, AxisEnum, InKey, InScale, WhoMappingIsFor,
		InFunctionPtr, bShouldExecuteWhenPaused, bIsAllowedToBeRemapped, bIsEditorOnly);

#if WITH_EDITOR
	NumArrayEntries++;
#endif
}

int32 AxisMappings::AxisTypeToArrayIndex(EKeyMappingAxis AxisEnum)
{
	return static_cast<int32>(AxisEnum) - 1;
}

EKeyMappingAxis AxisMappings::ArrayIndexToAxisType(int32 ArrayIndex)
{
	return static_cast<EKeyMappingAxis>(ArrayIndex + 1);
}


