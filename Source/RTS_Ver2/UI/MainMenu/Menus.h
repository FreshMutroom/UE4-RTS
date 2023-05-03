// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/EditableText.h"

#include "Statics/CommonEnums.h"
#include "Menus.generated.h"

class URTSGameInstance;
class UImage;
class UComboBoxString;
class ARTSGameState;


/**
 *	The header of this class contains some variables for the main menu and some custom basic 
 *	widget implementations. The source file contains many functions of URTSGameInstance
 */
class RTS_VER2_API Menus
{
public:

	//=======================================================================================
	//	Statics
	//=======================================================================================

	/* Category to appear in palette panel in editor  */
	static const FText PALETTE_CATEGORY;

	/* Text with nothing in it */
	static const FText BLANK_TEXT;


	// TODO: move all static variables acossiated with menus/lobbies to here, and update includes

	/* Get game instance from cast. Ideally if this could be set in the widget constructors
	then no constant casting required but I don't know where the widget constructor is */
	static URTSGameInstance * GetGameInstance(const UWorld * World);

	/* Get game state from cast like game instance */
	static ARTSGameState * GetGameState(const UWorld * World);
};


//-----------------------------------------------------
//	Base widget classes
//-----------------------------------------------------

/* Base class for custom buttons */
UCLASS(Abstract, NotBlueprintable)
class RTS_VER2_API UMenuButton : public UButton
{
	GENERATED_BODY()

public:

	UMenuButton();

protected:

	/* Override to add press fuctionality */
	UFUNCTION()
	virtual void UIBinding_OnPress();

#if WITH_EDITOR
	/* To set category that shows up in palette in editor */
	virtual const FText GetPaletteCategory() override;
#endif
};


UCLASS(Abstract, NotBlueprintable)
class RTS_VER2_API UMenuTextBox : public UTextBlock
{
	GENERATED_BODY()

protected:

#if WITH_EDITOR
	/* To set category that shows up in palette in editor */
	virtual const FText GetPaletteCategory() override;
#endif
};


UCLASS(Abstract, NotBlueprintable)
class RTS_VER2_API UMenuEditableText : public UEditableText
{
	GENERATED_BODY()

protected:

	virtual void UIBinding_OnTextChanged(const FText & NewText);
	virtual void UIBinding_OnTextCommitted(const FText & NewText, ETextCommit::Type CommitMethod);

#if WITH_EDITOR
	/* To set category that shows up in palette in editor */
	virtual const FText GetPaletteCategory() override;
#endif
};

//-----------------------------------------------------
//	Confirm exit to OS buttons
//-----------------------------------------------------

UCLASS(meta = (DisplayName = "Confirm Exit to OS: Yes Button"))
class RTS_VER2_API UConfirmExitToOS_YesButton : public UMenuButton
{
	GENERATED_BODY()

protected:

	virtual void UIBinding_OnPress() override;
};

UCLASS(meta = (DisplayName = "Confirm Exit to OS: No Button"))
class RTS_VER2_API UConfirmExitToOS_NoButton : public UMenuButton
{
	GENERATED_BODY()

protected:

	virtual void UIBinding_OnPress() override;
};
