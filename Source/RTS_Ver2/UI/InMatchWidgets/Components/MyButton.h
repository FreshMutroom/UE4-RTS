// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ContentWidget.h"
#include "MyButton.generated.h"

class SMyButton;
class ARTSPlayerController;
struct FContextButtonInfo;
struct FSlateBrush;
struct FPropertyChangedEvent;
class UInGameWidgetBase;
enum class EUIElementType : uint8;
struct FUnifiedImageAndSoundFlags;
struct FUnifiedImageAndSoundAssets;


//--------------------------------------------------------------------
//	Reference for this file:
//	https://wiki.unrealengine.com/UMG,_Custom_Widget_Components_And_Render_Code,_Usable_In_UMG_Designer
//
//	Credit to Rama
//--------------------------------------------------------------------


/**
 *	My button class. Uses my slate button class as its slate widget.
 *
 *	This button does not handle (or consume) mouse press/release events. So
 *	do not stack buttons on top of each other in widget designer. Generally this isn't something 
 *	you do anyway. 
 */
UCLASS()
class RTS_VER2_API UMyButton : public UContentWidget
{
	GENERATED_BODY()
	
	//-------------------------------------------------------------------------------
	//	Setupy Type Functions
	//-------------------------------------------------------------------------------

public:

	/* Sets default values */
	UMyButton(const FObjectInitializer & ObjectInitializer);

	// UWidget interface
	virtual void SynchronizeProperties() override;
	// End of UWidget interface

protected:

	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	// End of UWidget interface
	
	// UVisual interface
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	// End of UVisual interface

#if WITH_EDITOR
	// UWidget interface
	virtual const FText GetPaletteCategory() override;
	// End UWidget interface
#endif
	
	// UPanelWidget
	virtual UClass * GetSlotClass() const override;
	virtual void OnSlotAdded(UPanelSlot * InSlot) override;
	virtual void OnSlotRemoved(UPanelSlot * InSlot) override;
	// End UPanelWidget

public:

	//~ Begin UObject Interface
	virtual void PostLoad() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent & PropertyChangedEvent) override;
#endif
	//~ End UObject Interface

	//-------------------------------------------------------------------------------
	//	Call during design time
	//-------------------------------------------------------------------------------
	//	These will actually change this UWidget as well as the SWidget.
	//	The HUD's pause menu button for one will need these
	//-------------------------------------------------------------------------------

	//-------------------------------------------------------------------------------
	//	Useful functions you might call during a match
	//-------------------------------------------------------------------------------

	/* @param Owner - if this button is part of a user widget class thats purpose is a 
	button then this should point to it. Otherwise it can be null. 
	Examples of widgets you would pass into this right now would be: 
	- UItemOnDisplayInShopButton 
	- UInventoryItemButton 
	This is because they contain a UMyButton as a member instead of actually deriving from 
	UMyButton themself */
	void SetPurpose(EUIElementType InPurpose, UInGameWidgetBase * Owner);
	EUIElementType GetPurpose() const;

	/* All these are intended to be called after the game has started NOT during design time, 
	so they do not set anything on this class and just bubble to the slate button */
	void SetImages(const FUnifiedImageAndSoundFlags & UnifiedAssetFlags,
		const FSlateBrush * InNormalImage,
		const FSlateBrush * InHoveredImage,
		const FSlateBrush * InPressedImage,
		const FSlateBrush * InHighlightedImage);
	void SetImagesAndUnifiedImageFlags_ExcludeNormalImage(const FUnifiedImageAndSoundFlags & UnifiedAssetFlags,
		const FSlateBrush * InHoveredImage,
		const FSlateBrush * InPressedImage,
		const FSlateBrush * InHighlightedImage);
	void SetImagesAndUnifiedImageFlags_ExcludeNormalAndHighlightedImage(const FUnifiedImageAndSoundFlags & UnifiedAssetFlags, 
		const FSlateBrush * InHoveredImage,
		const FSlateBrush * InPressedImage);
	void SetImages_PlayerTargetingPanel(const FUnifiedImageAndSoundFlags & UnifiedAssetFlags,
		const FSlateBrush * InNormalImage,
		const FSlateBrush * InHoveredImage,
		const FSlateBrush * InPressedImage);
	void SetImages_ExcludeHighlightedImage(const FUnifiedImageAndSoundFlags & UnifiedAssetFlags,
		const FSlateBrush * InNormalImage,
		const FSlateBrush * InHoveredImage,
		const FSlateBrush * InPressedImage);
	void SetUnifiedImages_ExcludeNormalAndHighlightedImage(const FUnifiedImageAndSoundFlags & UnifiedAssetFlags,
		const FSlateBrush * InHoveredImage,
		const FSlateBrush * InPressedImage);
	/* Probably need to have another look at this func */
	void SetImages_CommandSkillTreeNode(const FUnifiedImageAndSoundFlags & UnifiedAssetFlags,
		const FSlateBrush * InNormalImage,
		const FSlateBrush * InHoveredImage,
		const FSlateBrush * InPressedImage);
	void SetImages_ExcludeHighlightedImageAndIgnoreUnifiedFlags(const FSlateBrush * InNormalImage,
		const FSlateBrush * InHoveredImage,
		const FSlateBrush * InPressedImage);

	void SetHoveredSound(USoundBase * InSound);
	void SetPressedByLMBSound(USoundBase * InSound);
	void SetPressedByRMBSound(USoundBase * InSound);
	void SetIgnoreHoveredImage(bool bIgnoreHoveredImage);
	void SetIgnorePressedImage(bool bIgnorePressedImage);
	void SetPC(ARTSPlayerController * InPlayerController);
	void SetOwningWidget();
	/* Sets the pressed/hovered images/sounds on this button. This version of this func is 
	for buttons that do not get there images from the GI or whatnot i.e. do not call this 
	for selectable action bar buttons. I should make a seperate func for those that also 
	takes a FContextButtonInfo param too probably */
	void SetImagesAndSounds(const FUnifiedImageAndSoundFlags & AssetFlags, const FUnifiedImageAndSoundAssets & Assets);

	const FSlateBrush & GetNormalImage() const;
	const FSlateBrush & GetHoverImage() const;
	const FSlateBrush & GetPressedImage() const;
	const FSlateBrush & GetHighlightedImage() const;

	/* Get ref to mouse delegates so bindings can be done */
	FSimpleDelegate & GetOnLeftMouseButtonPressDelegate() const;
	FSimpleDelegate & GetOnRightMouseButtonPressDelegate() const;
	FSimpleDelegate & GetOnLeftMouseButtonReleasedDelegate() const;
	FSimpleDelegate & GetOnRightMouseButtonReleasedDelegate() const;

	// Same as above. For easy code replacement
	FSimpleDelegate & OnLeftMouseButtonPressed() const;
	FSimpleDelegate & OnRightMouseButtonPressed() const;
	FSimpleDelegate & OnLeftMouseButtonReleased() const;
	FSimpleDelegate & OnRightMouseButtonReleased() const;

	void SetIsPressedByLMB(bool bInValue);
	void SetIsPressedByRMB(bool bInValue);
	void SetIsHighlighted(bool bInValue);

	void ForceUnhover();

protected:

	//-------------------------------------------------------------------------------
	//	Data
	//-------------------------------------------------------------------------------
	//	A lot of this is just copied from UBorder.h

	/* What this button is used for */
	EUIElementType Purpose;

	/** The alignment of the content horizontally. */
	UPROPERTY(EditAnywhere, Category = "Content")
	TEnumAsByte<EHorizontalAlignment> HorizontalAlignment;

	/** The alignment of the content vertically. */
	UPROPERTY(EditAnywhere, Category = "Content")
	TEnumAsByte<EVerticalAlignment> VerticalAlignment;

	/** Color and opacity multiplier of content in the border */
	UPROPERTY(EditAnywhere, Category = "Content", meta = (sRGB = "true"))
	FLinearColor ContentColorAndOpacity;

	/** The padding area between the slot and the content it contains. */
	UPROPERTY(EditAnywhere, Category = "Content")
	FMargin Padding;

	/** Brush to draw as the background */
	UPROPERTY(EditAnywhere, Category = "Appearance", meta = (DisplayName = "Brush"))
	FSlateBrush NormalImage;

	/* Brush to draw when button is hovered by the mouse */
	UPROPERTY(EditAnywhere, Category = "Appearance", meta = (DisplayName = "Hovered Brush"))
	FSlateBrush HoveredImage;

	/* Brush to draw when button is pressed by LMB/RMB/possibly.more */
	UPROPERTY(EditAnywhere, Category = "Appearance", meta = (DisplayName = "Pressed Brush"))
	FSlateBrush PressedImage;

	/** 
	 *	Brush to draw when button is highlighted. 
	 *	
	 *	I recently added this variable. I might forget to set it at a lot of times. Over time 
	 *	I will correct those 
	 */
	UPROPERTY(EditAnywhere, Category = "Appearance", meta = (DisplayName = "Highlighted Brush"))
	FSlateBrush HighlightedImage;

	/** Color and opacity of the actual border image */
	UPROPERTY(EditAnywhere, Category = "Appearance", meta = (sRGB = "true"))
	FLinearColor BrushColor;

	/**
	 *	Scales the computed desired size of this border and its contents. Useful
	 *	for making things that slide open without having to hard-code their size.
	 *	Note: if the parent widget is set up to ignore this widget's desired size,
	 *	then changing this value will have no effect.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Appearance)
	FVector2D DesiredSizeScale;

public:

	void SetPadding(FMargin InPadding);
	void SetHorizontalAlignment(EHorizontalAlignment InHorizontalAlignment);
	void SetVerticalAlignment(EVerticalAlignment InVerticalAlignment);
	void SetBrushColor(FLinearColor Color);
	
	// Slate button
	TSharedPtr<SMyButton> MyButton;


	//-----------------------------------------------------------------------------------------
	//=======================================================================================
	//	------ Stuff below isn't really related to this class in general -------
	//	Work around for not having params on mouse press/release delegates.
	//=======================================================================================
	//-----------------------------------------------------------------------------------------

	void PlayerTargetingButton_OnLMBPressed();
	void PlayerTargetingButton_OnLMBReleased();
	void PlayerTargetingButton_OnRMBPressed();
	void PlayerTargetingButton_OnRMBReleased();
};
