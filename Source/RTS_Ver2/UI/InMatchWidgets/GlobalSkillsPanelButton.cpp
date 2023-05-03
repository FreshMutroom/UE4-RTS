// Fill out your copyright notice in the Description page of Project Settings.

#include "GlobalSkillsPanelButton.h"

#include "UI/InMatchWidgets/Components/MyButton.h"
#include "GameFramework/RTSGameInstance.h"


const FName UGlobalSkillsPanelButton::EnterAnimFName = FName("EnterAnimation_INST");

UGlobalSkillsPanelButton::UGlobalSkillsPanelButton(const FObjectInitializer & ObjectInitializer)
	: Super(ObjectInitializer)
{
	OutOfChargesOpacityMultiplier = 0.5f;
}

bool UGlobalSkillsPanelButton::SetupWidget(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController)
{
	if (Super::SetupWidget(InGameInstance, InPlayerController))
	{
		return true;
	}

	OriginalOpacity = GetRenderOpacity();
	EnterAnim = UIUtilities::FindWidgetAnim(this, EnterAnimFName);

	if (IsWidgetBound(Button_Button))
	{
		Button_Button->SetPC(InPlayerController);
		Button_Button->SetOwningWidget();
		Button_Button->SetPurpose(EUIElementType::GlobalSkillsPanelButton, this);
		Button_Button->OnLeftMouseButtonPressed().BindUObject(this, &UGlobalSkillsPanelButton::UIBinding_OnLMBPress);
		Button_Button->OnLeftMouseButtonReleased().BindUObject(this, &UGlobalSkillsPanelButton::UIBinding_OnLMBReleased);
		Button_Button->OnRightMouseButtonPressed().BindUObject(this, &UGlobalSkillsPanelButton::UIBinding_OnRMBPress);
		Button_Button->OnRightMouseButtonReleased().BindUObject(this, &UGlobalSkillsPanelButton::UIBinding_OnRMBReleased);

		const FUnifiedImageAndSoundFlags UnifiedAssetFlags = GI->GetUnifiedButtonAssetFlags_ActionBar();
		if (UnifiedAssetFlags.bUsingUnifiedHoverSound == true)
		{
			Button_Button->SetHoveredSound(GI->GetUnifiedHoverSound_ActionBar().GetSound());
		}
		if (UnifiedAssetFlags.bUsingUnifiedLMBPressSound == true)
		{
			Button_Button->SetPressedByLMBSound(GI->GetUnifiedLMBPressedSound_ActionBar().GetSound());
		}
		if (UnifiedAssetFlags.bUsingUnifiedRMBPressSound == true)
		{
			Button_Button->SetPressedByRMBSound(GI->GetUnifiedRMBPressedSound_ActionBar().GetSound());
		}
	}

	return false;
}

void UGlobalSkillsPanelButton::OnCommanderSkillAquired_FirstRank(FAquiredCommanderAbilityState * AquiredAbilityInfo, 
	int32 PanelArrayIndex)
{
	const FCommanderAbilityInfo * AbilityInfo = AquiredAbilityInfo->GetAbilityInfo();

	if (IsWidgetBound(Button_Button))
	{
		// -------- Set images and sounds ---------
		
		const FUnifiedImageAndSoundFlags UnifiedAssetFlags = GI->GetUnifiedButtonAssetFlags_ActionBar();
		Button_Button->SetImages(UnifiedAssetFlags,
			&AbilityInfo->GetNormalImage(),
			&AbilityInfo->GetHoveredImage(),
			&AbilityInfo->GetPressedImage(),
			&AbilityInfo->GetHighlightedImage());

		if (UnifiedAssetFlags.bUsingUnifiedHoverSound == false)
		{
			Button_Button->SetHoveredSound(AbilityInfo->GetHoveredSound());
		}
		if (UnifiedAssetFlags.bUsingUnifiedLMBPressSound == false)
		{
			Button_Button->SetPressedByLMBSound(AbilityInfo->GetPressedByLMBSound());
		}
		if (UnifiedAssetFlags.bUsingUnifiedRMBPressSound == false)
		{
			Button_Button->SetPressedByRMBSound(AbilityInfo->GetPressedByRMBSound());
		}
	}
	
	if (IsWidgetBound(ProgressBar_Cooldown))
	{
		ProgressBar_Cooldown->SetPercent(AquiredAbilityInfo->GetCooldownPercent(GetWorld()->GetTimerManager()));
	}

	AssignedCommanderAbilityInfo = AquiredAbilityInfo;

	const bool bWasPanelPreviouslyEmpty = (PanelArrayIndex == 0);
	if (bWasPanelPreviouslyEmpty)
	{
		SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	else
	{
		if (EnterAnim != nullptr)
		{
			/* The animation could handle doing this but I'll do it now anyway */
			SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			
			PlayAnimation(EnterAnim);
		}
		else
		{
			// Appear instantly
			SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
	}
}

void UGlobalSkillsPanelButton::OnCommanderSkillAquired_NotFirstRank()
{
	const FCommanderAbilityInfo * AbilityInfo = AssignedCommanderAbilityInfo->GetAbilityInfo();

	if (IsWidgetBound(Button_Button))
	{
		// -------- Set images and sounds ---------

		const FUnifiedImageAndSoundFlags UnifiedAssetFlags = GI->GetUnifiedButtonAssetFlags_ActionBar();
		Button_Button->SetImages(UnifiedAssetFlags,
			&AbilityInfo->GetNormalImage(),
			&AbilityInfo->GetHoveredImage(),
			&AbilityInfo->GetPressedImage(),
			&AbilityInfo->GetHighlightedImage());

		if (UnifiedAssetFlags.bUsingUnifiedHoverSound == false)
		{
			Button_Button->SetHoveredSound(AbilityInfo->GetHoveredSound());
		}
		if (UnifiedAssetFlags.bUsingUnifiedLMBPressSound == false)
		{
			Button_Button->SetPressedByLMBSound(AbilityInfo->GetPressedByLMBSound());
		}
		if (UnifiedAssetFlags.bUsingUnifiedRMBPressSound == false)
		{
			Button_Button->SetPressedByRMBSound(AbilityInfo->GetPressedByRMBSound());
		}
	}

	if (IsWidgetBound(ProgressBar_Cooldown))
	{
		ProgressBar_Cooldown->SetPercent(AssignedCommanderAbilityInfo->GetCooldownPercent(GetWorld()->GetTimerManager()));
	}

	/* The ability may have run out of charges for the previous rank so restore it's appearance 
	to normal. Since it's assumed all abilities have at least one charge at the time they are 
	aquired we do this always */
	SetAppearanceForNotOutOfCharges();
}

void UGlobalSkillsPanelButton::UpdateCooldownProgress(const FTimerManager & TimerManager)
{
	if (IsWidgetBound(ProgressBar_Cooldown))
	{
		ProgressBar_Cooldown->SetPercent(AssignedCommanderAbilityInfo->GetCooldownPercent(TimerManager));
	}
}

void UGlobalSkillsPanelButton::OnLastChargeUsed()
{
	SetAppearanceForOutOfCharges();
}

const FAquiredCommanderAbilityState * UGlobalSkillsPanelButton::GetAbilityState() const
{
	return AssignedCommanderAbilityInfo;
}

void UGlobalSkillsPanelButton::NativeTick(const FGeometry & MyGeometry, float InDeltaTime)
{
	// If this ensure is hit it is likely UpdateCanTick as not called somewhere
	if (ensureMsgf(GetDesiredTickFrequency() != EWidgetTickFrequency::Never, TEXT("SObjectWidget and UUserWidget have mismatching tick states or UUserWidget::NativeTick was called manually (Never do this)")))
	{
		GInitRunaway();

		TickActionsAndAnimation(MyGeometry, InDeltaTime);
	}
}

void UGlobalSkillsPanelButton::SetAppearanceForNotOutOfCharges()
{
	SetRenderOpacity(OriginalOpacity);
}

void UGlobalSkillsPanelButton::SetAppearanceForOutOfCharges()
{
	SetRenderOpacity(OriginalOpacity * OutOfChargesOpacityMultiplier);
}

void UGlobalSkillsPanelButton::UIBinding_OnLMBPress()
{
	PC->OnLMBPressed_GlobalSkillsPanelButton(Button_Button);
}

void UGlobalSkillsPanelButton::UIBinding_OnLMBReleased()
{
	PC->OnLMBReleased_GlobalSkillsPanelButton(Button_Button, this);
}

void UGlobalSkillsPanelButton::UIBinding_OnRMBPress()
{
	PC->OnRMBPressed_GlobalSkillsPanelButton(Button_Button);
}

void UGlobalSkillsPanelButton::UIBinding_OnRMBReleased()
{
	PC->OnRMBReleased_GlobalSkillsPanelButton(Button_Button, this);
}
