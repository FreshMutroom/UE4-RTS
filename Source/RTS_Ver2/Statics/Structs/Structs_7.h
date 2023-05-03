// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Structs_7.generated.h"

struct FKey;
struct FSlateFontInfo;


/* How an image that represents a key binding should be displayed */
UENUM()
enum class EInputKeyDisplayMethod : uint8
{
	/* You have an image that sufficiently conveys which key it represents */
	ImageOnly,

	/* You have a image but want to add some text to it e.g. you have a sqaure key image that
	is blank and want to write "Enter" overtop of it to convey it is the enter key */
	ImageAndAddTextAtRuntime
};


/* Information about a single input key for example on the keymoard or mouse */
USTRUCT()
struct FKeyInfo
{
	GENERATED_BODY()

public:

	FKeyInfo();

	FKeyInfo(const FKey & InKey);

	FKeyInfo(const FString & InDisplayName, const FString & InPartialText);

protected:

	// Set default values for PartialBrushTextFont
	void DefaultConstructFontInfo();

public:

	// Trivial getters and setters
	const FString & GetText() const { return DisplayText; }
	EInputKeyDisplayMethod GetImageDisplayMethod() const { return ImageDisplayMethod; }
	void SetImageDisplayMethod(EInputKeyDisplayMethod InMethod) { ImageDisplayMethod = InMethod; }
	const FSlateBrush & GetFullBrush() const { return FullBrush; }
	const FSlateBrush & GetPartialBrush() const { return PartialBrush; }
	void SetPartialBrush(const FSlateBrush & InBrush) { PartialBrush = InBrush; }
	const FText & GetPartialBrushText() const { return PartialBrushText; }
	const FSlateFontInfo & GetPartialBrushTextFont() const { return PartialBrushTextFont; }
	void SetPartialFont(const FSlateFontInfo & InFont) { PartialBrushTextFont = InFont; }

protected:

	//-----------------------------------------------------
	//	Data
	//-----------------------------------------------------

	/** The text to display on UI to represent this key */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FString DisplayText;

	/**
	 *	Whether to draw text overtop of the image. 
	 *	
	 *	@See URTSGameInstance::KeyMappings_bForceUsePartialBrushes because it can override this 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EInputKeyDisplayMethod ImageDisplayMethod;

	/* I don't need both FSlateBrush on here, 1 is enough */

	/* The image to display on the UI to represent this button */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FSlateBrush FullBrush;

	/* The image to display on the UI to represent this key. This brush will have text drawn 
	overtop of it */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FSlateBrush PartialBrush;
	
	/* The text to draw overtop of PartialBrush */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FText PartialBrushText;

	/* This is here mainly so you can change the size of the font because like showing "Q"
	or "Backspace" there'sa big difference in length between the two */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FSlateFontInfo PartialBrushTextFont;
};
