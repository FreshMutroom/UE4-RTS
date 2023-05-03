// Fill out your copyright notice in the Description page of Project Settings.

#include "CollidingProjectileBase.h"
#include "Components/SphereComponent.h"
#include "Public/TimerManager.h"
#include "Engine/World.h"
#include "Components/ArrowComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/StaticMeshComponent.h"

#include "GameFramework/RTSGameState.h"
#include "Statics/Statics.h"
#include "Statics/DevelopmentStatics.h"

ACollidingProjectileBase::ACollidingProjectileBase()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	SphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
	SetRootComponent(SphereComp);
	SphereComp->PrimaryComponentTick.bCanEverTick = false;
	SphereComp->PrimaryComponentTick.bStartWithTickEnabled = false;
	SphereComp->SetGenerateOverlapEvents(false);
	SphereComp->SetCanEverAffectNavigation(false);
	SphereComp->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	SphereComp->SetCollisionProfileName(FName("Projectile"));

#if WITH_EDITORONLY_DATA
	Arrow = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	if (Arrow != nullptr)
	{
		Arrow->SetupAttachment(SphereComp);
		Arrow->PrimaryComponentTick.bCanEverTick = false;
		Arrow->PrimaryComponentTick.bStartWithTickEnabled = false;
		Arrow->bReceivesDecals = false;
		Arrow->SetGenerateOverlapEvents(false);
		Arrow->SetCanEverAffectNavigation(false);
		Arrow->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
		Arrow->SetCollisionProfileName(FName("NoCollision"));
	}
#endif

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(SphereComp);
	Mesh->PrimaryComponentTick.bCanEverTick = false;
	Mesh->PrimaryComponentTick.bStartWithTickEnabled = false;
	Mesh->SetCanEverAffectNavigation(false);
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	Mesh->SetCollisionProfileName(FName("NoCollision"));

	TrailParticles->SetupAttachment(SphereComp);

	bReplicates = false;
	bReplicateMovement = false;
	bAlwaysRelevant = false;

	bCanHitWorld = true;
	bCanHitEnemies = true;
	Lifetime = 20.f;
	bRegistersHitOnTimeout = false;
#if WITH_EDITORONLY_DATA
	bCanEditTeamCollisionProperties = true;
#endif

	/* Set this so first call of ResetTeamCollision will not cause crash */
	OwningTeam = ETeam::Team1;
}

void ACollidingProjectileBase::BeginPlay()
{
	Super::BeginPlay();

	if (bCanHitWorld)
	{
		SphereComp->SetCollisionResponseToChannel(ENVIRONMENT_CHANNEL, ECR_Block);
	}
}

void ACollidingProjectileBase::SetupForEnteringObjectPool()
{
	/* N.B. this will have a garbage value for OwningTeam on first call but should not matter */
	ResetTeamCollision(OwningTeam);
	Mesh->SetHiddenInGame(true);
	
	Super::SetupForEnteringObjectPool();
}

void ACollidingProjectileBase::AddToPool(bool bActuallyAddToPool, bool bDisableTrailParticles)
{
	/* N.B. this will have a garbage value for OwningTeam on first call but should not matter */
	ResetTeamCollision(OwningTeam);
	Mesh->SetHiddenInGame(true);

	Super::AddToPool(bActuallyAddToPool, bDisableTrailParticles);
}

void ACollidingProjectileBase::ResetTeamCollision(ETeam Team)
{
	/* SetCollisionProfileName may be a faster option. 
	Also SetCollisionResponseToChannels (plural) may be better too */

	SphereComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (bCanHitEnemies)
	{
		for (const auto & Elem : GS->GetEnemyChannels(Team))
		{
			SphereComp->SetCollisionResponseToChannel(static_cast<ECollisionChannel>(Elem),
				ECR_Ignore);
		}
	}
	if (bCanHitFriendlies)
	{
		const ECollisionChannel Channel = GS->GetTeamCollisionChannel(Team);
		SphereComp->SetCollisionResponseToChannel(Channel, ECR_Ignore);
	}
}

void ACollidingProjectileBase::SetupCollisionChannels(ETeam Team)
{
	if (bCanHitEnemies)
	{
		for (const auto & Elem : GS->GetEnemyChannels(Team))
		{
			SphereComp->SetCollisionResponseToChannel(static_cast<ECollisionChannel>(Elem),
				ECR_Block);
		}
	}
	if (bCanHitFriendlies)
	{
		const ECollisionChannel Channel = GS->GetTeamCollisionChannel(Team);
		SphereComp->SetCollisionResponseToChannel(Channel, ECR_Block);
	}

	SphereComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

FRotator ACollidingProjectileBase::GetStartingRotation(const FVector & StartLoc, const AActor * InTarget, float RollRotation) const
{
	return FRotator(0.f, 0.f, RollRotation);
}

FRotator ACollidingProjectileBase::GetStartingRotation(const FVector & StartLoc, const FVector & TargetLoc, float RollRotation) const
{
	return FRotator(0.f, 0.f, RollRotation);
}

void ACollidingProjectileBase::OnLifetimeExpired()
{
	if (bRegistersHitOnTimeout)
	{
		HitResult.Actor = nullptr;
		HitResult.ImpactPoint = GetActorLocation();
		OnHit(HitResult);
	}
	else
	{
		AddToPool();
	}
}

void ACollidingProjectileBase::Delay(FTimerHandle & TimerHandle, void(ACollidingProjectileBase::* Function)(), float Delay)
{
	assert(Delay > 0.f);
	TimerManager->SetTimer(TimerHandle, this, Function, Delay, false);
}

void ACollidingProjectileBase::FireAtTarget(AActor * Firer, const FBasicDamageInfo & AttackAttributes, 
	float AttackRange, ETeam Team, const FVector & MuzzleLoc, AActor * ProjectileTarget, float RollRotation)
{
	Super::FireAtTarget(Firer, AttackAttributes, AttackRange, Team, MuzzleLoc, ProjectileTarget, RollRotation);

	Mesh->SetHiddenInGame(false);

	/* Teleport to muzzle location and set rotation. I think roll isn't set anywhere else so 
	doing it now isn't redundent */
	const bool bItWorked = SetActorLocationAndRotation(MuzzleLoc, GetStartingRotation(MuzzleLoc, ProjectileTarget, RollRotation));
	assert(bItWorked);

	SetupCollisionChannels(Team);

	Delay(TimerHandle_Lifetime, &ACollidingProjectileBase::OnLifetimeExpired, Lifetime);
}

void ACollidingProjectileBase::FireAtLocation(AActor * Firer, const FBasicDamageInfo & AttackAttributes, 
	float AttackRange, ETeam Team, const FVector & StartLoc, const FVector & TargetLoc, float RollRotation,
	AAbilityBase * ListeningAbility, int32 ListeningAbilityUniqueID)
{
	// Basically the same as FireAtTarget

	Super::FireAtLocation(Firer, AttackAttributes, AttackRange, Team, StartLoc, TargetLoc, RollRotation, ListeningAbility, ListeningAbilityUniqueID);

	Mesh->SetHiddenInGame(false);

	// const bool bItWorked = SetActorLocation(StartLoc); // Old code
	/* Teleport to muzzle location. Could perhaps be good to implement a virtual func 
	for ACollidingProjectileBase like GetInitialRotation that we use here for the rotation 
	because the leave then come back projectiles might want to face up to start with. Doesn't 
	really matter because the curves from the move comp take over */
	const bool bItWorked = SetActorLocationAndRotation(StartLoc, GetStartingRotation(StartLoc, TargetLoc, RollRotation));
	assert(bItWorked);

	SetupCollisionChannels(Team);

	Delay(TimerHandle_Lifetime, &ACollidingProjectileBase::OnLifetimeExpired, Lifetime);
}

void ACollidingProjectileBase::FireInDirection(AActor * Firer, const FBasicDamageInfo & AttackAttributes, 
	float AttackRange, ETeam Team, const FVector & StartLoc, const FRotator & Direction, AAbilityBase * ListeningAbility,
	int32 ListeningAbilityUniqueID)
{
	Super::FireInDirection(Firer, AttackAttributes, AttackRange, Team, StartLoc, Direction, ListeningAbility, ListeningAbilityUniqueID);

	Mesh->SetHiddenInGame(false);

	const bool bItWorked = SetActorLocationAndRotation(StartLoc, Direction);
	assert(bItWorked);

	SetupCollisionChannels(Team);

	Delay(TimerHandle_Lifetime, &ACollidingProjectileBase::OnLifetimeExpired, Lifetime);
}

#if !UE_BUILD_SHIPPING
bool ACollidingProjectileBase::IsFitForEnteringObjectPool() const
{
	return Super::IsFitForEnteringObjectPool()
		&& SphereComp->IsComponentTickEnabled() == false 
		&& SphereComp->bHiddenInGame == true
		&& IsSphereCollisionAcceptableForEnteringObjectPool()
		&& Mesh->IsComponentTickEnabled() == false
		&& Mesh->bHiddenInGame == true
		&& Mesh->IsCollisionEnabled() == false;
}

bool ACollidingProjectileBase::IsSphereCollisionAcceptableForEnteringObjectPool() const
{	
	/* Might want to also check that all the channels collision respones (excluding ground 
	channel which is set in BeginPlay and is never altered) are set to ECR_Ignore */
	return SphereComp->IsCollisionEnabled() == false;
}
#endif

#if WITH_EDITOR
void ACollidingProjectileBase::PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	// bQueryPhysicalMaterialForHits would have been set in Super
	SphereComp->bReturnMaterialOnMove = bQueryPhysicalMaterialForHits;
}
#endif
