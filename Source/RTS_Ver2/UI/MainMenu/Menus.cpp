// Fill out your copyright notice in the Description page of Project Settings.

#include "Menus.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Public/TimerManager.h"
#include "Animation/WidgetAnimation.h"
#include "GameFramework/PlayerStart.h"
#include "Online.h"
#include "EngineUtils.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"

#include "GameFramework/RTSGameInstance.h"
#include "GameFramework/FactionInfo.h"
#include "UI/MainMenu/MainMenuWidgetBase.h"
#include "UI/MainMenu/Lobby.h"
#include "GameFramework/RTSGameMode.h"
#include "GameFramework/RTSGameState.h"
#include "GameFramework/RTSPlayerController.h"
#include "Miscellaneous/CPUPlayerAIController.h"
#include "GameFramework/RTSPlayerState.h"
#include "Networking/RTSGameSession.h"
#include "Statics/DevelopmentStatics.h"
#include "Settings/DevelopmentSettings.h"
#include "UI/MainMenu/PopupWidget.h"


const FText Menus::PALETTE_CATEGORY = NSLOCTEXT("Widgets", "Palette Category", "RTS");

/* This variable cost me two weeks. If 3rd param is just ""
then only in packaged games game will crash the moment the game is 
started - no logs or anything. Editor PIE/standalone works fine however */
const FText Menus::BLANK_TEXT = FText::GetEmpty();//NSLOCTEXT("Your Namespace", "Blank Text", "");


URTSGameInstance * Menus::GetGameInstance(const UWorld * World)
{
	return World->GetGameInstance<URTSGameInstance>();
}

ARTSGameState * Menus::GetGameState(const UWorld * World)
{
	return CastChecked<ARTSGameState>(World->GetGameState());
}


//================================================================================================
//	Widget Functionality
//================================================================================================

void URTSGameInstance::ShowWidget(EWidgetType WidgetType, ESlateVisibility NewCurrentWidgetsVisibility)
{
	/* TODO: stuff to look at before finishing anims: */
	/* Maybe look into this  func and double check it is setting visibility  correctly */

	/* Only either hiding or making uninteractable are expected for current widget */
	assert(NewCurrentWidgetsVisibility == ESlateVisibility::Collapsed
		|| NewCurrentWidgetsVisibility == ESlateVisibility::HitTestInvisible);

	UMainMenuWidgetBase * const CurrentWidget = GetCurrentWidget();
	// Check because could be first widget shown
	if (CurrentWidget != nullptr)
	{
		/* Make uninteractable straight away */
		CurrentWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	WidgetHistory.Push(WidgetType);

	WidgetToShow = WidgetType;
	NumWidgetsBack = 0;

	if (NewCurrentWidgetsVisibility == ESlateVisibility::HitTestInvisible)
	{
		PlayWidgetEnterAnim(WidgetType);
	}
	else if (NewCurrentWidgetsVisibility == ESlateVisibility::Collapsed)
	{
		if (CurrentWidget != nullptr)
		{
			const EWidgetType CurrentWidgetType = WidgetHistory.Last(1);
			PlayWidgetExitAnim(CurrentWidgetType);
		}
		else // Very first widget being shown
		{
			PlayWidgetEnterAnim(WidgetType);
		}
	}
}

void URTSGameInstance::ShowPreviousWidget(int32 NumToGoBack)
{
	assert(WidgetHistory.Num() > 0);

	/* 0 is code for 'return to the very first widget in WidgetHistory' */
	if (NumToGoBack == 0)
	{
		NumToGoBack = WidgetHistory.Num() - 1;
	}

	NumWidgetsBack = NumToGoBack;

	if (NumWidgetsBack == 0)
	{
		/* Already showing first widget */
		return;
	}

	WidgetToShow = WidgetHistory.Last(NumToGoBack);
	const EWidgetType CurrentWidgetType = WidgetHistory.Last();

	/* Make uninteractive */
	GetCurrentWidget()->SetVisibility(ESlateVisibility::HitTestInvisible);

	PlayWidgetExitAnim(CurrentWidgetType);
}

bool URTSGameInstance::Internal_HideWidget(EWidgetType WidgetType)
{
	if (Widgets.Contains(WidgetType) && Widgets[WidgetType] != nullptr)
	{
		UMainMenuWidgetBase * const Widget = Widgets[WidgetType];
		if (Widget->IsInViewport())
		{
			Widget->SetVisibility(ESlateVisibility::Collapsed);
			return true;
		}
	}

	return false;
}

void URTSGameInstance::HideAllMenuWidgets()
{
	// Hide popup widget
	HidePopupWidget();
	
	// Hide menu widgets
	for (const auto & Elem : Widgets)
	{
		UMainMenuWidgetBase * const Widget = Elem.Value;
		if (Widget != nullptr)
		{
			Internal_HideWidget(Elem.Key);
		}
	}

	WidgetHistory.Empty();
}

void URTSGameInstance::RemoveFromWidgetHistory(EWidgetType WidgetType)
{
	WidgetHistory.RemoveSingle(WidgetType);
}

void URTSGameInstance::DestroyMenuWidgetsBeforeMatch()
{
	for (auto Iter = Widgets.CreateConstIterator(); Iter; ++Iter)
	{
		if (Iter.Key() != EWidgetType::LoadingScreen)
		{
			UMainMenuWidgetBase * Widg = Widgets.FindAndRemoveChecked(Iter.Key());
			
			/* Don't actually know if this is the best option for performance */
			Widg->RemoveFromViewport();
		}
	}

	/* These 2 lines probably aren't necessary - history gets cleared later but do it in case user
	wants to navigate back to loading screen at sometime during match init */
	WidgetHistory.Empty();
	WidgetHistory.Emplace(EWidgetType::LoadingScreen);

	//GEngine->ForceGarbageCollection(true);
}

UMainMenuWidgetBase * URTSGameInstance::GetWidget(EWidgetType WidgetType)
{
	if (Widgets.Contains(WidgetType))
	{
		assert(Widgets[WidgetType] != nullptr);
		return Widgets[WidgetType];
	}
	else
	{
		/* Need to construct widget first */

		UE_CLOG(Widgets_BP[WidgetType] == nullptr, RTSLOG, Fatal, TEXT("Request to construct a %s "
			"widget but blueprint is null in game instance; need to set a blueprint"),
			TO_STRING(EWidgetType, WidgetType));

		UMainMenuWidgetBase * Widget = CreateWidget<UMainMenuWidgetBase>(this, Widgets_BP[WidgetType]);
		Widget->Setup();
		Widgets.Emplace(WidgetType, Widget);

		return Widget;
	}
}

UMainMenuWidgetBase * URTSGameInstance::GetCurrentWidget()
{
	return WidgetHistory.Num() > 0 ? GetWidget(WidgetHistory.Last()) : nullptr;
}

uint8 URTSGameInstance::GetZOrder(EWidgetType WidgetType)
{
	return static_cast<uint8>(WidgetType);
}

void URTSGameInstance::PlayWidgetExitAnim(EWidgetType WidgetType)
{
	// Hide the popup widget if showing
	HidePopupWidget();
	
	UMainMenuWidgetBase * const Widget = GetWidget(WidgetType);

	UWidgetAnimation * ExitAnim = nullptr;
	if (Widget->UsesOneTransitionAnimation())
	{
		ExitAnim = Widget->GetTransitionAnimation();
		/* When using one anim for both exit and enter only when the animation has completely
		finished can it be played again */
		assert(!Widget->IsAnimationPlaying(ExitAnim));

		Widget->PlayAnimation(ExitAnim);
	}
	else if (Widget->GetExitAnimation() != nullptr)
	{
		ExitAnim = Widget->GetExitAnimation();
		assert(!Widget->IsAnimationPlaying(ExitAnim));

		/* Check if enter anim is playing and if so stop it and play this anim starting part
		way through */
		UWidgetAnimation * const EnterAnim = Widget->GetEnterAnimation();
		if (Widget->IsAnimationPlaying(EnterAnim))
		{
			/* If enter anim is playing then play this animation part way through. Who knows if
			it will line up */
			const float CurrentTime = Widget->GetAnimationCurrentTime(EnterAnim);
			const float PercentageThroughCurrentAnim = CurrentTime / (EnterAnim->GetEndTime() - EnterAnim->GetStartTime());
			const float ExitAnimLength = ExitAnim->GetEndTime() - ExitAnim->GetStartTime();
			const float StartTime = (1.f - PercentageThroughCurrentAnim) * ExitAnimLength;
			Widget->StopAnimation(EnterAnim);

			/* Make uninteractable since may have been made interactable when EnterAnim finished */
			Widget->SetVisibility(ESlateVisibility::HitTestInvisible);

			Widget->PlayAnimation(ExitAnim, StartTime);
		}
		else
		{
			Widget->PlayAnimation(ExitAnim);
		}
	}
	else // Does not have exit anim
	{
		OnWidgetExitAnimFinished(Widget);
	}
}

void URTSGameInstance::OnWidgetExitAnimFinished(UMainMenuWidgetBase * Widget)
{
	Widget->SetVisibility(ESlateVisibility::Collapsed);

	while (NumWidgetsBack > 0)
	{
		NumWidgetsBack--;

		assert(WidgetHistory.Num() > 0);

		const EWidgetType CurrentWidgetType = WidgetHistory.Last();

		/* Hide current widget */
		const bool bResult = Internal_HideWidget(CurrentWidgetType);
		assert(bResult);

		WidgetHistory.Pop();
	}

	if (WidgetTransitionFunction != nullptr)
	{
		// Call function pointed to by function pointer 
		(this->* (WidgetTransitionFunction))();

		WidgetTransitionFunction = nullptr;

		/* This WidgetTransition function should only really be used when changing map. But for
		some reason the widget doesn't show from PlayWidgetEnterAnim after changing map. So return
		here and have to show it manually elsewhere */
		return;
	}

	PlayWidgetEnterAnim(WidgetToShow);
}

void URTSGameInstance::PlayWidgetEnterAnim(EWidgetType WidgetType)
{
	UMainMenuWidgetBase * Widget = GetWidget(WidgetType);

	Widget->OnShown();

	assert(!Widget->IsVisible());

	if (!Widget->IsInViewport())
	{
		const uint8 ZOrder = GetZOrder(WidgetToShow);
		Widget->AddToViewport(ZOrder);
		Widget->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (Widget->GetVisibility() == ESlateVisibility::Collapsed)
	{
		UWidgetAnimation * EnterAnim = nullptr;
		if (Widget->UsesOneTransitionAnimation())
		{
			/* Make uninteractable straight away. Will be made interactive again when animation
			finishes */
			Widget->SetVisibility(ESlateVisibility::HitTestInvisible);

			EnterAnim = Widget->GetTransitionAnimation();
			assert(!Widget->IsAnimationPlaying(EnterAnim));

			Widget->PlayAnimation(EnterAnim, 0.f, 1, EUMGSequencePlayMode::Reverse);
		}
		else if (Widget->GetEnterAnimation() != nullptr)
		{
			EnterAnim = Widget->GetEnterAnimation();

			/* Made uninteractable straight away. User can make widget interactive partway through
			animation. Widget will also be made interative when animation finishes */
			Widget->SetVisibility(ESlateVisibility::HitTestInvisible);

			Widget->PlayAnimation(EnterAnim);
		}
		else
		{
			/* No enter animation so make visible straight away */
			Widget->SetVisibility(ESlateVisibility::Visible);
		}
	}
	else
	{
		/* Already in viewport but hit test invisible so just now make it interactive */
		Widget->SetVisibility(ESlateVisibility::Visible);
	}
}

void URTSGameInstance::OnWidgetEnterAnimFinished(UMainMenuWidgetBase * Widget)
{
	/* Using one transition animiation requires this. If using seperate exit/enter anims then
	visibility can be set part way through anim by user. Regardless if it has not been made
	interactive by the end of the animation then it is assumed it should be */
	Widget->SetVisibility(ESlateVisibility::Visible);
}

bool URTSGameInstance::IsWidgetBlueprintSet(EWidgetType WidgetType) const
{
	return Widgets_BP.Contains(WidgetType) && Widgets_BP[WidgetType] != nullptr;
}

void URTSGameInstance::Delay(FTimerHandle & TimerHandle, void(URTSGameInstance::* Function)(), float Delay)
{
	assert(Delay > 0.f);
	GetTimerManager().SetTimer(TimerHandle, this, Function, Delay, false);
}

void URTSGameInstance::ShowPopupWidget(const FText & Message, float Duration, bool bSpecifyTextColor,
	const FLinearColor & TextColor)
{
	if (PopupWidget_BP != nullptr)
	{
		if (PopupWidget == nullptr)
		{
			// Need to create widget first 

			PopupWidget = CreateWidget<UPopupWidget>(this, PopupWidget_BP);

			PopupWidget->Init();
		}

		PopupWidget->Show(Message, Duration, bSpecifyTextColor, TextColor);
	}
}

void URTSGameInstance::HidePopupWidget()
{
	if (PopupWidget != nullptr)
	{
		PopupWidget->Hide();
	}
}


//===============================================================================================
//	Main Menu
//===============================================================================================

void URTSGameInstance::OnEnterMainMenuFromStartup()
{
	// Start playing menu music
	PlayMusic(MainMenuMusic);

	OnEnterMainMenu();
}

void URTSGameInstance::OnEnterMainMenu()
{
	bIsInMainMenuMap = true;

	ShowWidget(EWidgetType::MainMenu, ESlateVisibility::Collapsed);

	APlayerController * const PlayCon = GetWorld()->GetFirstPlayerController();

	PlayCon->SetInputMode(FInputModeUIOnly());
	PlayCon->bShowMouseCursor = true;

	// Switch to playing menu music
	if (MusicAudioComp->Sound != MainMenuMusic)
	{
		PlayMusic(MainMenuMusic);
	}
}

void URTSGameInstance::OnQuitGameInitiated()
{
	QuitGame();
}

void URTSGameInstance::QuitGame(UWorld * World)
{
	UKismetSystemLibrary::QuitGame(World, World->GetFirstPlayerController(),
		EQuitPreference::Quit, true);
}

void URTSGameInstance::QuitGame()
{
	QuitGame(GetWorld());
}


//================================================================================================
//	Lobby Creation Functionality
//================================================================================================

void URTSGameInstance::CreateSingleplayerLobby()
{
	bIsInMainMenuMap = false;

	/* On next ShowWidget call LoadLobbyMap will be called when current widget exit anim finishes */
	WidgetTransitionFunction = &URTSGameInstance::LoadLobbyMapForSingleplayer;

	/* Yeah there is something going on here. This will not show widget when there is a map
	change. The order of function calls is exactly what I expect - the widget is just not
	showing in the viewport, so this will not show the widget and another method needs to be used */
	ShowWidget(EWidgetType::Lobby, ESlateVisibility::Collapsed);
}

void URTSGameInstance::LoadLobbyMapForSingleplayer()
{
	/* Lobby is in new world so using PrevousWidget() will no longer return to what was the
	pervious widget and will most likely cause a crash if tried */

	/* Signal that we will need to remake info actors destroyed during map transition */
	bHasSpawnedInfoActors = false;

	/* Travel to persistent match map. Should be a blank map */
	UGameplayStatics::OpenLevel(GetWorld(), MapOptions::BLANK_PERSISTENT_LEVEL);

	/* Stream in lobby map if any. Show lobby widget when this is complete.
	Important: function to call must be a UFUNCTION. Silent fail otherwise.
	Also logs say this fails and I think it is because OpenLevel is latent or something and
	this actually happens before the blank persistent level is loaded so might need
	to poll that some how before doing this */
	const FLatentActionInfo LatentAction = FLatentActionInfo(0, 0,
		*FString("SetupLobbyWidget_Singleplayer"), this);
	UGameplayStatics::LoadStreamLevel(GetWorld(), MapOptions::LOBBY_MAP_NAME, true, false,
		LatentAction);
}

void URTSGameInstance::SetupLobbyWidget_Singleplayer()
{
	/* A delay here may fix some problems if I encounter any. Player state still null initially
	when opening lobby */

	/* Doing this because used ShowWidget(Lobby) previously. History does not like duplicates
	in it */
	WidgetHistory.Empty();

	/* Set lobby values */
	ARTSGameState * GameState = CastChecked<ARTSGameState>(GetWorld()->GetGameState());
	GameState->SetupSingleplayerLobby();

	/* Mouse cursor hidden previously, assuming because map change */
	GetWorld()->GetFirstPlayerController()->bShowMouseCursor = true;

	ShowWidget(EWidgetType::Lobby, ESlateVisibility::Collapsed);
}

void URTSGameInstance::CreateNetworkedSession(const FText & LobbyName, bool bIsLAN, const FText & Password,
	uint32 NumSlots, uint8 MapID, EStartingResourceAmount StartingResources,
	EDefeatCondition DefeatCondition)
{
	AGameModeBase * GameMode = GetWorld()->GetAuthGameMode();
	ARTSGameSession * const GameSesh = CastChecked<ARTSGameSession>(GameMode->GameSession);

	ULocalPlayer * const Player = GetWorld()->GetFirstLocalPlayerFromController();
	const TSharedPtr <const FUniqueNetId> PlayerID = Player->GetPreferredUniqueNetId().GetUniqueNetId();

	// GameSessionName is a GameInstance variable
	GameSesh->HostSession(PlayerID, GameSessionName, LobbyName, bIsLAN, /*bIsPresence=*/true,
		NumSlots, Password, MapID, StartingResources, DefeatCondition);

	/* Now wait for callback from ARTSGameSession::OnCreateSessionComplete */
}

void URTSGameInstance::OnNetworkedSessionCreated(const TSharedPtr <FOnlineSessionSettings> InSessionSettings, bool bSuccessful)
{
	if (bSuccessful)
	{
		CreateMultiplayerLobby(InSessionSettings);
	}
	else
	{
		/* Session creation not successful, probably because there is already one running. Destroy
		the current session and make another */

		ARTSGameSession * GameSesh = CastChecked<ARTSGameSession>(GetWorld()->GetAuthGameMode()->GameSession);
		IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
		if (OnlineSub != nullptr)
		{
			IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();

			if (Sessions.IsValid())
			{
				GameSesh->DestroySession(EActionToTakeAfterDestroySession::CreateNewSession,
					InSessionSettings);

				/* Eventually a delegate will be called in ARTSGameSession which will create our
				lobby */
			}
		}
	}
}

void URTSGameInstance::CreateMultiplayerLobby(const TSharedPtr <FOnlineSessionSettings> InSessionSettings)
{
	bIsInMainMenuMap = false;

	SessionSettings = InSessionSettings;

	/* On next ShowWidget call LoadLobbyMap will be called when current widget exit anim finishes */
	WidgetTransitionFunction = &URTSGameInstance::LoadLobbyMapForMultiplayer;

	ShowWidget(EWidgetType::Lobby, ESlateVisibility::Collapsed);
}

void URTSGameInstance::LoadLobbyMapForMultiplayer()
{
	/* Signal that we will need to remake info actors destroyed during map transition */
	bHasSpawnedInfoActors = false;

	/* Travel to blank persistent map */
	UGameplayStatics::OpenLevel(GetWorld(), MapOptions::BLANK_PERSISTENT_LEVEL, true,
		FString("listen"));

	/* Stream in lobby map if any. Show lobby widget when this completes.
	Important: function to call must be a UFUNCTION. Silent fail otherwise. */
	const FLatentActionInfo LatentAction = FLatentActionInfo(0, 0,
		*FString("SetupLobbyWidget_Multiplayer"), this);
	UGameplayStatics::LoadStreamLevel(GetWorld(), MapOptions::LOBBY_MAP_NAME, true, false,
		LatentAction);
}

void URTSGameInstance::SetupLobbyWidget_Multiplayer()
{
	/* A delay here may fix some problems if I encounter any. Player state still null initially
	when opening lobby */

	/* It may be possible that a player could join before we put ourselves in the first lobby
	slot during SetupNetworkedLobby below. A work around is that we just join into the next
	available slot, so it is possible the first slot is not the host. Actually it's technically
	possible a whole group of players could occupy all slots before host joins TODO */

	/* Doing this because used ShowWidget(Lobby) previously. History does not like duplicates
	in it */
	WidgetHistory.Empty();

	/* Set lobby values */
	ARTSGameState * const GameState = GetWorld()->GetGameState<ARTSGameState>();
	GameState->SetupNetworkedLobby(SessionSettings);

	/* Mouse cursor hidden previously, assuming because map change */
	GetWorld()->GetFirstPlayerController()->bShowMouseCursor = true;

	ShowWidget(EWidgetType::Lobby, ESlateVisibility::Collapsed);
}

TSharedPtr <FOnlineSessionSettings> & URTSGameInstance::GetSessionSettings()
{
	return SessionSettings;
}

const FString & URTSGameInstance::GetLobbyPassword() const
{
	return LobbyPassword;
}

void URTSGameInstance::SetLobbyPassword(const FString & InPassword)
{
	LobbyPassword = InPassword;
}

TSharedPtr <FOnlineSessionSettings> & URTSGameInstance::GetPendingSessionSettings()
{
	return PendingSessionSettings;
}

void URTSGameInstance::SetPendingSessionSettings(const TSharedPtr <FOnlineSessionSettings> InSessionSettings)
{
	PendingSessionSettings = InSessionSettings;
}


//--------------------------------------------------------
//	Going from lobby or match to main menu
//--------------------------------------------------------

void URTSGameInstance::GoFromLobbyToMainMenu()
{
	/* Set this so game mode knows what map we are in after OpenLevel (the main menu map) */
	bIsInMainMenuMap = true;

	/* Flag that info actors like faction infos will need to be respawned */
	bHasSpawnedInfoActors = false;

	/* Interesting... no remove from viewport on widget that initiated this required, must mean
	widget is destroyed along with UWorld (which is good) */

	// Seems to work fine without this
	WidgetHistory.Empty();

	UGameplayStatics::OpenLevel(GetWorld(), MapOptions::MAIN_MENU_LEVEL);

	// TODO perhaps do some checks that we're persisting any objects during these changes i.e. 
	// you can change map all day and memory will not be an issue
}

void URTSGameInstance::GoFromMatchToMainMenu()
{
	/* Same as going from lobby right now */
	GoFromLobbyToMainMenu();

	// TODO see note in GoFromLobbyToMainMenu() if this func calls that func
}

void URTSGameInstance::ReturnToMainMenu()
{
	/* Super will do a map change so set these. Game mode will show main menu */
	/* Would like host to return to main menu and clients to return to lobby browsing screen, but
	cannot use WidgetHistory as it's empty. Setting a bool when creating/joining
	lobby is an option but need to make sure widget history gets restored so navigating backwards
	from widgets still works. Anyways, for now everyone returns to main menu */

	bIsInMainMenuMap = true;
	bHasSpawnedInfoActors = false;

	WidgetHistory.Empty();

	Super::ReturnToMainMenu();
}


//================================================================================================
//	Searching for and Joining Lobbies
//================================================================================================

void URTSGameInstance::SearchForNetworkedLobbies()
{
	ARTSGameMode * const GameMode = CastChecked<ARTSGameMode>(GetWorld()->GetAuthGameMode());
	ARTSGameSession * const GameSesh = CastChecked<ARTSGameSession>(GameMode->GameSession);
	ULocalPlayer * const Player = GetWorld()->GetFirstLocalPlayerFromController();

	// Just setting this to true for now TODO figure out correct value
	const bool bIsLAN = true;

	const int32 NumSearchResults = bIsLAN ? SessionOptions::MAX_NUM_LAN_SEARCH_RESULTS : SessionOptions::MAX_NUM_STEAM_SEARCH_RESULTS;

	// TODO specify if LAN and/or presence or get both is possible
	GameSesh->FindSessions(Player->GetPreferredUniqueNetId().GetUniqueNetId(), bIsLAN,
		/*bIsPresence=*/true, NumSearchResults, SessionOptions::PING_BUCKET_SIZE);

	/* Wait for ARTSGameSession::OnFindSessionsComplete to fire */
}

void URTSGameInstance::TryJoinNetworkedSession(const FOnlineSessionSearchResult & SearchResult)
{
	AGameModeBase * const GameMode = GetWorld()->GetAuthGameMode();

	ULocalPlayer * const Player = GetWorld()->GetFirstLocalPlayerFromController();

	ARTSGameSession * const GameSesh = CastChecked<ARTSGameSession>(GameMode->GameSession);
	GameSesh->JoinSession(Player->GetPreferredUniqueNetId().GetUniqueNetId(), GameSessionName,
		SearchResult);
}


//================================================================================================
//	Transition from lobby to match
//================================================================================================

// Will need to do the things PIE setup does like call the things in PC::Client_SetupForMatch. 
// See ARTSGameMode::GoToMapFromStartupPart2 line 917 or search SetFactionInfo in that cpp file

void URTSGameInstance::TryCreateMatch(EMatchType MatchNetworkType)
{
	/* This is the first function called when transitioning from lobby to match */

	assert(GetWorld()->IsServer());

	if (MatchNetworkType == EMatchType::Offline)
	{
		ULobbyWidget * Lobby = CastChecked<ULobbyWidget>(GetWidget(EWidgetType::Lobby));
		Lobby->OnStartSessionComplete(true);
	}
	else // Assumed LAN or SteamOnline
	{
		/* This will stop anymore players from joining lobby
		@See ARTSGameSession::GetSessionJoinability */
		ARTSGameState * GameState = CastChecked<ARTSGameState>(GetWorld()->GetGameState());
		if (!GameState->AreLobbySlotsLocked())
		{
			GameState->ChangeLockedSlotsStatusInLobby(true);
		}

		// Start session
		AGameMode * GameMode = CastChecked<AGameMode>(GetWorld()->GetAuthGameMode());
		ARTSGameSession * GameSesh = CastChecked<ARTSGameSession>(GameMode->GameSession);
		GameSesh->StartSession();

		/* Waiting for ARTSGameSession::OnStartOnlineGameComplete to be called. OnStartSessionComplete
		will be called when done */
	}
}

void URTSGameInstance::OnStartSessionComplete(bool bWasSuccessful)
{
	if (!bWasSuccessful)
	{
		// Warning maybe too strict dunno
		UE_LOG(RTSLOG, Warning, TEXT("Unsuccessful starting session"));
	}

	ULobbyWidget * Lobby = GetWidget<ULobbyWidget>(EWidgetType::Lobby);
	Lobby->OnStartSessionComplete(bWasSuccessful);
}

void URTSGameInstance::AssignOptimalStartingSpots(const FMapInfo & MatchMap, 
	TArray<FMinimalPlayerInfo> & PlayerInfo)
{
	/* Haven't tested this func */
	
	// Structs to make this function go more smoothly
	struct FTeamEntry 
	{
	private:
		
		uint8 LobbySlotIndex;
		int16 AssignedStartingSpot;

	public:

		FTeamEntry(uint8 InLobbyIndex, int16 InStartingSpot)
		{
			LobbySlotIndex = InLobbyIndex;
			AssignedStartingSpot = InStartingSpot;
		}

		uint8 GetLobbySlotIndex() const { return LobbySlotIndex; }
		uint16 GetAssignedSpot() const { return AssignedStartingSpot; }
		
		/** 
		 *	@param OutInfo - the param of this function PlayerInfo 
		 *	@param UnassignedSet - UnassignedSpots from this function 
		 */
		void SetAssignedSpot(int16 InSpotID, TArray<FMinimalPlayerInfo> & OutInfo, TSet < uint8 > & UnassignedSet)
		{ 
			assert(InSpotID != -1);
			
			AssignedStartingSpot = InSpotID;
			OutInfo[LobbySlotIndex].SetStartingSpot(InSpotID);

			if (InSpotID != -2)
			{
				int32 NumRemoved = UnassignedSet.Remove(InSpotID);
				
				/* Implies tried to assign player to a spot that was already taken by another player */
				assert(NumRemoved == 1);
			}
		}
	};
	
	struct FTeamArray
	{
		// Players that already have a starting spot assigned to them 
		TArray < FTeamEntry > AssignedPlayers;
		
		// Players that do not have a starting spot assigned to them
		TArray < FTeamEntry > UnassignedPlayers;
	};

	//~~~
	
	const TArray < FPlayerStartInfo > & MapsPlayerStarts = MatchMap.GetPlayerStarts();

	// Check fast path: if map has 0 player starts on it
	if (MapsPlayerStarts.Num() == 0)
	{
		/* Change them from -1s to -2s. This is more for development but says we at least 
		considered it */
		for (auto & Player : PlayerInfo)
		{
			Player.SetStartingSpot(-2);
		}

		return;
	}

	switch (PlayerSpawnRule)
	{
		case EPlayerSpawnRule::Random:
		{
			/* Random is the easier case. For each unassigned player just pick a random spot on map */

			TArray < FTeamEntry > UnassignedPlayers;
			TArray < uint8 > UnassignedSpotsArray;

			// Populate UnassignedSpotsArray
			UnassignedSpotsArray.Reserve(MapsPlayerStarts.Num());
			for (const auto & Elem : MapsPlayerStarts)
			{
				UnassignedSpotsArray.Emplace(Elem.GetUniqueID());
			}

			for (int32 i = 0; i < PlayerInfo.Num(); ++i)
			{
				if (PlayerInfo[i].GetStartingSpot() == -1)
				{
					UnassignedPlayers.Emplace(FTeamEntry(i, PlayerInfo[i].GetStartingSpot()));
				}
				else
				{
					// This makes it O(n^2). Maybe a faster way of doing this
					UnassignedSpotsArray.RemoveSingleSwap(PlayerInfo[i].GetStartingSpot());
				}
			}

			bool bHaveAssignedAllSpots = false;

			for (int32 i = UnassignedPlayers.Num() - 1; i >= 0; --i)
			{
				if (UnassignedSpotsArray.Num() == 0)
				{
					// Have assigned all spots
					bHaveAssignedAllSpots = true;
					break;
				}
					
				const FTeamEntry & Player = UnassignedPlayers[i];

				// Pick random entry from array

				const int32 RandomIndex = FMath::RandRange(0, UnassignedSpotsArray.Num() - 1);
					
				// Assuming if only one element in array before remove that this does not cause crash
				UnassignedSpotsArray.RemoveAtSwap(RandomIndex, 1, false);

				UnassignedPlayers.RemoveAt(i);
			}
			
			if (bHaveAssignedAllSpots)
			{
				for (const auto & Player : UnassignedPlayers)
				{
					PlayerInfo[Player.GetLobbySlotIndex()].SetStartingSpot(-2);
				}
			}

			break;
		} // End case EPlayerSpawnRule::Random:
	
		case EPlayerSpawnRule::NearTeammates:
		{
			/**
			 *	=================================================================================
			 *	PlayerSpawnRule == EPlayerSpawnRule::NearTeammates
			 *	=================================================================================
			 *
			 *	Logic here is to basically spawn players close to their teammates. Breakdown is:
			 *
			 *	- For each player without a starting spot assigned:
			 *		- If at least one teammate already has a spot assigned:
			 *			- Spawn them at the spot closest to a teammate.
			 *		- Else:
			 *			- If there is at least one enemy player with a spot on map:
			 *				- Spawn them at the spot that is furtherest from all enemies = (choosing any
			 *				  other spot would result in being closer to at least one enemy).
			 *			- Else:
			 *				- Spawn them at a random spot since it is implied no spot has been assigned to
			 *				  anyone yet
			 */

			TMap <ETeam, FTeamArray> Teams;

			// Holds spots that don't have any player assigned. 
			TSet <uint8> UnassignedSpots;

			// Populate UnassignedSpots
			UnassignedSpots.Reserve(MapsPlayerStarts.Num());
			for (int32 i = 0; i < MapsPlayerStarts.Num(); ++i)
			{
				UnassignedSpots.Emplace(MapsPlayerStarts[i].GetUniqueID());
			}

			// Populate Teams and UnassignedSpots
			for (int32 i = 0; i < PlayerInfo.Num(); ++i)
			{
				const FMinimalPlayerInfo & Elem = PlayerInfo[i];

				if (!Teams.Contains(Elem.GetTeam()))
				{
					Teams.Emplace(Elem.GetTeam(), FTeamArray());
				}

				if (Elem.GetStartingSpot() == -1)
				{
					Teams[Elem.GetTeam()].UnassignedPlayers.Emplace(FTeamEntry(i, Elem.GetStartingSpot()));

					UnassignedSpots.Remove(Elem.GetStartingSpot());
				}
				else
				{
					Teams[Elem.GetTeam()].AssignedPlayers.Emplace(FTeamEntry(i, Elem.GetStartingSpot()));
				}
			}

			// When true then we know there are no more spots available to assign on map
			bool bOutOfSpots = false;

			// Teams that have no one assigned originally on map 
			TArray < ETeam > DeferredTeams;

			// Loop that will actually assign spots
			for (auto & TeamArray : Teams)
			{
				if (TeamArray.Value.AssignedPlayers.Num() > 0)
				{
					// For each player without a spot...
					for (auto & Player : TeamArray.Value.UnassignedPlayers)
					{
						// Check if should take fast path
						if (bOutOfSpots)
						{
							Player.SetAssignedSpot(-2, PlayerInfo, UnassignedSpots);
							continue;
						}

						// Find spot closest to a teammate

						float BestDistance = FLT_MAX;
						int16 BestSpotID = -1;

						// For each teammate who already has a spot...
						for (const auto & Teammate : TeamArray.Value.AssignedPlayers)
						{
							const FPlayerStartInfo & AssignedSpot = MapsPlayerStarts[Teammate.GetAssignedSpot()];

							for (int32 i = 0; i < AssignedSpot.GetNearbyPlayerStartsSorted().Num(); ++i)
							{
								const FPlayerStartDistanceInfo & Spot = AssignedSpot.GetNearbyPlayerStartsSorted()[i];

								if (UnassignedSpots.Contains(Spot.GetID()) && Spot.GetDistance() < BestDistance)
								{
									BestDistance = Spot.GetDistance();
									BestSpotID = Spot.GetID();

									// Can stop here because array is sorted by distance and know 
									// distances only get further away
									break;
								}
							}
						}

						if (BestSpotID == -1)
						{
							/* This func requires -1s to be changed to -2s to ensure we tried to find
							a spot for them since -1 can be passed in from param and a -1 leaving
							the function could mean we never tried. Getting here also implies we are
							out of spots on map */
							Player.SetAssignedSpot(-2, PlayerInfo, UnassignedSpots);

							bOutOfSpots = true;
						}
						else
						{
							Player.SetAssignedSpot(BestSpotID, PlayerInfo, UnassignedSpots);
						}
					}
				}
				else
				{
					/* Team has 0 players already with assigned spots. Defer trying to spawn them 
					until other teams are done */
					DeferredTeams.Emplace(TeamArray.Key);
				}
			}

			if (!bOutOfSpots)
			{
				bool bIsFirstPlayer = true; // Is this first player on team to get a spot
				bool bDoesEnemyHaveSpot = false;
				for (const auto & Elem : PlayerInfo)
				{
					// bOutOfSpots should be true if this is the case
					assert(Elem.GetStartingSpot() != -2);

					if (Elem.GetStartingSpot() != -1)
					{
						bDoesEnemyHaveSpot = true;
						break;
					}
				}

				// Now assign spots to the teams that had no one originally on map
				for (const auto & Team : DeferredTeams)
				{
					for (auto & Player : Teams[Team].UnassignedPlayers)
					{
						// Check if should take fast path
						if (bOutOfSpots)
						{
							Player.SetAssignedSpot(-2, PlayerInfo, UnassignedSpots);
							continue;
						}

						if (bIsFirstPlayer)
						{
							bIsFirstPlayer = false;

							if (bDoesEnemyHaveSpot)
							{
								// Assign spot furtherest away from enemy

								float BestDistance = 0.f;
								int16 BestSpotID = -1;

								for (const auto & Spot : MapsPlayerStarts)
								{
									if (UnassignedSpots.Contains(Spot.GetUniqueID()))
									{
										float BestDistanceInner = FLT_MAX;

										for (const auto & EnemyTeam : Teams)
										{
											// Skip if our own team
											if (EnemyTeam.Key == Team)
											{
												continue;
											}
											
											for (const auto & Enemy : EnemyTeam.Value.AssignedPlayers)
											{
												const FPlayerStartInfo & EnemySpotInfo = MapsPlayerStarts[Enemy.GetAssignedSpot()];
												
												const float Distance = FPlayerStartDistanceInfo::GetDistanceBetweenSpots(EnemySpotInfo, Spot);
												if (Distance < BestDistanceInner)
												{
													BestDistanceInner = Distance;
												}
											}
										}

										if (BestDistanceInner > BestDistance)
										{
											BestDistance = BestDistanceInner;
											BestSpotID = Spot.GetUniqueID();
										}
									}
								}

								if (BestSpotID == -1)
								{
									Player.SetAssignedSpot(-2, PlayerInfo, UnassignedSpots);

									bOutOfSpots = true;
								}
								else
								{
									Player.SetAssignedSpot(BestSpotID, PlayerInfo, UnassignedSpots);
								}
							}
							else
							{
								// Assign random spot

								const int32 RandomIndex = FMath::RandRange(0, MapsPlayerStarts.Num() - 1);

								const FPlayerStartInfo & PlayerStart = MapsPlayerStarts[RandomIndex];

								Player.SetAssignedSpot(PlayerStart.GetUniqueID(), PlayerInfo, UnassignedSpots);
							}
						}
						else
						{
							// Assign spot closest to teammate

							float BestDistance = FLT_MAX;
							int16 BestSpotID = -1;

							for (const auto & Teammate : Teams[Team].AssignedPlayers)
							{
								const FPlayerStartInfo & AssignedSpot = MapsPlayerStarts[Teammate.GetAssignedSpot()];

								for (int32 i = 0; i < AssignedSpot.GetNearbyPlayerStartsSorted().Num(); ++i)
								{
									const FPlayerStartDistanceInfo & Spot = AssignedSpot.GetNearbyPlayerStartsSorted()[i];

									if (UnassignedSpots.Contains(Spot.GetID()) && Spot.GetDistance() < BestDistance)
									{
										BestDistance = Spot.GetDistance();
										BestSpotID = Spot.GetID();

										// Can stop here because distances only get further away
										break;
									}
								}
							}

							if (BestSpotID == -1)
							{
								Player.SetAssignedSpot(-2, PlayerInfo, UnassignedSpots);

								bOutOfSpots = true;
							}
							else
							{
								Player.SetAssignedSpot(BestSpotID, PlayerInfo, UnassignedSpots);
							}
						}
					}
				}
			}
			else
			{
#if !UE_BUILD_SHIPPING
				for (const auto & Elem : PlayerInfo)
				{
					// Everything should either be >= 0 or -2
					assert(Elem.GetStartingSpot() != -1);
				}
#endif 
			}

#if !UE_BUILD_SHIPPING
			// Log if at least one player could not be given a starting spot
			int32 NumWithoutSpots = 0;
			for (const auto & Player : PlayerInfo)
			{
				if (Player.GetStartingSpot() == -2)
				{
					NumWithoutSpots++;
				}
			}

			UE_CLOG(NumWithoutSpots > 0, RTSLOG, Warning, TEXT("%d players could not be assigned "
				"starting spots most likely because there weren't enough spots on map and were "
				"therefore spawned at a default location (usually origin)"), NumWithoutSpots);
#endif

			// O(n^3) for all that

			break;
		} // End case EPlayerSpawnRule::NearTeammates:

		default:
		{
			UE_LOG(RTSLOG, Fatal, TEXT("Unknown enum value. Value was [%s] "),
				TO_STRING(EPlayerSpawnRule, PlayerSpawnRule));
			break;
		}

	} // End switch (PlayerSpawnRule)
}

void URTSGameInstance::LoadMatch()
{
	SERVER_CHECK;
	// Ensure that match info struct has been set up for the match
	assert(MatchInfo.GetPlayers().Num() > 0);

	// Show loading screen
	ShowWidget(EWidgetType::LoadingScreen, ESlateVisibility::Collapsed);

	ARTSGameState * GameState = CastChecked<ARTSGameState>(GetWorld()->GetGameState());

	// Recently added this 26/02/19 to avoid crashing with CPU players during their Setup
	GameState->SetupForMatch(GetPoolingManager(), MatchInfo.GetNumTeams());

	// Resource array each player starts with
	const TArray <int32> & StartingResourceArray = GetStartingResourceConfig(MatchInfo.GetStartingResources()).GetResourceArraySlow();

	/* Figure out number of human players and while we're at it spawn CPU players if any */
	int32 NumHumanPlayers = 0;
	for (auto & PlayerInfo : MatchInfo.GetPlayers())
	{
		if (PlayerInfo.PlayerType == ELobbySlotStatus::Human)
		{
			NumHumanPlayers++;
		}
		else if (PlayerInfo.PlayerType == ELobbySlotStatus::CPU)
		{
			// Spawn a CPU player
			ACPUPlayerAIController * CPUPlayer = SpawnCPUPlayer(PlayerInfo.CPUDifficulty, PlayerInfo.Team,
				PlayerInfo.Faction, PlayerInfo.StartingSpotID, StartingResourceArray);

			PlayerInfo.PlayerState = CastChecked<ARTSPlayerState>(CPUPlayer->PlayerState);
			PlayerInfo.SetCPUPlayerState(PlayerInfo.PlayerState);
		}
	}

	/* Tell GS how many acks to wait for */
	GameState->SetNumPlayersForMatch(MatchInfo.GetPlayers().Num(), NumHumanPlayers);

	GameState->Multicast_LoadMatchMap(MatchInfo.GetMapFName());

	/* At this point if server called AGameMode::StartMatch the map will not have loaded yet. The
	implication of this is player starts haven't loaded either so players start at origin, so
	best wait for map to load before continuing. Pretty sure clients do not have to wait for their
	map to stream in so only doing this for server, but if it becomes an issue then will have to
	add another RPC ack counter */

	// Wait for GS to call OnMatchLevelLoaded
}

void URTSGameInstance::OnMatchLevelLoaded()
{
	/* This will spawn the pawn used as the camera for each player. Because bDelayedStart is true
	in game mode this isn't called automatically and must be called explicitly */
	AGameMode * GameMode = CastChecked<AGameMode>(GetWorld()->GetAuthGameMode());
	GameMode->StartMatch();

	ARTSGameState * GameState = CastChecked<ARTSGameState>(GetWorld()->GetGameState());
	const uint8 NumTeams = MatchInfo.GetNumTeams();

	/* Setup a lot of things for match */
	for (TActorIterator <ARTSPlayerController> Iter(GetWorld()); Iter; ++Iter)
	{
		ARTSPlayerController * PlayCon = *Iter;

		for (const auto & PlayerInfo : MatchInfo.GetPlayers())
		{
			if (PlayCon->PlayerState == PlayerInfo.PlayerState)
			{	
				PlayCon->Server_SetupForMatch(GameState->GenerateUniquePlayerID(),
					PlayerInfo.Team, PlayerInfo.Faction, MatchInfo.GetStartingResources(), NumTeams, 
					MatchInfo.GetMapUniqueID(), PlayerInfo.StartingSpotID);
			}
		}
	}

	/* Now need to wait for all acks for loading map and PC::Client_SetupForMatch to come in.
	Once all are through then OnLevelLoadedAndPCSetupsDone will be called */
}

void URTSGameInstance::OnLevelLoadedAndPCSetupsDone()
{
	SERVER_CHECK;

	//================================================================================
	//	Polling stage
	//================================================================================

	// Busy wait for all CPU player states to be valid, very rare to happen, but I have had 
	// player states null even on server 
	TArray < AActor * > AllPlayerStates;
	UGameplayStatics::GetAllActorsOfClass(this, ARTSPlayerState::StaticClass(), AllPlayerStates);

	if (MatchInfo.GetPlayers().Num() != AllPlayerStates.Num())
	{
		FTimerHandle TimerHandle_Dummy;
		Delay(TimerHandle_Dummy, &URTSGameInstance::OnLevelLoadedAndPCSetupsDone, 0.01f);
		return;
	}

	//================================================================================
	//	Part of function that does stuff
	//================================================================================

	ARTSGameState * GameState = CastChecked<ARTSGameState>(GetWorld()->GetGameState());
	GameState->SetMatchLoadingStatus(ELoadingStatus::WaitingForInitialValuesAcknowledgementFromAllPlayers);

	for (const auto & PlayerInfo : MatchInfo.GetPlayers())
	{
		if (true || PlayerInfo.PlayerState->bIsABot)
		{
			/* Important that AMyPlayerState::ID has been set by now (which it should have
			been) */
			/* Important that human players get CPU initial values too */
			ARTSPlayerState * MyPlayerState = CastChecked<ARTSPlayerState>(PlayerInfo.PlayerState);
			MyPlayerState->Multicast_SetInitialValues(MyPlayerState->GetPlayerIDAsInt(), 
				MyPlayerState->GetPlayerID(), PlayerInfo.Team, PlayerInfo.Faction);
		}
	}

	/* Game state is now waiting for acknowledgements that all players inital values have
	been set. Once all are through then OnInitialValuesAcked() will be called */
}

void URTSGameInstance::OnInitialValuesAcked()
{
	// Do final setup part
	for (const auto & Elem : MatchInfo.GetPlayers())
	{
		Elem.PlayerState->Client_FinalSetup();
	}

	/* Now wait for acks to come through that this has completed. Once all are through then
	OnMatchFinalSetupComplete() will be called */
}

void URTSGameInstance::OnMatchFinalSetupComplete()
{
	/* Well that's it. No more acknowledges to wait for from here on out. Just spawn the starting
	selectables and start the match */

	const TMap<ARTSPlayerState *, FStartingSelectables> & SpawnedTypes = SpawnStartingSelectables();

	ARTSGameState * GameState = CastChecked<ARTSGameState>(GetWorld()->GetGameState());
	GameState->StartMatch(SpawnedTypes);

	/* - Make sure input is disabled by default in AMyPlayerCOntroller and enable it when
	all this is done */
}

const FMatchInfo & URTSGameInstance::GetMatchInfo() const
{
	return MatchInfo;
}

FMatchInfo & URTSGameInstance::GetMatchInfoModifiable()
{
	return MatchInfo;
}

const FText & URTSGameInstance::GetMatchLoadingStatusText(ELoadingStatus LoadingStatus) const
{
	return LoadingScreenMessages.Contains(LoadingStatus)
		? LoadingScreenMessages[LoadingStatus] : Menus::BLANK_TEXT;
}

bool URTSGameInstance::IsColdBooting() const
{
	return bIsColdBooting;
}

void URTSGameInstance::SetIsColdBooting(bool bNewValue)
{
	bIsColdBooting = bNewValue;
}

bool URTSGameInstance::IsInMainMenuMap() const
{
	return bIsInMainMenuMap;
}

ACPUPlayerAIController * URTSGameInstance::SpawnCPUPlayer(ECPUDifficulty InCPUDifficulty, 
	ETeam InTeam, EFaction InFaction, int16 StartingSpot, const TArray <int32> & InStartingResources)
{
	assert(InCPUDifficulty != ECPUDifficulty::None);

	ARTSGameState * GameState = CastChecked<ARTSGameState>(GetWorld()->GetGameState());

	TSubclassOf <ACPUPlayerAIController> Blueprint = (InCPUDifficulty == ECPUDifficulty::DoesNothing)
		? ACPUPlayerAIController::StaticClass()
		: CPUPlayerInfo[InCPUDifficulty].GetControllerBP();

	ACPUPlayerAIController * CPUPlayer = GetWorld()->SpawnActor<ACPUPlayerAIController>(Blueprint,
		Statics::INFO_ACTOR_SPAWN_LOCATION, FRotator::ZeroRotator);
	assert(CPUPlayer != nullptr);

	const uint8 UniqueID = GameState->GenerateUniquePlayerID();
	CPUPlayer->Setup(UniqueID, *FString::FromInt(UniqueID), InTeam, InFaction, StartingSpot,
		InStartingResources, InCPUDifficulty);

	// TODO Assign player state to MAtchInfo.GetPLayers()[SomeIndex].PlayerState

	return CPUPlayer;
}

TMap <ARTSPlayerState *, FStartingSelectables> URTSGameInstance::SpawnStartingSelectables()
{
	ARTSGameState * GameState = CastChecked<ARTSGameState>(GetWorld()->GetGameState());
	GameState->SetMatchLoadingStatus(ELoadingStatus::SpawningStartingSelectables);

	const FMapInfo & MapInfo = GetMapInfo(MatchInfo.GetMapUniqueID());
	const TArray <FPlayerStartInfo> & PlayerStarts = MapInfo.GetPlayerStarts();

	// Can't call get start spot here because it takes a controller, not a player state 

	const TArray<ARTSPlayerController *> & HumanPlayers = GameState->GetPlayers();
	const TArray<ACPUPlayerAIController *> & CPUPlayers = GameState->GetCPUControllers();

	if (PlayerStarts.Num() < HumanPlayers.Num() + CPUPlayers.Num())
	{
		UE_LOG(RTSLOG, Fatal, TEXT("Not enough player starts for number of players in match. "
			"Need %d but there are only %d"), HumanPlayers.Num() + CPUPlayers.Num(), PlayerStarts.Num());
	}

	TMap < ARTSPlayerState *, FStartingSelectables > ReturnValue;

	for (int32 i = 0; i < MatchInfo.GetPlayers().Num(); ++i)
	{
		const FPlayerInfo & PlayerInfo = MatchInfo.GetPlayers()[i];

		/* Different player states for human and CPU, don't really have to do that though */
		ARTSPlayerState * PlayerState = nullptr;
		const bool bIsForHumanPlayer = (PlayerInfo.PlayerType == ELobbySlotStatus::Human);
		const bool bIsForCPUPlayer = (PlayerInfo.PlayerType == ELobbySlotStatus::CPU);
		if (bIsForHumanPlayer)
		{
			PlayerState = PlayerInfo.PlayerState;
		}
		else if (bIsForCPUPlayer)
		{
			PlayerState = PlayerInfo.CPUPlayerState;
		}
		else
		{
			UE_LOG(RTSLOG, Fatal, TEXT("Unexpected player type"));
		}

		const FTransform SpawnTransform = FTransform(PlayerStarts[i].GetRotation(), PlayerStarts[i].GetLocation(), FVector::OneVector);
		PlayerState->SetStartLocation(SpawnTransform.GetLocation());
		TArray<EBuildingType> StartingBuildings;
		TArray<EUnitType> StartingUnits;
		Statics::SpawnStartingSelectables(PlayerState, SpawnTransform, this, StartingBuildings, 
			StartingUnits);

		/* Add entry to TMap. Only really need to do this for CPU players though */
		if (bIsForCPUPlayer)
		{
			ReturnValue.Emplace(PlayerState, FStartingSelectables());
			ReturnValue[PlayerState].SetStartingBuildings(StartingBuildings);
			ReturnValue[PlayerState].SetStartingUnits(StartingUnits);
		}
		
		// This all works. Now to get fog of war rendering correctly
		// TODO check right amount of player states/controllers etc in world outlier when 
		// tesing with PIE

		/* Move pawn to location just in case not already there */
		//PlayCon->GetPawn()->SetActorLocation(SpawnTransform.GetLocation());
	}

	return ReturnValue;
}


//================================================================================================
//	Development
//================================================================================================

#if WITH_EDITOR
void URTSGameInstance::OnInitiateSkipMainMenu()
{
	bIsInMainMenuMap = false;
}

void URTSGameInstance::SetupMatchInfoForPIE(int32 InNumTeams, const TArray<ARTSPlayerState*>& PlayerStates,
	const TMap<ETeam, ETeam> & NewTeams)
{	
	const FMapInfo & MapInfo = GetMapInfo(*Statics::GetMapName(GetWorld()));
	
	/* Using the same value the lobby sets which is the map display name, not file name */
	MatchInfo.SetMap(MapInfo.GetMapFName(), MapInfo.GetUniqueID(), MapInfo.GetDisplayName());
	MatchInfo.SetMatchType(EMatchType::LAN); 
	MatchInfo.SetDefeatCondition(EditorPlaySession_GetDefeatCondition());
	MatchInfo.SetNumTeams(InNumTeams);

	const TArray < FPIEPlayerInfo > & HumanPlayerInfo = EditorPlaySession_GetHumanPlayerInfo();

	/* Create this array so it can be passed into AssignOptimalStartingSpots
	Array should already be sorted with human PSs first followed by CPU PSs */
	TArray <FMinimalPlayerInfo> PlayerInfo;
	for (int32 i = 0; i < PlayerStates.Num(); ++i)
	{
		ARTSPlayerState * PlayState = PlayerStates[i];
		bool bIsForHumanPlayer;
		ETeam Team;
		EFaction Faction;
		int16 StartingSpot;
		
		/* CPU player states already have the right info on them. Human player states will
		need to query the development settings. Note the starting spot can change later on 
		during AssignOptimalStartingSpots */
		if (PlayState->bIsABot)
		{
			bIsForHumanPlayer = false;
			Team = PlayState->GetTeam();
			Faction = PlayState->GetFaction();
			StartingSpot = PlayState->GetStartingSpot();
		}
		else
		{
			/* Using i here should be ok since all human player states are first in array */
			bIsForHumanPlayer = true;
			Team = NewTeams[HumanPlayerInfo[i].GetTeam()];
			Faction = HumanPlayerInfo[i].GetFaction();
			StartingSpot = HumanPlayerInfo[i].GetStartingSpot();
		}
		
		PlayerInfo.Emplace(FMinimalPlayerInfo(bIsForHumanPlayer, Team, Faction, StartingSpot));
	}
	
	// Assign starting spots to those without one
	AssignOptimalStartingSpots(MapInfo, PlayerInfo);

	// Now create entries for MatchInfo by adding on the player state. Could have just done this above
	for (int32 i = 0; i < PlayerStates.Num(); ++i)
	{
		ARTSPlayerState * PlayState = PlayerStates[i];

		const ETeam Team = PlayerInfo[i].GetTeam();
		const EFaction Faction = PlayerInfo[i].GetFaction();
		const int16 StartingSpot = PlayerInfo[i].GetStartingSpot();

		MatchInfo.AddPlayer(FPlayerInfo(PlayState, Team, Faction, StartingSpot));

		if (PlayState->bIsABot)
		{
			/* Now that starting spots are final we can do this */
			PlayState->AICon_SetFinalStartingSpot(StartingSpot);
		}
	}

	// Missed out on setting starting resources
}
#endif


//============================================================================================
//	Structs defined in RTSGameInstance.h
//============================================================================================

FMinimalPlayerInfo::FMinimalPlayerInfo()
{
	// Commeneted because crash here on startup after migrating to 4.21
	//assert(0);
}

FMinimalPlayerInfo::FMinimalPlayerInfo(bool bIsForHuman, ETeam InTeam, EFaction InFaction, 
	int16 InStartingSpotID)
{
	bIsForHumanPlayer = bIsForHuman;
	Team = InTeam;
	Faction = InFaction;
	StartingSpot = InStartingSpotID;
}

bool FMinimalPlayerInfo::IsHumanPlayer() const
{
	return bIsForHumanPlayer;
}

ETeam FMinimalPlayerInfo::GetTeam() const
{
	return Team;
}

EFaction FMinimalPlayerInfo::GetFaction() const
{
	return Faction;
}

int16 FMinimalPlayerInfo::GetStartingSpot() const
{
	return StartingSpot;
}

void FMinimalPlayerInfo::SetStartingSpot(int16 InStartingSpot)
{
	StartingSpot = InStartingSpot;
}


//================================================================================================
//	Base widget classes
//================================================================================================

UMenuButton::UMenuButton()
{
	OnPressed.AddDynamic(this, &UMenuButton::UIBinding_OnPress);
}

void UMenuButton::UIBinding_OnPress()
{
	/* To be overridden */
}

#if WITH_EDITOR
const FText UMenuButton::GetPaletteCategory()
{
	return Menus::PALETTE_CATEGORY;
}

const FText UMenuTextBox::GetPaletteCategory()
{
	return Menus::PALETTE_CATEGORY;
}
#endif //WITH_EDITOR

void UMenuEditableText::UIBinding_OnTextChanged(const FText & NewText)
{
}

void UMenuEditableText::UIBinding_OnTextCommitted(const FText & NewText, ETextCommit::Type CommitMethod)
{
}

#if WITH_EDITOR
const FText UMenuEditableText::GetPaletteCategory()
{
	return Menus::PALETTE_CATEGORY;
}
#endif


//-------------------------------------------------
//	Confirm Exit to OS Buttons
//-------------------------------------------------

void UConfirmExitToOS_YesButton::UIBinding_OnPress()
{
	URTSGameInstance * GameInstance = Menus::GetGameInstance(GetWorld());
	GameInstance->OnQuitGameInitiated();
}

void UConfirmExitToOS_NoButton::UIBinding_OnPress()
{
	URTSGameInstance * GameInstance = Menus::GetGameInstance(GetWorld());
	GameInstance->ShowPreviousWidget();
}
