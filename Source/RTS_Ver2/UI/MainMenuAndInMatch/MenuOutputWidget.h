// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MenuOutputWidget.generated.h"

enum class EGameWarning : uint8;
class UTextBlock;
class ARTSPlayerController;
class URTSGameInstance;
class UUMGSequencePlayer;
class UMenuOutputWidget;
struct FGameWarningInfo;


/** 
 *	A widget that shows a single message to be displayed on the menu output widget. 
 *	You must create a widget anim for this. The anim decides how long the message is shown for. 
 */
UCLASS(Abstract)
class RTS_VER2_API USingleMenuOutputMessageWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	USingleMenuOutputMessageWidget(const FObjectInitializer & ObjectInitializer);
	void InitialSetup(UMenuOutputWidget * InOwningWidget);

protected:

	virtual void NativeTick(const FGeometry & MyGeometry, float InDeltaTime) override;

public:

	void SetupFor(URTSGameInstance * GameInst, EGameWarning WarningType, const FGameWarningInfo & WarningInfo);
	void Show();

protected:

	virtual void OnAnimationFinished_Implementation(const UWidgetAnimation * Animation) override;

	//---------------------------------------------------
	//	Data
	//---------------------------------------------------

	UMenuOutputWidget * OwningWidget;

	UPROPERTY(VisibleAnywhere, Category = "RTS", meta = (DisplayName = "ShowAnimation"))
	FString ABC;

	static const FName ShowAnimName;

	UWidgetAnimation * ShowAnim;

	/** 
	 *	This is the point in ShowAnimation where the animation will play from if the time 
	 *	it has spent playing the anim is past this point. 
	 *	e.g. suppose your anim has a 1 sec fade in at the start, then shows the message for 5 sec, 
	 *	then disappears. If another message comes in and the anim is already 0.5 sec through then 
	 *	we play the anim from 0.5 sec instead of restarting it back at 0. That way if a lot of messages 
	 *	come in fast then the fade in won't play from the start for each one and you'll actually get 
	 *	to see your message instead of sometimes seeing nothing 
	 */
	UPROPERTY(EditAnywhere, Category = "RTS", meta = (ClampMin = 0))
	float ShowAnimLatestStartPoint;

	UPROPERTY(meta = (BindWidget))
	UTextBlock * Text_Message;
};


/**
 *	Shows messages such as 'you cannot rebind to that key'
 *
 *	My notes:
 *	I have written this class with the assumption that UUserWidgets with their tick frequency set 
 *	Auto may consume CPU cycles each tick. This means I will not spawn all the single message 
 *	widgets required in InitialSetup and instead will create and destroy them as messages 
 *	come and go.
 */
UCLASS(Abstract)
class RTS_VER2_API UMenuOutputWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:

	UMenuOutputWidget(const FObjectInitializer & ObjectInitializer);
	
	void InitialSetup(ARTSPlayerController * InPlayerController);

protected:

	virtual void NativeTick(const FGeometry & MyGeometry, float InDeltaTime) override;

public:

	void ShowWarningMessageAndPlaySound(URTSGameInstance * GameInst, EGameWarning WarningType);
	void OnWarningMessageExpired(USingleMenuOutputMessageWidget * ExpiredMessage);

protected:

	USingleMenuOutputMessageWidget * AddSingleWarningMessageWidget();

	/* Return whether any messages are showing. Warnings, anything else etc */
	bool AreAnyMessagesShowing() const;

	static bool IsWidgetBound(UWidget * Widget) { return Widget != nullptr; }

	//-----------------------------------------------------
	//	Data
	//-----------------------------------------------------

	ARTSPlayerController * PC;

	/** 
	 *	This will not have any message widgets added 
	 *	to it. It is here in case you want a background or something to your message box. 
	 *	It should have Panel_Warnings_Inner as a child. 
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	UPanelWidget * Panel_Warnings_Outer;

	/** Widget that displays warning messages */
	UPROPERTY(meta = (BindWidgetOptional))
	UPanelWidget * Panel_Warnings_Inner;

	/* Widget to use for single warning messages */
	UPROPERTY(EditAnywhere, Category = "RTS", meta = (DisplayName = "Single Warning Message Widget"))
	TSubclassOf<USingleMenuOutputMessageWidget> SingleWarningMessage_BP;

	/* Array of warning message widgets. Probably a replica of widgets in 
	Panel_Warnings_Inner->Slots */
	TArray<USingleMenuOutputMessageWidget *> WarningMessages;

	/* Number of warning messages this widget is displaying */
	int32 NumWarningMessages;

	/** 
	 *	Maximum number of warning messages that can be shown. 
	 *	MaxNumWarningMessages user widget are created at setup time so don't set this to insanely 
	 *	high numbers 
	 */
	UPROPERTY(EditAnywhere, Category = "RTS", meta = (ClampMin = 1, ClampMax = 1))
	int32 MaxNumWarningMessages;
};
