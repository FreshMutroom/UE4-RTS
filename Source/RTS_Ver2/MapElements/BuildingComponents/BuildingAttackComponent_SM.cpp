// Fill out your copyright notice in the Description page of Project Settings.


#include "BuildingAttackComponent_SM.h"
#include "Kismet/KismetMathLibrary.h"

#include "Statics/Statics.h"
#include "Managers/ObjectPoolingManager.h"
#include "Statics/DevelopmentStatics.h"


UBuildingAttackComponent_SM::UBuildingAttackComponent_SM()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	SetGenerateOverlapEvents(false);
	SetCanEverAffectNavigation(false);
	CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	bReceivesDecals = false;
	SetCollisionProfileName(FName("BuildingMesh"));

	TargetingTraceDelegate.BindUObject(this, &UBuildingAttackComponent_SM::OnSweepForTargetsComplete);

	RangeLenienceMultiplier = 1.1f;
	TimeAttackWarmupStarted = -1.f;
	TimeSpentWarmingUpAttack = -1.f;
	TargetAquireMethod = ETargetAquireMethodPriorties::None;
	TargetAquirePreferredDistance = EDistanceCheckMethod::Furtherest;
	bHasTunnelVision = true;
	YawFacingRequirement = 180.f;
	AttackCompUniqueID = UINT8_MAX;
}

void UBuildingAttackComponent_SM::BeginPlay()
{
	Super::BeginPlay();

	if (GetWorld()->IsServer() == false)
	{
		SetComponentTickEnabled(false);
	}
}

void UBuildingAttackComponent_SM::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction * ThisTickFunction)
{
	// Currently only the server needs to tick component
	SERVER_CHECK;

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Try attack target.
	if (HasAttackWarmup())
	{
		// Check if attack is ready
		if (HasAttackFullyWarmedUp()) 
		{
			const ETargetTargetingCheckResult TargetingStatus = GetCurrentTargetTargetingStatus();
			if (TargetingStatus == ETargetTargetingCheckResult::CanAttack)
			{
				ServerDoAttack();
			}
			else
			{
				SetTarget(nullptr);
			}
		}
		else if (IsAttackWarmingUp())
		{
			TimeSpentWarmingUpAttack += DeltaTime;
		}
		else
		{
			TimeRemainingTillAttackOffCooldown -= DeltaTime;
			TimeRemainingTillAttackOffCooldown = FMath::Max(0.f, TimeRemainingTillAttackOffCooldown);
			if (TimeRemainingTillAttackOffCooldown <= 0.f)
			{
				const ETargetTargetingCheckResult TargetingStatus = GetCurrentTargetTargetingStatus();
				if (TargetingStatus == ETargetTargetingCheckResult::CanAttack)
				{
					StartAttackWarmUp();
				}
				else if (TargetingStatus == ETargetTargetingCheckResult::Unaquire)
				{
					SetTarget(nullptr);
				}
			}
		}
	}
	else
	{
		TimeRemainingTillAttackOffCooldown -= DeltaTime;
		if (TimeRemainingTillAttackOffCooldown <= 0.f)
		{
			const ETargetTargetingCheckResult TargetingStatus = GetCurrentTargetTargetingStatus();
			if (TargetingStatus == ETargetTargetingCheckResult::CanAttack)
			{
				ServerDoAttack();
			}
			else if (TargetingStatus == ETargetTargetingCheckResult::Unaquire)
			{
				SetTarget(nullptr);
				TimeRemainingTillAttackOffCooldown = 0.f;
			}
			else
			{
				TimeRemainingTillAttackOffCooldown = 0.f;
			}
		}
	}
}

void UBuildingAttackComponent_SM::ServerDoAttack()
{	
	const FVector MuzzleLocation = GetSocketLocation(AttackAttributes.GetMuzzleSocket());
	const FRotator MuzzleRotation = GetSocketRotation(AttackAttributes.GetMuzzleSocket());

	// Fire projectile or projectiles
	while (TimeRemainingTillAttackOffCooldown <= 0.f)
	{
		TimeRemainingTillAttackOffCooldown += AttackAttributes.GetAttackRate();

		PoolingManager->Server_FireProjectileAtTarget(OwningBuilding, AttackAttributes.GetProjectileBP(),
			AttackAttributes.GetDamageInfo(), AttackAttributes.GetAttackRange(), Team,
			MuzzleLocation, Target.Get(), 0.f);
	}

	//----------------------------------------------------------------------------------
	//	Particles and sounds are outside the loop. Even if we fire 100 projectiles in a 
	//	single frame it's best for performance that we only show one particles 
	//	and play one sound. Actually I have done the same with the firing of the 
	//	projectile too to remove strain on net driver.
	//----------------------------------------------------------------------------------

	// RPC event of firing projectile
	OwningBuilding->Multicast_OnBuildingAttackComponentAttackMade(AttackCompUniqueID, Target.Get());

	// Create muzzle particles
	if (AttackAttributes.GetMuzzleParticles() != nullptr)
	{
		Statics::SpawnFogParticles(AttackAttributes.GetMuzzleParticles(), GS, MuzzleLocation, MuzzleRotation, FVector::OneVector);
	}

	// Play sound
	if (AttackAttributes.GetAttackMadeSound() != nullptr)
	{
		Statics::SpawnSoundAtLocation(GS, Team, AttackAttributes.GetAttackMadeSound(),
			MuzzleLocation, ESoundFogRules::DecideOnSpawn, MuzzleRotation);
	}

	/* Optional if statement */
	//if (HasAttackWarmup())
	{
		TimeAttackWarmupStarted = -1.f;
		TimeSpentWarmingUpAttack = -1.f;

		// Don't stop the warm up particles or sound. It is assumed their lengths are the 
		// same length as the warm up duration so they'll stop on their own
	}
}

void UBuildingAttackComponent_SM::ClientDoAttack(AActor * AttackTarget)
{
	CLIENT_CHECK;

	// Valid check because networking. I assume the team check might also be needed since 
	// RPCs can be called on OwningBuilding before Setup() has been called probably
	if (Team != ETeam::Uninitialized && Statics::IsValid(AttackTarget))
	{
		const FVector MuzzleLocation = GetSocketLocation(AttackAttributes.GetMuzzleSocket());
		const FRotator MuzzleRotation = GetSocketRotation(AttackAttributes.GetMuzzleSocket());

		PoolingManager->Client_FireProjectileAtTarget(AttackAttributes.GetProjectileBP(),
			AttackAttributes.GetDamageInfo(), AttackAttributes.GetAttackRange(), Team,
			MuzzleLocation, AttackTarget, 0.f);

		// Create muzzle particles
		if (AttackAttributes.GetMuzzleParticles() != nullptr)
		{
			Statics::SpawnFogParticles(AttackAttributes.GetMuzzleParticles(), GS, MuzzleLocation, MuzzleRotation, FVector::OneVector);
		}

		// Play sound
		if (AttackAttributes.GetAttackMadeSound() != nullptr)
		{
			Statics::SpawnSoundAtLocation(GS, Team, AttackAttributes.GetAttackMadeSound(),
				MuzzleLocation, ESoundFogRules::DecideOnSpawn, MuzzleRotation);
		}
	}
}

void UBuildingAttackComponent_SM::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> & OutLifetimeProps) const
{
	// Hoping that all the DOREPLIFETIMEs in USceneComponent::GetLifetimeReplicatedProps
	// and aren't UActorComponent::GetLifetimeReplicatedProps really needed. I'm not going to be detatching it or anything and it 
	// won't move either

	DOREPLIFETIME(UBuildingAttackComponent_SM, TimeAttackWarmupStarted);
}

UMeshComponent * UBuildingAttackComponent_SM::GetAsMeshComponent()
{
	return this;
}

void UBuildingAttackComponent_SM::SetupAttackComp(ABuilding * InOwningBuilding, ARTSGameState * GameState, AObjectPoolingManager * InPoolingManager, uint8 InUniqueID, IBuildingAttackComp_TurretsBase * InTurretRotatingBase)
{
	OwningBuilding = InOwningBuilding;
	GS = GameState;
	PoolingManager = InPoolingManager;
	AttackCompUniqueID = InUniqueID;
	SetRotatingBase(InTurretRotatingBase);
}

void UBuildingAttackComponent_SM::ServerSetupAttackCompMore(ETeam InTeam, const FVisibilityInfo * InTeamsVisibilityInfo, FCollisionObjectQueryParams InQueryParams)
{
	Team = InTeam;
	TeamVisibilityInfo = InTeamsVisibilityInfo;
	QueryParams = InQueryParams;
}

void UBuildingAttackComponent_SM::ClientSetupAttackCompMore(ETeam InTeam)
{
	Team = InTeam;
}

FOverlapDelegate * UBuildingAttackComponent_SM::GetTargetingTraceDelegate()
{
	return &TargetingTraceDelegate;
}

FCollisionObjectQueryParams UBuildingAttackComponent_SM::GetTargetingQueryParams() const
{
	return QueryParams;
}

FVector UBuildingAttackComponent_SM::GetTargetingSweepOrigin() const
{
	return GetComponentLocation();
}

float UBuildingAttackComponent_SM::GetTargetingSweepRadius() const
{
	return AttackAttributes.GetAttackRange();
}

void UBuildingAttackComponent_SM::SetTaskManagerBucketIndices(uint8 BucketIndex, int16 ArrayIndex)
{
	TaskManagerBucket = FTaskManagerBucketInfo(BucketIndex, ArrayIndex);
}

void UBuildingAttackComponent_SM::SetTaskManagerArrayIndex(int16 ArrayIndex)
{
	TaskManagerBucket.ArrayIndex = ArrayIndex;
}

void UBuildingAttackComponent_SM::GrabTaskManagerBucketIndices(uint8 & OutBucketIndex, int16 & OutArrayIndex) const
{
	OutBucketIndex = TaskManagerBucket.BucketIndex;
	OutArrayIndex = TaskManagerBucket.ArrayIndex;
}

void UBuildingAttackComponent_SM::OnParentBuildingEnterFogOfWar(bool bOnServer)
{
	// I think this can be a NOOP. I spent a lot of time not bothering to implement this 
	// and I think in my head I thought it would be a noop
}

void UBuildingAttackComponent_SM::OnParentBuildingExitFogOfWar(bool bOnServer)
{
	// NOOP. The turret base will update it's own rotation and ours in it's OnParentBuildingExitFogOfWar implementation
}

void UBuildingAttackComponent_SM::SetRotatingBase(IBuildingAttackComp_TurretsBase * InTurretRotatingBase)
{
	RotatingBase = InTurretRotatingBase;
	if (HasRotatingBase() == false)
	{
		/* Remove the rotation required part from the target aquiring method - this structure
		cannot rotate it's yaw so it should only bother aquiring targets in it's attack cone */
		if (TargetAquireMethod == ETargetAquireMethodPriorties::LeastRotationRequired)
		{
			TargetAquireMethod = ETargetAquireMethodPriorties::None;
		}
		else if (TargetAquireMethod == ETargetAquireMethodPriorties::HasAttack_LeastRotationRequired)
		{
			TargetAquireMethod = ETargetAquireMethodPriorties::HasAttack;
		}
	}
}

bool UBuildingAttackComponent_SM::HasRotatingBase() const
{
	return RotatingBase != nullptr;
}

bool UBuildingAttackComponent_SM::IsAttackOffCooldown() const
{
	return TimeRemainingTillAttackOffCooldown <= 0.f;
}

bool UBuildingAttackComponent_SM::HasAttackWarmup() const
{
	return AttackWarmupTime > 0.f;
}

bool UBuildingAttackComponent_SM::HasAttackFullyWarmedUp() const
{
	return TimeSpentWarmingUpAttack >= AttackWarmupTime;
}

bool UBuildingAttackComponent_SM::IsAttackWarmingUp() const
{
	return TimeSpentWarmingUpAttack >= 0.f && HasAttackFullyWarmedUp() == false;
}

void UBuildingAttackComponent_SM::OnSweepForTargetsComplete(const FTraceHandle & TraceHandle, FOverlapDatum & TraceData)
{
	/* Aquire target or switch to another if the current target is no longer attackable */
	
	if (bHasTunnelVision == false)
	{
		if (IsCurrentTargetStillAquirable())
		{
			BuldingTurretStatics::SelectTargetFromOverlapTest(this, TraceData, TargetAquireMethod, TargetAquirePreferredDistance, false);
		}
		else
		{
			BuldingTurretStatics::SelectTargetFromOverlapTest(this, TraceData, TargetAquireMethod, TargetAquirePreferredDistance, true);
		}
	}
	else if (IsCurrentTargetStillAquirable() == false)
	{
		BuldingTurretStatics::SelectTargetFromOverlapTest(this, TraceData, TargetAquireMethod, TargetAquirePreferredDistance, true);
	}
}

bool UBuildingAttackComponent_SM::CanSweptActorBeAquiredAsTarget(AActor * Actor) const
{
	if (Statics::IsValid(Actor) == false)
	{
		return false;
	}
	if (Statics::HasZeroHealth(Actor) == true)
	{
		return false;
	}
	if (AttackAttributes.CanAttackAir() == false && Statics::IsAirUnit(Actor))
	{
		return false;
	}
	if (Statics::CanTypeBeTargeted(Actor, AttackAttributes.GetAcceptableTargetTypes()) == false)
	{
		return false;
	}
	// Check angle
	if (HasRotatingBase() == false && YawFacingRequirement < 180.f && GetYawToActor(Actor) > YawFacingRequirement)
	{
		return false;
	}
	if (Statics::IsSelectableVisible(Actor, TeamVisibilityInfo) == false)
	{
		return false;
	}

	return true;
}

bool UBuildingAttackComponent_SM::CanSweptActorBeAquiredAsTarget(AActor * Actor, float & OutRotationRequiredToFace) const
{
	assert(HasRotatingBase() == true);
	/* This func is only expected to be called if the structure cannot attack at all angles
	i.e. YawFacingRequirement < 180.f */
	assert(YawFacingRequirement < 180.f);

	if (Statics::IsValid(Actor) == false)
	{
		return false;
	}
	if (Statics::HasZeroHealth(Actor) == true)
	{
		return false;
	}
	if (AttackAttributes.CanAttackAir() == false && Statics::IsAirUnit(Actor))
	{
		return false;
	}
	if (Statics::CanTypeBeTargeted(Actor, AttackAttributes.GetAcceptableTargetTypes()) == false)
	{
		return false;
	}
	if (Statics::IsSelectableVisible(Actor, TeamVisibilityInfo) == false)
	{
		return false;
	}

	OutRotationRequiredToFace = GetYawToActor(Actor);

	return true;
}

bool UBuildingAttackComponent_SM::IsCurrentTargetStillAquirable() const
{
	AActor * Tar = Target.Get();
	if (Statics::IsValid(Tar) == false)
	{
		return false;
	}
	if (Statics::HasZeroHealth(Tar) == true)
	{
		return false;
	}
	// Perhaps target's gained flying now so check
	if (AttackAttributes.CanAttackAir() == false && Statics::IsAirUnit(Tar))
	{
		return false;
	}
	if (IsActorInRange_UseLeneince(Tar) == false)
	{
		return false;
	}
	// Check angle
	if (HasRotatingBase() == false && YawFacingRequirement < 180.f && GetYawToActor(Tar) > YawFacingRequirement)
	{
		return false;
	}
	if (Statics::IsSelectableVisible(Tar, TeamVisibilityInfo) == false)
	{
		return false;
	}

	return true;
}

UBuildingAttackComponent_SM::ETargetTargetingCheckResult UBuildingAttackComponent_SM::GetCurrentTargetTargetingStatus() const
{
	AActor * Tar = Target.Get();
	if (Statics::IsValid(Tar) == false)
	{
		return ETargetTargetingCheckResult::Unaquire;
	}
	if (Statics::HasZeroHealth(Tar) == true)
	{
		return ETargetTargetingCheckResult::Unaquire;
	}
	// Perhaps target's gained flying now so check
	if (AttackAttributes.CanAttackAir() == false && Statics::IsAirUnit(Tar))
	{
		return ETargetTargetingCheckResult::Unaquire;
	}
	if (IsActorInRange_UseLeneince(Tar) == false)
	{
		return ETargetTargetingCheckResult::Unaquire;
	}
	// Check angle
	if (YawFacingRequirement < 180.f && GetYawToActor(Tar) > YawFacingRequirement)
	{
		return HasRotatingBase() ? ETargetTargetingCheckResult::CannotAttack : ETargetTargetingCheckResult::Unaquire;
	}
	if (Statics::IsSelectableVisible(Tar, TeamVisibilityInfo) == false)
	{
		return ETargetTargetingCheckResult::Unaquire;
	}

	return ETargetTargetingCheckResult::CanAttack;
}

bool UBuildingAttackComponent_SM::IsActorInRange_NoLenience(AActor * Actor) const
{
	// Probably want to take bounds into account here, and maybe throw in some leneince too 
	const float DistanceSqr = Statics::GetDistance2DSquared(GetComponentLocation(), Actor->GetActorLocation());
	return DistanceSqr <= FMath::Square(AttackAttributes.GetAttackRange());
}

bool UBuildingAttackComponent_SM::IsActorInRange_UseLeneince(AActor * Actor) const
{
	const float DistanceSqr = Statics::GetDistance2DSquared(GetComponentLocation(), Actor->GetActorLocation());
	return DistanceSqr <= FMath::Square(AttackAttributes.GetAttackRange() * RangeLenienceMultiplier);
}

float UBuildingAttackComponent_SM::GetYawToActor(AActor * Actor) const
{
	/* Maybe wanna take bounds into consideration here */

	const float OurYawRotation = GetEffectiveComponentYawRotation();
	const FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(GetComponentLocation(), Actor->GetActorLocation());

	float Difference;
	if (OurYawRotation > LookAtRotation.Yaw)
	{
		Difference = OurYawRotation - LookAtRotation.Yaw;
	}
	else
	{
		Difference = LookAtRotation.Yaw - OurYawRotation;
	}

	return Difference > 180.f ? (Difference - 360.f) * -1.f : Difference;
}

float UBuildingAttackComponent_SM::GetYawToActor(AActor * Actor, FRotator & OutLookAtRot) const
{
	const float OurYawRotation = GetEffectiveComponentYawRotation();
	const FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(GetComponentLocation(), Actor->GetActorLocation());
	OutLookAtRot = LookAtRotation;

	float Difference;
	if (OurYawRotation > LookAtRotation.Yaw)
	{
		Difference = OurYawRotation - LookAtRotation.Yaw;
	}
	else
	{
		Difference = LookAtRotation.Yaw - OurYawRotation;
	}

	return Difference > 180.f ? (Difference - 360.f) * -1.f : Difference;
}

float UBuildingAttackComponent_SM::GetEffectiveComponentYawRotation() const
{
	return RotatingBase->GetEffectiveComponentYawRotation();
}

void UBuildingAttackComponent_SM::SetTarget(AActor * InTarget)
{
	Target = InTarget;
	
	if (HasAttackWarmup())
	{
		if (Target == nullptr)
		{
			if (IsAttackWarmingUp() || HasAttackFullyWarmedUp())
			{
				ServerStopAttackWarmUp();
			}
		}
		else
		{
			if (IsAttackOffCooldown() == true && IsAttackWarmingUp() == false && HasAttackFullyWarmedUp() == false)
			{
				StartAttackWarmUp();
			}
		}
	}
}

AActor * UBuildingAttackComponent_SM::GetTarget() const
{
	return Target.Get();
}

void UBuildingAttackComponent_SM::StartAttackWarmUp()
{
	// Start particles
	if (AttackPreparationParticles != nullptr)
	{
		/* Would it be faster to like rewind WarmupParticlesComp and play from start instead 
		if it's valid? Possible do the same for audio comp too */
		WarmupParticlesComp = Statics::SpawnFogParticlesAttached(GS, AttackPreparationParticles, this, 
			AttackAttributes.GetMuzzleSocket());
	}
	
	// Start sound
	if (AttackAttributes.GetPreparationSound() != nullptr)
	{
		if (Statics::IsValid(WarmupAudioComp))
		{
			// Re-use already spawned audio comp to avoid having to spawn another
			WarmupAudioComp->PlaySound(GS, WarmupAudioComp->Sound, 0.f, Team, ESoundFogRules::DynamicExceptForInstigatorsTeam);
		}
		else
		{
			WarmupAudioComp = Statics::SpawnSoundAttached(AttackAttributes.GetPreparationSound(), 
				this, ESoundFogRules::DynamicExceptForInstigatorsTeam, AttackAttributes.GetMuzzleSocket());
		}
	}

	TimeAttackWarmupStarted = GetWorld()->GetTimeSeconds();
	TimeSpentWarmingUpAttack = 0.f;
}

void UBuildingAttackComponent_SM::ServerStopAttackWarmUp()
{	
	TimeAttackWarmupStarted = -1.f;
	TimeSpentWarmingUpAttack = -1.f;
	
	/* Just null checks here, not IsValid also. But if I get problems here then may need to 
	switch to IsValid checks */

	// Stop attack warm up particles
	if (WarmupParticlesComp != nullptr)
	{
		// Hoping the particle comp gets GCed eventually if I call DeactivateSystem
		WarmupParticlesComp->DeactivateSystem();
	}

	// Stop attack warm up sound
	if (WarmupAudioComp != nullptr)
	{
		WarmupAudioComp->Stop();
	}
}

void UBuildingAttackComponent_SM::ClientStopAttackWarmUp()
{
	/* Just null checks here, not IsValid also. But if I get problems here then may need to 
	switch to IsValid checks */

	// Stop attack warm up particles
	if (WarmupParticlesComp != nullptr)
	{
		// Hoping the particle comp gets GCed eventually if I call DeactivateSystem
		WarmupParticlesComp->DeactivateSystem();
	}

	// Stop attack warm up sound
	if (WarmupAudioComp != nullptr)
	{
		WarmupAudioComp->Stop();
	}
}

void UBuildingAttackComponent_SM::OnRep_TimeAttackWarmupStarted()
{
	// <0 is code for warmup is stopped
	if (TimeAttackWarmupStarted < 0.f)
	{
		ClientStopAttackWarmUp();
	}
	else
	{
		const float HowLongAgoWarmupStarted = GS->GetServerWorldTimeSeconds() - TimeAttackWarmupStarted;

		// Particles
		if (AttackPreparationParticles != nullptr)
		{
			WarmupParticlesComp = Statics::SpawnFogParticlesAttached(GS, AttackPreparationParticles, this,
				AttackAttributes.GetMuzzleSocket(), HowLongAgoWarmupStarted);
		}

		// Sound
		if (AttackAttributes.GetPreparationSound() != nullptr)
		{
			if (Statics::IsValid(WarmupAudioComp))
			{
				// Re-use already spawned audio comp to avoid having to spawn another
				WarmupAudioComp->PlaySound(GS, WarmupAudioComp->Sound, HowLongAgoWarmupStarted, Team, ESoundFogRules::DynamicExceptForInstigatorsTeam);
			}
			else
			{
				WarmupAudioComp = Statics::SpawnSoundAttached(AttackAttributes.GetPreparationSound(),
					this, ESoundFogRules::DynamicExceptForInstigatorsTeam, AttackAttributes.GetMuzzleSocket(), 
					FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, 
					false, 1.f, 1.f, HowLongAgoWarmupStarted);
			}
		}
	}
}

#if WITH_EDITOR
void UBuildingAttackComponent_SM::PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	AttackAttributes.OnPostEdit();

	if (YawFacingRequirement >= 180.f)
	{
		/* Remove the rotation required part from the target aquiring method - this structure 
		can attack at all angles so it does not need it */
		if (TargetAquireMethod == ETargetAquireMethodPriorties::LeastRotationRequired)
		{
			TargetAquireMethod = ETargetAquireMethodPriorties::None;
		}
		else if (TargetAquireMethod == ETargetAquireMethodPriorties::HasAttack_LeastRotationRequired)
		{
			TargetAquireMethod = ETargetAquireMethodPriorties::HasAttack;
		}
	}

	/* Set this to the attack rate so there is a bit of a delay from being built to when first 
	attack can happen. */
	TimeRemainingTillAttackOffCooldown = AttackAttributes.GetAttackRate();

	bReplicates = (AttackWarmupTime > 0.f);
}
#endif
