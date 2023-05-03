// Fill out your copyright notice in the Description page of Project Settings.

#include "LobbyBrowsing.h"
#include "Components/TextBlock.h"
#include "Components/PanelWidget.h"
#include "Components/Throbber.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/EditableText.h"
#include "Components/CircularThrobber.h"
#include "Online.h"

#include "Networking/RTSGameSession.h"
// For widget creation only, maybe possible to delete later
#include "GameFramework/RTSGameInstance.h"
#include "UI/MainMenu/Menus.h"
#include "UI/MainMenu/LobbyCreationWidget.h"
#include "GameFramework/RTSGameMode.h"
#include "GameFramework/RTSLocalPlayer.h"
#include "Statics/DevelopmentStatics.h"
#include "UI/MainMenu/MatchRulesWidget.h"
#include "UI/MainMenu/MapSelection.h"


//===============================================================================================
//	Single Lobby Info Widget
//===============================================================================================

bool ULobbyInfo::Setup()
{
	if (Super::Setup())
	{
		return true;
	}

	if (IsWidgetBound(Widget_MatchRules))
	{
		Widget_MatchRules->Setup();
	}

	if (IsWidgetBound(Widget_MapInfo))
	{
		Widget_MapInfo->Setup();
	}

	if (IsWidgetBound(Button_Select))
	{
		Button_Select->OnClicked.AddDynamic(this, &ULobbyInfo::UIBinding_OnSelectButtonClicked);
	}

	return false;
}

void ULobbyInfo::OnAddToList(UPanelWidget * OwningPanel)
{
}

void ULobbyInfo::SetValues(const FOnlineSessionSearchResult & SearchResult)
{
	if (IsWidgetBound(Text_HostName))
	{
		Text_HostName->SetText(FText::FromString(SessionOptions::GetHostName(SearchResult)));
	}
	if (IsWidgetBound(Text_LobbyName))
	{
		Text_LobbyName->SetText(FText::FromString(SessionOptions::GetServerName(SearchResult)));
	}
	if (IsWidgetBound(Text_NetworkType))
	{
		const bool bIsLAN = SessionOptions::IsLAN(SearchResult);
		const FText NetworkTypeText = bIsLAN ? LobbyOptions::LAN_TEXT : LobbyOptions::ONLINE_TEXT;
		Text_NetworkType->SetText(NetworkTypeText);
	}
	if (IsWidgetBound(Text_CurrentNumPlayers))
	{
		const FText NumPlayersText = Statics::IntToText(SessionOptions::GetCurrentNumPlayers(SearchResult));
		Text_CurrentNumPlayers->SetText(NumPlayersText);
	}
	if (IsWidgetBound(Text_MaxNumPlayers))
	{
		const FText NumPlayersText = Statics::IntToText(SessionOptions::GetMaxNumPlayers(SearchResult));
		Text_MaxNumPlayers->SetText(NumPlayersText);
	}
	if (IsWidgetBound(Image_PasswordProtected))
	{
		const ESlateVisibility NewVisibility = (SessionOptions::IsPasswordProtected(SearchResult))
			? ESlateVisibility::Visible : ESlateVisibility::Collapsed;

		Image_PasswordProtected->SetVisibility(NewVisibility);
	}
	if (IsWidgetBound(Text_Ping))
	{
		Text_Ping->SetText(Statics::IntToText(SessionOptions::GetPing(SearchResult)));
	}
	if (IsWidgetBound(Widget_MatchRules))
	{
		Widget_MatchRules->SetStartingResources(SessionOptions::GetStartingResources(SearchResult));

		Widget_MatchRules->SetDefeatCondition(SessionOptions::GetDefeatCondition(SearchResult));
	}
	if (IsWidgetBound(Widget_MapInfo))
	{
		Widget_MapInfo->SetMap(&GI->GetMapInfo(SessionOptions::GetMapID(SearchResult)), bTryShowPlayerStartLocations 
			? EPlayerStartDisplay::VisibleButUnclickable : EPlayerStartDisplay::Hidden);
	}
}

void ULobbyInfo::SetLobbyBrowserWidget(ULobbyBrowserWidget * InLobbyBrowserWidget)
{
	LobbyBrowserWidget = InLobbyBrowserWidget;
}

void ULobbyInfo::UIBinding_OnSelectButtonClicked()
{
	LobbyBrowserWidget->SetCurrentlySelectedLobby(this);
}


//===============================================================================================
//	Lobby Browser Widget
//===============================================================================================

bool ULobbyBrowserWidget::Setup()
{
	if (Super::Setup())
	{
		return true;
	}

	if (IsWidgetBound(Button_Join))
	{
		Button_Join->OnClicked.AddDynamic(this, &ULobbyBrowserWidget::UIBinding_OnJoinButtonClicked);
	}
	if (IsWidgetBound(Button_Return))
	{
		Button_Return->OnClicked.AddDynamic(this, &ULobbyBrowserWidget::UIBinding_OnReturnButtonClicked);
	}
	if (IsWidgetBound(Button_Refresh))
	{
		Button_Refresh->OnClicked.AddDynamic(this, &ULobbyBrowserWidget::UIBinding_OnRefreshButtonClicked);
	}
	if (IsWidgetBound(Text_IPConnect))
	{
		Text_IPConnect->OnTextCommitted.AddDynamic(this, &ULobbyBrowserWidget::UIBinding_OnDirectIPConnectTextCommitted);
	}

	return false;
}

void ULobbyBrowserWidget::OnShown()
{
	/* If pressing back and then returning to widget it may still be searching
	for results. I think this is fine. Just let is complete but do not start new search */
	if (!bIsFindingSessions)
	{
		/* Start finding sessions the moment widget is opened */
		FindSessions();
	}
}

void ULobbyBrowserWidget::OnSearchingStarted()
{
	/* Prevent trying to search for sessions while already searching */
	bIsFindingSessions = true;

	/* Show throbber(s) */
	if (IsWidgetBound(Throbber_Searching))
	{
		Throbber_Searching->SetVisibility(ESlateVisibility::Visible);
	}
	if (IsWidgetBound(CircularThrobber_Searching))
	{
		CircularThrobber_Searching->SetVisibility(ESlateVisibility::Visible);
	}

	// Clear num results text and show "searching" text
	if (IsWidgetBound(Text_NumSearchResults))
	{
		Text_NumSearchResults->SetText(FText::FromString("Searching..."));
	}
}

void ULobbyBrowserWidget::OnSearchingComplete(FOnlineSessionSearch & SessionSearch)
{
	TArray <FOnlineSessionSearchResult> & SearchResults = SessionSearch.SearchResults;

	/* Hide throbber(s) */
	if (IsWidgetBound(Throbber_Searching))
	{
		Throbber_Searching->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (IsWidgetBound(CircularThrobber_Searching))
	{
		CircularThrobber_Searching->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (IsWidgetBound(Text_NumSearchResults))
	{
		const FString ToShow = FString::FromInt(SearchResults.Num()) + FString(" lobbies found");
		Text_NumSearchResults->SetText(FText::FromString(ToShow));
	}

	/* Return if there is no widget to update */
	if (!IsWidgetBound(Panel_SearchResults))
	{
		return;
	}

	UE_CLOG(SearchResult_BP == nullptr, RTSLOG, Fatal, TEXT("Search Result Widget is null. Set it "
		"in lobby browser widget blueprint"));
		
	/* Sort search results. Sort based on ping */

	// Insertion sort. TODO: use nlog(n) sort instead 
	for (int32 i = 1; i < SearchResults.Num(); ++i)
	{
		int32 j = 1;
		while (j > 0 && (SessionOptions::GetPing(SearchResults[j - 1]) > SessionOptions::GetPing(SearchResults[j])))
		{
			const FOnlineSessionSearchResult & Temp = SearchResults[j - 1];
			SearchResults[j - 1] = SearchResults[j];
			SearchResults[j] = Temp;
			j--;
		}
	}

	/* Remove all previous search results */
	Panel_SearchResults->ClearChildren();

	/* Add new search results to widget */
	for (int32 i = 0; i < SearchResults.Num(); ++i)
	{
		const FOnlineSessionSearchResult & SearchResult = SearchResults[i];

		/* Either reuse a widget or create a new one if needed */
		ULobbyInfo * Widget = nullptr;
		if (SearchResultWidgets.IsValidIndex(i))
		{
			Widget = SearchResultWidgets[i];
		}
		else
		{
			// TODO: Widget created with GI as owner. Change to GetWorld() for GC when loading 
			// new level? Do this for all other widgets created in GI too?
			Widget = CreateWidget<ULobbyInfo>(GI, SearchResult_BP);
			SearchResultWidgets.Emplace(Widget);
		}

		Widget->OnAddToList(Panel_SearchResults);
		Widget->SetLobbyBrowserWidget(this);
		Widget->SetValues(SearchResult);
		Panel_SearchResults->AddChild(Widget);
	}

	bIsFindingSessions = false;
}

void ULobbyBrowserWidget::UIBinding_OnJoinButtonClicked()
{
	if (bIsFindingSessions)
	{
		/* For now disable joining until search has completed */
		return;
	}

	FOnlineSessionSearchResult * const SearchResult = GetSearchResult();
	if (SearchResult != nullptr)
	{
		/* Wondering how password is sent? Check out URTSLocalPlayer::GetGameLoginOptions. It
		returns the Options string that AGameModeBase::PreLogin uses which calls
		AGameSession::ApproveLogin. Check out ARTSGameSession::ApproveLogin to see how the
		server handles the password and ARTSGameMode::PostLogin to see how it handles assigning
		the default faction */
		if (SessionOptions::IsPasswordProtected(*SearchResult))
		{
			/* Show password entry widget on top of this one */
			UPasswordEntryWidget * PasswordWidget = CastChecked<UPasswordEntryWidget>(GI->GetWidget(EWidgetType::PasswordEntry));
			PasswordWidget->SetLobbyBrowserWidget(this);
			GI->ShowWidget(EWidgetType::PasswordEntry, ESlateVisibility::HitTestInvisible);
		}
		else
		{
			//TODO: Disable input while this goes through?

			/* Clear password. Server will ignore it anyway so save some bandwidth */
			URTSLocalPlayer * Player = CastChecked<URTSLocalPlayer>(GetWorld()->GetFirstLocalPlayerFromController());
			Player->SetPassword(TEXT(""));

			GI->TryJoinNetworkedSession(*SearchResult);

			/* The RPC the client will wait on now is ARTSPlayerController::Client_ShowLobbyWidget */
		}
	}
}

void ULobbyBrowserWidget::UIBinding_OnReturnButtonClicked()
{
	if (bIsFindingSessions)
	{
		/* For now disable returning until search has completed */
		return;
	}

	GI->ShowPreviousWidget();
}

void ULobbyBrowserWidget::UIBinding_OnRefreshButtonClicked()
{
	if (bIsFindingSessions)
	{
		/* Do nothing while sessions are already being searched for */
		return;
	}

	// TODO add a throttle here, like refreshes done less than 10 secs apart incur a 2 sec delay
	// Also might need a OnAddedToViewport func for widget because it needs to refresh when it 
	// is shown

	FindSessions();
}

void ULobbyBrowserWidget::UIBinding_OnDirectIPConnectTextCommitted(const FText & NewText, ETextCommit::Type CommitMethod)
{
	if (CommitMethod == ETextCommit::OnEnter)
	{
		// Try connect to LAN server
		GetWorld()->Exec(GetWorld(), *FString("open " + NewText.ToString()));
	}
}

void ULobbyBrowserWidget::FindSessions()
{
	OnSearchingStarted();

	GI->SearchForNetworkedLobbies();

	/* Wait for ARTSGameSession::OnFindSessionsComplete to fire */
}

FOnlineSessionSearchResult * ULobbyBrowserWidget::GetSearchResult() const
{
	if (CurrentlySelectedLobby != nullptr)
	{
		/* Get search result data corrisponding to selected lobby */
		ARTSGameMode * const GameMode = CastChecked<ARTSGameMode>(GetWorld()->GetAuthGameMode());
		ARTSGameSession * const GameSesh = CastChecked<ARTSGameSession>(GameMode->GameSession);
		/* Can avoid array iteration and store index during OnSearchingComplete */
		for (int32 i = 0; i < SearchResultWidgets.Num(); ++i)
		{
			ULobbyInfo * const Widget = SearchResultWidgets[i];
			if (Widget == CurrentlySelectedLobby)
			{
				return &GameSesh->GetSessionSearch().SearchResults[i];
			}
		}
#if WITH_EDITOR
		// TODO: log something like "expected widget to be in array but was not"	
#endif
	}

	return nullptr;
}

void ULobbyBrowserWidget::SetCurrentlySelectedLobby(ULobbyInfo * NewLobby)
{
	CurrentlySelectedLobby = NewLobby;
}

bool UPasswordEntryWidget::Setup()
{
	if (Super::Setup())
	{
		return true;
	}

	SetupBindings();

	return false;
}

void UPasswordEntryWidget::SetupBindings()
{
	if (IsWidgetBound(Button_Accept))
	{
		Button_Accept->OnClicked.AddDynamic(this, &UPasswordEntryWidget::UIBinding_OnAcceptButtonClicked);
	}
	if (IsWidgetBound(Button_Return))
	{
		Button_Return->OnClicked.AddDynamic(this, &UPasswordEntryWidget::UIBinding_OnReturnButtonClicked);
	}
}

void UPasswordEntryWidget::UIBinding_OnPasswordEntryTextChanged(const FText & NewText)
{
	Text_PasswordEntry->SetText(Statics::ConcateText(NewText, LobbyOptions::MAX_PASSWORD_LENGTH));
}

void UPasswordEntryWidget::UIBinding_OnPasswordEntryTextCommitted(const FText & NewText, ETextCommit::Type CommitMethod)
{
	if (CommitMethod == ETextCommit::OnEnter)
	{
		TryPassword();
	}
	else if (CommitMethod == ETextCommit::OnCleared)
	{
		Text_PasswordEntry->SetText(Menus::BLANK_TEXT);
	}
}

void UPasswordEntryWidget::UIBinding_OnAcceptButtonClicked()
{
	TryPassword();
}

void UPasswordEntryWidget::UIBinding_OnReturnButtonClicked()
{
	GI->ShowPreviousWidget();
}

void UPasswordEntryWidget::TryPassword()
{
	// TODO: disable input while this goes through?

	const FOnlineSessionSearchResult & SearchResult = *LobbyBrowserWidget->GetSearchResult();

	const FString & Password = Text_PasswordEntry->GetText().ToString();

	URTSLocalPlayer * const Player = CastChecked<URTSLocalPlayer>(GetWorld()->GetFirstPlayerController()->GetLocalPlayer());
	Player->SetPassword(Password);

	GI->TryJoinNetworkedSession(SearchResult);
}

void UPasswordEntryWidget::SetLobbyBrowserWidget(ULobbyBrowserWidget * InLobbyBrowserWidget)
{
	LobbyBrowserWidget = InLobbyBrowserWidget;
}
