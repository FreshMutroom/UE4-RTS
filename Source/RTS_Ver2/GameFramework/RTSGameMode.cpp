// Fill out your copyright notice in the Description page of Project Settings.

#include "RTSGameMode.h"
#include "Public/TimerManager.h"
#include "Engine/World.h"
#include "Public/EngineUtils.h"
#if WITH_EDITOR
#include "Settings/LevelEditorPlaySettings.h"
#endif

#include "GameFramework/RTSGameState.h"
#include "Statics/DevelopmentStatics.h"
#include "GameFramework/RTSPlayerController.h"
#include "GameFramework/RTSPlayerState.h"
#include "GameFramework/RTSGameInstance.h"
#include "UI/MainMenu/Lobby.h"
#include "Miscellaneous/PlayerCamera.h"
#include "Networking/RTSGameSession.h"
#include "UI/MarqueeHUD.h"
#include "UI/MainMenu/Menus.h"
#include "Settings/DevelopmentSettings.h"
#include "Miscellaneous/StartingGrid/StartingGrid.h"
#include "MapElements/Building.h"
#include "GameFramework/FactionInfo.h"
#include "Miscellaneous/StartingGrid/StartingGridComponent.h"
#include "Miscellaneous/CPUPlayerAIController.h"


//================================================================================================
//	Structs
//================================================================================================

FPIESelectableLoggingInfo::FPIESelectableLoggingInfo()
{
}

FPIESelectableLoggingInfo::FPIESelectableLoggingInfo(const FString & ActorName, int32 InOwnerIndex)
{
	Name = ActorName;
	OwnerIndex = InOwnerIndex;
}


FPIEStartingSelectableInfo::FPIEStartingSelectableInfo()
{
	// Commented because crash here on startup after migrating to 4.21
	//assert(0);
}

FPIEStartingSelectableInfo::FPIEStartingSelectableInfo(AActor * InSelectable, ISelectable * AsSelectable, 
	int32 InOwnerIndex, bool bForCPUPlayer)
{
	Actor_BP = InSelectable->GetClass();
	SpawnTransform = InSelectable->GetActorTransform();
	OwnerIndex = InOwnerIndex;
	bIsForCPUPlayer = bForCPUPlayer;

	BuildingType = AsSelectable->GetAttributesBase().GetBuildingType();
	UnitType = AsSelectable->GetAttributesBase().GetUnitType();
}

TSubclassOf<AActor> FPIEStartingSelectableInfo::GetActorBP() const
{
	return Actor_BP;
}

const FTransform & FPIEStartingSelectableInfo::GetSpawnTransform() const
{
	return SpawnTransform;
}

int32 FPIEStartingSelectableInfo::GetOwnerIndex() const
{
	return OwnerIndex;
}

bool FPIEStartingSelectableInfo::IsForCPUPlayer() const
{
	return bIsForCPUPlayer;
}

EBuildingType FPIEStartingSelectableInfo::GetBuildingType() const
{
	return BuildingType;
}

EUnitType FPIEStartingSelectableInfo::GetUnitType() const
{
	return UnitType;
}

bool FPIEStartingSelectableInfo::IsForBuilding() const
{
	return BuildingType != EBuildingType::NotBuilding;
}

bool FPIEStartingSelectableInfo::IsForUnit() const
{
	return UnitType != EUnitType::NotUnit;
}


FNewPlayerDefaultValues::FNewPlayerDefaultValues()
{
	// Commented because crash here on startup after migrating to 4.21
	//UE_LOG(RTSLOG, Fatal, TEXT("Unexpected call to default constructor"));
}

FNewPlayerDefaultValues::FNewPlayerDefaultValues(EFaction InFaction)
{
	Faction = InFaction;
}

EFaction FNewPlayerDefaultValues::GetFaction() const
{
	return Faction;
}


//================================================================================================
//	Game Mode Implementation
//================================================================================================

ARTSGameMode::ARTSGameMode()
{
	PlayerControllerClass = ARTSPlayerController::StaticClass();
	DefaultPawnClass = APlayerCamera::StaticClass();
	HUDClass = AMarqueeHUD::StaticClass();
	GameSessionClass = ARTSGameSession::StaticClass();
	PlayerStateClass = ARTSPlayerState::StaticClass();
	GameStateClass = ARTSGameState::StaticClass();

	bDelayedStart = true;	// This is causing pawn not to spawn
	bUseSeamlessTravel = true;

	/* Will show up if not connected to Steam */
	DefaultPlayerName = NSLOCTEXT("Player Profile", "Default Player Name", "Player");

	SetupDefeatFunctions();

	DefeatConditionCheckRate = 1.f / 30.f;
}

FString ARTSGameMode::InitNewPlayer(APlayerController * NewPlayerController,
	const FUniqueNetIdRepl & UniqueId, const FString & Options, const FString & Portal)
{
	/* Most of this is a copy of AGameStateBase verison but has changed the name length limit
	from 20 to user a defined value */

	GI = CastChecked<URTSGameInstance>(GetWorld()->GetGameInstance());
	GS = CastChecked<ARTSGameState>(GetWorld()->GetGameState());

	/* Since I use one game mode for everything this is the case where we are using it for the
	main menu */
	if (GI->IsInMainMenuMap())
	{
		return TEXT("");
	}

#if WITH_EDITOR
	// I needed this for PostLogin. I probably need it here too
	if (GI->EditorPlaySession_ShouldSkipMainMenu())
	{
		return TEXT("");
	}
#endif

	// If here then we are in the blank persistent map used for lobby/matches 

	check(NewPlayerController);

	FString ErrorMessage;

	// Register the player with the session
	GameSession->RegisterPlayer(NewPlayerController, UniqueId.GetUniqueNetId(), UGameplayStatics::HasOption(Options, TEXT("bIsFromInvite")));

	// Set up spectating
	bool bSpectator = FCString::Stricmp(*UGameplayStatics::ParseOption(Options, TEXT("SpectatorOnly")), TEXT("1")) == 0;
	if (bSpectator || MustSpectate(NewPlayerController))
	{
		NewPlayerController->StartSpectatingOnly();
	}

	// Init player's name
	FString InName = UGameplayStatics::ParseOption(Options, TEXT("Name")).Left(PlayerOptions::MAX_ALIAS_LENGTH);
	if (InName.IsEmpty())
	{
		InName = FString::Printf(TEXT("%s%i"), *DefaultPlayerName.ToString(), NewPlayerController->PlayerState->PlayerId);
	}

	ChangeName(NewPlayerController, InName, false);

	/* If this is for us stop here. We are setup in SetupSingleplayerLobby/SetupMultiplayerLobby */
	if (NewPlayerController == GetWorld()->GetFirstPlayerController())
	{
		return ErrorMessage;
	}

	/* Get their default faction in lobby */
	FString StartingFaction = UGameplayStatics::ParseOption(Options, TEXT("faction"));
	const EFaction Faction = Statics::StringToFaction(StartingFaction);

	/* Store it in a list so it can be sent as an RPC in PostLogin since unsafe to call RPCs 
	right now */
	NewPlayerDefaultValues.Emplace(NewPlayerController, FNewPlayerDefaultValues(Faction));

	return ErrorMessage;
}

void ARTSGameMode::PostLogin(APlayerController * NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// Different behavior depending on which map we're on
	if (GI->IsInMainMenuMap())
	{
		//UE_CLOG(!GetWorld()->IsServer() || NewPlayer != GetWorld()->GetFirstPlayerController(),
		//	RTSLOG, Fatal, TEXT("AGameMode::PostLogin called while in main menu map but new "
		//		"player is not host - not expecting remote connections while in main menu map"));
		return;
	}

#if WITH_EDITOR
	if (GI->EditorPlaySession_ShouldSkipMainMenu())
	{
		return;
	}
#endif

	ARTSPlayerController * NewPlayCon = CastChecked<ARTSPlayerController>(NewPlayer);

	/* Stop here if it is being called for host. Not 100% sure this logic is correct */
	if (NewPlayer == GetWorld()->GetFirstPlayerController())
	{
		NewPlayCon->SetLobbySlotIndexForHost();
		
		return;
	}

	const FNewPlayerDefaultValues & NewPlayersDefaultValues = NewPlayerDefaultValues[NewPlayer];

	/* Update our lobby and the replicated arrays causing new player to be seen by all clients */
	const int32 LobbySlot = GS->OnClientJoinsLobby(NewPlayer, NewPlayersDefaultValues.GetFaction());

	if (LobbySlot == -1)
	{
		/* Could not find open slot for player to join into. Not logical to actually get here - 
		player should not have been allowed to join in the first place so log something and 
		disconnect new player */

		UE_LOG(RTSLOG, Warning, TEXT("Client joined lobby but no open lobby slot could be found. "
			"Number of players already in lobby before new player tried to join: %d"),
			GS->PlayerArray.Num());

		/* Call this RPC to display a message to client because I do not think KickPlayer 
		does it despite having a param for a message */
		NewPlayCon->Client_OnTryJoinButLobbyFull();

		GameSession->KickPlayer(NewPlayer, FText::FromString("Lobby is full"));
	}
	else
	{
		/* Send an RPC to the new player telling them what lobby slot they should occupy. This
		also actually updates the clients UI and is what they wait for before showing their
		lobby widget */
		NewPlayCon->Client_ShowLobbyWidget(LobbySlot, NewPlayersDefaultValues.GetFaction());
	}

	// Now that default values have been assigned can delete list entry
	NewPlayerDefaultValues.Remove(NewPlayer);

	// TODO possibly add to GS::PlayerControllers here instead of later. Maybe updae other stuff 
	// too  
}

void ARTSGameMode::Logout(AController * Exiting)
{
	// Just testing whether this is ever called for AI players
	assert(Exiting->IsA(APlayerController::StaticClass()));

	/* Called when in PIE and exiting it. Appears to not be called on AIControllers. I
	believe server player has it called on them first when exiting PIE */

	/* Appears this is called the moment player presses their exit button. Behavior in here
	only needs to be executed if it is a multiplayer match.

	Could help to figure out the exact point where we should switch from using lobby behavior
	to using in-match behavior */

	// TODO also need to check if we are host or not

	/* Need different behavior depending if we are in match or lobby */
	if (GS->IsInMatch())
	{
		// Check if host is leaving 
		if (Exiting == GetWorld()->GetFirstPlayerController())
		{
			/* Notify all clients so they can have a painless disconnect. This is already done
			when host presses their 'leave game' button so usually this array should be empty.
			If it isn't it means that host has lost connection but probably not by choice */
			GS->DisconnectAllClients();
		}
		else
		{
			ARTSPlayerController * PlayCon = CastChecked<ARTSPlayerController>(Exiting);

			/* To prevent crashes when exiting PIE with 2+ players on a team. If we want to
			simulate returning to main menu even if going straight to map with PIE then will
			need to change this maybe */
#if WITH_EDITOR
			if (GI->EditorPlaySession_ShouldSkipMainMenu() == false)
			{
				GS->HandleClientLoggingOutInMatch(PlayCon);
			}
#else
			GS->HandleClientLoggingOutInMatch(PlayCon);
#endif
		}
	}
	else // Assumed in lobby
	{
		// Check if host leaving
		if (Exiting == GetWorld()->GetFirstPlayerController())
		{
			/* Tell all clients to disconnect if they haven't already (should have already if
			we're leaving by choice just like in match case) */
			GS->DisconnectAllClients();
		}
		else
		{
			/* Notify lobby that a player left and if slots are locked then they should unlock
			because:
			1. state of lobby has changed
			2. so host cannot start match (there a requirement right now that slots must be locked
			in order to start match)
			Also doing this before Super because param might be null if done after? */
			ULobbyWidget * Lobby = GI->GetWidget<ULobbyWidget>(EWidgetType::Lobby);
			Lobby->NotifyPlayerLeft(CastChecked<ARTSPlayerController>(Exiting));
		}
	}

	Super::Logout(Exiting);
}

void ARTSGameMode::BeginPlay()
{
	Super::BeginPlay();

	/* Entry point I use for game starting from operating system. Things are called before this
	such as InitNewPlayer and PostLogin but they will do nothing interesting when starting from OS */

	/* Because same game mode used for everything we need to know if we are starting the game
	for the first time to show the intro or not and do inital setup stuff */
	if (GI->IsColdBooting())
	{
		GI->SetIsColdBooting(false);

		// Maybe can try do this asyncronously while intro plays
		/* Player controller needs to be spawned for this to work - note I had in previous GM */
		GI->Initialize();

		/* Set mouse cursor. Hoping this stays set after map changes, otherwise will need to do
		this again after map change and likely should be moved to somewhere else other than
		BeginPlay */
		for (TActorIterator <ARTSPlayerController> Iter(GetWorld()); Iter; ++Iter)
		{
			ARTSPlayerController * PlayCon = *Iter;
			PlayCon->Server_BeginPlay();
			break;
		}

#if WITH_EDITOR
		if (GI->EditorPlaySession_ShouldSkipMainMenu())
		{
			/* Skip main menu and jump straight into map for testing purposes */
			GoToMapFromStartup();
		}
		else if (GI->EditorPlaySession_ShouldSkipOpeningCutscene())
		{
			/* To show main menu */
			OnIntroFinished();
		}
		else
		{
			ShowIntro();
		}
#else
		ShowIntro();
#endif
	}
	else
	{
		GI->OnMapChange();

		/* Same game mode used for everything. Need to know whether in main menu map or lobby map */
		if (GI->IsInMainMenuMap())
		{
			/* To show main menu */
			GI->OnEnterMainMenu();
		}
		else
		{
			// If here then in match map
		}
	}
}

void ARTSGameMode::RestartPlayer(AController * NewPlayer)
{
	// Most of this just copy of AGameModeBase version but player start choosing is different

	if (NewPlayer == nullptr || NewPlayer->IsPendingKillPending())
	{
		UE_LOG(RTSLOG, Fatal, TEXT("Unexpected case: NewPlayer null in RestartPlayer(). Have already "
			"busy waited for all PCs to become valid so this should not happen"));
		
		return;
	}

	/* Just pawn, not starting units/buildings. 
	Also 2nd param is irrelevant - players start location will be given to them later and 
	clients will teleport themselves there later */
	RestartPlayerAtTransform(NewPlayer, FTransform::Identity);
}

#if WITH_EDITOR
void ARTSGameMode::GoToMapFromStartup()
{
	GI->OnInitiateSkipMainMenu();

	/* Get the number of PIE sessions so we know how many players to busy wait for */
	const ULevelEditorPlaySettings * PlayInSettings = GetDefault<ULevelEditorPlaySettings>();
	NumPIEClients = -1;
	const bool bResult = PlayInSettings->GetPlayNumberOfClients(NumPIEClients);
	assert(bResult);

	/* When I test with editor using standalone game I get the number of clients set in the
	editor PIE settings but only one window so this is here to remedy that */
	if (GetWorld()->WorldType == EWorldType::Game)
	{
		NumPIEClients = 1;
	}

	GoToMapFromStartupPart0();
}

void ARTSGameMode::GoToMapFromStartupPart0()
{
	// Some variables that will commonly be refered to in this func
	const int32 NumDesiredCPUPlayers = GI->EditorPlaySession_GetNumCPUPlayers();
	const EInvalidOwnerIndexAction InvalidHumanOwnerRule = GI->EditorPlaySession_GetInvalidHumanOwnerRule();
	const EInvalidOwnerIndexAction InvalidCPUOwnerRule = GI->EditorPlaySession_GetInvalidCPUOwnerRule();
	const TArray < FPIEPlayerInfo > & HumanPlayerInfo = GI->EditorPlaySession_GetHumanPlayerInfo();
	const TArray < FPIECPUPlayerInfo > & CPUPlayerInfo = GI->EditorPlaySession_GetCPUPlayerInfo();

	/* Starting grids found already placed in map. Iterated through and each is destroyed */
	TArray < AActor * > FactionStartingGrids;

	/* Contains which CPU owner indices have at least one selectable on map */
	TSet < int32 > CPUPlayersWithAtLeastOneSelectable;

	//-----------------------------------------
	//	Arrays for logging only
	//-----------------------------------------

	/* Selectables that were reassigned to server player because the owner index they were
	given was invalid but will still be respawned */
	TArray < FPIESelectableLoggingInfo > ReassignedToServerSelectables;

	/* Selectables that did not have valid owner indices and were destroyed */
	TArray < FPIESelectableLoggingInfo > InvalidOwnerSelectables;

	// Selectables whose owner is an observer and therefore were excluded from PIE
	TArray < FPIESelectableLoggingInfo > ObserverOwnerSelectables;

	// Selectables whose owner is on the wrong faction and therefore were excluded from PIE
	TArray < FPIESelectableLoggingInfo > WrongFactionSelectables;

	//--------------------------------------------------------------------------------------
	//	Function logic
	//--------------------------------------------------------------------------------------

	/* Do a few things here:
	- Check that the owner index on each non-neutral selectable placed on map is low enough
	to be used with the number of PIE clients and CPU players user has specified. Also check
	the owner is of the right faction and isn't an observer.
	- Selectables that fulfill all the  criteria above will have their transform + owner info
	stored. Using this info it will be respawned later, and destroy it for now.
	- If user is playing with CPU players then store how many will start with at least one
	selectable. If they start with 0 then won't bother spawning a CPU player AI controller at
	all since they will have nothing to control */

	/* Make assumption here that actor iterator will return all the actors already placed
	on map */

	// For all actors...
	for (FActorIterator Iter(GetWorld()); Iter; ++Iter)
	{
		AActor * const Actor = *Iter;

		if (Actor->IsA(AStartingGrid::StaticClass()))
		{
			FactionStartingGrids.Emplace(Actor);
			continue;
		}

		// If actor is a selectable...
		if (UKismetSystemLibrary::DoesImplementInterface(Actor, USelectable::StaticClass()))
		{
			/* Going to add selectable to an array so that it will be respawned later and
			destroy it for now. Need to check that the owner index is valid, owner's faction
			is ok and that owner is also not an observer since observers should not own
			any selectables */

			ISelectable * AsSelectable = CastChecked<ISelectable>(Actor);

			/* Whether selectable will be respawned or not */
			EPIEOnMapSelectableSetupResult Result = EPIEOnMapSelectableSetupResult::None;

			bool bIsForCPUPlayer;
			int32 OwnerIndex;

			/* Difficult to tell from source but PostLoad might be where AActor::ParentComponent
			is set which means GetParentComponent() should return correct value */
			if (Actor->GetParentComponent() != nullptr
				&& Actor->GetParentComponent()->IsA(UStartingGridComponent::StaticClass()))
			{
				/* Use starting grids owner index */
				AStartingGrid * Grid = CastChecked<AStartingGrid>(Actor->GetParentComponent()->GetOwner());
				bIsForCPUPlayer = Grid->PIE_IsForCPUPlayer();
				OwnerIndex = bIsForCPUPlayer
					? Grid->PIE_GetCPUOwnerIndex() : Grid->PIE_GetHumanOwnerIndex();
			}
			else
			{
				bIsForCPUPlayer = AsSelectable->PIE_IsForCPUPlayer();

				/* Assign owner as the value set in editor on selectable */
				OwnerIndex = bIsForCPUPlayer
					? AsSelectable->PIE_GetCPUOwnerIndex() : AsSelectable->PIE_GetHumanOwnerIndex();
			}

			UE_CLOG(OwnerIndex == -1, RTSLOG, Fatal, TEXT("Class %s has not overridden "
				"ISelectable::PIE_GetCPUOwnerIndex() and/or ISelectable::PIE_GetHumanOwnerIndex()."),
				*Actor->GetClass()->GetName());

			// -2 is code for neutral which shouldn't have a player state as owner
			if (OwnerIndex == -2)
			{
				/* Check if the selectable is a type we specified as wanting to stay on map. */
				if (GI->ShouldStayOnMap(Actor))
				{
					// Add to array to be have ISelectable::Setup() called on it later
					NeutralSelectables.Emplace(AsSelectable);
				}
				else
				{
					Actor->Destroy();
				}
			}
			else
			{
				/* Now figure out if their owner index is valid */

				if (bIsForCPUPlayer)
				{
					if (OwnerIndex < NumDesiredCPUPlayers && CPUPlayerInfo.IsValidIndex(OwnerIndex))
					{
						if (CPUPlayerInfo[OwnerIndex].GetTeam() != ETeam::Observer)
						{
							// Check if on an ok faction
							if (GI->GetFactionInfo(CPUPlayerInfo[OwnerIndex].GetFaction())->HasSelectable(Actor->GetClass()))
							{
								Result = EPIEOnMapSelectableSetupResult::Success;
								CPUPlayersWithAtLeastOneSelectable.Emplace(OwnerIndex);
							}
							else
							{
								Result = EPIEOnMapSelectableSetupResult::WrongFaction;
							}
						}
						else
						{
							Result = EPIEOnMapSelectableSetupResult::ObserverOwner;
						}
					}
					else
					{
						// Owner index not valid. Either reassign or do not respawn

						if (InvalidCPUOwnerRule == EInvalidOwnerIndexAction::AssignToServerPlayer)
						{
							// For logging
							ReassignedToServerSelectables.Emplace(FPIESelectableLoggingInfo(Actor->GetName(),
								OwnerIndex));

							// Give selectable to server player
							bIsForCPUPlayer = false;
							OwnerIndex = 0;

							// Check if server player is observer
							if (HumanPlayerInfo[OwnerIndex].GetTeam() != ETeam::Observer)
							{
								if (GI->GetFactionInfo(HumanPlayerInfo[OwnerIndex].GetFaction())->HasSelectable(Actor->GetClass()))
								{
									Result = EPIEOnMapSelectableSetupResult::Success;
								}
								else
								{
									Result = EPIEOnMapSelectableSetupResult::WrongFaction;
								}
							}
							else
							{
								Result = EPIEOnMapSelectableSetupResult::ObserverOwner;
							}
						}
						else // Assumed EInvalidOwnerIndexAction::DoNotSpawn
						{
							assert(InvalidCPUOwnerRule == EInvalidOwnerIndexAction::DoNotSpawn);
							
							Result = EPIEOnMapSelectableSetupResult::OwnerIndexTooHigh;
						}
					}
				}
				else
				{
					if (OwnerIndex < NumPIEClients && HumanPlayerInfo.IsValidIndex(OwnerIndex))
					{
						// Check if observer
						if (HumanPlayerInfo[OwnerIndex].GetTeam() != ETeam::Observer)
						{
							// Check on ok faction
							if (GI->GetFactionInfo(HumanPlayerInfo[OwnerIndex].GetFaction())->HasSelectable(Actor->GetClass()))
							{
								Result = EPIEOnMapSelectableSetupResult::Success;
							}
							else
							{
								Result = EPIEOnMapSelectableSetupResult::WrongFaction;
							}
						}
						else
						{
							Result = EPIEOnMapSelectableSetupResult::ObserverOwner;
						}
					}
					else
					{
						// Owner index not valid

						if (InvalidHumanOwnerRule == EInvalidOwnerIndexAction::AssignToServerPlayer)
						{
							// For logging
							ReassignedToServerSelectables.Emplace(FPIESelectableLoggingInfo(Actor->GetName(),
								OwnerIndex));

							// Give selectable to server player
							OwnerIndex = 0;

							// Verify server player is not observer
							if (HumanPlayerInfo[OwnerIndex].GetTeam() != ETeam::Observer)
							{
								// Verify faction ok
								if (GI->GetFactionInfo(HumanPlayerInfo[OwnerIndex].GetFaction())->HasSelectable(Actor->GetClass()))
								{
									Result = EPIEOnMapSelectableSetupResult::Success;
								}
								else
								{
									Result = EPIEOnMapSelectableSetupResult::WrongFaction;
								}
							}
							else
							{
								Result = EPIEOnMapSelectableSetupResult::ObserverOwner;
							}
						}
						else // Assumed EInvalidOwnerIndexAction::DoNotSpawn
						{
							assert(InvalidHumanOwnerRule == EInvalidOwnerIndexAction::DoNotSpawn);
							
							Result = EPIEOnMapSelectableSetupResult::OwnerIndexTooHigh;
						}
					}
				}


				//---------------------------------------------------
				//	Process result
				//---------------------------------------------------
				switch (Result)
				{
				case EPIEOnMapSelectableSetupResult::Success:
				{
					// Add to array for respawning later
					ToBeRespawned.Emplace(FPIEStartingSelectableInfo(Actor, AsSelectable, OwnerIndex, 
						bIsForCPUPlayer));
					break;
				}
				// Below are failure results. Add to array for logging later
				case EPIEOnMapSelectableSetupResult::OwnerIndexTooHigh:
				{
					InvalidOwnerSelectables.Emplace(FPIESelectableLoggingInfo(Actor->GetName(), OwnerIndex));
					break;
				}
				case EPIEOnMapSelectableSetupResult::ObserverOwner:
				{
					ObserverOwnerSelectables.Emplace(FPIESelectableLoggingInfo(Actor->GetName(), OwnerIndex));
					break;
				}
				case EPIEOnMapSelectableSetupResult::WrongFaction:
				{
					WrongFactionSelectables.Emplace(FPIESelectableLoggingInfo(Actor->GetName(), OwnerIndex));
					break;
				}
				default:
				{
					UE_LOG(RTSLOG, Fatal, TEXT("Unexpected result, result was %s"),
						TO_STRING(EPIEOnMapSelectableSetupResult, Result));
					break;
				}
				}

				// Regardless successful or not always destroy actor
				Actor->Destroy();
			}
		}
	}

	/* Can destroy faction starting grids. Needed to let all their CACs be setup first */
	for (const auto & Elem : FactionStartingGrids)
	{
		Elem->Destroy();
	}

	//--------------------------------------------------------------------------------
	//	Just logging
	//--------------------------------------------------------------------------------

	if (ReassignedToServerSelectables.Num() > 0)
	{
		UE_LOG(RTSLOG, Log, TEXT("The following selectables already placed on map had owner indices "
			"too high and were instead assigned to the server player:"));

		for (const auto & Elem : ReassignedToServerSelectables)
		{
			UE_LOG(RTSLOG, Log, TEXT("%s with owner index %d"), *Elem.Name, Elem.OwnerIndex);
		}
	}

	if (InvalidOwnerSelectables.Num() > 0)
	{
		UE_LOG(RTSLOG, Warning, TEXT("The following selectables already placed on map had owner "
			"indices too high and were removed from PIE/standalone play: "));

		for (const auto & Elem : InvalidOwnerSelectables)
		{
			UE_LOG(RTSLOG, Warning, TEXT("%s with owner index %d"), *Elem.Name, Elem.OwnerIndex);
		}
	}

	if (ObserverOwnerSelectables.Num() > 0)
	{
		UE_LOG(RTSLOG, Warning, TEXT("The following selectables already placed on map had an owner "
			"who is a match observer and therefore were removed from PIE/standalone play: "));

		for (const auto & Elem : ObserverOwnerSelectables)
		{
			UE_LOG(RTSLOG, Warning, TEXT("%s with owner index %d"), *Elem.Name, Elem.OwnerIndex);
		}
	}

	if (WrongFactionSelectables.Num() > 0)
	{
		UE_LOG(RTSLOG, Warning, TEXT("The following selectables already placed on map were assigned "
			"to an owner with a faction that does not have them on its building/unit roster. "
			"Therefore they have been removed from PIE/standalone play:"));

		for (const auto & Elem : WrongFactionSelectables)
		{
			UE_LOG(RTSLOG, Warning, TEXT("%s with owner index %d"),
				*Elem.Name, Elem.OwnerIndex);
		}
	}

	// Go on to next stage
	GoToMapFromStartupPartZeroPointFive(CPUPlayersWithAtLeastOneSelectable);
}

void ARTSGameMode::GoToMapFromStartupPartZeroPointFive(const TSet < int32 > & CPUPlayersWithAtLeastOneSelectable)
{
	/* Compress the teams to lowest enum values possible e.g. if match will have Team1, Team3
	and Team4 then change these to Team1, Team2 and Team3 respecively */

	TArray <ETeam> NoDuplicateTeams;

	for (const auto & Elem : GI->EditorPlaySession_GetHumanPlayerInfo())
	{
		if (Elem.GetTeam() != ETeam::Observer)
		{
			NoDuplicateTeams.AddUnique(Elem.GetTeam());
		}
	}
	for (const auto & Elem : GI->EditorPlaySession_GetCPUPlayerInfo())
	{
		if (Elem.GetTeam() != ETeam::Observer)
		{
			NoDuplicateTeams.AddUnique(Elem.GetTeam());
		}
	}

	NoDuplicateTeams.Sort();
	TMap <ETeam, ETeam> NewTeams;
	NewTeams.Reserve(NoDuplicateTeams.Num() + 1);
	NewTeams.Emplace(ETeam::Observer, ETeam::Observer);
	for (uint8 i = 0; i < NoDuplicateTeams.Num(); ++i)
	{
		NewTeams.Emplace(NoDuplicateTeams[i], Statics::ArrayIndexToTeam(i));
	}

	/* NewTeams now contains mapping from old team to new correct 'lowest value possible' value
	team */

	// The -1 is to remove observer team
	int32 NumPIETeams = NewTeams.Num() - 1;

	/* Recently added this here 26/02/19. Make it so there are at least 2 teams. This really 
	needs to happen although maybe I can see if it's ok now to do without it */
	if (NumPIETeams < 2)
	{
		NumPIETeams = 2;
	}

	// Recently added this here 26/02/19
	GS->SetupForMatch(GI->GetPoolingManager(), NumPIETeams);

	/* Spawn the CPU players if any */

	const int32 NumDesiredCPUPlayers = GI->EditorPlaySession_GetNumCPUPlayers();
	const TArray <FPIECPUPlayerInfo> & CPUPlayerInfo = GI->EditorPlaySession_GetCPUPlayerInfo();

	// Construct resource array that SpawnCPUPlayer needs. Use what's defined in development settings
	const TArray <int32> & ResourceArray = GI->GetStartingResourceConfig(EStartingResourceAmount::DevelopmentSettings).GetResourceArraySlow();

	assert(NumPIECPUPlayers == 0);

	// For logging
	int32 NumCPUPlayersAvoidedSpawning = 0;

	for (int32 i = 0; i < NumDesiredCPUPlayers; ++i)
	{
		/* Check at least one selectable was placed on map for this player */
		if (CPUPlayersWithAtLeastOneSelectable.Contains(i))
		{
			const ECPUDifficulty Difficulty = CPUPlayerInfo[i].GetDifficulty();
			const ETeam Team = NewTeams[CPUPlayerInfo[i].GetTeam()];
			const EFaction Faction = CPUPlayerInfo[i].GetFaction();
			// This player start is not final and can be changed during SetupMatchInfoForPIE
			const int16 PlayerStart = CPUPlayerInfo[i].GetStartingSpot();

			ACPUPlayerAIController * CPUPlayer = GI->SpawnCPUPlayer(Difficulty, Team, Faction, 
				PlayerStart, ResourceArray);

			NumPIECPUPlayers++;
		}
		else
		{
			/* No selectables for this CPU were placed on map for them. Therefore do not spawn
			any CPU AI controller since they will have nothing to control. If game is the
			type of game where players can start with nothing then taking this branch was the
			wrong thing to do */

			NumCPUPlayersAvoidedSpawning++;
		}
	}

	UE_CLOG(NumCPUPlayersAvoidedSpawning > 0, RTSLOG, Warning, TEXT("Request to spawn %d CPU players "
		"but only %d were spawned due to some not having at least one selectable belonging to them "
		"already placed on map"), NumDesiredCPUPlayers, NumDesiredCPUPlayers - NumCPUPlayersAvoidedSpawning);

	GoToMapFromStartupPartZeroPointSevenFive(NumPIETeams, NewTeams);
}

void ARTSGameMode::GoToMapFromStartupPartZeroPointSevenFive(int32 InNumTeams, const TMap < ETeam, ETeam > & NewTeamsMap)
{
	//================================================================================
	//	Polling stage - check all player states have spawned, both human and CPU
	//================================================================================

	TArray <AActor *> PlayerStates;
	UGameplayStatics::GetAllActorsOfClass(this, ARTSPlayerState::StaticClass(), PlayerStates);

	assert(PlayerStates.Num() <= NumPIEClients + NumPIECPUPlayers);

	static int32 NumTimesPolled = 0;

	if (PlayerStates.Num() < NumPIEClients + NumPIECPUPlayers)
	{
		NumTimesPolled++;
		FTimerDelegate TimerDel;
		FTimerHandle DummyTimerHandle;
		static const FName FuncName = GET_FUNCTION_NAME_CHECKED(ARTSGameMode, GoToMapFromStartupPartZeroPointSevenFive);
		TimerDel.BindUFunction(this, FuncName, InNumTeams, NewTeamsMap);
		GetWorldTimerManager().SetTimer(DummyTimerHandle, TimerDel, 0.01f, false);
		return;
	}

	UE_CLOG(NumTimesPolled > 0, RTSLOG, Log, TEXT("Number of times polling for all player states to "
		"be valid: %d"), NumTimesPolled);
	NumTimesPolled = 0;

	// Polling complete. Move on to next step
	GoToMapFromStartupPart1(InNumTeams, NewTeamsMap);
}

void ARTSGameMode::GoToMapFromStartupPart1(int32 InNumTeams, const TMap <ETeam, ETeam> & NewTeamsMap)
{
	//================================================================================
	//	Polling stage - check all player controllers have spawned
	//================================================================================
	//	Check logs to confirm this is necessary now that PS polling is done earlier

	TArray <AActor *> PlayCons;
	UGameplayStatics::GetAllActorsOfClass(this, ARTSPlayerController::StaticClass(), PlayCons);

	// Shouldn't be more PCs than we expect
	assert(PlayCons.Num() <= NumPIEClients);

	static int32 NumTimesPolled = 0;

	if (PlayCons.Num() < NumPIEClients)
	{
		/* Not all PCs are spawned, try again soon */

		FTimerDelegate TimerDel;
		FTimerHandle DummyTimerHandle;
		static const FName FuncName = GET_FUNCTION_NAME_CHECKED(ARTSGameMode, GoToMapFromStartupPart1);
		TimerDel.BindUFunction(this, FuncName, InNumTeams, NewTeamsMap);
		GetWorldTimerManager().SetTimer(DummyTimerHandle, TimerDel, 0.01f, false);
		NumTimesPolled++;
		return;
	}

	UE_CLOG(NumTimesPolled > 0, RTSLOG, Log, TEXT("Number of times player controllers weren't ready yet: %d"), NumTimesPolled);
	NumTimesPolled = 0;

	//================================================================================
	//	Part of function that does stuff
	//================================================================================

	/* If here then all player controllers are spawned and we can continue */

	TArray <ARTSPlayerController *> PlayerControllers;
	PlayerControllers.Reserve(PlayCons.Num());

	TArray <AActor *> PlayerStates;
	UGameplayStatics::GetAllActorsOfClass(this, ARTSPlayerState::StaticClass(), PlayerStates);

	/* Now sort the player state array so that player controllers players states are first
	and in order, then CPU player states in order. */
	int32 Index = 0;
	int32 NumSwaps = 0;
	/* Assuming here that PC iterator is in "correct" order i.e. first element is host, next
	is 1st remote client etc */
	for (const auto & Elem : PlayCons)
	{
		ARTSPlayerController * PlayCon = CastChecked<ARTSPlayerController>(Elem);

		if (PlayCon->PlayerState != PlayerStates[Index])
		{
			/* Got to reorder */

			int32 InnerLoopIndex = Index + 1;
			NumSwaps++;
			while (InnerLoopIndex < PlayerStates.Num())
			{
				if (PlayCon->PlayerState == PlayerStates[InnerLoopIndex])
				{
					PlayerStates.SwapMemory(Index, InnerLoopIndex);
					break;
				}

				InnerLoopIndex++;
			}
		}

		Index++;

		PlayerControllers.Emplace(PlayCon);
	}

	UE_CLOG(NumSwaps > 0, RTSLOG, Log, TEXT("Player controller and player state arrays were out "
		"out of order, number of swaps required: %d"), NumSwaps);

	/* Now sort CPU player portion of array */
	NumSwaps = 0;
	for (TActorIterator <ACPUPlayerAIController> Iter(GetWorld()); Iter; ++Iter)
	{
		ACPUPlayerAIController * Elem = *Iter;

		if (Elem->PlayerState != PlayerStates[Index])
		{
			// Got to reorder

			int32 InnerLoopIndex = Index + 1;
			NumSwaps++;
			while (InnerLoopIndex < PlayerStates.Num())
			{
				if (Elem->PlayerState == PlayerStates[InnerLoopIndex])
				{
					PlayerStates.SwapMemory(Index, InnerLoopIndex);
					break;
				}

				InnerLoopIndex++;
			}
		}

		Index++;
	}

	UE_CLOG(NumSwaps > 0, RTSLOG, Log, TEXT("CPU player player states were out of order, number "
		"of swaps required: %d "), NumSwaps);

	/* Now PlayerStates should contain player states in order with player controller states
	first followed by CPU player states */

	TArray <ARTSPlayerState *> CastedStates;
	CastedStates.Reserve(PlayerStates.Num());
	for (int32 i = 0; i < PlayerStates.Num(); ++i)
	{
		CastedStates.Emplace(CastChecked<ARTSPlayerState>(PlayerStates[i]));
	}

	int32 NumPIETeams = InNumTeams;

	/* Make sure there are at least 2 teams. This really needs to happen. Haven't tested it in a 
	while though maybe can get away with only 1 team */
	if (NumPIETeams < 2)
	{
		NumPIETeams = 2;
	}

	/* Create a dummy match info that contains most information */
	GI->SetupMatchInfoForPIE(NumPIETeams, CastedStates, NewTeamsMap);

	/* Call AGameMode func to spawn player pawns, but we're nowhere near starting the game for 
	real yet */
	StartMatch();

	GoToMapFromStartupPart2(PlayerControllers, NumPIETeams, CastedStates);
}

void ARTSGameMode::GoToMapFromStartupPart2(const TArray<ARTSPlayerController*>& PlayerControllers,
	int32 InNumTeams, const TArray <ARTSPlayerState *> & PlayerStates)
{
	/* Setup player controller important stuff */
	for (int32 i = 0; i < PlayerControllers.Num(); ++i)
	{
		ARTSPlayerController * const PlayCon = PlayerControllers[i];

		/* Assign team and faction based on what user defined in development settings */
		const ETeam Team = GI->GetMatchInfo().GetPlayers()[i].Team;
		const EFaction Faction = GI->GetMatchInfo().GetPlayers()[i].Faction;
		const int16 StartingSpot = GI->GetMatchInfo().GetPlayers()[i].StartingSpotID;

		PlayCon->Server_SetupForMatch(GS->GenerateUniquePlayerID(), Team, Faction,
			EStartingResourceAmount::DevelopmentSettings, InNumTeams, 
			GI->GetMatchInfo().GetMapUniqueID(), StartingSpot);
	}

	/* Setup the player states for the CPU players. This is stuff that would normally be
	done in PC::Client_SetupForMatch. This feels kind of weird doing it here. Can probably
	be done in PS::Server_SetInitialValues for bots */
	for (int32 i = NumPIEClients; i < PlayerStates.Num(); ++i)
	{
		ARTSPlayerState * CPUPlayerState = PlayerStates[i];

		CPUPlayerState->SetFactionInfo(GI->GetFactionInfo(CPUPlayerState->GetFaction()));
		CPUPlayerState->SetupProductionCapableBuildingsMap();
		CPUPlayerState->SetTeamTag(GS->GetTeamTag(CPUPlayerState->GetTeam()));
	}

	GoToMapFromStartupPart3(PlayerStates);
}

void ARTSGameMode::GoToMapFromStartupPart3(const TArray <ARTSPlayerState *> & PlayerStates)
{	
	//================================================================================
	//	Polling stage - check all PCs have acked Client_SetupForMatch has completed
	//================================================================================

	static int32 NumTimesPolled = 0;

	assert(GS->GetNumPCSetupAcksForPIE() <= NumPIEClients)

	if (GS->GetNumPCSetupAcksForPIE() < NumPIEClients)
	{
		FTimerDelegate TimerDel;
		FTimerHandle DummyTimerHandle;
		static const FName FuncName = GET_FUNCTION_NAME_CHECKED(ARTSGameMode, GoToMapFromStartupPart3);
		TimerDel.BindUFunction(this, FuncName, PlayerStates);
		GetWorldTimerManager().SetTimer(DummyTimerHandle, TimerDel, 0.01f, false);
		NumTimesPolled++;
		return;
	}

	UE_CLOG(NumTimesPolled > 0, RTSLOG, Log, TEXT("Number of times PC's own player state had not repped: %d"),
		NumTimesPolled);
	NumTimesPolled = 0;

	//================================================================================
	//	Part of function that does stuff
	//================================================================================

	/* Next need to make sure all clients have all players player state variables made up to date */
	for (const auto & Elem : PlayerStates)
	{
		Elem->Multicast_SetInitialValues(Elem->GetPlayerIDAsInt(), Elem->GetPlayerID(), 
			Elem->GetTeam(), Elem->GetFaction());
	}

	GoToMapFromStartupPart4(PlayerStates);
}

void ARTSGameMode::GoToMapFromStartupPart4(const TArray<ARTSPlayerState*>& PlayerStates)
{
	//================================================================================
	//	Polling stage - check all PS::Multicast_SetInitialValues have been acked
	//================================================================================

	static int32 NumTimesPSVariablesHadntRepped = 0;

	const int32 NumAcksRequired = (NumPIEClients + NumPIECPUPlayers) * NumPIEClients;

	// Make sure haven't gone over number of acks expected
	assert(GS->GetNumPSSetupAcksForPIE() <= NumAcksRequired);

	if (GS->GetNumPSSetupAcksForPIE() < NumAcksRequired)
	{
		FTimerDelegate TimerDel;
		FTimerHandle DummyTimerHandle;
		static const FName FuncName = GET_FUNCTION_NAME_CHECKED(ARTSGameMode, GoToMapFromStartupPart4);
		TimerDel.BindUFunction(this, FuncName, PlayerStates);
		GetWorldTimerManager().SetTimer(DummyTimerHandle, TimerDel, 0.01f, false);
		NumTimesPSVariablesHadntRepped++;
		return;
	}

	UE_CLOG(NumTimesPSVariablesHadntRepped > 0, RTSLOG, Log, TEXT("Number of times needed to wait "
		"on all PS variables to replicate: %d"), NumTimesPSVariablesHadntRepped);
	NumTimesPSVariablesHadntRepped = 0;

	//================================================================================
	//	Part of function that does stuff
	//================================================================================

	/* Now do setup that requires all info about all players to be known */
	for (const auto & Elem : PlayerStates)
	{
		Elem->Client_FinalSetup();
	}

	GoToMapFromStartupPart5(PlayerStates);
}

void ARTSGameMode::GoToMapFromStartupPart5(const TArray<ARTSPlayerState*>& PlayerStates)
{
	//================================================================================
	//	Polling stage - check all PS::Client_FinalSetup have been acked
	//================================================================================

	assert(GS->GetNumFinalSetupAcks() <= NumPIEClients);
	if (GS->GetNumFinalSetupAcks() != NumPIEClients)
	{
		FTimerDelegate TimerDel;
		FTimerHandle DummyTimerHandle;
		static const FName FuncName = GET_FUNCTION_NAME_CHECKED(ARTSGameMode, GoToMapFromStartupPart5);
		TimerDel.BindUFunction(this, FuncName, PlayerStates);
		GetWorldTimerManager().SetTimer(DummyTimerHandle, TimerDel, 0.01f, false);
		return;
	}

	//================================================================================
	//	Part of function that does stuff
	//================================================================================

	/* Setup all neutral selectables now */
	for (const auto & Elem : NeutralSelectables)
	{
		Elem->Setup();
	}

	/* Maps player to the selectables they started match with. Used by CPU players only */
	TMap < ARTSPlayerState *, FStartingSelectables > CPUStartingSelectables;

	/* Add entry for each CPU player */
	CPUStartingSelectables.Reserve(NumPIECPUPlayers);
	for (int32 i = 0; i < NumPIECPUPlayers; ++i)
	{
		/* Have not tested whether key is correct */
		CPUStartingSelectables.Emplace(PlayerStates[NumPIEClients + i], FStartingSelectables());
	}

	/* For logging purposes */
	int32 NumNotSpawnedBecauseAtSelectableCap = 0;
	int32 NumNotSpawnedBecauseOfUniqueSelectableCap = 0;

	/* Respawn all the destroyed selectables now */
	for (const auto & Elem : ToBeRespawned)
	{
		ARTSPlayerState * SelectablesOwner = nullptr;
		if (Elem.IsForCPUPlayer())
		{
			// Make sure to not use a human player state
			const int32 OwnerIndex = NumPIEClients + Elem.GetOwnerIndex();
			
			SelectablesOwner = PlayerStates[OwnerIndex];
		}
		else
		{
			SelectablesOwner = PlayerStates[Elem.GetOwnerIndex()];
		}

		if (SelectablesOwner->IsAtSelectableCap(false, false))
		{
			/* Player is at the selectable cap so we will decide not to spawn the selectable. 
			Note down that this occured for logging purposes */
			NumNotSpawnedBecauseAtSelectableCap++;

			continue;
		}
		
		if (Elem.IsForBuilding())
		{
			if (SelectablesOwner->IsAtUniqueBuildingCap(Elem.GetBuildingType(), false, false))
			{
				NumNotSpawnedBecauseOfUniqueSelectableCap++;

				continue;
			}
		}
		else // Assuming for unit
		{
			assert(Elem.IsForUnit());
		
			if (SelectablesOwner->IsAtUniqueUnitCap(Elem.GetUnitType(), false, false))
			{
				NumNotSpawnedBecauseOfUniqueSelectableCap++;

				continue;
			}
		}
			
		// Using IsChildOf to distinguish between buildings and units
		if (Elem.GetActorBP()->IsChildOf(ABuilding::StaticClass()))
		{
			AStartingGrid::SpawnStartingBuilding(GS, GetWorld(), Elem.GetSpawnTransform(),
				SelectablesOwner->GetFI()->GetBuildingInfoSlow(Elem.GetActorBP()), SelectablesOwner);

			if (Elem.IsForCPUPlayer())
			{
				/* Note down what we spawned for CPU player AI controller for behavior purposes */
				CPUStartingSelectables[SelectablesOwner].AddStartingSelectable(Elem.GetBuildingType());
			}
		}
		else // Assumed infantry
		{
			AStartingGrid::SpawnStartingInfantry(GS, GetWorld(), Elem.GetSpawnTransform(),
				Elem.GetActorBP(), SelectablesOwner);

			if (Elem.IsForCPUPlayer())
			{
				/* Note down what we spawned for CPU player AI controller for behavior purposes */
				CPUStartingSelectables[SelectablesOwner].AddStartingSelectable(Elem.GetUnitType());
			}
		}
	}

	// Do some logging
	if (NumNotSpawnedBecauseAtSelectableCap > 0)
	{
		UE_LOG(RTSLOG, Warning, TEXT("On bringing up PIE/standalone some selectables already placed "
			"on the map were not spawned because their owner was at the selectable cap. Num not spawned: %d"),
			NumNotSpawnedBecauseAtSelectableCap);
	}
	if (NumNotSpawnedBecauseOfUniqueSelectableCap > 0)
	{
		UE_LOG(RTSLOG, Warning, TEXT("On bringing up PIE/standalone some selectables already placed "
			"on the map were not spawned because their owner was at a \"unique selectable cap\""));
	}

	// Force Garbage Collection?

	GS->StartPIEMatch(CPUStartingSelectables);
}
#endif // WITH_EDITOR

void ARTSGameMode::ShowIntro()
{
	OnIntroFinished();
}

void ARTSGameMode::OnIntroFinished()
{
	GI->OnEnterMainMenuFromStartup();
}

void ARTSGameMode::Delay(FTimerHandle & TimerHandle, void(ARTSGameMode::* Function)(), float Delay)
{
	assert(Delay > 0.f);
	GetWorldTimerManager().SetTimer(TimerHandle, this, Function, Delay, false);
}


//--------------------------------------------------
//	Defeat conditions
//--------------------------------------------------

bool ARTSGameMode::IsPlayerDefeated(const ARTSPlayerState * Player) const
{
	/* Call function pointed to by function pointer */
	return (this->* (DefeatFunctions[Statics::DefeatConditionToArrayIndex(DefeatCondition)]))(Player);
}

void ARTSGameMode::SetupDefeatFunctions()
{
	EDefeatCondition DefeatCondish;
	
	DefeatCondish = EDefeatCondition::NoCondition;
	DefeatFunctions[Statics::DefeatConditionToArrayIndex(DefeatCondish)] = &ARTSGameMode::DefeatFunction_NoCondition;

	DefeatCondish = EDefeatCondition::AllBuildingsDestroyed;
	DefeatFunctions[Statics::DefeatConditionToArrayIndex(DefeatCondish)] = &ARTSGameMode::DefeatFunction_AllBuildingsDestroyed;
}

bool ARTSGameMode::DefeatFunction_NoCondition(const ARTSPlayerState * Player) const
{
	// Player is never defeated when using this rule
	return false;
}

bool ARTSGameMode::DefeatFunction_AllBuildingsDestroyed(const ARTSPlayerState * Player) const
{
	assert(Player->GetBuildings().Num() >= 0);

	// Player defeated if they have no buildings remaining
	return Player->GetBuildings().Num() == 0;
}

void ARTSGameMode::StartDefeatConditionChecking(EDefeatCondition MatchDefeatCondition)
{
	/* Make sure we aren't already checking for defeat conditions, not expected to be */
	assert(!GetWorldTimerManager().IsTimerActive(TimerHandle_DefeatCondition));

	DefeatCondition = MatchDefeatCondition;

	/* Check for defeated players straight away */
	CheckDefeatCondition();
}

void ARTSGameMode::StopDefeatConditionChecking()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_DefeatCondition);
}

void ARTSGameMode::CheckDefeatCondition()
{
	// Holds players that go from undefeated to defeated this check
	TArray < ARTSPlayerState * > DefeatedThisCheck;

	// For each non-defeated player...
	for (int32 i = GS->GetUndefeatedPlayers().Num() - 1; i >= 0; --i)
	{
		ARTSPlayerState * Elem = GS->GetUndefeatedPlayers()[i];

		// Check if they are now defeated
		if (IsPlayerDefeated(Elem))
		{
			/* Remove player from non defeated array and add to defeated array */
			GS->GetUndefeatedPlayers().RemoveAt(i, 1, false);
			DefeatedThisCheck.Emplace(Elem);
		}
	}

	/* If number of defeated players has changed this check then we need to see if a team has
	won the match */
	if (DefeatedThisCheck.Num() > 0)
	{
		TSet < ETeam > WinningTeams;

		/* Check if match should end */
		if (GS->IsMatchNowOver(DefeatedThisCheck, WinningTeams))
		{
			GS->OnMatchWinnerFound(WinningTeams);
			return;
		}
		else
		{
			/* If here then match is still going. Send a notification out to all players that a
			player has been defeated */
			GS->Multicast_OnPlayersDefeated(DefeatedThisCheck);
		}
	}

	// Check again soon
	Delay(TimerHandle_DefeatCondition, &ARTSGameMode::CheckDefeatCondition,
		DefeatConditionCheckRate);
}


