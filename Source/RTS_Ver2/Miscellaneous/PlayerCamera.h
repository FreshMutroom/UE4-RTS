// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "PlayerCamera.generated.h"

class USphereComponent;
class UCameraComponent;
class USpringArmComponent;
class UPlayerCameraMovement;

UCLASS()
class RTS_VER2_API APlayerCamera : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	APlayerCamera();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	/* Called when possessed */
	virtual void PossessedBy(class AController * NewController) override;

	virtual void OnRep_Controller() override;

private:

	// Needed to use collision with camera
	UPROPERTY(VisibleDefaultsOnly)
	USphereComponent * InvisibleSphere;

	UPROPERTY(VisibleDefaultsOnly)
	USpringArmComponent * SpringArm;

	UPROPERTY(VisibleDefaultsOnly)
	UCameraComponent * Camera;

	/* This was a USpectatorPawnMovement component for a long time */
	UPROPERTY(VisibleDefaultsOnly)
	UPlayerCameraMovement * MovementComponent;

	/* Destroy this camera if it does not belong to the local player controller */
	void DestroyIfNotOurs();

public:

	/**
	 *	Teleport the camera to a location. 
	 *	@param Location - world location to teleport camera to
	 *	@param bSnapToFloor - if true then the Z axis value of Location will be ignored and 
	 *	camera will be placed on the landscape
	 */
	void SnapToLocation(const FVector & Location, bool bSnapToFloor);

	/* Getters and setters */

	FORCEINLINE USpringArmComponent * GetSpringArm() const { return SpringArm; }
};
