// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Statics/Structs/Structs_8.h"
#include "KeyMappings.generated.h"

class UPlayerInput;
enum class EKeyModifiers : uint8;


/**
 *	In KeyMappings::CreateDefaultActionInfos and AxisMappings::CreateDefaultAxisInfos you 
 *	can define default keys + more for actions.
 */


/*------------------------------------------------------------------------------------------------
	FIXME
	Issues in this file: 
	I ran into a crazy bug. The jist of it was that I could not declare function pointer variables 
	in my class otherwise the arrays would not work. That's right. Simply declaring the 
	function pointer variable and doing absolutally nothing with it will cause the array 
	to have .Num() of zero or if using regular C arrays the values will be different every 
	time and never the correct ones. But 
	if I simply removed the function pointer variables from the class then it would work.
	The implications of this bug is that I have to use FName to define the function to call, 
	and every input function in PC must be a UFUNCTION. 
	In PC::SetupInputComponent the functions are bound by name or function pointer. 
	I hope binding input to a UInputComponent 
	with FNames instead of raw function pointers doesn't have any effect on performance. 
	But it binds it as a dynamic delegate so I'm pretty sure it does.
------------------------------------------------------------------------------------------------*/


struct FKey;
class ARTSPlayerController;


enum class EMappingTarget : uint8
{
	PlayerOnly, 
	ObserverOnly, 
	PlayerAndObserver
};


/** 
 *	If you're adding a value for CreateDefaultActionInfos() then just add it. You don't need to 
 *	do anything else with the new enum value 
 */
UENUM()
enum class EKeyMappingAction : uint16
{
	None,
	 
	//-----------------------------------------
	//	Add new values below here
	//-----------------------------------------

	LMB, 
	RMB, 
	ZoomCameraIn, 
	ZoomCameraOut, 
	EnableCameraLookAround, 
	ResetCameraRotationToOriginal, 
	ResetCameraZoomToOriginal, 
	ResetCameraRotationAndZoomToOriginal, 
	OpenTeamChat, 
	OpenAllChat, 
	OpenPauseMenuSlashCancel, 
	InstaQuitGame, 
	ShowDevelopmentCheatWidget, 
	OpenCommanderSkillTree, 
	CloseCommanderSkillTree, 
	ToggleCommanderSkillTree, 
	AssignControlGroup0,
	AssignControlGroup1,
	AssignControlGroup2,
	AssignControlGroup3,
	AssignControlGroup4,
	AssignControlGroup5,
	AssignControlGroup6,
	AssignControlGroup7,
	AssignControlGroup8,
	AssignControlGroup9,
	SelectControlGroup0, 
	SelectControlGroup1,
	SelectControlGroup2,
	SelectControlGroup3,
	SelectControlGroup4,
	SelectControlGroup5,
	SelectControlGroup6,
	SelectControlGroup7,
	SelectControlGroup8,
	SelectControlGroup9, 

	//-----------------------------------------
	//	Add new values above here
	//-----------------------------------------

	z_ALWAYS_LAST_IN_ENUM    UMETA(Hidden)
};


UENUM()
enum class EKeyMappingAxis : uint16
{
	None,
	
	//-----------------------------------------
	//	Add new values below here
	//-----------------------------------------

	MouseMoveX,
	MouseMoveY, 
	MoveCameraLeft,
	MoveCameraRight,
	MoveCameraForward,
	MoveCameraBackward,

	//-----------------------------------------
	//	Add new values above here
	//-----------------------------------------

	z_ALWAYS_LAST_IN_ENUM    UMETA(Hidden)
};


struct FInputInfoBase
{
	FInputInfoBase();

	explicit FInputInfoBase(const FString & InDisplayName, EKeyMappingAction ActionEnum, EKeyMappingAxis AxisEnum, 
		const FKey & InDefaultKey, EKeyModifiers KeyModifierFlags,
		EMappingTarget WhoMappingIsFor, bool bShouldExecuteWhenPaused, bool bIsAllowedToBeRemapped, 
		bool bInIsEditorOnly);

	/* The name to appear in game's UI */
	FText DisplayName;

	/**
	 *	The name to appear in the project settings. By default the ctor will use the display name.
	 */
	FName ActionName;

	/**
	 *	The default key for this action. This is the key it will be assigned to the first time
	 *	the player starts the game.
	 *	Can use EKeys::Invalid to signal it doesn't have a key bound to it by default.
	 */
	FKeyWithModifiers KeyWithModifiers;

	/* What this action is in enum form. Will be "None" if this is for an axis mapping */
	EKeyMappingAction ActionAsEnum;

	/* Will be "None" if this is for an action mapping */
	EKeyMappingAxis AxisAsEnum;

	/* Whether the action needs to be executed when the game is paused */
	uint8 bExecuteWhenPaused : 1;

	/* Whether the action is even allowed to be remapped from the default value by the player */
	uint8 bCanBeRemapped : 1;

	/* Whether the action is for when WITH_EDITOR is true only */
	uint8 bIsEditorOnly : 1;

	/* Whether this action is for a player participating in the match */
	uint8 bIsForPlayer : 1;

	/* Whether this mapping is for a person observing a match */
	uint8 bIsForObserver : 1;

	/* Keys that are not allowed to be mapped to this action */
	//TSet<FKeyWithModifiers> DisallowedKeys;

	/* Whether the modifier key has to be pressed as well for the key event to happen */
	bool HasCTRLKeyModifier() const;
	bool HasALTKeyModifier() const;
	bool HasShiftKeyModifier() const;
};


#define PC_FUNC(x) GET_FUNCTION_NAME_CHECKED(ARTSPlayerController, x)

struct FInputActionInfo : public FInputInfoBase
{
	typedef void(ARTSPlayerController:: *RTSPlayerControllerFuncPtr)(void);
	
	FInputActionInfo() {}

	/** 
	 *	@param InDisplayName - name that will appear in game UI. Also the name that will appear in 
	 *	project settings in Key Mappings. 
	 *	@param InDefaultKey - the key that will be bound to this action by default. Use EKeys::Invalid 
	 *	to let it have no default key. 
	 *	@param KeyModifierFlags - whether a modifier key has to be held down while key is pressed
	 *	@param PressedAction - function to call when the key is pressed
	 *	@param ReleasedAction - the function to call when the button is released. Can be left null 
	 *	@param bShouldExecuteWhenPaused - whether the key event should register while the game is paused 
	 *	@param bIsAllowedToBeRemapped - whether the key is allowed to be remapped by the player 
	 */
	explicit FInputActionInfo(const FString & InDisplayName, EKeyMappingAction ActionEnum, const FKey & InDefaultKey, EKeyModifiers KeyModifierFlags,
		EMappingTarget WhoMappingIsFor, FName PressedAction, FName ReleasedAction = NAME_None,
		bool bShouldExecuteWhenPaused = false, bool bIsAllowedToBeRemapped = true, bool bIsEditorOnly = false);

	/* Name of function to call for input event */
	FName OnPressedAction;
	FName OnReleasedAction;

	bool HasActionForKeyPress() const;
	bool HasActionForKeyRelease() const;
};


class RTS_VER2_API KeyMappings
{
public:

	typedef void(ARTSPlayerController:: *RTSPlayerControllerFuncPtr)(void);
	static constexpr int32 NUM_ACTIONS = static_cast<int32>(EKeyMappingAction::z_ALWAYS_LAST_IN_ENUM) - 1;

private:
	static TArray<FInputActionInfo, TFixedAllocator<NUM_ACTIONS>> CreateDefaultActionInfos();

public:

	static void AddActionInfo(TArray<FInputActionInfo, TFixedAllocator<NUM_ACTIONS>> & OutArray, const FString & DisplayName,
		EKeyMappingAction KeyMappingAction, const FKey & Key, EKeyModifiers Modifiers, EMappingTarget WhoItIsFor,
		FName PressedAction, FName ReleasedAction = NAME_None,
		bool bShouldExecuteWhenPaused = false, bool bIsAllowedToBeRemapped = true,
		bool bIsEditorOnly = false);

	/* This takes a regular C array instead of a TArray */
	static void AddActionInfo(FInputActionInfo * OutArray, const FString & DisplayName,
		EKeyMappingAction KeyMappingAction, const FKey & Key, EKeyModifiers Modifiers, EMappingTarget WhoItIsFor,
		FName PressedAction, FName ReleasedAction = NAME_None,
		bool bShouldExecuteWhenPaused = false, bool bIsAllowedToBeRemapped = true,
		bool bIsEditorOnly = false);

	static int32 ActionTypeToArrayIndex(EKeyMappingAction KeyMappingAction);
	static EKeyMappingAction ArrayIndexToActionType(int32 ArrayIndex);

	//--------------------------------------------------------
	//	Data
	//--------------------------------------------------------

	/* This can actually be made const and TFixedAllocator<NUM_ACTIONS>. Same with 
	AxisInfos. In fact in order to stop users accidentially modifiying it it should */
	static const TArray<FInputActionInfo, TFixedAllocator<NUM_ACTIONS>> ActionInfos;
	//static FInputActionInfo ActionInfos[NUM_ACTIONS];

#if WITH_EDITOR
	static int32 NumArrayEntries;
#endif
};


struct FInputAxisInfo : public FInputInfoBase
{
	typedef void(ARTSPlayerController:: *RTSPlayerControllerAxisFuncPtr)(float);
	
	FInputAxisInfo() {}

	explicit FInputAxisInfo(const FString & InName, EKeyMappingAxis AxisEnum, const FKey & InKey, float InScale,
		EMappingTarget WhoMappingIsFor, FName InFunctionPtr, 
		bool bShouldExecuteWhenPaused = false, bool bIsAllowedToBeRemapped = true, bool bIsEditorOnly = false);
	
	/* Name of function to call for input event */
	FName FunctionPtr;

	float Scale;
};


class RTS_VER2_API AxisMappings
{
public:
	
	typedef void(ARTSPlayerController:: *RTSPlayerControllerAxisFuncPtr)(float);
	static constexpr int32 NUM_AXIS = static_cast<int32>(EKeyMappingAxis::z_ALWAYS_LAST_IN_ENUM) - 1;

private:
	static TArray<FInputAxisInfo, TFixedAllocator<NUM_AXIS>> CreateDefaultAxisInfos();

public:

	static void AddAxisInfo(TArray<FInputAxisInfo, TFixedAllocator<NUM_AXIS>> & OutArray, const FString & InName, EKeyMappingAxis AxisEnum, const FKey & InKey, float InScale,
		EMappingTarget WhoMappingIsFor, FName InFunctionPtr,
		bool bShouldExecuteWhenPaused = false, bool bIsAllowedToBeRemapped = true, 
		bool bIsEditorOnly = false);

	/* This takes a regular C array instead of TArray */
	static void AddAxisInfo(FInputAxisInfo * OutArray, const FString & InName, EKeyMappingAxis AxisEnum, const FKey & InKey, float InScale,
		EMappingTarget WhoMappingIsFor, FName InFunctionPtr,
		bool bShouldExecuteWhenPaused = false, bool bIsAllowedToBeRemapped = true,
		bool bIsEditorOnly = false);

	static int32 AxisTypeToArrayIndex(EKeyMappingAxis AxisEnum);
	static EKeyMappingAxis ArrayIndexToAxisType(int32 ArrayIndex);

	//--------------------------------------------------------
	//	Data
	//--------------------------------------------------------

	static const TArray<FInputAxisInfo, TFixedAllocator<NUM_AXIS>> AxisInfos;
	//static FInputAxisInfo AxisInfos[NUM_AXIS];

#if WITH_EDITOR
	static int32 NumArrayEntries;
#endif
};


/* Class for input in general (both action mappings and axis mappings) */
class RTS_VER2_API InputMappings
{
public:
};

