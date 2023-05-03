// Fill out your copyright notice in the Description page of Project Settings.


#include "RTSUnrealEdEngine.h"

#include "UI/EditorWidgets/EditorPlaySettingsWidget.h"
#include "Statics/DevelopmentStatics.h"


#define LOCTEXT_NAMESPACE "RTSUnrealEdEngine"

void URTSUnrealEdEngine::Init(IEngineLoop * InEngineLoop)
{
	Super::Init(InEngineLoop);

	CreateEditorPlaySettingsUtilityWidget();
}

void URTSUnrealEdEngine::CreateEditorPlaySettingsUtilityWidget()
{
	/* Check if user does not want the widget to be created now. By default it is always created, 
	but if user sets bAutoCreateRTSEditorPlaySettingsWidget=False then it will not be */
	FString CreateWidgetString = FString();
	GConfig->GetString(TEXT("/Script/Engine.Engine"), TEXT("bAutoCreateRTSEditorPlaySettingsWidget"), CreateWidgetString, GEngineIni);
	if (CreateWidgetString == "False")
	{
		return;
	}

	UClass * WidgetClass;
	FString WidgetClassName = FString();
	GConfig->GetString(TEXT("/Script/Engine.Engine"), TEXT("RTSEditorPlaySettingsWidget"), WidgetClassName, GEngineIni);
	if (WidgetClassName == FString())
	{
		/* If user has not specified a class then use the default class */
		WidgetClass = UEditorPlaySettingsWidget::StaticClass();
	}
	else
	{
		WidgetClass = StaticLoadClass(UEditorPlaySettingsWidget::StaticClass(), nullptr, *WidgetClassName);
		if (WidgetClass == nullptr)
		{
			/* If user mispelled it then use default class */
			WidgetClass = UEditorPlaySettingsWidget::StaticClass();
		}
	}
	
	assert(WidgetClass != nullptr);
	
	/* TODO Create the widget */
}

#undef LOCTEXT_NAMESPACE
