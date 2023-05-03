// Fill out your copyright notice in the Description page of Project Settings.

#include "Workarounds.h"
#if WITH_EDITOR
#include "AssetToolsModule.h"
#include "PackageTools.h"
#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Logging/MessageLog.h"
#endif


#if WITH_EDITOR

//@See UBlueprintMaterialTextureNodesBPLibrary::CreateMIC_EditorOnly
UMaterialInstanceConstant * Workarounds::CreateMaterialInstanceConstant(UMaterialInterface * Material, FString InName)
{
#if WITH_EDITOR
	TArray<UObject*> ObjectsToSync;

	if (Material != nullptr)
	{
		// Create an appropriate and unique name 
		FString Name;
		FString PackageName;
		IAssetTools& AssetTools = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

		//Use asset name only if directories are specified, otherwise full path
		if (!InName.Contains(TEXT("/")))
		{
			FString AssetName = Material->GetOutermost()->GetName();
			const FString SanitizedBasePackageName = PackageTools::SanitizePackageName(AssetName);
			const FString PackagePath = FPackageName::GetLongPackagePath(SanitizedBasePackageName) + TEXT("/");
			AssetTools.CreateUniqueAssetName(PackagePath, InName, PackageName, Name);
		}
		else
		{
			InName.RemoveFromStart(TEXT("/"));
			InName.RemoveFromStart(TEXT("Content/"));
			InName.StartsWith(TEXT("Game/")) == true ? InName.InsertAt(0, TEXT("/")) : InName.InsertAt(0, TEXT("/Game/"));
			AssetTools.CreateUniqueAssetName(InName, TEXT(""), PackageName, Name);
		}

		UMaterialInstanceConstantFactoryNew* Factory = NewObject<UMaterialInstanceConstantFactoryNew>();
		Factory->InitialParent = Material;

		UObject* NewAsset = AssetTools.CreateAsset(Name, FPackageName::GetLongPackagePath(PackageName), UMaterialInstanceConstant::StaticClass(), Factory);

		ObjectsToSync.Add(NewAsset);
		GEditor->SyncBrowserToObjects(ObjectsToSync);

		UMaterialInstanceConstant* MIC = Cast<UMaterialInstanceConstant>(NewAsset);

		return MIC;
	}
	FMessageLog("Blueprint").Warning(NSLOCTEXT("BlueprintMaterialTextureLibrary", "CreateMIC_InvalidMaterial", "CreateMIC_EditorOnly: Material must be non-null."));
#else
	FMessageLog("Blueprint").Error(NSLOCTEXT("BlueprintMaterialTextureLibrary", "CreateMIC_CannotBeCreatedAtRuntime", "CreateMIC: Can't create MIC at run time."));
#endif
	return nullptr;
}

#endif // WITH_EDITOR
