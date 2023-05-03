// Fill out your copyright notice in the Description page of Project Settings.

#include "MenuOutputWidget.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"

#include "UI/UIUtilities.h"
#include "Statics/DevelopmentStatics.h"
#include "GameFramework/RTSGameInstance.h"


//-------------------------------------------------------------------------------------------
//===========================================================================================
//	------- Single message widget -------
//===========================================================================================
//-------------------------------------------------------------------------------------------

const FName USingleMenuOutputMessageWidget::ShowAnimName = FName("ShowAnimation_INST");

USingleMenuOutputMessageWidget::USingleMenuOutputMessageWidget(const FObjectInitializer & ObjectInitializer)
	: Super(ObjectInitializer)
{
	ShowAnimLatestStartPoint = 0.f;
}

void USingleMenuOutputMessageWidget::InitialSetup(UMenuOutputWidget * InOwningWidget)
{
	OwningWidget = InOwningWidget;
	ShowAnim = UIUtilities::FindWidgetAnim(this, ShowAnimName);

	UE_CLOG(ShowAnim == nullptr, RTSLOG, Fatal, TEXT("Widget [%s] needs an animation named "
		"ShowAnimation on it"), *GetClass()->GetName());
}

void USingleMenuOutputMessageWidget::NativeTick(const FGeometry & MyGeometry, float InDeltaTime)
{	
	// If this ensure is hit it is likely UpdateCanTick as not called somewhere
	if (ensureMsgf(GetDesiredTickFrequency() != EWidgetTickFrequency::Never, TEXT("SObjectWidget and UUserWidget have mismatching tick states or UUserWidget::NativeTick was called manually (Never do this)")))
	{
		GInitRunaway();

		TickActionsAndAnimation(MyGeometry, InDeltaTime);
	}
}

void USingleMenuOutputMessageWidget::SetupFor(URTSGameInstance * GameInst, EGameWarning WarningType, 
	const FGameWarningInfo & WarningInfo)
{
	Text_Message->SetText(WarningInfo.GetMessage());
}

void USingleMenuOutputMessageWidget::Show()
{
	float AnimStartTime = GetAnimationCurrentTime(ShowAnim);
	if (AnimStartTime > ShowAnimLatestStartPoint)
	{
		AnimStartTime = ShowAnimLatestStartPoint;
	}

	PlayAnimation(ShowAnim, AnimStartTime);
}

void USingleMenuOutputMessageWidget::OnAnimationFinished_Implementation(const UWidgetAnimation * Animation)
{	
	Super::OnAnimationFinished_Implementation(Animation);

	// If I ever add more animations to this widget might want to do a if (Animation == ShowAnim)
	// statement here first
	OwningWidget->OnWarningMessageExpired(this);
}


//-------------------------------------------------------------------------------------------
//===========================================================================================
//	------- Main widget -------
//===========================================================================================
//-------------------------------------------------------------------------------------------

UMenuOutputWidget::UMenuOutputWidget(const FObjectInitializer & ObjectInitializer)
	: Super(ObjectInitializer)
{
	NumWarningMessages = 0;
	MaxNumWarningMessages = 1;
}

void UMenuOutputWidget::InitialSetup(ARTSPlayerController * InPlayerController)
{
	PC = InPlayerController;
	
	if (IsWidgetBound(Panel_Warnings_Inner))
	{
		UE_CLOG(SingleWarningMessage_BP == nullptr, RTSLOG, Fatal, TEXT("Single warning message widget "
			"not set for [%s] "), *GetClass()->GetName());
		
		WarningMessages.Reserve(MaxNumWarningMessages);
		for (int32 i = 0; i < MaxNumWarningMessages; ++i)
		{
			USingleMenuOutputMessageWidget * Widget = CreateWidget<USingleMenuOutputMessageWidget>(PC, SingleWarningMessage_BP);
			Widget->InitialSetup(this);
			WarningMessages.Emplace(Widget);
			Panel_Warnings_Inner->AddChild(Widget);
		}
	}
}

void UMenuOutputWidget::NativeTick(const FGeometry & MyGeometry, float InDeltaTime)
{	
	GInitRunaway();
}

void UMenuOutputWidget::ShowWarningMessageAndPlaySound(URTSGameInstance * GameInst, EGameWarning WarningType)
{
	const FGameWarningInfo & WarningInfo = GameInst->GetGameWarningInfo(WarningType);

	PlaySound(WarningInfo.GetSound());

	// Show message
	if (IsWidgetBound(Panel_Warnings_Outer))
	{
		if (IsWidgetBound(Panel_Warnings_Inner))
		{
			USingleMenuOutputMessageWidget * Widget = AddSingleWarningMessageWidget();
			Widget->SetupFor(GameInst, WarningType, WarningInfo);
			Widget->Show();
		}

		Panel_Warnings_Outer->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else if (IsWidgetBound(Panel_Warnings_Inner))
	{
		USingleMenuOutputMessageWidget * Widget = AddSingleWarningMessageWidget();
		Widget->SetupFor(GameInst, WarningType, WarningInfo);
		Widget->Show();

		Panel_Warnings_Inner->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UMenuOutputWidget::OnWarningMessageExpired(USingleMenuOutputMessageWidget * ExpiredMessage)
{
	NumWarningMessages--;
	if (NumWarningMessages == 0)
	{
		if (IsWidgetBound(Panel_Warnings_Outer))
		{
			Panel_Warnings_Outer->SetVisibility(ESlateVisibility::Hidden);
		}
		else
		{
			Panel_Warnings_Inner->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	if (AreAnyMessagesShowing() == false)
	{
		// Hide widget to disable its tick
		SetVisibility(ESlateVisibility::Hidden);
	}
}

USingleMenuOutputMessageWidget * UMenuOutputWidget::AddSingleWarningMessageWidget() 
{
	/* This function is pretty much written with the assumption that only one message can
	be displayed at a time */

	// This widget is hidden when not displaying anything so do this
	SetVisibility(ESlateVisibility::HitTestInvisible);

	// Index in array to put new message
	int32 Index;
	if (NumWarningMessages == MaxNumWarningMessages)
	{
		Index = MaxNumWarningMessages - 1;
	}
	else
	{
		Index = NumWarningMessages++;
	}
	
	return WarningMessages[Index];
}

bool UMenuOutputWidget::AreAnyMessagesShowing() const
{
	// Just warnings cause that's all I show on this widget right now
	return NumWarningMessages > 0;
}
