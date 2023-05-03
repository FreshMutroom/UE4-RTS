// Fill out your copyright notice in the Description page of Project Settings.

#include "PlayerCamera.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SphereComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"

#include "Statics/DevelopmentStatics.h"
#include "MapElements/Invisible/PlayerCamera/PlayerCameraMovement.h"
#include "Statics/Statics.h"

// Sets default values
APlayerCamera::APlayerCamera()
{
	PrimaryActorTick.bCanEverTick = true;
	
	InvisibleSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InvisibleMesh"));
	SetRootComponent(InvisibleSphere);
	InvisibleSphere->SetCanEverAffectNavigation(false);
	InvisibleSphere->SetGenerateOverlapEvents(false);
	InvisibleSphere->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	InvisibleSphere->SetCollisionProfileName(FName("PlayerCamera"));

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(InvisibleSphere);
	// Disable the spring arm auto-adjusting with ray trace
	SpringArm->bDoCollisionTest = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);

	MovementComponent = CreateDefaultSubobject<UPlayerCameraMovement>(TEXT("MovementComponent"));

	// These 2 lines are to allow MMB free-look
	bUseControllerRotationPitch = true;
	bUseControllerRotationYaw = true;

	SetActorHiddenInGame(true);

	AutoPossessAI = EAutoPossessAI::Disabled;
	AIControllerClass = nullptr;

	/* Replication properties */
	/* TODO try and set bReplicates to false, and try and spawn pawn on client. Will have to
	override AMyPlayerController::Possess to let it run on client */
	/* Must be true for pawn to be possessed by client */
	bReplicates = true;
	bReplicateMovement = false;
	bAlwaysRelevant = false;
	bOnlyRelevantToOwner = true;
}

// Called when the game starts or when spawned
void APlayerCamera::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void APlayerCamera::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void APlayerCamera::PossessedBy(AController * NewController)
{
	Super::PossessedBy(NewController);

	DestroyIfNotOurs();
}

void APlayerCamera::OnRep_Controller()
{
	Super::OnRep_Controller();

	DestroyIfNotOurs();
}

void APlayerCamera::DestroyIfNotOurs()
{
	assert(GetController() != nullptr);

	// Server can't destroy otherwise it will replicate to client and they lose their camera
	if (!GetWorld()->IsServer() && GetController() != GetWorld()->GetFirstPlayerController())
	{
		Destroy(true);
	}
}

void APlayerCamera::SnapToLocation(const FVector & Location, bool bSnapToFloor)
{
	/* If this doesn't work like I expect: problem may be that the movement component has 
	given this some velocity and it gets applied after the move happens so maybe need to 
	consume the velocity vector first */
	
	FVector ActualLocation = Location;
	
	if (bSnapToFloor)
	{
		/* Ray trace from above param loc searching for ground. Give it some height because 
		could be snapping to an air unit */
		const float TraceHalfHeight = Statics::SWEEP_HEIGHT * 1.5f;
		const FVector TraceStartLoc = FVector(Location.X, Location.Y, Location.Z + TraceHalfHeight);
		const FVector TraceEndLoc = FVector(Location.X, Location.Y, Location.Z - TraceHalfHeight);
	
		FHitResult HitResult;
		if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStartLoc, TraceEndLoc, 
			CAMERA_CHANNEL))
		{
			// If adding padding here also check UPlayerCameraMovement::GlueToGround
			ActualLocation.Z = HitResult.ImpactPoint.Z/*+ Padding to keep it slightly above ground?*/;
		}
	}

	SetActorLocation(ActualLocation);
}
