// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"

#include "Statics/Structs_1.h"
#include "Statics/Statics.h"
#include "RTSGameMode.generated.h"

class ARTSPlayerController;
class URTSGameInstance;
class ARTSGameState;
class ARTSPlayerState;
class ISelectable;


//==============================================================================================
//	Enums and Structs
//==============================================================================================

/* Result of what to do with a selectable already placed on map when PIE/standalone starts */
UENUM()
enum class EPIEOnMapSelectableSetupResult : uint8
{
	// Here to pick up uninitialized values. Means nothing else. Never to be used
	None,

	/* Owner index on instance on map is too high for the number of players in PIE and was not
	reassigned to the server player so will not be respawned */
	OwnerIndexTooHigh,

	/* Was assigned an owner who is a match observer so cannot be respawned */
	ObserverOwner,

	/* Was assigned owner but the owner's faction does not have this selectable on its
	building/unit roster so could not be respawned */
	WrongFaction,

	/* Selectable will be respawned later and will appear in PIE/standalone session */
	Success
};


/* Info for logging about selectables already in map on PIE */
USTRUCT()
struct FPIESelectableLoggingInfo
{
	GENERATED_BODY()

	/* Actor name */
	UPROPERTY()
	FString Name;

	/* Owner index */
	UPROPERTY()
	int32 OwnerIndex;

	FPIESelectableLoggingInfo();
	FPIESelectableLoggingInfo(const FString & ActorName, int32 InOwnerIndex);
};


/* For PIE testing only. Info needed to respawn a selectable already placed in map */
USTRUCT()
struct FPIEStartingSelectableInfo
{
	GENERATED_BODY()

protected:

	/* Blueprint of selectable */
	UPROPERTY()
	TSubclassOf < AActor > Actor_BP;

	/* Transform to spawn actor at */
	UPROPERTY()
	FTransform SpawnTransform;

	/* Index of player that will own selectable.
	-2 = no one
	0 = server player
	1 = client 1
	2 = client 2
	and so on */
	UPROPERTY()
	int32 OwnerIndex;

	/* Whether the owner index is for a CPU or human player */
	UPROPERTY()
	bool bIsForCPUPlayer;

	/* The type of building/unit this is. Here for CPU player AI controllers. NotBuilding if 
	selectablke is not a building, and NotUnit if selectable is not a unit */
	EBuildingType BuildingType;
	EUnitType UnitType;

public:

	FPIEStartingSelectableInfo();
	FPIEStartingSelectableInfo(AActor * InSelectable, ISelectable * AsSelectable, int32 InOwnerIndex, 
		bool bForCPUPlayer);

	TSubclassOf < AActor > GetActorBP() const;
	const FTransform & GetSpawnTransform() const;
	int32 GetOwnerIndex() const;
	bool IsForCPUPlayer() const;
	EBuildingType GetBuildingType() const;
	EUnitType GetUnitType() const;
	bool IsForBuilding() const;
	bool IsForUnit() const;
};


/* Multiplayer lobbies only. Holds what settings a newly joined player wants to set as their 
default values when they join */
USTRUCT()
struct FNewPlayerDefaultValues
{
	GENERATED_BODY()

protected:

	/* The faction they want to be when they join */
	UPROPERTY()
	EFaction Faction;

public:

	/* Default constructor, never to be used */
	FNewPlayerDefaultValues();

	/* Constructor intended to be used */
	FNewPlayerDefaultValues(EFaction InFaction);

	EFaction GetFaction() const;
};


//==============================================================================================
//	Game Mode Implementation
//==============================================================================================

/**
 *	Game mode handles checking if players are defeated during a match. It also handles what
 *	to do when testing in PIE/standalone
 *
 *	When testing in PIE game mode and auto-connect true game mode will only be spawned for
 *	the server player, or at least BeginPlay only gets called for server player
 */
UCLASS()
class RTS_VER2_API ARTSGameMode : public AGameMode
{
	GENERATED_BODY()

public:

	ARTSGameMode();

protected:

	/* Called during Login(). Handles assigning player name via APlayerState::SetPlayerName */
	virtual FString InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId,
		const FString& Options, const FString& Portal) override;

	virtual void PostLogin(APlayerController * NewPlayer) override;

	virtual void Logout(AController * Exiting) override;

	/* I use this as the entry point for starting the game from the operating system */
	virtual void BeginPlay() override;

	/* Called during StartMatch. Even though param is just AController pretty sure it does nothing
	for bot players */
	virtual void RestartPlayer(AController * NewPlayer) override;

	/* Info about players that gets parsed from their options string during Login(). It needs 
	to be cached here because it is not safe to send by RPC until PostLogin() */
	UPROPERTY()
	TMap < APlayerController *, FNewPlayerDefaultValues > NewPlayerDefaultValues;

	/* Reference to game instance */
	UPROPERTY()
	URTSGameInstance * GI;

	/* Reference to game state */
	UPROPERTY()
	ARTSGameState * GS;


#if WITH_EDITORONLY_DATA

	//========================================================================================
	//	Testing in PIE/standalone
	//========================================================================================

	/* The number of PIE players including server player. Excludes CPU players */
	int32 NumPIEClients;

	// Number of CPU players actually spawned for PIE/standalone session, not the number desired
	int32 NumPIECPUPlayers;

	/* Array of info about actors to respawn as part of the PIE/standalone setup */
	UPROPERTY()
	TArray < FPIEStartingSelectableInfo > ToBeRespawned;

	/* Neutral selectables that should be setup close to starting match */
	TArray < ISelectable * > NeutralSelectables;

#endif

#if WITH_EDITOR

	/* Handles loading straight into map instead of showing main menu. Only called when WITH_EDITOR
	is true */
	virtual void GoToMapFromStartup();

	void GoToMapFromStartupPart0();

	void GoToMapFromStartupPartZeroPointFive(const TSet < int32 > & CPUPlayersWithAtLeastOneSelectable);

	UFUNCTION()
	void GoToMapFromStartupPartZeroPointSevenFive(int32 InNumTeams, const TMap < ETeam, ETeam > & NewTeamsMap);

	/* @param NewTeamsMap - mapping from team set in dev settings to lowest enum value possible
	team */
	UFUNCTION()
	void GoToMapFromStartupPart1(int32 InNumTeams, const TMap < ETeam, ETeam > & NewTeamsMap);

	UFUNCTION()
	void GoToMapFromStartupPart2(const TArray < ARTSPlayerController * > & PlayerControllers,
		int32 InNumTeams, const TArray < ARTSPlayerState * > & PlayerStates);

	UFUNCTION()
	void GoToMapFromStartupPart3(const TArray < ARTSPlayerState * > & PlayerStates);

	UFUNCTION()
	void GoToMapFromStartupPart4(const TArray < ARTSPlayerState * > & PlayerStates);

	UFUNCTION()
	void GoToMapFromStartupPart5(const TArray < ARTSPlayerState * > & PlayerStates);

#endif // WITH_EDITOR


	//========================================================================================
	//	Other stuff
	//========================================================================================

	/* Show startup movie for when starting from OS */
	void ShowIntro();

	UFUNCTION()
	void OnIntroFinished();

	/** 
	 *	Call function that returns void after delay
	 *
	 *	@param TimerHandle - timer handle to use
	 *	@param Function - function to call
	 *	@param Delay - delay before calling function 
	 */
	void Delay(FTimerHandle & TimerHandle, void(ARTSGameMode:: *Function)(), float Delay);


	//========================================================================================
	//	Defeat conditions
	//========================================================================================

	/* Rule for if player is defeated */
	EDefeatCondition DefeatCondition;

	typedef bool (ARTSGameMode:: *FunctionPtrType)(const ARTSPlayerState *) const;
	/* Function pointer to function to use to check if team has been defeated */
	FunctionPtrType DefeatFunctions[Statics::NUM_DEFEAT_CONDITIONS];

	/** 
	 *	Having custom victory conditions means polling seems like the most feasible option. Rate
	 *	at which checks to see if defeat conditions are fulfilled. The larger the value the greater
	 *	the chance of a draw 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	float DefeatConditionCheckRate;

	/* Timer handle for checking if players have been defeated and as a consquence if the match
	should end */
	FTimerHandle TimerHandle_DefeatCondition;

	/* Checks to see if a player is defeated */
	virtual bool IsPlayerDefeated(const ARTSPlayerState * Player) const;

	/* Setup DefeatFunctions array in constructor */
	void SetupDefeatFunctions();

	/* Checks if a player has been defeated under the no condition defeat condition. This will
	always return false */
	bool DefeatFunction_NoCondition(const ARTSPlayerState * Player) const;

	/** 
	 *	Checks to see if a player is defeated when using the AllBuildingsDestroyed defeat
	 *	condition
	 *
	 *	@return - true if player has been defeated 
	 */
	bool DefeatFunction_AllBuildingsDestroyed(const ARTSPlayerState * Player) const;

public:

	/** 
	 *	Start constantly checking if players in match have been defeated. If only one (or zero)
	 *	teams are undefeated then the match will end 
	 */
	void StartDefeatConditionChecking(EDefeatCondition MatchDefeatCondition);

	/* Stop checking if match is over */
	void StopDefeatConditionChecking();

protected:

	/* Check if a team has won the match and notify game state */
	void CheckDefeatCondition();
};
