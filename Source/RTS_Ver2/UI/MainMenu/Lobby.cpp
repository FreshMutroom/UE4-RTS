// Fill out your copyright notice in the Description page of Project Settings.

#include "Lobby.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/Image.h"
#include "Components/EditableText.h"
#include "Online.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Spacer.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/CanvasPanelSlot.h"

#include "UI/MainMenu/Menus.h"
#include "Statics/Statics.h"
#include "GameFramework/RTSGameInstance.h"
#include "GameFramework/FactionInfo.h"
#include "Networking/RTSGameSession.h"
#include "GameFramework/RTSGameState.h"
#include "GameFramework/RTSPlayerController.h"
#include "GameFramework/RTSPlayerState.h"
#include "Statics/DevelopmentStatics.h"
#include "UI/MainMenu/LobbyCreationWidget.h"
#include "Settings/RTSGameUserSettings.h"
#include "UI/MainMenu/MatchRulesWidget.h"
#include "UI/MainMenu/MapSelection.h"
#include "UI/UIUtilities.h"


//==============================================================================================
//	Single Chat Message Widget
//==============================================================================================

UChatOutputSingleMessageWidget::UChatOutputSingleMessageWidget(const FObjectInitializer & ObjectInitializer)
	: Super(ObjectInitializer)
{
	ReceivedAnimName = "ReceivedAnim";
}

bool UChatOutputSingleMessageWidget::Setup()
{
	if (Super::Setup())
	{
		return true;
	}

	// Cache original color set in editor
	if (IsWidgetBound(Text_Message))
	{
		OriginalTextColor = Text_Message->ColorAndOpacity;
		// Set this manually in editor - protected variables cannot be accessed here
		//Text_Message->AutoWrapText = true;
		//Text_Message->WrappingPolicy = ETextWrappingPolicy::AllowPerCharacterWrapping;
	}

	/* TODO logic can be moved to design time if can find reliable post edit func. Or when 
	BindAnimation meta support drops then just use that instead */
	// Assign widget anim
	ReceivedAnimation = UIUtilities::FindWidgetAnim(this, *FString(ReceivedAnimName + "_INST"));

	return false;
}

void UChatOutputSingleMessageWidget::SetMessage(const FText & InMessage, bool bOverrideTextColor, 
	const FLinearColor & TextColor)
{
	if (IsWidgetBound(Text_Message))
	{
		Text_Message->SetText(InMessage);

		if (bOverrideTextColor)
		{
			Text_Message->SetColorAndOpacity(TextColor);
		}
		else
		{
			Text_Message->SetColorAndOpacity(OriginalTextColor);
		}
	}
}

void UChatOutputSingleMessageWidget::PlayReceivedAnim()
{
	if (ReceivedAnimation != nullptr)
	{
		PlayAnimation(ReceivedAnimation);
	}
}

void UChatOutputSingleMessageWidget::OnAnimationFinished_Implementation(const UWidgetAnimation * Animation)
{
}


//==============================================================================================
//	Lobby Chat Widget
//==============================================================================================

ULobbyChat::ULobbyChat(const FObjectInitializer & ObjectInitializer)
	: Super(ObjectInitializer)
{
	MaxMessageLength = 200;
	InputMessageStart = "[To Lobby]: ";
	OutputMessageStart = "[";
	OutputMessageMiddle = "]: ";
}

bool ULobbyChat::Setup()
{
	if (Super::Setup())
	{
		return true;
	}

	if (IsWidgetBound(Text_Input))
	{
		Text_Input->SetText(FText::FromString(InputMessageStart));
		
		Text_Input->RevertTextOnEscape = true;
		Text_Input->OnTextChanged.AddDynamic(this, &ULobbyChat::UIBinding_OnChatInputTextChanged);
		Text_Input->OnTextCommitted.AddDynamic(this, &ULobbyChat::UIBinding_OnChatInputTextCommitted);
	}

	if (IsWidgetBound(Panel_Output))
	{
		/* Put a spacer in panel. Set its slot to vertical fill so messages are forced to the 
		bottom of the panel */
		CreateAndAddOutputSpacer();
	}

	return false;
}

void ULobbyChat::UIBinding_OnChatInputTextChanged(const FText & NewText)
{
	/* Limit length of text making sure not to remove the persistent start part */
	FString ToShow = InputMessageStart;
	const FString & AsString = NewText.ToString();
	const int32 MidStartPoint = AsString.Len() < InputMessageStart.Len() ? AsString.Len() : InputMessageStart.Len();
	ToShow += AsString.Mid(MidStartPoint, MaxMessageLength);
	Text_Input->SetText(FText::FromString(ToShow));
}

void ULobbyChat::UIBinding_OnChatInputTextCommitted(const FText & Text, ETextCommit::Type CommitMethod)
{
	if (CommitMethod == ETextCommit::OnEnter)
	{
		const FString & WhatWasTyped = Text.ToString().RightChop(InputMessageStart.Len());
		if (WhatWasTyped.Len() > 0)
		{
			/* Send message */
			ARTSPlayerController * PlayCon = CastChecked<ARTSPlayerController>(GetWorld()->GetFirstPlayerController());
			PlayCon->Server_SendLobbyChatMessage(WhatWasTyped);
		}

		/* Keep focus on chat input to make sending multiple messages easier */
		Text_Input->SetKeyboardFocus();
	}
	else if (CommitMethod == ETextCommit::OnCleared)
	{
		/* Clear message */
		Text_Input->SetText(FText::FromString(InputMessageStart));
	}
	else
	{
		/* Leave message as it was */
	}
}

void ULobbyChat::CreateAndAddOutputSpacer()
{
	// Assumed only calling this when panel has 0 children
	assert(Panel_Output->GetChildrenCount() == 0);
		
	USpacer * OutputSpacer = WidgetTree->ConstructWidget<USpacer>();
	assert(OutputSpacer != nullptr);

	UPanelSlot * PanelSlot = Panel_Output->AddChild(OutputSpacer);

	if (PanelSlot->IsA(UVerticalBoxSlot::StaticClass()))
	{
		UVerticalBoxSlot * VBoxSlot = CastChecked<UVerticalBoxSlot>(PanelSlot);
		VBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	else // Assumed UScrollBoxSlot
	{
		/* Actually scroll box slots don't allow fill, so don't need to do anything. But messages 
		will not appear at bottom so scroll box isn't really an option */
	}
}

void ULobbyChat::RemoveMessagesToFit()
{	
	/* Currently doing nothing but returning, so will have unlimited messages in chat log. 
	A user defined value can be used but won't make chat log display correctly always since 
	messages can take up multiple lines at times.
	Once find a way to get widget height then can remove return */
	return;
	
	const int32 NumChildren = Panel_Output->GetChildrenCount();
	
	float Total_Y = 0.f;
	int32 ChildIndex = NumChildren - 1;

	/* Assuming here that Panel_Output is on a canvas panel. Can be moved to design time */
	UCanvasPanelSlot * PanelsSlot = CastChecked<UCanvasPanelSlot>(Panel_Output->Slot);
	const float MaxHeight = PanelsSlot->GetSize().Y;

	while (Total_Y <= MaxHeight && ChildIndex >= 1)
	{
		Total_Y += 0.f/*The message widget's height*/;
		assert(0);//Line above needs implementing

		if (Total_Y > MaxHeight)
		{
			break;
		}
		else
		{
			ChildIndex--;
		}
	}
	
	// ChildIndex now says how many widgets to remove, so if it's 0 then don't need to remove any
	const int32 NumToRemove = ChildIndex;

	/* Make a copy of the Slots array but with the widgets not the slots */
	TArray < UWidget * > SlotsCopy;
	SlotsCopy.Reserve(NumChildren);
	for (int32 i = 0; i < NumChildren; ++i)
	{
		SlotsCopy.Emplace(Panel_Output->GetChildAt(i));
	}

	// Remove slots at end of array
	for (int32 i = NumChildren - 1; i >= NumChildren - NumToRemove; --i)
	{
		Panel_Output->RemoveChildAt(i);

		NumChatLogMessages--;
	}

	int32 Count = 0;

	// Fill remaining slots with correct widget
	for (int32 i = NumChildren - NumToRemove - 1; i >= 1; --i)
	{
		UWidget * AboutToBeReplaced = Panel_Output->GetChildAt(i);
		
		if (i == 1)
		{
			// Save the last widget to be recycled
			LastRemovedMessageWidget = CastChecked<UChatOutputSingleMessageWidget>(AboutToBeReplaced);
		}
		
		UWidget * TheReplacer = SlotsCopy.Last(Count++);

		Panel_Output->ReplaceChildAt(i, TheReplacer);
	}
}

void ULobbyChat::OnChildAddedToOutputPanel(UChatOutputSingleMessageWidget * WidgetJustAdded, UPanelSlot * PanelSlot)
{
}

bool ULobbyChat::ShouldPlayMessageReceivedSound()
{
	const float CurrentTime = GetWorld()->GetRealTimeSeconds();
	
	// Minimum 2 seconds between message sounds
	const bool bPlaySoundTimeAtLastMessageReceived = (CurrentTime - TimeAtLastMessageReceived > 2.f);

	TimeAtLastMessageReceived = CurrentTime;

	return bPlaySoundTimeAtLastMessageReceived;
}

void ULobbyChat::SetupFor(EMatchType InLobbyType)
{
	if (InLobbyType == EMatchType::Offline)
	{
		SetVisibility(ESlateVisibility::Hidden);
	}
	else
	{
		if (IsWidgetBound(Text_Input))
		{
			Text_Input->SetVisibility(ESlateVisibility::Visible);
		}

		// In case hidden from singleplayer lobby
		SetVisibility(ESlateVisibility::Visible);
	}
}

void ULobbyChat::OnChatMessageReceived(const FString & SendersName, const FString & Message)
{
	if (IsWidgetBound(Panel_Output))
	{
		// Add new message

		NumChatLogMessages++;
			
		// Check if can recycle a previously constructed widget
		UChatOutputSingleMessageWidget * MessageWidget = LastRemovedMessageWidget.Get();

		if (MessageWidget == nullptr)
		{
			// Must create widget

			UE_CLOG(OutputMessage_BP == nullptr, RTSLOG, Fatal, TEXT("Trying to create widget for "
				"chat log message but OutputMessage_BP is null"));

			MessageWidget = CreateWidget<UChatOutputSingleMessageWidget>(GetWorld()->GetFirstPlayerController(), 
				OutputMessage_BP);

			MessageWidget->Setup();
		}

		MessageWidget->SetMessage(FText::FromString(OutputMessageStart + SendersName + OutputMessageMiddle + Message));

		// Add single message widget to panel widget
		UPanelSlot * PanelSlot = Panel_Output->AddChild(MessageWidget);

		OnChildAddedToOutputPanel(MessageWidget, PanelSlot);

		/* Remove messages until they all fit in panel widget */
		RemoveMessagesToFit();

		MessageWidget->PlayReceivedAnim();
	}

	// Possibly play a sound
	if (ShouldPlayMessageReceivedSound())
	{
		if (ReceivedMessageSound != nullptr)
		{
			Statics::PlaySound2D(this, ReceivedMessageSound);
		}
	}
}

void ULobbyChat::ClearChat()
{
	if (IsWidgetBound(Text_Input))
	{
		Text_Input->SetText(FText::FromString(InputMessageStart));
	}
	if (IsWidgetBound(Panel_Output))
	{
		/* Remove every child except the spacer */
		
		/* UPanelWidget::ClearChildren iterates forwards... was expecting backwards. I will try
		backwards. Either way do not remove the 1st child which is the spacer */
		const int32 ChildCount = Panel_Output->GetChildrenCount();
		for (int32 i = ChildCount - 1; i >= 1; --i)
		{
			Panel_Output->RemoveChildAt(i);
		}
	}

	NumChatLogMessages = 0;
}


//==============================================================================================
//	Lobby Slot Widget
//==============================================================================================

ULobbySlot::ULobbySlot(const FObjectInitializer & ObjectInitializer) : Super(ObjectInitializer)
{
	SlotIndex = -1;
}

void ULobbySlot::PreSetup(ULobbyWidget * InOwningLobby, int32 InSlotIndex)
{
	Lobby = InOwningLobby;
	SlotIndex = InSlotIndex;
}

bool ULobbySlot::Setup()
{
	if (Super::Setup())
	{
		return true;
	}

	if (IsWidgetBound(Button_AddCPUPlayer))
	{
		Button_AddCPUPlayer->OnClicked.AddDynamic(this, &ULobbySlot::UIBinding_OnAddCPUPlayerButtonClicked);
	}
	if (IsWidgetBound(Button_Kick))
	{
		Button_Kick->OnClicked.AddDynamic(this, &ULobbySlot::UIBinding_OnKickButtonClicked);
	}
	if (IsWidgetBound(Button_Close))
	{
		Button_Close->OnClicked.AddDynamic(this, &ULobbySlot::UIBinding_OnCloseButtonClicked);
	}
	if (IsWidgetBound(ComboBox_CPUDifficulty))
	{
		/* Add options in order from worst difficulty to best (given enum is ordered that way) */
		TArray < ECPUDifficulty > Temp;
		for (const auto Elem : GI->GetCPUDifficultyInfo())
		{
			Temp.Emplace(Elem.Key);
		}
		Temp.Sort();
		for (int32 i = 0; i < Temp.Num(); ++i)
		{
			ComboBox_CPUDifficulty->AddOption(GI->GetCPUDifficulty(Temp[i]));
		}

		ComboBox_CPUDifficulty->OnSelectionChanged.AddDynamic(this, &ULobbySlot::UIBinding_OnCPUDifficultySelectionChanged);
	}
	if (IsWidgetBound(ComboBox_Faction))
	{
		/* Set faction combo box options */
		for (const auto & Elem : GI->GetAllFactionInfo())
		{
			ComboBox_Faction->AddOption(Elem->GetName().ToString());
		}

		ComboBox_Faction->OnSelectionChanged.AddDynamic(this, &ULobbySlot::UIBinding_OnFactionSelectionChanged);
	}
	if (IsWidgetBound(ComboBox_Team))
	{
		/* Set team combo box options */
		for (uint8 i = 0; i < ProjectSettings::MAX_NUM_TEAMS; ++i)
		{
			ComboBox_Team->AddOption(FString::FromInt(i + 1));
		}

		/* Add the extra observer option */
		ComboBox_Team->AddOption(Statics::LOBBY_OBSERVER_COMBO_BOX_OPTION);

		ComboBox_Team->OnSelectionChanged.AddDynamic(this, &ULobbySlot::UIBinding_OnTeamSelectionChanged);
	}

	Status = ELobbySlotStatus::Closed;

	return false;
}

void ULobbySlot::UIBinding_OnAddCPUPlayerButtonClicked()
{
	UE_CLOG(!GetWorld()->IsServer(), RTSLOG, Fatal, TEXT("Add CPU player button pressed on client - "
		"should not be clickable on clients"));
	
	UE_CLOG(HasHumanPlayer(), RTSLOG, Fatal, TEXT("Add CPU player button pressed with human player "
		"in slot - should not be clickable"));

	ARTSGameState * GameState = CastChecked<ARTSGameState>(GetWorld()->GetGameState());
	GameState->PopulateLobbySlot(LobbyOptions::DEFAULT_CPU_DIFFICULTY, SlotIndex);
}

void ULobbySlot::UIBinding_OnCPUDifficultySelectionChanged(FString Item, ESelectInfo::Type SelectionType)
{
	UE_CLOG(!IsLocalPlayerHost(), RTSLOG, Fatal, TEXT("Button should not be clickable on clients"));
	
	UE_CLOG(HasHumanPlayer(), RTSLOG, Fatal, TEXT("Trying to change CPU difficulty of slot with "
		"human player. Button should not be clickable in the first place"));
	
	ARTSGameState * GameState = GetWorld()->GetGameState<ARTSGameState>();
	GameState->ChangeCPUDifficultyInLobby(SlotIndex, Statics::StringToCPUDifficultySlow(Item, GI));
}

void ULobbySlot::UIBinding_OnKickButtonClicked()
{
#if !UE_BUILD_SHIPPING
	if (!IsLocalPlayerHost())
	{
		UE_LOG(RTSLOG, Fatal, TEXT("Kick button pressed on client - should not be clickable on "
			"clients"));
		return;
	}
#endif

	/* Do not allow kicking ourselves */
	if (PlayerState == GetWorld()->GetFirstPlayerController()->PlayerState)
	{
		return;
	}

	ARTSGameState * GameState = GetWorld()->GetGameState<ARTSGameState>();

	GameState->KickPlayerInLobby(SlotIndex, PlayerState);
}

bool ULobbySlot::IsLocalPlayerHost() const
{
	return GetWorld()->IsServer();
}

void ULobbySlot::UIBinding_OnCloseButtonClicked()
{
	UE_CLOG(!IsLocalPlayerHost(), RTSLOG, Fatal, TEXT("Close slot button pressed on client - should "
		"not be clickable on clients"));
		
	/* Do not allow closing slot with human player. Button should have been made unclickable if
	human player is in slot */
	assert(PlayerState == nullptr);

	ARTSGameState * GameState = GetWorld()->GetGameState<ARTSGameState>();
	GameState->CloseLobbySlot(SlotIndex);
}

void ULobbySlot::UIBinding_OnFactionSelectionChanged(FString Item, ESelectInfo::Type SelectionType)
{
	/* As long as this is changed like the func name implies and not just selected then we will
	save no bandwidth by checking if selection actually changed first */

	/* Change param from FString to EFaction. This is a relatively slow way of doing it,
	TMap populated when GI initializes would be faster */
	for (const auto & Elem : GI->GetAllFactionInfo())
	{
		if (Elem->GetName().ToString() == Item)
		{
			const EFaction NewFaction = Elem->GetFaction();

			if (HasHumanPlayer())
			{
				/* Get reference to player controller to call RPC */
				ARTSPlayerController * PlayCon = CastChecked<ARTSPlayerController>(GetWorld()->GetFirstPlayerController());
				PlayCon->Server_ChangeFactionInLobby(NewFaction);

				/* Remember faction we picked by saving it in game user settings. This is the faction
				we will be automatically set as when joining lobbies in future */
				URTSGameUserSettings * Settings = CastChecked<URTSGameUserSettings>(UGameUserSettings::GetGameUserSettings());
				Settings->ChangeDefaultFaction(NewFaction);

				/* Save default faction to permanent storage. Note: this will save all settings
				such as video, controls etc, not just the faction */
				Settings->ApplyAllSettings(GI, PlayCon);
			}
			else if (HasCPUPlayer())
			{
				assert(IsLocalPlayerHost());

				ARTSGameState * const GameState = GetWorld()->GetGameState<ARTSGameState>();
				GameState->ChangeFactionInLobby(SlotIndex, NewFaction);
			}
			else
			{
				/* Nothing in slot. No point in replicating change. Faction will be overridden when new
				player joins */
			}

			return;
		}
	}

	UE_LOG(RTSLOG, Fatal, TEXT("No faction matched name of \"%s\" "), *Item);
}

void ULobbySlot::UIBinding_OnTeamSelectionChanged(FString Item, ESelectInfo::Type SelectionType)
{
	const ETeam NewTeam = Statics::StringToTeamSlow(Item);

	if (HasHumanPlayer())
	{
		// Only allow changing our own team
		if (PlayerState == GetWorld()->GetFirstPlayerController()->PlayerState)
		{
			ARTSPlayerController * const PlayCon = CastChecked<ARTSPlayerController>(GetWorld()->GetFirstPlayerController());
			PlayCon->Server_ChangeTeamInLobby(NewTeam);
		}
	}
	else if (HasCPUPlayer())
	{
		if (IsLocalPlayerHost())
		{
			ARTSGameState * const GameState = GetWorld()->GetGameState<ARTSGameState>();
			GameState->ChangeTeamInLobby(SlotIndex, NewTeam);
		}
		else
		{
			UE_LOG(RTSLOG, Fatal, TEXT("Button clicked on client while CPU player in slot"));
		}
	}
	else
	{
		/* Nothing in slot. No point in replicating change. Team will be overridden when new
		player joins */
	}
}

void ULobbySlot::Close()
{
	SetSlotVisibility(ESlateVisibility::Hidden);
}

void ULobbySlot::SetSlotVisibility(ESlateVisibility NewVis)
{
	SetWidgetVisibility(Text_PlayerName, NewVis);
	SetWidgetVisibility(Image_PlayerIcon, NewVis);
	SetWidgetVisibility(ComboBox_Faction, NewVis);
	SetWidgetVisibility(Image_Faction, NewVis);
	SetWidgetVisibility(ComboBox_Team, NewVis);
	SetWidgetVisibility(ComboBox_CPUDifficulty, NewVis);
	SetWidgetVisibility(Button_AddCPUPlayer, NewVis);
	SetWidgetVisibility(Button_Kick, NewVis);
	SetWidgetVisibility(Button_Close, NewVis);
}

void ULobbySlot::SetWidgetVisibility(UWidget * Widget, ESlateVisibility NewVisibility)
{
	if (IsWidgetBound(Widget))
	{
		Widget->SetVisibility(NewVisibility);
	}
}

int32 ULobbySlot::GetSlotIndex() const
{
	return SlotIndex;
}

bool ULobbySlot::HasHumanPlayer() const
{
	/* The || is tagged on because rep order may make them come through out of order. Tbh Human
	status isn't really needed, just need PlayerState */
	const bool bReturn = (Status == ELobbySlotStatus::Human || Statics::IsValid(PlayerState));

	return bReturn;
}

bool ULobbySlot::HasCPUPlayer() const
{
	return Status == ELobbySlotStatus::CPU;
}

ARTSPlayerState * ULobbySlot::GetPlayerState() const
{
	return PlayerState;
}

void ULobbySlot::SetPlayerState(ARTSPlayerState * InPlayerState, bool bUpdateStatusToo)
{
	/* What to show when we know slot has a human player but their player state has not repped
	yet. Usually shown very briefly if at all */
	static const FString UnknownString = FString("<Unknown>");

	PlayerState = InPlayerState;

	if (IsWidgetBound(Text_PlayerName))
	{
		FText TextToShow;

		if (Statics::IsValid(InPlayerState))
		{
			TextToShow = FText::FromString(InPlayerState->GetPlayerName());
		}
		else if (Status == ELobbySlotStatus::Human)
		{
			/* Basically apparently there is a human player in this slot but their player state
			hasn't repped through so until then we just show unknown */
			TextToShow = FText::FromString(UnknownString);
		}
		else if (Status == ELobbySlotStatus::CPU)
		{
			TextToShow = LobbyOptions::CPU_PLAYER_SLOT_NAME;
		}
		else if (Status == ELobbySlotStatus::Open)
		{
			TextToShow = LobbyOptions::OPEN_SLOT_NAME;
		}
		else if (Status == ELobbySlotStatus::Closed)
		{
			TextToShow = LobbyOptions::CLOSED_SLOT_NAME;
		}

		Text_PlayerName->SetText(TextToShow);
	}

	if (IsWidgetBound(Image_PlayerIcon))
	{
		if (HasHumanPlayer())
		{
			// TOCONSIDER set human player image from somewhere
			//Image_PlayerIcon->SetBrushFromTexture(ThePlayersImageFromSomewhere);
			Image_PlayerIcon->SetVisibility(ESlateVisibility::Visible);
		}
		else if (HasCPUPlayer())
		{
			// TOCONSIDER set CPU icon
			// Maybe CPU difficulty hasn't repped through yet though, maybe just forget it
			//Image_PlayerIcon->SetBrushFromTexture(ImageFromSomewhere);
			Image_PlayerIcon->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			Image_PlayerIcon->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	if (bUpdateStatusToo)
	{
		if (Statics::IsValid(InPlayerState))
		{
			SetStatus(ELobbySlotStatus::Human, false);
		}
	}
}

ELobbySlotStatus ULobbySlot::GetStatus() const
{
	return Status;
}

void ULobbySlot::SetStatus(ELobbySlotStatus NewPlayerType, bool bUpdatePlayerStateToo)
{
	Status = NewPlayerType;

	if (NewPlayerType == ELobbySlotStatus::Closed)
	{
		SetVisibility(ESlateVisibility::Hidden);
	}
	else
	{
		SetVisibility(ESlateVisibility::Visible);

		if (IsWidgetBound(ComboBox_Team))
		{
			/* Because clients shouldn't be able to interact with the team combo box only server
			needs to do this */
			if (IsLocalPlayerHost())
			{
				/* Need to make sure the Observer option in team combo box cannot be picked if CPU
				player */
				if (NewPlayerType == ELobbySlotStatus::CPU)
				{
					ComboBox_Team->RemoveOption(Statics::LOBBY_OBSERVER_COMBO_BOX_OPTION);
				}
				else
				{
					/* Check if last entry is the observer entry, if it isn't then add it */
					if (ComboBox_Team->GetOptionAtIndex(ComboBox_Team->GetOptionCount() - 1) != Statics::LOBBY_OBSERVER_COMBO_BOX_OPTION)
					{
						ComboBox_Team->AddOption(Statics::LOBBY_OBSERVER_COMBO_BOX_OPTION);
					}
				}
			}
		}
	}

	if (bUpdatePlayerStateToo)
	{
		// To update player name text box
		SetPlayerState(PlayerState, false);
	}
}

ECPUDifficulty ULobbySlot::GetCPUDifficulty() const
{	
	return Statics::StringToCPUDifficultySlow(ComboBox_CPUDifficulty->GetSelectedOption(), GI);
}

void ULobbySlot::SetCPUDifficulty(ECPUDifficulty NewDifficulty)
{
	if (IsWidgetBound(ComboBox_CPUDifficulty))
	{
		const FString & AsString = GI->GetCPUDifficulty(NewDifficulty);

		ComboBox_CPUDifficulty->SetSelectedOption(AsString);
	}
}

ETeam ULobbySlot::GetTeam() const
{
	if (IsWidgetBound(ComboBox_Team))
	{
		return Statics::StringToTeamSlow(ComboBox_Team->GetSelectedOption());
	}
	else
	{
		/* No widget to go by. It's probably better to use the game state's array value 
		but will just return default value for now */
		return LobbyOptions::DEFAULT_TEAM;
	}
}

void ULobbySlot::SetTeam(ETeam NewTeam)
{
	if (IsWidgetBound(ComboBox_Team))
	{
		ComboBox_Team->SetSelectedOption(Statics::TeamToStringSlow(NewTeam));
	}
}

EFaction ULobbySlot::GetFaction() const
{
	if (IsWidgetBound(ComboBox_Faction))
	{
		/* Relatively slow way of getting this */
		for (const auto & Elem : GI->GetAllFactionInfo())
		{
			if (Elem->GetName().ToString() == ComboBox_Faction->GetSelectedOption())
			{
				return Elem->GetFaction();
			}
		}

		UE_LOG(RTSLOG, Fatal, TEXT("Faction combo box does not hold a string corrisponding to "
			"any valid faction info, string was %s"), *ComboBox_Faction->GetSelectedOption());
	}

	UE_LOG(RTSLOG, Fatal, TEXT("ComboBox_Faction not bound, cannot correctly determine faction"));

	return EFaction::None;
}

void ULobbySlot::SetFaction(EFaction NewFaction)
{
	UE_CLOG(NewFaction == EFaction::None, RTSLOG, Fatal, TEXT("Trying to set \"None\" faction"));

	const AFactionInfo * FactionInfo = GI->GetFactionInfo(NewFaction);

	if (IsWidgetBound(ComboBox_Faction))
	{
		// Silent fail if option not found
		ComboBox_Faction->SetSelectedOption(FactionInfo->GetName().ToString());
	}

	if (IsWidgetBound(Image_Faction))
	{
		UTexture2D * Image = FactionInfo->GetFactionIcon();
		Image_Faction->SetBrushFromTexture(Image);

		/* If  the image was null then hide widget to avoid showing white image */
		if (Image == nullptr)
		{
			Image_Faction->SetVisibility(ESlateVisibility::Hidden);
		}
		else
		{
			Image_Faction->SetVisibility(ESlateVisibility::Visible);
		}
	}
}

void ULobbySlot::UpdateVisibilities()
{
	assert(Status != ELobbySlotStatus::JustInitialized);

	if (IsWidgetBound(Image_PlayerIcon))
	{
		if (HasHumanPlayer() || HasCPUPlayer())
		{
			Image_PlayerIcon->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			Image_PlayerIcon->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	if (IsWidgetBound(Image_Faction))
	{
		if (HasHumanPlayer() || HasCPUPlayer())
		{
			Image_Faction->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			Image_Faction->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	if (IsWidgetBound(ComboBox_Faction))
	{
		if (Status == ELobbySlotStatus::Open || Status == ELobbySlotStatus::Closed)
		{
			ComboBox_Faction->SetVisibility(ESlateVisibility::Hidden);
		}
		else
		{
			if (PlayerState == GetWorld()->GetFirstPlayerController()->PlayerState)
			{
				// Let players change their own faction
				ComboBox_Faction->SetVisibility(ESlateVisibility::Visible);
			}
			else
			{
				if (IsLocalPlayerHost())
				{
					if (HasCPUPlayer())
					{
						ComboBox_Faction->SetVisibility(ESlateVisibility::Visible);
					}
					else
					{
						ComboBox_Faction->SetVisibility(ESlateVisibility::Hidden);
					}
				}
				else
				{
					if (HasCPUPlayer())
					{
						ComboBox_Faction->SetVisibility(ESlateVisibility::HitTestInvisible);
					}
					else
					{
						ComboBox_Faction->SetVisibility(ESlateVisibility::Hidden);
					}
				}
			}
		}
	}

	if (IsWidgetBound(ComboBox_Team))
	{
		if (Status == ELobbySlotStatus::Open || Status == ELobbySlotStatus::Closed)
		{
			ComboBox_Team->SetVisibility(ESlateVisibility::Hidden);
		}
		else
		{
			if (PlayerState == GetWorld()->GetFirstPlayerController()->PlayerState)
			{
				// Let players change their own team
				ComboBox_Team->SetVisibility(ESlateVisibility::Visible);
			}
			else
			{
				if (IsLocalPlayerHost())
				{
					if (HasCPUPlayer())
					{
						ComboBox_Team->SetVisibility(ESlateVisibility::Visible);
					}
					else
					{
						ComboBox_Team->SetVisibility(ESlateVisibility::Hidden);
					}
				}
				else
				{
					if (HasCPUPlayer())
					{
						ComboBox_Team->SetVisibility(ESlateVisibility::HitTestInvisible);
					}
					else
					{
						ComboBox_Team->SetVisibility(ESlateVisibility::Hidden);
					}
				}
			}
		}
	}

	if (IsWidgetBound(Button_AddCPUPlayer))
	{
		if (IsLocalPlayerHost())
		{
			if (HasHumanPlayer() || HasCPUPlayer() || Status == ELobbySlotStatus::Closed)
			{
				Button_AddCPUPlayer->SetVisibility(ESlateVisibility::Hidden);
			}
			else if (Status == ELobbySlotStatus::Open)
			{
				Button_AddCPUPlayer->SetVisibility(ESlateVisibility::Visible);
			}
			else // Unrecognized enum value
			{
				assert(0);
			}
		}
		else
		{
			Button_AddCPUPlayer->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	if (IsWidgetBound(ComboBox_CPUDifficulty))
	{
		if (IsLocalPlayerHost())
		{
			if (HasCPUPlayer())
			{
				ComboBox_CPUDifficulty->SetVisibility(ESlateVisibility::Visible);
			}
			else
			{
				ComboBox_CPUDifficulty->SetVisibility(ESlateVisibility::Hidden);
			}
		}
		else
		{
			if (HasCPUPlayer())
			{
				ComboBox_CPUDifficulty->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
			else
			{
				ComboBox_CPUDifficulty->SetVisibility(ESlateVisibility::Hidden);
			}
		}
	}

	if (IsWidgetBound(Button_Kick))
	{
		if (IsLocalPlayerHost())
		{
			if (Status == ELobbySlotStatus::Human)
			{
				if (PlayerState == GetWorld()->GetFirstPlayerController()->PlayerState)
				{
					// Do not allow kicking ourselves
					Button_Kick->SetVisibility(ESlateVisibility::Hidden);
				}
				else
				{
					Button_Kick->SetVisibility(ESlateVisibility::Visible);
				}
			}
			else if (Status == ELobbySlotStatus::CPU)
			{
				Button_Kick->SetVisibility(ESlateVisibility::Visible);
			}
			else if (Status == ELobbySlotStatus::Open || Status == ELobbySlotStatus::Closed)
			{
				Button_Kick->SetVisibility(ESlateVisibility::Hidden);
			}
		}
		else
		{
			Button_Kick->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	if (IsWidgetBound(Button_Close))
	{
		if (IsLocalPlayerHost())
		{
			if (Status == ELobbySlotStatus::Human || Status == ELobbySlotStatus::CPU
				|| Status == ELobbySlotStatus::Closed)
			{
				/* While a human/CPU player is in this slot don't allow this button to be used.
				Instead host can use the clear button to kick player from slot */
				Button_Close->SetVisibility(ESlateVisibility::Hidden);
			}
			else if (Status == ELobbySlotStatus::Open)
			{
				Button_Close->SetVisibility(ESlateVisibility::Visible);
			}
			else // Unexpected enum value
			{
				assert(0);
			}
		}
		else
		{
			Button_Close->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	// General TODO when player joins cancel combo box selecting if not done automatically
}

bool ULobbySlot::IsMultiplayerLobby() const
{
	return Lobby->IsMultiplayerLobby();
}


//==============================================================================================
//	Lobby Widget
//==============================================================================================

ULobbyWidget::ULobbyWidget(const FObjectInitializer & ObjectInitializer)
	: Super(ObjectInitializer)
{
	PlayerStartDisplayRule = EPlayerStartDisplay::VisibleAndClickable;

	CannotStartWarnings.Reserve(static_cast<int32>(ECannotStartMatchReason::z_ALWAYS_LAST_IN_ENUM) - 1);

	CannotStartWarnings.Emplace(ECannotStartMatchReason::EveryonesOnOneTeam, 
		FTextAndSound(NSLOCTEXT("Lobby Warnings", "Everyone On Same Team", "Need 2+ different teams in order to start match"), nullptr));

	CannotStartWarnings.Emplace(ECannotStartMatchReason::NotEnoughPlayers,
		FTextAndSound(NSLOCTEXT("Lobby Warnings", "Not Enough Players", "Not enough players"), nullptr));

	CannotStartWarnings.Emplace(ECannotStartMatchReason::SlotsNotLocked,
		FTextAndSound(NSLOCTEXT("Lobby Warnings", "Slots Not Locked", "Press the \"Lock Slots\" button first"), nullptr));
}

bool ULobbyWidget::Setup()
{
	if (Super::Setup())
	{
		return true;
	}

	if (IsWidgetBound(Button_StartMatch))
	{
		Button_StartMatch->OnClicked.AddDynamic(this, &ULobbyWidget::UIBinding_OnStartMatchButtonClicked);
	}
	if (IsWidgetBound(Button_Return))
	{
		Button_Return->OnClicked.AddDynamic(this, &ULobbyWidget::UIBinding_OnReturnButtonClicked);
	}
	if (IsWidgetBound(Button_AddSlot))
	{
		Button_AddSlot->OnClicked.AddDynamic(this, &ULobbyWidget::UIBinding_OnAddSlotButtonClicked);
	}
	if (IsWidgetBound(Button_LockSlots))
	{
		Button_LockSlots->OnClicked.AddDynamic(this, &ULobbyWidget::UIBinding_OnLockSlotsButtonClicked);
	}
	if (IsWidgetBound(Button_ChangeMap))
	{
		Button_ChangeMap->OnClicked.AddDynamic(this, &ULobbyWidget::UIBinding_OnChangeMapButtonClicked);
	}
	if (IsWidgetBound(Widget_Chat))
	{
		Widget_Chat->Setup();
	}

	SetupSlots();

	if (IsWidgetBound(Widget_MatchRules))
	{
		Widget_MatchRules->SetIsForLobby(true);
		Widget_MatchRules->Setup();
	}
	if (IsWidgetBound(Widget_MapInfo))
	{
		Widget_MapInfo->Setup();
	}
	if (IsWidgetBound(Image_LockSlots))
	{
		Image_LockSlots->SetVisibility(ESlateVisibility::Hidden);
	}

	return false;
}

void ULobbyWidget::SetupSlots()
{
	const uint8 NumSlotWidgetsRequired = ProjectSettings::MAX_NUM_PLAYERS;
	
	if (bAutoPopulateSlotsPanel)
	{
		if (IsWidgetBound(Panel_LobbySlots))
		{
			if (LobbySlot_BP != nullptr)
			{
				for (uint8 i = 0; i < NumSlotWidgetsRequired; ++i)
				{
					// Construct a new slot widget and add it to Panel_LobbySlots
					ULobbySlot * NewSlot = WidgetTree->ConstructWidget<ULobbySlot>(LobbySlot_BP);
					NewSlot->PreSetup(this, i);
					NewSlot->Setup();
					Slots.Emplace(NewSlot);
					Panel_LobbySlots->AddChild(NewSlot);

					// Allow any user defined adjusts to slot widget to take place
					OnLobbySlotConstructed(Panel_LobbySlots, NewSlot);
				}
			}
			else
			{
				UE_LOG(RTSLOG, Fatal, TEXT("Lobby Slot Widget blueprint is set to null"));
			}
		}
		else
		{
			/* Panel widget wasn't bound, kind of expected it to be */

			UE_LOG(RTSLOG, Fatal, TEXT("Lobby widget is set to auto create slot widgets but "
				"there is no widget bound to Panel_LobbySlots. Either add a panel widget named "
				"Panel_LobbySlots or set AutoPopulateSlotsPanel to false and add your own slot widgets"));
		}
	}
	else
	{
		// First gather all ULobbySlot widgets

		TArray < UWidget * > AllChildren;
		WidgetTree->GetAllWidgets(AllChildren);

		TArray < ULobbySlot * > SlotWidgets;

		for (const auto & Elem : AllChildren)
		{
			if (Elem->IsA(ULobbySlot::StaticClass()))
			{
				// HeapPush will be better if operator< was overridden
				SlotWidgets.Emplace(CastChecked<ULobbySlot>(Elem));
			}
		}

		// Setup lobby slot widgets and add them to array
		int32 NumSetUp = 0;
		for (int32 i = 0; i < SlotWidgets.Num(); ++i)
		{
			ULobbySlot * Elem = SlotWidgets[i];
			const FString SlotName = "Widget_Slot" + FString::FromInt(i + 1);
			if (Elem->GetName() == SlotName)
			{
				Elem->PreSetup(this, i);
				Elem->Setup();
				Slots.Emplace(Elem);
				NumSetUp++;
				continue;
			}
		}

		UE_CLOG(NumSetUp < SlotWidgets.Num(), RTSLOG, Fatal, TEXT("Lobby widget %s has enough lobby "
		"slot widgets but %d were not named correctly. They need to be named like so: Widget_Slot1, "
			"Widget_Slot2, etc"), *GetClass()->GetName(), SlotWidgets.Num() - NumSetUp);
		
		/* This assert may be too strong if the lobby has a max player limit lower than this
		static const */
		UE_CLOG(SlotWidgets.Num() < NumSlotWidgetsRequired, RTSLOG, Fatal, TEXT("Lobby widget %s "
			"does not have enough lobby slot widgets to support the max number of players defined "
			"for project. Requires: %d, has: %d"), 
			*GetClass()->GetName(), NumSlotWidgetsRequired, SlotWidgets.Num());

		UE_CLOG(SlotWidgets.Num() > NumSlotWidgetsRequired, RTSLOG, Warning, TEXT("Lobby widget %s "
			"has more slot widgets than are required. Num slot widgets: %d, num required: %d"),
			*GetClass()->GetName(), SlotWidgets.Num(), NumSlotWidgetsRequired);
	}
}

void ULobbyWidget::OnLobbySlotConstructed(UPanelWidget * OwningPanel, ULobbySlot * RecentlyAddedSlot)
{
}

bool ULobbyWidget::IsHost() const
{
	return GetWorld()->IsServer();
}

void ULobbyWidget::UIBinding_OnStartMatchButtonClicked()
{
	if (!IsHost())
	{
		UE_LOG(RTSLOG, Warning, TEXT("Button pressed on client but should not be clickable on client"));
		return;
	}

	// CanStartMatch will handle showing warning popups if cannot start match
	if (CanStartMatch(true))
	{
		NumPlayersWhenStartMatchButtonPressed = GetNumPlayers();

		// Make the whole widget unclickable while this goes through
		SetVisibility(ESlateVisibility::HitTestInvisible);

		// OnStartSessionComplete will be called when delegate fires
		GI->TryCreateMatch(LobbyType);
	}
}

void ULobbyWidget::OnStartSessionComplete(bool bWasSuccessful)
{
	ARTSGameState * GameState = CastChecked<ARTSGameState>(GetWorld()->GetGameState());

	if (bWasSuccessful)
	{
		/* Unlike if bWasSuccessful is false we do not make the widget clickable again because
		match loading has polling and probably async operations. The issue from this is that if
		another lobby is created it may keep the hit test invisible vis, but I think a new widget
		is created for every lobby because lobby is on its own map so hopefully this shouldn't be
		an issue */

		if (IsMultiplayerLobby())
		{
			if (NumPlayersWhenStartMatchButtonPressed != GetNumPlayers() || !CanStartMatch(true))
			{
				/* This is a situation where while the async operation was completing a player left.
				We've already started the session meaning players cannot join... I don't really
				know if it's possible to return a session to 'unstarted', so just destroy lobby */

				ExitLobby();
				return;
			}
		}

		// Match will be made
		GatherMatchInfo();
	}
	else
	{
		// Unlock slots, let players leave again
		GameState->ChangeLockedSlotsStatusInLobby(false);

		// Make widget clickable again 
		SetVisibility(ESlateVisibility::Visible);
	}
}

void ULobbyWidget::GatherMatchInfo()
{
	/* Gather all the info game instance needs to create match into a struct */

	FMatchInfo & MatchInfo = GI->GetMatchInfoModifiable();

	/* Add whether offline, LAN or online match */
	MatchInfo.SetMatchType(LobbyType);

	/* Add which map to play on */
	const FMapInfo * MapInfo = Widget_MapInfo->GetSetMapInfo();
	MatchInfo.SetMap(MapInfo->GetMapFName(), MapInfo->GetUniqueID(), MapInfo->GetDisplayName());

	/* Add the rules: starting resources and defeat condition for match */
	if (IsWidgetBound(Widget_MatchRules))
	{
		MatchInfo.SetStartingResources(Widget_MatchRules->GetStartingResources());

		MatchInfo.SetDefeatCondition(Widget_MatchRules->GetDefeatCondition());
	}
	else // No widget to go off - will use default values
	{
		/* Starting resource amounts will be set to whatever I hardcoded somewhere. You probably
		don't want this for your shipping build */
		MatchInfo.SetStartingResources(EStartingResourceAmount::Default);

		// Use condition where player cannot be defeated
		MatchInfo.SetDefeatCondition(EDefeatCondition::NoCondition);

		UE_LOG(RTSLOG, Warning, TEXT("Lobby widget needs a Match Rules Widget called "
			"Widget_MatchRules for starting resources and defeat condition to be specified. "
			"Starting resources set to default values and defeat condition set to NoCondition"));
	}

	/* Move team numbers to lowest values possible e.g. if players have team numbers 1, 3
	and 4 then they will be changed to team numbers 1, 2 and 3 respectively. Game state relies
	on this assumption */
	TArray < ETeam > UniqueTeams;
	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		ULobbySlot * const LobbySlot = Slots[i];
		if ((LobbySlot->HasHumanPlayer() || LobbySlot->HasCPUPlayer())
			&& LobbySlot->GetTeam() != ETeam::Observer)
		{
			UniqueTeams.AddUnique(LobbySlot->GetTeam());
		}
	}

	assert(UniqueTeams.Num() >= 2);

	UniqueTeams.Sort();

	/* Map for mapping old team value to new team value in next for loop */
	TMap < ETeam, ETeam > NewTeams;
	NewTeams.Reserve(UniqueTeams.Num() + 1);
	// This is just so observers don't cause crash later
	NewTeams.Emplace(ETeam::Observer, ETeam::Observer);

	for (uint8 i = 0; i < UniqueTeams.Num(); ++i)
	{
		if (UniqueTeams[i] != ETeam::Observer)
		{
			NewTeams.Emplace(UniqueTeams[i], Statics::ArrayIndexToTeam(i));
		}
	}

	MatchInfo.SetNumTeams(UniqueTeams.Num());

	// Get the starting spots for each player
	TArray <int16> StartingSpots;
	if (IsWidgetBound(Widget_MapInfo))
	{
		Widget_MapInfo->GetStartingSpots(StartingSpots);
	}
	else
	{
		// No map widget. Treat everyone as if they have "unassigned" start spots
		StartingSpots.Init(-1, Slots.Num());
	}

	ARTSGameState * GameState = GetWorld()->GetGameState<ARTSGameState>();

	/* Some players may not have a starting spot assigned. For those players give them the 
	best starting spot possible */
	TArray < FMinimalPlayerInfo > MinimalPlayerInfo;
	MinimalPlayerInfo.Reserve(Slots.Num());
	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		ULobbySlot * LobbySlot = Slots[i];
		
#if !UE_BUILD_SHIPPING
		// Check game state state and lobby state were the same in regards to player starts
		
		// Init as -1 to signal 'no player start assigned'
		int16 Spot = -1;
		// Nice if there is a faster way of doing this than iterating all player starts. There might be already, look into it
		for (int32 j = 0; j < StartingSpots.Num(); ++j)
		{
			if (StartingSpots[j] == i)
			{
				Spot = j;
				break;
			}
		}

		UE_CLOG(Spot != GameState->GetLobbyPlayerStart(i), RTSLOG, Fatal, TEXT("Player start mismatch "
			"in lobby widget: for player [%d]: lobby player start = [%d], game state player start = [%d]"),
			i, Spot, GameState->GetLobbyPlayerStart(i));
#endif

		if (LobbySlot->HasHumanPlayer() || LobbySlot->HasCPUPlayer())
		{
			MinimalPlayerInfo.Emplace(FMinimalPlayerInfo(LobbySlot->HasHumanPlayer(),
				NewTeams[LobbySlot->GetTeam()], LobbySlot->GetFaction(), StartingSpots[i]));
		}
	}
	GI->AssignOptimalStartingSpots(*Map, MinimalPlayerInfo);

	/* Add information about each player */
	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		ULobbySlot * LobbySlot = Slots[i];
		if (LobbySlot->HasHumanPlayer())
		{
			ARTSPlayerState * PlayerState = LobbySlot->GetPlayerState();
			const ETeam Team = NewTeams[LobbySlot->GetTeam()];
			const EFaction Faction = LobbySlot->GetFaction();
			const int16 StartingSpotIndex = MinimalPlayerInfo[i].GetStartingSpot();

			MatchInfo.AddPlayer(FPlayerInfo(PlayerState, Team, Faction, StartingSpotIndex));
		}
		else if (LobbySlot->HasCPUPlayer())
		{
			const ECPUDifficulty CPUDifficulty = LobbySlot->GetCPUDifficulty();
			const ETeam Team = NewTeams[LobbySlot->GetTeam()];
			const EFaction Faction = LobbySlot->GetFaction();
			const int16 StartingSpotIndex = MinimalPlayerInfo[i].GetStartingSpot();

			MatchInfo.AddPlayer(FPlayerInfo(CPUDifficulty, Team, Faction, StartingSpotIndex));
		}
	}

#if !UE_BUILD_SHIPPING

	/* Ensure info shown on widgets reflected the correct lobby state */
	const FString & Result = GameState->IsMatchInfoCorrect(MatchInfo, NewTeams);
	if (Result != FString())
	{
		UE_LOG(RTSLOG, Fatal, TEXT("Lobby state does not match game state, reason : %s"), *Result);
	}

#endif

	GI->LoadMatch();
}

int32 ULobbyWidget::GetNumHumanPlayers() const
{
	assert(Slots.Num() > 0);

	int32 NumHumanPlayers = 0;

	for (const auto & Elem : Slots)
	{
		if (Elem->HasHumanPlayer())
		{
			NumHumanPlayers++;
		}
	}

	assert(NumHumanPlayers > 0);

	return NumHumanPlayers;
}

bool ULobbyWidget::CanLockSlots() const
{
	return GetNumPlayers() >= 2 && HaveDifferentTeams();
}

bool ULobbyWidget::CanStartMatch(bool bTryShowWarning)
{
	ECannotStartMatchReason Reason;
	
	if (IsMultiplayerLobby())
	{
		if (GetNumPlayers() >= 2)
		{
			if (HaveDifferentTeams())
			{
				if (GetNumHumanPlayers() == 1)
				{
					Reason = ECannotStartMatchReason::NoReason;
				}
				else
				{
					if (AreSlotsLocked())
					{
						Reason = ECannotStartMatchReason::NoReason;
					}
					else
					{
						Reason = ECannotStartMatchReason::SlotsNotLocked;
					}
				}
			}
			else
			{
				Reason = ECannotStartMatchReason::EveryonesOnOneTeam;
			}
		}
		else
		{
			Reason = ECannotStartMatchReason::NotEnoughPlayers;
		}
	}
	else // Singleplayer lobby
	{
		if (GetNumPlayers() >= 2)
		{
			if (HaveDifferentTeams())
			{
				Reason = ECannotStartMatchReason::NoReason;
			}
			else
			{
				Reason = ECannotStartMatchReason::EveryonesOnOneTeam;
			}
		}
		else
		{
			Reason = ECannotStartMatchReason::NotEnoughPlayers;
		}
	}

	if (Reason != ECannotStartMatchReason::NoReason)
	{
		if (bTryShowWarning)
		{
			const FTextAndSound & WarningInfo = CannotStartWarnings[Reason];
			
			if (ShouldShowNoStartWarning())
			{
				// Show popup widget
				GI->ShowPopupWidget(WarningInfo.GetMessage() /* + some default params */);

				// Put on "cooldown" for half a second
				Delay(TimerHandle_CannotStartMatchWidget, &ULobbyWidget::DoNothing, 0.5f);
			}

			if (WarningInfo.GetSound() != nullptr && ShouldPlayNoStartSound())
			{
				// Play some sound
				Statics::PlaySound2D(this, WarningInfo.GetSound());

				// Put on "cooldown" for half a second
				Delay(TimerHandle_CannotStartMatchSound, &ULobbyWidget::DoNothing, 0.5f);
			}
		}

		return false;
	}
	else
	{
		return true;
	}
}

bool ULobbyWidget::ShouldShowNoStartWarning() const
{
	return GetWorld()->GetTimerManager().IsTimerActive(TimerHandle_CannotStartMatchWidget) == false;
}

bool ULobbyWidget::ShouldPlayNoStartSound() const
{
	return GetWorld()->GetTimerManager().IsTimerActive(TimerHandle_CannotStartMatchSound) == false;
}

int32 ULobbyWidget::GetNumPlayers() const
{
	int32 NumPlayers = 0;

	for (const auto & Elem : Slots)
	{
		if (Elem->HasHumanPlayer() || Elem->HasCPUPlayer())
		{
			NumPlayers++;
		}
	}

	return NumPlayers;
}

bool ULobbyWidget::HaveDifferentTeams() const
{
	TSet < ETeam > Teams;
	Teams.Reserve(2);
	for (const auto & Elem : Slots)
	{	
		if (( Elem->HasHumanPlayer() || Elem->HasCPUPlayer() ) && Elem->GetTeam() != ETeam::Observer)
		{
			Teams.Emplace(Elem->GetTeam());
			if (Teams.Num() >= 2)
			{
				return true;
			}
		}
	}

	return false;
}

void ULobbyWidget::UIBinding_OnReturnButtonClicked()
{
	if (IsHost())
	{
		/* Behaviour here is to exit lobby if offline lobby.
		If a LAN or online lobby then check if there are other players in lobby. If there are
		then show a confirmation widget first */
		if (LobbyType == EMatchType::Offline)
		{
			ExitLobby();
		}
		else if (IsMultiplayerLobby())
		{
			if (GetNumHumanPlayers() > 1)
			{
				GI->ShowWidget(EWidgetType::ConfirmExitFromLobby, ESlateVisibility::HitTestInvisible);
			}
			else
			{
				ExitLobby();
			}
		}
		else
		{
			UE_LOG(RTSLOG, Fatal, TEXT("Unexpected network type"));
		}
	}
	else
	{
		ExitLobby();
	}
}

void ULobbyWidget::UIBinding_OnAddSlotButtonClicked()
{
	if (IsHost())
	{
		if (!AreSlotsLocked())
		{
			ARTSGameState * GameState = CastChecked<ARTSGameState>(GetWorld()->GetGameState());
			GameState->TryOpenLobbySlot();
		}
	}
	else
	{
		UE_LOG(RTSLOG, Fatal, TEXT("Add slot button clicked on client but should not be clickable "
			"on client"));
	}
}

void ULobbyWidget::UIBinding_OnLockSlotsButtonClicked()
{
	if (!IsHost())
	{
		UE_LOG(RTSLOG, Fatal, TEXT("Lock slots button pressed on client - should not be clickable "
			"on clients"));
		return;
	}

	// Always allow to unlock slots but check extra condition to lock them
	if (AreSlotsLocked() || (!AreSlotsLocked() && CanLockSlots()))
	{
		// Will eventually change lock slot image
		ARTSGameState * GameState = CastChecked<ARTSGameState>(GetWorld()->GetGameState());
		GameState->ChangeLockedSlotsStatusInLobby(!bAreSlotsLocked);
	}
}

void ULobbyWidget::UIBinding_OnChangeMapButtonClicked()
{
	if (!IsHost())
	{
		UE_LOG(RTSLOG, Warning, TEXT("Change map button pressed by client - should not be "
			"clickable on clients"));
		return;
	}

	if (!AreSlotsLocked())
	{
		/* Setup map selection widget to make sure it updates our lobby map when we exit it */
		UMainMenuWidgetBase * Widget = GI->GetWidget(EWidgetType::MapSelectionScreen);
		UMapSelectionWidget * MapSelectionWidget = CastChecked<UMapSelectionWidget>(Widget);
		MapSelectionWidget->SetRefToUpdatedWidget(this);
		MapSelectionWidget->SetupMapList();

		if (IsWidgetBound(Widget_MapInfo))
		{
			MapSelectionWidget->SetCurrentMap(&GI->GetMapInfo(Widget_MapInfo->GetSetMapID()));
		}

		GI->ShowWidget(EWidgetType::MapSelectionScreen, ESlateVisibility::Collapsed);
	}
}

void ULobbyWidget::DoNothing()
{
}

void ULobbyWidget::Delay(FTimerHandle & TimerHandle, void(ULobbyWidget::* Function)(), float Delay)
{
	assert(Delay > 0.f);
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, Function, Delay, false);
}

void ULobbyWidget::NotifyPlayerLeft(ARTSPlayerController * Exiting)
{
	assert(GetWorld()->IsServer());

	APlayerState * ExitingPlayerState = Exiting != nullptr ? Exiting->PlayerState : nullptr;

	ARTSGameState * GameState = CastChecked<ARTSGameState>(GetWorld()->GetGameState());

	/* Remove player from lobby and update appearance. Even if they are null or their player state
	is still update lobby */
	for (const auto & Elem : Slots)
	{
		if (Elem->GetPlayerState() == ExitingPlayerState && !Elem->HasCPUPlayer())
		{
			GameState->RemoveFromLobby(Elem->GetSlotIndex(), ELobbySlotStatus::Open);
		}
	}

	/* Unlock slots whether they were locked before or not */
	GameState->ChangeLockedSlotsStatusInLobby(false);
}

void ULobbyWidget::ExitLobby()
{
	if (IsHost())
	{
		/* Destroy session if one exists */
		IOnlineSubsystem * OnlineSub = IOnlineSubsystem::Get();
		if (OnlineSub != nullptr)
		{
			IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
			if (Sessions.IsValid())
			{
				/* For singleplayer lobbies this is needed. DestroySession does not fire the
				delegate if no session has been created */
				if (LobbyType == EMatchType::Offline)
				{
					GI->GoFromLobbyToMainMenu();
				}
				else
				{
					/* Disable input while this goes through since DestroySession is async */
					SetVisibility(ESlateVisibility::HitTestInvisible);

					ARTSGameSession * GameSesh = CastChecked<ARTSGameSession>(GetWorld()->GetAuthGameMode()->GameSession);
					GameSesh->DestroySession(EActionToTakeAfterDestroySession::HostToMainMenuClientsToLobbyBrowsing);

					/* ARTSGameSession::OnDestroySessionComplete callback will decide what to do
					next */
				}
			}
			else
			{
				UE_LOG(RTSLOG, Warning, TEXT("ULobbyWidget::ExitLobby(): session ptr null"));
			}
		}
		else
		{
			UE_LOG(RTSLOG, Fatal, TEXT("Online sussystem was null"));

			GI->GoFromLobbyToMainMenu();
		}
	}
	else
	{
		/* This basically opens a new map. Is that enough to leave a session as a client? Maybe
		the stuff done as host above also needs to be done for clients */

		GI->GoFromLobbyToMainMenu();
	}
}

bool ULobbyWidget::AreSlotsLocked() const
{
	return bAreSlotsLocked;
}

bool ULobbyWidget::IsMultiplayerLobby() const
{
	return LobbyType == EMatchType::LAN || LobbyType == EMatchType::SteamOnline;
}

void ULobbyWidget::OnChatMessageReceived(const FString & SendersName, const FString & Message)
{
	if (IsWidgetBound(Widget_Chat))
	{
		Widget_Chat->OnChatMessageReceived(SendersName, Message);
	}
}

void ULobbyWidget::UpdateMapDisplay(const FMapInfo & MapInfo)
{
	SetMap(MapInfo);
}

EMatchType ULobbyWidget::GetLobbyType() const
{
	return LobbyType;
}

void ULobbyWidget::SetLobbyType(EMatchType InLobbyType)
{
	LobbyType = InLobbyType;

	/* Set chat widget visibility based on lobby type */
	if (IsWidgetBound(Widget_Chat))
	{
		Widget_Chat->SetupFor(InLobbyType);
	}

	/* Set lock slots button visibility based on lobby type */
	if (IsWidgetBound(Button_LockSlots))
	{
		const ESlateVisibility NewVis = InLobbyType == EMatchType::Offline
			? ESlateVisibility::Hidden : ESlateVisibility::Visible;

		Button_LockSlots->SetVisibility(NewVis);
	}
}

void ULobbyWidget::SetLobbyName(const FText & NewName)
{
	if (IsWidgetBound(Text_LobbyName))
	{
		Text_LobbyName->SetText(NewName);
	}
}

void ULobbyWidget::SetStartingResources(EStartingResourceAmount NewAmount)
{
	if (IsWidgetBound(Widget_MatchRules))
	{
		// Maybe don't need to do this on server
		Widget_MatchRules->SetStartingResources(NewAmount);
	}
}

void ULobbyWidget::SetDefeatCondition(EDefeatCondition NewCondition)
{
	if (IsWidgetBound(Widget_MatchRules))
	{
		// Maybe don't need to do this on server
		Widget_MatchRules->SetDefeatCondition(NewCondition);
	}
}

void ULobbyWidget::SetMap(const FMapInfo & MapInfo)
{
	Map = &MapInfo;
	
	if (IsWidgetBound(Widget_MapInfo))
	{
		Widget_MapInfo->SetMap(&MapInfo, PlayerStartDisplayRule);
	}
}

void ULobbyWidget::SetMap(const FString & MapName)
{
	Map = &GI->GetMapInfo(*MapName);
	
	if (IsWidgetBound(Widget_MapInfo))
	{
		Widget_MapInfo->SetMap(Map, PlayerStartDisplayRule);
	}
}

void ULobbyWidget::SetAreSlotsLocked(bool bInNewValue)
{
	bAreSlotsLocked = bInNewValue;

	/* Change visibility of indicator */
	const ESlateVisibility NewLockSlotImageVis = bAreSlotsLocked
		? ESlateVisibility::Visible : ESlateVisibility::Hidden;
	if (IsWidgetBound(Image_LockSlots))
	{
		Image_LockSlots->SetVisibility(NewLockSlotImageVis);
	}

	/* Make most buttons/widgets interactable/uninteractable */

	const ESlateVisibility NewVis = bAreSlotsLocked
		? ESlateVisibility::HitTestInvisible : ESlateVisibility::Visible;

	if (IsWidgetBound(Button_AddSlot))
	{
		Button_AddSlot->SetVisibility(NewVis);
	}
	if (IsWidgetBound(Button_ChangeMap))
	{
		Button_ChangeMap->SetVisibility(NewVis);
	}
	if (bAutoPopulateSlotsPanel)
	{
		if (IsWidgetBound(Panel_LobbySlots))
		{
			Panel_LobbySlots->SetVisibility(NewVis);
		}
	}
	else
	{
		for (const auto & Elem : Slots)
		{
			Elem->SetVisibility(NewVis);
		}
	}
	if (IsWidgetBound(Widget_MatchRules))
	{
		Widget_MatchRules->SetVisibility(NewVis);
	}
	if (IsWidgetBound(Widget_MapInfo))
	{
		Widget_MapInfo->SetVisibility(NewVis);
	}
}

void ULobbyWidget::UpdatePlayerStartAssignments(const TArray<int16>& PlayerStartArray)
{
	if (IsWidgetBound(Widget_MapInfo))
	{
		Widget_MapInfo->UpdatePlayerStartAssignments(PlayerStartArray);
	}
}

const TArray<ULobbySlot*>& ULobbyWidget::GetSlots() const
{
	return Slots;
}

ULobbySlot * ULobbyWidget::GetSlot(uint8 SlotIndex) const
{
	// Crash here may indicate not enough slot widgets in lobby widget
	return Slots[SlotIndex];
}

void ULobbyWidget::UpdateVisibilities()
{
	for (const auto & Elem : Slots)
	{
		Elem->UpdateVisibilities();
	}
}

void ULobbyWidget::ClearChat()
{
	if (IsWidgetBound(Widget_Chat))
	{
		Widget_Chat->ClearChat();
	}
}

void ULoadingScreen::SetStatusText(const FText & NewText)
{
	if (IsWidgetBound(Text_Status))
	{
		Text_Status->SetText(NewText);
	}
}

// TODO limit number of messages that can be in chat maybe


