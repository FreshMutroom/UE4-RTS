// Fill out your copyright notice in the Description page of Project Settings.

#include "MyButton.h"
#include "UI/InMatchWidgets/Components/MySlateButton.h"
#include "UI/InMatchWidgets/Components/MyButtonSlot.h"
#include "Statics/OtherEnums.h"

#include "UI/RTSHUD.h" // Need this for the ugly workaround at the bottom of the file


#define LOCTEXT_NAMESPACE "UMG"

UMyButton::UMyButton(const FObjectInitializer & ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Just copying UBorder 
	bIsVariable = false;

	ContentColorAndOpacity = FLinearColor::White;
	BrushColor = FLinearColor::White;
	
	Padding = FMargin(4, 2);

	HorizontalAlignment = HAlign_Fill;
	VerticalAlignment = VAlign_Fill;

	DesiredSizeScale = FVector2D(1, 1);

	Purpose = EUIElementType::None;
}

void UMyButton::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	// UBorder code needs to set up the OnPress/Release events, UButton does not. Not too sure
	// I feel like synchoronizeProperties and RebuildWidget allow for you to put the logic 
	// in either one
	
	MyButton->SetPadding(Padding);
	MyButton->SetBackgroundColor(BrushColor);
	MyButton->SetColorAndOpacity(ContentColorAndOpacity);
	MyButton->SetNormalImageUPROPERTY(&NormalImage);
	MyButton->SetHoverImage(&HoveredImage);
	MyButton->SetPressedImage(&PressedImage);
	MyButton->SetHighlightedImage(&HighlightedImage);
	MyButton->SetDesiredSizeScale(DesiredSizeScale);
}

TSharedRef<SWidget> UMyButton::RebuildWidget()
{
	/* Might need more stuff here */
	MyButton = SNew(SMyButton);

	if (GetChildrenCount() > 0)
	{
		CastChecked<UMyButtonSlot>(GetContentSlot())->BuildSlot(MyButton.ToSharedRef());
	}

	return MyButton.ToSharedRef();
}

void UMyButton::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	MyButton.Reset();
}

#if WITH_EDITOR
const FText UMyButton::GetPaletteCategory()
{
	return LOCTEXT("RTS", "RTS");
}
#endif

UClass * UMyButton::GetSlotClass() const
{
	return UMyButtonSlot::StaticClass();
}

void UMyButton::OnSlotAdded(UPanelSlot * InSlot)
{
	// Basically just copied UBorder
	
	// Copy the content properties into the new slot so that it matches what has been setup
	// so far by the user.
	UMyButtonSlot * ButtonSlot = CastChecked<UMyButtonSlot>(InSlot);
	ButtonSlot->SetPadding(Padding);
	ButtonSlot->SetHorizontalAlignment(HorizontalAlignment);
	ButtonSlot->SetVerticalAlignment(VerticalAlignment);

	// Add the child to the live slot if it already exists
	if (MyButton.IsValid())
	{
		// Construct the underlying slot.
		ButtonSlot->BuildSlot(MyButton.ToSharedRef());
	}
}

void UMyButton::OnSlotRemoved(UPanelSlot * InSlot)
{
	// Remove the widget from the live slot if it exists.
	if (MyButton.IsValid())
	{
		MyButton->SetContent(SNullWidget::NullWidget);
	}
}

void UMyButton::PostLoad()
{
	Super::PostLoad();

	if (GetChildrenCount() > 0)
	{
		//TODO UMG Pre-Release Upgrade, now have slots of their own.  Convert existing slot to new slot.
		if (UPanelSlot * PanelSlot = GetContentSlot())
		{
			UMyButtonSlot * BorderSlot = Cast<UMyButtonSlot>(PanelSlot);
			if (BorderSlot == nullptr)
			{
				BorderSlot = NewObject<UMyButtonSlot>(this);
				BorderSlot->Content = GetContentSlot()->Content;
				BorderSlot->Content->Slot = BorderSlot;
				Slots[0] = BorderSlot;
			}
		}
	}
}

#if WITH_EDITOR
void UMyButton::PostEditChangeProperty(FPropertyChangedEvent & PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void UMyButton::SetPadding(FMargin InPadding)
{
	Padding = InPadding;
	if (MyButton.IsValid())
	{
		MyButton->SetPadding(InPadding);
	}
}

void UMyButton::SetHorizontalAlignment(EHorizontalAlignment InHorizontalAlignment)
{
	HorizontalAlignment = InHorizontalAlignment;
	if (MyButton.IsValid())
	{
		MyButton->SetHAlign(InHorizontalAlignment);
	}
}

void UMyButton::SetVerticalAlignment(EVerticalAlignment InVerticalAlignment)
{
	VerticalAlignment = InVerticalAlignment;
	if (MyButton.IsValid())
	{
		MyButton->SetVAlign(InVerticalAlignment);
	}
}

void UMyButton::SetBrushColor(FLinearColor Color)
{
	BrushColor = Color;
	if (MyButton.IsValid())
	{
		MyButton->SetBackgroundColor(Color);
	}
}

void UMyButton::SetPurpose(EUIElementType InPurpose, UInGameWidgetBase * Owner)
{
	Purpose = InPurpose;

	MyButton->SetUserWidgetOwnerButton(Owner);
}

EUIElementType UMyButton::GetPurpose() const
{
	return Purpose;
}

void UMyButton::SetImages(const FUnifiedImageAndSoundFlags & UnifiedAssetFlags, const FSlateBrush * InNormalImage, const FSlateBrush * InHoveredImage, const FSlateBrush * InPressedImage, const FSlateBrush * InHighlightedImage)
{
	MyButton->SetNormalImage(InNormalImage);
	if (UnifiedAssetFlags.bUsingUnifiedHoverImage == false)
	{
		MyButton->SetHoverImage(InHoveredImage);
	}
	if (UnifiedAssetFlags.bUsingUnifiedPressedImage == false)
	{
		MyButton->SetPressedImage(InPressedImage);
	}
	MyButton->SetHighlightedImage(InHighlightedImage);
}

void UMyButton::SetImagesAndUnifiedImageFlags_ExcludeNormalImage(const FUnifiedImageAndSoundFlags & UnifiedAssetFlags, const FSlateBrush * InHoveredImage, const FSlateBrush * InPressedImage, const FSlateBrush * InHighlightedImage)
{
	if (UnifiedAssetFlags.bUsingUnifiedHoverImage)
	{
		MyButton->SetHoverImage(InHoveredImage);
		SetIgnoreHoveredImage(true);
	}
	if (UnifiedAssetFlags.bUsingUnifiedPressedImage)
	{
		MyButton->SetPressedImage(InPressedImage);
		SetIgnorePressedImage(true);
	}
	MyButton->SetHighlightedImage(InHighlightedImage);
}

void UMyButton::SetImagesAndUnifiedImageFlags_ExcludeNormalAndHighlightedImage(const FUnifiedImageAndSoundFlags & UnifiedAssetFlags, const FSlateBrush * InHoveredImage, const FSlateBrush * InPressedImage)
{
	if (UnifiedAssetFlags.bUsingUnifiedHoverImage)
	{
		MyButton->SetHoverImage(InHoveredImage);
		SetIgnoreHoveredImage(true);
	}
	if (UnifiedAssetFlags.bUsingUnifiedPressedImage)
	{
		MyButton->SetPressedImage(InPressedImage);
		SetIgnorePressedImage(true);
	}
}

void UMyButton::SetImages_PlayerTargetingPanel(const FUnifiedImageAndSoundFlags & UnifiedAssetFlags, const FSlateBrush * InNormalImage, const FSlateBrush * InHoveredImage, const FSlateBrush * InPressedImage)
{
	MyButton->SetNormalImage(InNormalImage);
	if (UnifiedAssetFlags.bUsingUnifiedHoverImage == false)
	{
		MyButton->SetHoverImage(InHoveredImage);
	}
	else
	{
		SetIgnoreHoveredImage(true);
	}
	if (UnifiedAssetFlags.bUsingUnifiedPressedImage == false)
	{
		MyButton->SetPressedImage(InPressedImage);
	}
	else
	{
		SetIgnorePressedImage(true);
	}
}

void UMyButton::SetImages_ExcludeHighlightedImage(const FUnifiedImageAndSoundFlags & UnifiedAssetFlags, const FSlateBrush * InNormalImage, const FSlateBrush * InHoveredImage, const FSlateBrush * InPressedImage)
{
	MyButton->SetNormalImage(InNormalImage);
	if (UnifiedAssetFlags.bUsingUnifiedHoverImage == false)
	{
		MyButton->SetHoverImage(InHoveredImage);
	}
	if (UnifiedAssetFlags.bUsingUnifiedPressedImage == false)
	{
		MyButton->SetPressedImage(InPressedImage);
	}
}

void UMyButton::SetUnifiedImages_ExcludeNormalAndHighlightedImage(const FUnifiedImageAndSoundFlags & UnifiedAssetFlags, const FSlateBrush * InHoveredImage, const FSlateBrush * InPressedImage)
{
	if (UnifiedAssetFlags.bUsingUnifiedHoverImage)
	{
		MyButton->SetHoverImage(InHoveredImage);
		SetIgnoreHoveredImage(true);
	}
	if (UnifiedAssetFlags.bUsingUnifiedPressedImage)
	{
		MyButton->SetPressedImage(InPressedImage);
		SetIgnorePressedImage(true);
	}
}

void UMyButton::SetImages_CommandSkillTreeNode(const FUnifiedImageAndSoundFlags & UnifiedAssetFlags, const FSlateBrush * InNormalImage, const FSlateBrush * InHoveredImage, const FSlateBrush * InPressedImage)
{
	/* Not sure why commander skill tree node does it like this. It may be right, it may 
	be wrong, I can't remember but I'll let it have it's own way of doing it for now. Maybe check 
	this in the future though */

	MyButton->SetNormalImage(InNormalImage);
	if (UnifiedAssetFlags.bUsingUnifiedHoverImage == false)
	{
		MyButton->SetHoverImage(InHoveredImage);
	}
	else
	{
		SetIgnoreHoveredImage(true);
	}
	if (UnifiedAssetFlags.bUsingUnifiedPressedImage == false)
	{
		MyButton->SetPressedImage(InPressedImage);
	}
	else
	{
		SetIgnorePressedImage(true);
	}
}

void UMyButton::SetImages_ExcludeHighlightedImageAndIgnoreUnifiedFlags(const FSlateBrush * InNormalImage, const FSlateBrush * InHoveredImage, const FSlateBrush * InPressedImage)
{
	MyButton->SetNormalImage(InNormalImage);
	MyButton->SetHoverImage(InHoveredImage);
	MyButton->SetPressedImage(InPressedImage);
}

void UMyButton::SetHoveredSound(USoundBase * InSound)
{
	MyButton->SetHoveredSound(InSound);
}

void UMyButton::SetPressedByLMBSound(USoundBase * InSound)
{
	MyButton->SetPressedByLMBSound(InSound);
}

void UMyButton::SetPressedByRMBSound(USoundBase * InSound)
{
	MyButton->SetPressedByRMBSound(InSound);
}

void UMyButton::SetIgnoreHoveredImage(bool bIgnoreHoveredImage)
{
	MyButton->SetIgnoreHoveredImage(bIgnoreHoveredImage);
}

void UMyButton::SetIgnorePressedImage(bool bIgnorePressedImage)
{
	MyButton->SetIgnorePressedImage(bIgnorePressedImage);
}

void UMyButton::SetPC(ARTSPlayerController * InPlayerController)
{
	MyButton->SetPC(InPlayerController);
}

void UMyButton::SetOwningWidget()
{
	MyButton->SetOwningWidget(this);
}

void UMyButton::SetImagesAndSounds(const FUnifiedImageAndSoundFlags & AssetFlags, const FUnifiedImageAndSoundAssets & Assets)
{
	/* Wrote this kind of fast. I think it's correct but may not be */
	
	if (AssetFlags.bUsingUnifiedHoverImage == true)
	{
		// Not sure if the image needs to get set but will set it anyway
		MyButton->SetHoverImage(&Assets.GetUnifiedMouseHoverImage().GetBrush());
		SetIgnoreHoveredImage(true);
	}
	if (AssetFlags.bUsingUnifiedPressedImage == true)
	{
		// Not sure if the image needs to get set but will set it anyway
		MyButton->SetPressedImage(&Assets.GetUnifiedMousePressImage().GetBrush());
		SetIgnorePressedImage(true);
	}
	if (AssetFlags.bUsingUnifiedHoverSound == true)
	{
		SetHoveredSound(Assets.GetUnifiedMouseHoverSound().GetSound());
	}
	if (AssetFlags.bUsingUnifiedLMBPressSound == true)
	{
		SetPressedByLMBSound(Assets.GetUnifiedPressedByLMBSound().GetSound());
	}
	if (AssetFlags.bUsingUnifiedRMBPressSound == true)
	{
		SetPressedByRMBSound(Assets.GetUnifiedPressedByRMBSound().GetSound());
	}
}

const FSlateBrush & UMyButton::GetNormalImage() const
{
	return NormalImage;
}

const FSlateBrush & UMyButton::GetHoverImage() const
{
	return HoveredImage;
}

const FSlateBrush & UMyButton::GetPressedImage() const
{
	return PressedImage;
}

const FSlateBrush & UMyButton::GetHighlightedImage() const
{
	return HighlightedImage;
}

FSimpleDelegate & UMyButton::GetOnLeftMouseButtonPressDelegate() const
{
	//UE_CLOG(MyButton.IsValid() == false, RTSLOG, Fatal, TEXT("Blah"));
	
	//UMyButton * This = const_cast<UMyButton*>(this);
	//This->RebuildWidget();

	return MyButton->GetOnLeftMouseButtonPressDelegate();
}

FSimpleDelegate & UMyButton::GetOnRightMouseButtonPressDelegate() const
{
	return MyButton->GetOnRightMouseButtonPressDelegate();
}

FSimpleDelegate & UMyButton::GetOnLeftMouseButtonReleasedDelegate() const
{
	return MyButton->GetOnLeftMouseButtonReleasedDelegate();
}

FSimpleDelegate & UMyButton::GetOnRightMouseButtonReleasedDelegate() const
{
	return MyButton->GetOnRightMouseButtonReleasedDelegate();
}

FSimpleDelegate & UMyButton::OnLeftMouseButtonPressed() const
{
	return GetOnLeftMouseButtonPressDelegate();
}

FSimpleDelegate & UMyButton::OnRightMouseButtonPressed() const
{
	return GetOnRightMouseButtonPressDelegate();
}

FSimpleDelegate & UMyButton::OnLeftMouseButtonReleased() const
{
	return GetOnLeftMouseButtonReleasedDelegate();
}

FSimpleDelegate & UMyButton::OnRightMouseButtonReleased() const
{
	return GetOnRightMouseButtonReleasedDelegate();
}

void UMyButton::SetIsPressedByLMB(bool bInValue)
{
	MyButton->SetIsPressedByLMB(bInValue);
}

void UMyButton::SetIsPressedByRMB(bool bInValue)
{
	MyButton->SetIsPressedByRMB(bInValue);
}

void UMyButton::SetIsHighlighted(bool bInValue)
{
	MyButton->SetIsHighlighted(bInValue);
}

void UMyButton::ForceUnhover()
{
	MyButton->ForceUnhover();
}


//-----------------------------------------------------------------------------------------
//=======================================================================================
//	------ Stuff below isn't really related to this class in general -------
//=======================================================================================
//-----------------------------------------------------------------------------------------

void UMyButton::PlayerTargetingButton_OnLMBPressed()
{
	ARTSPlayerController * PlayCon = CastChecked<ARTSPlayerController>(GetWorld()->GetFirstPlayerController());
	PlayCon->GetHUDWidget()->TellPlayerTargetingPanelAboutButtonAction_LMBPressed(this, PlayCon);
}

void UMyButton::PlayerTargetingButton_OnLMBReleased()
{
	ARTSPlayerController * PlayCon = CastChecked<ARTSPlayerController>(GetWorld()->GetFirstPlayerController());
	PlayCon->GetHUDWidget()->TellPlayerTargetingPanelAboutButtonAction_LMBReleased(this, PlayCon);
}

void UMyButton::PlayerTargetingButton_OnRMBPressed()
{
	ARTSPlayerController * PlayCon = CastChecked<ARTSPlayerController>(GetWorld()->GetFirstPlayerController());
	PlayCon->GetHUDWidget()->TellPlayerTargetingPanelAboutButtonAction_RMBPressed(this, PlayCon);
}

void UMyButton::PlayerTargetingButton_OnRMBReleased()
{
	ARTSPlayerController * PlayCon = CastChecked<ARTSPlayerController>(GetWorld()->GetFirstPlayerController());
	PlayCon->GetHUDWidget()->TellPlayerTargetingPanelAboutButtonAction_RMBReleased(this, PlayCon);
}


#undef LOCTEXT_NAMESPACE
