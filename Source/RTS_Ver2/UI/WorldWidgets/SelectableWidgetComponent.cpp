// Fill out your copyright notice in the Description page of Project Settings.

#include "SelectableWidgetComponent.h"

#include "UI/WorldWidgets/WorldWidget.h"

USelectableWidgetComponent::USelectableWidgetComponent()
{
	SetCollisionProfileName(FName("NoCollision"));
	SetGenerateOverlapEvents(false);
	SetCanEverAffectNavigation(false);
	SetVisibility(true);
	SetWidgetSpace(EWidgetSpace::Screen);
	bReceivesDecals = false;
	CastShadow = false;
	CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
}

void USelectableWidgetComponent::InitWidget()
{
	/* Null widget if set in editor. Will not use that. Will use one from faction info when
	owning selectable's faction is known */
	Widget = nullptr;
}

void USelectableWidgetComponent::SetWidgetClassAndSpawn(ISelectable * SelectableOwner, const TSubclassOf<UWorldWidget>& InClass,
	float CurrentHealth, const FSelectableAttributesBasic & Attributes, URTSGameInstance * GameInst)
{
	/* Only bother doing all this if the class is not null i.e. will actually draw widget */
	if (InClass != nullptr)
	{
		WidgetClass = InClass;

		// Spawn widget
		SelectableWidget = CreateWidget<UWorldWidget>(GetWorld(), InClass);

		// Setup its properties
		SelectableWidget->Setup(SelectableOwner, CurrentHealth, Attributes, GameInst);

		// Show widget
		SetWidget(SelectableWidget);
	}
}

void USelectableWidgetComponent::SetWidgetClassAndSpawn(ISelectable * SelectableOwner, const TSubclassOf<UWorldWidget>& InClass, 
	const FSelectableAttributesBasic & Attributes, URTSGameInstance * GameInst)
{
	if (InClass != nullptr)
	{
		WidgetClass = InClass;

		// Spawn widget
		SelectableWidget = CreateWidget<UWorldWidget>(GetWorld(), InClass);

		// Setup its properties
		SelectableWidget->Setup(SelectableOwner, Attributes, GameInst);

		// Show widget
		SetWidget(SelectableWidget);
	}
}

void USelectableWidgetComponent::OnHealthChanged(float NewHealthAmount)
{
	if (SelectableWidget != nullptr)
	{
		// Bubble this to user widget
		SelectableWidget->OnHealthChanged(NewHealthAmount);
	}
}

void USelectableWidgetComponent::OnZeroHealth()
{
	/* Alternatively instead of updating the health PBar and text we may just want to make 
	the widget hidden */
	
	if (SelectableWidget != nullptr)
	{
		SelectableWidget->OnZeroHealth();
	}
}

void USelectableWidgetComponent::OnSelectableResourceAmountChanged(int32 CurrentAmount, int32 MaxAmount)
{
	if (SelectableWidget != nullptr)
	{
		SelectableWidget->OnSelectableResourceAmountChanged(CurrentAmount, MaxAmount);
	}
}

void USelectableWidgetComponent::OnConstructionProgressChanged(float NewPercentageComplete)
{
	if (SelectableWidget != nullptr)
	{
		SelectableWidget->OnConstructionProgressChanged(NewPercentageComplete);
	}
}

void USelectableWidgetComponent::OnConstructionComplete(float HealthAmount)
{
	if (SelectableWidget != nullptr)
	{
		SelectableWidget->OnConstructionComplete(HealthAmount);
	}
}

void USelectableWidgetComponent::OnConstructionComplete()
{
	if (SelectableWidget != nullptr)
	{
		SelectableWidget->OnConstructionComplete();
	}
}
