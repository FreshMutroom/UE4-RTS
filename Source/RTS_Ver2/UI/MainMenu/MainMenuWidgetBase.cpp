// Fill out your copyright notice in the Description page of Project Settings.

#include "MainMenuWidgetBase.h"
#include "Animation/WidgetAnimation.h"

#include "UI/MainMenu/Menus.h"
#include "GameFramework/RTSGameInstance.h"
#include "Statics/DevelopmentStatics.h"

/* The "_INST" part doesn't need to be added in editor */
const FName UMainMenuWidgetBase::ExitAnimationName = FName("ExitAnim_INST");
const FName UMainMenuWidgetBase::EnterAnimationName = FName("EnterAnim_INST");
const FName UMainMenuWidgetBase::TransitionAnimationName = FName("TransitionAnim_INST");

UMainMenuWidgetBase::UMainMenuWidgetBase(const FObjectInitializer & ObjectInitializer) 
	: Super(ObjectInitializer)
{
#if WITH_EDITORONLY_DATA

	/* Populate the BindWidgetOptionalList.
	Note: if getting crashes here try moving logic to child ctor instead */
	for (TFieldIterator<UProperty> PropIt(GetClass()); PropIt; ++PropIt)
	{
		if (PropIt->HasMetaData("BindWidgetOptional"))
		{
			const FText & ValueText = FText::Format(NSLOCTEXT("MainMenuWidgetBase", "MapKey", "{0}{1}"), FText::FromString(PropIt->GetCPPTypeForwardDeclaration() + "\n\n"), PropIt->GetToolTipText());
			BindWidgetOptionalList.Emplace(PropIt->GetName(), ValueText);
		}
	}

#endif
}

bool UMainMenuWidgetBase::IsWidgetBound(UWidget * Widget) const
{
	return Widget != nullptr;
}

void UMainMenuWidgetBase::AssignAnimations()
{
	UProperty * Prop = GetClass()->PropertyLink;

	// Run through all properties of this class to find any widget animations
	while (Prop != nullptr)
	{
		// Only interested in object properties
		if (Prop->GetClass() == UObjectProperty::StaticClass())
		{
			UObjectProperty * ObjectProp = Cast<UObjectProperty>(Prop);

			// Only want the properties that are widget animations
			if (ObjectProp->PropertyClass == UWidgetAnimation::StaticClass())
			{
				UObject * Object = ObjectProp->GetObjectPropertyValue_InContainer(this);

				UWidgetAnimation * WidgetAnim = Cast<UWidgetAnimation>(Object);

				if (WidgetAnim != nullptr)
				{
					if (WidgetAnim->GetFName() == UMainMenuWidgetBase::ExitAnimationName)
					{
						ExitAnimation = WidgetAnim;
					}
					else if (WidgetAnim->GetFName() == UMainMenuWidgetBase::EnterAnimationName)
					{
						EnterAnimation = WidgetAnim;
					}
					else if (WidgetAnim->GetFName() == UMainMenuWidgetBase::TransitionAnimationName)
					{
						/* Either use exit anim + enter anim or use just transition anim */
						assert(ExitAnimation == nullptr && EnterAnimation == nullptr);

						TransitionAnimation = WidgetAnim;
						return;
					}
				}
			}
		}

		Prop = Prop->PropertyLinkNext;
	}
}

void UMainMenuWidgetBase::OnAnimationFinished_Implementation(const UWidgetAnimation * Animation)
{
	assert(Animation != nullptr);

	/* The forward playing check if because the same anim can be used for exit and enter. This
	apparently works for checking which one it is */
	if (Animation == ExitAnimation ||
		(Animation == TransitionAnimation && IsAnimationPlayingForward(Animation)))
	{
		GI->OnWidgetExitAnimFinished(this);
	}
	else if (Animation == EnterAnimation || Animation == TransitionAnimation)
	{
		GI->OnWidgetEnterAnimFinished(this);
	}
	else
	{
		UE_LOG(RTSLOG, Fatal, TEXT("UWidgetBase::OnAnimationFinished_Implementation(): Unrecognized animation"));
	}
}

#if WITH_EDITOR
const FText UMainMenuWidgetBase::GetPaletteCategory()
{
	return Menus::PALETTE_CATEGORY;
}

void UMainMenuWidgetBase::PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);
}
#endif // WITH_EDITOR

bool UMainMenuWidgetBase::Setup()
{
	if (!bHasBeenSetup)
	{
		GI = GetWorld()->GetGameInstance<URTSGameInstance>();
		AssignAnimations();
		bHasBeenSetup = true;
		return false;
	}

	return true;
}

void UMainMenuWidgetBase::OnShown()
{
}

void UMainMenuWidgetBase::UpdateMapDisplay(const FMapInfo & MapInfo)
{
	/* To be overridden */
	assert(0);
}

UWidgetAnimation * UMainMenuWidgetBase::GetExitAnimation() const
{
	return ExitAnimation;
}

UWidgetAnimation * UMainMenuWidgetBase::GetEnterAnimation() const
{
	return EnterAnimation;
}

bool UMainMenuWidgetBase::UsesOneTransitionAnimation() const
{
	return TransitionAnimation != nullptr;
}

UWidgetAnimation * UMainMenuWidgetBase::GetTransitionAnimation() const
{
	return TransitionAnimation;
}
