// Fill out your copyright notice in the Description page of Project Settings.

#include "ProjectileBase.h"
#include "EngineUtils.h"
#include "Public/TimerManager.h"
#include "Particles/ParticleSystemComponent.h"

#include "Statics/DevelopmentStatics.h"
#include "Managers/ObjectPoolingManager.h"
#include "GameFramework/RTSPlayerController.h"
#include "Statics/DamageTypes.h"
#include "GameFramework/RTSPlayerState.h"
#include "GameFramework/RTSGameState.h"
#include "Managers/FogOfWarManager.h"
#include "MapElements/Projectiles/CollidingProjectileBase.h"
#include "Statics/Statics.h"
#include "MapElements/Abilities/AbilityBase.h"


// Sets default values
AProjectileBase::AProjectileBase()
{
	/* TODO: check if disabling actor ticking disables component ticking too. If not then
	need to manually toggle each components tick when adding/removing from pool */

	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	TrailParticles = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("Trail Particles"));
	TrailParticles->PrimaryComponentTick.bStartWithTickEnabled = false;

	ImpactDamage = 5.f;
	ImpactDamageType = UDamageType_Default::StaticClass();
	ImpactRandomDamageFactor = 0.f;
	AoEDamage = 0.f;
	AoEDamageType = UDamageType_Default::StaticClass();
	AoERandomDamageFactor = 0.f;
	// Index 0 must exist otherwise will crash in GetImpactParticles/Sound
	ImpactEffects.Emplace(FParticleAudioPair());
	TrailParticlesExtraTime = 3.f;
	ImpactShakeRadius = 300.f;
	ImpactShakeFalloff = 1.f;
	bCanAoEHitEnemies = true;
}

// Called when the game starts or when spawned
void AProjectileBase::BeginPlay()
{
	/* N.B. this is called only when creating object pools, not when a projectile
	is fired */

	Super::BeginPlay();

	SetupCurveValues();

	PC = CastChecked<ARTSPlayerController>(GetWorld()->GetFirstPlayerController());
	GS = CastChecked<ARTSGameState>(GetWorld()->GetGameState());
	assert(GS != nullptr);
	TimerManager = &GetWorldTimerManager();

	/* Put in state ready for pool but leave pooling manager to do it */
	AddToPool(false);
}

void AProjectileBase::FellOutOfWorld(const UDamageType & dmgType)
{
#if WITH_EDITOR
	assert(TimerManager->IsTimerActive(TimerHandle_Lifetime));

	static uint32 WarningCount = 0;
	WarningCount++;
	if (WarningCount < 50)
	{
		UE_LOG(RTSLOG, Warning, TEXT("ProjectileBase::FellOutOfWorld() called: Projectile has "
			"fallen below KillZ"));
	}
	else if (WarningCount == 50)
	{
		UE_LOG(RTSLOG, Warning, TEXT("ProjectileBase::FellOutOfWorld() called at least 50 times "
			"haulting warnings for this")); 
	}
#else
	if (TimerManager->IsTimerActive(TimerHandle_Lifetime) == false)
	{
		return;
	}
#endif

	/* Put back in object pool */
	AddToPool();
}

void AProjectileBase::SetupForEnteringObjectPool()
{
	TimerManager->ClearAllTimersForObject(this);
}

void AProjectileBase::AddToPool(bool bActuallyAddToPool, bool bDisableTrailParticles)
{
	TimerManager->ClearAllTimersForObject(this);

	if (bDisableTrailParticles)
	{
		DisableTrailParticles();
	}

	if (bActuallyAddToPool)
	{
		/* Recently moved this from below the reset timers line */
		AoEObjectQueryParams = FCollisionObjectQueryParams::DefaultObjectQueryParam;
		
		PoolingManager->AddToPool(this, Projectile_BP);
	}
}

void AProjectileBase::DisableTrailParticles()
{
	TrailParticles->SetComponentTickEnabled(false);
	TrailParticles->SetHiddenInGame(true);
}

void AProjectileBase::DisableTrailsAndTryAddToPool()
{
	DisableTrailParticles();
	
	/* If damage still needs to be dealt we do not add to pool. After damage has been dealt 
	projectile will add itself to pool */
	if (TimerManager->IsTimerActive(TimerHandle_DealDamage) == false)
	{
		// Reset AoEObjectQueryParams here like in AddToPool? 
		PoolingManager->AddToPool(this, Projectile_BP);
	}
}

void AProjectileBase::GetTargetsWithinRadius(const FVector & Location)
{
	HitResults.Reset();
	Statics::CapsuleSweep(GetWorld(), HitResults, Location, AoEObjectQueryParams, AoERadius);
}

void AProjectileBase::OnHit(const FHitResult & Hit)
{
	const FVector HitLocation = Hit.ImpactPoint;

	/* Spawn impact particles if any */
	UParticleSystem * ImpactParticles = GetImpactParticles(Hit);
	if (ImpactParticles != nullptr)
	{
		const FRotator Rotation = Hit.ImpactNormal.Rotation();

		Statics::SpawnFogParticles(ImpactParticles, GS, HitLocation, Rotation, FVector::OneVector);
	}

	/* Possibly play impact sound */
	USoundBase * ImpactSoundToPlay = GetImpactSound(Hit);
	if (ImpactSoundToPlay != nullptr)
	{
		Statics::SpawnSoundAtLocation(GS, OwningTeam, ImpactSoundToPlay, HitLocation,
			ESoundFogRules::AlwaysKnownOnceHeard);
	}

	/* Play camera shake if set */
	if (ImpactCameraShake_BP != nullptr)
	{
		PC->PlayCameraShake(ImpactCameraShake_BP, HitLocation, ImpactShakeRadius,
			ImpactShakeFalloff);
	}

	/* Broadcast to an ability if it is listening */
	if (ListeningForOnHit != nullptr)
	{
		ListeningForOnHit->OnListenedProjectileHit(ListeningForOnHitData);
	}

	const bool bDealDamageNow = (DealDamageDelay <= 0.f);
	const bool bStopTrailParticlesNow = (TrailParticlesExtraTime <= 0.f);
	const bool bCanAddToPoolNow = (bDealDamageNow && bStopTrailParticlesNow);

	if (!bCanAddToPoolNow)
	{
		/* Do this now because this resets timers and we're possibly going to start 
		timer handles soon below */
		AddToPool(false, false);
	}

	if (bDealDamageNow)
	{
		DealDamage(Hit);
	}
	else
	{
		CallDealDamageAfterDelay(Hit);
	}
	
	/* Wait for trail particles or add back to pool now */
	if (!bStopTrailParticlesNow)
	{
		assert(TimerManager->IsTimerActive(TimerHandle_TrailParticles) == false);
		Delay(TimerHandle_TrailParticles, &AProjectileBase::DisableTrailsAndTryAddToPool,
			TrailParticlesExtraTime);
	}
	
	if (bCanAddToPoolNow)
	{
		AddToPool();
	}
}

void AProjectileBase::CallDealDamageAfterDelay(const FHitResult & Hit)
{
	assert(TimerManager->IsTimerActive(TimerHandle_DealDamage) == false);
	
	/* Store the hit result in this array. Saves us having to call the SetTimer func that 
	requires an FName that takes params. I assume doing it this way is faster. Can always 
	change it back though */
	HitResults.Reset();
	HitResults.Emplace(Hit);

	Delay(TimerHandle_DealDamage, &AProjectileBase::DealDamageAndTryAddToPool, DealDamageDelay);
}

void AProjectileBase::DealDamageAndTryAddToPool()
{
	DealDamage(HitResults[0]);

	/* If trails are still going then wait for them */
	if (TimerManager->IsTimerActive(TimerHandle_TrailParticles) == false)
	{
		// Reset AoEObjectQueryParams here like in AddToPool?
		PoolingManager->AddToPool(this, Projectile_BP);
	}
}

void AProjectileBase::DealDamage(const FHitResult & Hit)
{
	// Deal damage to what the projectile hit
	if (Hit.GetActor() != nullptr && ImpactDamage != 0.f)
	{
		AActor * const SingleTarget = Hit.GetActor();

		if (Statics::IsValid(SingleTarget))
		{
			const float DamageMulti = FMath::FRandRange(1.f - ImpactRandomDamageFactor, 1.f + ImpactRandomDamageFactor);

			const FDamageEvent DamageEvent = FDamageEvent(ImpactDamageType);
			SingleTarget->TakeDamage(ImpactDamage * DamageMulti, DamageEvent, nullptr, Shooter);
		}
	}

	// Deal AoE damage
	if (AoERadius > 0.f)
	{
		GetTargetsWithinRadius(Hit.ImpactPoint);

		for (const auto & Elem : HitResults)
		{
			AActor * const Actor = Elem.GetActor();

			const float DistanceToActor = 0.f;// TODO 
			const float MultiFromDistance = GetDamageDistanceMultiplier(DistanceToActor);
			const float DamageMulti = MultiFromDistance * FMath::FRandRange(1.f - AoERandomDamageFactor,
				1.f + AoERandomDamageFactor);

			Actor->TakeDamage(AoEDamage * DamageMulti, FDamageEvent(AoEDamageType), nullptr, Shooter);
		}
	}
}

float AProjectileBase::GetDamageDistanceMultiplier(float Distance) const
{
	if (DamageFalloffCurve != nullptr)
	{
		/* Map distance from actor from range [0, AoERaidus] to range [Curve.X.Min, Curve.X.Max] */
		const FVector2D InRange = FVector2D(0, AoERadius);
		const FVector2D OutRange = FVector2D(DamageCurveMinX, DamageCurveMaxX);
		const float XAxisValue = FMath::GetMappedRangeValueUnclamped(InRange, OutRange, Distance);

		const float YAxisValue = DamageFalloffCurve->GetFloatValue(XAxisValue);
		const float Multiplier = FMath::GetRangePct(DamageCurveMinY, DamageCurveMaxY, YAxisValue);

		assert(Multiplier >= 0.f && Multiplier <= 1.f);

		return Multiplier;
	}
	else
	{
		/* Max damage no matter the distance */
		return 1.f;
	}
}

UParticleSystem * AProjectileBase::GetImpactParticles(const FHitResult & Hit) const
{
	if (Hit.PhysMaterial.IsValid())
	{
		const EPhysicalSurface Surface = Hit.PhysMaterial.Get()->SurfaceType;
		const int32 ArrayIndex = Statics::PhysicalSurfaceTypeToArrayIndex(Surface);
		return ImpactEffects[ArrayIndex].GetImpactParticles();
	}
	else
	{
		// If no physical material was set then assume Default surface type
		return ImpactEffects[0].GetImpactParticles();
	}
}

USoundBase * AProjectileBase::GetImpactSound(const FHitResult & Hit) const
{
	if (Hit.PhysMaterial.IsValid())
	{
		const EPhysicalSurface Surface = Hit.PhysMaterial.Get()->SurfaceType;
		const int32 ArrayIndex = Statics::PhysicalSurfaceTypeToArrayIndex(Surface);
		return ImpactEffects[ArrayIndex].GetImpactSound();
	}
	else
	{
		// If no physical material was set then assume Default surface type
		return ImpactEffects[0].GetImpactSound();
	}
}

void AProjectileBase::SetupAoECollisionChannels(ETeam Team)
{
	if (bCanAoEHitEnemies)
	{
		for (const auto & Channel : GS->GetEnemyChannels(Team))
		{
			AoEObjectQueryParams.AddObjectTypesToQuery(static_cast<ECollisionChannel>(Channel));
		}
	}
	if (bCanAoEHitFriendlies)
	{
		const ECollisionChannel TeamChannel = GS->GetTeamCollisionChannel(Team);
		AoEObjectQueryParams.AddObjectTypesToQuery(TeamChannel);
	}
}

#if WITH_EDITOR
void AProjectileBase::PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	bCanEditCameraShakeOptions = ImpactCameraShake_BP;
	bCanEditAoEOptions = AoERadius > 0.f;

	// Populate ImpactEffects with the contents of ImpactEffectsTMap and set bQueryPhysicalMaterialForHits
	bQueryPhysicalMaterialForHits = false;
	ImpactEffects.Init(FParticleAudioPair(), 64);
	for (const auto & Elem : ImpactEffectsTMap)
	{
		ImpactEffects[Statics::PhysicalSurfaceTypeToArrayIndex(Elem.Key)] = Elem.Value;
		
		/* If any surface type other than Default has an impact particle or sound set then set 
		bQueryPhysicalMaterialForHits to true */
		bQueryPhysicalMaterialForHits |=
			(Elem.Key != EPhysicalSurface::SurfaceType_Default)
			&& (Elem.Value.GetImpactParticles() != nullptr || Elem.Value.GetImpactSound() != nullptr);
	} 
}
#endif

void AProjectileBase::SetupCurveValues()
{
	if (DamageFalloffCurve != nullptr)
	{
		DamageFalloffCurve->GetTimeRange(DamageCurveMinX, DamageCurveMaxX);
		DamageFalloffCurve->GetValueRange(DamageCurveMinY, DamageCurveMaxY);

		/* If true then curve is either has no points, a single point or is a single
		vertical or horizontal line. Null it to signal we will not use it and use a
		linear curve from here on out */
		if (DamageCurveMinY == DamageCurveMaxY
			|| DamageCurveMaxX == DamageCurveMinX)
		{
			DamageFalloffCurve = nullptr;
		}
	}
}

void AProjectileBase::Delay(FTimerHandle & TimerHandle, void(AProjectileBase::* Function)(), float Delay)
{
	assert(Delay > 0.f);
	TimerManager->SetTimer(TimerHandle, this, Function, Delay, false);
}

float AProjectileBase::GetImpactDamage() const
{
	return ImpactDamage;
}

TSubclassOf<URTSDamageType> AProjectileBase::GetImpactDamageType() const
{
	return ImpactDamageType;
}

float AProjectileBase::GetImpactRandomDamageFactor() const
{
	return ImpactRandomDamageFactor;
}

float AProjectileBase::GetAoEDamage() const
{
	return AoEDamage;
}

TSubclassOf<URTSDamageType> AProjectileBase::GetAoEDamageType() const
{
	return AoEDamageType;
}

float AProjectileBase::GetAoERandomDamageFactor() const
{
	return AoERandomDamageFactor;
}

void AProjectileBase::SetProjectileBP(TSubclassOf<AProjectileBase> NewValue)
{
	Projectile_BP = NewValue;
}

void AProjectileBase::SetPoolingManager(AObjectPoolingManager * InPoolingManager)
{
	PoolingManager = InPoolingManager;
}

void AProjectileBase::FireAtTarget(AActor * Firer, const FBasicDamageInfo & AttackAttributes, 
	float AttackRange, ETeam Team, const FVector & MuzzleLoc, AActor * ProjectileTarget, float RollRotation)
{
	/* If thrown then projectile has been added to pool but might still be active */
	assert(TimerManager->IsTimerActive(TimerHandle_Lifetime) == false);
	assert(TimerManager->IsTimerActive(TimerHandle_TrailParticles) == false);

	if (Statics::IsValid(ProjectileTarget) == false)
	{
		PoolingManager->AddToPool(this, Projectile_BP);
		return;
	}

#if !UE_BUILD_SHIPPING
	if (GetWorld()->IsServer())
	{
		assert(Firer != nullptr);
	}
#endif

	if (AoERadius > 0.f)
	{
		SetupAoECollisionChannels(Team);
	}

	Shooter = Firer;
	Target = ProjectileTarget;
	OwningTeam = Team;

	ImpactDamage = AttackAttributes.ImpactDamage;
	ImpactDamageType = AttackAttributes.ImpactDamageType;
	ImpactRandomDamageFactor = AttackAttributes.ImpactRandomDamageFactor;
	AoEDamage = AttackAttributes.AoEDamage;
	AoEDamageType = AttackAttributes.AoEDamageType;
	AoERandomDamageFactor = AttackAttributes.AoERandomDamageFactor;

	/* TODO: Hiding actor and individual components may not be necessary if trail particles do
	not stay visible after hit. Could add if statement that if TrailExtendTime is > 0 then they
	avoid setting each component hidden and just do a straight SetActorHiddenInGame */

	/* Decide if should be visible or not on spawn in regards to fog of war. But we haven't 
	moved it to the right location yet. So we should remove this from here and let child 
	classes handle it */
	const bool bCanBeSeen = PC->GetFogOfWarManager()->IsProjectileVisible(this, Team);
	/* Nothing wrong with using a custom func - make sure to update fog managers hide temporaries
	func too */
	SetActorHiddenInGame(!bCanBeSeen);
	GS->RegisterFogProjectile(this);

	TrailParticles->SetComponentTickEnabled(true);
	TrailParticles->SetHiddenInGame(false);
}

void AProjectileBase::FireAtLocation(AActor * Firer, const FBasicDamageInfo & AttackAttributes, 
	float AttackRange, ETeam Team, const FVector & StartLoc, const FVector & TargetLoc, float RollRotation,
	AAbilityBase * ListeningAbility, int32 ListeningAbilityUniqueID)
{
	// Basically the same as fire at target 

	/* If thrown then projectile has been added to pool but might still be active */
	assert(TimerManager->IsTimerActive(TimerHandle_Lifetime) == false);
	assert(TimerManager->IsTimerActive(TimerHandle_TrailParticles) == false);

#if !UE_BUILD_SHIPPING
	if (GetWorld()->IsServer())
	{
		/* This can be hit if say you call in an artillery strike, then die but the projectiles 
		are still spawning, so either change artillery strike to pass null as the firer or 
		remove this assert and fix code that relies on it (if any). I don't think there is any 
		off the top of my head */
		//assert(Firer != nullptr);
	}
#endif

	if (AoERadius > 0.f)
	{
		SetupAoECollisionChannels(Team);
	}

	ListeningForOnHit = ListeningAbility;
	ListeningForOnHitData = ListeningAbilityUniqueID;
	
	Shooter = Firer;
	Target = nullptr;
	OwningTeam = Team;

	ImpactDamage = AttackAttributes.ImpactDamage;
	ImpactDamageType = AttackAttributes.ImpactDamageType;
	ImpactRandomDamageFactor = AttackAttributes.ImpactRandomDamageFactor;
	AoEDamage = AttackAttributes.AoEDamage;
	AoEDamageType = AttackAttributes.AoEDamageType;
	AoERandomDamageFactor = AttackAttributes.AoERandomDamageFactor;

	/* TODO: Hiding actor and individual components may not be necessary if trail particles do
	not stay visible after hit. Could add if statement that if TrailExtendTime is > 0 then they
	avoid setting each component hidden and just do a straight SetActorHiddenInGame */

	/* Decide if should be visible or not on spawn in regards to fog of war */
	const bool bCanBeSeen = PC->GetFogOfWarManager()->IsProjectileVisible(this, Team);
	/* Nothing wrong with using a custom func - make sure to update fog managers hide temporaries
	func too */
	SetActorHiddenInGame(!bCanBeSeen);
	GS->RegisterFogProjectile(this);

	TrailParticles->SetComponentTickEnabled(true);
	TrailParticles->SetHiddenInGame(false);
}

void AProjectileBase::FireInDirection(AActor * Firer, const FBasicDamageInfo & AttackAttributes, 
	float AttackRange, ETeam Team, const FVector & StartLoc, const FRotator & Direction, AAbilityBase * ListeningAbility,
	int32 ListeningAbilityUniqueID)
{
	// Basically the same as fire at target 

	/* If thrown then projectile has been added to pool but might still be active */
	assert(TimerManager->IsTimerActive(TimerHandle_Lifetime) == false);
	assert(TimerManager->IsTimerActive(TimerHandle_TrailParticles) == false);

	if (AoERadius > 0.f)
	{
		SetupAoECollisionChannels(Team);
	}

	ListeningForOnHit = ListeningAbility;
	ListeningForOnHitData = ListeningAbilityUniqueID;

	Shooter = Firer;
	Target = nullptr;
	OwningTeam = Team;

	ImpactDamage = AttackAttributes.ImpactDamage;
	ImpactDamageType = AttackAttributes.ImpactDamageType;
	ImpactRandomDamageFactor = AttackAttributes.ImpactRandomDamageFactor;
	AoEDamage = AttackAttributes.AoEDamage;
	AoEDamageType = AttackAttributes.AoEDamageType;
	AoERandomDamageFactor = AttackAttributes.AoERandomDamageFactor;

	/* TODO: Hiding actor and individual components may not be necessary if trail particles do
	not stay visible after hit. Could add if statement that if TrailExtendTime is > 0 then they
	avoid setting each component hidden and just do a straight SetActorHiddenInGame */

	/* Decide if should be visible or not on spawn in regards to fog of war */
	const bool bCanBeSeen = PC->GetFogOfWarManager()->IsProjectileVisible(this, Team);
	/* Nothing wrong with using a custom func - make sure to update fog managers hide temporaries
	func too */
	SetActorHiddenInGame(!bCanBeSeen);
	GS->RegisterFogProjectile(this);

	TrailParticles->SetComponentTickEnabled(true);
	TrailParticles->SetHiddenInGame(false);
}

void AProjectileBase::GetFogLocations(TArray<FVector, TInlineAllocator<AProjectileBase::NUM_FOG_LOCATIONS>>& OutLocations) const
{
	/* Array expected to be empty before calling this */
	assert(OutLocations.Num() == 0);

	OutLocations.Emplace(GetActorLocation());
}

void AProjectileBase::OnProjectileStopFromMovementComp(const FHitResult & Hit)
{
}

#if !UE_BUILD_SHIPPING
bool AProjectileBase::IsFitForEnteringObjectPool() const
{
	return IsActorTickEnabled() == false
		&& TrailParticles->IsComponentTickEnabled() == false
		&& TrailParticles->bHiddenInGame == true
		&& TrailParticles->IsCollisionEnabled() == false // Never do anything with collision but check in case users change it at some point
		&& AreAllTimerHandlesCleared() == true;
}

bool AProjectileBase::AreAllTimerHandlesCleared() const
{
	return TimerManager->IsTimerPending(TimerHandle_Lifetime) == false
		&& TimerManager->IsTimerPending(TimerHandle_DealDamage) == false
		&& TimerManager->IsTimerPending(TimerHandle_TrailParticles) == false;
}
#endif // !UE_BUILD_SHIPPING
