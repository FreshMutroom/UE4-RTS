// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/MainMenu/MainMenuWidgetBase.h"

#include "Statics/CommonEnums.h"
#include "Statics/OtherEnums.h"
#include "Lobby.generated.h"

class URTSGameInstance;
class UTextBlock;
class UImage;
class UComboBoxString;
class UButton;
class UEditableText;
class UMapInfoWidget;
class FOnlineSessionSettings;
struct FMapInfo;
class ARTSPlayerState;
class ULobbyWidget;
class UMatchRulesWidget;
class ARTSPlayerController;
class UPanelWidget;


/** 
 *	Widget for a single message received that should be placed in chat log.
 *
 *	Tips on how to create this:
 *	Remove the canvas panel then just add a text block to the widget. Rename it Text_Message 
 *	and make it a variable. Under the category "Wrapping" enable "Auto Wrap Text". In advanced 
 *	options change the wrapping policy to "Allow Per Character Wrapping"
 */
UCLASS(Abstract)
class RTS_VER2_API UChatOutputSingleMessageWidget : public UMainMenuWidgetBase
{
	GENERATED_BODY()

public:

	UChatOutputSingleMessageWidget(const FObjectInitializer & ObjectInitializer);

	virtual bool Setup() override;

protected:

	/** Animation to play when the message is received. */
	UPROPERTY(VisibleAnywhere, Category = "RTS")
	FString ReceivedAnimName;

	/* Anim to play when message is received */
	UPROPERTY()
	UWidgetAnimation * ReceivedAnimation;

	/** The text that shows the message. Is pretty core to this widget so shouldn't be optional */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_Message;

	FSlateColor OriginalTextColor;

public:

	/* Set the text for Text_Message */
	void SetMessage(const FText & InMessage, bool bOverrideTextColor = false, 
		const FLinearColor & TextColor = FLinearColor());

	// Play animation for when this message is received
	void PlayReceivedAnim();

	/* Overriding this from UMainMenuWidgetBase so it does nothing */
	virtual void OnAnimationFinished_Implementation(const UWidgetAnimation * Animation) override;
};


/* TODO for this class:
- Need to implement RemoveMessagesToFit(). Currently widgets are never removed from chat log.
- When adding a new message to chat log the text block's unwrapped version flashes on screen 
quickly which is not pretty. 
- Also this class has dependency on Panel_Output being a VBox and some widgets beingplaced 
on a canvas panel because they cast their slot to UCanvasPanelSlot */

/** 
 *	Widget for the chat in a lobby. Made its own seperate widget so singleplayer lobbies can hide
 *	all chat related widgets easily 
 */
UCLASS(Abstract)
class RTS_VER2_API ULobbyChat : public UMainMenuWidgetBase
{
	GENERATED_BODY()

public:

	ULobbyChat(const FObjectInitializer & ObjectInitializer);

	virtual bool Setup() override;

protected:

	// Number of chat messages in log
	int32 NumChatLogMessages;

	/** 
	 *	Max length a chat message can be ignoring any extra things tagged onto the message like 
	 *	the sender's name 
	 */
	UPROPERTY(EditAnywhere, Category = "RTS", meta = (ClampMin = 1))
	int32 MaxMessageLength;

	/** What to put at the start of the text block for message input. This does not get sent */
	UPROPERTY(EditAnywhere, Category = "RTS")
	FString InputMessageStart;

	/** For received messages, what to put before chat message sender's name */
	UPROPERTY(EditAnywhere, Category = "RTS")
	FString OutputMessageStart;

	/**
	 *	For received messages, what to put in between the message sender's name and what they 
	 *	typed 
	 */
	UPROPERTY(EditAnywhere, Category = "RTS")
	FString OutputMessageMiddle;

	/* Box to type chat into */
	UPROPERTY(meta = (BindWidgetOptional))
	UEditableText * Text_Input;

	/**
	 *	The panel widget that holds all the chat messages received from others. Must either be 
	 *	a vertical box or a scroll box. Actually only vertical box is an option at the moment 
	 *	Single message widgets are added using UPanelWidget::AddChild and 
	 *	ULobbyChat::OnChildAddedToOutputPanel will be called after. 
	 *	There is an assumption no children will be added to this panel in editor, and only 
	 *	the single message widgets should be added to it at runtime
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	UPanelWidget * Panel_Output;

	/* The widget to use for each seperate message that should be in the chat output */
	UPROPERTY(EditAnywhere, Category = "RTS")
	TSubclassOf < UChatOutputSingleMessageWidget > OutputMessage_BP;

	/* This is for widget recycling only. The last widget that was removed from chat log */
	UPROPERTY()
	TWeakObjectPtr < UChatOutputSingleMessageWidget > LastRemovedMessageWidget;

	/* Sound to play when a message is received */
	UPROPERTY(EditAnywhere, Category = "RTS")
	USoundBase * ReceivedMessageSound;

	UFUNCTION()
	void UIBinding_OnChatInputTextChanged(const FText & NewText);

	UFUNCTION()
	void UIBinding_OnChatInputTextCommitted(const FText & Text, ETextCommit::Type CommitMethod);

	/* Create a spacer and add it to the output panel */
	void CreateAndAddOutputSpacer();

	/* Remove messages from Panel_Output until all the messages in it will fit. Remove 
	oldest messages first */
	void RemoveMessagesToFit();

	/** 
	 *	Called soon after a single message widget is added to Panel_Output 
	 *	@param WidgetJustAdded - widget that was just added
	 *	@param PanelSlot - the slot WidgetJustAdded belongs to 
	 */
	virtual void OnChildAddedToOutputPanel(UChatOutputSingleMessageWidget * WidgetJustAdded, 
		UPanelSlot * PanelSlot);

	/* Return whether we should play a sound because a message was just received */
	virtual bool ShouldPlayMessageReceivedSound();

	/* GetWorld()->GetRealTimeSeconds() last time a message was received. 
	For ShouldPlayMessageReceivedSound */
	float TimeAtLastMessageReceived;

public:

	void SetupFor(EMatchType InLobbyType);

	/* Called when a chat message is received 
	@param SendersName - name of player who sent message 
	@param Message - what they typed */
	void OnChatMessageReceived(const FString & SendersName, const FString & Message);

	void ClearChat();
};


/**
 *	Widget for a slot in a lobby
 */
UCLASS(Abstract)
class RTS_VER2_API ULobbySlot : public UMainMenuWidgetBase
{
	GENERATED_BODY()

public:

	ULobbySlot(const FObjectInitializer & ObjectInitializer);

	void PreSetup(ULobbyWidget * InOwningLobby, int32 InSlotIndex);

	virtual bool Setup() override;

protected:

	/* The status of the slot e.g. whether it is closed, has CPU player etc */
	ELobbySlotStatus Status;

	/* Reference to player state for player in this slot. Null if CPU player */
	UPROPERTY()
	ARTSPlayerState * PlayerState;

	/* Index in owning lobby slots box's Slots array */
	int32 SlotIndex;

	//==========================================================================================
	//	BindWidgetOptional widgets
	//==========================================================================================

	/* This text shows a human players profile name as well as text indicating if the slot is
	open/closed/CPU */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_PlayerName;

	UPROPERTY(meta = (BindWidgetOptional))
	UImage * Image_PlayerIcon;

	/* Combo box to select faction */
	UPROPERTY(meta = (BindWidgetOptional))
	UComboBoxString * ComboBox_Faction;

	/* Shows the faction's display image */
	UPROPERTY(meta = (BindWidgetOptional))
	UImage * Image_Faction;

	/* Combo box to select team */
	UPROPERTY(meta = (BindWidgetOptional))
	UComboBoxString * ComboBox_Team;

	/* Host only. Button to add a CPU player */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton * Button_AddCPUPlayer;

	/* Text to show CPU player difficulty. Should be blank for human players */
	UPROPERTY(meta = (BindWidgetOptional))
	UComboBoxString * ComboBox_CPUDifficulty;

	/* Host only. Button to remove what is in slot. In multiplayer this would kick the player */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton * Button_Kick;

	/* Host only. Close slot so no one can join but only if unoccupied. Re-open slot from lobby
	widget's add slot button */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton * Button_Close;

	//==========================================================================================
	//	Other stuff
	//==========================================================================================

	/* Reference to lobby widget this is a part of */
	UPROPERTY()
	ULobbyWidget * Lobby;

	UFUNCTION()
	void UIBinding_OnAddCPUPlayerButtonClicked();

	UFUNCTION()
	void UIBinding_OnCPUDifficultySelectionChanged(FString Item, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void UIBinding_OnKickButtonClicked();

	/* Return whether the local player controller is host or not */
	bool IsLocalPlayerHost() const;

	UFUNCTION()
	void UIBinding_OnCloseButtonClicked();

	UFUNCTION()
	void UIBinding_OnFactionSelectionChanged(FString Item, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void UIBinding_OnTeamSelectionChanged(FString Item, ESelectInfo::Type SelectionType);

	void Close();

	void SetSlotVisibility(ESlateVisibility NewVis);
	void SetWidgetVisibility(UWidget * Widget, ESlateVisibility NewVisibility);

public:

	int32 GetSlotIndex() const;

	/* True if slot has human player in it */
	bool HasHumanPlayer() const;

	/* True if slot has CPU player in it */
	bool HasCPUPlayer() const;

	ARTSPlayerState * GetPlayerState() const;
	void SetPlayerState(ARTSPlayerState * InPlayerState, bool bUpdateStatusToo);
	ELobbySlotStatus GetStatus() const;
	void SetStatus(ELobbySlotStatus NewPlayerType, bool bUpdatePlayerStateToo);
	ECPUDifficulty GetCPUDifficulty() const;
	void SetCPUDifficulty(ECPUDifficulty NewDifficulty);
	ETeam GetTeam() const;
	void SetTeam(ETeam NewTeam);
	EFaction GetFaction() const;
	void SetFaction(EFaction NewFaction);

	/* Update clickability of buttons or visibility due to change */
	void UpdateVisibilities();

	bool IsMultiplayerLobby() const;
};


/* A reason the host cannot start the match */
UENUM()
enum class ECannotStartMatchReason : uint8
{
	// Need more players in lobby
	NotEnoughPlayers,
	
	// Need 2+ different teams
	EveryonesOnOneTeam,
	
	// Lobby slots are not locked
	SlotsNotLocked,

	// No reason, can actually start match
	NoReason, 

	z_ALWAYS_LAST_IN_ENUM
};


/** 
 *	Simple struct that holds a FText and a USoundBase 
 */
USTRUCT()
struct FTextAndSound
{
	GENERATED_BODY()

protected:

	UPROPERTY(EditAnywhere, Category = "RTS")
	FText Message;

	UPROPERTY(EditAnywhere, Category = "RTS")
	USoundBase * Sound;

public:

	FTextAndSound()
	{
	}

	FTextAndSound(const FText & InText, USoundBase * InSound)
	{
		Message = InText;
		Sound = InSound;
	}

	const FText & GetMessage() const { return Message; }
	USoundBase * GetSound() const { return Sound; }
};


/**
 *	Main widget for the match lobby
 *
 *	Some notes: If buttons aren't clickable then may have to increase their Z order. The other 
 *	user widgets like Widget_Chat if anchored to the whole screen can get in the way
 */
UCLASS(Abstract)
class RTS_VER2_API ULobbyWidget : public UMainMenuWidgetBase
{
	GENERATED_BODY()

public:

	//==========================================================================================
	//	Default Values
	//==========================================================================================

	/* When creating a sigleplayer lobby for the first time what to call it */
	static const FText SINGLEPLAYER_DEFAULT_LOBBY_NAME;

	/* When creating a singleplayer lobby for the first time the default number of CPU opponents 
	to have */
	static const uint32 DEFAULT_NUM_CPU_OPPONENTS;

	/* When adding CPU players their default difficulty */
	static const ECPUDifficulty DEFAULT_CPU_DIFFICULTY;

	/* When a new human/CPU joins lobby the team they are placed on */
	static const ETeam DEFAULT_TEAM;

	ULobbyWidget(const FObjectInitializer & ObjectInitializer);

protected:

	virtual bool Setup() override;

	void SetupSlots();

	// Called right after AddChild is called on panel to give user a chance to adjust position of slot
	virtual void OnLobbySlotConstructed(UPanelWidget * OwningPanel, ULobbySlot * RecentlyAddedSlot);

	/* Type of match this is for e.g. LAN, offline etc */
	EMatchType LobbyType;

	/* Return true if local player is the host for the lobby */
	bool IsHost() const;

	/** 
	 *	Whether the image displaying the map should have player start locations also shown on it 
	 *	so players can click on them to choose where they will start in match.
	 */
	UPROPERTY(EditAnywhere, Category = "RTS")
	EPlayerStartDisplay PlayerStartDisplayRule;

	/** 
	 *	If true the widget bound to Panel_LobbySlots will be auto populated with the lobby
	 *	slot widget specified below at runtime. If false then you will need to place ULobbySlot
	 *	widgets onto the widget yourself in editor. They do not have to on a panel widget.
	 *	The naming scheme for them should be Widget_Slot1, Widget_Slot2. Order is important - slot 1
	 *	will be considered the 'first' slot and is usually populated by the host. Make sure to add as
	 *	many slot widgets as UStatics::MAX_NUM_PLAYERS 
	 */
	UPROPERTY(EditAnywhere, Category = "RTS")
	bool bAutoPopulateSlotsPanel;

	/* Widget to use for a lobby slot. They will be added to Panel_LobbySlots when required. */
	UPROPERTY(EditAnywhere, Category = "RTS", meta = (DisplayName = "Lobby Slot Widget", EditCondition = bAutoPopulateSlotsPanel))
	TSubclassOf < ULobbySlot > LobbySlot_BP;

	/** The messages and sounds to display when trying to start the game but cannot */
	UPROPERTY(EditAnywhere, EditFixedSize, Category = "RTS")
	TMap < ECannotStartMatchReason, FTextAndSound > CannotStartWarnings;

	UPROPERTY()
	TArray < ULobbySlot * > Slots;

	// The map that is set for lobby
	const FMapInfo * Map;

	//==========================================================================================
	//	BindWidget widgets
	//==========================================================================================

	/* Text that has lobby name - the name the player chose for the lobby */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_LobbyName;

	/* Button to start match. Only visible to host */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton * Button_StartMatch;

	/* Button to return to main menu */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton * Button_Return;

	/* Button to add a slot. Only visible to host */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton * Button_AddSlot;

	/* Button to lock slots. Only visible if lobby is a multiplayer lobby and you are the host */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton * Button_LockSlots;

	/* Button to change map. Only visible to host */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton * Button_ChangeMap;

	/** Panel widget for displaying each slot. Only relevant if bAutoPopulateSlotsPanel is true */
	UPROPERTY(meta = (BindWidgetOptional))
	UPanelWidget * Panel_LobbySlots;

	/* Widget displaying the match rules */
	UPROPERTY(meta = (BindWidgetOptional))
	UMatchRulesWidget * Widget_MatchRules;

	/* Displays info about the map match will be played on */
	UPROPERTY(meta = (BindWidgetOptional))
	UMapInfoWidget * Widget_MapInfo;

	/* Image to show whether slots are locked or not */
	UPROPERTY(meta = (BindWidgetOptional))
	UImage * Image_LockSlots;

	/* Widget for chat input and output */
	UPROPERTY(meta = (BindWidgetOptional))
	ULobbyChat * Widget_Chat;

	//==========================================================================================
	//	Other stuff
	//==========================================================================================

	UFUNCTION()
	void UIBinding_OnStartMatchButtonClicked();

public:

	void OnStartSessionComplete(bool bWasSuccessful);

protected:

	/* Take all the info set in the widgets and put it into a struct, then pass this struct onto
	the game instance which will then create match from it */
	void GatherMatchInfo();

	/* Returns number of slots that have human players in them. Should always be at least 1
	because host
	@return - number of human players in lobby, or -1 if cannot determine */
	int32 GetNumHumanPlayers() const;

	/* Whether slots can be locked */
	bool CanLockSlots() const;

	/** 
	 *	Returns whether match can start 
	 *
	 *	@param bTryShowWaring - if true then a warning will try to be displayed on screen, subject 
	 *	to function ShouldShowNoStartWarning()
	 *	@return - true if the match can start
	 */
	bool CanStartMatch(bool bTryShowWarning);

	/* Return true if a warning widget should be shown on screen */
	bool ShouldShowNoStartWarning() const;

	/* Return true if a sound should be played when trying to start match but cannot */
	bool ShouldPlayNoStartSound() const;

	/* Timer handle for controlling how often popup widget can show for not being able to 
	start match */
	FTimerHandle TimerHandle_CannotStartMatchWidget;

	// Same as above but for sound
	FTimerHandle TimerHandle_CannotStartMatchSound;

	/* Get the number human and CPU players in the lobby. Includes observers */
	int32 GetNumPlayers() const;

	/* True if all players in lobby are not on the same team. Observers do not count towards
	any team */
	bool HaveDifferentTeams() const;

	UFUNCTION()
	void UIBinding_OnReturnButtonClicked();

	/* If true then players cannot change anything such as their team, faction etc. Also
	currently slots need to be locked in order for host to start match. A player leaving the
	lobby will automatically unlock slots */
	bool bAreSlotsLocked;

	/* Used to know whether to start match after async start session completes */
	int32 NumPlayersWhenStartMatchButtonPressed;

	UFUNCTION()
	void UIBinding_OnAddSlotButtonClicked();

	UFUNCTION()
	void UIBinding_OnLockSlotsButtonClicked();

	UFUNCTION()
	void UIBinding_OnChangeMapButtonClicked();

	void DoNothing();

	/* Call function that returns void after delay
	@param TimerHandle - timer handle to use
	@param Function - function to call
	@param Delay - delay before calling function */
	void Delay(FTimerHandle & TimerHandle, void(ULobbyWidget:: *Function)(), float Delay);

public:

	/* Called by game mode when a player leaves the lobby */
	void NotifyPlayerLeft(ARTSPlayerController * Exiting);

	/* Leave lobby. Callable as host or client */
	virtual void ExitLobby();

	/* If true the host has locked any changes from being made. No new players can join, and
	players can only leave or send chat messages */
	bool AreSlotsLocked() const;

	/* Return true if LAN or online lobby */
	bool IsMultiplayerLobby() const;

	/* Called when chat message is received in lobby 
	@param SendersName - name of player who sent the message 
	@param Message - what the player typed */
	void OnChatMessageReceived(const FString & SendersName, const FString & Message);

	/* Lets another widget update what map info is displayed */
	virtual void UpdateMapDisplay(const FMapInfo & MapInfo) override;

	/* Functions to be called from game state to query and change appearance of widget */

	EMatchType GetLobbyType() const;
	void SetLobbyType(EMatchType InLobbyType);
	void SetLobbyName(const FText & NewName);
	void SetStartingResources(EStartingResourceAmount NewAmount);
	void SetDefeatCondition(EDefeatCondition NewCondition);
	void SetMap(const FMapInfo & MapInfo);
	void SetMap(const FString & MapName);
	void SetAreSlotsLocked(bool bInNewValue);
	void UpdatePlayerStartAssignments(const TArray < int16 > & PlayerStartArray);
	const TArray <ULobbySlot *> & GetSlots() const;
	ULobbySlot * GetSlot(uint8 SlotIndex) const;

	/* Function to call when the state of the lobby changes. Change visibility of various widgets
	like making button unclickable for example or making a new slot appear */
	void UpdateVisibilities();

	/* Clear chat input and output */
	void ClearChat();
};


/**
 *	Loading screen when going from lobby to match
 */
UCLASS(Abstract)
class RTS_VER2_API ULoadingScreen : public UMainMenuWidgetBase
{
	GENERATED_BODY()

protected:

	/* Text that shows the status of loading */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_Status;

public:

	/* Set the status text */
	void SetStatusText(const FText & NewText);
};
