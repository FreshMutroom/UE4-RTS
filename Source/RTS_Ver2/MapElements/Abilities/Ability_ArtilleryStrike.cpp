// Fill out your copyright notice in the Description page of Project Settings.

#include "Ability_ArtilleryStrike.h"
#include "Engine/World.h"
#include "Public/TimerManager.h"
#include "Kismet/KismetMathLibrary.h"

#include "Statics/DevelopmentStatics.h"
#include "MapElements/Projectiles/CollidingProjectile.h"
#include "Managers/ObjectPoolingManager.h"
#include "MapElements/Projectiles/NoCollisionTimedProjectile.h"


FArtilleryStrikeInfo::FArtilleryStrikeInfo()
{
	// Commented becaue apparently 4.21 crashes here now
	//assert(0);
}

FArtilleryStrikeInfo::FArtilleryStrikeInfo(const FVector & InLocation, uint32 InNumProjectiles, 
	AActor * InInstigator, ETeam InInstigatorsTeam, const FRandomStream & InRandomStream)
	: Location(InLocation) 
	, NumProjectilesRemaining(InNumProjectiles) 
	, Instigator(InInstigator)
	, InstigatorsTeam(InInstigatorsTeam) 
	, RandomStream(InRandomStream) 
	, LastYawRot(UKismetMathLibrary::RandomFloatInRangeFromStream(0.f, 360.f, InRandomStream))
{
	/* If LastYawRot is left as 0 then first projectile spawned could always avoid a certain side */
}


AAbility_ArtilleryStrike::AAbility_ArtilleryStrike()
{
	//~ Begin AAbilityBase interface
	bHasMultipleOutcomes = EUninitializableBool::False;
	bCallAoEStartFunction = EUninitializableBool::False;
	bAoEHitsHaveMultipleOutcomes = EUninitializableBool::False;
	bRequiresTargetOtherThanSelf = EUninitializableBool::False;
	bRequiresLocation = EUninitializableBool::True;
	bHasRandomBehavior = EUninitializableBool::True;
	bRequiresTickCount = EUninitializableBool::False;
	//~ End AAbilityBase interface

	BeaconProjectileDamageInfo.ImpactDamage = 0.f;
	Radius = 600.f;
	DamageInfo.AoEDamage = 20.f;
	NumProjectiles = 10;
	MinInterval = 0.3f;
	MaxInterval = 0.6f;
	SpawnHeight = 2000.f;
}

void AAbility_ArtilleryStrike::BeginPlay()
{
	Super::BeginPlay();

	/* Set reference to pooling manager */
	URTSGameInstance * GameInst = CastChecked<URTSGameInstance>(GetGameInstance());
	PoolingManager = GameInst->GetPoolingManager();
	assert(PoolingManager != nullptr);

	UE_CLOG(Projectile_BP == nullptr, RTSLOG, Fatal, TEXT("Artillery strike class [%s] does not "
		"have a projectile blueprint set."), *GetClass()->GetName());

	PoolingManager->AddProjectileBP(Projectile_BP);

	SetupCurveValues();

	if (!bOverrideProjectileDamageValues)
	{
		/* Spawn projectile and set values from it. 
		Have projectile pools been created yet? If so we could check if pooling manager already 
		has a projectile spawned and just use that */
		DamageInfo.SetValuesFromProjectile(GetWorld(), Projectile_BP);
	}
}

void AAbility_ArtilleryStrike::Server_Begin(FUNCTION_SIGNATURE_ServerBegin)
{
	SuperServerBegin;

	OutRandomNumberSeed = GenerateInitialRandomSeed();

	/* Teleport higher from where originally spawned */
	const FVector StartLoc = Location + FVector(0.f, 0.f, SpawnHeight);

	/* It's ok to end up reusing the same IDs eventually. Just as long as 4 billion+ artillery 
	strikes aren't active at the same time then we're ok */
	static int32 UniqueID = 0;

	UniqueID++;

	/* Remember that this is an info type actor so we store the information about this strike 
	in a TMap and will use it to keep track of its state */
	ActiveStrikes.Emplace(UniqueID, FArtilleryStrikeInfo(StartLoc, NumProjectiles, EffectInstigator, 
		InstigatorsTeam, FRandomStream(SeedAs16BitTo32Bit(OutRandomNumberSeed))));

	if (BeaconProjectile_BP != nullptr)
	{ 
		PoolingManager->Server_FireProjectileAtLocation(EffectInstigator, BeaconProjectile_BP, 
			BeaconProjectileDamageInfo, 0.f, InstigatorsTeam, InstigatorAsSelectable->GetMuzzleLocation(), 
			Location, 0.f, this, UniqueID);
	}
	else
	{
		/* Begin spawning projectiles */
		Delay_SpawnProjectile(UniqueID, InitialDelay);
	}
}

void AAbility_ArtilleryStrike::Client_Begin(FUNCTION_SIGNATURE_ClientBegin)
{
	SuperClientBegin;

	/* Teleport higher from where originally spawned */
	const FVector StartLoc = Location + FVector(0.f, 0.f, SpawnHeight);

	static int32 UniqueID = 0;

	UniqueID++;

	ActiveStrikes.Emplace(UniqueID, FArtilleryStrikeInfo(StartLoc, NumProjectiles, EffectInstigator, 
		InstigatorsTeam, FRandomStream(RandomNumberSeed)));

	if (BeaconProjectile_BP != nullptr)
	{
		PoolingManager->Client_FireProjectileAtLocation(BeaconProjectile_BP, BeaconProjectileDamageInfo,
			0.f, InstigatorsTeam, InstigatorAsSelectable->GetMuzzleLocation(), Location, 0.f, this, UniqueID);
	}
	else
	{
		/* Begin spawning projectiles */
		Delay_SpawnProjectile(UniqueID, InitialDelay);
	}
}

void AAbility_ArtilleryStrike::OnListenedProjectileHit(int32 AbilityInstanceUniqueID)
{
	/* This being called means the beacon has reached its target */

	Delay_SpawnProjectile(AbilityInstanceUniqueID, InitialDelay);
}

FVector AAbility_ArtilleryStrike::GetRandomSpot(int32 UniqueID)
{
	FArtilleryStrikeInfo & CurrentStrike = ActiveStrikes[UniqueID];

	const float LengthFromCenter = GetRandomDistanceFromCenter(UniqueID);
	const float YawRot = GetRandomYawRotation(CurrentStrike.GetLastYawRot(), UniqueID);

	/* Only rotate around Z axis */
	const FRotator Rotation = FRotator(0.f, YawRot, 0.f);

	FVector Vector = FVector(LengthFromCenter, 0.f, 0.f);
	Vector = Rotation.RotateVector(Vector);

	CurrentStrike.SetLastYawRot(YawRot);

	return CurrentStrike.GetLocation() + Vector;
}

float AAbility_ArtilleryStrike::GetRandomDistanceFromCenter(int32 UniqueID)
{
	FArtilleryStrikeInfo & CurrentStrike = ActiveStrikes[UniqueID];
	
	if (DistanceFromCenterCurve != nullptr)
	{
		/* Get random X axis value and it's Y value */
		const float XAxisValue = GetRandomFloat(CurrentStrike.GetRandomStream(), DistanceCurveMinX, DistanceCurveMaxX);
		const float YAxisValue = DistanceFromCenterCurve->GetFloatValue(XAxisValue);

		/* Return Y axis value mapped from [Curve.YAxis.Min, Curve.YAxis.Max] to [0, Radius] */
		const FVector2D InRange = FVector2D(DistanceCurveMinY, DistanceCurveMaxY);
		const FVector2D OutRange = FVector2D(0.f, Radius);
		return FMath::GetMappedRangeValueUnclamped(InRange, OutRange, YAxisValue);
	}
	else
	{
		return GetRandomFloat(CurrentStrike.GetRandomStream(), 0.f, Radius);
	}
}

float AAbility_ArtilleryStrike::GetRandomYawRotation(float InYaw, int32 UniqueID)
{
	FArtilleryStrikeInfo & CurrentStrike = ActiveStrikes[UniqueID];
	
	if (YawRotationCurve != nullptr)
	{
		/* Get random X axis value and it's Y value */
		const float XAxisValue = GetRandomFloat(CurrentStrike.GetRandomStream(), RotationCurveMinX, RotationCurveMaxX);
		const float YAxisValue = YawRotationCurve->GetFloatValue(XAxisValue);

		/* Map Y axis value from [Curve.YAxis.Min, Curve.YAxis.Max] to [0, 180] */
		const FVector2D InRange = FVector2D(RotationCurveMinY, RotationCurveMaxY);
		const FVector2D OutRange = FVector2D(0.f, 180.f);
		float Rotation = FMath::GetMappedRangeValueUnclamped(InRange, OutRange, YAxisValue);

		/* Give random sign */
		const float Sign = GetRandomFloat(CurrentStrike.GetRandomStream(), 0.f, 1.f) < 0.5f ? 1.f : -1.f;
		Rotation *= Sign;

		/* Clamp to between [0, 360). Maybe don't need it */
		Rotation = FMath::Fmod(Rotation, 360.f);

		return InYaw + Rotation;
	}
	else
	{
		/* Same as linear curve */
		float Rotation = GetRandomFloat(CurrentStrike.GetRandomStream(), 0.f, 180.f);

		/* Give random sign */
		const float Sign = GetRandomFloat(CurrentStrike.GetRandomStream(), 0.f, 1.f) < 0.5f ? 1.f : -1.f;
		Rotation *= Sign;

		/* Clamp to between[0, 360). Maybe don't need it */
		Rotation = FMath::Fmod(Rotation, 360.f);

		return CurrentStrike.GetLastYawRot() + Rotation;
	}
}

void AAbility_ArtilleryStrike::SpawnProjectile(int32 UniqueID)
{
	FArtilleryStrikeInfo & StrikeInfo = ActiveStrikes[UniqueID];

	StrikeInfo.DecrementNumProjectiles();

	const FVector SpawnLoc = GetRandomSpot(UniqueID);
	/* Subtract 1000. Z axis value's magnitude does not really mean anything - the spot just needs 
	to be directly below spawn loc so projectile fires straight down */
	const FVector TargetLoc = SpawnLoc + FVector(0.f, 0.f, -1000.f);
	/* Give projectile some random roll rotation */
	const float RandomRollRot = GetRandomFloat(StrikeInfo.GetRandomStream(), 0.f, 360.f);

	/* Fire projectile from object pool */
	if (GetWorld()->IsServer())
	{
		PoolingManager->Server_FireProjectileAtLocation(StrikeInfo.GetInstigator(), Projectile_BP,
			DamageInfo, 0.f, StrikeInfo.GetInstigatorsTeam(), SpawnLoc, TargetLoc, RandomRollRot);
	}
	else
	{
		PoolingManager->Client_FireProjectileAtLocation(Projectile_BP, DamageInfo, 0.f,
			StrikeInfo.GetInstigatorsTeam(), SpawnLoc, TargetLoc, RandomRollRot);
	}

	if (StrikeInfo.GetNumProjectilesRemaining() <= 0)
	{
		/* Done spawning projectiles */
		EndEffect(UniqueID);
	}
	else
	{
		const float NextDelay = GetRandomFloat(StrikeInfo.GetRandomStream(), MinInterval, MaxInterval);

		/* Spawn next projectile soon */
		Delay_SpawnProjectile(UniqueID, NextDelay);
	}
}

void AAbility_ArtilleryStrike::EndEffect(int32 UniqueID)
{
	ActiveStrikes.FindAndRemoveChecked(UniqueID);
}

void AAbility_ArtilleryStrike::SetupCurveValues()
{
	/* All this can be done whenever the curves change. Not sure if changing the curves
	values truggers post edit function though */

	if (YawRotationCurve != nullptr)
	{
		YawRotationCurve->GetTimeRange(RotationCurveMinX, RotationCurveMaxX);
		YawRotationCurve->GetValueRange(RotationCurveMinY, RotationCurveMaxY);

		/* If true then curve either has no points or is just a vertical or horizontal line.
		Flag curve to be ignored in calculations and from here on out the behavior of a linear 
		curve is used */
		if (RotationCurveMinY == RotationCurveMaxY || RotationCurveMaxX == RotationCurveMinX)
		{
			YawRotationCurve = nullptr;
		}
	}

	if (DistanceFromCenterCurve != nullptr)
	{
		DistanceFromCenterCurve->GetTimeRange(DistanceCurveMinX, DistanceCurveMaxX);
		DistanceFromCenterCurve->GetValueRange(DistanceCurveMinY, DistanceCurveMaxY);

		/* If true then curve either has no points or is just a vertical or horizontal line.
		Flag curve to be ignored in calculations and from here on out the behavior of a linear
		curve is used */
		if (DistanceCurveMaxY == DistanceCurveMinY || DistanceCurveMaxX == DistanceCurveMinX)
		{
			DistanceFromCenterCurve = nullptr;
		}
	}
}

void AAbility_ArtilleryStrike::Delay_SpawnProjectile(int32 UniqueID, float Delay)
{
	if (Delay <= 0.f)
	{
		/* Call function now */
		SpawnProjectile(UniqueID);
	}
	else
	{
		// Function to call after delay
		static const FName FuncName = GET_FUNCTION_NAME_CHECKED(AAbility_ArtilleryStrike, SpawnProjectile);

		FTimerDelegate TimerDel;
		TimerDel.BindUFunction(this, FuncName, UniqueID);

		FTimerHandle & TimerHandle = ActiveStrikes[UniqueID].GetActionTimerHandle();
		GetWorldTimerManager().SetTimer(TimerHandle, TimerDel, Delay, false);
	}
}
