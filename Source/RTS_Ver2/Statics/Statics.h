// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Statics/CommonEnums.h"
#include "Statics/OtherEnums.h"


class ARTSGameState;
class ISelectable;
class UWorld;
class URTSGameInstance;
class ARTSPlayerState;
class ABuilding;
class USoundConcurrency;
class USoundBase;
class AFactionInfo;
class AInfantry;
struct FContextButtonInfo;
struct FBuildingInfo;
struct FSpawnDecalInfo;
class AFogOfWarManager;
class UFogObeyingAudioComponent;
enum class ESoundFogRules : uint8;
struct FVisibilityInfo;
enum class EFogStatus : uint8;


/* Performance related variables */

// Whether to run fog of war calculations on a seperate thread
#define FOG_AND_MINIMAP_ON_SEPERATE_THREAD 0


//============================================================================================
//	Collision Channels
//============================================================================================

/** 
 *	Some custom collision profiles other classes require: 
 *	- Infantry 
 *	- BuildingMesh 
 *	- Bounds 
 *	- PlayerCamera 
 */

/* Channel for things in the environment e.g. landscape, trees. Just defined as world
static but may want to change this later

Some things it is used for:
- Setting loc for building's rally point
- Tracing to see if ground is flat when placing building 
- where to drop inventory items on the ground
- some other things */
#define ENVIRONMENT_CHANNEL ECC_WorldStatic

/* Channel to trace against to know what the player camera collides with */
#define CAMERA_CHANNEL ECC_Camera

/* The edge of the map */
#define MAP_BOUNDRY_CHANNEL ECollisionChannel::ECC_GameTraceChannel12

/* Trace channel for selectables. Needs to be whatever the "Selectables" trace channel is in
the .ini file */
#define SELECTABLES_CHANNEL ECollisionChannel::ECC_GameTraceChannel9

/* Channel to trace against for: 
- where a command is issued 
- possibly a lot of things ENVIRONMENT_CHANNEL was used for */
#define GROUND_CHANNEL ECollisionChannel::ECC_GameTraceChannel10

/* Selectables channel and ground channel combined into one because line traces can only be 
against one channel. So basically if something blocks either/both selectables or ground 
then it should block for this channel too */
#define SELECTABLES_AND_GROUND_CHANNEL ECollisionChannel::ECC_GameTraceChannel11


class RTS_VER2_API Statics
{
public:

	/* Path where hardware cursors are located */
	static const FName HARDWARE_CURSOR_PATH;

	// TODO make this specific to each map, and spawn projectiles and other things here
	/* A location to spawn lots of things that are pooled. It shouldn't have an effect on the
	game/performance even if it is in the middle of the map but to be safe it should be somewhere
	away from the play area */
	static const FVector POOLED_ACTOR_SPAWN_LOCATION;

	/* The location in the world where AInfo actors are spawned TODO should be specific to
	map */
	static const FVector INFO_ACTOR_SPAWN_LOCATION;

	/* Names of sockets that can be added to meshes that represent locations for abilities to
	attach to or spawn stuff at */
	static constexpr int32 NUM_CUSTOM_MESH_SOCKETS = 3;
	static const FName MeshFloorSocket;
	static const FName MeshMiddleSocket;
	static const FName MeshHeadSocket;

	/* Text to appear in lobby team combo box for being an observer */
	static const FString LOBBY_OBSERVER_COMBO_BOX_OPTION;

	/* For knowing how many elements in enums */
	static constexpr uint8 NUM_FACTIONS =							static_cast<uint8>(EFaction::z_ALWAYS_LAST_IN_ENUM) - 1;
	static constexpr uint8 NUM_MENU_WIDGET_TYPES =					static_cast<uint8>(EWidgetType::z_ALWAYS_LAST_IN_ENUM);
	static constexpr uint8 NUM_BUILDING_TYPES =						static_cast<uint8>(EBuildingType::z_ALWAYS_2ND_LAST_IN_ENUM);
	static constexpr uint8 NUM_UNIT_TYPES =							static_cast<uint8>(EUnitType::z_ALWAYS_2ND_LAST_IN_ENUM) - 1;
	static constexpr uint8 NUM_CONTEXT_ACTIONS =					static_cast<uint8>(EContextButton::z_ALWAYS_3RD_LAST_IN_ENUM) - 1;
	static constexpr uint8 NUM_CUSTOM_CONTEXT_ACTIONS =				static_cast<uint8>(EContextButton::AttackMove) - 1;
	static constexpr uint8 NUM_ANIMATIONS =							static_cast<uint8>(EAnimation::z_ALWAYS_LAST_IN_ENUM) - 1;
	static constexpr uint8 NUM_GAME_NOTIFICATION_TYPES =			static_cast<uint8>(EGameNotification::z_ALWAYS_LAST_IN_ENUM) - 1;
	static constexpr uint8 NUM_GENERIC_GAME_WARNING_TYPES =			static_cast<uint8>(EGameWarning::z_ALWAYS_2ND_LAST_IN_ENUM) - 1;
	static constexpr uint8 NUM_ARMOUR_TYPES =						static_cast<uint8>(EArmourType::z_ALWAYS_LAST_IN_ENUM) - 1;
	static constexpr uint8 NUM_UPGRADE_TYPES =						static_cast<uint8>(EUpgradeType::z_ALWAYS_2ND_LAST_IN_ENUM);
	static constexpr uint8 NUM_CPU_DIFFICULTIES =					static_cast<uint8>(ECPUDifficulty::z_ALWAYS_LAST_IN_ENUM) - 2; // Excludes DoesNothing
	static constexpr uint8 NUM_DEFEAT_CONDITIONS =					static_cast<uint8>(EDefeatCondition::z_ALWAYS_LAST_IN_ENUM) - 1;
	static constexpr uint8 NUM_RESOURCE_TYPES =						static_cast<uint8>(EResourceType::z_ALWAYS_LAST_IN_ENUM) - 1;
	static constexpr uint8 NUM_HOUSING_RESOURCE_TYPES =				static_cast<uint8>(EHousingResourceType::z_ALWAYS_LAST_IN_ENUM) - 1;
	static constexpr uint8 NUM_PERSISTENT_HUD_TAB_TYPES =			static_cast<uint8>(EHUDPersistentTabType::z_ALWAYS_LAST_IN_ENUM); // Includes None
	static constexpr uint8 NUM_MATCH_WIDGET_TYPES =					static_cast<uint8>(EMatchWidgetType::z_ALWAYS_3RD_LAST_IN_ENUM) - 1;
	static constexpr uint8 NUM_BUILDING_ANIMATIONS =				static_cast<uint8>(EBuildingAnimation::z_ALWAYS_2ND_LAST_IN_ENUM) - 1;
	static constexpr uint8 NUM_TARGETING_TYPES =					static_cast<uint8>(ETargetingType::z_ALWAYS_LAST_IN_ENUM) - 1;
	static constexpr uint8 NUM_COMMAND_TARGET_TYPES =				static_cast<uint8>(ECommandTargetType::z_ALWAYS_LAST_IN_ENUM) - 1; // Exclude observer
	static constexpr uint8 NUM_AFFILIATIONS =						static_cast<uint8>(EAffiliation::z_ALWAYS_LAST_IN_ENUM) - 2; // Exclude observer
	static constexpr uint8 NUM_MATCH_LOADING_STATUSES =				static_cast<uint8>(ELoadingStatus::z_ALWAYS_LAST_IN_ENUM) - 1;
	static constexpr uint8 NUM_STARTING_RESOURCE_AMOUNT_TYPES =		static_cast<uint8>(EStartingResourceAmount::z_ALWAYS_2ND_LAST_IN_ENUM) - 1; // Excludes DevSettings
	static constexpr uint8 NUM_MESSAGE_RECIPIENT_TYPES =			static_cast<uint8>(EMessageRecipients::z_ALWAYS_LAST_IN_ENUM) - 1;
	static constexpr uint8 NUM_STATIC_BUFF_AND_DEBUFF_TYPES =		static_cast<uint8>(EStaticBuffAndDebuffType::z_ALWAYS_LAST_IN_ENUM) - 1;
	static constexpr uint8 NUM_TICKABLE_BUFF_AND_DEBUFF_TYPES =		static_cast<uint8>(ETickableBuffAndDebuffType::z_ALWAYS_LAST_IN_ENUM) - 1;
	static constexpr uint8 NUM_BUFF_AND_DEBUFF_SUBTYPES =			static_cast<uint8>(EBuffAndDebuffSubType::z_ALWAYS_LAST_IN_ENUM) - 1; // Exclude Default
	static constexpr uint8 NUM_CUSTOM_ABILITY_CHECK_TYPES =			static_cast<uint8>(EAbilityRequirement::z_ALWAYS_LAST_IN_ENUM) - 2; // Exclude uninitialized and "success"
	static constexpr uint8 NUM_SELECTABLE_RESOURCE_TYPES =			static_cast<uint8>(ESelectableResourceType::None);
	static constexpr uint8 NUM_INVENTORY_ITEM_TYPES =				static_cast<uint8>(EInventoryItem::None);
	static constexpr uint8 NUM_COMMANDER_ABILITY_TYPES =			static_cast<uint8>(ECommanderAbility::z_ALWAYS_LAST_IN_ENUM) - 1;
	static constexpr uint8 NUM_COMMANDER_SKILL_TREE_NODE_TYPES =	static_cast<uint8>(ECommanderSkillTreeNodeType::z_ALWAYS_LAST_IN_ENUM) - 1;
	static constexpr uint8 NUM_CONTROL_SETTING_TYPES =				static_cast<uint8>(EControlSettingType::z_ALWAYS_LAST_IN_ENUM) - 1;
	static constexpr uint8 NUM_BUILDING_GARRISON_NETWORK_TYPES =	static_cast<uint8>(EBuildingNetworkType::z_ALWAYS_LAST_IN_ENUM) - 1;
	static constexpr uint8 NUM_EDITOR_PLAY_SKIP_OPTIONS =			static_cast<uint8>(EEditorPlaySkippingOption::z_ALWAYS_LAST_IN_ENUM);
	static constexpr uint8 NUM_PIE_SESSION_INVALID_OWNER_RULES =	static_cast<uint8>(EInvalidOwnerIndexAction::z_ALWAYS_LAST_IN_ENUM);

	/* How high to check when making capsule sweeps for targets. Depending on how bumpy
	the map is or how high flying units fly this may need to be adjusted. Maybe used by 
	other things too like line traces */
	static constexpr float SWEEP_HEIGHT = 3000.f;

	/* Check if actor is valid */
	static bool IsValid(const AActor * Actor);
	static bool IsValid(AActor * Actor);

	/* Check if UObject is valid */
	static bool IsValid(UObject * Object);

	/* Check if selectable is valid */
	static bool IsValid(const TScriptInterface<ISelectable> & Selectable);
	static bool IsValid(TScriptInterface<ISelectable> & Selectable);

	/* Convert a TScriptInterface to an ISelectable ptr. This is mainly here because 
	I have written many functions to take ISelectable* instead of 
	TScriptInterface and don't want to overload them to accomidate TScriptInterface. 
	We assume the object of the TScriptInterface is not null.
	Tbh this func is not as fast as it could be because it checks for null on the UObject 
	but I don't know a way around that without changing engine source */
	static ISelectable * ToSelectablePtr(const TScriptInterface<ISelectable> & Obj);

	/** 
	 *	Spawn building
	 *	
	 *	@param BuildingInfo - info for building to spawn
	 *	@param Loc - location. On the ground is where you want. Building will rise up from correct
	 *	position
	 *	@param Rot - rotation
	 *	@param Owner - player state that built this building
	 *	@param World - reference to UWorld
	 *	@param bIsStartingBuilding - skip construction and spawn as already constructed. Do not play
	 *	any 'has been built' sounds or effects. Used for placing buildings at the very start of the
	 *	match
	 *	@return - a reference to the building as an ABuilding 
	 */
	static ABuilding * SpawnBuilding(const FBuildingInfo * BuildingInfo, const FVector & Loc, 
		const FRotator & Rot, ARTSPlayerState * Owner, ARTSGameState * GameState, UWorld * World,
		ESelectableCreationMethod CreationMethod);

	/** 
	 *	Spawn building for faction info but do not run its overridden BeginPlay
	 *
	 *	@return - a reference to the building as an actor 
	 */
	static ABuilding * SpawnBuildingForFactionInfo(TSubclassOf<ABuilding> Building_BP, 
		const FVector & Loc, const FRotator & Rot, UWorld * World);

	/** 
	 *	Spawn unit
	 *	
	 *	@return - a reference to the unit as a selectable 
	 */
	static ISelectable * SpawnUnitAsSelectable(TSubclassOf <AActor> Unit_BP, const FVector & Loc,
		const FRotator & Rot, ARTSPlayerState * Owner, ARTSGameState * GameState, UWorld * World,
		ESelectableCreationMethod CreationMethod, ABuilding * Builder = nullptr);

	/** 
	 *	Spawn unit
	 *
	 *	@param Unit_BP - class to spawn
	 *	@param - Loc - spawn location
	 *	@param Rot - spawn rotation
	 *	@param Owner - player state of player that owns this unit
	 *	@param World - reference to world
	 *	@param bIsStartingUnit - true if being spawned as a unit player starts match with
	 *	@param Builder - the building that built this unit, or null if none
	 *	@return - a reference to the unit as an actor 
	 */
	static AActor * SpawnUnit(TSubclassOf <AActor> Unit_BP, const FVector & Loc, const FRotator & Rot,
		ARTSPlayerState * Owner, ARTSGameState * GameState, UWorld * World, 
		ESelectableCreationMethod CreationMethod, ABuilding * Builder);

	/** 
	 *	Spawn unit for the purposes of creating faction info
	 *	
	 *	@return - a reference to the unit 
	 */
	static AInfantry * SpawnUnitForFactionInfo(TSubclassOf <AActor> Unit_BP, const FVector & Loc, const FRotator & Rot, UWorld * World);

	/**
	 *	Spawn the starting selectables for a player at start of match
	 *	
	 *	@param Owner - player state that will own the spawned selectables
	 *	@param GridTransform - the location to center the starting grid at 
	 *	@param GameInstance - reference to game instance 
	 *	@param OutSpawnedBuildingTypes - the building types of all the buildings that were spawned 
	 *	by the starting grid 
	 *	@param OutSpawnedUnitTypes - just like OutSpawnedBuildingTypes but for units 
	 */
	static void SpawnStartingSelectables(ARTSPlayerState * Owner, const FTransform & GridTransform,
		URTSGameInstance * GameInstance, TArray < EBuildingType > & OutSpawnedBuildingTypes,
		TArray<EUnitType> & OutSpawnedUnitTypes);

	/**
	 *	Spawn a particle system that obeys fog. Local only, not replicated
	 *
	 *	@param Template - template for particles
	 *	@param GameState - reference to game state
	 *	@return - reference to spawned particles 
	 */
	static UParticleSystemComponent * SpawnFogParticles(UParticleSystem * Template,
		ARTSGameState * GameState, const FVector & Location, const FRotator & Rotation, 
		const FVector & Scale3D);

	/** 
	 *	Spawn partcile system attached to a component. Particles obey fog. Not replicated
	 *	
	 *	@param GameState - reference to game state
	 *	@param EmitterTemplate - template for particles
	 *	@param WarmUpTime - warm up time to set on particles == how far through simulation 
	 *	they should start at.
	 *	@return - reference to spawned particles 
	 */
	static UParticleSystemComponent * SpawnFogParticlesAttached(ARTSGameState * GameState,
		UParticleSystem * EmitterTemplate, USceneComponent * AttachToComponent,
		FName AttachPointName = NAME_None, float WarmUpTime = 0.f, FVector Location = FVector(ForceInit),
		FRotator Rotation = FRotator::ZeroRotator, FVector Scale = FVector(1.f),
		EAttachLocation::Type LocationType = EAttachLocation::KeepRelativeOffset,
		bool bAutoDestroy = true, EPSCPoolMethod PoolingMethod = EPSCPoolMethod::None);

	/* Spawn particles that do not obey fog of war */
	static UParticleSystemComponent * SpawnParticles(UParticleSystem * Template,
		ARTSGameState * GameState, const FVector & Location, const FRotator & Rotation,
		const FVector & Scale3D);

	/** 
	 *	Given a world get its map name. When in PIE parts will be removed. 
	 *	This may return the blank persistent map instead of the streamable sublevel 
	 */
	static FString GetMapName(const UWorld * World);

	/** 
	 *	Return whether a location is an acceptable location to build a building at 
	 *
	 *	@param World - reference to world
	 *	@param GameState - reference to game state 
	 *	@param PlayerState - player state of player requesting this
	 *	@param FactionInfo - faction info for the faction player is playing as
	 *	@param BuildingType - type of building we're trying to place 
	 *	@param Location - world location where we want to place building 
	 *	@param Rotation - rotation of building 
	 *	@param bShowHUDMessage - if location is NOT suitable whether to try and show a message 
	 *	on the requesting player's HUD. CPU players should not set this true
	 *	@return - true if building can be placed at location
	 */
	static bool IsBuildableLocation(const UWorld * World, ARTSGameState * GameState, 
		ARTSPlayerState * PlayerState, AFactionInfo * FactionInfo, EBuildingType BuildingType, 
		const FVector & Location, const FRotator & Rotation, bool bShowHUDMessage);

private:

	/* Inner function for IsBuildableLocation. Handles showing message on HUD if desired */
	static bool IsBuildableLocationReturn(ARTSPlayerState * PlayerState, EGameWarning PlacementError,
		bool bShowHUDMessage);

public:

	/** 
	 *	[Server] Try and apply a buff or debuff to a target. Assumes validity of target. Instigator 
	 *	probably does not have to be valid
	 *	This is not replicated 
	 *	
	 *	@param Type - type of buff/debuff to try and apply 
	 *	@return - the result of the application 
	 */
	static EBuffOrDebuffApplicationOutcome Server_TryApplyBuffOrDebuff(AActor * BuffOrDebuffInstigator, 
		ISelectable * InstigatorAsSelectable,  ISelectable * Target, EStaticBuffAndDebuffType Type, URTSGameInstance * GameInstance);
	static EBuffOrDebuffApplicationOutcome Server_TryApplyBuffOrDebuff(AActor * BuffOrDebuffInstigator,
		ISelectable * InstigatorAsSelectable, ISelectable * Target, ETickableBuffAndDebuffType Type, URTSGameInstance * GameInstance);

	/* Given an outcome from the server, apply a buff/debuff to a target using that outcome */
	static void Client_ApplyBuffOrDebuffGivenOutcome(AActor * BuffOrDebuffInstigator,
		ISelectable * InstigatorAsSelectable, ISelectable * Target, EStaticBuffAndDebuffType Type,
		URTSGameInstance * GameInstance, EBuffOrDebuffApplicationOutcome Result);
	static void Client_ApplyBuffOrDebuffGivenOutcome(AActor * BuffOrDebuffInstigator,
		ISelectable * InstigatorAsSelectable, ISelectable * Target, ETickableBuffAndDebuffType Type,
		URTSGameInstance * GameInstance, EBuffOrDebuffApplicationOutcome Result);

	/* Given a player get the color their selectables should be colored assuming we are 
	coloring selectables based on their owning player */
	static const FLinearColor & GetPlayerColor(const ARTSPlayerState * PlayerState);

	/* Get the team color for a team. Btw the teams I am assigning for PIE sessions seem to 
	be 1 higher than I expect which is the reason why the colors are not what I expect */
	static const FLinearColor & GetTeamColor(ETeam Team);

	/* Given an FOverlapInfo get the actor that overlap is for */
	static AActor * GetActorFromOverlap(const FOverlapInfo & OverlapInfo);


	////////////////////////////////////////////////////////////////
	//	Audio
	////////////////////////////////////////////////////////////////

	/* Play a sound not anywhere in the world. Not replicated */
	static void PlaySound2D(const UObject * WorldContextObject, USoundBase * Sound,
		float VolumeMultiplier = 1.f, float PitchMultiplier = 1.f, float StartTime = 0.f,
		USoundConcurrency * ConcurrencySettings = nullptr, AActor* OwningActor = nullptr);

	/** 
	 *	Spawn fog obeying audio comp and play sound at world location. Not replicated.
	 *	
	 *	@param SoundOwnersTeam - the team that the sound is coming from
	 *	@param FogRules - how the sound obeys fog of war
	 *	@return - An audio component to manipulate the spawned sound 
	 */
	static UFogObeyingAudioComponent * SpawnSoundAtLocation(ARTSGameState * GameState, 
		ETeam SoundOwnersTeam, USoundBase * Sound,
		FVector Location, ESoundFogRules FogRules, FRotator Rotation = FRotator::ZeroRotator,
		float VolumeMultiplier = 1.f, float PitchMultiplier = 1.f, float StartTime = 0.f,
		USoundAttenuation * AttenuationSettings = nullptr, USoundConcurrency * ConcurrencySettings = nullptr,
		AActor * OwningActor = nullptr, bool bAutoDestroy = true);

	/** 
	 *	Play a sound attached to a component. Not replicated
	 *	
	 *	@param FogRules - how the sound obeys fog of war
	 *	@return - An audio component to manipulate the spawned sound 
	 */
	static UFogObeyingAudioComponent * SpawnSoundAttached(USoundBase * Sound, USceneComponent * AttachToComponent,
		ESoundFogRules FogRules, FName AttachPointName = NAME_None,
		FVector Location = FVector(ForceInit), FRotator Rotation = FRotator::ZeroRotator,
		EAttachLocation::Type LocationType = EAttachLocation::KeepRelativeOffset,
		bool bStopWhenAttachedToDestroyed = false, float VolumeMultiplier = 1.f,
		float PitchMultiplier = 1.f, float StartTime = 0.f,
		USoundAttenuation * AttenuationSettings = nullptr, USoundConcurrency * ConcurrencySettings = nullptr,
		bool bAutoDestroy = true);

protected:

	// Clone of FAudioDevice::FCreateComponentParams because private access
	struct FCreateRTSAudioCompParams
	{
		FCreateRTSAudioCompParams();
		FCreateRTSAudioCompParams(UWorld * InWorld, AActor * InActor = nullptr);
		FCreateRTSAudioCompParams(AActor * InActor);
		FCreateRTSAudioCompParams(FAudioDevice * InAudioDevice);

		USoundAttenuation* AttenuationSettings;
		TSubclassOf<UAudioComponent> AudioComponentClass;
		TSet<USoundConcurrency*> ConcurrencySet;
		bool bAutoDestroy;
		bool bPlay;
		bool bStopWhenOwnerDestroyed;

		void SetLocation(FVector InLocation);

		UWorld * World;
		AActor * Actor;
		FAudioDevice * AudioDevice;

		bool bLocationSet;
		FVector Location;

		void CommonInit();
	};

public:

	////////////////////////////////////////////////////////////////
	//	Decals
	////////////////////////////////////////////////////////////////

	/* Spawn a decal at a location. Does not obey fog of war */
	static UDecalComponent * SpawnDecalAtLocation(UObject * WorldContextObject, UMaterialInterface * DecalMaterial, FVector DecalSize,
		FVector Location, FRotator Rotation, float LifeSpan);

	/* Version that takes a FSpawnDecalInfo */
	static UDecalComponent * SpawnDecalAtLocation(UObject * WorldContextObject, const FSpawnDecalInfo & DecalInfo, FVector Location);


	////////////////////////////////////////////////////////////////
	//	Environment Queries
	////////////////////////////////////////////////////////////////

	/** 
	 *	Wrapper for line trace, but I'm getting problems with regular line trace. It misses the landscape so 
	 *	this function will likely do a very small sphere sweep instead. 
	 */
	static bool LineTraceSingleByChannel(const UWorld * World, FHitResult & OutHitResult, const FVector & TraceStart,
		const FVector & TraceEnd, ECollisionChannel Channel, const FCollisionQueryParams & QueryParams = FCollisionQueryParams::DefaultQueryParam);
	static bool LineTraceMultiByChannel(const UWorld * World, TArray<FHitResult> & OutHitResults, const FVector & TraceStart,
		const FVector & TraceEnd, ECollisionChannel Channel, const FCollisionQueryParams & QueryParams = FCollisionQueryParams::DefaultQueryParam);
	static bool LineTraceSingleByObjectType(const UWorld * World, struct FHitResult & OutHit, const FVector & Start, const FVector & End,
		const FCollisionObjectQueryParams & ObjectQueryParams, const FCollisionQueryParams & Params = FCollisionQueryParams::DefaultQueryParam);

	/** 
	 *	Do a SweepMultiByObjectType on one location using a capsule, effectively getting all
	 *	requested object types within a certain radius.
	 *	
	 *	@param World - reference to world
	 *	@param OutNearbyHits - array of all hit results
	 *	@param Location - location to center sweep on
	 *	@param QueryParams - holds the object types to query against
	 *	@param Radius - radius of the capsule
	 *	@return - true if any hit is found 
	 */
	static bool CapsuleSweep(const UWorld * World, TArray < FHitResult > & OutNearbyHits, 
		const FVector & Location, const FCollisionObjectQueryParams & QueryParams, float Radius);

	/* Simple function to get distance between 2 points excluding Z axis */
	static float GetDistance2D(const FVector & Loc1, const FVector & Loc2);

	/* Simple function to get the distance squared between two locations excluding the Z axis */
	static float GetDistance2DSquared(const FVector & Loc1, const FVector & Loc2);

	/* Calculate distance squared bewteen two actors. Just uses straight
	line but in future should use path distance i.e. taking into account
	obstacles */
	static float GetPathDistanceSqr(const AActor * Actor1, const AActor * Actor2);

	/** 
	 *	Return whether a selectable is in range of another selectable to use an ability. Assumes 
	 *	validity
	 *	
	 *	@param AbilityInfo - info for the ability AbilityUser wants to use
	 *	@param AbilityUser - the selectable that wants to use the ability
	 *	@return - true if in range 
	 */
	static bool IsSelectableInRangeForAbility(const FContextButtonInfo & AbilityInfo, ISelectable * AbilityUser, ISelectable * AbilityTarget);
	
	/**
	 *	Version for abilities that require targeting a location in the world 
	 *	
	 *	@param AbilityInfo - info for the ability AbilityUser wants to use
	 *	@param AbilityUser - the selectable that wants to use the ability 
	 *	@param Location - the world location where the selectable wants to use the ability
	 *	@return - true if in range 
	 */
	static bool IsSelectableInRangeForAbility(const FContextButtonInfo & AbilityInfo, ISelectable * AbilityUser, const FVector & Location);
	
	/** 
	 *	Return the range of an ability squared.  
	 *	
	 *	@return - distance squared 
	 */
	static float GetAbilityRangeSquared(const FContextButtonInfo & AbilityInfo);
	
	/** 
	 *	Functions for returning the distance of a selectable from another selectable or location.  
	 *	
	 *	@return - distance squared from AbilityUser's GetActorLocation() to the Target's GetActorLocation() 
	 *	or just the location if it is a location targeting ability 
	 */
	static float GetSelectablesDistanceForAbilitySquared(const FContextButtonInfo & AbilityInfo, ISelectable * AbilityUser, ISelectable * Target);
	static float GetSelectablesDistanceForAbilitySquared(const FContextButtonInfo & AbilityInfo, ISelectable * AbilityUser, const FVector & Location);

	/* Get the distance squared a selectable is from an inventory item actor. Change the 
	AActor param to an AInventoryItem if required */
	static float GetSelectablesDistanceSquaredFromInventoryItemActor(ISelectable * Selectable, AActor * InventoryItem);

	/* Returns true if a selectable can be selected by local player */
	static bool CanBeSelected(const AActor * Selectable);

	/* Return whether a selectable is visible to the local player i.e. this usually means 
	it's outside fog of war and either unstealthed or stealthed but being stealth detected */
	static bool IsSelectableVisibleLocally(const AActor * Selectable);

	/* Return whether a selectable is visible. Usually means it's outside fog of war 
	and either unstealthed or stealthed but being stealth detected */
	static bool IsSelectableVisible(const AActor * Selectable, const FVisibilityInfo * TeamVisibilityInfo);

	/**
	 *	Return whether a selectable is outside fog for a certain player. 
	 *	
	 *	I don't know if this should ever be used 
	 *	@param Selectable - selectable to check if outside fog 
	 *	@param AsSelectable - Selectable casted to a ISelectable 
	 */
	static bool IsOutsideFog(const AActor * Selectable, const ISelectable * AsSelectable, 
		ETeam Team, const FName & TeamTag, const ARTSGameState * GameState);

	/** 
	 *	Return whether a world location is outside fog of war for a particular team 
	 *
	 *	@param FogManager - game thread fog manager. If using multithreaded fog of war then this 
	 *	can be null 
	 */
	static bool IsLocationOutsideFog(const FVector & Location, ETeam Team, AFogOfWarManager * FogManager);

	/** 
	 *	Return true if a world location is outside fog of war to the local player 
	 *	
	 *	@param FogManager - game thread fog manager. If using multithreaded fog of war then this 
	 *	can be null 
	 */
	static bool IsLocationOutsideFogLocally(const FVector & Location, AFogOfWarManager * FogManager);

	/* With this version it's OK to pass in Location that is off the fog grid */
	static bool IsLocationOutsideFogLocallyNotChecked(const FVector & Location, AFogOfWarManager * FogManager);

	/** 
	 *	Returns the vision status of a tile for the local player 
	 *
	 *	@param FogManager - game thread fog manager. If using multithreaded fog of war then this 
	 *	can be null
	 */
	static EFogStatus GetLocationVisionStatusLocally(const FVector & Location, AFogOfWarManager * FogManager);

	/**
	 *	Returns the vision status of a tile for the local player or Hidden if Location 
	 *	is outside the grid. 
	 *
	 *	@param FogManager - game thread fog manager. If using multithreaded fog of war then this
	 *	can be null
	 */
	static EFogStatus GetLocationVisionStatusLocallyNotChecked(const FVector & Location, AFogOfWarManager * FogManager);

#if !UE_BUILD_SHIPPING
	/* Given an animation type return whether it is considered not good if game code says we 
	should play it but it is null. Usually these are animations with anim notifies in them */
	static bool IsShouldBeSetAnim(EAnimation AnimType);
#endif


	//////////////////////////////////////////////////////////////
	//	Selectable Actor Tag System
	//////////////////////////////////////////////////////////////

	/* Number of actor tags I add to selectables when they setup */
	static const int32 NUM_ACTOR_TAGS;

	/* Some arbitrary names of tags to put in tags array */

	/* The player ID for a neutral selectable. This goes in AActor::Tags[0] */
	static const FName NeutralID;

	/* Team tags for neutral and observer. The neutral tag will probably go on selectables while
	the observer tag will probably only ever go on the PC and PS */
	static const FName NEUTRAL_TEAM_TAG;
	static const FName OBSERVER_TEAM_TAG;

	/* Tag to say a selectable cannot be targeted by attacks or abilities */
	static const FName UNTARGETABLE_TAG;

	/* Tag for is selectable is a building */
	static const FName BuildingTag;

	/* Tag for if selectable is a unit */
	static const FName UnitTag;

	/* Tag for if selectable is an inventory item */
	static const FName InventoryItemTag;

	/* Tag to look for on actors to know if they are an air unit or not */
	static const FName AirTag;

	/* Tag for selectables that cannot fly. Oppisite of AirTag */
	static const FName NotAirTag;

	/* Tag for whether the selectable has an attack or not */
	static const FName HasAttackTag;
	static const FName NotHasAttackTag;

	/* Tags for whether selectable is above zero health or not */
	static const FName HasZeroHealthTag;
	static const FName AboveZeroHealthTag;

	/* Tags about inventory */
	static const FName NotHasInventoryTag;
	static const FName HasZeroCapacityInventoryTag;
	static const FName HasInventoryWithCapacityGreaterThanZeroTag;
	static const FName IsShopThatAcceptsRefundsTag;

	//----------------------------------------------
	// Array index getters 
	//----------------------------------------------

protected:

	/* Get index in actor's Tags array where the owning players ID should be stored */
	static int32 GetPlayerIDTagIndex();

public:

	/* Get the index in Tags where the the team tag should be stored */
	static int32 GetTeamTagIndex();

protected:

	/* Get the index in the Tags array where the targeting type tag should be */
	static int32 GetTargetingTypeTagIndex();

	/* Get index for whether selectable is an air unit or not */
	static int32 GetAirTagIndex();

	/* Get index for whether selectable is a building or unit */
	static int32 GetSelectableTypeTagIndex();

public:

	/* Get index for whether selectable is capable of attacking or not. This made public to
	allow selectable to toggle this when it gets upgraded */
	static int32 GetHasAttackTagIndex();

	/* Get index for whether the selectable has more than zero health or not */
	static int32 GetZeroHealthTagIndex();

	/* Get index for whether selectable has an inventory */
	static int32 GetInventoryTagIndex();

	// TODO replace AActor::bCanBeDamaged usage for !UStatics::HasZeroHealth

	/* Generates an FName to be used in actor tags that identifies what team they are on. Good
	to make FNames that will avoid collisions when being hashed
	@param SomeNum - a number that is different for each call */
	static FName GenerateTeamTag(int32 SomeNum);


	//----------------------------------------------
	// Queries That Use Tags
	//----------------------------------------------

	/* Return whether a selectable has zero (or less) health
	@return - true if selectable has zero health */
	static bool HasZeroHealth(const AActor * Selectable);

	/* Return whether a selectable is a building. If not then it is probably a unit */
	static bool IsABuilding(const AActor * Selectable);

	/* Return whether a selectable is a unit. If not then it probably is a building */
	static bool IsAUnit(const AActor * Selectable);

	/* Return whether a selectable is owned by a player */
	static bool IsOwned(const AActor * Selectable, ARTSPlayerState * PlayerState);
	static bool IsOwned(const AActor * Selectable, const FName & PlayerID);

	/* Return whether a selectable is friendly i.e. either owned by us and/or on our team */
	static bool IsFriendly(const AActor * Selectable, const FName & LocalPlayersTeamTag);

	/* Return whether a selectable is hostile towards a team */
	static bool IsHostile(const AActor * Selectable, const FName & LocalPlayersTeamTag);

	/* Return whether a type can be targeted */
	static bool CanTypeBeTargeted(const AActor * Target, const TArray < FName > & TypesThatCanBeTargeted);
	static bool CanTypeBeTargeted(const AActor * Target, const TSet < FName > & TypesThatCanBeTargeted);

	/* Return whether a selectable is capable of attacking or not */
	static bool HasAttack(const AActor * Selectable);

	/* For a given selectable get its targeting type */
	static const FName & GetTargetingType(const AActor * Selectable);

	/* Given a targeting type, get the FName corrisponding to it */
	static const FName & GetTargetingType(ETargetingType TargetingType);

	/* Return whether selectable is a flying selectable or not */
	static bool IsAirUnit(const AActor * Selectable);

	/* Return whether the selectable has an inventory. By has an inventory we mean they have 
	an FInventory struct and that its capacity is at least 1 */
	static bool HasInventory(const AActor * Selectable);

	/* Return whether the selectable is a shop that can also have items sold to it */
	static bool IsAShopThatAcceptsRefunds(const AActor * Selectable);

	/* Given a body location get the FName of a socket that represents that location */
	static const FName & GetSocketName(ESelectableBodySocket SocketType);


	//============================================================================================
	/* Variables */
	//============================================================================================

	/* Holds each user defined damage type */
	static TArray <TSubclassOf <UDamageType>> DamageTypes;

	/* Convert a ESelectableBodySocket to the index it corrisponds to in SocketNames */
	static int32 SocketLocationToArrayIndex(ESelectableBodySocket AttachLocation);
	static const TArray <FName, TFixedAllocator <Statics::NUM_CUSTOM_MESH_SOCKETS>> SocketNames;
	
	// Should these be UPROPERTY so to avoid instant garbage collection?

private:

	/* Maps ETargetingType to an arbitrary FName using key as TargetingTypeToArrayIndex() */
	static const TArray < FName, TFixedAllocator < Statics::NUM_TARGETING_TYPES > > TargetTypeFNames;

#if !UE_BUILD_SHIPPING

	/* Holds which animation types should throw at least a warning when the player tries to 
	play one but no animation is set */
	static const TSet < EAnimation > ImportantAnims;

#endif

	/* Setup functions */

	static TArray < FName, TFixedAllocator < Statics::NUM_CUSTOM_MESH_SOCKETS > > InitSocketNames();

	static TArray < FName, TFixedAllocator < Statics::NUM_TARGETING_TYPES > > InitTargetTypeFNames();

#if !UE_BUILD_SHIPPING

	static TSet < EAnimation > InitImportantAnims();

#endif

	//==========================================================================================
	/* Stuff to improve IDE workflow */
	//==========================================================================================

public:

	/* Return a FText of input text that is the first Length letters of text */
	static FText ConcateText(const FText & Text, int32 Length);

	/* Convert FText to int32 */
	static int32 TextToInt(const FText & Text);

	/* Convert int32 to FText */
	static FText IntToText(int32 Int);

	/* Convert ETeam to FString */
	static FString TeamToStringSlow(ETeam Team);

	/* Convert FString to ETeam */
	static ETeam StringToTeamSlow(const FString & AsString);

	/* Convert string to ECPUDifficulty. Slow */
	static ECPUDifficulty StringToCPUDifficultySlow(const FString & AsString, URTSGameInstance * GameInstance);

	/* Convert EFaction to FString. Nothing to do with the actual display name of the faction */
	static FString FactionToString(EFaction Faction);

	/* Convert a FString to EFaction */
	static EFaction StringToFaction(const FString & AsString);

	/* Returns true if the param is a power of 2, but assumes it is not 0. 
	The FMath version is not constexpr hence why this function exists */
	static constexpr bool IsPowerOfTwo(int32 Value)
	{
		return (Value & (Value - 1)) == 0;
	}


	/* Conversion functions */

	static ECollisionChannel IntToCollisionChannel(uint64 AsInt);


	/* Simple conversion functions */

	// TODO remove all static_cast from files, and use only conversion functions found here in UStatics instead

	static int32 TeamToArrayIndex(ETeam Team, bool bNeutralAndObserverAcceptable = false);
	static ETeam ArrayIndexToTeam(int32 ArrayIndex);

	static int32 AnimationToArrayIndex(EAnimation Animation);
	static EAnimation ArrayIndexToAnimation(int32 ArrayIndex);

	static int32 ContextButtonToArrayIndex(EContextButton InButton);
	static EContextButton ArrayIndexToContextButton(int32 ArrayIndex);

	static int32 ArmourTypeToArrayIndex(EArmourType ArmourType);
	static EArmourType ArrayIndexToArmourType(int32 ArrayIndex);

	static int32 TargetingTypeToArrayIndex(ETargetingType TargetingType);

	static int32 ResourceTypeToArrayIndex(EResourceType ResourceType);
	static EResourceType ArrayIndexToResourceType(int32 Index);

	static int32 HousingResourceTypeToArrayIndex(EHousingResourceType ResourceType);
	static EHousingResourceType ArrayIndexToHousingResourceType(int32 ArrayIndex);

	static int32 StartingResourceAmountToArrayIndex(EStartingResourceAmount Amount);
	static EStartingResourceAmount ArrayIndexToStartingResourceAmount(int32 ArrayIndex);

	static EGameNotification ArrayIndexToGameNotification(int32 ArrayIndex);

	static int32 FactionToArrayIndex(EFaction AsFaction);
	static EFaction ArrayIndexToFaction(int32 ArrayIndex);

	static EHUDPersistentTabType ArrayIndexToPersistentTabType(int32 ArrayIndex);

	static EAffiliation ArrayIndexToAffiliation(int32 ArrayIndex);

	static ECommandTargetType ArrayIndexToCommandTargetType(int32 ArrayIndex);
	static ECommandTargetType AffiliationToCommandTargetType(EAffiliation Affiliation);

	static int32 DefeatConditionToArrayIndex(EDefeatCondition DefeatCondition);
	static EDefeatCondition ArrayIndexToDefeatCondition(int32 ArrayIndex);

	static EGameWarning ArrayIndexToGameWarning(int32 ArrayIndex);

	static EMessageRecipients ArrayIndexToMessageRecipientType(int32 ArrayIndex);

	static ECPUDifficulty ArrayIndexToCPUDifficulty(int32 ArrayIndex);

	static EMatchWidgetType ArrayIndexToMatchWidgetType(int32 ArrayIndex);

	static EUnitType ArrayIndexToUnitType(int32 ArrayIndex);
	static int32 UnitTypeToArrayIndex(EUnitType UnitType);

	static EUpgradeType ArrayIndexToUpgradeType(int32 ArrayIndex);

	static EStaticBuffAndDebuffType ArrayIndexToStaticBuffOrDebuffType(int32 ArrayIndex);
	
	static ETickableBuffAndDebuffType ArrayIndexToTickableBuffOrDebuffType(int32 ArrayIndex);

	static EBuffAndDebuffSubType ArrayIndexToBuffOrDebuffSubType(int32 ArrayIndex);

	static EAbilityRequirement ArrayIndexToAbilityRequirement(int32 ArrayIndex);

	static ESelectableResourceType ArrayIndexToSelectableResourceType(int32 ArrayIndex);

	static EInventoryItem ArrayIndexToInventoryItem(int32 ArrayIndex);
	static int32 InventoryItemToArrayIndex(EInventoryItem InventoryItem);

	static ECommanderAbility ArrayIndexToCommanderAbilityType(int32 ArrayIndex);
	static int32 CommanderAbilityTypeToArrayIndex(ECommanderAbility AbilityType);
	
	static int32 CommanderSkillTreeNodeToArrayIndex(ECommanderSkillTreeNodeType NodeType);

	static EControlSettingType ArrayIndexToControlSettingType(int32 ArrayIndex);

	static int32 PhysicalSurfaceTypeToArrayIndex(EPhysicalSurface SurfaceType);

	static EBuildingNetworkType ArrayIndexToBuildingNetworkType(int32 ArrayIndex);
	static int32 BuildingNetworkTypeToArrayIndex(EBuildingNetworkType NetworkType);

	static EEditorPlaySkippingOption ArrayIndexToEditorPlaySkippingOption(int32 ArrayIndex);

	static EInvalidOwnerIndexAction ArrayIndexToPIESeshInvalidOwnerRule(int32 ArrayIndex);
};
