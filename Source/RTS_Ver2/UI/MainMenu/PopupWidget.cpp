// Fill out your copyright notice in the Description page of Project Settings.

#include "PopupWidget.h"
#include "Components/TextBlock.h"
#include "Animation/WidgetAnimation.h"

const FString UPopupWidget::ShowAnimFullName = FString("ShowAnim_INST");
const FString UPopupWidget::HideAnimFullName = FString("HideAnim_INST");
const FString UPopupWidget::SingleAnimFullName = FString("SingleAnim_INST");

UPopupWidget::UPopupWidget(const FObjectInitializer & ObjectInitializer)
	: Super(ObjectInitializer)
{
	AnimStatus = EWidgetAnimStatus::NoAnims;

#if WITH_EDITORONLY_DATA
	AnimNames.Emplace(*ShowAnimFullName.LeftChop(5));
	AnimNames.Emplace(*HideAnimFullName.LeftChop(5));
	AnimNames.Emplace(*SingleAnimFullName.LeftChop(5));
#endif
}

void UPopupWidget::Init()
{
	OriginalTextColor = Text_Message->ColorAndOpacity;

	/* Init() is called on first request to show one of these, may want to call Init() during 
	GI initialization or something */
	AssignWidgetAnims();
}

void UPopupWidget::AssignWidgetAnims()
{	
	// When BindAnimationOptional support drops can change this code to just assign AnimStatus
	
	bool bFoundShowAnim = false;
	bool bFoundHideAnim = false;
	
	UProperty * Prop = GetClass()->PropertyLink;

	while (Prop != nullptr)
	{
		if (Prop->GetClass() == UObjectProperty::StaticClass())
		{
			UObjectProperty * ObjectProp = Cast<UObjectProperty>(Prop);

			if (ObjectProp->PropertyClass == UWidgetAnimation::StaticClass())
			{
				UObject * Object = ObjectProp->GetObjectPropertyValue_InContainer(this);

				if (Object != nullptr)
				{
					UWidgetAnimation * WidgetAnim = CastChecked<UWidgetAnimation>(Object);

					if (WidgetAnim->GetFName() == *UPopupWidget::SingleAnimFullName)
					{
						AnimStatus = EWidgetAnimStatus::SingleAnim;
						SingleAnim = WidgetAnim;
						
						// Null these for GC purposes if it matters
						ShowAnim = HideAnim = nullptr;

						return;
					}
					else if (WidgetAnim->GetFName() == *UPopupWidget::ShowAnimFullName)
					{
						bFoundShowAnim = true;
						ShowAnim = WidgetAnim;

						if (bFoundHideAnim)
						{
							AnimStatus = EWidgetAnimStatus::ShowAndHideAnims;
							return;
						}
					}
					else if (WidgetAnim->GetFName() == *UPopupWidget::HideAnimFullName)
					{
						bFoundHideAnim = true;
						HideAnim = WidgetAnim;

						if (bFoundShowAnim)
						{
							AnimStatus = EWidgetAnimStatus::ShowAndHideAnims;
							return;
						}
					}
				}
			}
		}

		Prop = Prop->PropertyLinkNext;
	}

	assert(!(bFoundShowAnim && bFoundHideAnim));

	if (bFoundShowAnim)
	{
		AnimStatus = EWidgetAnimStatus::ShowAnimOnly;
	}
	else if (bFoundHideAnim)
	{
		AnimStatus = EWidgetAnimStatus::HideAnimOnly;
	}
}

void UPopupWidget::TimeToHide()
{
	if (AnimStatus == EWidgetAnimStatus::ShowAndHideAnims
		|| AnimStatus == EWidgetAnimStatus::HideAnimOnly)
	{
		PlayAnimation(HideAnim);
	}
	else
	{
		HideInternal();
	}
}

void UPopupWidget::HideInternal()
{
	RemoveFromViewport();
}

void UPopupWidget::Delay(FTimerHandle & TimerHandle, void(UPopupWidget::* Function)(), float Delay)
{
	assert(Delay > 0.f);
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, Function, Delay, false);
}

void UPopupWidget::OnAnimationFinished_Implementation(const UWidgetAnimation * Animation)
{
	if (Animation == ShowAnim)
	{
		if (ShowDuration > 0.f)
		{
			Delay(TimerHandle_Hide, &UPopupWidget::TimeToHide, ShowDuration);
		}
		else
		{
			TimeToHide();
		}
	}
	else if (Animation == HideAnim || Animation == SingleAnim)
	{
		HideInternal();
	}
}

void UPopupWidget::Show(const FText & Message, float Duration, bool bSetTextColor,
	const FLinearColor & TextColor)
{
	Text_Message->SetText(Message);

	if (bSetTextColor)
	{
		Text_Message->SetColorAndOpacity(TextColor);
	}
	else
	{
		// Use color specified in editor
		Text_Message->SetColorAndOpacity(OriginalTextColor);
	}

	// Play anim
	if (AnimStatus == EWidgetAnimStatus::NoAnims 
		|| AnimStatus == EWidgetAnimStatus::HideAnimOnly)
	{
		// No anim to play
		if (Duration > 0.f)
		{
			Delay(TimerHandle_Hide, &UPopupWidget::TimeToHide, Duration);
		}
		else
		{
			// Call func now
			TimeToHide();
		}
	}
	else if (AnimStatus == EWidgetAnimStatus::ShowAnimOnly 
		|| AnimStatus == EWidgetAnimStatus::ShowAndHideAnims)
	{
		ShowDuration = Duration;
		
		const float CurrentAnimTime  = GetAnimationCurrentTime(ShowAnim);
		float StartTime = CurrentAnimTime;

		if (CurrentAnimTime > AnimNoReplayTime)
		{
			StartTime = AnimNoReplayTime;
		}
			
		PlayAnimation(ShowAnim, StartTime);
	}
	else // Assumed EWidgetAnimStatus::SingleAnim
	{
		const float CurrentAnimTime = GetAnimationCurrentTime(ShowAnim);
		float StartTime = CurrentAnimTime;

		if (CurrentAnimTime > AnimNoReplayTime)
		{
			StartTime = AnimNoReplayTime;
		}

		PlayAnimation(SingleAnim, StartTime);
	}

	// UINT8_MAX + 1 to ensure to goes above all menu widgets 
	AddToViewport(UINT8_MAX + 1);
}

void UPopupWidget::Hide()
{
	StopAllAnimations();
	
	RemoveFromViewport();
}
