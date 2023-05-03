// Fill out your copyright notice in the Description page of Project Settings.

#include "InGameWidgetBase.h"

#include "GameFramework/RTSGameInstance.h"
#include "GameFramework/RTSPlayerController.h"
#include "Statics/DevelopmentStatics.h"

UInGameWidgetBase::UInGameWidgetBase(const FObjectInitializer & ObjectInitializer)
	: Super(ObjectInitializer)
{
	/* Should disable just the optional blueprint implementable tick. Now apparently
	undefined in 4.20. Looks like user widgets have tick disabled by default now. May wanna
	confirm this */
	//bCanEverTick = false;

#if WITH_EDITORONLY_DATA
	
	/* Populate the BindWidgetOptionalList. 
	Note: if getting crashes here try moving logic to child ctor instead */
	for (TFieldIterator<UProperty> PropIt(GetClass()); PropIt; ++PropIt)
	{
		if (PropIt->HasMetaData("BindWidgetOptional"))
		{
			/* The key entry will be the widget's class plus it's description */
			const FText & ValueText = FText::Format(NSLOCTEXT("InGameWidgetBase", "MapKey", "{0}{1}"), FText::FromString(PropIt->GetCPPTypeForwardDeclaration() + "\n\n"), PropIt->GetToolTipText());
			BindWidgetOptionalList.Emplace(PropIt->GetName(), ValueText);
		}
	}

#endif
}

bool UInGameWidgetBase::SetupWidget(URTSGameInstance * InGameInstance, ARTSPlayerController * InPlayerController)
{
	if (!bHasSetup)
	{
		bHasSetup = true;

		GI = InGameInstance == nullptr ? CastChecked<URTSGameInstance>(GetWorld()->GetGameInstance()) : InGameInstance;
		PC = InPlayerController == nullptr ? CastChecked<ARTSPlayerController>(GetWorld()->GetFirstPlayerController()) : InPlayerController;
		GS = PC->GetGS();
		PS = PC->GetPS();
		FI = PC->GetFactionInfo();
		assert(PC != nullptr);
		assert(PS != nullptr);
		assert(FI != nullptr);

		return false;
	}
	else
	{
		return true;
	}
}

bool UInGameWidgetBase::RespondToEscapeRequest(URTSHUD * HUD)
{
	// To be overridden by child classes
	// SetVisibility(ESlateVisibility::Hidden); is a posibility for a default implementation 
	// but really I think it will cause more headaches
	assert(0);
	return false;
}

void UInGameWidgetBase::NativeTick(const FGeometry & MyGeometry, float InDeltaTime)
{
	// No Super call here - it just updates widget animations and latent actions
	GInitRunaway();
}

const TScriptInterface<ISelectable>& UInGameWidgetBase::GetCurrentSelected() const
{
	return CurrentSelected;
}

bool UInGameWidgetBase::IsWidgetBound(UWidget * Widget)
{
	return Widget != nullptr;
}
