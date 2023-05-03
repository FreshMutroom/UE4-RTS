// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Editor/UnrealEdEngine.h"
#include "RTSUnrealEdEngine.generated.h"


/**
 *	Custom editor engine. 
 *	I added this class so important editor utility widgets are automatically created.
 *
 *	My notes:
 *	I tried making this class a part of my RTS_Ver2Editor module but it wouldn't load. 
 *	Not sure if there's something I'm missing or if that's just not allowed in the first place.
 */
UCLASS()
class RTS_VER2_API URTSUnrealEdEngine : public UUnrealEdEngine
{
	GENERATED_BODY()
	
public:

	virtual void Init(IEngineLoop * InEngineLoop) override;

protected:

	void CreateEditorPlaySettingsUtilityWidget();

	//------------------------------------------------------------------
	//	Data
	//------------------------------------------------------------------

	//UEditorPlaySettingsWidget * EditorPlaySettingsWidget;
};
