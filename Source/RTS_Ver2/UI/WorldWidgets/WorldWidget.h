// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WorldWidget.generated.h"

class ISelectable;
class UProgressBar;
class UTextBlock;
struct FSelectableAttributesBasic;
class URTSGameInstance;


/**
 *	Widget that appears on a selectable in world
 */
UCLASS(Abstract)
class RTS_VER2_API UWorldWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UWorldWidget(const FObjectInitializer & ObjectInitializer);

	/* Initial setup of widget when selectable is created. This function assumes all upgrades 
	that should be applied on spawn have been applied */
	void Setup(ISelectable * OwningSelectable, float CurrentHealth, const FSelectableAttributesBasic & Attributes,
		URTSGameInstance * GameInst);

	/* Version that does not try set any health values. This function assumes all upgrades 
	that should be applied on spawn have been applied */
	void Setup(ISelectable * OwningSelectable, const FSelectableAttributesBasic & Attributes,
		URTSGameInstance * GameInst);

protected:

	virtual void NativeTick(const FGeometry & MyGeometry, float InDeltaTime) override;

	// Health bar
	UPROPERTY(meta = (BindWidgetOptional))
	UProgressBar * ProgressBar_Health;

	// Health as a number 
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_Health;

	// Progress bar for selectable resource. Selectable resource is something like mana
	UPROPERTY(meta = (BindWidgetOptional))
	UProgressBar * ProgressBar_SelectableResource;

	// Construction progress bar. Only really relevant for buildings
	UPROPERTY(meta = (BindWidgetOptional))
	UProgressBar * ProgressBar_ConstructionProgress;

	// Construction progess as a percentage. Does not show % sign
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_ConstructionProgress;

	// Attributes about the owning selectable
	float SelectableMaxHealth;

	/* Convert a health value to an FText so it can be displayed on the UI */
	static FText GetHealthText(float HealthValue);

	/** 
	 *	Convert a construction progress complete to FText so it can be displayed on the UI 
	 *	
	 *	@param NormalizedPercentageComplete - how far along construction is in the range of [0, 1] 
	*/
	static FText ConstructionProgressAsText(float NormalizedPercentageComplete);

	FORCEINLINE bool IsWidgetBound(UWidget * Widget) { return Widget != nullptr; }

private:

#if WITH_EDITORONLY_DATA
	/**
	 *	List of widgets that can be bound with BindWidgetOptional. Auto populated inside ctor.
	 *	Maps widget name to widget class
	 */
	UPROPERTY(VisibleAnywhere, Category = "RTS")
	TMap < FString, FString > BindWidgetOptionalList;
#endif

public:

	void OnHealthChanged(float NewHealthAmount);

	void OnZeroHealth();

	void OnSelectableResourceAmountChanged(int32 CurrentAmount, int32 MaxAmount);

	/* When a buildings construction progress changes
	@param NewPercentageComplete - how far along construction is in the range from 0 to 1 */
	void OnConstructionProgressChanged(float NewPercentageComplete);

	void OnConstructionComplete(float HealthAmount);

	void OnConstructionComplete();
};
