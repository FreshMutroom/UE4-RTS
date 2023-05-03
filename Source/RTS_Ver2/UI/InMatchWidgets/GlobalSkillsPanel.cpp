// Fill out your copyright notice in the Description page of Project Settings.

#include "GlobalSkillsPanel.h"

#include "UI/InMatchWidgets/GlobalSkillsPanelButton.h"


const FName UGlobalSkillsPanel::EnterAnimFName = FName("EnterAnimation_INST");

UGlobalSkillsPanel::UGlobalSkillsPanel(const FObjectInitializer & ObjectInitializer)
	: Super(ObjectInitializer)
{
	NumActiveButtons = 0;
}

bool UGlobalSkillsPanel::SetupWidget(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController)
{
	if (Super::SetupWidget(InGameInstance, InPlayerController))
	{
		return true;
	}
	
	MoreSetup();

	return false;
}

void UGlobalSkillsPanel::MoreSetup()
{
	/* Start hidden because player doesn't have any skills at start of match */
	SetVisibility(ESlateVisibility::Hidden);

	//-------------------------------------------------------------
	// ------- Assign widget animation references -------
	//-------------------------------------------------------------

	EnterAnim = UIUtilities::FindWidgetAnim(this, UGlobalSkillsPanel::EnterAnimFName);

	//-------------------------------------------------------------
	// ------- Setup buttons -------
	//-------------------------------------------------------------

	TArray <UWidget *> AllChildren;
	WidgetTree->GetAllWidgets(AllChildren);
	for (UWidget * Widget : AllChildren)
	{
		UGlobalSkillsPanelButton * Button = Cast<UGlobalSkillsPanelButton>(Widget);
		if (Button != nullptr)
		{
			Button->SetupWidget(GI, PC);
			Button->SetVisibility(ESlateVisibility::Hidden);
			AllButtons.Emplace(Button);
		}
	}
}

void UGlobalSkillsPanel::NativeTick(const FGeometry & MyGeometry, float InDeltaTime)
{
	// If this ensure is hit it is likely UpdateCanTick as not called somewhere
	if (ensureMsgf(GetDesiredTickFrequency() != EWidgetTickFrequency::Never, TEXT("SObjectWidget and UUserWidget have mismatching tick states or UUserWidget::NativeTick was called manually (Never do this)")))
	{
		GInitRunaway();

		TickActionsAndAnimation(MyGeometry, InDeltaTime);

		const FTimerManager & TimerManager = GetWorld()->GetTimerManager();
		for (auto & Button : CoolingDownAbilities)
		{
			Button->UpdateCooldownProgress(TimerManager);
		}
	}
}

void UGlobalSkillsPanel::OnCommanderSkillAquired_FirstRank(FAquiredCommanderAbilityState * AquiredAbilityInfo)
{
	if (NumActiveButtons == 0)
	{
		/* There are no buttons showing, so play the enter animation if set on this widget */
		if (EnterAnim != nullptr)
		{
			/* The animation could do this, but I'll do it here now anyway */
			SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			
			PlayAnimation(EnterAnim);
		}
		else
		{
			// No animation so just make panel instantly appear
			SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
	}

	AquiredAbilityInfo->SetGlobalSkillsPanelButtonIndex(NumActiveButtons);

	UE_CLOG(AllButtons.IsValidIndex(NumActiveButtons) == false, RTSLOG, Fatal, TEXT("Faction [%s]'s "
		"global skills panel widget [%s] does not have enough buttons on it "), 
		TO_STRING(EFaction, FI->GetFaction()), *GetClass()->GetName());

	UGlobalSkillsPanelButton * AffectedButton = AllButtons[NumActiveButtons];

	/* Check if the ability has an initial cooldown and if so add it to a container */
	if (AquiredAbilityInfo->GetAbilityInfo()->GetInitialCooldown() > 0.f)
	{
		CoolingDownAbilities.Emplace(AffectedButton);
	}

	AffectedButton->OnCommanderSkillAquired_FirstRank(AquiredAbilityInfo, NumActiveButtons);
	
	NumActiveButtons++;
}

void UGlobalSkillsPanel::OnCommanderSkillAquired_NotFirstRank(int32 GlobalPanelIndex)
{
	assert(NumActiveButtons > 0);

	AllButtons[GlobalPanelIndex]->OnCommanderSkillAquired_NotFirstRank();
}

void UGlobalSkillsPanel::OnCommanderSkillUsed(float AbilityCooldown, int32 AbilitysAllButtonsIndex, 
	bool bWasLastCharge)
{
	if (bWasLastCharge)
	{
		UGlobalSkillsPanelButton * AffectedButton = AllButtons[AbilitysAllButtonsIndex];
		
		AffectedButton->OnLastChargeUsed();
	}
	else if (AbilityCooldown > 0.f)
	{
		UGlobalSkillsPanelButton * AffectedButton = AllButtons[AbilitysAllButtonsIndex];
		
		assert(CoolingDownAbilities.Contains(AffectedButton) == false);
		CoolingDownAbilities.Emplace(AffectedButton);
	}
}

void UGlobalSkillsPanel::OnCommanderAbilityCooledDown(int32 AbilitysAllButtonsIndex)
{
	assert(CoolingDownAbilities.Contains(AllButtons[AbilitysAllButtonsIndex]) == true);
	CoolingDownAbilities.RemoveSingleSwap(AllButtons[AbilitysAllButtonsIndex]);
}
