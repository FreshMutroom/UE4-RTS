// Fill out your copyright notice in the Description page of Project Settings.

#include "PlayerTargetingPanel.h"
#include "Blueprint/WidgetTree.h"

#include "UI/InMatchWidgets/Components/MyButton.h"
#include "GameFramework/RTSPlayerState.h"
#include "GameFramework/RTSGameInstance.h"
#include "Statics/DevelopmentStatics.h"
#include "UI/UIUtilities.h"
#include "GameFramework/RTSGameState.h"
#include "Statics/Statics.h"
#include "GameFramework/RTSPlayerController.h"


FButtonArrayEntry::FButtonArrayEntry(URTSGameInstance * GameInst, UMyButton * InButton, ARTSPlayerState * InAssignedPlayer)
{
	Button = InButton;
	AssignedPlayer = InAssignedPlayer;
	ButtonOriginalOpacity = Button->GetRenderOpacity();
	bHasAssignedPlayerBeenDefeated = false;
	AssignedPlayersAffiliation = AssignedPlayer->GetAffiliation();

	//--------------------------------------------------------
	//	Button images and sounds
	//--------------------------------------------------------

	const FUnifiedImageAndSoundFlags UnifiedAssetFlags = GameInst->GetUnifiedButtonAssetFlags_ActionBar();
	Button->SetImages_PlayerTargetingPanel(UnifiedAssetFlags,
		AssignedPlayer->GetPlayerNormalImage(),
		AssignedPlayer->GetPlayerHoveredImage(),
		AssignedPlayer->GetPlayerPressedImage());

	/* Don't have any player hovered sounds so use unified sounds */
	Button->SetHoveredSound(GameInst->GetUnifiedHoverSound_ActionBar().GetSound());
	Button->SetPressedByLMBSound(GameInst->GetUnifiedLMBPressedSound_ActionBar().GetSound());
	Button->SetPressedByRMBSound(GameInst->GetUnifiedRMBPressedSound_ActionBar().GetSound());
}

void FButtonArrayEntry::UpdateAppearance(const FCommanderAbilityInfo & AbilityInfo)
{
	/* I swear there is a faster way of doing this like just assign an offset based 
	off affiliation during setup as a variable 
	then do 
	if (!bHasAssignedPlayerBeenDefeated)
	{ 
		bIsTargetable = AbilityInfo[Offset];
	} 
	*/

	bool bIsTargetable = false;
	
	if (!bHasAssignedPlayerBeenDefeated)
	{
		if (AssignedPlayersAffiliation == EAffiliation::Hostile)
		{
			if (AbilityInfo.CanTargetEnemies())
			{
				bIsTargetable = true;
			}
		}
		else if (AssignedPlayersAffiliation == EAffiliation::Allied)
		{
			if (AbilityInfo.CanTargetFriendlies())
			{
				bIsTargetable = true;
			}
		}
		else // Assumed Owned
		{
			assert(AssignedPlayersAffiliation == EAffiliation::Owned);

			if (AbilityInfo.CanTargetSelf())
			{
				bIsTargetable = true;
			}
		}
	}
	
	if (bIsTargetable)
	{
		SetButtonAppearanceForTargetable();
	}
	else
	{
		SetButtonAppearanceForUntargetable();
	}
}

void FButtonArrayEntry::OnPlayerDefeated(ARTSPlayerState * DefeatedPlayer)
{
	if (DefeatedPlayer == AssignedPlayer)
	{
		AssignedPlayer = nullptr;
		bHasAssignedPlayerBeenDefeated = true;
	}
}

void FButtonArrayEntry::SetButtonAppearanceForTargetable()
{
	Button->SetRenderOpacity(ButtonOriginalOpacity);
}

void FButtonArrayEntry::SetButtonAppearanceForUntargetable()
{
	Button->SetRenderOpacity(ButtonOriginalOpacity * 0.4f);
}


const FName UPlayerTargetingPanel::ShowAnimFName = FName("ShowAnimation_INST");
const FName UPlayerTargetingPanel::HideAnimFName = FName("HideAnimation_INST");

bool UPlayerTargetingPanel::SetupWidget(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController)
{
	if (Super::SetupWidget(InGameInstance, InPlayerController))
	{
		return true;
	}

	ShowAnim = UIUtilities::FindWidgetAnim(this, ShowAnimFName);
	HideAnim = UIUtilities::FindWidgetAnim(this, HideAnimFName);

	return false;
}

void UPlayerTargetingPanel::MoreSetup()
{
	TArray <ARTSPlayerState *> AllPlayerStates = GS->GetPlayerStatesByValue();
	
	/* The array may already be in order I don't know I can't remember it probably isn't. 
	Anyway we need it in the order of us first, then our teammates then enemies so do 
	that now */
	AllPlayerStates.Sort([](const ARTSPlayerState & PlayerState_1, const ARTSPlayerState & PlayerState_2)
	{
		return PlayerState_1.GetAffiliation() < PlayerState_2.GetAffiliation();
	});

	/* For ordering to be correct it is assumed that in the for loop the first button 
	iterated on is the button you want to display yourself, then teammates then finally enemies */
	
	TArray <UWidget *> AllChildren;
	WidgetTree->GetAllWidgets(AllChildren);
	int32 PlayerStatesArrayIndex = 0;
	for (int32 i = 0; i < AllChildren.Num(); ++i)
	{
		UWidget * Widget = AllChildren[i];
	
		if (Widget != Button_Cancel)
		{
			/* Here assuming that all buttons on the panel are for player targeting (excluding
			the cancel button) */
			UMyButton * Button = Cast<UMyButton>(Widget);
			if (Button != nullptr)
			{
				/* Check if there are enough players in the match that the button is needed */
				if (Buttons.Num() < AllPlayerStates.Num())
				{
					SetupPlayerTargetingButton(Button);

					Buttons.Emplace(FButtonArrayEntry(GI, Button, AllPlayerStates[PlayerStatesArrayIndex++]));
				}
				else
				{
					/* Destroy the button since it is not needed this match because not enough players */
					Button->RemoveFromParent();
				}
			}
		}
	}

	UE_CLOG(Buttons.Num() < AllPlayerStates.Num(), RTSLOG, Fatal, TEXT("Not enough buttons on "
		"the player targeting panel for all players in match. [%d] players in match but only [%d] "
		"buttons on the panel"), AllPlayerStates.Num(),  Buttons.Num());

	if (IsWidgetBound(Button_Cancel))
	{
		Button_Cancel->SetPC(PC);
		Button_Cancel->SetOwningWidget();
		Button_Cancel->SetPurpose(EUIElementType::NoTooltipRequired, nullptr);
		
		Button_Cancel->GetOnLeftMouseButtonPressDelegate().BindUObject(this, &UPlayerTargetingPanel::UIBinding_OnCancelButtonLMBPressed);
		Button_Cancel->GetOnLeftMouseButtonReleasedDelegate().BindUObject(this, &UPlayerTargetingPanel::UIBinding_OnCancelButtonLMBReleased);
		Button_Cancel->GetOnRightMouseButtonPressDelegate().BindUObject(this, &UPlayerTargetingPanel::UIBinding_OnCancelButtonRMBPressed);
		Button_Cancel->GetOnRightMouseButtonReleasedDelegate().BindUObject(this, &UPlayerTargetingPanel::UIBinding_OnCancelButtonRMBReleased);

		/* No images or sounds here. You are expected to set them in editor... although I 
		don't think the sounds have been exposed to editor TODO */
	}
}

void UPlayerTargetingPanel::SetupPlayerTargetingButton(UMyButton * Button)
{
	Button->SetPC(PC);
	Button->SetOwningWidget();
	Button->SetPurpose(EUIElementType::NoTooltipRequired, nullptr);

	Button->GetOnLeftMouseButtonPressDelegate().BindUObject(Button, &UMyButton::PlayerTargetingButton_OnLMBPressed);
	Button->GetOnLeftMouseButtonReleasedDelegate().BindUObject(Button, &UMyButton::PlayerTargetingButton_OnLMBReleased);
	Button->GetOnRightMouseButtonPressDelegate().BindUObject(Button, &UMyButton::PlayerTargetingButton_OnRMBPressed);
	Button->GetOnRightMouseButtonReleasedDelegate().BindUObject(Button, &UMyButton::PlayerTargetingButton_OnRMBReleased);
}

void UPlayerTargetingPanel::NativeTick(const FGeometry & MyGeometry, float InDeltaTime)
{
	// If this ensure is hit it is likely UpdateCanTick as not called somewhere
	if (ensureMsgf(GetDesiredTickFrequency() != EWidgetTickFrequency::Never, TEXT("SObjectWidget and UUserWidget have mismatching tick states or UUserWidget::NativeTick was called manually (Never do this)")))
	{
		GInitRunaway();

		TickActionsAndAnimation(MyGeometry, InDeltaTime);
	}
}

bool UPlayerTargetingPanel::IsShowingOrPlayingShowAnimation() const
{
	return bShownOrPlayingShowAnim;
}

void UPlayerTargetingPanel::OnRequestToBeShown(const FCommanderAbilityInfo * AbilityInfo)
{	
	assert(!bShownOrPlayingShowAnim);
	
	AssignedAbilityInfo = AbilityInfo;
	
	SetupTargetingButtonsForAbility(AbilityInfo);

	/* Quick note: UWidgetAnimation::GetEndTime() will look at where the red line at the end is,
	so make sure to move this red line to the point where you consider the animation done */

	bShownOrPlayingShowAnim = true;

	/* Play animation if set. Otherwise just show straight away */
	if (ShowAnim != nullptr)
	{
		float StartTime;
		if (HideAnim != nullptr)
		{
			if (IsAnimationPlaying(HideAnim))
			{
				StartTime = (1.f - (GetAnimationCurrentTime(HideAnim) / HideAnim->GetEndTime())) * ShowAnim->GetEndTime();

				StopAnimation(HideAnim);
			}
			else
			{
				StartTime = 0.f;
			}
		}
		else
		{
			StartTime = 0.f;
		}

		PlayAnimation(ShowAnim, StartTime);
	}
	else
	{
		StopAnimation(HideAnim);

		SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
}

void UPlayerTargetingPanel::OnRequestToBeHidden()
{
	assert(bShownOrPlayingShowAnim);
	
	bShownOrPlayingShowAnim = false;

	/* Play animation if set. Otherwise just hide straight away */
	if (HideAnim != nullptr)
	{
		float StartTime;
		if (ShowAnim != nullptr)
		{
			if (IsAnimationPlaying(ShowAnim))
			{
				// Inverse of other anim e.g. if show anim is 60% through we'll start at 40%
				StartTime = (1.f - (GetAnimationCurrentTime(ShowAnim) / ShowAnim->GetEndTime())) * HideAnim->GetEndTime();

				StopAnimation(ShowAnim);
			}
			else
			{
				StartTime = 0.f;
			}
		}
		else
		{
			StartTime = 0.f;
		}

		PlayAnimation(HideAnim, StartTime);
	}
	else
	{
		StopAnimation(ShowAnim);

		SetVisibility(ESlateVisibility::Hidden);
	}
}

bool UPlayerTargetingPanel::RespondToEscapeRequest(URTSHUD * HUD)
{
	assert(IsShowingOrPlayingShowAnimation());
	OnRequestToBeHidden();
	return true;
}

void UPlayerTargetingPanel::OnAnotherPlayerDefeated(ARTSPlayerState * DefeatedPlayer)
{
	/* Written with this assumption. If this is hit then we may need to pass some ID earlier 
	on to identify the PS now */
	assert(Statics::IsValid(DefeatedPlayer));
	
	for (auto & Elem : Buttons)
	{
		Elem.OnPlayerDefeated(DefeatedPlayer);
	}
}

void UPlayerTargetingPanel::OnPlayerTargetingButtonEvent_LMBPressed(UMyButton * Button, ARTSPlayerController * LocalPlayCon)
{
	LocalPlayCon->OnLMBPressed_PlayerTargetingPanelButton(Button);
}

void UPlayerTargetingPanel::OnPlayerTargetingButtonEvent_LMBReleased(UMyButton * Button, ARTSPlayerController * LocalPlayCon)
{
	/* There are faster ways of doing this, but because it's expected that the number of 
	players in the match will be low this way is fine. 
	A good alternative is add an AuxilleryData uint8 to UMyButton, set that in setup, then 
	use that to get the array index */
	ARTSPlayerState * ButtonsAssignedPlayer = nullptr;
	for (int32 i = 0; i < Buttons.Num(); ++i)
	{
		if (Buttons[i].GetButton() == Button)
		{
			ButtonsAssignedPlayer = Buttons[i].GetAssignedPlayer();
			break;
		}
	}
	
	LocalPlayCon->OnLMBReleased_PlayerTargetingPanelButton(Button, ButtonsAssignedPlayer);
}

void UPlayerTargetingPanel::OnPlayerTargetingButtonEvent_RMBPressed(UMyButton * Button, ARTSPlayerController * LocalPlayCon)
{
	LocalPlayCon->OnRMBPressed_PlayerTargetingPanelButton(Button);
}

void UPlayerTargetingPanel::OnPlayerTargetingButtonEvent_RMBReleased(UMyButton * Button, ARTSPlayerController * LocalPlayCon)
{
	LocalPlayCon->OnRMBReleased_PlayerTargetingPanelButton(Button);
}

void UPlayerTargetingPanel::SetupTargetingButtonsForAbility(const FCommanderAbilityInfo * AbilityInfo)
{
	/* How the buttons are assigned players is as follows: 
	- yourself is always the first button
	- then your teammates 
	- then hostile players 
	If an ability cannot target yourself and/or teammates and/or hostiles then those will be 
	greyed out but the positioning will stay the same. If a player has been defeated they will 
	be greyed out but positioning will stay the same */

	for (auto & Elem : Buttons)
	{
		Elem.UpdateAppearance(*AbilityInfo);
	}
}

void UPlayerTargetingPanel::UIBinding_OnCancelButtonLMBPressed()
{
	PC->OnLMBPressed_HidePlayerTargetingPanel(Button_Cancel);
}

void UPlayerTargetingPanel::UIBinding_OnCancelButtonLMBReleased()
{
	PC->OnLMBReleased_HidePlayerTargetingPanel(Button_Cancel);
}

void UPlayerTargetingPanel::UIBinding_OnCancelButtonRMBPressed()
{
	PC->OnRMBPressed_HidePlayerTargetingPanel(Button_Cancel);
}

void UPlayerTargetingPanel::UIBinding_OnCancelButtonRMBReleased()
{
	PC->OnRMBReleased_HidePlayerTargetingPanel(Button_Cancel);
}
