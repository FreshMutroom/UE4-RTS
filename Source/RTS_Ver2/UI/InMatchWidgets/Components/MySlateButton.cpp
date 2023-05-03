// Fill out your copyright notice in the Description page of Project Settings.

#include "MySlateButton.h"
#include "Public/SlateOptMacros.h"
#include "Public/AudioDevice.h"
#include "Public/ActiveSound.h"

#include "GameFramework/RTSPlayerController.h"


#define LOCTEXT_NAMESPACE "SMyButton"

// Dunno if this is needed, just copying source
static FName SMyButtonTypeName("SMyButton");

SMyButton::SMyButton() 
	// These null/false assignments need to happen otherwise they are not automatically done
	: NormalImage(nullptr) 
	, HoverImage(nullptr) 
	, PressedImage(nullptr) 
	, HoveredSound(nullptr) 
	, PressedByLMBSound(nullptr) 
	, PressedByRMBSound(nullptr)
	, bIsHighlighted(false)
	, bIsPressedByLMB(false)
	, bIsPressedByRMB(false) 
	, bUsingUnifiedHoverImage(false) 
	, bUsingUnifiedPressedImage(false)
	, PC(nullptr) 
	, UMGWidget(nullptr)
	, UserWidgetButtonOwner(nullptr)
	, BorderImage(FCoreStyle::Get().GetBrush("Border"))
	, DesiredSizeScale(FVector2D(1, 1))
	, BorderBackgroundColor(FLinearColor::White)
{
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION // Auto-generated code does this
void SMyButton::Construct(const FArguments & InArgs)
{
	/* Not sure I have this function right. And is SBorder::Construct called too by this? */
	
	if (GetType() == SMyButtonTypeName)
	{
		SetCanTick(false);
	}
	
	ContentScale = InArgs._ContentScale;
	DesiredSizeScale = InArgs._DesiredSizeScale;
	ColorAndOpacity = InArgs._ColorAndOpacity;
	BorderBackgroundColor = InArgs._BorderBackgroundColor;
	ForegroundColor = InArgs._ForegroundColor;

	OnLeftMouseButtonPressed = InArgs._OnLeftMouseButtonPressed;
	OnLeftMouseButtonReleased = InArgs._OnLeftMouseButtonReleased;
	OnRightMouseButtonPressed = InArgs._OnRightMouseButtonPressed;
	OnRightMouseButtonReleased = InArgs._OnRightMouseButtonReleased;

	//NormalImage = InArgs._NormalImage;

	ChildSlot
	.HAlign(InArgs._HAlign)
	.VAlign(InArgs._VAlign)
	.Padding(InArgs._Padding)
	[
		InArgs._Content.Widget
	];
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SMyButton::SetContent(TSharedRef<SWidget> InContent)
{
	ChildSlot
	[
		InContent
	];
}

void SMyButton::SetDesiredSizeScale(const TAttribute<FVector2D>& InDesiredSizeScale)
{
	if (!DesiredSizeScale.IdenticalTo(InDesiredSizeScale))
	{
		DesiredSizeScale = InDesiredSizeScale;
		Invalidate(EInvalidateWidget::LayoutAndVolatility);
	}
}

void SMyButton::SetHAlign(EHorizontalAlignment HAlign)
{
	if (ChildSlot.HAlignment != HAlign)
	{
		ChildSlot.HAlignment = HAlign;
		Invalidate(EInvalidateWidget::Layout);
	}
}

void SMyButton::SetVAlign(EVerticalAlignment VAlign)
{
	if (ChildSlot.VAlignment != VAlign)
	{
		ChildSlot.VAlignment = VAlign;
		Invalidate(EInvalidateWidget::Layout);
	}
}

void SMyButton::SetPadding(const TAttribute<FMargin>& InPadding)
{
	if (!ChildSlot.SlotPadding.IdenticalTo(InPadding))
	{
		ChildSlot.SlotPadding = InPadding;
		Invalidate(EInvalidateWidget::LayoutAndVolatility);
	}
}

void SMyButton::SetNormalImageUPROPERTY(const TAttribute<const FSlateBrush*>& InBorderImage)
{
	if (!BorderImage.IdenticalTo(InBorderImage))
	{
		BorderImage = InBorderImage;
		Invalidate(EInvalidateWidget::LayoutAndVolatility);

		SetNormalImage(InBorderImage.Get());
	}
}

void SMyButton::SetBackgroundColor(const TAttribute<FSlateColor>& InColorAndOpacity)
{
	if (!BorderBackgroundColor.IdenticalTo(InColorAndOpacity))
	{
		BorderBackgroundColor = InColorAndOpacity;
		Invalidate(EInvalidateWidget::PaintAndVolatility);
	}
}

int32 SMyButton::OnPaint(const FPaintArgs & Args, const FGeometry & AllottedGeometry, 
	const FSlateRect & MyCullingRect, FSlateWindowElementList & OutDrawElements, int32 LayerId, 
	const FWidgetStyle & InWidgetStyle, bool bParentEnabled) const
{
	const bool bEnabled = ShouldBeEnabled(bParentEnabled);
	
	const FSlateBrush * BrushResource = GetImage();

	if (BrushResource != nullptr && BrushResource->DrawAs != ESlateBrushDrawType::NoDrawType)
	{
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			BrushResource,
			ESlateDrawEffect::None,
			BrushResource->GetTint(InWidgetStyle) * InWidgetStyle.GetColorAndOpacityTint() * BorderBackgroundColor.Get().GetColor(InWidgetStyle)
		);
	}

	return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, 
		InWidgetStyle, bEnabled);
}

bool SMyButton::SupportsKeyboardFocus() const
{
	return false;
}

FReply SMyButton::OnMouseButtonDown(const FGeometry & MyGeometry, const FPointerEvent & MouseEvent)
{
	if (IsEnabled())
	{
		if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
		{
			if (PC->ShouldIgnoreButtonUpOrDownEvents())
			{
				return FReply::Unhandled();
			}

			bIsPressedByLMB = true;
			
			if (bUsingUnifiedPressedImage)
			{
				/* For this to work we will need to set the PressedImage during HUD setup */
				PC->ShowUIButtonPressedImage_Mouse(this, PressedImage);
			}

			PlayPressedByLMBSound();

			// Getting crashes here. Busy right now, will have to address it later
			OnLeftMouseButtonPressed.Execute();
		}
		else if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
		{
			if (PC->ShouldIgnoreButtonUpOrDownEvents())
			{
				return FReply::Unhandled();
			}

			bIsPressedByRMB = true;
			
			if (bUsingUnifiedPressedImage)
			{
				PC->ShowUIButtonPressedImage_Mouse(this, PressedImage);
			}

			PlayPressedByRMBSound();

			OnRightMouseButtonPressed.Execute();
		}
	}

	Invalidate(EInvalidateWidget::Layout);

	return FReply::Handled();
}

FReply SMyButton::OnMouseButtonUp(const FGeometry & MyGeometry, const FPointerEvent & MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (PC->ShouldIgnoreButtonUpOrDownEvents())
		{
			return FReply::Unhandled();
		}

		bIsPressedByLMB = false;

		if (bUsingUnifiedPressedImage)
		{
			PC->HideUIButtonPressedImage_Mouse(UMGWidget, MyGeometry, HoverImage, bUsingUnifiedHoverImage);
		}

		OnLeftMouseButtonReleased.Execute();
	}
	else if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		if (PC->ShouldIgnoreButtonUpOrDownEvents())
		{
			return FReply::Unhandled();
		}

		bIsPressedByRMB = false;

		if (bUsingUnifiedPressedImage)
		{
			PC->HideUIButtonPressedImage_Mouse(UMGWidget, MyGeometry, HoverImage, bUsingUnifiedHoverImage);
		}

		OnRightMouseButtonReleased.Execute();
	}
	
	Invalidate(EInvalidateWidget::Layout);

	return FReply::Handled(); 
}

/* This is called on the 2nd press. On the mouse release OnMouseButtonUp is called */
FReply SMyButton::OnMouseButtonDoubleClick(const FGeometry & InMyGeometry, const FPointerEvent & InMouseEvent)
{
	// Don't have any special behavior for double clicks right now so just do the behavior of a regular press
	return OnMouseButtonDown(InMyGeometry, InMouseEvent);
}

void SMyButton::OnMouseEnter(const FGeometry & MyGeometry, const FPointerEvent & MouseEvent)
{	
	if (PC->OnButtonHovered(UMGWidget, MyGeometry, HoverImage, bUsingUnifiedHoverImage, UserWidgetButtonOwner))
	{
		if (IsEnabled())
		{
			PlayHoverSound();
		}

		SCompoundWidget::OnMouseEnter(MyGeometry, MouseEvent);
		
		Invalidate(EInvalidateWidget::Layout);
	}
	else
	{
		/* Not sure I need the OnMouseEnter or Invalidate calls here. Actually yup I believe 
		SCompoundWidget::OnMouseEnter will set SWidget::bIsHovered to true which I don't want */
		//SCompoundWidget::OnMouseEnter(MyGeometry, MouseEvent);
		//Invalidate(EInvalidateWidget::Layout);
	}
}

void SMyButton::OnMouseLeave(const FPointerEvent & MouseEvent)
{
	SCompoundWidget::OnMouseLeave(MouseEvent);

	PC->OnButtonUnhovered(UMGWidget, bUsingUnifiedHoverImage, UserWidgetButtonOwner);

	Invalidate(EInvalidateWidget::Layout);
}

FVector2D SMyButton::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
	return DesiredSizeScale.Get() * SCompoundWidget::ComputeDesiredSize(LayoutScaleMultiplier);
}

const FSlateBrush * SMyButton::GetImage() const
{
	/* RMB is generally associated with cancelling stuff and whatnot so if RMB is NOT pressed 
	then it's possible to show the pressed image */
	if (!bUsingUnifiedPressedImage && bIsPressedByLMB && !bIsPressedByRMB)
	{
		return PressedImage;
	}
	else if (bIsHighlighted)
	{
		return HighlightedImage;
	}
	else if (!bUsingUnifiedHoverImage && IsHovered())
	{
		return HoverImage;
	}
	else
	{
		return NormalImage;
	}
}

void SMyButton::PlayHoverSound()
{
	PlaySound(HoveredSound);
}

void SMyButton::PlayPressedByLMBSound()
{
	PlaySound(PressedByLMBSound);
}

void SMyButton::PlayPressedByRMBSound()
{
	PlaySound(PressedByRMBSound);
}

FSimpleDelegate & SMyButton::GetOnLeftMouseButtonPressDelegate()
{
	return OnLeftMouseButtonPressed;
}

FSimpleDelegate & SMyButton::GetOnRightMouseButtonPressDelegate()
{
	return OnRightMouseButtonPressed;
}

FSimpleDelegate & SMyButton::GetOnLeftMouseButtonReleasedDelegate()
{
	return OnLeftMouseButtonReleased;
}

FSimpleDelegate & SMyButton::GetOnRightMouseButtonReleasedDelegate()
{
	return OnRightMouseButtonReleased;
}

void SMyButton::SetNormalImage(const FSlateBrush * InImage)
{
	NormalImage = InImage;
}

void SMyButton::SetHoverImage(const FSlateBrush * InImage)
{
	HoverImage = InImage;
}

void SMyButton::SetPressedImage(const FSlateBrush * InImage)
{
	PressedImage = InImage;
}

void SMyButton::SetHighlightedImage(const FSlateBrush * InImage)
{
	HighlightedImage = InImage;
}

void SMyButton::SetHoveredSound(USoundBase * InSound)
{
	HoveredSound = InSound;
}

void SMyButton::SetPressedByLMBSound(USoundBase * InSound)
{
	PressedByLMBSound = InSound;
}

void SMyButton::SetPressedByRMBSound(USoundBase * InSound)
{
	PressedByRMBSound = InSound;
}

void SMyButton::SetIgnoreHoveredImage(bool bIgnoreHoveredImage)
{
	bUsingUnifiedHoverImage = bIgnoreHoveredImage;
}

void SMyButton::SetIgnorePressedImage(bool bIgnorePressedImage)
{
	bUsingUnifiedPressedImage = bIgnorePressedImage;
}

void SMyButton::SetPC(ARTSPlayerController * InPlayerController)
{
	PC = InPlayerController;
}

void SMyButton::SetOwningWidget(UMyButton * InUMGWidget)
{
	UMGWidget = InUMGWidget;
}

void SMyButton::SetUserWidgetOwnerButton(UInGameWidgetBase * OwnerUserWidget)
{
	UserWidgetButtonOwner = OwnerUserWidget;
}

void SMyButton::SetIsPressedByLMB(bool bInValue)
{
	bIsPressedByLMB = bInValue;
}

void SMyButton::SetIsPressedByRMB(bool bInValue)
{
	bIsPressedByRMB = bInValue;
}

void SMyButton::SetIsHighlighted(bool bInValue)
{
	bIsHighlighted = bInValue;
}

void SMyButton::ForceUnhover()
{
	/* This is similar to OnMouseLeave but I don't have a FPointerEvent as a param, I just 
	do bIsHovered = false instead. I hope this does the job ok */

	bIsHovered = false;

	PC->OnButtonUnhovered(UMGWidget, bUsingUnifiedHoverImage, UserWidgetButtonOwner);

	Invalidate(EInvalidateWidget::Layout);
}

void SMyButton::PlaySound(USoundBase * Sound)
{
	if (Sound != nullptr && GEngine != nullptr)
	{
		FAudioDevice * const AudioDevice = GEngine->GetActiveAudioDevice();
		if (AudioDevice != nullptr)
		{
			FActiveSound NewActiveSound;
			NewActiveSound.SetSound(Sound);
			NewActiveSound.bIsUISound = true;
			NewActiveSound.UserIndex = 0;
			NewActiveSound.Priority = Sound->Priority;

			AudioDevice->AddNewActiveSound(NewActiveSound);
		}
	}
}

#undef LOCTEXT_NAMESPACE
