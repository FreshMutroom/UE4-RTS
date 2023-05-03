// Fill out your copyright notice in the Description page of Project Settings.

#include "RTSGameSession.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

#include "GameFramework/RTSGameInstance.h"
#include "UI/MainMenu/Menus.h"
#include "UI/MainMenu/LobbyBrowsing.h"
#include "Statics/DevelopmentStatics.h"
#include "GameFramework/RTSGameState.h"
#include "Statics/Statics.h"


//==============================================================================================
//	Structs and Statics
//==============================================================================================

// Settings
const int32 SessionOptions::MAX_NUM_LAN_SEARCH_RESULTS = 20;
const int32 SessionOptions::MAX_NUM_STEAM_SEARCH_RESULTS = 1000;
const int32 SessionOptions::PING_BUCKET_SIZE = 50;

// Keys
const FName SessionOptions::LOBBY_NAME_KEY = FName("LobbyName");
const FName SessionOptions::IS_PASSWORD_PROTECTED_KEY = FName("PasswordProtected");
const FName SessionOptions::MAP_NAME_KEY = FName("MapName");
const FName SessionOptions::STARTING_RESOURCES_KEY = FName("StartingResources");
const FName SessionOptions::DEFEAT_CONDITION_KEY = FName("DefeatCondition");
const FName SessionOptions::UNIQUE_RTS_KEY = FName("RTSqx7batmansign");


FString SessionOptions::Map_ToString(uint8 InMapID)
{
	return FString::FromInt(InMapID);
}

uint8 SessionOptions::Map_ToID(const FString & StringForm)
{
	return FCString::Atoi(*StringForm);
}

FString SessionOptions::IsPasswordProtected_ToString(bool bIsPasswordProtected)
{
	return bIsPasswordProtected ? FString("1") : FString("0");
}

bool SessionOptions::IsPasswordProtected_ToBool(const FString & StringForm)
{
	return StringForm == FString("1");
}

FString SessionOptions::StartingResources_ToString(EStartingResourceAmount Amount)
{
	return FString::FromInt(Statics::StartingResourceAmountToArrayIndex(Amount));
}

EStartingResourceAmount SessionOptions::StartingResources_ToEnumValue(const FString & StringForm)
{
	return Statics::ArrayIndexToStartingResourceAmount(FCString::Atoi(*StringForm));
}

FString SessionOptions::DefeatCondition_ToString(EDefeatCondition DefeatCondition)
{
	return FString::FromInt(Statics::DefeatConditionToArrayIndex(DefeatCondition));
}

EDefeatCondition SessionOptions::DefeatCondition_ToEnumValue(const FString & StringForm)
{
	return Statics::ArrayIndexToDefeatCondition(FCString::Atoi(*StringForm));
}

bool SessionOptions::IsLANSession(const TSharedPtr<FOnlineSessionSettings>& SessionSettings)
{
	return SessionSettings->bIsLANMatch;
}

FString SessionOptions::GetLobbyName(const TSharedPtr<FOnlineSessionSettings>& SessionSettings)
{
	FString NetworkedLobbyName;
	SessionSettings->Get<FString>(LOBBY_NAME_KEY, NetworkedLobbyName);
	
	return NetworkedLobbyName;
}

EStartingResourceAmount SessionOptions::GetStartingResources(const TSharedPtr<FOnlineSessionSettings>& SessionSettings)
{
	FString StartingResources;
	SessionSettings->Get<FString>(STARTING_RESOURCES_KEY, StartingResources);

	return StartingResources_ToEnumValue(StartingResources);
}

EDefeatCondition SessionOptions::GetDefeatCondition(const TSharedPtr<FOnlineSessionSettings>& SessionSettings)
{
	FString DefeatCondition;
	SessionSettings->Get<FString>(DEFEAT_CONDITION_KEY, DefeatCondition);

	return DefeatCondition_ToEnumValue(DefeatCondition);
}

uint8 SessionOptions::GetMapID(const TSharedPtr<FOnlineSessionSettings>& SessionSettings)
{
	FString MapID;
	SessionSettings->Get<FString>(MAP_NAME_KEY, MapID);

	return Map_ToID(MapID);
}

int32 SessionOptions::GetNumPublicConnections(const TSharedPtr<FOnlineSessionSettings>& SessionSettings)
{
	return SessionSettings->NumPublicConnections;
}

bool SessionOptions::IsLAN(const FOnlineSessionSearchResult & SearchResult)
{
	return SearchResult.Session.SessionSettings.bIsLANMatch;
}

const FString & SessionOptions::GetHostName(const FOnlineSessionSearchResult & SearchResult)
{
	return SearchResult.Session.OwningUserName;
}

FString SessionOptions::GetServerName(const FOnlineSessionSearchResult & SearchResult)
{
	return SearchResult.Session.SessionSettings.Settings[SessionOptions::LOBBY_NAME_KEY].ToString();
}

int32 SessionOptions::GetCurrentNumPlayers(const FOnlineSessionSearchResult & SearchResult)
{
	// Test: may not be correct
	return GetMaxNumPlayers(SearchResult) - SearchResult.Session.NumOpenPublicConnections;
}

int32 SessionOptions::GetMaxNumPlayers(const FOnlineSessionSearchResult & SearchResult)
{
	return SearchResult.Session.SessionSettings.NumPublicConnections;
}

bool SessionOptions::IsPasswordProtected(const FOnlineSessionSearchResult & SearchResult)
{
	// String should be "1" for true, "0" for false
	const FString AsString = SearchResult.Session.SessionSettings.Settings[SessionOptions::IS_PASSWORD_PROTECTED_KEY].ToString();
	
	UE_CLOG(AsString != FString("1") && AsString != FString("0"), RTSLOG, Fatal, 
		TEXT("Unexpected password value: %s but expected \"0\" or \"1\""), *AsString);

	return SessionOptions::IsPasswordProtected_ToBool(AsString);
}

int32 SessionOptions::GetPing(const FOnlineSessionSearchResult & SearchResult)
{
	return SearchResult.PingInMs;
}

uint8 SessionOptions::GetMapID(const FOnlineSessionSearchResult & SearchResult)
{
	FString MapID;
	SearchResult.Session.SessionSettings.Get<FString>(SessionOptions::MAP_NAME_KEY, MapID);
	
	return Map_ToID(MapID);
}

EStartingResourceAmount SessionOptions::GetStartingResources(const FOnlineSessionSearchResult & SearchResult)
{
	FString StartingResources;
	bool bResult = SearchResult.Session.SessionSettings.Get<FString>(SessionOptions::STARTING_RESOURCES_KEY,
		StartingResources);
	assert(bResult);
	
	return StartingResources_ToEnumValue(StartingResources);
}

EDefeatCondition SessionOptions::GetDefeatCondition(const FOnlineSessionSearchResult & SearchResult)
{
	FString DefeatCondition;
	bool bResult = SearchResult.Session.SessionSettings.Get<FString>(SessionOptions::DEFEAT_CONDITION_KEY,
		DefeatCondition);
	assert(bResult);
	
	return DefeatCondition_ToEnumValue(DefeatCondition);
}


//==============================================================================================
//	Game Session Implementation
//==============================================================================================

ARTSGameSession::ARTSGameSession()
{
	/* Bind functions */
	OnCreateSessionCompleteDelegate = FOnCreateSessionCompleteDelegate::CreateUObject(this, &ARTSGameSession::OnCreateSessionComplete);
	OnStartSessionCompleteDelegate = FOnStartSessionCompleteDelegate::CreateUObject(this, &ARTSGameSession::OnStartOnlineGameComplete);

	OnFindSessionsCompleteDelegate = FOnFindSessionsCompleteDelegate::CreateUObject(this, &ARTSGameSession::OnFindSessionsComplete);
	OnJoinSessionCompleteDelegate = FOnJoinSessionCompleteDelegate::CreateUObject(this, &ARTSGameSession::OnJoinSessionComplete);

	OnDestroySessionCompleteDelegate = FOnDestroySessionCompleteDelegate::CreateUObject(this, &ARTSGameSession::OnDestroySessionComplete);
}

void ARTSGameSession::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	GI = CastChecked<URTSGameInstance>(GetWorld()->GetGameInstance());
}

bool ARTSGameSession::HostSession(TSharedPtr<const FUniqueNetId> UserId, FName NewSessionName,
	const FText & LobbyName, bool bIsLAN, bool bIsPresence, int32 MaxNumPlayers, 
	const FText & LobbyPassword, uint8 MapID, EStartingResourceAmount StartingResources, 
	EDefeatCondition DefeatCondition)
{
	// Get the Online Subsystem to work with
	IOnlineSubsystem * OnlineSub = IOnlineSubsystem::Get();

#if WITH_EDITOR
	if (OnlineSub == nullptr)
	{
		// TODO: log something like "No online subsystem"
		return false;
	}
#endif

	TSharedPtr <FOnlineSessionSettings> & SessionSettings = GI->GetSessionSettings();

	IOnlineSessionPtr Session = GetSessionInterface(OnlineSub);
	if (Session.IsValid() && UserId.IsValid())
	{
		UE_LOG(RTSLOG, Log, TEXT("Creating session for online subsystem %s"),
			*OnlineSub->GetSubsystemName().ToString());

		SessionSettings = MakeShareable(new FOnlineSessionSettings());

		SessionSettings->bIsLANMatch = bIsLAN;
		SessionSettings->bUsesPresence = bIsPresence;
		SessionSettings->NumPublicConnections = MaxNumPlayers;
		SessionSettings->NumPrivateConnections = 0;
		SessionSettings->bAllowInvites = true;
		SessionSettings->bAllowJoinInProgress = false; // Different from wiki
		SessionSettings->bShouldAdvertise = true;
		SessionSettings->bAllowJoinViaPresence = true;
		SessionSettings->bAllowJoinViaPresenceFriendsOnly = false;
		SessionSettings->bUsesStats = false;
		SessionSettings->bIsDedicated = false;

		/* Set lobby name, match rules, map name and password protected */
		SessionSettings->Set(SessionOptions::LOBBY_NAME_KEY, LobbyName.ToString(),
			EOnlineDataAdvertisementType::ViaOnlineService);
		SessionSettings->Set(SessionOptions::MAP_NAME_KEY, SessionOptions::Map_ToString(MapID),
			EOnlineDataAdvertisementType::ViaOnlineService);
		GI->SetLobbyPassword(LobbyPassword.ToString());
		const bool bIsPasswordProtected = LobbyPassword.ToString().Len() > 0;
		const FString IsPasswordProtected = SessionOptions::IsPasswordProtected_ToString(bIsPasswordProtected);
		SessionSettings->Set(SessionOptions::IS_PASSWORD_PROTECTED_KEY, IsPasswordProtected,
			EOnlineDataAdvertisementType::ViaOnlineService);
		SessionSettings->Set(SessionOptions::STARTING_RESOURCES_KEY, SessionOptions::StartingResources_ToString(StartingResources),
			EOnlineDataAdvertisementType::ViaOnlineService);
		SessionSettings->Set(SessionOptions::DEFEAT_CONDITION_KEY, SessionOptions::DefeatCondition_ToString(DefeatCondition),
			EOnlineDataAdvertisementType::ViaOnlineService);

		/* Add a (hopefully) unique identifier to distinguish this session from other Space War
		sessions so finding RTS sessions via steam is easy */
		SessionSettings->Set(SessionOptions::UNIQUE_RTS_KEY, SessionOptions::UNIQUE_RTS_KEY.ToString(),
			EOnlineDataAdvertisementType::ViaOnlineService);

		// Set the delegate to the Handle of the SessionInterface
		DelegateHandle_OnCreateSessionComplete = Session->AddOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegate);

		// Our delegate should get called when this is complete (doesn't need to be successful!)
		return Session->CreateSession(*UserId, NewSessionName, *SessionSettings);
	}

	return false;
}

void ARTSGameSession::OnCreateSessionComplete(FName NewSessionName, bool bWasSuccessful)
{
	// Get the OnlineSubsystem so we can get the Session Interface
	IOnlineSubsystem * OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub != nullptr)
	{
		// Get the Session Interface to call the StartSession function
		IOnlineSessionPtr Session = GetSessionInterface(OnlineSub);

		if (Session.IsValid())
		{
			// Clear the SessionComplete delegate handle, since we finished this call
			Session->ClearOnCreateSessionCompleteDelegate_Handle(DelegateHandle_OnCreateSessionComplete);
			if (bWasSuccessful)
			{
				// Moved stuff from here to OnStartOnlineGameComplete
			}

			UE_LOG(RTSLOG, Log, TEXT("Done creating session for online subsystem %s, result was %s"),
				*OnlineSub->GetSubsystemName().ToString(),
				bWasSuccessful ? *FString("success") : *FString("failed"));

			GI->OnNetworkedSessionCreated(GI->GetSessionSettings(), bWasSuccessful);
		}
	}
}

void ARTSGameSession::FindSessions(TSharedPtr<const FUniqueNetId> UserId, bool bIsLAN, bool bIsPresence, int32 MaxSearchResults, int32 PingBucketSize)
{
	// Get the OnlineSubsystem we want to work with
	IOnlineSubsystem * OnlineSub = IOnlineSubsystem::Get();

	if (OnlineSub == nullptr)
	{
#if WITH_EDITOR
		// TODO: log something
#endif
		// If something goes wrong, just call the Delegate Function directly with "false".
		OnFindSessionsComplete(false);

		return;
	}

	// Get the SessionInterface from our OnlineSubsystem
	IOnlineSessionPtr Sessions = GetSessionInterface(OnlineSub);

	if (Sessions.IsValid() && UserId.IsValid())
	{
		/*
		Fill in all the SearchSettings, like if we are searching for a LAN game and how many results we want to have!
		*/
		SessionSearch = MakeShareable(new FOnlineSessionSearch());

		SessionSearch->bIsLanQuery = bIsLAN;
		SessionSearch->MaxSearchResults = MaxSearchResults;
		SessionSearch->PingBucketSize = PingBucketSize;
		SessionSearch->TimeoutInSeconds = 20.f;

		// We only want to set this Query Setting if "bIsPresence" is true
		if (bIsPresence)
		{
			SessionSearch->QuerySettings.Set(SEARCH_PRESENCE, bIsPresence, EOnlineComparisonOp::Equals);
		}

		/* Make sure to add this to only get RTS game results, not all Space Wars results */
		SessionSearch->QuerySettings.Set(SEARCH_KEYWORDS, SessionOptions::UNIQUE_RTS_KEY.ToString(),
			EOnlineComparisonOp::Equals);

		TSharedRef<FOnlineSessionSearch> SearchSettingsRef = SessionSearch.ToSharedRef();

		// Set the Delegate to the Delegate Handle of the FindSession function
		DelegateHandle_OnFindSessionsComplete = Sessions->AddOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegate);

		// Finally call the SessionInterface function. The Delegate gets called once this is finished
		Sessions->FindSessions(*UserId, SearchSettingsRef);
	}
#if !UE_BUILD_SHIPPING
	else
	{
		UE_LOG(RTSLOG, Fatal, TEXT("In FindSessions session interface was null"));
	}
#endif
}

void ARTSGameSession::OnFindSessionsComplete(bool bWasSuccessful)
{
	// Get OnlineSubsystem we want to work with
	IOnlineSubsystem * OnlineSub = IOnlineSubsystem::Get();

#if !UE_BUILD_SHIPPING
	if (OnlineSub == nullptr)
	{
		UE_LOG(RTSLOG, Warning, TEXT("No online subsystem"));
		return;
	}

	if (!bWasSuccessful)
	{
		assert(0);
	}
#endif

	UE_LOG(RTSLOG, Log, TEXT("Done finding sessions for online subsystem %s, result was %s"),
		*OnlineSub->GetSubsystemName().ToString(),
		bWasSuccessful ? *FString("success") : *FString("failed"));

	// Get SessionInterface of the OnlineSubsystem
	IOnlineSessionPtr Sessions = GetSessionInterface(OnlineSub);
	if (Sessions.IsValid())
	{
		MESSAGE("***************Server browsing found some sessions************* Num", SessionSearch->SearchResults.Num());

		// Clear the Delegate handle, since we finished this call
		Sessions->ClearOnFindSessionsCompleteDelegate_Handle(DelegateHandle_OnFindSessionsComplete);

		// "SessionSearch->SearchResults" is an Array that contains all the information. You can access the Session in this and get a lot of information.
		// This can be customized later on with your own classes to add more information that can be set and displayed

		/* Notify lobby browsing widget */
		ULobbyBrowserWidget * BrowserWidget = CastChecked<ULobbyBrowserWidget>(GI->GetWidget(EWidgetType::LobbyBrowser));
		BrowserWidget->OnSearchingComplete(*SessionSearch);
	}
}

bool ARTSGameSession::JoinSession(TSharedPtr<const FUniqueNetId> UserId, FName TargetSessionName, const FOnlineSessionSearchResult & SearchResult)
{
	// Return bool
	bool bSuccessful = false;

	// Get OnlineSubsystem we want to work with
	IOnlineSubsystem * OnlineSub = IOnlineSubsystem::Get();

#if WITH_EDITOR
	if (OnlineSub == nullptr)
	{
		// TODO: log something
		return bSuccessful;
	}
#endif

	// Get SessionInterface from the OnlineSubsystem
	IOnlineSessionPtr Sessions = GetSessionInterface(OnlineSub);

	if (Sessions.IsValid() && UserId.IsValid())
	{
		// Set the Handle again
		DelegateHandle_OnJoinSessionComplete = Sessions->AddOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegate);

		// Call the "JoinSession" Function with the passed "SearchResult". The "SessionSearch->SearchResults" can be used to get such a
		// "FOnlineSessionSearchResult" and pass it. Pretty straight forward!
		bSuccessful = Sessions->JoinSession(*UserId, TargetSessionName, SearchResult);
	}

	return bSuccessful;
}

void ARTSGameSession::OnJoinSessionComplete(FName NewSessionName, EOnJoinSessionCompleteResult::Type Result)
{
	// Get the OnlineSubsystem we want to work with
	IOnlineSubsystem * OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub == nullptr)
	{
#if WITH_EDITOR
		// TODO: log something
#endif
		return;
	}

	// Get SessionInterface from the OnlineSubsystem
	IOnlineSessionPtr Sessions = GetSessionInterface(OnlineSub);

	if (Sessions.IsValid())
	{
		// Clear the Delegate again
		Sessions->ClearOnJoinSessionCompleteDelegate_Handle(DelegateHandle_OnJoinSessionComplete);

		if (Result != EOnJoinSessionCompleteResult::Success)
		{
			return;
		}

		// Get the first local PlayerController, so we can call "ClientTravel" to get to the Server Map
		// This is something the Blueprint Node "Join Session" does automatically!
		APlayerController * const PlayerController = GetWorld()->GetFirstPlayerController();

		// We need a FString to use ClientTravel and we can let the SessionInterface contruct such a
		// String for us by giving him the SessionName and an empty String. We want to do this, because
		// Every OnlineSubsystem uses different TravelURLs
		FString TravelURL;

		if (PlayerController != nullptr && Sessions->GetResolvedConnectString(SessionName, TravelURL))
		{
			// Finally call the ClientTravel. If you want, you could print the TravelURL to see
			// how it really looks like
			PlayerController->ClientTravel(TravelURL, ETravelType::TRAVEL_Absolute);
		}
	}
}

void ARTSGameSession::StartSession()
{
	IOnlineSubsystem * OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub != nullptr)
	{
		IOnlineSessionPtr Session = GetSessionInterface(OnlineSub);
		if (Session.IsValid())
		{
			// Move this stuff to the lobby logic
			// Set the StartSession delegate handle
			DelegateHandle_OnStartSessionComplete = Session->AddOnStartSessionCompleteDelegate_Handle(OnStartSessionCompleteDelegate);

			// Our StartSessionComplete delegate should get called after this
			Session->StartSession(SessionName);
		}
	}
}

void ARTSGameSession::OnStartOnlineGameComplete(FName NewSessionName, bool bWasSuccessful)
{
	// Get the Online Subsystem so we can get the Session Interface
	IOnlineSubsystem * OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub != nullptr)
	{
		// Get the Session Interface to clear the Delegate
		IOnlineSessionPtr Session = GetSessionInterface(OnlineSub);
		if (Session.IsValid())
		{
			// Clear the delegate, since we are done with this call
			Session->ClearOnStartSessionCompleteDelegate_Handle(DelegateHandle_OnStartSessionComplete);

			GI->OnStartSessionComplete(bWasSuccessful);
		}
	}
}

void ARTSGameSession::DestroySession(EActionToTakeAfterDestroySession DoOnSessionDestroyed, const TSharedPtr<FOnlineSessionSettings> SessionToCreate)
{
	GI->SetPendingSessionSettings(SessionToCreate);

	// Cache what to do in OnDestroySessionComplete
	DoOnDestroySessionComplete = DoOnSessionDestroyed;

	IOnlineSubsystem * OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub != nullptr)
	{
		IOnlineSessionPtr Sessions = GetSessionInterface(OnlineSub);

		if (Sessions.IsValid())
		{
			Sessions->AddOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegate);

			Sessions->DestroySession(GameSessionName);

			// OnDestroySessionComplete will be called when this completes
		}
	}
}

void ARTSGameSession::OnDestroySessionComplete(FName TheSessionName, bool bWasSuccessful)
{
	// Get the OnlineSubsystem we want to work with
	IOnlineSubsystem * OnlineSub = IOnlineSubsystem::Get();

	// Get the SessionInterface from the OnlineSubsystem
	IOnlineSessionPtr Sessions = GetSessionInterface(OnlineSub);

	if (Sessions.IsValid())
	{
		// Clear the Delegate
		Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DelegateHandle_OnDestroySessionComplete);

		// If it was successful, we just load another level (could be a MainMenu!)
		if (bWasSuccessful)
		{
			switch (DoOnDestroySessionComplete)
			{
				case EActionToTakeAfterDestroySession::ReturnToLobbyBrowsingScreen:
				{
					GI->GoFromLobbyToMainMenu(); // TODO return to browser, not main menu
					break;
				}
				case EActionToTakeAfterDestroySession::HostToMainMenuClientsToLobbyBrowsing:
				{
					/* Eventually bubbles to call UGameInstance::ReturnToMainMenu */
					GetWorld()->GetAuthGameMode()->ReturnToMainMenuHost();
					break;
				}
				case EActionToTakeAfterDestroySession::CreateNewSession:
				{
					assert(GetWorld()->IsServer());

					// Get session info such as lobby name, default map etc

					const TSharedPtr <const FUniqueNetId> PlayerID = GetWorld()->GetFirstLocalPlayerFromController()->GetPreferredUniqueNetId().GetUniqueNetId();

					TSharedPtr <FOnlineSessionSettings> & PendingSessionSettings = GI->GetPendingSessionSettings();

					/* TODO: pressing lobby creation's "Create" button quickly after switching
					to it can cause a crash because pointer is null */
					FString LobbyName;
					PendingSessionSettings->Get<FString>(SessionOptions::LOBBY_NAME_KEY, LobbyName);

					FString MapID;
					PendingSessionSettings->Get<FString>(SessionOptions::MAP_NAME_KEY, MapID);

					FString StartingResources;
					PendingSessionSettings->Get<FString>(SessionOptions::STARTING_RESOURCES_KEY,
						StartingResources);

					FString DefeatCondition;
					PendingSessionSettings->Get<FString>(SessionOptions::DEFEAT_CONDITION_KEY,
						DefeatCondition);

					/* Here convert all the strings to primitive values... they just get converted again
					later so it is pointless. Alternative was to overload HostSession with FString 
					params but I don't really want two version of it floating around */
					HostSession(PlayerID, GameSessionName, FText::FromString(LobbyName),
						PendingSessionSettings->bIsLANMatch, PendingSessionSettings->bUsesPresence,
						PendingSessionSettings->NumPublicConnections, 
						FText::FromString(GI->GetLobbyPassword()), // LobbyPassword set during HostSession()
						SessionOptions::Map_ToID(MapID),
						SessionOptions::StartingResources_ToEnumValue(StartingResources), 
						SessionOptions::DefeatCondition_ToEnumValue(DefeatCondition));

					break;
				}
				default:
				{
					break;
				}
			}
		}
		else
		{
			// TODO. Most likely on Lobby widget but was made HitTestInvis while destroysession 
			// went through. Make sure to set vis to Visible if wanting to stay on it
		}
	}
	else
	{
		// TODO need something here?
	}
}

bool ARTSGameSession::GetSessionJoinability(FName InSessionName, FJoinabilitySettings & OutSettings)
{
	assert(0); // Just to see when this gets called

			   // GS has not been set during setup intentionally to see wher ethis is called
	return Super::GetSessionJoinability(InSessionName, OutSettings) && !GS->AreLobbySlotsLocked();
}

FString ARTSGameSession::ApproveLogin(const FString & Options)
{
	FString ErrorMessage = Super::ApproveLogin(Options);
	if (ErrorMessage == FString(""))
	{
		/* No errors so far. Now check if password is correct */
		const FString SentPassword = UGameplayStatics::ParseOption(Options, "password");
		if (SentPassword != GI->GetLobbyPassword())
		{
			ErrorMessage += TEXT("Incorrect password");
		}
	}

	return ErrorMessage;
}

FOnlineSessionSearch & ARTSGameSession::GetSessionSearch() const
{
	return *SessionSearch;
}

IOnlineSessionPtr ARTSGameSession::GetSessionInterface(const IOnlineSubsystem * OnlineSub) const
{
	//return OnlineSub->GetSessionInterface();
	return Online::GetSessionInterface(OnlineSub->GetSubsystemName());
}


