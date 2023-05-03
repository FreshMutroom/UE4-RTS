// Fill out your copyright notice in the Description page of Project Settings.

#include "CommanderSkillTreeWidget.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Animation/WidgetAnimation.h"
#include "Blueprint/WidgetTree.h"

#include "UI/InMatchWidgets/CommanderSkillTreeNodeWidget.h"
#include "UI/UIUtilities.h"
#include "UI/RTSHUD.h" // For ECommanderSkillTreeAnimationPlayRule


const FName UCommanderSkillTreeWidget::ShowAnimationFName = FName("ShowAnimation_INST");
const FName UCommanderSkillTreeWidget::HideAnimationFName = FName("HideAnimation_INST");

UCommanderSkillTreeWidget::UCommanderSkillTreeWidget(const FObjectInitializer & ObjectInitializer)
	: Super(ObjectInitializer)
{
	bShownOrPlayingShowAnim = false;
	
	NodesUnlockedByRank.Reserve(LevelingUpOptions::MAX_COMMANDER_LEVEL);
	for (uint8 i = 1; i <= LevelingUpOptions::MAX_COMMANDER_LEVEL; ++i)
	{
		NodesUnlockedByRank.Emplace(i, FNodeArray());
	}
}

void UCommanderSkillTreeWidget::SetupWidget_GettingDestroyedAfter(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController)
{
	GI = InGameInstance;
	PC = CastChecked<ARTSPlayerController>(GetWorld()->GetFirstPlayerController());
}

bool UCommanderSkillTreeWidget::SetupWidget(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController)
{
	/* Bear in mind that this func is not called if the widget is only being spawned so info 
	can be extracted for it */

	if (Super::SetupWidget(InGameInstance, InPlayerController))
	{
		return true;
	}

	ShowAnim = UIUtilities::FindWidgetAnim(this, ShowAnimationFName);
	HideAnim = UIUtilities::FindWidgetAnim(this, HideAnimationFName);

	return false;
}

bool UCommanderSkillTreeWidget::MoreSetup(ARTSPlayerState * PlayerState, AFactionInfo * FactionInfo, 
	ECommanderSkillTreeAnimationPlayRule PlayAnimRule)
{
#if !UE_BUILD_SHIPPING
	assert(PC != nullptr);
	// If calling this for non-local player state then we better be the server
	if (PlayerState != GetWorld()->GetFirstPlayerController()->PlayerState)
	{
		assert(GetWorld()->IsServer());
	}

	TSet<ECommanderSkillTreeNodeType> CheckForDuplicates;
#endif

	/* Whether to play the "show me" anim for the toggle skill panel button on the HUD */
	bool bPlayShowMeAnim = false;

	// Setup all the node widgets
	const int32 InitialSkillPoints = FactionInfo->GetNumInitialCommanderSkillPoints();
	TArray <UWidget *> AllChildren;
	WidgetTree->GetAllWidgets(AllChildren);
	int32 ArrayIndex = 0;
	for (UWidget * Widget : AllChildren)
	{
		UCommanderSkillTreeNodeWidget * NodeWidget = Cast<UCommanderSkillTreeNodeWidget>(Widget);
		if (NodeWidget != nullptr)
		{
			const ECommanderSkillTreeNodeType NodeType = NodeWidget->SetupNodeWidget(GI, PC, FactionInfo, this, ArrayIndex);

#if !UE_BUILD_SHIPPING

			UE_CLOG(CheckForDuplicates.Contains(NodeType), RTSLOG, Fatal, TEXT("Commander skill tree [%s] "
				"contains a duplicate of the node type [%s]. This is not allowed. "), *GetClass()->GetName(),
				TO_STRING(ECommanderSkillTreeNodeType, NodeType));
			
			CheckForDuplicates.Emplace(NodeType);

#endif

			PlayerState->RegisterCommanderSkillTreeNode(NodeWidget->GetNodeInfo(), ArrayIndex++);

			const uint8 NumAbilityRanks = NodeWidget->GetNodeInfo()->GetNumRanks();
			for (uint8 i = 0; i < NumAbilityRanks; ++i)
			{
				const uint8 UnlockRank = NodeWidget->GetNodeInfo()->GetUnlockRank(i);

				if (UnlockRank > 0)
				{
					NodesUnlockedByRank[UnlockRank].Array.Emplace(NodeWidget);
				}

				/* Check if the first rank of the ability is unlocked at rank 0 (i.e. can possibly 
				be purchased now if the player has enough skill points) */
				if (i == 0)
				{
					if (UnlockRank == 0)
					{
						assert(RankHighEnoughButNotFullyAquiredNodes.Contains(NodeWidget) == false);
						RankHighEnoughButNotFullyAquiredNodes.Emplace(NodeWidget);

						if (PlayerState->ArePrerequisitesForNextCommanderAbilityRankMet(*NodeWidget->GetNodeInfo())
							&& PlayerState->CanAffordCommanderAbilityAquireCost(*NodeWidget->GetNodeInfo()))
						{
							NodeWidget->SetAppearanceForAquirable();

							bPlayShowMeAnim = (PlayAnimRule == ECommanderSkillTreeAnimationPlayRule::CanAffordAbilityAtStartOfMatchOrLevelUp);
						}
						else
						{
							NodeWidget->SetAppearanceForCannotAffordOrPrerequisitesNotMet();
						}
					}
					else if (UnlockRank > 0) // Shouldn't this be implied? i.e. just else here should work
					{
						NodeWidget->SetAppearanceForRankNotHighEnough();
					}
				}
			}
		}
	}

	//----------------------------------------------------------------
	//	Setup the info widgets
	//----------------------------------------------------------------
	
	if (IsWidgetBound(Text_Rank))
	{
		// Set text for rank 0 because that is what the player starts the match at
		Text_Rank->SetText(FText::AsNumber(0)); // Possibly want to show some text like a name instead of a number
	}
	if (IsWidgetBound(Text_UnspentSkillPoints))
	{
		Text_UnspentSkillPoints->SetText(FText::AsNumber(InitialSkillPoints));
	}
	if (IsWidgetBound(Text_ExperienceAquired))
	{
		Text_ExperienceAquired->SetText(GetExperienceAquiredAsText(0.f));
	}
	if (IsWidgetBound(Text_ExperienceRequired)) 
	{
		if (LevelingUpOptions::MAX_COMMANDER_LEVEL > 0)
		{
			const float ExperienceRequired = FactionInfo->GetCommanderLevelUpInfo(1).GetExperienceRequired();
			Text_ExperienceRequired->SetText(GetExperienceRequiredAsText(ExperienceRequired));
		}
	}
	if (IsWidgetBound(ProgressBar_Experience))
	{
		ProgressBar_Experience->SetPercent(0.f);
	}

	return bPlayShowMeAnim;
}

void UCommanderSkillTreeWidget::NativeTick(const FGeometry & MyGeometry, float InDeltaTime)
{
	// If this ensure is hit it is likely UpdateCanTick as not called somewhere
	if (ensureMsgf(GetDesiredTickFrequency() != EWidgetTickFrequency::Never, TEXT("SObjectWidget and UUserWidget have mismatching tick states or UUserWidget::NativeTick was called manually (Never do this)")))
	{
		GInitRunaway();

		TickActionsAndAnimation(MyGeometry, InDeltaTime);
	}
}

bool UCommanderSkillTreeWidget::IsShowingOrPlayingShowAnimation() const
{
	return bShownOrPlayingShowAnim;
}

void UCommanderSkillTreeWidget::OnRequestToBeShown()
{
	/* Quick note: UWidgetAnimation::GetEndTime() will look at where the red line at the end is, 
	so make sure to move this red line to the point where you consider the animation done */
	
	assert(!bShownOrPlayingShowAnim);

	bShownOrPlayingShowAnim = true;

	/* Play animation if set. Otherwise just show straight away */
	if (ShowAnim != nullptr)
	{
		float StartTime;
		if (HideAnim != nullptr)
		{
			if (IsAnimationPlaying(HideAnim))
			{
				// I hope GetEndTime just gets the duration of the animation
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

void UCommanderSkillTreeWidget::OnRequestToBeHidden()
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

bool UCommanderSkillTreeWidget::RespondToEscapeRequest(URTSHUD * HUD)
{
	assert(IsShowingOrPlayingShowAnimation());
	OnRequestToBeHidden();
	return true;
}

void UCommanderSkillTreeWidget::OnExperienceGained(float ExperienceTowardsNextRank, float ExperienceRequiredForNextRank)
{
	if (IsWidgetBound(Text_ExperienceAquired))
	{
		Text_ExperienceAquired->SetText(GetExperienceAquiredAsText(ExperienceTowardsNextRank));
	}
	if (IsWidgetBound(ProgressBar_Experience))
	{
		ProgressBar_Experience->SetPercent(ExperienceTowardsNextRank / ExperienceRequiredForNextRank);
	}
}

bool UCommanderSkillTreeWidget::OnLevelUp_LastForEvent(uint8 OldRank, uint8 NewRank, int32 NumUnspentSkillPoints, 
	float ExperienceTowardsNextRank, float ExperienceRequiredForNextRank, ECommanderSkillTreeAnimationPlayRule PlayAnimRule)
{
	if (IsWidgetBound(Text_Rank))
	{
		Text_Rank->SetText(FText::AsNumber(NewRank));
	}
	if (IsWidgetBound(Text_UnspentSkillPoints))
	{
		Text_UnspentSkillPoints->SetText(FText::AsNumber(NumUnspentSkillPoints));
	}
	if (IsWidgetBound(Text_ExperienceAquired))
	{
		Text_ExperienceAquired->SetText(GetExperienceAquiredAsText(ExperienceTowardsNextRank));
	}
	if (IsWidgetBound(Text_ExperienceRequired))
	{
		Text_ExperienceRequired->SetText(GetExperienceRequiredAsText(ExperienceRequiredForNextRank));
	}
	if (IsWidgetBound(ProgressBar_Experience))
	{
		ProgressBar_Experience->SetPercent(ExperienceTowardsNextRank / ExperienceRequiredForNextRank);
	}

	/* Whether to play the "show me" anim on the HUD for the toggle skill tree button */
	bool bPlayShowMeAnim = false;

	for (UCommanderSkillTreeNodeWidget * NodeWidget : RankHighEnoughButNotFullyAquiredNodes)
	{
		if (PS->GetCommanderAbilityObtainedRank(*NodeWidget->GetNodeInfo()) == -1)
		{
			// Check if we can afford it now and prereqs are met
			if (PS->ArePrerequisitesForNextCommanderAbilityRankMet(*NodeWidget->GetNodeInfo())
				&& PS->CanAffordCommanderAbilityAquireCost(*NodeWidget->GetNodeInfo()))
			{
				NodeWidget->SetAppearanceForAquirable();
				bPlayShowMeAnim = (PlayAnimRule == ECommanderSkillTreeAnimationPlayRule::CanAffordAbilityAtStartOfMatchOrLevelUp);
			}
		}
	}

	TSet <UCommanderSkillTreeNodeWidget *> JustAddedToRankHighEnoughButNotFullyAquiredNodes;

	/* Now update the tree so abilities unlock or become aquirable now that we have more 
	skill points to spend */
	for (uint8 i = OldRank + 1; i <= NewRank; ++i)
	{
		for (UCommanderSkillTreeNodeWidget * NodeWidget : NodesUnlockedByRank[i].Array)
		{
			/* Ahh damn this got complicated kinda. Basically a node can have multiple ranks 
			of the ability. We only want to add one of the node to 
			RankHighEnoughButNotFullyAquiredNodes */
			if (JustAddedToRankHighEnoughButNotFullyAquiredNodes.Contains(NodeWidget) == false)
			{
				JustAddedToRankHighEnoughButNotFullyAquiredNodes.Emplace(NodeWidget);
				RankHighEnoughButNotFullyAquiredNodes.Emplace(NodeWidget);
			}
			
			/* If have already aquired at least one rank then do not alter appearance */
			const int32 AquiredAbilityRank = PS->GetCommanderAbilityObtainedRank(*NodeWidget->GetNodeInfo());
			if (AquiredAbilityRank == -1)
			{
				if (PS->ArePrerequisitesForNextCommanderAbilityRankMet(*NodeWidget->GetNodeInfo())
					&& PS->CanAffordCommanderAbilityAquireCost(*NodeWidget->GetNodeInfo()))
				{
					NodeWidget->SetAppearanceForAquirable();
					bPlayShowMeAnim = (PlayAnimRule == ECommanderSkillTreeAnimationPlayRule::CanAffordAbilityAtStartOfMatchOrLevelUp);
				}
				else
				{
					NodeWidget->SetAppearanceForCannotAffordOrPrerequisitesNotMet();
				}
			}
		}
	}

	return bPlayShowMeAnim;
}

void UCommanderSkillTreeWidget::OnNewAbilityRankAquired(const FCommanderAbilityTreeNodeInfo * AquiredNodeInfo, 
	int32 NewAbilityRank, int32 NumUnspentSkillPoints)
{
	if (IsWidgetBound(Text_UnspentSkillPoints))
	{
		Text_UnspentSkillPoints->SetText(FText::AsNumber(NumUnspentSkillPoints));
	}

	/* For each node check if 
	- can afford 
	- prereqs are met 
	Both could have changed because aquiring an ability usually requires spending skill points 
	and it can unlock prereqs for some nodes */
	for (int32 i = RankHighEnoughButNotFullyAquiredNodes.Num() - 1; i >= 0; --i)
	{
		UCommanderSkillTreeNodeWidget * NodeWidget = RankHighEnoughButNotFullyAquiredNodes[i];
		
		if (AquiredNodeInfo == NodeWidget->GetNodeInfo())
		{
			/* Check if at max rank for ability */
			const bool bAtMaxRankForAbility = (AquiredNodeInfo->GetNumRanks() == NewAbilityRank + 1);
			if (bAtMaxRankForAbility)
			{
				RankHighEnoughButNotFullyAquiredNodes.RemoveAtSwap(i, 1, false);
			}
			/* Check if commander rank isn't high enough to aquire the next rank of the ability */
			else if (AquiredNodeInfo->GetUnlockRank(NewAbilityRank + 1) > PS->GetRank())
			{
				RankHighEnoughButNotFullyAquiredNodes.RemoveAtSwap(i, 1, false);
			}
			
			NodeWidget->OnAbilityRankAquired(NewAbilityRank);
		}
		else
		{
			if (PS->GetCommanderAbilityObtainedRank(*NodeWidget->GetNodeInfo()) == -1)
			{
				if (PS->CanAffordCommanderAbilityAquireCost(*NodeWidget->GetNodeInfo()))
				{
					if (PS->ArePrerequisitesForNextCommanderAbilityRankMet(*NodeWidget->GetNodeInfo()))
					{
						NodeWidget->SetAppearanceForAquirable();
					}
					else
					{
						NodeWidget->SetAppearanceForCannotAffordOrPrerequisitesNotMet();
					}
				}
				else
				{
					NodeWidget->SetAppearanceForCannotAffordOrPrerequisitesNotMet();
				}
			}
		}
	}
}

FText UCommanderSkillTreeWidget::GetExperienceAquiredAsText(float ExperienceAquired)
{
	FloatToText(ExperienceAquired, UIOptions::PlayerExperienceGainedDisplayMethod);
}

FText UCommanderSkillTreeWidget::GetExperienceRequiredAsText(float ExperienceRequired)
{
	FloatToText(ExperienceRequired, UIOptions::PlayerExperienceRequiredDisplayMethod);
}
