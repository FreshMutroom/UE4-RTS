// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/InGameWidgetBase.h"
#include "PlayerTargetingPanel.generated.h"

class UMyButton;
struct FCommanderAbilityInfo;
enum class EAffiliation : uint8;


USTRUCT()
struct FButtonArrayEntry
{
	GENERATED_BODY()

public:

	// Never call this ctor
	FButtonArrayEntry() { }

	explicit FButtonArrayEntry(URTSGameInstance * GameInst, UMyButton * InButton, ARTSPlayerState * InAssignedPlayer);

	void UpdateAppearance(const FCommanderAbilityInfo & AbilityInfo);

	/* Called when a player is defeated. May not be the one assigned to this struct */
	void OnPlayerDefeated(ARTSPlayerState * DefeatedPlayer);

	UMyButton * GetButton() const { return Button; }
	ARTSPlayerState * GetAssignedPlayer() const { return AssignedPlayer; }

	void SetButtonAppearanceForTargetable();
	void SetButtonAppearanceForUntargetable();

protected:

	UMyButton * Button;

	ARTSPlayerState * AssignedPlayer;

	float ButtonOriginalOpacity;

	bool bHasAssignedPlayerBeenDefeated;

	EAffiliation AssignedPlayersAffiliation;
};


/**
 *	A panel that shows players and makes them clickable. This widget is what 
 *	would appear if you have an ability that targets a player and not a selectable.
 *
 *	In editor add UMyButton widgets to it. These will be the buttons that will show the 
 *	players as targets. 
 * 
 *	Note: when you move a button around in the editor it gets rebuilt or something. Anyway 
 *	point is is that it will now be last for the all widgets iteration. Maybe I should allow 
 *	the user to tag which over the buttons should be. 
 */
UCLASS(Abstract)
class RTS_VER2_API UPlayerTargetingPanel : public UInGameWidgetBase
{
	GENERATED_BODY()
	
public:

	virtual bool SetupWidget(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController) override;

	/* This is intended to be called after all the players that are going to be in the match 
	are known */
	void MoreSetup();

	/* Setup a single button that is to be used for player targeting */
	void SetupPlayerTargetingButton(UMyButton * Button);

protected:

	virtual void NativeTick(const FGeometry & MyGeometry, float InDeltaTime) override;

public:

	bool IsShowingOrPlayingShowAnimation() const;

	/**
	 *	These two funcs aren't really requests since I check before calling them whether the 
	 *	widget is showing or not so better names are Show() and Hide()
	 *	
	 *	@param AbilityInfo - the ability that requires this panel to be shown for it 
	 */
	void OnRequestToBeShown(const FCommanderAbilityInfo * AbilityInfo);
	void OnRequestToBeHidden();

	virtual bool RespondToEscapeRequest(URTSHUD * HUD) override;

	/* Called when a player other than ourselves is defeated */
	void OnAnotherPlayerDefeated(ARTSPlayerState * DefeatedPlayer);

	/* These functions are called when a player targeting button is pressed/released */
	void OnPlayerTargetingButtonEvent_LMBPressed(UMyButton * Button, ARTSPlayerController * LocalPlayCon);
	void OnPlayerTargetingButtonEvent_LMBReleased(UMyButton * Button, ARTSPlayerController * LocalPlayCon);
	void OnPlayerTargetingButtonEvent_RMBPressed(UMyButton * Button, ARTSPlayerController * LocalPlayCon);
	void OnPlayerTargetingButtonEvent_RMBReleased(UMyButton * Button, ARTSPlayerController * LocalPlayCon);

protected:

	/* Update the buttons that the player can click on to target */
	void SetupTargetingButtonsForAbility(const FCommanderAbilityInfo * AbilityInfo);

	void UIBinding_OnCancelButtonLMBPressed();
	void UIBinding_OnCancelButtonLMBReleased();
	void UIBinding_OnCancelButtonRMBPressed();
	void UIBinding_OnCancelButtonRMBReleased();

	//-------------------------------------------------
	//	------- Data -------
	//-------------------------------------------------

	/* Pointer to info struct for the ability this widget is being displayed for */
	const FCommanderAbilityInfo * AssignedAbilityInfo;

	/* All the buttons on the panel that are used for player targeting */
	TArray<FButtonArrayEntry> Buttons;

#if WITH_EDITORONLY_DATA
	/** 
	 *	If you name a widget animation this then it will play whenever a request is made to 
	 *	show this panel. You are in charge of making the panel SelfHitTestInvisible/Visible 
	 *	at some point in the anim.
	 */
	UPROPERTY(VisibleAnywhere, Category = "RTS", meta = (DisplayName = "ShowAnimation"))
	FString ABC;

	/** 
	 *	If you name a widget animation this then it will play whenever a request is made to 
	 *	hide this panel. You are in charge of setting the visibility of the panel with the 
	 *	anim 
	 */
	UPROPERTY(VisibleAnywhere, Category = "RTS", meta = (DisplayName = "HideAnimation"))
	FString ABCDEF;
#endif

	static const FName ShowAnimFName;
	static const FName HideAnimFName;

	UWidgetAnimation * ShowAnim;
	UWidgetAnimation * HideAnim;

	/* If false then the widget is hidden or playing the hide anim */
	bool bShownOrPlayingShowAnim;

	/** Button that will close the targeting panel */
	UPROPERTY(meta = (BindWidgetOptional))
	UMyButton * Button_Cancel;
};
