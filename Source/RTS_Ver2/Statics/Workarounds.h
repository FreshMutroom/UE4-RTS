// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class UMaterialInstanceConstant;
class UMaterialInterface;


/**
 *	A file that contains copies of already created functions
 */
class RTS_VER2_API Workarounds
{

public:

#if WITH_EDITOR
	
	/** 
	 *	Create a UMaterialInstanceConstant from a material. Duplicate of 
	 *	UBlueprintMaterialTextureNodesBPLibrary::CreateMIC_EditorOnly but I cannot get code to 
	 *	compile with its include file included 
	 *	
	 *	The material instance will be in the same content folder as the param material
	 *
	 *	Description from original function:
	 *	Creates a new Material Instance Constant asset
	 *	Only works in the editor
	 */
	static UMaterialInstanceConstant * CreateMaterialInstanceConstant(UMaterialInterface * Material, 
		FString InName = "MIC_");

#endif // WITH_EDITOR

};
