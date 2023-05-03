// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InGameWidgetBase.generated.h"

class URTSGameInstance;
class ARTSPlayerState;
class ARTSPlayerController;
class ISelectable;
class AFactionInfo;
class ARTSGameState;
class URTSHUD;


/**
 *	A widget that appears during a match
 */
UCLASS(Abstract)
class RTS_VER2_API UInGameWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:

	UInGameWidgetBase(const FObjectInitializer & ObjectInitializer);

	/* Called basically when its design time and will cause crashes when trying to assign
	references. Using SetupWidget instead */
	//virtual bool Initialize() override;

	/* Setup widget right after it is created. Should only be called once
	@return - true if widget has already setup, false if it has not */
	virtual bool SetupWidget(URTSGameInstance * InGameInstance = nullptr, ARTSPlayerController * InPlayerController = nullptr);

	/** 
	 *	This is called when the player presses the ESC key and this is the widget that is chosen 
	 *	to be closed. 
	 *
	 *	@return - false if it did nothing.
	 */
	virtual bool RespondToEscapeRequest(URTSHUD * HUD);

protected:

	virtual void NativeTick(const FGeometry & MyGeometry, float InDeltaTime) override;

	/* Reference to game state */
	UPROPERTY()
	ARTSGameState * GS;

	/* Reference to game instance */
	UPROPERTY()
	URTSGameInstance * GI;

	/* Reference to local player controller */
	UPROPERTY()
	ARTSPlayerController * PC;

	/* Reference to PCs player state */
	UPROPERTY()
	ARTSPlayerState * PS;

	/* Reference to faction info of player state */
	UPROPERTY()
	AFactionInfo * FI;

	/* Reference to the local players current selected */
	UPROPERTY()
	TScriptInterface <ISelectable> CurrentSelected;

private:

	/* True if SetupWidget has been called at least once */
	bool bHasSetup;

#if WITH_EDITORONLY_DATA

	/**
	 *	List of widgets that can be bound with BindWidgetOptional. Auto populated inside ctor.
	 *	Maps widget name to widget class + it's description
	 */
	UPROPERTY(VisibleAnywhere, Category = "RTS")
	TMap < FString, FText > BindWidgetOptionalList;

#endif

public:

	const TScriptInterface <ISelectable> & GetCurrentSelected() const;

	/* Return true if the widget was named correctly in editor, is also the right class
	and is flagged as a variable */
	static bool IsWidgetBound(UWidget * Widget);
};
