// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BuffAndDebuffWidgets.generated.h"

class UTextBlock; 
class UImage;
class UProgressBar;
struct FTickableBuffOrDebuffInstanceInfo;
struct FStaticBuffOrDebuffInstanceInfo;
class UBorder;
class URTSGameInstance;


/** 
 *	A widget to display a single buff or debuff that does not need to show any duration. 
 *
 *	TODO disable tick and make add a array to display the bindwidgetoptionals in editor 
 */
UCLASS(Abstract)
class RTS_VER2_API USingleBuffOrDebuffWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	USingleBuffOrDebuffWidget(const FObjectInitializer & ObjectInitializer);

private:

	void PopulateBindWidgetOptionalList();

protected:

	/* Text to display the name of the buff/debuff */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_Name;

	/* Image to display the icon for the buff/debuff */
	UPROPERTY(meta = (BindWidgetOptional))
	UImage * Image_Icon;

	/** 
	 *	A border that will display a certain color depending on the subtype of the buff/debuff. 
	 *	e.g. in a certain MMO magic buffs/debuffs have blue borders, curses have purple borders. 
	 *	
	 *	Users can just incorperate the border color into the image set on Image_Icon effectively 
	 *	making this irrelevant, but I'll keep it here anyway just in case. 
	 *	
	 *	I really have no idea what the difference between the border's Brush Tint and Brush Color 
	 *	is.
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	UBorder * Border_SubType;

	/* Given a time return the text that should be shown on UI for that time */
	static FText GetDisplayTextForDuration(float Duration);

	/* Just like in UInGameWidgetBase */
	static bool IsWidgetBound(UWidget * InWidget);

#if WITH_EDITORONLY_DATA

	/**
	 *	List of widgets that can be bound with BindWidgetOptional. Auto populated inside ctor.
	 *	Maps widget name to widget class + it's description
	 */
	UPROPERTY(VisibleAnywhere, Category = "RTS")
	TMap < FString, FText > BindWidgetOptionalList;

#endif

public:

	/* Set what text and image to show  */
	void SetupFor(const FStaticBuffOrDebuffInstanceInfo & StateInfo, URTSGameInstance * GameInst);
};


/** 
 *	A widget to display a single buff or debuff that would like to show its duration 
 */
UCLASS(Abstract)
class RTS_VER2_API USingleTickableBuffOrDebuffWidget : public USingleBuffOrDebuffWidget
{
	GENERATED_BODY()

protected:

	/* Text to show the remaining duration as a number */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_DurationRemaining;

	/* Progress bar to show the duration remaining in progress bar form */
	UPROPERTY(meta = (BindWidgetOptional))
	UProgressBar * ProgressBar_PercentageRemaining;

	/* How long buff/debuff takes to complete from start to finish, or 0 if infinite in 
	duration */
	float TotalDuration;

public:

	/** 
	 *	Setup the widget to display info about a buff/debuff applied
	 *
	 *	@param Info - state info struct of the buff/debuff that is applied 
	 */
	void SetupFor(const FTickableBuffOrDebuffInstanceInfo & Info, URTSGameInstance * GameInst);

	/**
	 *	Adjust the appearance of the widget to reflect the duration remaining before the
	 *	buff/debuff falls off. This only needs to be called for buffs/debuffs that no not have 
	 *	an infinite duration
	 *
	 *	@param DurationRemaining - how long buff/debuff has remaining before it expires
	 */
	void SetDurationRemaining(float DurationRemaining);
};
