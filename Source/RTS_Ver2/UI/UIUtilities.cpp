// Fill out your copyright notice in the Description page of Project Settings.

#include "UIUtilities.h"

#include "Blueprint/UserWidget.h"
#include "Animation/WidgetAnimation.h"


UWidgetAnimation * UIUtilities::FindWidgetAnim(UUserWidget * Widget, const FName & WidgetAnimName)
{
	UProperty * Prop = Widget->GetClass()->PropertyLink;

	// Run through all properties of this class to find any widget animations
	while (Prop != nullptr)
	{
		// Only interested in object properties
		if (Prop->GetClass() == UObjectProperty::StaticClass())
		{
			UObjectProperty * ObjectProp = CastChecked<UObjectProperty>(Prop);

			// Only want the properties that are widget animations
			if (ObjectProp->PropertyClass == UWidgetAnimation::StaticClass())
			{
				UObject * Object = ObjectProp->GetObjectPropertyValue_InContainer(Widget);

				// This null check is needed, did not expect that
				if (Object != nullptr && Object->GetFName() == WidgetAnimName)
				{
					return CastChecked<UWidgetAnimation>(Object);
				}
			}
		}

		Prop = Prop->PropertyLinkNext;
	}

	return nullptr;
}
