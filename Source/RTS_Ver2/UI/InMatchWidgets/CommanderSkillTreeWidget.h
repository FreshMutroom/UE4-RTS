// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/InGameWidgetBase.h"
#include "CommanderSkillTreeWidget.generated.h"

class UTextBlock;
class UProgressBar;
class UCommanderSkillTreeNodeWidget;
struct FCommanderAbilityTreeNodeInfo;
enum class ECommanderSkillTreeAnimationPlayRule : uint8;


struct FNodeArray
{
	TArray<UCommanderSkillTreeNodeWidget *> Array;
};


/**
 *	The widget that is the skill tree for the commander. 
 *
 *	Place skill tree node widgets onto it. Cannot have duplicate of the same node type.
 */
UCLASS(Abstract)
class RTS_VER2_API UCommanderSkillTreeWidget : public UInGameWidgetBase
{
	GENERATED_BODY()
	
public:

	UCommanderSkillTreeWidget(const FObjectInitializer & ObjectInitializer);

	/* Setup the widget */
	void SetupWidget_GettingDestroyedAfter(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController);

	virtual bool SetupWidget(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController) override;
	
	/** 
	 *	@param PlayerState - the player state for the player that owns this widget 
	 *	@param FactionInfo - faction info of PlayerState 
	 *	@param PlayAnimRule - the rule that says whether to play the "show me" anim for the 
	 *	toggle commander skill tree button on the HUD
	 *	@return - true if the "show me" anim should be played
	 */
	bool MoreSetup(ARTSPlayerState * PlayerState, AFactionInfo * FactionInfo, 
		ECommanderSkillTreeAnimationPlayRule PlayAnimRule);

	/* Overridden because UInGameWidgetBase::NativeTick does nothing */
	virtual void NativeTick(const FGeometry & MyGeometry, float InDeltaTime) override;

	bool IsShowingOrPlayingShowAnimation() const;

	/**
	 *	These two funcs aren't really requests since I check before calling them whether the 
	 *	widget is showing or not so better names are Show() and Hide() 
	 */
	void OnRequestToBeShown();
	void OnRequestToBeHidden();

	virtual bool RespondToEscapeRequest(URTSHUD * HUD) override;

	/* Called when the local player gains experience but does not level up as a result of it */
	void OnExperienceGained(float ExperienceTowardsNextRank, float ExperienceRequiredForNextRank);

	/** 
	 *	Called when the local player levels up but only for the last level gained if multiple were
	 *	gained at a time 
	 *
	 *	@param PlayAnimRule - the rule that says when to play the "show me" anim for the 
	 *	toggle skill tree button on the HUD
	 *	@return - true if the "show me" anim should be played 
	 */
	bool OnLevelUp_LastForEvent(uint8 OldRank, uint8 NewRank, int32 NumUnspentSkillPoints,
		float ExperienceTowardsNextRank, float ExperienceRequiredForNextRank, 
		ECommanderSkillTreeAnimationPlayRule PlayAnimRule);

	void OnNewAbilityRankAquired(const FCommanderAbilityTreeNodeInfo * AquiredNodeInfo, int32 NewAbilityRank, 
		int32 NumUnspentSkillPoints);

protected:

	static FText GetExperienceAquiredAsText(float ExperienceAquired);
	static FText GetExperienceRequiredAsText(float ExperienceRequired);

	//------------------------------------------------------------
	//	Data
	//------------------------------------------------------------

	/* If false then this is either hidden or playing the hide animation */
	bool bShownOrPlayingShowAnim;

	/** 
	 *	Maps rank to the nodes that are unlocked when that rank is aquired. Might not 
	 *	include and entry for rank 0. 
	 *	The same node can appear in different value arrays e.g. rank 1 of ability unlocks 
	 *	at commander rank 1 but ability rank 2 unlocks at commander rank 2
	 */
	TMap <uint8, FNodeArray> NodesUnlockedByRank;

	/* Nodes that the player does not have enough skill points to aquire or prereqs not met 
	but they are a high enough rank. This will include nodes that the player may have aquired 
	the lower ranks of the ability but they have not aquired every rank of the ability yet */
	TArray<UCommanderSkillTreeNodeWidget *> RankHighEnoughButNotFullyAquiredNodes;

#if WITH_EDITORONLY_DATA
	/* ***** IF ANIMS DONT WORK IT MIGHT BE BECAUSE TICK IS DISABLED ***** */
	
	/** 
	 *	If you name a widget animation this then it will be played when this widget is requested 
	 *	to be shown. You are in charge of making it SelfHitTestInvisible/Visible at a certain point 
	 *	in the animation. 
	 */
	UPROPERTY(VisibleAnywhere, Category = "RTS", meta = (DisplayName = "ShowAnimation"))
	FString ABC;

	/**
	 *	If you name a widget animation this then it will be played when this widget is requested
	 *	to be hidden. You are in charge of making it HitTestInvisible and/or Hidden at a certain point in the animation.
	 */
	UPROPERTY(VisibleAnywhere, Category = "RTS", meta = (DisplayName = "HideAnimation"))
	FString ABCDEFG;

	static const FName ShowAnimationFName;
	static const FName HideAnimationFName;
#endif

	/* Animations to play when the widget is requested to be shown hidden. Can be null implying 
	you do not want an animation */
	UWidgetAnimation * ShowAnim;
	UWidgetAnimation * HideAnim;

	/** Displays the player's rank */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_Rank;

	/** Displays how many skill points the player has not spent */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_UnspentSkillPoints;

	/** Displays how much experience the player has towards the next level. Not cumulative */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_ExperienceAquired;

	/** Displays how much experience the player requires to reach the next level */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_ExperienceRequired;

	/** Progress bar to display how much experience the player has */
	UPROPERTY(meta = (BindWidgetOptional))
	UProgressBar * ProgressBar_Experience;
};
