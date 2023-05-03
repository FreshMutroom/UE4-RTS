// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameSession.h"
#include "Online.h"

#include "Statics/CommonEnums.h"
#include "RTSGameSession.generated.h"

class FOnlineSessionSearch;
class URTSGameInstance;
class ARTSGameState;


/* What action to take when OnDestroySessionComplete */
UENUM()
enum class EActionToTakeAfterDestroySession : uint8
{
	// Do nothing. Probably not desired
	None,

	/* Return to lobby browsing screen */
	ReturnToLobbyBrowsingScreen,

	/* Return host to main menu and any connected clients to lobby browsing screen */
	HostToMainMenuClientsToLobbyBrowsing,

	/* Create a new session. Use this if trying to create a session but one is already made */
	CreateNewSession
};


class RTS_VER2_API SessionOptions
{
	// Ideally just move the private members from this class to ARTSGameSession
	friend class ARTSGameSession;

	//========================================================================================
	//	Const statics for browsing and creating lobbies
	//========================================================================================

public:

	/* Max number of lobbies shown when browsing using online subsystem null */
	static const int32 MAX_NUM_LAN_SEARCH_RESULTS;

	/* Max number of search results to look for when browsing using steam online subsystem */
	static const int32 MAX_NUM_STEAM_SEARCH_RESULTS;

	static const int32 PING_BUCKET_SIZE;

private:

	/* The key for FSessionSettings that looks up lobby name */
	static const FName LOBBY_NAME_KEY;

	/*  Key for FSessionSettings that looks up whether the lobby is password protected or not */
	static const FName IS_PASSWORD_PROTECTED_KEY;

	/* Key for FSessionSettings */
	static const FName MAP_NAME_KEY;

	/* Key for retrieving starting resources amount from session */
	static const FName STARTING_RESOURCES_KEY;

	/* Key for retrieving defeat condition for session */
	static const FName DEFEAT_CONDITION_KEY;

	/* Key for unique setting for RTS games to filter out Space Wars search results when using
	steam */
	static const FName UNIQUE_RTS_KEY;

	//=========================================================================================
	//	Conversion Functions
	//=========================================================================================
	//	Everything needs to be converted to an FString to be used with sessions. Anything 
	//	not here like lobby name just uses ToString()

	static FString Map_ToString(uint8 InMapID);
	static uint8 Map_ToID(const FString & StringForm);
	
	static FString IsPasswordProtected_ToString(bool bIsPasswordProtected);
	static bool IsPasswordProtected_ToBool(const FString & StringForm);
	
	static FString StartingResources_ToString(EStartingResourceAmount Amount);
	static EStartingResourceAmount StartingResources_ToEnumValue(const FString & StringForm);
	
	static FString DefeatCondition_ToString(EDefeatCondition DefeatCondition);
	static EDefeatCondition DefeatCondition_ToEnumValue(const FString & StringForm);

public:

	//=========================================================================================
	//	Session Query Functions
	//=========================================================================================
	//	Not sure if all previous code has been replaced with these yet  

	static bool IsLANSession(const TSharedPtr <FOnlineSessionSettings> & SessionSettings);
	static FString GetLobbyName(const TSharedPtr <FOnlineSessionSettings> & SessionSettings);
	static EStartingResourceAmount GetStartingResources(const TSharedPtr <FOnlineSessionSettings> & SessionSettings);
	static EDefeatCondition GetDefeatCondition(const TSharedPtr <FOnlineSessionSettings> & SessionSettings);
	static uint8 GetMapID(const TSharedPtr <FOnlineSessionSettings> & SessionSettings);
	// Hopefully returns the number of lobby slots needed in total
	static int32 GetNumPublicConnections(const TSharedPtr <FOnlineSessionSettings> & SessionSettings);

	//=========================================================================================
	//	Search Result Query Functions
	//=========================================================================================

	static bool IsLAN(const FOnlineSessionSearchResult & SearchResult);
	static const FString & GetHostName(const FOnlineSessionSearchResult & SearchResult);
	static FString GetServerName(const FOnlineSessionSearchResult & SearchResult);
	static int32 GetCurrentNumPlayers(const FOnlineSessionSearchResult & SearchResult);
	static int32 GetMaxNumPlayers(const FOnlineSessionSearchResult & SearchResult);
	static bool IsPasswordProtected(const FOnlineSessionSearchResult & SearchResult);
	static int32 GetPing(const FOnlineSessionSearchResult & SearchResult);

	static uint8 GetMapID(const FOnlineSessionSearchResult & SearchResult);
	static EStartingResourceAmount GetStartingResources(const FOnlineSessionSearchResult & SearchResult);
	static EDefeatCondition GetDefeatCondition(const FOnlineSessionSearchResult & SearchResult);
};


/**
 *	Most of the stuff in this class does not need to be in here; it could go in say game instance
 *	instead.
 */
UCLASS()
class RTS_VER2_API ARTSGameSession : public AGameSession
{
	GENERATED_BODY()

public:

	ARTSGameSession();

protected:

	virtual void PreInitializeComponents() override;

	// Reference to game instance
	UPROPERTY()
	URTSGameInstance * GI;

	// Reference to game state 
	UPROPERTY()
	ARTSGameState * GS;

	/* What to do in OnDestroySessionComplete */
	EActionToTakeAfterDestroySession DoOnDestroySessionComplete;

	/* Search results when looking for sessions */
	TSharedPtr <FOnlineSessionSearch> SessionSearch;

	/* Delegate stuff */

	/* Delegate called when session created */
	FOnCreateSessionCompleteDelegate OnCreateSessionCompleteDelegate;
	/* Delegate called when session started */
	FOnStartSessionCompleteDelegate OnStartSessionCompleteDelegate;
	/** Delegate for destroying a session */
	FOnDestroySessionCompleteDelegate OnDestroySessionCompleteDelegate;
	/** Delegate for searching for sessions */
	FOnFindSessionsCompleteDelegate OnFindSessionsCompleteDelegate;
	/** Delegate after joining a session */
	FOnJoinSessionCompleteDelegate OnJoinSessionCompleteDelegate;

	/** Handles to registered delegates for session */
	FDelegateHandle DelegateHandle_OnCreateSessionComplete;
	FDelegateHandle DelegateHandle_OnStartSessionComplete;
	FDelegateHandle DelegateHandle_OnDestroySessionComplete;
	FDelegateHandle DelegateHandle_OnFindSessionsComplete;
	FDelegateHandle DelegateHandle_OnJoinSessionComplete;

public:

	/**
	 *	https://wiki.unrealengine.com/How_To_Use_Sessions_In_C%2B%2B
	 *
	 *	Function to host a game!
	 *
	 *	@Param		UserID			User that started the request
	 *	@Param		NewSessionName		Name of the Session
	 *	@Param		bIsLAN			Is this is LAN Game?
	 *	@Param		bIsPresence		"Is the Session to create a presence Session"
	 *	@Param		MaxNumPlayers	        Number of Maximum allowed players on this "Session" (Server)
	 *	@Return -  true if session was created?
	 */
	bool HostSession(
		TSharedPtr<const FUniqueNetId> UserId,
		FName NewSessionName,
		const FText & LobbyName,
		bool bIsLAN,
		bool bIsPresence,
		int32 MaxNumPlayers,
		const FText & LobbyPassword,
		uint8 MapID,
		EStartingResourceAmount StartingResources,
		EDefeatCondition DefeatCondition
	);

protected:

	/**
	 * Delegate fired when a session create request has completed
	 *
	 * @param SessionName - the name of the session this callback is for
	 * @param bWasSuccessful - true if the async action completed without error, false if there was an error
	 */
	virtual void OnCreateSessionComplete(FName NewSessionName, bool bWasSuccessful);

public:

	/**
	 *	Find an online session
	 *
	 *	@param UserId - user that initiated the request
	 *	@param bIsLAN - are we searching LAN matches
	 *	@param bIsPresence - are we searching presence sessions
	 */
	void FindSessions(TSharedPtr<const FUniqueNetId> UserId, bool bIsLAN, bool bIsPresence, int32 MaxSearchResults, int32 PingBucketSize);

protected:

	/**
	 * Delegate fired when a session search query has completed
	 *
	 * @param bWasSuccessful - true if the async action completed without error, false if there was an error
	 */
	void OnFindSessionsComplete(bool bWasSuccessful);

public:

	/**
	 *	Joins a session via a search result
	 *
	 *	@param SessionName - name of session
	 *	@param SearchResult - Session to join
	 *
	 *	@return bool true if successful, false otherwise
	 */
	bool JoinSession(TSharedPtr<const FUniqueNetId> UserId, FName SessionName, const FOnlineSessionSearchResult& SearchResult);

protected:

	/**
	 * Delegate fired when a session join request has completed
	 *
	 * @param SessionName - the name of the session this callback is for
	 * @param bWasSuccessful - true if the async action completed without error, false if there was an error
	 */
	void OnJoinSessionComplete(FName NewSessionName, EOnJoinSessionCompleteResult::Type Result);

public:

	/* Start a networked match */
	void StartSession();

protected:

	/**
	 * Delegate fired when a session start request has completed
	 *
	 * @param SessionName - the name of the session this callback is for
	 * @param bWasSuccessful - true if the async action completed without error, false if there was an error
	 */
	void OnStartOnlineGameComplete(FName NewSessionName, bool bWasSuccessful);

public:

	/* Destroy session and do action after
	@param DoOnSessionDestroyed - action to do during OnDestroySessionComplete
	@param SessionToCreate - info about session to create if DoOnSessionDestroyed was 'create new
	session' */
	void DestroySession(EActionToTakeAfterDestroySession DoOnSessionDestroyed, const TSharedPtr <FOnlineSessionSettings> SessionToCreate = nullptr);

protected:

	/**
	 * Delegate fired when a destroying an online session has completed
	 *
	 * @param SessionName - the name of the session this callback is for
	 * @param bWasSuccessful - true if the async action completed without error, false if there was an error
	 */
	virtual void OnDestroySessionComplete(FName TheSessionName, bool bWasSuccessful);

public:

	virtual bool GetSessionJoinability(FName InSessionName, FJoinabilitySettings & OutSettings) override;

	/* Function for checking if password is correct */
	virtual FString ApproveLogin(const FString & Options) override;

	FOnlineSessionSearch & GetSessionSearch() const;

protected:

	IOnlineSessionPtr GetSessionInterface(const IOnlineSubsystem * OnlineSub) const;
};
