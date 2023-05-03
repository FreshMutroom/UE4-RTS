// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/PanelSlot.h"
#include "MyButtonSlot.generated.h"

class SMyButton;


/**
 *	Slot for UMyButton class.
 */
UCLASS()
class RTS_VER2_API UMyButtonSlot : public UPanelSlot
{
	GENERATED_BODY()
	
	/* Pretty much all of this is just copied from UBorderSlot.h */

public:

	UMyButtonSlot(const FObjectInitializer & ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Layout|Border Slot")
	void SetPadding(FMargin InPadding);

	UFUNCTION(BlueprintCallable, Category = "Layout|Border Slot")
	void SetHorizontalAlignment(EHorizontalAlignment InHorizontalAlignment);

	UFUNCTION(BlueprintCallable, Category = "Layout|Border Slot")
	void SetVerticalAlignment(EVerticalAlignment InVerticalAlignment);

protected:
	/** The padding area between the slot and the content it contains. */
	UPROPERTY(EditAnywhere, Category = "Layout|Border Slot")
	FMargin Padding;

	/** The alignment of the object horizontally. */
	UPROPERTY(EditAnywhere, Category = "Layout|Border Slot")
	TEnumAsByte<EHorizontalAlignment> HorizontalAlignment;

	/** The alignment of the object vertically. */
	UPROPERTY(EditAnywhere, Category = "Layout|Border Slot")
	TEnumAsByte<EVerticalAlignment> VerticalAlignment;

public:

	// UPanelSlot interface
	virtual void SynchronizeProperties() override;
	// End of UPanelSlot interface

	/** Builds the underlying slot for the slate border. */
	void BuildSlot(TSharedRef<SMyButton> InButton);

	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

public:

#if WITH_EDITOR

	// UObject interface
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	// End of UObject interface

#endif

private:

	/** A pointer to the button to allow us to adjust the size, padding...etc at runtime. */
	TSharedPtr<SMyButton> Button;
};
