// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/MainMenu/MainMenuWidgetBase.h"
#include "LobbyBrowsing.generated.h"

class UTextBlock;
class UImage;
class UButton;
class UThrobber;
class FOnlineSessionSearch;
class UMapInfoWidget;
class ULobbyBrowserWidget;
class UEditableText;
class UCircularThrobber;
class UMatchRulesWidget;

/**
 *	The class LobbyBrowsing used to be here but was moved to RTSGameSession.h
 */


/**
 *	A widget that can show information about a lobby when browsing for lobbies
 */
UCLASS(Abstract)
class RTS_VER2_API ULobbyInfo : public UMainMenuWidgetBase
{
	GENERATED_BODY()

public:

	virtual bool Setup() override;

	/* Called when added as child to search results. Allows for adjusting position if the default
	AddChild does not make it look ok */
	virtual void OnAddToList(UPanelWidget * OwningPanel);

	/* Set values to display from a search result */
	void SetValues(const FOnlineSessionSearchResult & SearchResult);

	void SetLobbyBrowserWidget(ULobbyBrowserWidget * InLobbyBrowserWidget);

protected:

	/** 
	 *	Whether to show the player start locations on the map image. Which specific player is 
	 *	assigned to which spot will not be shown either way
	 */
	UPROPERTY(EditAnywhere, Category = "RTS")
	bool bTryShowPlayerStartLocations;

	/* Reference to the lobby browser that owns this */
	UPROPERTY()
	ULobbyBrowserWidget * LobbyBrowserWidget;

	/* Widget to display name of lobby host */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_HostName;

	/* Widget to display lobby name */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_LobbyName;

	/* Widget to display the network type e.g. LAN, online */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_NetworkType;

	/* Widget to diplay the current number of players in the lobby */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_CurrentNumPlayers;

	/* Widget to display the maximum number of players the lobby can have */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_MaxNumPlayers;

	/* Image to display signalling whether lobby is password protected or not */
	UPROPERTY(meta = (BindWidgetOptional))
	UImage * Image_PasswordProtected;

	/* Widget to display the ping in ms */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_Ping;

	/* Widget to display match rules. This one doesn't need any combo boxes to change options
	because it should only be displaying options */
	UPROPERTY(meta = (BindWidgetOptional))
	UMatchRulesWidget * Widget_MatchRules;

	/* Widget to display map info */
	UPROPERTY(meta = (BindWidgetOptional))
	UMapInfoWidget * Widget_MapInfo;

	/* The button to click to select this lobby. Can be the root of all other widgets */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton * Button_Select;

	UFUNCTION()
	void UIBinding_OnSelectButtonClicked();
};


UCLASS(Abstract)
class RTS_VER2_API ULobbyBrowserWidget : public UMainMenuWidgetBase
{
	GENERATED_BODY()

public:

	virtual bool Setup() override;

	virtual void OnShown() override;

	virtual void OnSearchingStarted();

	/* Param non-const because they are sorted */
	virtual void OnSearchingComplete(FOnlineSessionSearch & SessionSearch);

protected:

	/* Array of widgets previously created for showing a search results. When new search results
	come in the widgets in this array are modified first before any new widgets are created to
	avoid creating unnecessary widgets and losing references to them */
	UPROPERTY()
	TArray < ULobbyInfo * > SearchResultWidgets;

	/* Button to join current selected lobby */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton * Button_Join;

	/* Button to return to previous menu */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton * Button_Return;

	/* Refresh search results */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton * Button_Refresh;

	/* The currently selected lobby. Pressing join button will join this lobby */
	UPROPERTY()
	ULobbyInfo * CurrentlySelectedLobby;

	/* The widget to use for displaying each individual search result */
	UPROPERTY(EditAnywhere, Category = "RTS", meta = (DisplayName = "Search Result Widget"))
	TSubclassOf <ULobbyInfo> SearchResult_BP;

	/* The widget browsing results get added to e.g. scroll box.

	Results get added with UWidget::AddChild. In editor to preview what the results would
	look like add multiple of whatever widget is set as Search Result Widget to this widget.
	What it looks like is how search results will be displayed */
	UPROPERTY(meta = (BindWidgetOptional))
	UPanelWidget * Panel_SearchResults;

	/* Throbber to display when searching is complete */
	UPROPERTY(meta = (BindWidgetOptional))
	UThrobber * Throbber_Searching;

	/* Circular throbber if you wish to use that instead */
	UPROPERTY(meta = (BindWidgetOptional))
	UCircularThrobber * CircularThrobber_Searching;

	/* Displays how many search results were found */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_NumSearchResults;

	/* This is mainly here for testing. Type in an IP address to connect to it */
	UPROPERTY(meta = (BindWidgetOptional))
	UEditableText * Text_IPConnect;

	UFUNCTION()
	void UIBinding_OnJoinButtonClicked();

	UFUNCTION()
	void UIBinding_OnReturnButtonClicked();

	UFUNCTION()
	void UIBinding_OnRefreshButtonClicked();

	UFUNCTION()
	void UIBinding_OnDirectIPConnectTextCommitted(const FText & NewText, ETextCommit::Type CommitMethod);

	// Call to populate server list with up to date sessions
	void FindSessions();

	// True if refreshing server list 
	bool bIsFindingSessions;

public:

	/* Get a pointer to the search result of CurrentlySelectedLobby or null if lobby is null */
	FOnlineSessionSearchResult * GetSearchResult() const;

	void SetCurrentlySelectedLobby(ULobbyInfo * NewLobby);
};


UCLASS(Abstract)
class RTS_VER2_API UPasswordEntryWidget : public UMainMenuWidgetBase
{
	GENERATED_BODY()

public:

	virtual bool Setup() override;

protected:

	void SetupBindings();

	/* Reference to browser widget */
	UPROPERTY()
	ULobbyBrowserWidget * LobbyBrowserWidget;

	/* Text box to enter password into */
	UPROPERTY(meta = (BindWidgetOptional))
	UEditableText * Text_PasswordEntry;

	/* Accept button to try password */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton * Button_Accept;

	/* Return button to not try password */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton * Button_Return;

	UFUNCTION()
	void UIBinding_OnPasswordEntryTextChanged(const FText & NewText);

	UFUNCTION()
	void UIBinding_OnPasswordEntryTextCommitted(const FText & NewText, ETextCommit::Type CommitMethod);

	UFUNCTION()
	void UIBinding_OnAcceptButtonClicked();

	UFUNCTION()
	void UIBinding_OnReturnButtonClicked();

	/* Try join session with password entered in text box and potentially join session */
	void TryPassword();

public:

	/* Get pointer to owning browser widget */
	void SetLobbyBrowserWidget(ULobbyBrowserWidget * InLobbyBrowserWidget);
};
