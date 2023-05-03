// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "SelectableWidgetComponent.generated.h"

class UWorldWidget;
class ISelectable;
struct FSelectableAttributesBasic;
struct FSelectableResourceColorInfo;
class URTSGameInstance;


/**
 *	Widget component that shows information about a selectable. It should not have a user
 *	widget assigned but instead the faction info will set one
 */
UCLASS()
class RTS_VER2_API USelectableWidgetComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:

	USelectableWidgetComponent();

	// Overridden to do nothing
	virtual void InitWidget() override;

	/* Sets the owner and spawns the appropriate user widget */
	void SetWidgetClassAndSpawn(ISelectable * SelectableOwner, const TSubclassOf < UWorldWidget > & InClass,
		float CurrentHealth, const FSelectableAttributesBasic & Attributes, URTSGameInstance * GameInst);

	/* Version that does not set a health value. 
	Put this here since I changed SetWidgetClassAndSpawn (with health param) to not allow values 
	<= 0.f. I'm just using this as a workaround to get project going again. If issues rise from 
	initial health values not being correct then will need to sort that out */
	void SetWidgetClassAndSpawn(ISelectable * SelectableOwner, const TSubclassOf < UWorldWidget > & InClass, 
		const FSelectableAttributesBasic & Attributes, URTSGameInstance * GameInst);

protected:

	/* Pointer to the user widget this component has*/
	UPROPERTY()
	UWorldWidget * SelectableWidget;

public:

	/* Called by owning selectable when its health changes */
	void OnHealthChanged(float NewHealthAmount);

	/* Called by onwing selectable when it reaches zero health */
	void OnZeroHealth();

	/* Call when selectable resource's either current or max amount changes */
	void OnSelectableResourceAmountChanged(int32 CurrentAmount, int32 MaxAmount);

	/* Call by owning selectable when building construction progresses
	@param NewPercentageComplete - percentage complete in the range 0 to 1 */
	void OnConstructionProgressChanged(float NewPercentageComplete);

	void OnConstructionComplete(float HealthAmount);

	void OnConstructionComplete();
};

// Overide OnRegister to not call InitWidget - don't want to spawn user widget if user set 
// one in editor

// TODO need to basically get widget BP from FI then spawn that and assign it to SelectableWidget.
// Then when selectable takes damage etc call a func on this which will in turn update the 
// user widget is not null

// CLeanup Ranged inf code in general.
// Change ranged inf ctor default value setting of widget component from there to this class
