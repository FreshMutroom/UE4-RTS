// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/InGameWidgetBase.h"
#include "GlobalSkillsPanelButton.generated.h"

class UMyButton;
struct FAquiredCommanderAbilityState;
class UProgressBar;


/* What kind of ability is assigned to this button */
UENUM()
enum class EAssignedAbilityType : uint8
{
	CommanderSkill,
};


/**
 *	A single button on the global skills panel. 
 *
 *	Made this a UUserWidget so I could get animations to play on it because I want a animation 
 *	for when the button becomes active.
 */
UCLASS(Abstract)
class RTS_VER2_API UGlobalSkillsPanelButton : public UInGameWidgetBase
{
	GENERATED_BODY()

public:

	UGlobalSkillsPanelButton(const FObjectInitializer & ObjectInitializer);

	virtual bool SetupWidget(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController) override;

	void OnCommanderSkillAquired_FirstRank(FAquiredCommanderAbilityState * AquiredAbilityInfo, int32 PanelArrayIndex);
	void OnCommanderSkillAquired_NotFirstRank();

	void UpdateCooldownProgress(const FTimerManager & TimerManager);

	/* Called when the ability this button is for uses its last charge */
	void OnLastChargeUsed();

	/* Get the state info for the ability this widget is displaying info for */
	const FAquiredCommanderAbilityState * GetAbilityState() const;

protected:

	virtual void NativeTick(const FGeometry & MyGeometry, float InDeltaTime) override;

	void SetAppearanceForNotOutOfCharges();
	void SetAppearanceForOutOfCharges();

	void UIBinding_OnLMBPress();
	void UIBinding_OnLMBReleased();
	void UIBinding_OnRMBPress();
	void UIBinding_OnRMBReleased();

	//------------------------------------------------------
	//	------- Data -------
	//------------------------------------------------------

	/* If you name a widget animation this then it will play when the button becomes active */
	UPROPERTY(VisibleAnywhere, Category = "RTS", meta = (DisplayName = "EnterAnimation"))
	FString ABC;

	static const FName EnterAnimFName;

	/* Animation to play when the button becomes active */
	UWidgetAnimation * EnterAnim;

	/** Kind of pivital to this whole widget; shouldn't really be optional */
	UPROPERTY(meta = (BindWidgetOptional))
	UMyButton * Button_Button;

	/** Progress bar to show the cooldown remaining of the ability */
	UPROPERTY(meta = (BindWidgetOptional))
	UProgressBar * ProgressBar_Cooldown;

	float OriginalOpacity;

	/* What to multiply render opacity by when the button's ability is out of charges */
	UPROPERTY(EditAnywhere, Category = "RTS")
	float OutOfChargesOpacityMultiplier;

private:

	/* The assigned commander ability info */
	FAquiredCommanderAbilityState * AssignedCommanderAbilityInfo;
};
