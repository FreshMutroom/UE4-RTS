// Fill out your copyright notice in the Description page of Project Settings.


#include "Warthog.h"
#include "Components/SkeletalMeshComponent.h"
#include "Curves/CurveFloat.h"
#include "Kismet/KismetMathLibrary.h"

#include "MapElements/Projectiles/InstantHitProjectile.h"
#include "Managers/ObjectPoolingManager.h"
#include "Statics/Statics.h"
#include "Statics/DevelopmentStatics.h"
#include "MapElements/CommanderAbilities/Actors/WarthogMovementComponent.h"


FWarthogAttackAttributes::FWarthogAttackAttributes()
	: ProjectileType1_BP(AInstantHitProjectile::StaticClass()) 
	, ProjectileType1_DamageAttributes(FBasicDamageInfo(150.f, 0.1f)) 
	, ProjectileType2_BP(AInstantHitProjectile::StaticClass()) 
	, ProjectileType2_DamageAttributes(FBasicDamageInfo(50.f, 0.1f, 100.f, 0.1f))
	, Curve_TimeBetweenShots(nullptr) 
	, Projectile2Ratio(4)
	, NumProjectiles(100)
	, MuzzleParticles(nullptr) 
	, MuzzleName("Muzzle")
	, Curve_PitchSpread(nullptr)
	, Curve_YawSpread(nullptr)
	, FiringSound(nullptr)
{
}


AWarthog::AWarthog()
{
	DummyRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Dummy Root"));
	SetRootComponent(DummyRoot);
	DummyRoot->bVisible = false;
	DummyRoot->bHiddenInGame = true;

	Mesh_SK = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	Mesh_SK->SetupAttachment(DummyRoot);
	Mesh_SK->bReceivesDecals = false;
	Mesh_SK->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	Mesh_SK->SetCanEverAffectNavigation(false);
	Mesh_SK->SetGenerateOverlapEvents(false);
	Mesh_SK->SetCollisionProfileName(FName("StrikeAircraft"));

	MoveComp = CreateDefaultSubobject<UWarthogMovementComponent>(TEXT("Move Comp"));

	EngineAudioComp = CreateDefaultSubobject<UFogObeyingAudioComponent>(TEXT("AudioComp"));
	EngineAudioComp->SetupAttachment(Mesh_SK);

	PrimaryActorTick.bCanEverTick = true;

	Phase = EWarthogAttackPhase::Phase1_MovingToTarget;
	SpawnHeight = 1500.f;
	MoveSpeed_Phase1AndPhase4 = 2000.f;
	TransitionDistance_Phase2 = 8000.f;
	MoveSpeed_Phase2AndPhase3 = 1800.f;
	Tilt_Phase2 = -9.f;
	TransitionDistance_Phase3 = 1000.f;
	TransitionTime_Phase4 = 0.4f;
	Tilt_Phase4 = 25.f;

	MaxHealth = 500.f;
	TargetingType = ETargetingType::Default;
	ArmourType = EArmourType::Default;

#if WITH_EDITOR
	RunPostEditLogic();
#endif
}

void AWarthog::BeginPlay()
{
	Super::BeginPlay();

	CheckCurveAssets();

	UE_CLOG(Mesh_SK->GetAnimInstance() == nullptr, RTSLOG, Fatal, TEXT("[%s]: anim instance is "
		"null. "), *GetClass()->GetName());

	Mesh_SK->GetAnimInstance()->Montage_Play(AnimMontage_EnginesRunning);
}

void AWarthog::CheckCurveAssets()
{
	UE_CLOG(AttackAttributes.Curve_TimeBetweenShots == nullptr, RTSLOG, Fatal, TEXT("[%s]: Curve_TimeBetweenShots is null"), *GetClass()->GetName());
	UE_CLOG(AttackAttributes.Curve_PitchSpread == nullptr, RTSLOG, Fatal, TEXT("[%s]: Curve_PitchSpread is null"), *GetClass()->GetName());
	UE_CLOG(AttackAttributes.Curve_YawSpread == nullptr, RTSLOG, Fatal, TEXT("[%s]: Curve_YawSpread is null"), *GetClass()->GetName());

	{
		float Min_Y, Max_Y;
		AttackAttributes.Curve_TimeBetweenShots->GetValueRange(Min_Y, Max_Y);
		UE_CLOG(Min_Y <= 0.f, RTSLOG, Fatal, TEXT("[%s]: Curve_TimeBetweenShots cannot go below 0 on the Y axis"), *GetClass()->GetName());
	}
	
	{
		float Min_X, Max_X;
		AttackAttributes.Curve_PitchSpread->GetTimeRange(Min_X, Max_X);
		UE_CLOG(Min_X != 0.f || Max_X != 1.f, RTSLOG, Fatal, TEXT("[%s]: Curve_PitchSpread's X axis must start at 0 and end at 1. "
			"Values were Min_X: [%f], Max_X: [%f]"), *GetClass()->GetName(), Min_X, Max_X);
	}

	{
		float Min_X, Max_X;
		AttackAttributes.Curve_YawSpread->GetTimeRange(Min_X, Max_X);
		UE_CLOG(Min_X != 0.f || Max_X != 1.f, RTSLOG, Fatal, TEXT("[%s]: Curve_YawSpread's X axis must start at 0 and end at 1. "
			"Values were Min_X : [%f], Max_X : [%f]"), *GetClass()->GetName(), Min_X, Max_X);
	}
}

void AWarthog::InitialSetup(URTSGameInstance * InGameInstance, AObjectPoolingManager * InObjectPoolingManager)
{
	GI = InGameInstance;
	PoolingManager = InObjectPoolingManager;
}

void AWarthog::OnOwningAbilityUsed(ARTSGameState * GameState, ARTSPlayerState * InInstigator, const FVector & TargetLocation, int32 InRandomNumberSeed)
{
	Tags.Reserve(Statics::NUM_ACTOR_TAGS);
	Tags.Emplace(InInstigator->GetPlayerID());
	Tags.Emplace(InInstigator->GetTeamTag());
	Tags.Emplace(Statics::GetTargetingType(TargetingType));
	Tags.Emplace(Statics::AirTag);
	Tags.Emplace(Statics::UnitTag);
	Tags.Emplace(Statics::HasAttackTag);
	Tags.Emplace(Statics::AboveZeroHealthTag);
	Tags.Emplace(Statics::NotHasInventoryTag);
	assert(Tags.Num() == Statics::NUM_ACTOR_TAGS); // Make sure I did not forget one

	AbilityUseLocation = TargetLocation;
	Phase = EWarthogAttackPhase::Phase1_MovingToTarget;
	InstigatorsTeam = InInstigator->GetTeam();
	RandomStream = FRandomStream(InRandomNumberSeed);
	TimeSpentInPhase2AndPhase3 = 0.f;
	NumProjectilesRemaining = AttackAttributes.NumProjectiles;
	TimeSpentInPhase4 = 0.f;

	Health = MaxHealth;

	/* Setup collision */
	const ECollisionChannel TeamCollisionChannel = GameState->GetTeamCollisionChannel(InstigatorsTeam);
	Mesh_SK->SetCollisionObjectType(TeamCollisionChannel);

	MoveComp->OnOwningAbilityUsed(TargetLocation);
	
	EngineAudioComp->PlaySound(GameState, EngineAudioComp->Sound, 0.f, InstigatorsTeam, ESoundFogRules::DynamicExceptForInstigatorsTeam);
}

void AWarthog::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (Statics::HasZeroHealth(this))
	{
		return;
	}

	if (Phase == EWarthogAttackPhase::Phase1_MovingToTarget)
	{
		const float DistanceToLocation2DSqr = Statics::GetDistance2DSquared(GetActorLocation(), AbilityUseLocation);
		if (DistanceToLocation2DSqr < FMath::Square(TransitionDistance_Phase2))
		{
			Phase = EWarthogAttackPhase::Phase2_Descending;
		}
	}
	else if (Phase == EWarthogAttackPhase::Phase2_Descending)
	{
		TimeSpentInPhase2AndPhase3 += DeltaTime;
		
		const float DistanceToLocation2DSqr = Statics::GetDistance2DSquared(GetActorLocation(), AbilityUseLocation);
		if (DistanceToLocation2DSqr < FMath::Square(TransitionDistance_Phase2 - TransitionDistance_Phase3))
		{
			Phase = EWarthogAttackPhase::Phase3_Firing;

			TimeBetweenShotsCurveYValue = 0.f;
			TimeTillFireNextProjectile = 0.f;
			// Fire first projectile now
			Statics::SpawnSoundAttached(AttackAttributes.FiringSound, Mesh_SK, ESoundFogRules::AlwaysKnownOnceHeard,
				AttackAttributes.MuzzleName);
			FireProjectiles();
		}
	}
	else if (Phase == EWarthogAttackPhase::Phase3_Firing)
	{
		TimeSpentInPhase2AndPhase3 += DeltaTime;
		
		TimeTillFireNextProjectile -= DeltaTime;
		FireProjectiles();
		
		if (NumProjectilesRemaining == 0)
		{
			Phase = EWarthogAttackPhase::Phase4_PostFired;
		}
	}
	else
	{
		TimeSpentInPhase4 += DeltaTime;

		if (TimeSpentInPhase4 > 10.f)
		{
			// Destroy actor after 10secs of flying away
			Destroy();
		}
	}
}

float AWarthog::TakeDamage(float DamageAmount, FDamageEvent const & DamageEvent, AController * EventInstigator, AActor * DamageCauser)
{
	if (GetWorld()->IsServer() == false)
	{
		return 0.f;
	}

	if (DamageAmount == 0.f || Statics::HasZeroHealth(this))
	{
		return 0.f;
	}

	/* Get multiplier for damage type and armour type pairing */
	const float ArmourTypeMultiplier = GI->GetDamageMultiplier(DamageEvent.DamageTypeClass, ArmourType);
	const float FinalDamageAmount = DamageAmount * ArmourTypeMultiplier;

	if ((FinalDamageAmount > 0.f) && (!bCanBeDamaged))
	{
		return 0.f;
	}

	Health -= FinalDamageAmount;
	if (Health <= 0.f)
	{
		// Setting this will stop TakeDamage from reaching here again and also stops Tick from doing anything 
		// like firing shots
		Tags[Statics::GetZeroHealthTagIndex()] = Statics::HasZeroHealthTag;

		// Play an animation or something
	}

	return FinalDamageAmount;
}

FVector AWarthog::GetStartingLocation(ARTSPlayerState * InInstigator, const FVector & TargetLocation, float & OutYawBearing)
{
	const FVector CommandLocation = InInstigator->GetCommandLocation();

	/* Create a line from the ability's use location to the player's command location. Find where 
	it intersects with the edge of the map. I'm using a line trace. I could not find anything 
	in FMath that would do this more efficiently */
	const float TraceLength = 30000.f;
	FVector TraceStart, TraceEnd, TraceDirectionUnitVector;
	// Check if the ability's use location is also
	if (CommandLocation == TargetLocation)
	{
		// Just default to making the trace go south
		TraceStart = TargetLocation;
		TraceEnd = FVector(CommandLocation.X - TraceLength, CommandLocation.Y, TargetLocation.Z) - TraceStart;
		TraceDirectionUnitVector = TraceEnd - TraceStart;
		TraceDirectionUnitVector.Normalize();
	}
	else
	{
		TraceStart = TargetLocation;
		TraceDirectionUnitVector = FVector(CommandLocation.X, CommandLocation.Y, TargetLocation.Z) - TraceStart;
		TraceDirectionUnitVector.Normalize();
		TraceEnd = TraceStart + (TraceDirectionUnitVector * TraceLength);
	}
	
	FCollisionObjectQueryParams QueryParams = FCollisionObjectQueryParams(MAP_BOUNDRY_CHANNEL);
	FHitResult HitResult;
	if (Statics::LineTraceSingleByObjectType(GetWorld(), HitResult, TraceStart, TraceEnd, QueryParams))
	{
		OutYawBearing = TraceDirectionUnitVector.Rotation().Yaw;

		/* Start outside the map. Use TransitionDistance_Phase2 */
		FVector Vector = HitResult.ImpactPoint + (TraceDirectionUnitVector * (TransitionDistance_Phase2 * 1.1f));
		Vector.Z = TargetLocation.Z + SpawnHeight;
		return Vector;
	}
	else
	{
		UE_LOG(RTSLOG, Fatal, TEXT("[%s]: trace for map boundry did not hit anything. "
			"Trace start: [%s], trace end: [%s]"), *GetClass()->GetName(), *TraceStart.ToCompactString(), *TraceEnd.ToCompactString());
		return CommandLocation;
	}
}

FVector AWarthog::GetStartingLocation(const FVector & TargetLocation, float YawBearing) const
{
	FVector TraceStart, TraceEnd, TraceDirectionUnitVector;
	const float TraceLength = 30000.f;
	TraceStart = TargetLocation;
	TraceDirectionUnitVector = FRotator(0.f, YawBearing, 0.f).Vector();
	TraceEnd = TraceStart + (TraceDirectionUnitVector * TraceLength);

	FCollisionObjectQueryParams QueryParams = FCollisionObjectQueryParams(MAP_BOUNDRY_CHANNEL);
	FHitResult HitResult;
	if (Statics::LineTraceSingleByObjectType(GetWorld(), HitResult, TraceStart, TraceEnd, QueryParams))
	{
		/* Start outside the map. Use TransitionDistance_Phase2 */
		FVector Vector = HitResult.ImpactPoint + (TraceDirectionUnitVector * (TransitionDistance_Phase2 * 1.1f));
		Vector.Z = TargetLocation.Z + SpawnHeight;
		return Vector;
	}
	else
	{
		// Trace did not hit anything
		assert(0);
	}

	return FVector::ZeroVector;
}

FRotator AWarthog::GetStartingRotation(const FVector & SpawnLocation, float YawBearing)
{
	return FRotator();// Todo, maybe not even needed though because move comp sets rotation
}

void AWarthog::FireProjectiles()
{
	while (TimeTillFireNextProjectile < 0.f && NumProjectilesRemaining > 0)
	{
		const float CurveValue = AttackAttributes.Curve_TimeBetweenShots->GetFloatValue(TimeBetweenShotsCurveYValue);
		TimeTillFireNextProjectile += CurveValue;
		TimeBetweenShotsCurveYValue = CurveValue;

		const FTransform & MuzzleTransform = Mesh_SK->GetSocketTransform(AttackAttributes.MuzzleName);
		const FVector ProjectileSpawnLocation = MuzzleTransform.GetTranslation();
		const FRotator ProjectileSpawnRotation = GetProjectileFireDirection(MuzzleTransform);
		// Choose projectile type to fire
		TSubclassOf<AProjectileBase> ProjectileBP;
		const FBasicDamageInfo * DamageInfo;
		if (NumProjectilesRemaining % (AttackAttributes.Projectile2Ratio + 1) == 0)
		{
			ProjectileBP = AttackAttributes.ProjectileType2_BP;
			DamageInfo = &AttackAttributes.ProjectileType2_DamageAttributes;
		}
		else
		{
			ProjectileBP = AttackAttributes.ProjectileType1_BP;
			DamageInfo = &AttackAttributes.ProjectileType1_DamageAttributes;
		}

		if (GetWorld()->IsServer())
		{
			/* Passing in null as the instigator. The reason for this is that this class is
			not an ISelectable and I think the take damage code on the AI cons will assume that a selectable
			damaged them */
			PoolingManager->Server_FireProjectileInDirection(nullptr, ProjectileBP,
				*DamageInfo, 0.f, InstigatorsTeam, ProjectileSpawnLocation, ProjectileSpawnRotation);
		}
		else
		{
			PoolingManager->Client_FireProjectileInDirection(ProjectileBP,
				*DamageInfo, 0.f, InstigatorsTeam, ProjectileSpawnLocation, ProjectileSpawnRotation);
		}

		/* Show muzzle particles */
		if (AttackAttributes.MuzzleParticles != nullptr)
		{
			ARTSGameState * GameState = CastChecked<ARTSGameState>(GetWorld()->GetGameState());
			Statics::SpawnFogParticles(AttackAttributes.MuzzleParticles, GameState, ProjectileSpawnLocation,
				MuzzleTransform.GetRotation().Rotator(), FVector::OneVector);
		}

		NumProjectilesRemaining--;
	}
}

FRotator AWarthog::GetProjectileFireDirection(const FTransform & MuzzleTransform) const
{
	FRotator Rotation = MuzzleTransform.Rotator();

	// Apply some spread. Probably assumed here that warthog has 0 roll
	const float RandomFloatForPitch = UKismetMathLibrary::RandomFloatInRangeFromStream(0.f, 1.f, RandomStream);
	const float RandomFloatForYaw = UKismetMathLibrary::RandomFloatInRangeFromStream(0.f, 1.f, RandomStream);
	const float PitchSign = UKismetMathLibrary::RandomIntegerInRangeFromStream(0, 1, RandomStream) == 1 ? 1.f : -1.f;
	const float YawSign = UKismetMathLibrary::RandomIntegerInRangeFromStream(0, 1, RandomStream) == 1 ? 1.f : -1.f;
	const float InaccuracyPitchAmount = AttackAttributes.Curve_PitchSpread->GetFloatValue(RandomFloatForPitch) * PitchSign;
	const float InaccuracyYawAmount = AttackAttributes.Curve_YawSpread->GetFloatValue(RandomFloatForYaw) * YawSign;
	const float RollAmount = UKismetMathLibrary::RandomFloatInRangeFromStream(0.f, 360.f, RandomStream);
	Rotation += FRotator(InaccuracyPitchAmount, InaccuracyYawAmount, RollAmount);

	return Rotation;
}

float AWarthog::GetPhase1AndPhase4MoveSpeed() const
{
	return MoveSpeed_Phase1AndPhase4;
}

float AWarthog::GetPhase2AndPhase3MoveSpeed() const
{
	return MoveSpeed_Phase2AndPhase3;
}

EWarthogAttackPhase AWarthog::GetAttackPhase() const
{
	return Phase;
}

float AWarthog::GetPhase2Tilt() const
{
	return Tilt_Phase2;
}

float AWarthog::GetTimeSpentInPhase2AndPhase3() const
{
	return TimeSpentInPhase2AndPhase3;
}

float AWarthog::GetPhase4Tilt() const
{
	return Tilt_Phase4;
}

float AWarthog::GetTimeSpentInPhase4() const
{
	return TimeSpentInPhase4;
}

#if WITH_EDITOR
void AWarthog::PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);
	RunPostEditLogic();
}

void AWarthog::RunPostEditLogic()
{
	Health = MaxHealth;
}
#endif
