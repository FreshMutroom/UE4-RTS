// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/InGameWidgetBase.h"
#include "GlobalSkillsPanel.generated.h"

struct FAquiredCommanderAbilityState;
class UGlobalSkillsPanelButton;


/**
 *	This panel holds buttons to abilities. It is like the panel you see on the side of the 
 *	screen in C&C Generals that has your commander abilities on it such as fuel air bomb 
 *	or artillery strike, but it may also be possible to have selectable abilities also appear 
 *	on it such as a nuke.
 *
 *	You will need to add enough UGlobalSkillsPanelButtons to it to accomodate the maximum 
 *	number of abilities that can appear on the panel.
 *
 *	About animations: 
 *	There will be an animation that plays when you aquire the first button. This will be an 
 *	animation on this widget. Then there will be another animation that will play for each 
 *	button when it becomes active.
 */
UCLASS(Abstract)
class RTS_VER2_API UGlobalSkillsPanel : public UInGameWidgetBase
{
	GENERATED_BODY()

public:

	UGlobalSkillsPanel(const FObjectInitializer & ObjectInitializer);
	
	virtual bool SetupWidget(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController) override;

protected:

	void MoreSetup();

	virtual void NativeTick(const FGeometry & MyGeometry, float InDeltaTime) override;

public:

	void OnCommanderSkillAquired_FirstRank(FAquiredCommanderAbilityState * AquiredAbilityInfo);
	void OnCommanderSkillAquired_NotFirstRank(int32 GlobalPanelIndex);

	void OnCommanderSkillUsed(float AbilityCooldown, int32 AbilitysAllButtonsIndex, bool bWasLastCharge);
	void OnCommanderAbilityCooledDown(int32 AbilitysAllButtonsIndex);

protected:

	//------------------------------------------------------
	//	------- Data -------
	//------------------------------------------------------

	/* Buttons whose ability is cooling down */
	TArray <UGlobalSkillsPanelButton *> CoolingDownAbilities;

	/* All the buttons on the panel. The lower index ones will be made active first */
	TArray <UGlobalSkillsPanelButton *> AllButtons;

#if WITH_EDITORONLY_DATA
	/**
	 *	If you name a widget animation this then it will be played when the first button 
	 *	becomes active 
	 */
	UPROPERTY(VisibleAnywhere, Category = "RTS", meta = (DisplayName = "EnterAnimation"))
	FString ABC;
#endif

	static const FName EnterAnimFName;

	UWidgetAnimation * EnterAnim;

	int32 NumActiveButtons;
};
