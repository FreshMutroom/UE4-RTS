// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PopupWidget.generated.h"

class UTextBlock;


UENUM()
enum class EWidgetAnimStatus : uint8
{
	NoAnims,

	ShowAnimOnly,

	HideAnimOnly,

	ShowAndHideAnims,

	SingleAnim
};


/**
 *	Popup widgets are widgets that get shown briefly on screen to convey a message. 
 *	e.g. "Not enough players in lobby". They are not menus and have no interactibility. 
 *	They will become hidden anytime the menu widget is changed.
 *
 *	Widget anim breakdown:
 *	All widget anims are optional. 
 *	- If no anims are set then widget will be shown/hidden instantly. It will be shown for 
 *	the duration param of func Show().
 *	- If ShowAnim and/or HideAnim are set then they will play when the widget is requested to 
 *	be shown/hidden. Duration param in func Show() times from the point where ShowAnim finishes.
 *	- If SingleAnim is set then ShowAnim and HideAnim are both ignored. Duration param in func 
 *	Show() is also ignored. This SingleAnim will play when the widget is requested to be shown 
 *	and is repsonsible for making the widget go away eventaully 
 */
UCLASS(Abstract)
class RTS_VER2_API UPopupWidget : public UUserWidget
{
	GENERATED_BODY()

	static const FString ShowAnimFullName;
	static const FString HideAnimFullName;
	static const FString SingleAnimFullName;

public:

	UPopupWidget(const FObjectInitializer & ObjectInitializer);

	/* To be called when this widget is created */
	void Init();

protected:

	void AssignWidgetAnims();

	// The text box that shows the message
	UPROPERTY(meta = (BindWidget))
	UTextBlock * Text_Message;

	/* The color of Text_Message that was set in editor */
	FSlateColor OriginalTextColor;

#if WITH_EDITORONLY_DATA
	/* Names you can call widget anims to have them play at certain times */
	UPROPERTY(VisibleAnywhere, Category = "RTS")
	TArray < FString > AnimNames;
#endif

	/* Only relevant if using either ShowAnim or SingleAnim. Time in anim to never go below 
	when requested to play anim while anim is already playing */
	UPROPERTY(EditAnywhere, Category = "RTS", meta = (ClampMin = 0))
	float AnimNoReplayTime;

	/* Widget animation to play when showing widget */
	UPROPERTY()
	UWidgetAnimation * ShowAnim;

	/* Widget anim to play when hiding widget */
	UPROPERTY()
	UWidgetAnimation * HideAnim;

	/* One anim to play for both showing and hiding widget. It is responsible for making 
	sure widget hides eventually. With this duration in func Show is irrelevant */
	UPROPERTY()
	UWidgetAnimation * SingleAnim;

	/* How many animations this widget has. Calculated during AssignWidgetAnims() */
	EWidgetAnimStatus AnimStatus;

	/* Duration to set for TimerHandle_Hide when an animation finishes */
	float ShowDuration;

	/* Timer handle to say when to hide widget */
	FTimerHandle TimerHandle_Hide;

	/* Call this when it is time to hide the widget */
	void TimeToHide();

	void HideInternal();

	/* Call function that returns void after delay
	@param TimerHandle - timer handle to use
	@param Function - function to call
	@param Delay - delay before calling function */
	void Delay(FTimerHandle & TimerHandle, void(UPopupWidget:: *Function)(), float Delay);

public:

	virtual void OnAnimationFinished_Implementation(const UWidgetAnimation * Animation) override;

	/** 
	 *	Show and set what message should be displayed 
	 *
	 *	@param Message - the text to display
	 *	@param Duration - how long to show widget for. Specifically: if using a show anim then 
	 *	this is the time from when it finishes.
	 *	@param bSetTextColor - if true 3rd param will define color of text. If false text color 
	 *	will be whatever color it was set to in editor 
	 *	@param TextColor - color of text if bSetTextColor is true
	 */
	void Show(const FText & Message, float Duration, bool bSetTextColor, 
		const FLinearColor & TextColor);

	/* What game instance calls when it wants this widget to hide */
	void Hide();
};
