// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Statics/CommonEnums.h"
#include "Statics/OtherEnums.h"
#include "Statics/Structs_4.h"
#include "Statics/Structs_5.h"
#include "Structs_2.generated.h"

class AActor;
class AProjectileBase;
class URTSDamageType;
class UCameraShake;
class ACPUPlayerAIController;
class ARTSPlayerState;
class ARTSLevelVolume;
class ARTSPlayerStart;
struct FPlayerStartInfo;
class ISelectable;
struct FTickableBuffOrDebuffInstanceInfo;
struct FStaticBuffOrDebuffInstanceInfo;
class AInfantryController;
struct FUnitInfo;
class USoundBase;
struct FTickableBuffOrDebuffInfo;
struct FStaticBuffOrDebuffInfo;
struct FContextButtonInfo;
class UParticleSystemComponent;


//=============================================================================================
//	File that holds lots of structs
//=============================================================================================


/* Image + sound cue for a game notification */
USTRUCT()
struct FGameNotificationInfo
{
	GENERATED_BODY()

	/* Message to show on the HUD */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FText Message;

	/* UI sound to play */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	USoundBase * Sound;

public:

	FGameNotificationInfo()
		: Message(FText())
		, Sound(nullptr)
	{
	}

	explicit FGameNotificationInfo(const FText & InMessage)
		: Message(InMessage)
		, Sound(nullptr)
	{
	}

	const FText & GetMessage() const { return Message; }
	USoundBase * GetSound() const { return Sound; }
	void SetMessage(const FText & InMessage) { Message = InMessage; }
	void SetSound(USoundBase * InSound) { Sound = InSound; }
};


/* Image + sound cue for a game warning */
USTRUCT()
struct FGameWarningInfo
{
	GENERATED_BODY()

	/* Message to show on the HUD */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FText Message;

	/* UI sound to play */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	USoundBase * Sound;

public:

	FGameWarningInfo()
		: Message(FText())
		, Sound(nullptr)
	{
	}

	explicit FGameWarningInfo(const FText & InMessage)
		: Message(InMessage)
		, Sound(nullptr)
	{
	}

	const FText & GetMessage() const { return Message; }
	USoundBase * GetSound() const { return Sound; }
	void SetMessage(const FText & InMessage) { Message = InMessage; }
	void SetSound(USoundBase * InSound) { Sound = InSound; }
};


/* Holds info about how the image to draw overtop of a button when it is being hovered/pressed */
USTRUCT()
struct FUnifiedMouseFocusImage
{
	GENERATED_BODY()

protected:

	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	bool bEnabled;

	/* My notes on FSlateBrush: could probably create my own struct for images. It could do 
	away with some of the extras on FSlateBrush that I don't need */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bEnabled))
	FSlateBrush Brush;

public:

	FUnifiedMouseFocusImage();

	bool UsingImage() const { return bEnabled; }
	const FSlateBrush & GetBrush() const { return Brush; }
};


/* Info about a sound */
USTRUCT()
struct FUnifiedUIButtonSound
{
	GENERATED_BODY()

protected:

	/* Whether to use the sound for every button type this struct is for. */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	bool bEnabled;

	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bEnabled))
	USoundBase * Sound;

public:

	FUnifiedUIButtonSound()
		: bEnabled(false)
		, Sound(nullptr)
	{
	}

	bool UsingSound() const { return bEnabled; }
	USoundBase * GetSound() const { return Sound; }
};


/* Contains images and sounds */
USTRUCT()
struct FUnifiedImageAndSoundAssets 
{
	GENERATED_BODY()

public:

	const FUnifiedMouseFocusImage & GetUnifiedMouseHoverImage() const { return UnifiedMouseHoverImage; }
	const FUnifiedMouseFocusImage & GetUnifiedMousePressImage() const { return UnifiedMousePressImage; }
	const FUnifiedUIButtonSound & GetUnifiedMouseHoverSound() const { return UnifiedMouseHoverSound; }
	const FUnifiedUIButtonSound & GetUnifiedPressedByLMBSound() const { return UnifiedPressedByLMBSound; }
	const FUnifiedUIButtonSound & GetUnifiedPressedByRMBSound() const { return UnifiedPressedByRMBSound; }

protected:

	UPROPERTY(EditDefaultsOnly, Category = "RTS|HUD")
	FUnifiedMouseFocusImage UnifiedMouseHoverImage;
	UPROPERTY(EditDefaultsOnly, Category = "RTS|HUD")
	FUnifiedMouseFocusImage UnifiedMousePressImage;
	UPROPERTY(EditDefaultsOnly, Category = "RTS|HUD")
	FUnifiedUIButtonSound UnifiedMouseHoverSound;
	UPROPERTY(EditDefaultsOnly, Category = "RTS|HUD")
	FUnifiedUIButtonSound UnifiedPressedByLMBSound;
	UPROPERTY(EditDefaultsOnly, Category = "RTS|HUD")
	FUnifiedUIButtonSound UnifiedPressedByRMBSound;
};


/* Holds a TMap that maps each unit to whether it is currently
visible or hidden. */
USTRUCT()
struct FVisibilityInfo
{
	GENERATED_BODY()

	UPROPERTY()
	TMap <AActor *, bool> Actors;

	/* Add to map and set visibility info false */
	void AddToMap(AActor * Actor);

	void RemoveFromMap(AActor * Actor);

	/* Returns true if an actor is currently visible */
	bool IsVisible(const AActor * Actor) const;

	void SetVisibility(const AActor * Actor, bool bNewVisibility);
};


/* Holds array of uint64. Workaround for non multidimension TArrays */
USTRUCT()
struct FUint64Array
{
	GENERATED_BODY()

protected:

	UPROPERTY()
	TArray <uint64> Array;

public:

	void AddToArray(uint64 Value)
	{
		Array.AddUnique(Value);
	}

	/* Getters and setters */

	const TArray < uint64 > & GetArray() const { return Array; }
};


/* Attributes common to anything that attacks */
USTRUCT()
struct FAttackAttributes
{
	GENERATED_BODY()

protected:

	/* Projectile blueprint */
	UPROPERTY(EditDefaultsOnly, NoClear, Category = "RTS")
	TSubclassOf <AProjectileBase> Projectile_BP;

	/**
	 *	Whether to override the damage attributes of the projectile class. 
	 *
	 *	Useful if you want to reuse the same projectile for visual purposes but just want different
	 *	damage values, or if you like having your damage values defined on your units instead of on 
	 *	your projectiles. Of course you can always create a seperate projectile and give it seperate 
	 *	damage values and never have to use this
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	bool bOverrideProjectileDamageValues;

	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ShowOnlyInnerProperties, EditCondition = bOverrideProjectileDamageValues))
	FBasicDamageInfo DamageInfo;

	/**
	 *	Amount of time between point when attack animation fires projectile to the time when the
	 *	attack animation can start again. 
	 *
	 *	Lower = attack faster
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = "0.05"))
	float AttackRate;

	/* Attack range. Ignores Z axis - all range calculations are 2D */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = "0.0"))
	float AttackRange;

	/* Whether we can attack air units */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	bool bCanAttackAir;

	/* The types of selectables we are allowed to attack */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TSet <ETargetingType> AcceptableTargetTypes;

	/* Acceptable target types as FNames */
	UPROPERTY()
	TSet <FName> AcceptableTargetFNames;

	/* Name of muzzle socket on mesh. The location projectile spawns at */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FName MuzzleSocket;

	/* Particle effect to play at muzzle on attack. Optional */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	UParticleSystem * MuzzleParticles;

	/* Camera shake to play at muzzle location when firing. Optional */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TSubclassOf <UCameraShake> MuzzleCameraShake_BP;

	/* Radius of MuzzleCameraShake if MuzzleCameraShake_BP is set */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bCanEditCameraShakeOptions))
	float MuzzleShakeRadius;

	/* Falloff of MuzzleCameraShake if MuzzleCameraShake_BP is set */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bCanEditCameraShakeOptions))
	float MuzzleShakeFalloff;

	/** 
	 *	Sound to play when the unit is 'preparing' its attack e.g. oblisk of light.
	 *	To play this sound add the anim notify 'PlayAttackPreparationSound' to anim montage.
	 *	This sound is stopped if the attack animation stops. 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	USoundBase * PreparationSound;

	/* Sound to play when attack is made */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	USoundBase * AttackMadeSound;

	//========================================================================================
	//	HUD Update Functions
	//========================================================================================

	void TellHUDAboutChange_ImpactDamage(ISelectable * Owner);
	void TellHUDAboutChange_ImpactDamageType(ISelectable * Owner);
	void TellHUDAboutChange_AttackRate(ISelectable * Owner);
	void TellHUDAboutChange_AttackRange(ISelectable * Owner);

	//========================================================================================
	//		-------- Temporary modifiers --------
	//----------------------------------------------------------------------------------------
	//	These only exist because of floating point number drift 
	//----------------------------------------------------------------------------------------

	int16 NumTempImpactDamageModifiers;

	/* What our damage is without any buffs/debuffs */
	float DefaultImpactDamage;

	int16 NumTempAoEDamageModifiers;

	/* What AoE damage is without any buffs/debuffs */
	float DefaultAoEDamage;

	int16 NumTempAttackRateModifiers;

	/* What our attack rate is without any temporary buffs/debuffs */
	float DefaultAttackRate;

	int16 NumTempAttackRangeModifiers;

	/* What our attack range is without any temporary buffs/debuffs */
	float DefaultAttackRange;

public:

	FAttackAttributes();

	/* This function was added for artillery strike ability */
	void SetValues(bool bInOverrideProjectileDamageValues, float InImpactDamage,
		TSubclassOf<URTSDamageType> InImpactDamageType, float InImpactRandomDamageFactor,
		float InAoEDamage, TSubclassOf < URTSDamageType > InAoEDamageType, float InAoERandomDamageFactor,
		TSubclassOf<AProjectileBase> InProjectileBP);

	void SetValues(float InImpactDamage, TSubclassOf < URTSDamageType > InImpactDamageType, float InImpactRandomDamageFactor, 
		float InAoEDamage, TSubclassOf < URTSDamageType > InAoEDamageType, float InAoERandomDamageFactor);

	/* Given some unit info set what the damage values should be on this struct. This will set 
	the values of this struct without any upgrades considered */
	void SetDamageValues(const FUnitInfo * UnitInfo);

	/* Static to prevent creating temporaries */
	static FAttackAttributes Default;

	const TSet < FName > & GetAcceptableTargetTypes() const;
	UParticleSystem * GetMuzzleParticles() const;
	TSubclassOf<AProjectileBase> GetProjectileBP() const;
	const FBasicDamageInfo & GetDamageInfo() const { return DamageInfo; }
	bool ShouldOverrideProjectileDamageValues() const;
	const FName & GetMuzzleSocket() const;
	TSubclassOf <UCameraShake> GetMuzzleCameraShakeBP() const;
	float GetMuzzleShakeRadius() const;
	float GetMuzzleShakeFalloff() const;
	USoundBase * GetPreparationSound() const;
	USoundBase * GetAttackMadeSound() const;

	float GetImpactDamage() const;

	/* Get how much damage we deal when no temporary effects like buffs/debuffs are applied */
	float GetDefaultImpactDamage() const;

	/** 
	 *	Functions to change how much damage we deal. Suitable to call for permanent type effects 
	 *	such as upgrades or level up bonuses 
	 */
	void SetImpactDamage(float NewDamage, ISelectable * Owner);
	void SetImpactDamageViaMultiplier(float Multiplier, ISelectable * Owner);
	
	/** 
	 *	Functions to change how much damage we deal. Suitable to call for temporary effects such 
	 *	as buffs/debuffs and inventory items
	 */
	float ApplyTempImpactDamageModifierViaMultiplier(float Multiplier, ISelectable * Owner);
	float RemoveTempImpactDamageModifierViaMultiplier(float Multiplier, ISelectable * Owner);

	const TSubclassOf<URTSDamageType> & GetImpactDamageType() const;
	
	/** 
	 *	Set what the new damage type is. Suitable to call for permanent type effects like 
	 *	upgrades or level up bonuses 
	 */
	void SetImpactDamageType(TSubclassOf <URTSDamageType> & NewDamageType, ISelectable * Owner);

	float GetImpactRandomDamageFactor() const;

	float GetAoEDamage() const;
	const TSubclassOf<URTSDamageType> & GetAoEDamageType() const;
	float GetAoERandomDamageFactor() const;

	// TODO the AoE damage adjustment funcs which are the same as the impact ones except they're for AoE.
	// Also perhaps do a funcs that modify both impact and AoE damage. In my abilities I use 
	// the impact adjusting funcs when really I probably want to do both impact and AoE so 
	// I should change them

	bool CanAttackAir() const;

	/** 
	 *	Set whether we can attack air selectables. Suitable to call for permanent type effects like 
	 *	upgrades or level up bonuses 
	 */
	void SetCanAttackAir(bool bNewValue);

	float GetAttackRate() const;

	/**
	 *	Get the attack rate not taking into account any temporary effects like buffs/debuffs, 
	 *	so not necessarily our current attack rate
	 */
	float GetDefaultAttackRate() const;
	
	/** 
	 *	Set what the default attack rate is. Also updates current attack rate based on 
	 *	percentage 
	 *
	 *	@param Owner - the selectable these attributes belong to
	 */
	void SetAttackRate(float NewAttackRate, ISelectable * Owner);
	
	/** 
	 *	Set what the default attack rate is by multiplying it by a multiplier. Also updates 
	 *	current attack rate 
	 */
	void SetAttackRateViaMultiplier(float Multiplier, ISelectable * Owner);

	/**
	 *	Multiply the attack rate by a multiplier 
	 *	
	 *	@return - attack rate after change 
	 */
	float ApplyTempAttackRateModifierViaMultiplier(float Multiplier, ISelectable * Owner);
	
	/**
	 *	Multiply the attack rate by a multiplier. Expects Multiplier to be in (1.f / Num) form 
	 *	where Num is what you passed into ApplyTempAttackRateModifierViaMultiplier
	 *
	 *	@return - attack rate after change
	 */
	float RemoveTempAttackRateModifierViaMultiplier(float Multiplier, ISelectable * Owner);

	float GetAttackRange() const;

	/** 
	 *	Get default attack range. Not necessarily the current attack range. Takes into account
	 *	upgrades 
	 */
	float GetDefaultAttackRange() const;
	
	/** 
	 *	Set what the default attack range should be. The current arrack range will also be 
	 *	changed based on percentage 
	 */
	void SetAttackRange(float NewAttackRange, AInfantryController * AIController, ISelectable * Owner);

	/* Set Set what the default attack range should be by multiplying it by a multiplier */
	void SetAttackRangeViaMultiplier(float Multiplier, AInfantryController * AIController, ISelectable * Owner);

	/**
	 *	Multiply the attack range by a multiplier 
	 *	
	 *	@return - attack range after change 
	 */
	float ApplyTempAttackRangeModifierViaMultiplier(float Multiplier, AInfantryController * AIController, ISelectable * Owner);
	
	/**
	 *	Multiply the attack range by a multiplier. Expects Multiplier to be in (1.f / Num) form
	 *	where Num is what you passed into ApplyTempAttackRangeModifierViaMultiplier
	 *
	 *	@return - attack range after change
	 */
	float RemoveTempAttackRangeModifierViaMultiplier(float Multiplier, AInfantryController * AIController, ISelectable * Owner);

protected:

#if WITH_EDITORONLY_DATA
	/* Bool to control editability of camera shake options */
	UPROPERTY()
	bool bCanEditCameraShakeOptions;
#endif

public:

#if WITH_EDITOR
	void OnPostEdit();
#endif
};


/* TMap that maps armour type to its damage multiplier */
USTRUCT()
struct FDamageMultipliers
{
	GENERATED_BODY()

	/* Multiplier for how much damage this damage type will do against
	this armour type before any upgrades are considered. */
	UPROPERTY(EditDefaultsOnly)
	TMap < EArmourType, float > Multipliers;

	FDamageMultipliers();

	/* Getters and setters */

	float GetMultiplier(EArmourType ArmourType) const;
};


/* Get info about a decal that appears under a selectable when it is selected or hovered over
or inside marquee box */
USTRUCT()
struct FSelectionDecalInfo
{
	GENERATED_BODY()

protected:

	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	UMaterialInterface * Decal;

	/* Optional scalar parameter in the material to adjust when either selected or hovered over */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FName ParamName;

	/* Whether the param name is valid or not to know whether to try changing value or not.
	Actually not needed, thought game crashed is param name was not valid and try to set it */
	UPROPERTY()
	bool bIsParamNameValid;

	/* The value to change this param value to when the selectable is hovered over with mouse or
	inside marquee selection box */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	float MouseoverParamValue;

	/* The orginal value of the param so it can be restored when selectable is selected */
	UPROPERTY()
	float OriginalParamValue;

	/* Could be a good idea to check if (MouseoverParamValue == OriginalParamValue). If so then 
	we don't actually need to create a material instance dynamic which could save some performance */

public:

	FSelectionDecalInfo();

	UMaterialInterface * GetDecal() const;
	const FName & GetParamName() const;
	bool IsParamNameValid() const;
	void SetIsParamNameValid(bool bNewValue);
	float GetMouseoverParamValue() const;
	float GetOriginalParamValue() const;
	void SetOriginalParamValue(float InValue);

	/** 
	 *	Return whether we need to create a material instance dynamic in order to show what this 
	 *	decal wants to show. If false then we can get by without creating one. 
	 */
	bool RequiresCreatingMaterialInstanceDynamic() const;
};


/* Info about defeat conditions */
USTRUCT()
struct FDefeatConditionInfo
{
	GENERATED_BODY()

protected:

	/* Name to display in UI */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FText Name;

public:

	const FText & GetName() const { return Name; }
};


/* CPU difficulty is mapped to one of these. Static information about a CPU player */
USTRUCT()
struct FCPUDifficultyInfo
{
	GENERATED_BODY()

protected:

	/**
	 *	The name to display for this difficulty
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FString Name;

	/**
	 *	The icon to display in lobby
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	UTexture2D * Icon;

	/**
	 *	AI controller to spawn for this difficulty
	 */
	UPROPERTY(EditDefaultsOnly, NoClear, Category = "RTS", meta = (DisplayName = "AI Controller Class"))
	TSubclassOf < ACPUPlayerAIController > AIController_BP;

public:

	FCPUDifficultyInfo();
	FCPUDifficultyInfo(const FString & InName);

	const FString & GetName() const;
	UTexture2D * GetIcon() const;
	TSubclassOf < ACPUPlayerAIController > GetControllerBP() const;
};


/* Holds a uint8 and a float */
USTRUCT()
struct FPlayerStartDistanceInfo
{
	GENERATED_BODY()

protected:

	UPROPERTY()
	uint8 PlayerStartID;

	UPROPERTY()
	float Distance;

public:

	FPlayerStartDistanceInfo();
	FPlayerStartDistanceInfo(uint8 InID, float InDistance);

	// Get the unique player start ID
	uint8 GetID() const;
	float GetDistance() const;

	/**
	 *	Returns to distance between two player starts
	 */
	static float GetDistanceBetweenSpots(const FPlayerStartInfo & Info1, const FPlayerStartInfo & Info2);

	// Operator for sorting in array
	friend bool operator<(const FPlayerStartDistanceInfo & Info1, const FPlayerStartDistanceInfo & Info2);
};


/* Info about a player start */
USTRUCT()
struct FPlayerStartInfo
{
	GENERATED_BODY()

protected:

	UPROPERTY(EditAnywhere)
	uint8 UniqueID;
	
	/* World location on map */
	UPROPERTY(EditAnywhere)
	FVector Location;

	/* World rotation on map */
	UPROPERTY(EditAnywhere)
	FRotator Rotation;

	/* Sorted array of all other player starts in map. Sorted by distance with shortest 
	distance at lower indices. 
	
	Not correct until I remove my hardcoding method for doing player starts */
	UPROPERTY()
	TArray < FPlayerStartDistanceInfo > NearbyPlayerStarts;

public:

	FPlayerStartInfo();
	FPlayerStartInfo(ARTSPlayerStart * PlayerStart);

	/* A ctor for RTS level volume workaround TODO remove eventually */
	FPlayerStartInfo(uint8 InUniqueID, const FVector & InLoc, const FRotator & InRot);

	uint8 GetUniqueID() const { return UniqueID; }
	const FVector & GetLocation() const { return Location; }
	const FRotator & GetRotation() const { return Rotation; }
	
	//------------------------------------------------------------------
	// NearbyPlayerStarts functions
	//------------------------------------------------------------------
	
	// Empty the array
	void EmptyNearbyPlayerStarts();

	void AddNearbyPlayerStart(const FPlayerStartDistanceInfo & OthersInfo);
	
	// Sort the array
	void SortNearbyPlayerStarts();
	
	// Get const access to the array. Array should already be sorted before calling this
	const TArray < FPlayerStartDistanceInfo > & GetNearbyPlayerStartsSorted() const;
};


/* Info about a map */
USTRUCT()
struct FMapInfo
{
	GENERATED_BODY()

protected:

	/* Name of map to appear in UI */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FText DisplayName;

	/* The name that should be passed into UGameplayStatics::OpenLevel */
	UPROPERTY()
	FName MapFName;

	/* To reduce bandwidth */
	UPROPERTY()
	uint8 UniqueID;

	/** 
	 *	Image to appear in UI. Can be left blank in which case an image generated by the level 
	 *	volume will be used
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	UTexture2D * Image;

	/* Description of map */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FText Description;

	/** Maximum amount of players allowed for map. Not actually enforced anywhere */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 2))
	int32 MaxPlayers;

	/* The bounds of the level volume. This is used later to get the correct screen 
	location of player starts placed overtop of map image. 
	Note I don't think this is actually axis aligned. I'm not even 100% sure what axis 
	aligned means but FBoxSphereBounds's description says it's axis aligned even though 
	I have not done that */
	UPROPERTY()
	FBoxSphereBounds LevelVolumeBounds;

	UPROPERTY()
	float LevelVolumeBoundsYawRotation;

	/* The texture to use as the minimap for this map. This will be null until map is actually
	loaded meaning it will not be able to be referenced in lobby */
	UPROPERTY()
	UTexture2D * MinimapTexture;

	/* Array of each player starts transform */
	UPROPERTY()
	TArray < FPlayerStartInfo > PlayerStartPoints;

	/** 
	 *	This must be set by user. This is a workaround for not being able to modify the game 
	 *	instance blueprint while editing the level volume. Eventually I would like to get rid 
	 *	of this variable. Contains all the data about the level such as its bounds, player 
	 *	start locations and minimap texture.
	 */
	UPROPERTY(EditDefaultsOnly, NoClear, Category = "RTS")
	TSubclassOf < ARTSLevelVolume > LevelVolume_BP;

public:

	FMapInfo()
	{
		Image = nullptr;
		MaxPlayers = 2;
	}

	/** 
	 *	Given a player start ID get the start transform for the player 
	 *	@param InPlayerStartID - player start ID player was assigned. -2 = unassigned. 
	 *	-1 = error
	 *	@param OutLocation - players initial location
	 *	@param OutRotation - players initial rotation
	 */
	void GetPlayerStartTransform(int16 InPlayerStartID, FVector & OutLocation, 
		FRotator & OutRotation) const;

	//~ Getters and setters
	const FText & GetDisplayName() const { return DisplayName; }
	void SetMapFName(const FName & InFName) { MapFName = InFName; }
	const FName & GetMapFName() const { return MapFName; }
	void SetUniqueID(uint8 NewID) { UniqueID = NewID; }
	uint8 GetUniqueID() const { return UniqueID; }
	UTexture2D * GetUserDefinedImage() const { return Image; }
	const FText & GetDescription() const { return Description; }
	int32 GetMaxPlayers() const { return MaxPlayers; }
	const FBoxSphereBounds & GetMapBounds() const { return LevelVolumeBounds; }
	void SetMapBounds(const FBoxSphereBounds & InBounds, float InYawRotation) 
	{ 
		LevelVolumeBounds = InBounds; 
		LevelVolumeBoundsYawRotation = InYawRotation;

		/* You might wanna axis align the LevelVolumeBounds now so it saves you having to 
		do it all the time in the future. So calls to GetMapBounds might need to be looked 
		at and those that try to axis align it don't have to anymore */
	}
	float GetMapBoundsYawRotation() const { return LevelVolumeBoundsYawRotation; }
	UTexture2D * GetMinimapTexture() const { return MinimapTexture; }
	void SetMinimapTexture(UTexture2D * InTexture) { MinimapTexture = InTexture; }
	const TArray <FPlayerStartInfo> & GetPlayerStarts() const { return PlayerStartPoints; }
	void SetPlayerStartPoints(const TArray <FPlayerStartInfo> & Array) { PlayerStartPoints = Array; }
	TSubclassOf < ARTSLevelVolume > GetLevelVolumeBP() const { return LevelVolume_BP; }
};


/* Info about a resource like minerals or vespene gas in SCII */
USTRUCT()
struct FResourceInfo
{
	GENERATED_BODY()

protected:

	/* Image to appear on HUD. Can be overridden on a per-faction basis */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	UTexture2D * Icon;

public:

	FResourceInfo()
	{
		Icon = nullptr;
	}

	UTexture2D * GetIcon() const { return Icon; }
};


/* Holds amounts of resources to start match with */
USTRUCT()
struct FStartingResourceConfig
{
	GENERATED_BODY()

protected:

	/* The name to appear in UI */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FText Name;

	/* Maps resource type to amount to start match with */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TMap < EResourceType, int32 > Amounts;

public:

	FStartingResourceConfig();

#if WITH_EDITOR
	// Ctor for creating one from the development settings
	FStartingResourceConfig(const TMap < EResourceType, int32 > & InResourceMap);

	FString ToString() const;
#endif

	/* Get name to appear on UI */
	const FText & GetName() const;

	/* Get the starting amounts as a resource array that player state uses */
	const TArray < int32 > GetResourceArraySlow() const;
};


/* Information about a player for starting a match such as team, faction etc */
USTRUCT()
struct FPlayerInfo
{
	GENERATED_BODY()

	/* Either human player or CPU */
	UPROPERTY()
	ELobbySlotStatus PlayerType;

	/* Only valid for human players. Will be null for CPU players */
	UPROPERTY()
	ARTSPlayerState * PlayerState;

	/* CPU difficulty if a CPU player. Irrelevant if CPU player in this slot */
	UPROPERTY()
	ECPUDifficulty CPUDifficulty;

	/* If CPU player, their player state once it has been spawned */
	UPROPERTY()
	ARTSPlayerState * CPUPlayerState;

	UPROPERTY()
	ETeam Team;

	UPROPERTY()
	EFaction Faction;

	// -1 or -2 means unassigned
	UPROPERTY()
	int16 StartingSpotID;

	/** 
	 *	Human player constructor 
	 */
	FPlayerInfo(ARTSPlayerState * InPlayerState, ETeam InTeam, EFaction InFaction, 
		int16 InStartingSpot);

	/** 
	 *	CPU player constructor 
	 */
	FPlayerInfo(ECPUDifficulty InCPUDifficulty, ETeam InTeam, EFaction InFaction, 
		int16 InStartingSpot);

	FPlayerInfo()
	{
	}

	void SetCPUPlayerState(ARTSPlayerState * InState) { CPUPlayerState = InState; }

	/* Return whether this player is a match observer */
	bool IsObserver() const { return Team == ETeam::Observer; }
};


/* All the info required by game mode to start a match */
USTRUCT()
struct FMatchInfo
{
	GENERATED_BODY()

protected:

	/* Number of teams in match */
	UPROPERTY()
	int32 NumTeams;

	/* Online, LAN, offline etc */
	UPROPERTY()
	EMatchType MatchType;

	/* Display name of map, not file name */
	UPROPERTY()
	FText MapDisplayName;

	/* Unique ID for the map */
	UPROPERTY()
	FMapID MapID;

	/* The name of the map that can be passed into UGameplayStatics::OpenLevel e.g. Entry, 
	Minimal_Default, etc */
	UPROPERTY()
	FName MapFName;

	/* The defeat condition */
	UPROPERTY()
	EDefeatCondition DefeatCondition;

	/* Resources players start match with. How much this actually is is defined in game
	instance */
	UPROPERTY()
	EStartingResourceAmount StartingResources;

	/* All players and their info */
	UPROPERTY()
	TArray <FPlayerInfo> Players;

public:

	int32 GetNumTeams() const { return NumTeams; }
	void SetNumTeams(int32 NewValue) { NumTeams = NewValue; }
	EMatchType GetMatchType() const { return MatchType; }
	void SetMatchType(EMatchType InMatchType) { MatchType = InMatchType; }
	/**
	 *	@param InMapName - the name map was named in editor e.g. Minimal_Default, entry, etc 
	 *	@param InDisplayName - name of map to appear on UI
	 */
	void SetMap(const FName & InMapName, FMapID InMapID, const FText & InDisplayName);
	const FText & GetMapDisplayName() const { return MapDisplayName; }
	FMapID GetMapUniqueID() const { return MapID; }
	/** @return - the name that can be used in UGameplayStatics::OpenLevel */
	const FName & GetMapFName() const { return MapFName; }
	EDefeatCondition GetDefeatCondition() const { return DefeatCondition; }
	void SetDefeatCondition(EDefeatCondition InCondition) { DefeatCondition = InCondition; }
	EStartingResourceAmount GetStartingResources() const { return StartingResources; }
	void SetStartingResources(EStartingResourceAmount InValue) { StartingResources = InValue; }
	const TArray <FPlayerInfo> & GetPlayers() const { return Players; }
	TArray <FPlayerInfo> & GetPlayers() { return Players; }
	void AddPlayer(const FPlayerInfo & PlayerInfo) { Players.Emplace(PlayerInfo); }
};


//==============================================================================================
//	Buff/Debuff related function pointer typedefs and structs  
//==============================================================================================

/**
 *	Function type for trying to initially apply debuff to a target.
 *	Ok to assume the instigator and target are both valid
 *
 *	@param BuffOrDebuffInstigator - the infantry unit applying the buff/debuff
 *	@param Target - the target we're trying to apply the buff/debuff to
 *	@param BuffOrDebuffInfo - info struct in case needed. Perhaps the particle system contained on 
 *	it wants to be used for example.
 *	@return - true if the buff/debuff is successfully applied to the target 
 */
typedef EBuffOrDebuffApplicationOutcome(*Tickable_FunctionPtr_TryApplyTo)(AActor * BuffOrDebuffInstigator,
	ISelectable * InstigatorAsSelectable, ISelectable * Target, const FTickableBuffOrDebuffInfo & BuffOrDebuffInfo);
typedef EBuffOrDebuffApplicationOutcome(*Static_FunctionPtr_TryApplyTo)(AActor * BuffOrDebuffInstigator, 
	ISelectable * InstigatorAsSelectable, ISelectable * Target, const FStaticBuffOrDebuffInfo & BuffOrDebuffInfo);

/**
 *	For tickable buffs/debuffs only. Called everytime the buff/debuff should tick. Could do 
 *	stuff like damage target, whatever
 *
 *	@param Target - the selectable this buff/debuff is applied to
 *	@param StateInfo - info about the state of the buff/debuff. Can access the
 *	tick count through this. Tick count range = [1, NumberOfTicks]. Could possibly relax the const
 *	to accomodate some logic that may want to change state during the tick
 */
typedef EBuffOrDebuffTickOutcome(*FunctionPtr_DoTick)(ISelectable * Target, const FTickableBuffOrDebuffInstanceInfo & StateInfo);

/**
 *	The function that is called when the buff/debuff is removed from the target (tickable buffs/debuffs 
 *	only, use Static_FunctionPtr_OnRemoved for static buffs/debuffs)
 *
 *	@param Target - the selectable this buff/debuff was applied to
 *	@param RemovalInstigator - the selectable that instigated removing the buff/debuff. When the
 *	buff/debuff is removed because its time has expired then this will just be null or point to
 *	the target or something - haven't decided yet. When the buff/debuff is removed because the
 *	target reaches zero health from another source then it will point to the instigator of the
 *	damage that caused the target to reach zero health. Finally when removed for some other
 *	reason (e.g. some 'cleanse' type spell) then it will point to the selectable that instigated
 *	that spell. At least that's the plan
 *	@param StateInfo - info about the state of the buff/debuff
 *	@param RemovalReason - the reason the buff/debuff has been removed
 *	@param BuffOrDebuffInfo - info struct in case it is needed
 */
typedef EBuffOrDebuffRemovalOutcome(*Tickable_FunctionPtr_OnRemoved)(ISelectable * Target, const FTickableBuffOrDebuffInstanceInfo & StateInfo,
	AActor * Remover, ISelectable * RemoverAsSelectable, EBuffAndDebuffRemovalReason RemovalReason, const FTickableBuffOrDebuffInfo & BuffOrDebuffInfo);
/* Version for 'static' buffs/debuffs */
typedef EBuffOrDebuffRemovalOutcome(*Static_FunctionPtr_OnRemoved)(ISelectable * Target, const FStaticBuffOrDebuffInstanceInfo & StateInfo,
	AActor * Remover, ISelectable * RemoverAsSelectable, EBuffAndDebuffRemovalReason RemovalReason, const FStaticBuffOrDebuffInfo & BuffOrDebuffInfo);


/**
 *	Info struct for a buff/debuff that never expires on its own and does not do anything over
 *	time.
 *	@See CommonEnums.h EImmortalBuffAndDebuffType
 *
 *	Only some of this struct is exposed to blueprints. 2 functions will need to be implemented in
 *	C++
 *
 *	Does not hold any state
 */
USTRUCT()
struct FStaticBuffOrDebuffInfo
{
	GENERATED_BODY()

protected:

	EStaticBuffAndDebuffType SpecificType;
	
	//====================================================================================
	/* Function pointers that define the behavior for certain events */
	Static_FunctionPtr_TryApplyTo TryApplyToPtr;
	Static_FunctionPtr_OnRemoved OnRemovedPtr;

	/**
	 *	Whether buff or debuff. May add logic that distinguishes between the two
	 *	so adding this variable
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EBuffOrDebuffType Type;

	/* The subtype of this buff/debuff */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EBuffAndDebuffSubType SubType;

	/* The name of this buff/debuff */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FText DisplayName;

	/* The image that represents this buff/debuff */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FSlateBrush DisplayImage;

	/**
	 *	Some particles. This isn't referenced anywhere in code. These are just here in case
	 *	you want to attach particles to target and want to avoid writing hardcoded asset paths
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	UParticleSystem * ParticlesTemplate;

	/* Where on the selectable the particles should try attach to */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	ESelectableBodySocket ParticlesAttachPoint;


public:

	FStaticBuffOrDebuffInfo();
	explicit FStaticBuffOrDebuffInfo(EStaticBuffAndDebuffType InSpecificType);

	/** @See FTickableBuffOrDebuffInfo::SetFunctionPointers */
	void SetFunctionPointers(Static_FunctionPtr_TryApplyTo InTryApplyToPtr, Static_FunctionPtr_OnRemoved InOnRemovedPtr);

	/** Return whether this is considered a buff */
	bool IsBuff() const;

	/** Return whether this is considered a debuff */
	bool IsDebuff() const;

	UParticleSystem * GetParticlesTemplate() const { return ParticlesTemplate; }
	ESelectableBodySocket GetParticlesAttachPoint() const { return ParticlesAttachPoint; }

	/* Call the function pointed to by TryApplyToPtr and try apply the buff/debuff to a target 
	@return - true if the buff/debuff considers itself successfully applied */
	EBuffOrDebuffApplicationOutcome ExecuteTryApplyBehavior(AActor * BuffOrDebuffInstigator, 
		ISelectable * InstigatorAsSelectable, ISelectable * Target) const;

	EBuffOrDebuffRemovalOutcome ExecuteOnRemovedBehavior(ISelectable * Target, const FStaticBuffOrDebuffInstanceInfo & StateInfo,
		AActor * Remover, ISelectable * RemoverAsSelectable, EBuffAndDebuffRemovalReason RemovalReason) const;

	const FText & GetDisplayName() const;
	const FSlateBrush & GetDisplayImage() const;
	EStaticBuffAndDebuffType GetSpecificType() const;
	EBuffAndDebuffSubType GetSubType() const;

#if WITH_EDITOR
	void SetSpecificType(EStaticBuffAndDebuffType InSpecificType);
#endif
};


/**
 *	Info that defines the behavior of a buff/debuff that requires one or both of the following:
 *	- times out by itself e.g. a 10 sec speed increase
 *	- logic that executes at intervals e.g. a DoT that does 20 damage every 2 sec
 *
 *	Only some of this struct is exposed to blueprints. 3 functions will need to be implemented in
 *	C++
 *
 *	Does not hold any state
 */
USTRUCT()
struct FTickableBuffOrDebuffInfo
{
	GENERATED_BODY()

protected:

	/**
	 *	The amount of time between ticks.
	 *	NumberOfTicks * TickInterval = total duration of buff/debuff.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = "0.1"))
	float TickInterval;

	/**
	 *	How ticks this buff/debuff will execute before it will fall off.
	 *
	 *	Use 0 for buff/debuff that never expires.
	 *	If you want a buff/debuff that never expires and doesn't have any tick logic then
	 *	you can use the other buff/debuff struct instead
	 *
	 *	If you want a buff that say increases move speed for 10sec then set NumberOfTicks
	 *	to 1 and TickInterval to 10.
	 *
	 *	Keeping this as 1 byte since might have to send this over the wire
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	uint8 NumberOfTicks;

	ETickableBuffAndDebuffType SpecificType;

	//====================================================================================
	/* Function pointers that define the behavior for certain events */
	Tickable_FunctionPtr_TryApplyTo TryApplyToPtr;
	FunctionPtr_DoTick DoTickPtr;
	Tickable_FunctionPtr_OnRemoved OnRemovedPtr;

	/**
	 *	Whether buff or debuff. May add logic that distinguishes between the two
	 *	so adding this variable
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EBuffOrDebuffType Type;

	/* The subtype of this buff/debuff */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EBuffAndDebuffSubType SubType;

	/* The name of this buff/debuff */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FText DisplayName;

	/* The image that represents this buff/debuff */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	UTexture2D * DisplayImage;

	/**
	 *	Some particles. This isn't referenced anywhere in code. These are just here in case
	 *	you want to attach particles to target and want to avoid writing hardcoded asset paths 
	 *	in C++ such as inside the TryApplyTo function.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	UParticleSystem * ParticlesTemplate;

	/* Where on the selectable the particles should try attach to */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	ESelectableBodySocket ParticlesAttachPoint;

public:

	FTickableBuffOrDebuffInfo();
	explicit FTickableBuffOrDebuffInfo(ETickableBuffAndDebuffType InSpecificType);

	/**
	 *	Set what the logic functions are for this buff/debuff. Could be used mid game to improve
	 *	the effects of the buff/debuff due to upgrade or whatnot
	 *
	 *	@param InTypeApplyToPtr - function pointer to the function that is called when the
	 *	buff/debuff is tried to be applied to something
	 *	@param InDoTickPtr - function pointer to the logic that happens each tick
	 *	@param InOnRemovedPtr - function pointer to the function that is called when this buff/debuff
	 *	is removed
	 */
	void SetFunctionPointers(Tickable_FunctionPtr_TryApplyTo InTryApplyToPtr, FunctionPtr_DoTick InDoTickPtr,
		Tickable_FunctionPtr_OnRemoved InOnRemovedPtr);

	/* Get the time between ticks */
	float GetTickInterval() const;

	/* Get how many times this buff/debuff should tick. 0 means tick indefinently */
	uint8 GetNumberOfTicks() const;

	/**
	 *	Return whether this buff/debuff will remove itself over time or not
	 *
	 *	@return - true if buff/debuff will eventually fall off some time
	 */
	bool ExpiresOverTime() const;

	/** 
	 *	Get how long this buff/debuff lasts from application. Returns 0 if buff/debuff has an 
	 *	infinite duration 
	 */
	float GetFullDuration() const;

	/** Return whether this is considered a buff */
	bool IsBuff() const;

	/** Return whether this is considered a debuff */
	bool IsDebuff() const;

	UParticleSystem * GetParticlesTemplate() const { return ParticlesTemplate; }
	ESelectableBodySocket GetParticlesAttachPoint() const { return ParticlesAttachPoint; }

	/* Call the function pointed to by TryApplyToPtr and try apply the buff/debuff to a target
	@return - true if the buff/debuff considers itself successfully applied */
	EBuffOrDebuffApplicationOutcome ExecuteTryApplyBehavior(AActor * BuffOrDebuffInstigator,
		ISelectable * InstigatorAsSelectable, ISelectable * Target) const;

	/** 
	 *	Call the function pointed to by DoTickPtr 
	 *	
	 *	@param StateInfo - the state that is executing this 
	 */
	EBuffOrDebuffTickOutcome ExecuteDoTickBehavior(ISelectable * Target, const FTickableBuffOrDebuffInstanceInfo & StateInfo) const;

	/**
	 *	Call the function pointed to by OnRemovedPtr 
	 *	
	 *	@param StateInfo - the state that is executing this 
	 */
	EBuffOrDebuffRemovalOutcome ExecuteOnRemovedBehavior(ISelectable * Target, const FTickableBuffOrDebuffInstanceInfo & StateInfo,
		AActor * Remover, ISelectable * RemoverAsSelectable, EBuffAndDebuffRemovalReason RemovalReason) const;

	const FText & GetDisplayName() const;
	UTexture2D * GetDisplayImage() const;
	ETickableBuffAndDebuffType GetSpecificType() const;
	EBuffAndDebuffSubType GetSubType() const;

#if WITH_EDITOR
	void SetSpecificType(ETickableBuffAndDebuffType InSpecificType);
#endif
};


USTRUCT()
struct FBuffOrDebuffSubTypeInfo
{
	GENERATED_BODY()

protected:

	/* The texture to set on the border widget for buffs/debuffs of this subtype */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	UTexture2D * Texture;

	/* The color to set on the border widget for buffs/debuffs of this subtype */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FLinearColor BorderBrushColor;

public:

	FBuffOrDebuffSubTypeInfo();

	UTexture2D * GetTexture() const;
	const FLinearColor & GetBrushColor() const;
};


/* FSelectableResourceInfo is already taken so chose a too sepcific name */
USTRUCT()
struct FSelectableResourceColorInfo
{
	GENERATED_BODY()

protected:

	/* The param we use for UProgressBar::SetFillColorAndOpacity */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FLinearColor PBarFillColor;

public:

	FSelectableResourceColorInfo()
		: PBarFillColor(FLinearColor(0.f, 0.25f, 1.f, 1.f))
	{
	}

	const FLinearColor & GetPBarFillColor() const { return PBarFillColor; }
};
