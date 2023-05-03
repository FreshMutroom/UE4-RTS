// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/InMatchWidgets/Components/MyButton.h"
#include "CommanderSkillTreeNodeWidget.generated.h"

class URTSGameInstance;
class AFactionInfo;
struct FCommanderAbilityTreeNodeInfo;
enum class ECommanderSkillTreeNodeType : uint8;
struct FCommanderAbilityTreeNodeSingleRankInfo;
class ARTSPlayerController;
class ARTSPlayerState;
class UCommanderSkillTreeWidget;


/**
 *	Single node on commander's skill tree
 */
UCLASS(meta = (DisplayName = "Commander Skill Tree: Node Button"))
class RTS_VER2_API UCommanderSkillTreeNodeWidget : public UMyButton
{
	GENERATED_BODY()

	/* Tint to multiply button color by when it cannot be aquired either because: 
	- the player's rank is not high enough 
	- the player does not have enough skill points */
	static const FLinearColor UNAQUIRABLE_COLOR_MULTIPLIER;

public:

	UCommanderSkillTreeNodeWidget(const FObjectInitializer & ObjectInitializer);

	/* @return - node tyep for this widget */
	ECommanderSkillTreeNodeType SetupNodeWidget(URTSGameInstance * GameInst, ARTSPlayerController * InPlayerController, 
		AFactionInfo * FactionInfo, UCommanderSkillTreeWidget * InSkillTreeWidget, 
		uint8 InAllNodesArrayIndex);

	void SetAppearanceForCannotAffordOrPrerequisitesNotMet();
	void SetAppearanceForRankNotHighEnough();
	void SetAppearanceForAquirable();
	void SetAppearanceForAquired();

	const FCommanderAbilityTreeNodeInfo * GetNodeInfo() const;

	/**
	 *	Called when the button is clicked.
	 *	
	 *	@param PlayCon - player controller for the local player 
	 *	@param PlayerState - player state of the local player 
	 */
	void OnClicked(ARTSPlayerController * PlayCon, ARTSPlayerState * PlayerState);

	/* @param NewRank - 0 indexed rank of the ability now */
	void OnAbilityRankAquired(int32 NewRank);

protected:

	void UIBinding_OnLMBPress();
	void UIBinding_OnLMBReleased();
	void UIBinding_OnRMBPress();
	void UIBinding_OnRMBReleased();

	//------------------------------------------------------
	//	Data
	//------------------------------------------------------

	URTSGameInstance * GI;

	/* Pointer to player controller... even though there's one on the slate button */
	TWeakObjectPtr<ARTSPlayerController> PC;

	FLinearColor OriginalColor;

	/** 
	 *	Color of node when it has been aquired. 
	 *	
	 *	My notes: Probably can put this on the tree widget instead 
	 *	since all nodes will likely be the same color when aquired 
	 */
	UPROPERTY(EditAnywhere, Category = "RTS")
	FLinearColor AquiredColor;

	// Pointer to the info struct this node is for
	const FCommanderAbilityTreeNodeInfo * NodeInfo;

	/* The same type of node in a tree is probably not allowed */
	UPROPERTY(EditAnywhere, Category = "RTS")
	ECommanderSkillTreeNodeType Type;

	/* The index in UCommanderSkillTreeWidget::AllNodes that this widget is at. 
	Note: I need to make sure this is the same across server/clients since we send this 
	as an RPC param to signal which node we're refering to */
	uint8 AllNodesArrayIndex;
};
