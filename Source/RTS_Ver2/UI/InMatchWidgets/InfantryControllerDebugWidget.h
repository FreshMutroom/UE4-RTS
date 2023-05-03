// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InfantryControllerDebugWidget.generated.h"

class UTextBlock;
enum class EUnitState : uint8;
enum class EUnitAnimState : uint8;
class AInfantryController;


/**
 *	Widget to help debug AInfantryController. Has default implementation. 
 *	Ahhh never used this
 */
UCLASS()
class RTS_VER2_API UInfantryControllerDebugWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:

	virtual TSharedRef<SWidget> RebuildWidget() override;

public:

	virtual bool IsEditorOnly() const override { return true; }

	void SetInitialValues(AInfantryController * AIController);

	void OnUnitsStateChanged(EUnitState NewState);
	void OnUnitsAnimStateChanged(EUnitAnimState NewState);

protected:

	static bool IsWidgetBound(UWidget * Widget) { return Widget != nullptr; }

	//----------------------------------------------------------------
	//	Data
	//----------------------------------------------------------------

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_State;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_AnimState;

	/* How many MoveToLocation + MoveToActor calls didn't find a path */
	//UPROPERTY(meta = (BindWidgetOptional))
	//UTextBlock * Text_NumFailedMoves;
};
