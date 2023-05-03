// Fill out your copyright notice in the Description page of Project Settings.

#include "Structs_7.h"


FKeyInfo::FKeyInfo()
	: DisplayText(FString("Default"))
	, PartialBrushText(FText::FromString("Default"))
{
	DefaultConstructFontInfo();
}

FKeyInfo::FKeyInfo(const FKey & InKey) 
	: DisplayText(InKey.ToString()) 
	, PartialBrushText(FText::FromString(InKey.ToString()))
{
	DefaultConstructFontInfo();
}

FKeyInfo::FKeyInfo(const FString & InDisplayName, const FString & InPartialText) 
	: DisplayText(InDisplayName) 
	, PartialBrushText(FText::FromString(InPartialText))
{
	DefaultConstructFontInfo();
}

void FKeyInfo::DefaultConstructFontInfo()
{
	PartialBrushTextFont.Size = 48;
}
