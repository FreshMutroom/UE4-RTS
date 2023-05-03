// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainMenuWidgetBase.generated.h"

class URTSGameInstance;
struct FMapInfo;


/* Rule for showing player start locations overtop of map image. Not really suitable to be 
here in this class but don't know of any better place */
UENUM()
enum class EPlayerStartDisplay
{
	Uninitialized   UMETA(Hidden),

	/** 
	 *	Player starts are not drawn overtop map image 
	 */
	Hidden,

	/** 
	 *	Player starts are drawn overtop map image but cannot be clicked on by players 
	 */
	VisibleButUnclickable,

	/**
	 *	Player starts are drawn overtop map image and can be clicked on by players to secure
	 *	where they will start in match
	 */
	VisibleAndClickable
};


/**
 *	Base class for menu widgets, not just the main menu widget
 */
UCLASS(Abstract)
class RTS_VER2_API UMainMenuWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:

	UMainMenuWidgetBase(const FObjectInitializer & ObjectInitializer);

protected:

	/* If you create a widget animation in editor and call it one of these they will be
	automatically played when the widget is hidden/shown. But it is important that the
	anims reverse what they do e.g. if the hide anim makes the widget slide off the screen
	then the show anim should make it slide back to it's desired position */
	static const FName ExitAnimationName;
	static const FName EnterAnimationName;
	/* Transition anim should be setup for exiting the screen. It will be played in reverse
	for entering the screen. It should not change the widgets visibility i.e. do not
	go from HitTestInvisible or SelfHitTestInvisible to Visible. */
	static const FName TransitionAnimationName;

	/* Reference to game instance */
	UPROPERTY()
	URTSGameInstance * GI;

	/* The animation to play when the widget is to being shown/hidden */
	UPROPERTY()
	UWidgetAnimation * TransitionAnimation;

	/* The animation to play when this widget is getting removed from view if TransitionAnimation
	is not set */
	UPROPERTY()
	UWidgetAnimation * ExitAnimation;

	/* The animation to play when the widget is to be shown if TransitionAnimation is not set */
	UPROPERTY()
	UWidgetAnimation * EnterAnimation;

	/* Set to true if Setup() has been called at least once */
	bool bHasBeenSetup;

	/* Just checks for null, but a widget passed into this should be one that isn't assigned
	by code but from being named correctly in editor */
	bool IsWidgetBound(UWidget * Widget) const;

	/* Get widget animations created in editor and assign them */
	void AssignAnimations();

	virtual void OnAnimationFinished_Implementation(const UWidgetAnimation * Animation) override;

#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;

	virtual void PostEditChangeChainProperty(struct FPropertyChangedChainEvent & PropertyChangedEvent) override;
#endif

private:

#if WITH_EDITORONLY_DATA

	/** 
	 *	Names of BindWidgetOptional widgets. Maps their name to their widget type. Here as a 
	 *	convenience to quickly know which widgets can be used with BindWidgetOptional. 
	 *	Maps the widget's name to the widget's class + description
	 */
	UPROPERTY(VisibleAnywhere, Category = "RTS")
	TMap < FString, FText > BindWidgetOptionalList;

#endif

public:

	/* Setup bindings, initial values etc. NativeConstruct is called every time widget is added
	to viewport so can't put stuff in there. N.B. UUserWidget::Initialize() may be what I was
	looking for instead of creating this.
	@return - true if has already been set up */
	virtual bool Setup();

	/* Called whenever changed to using ShowWidget or SomePreviousWidget, specifically at the
	start of PlayWidgetEnterAnim */
	virtual void OnShown();

	/* For widgets that display a map. Currently that's lobby and lobby creation screen only.
	Sets the map data to display. Really should be it's own interface instead of a function
	in this class */
	virtual void UpdateMapDisplay(const FMapInfo & MapInfo);

	/* Return the animation for when this widget leaves the screen */
	UWidgetAnimation * GetExitAnimation() const;
	/* Return the animation for when this widget enters the screen */
	UWidgetAnimation * GetEnterAnimation() const;

	/* Returns true if TransitionAnimation is used for both exit and enter animations */
	bool UsesOneTransitionAnimation() const;

	/**
	 *	Get the animation to be played when the widget is requested to be shown/hidden
	 */
	UWidgetAnimation * GetTransitionAnimation() const;
};
