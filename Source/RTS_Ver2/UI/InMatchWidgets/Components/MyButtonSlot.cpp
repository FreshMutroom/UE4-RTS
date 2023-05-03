// Fill out your copyright notice in the Description page of Project Settings.

#include "MyButtonSlot.h"
#include "ObjectEditorUtils.h"

#include "UI/InMatchWidgets/Components/MySlateButton.h"
#include "UI/InMatchWidgets/Components/MyButton.h"


UMyButtonSlot::UMyButtonSlot(const FObjectInitializer & ObjectInitializer)
	: Super(ObjectInitializer)
{
	Padding = FMargin(4, 2);

	HorizontalAlignment = HAlign_Fill;
	VerticalAlignment = VAlign_Fill;
}

void UMyButtonSlot::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	Button.Reset();
}

void UMyButtonSlot::BuildSlot(TSharedRef<SMyButton> InButton)
{
	Button = InButton;

	Button->SetPadding(Padding);
	Button->SetHAlign(HorizontalAlignment);
	Button->SetVAlign(VerticalAlignment);

	Button->SetContent(Content ? Content->TakeWidget() : SNullWidget::NullWidget);
}

void UMyButtonSlot::SetPadding(FMargin InPadding)
{
	CastChecked<UMyButton>(Parent)->SetPadding(InPadding);
}

void UMyButtonSlot::SetHorizontalAlignment(EHorizontalAlignment InHorizontalAlignment)
{
	CastChecked<UMyButton>(Parent)->SetHorizontalAlignment(InHorizontalAlignment);
}

void UMyButtonSlot::SetVerticalAlignment(EVerticalAlignment InVerticalAlignment)
{
	CastChecked<UMyButton>(Parent)->SetVerticalAlignment(InVerticalAlignment);
}

void UMyButtonSlot::SynchronizeProperties()
{
	if (Button.IsValid())
	{
		SetPadding(Padding);
		SetHorizontalAlignment(HorizontalAlignment);
		SetVerticalAlignment(VerticalAlignment);
	}
}

#if WITH_EDITOR

void UMyButtonSlot::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	static bool IsReentrant = false;

	if (!IsReentrant)
	{
		IsReentrant = true;

		if (PropertyChangedEvent.Property)
		{
			static const FName PaddingName("Padding");
			static const FName HorizontalAlignmentName("HorizontalAlignment");
			static const FName VerticalAlignmentName("VerticalAlignment");

			FName PropertyName = PropertyChangedEvent.Property->GetFName();

			if (UMyButton * ParentButton = CastChecked<UMyButton>(Parent))
			{
				if (PropertyName == PaddingName)
				{
					FObjectEditorUtils::MigratePropertyValue(this, PaddingName, ParentButton, PaddingName);
				}
				else if (PropertyName == HorizontalAlignmentName)
				{
					FObjectEditorUtils::MigratePropertyValue(this, HorizontalAlignmentName, ParentButton, HorizontalAlignmentName);
				}
				else if (PropertyName == VerticalAlignmentName)
				{
					FObjectEditorUtils::MigratePropertyValue(this, VerticalAlignmentName, ParentButton, VerticalAlignmentName);
				}
			}
		}

		IsReentrant = false;
	}
}

#endif
