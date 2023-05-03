// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"

#include "Statics/CommonEnums.h"
#include "Statics/Structs/Structs_8.h"
#include "RTSGameUserSettings.generated.h"

class USoundClass;
class URTSGameInstance;
class ARTSPlayerController;
class APlayerState;
enum class EKeyMappingAction : uint16;
enum class EKeyMappingAxis : uint16;
struct FTryBindActionResult;


UENUM()
enum class EVariableType : uint8
{
	None, 

	Bool, 
	Float
};


UENUM()
enum class EDependencyRequirementType : uint8
{
	Bool, 
	
	//~ Floats
	Float_IsLessThanOrEqualTo, 
	Float_IsLessThan, 
	Float_IsEqualTo, 
	Float_IsGreaterThan, 
	Float_IsGreaterThanOrEqualTo
};


/* Struct to hold information about a dependency a setting has on another setting */
USTRUCT()
struct FControlSettingDependencyInfo
{
	GENERATED_BODY()

protected:

	EControlSettingType Dependency;

	/* What the dependency must be in relation to required value for it to be fullfilled */
	EDependencyRequirementType DependencyFullfilledOperator;

	/* Required value if dependency is a bool */
	bool bRequiredValue;

	/* Required value if dependency is a float */
	float RequiredValue;

public:

	// Never call this ctor
	FControlSettingDependencyInfo();

	/**
	 *	Ctor for when dependency is a bool 
	 *	@param InDependency - the other setting 
	 *	@param bInRequiredValue - value InDependency must be in order to be editable in UI
	 */
	explicit FControlSettingDependencyInfo(
		EControlSettingType InDependency,
		bool bInRequiredValue);

	/** 
	 *	Ctor for when dependency is a float
	 */
	explicit FControlSettingDependencyInfo(
		EControlSettingType InDependency,
		EDependencyRequirementType InDependencyFullfilledOperator,
		float InRequiredValue);
};


/* A struct that stores the default value for a control setting + its min and max and how much
to adjust it by by default */
USTRUCT()
struct FControlSettingInfo
{
	GENERATED_BODY()

	typedef void (ARTSPlayerController:: *ApplyControlSettingFuncPtr_Float)(float);
	typedef void (ARTSPlayerController:: *ApplyControlSettingFuncPtr_Bool)(bool);

protected:

	/* The name it display on the UI */
	FText DisplayName;

	/* Pointer to the variable this struct is for. 
	If *VariablePtr == -1 then it is the default value. However if *VariablePtr == DefaultValueStep 
	then that also means it equals the defaul value. Either is fine */
	int16 * VariablePtr;

	EVariableType VariableType;

	/* Function that applies the setting. 
	And as usual adding func ptrs to the struct causes bad things to happen. My project crashes at 
	79% while loading. Go ahead an try it. Uncomment these variables. They're not used 
	anywhere (well in my code, obviously the engine is doing something with this struct and 
	its variables while loading) and observe that the project crashes while loading.
	TODO sort this out 
	
	List of things that should change when this is fixed: 
	1. search ApplyFuncPtr_Float and ApplyFuncPtr_Bool in RTSGameUserSettings.cpp and uncomment 
	any lines I have commented with them. 
	2. ApplyControlSettings can get rid of the hardcode for applying and instead do it inside 
	the loop
	I think that's everything */
	//ApplyControlSettingFuncPtr_Float ApplyFuncPtr_Float;
	//ApplyControlSettingFuncPtr_Bool ApplyFuncPtr_Bool;

	/* Default value to use first time starting up game */
	UPROPERTY()
	float DefaultValue;

public:

	/* Index in Steps that is where the default value is.
	If Steps does not contain DefaultValue then DefaultValueStep will be an index in Steps 
	that is close to DefaultValue */
	int16 DefaultValueStep;

	/* Values that this variable can take on */
	TArray<float> Steps;

	/* Array of other settings that must be certain values for this setting to be editable 
	in UI */
	UPROPERTY()
	TArray<FControlSettingDependencyInfo> UIDependencies;

	//==========================================================================================
	//	Constructors
	//==========================================================================================

	// Never use this ctor 
	FControlSettingInfo();

	/** 
	 *	Only use this constructor
	 *	@param VariablePtr - reference to the variable that this is for
	 *	@param InDefaultValue - the default value to write to .ini file when starting game for the
	 *	first time
	 *	@param InUIDependencies - this array contains what other variables values must be in
	 *	order for this setting to be editable in the UI. This applies to UI only. Ensure
	 *	your func that sets the value also takes its dependencies into account if it needs to
	 */
	explicit FControlSettingInfo(const FString & InDisplayName, int16 * InVariablePtr, ApplyControlSettingFuncPtr_Float ApplyFunc, float InDefaultValue,
		const TArray<float> & InSteps = {}, const TArray<FControlSettingDependencyInfo> & InUIDependencies = TArray <FControlSettingDependencyInfo>());

	/**
	 *	Float constructor that takes only one dependency for readability
	 */
	explicit FControlSettingInfo(const FString & InDisplayName, int16 * InVariablePtr, ApplyControlSettingFuncPtr_Float ApplyFunc, float InDefaultValue,
		const FControlSettingDependencyInfo & UIDependency, const TArray<float> & InSteps = {});

	/* Ctors for bools */
	explicit FControlSettingInfo(const FString & InDisplayName, int16 * InVariablePtr, ApplyControlSettingFuncPtr_Bool ApplyFunc, bool InDefaultValue,
		const TArray <FControlSettingDependencyInfo> & InUIDependencies = TArray <FControlSettingDependencyInfo>());
	explicit FControlSettingInfo(const FString & InDisplayName, int16 * InVariablePtr, ApplyControlSettingFuncPtr_Bool ApplyFunc, bool InDefaultValue,
		const FControlSettingDependencyInfo & UIDependency);

	//==========================================================================================
	//	Getters and Setters
	//==========================================================================================

	const FText & GetDisplayName() const { return DisplayName; }

	EVariableType GetVariableType() const { return VariableType; }

	/**
	 *	Get the current value the variable is set at. Always returns a float even if the variable 
	 *	is a bool, bust casted to a bool the result should be correct
	 */
	float GetValue() const;

	/* Get what step the variable is currently at. Will never return -1 */
	int16 GetStep() const;
	
	/* Get the total number of different values this variable can take on -1 if the 
	default value isn't contained in Steps */
	int16 GetNumSteps() const;

	void SetValue(int16 NewStepsIndex) const;

	/** 
	 *	Adjust the current value by another value, making sure the final set value stays within 
	 *	the limits.
	 *	@param StepAdjustAmount - number of steps to try adjust by 
	 *	@return - actual value the variable was set to
	 */
	void AdjustValue(int16 StepAdjustAmount) const;

	void ClampValue() const;

	/**
	 *	Set variable to default value
	 *	@return - the new value the variable was set to
	 */
	void ResetToDefault();

	/**
	 *	Should work ok even if variable is a bool
	 */
	float GetDefaultValue() const;

	/* Does not write to disk */
	void ApplyValue(ARTSPlayerController * LocalPlayerController) const;
};


UENUM()
enum class EGameStartType : uint8
{
	HasNotStartedGameBefore, 
	HasStartedGameBefore, 
};


/**
 *	Custom game user settings for RTS. All settings in this class have the possibility to be
 *	modified at run time.
 *
 *	Class updates the GameUserSettings.ini file
 *
 *	Note: while testing with standalone stuff might not behave exactly how you would expect. 
 *	I think this is because perhaps in standalone the .ini file never gets written to. 
 *	Best test with PIE.
 *
 *	-------------------------------------------------------------------------------------------
 *	Some terminology:
 *	- apply settings means to use the values stored in the config variable on this class (not 
 *	the ones in the .ini file) and make them take effect e.g. the volume actually changes, 
 *	or the mouse sensitivity actually changes. Then they will be written to the .ini file.
 *	- to restore settings to default means to put default values into the config variables 
 *	on this class (again NOT the .ini file). Settings still need to be applied for them to 
 *	take effect. 
 *	- to revert changes would mean to load from the .ini file into the config variables in 
 *	this class. This will probably happen when I implement a settings menu and it asks if 
 *	you would like to keep the changes.
 *	-------------------------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------------------------
 *	Some notes about UGameUserSettings:
 *	- When launching standalone it seems to ignore the ini file
 *	- when launching PIE the .ini file is not loaded. Therefore changes to the .ini file 
 *	must be done before launching the editor for them to take effect. I could probably 
 *	call LoadSettings some time after PIE starts to load them. It's just a matter of whether 
 *	it causes PIE to take significantly longer to load.
 *----------------------------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------------------------
 *	Some notes about how this class has been implemented:
 *	- If you modify your .ini file and start the editor then the changes will not be applied. 
 *	You must change the defaults defined in C++ instead. However when the game is packaged 
 *	then values in the .ini file will be used. 
 *	You can actually use your .ini file values if you set bUseIniValues to true (must be 
 *	done before starting the editor).
 *	That variable will stay set to true until you set it back to false via your .ini file. 
 *	While the editor is open you can make changes to the .ini file, save it, and those 
 *	changes will take effect for the next PIE session. Note the game has to load the ini 
 *	file right before each PIE session so it's advisable to set it back to false once you're 
 *	done fidling with ini file values. And once you do your next play session will use defaults.
 *
 *	- In a packaged game you can set GameStartType to HasNotStartedGameBefore to cause all 
 *	settings to be reset to default.
 *
 *	- If you want to modify your key mappings then do it in GameUserSettings.ini in 
 *	ActionToKey and AxisToKey. Do not modify Input.ini. Changes made there will have 
 *	no effect. The same goes for modifying the action mappings and axis mappings in 
 *	project settings in editor.
 *----------------------------------------------------------------------------------------------
 *
 *	To add a new control setting via C++:
 *	1. Add a config variable declaration below like the other variables
 *	2. In CommonEnums.h under EControlSettingType add a new entry for your variable. Name is arbitrary
 *	3. In ctor add a ControlSettings.Emplace entry with the values you would like
 *	4. In ApplyControlSettings(AMyPlayerController *) add an entry that will update what you want.
 *	You will need to implement this function yourself. 
 *	5. Compile
 *
 *	If you just want to change the default value of a variable defined in this class you need
 *	to do it in the ctor Emplace corrisponding to it.
 */
UCLASS()
class RTS_VER2_API URTSGameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()

public:

	//==========================================================================================
	//	Variables Unmodifiable at Runtime
	//==========================================================================================

	/* The min and max amounts the camera can be zoomed */
	static const float MIN_CAMERA_ZOOM_AMOUNT;
	static const float MAX_CAMERA_ZOOM_AMOUNT;

	/* Min/max amount of pitch that can be applied to camera rotation */
	static const float MIN_CAMERA_PITCH;
	static const float MAX_CAMERA_PITCH;

	/* This will only come into effect when player wants to move the camera in a diagonal
	direction because mouse is in a corner of the screen. By default total movement will
	be same for diagonal (not sqrt(2) or 41% faster) but it just feels a little slow so use
	this multiplier to speed it up. */
	static const float MOUSE_DIAGONAL_MOVEMENT_SPEED_MULTIPLIER;

	/* Same as above except for keyboard WASD movement */
	static const float KEYBOARD_DIAGONAL_MOVEMENT_SPEED_MULTIPLIER;


	//==========================================================================================
	//	Some public functions
	//==========================================================================================

	URTSGameUserSettings(const FObjectInitializer & ObjectInitializer);

	/** Called after the C++ constructor and after the properties have been loaded from .ini file */
	virtual void PostInitProperties() override;

protected:

	void SetupControlSettingStepArrays();

	/* Whether to use the default values defined in C++ */
	bool UseCPlusPlusDefaults() const;

public:

	virtual void SetToDefaults() override;

	/* Basically will not check the branch in URTSGameUserSettings::SetToDefaults. */
	void SetToDefaultsNoFail();

	virtual void LoadSettings(bool bForceReload) override;

	/* Basically this will not check the branch in URTSGameUserSettings::LoadSettings 
	so it will always load settings */
	void LoadSettingsNoFail(bool bForceReload);

protected:

	void LoadSettingsInner(bool bForceReload);

public:

	/* If starting game for very first time then set all config properties to default values */
	void InitialSetup(URTSGameInstance * GameInstance);

	/* The UGAmeUserSettings::ApplySettings(bool) func must get called early on when engine
	is intializing and I cannot get a world context or even use a class GI variable set
	eariler. Anyway, this function will apply all settings and save them to disk */
	void ApplyAllSettings(URTSGameInstance * GameInstance, APlayerController * LocalPlayCon);

	/* Applies all current user settings to the game and saves to permanent storage (e.g. file) */
	virtual void ApplySettings(bool bCheckForCommandLineOverrides) override;

	//==========================================================================================
	//	Config variables not related to any setting
	//==========================================================================================

	/** Whether the game has been started before. 
	Can set this no to force default values to be applied next time starting game */
	UPROPERTY(Config)
	EGameStartType GameStartType; 

#if WITH_EDITORONLY_DATA
	/** 
	 *	While testing with editor the C++ defaults are always used. Set this to true to use 
	 *	what is in your .ini file instead. It will stay true until you change it back to false. 
	 *	While true you the game will reload your ini file every PIE session so make sure 
	 *	to set it back to false when you are done testing values.
	 */
	UPROPERTY(Config)
	bool bUseIniValues;
#endif

protected:

	//==========================================================================================
	//	Profile settings
	//==========================================================================================

protected:

	/* True if has opened profile page. If false then it will be shown before they try to
	create/join a networked game */
	UPROPERTY(Config)
	bool bHasSeenProfile;

	/* Nickname to use in singleplayer matches */
	UPROPERTY(Config)
	FString PlayerAlias;

	/* The default faction to be set to when entering a lobby */
	UPROPERTY(Config)
	EFaction DefaultFaction;

public:

	bool HasSeenProfile() const;
	const FString & GetPlayerAlias() const;
	EFaction GetDefaultFaction() const;

	void ChangeHasSeenProfile(bool bNewValue);
	void ChangePlayerAlias(const FString & NewName);
	void ChangeDefaultFaction(EFaction NewFaction);

	void ResetProfileSettingsToDefaults();

	void ApplyProfileSettings(APlayerState * LocalPlayerState);


	//==========================================================================================
	//	Match camera controls
	//==========================================================================================

protected:

	/* These are all integers. The integer represents the index in FControlSettingInfo::Steps 
	that we should use */

	/* Speed camera pans in match from keyboard input */
	UPROPERTY(Config)
	int16 CameraPanSpeed_Keyboard;

	/* Speed camera pans when moving mouse to the edge of the screen */
	UPROPERTY(Config)
	int16 CameraPanSpeed_Mouse;

	/* Max speed of camera. How this is different from CameraPanSpeeds... they are more
	acceleration properties kinda of. They're actually both kind of redundant, just use this
	instead */
	UPROPERTY(Config)
	int16 CameraMaxSpeed;

	/* How much acceleration to apply to movement input for both keyboard and mouse edge. Must
	be greater than 0 otherwise camera will not move */
	UPROPERTY(Config)
	int16 CameraAcceleration;

	/** 
	 *	Whether to enable camera movement lag or not via setting var on USpringArmComponent
	 */
	UPROPERTY(Config)
	int16 bEnableCameraMovementLag;

	/** 
	 *	How much camera should lag behind when you're moving it.  
	 *	Just setting USpringArmComponent::CameraLagSpeed. 
	 *	0 = no lag (instant), low values = more lag (slower). 
	 *	Will only be relevant if bEnableCameraMovementLag is true
	 */
	UPROPERTY(Config)
	int16 CameraMovementLagSpeed;

	/* Turning boost... just setting variables on the floating pawn movement component.
	This makes turns happen quicker */
	UPROPERTY(Config)
	int16 CameraTurningBoost;

	/* How fast camera stops. Larger = camera stops faster which is what you will want I think */
	UPROPERTY(Config)
	int16 CameraDeceleration;

	/* How close mouse must be to the edge of the screen for camera panning to happen.
	0 = no panning, 0.05 = 5% in from screen edge */
	UPROPERTY(Config)
	int16 CameraEdgeMovementThreshold;

	/* How much a scroll of the mouse wheel will change the zoom amount. Change CameraZoomSpeed
	to set how fast the change occurs */
	UPROPERTY(Config)
	int16 CameraZoomIncrementalAmount;

	/* Speed camera changes to target zoom when scrolling middle mouse button. 0 = instant,
	increasing = faster */
	UPROPERTY(Config)
	int16 CameraZoomSpeed;

	/* The sensitivity on X axis when looking around while the MMB is held down */
	UPROPERTY(Config)
	int16 MMBYawSensitivity;

	/* The sensitivity on Y axis when looking around while the MMB is held down */
	UPROPERTY(Config)
	int16 MMBPitchSensitivity;

	/* Whether to invert the MMB yaw look-around. */
	UPROPERTY(Config)
	int16 bInvertMMBYaw;

	/* Whether to invert the MMB pitch look-around. 1.f = true, 0.f = false */
	UPROPERTY(Config)
	int16 bInvertMMBPitch;

	/* When looking around with MMB pressed, whether the camera should lag behind input or
	update instantly. True = lag behind and less responsive, false = updates instantly */
	UPROPERTY(Config)
	int16 bEnableMMBLookLag;

	/* The amount the camera will lag behind when looking around with MMB pressed. Larger implies
	more responsive */
	UPROPERTY(Config)
	int16 MMBLookLagAmount;

	/* The camera pitch to start a match with and to return to when using the 'return to default
	pitch' button */
	UPROPERTY(Config)
	int16 DefaultCameraPitch;

	/* The default camera zoom amount that player will start match with and will be returned to
	when the 'return to default camera zoom' button is pressed. Note this has nothing to do with
	how much you are allowed to zoom in/out during a match - this is just for the default amount
	to start match at */
	UPROPERTY(Config)
	int16 DefaultCameraZoomAmount;

	/* The speed at which the camera resets to default rotation/zoom when requested.
	Larger = faster. 0 will not work */
	UPROPERTY(Config)
	int16 ResetCameraToDefaultSpeed;

	//==========================================================================================
	//	Other Control Settings
	//==========================================================================================

	/**
	 *	Relevant to key presses. Amount of time allowed for an 
	 *	input to be considered a double click. Time is measured from when key is released to 
	 *	when key is pressed again. 
	 *	0 can be used to disable double clicks altogether
	 */
	UPROPERTY(Config)
	int16 DoubleClickTime;

	/** 
	 *	A threshold for how far the mouse can move from the click location before it is no 
	 *	longer considered a click 
	 */
	UPROPERTY(Config)
	int16 MouseMovementThreshold;

	//==========================================================================================
	//	Ghost Building
	//==========================================================================================

	/**
	 *	When a ghost building is spawned and LMB is pressed: the amount of mouse movement required 
	 *	for the ghost to start being considered for rotation. 
	 *	Setting this as 0 means ghost can sometimes undesirably rotate when you click your LMB 
	 *	when you're trying to place it.
	 */
	UPROPERTY(Config)
	int16 GhostRotationRadius;

	/** 
	 *	How much ghost building rotates when LMB is pressed. Does not apply to the ghost rotation 
	 *	method "Snap"
	 */
	UPROPERTY(Config)
	int16 GhostRotationSpeed;


	//	End Config Variables
	//==========================================================================================

	/* Not a config property. Maps control settings to their info which includes a ref to the
	actual variable + many default values + max and min */
	UPROPERTY()
	TMap <EControlSettingType, FControlSettingInfo> ControlSettings;

public:

	const FControlSettingInfo & GetControlSettingInfo(EControlSettingType SettingType) const;

	float GetControlSettingValue(EControlSettingType SettingType) const;

	/* Resets all control settings to defaults. Does not reset key mappings */
	void ResetControlSettingsToDefaults();

	/* Remember to call this anytime player controller changes. BeginPlay would be a good place.
	Param is not casted to allow some classes to avoid including MyPlayerController.h */
	void ApplyControlSettings(APlayerController * InPlayerController);


	//==========================================================================================
	//	Audio settings
	//==========================================================================================

public:

	/* Min/max value volume can be set to. These are kind of irrelevant now that the 
	VOLUME_STEPS array is a thing */
	static const float VOLUME_MIN;
	static const float VOLUME_MAX; // I THINK engine will also clamp at 4.f

	/* The index in VOLUME_STEPS that is considered the default volume */
	static const int16 VOLUME_DEFAULT_STEP;

	static constexpr int16 NUM_VOLUME_STEPS = 41;

	/* Different volume levels. When the UI wants to adjust volume it goes up or down 
	a certain number of steps */
	static const float VOLUME_STEPS[NUM_VOLUME_STEPS];

protected:

	/* Maps sound class GetName() to volume step level. This TMap exists so volumes can be stored on
	persistent storage. Using FString as key because I assume a pointer will mean nothing when
	stored in .ini files */
	UPROPERTY(Config)
	TMap <FString, int16> SoundClassVolumes;

public:

	/* Get volume for a sound class */
	float GetVolume(const FString & SoundClassName) const;

	/* Get what step count a sound class is at */
	int16 GetStepCount(const FString & SoundClassName) const;

	/* Adjust the volume of a sound class, making sure it stays within limits. Will apply
	it instantly but does not write the change to disk */
	void SetVolume(URTSGameInstance * GameInst, const FString & SoundClassName, int16 NewVolumeStep);

	/* Adjust the volume of a sound class, making sure it stays within limits. Will apply 
	it instantly but does not write the change to disk */
	void AdjustVolume(URTSGameInstance * GameInst, const FString & SoundClassName, int16 StepAdjustAmount);

	/* Populate the container SoundClassVolumes with all sound classes */
	void PopulateSoundClassVolumes(URTSGameInstance * GameInst);

	/* Note: SoundClassVolumes must be populated with all sound classes for this 
	to work correctly */
	void ResetAudioSettingsToDefaults();

	/* Applies sound settings */
	void ApplySoundSettings(URTSGameInstance * GameInstance);


	//==========================================================================================
	//	------- Key Bindings -------
	//==========================================================================================

protected:

	void ResetKeyMappingsToDefault();

	void ApplyKeyMappingSettings(bool bSaveToDisk);

	//-------------------------------------------------
	//	Data
	//-------------------------------------------------

protected:

	/* Maps key action to the key assigned to it. Will change as the user changings their key bindings */
	UPROPERTY(Config)
	TMap<EKeyMappingAction, FKeyWithModifiers> ActionToKey;

	/* Maps a key to the action assigned to it. Will change as the user changings their key bindings */
	TMap<FKeyWithModifiers, EKeyMappingAction> KeyToAction;

	/* Maps axis action to the key assigned to it. Will change as the user changings their key bindings */
	UPROPERTY(Config)
	TMap<EKeyMappingAxis, FKey> AxisToKey;

	/* Maps key to the axis action assigned to it. Will change as the user changings their key bindings */
	TMap<FKey, EKeyMappingAxis> KeyToAxis;

public:

	/** 
	 *	Changes a key mapping. Will unbind other actions if required if bForce is true. 
	 *	It becomes essentially applied instantly but does not write to disk, 
	 *	although after testing it it appears it is written to disk both the GameUserSettings.ini 
	 *	and the Input.ini.  
	 *	
	 *	@param ActionToBind - the action to receive a key binding 
	 *	@param Key - the key to bind to ActiontoBind
	 *	@param bForce - if true then actions will become unbound to bind this action if necessary. 
	 *	If an action that you specify as unremappable would become unbound though it will 
	 *	not happen and this func will return false
	 *	@param OutResult - will contain entries to what was remapped. If they are all None then 
	 *	nothing was remapped.
	 *	@return - true if successful. This can return false if the remap would unbind an action 
	 *	that isn't allowed to be unbinded 
	 */
	bool RemapKeyBinding(EKeyMappingAction ActionToBind, const FKeyWithModifiers & Key, bool bForce, FTryBindActionResult & OutResult);
	bool RemapKeyBinding(EKeyMappingAxis ActionToBind, const FKey & Key, bool bForce, FTryBindActionResult & OutResult);

	//-------------------------------------------------------
	//	Trivial getters and setters
	//-------------------------------------------------------

	/* Get the key assigned to a game action. Is this the applied key? Not necessarily. 
	Just the value stored in this class at the momenet */
	const FKeyWithModifiers & GetKey(EKeyMappingAction GameAction) const { return ActionToKey[GameAction]; }
	FKeyWithModifiers GetKey(EKeyMappingAxis GameAction) const { return FKeyWithModifiers(AxisToKey[GameAction]); }

	/* Returns the action mapping that is bound to a key, or None if no action mapping is bound 
	to the key */
	EKeyMappingAction GetBoundAction(const FKeyWithModifiers & Key) const;

	/* Will return an action bound to the key ignoring modifiers. e.g. if an action is bound 
	to CTRL + K and param Key == K then this can return that action. 
	Will return None if no action is bound to that key */
	EKeyMappingAction GetBoundActionIgnoringModifiers(const FKey & Key) const;

	/* Returns the axis mapping that is bound to a key, or None id no axis mapping is bound to 
	the key */
	EKeyMappingAxis GetBoundAxis(const FKeyWithModifiers & Key) const;
	EKeyMappingAxis GetBoundAxis(const FKey & Key) const;


	//=========================================================================================
	//	------- Some video settings stuff -------
	//=========================================================================================

protected:

	void ResetCustomVideoSettingsToDefaults();

public:

	/* Tbh I think there's a virtual ApplyNonResolutionSettings() which could be better than 
	using this */
	void ApplyCustomVideoSettings(UWorld * World);

	virtual int32 GetOverallScalabilityLevel() const override;
	virtual void SetOverallScalabilityLevel(int32 Value) override;

	/** 
	 *	Get the type of anti-aliasing that is applied e.g. FXAA, TAA, none
	 */
	EAntiAliasingMethod GetAntiAliasingMethod() const;

	/* Set the anti-aliasing method */
	void SetAntiAliasingMethod(UWorld * World, EAntiAliasingMethod Method);

	//-----------------------------------------------------------------------------
	//	Variables
	//-----------------------------------------------------------------------------

	/* Type of anti-aliasing e.g. FXAA, TAA */
	UPROPERTY(Config)
	TEnumAsByte<EAntiAliasingMethod> AntiAliasingMethod;
};
