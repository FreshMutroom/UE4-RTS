// Fill out your copyright notice in the Description page of Project Settings.

#include "CAbility_Barrage.h"

#include "MapElements/Projectiles/CollidingProjectile.h"


//----------------------------------------------------------------------------------------------
//==============================================================================================
//	------- FBarrageProjectileInfo -------
//==============================================================================================
//----------------------------------------------------------------------------------------------

FBarrageProjectileInfo::FBarrageProjectileInfo()
	: Projectile_BP(ACollidingProjectile::StaticClass()) 
	, bOverrideProjectileDamageValues(false)
	, MinInitialDelay(2.f) 
	, MaxInitialDelay(3.f)
	, MinDuration(10.f)
	, MaxDuration(10.f)
	, MinTimeBetweenSalvos(1.5f)
	, MaxTimeBetweenSalvos(3.f) 
	, SalvoRadius(200.f)
	, MinShotsPerSalvo(3)
	, MaxShotsPerSalvo(6)
	, MinTimeBetweenShots(0.2f)
	, MaxTimeBetweenShots(0.2f)
	, LocationOffset(0.f) 
	, DistanceFromCenterCurve_Salvo(nullptr) 
	, DistanceFromCenterCurve_Projectiles(nullptr)
	, ZAxisOption(ETargetLocationZAxisOption::DoNothing)
{
}

void FBarrageProjectileInfo::SetDamageValues(AObjectPoolingManager * PoolingManager)
{
	/* If pooling manager hasn't been created by the time this function is called then 
	I'll have to think of something else like a new UCommanderAbilityBase virtual 
	like FinalSetup for something and do it then */
	
	if (bOverrideProjectileDamageValues == false)
	{
		AProjectileBase * Projectile = PoolingManager->GetProjectileReference(Projectile_BP);
		ProjectileDamageValues.SetValuesFromProjectile(Projectile);
	}
	
}

void FBarrageProjectileInfo::CheckCurveAssets(UCAbility_Barrage * OwningObject)
{
	if (DistanceFromCenterCurve_Salvo != nullptr)
	{
		float Min_X, Max_X;
		DistanceFromCenterCurve_Salvo->GetTimeRange(Min_X, Max_X);
		if (Min_X != 0.f || Max_X != 1.f)
		{
			// Null it so it will not be used
			DistanceFromCenterCurve_Salvo = nullptr;

			UE_LOG(RTSLOG, Warning, TEXT("Salvo ability [%s]'s DistanceFromCenterCurve_Salvo is "
				"not usable. It will be ignored this play session"), *OwningObject->GetClass()->GetName());
		}
	}

	if (DistanceFromCenterCurve_Projectiles != nullptr)
	{
		float Min_X, Max_X;
		DistanceFromCenterCurve_Projectiles->GetTimeRange(Min_X, Max_X);
		if (Min_X != 0.f || Max_X != 1.f)
		{
			// Null it so it will not be used
			DistanceFromCenterCurve_Projectiles = nullptr;

			UE_LOG(RTSLOG, Warning, TEXT("Salvo ability [%s]'s DistanceFromCenterCurve_Projectiles is "
				"not usable. It will be ignored this play session"), *OwningObject->GetClass()->GetName());
		}
	}
}


//----------------------------------------------------------------------------------------------
//==============================================================================================
//	------- UActiveBarrageSingleProjectileState -------
//==============================================================================================
//----------------------------------------------------------------------------------------------

UActiveBarrageSingleSalvoTypeState::UActiveBarrageSingleSalvoTypeState()
	: TimeSpentAlive(0.f)
	/* Set this to false. Not entierly correct but it means the gap between firing shot 1 and 
	shot 2 of the first salvo is the correct amount */
	, bNextProjectileIsFirstOfSalvo(false)
{
}

void UActiveBarrageSingleSalvoTypeState::Init(FActiveBarrageState * InBarrageState, const FBarrageProjectileInfo & InInfo,
	AObjectPoolingManager * InPoolingManager, UWorld * InWorld, int32 InArrayIndex, float InInitialDelay, float InTimeTillStop) 
{	
	BarrageState = InBarrageState;
	Info = &InInfo;
	PoolingManager = InPoolingManager;
	World = InWorld;
	ArrayIndex = InArrayIndex;
	
	TimeTillNextProjectileSpawn = InInitialDelay;
	TimeTillStop = InTimeTillStop;

	// Do these because bNextProjectileIsFirstOfSalvo is set to false in ctor. Otherwise can leave them out
	NumProjectilesRemainingInSalvo = BarrageState->GetRandomInt(Info->GetMinShotsPerSalvo(), Info->GetMaxShotsPerSalvo());
	CurrentSalvoLocation = CalculateSalvoLocation();
}

void UActiveBarrageSingleSalvoTypeState::Tick(float DeltaTime)
{
	/* This is here because after Stop() is called the object still ticks. If we let 
	tick keep happening Stop() keeps getting called which is bad so put this here guard against 
	that until the object is GCed */
	if (TimeTillStop <= 0.f)
	{
		return;
	}
	
	TimeSpentAlive += DeltaTime;
	
	TimeTillStop -= DeltaTime;
	if (TimeTillStop <= 0.f)
	{
		Stop(); 
	}
	else
	{
		TimeTillNextProjectileSpawn -= DeltaTime;
		while (TimeTillNextProjectileSpawn <= 0.f)
		{
			SpawnProjectile();
		}
	}
}

TStatId UActiveBarrageSingleSalvoTypeState::GetStatId() const
{
	/* I just copied what I saw in engine source */
	RETURN_QUICK_DECLARE_CYCLE_STAT(UActiveBarrageSingleSalvoTypeState, STATGROUP_Tickables);
}

ETickableTickType UActiveBarrageSingleSalvoTypeState::GetTickableTickType() const
{
	// Stop CDO from ever ticking cause it will otherwise
	return IsTemplate() ? ETickableTickType::Never : ETickableTickType::Always;
}

FVector UActiveBarrageSingleSalvoTypeState::CalculateProjectileSpawnLocation() const
{
	/* How many degrees the firer has rotated from its original location */
	const float DegreesRotation = TimeSpentAlive * BarrageState->GetRotationRate();

	/* Rotate original location around Z axis proprtional to the amount of time 
	the ability has been active */
	FVector Vector = BarrageState->GetFirersOriginalLocation() - BarrageState->GetUseLocation();
	Vector = Vector.RotateAngleAxis(DegreesRotation, FVector(0.f, 0.f, 1.f));
	Vector += BarrageState->GetUseLocation();
	// Find vector perpendicular to Vector and up vector. Take into account rotation rate. 
	// I made a rule that +ive LocationOffset is always further up the ship while -ive is 
	// towards the back. Because the sign of RotationRate dictates which direction the ship 
	// is circling we need to take it into account here
	FVector Perpendicular = (BarrageState->GetRotationRate() < 0.f)
		? FVector::CrossProduct(Vector, FVector(0.f, 0.f, 1.f))
		: FVector::CrossProduct(Vector, FVector(0.f, 0.f, -1.f));
	Perpendicular.Normalize();
	Perpendicular *= Info->GetLocationOffset();
	Vector += Perpendicular;
	return Vector;
}

FVector UActiveBarrageSingleSalvoTypeState::CalculateSalvoLocation() const
{
	float SalvoLocationDistanceFromCenter;
	if (Info->GetDistanceFromCenterCurve_SalvoLocation() == nullptr)
	{
		// Linear
		SalvoLocationDistanceFromCenter = BarrageState->GetRandomFloat(0.f, BarrageState->GetRadius());
	}
	else
	{
		SalvoLocationDistanceFromCenter = Info->GetDistanceFromCenterCurve_SalvoLocation()->GetFloatValue(BarrageState->GetRandomFloat(0.f, 1.f)) * BarrageState->GetRadius();
	}

	/* Possibly want to store the last rotation about found out here and enforce a min rotation 
	rule e.g. this value must be at least 50 degrees different from the last value */
	const float SalvoLocationRotationAmount = BarrageState->GetRandomFloat(0.f, 360.f);

	FVector Vector = FVector(1.f, 0.f, 0.f);
	Vector *= SalvoLocationDistanceFromCenter;
	Vector = Vector.RotateAngleAxis(SalvoLocationRotationAmount, FVector(0.f, 0.f, 1.f));
	Vector += BarrageState->GetUseLocation();

	return Vector;
}

FVector UActiveBarrageSingleSalvoTypeState::CalculateProjectileFireAtLocation() const
{
	// The horizonal distance from the use location where the projectile should be fired at
	float DistanceFromCenter;
	if (Info->GetDistanceFromCenterCurve_Projectiles() == nullptr)
	{
		// Linear
		DistanceFromCenter = BarrageState->GetRandomFloat(0.f, Info->GetSalvoRadius());
	}
	else
	{
		DistanceFromCenter = Info->GetDistanceFromCenterCurve_Projectiles()->GetFloatValue(BarrageState->GetRandomFloat(0.f, 1.f)) * Info->GetSalvoRadius();
	}

	// Yaw rotation in degress around the salvo location where the projectile should be fired at
	const float RotationAmount = BarrageState->GetRandomFloat(0.f, 360.f);

	FVector Vector = FVector(1.f, 0.f, 0.f);
	Vector *= DistanceFromCenter;
	Vector = Vector.RotateAngleAxis(RotationAmount, FVector(0.f, 0.f, 1.f));
	Vector += CurrentSalvoLocation;

	// Line trace if requested so location is on the ground and not floating in the air. 
	if (Info->GetZAxisOption() == ETargetLocationZAxisOption::LineTrace)
	{
		const float TraceHalfHeight = 2000.f;
		const FVector TraceStartLoc = FVector(Vector.X, Vector.Y, Vector.Z + TraceHalfHeight);
		const FVector TraceEndLoc = FVector(Vector.X, Vector.Y, Vector.Z - TraceHalfHeight);

		FHitResult HitResult;
		if (World->LineTraceSingleByChannel(HitResult, TraceStartLoc, TraceEndLoc, ENVIRONMENT_CHANNEL))
		{
			Vector.Z = HitResult.ImpactPoint.Z;
		}
	}
	
	return Vector;
}

float UActiveBarrageSingleSalvoTypeState::CalculateTimeBetweenShots() const
{
	float Time;
	if (bNextProjectileIsFirstOfSalvo == false)
	{
		Time = BarrageState->GetRandomFloat(Info->GetMinTimeBetweenShots(), Info->GetMaxTimeBetweenShots());
	}
	else // Have fired all shots for salvo. Return the time gap between salvos
	{
		Time = BarrageState->GetRandomFloat(Info->GetMinTimeBetweenSalvos(), Info->GetMaxTimeBetweenSalvos());
	}

	return Time;
}

void UActiveBarrageSingleSalvoTypeState::SpawnProjectile()
{
	if (bNextProjectileIsFirstOfSalvo)
	{
		NumProjectilesRemainingInSalvo = BarrageState->GetRandomInt(Info->GetMinShotsPerSalvo(), Info->GetMaxShotsPerSalvo());
		CurrentSalvoLocation = CalculateSalvoLocation();
		bNextProjectileIsFirstOfSalvo = false;
	}

	const FVector SpawnLocation = CalculateProjectileSpawnLocation();
	const FVector FireAtLocation = CalculateProjectileFireAtLocation();

	if (World->IsServer())
	{
		PoolingManager->Server_FireProjectileAtLocation(nullptr, Info->GetProjectileBP(),
			Info->GetProjectileDamageValues(), 0.f, BarrageState->GetInstigatorsTeam(), SpawnLocation, FireAtLocation, 0.f);
	}
	else
	{
		PoolingManager->Client_FireProjectileAtLocation(Info->GetProjectileBP(),
			Info->GetProjectileDamageValues(), 0.f, BarrageState->GetInstigatorsTeam(), SpawnLocation, FireAtLocation, 0.f);
	}
	
	NumProjectilesRemainingInSalvo--;

	/* If that was the last projectile fired for that salvo then flag this as true */
	bNextProjectileIsFirstOfSalvo = (NumProjectilesRemainingInSalvo == 0);

	TimeTillNextProjectileSpawn += CalculateTimeBetweenShots();
}

void UActiveBarrageSingleSalvoTypeState::Stop()
{
	BarrageState->OnActiveProjectileTypeFinished(this);
}


//----------------------------------------------------------------------------------------------
//==============================================================================================
//	------- FActiveBarrageState -------
//==============================================================================================
//----------------------------------------------------------------------------------------------

FActiveBarrageState::FActiveBarrageState(UCAbility_Barrage * InEffectActor, AObjectPoolingManager * InPoolingManager, 
	const FVector & InTargetLocation, int32 InUniqueID, ETeam InInstigatorsTeam, int32 InRandomNumberSeed)
	: EffectActor(InEffectActor) 
	, RandomStream(FRandomStream(InRandomNumberSeed))  
	, TargetLocation(InTargetLocation) 
	, FirersOriginalLocation(CalculateFirersOriginalLocation(InEffectActor))
	, Radius(InEffectActor->GetRadius())
	, RotationRate(InEffectActor->GetRotationRate()) 
	, UniqueID(InUniqueID) 
	, InstigatorsTeam(InInstigatorsTeam) 
	, States(TArray<UActiveBarrageSingleSalvoTypeState*>())
{
}

void FActiveBarrageState::MoreSetup(AObjectPoolingManager * InPoolingManager)
{
	UWorld * World = EffectActor->GetWorld();
	States.Reserve(EffectActor->GetNumProjectileTypes());
	for (int32 i = 0; i < EffectActor->GetNumProjectileTypes(); ++i)
	{
		const FBarrageProjectileInfo & Elem = EffectActor->GetProjectileInfo(i);

		const float InitialDelay = GetRandomFloat(Elem.GetMinInitialDelay(), Elem.GetMaxInitialDelay());
		const float Duration = InitialDelay + GetRandomFloat(Elem.GetMinDuration(), Elem.GetMaxDuration());
		UActiveBarrageSingleSalvoTypeState * SingleProjectilState = NewObject<UActiveBarrageSingleSalvoTypeState>();
		SingleProjectilState->Init(this, Elem, InPoolingManager, World, i, InitialDelay, Duration);
		States.Emplace(SingleProjectilState);
	}
}

FVector FActiveBarrageState::CalculateFirersOriginalLocation(UCAbility_Barrage * InEffectActor) const
{
	FVector Vector = FVector(0.f, InEffectActor->GetFirerDistance_Length(), InEffectActor->GetFirerDistance_Height());
	// Apply a random yaw rotation so firer doesn't start at the same relative location every time
	Vector = Vector.RotateAngleAxis(GetRandomFloat(0.f, 360.f), FVector(0.f, 0.f, 1.f));
	Vector += GetUseLocation();
	return Vector;
}

int32 FActiveBarrageState::GetRandomInt(int32 Min, int32 Max) const
{
	return UKismetMathLibrary::RandomIntegerInRangeFromStream(Min, Max, RandomStream);
}

float FActiveBarrageState::GetRandomFloat(float Min, float Max) const
{
	return UKismetMathLibrary::RandomFloatInRangeFromStream(Min, Max, RandomStream);
}

FVector FActiveBarrageState::GetUseLocation() const
{
	return TargetLocation;
}

FVector FActiveBarrageState::GetFirersOriginalLocation() const
{
	return FirersOriginalLocation;
}

float FActiveBarrageState::GetRadius() const
{
	return Radius;
}

float FActiveBarrageState::GetRotationRate() const
{
	return RotationRate;
}

ETeam FActiveBarrageState::GetInstigatorsTeam() const
{
	return InstigatorsTeam;
}

void FActiveBarrageState::OnActiveProjectileTypeFinished(UActiveBarrageSingleSalvoTypeState * Finished)
{
	UE_CLOG(States[Finished->ArrayIndex] != Finished, RTSLOG, Fatal, TEXT("Barrage salvo [%s] "
		"does not have correct array index. Index was [%d]"), *Finished->GetName(), Finished->ArrayIndex);
	
	//-----------------------------------------------------------------------------------------
	//	Remove from container. This should hopefully let Finished be GCed later and hopefully 
	//	disable its tick straight away that would be cool
	//-----------------------------------------------------------------------------------------

	/* Complicated way of writing RemoveSingleSwap when we know the index ahead of time */
	if (States.Num() > 1)
	{
		/* Remove from array and make sure ArrayIndex gets updated for element that gets swapped 
		to where Finished was */
		States.Last()->ArrayIndex = Finished->ArrayIndex;
		States.RemoveAtSwap(Finished->ArrayIndex, 1, false);
	}
	else
	{
		States.RemoveAt(0, 1, false);

		/* Remove ourselves from TMap in UCAbility_Barrage */
		EffectActor->OnEffectInstanceExpired(*this);
	}
}


//----------------------------------------------------------------------------------------------
//==============================================================================================
//	------- UCAbility_Barrage -------
//==============================================================================================
//----------------------------------------------------------------------------------------------

UCAbility_Barrage::UCAbility_Barrage()
{
	//~ Begin UCommanderAbilityBase variables
	bHasMultipleOutcomes =			ECommanderUninitializableBool::False;
	bCallAoEStartFunction =			ECommanderUninitializableBool::False;
	bAoEHitsHaveMultipleOutcomes =	ECommanderUninitializableBool::False;
	bRequiresSelectableTarget =		ECommanderUninitializableBool::False;
	bRequiresPlayerTarget =			ECommanderUninitializableBool::False;
	bRequiresLocation =				ECommanderUninitializableBool::True;
	bHasRandomBehavior =			ECommanderUninitializableBool::True;
	bRequiresTickCount =			ECommanderUninitializableBool::False;
	//~ End UCommanderAbilityBase variables

	Radius = 1500.f;
	FirerDistance_Length = 10000.f;
	FirerDistance_Height = 10000.f;
	FirerRotationRate = -2.f;
}

void UCAbility_Barrage::FinalSetup(URTSGameInstance * GameInst, ARTSGameState * GameState, ARTSPlayerState * LocalPlayersPlayerState)
{
	Super::FinalSetup(GameInst, GameState, LocalPlayersPlayerState);

	AObjectPoolingManager * PoolingManager = GS->GetObjectPoolingManager();
	for (auto & Elem : ProjectileInfo)
	{	
		Elem.SetDamageValues(PoolingManager);
		Elem.CheckCurveAssets(this);
	}
}

void UCAbility_Barrage::Server_Execute(FUNCTION_SIGNATURE_ServerExecute)
{
	SuperServerExecute;

	OutRandomNumberSeed = GenerateInitialRandomSeed();

	static int32 UniqueID = 0;
	UniqueID++;

	AObjectPoolingManager * PoolingManager = GS->GetObjectPoolingManager();

	const FSetElementId ElemID = ActiveBarrages.Emplace(FActiveBarrageState(this, PoolingManager,
		Location, UniqueID, InstigatorsTeam, UCommanderAbilityBase::SeedAs16BitTo32Bit(OutRandomNumberSeed)));

	ActiveBarrages[ElemID].MoreSetup(PoolingManager);

	// Reveal fog of war at use location
	RevealFogAtTargetLocation(Location, InstigatorsTeam, true);

	PlaySound(AbilityInstigator);
}

void UCAbility_Barrage::Client_Execute(FUNCTION_SIGNATURE_ClientExecute)
{
	SuperClientExecute;

	static int32 UniqueID = 0;
	UniqueID++;

	AObjectPoolingManager * PoolingManager = GS->GetObjectPoolingManager();

	const FSetElementId ElemID = ActiveBarrages.Emplace(FActiveBarrageState(this, PoolingManager,
		Location, UniqueID, InstigatorsTeam, RandomNumberSeed));

	ActiveBarrages[ElemID].MoreSetup(PoolingManager);
 
	// Reveal fog of war at use location
	RevealFogAtTargetLocation(Location, InstigatorsTeam, false);

	PlaySound(AbilityInstigator);
}

void UCAbility_Barrage::RevealFogAtTargetLocation(const FVector & AbilityLocation, ETeam InstigatorsTeam, 
	bool bIsServer)
{
#if GAME_THREAD_FOG_OF_WAR

	TargetLocationTemporaryFogReveal.CreateForTeam(GS->GetFogManager(), FVector2D(AbilityLocation),
		InstigatorsTeam, bIsServer, PS->GetTeam() == InstigatorsTeam);

#elif MULTITHREADED_FOG_OF_WAR

	TargetLocationTemporaryFogReveal.CreateForTeam(FVector2D(TargetLocation),
		InstigatorsTeam, bIsServer, PS->GetTeam() == InstigatorsTeam);

#endif
}

void UCAbility_Barrage::PlaySound(ARTSPlayerState * AbilityInsitgator)
{
	// Play sound only for the player that instigated the ability
	if (UseSound != nullptr && AbilityInsitgator == PS) 
	{
		Statics::PlaySound2D(this, UseSound); 
	}
}

void UCAbility_Barrage::OnEffectInstanceExpired(FActiveBarrageState & Expired)
{
	ActiveBarrages.Remove(Expired);
}

int32 UCAbility_Barrage::GetNumProjectileTypes() const
{
	return ProjectileInfo.Num();
}

const FBarrageProjectileInfo & UCAbility_Barrage::GetProjectileInfo(int32 Index) const
{
	return ProjectileInfo[Index];
}

float UCAbility_Barrage::GetRadius() const
{
	return Radius;
}

float UCAbility_Barrage::GetFirerDistance_Length() const
{
	return FirerDistance_Length;
}

float UCAbility_Barrage::GetFirerDistance_Height() const
{
	return FirerDistance_Height;
}

float UCAbility_Barrage::GetRotationRate() const
{
	return FirerRotationRate;
}

#if WITH_EDITOR
void UCAbility_Barrage::PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	TargetLocationTemporaryFogReveal.OnPostEdit();
}
#endif
