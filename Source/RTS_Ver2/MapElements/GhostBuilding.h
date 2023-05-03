// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Statics/CommonEnums.h"
#include "GhostBuilding.generated.h"

class USkeletalMeshComponent;
class UBoxComponent;
class UMaterialInstanceDynamic;
struct FBuildingInfo;


/* TODO since these are now pooled check if defense component collision is enabled, would not be
good to have them eating up CPU time each tick */

/** 
 *	Ghost buildings are the buildings spawned when trying to select a location to build a 
 *	building. They are only visible to the owning player. 
 */
UCLASS(NotPlaceable)
class RTS_VER2_API AGhostBuilding : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AGhostBuilding();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:

	/* Used only to get correct positioning of mesh component  */
	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
	UBoxComponent * Bounds;

	/* Mesh */
	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
	USkeletalMeshComponent * Mesh;

#if WITH_EDITORONLY_DATA
	/* A static mesh that will be assigned a default engine shape to make default ghost as 
	good as possible for 'works out of the box' functionality. If user sets a skeletal mesh
	then this will be made invisible */
	UPROPERTY()
	UStaticMeshComponent * EditorOnlyMesh;
#endif

private:

	/* Type of building this is for. For a long time this was an editor exposed variable but 
	is now set automatically */
	EBuildingType Type;

	/* Whether current location is a buildable location or not */
	bool bIsAtBuildableLocation;

	/** Whether to override the param name specified in game instance */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (InlineEditConditionToggle))
	bool bOverrideBadLocationParamName;

	/** 
	 *	Optional override of name in game instance.
	 *	The name of the vector param on each of the mesh's materials to modify when the ghost
	 *	is in a suitable/non-suitable build location e.g. in C&C generals the building will change
	 *	to a red color when not in a suitable build location 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bOverrideBadLocationParamName, DisplayName = "Bad Location Param Name Override"))
	FName BadLocationParamName;

	/** 
	 *	Whether to override the value set in game instance to set bad location param to when in a
	 *	non-suitable build location 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (InlineEditConditionToggle))
	bool bOverrideBadLocationParamValue;

	/** 
	 *	Optional override of value set in game instance
	 *	Value to set param to
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bOverrideBadLocationParamValue, DisplayName = "Bad Location Param Value Override"))
	FLinearColor BadLocationParamValue;

	/* Will hold all the mesh's materials as dynamic material instances. We will modify these
	based on whether the ghost is at a suitable build location or not */
	UPROPERTY()
	TArray < UMaterialInstanceDynamic * > DynamicMaterials;

	/* Original values of the bad location param saved so it can be restored to it's original
	value during gameplay */
	UPROPERTY()
	TArray < FLinearColor > OriginalBadLocationParamValues;

	/* Reset actor rotation to whatever is considered the default */
	void SetRotationToDefault();

	/* The default camera yaw rotation of the player so ghost knows what is considered the 
	default rotation */
	float PlayersDefaultYawRot;

public:

	/* Here to prevent destroy */
	virtual void FellOutOfWorld(const UDamageType & dmgType) override;

	void SetInitialValues(const FBuildingInfo & BuildingInfo, float DefaultYawRotation);

	/* Set what is considered the default rotation i.e. the rotation the ghost should have if
	player has not tried to rotate it */
	void SetDefaultRotation(float DefaultYawRotation);

	/* Called when this enters the object pool */
	void OnEnterPool();

	/** 
	 *	Called when this object leaves the pool 
	 *
	 *	@param InBuildingType - the type of the building this ghost is being used for 
	 */
	void OnExitPool(EBuildingType InBuildingType);

	/* Function to set appearance based on whether at a suitable build location or not
	@param bIsBuildableLocation -whether the location is buildable or not */
	void TrySetAppearanceForBuildLocation(bool bIsBuildableLocation);

	/* Set the rotation of ghost given a yaw value */
	void SetRotationUsingOffset(float YawOffsetAmount);

	/* Getters and setters */

	EBuildingType GetType() const;

	FORCEINLINE class UBoxComponent * GetBoxComponent() const { return Bounds; }
	FORCEINLINE class USkeletalMeshComponent * GetMesh() const { return Mesh; }
};
