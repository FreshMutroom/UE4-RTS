// Fill out your copyright notice in the Description page of Project Settings.

#include "WorldWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

#include "GameFramework/Selectable.h"
#include "Statics/Structs_1.h"
#include "Statics/DevelopmentStatics.h"
#include "GameFramework/RTSGameInstance.h"


UWorldWidget::UWorldWidget(const FObjectInitializer & ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITORONLY_DATA

	/* Populate the BindWidgetOptionalList.
	Note: if getting crashes here try moving logic to child ctor instead */
	for (TFieldIterator<UProperty> PropIt(GetClass()); PropIt; ++PropIt)
	{
		if (PropIt->HasMetaData("BindWidgetOptional"))
		{
			BindWidgetOptionalList.Emplace(PropIt->GetName(), PropIt->GetCPPTypeForwardDeclaration());
		}
	}

#endif
}

void UWorldWidget::Setup(ISelectable * OwningSelectable, float CurrentHealth, const FSelectableAttributesBasic & Attributes, 
	URTSGameInstance * GameInst)
{
	SelectableMaxHealth = Attributes.GetMaxHealth();

	// To set inital progress bar percent and text value
	OnHealthChanged(CurrentHealth);

	if (IsWidgetBound(ProgressBar_SelectableResource))
	{
		if (Attributes.HasASelectableResource())
		{
			/* Will we get a flash of the PBar at the wrong percent and color? should we start it 
			as hidden, set its color and percent then make it visible? Same deal with the other 
			Setup function too */
			
			// Set color
			const FSelectableResourceColorInfo & ResourceInfo = GameInst->GetSelectableResourceInfo(Attributes.GetSelectableResource_1().GetType());
			ProgressBar_SelectableResource->SetFillColorAndOpacity(ResourceInfo.GetPBarFillColor());
			
			// Set initial value of mana PBar
			ProgressBar_SelectableResource->SetPercent((float)Attributes.GetSelectableResource_1().GetAmount() / Attributes.GetSelectableResource_1().GetMaxAmount());
		}
		else
		{
			// Hide bar if selectable does not use a selectable resource
			ProgressBar_SelectableResource->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void UWorldWidget::Setup(ISelectable * OwningSelectable, const FSelectableAttributesBasic & Attributes,
	URTSGameInstance * GameInst)
{
	SelectableMaxHealth = Attributes.GetMaxHealth();

	if (IsWidgetBound(ProgressBar_SelectableResource))
	{
		if (Attributes.HasASelectableResource())
		{
			// Set color
			const FSelectableResourceColorInfo & ResourceInfo = GameInst->GetSelectableResourceInfo(Attributes.GetSelectableResource_1().GetType());
			ProgressBar_SelectableResource->SetFillColorAndOpacity(ResourceInfo.GetPBarFillColor());

			// Set initial value of mana PBar
			ProgressBar_SelectableResource->SetPercent((float)Attributes.GetSelectableResource_1().GetAmount() / Attributes.GetSelectableResource_1().GetMaxAmount());
		}
		else
		{
			// Hide bar if selectable does not use a selectable resource
			ProgressBar_SelectableResource->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void UWorldWidget::NativeTick(const FGeometry & MyGeometry, float InDeltaTime)
{
	/* Super call purposly left out because widget anims not forseen to be used */
	GInitRunaway();
}

FText UWorldWidget::GetHealthText(float HealthValue)
{
	FloatToText(HealthValue, UIOptions::CurrentHealthDisplayMethod);
}

FText UWorldWidget::ConstructionProgressAsText(float NormalizedPercentageComplete)
{
	// Maybe try FText::AsPercent instead of multiplying by 100?
	FloatToText(NormalizedPercentageComplete * 100.f, UIOptions::ConstructionProgressDisplayMethod);
}

void UWorldWidget::OnHealthChanged(float NewHealthAmount)
{
#if !UE_BUILD_SHIPPING
	
	/* If calling this and selectable has reached zero health then should call OnZeroHealthReached 
	instead. Haven't actually implemented that zero health function yet though. It will likely 
	just set progress bar percent to 0, set the text value to 0 or hide it or whatever. */
	if (IsWidgetBound(ProgressBar_Health) || IsWidgetBound(Text_Health))
	{
		UE_CLOG(NewHealthAmount <= 0.f, RTSLOG, Fatal, TEXT("Passing in a health value equal to "
			"or less than zero (%.3f). This is not allowed. Use alternate function. If selectable does not "
			"have health as an attribute then use a different widget without ProgressBar_Health and "
			"Text_Health bound"), NewHealthAmount);
	}

#endif
	
	if (IsWidgetBound(ProgressBar_Health))
	{
		ProgressBar_Health->SetPercent(NewHealthAmount / SelectableMaxHealth);
	}
	if (IsWidgetBound(Text_Health))
	{
		Text_Health->SetText(GetHealthText(NewHealthAmount));
	}
}

void UWorldWidget::OnZeroHealth()
{
	if (IsWidgetBound(ProgressBar_Health))
	{
		ProgressBar_Health->SetPercent(0.f);
	}
	if (IsWidgetBound(Text_Health))
	{
		Text_Health->SetText(GetHealthText(0.f));
	}
}

void UWorldWidget::OnSelectableResourceAmountChanged(int32 CurrentAmount, int32 MaxAmount)
{
	if (IsWidgetBound(ProgressBar_SelectableResource))
	{
		ProgressBar_SelectableResource->SetPercent((float)CurrentAmount / MaxAmount);
	}
}

void UWorldWidget::OnConstructionProgressChanged(float NewPercentageComplete)
{
	if (IsWidgetBound(ProgressBar_ConstructionProgress))
	{
		ProgressBar_ConstructionProgress->SetPercent(NewPercentageComplete);
	}
	if (IsWidgetBound(Text_ConstructionProgress))
	{
		Text_ConstructionProgress->SetText(ConstructionProgressAsText(NewPercentageComplete));
	}
}

void UWorldWidget::OnConstructionComplete(float HealthAmount)
{
	OnHealthChanged(HealthAmount);

	// Make construction progress related widgets invisible
	if (IsWidgetBound(ProgressBar_ConstructionProgress))
	{
		ProgressBar_ConstructionProgress->SetVisibility(ESlateVisibility::Hidden);
	}
	if (IsWidgetBound(Text_ConstructionProgress))
	{
		Text_ConstructionProgress->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UWorldWidget::OnConstructionComplete()
{
	// Make construction progress related widgets invisible
	if (IsWidgetBound(ProgressBar_ConstructionProgress))
	{
		ProgressBar_ConstructionProgress->SetVisibility(ESlateVisibility::Hidden);
	}
	if (IsWidgetBound(Text_ConstructionProgress))
	{
		Text_ConstructionProgress->SetVisibility(ESlateVisibility::Hidden);
	}
}
