// Fill out your copyright notice in the Description page of Project Settings.

#include "BuffAndDebuffWidgets.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Border.h"

#include "Settings/ProjectSettings.h"
#include "Statics/DevelopmentStatics.h"
#include "Statics/Structs_1.h"
#include "Statics/Structs_2.h"
#include "GameFramework/RTSGameInstance.h"


USingleBuffOrDebuffWidget::USingleBuffOrDebuffWidget(const FObjectInitializer & ObjectInitializer)
	: Super(ObjectInitializer)
{
	PopulateBindWidgetOptionalList();
}

void USingleBuffOrDebuffWidget::PopulateBindWidgetOptionalList()
{
#if WITH_EDITORONLY_DATA
	
	/* Populate BindWidgetOptionalList */
	for (TFieldIterator<UProperty> PropIt(GetClass()); PropIt; ++PropIt)
	{
		if (PropIt->HasMetaData("BindWidgetOptional"))
		{
			/* The key entry will be the widget's class plus it's description */
			const FText & ValueText = FText::Format(NSLOCTEXT("USingleBuffOrDebuffWidget", "MapKey", "{0}{1}"), FText::FromString(PropIt->GetCPPTypeForwardDeclaration() + "\n\n"), PropIt->GetToolTipText());
			BindWidgetOptionalList.Emplace(PropIt->GetName(), ValueText);
		}
	}

#endif
}

FText USingleBuffOrDebuffWidget::GetDisplayTextForDuration(float Duration)
{	
	/* Duration remaining should never be negative */
	assert(Duration >= 0.f);

	// ERoundingMode::ToNegativeInfinity for rounding down
	// ERoundingMode::ToPositiveInfinity for rounding up
	// ERoundingMode::HalfToZero for rounding to nearest

	// https://wiki.unrealengine.com/Float_as_String_With_Precision
	
	//int32 Precision = (Method == EFloatDisplayMethod::ZeroDP_RoundDown)
	//	|| (Method == EFloatDisplayMethod::ZeroDP_RoundUp)
	//	|| (Method == EFloatDisplayMethod::ZeroDP_RoundToNearest) ? 0 : 1;
	//
	///* Not 100% what this is doing. Will see if I can get away with leaving it out */
	//float Rounded = roundf(Duration);
	//if (FMath::Abs(Duration - Rounded) < FMath::Pow(10, -1 * Precision))
	//{ 
	//	Duration = Rounded;
	//} 

	FloatToText(Duration, BuffAndDebuffOptions::DurationDisplayMethod);
}

bool USingleBuffOrDebuffWidget::IsWidgetBound(UWidget * InWidget)
{
	return InWidget != nullptr;
}

void USingleBuffOrDebuffWidget::SetupFor(const FStaticBuffOrDebuffInstanceInfo & StateInfo, 
	URTSGameInstance * GameInst)
{
	const FStaticBuffOrDebuffInfo * InfoStruct = StateInfo.GetInfoStruct();
	
	if (IsWidgetBound(Text_Name))
	{
		Text_Name->SetText(InfoStruct->GetDisplayName());
	}

	if (IsWidgetBound(Image_Icon))
	{
		// Check for null here and avoid showing white image? Make invis instead
		Image_Icon->SetBrush(InfoStruct->GetDisplayImage());
	}

	if (IsWidgetBound(Border_SubType))
	{
		const FBuffOrDebuffSubTypeInfo & SubTypeInfo = GameInst->GetBuffOrDebuffSubTypeInfo(InfoStruct->GetSubType());
		Border_SubType->SetBrushFromTexture(SubTypeInfo.GetTexture());
		Border_SubType->SetBrushColor(SubTypeInfo.GetBrushColor());
	}
}

void USingleTickableBuffOrDebuffWidget::SetupFor(const FTickableBuffOrDebuffInstanceInfo & Info, 
	URTSGameInstance * GameInst)
{
	const FTickableBuffOrDebuffInfo * InfoStruct = Info.GetInfoStruct();
	
	if (IsWidgetBound(Text_Name))
	{
		Text_Name->SetText(InfoStruct->GetDisplayName());
	}

	if (IsWidgetBound(Image_Icon))
	{
		// Check for null here and avoid showing white image? Make invis instead
		Image_Icon->SetBrushFromTexture(InfoStruct->GetDisplayImage());
	}

	if (IsWidgetBound(Border_SubType))
	{
		const FBuffOrDebuffSubTypeInfo & SubTypeInfo = GameInst->GetBuffOrDebuffSubTypeInfo(InfoStruct->GetSubType());
		Border_SubType->SetBrushFromTexture(SubTypeInfo.GetTexture());
		Border_SubType->SetBrushColor(SubTypeInfo.GetBrushColor());
	}

	const bool bInfiniteDuration = !InfoStruct->ExpiresOverTime();
	float DurationRemaining = -1.f;

	if (bInfiniteDuration)
	{
		/* Setting 0 here for infinite not a rerquirement but will do it anyway */
		TotalDuration = 0.f;
	}
	else
	{
		TotalDuration = InfoStruct->GetFullDuration();
		DurationRemaining = Info.CalculateDurationRemaining();
	}

	if (IsWidgetBound(Text_DurationRemaining))
	{
		const FText Text = bInfiniteDuration ? FText::GetEmpty() : GetDisplayTextForDuration(DurationRemaining);
		
		Text_DurationRemaining->SetText(Text);
	}

	if (IsWidgetBound(ProgressBar_PercentageRemaining))
	{
		const float Percent = bInfiniteDuration ? 0.f : (DurationRemaining / TotalDuration);
		
		ProgressBar_PercentageRemaining->SetPercent(Percent);
	}
}

void USingleTickableBuffOrDebuffWidget::SetDurationRemaining(float DurationRemaining)
{
	/* 0 implies an infinite duration buff/debuff and we shouldn't be calling this if that 
	is the case */
	assert(TotalDuration != 0.f);
	
	if (IsWidgetBound(Text_DurationRemaining))
	{
		Text_DurationRemaining->SetText(GetDisplayTextForDuration(DurationRemaining));
	}

	if (IsWidgetBound(ProgressBar_PercentageRemaining))
	{
		ProgressBar_PercentageRemaining->SetPercent(DurationRemaining / TotalDuration);
	}
}
