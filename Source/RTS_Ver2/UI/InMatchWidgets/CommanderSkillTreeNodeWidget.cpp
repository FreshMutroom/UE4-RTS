// Fill out your copyright notice in the Description page of Project Settings.

#include "CommanderSkillTreeNodeWidget.h"

#include "GameFramework/FactionInfo.h"
#include "Statics/DevelopmentStatics.h"
#include "GameFramework/RTSGameInstance.h"
#include "GameFramework/RTSPlayerState.h"
#include "GameFramework/RTSPlayerController.h"


const FLinearColor UCommanderSkillTreeNodeWidget::UNAQUIRABLE_COLOR_MULTIPLIER = FLinearColor(1.0f, 1.0f, 1.0f, 0.4f);

UCommanderSkillTreeNodeWidget::UCommanderSkillTreeNodeWidget(const FObjectInitializer & ObjectInitializer)
	: Super(ObjectInitializer)
{
	Purpose = EUIElementType::CommanderSkillTreeNode;
	Type = ECommanderSkillTreeNodeType::None;
	AquiredColor = FLinearColor(0.05f, 1.f, 0.05f, 0.85f);
}

ECommanderSkillTreeNodeType UCommanderSkillTreeNodeWidget::SetupNodeWidget(URTSGameInstance * GameInst,
	ARTSPlayerController * InPlayerController, AFactionInfo * FactionInfo, 
	UCommanderSkillTreeWidget * InSkillTreeWidget, uint8 InAllNodesArrayIndex)
{
	UE_CLOG(Type == ECommanderSkillTreeNodeType::None, RTSLOG, Fatal, TEXT("Commander skill tree "
		"node widget [%s] does not have its type set"), *GetName());

	assert(GameInst != nullptr);
	assert(InPlayerController != nullptr);

	GI = GameInst;
	PC = InPlayerController;
	SetPC(InPlayerController);
	SetOwningWidget();
	SetPurpose(Purpose, nullptr);

	OnLeftMouseButtonPressed().BindUObject(this, &UCommanderSkillTreeNodeWidget::UIBinding_OnLMBPress);
	OnLeftMouseButtonReleased().BindUObject(this, &UCommanderSkillTreeNodeWidget::UIBinding_OnLMBReleased);
	OnRightMouseButtonPressed().BindUObject(this, &UCommanderSkillTreeNodeWidget::UIBinding_OnRMBPress);
	OnRightMouseButtonReleased().BindUObject(this, &UCommanderSkillTreeNodeWidget::UIBinding_OnRMBReleased);

	OriginalColor = ContentColorAndOpacity;

	NodeInfo = &GameInst->GetCommanderSkillTreeNodeInfo(Type);
	AllNodesArrayIndex = InAllNodesArrayIndex;

	/* Set image/sounds for the 1st rank of the ability */
	
	// Using the action bar ones because who cares
	const FUnifiedImageAndSoundFlags UnifiedAssetFlags = GameInst->GetUnifiedButtonAssetFlags_ActionBar();
	SetImages_CommandSkillTreeNode(UnifiedAssetFlags,
		&NodeInfo->GetNormalImage(0),
		&NodeInfo->GetHoveredImage(0),
		&NodeInfo->GetPressedImage(0));
	if (UnifiedAssetFlags.bUsingUnifiedHoverSound == false)
	{
		SetHoveredSound(NodeInfo->GetHoveredSound(0));
	}
	else
	{
		SetHoveredSound(GameInst->GetUnifiedHoverSound_ActionBar().GetSound());
	}
	if (UnifiedAssetFlags.bUsingUnifiedLMBPressSound == false)
	{
		SetPressedByLMBSound(NodeInfo->GetPressedByLMBSound(0));
	}
	else
	{
		SetPressedByLMBSound(GameInst->GetUnifiedLMBPressedSound_ActionBar().GetSound());
	}
	if (UnifiedAssetFlags.bUsingUnifiedRMBPressSound == false)
	{
		SetPressedByRMBSound(NodeInfo->GetPressedByRMBSound(0));
	}
	else
	{
		SetPressedByRMBSound(GameInst->GetUnifiedRMBPressedSound_ActionBar().GetSound());
	}

	return Type;
}

void UCommanderSkillTreeNodeWidget::SetAppearanceForCannotAffordOrPrerequisitesNotMet()
{
	SetBrushColor(OriginalColor * UNAQUIRABLE_COLOR_MULTIPLIER);
}

void UCommanderSkillTreeNodeWidget::SetAppearanceForRankNotHighEnough()
{
	SetBrushColor(OriginalColor * UNAQUIRABLE_COLOR_MULTIPLIER);
}

void UCommanderSkillTreeNodeWidget::SetAppearanceForAquirable()
{
	SetBrushColor(OriginalColor);
}

void UCommanderSkillTreeNodeWidget::SetAppearanceForAquired()
{
	SetBrushColor(AquiredColor);
}

const FCommanderAbilityTreeNodeInfo * UCommanderSkillTreeNodeWidget::GetNodeInfo() const
{
	return NodeInfo;
}

void UCommanderSkillTreeNodeWidget::OnClicked(ARTSPlayerController * PlayCon, ARTSPlayerState * PlayerState)
{
	// 2nd param = handle HUD warnings
	const EGameWarning Warning = PlayerState->CanAquireCommanderAbility(*NodeInfo, true);
	if (Warning == EGameWarning::None)
	{	
		if (GetWorld()->IsServer())
		{
			PlayerState->AquireNextRankForCommanderAbility(NodeInfo, true, AllNodesArrayIndex);
		}
		else
		{
			/* Send RPC to server */
			PlayCon->Server_RequestAquireCommanderSkill(AllNodesArrayIndex);
		}
	}
}

void UCommanderSkillTreeNodeWidget::OnAbilityRankAquired(int32 NewRank)
{
	/* The node's appearance is already set up for the first rank, so if this is the 2nd, 3rd, etc 
	rank then we need to update the image + sounds */
	if (NewRank > 0)
	{
		const FCommanderAbilityInfo * NewAbilityRankInfo = NodeInfo->GetAbilityInfo(NewRank).GetAbilityInfo();
		
		const FUnifiedImageAndSoundFlags UnifiedAssetFlags = GI->GetUnifiedButtonAssetFlags_ActionBar();
		SetImages_ExcludeHighlightedImage(UnifiedAssetFlags,
			&NewAbilityRankInfo->GetNormalImage(),
			&NewAbilityRankInfo->GetHoveredImage(),
			&NewAbilityRankInfo->GetPressedImage());
		if (UnifiedAssetFlags.bUsingUnifiedHoverSound == false)
		{
			SetHoveredSound(NewAbilityRankInfo->GetHoveredSound());
		}
		if (UnifiedAssetFlags.bUsingUnifiedLMBPressSound == false)
		{
			SetPressedByLMBSound(NewAbilityRankInfo->GetPressedByLMBSound());
		}
		if (UnifiedAssetFlags.bUsingUnifiedRMBPressSound == false)
		{
			SetPressedByRMBSound(NewAbilityRankInfo->GetPressedByRMBSound());
		}
	}

	SetAppearanceForAquired();
}

void UCommanderSkillTreeNodeWidget::UIBinding_OnLMBPress()
{
	PC->OnLMBPressed_CommanderSkillTreeNode(this);
}

void UCommanderSkillTreeNodeWidget::UIBinding_OnLMBReleased()
{
	PC->OnLMBReleased_CommanderSkillTreeNode(this);
}

void UCommanderSkillTreeNodeWidget::UIBinding_OnRMBPress()
{
	PC->OnRMBPressed_CommanderSkillTreeNode(this);
}

void UCommanderSkillTreeNodeWidget::UIBinding_OnRMBReleased()
{
	PC->OnRMBReleased_CommanderSkillTreeNode(this);
}
