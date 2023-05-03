// Fill out your copyright notice in the Description page of Project Settings.

#include "MatchRulesWidget.h"
#include "Components/ComboBoxString.h"
#include "Components/TextBlock.h"

#include "GameFramework/RTSGameInstance.h"
#include "UI/MainMenu/Menus.h" // Just for blank text I think
#include "GameFramework/RTSGameState.h"
#include "Statics/Statics.h"
#include "Statics/DevelopmentStatics.h"

//-------------------------------------------------
//	Match Rules Widget
//-------------------------------------------------

void UMatchRulesWidget::SetIsForLobby(bool bNewValue)
{
	bIsForLobby = bNewValue;
}

bool UMatchRulesWidget::Setup()
{
	if (Super::Setup())
	{
		return true;
	}

	if (IsWidgetBound(ComboBox_StartingResources))
	{
		/* Setup combo box options. Here we sort the TMap entries so that the combo box options
		will appear in the order values are defined in the UENUM */
		TArray < EStartingResourceAmount > ToSort;
		ToSort.Reserve(GI->GetAllStartingResourceConfigs().Num());
		for (const auto & Elem : GI->GetAllStartingResourceConfigs())
		{
			ToSort.Emplace(Elem.Key);
		}
		ToSort.Sort();
		for (int32 i = 0; i < ToSort.Num(); ++i)
		{
			ComboBox_StartingResources->AddOption(GI->GetStartingResourceConfig(ToSort[i]).GetName().ToString());
		}

		/* Do not set initial selected option. Let game state do that */

		ComboBox_StartingResources->OnSelectionChanged.AddDynamic(this,
			&UMatchRulesWidget::UIBinding_OnStartingResourcesSelectionChanged);
	}

	if (IsWidgetBound(Text_StartingResources))
	{
		Text_StartingResources->SetText(Menus::BLANK_TEXT);
	}

	if (IsWidgetBound(ComboBox_DefeatCondition))
	{
		/* Setup combo box options. Here we sort the TMap entries so that the combo box options
		will appear in the order values are defined in the UENUM */
		TArray < EDefeatCondition > ToSort;
		ToSort.Reserve(GI->GetAllDefeatConditions().Num());
		for (const auto & Elem : GI->GetAllDefeatConditions())
		{
			ToSort.Emplace(Elem.Key);
		}
		// Sort them so they will appear in combo box in the order they appear in UENUM
		ToSort.Sort();
		for (int32 i = 0; i < ToSort.Num(); ++i)
		{
			/* Behavior of this func is duplicates will be added which is what we want even if it
			is confusing */
			ComboBox_DefeatCondition->AddOption(GI->GetDefeatConditionInfo(ToSort[i]).GetName().ToString());
		}

		/* Do not set an initial option. Game state should set one via SetupSingleplayerLobby or
		SetupNetworkedLobby */

		ComboBox_DefeatCondition->OnSelectionChanged.AddDynamic(this,
			&UMatchRulesWidget::UIBinding_OnDefeatConditionSelectionChanged);
	}

	if (IsWidgetBound(Text_DefeatCondition))
	{
		/* Do this because... don't know, I guess don't want stuff typed in editor to be there and
		give false impression that option has been set in game state */
		Text_DefeatCondition->SetText(Menus::BLANK_TEXT);
	}

	return false;
}

bool UMatchRulesWidget::IsForLobby() const
{
	return bIsForLobby;
}

void UMatchRulesWidget::UIBinding_OnStartingResourcesSelectionChanged(FString Item, ESelectInfo::Type SelectionType)
{
	if (IsWidgetBound(Text_StartingResources))
	{
		Text_StartingResources->SetText(FText::FromString(Item));
	}

	if (IsForLobby())
	{
		EStartingResourceAmount ResourceAmount = EStartingResourceAmount::Default;

		for (const auto & Elem : GI->GetAllStartingResourceConfigs())
		{
			if (Elem.Value.GetName().ToString() == Item)
			{
				ResourceAmount = Elem.Key;
				break;
			}
		}

#if !UE_BUILD_SHIPPING
		if (ResourceAmount == EStartingResourceAmount::Default)
		{
			UE_LOG(RTSLOG, Fatal, TEXT("No starting reousrce config with name %s is defined in game "
				"instance"), *Item);
		}
#endif

		// Notify game state so update can relicate 
		ARTSGameState * GameState = CastChecked<ARTSGameState>(GetWorld()->GetGameState());
		GameState->ChangeStartingResourcesInLobby(ResourceAmount);
	}
}

void UMatchRulesWidget::UIBinding_OnDefeatConditionSelectionChanged(FString Item, ESelectInfo::Type SelectionType)
{
	if (IsWidgetBound(Text_DefeatCondition))
	{
		Text_DefeatCondition->SetText(FText::FromString(Item));
	}

	// Only need to notify game state and replicate changes if this widget is in lobby
	if (IsForLobby())
	{
		EDefeatCondition NewCondish = EDefeatCondition::None;

		// Figure out what defeat condition string corrisponds to relatively slowly 
		for (const auto & Elem : GI->GetAllDefeatConditions())
		{
			if (Elem.Value.GetName().ToString() == Item)
			{
				NewCondish = Elem.Key;
				break;
			}
		}

#if !UE_BUILD_SHIPPING
		if (NewCondish == EDefeatCondition::None)
		{
			UE_LOG(RTSLOG, Fatal, TEXT("No victory condition with name %s is defined in game "
				"instance"), *Item);
		}
#endif 

		// Notify game state so update can relicate 
		ARTSGameState * GameState = CastChecked<ARTSGameState>(GetWorld()->GetGameState());
		GameState->ChangeDefeatConditionInLobby(NewCondish);
	}
}

EStartingResourceAmount UMatchRulesWidget::GetStartingResources() const
{
	// Try combo box first
	if (IsWidgetBound(ComboBox_StartingResources))
	{
		for (const auto & Elem : GI->GetAllStartingResourceConfigs())
		{
			if (Elem.Value.GetName().ToString() == ComboBox_StartingResources->GetSelectedOption())
			{
				return Elem.Key;
			}
		}
	}

	// Try text block now
	if (IsWidgetBound(Text_StartingResources))
	{
		for (const auto & Elem : GI->GetAllStartingResourceConfigs())
		{
			if (Elem.Value.GetName().EqualTo(Text_StartingResources->GetText()))
			{
				return Elem.Key;
			}
		}
	}

#if !UE_BUILD_SHIPPING
	if (!IsWidgetBound(ComboBox_StartingResources) && !IsWidgetBound(Text_StartingResources))
	{
		UE_LOG(RTSLOG, Fatal, TEXT("Combo box and text block are not bound - at least 1 needs to be"));
	}
	else
	{
		UE_LOG(RTSLOG, Fatal, TEXT("Text displyed by combo box and text block does not match a "
			"display name defined in game instance"));
	}
#endif	

	return EStartingResourceAmount::Default;
}

void UMatchRulesWidget::SetStartingResources(EStartingResourceAmount NewAmount)
{
	const FStartingResourceConfig & StartingResourceInfo = GI->GetStartingResourceConfig(NewAmount);

	if (IsWidgetBound(ComboBox_StartingResources))
	{
		ComboBox_StartingResources->SetSelectedOption(StartingResourceInfo.GetName().ToString());
	}

	if (IsWidgetBound(Text_StartingResources))
	{
		Text_StartingResources->SetText(StartingResourceInfo.GetName());
	}
}

EDefeatCondition UMatchRulesWidget::GetDefeatCondition() const
{
	if (IsWidgetBound(ComboBox_DefeatCondition))
	{
		// Relatively slow lookup here
		for (const auto & Elem : GI->GetAllDefeatConditions())
		{
			if (Elem.Value.GetName().ToString() == ComboBox_DefeatCondition->GetSelectedOption())
			{
				// Found desired entry
				return Elem.Key;
			}
		}
	}

	// Try TextBlock now
	if (IsWidgetBound(Text_DefeatCondition))
	{
		for (const auto & Elem : GI->GetAllDefeatConditions())
		{
			if (Elem.Value.GetName().EqualTo(Text_DefeatCondition->GetText()))
			{
				return Elem.Key;
			}
		}
	}

#if !UE_BUILD_SHIPPING
	if (!IsWidgetBound(ComboBox_DefeatCondition) && !IsWidgetBound(Text_DefeatCondition))
	{
		UE_LOG(RTSLOG, Fatal, TEXT("Combo box and text block are not bound - at least 1 needs to be"));
	}
	else
	{
		UE_LOG(RTSLOG, Fatal, TEXT("Name displayed in combo box or text block does not match any "
			"names defined in game instance"));
	}
#endif

	return EDefeatCondition::None;
}

void UMatchRulesWidget::SetDefeatCondition(EDefeatCondition NewCondition)
{
	const FDefeatConditionInfo & ConditionInfo = GI->GetDefeatConditionInfo(NewCondition);

	if (IsWidgetBound(ComboBox_DefeatCondition))
	{
		ComboBox_DefeatCondition->SetSelectedOption(ConditionInfo.GetName().ToString());
	}

	/* Update text box also */
	if (IsWidgetBound(Text_DefeatCondition))
	{
		Text_DefeatCondition->SetText(ConditionInfo.GetName());
	}
}
