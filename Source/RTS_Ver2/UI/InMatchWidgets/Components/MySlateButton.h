// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
//#include "MySlateButton.generated.h"


struct FContextButtonInfo;
class ARTSPlayerController;
class UMyButton;
class UInGameWidgetBase;


/**
 *	This is my own implementation of a slate button. I created this class to allow for the 
 *	following:
 *	
 *	- make it able to respond to mouse right clicks, not just left clicks 
 *	- store a pointer to the local player controller to allow better handling of presses/release. 
 *
 *	Some other things I've done with this class:
 *	- removed any touch support for performance 
 *	- There is no option to configure the click type. It is DownAndUp. For performance.
 */
class RTS_VER2_API SMyButton : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SMyButton)
		: _Content()
		, _HAlign(HAlign_Fill)
		, _VAlign(VAlign_Fill)
		, _Padding(FMargin(2.0f))
		, _BorderImage(FCoreStyle::Get().GetBrush("Border"))
		, _ContentScale(FVector2D(1, 1))
		, _DesiredSizeScale(FVector2D(1, 1))
		, _ColorAndOpacity(FLinearColor(1, 1, 1, 1))
		, _BorderBackgroundColor(FLinearColor::Gray)
		, _ForegroundColor(FSlateColor::UseForeground())
	{}
	
		SLATE_DEFAULT_SLOT(FArguments, Content)

		SLATE_ARGUMENT(EHorizontalAlignment, HAlign)
		SLATE_ARGUMENT(EVerticalAlignment, VAlign)
		SLATE_ATTRIBUTE(FMargin, Padding)

		/* ------- Mouse events ------ */
		SLATE_EVENT(FSimpleDelegate, OnLeftMouseButtonPressed)
		SLATE_EVENT(FSimpleDelegate, OnRightMouseButtonPressed)
		SLATE_EVENT(FSimpleDelegate, OnLeftMouseButtonReleased)
		SLATE_EVENT(FSimpleDelegate, OnRightMouseButtonReleased)

		SLATE_ATTRIBUTE(const FSlateBrush *, BorderImage)

		SLATE_ATTRIBUTE(FVector2D, ContentScale)
		SLATE_ATTRIBUTE(FVector2D, DesiredSizeScale)
			
		/** ColorAndOpacity is the color and opacity of content in the border */
		SLATE_ATTRIBUTE(FLinearColor, ColorAndOpacity)
		/** BorderBackgroundColor refers to the actual color and opacity of the supplied border image.*/
		SLATE_ATTRIBUTE(FSlateColor, BorderBackgroundColor)

		/** The foreground color of text and some glyphs that appear as the border's content. */
		SLATE_ATTRIBUTE(FSlateColor, ForegroundColor)

	SLATE_END_ARGS()

	SMyButton();
			
	/* This function isn't virtual. I should check somehow if it's actually being called. 
	I think it gets called with template function */
	void Construct(const FArguments & InArgs);

	/**
	 *	Sets the content for this border
	 *
	 *	@param	InContent	The widget to use as content for the border
	 */
	void SetContent(TSharedRef< SWidget > InContent);

	/** Set the desired size scale multiplier */
	void SetDesiredSizeScale(const TAttribute<FVector2D>& InDesiredSizeScale);

	/** See HAlign argument */
	void SetHAlign(EHorizontalAlignment HAlign);

	/** See VAlign argument */
	void SetVAlign(EVerticalAlignment VAlign);

	/** See Padding attribute */
	void SetPadding(const TAttribute<FMargin>& InPadding);

	/** See BorderImage attribute */
	void SetNormalImageUPROPERTY(const TAttribute<const FSlateBrush *> & InBorderImage);

	void SetBackgroundColor(const TAttribute<FSlateColor> & InColorAndOpacity);

protected:

	// Begin SWidget overrides
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, 
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, 
		const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	/* SButton has interesting behavior: if you press on it with the LMB and then 
	press space or enter then it counts as clicking the button. That is definently behavior 
	I do not want so I have overridden these functions to remove that functionaility. */
	//virtual FReply OnKeyDown(const FGeometry & MyGeometry, const FKeyEvent & InKeyEvent) override;
	//virtual FReply OnKeyUp(const FGeometry & MyGeometry, const FKeyEvent & InKeyEvent) override;
	virtual bool SupportsKeyboardFocus() const override;
	virtual FReply OnMouseButtonDown(const FGeometry & MyGeometry, const FPointerEvent & MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry & MyGeometry, const FPointerEvent & MouseEvent) override;
	virtual FReply OnMouseButtonDoubleClick(const FGeometry & InMyGeometry, const FPointerEvent & InMouseEvent) override;
	virtual void OnMouseEnter(const FGeometry & MyGeometry, const FPointerEvent & MouseEvent) override;
	virtual void OnMouseLeave(const FPointerEvent & MouseEvent) override;
	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
	// End SWidget overrides

	// Just like SButton::GetBorder()
	const FSlateBrush * GetImage() const;

	void PlayHoverSound();
	void PlayPressedByLMBSound();
	void PlayPressedByRMBSound();

public: 

	FSimpleDelegate & GetOnLeftMouseButtonPressDelegate();
	FSimpleDelegate & GetOnRightMouseButtonPressDelegate();
	FSimpleDelegate & GetOnLeftMouseButtonReleasedDelegate();
	FSimpleDelegate & GetOnRightMouseButtonReleasedDelegate();

	void SetNormalImage(const FSlateBrush * InImage);
	void SetHoverImage(const FSlateBrush * InImage);
	void SetPressedImage(const FSlateBrush * InImage);
	void SetHighlightedImage(const FSlateBrush * InImage);
	void SetHoveredSound(USoundBase * InSound);
	void SetPressedByLMBSound(USoundBase * InSound);
	void SetPressedByRMBSound(USoundBase * InSound);
	void SetIgnoreHoveredImage(bool bIgnoreHoveredImage);
	void SetIgnorePressedImage(bool bIgnorePressedImage);
	void SetPC(ARTSPlayerController * InPlayerController);
	// Set the UMG widget that wraps around this
	void SetOwningWidget(UMyButton * InUMGWidget);
	// Different from SetOwningWidget
	void SetUserWidgetOwnerButton(UInGameWidgetBase * OwnerUserWidget);

	void SetIsPressedByLMB(bool bInValue);
	void SetIsPressedByRMB(bool bInValue);

	void SetIsHighlighted(bool bInValue);

	void ForceUnhover();

protected:

	//============================================================================
	//	Variables
	//============================================================================

	/** Brush resource that represents a button */
	const FSlateBrush * NormalImage;
	/** Brush resource that represents a button when it is hovered */
	const FSlateBrush * HoverImage;
	/** Brush resource that represents a button when it is pressed */
	const FSlateBrush * PressedImage;
	/** Brush resource that represents a button when it is highlighted */
	const FSlateBrush * HighlightedImage;

	/* I don't actually need the mouse position data with these events since I get it
	in the player controller code, but if I ever do then change these to FOnPointerEvent */
	FSimpleDelegate OnLeftMouseButtonPressed;
	FSimpleDelegate OnRightMouseButtonPressed;
	FSimpleDelegate OnLeftMouseButtonReleased;
	FSimpleDelegate OnRightMouseButtonReleased;

	/* The sound to play when the button is hovered */
	USoundBase * HoveredSound;

	/* The sound to play when the button is pressed by the LMB */
	USoundBase * PressedByLMBSound;
	
	/* The sound to play when the button is pressed by the RMB */
	USoundBase * PressedByRMBSound;

	/** 
	 *	A time when a button is 'highlighted' is say when you press the button/hotkey for 
	 *	an ability but then you still have to choose a target. During that time the button 
	 *	is called 'highlighted'
	 */
	uint8 bIsHighlighted : 1;

	/** True if this button is currently in a pressed state */
	uint8 bIsPressedByLMB : 1;
	uint8 bIsPressedByRMB : 1;

	/* If true then never use the image even if it is valid */
	uint8 bUsingUnifiedHoverImage : 1;
	uint8 bUsingUnifiedPressedImage : 1;

	/* Local player controller */
	ARTSPlayerController * PC;

	/* The UMG widget that this widget belongs to */
	UMyButton * UMGWidget;

	UInGameWidgetBase * UserWidgetButtonOwner;

	TAttribute<const FSlateBrush*> BorderImage;
	TAttribute<FVector2D> DesiredSizeScale;
	TAttribute<FSlateColor> BorderBackgroundColor;

public:

	/* Plays a sound similar to how FSlateApplication plays one except it takes a USoundBase 
	ptr instead of a FSlateSound. 
	Doesn't really belong in this class - any widget can call it */
	static void PlaySound(USoundBase * Sound);
};
