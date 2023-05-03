// Fill out your copyright notice in the Description page of Project Settings.


#include "BuildingAttackComponent_SK.h"
#include "Kismet/KismetMathLibrary.h"
#include "Particles/ParticleSystemComponent.h"
#include "Animation/AnimInstance.h"

#include "Statics/Statics.h"
#include "Statics/DevelopmentStatics.h"
#include "Managers/ObjectPoolingManager.h"


UBuildingAttackComponent_SK::UBuildingAttackComponent_SK()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	SetGenerateOverlapEvents(false);
	SetCanEverAffectNavigation(false);
	CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	bReceivesDecals = false;
	SetCollisionProfileName(FName("BuildingMesh"));

	TargetingTraceDelegate.BindUObject(this, &UBuildingAttackComponent_SK::OnSweepForTargetsComplete);

	RangeLenienceMultiplier = 1.1f;
	TimeAttackWarmupOrAnimStarted = -1.f;
	TimeSpentWarmingUpAttack = -1.f;
	TargetAquireMethod = ETargetAquireMethodPriorties::None;
	TargetAquirePreferredDistance = EDistanceCheckMethod::Furtherest;
	bHasTunnelVision = true;
	YawFacingRequirement = 180.f;
	AttackCompUniqueID = UINT8_MAX;
}

void UBuildingAttackComponent_SK::BeginPlay()
{
	CheckAnimBlueprint();
	CheckAnimNotifies();
	
	Super::BeginPlay();
	
	if (GetWorld()->IsServer())
	{
		GetAnimInstance()->Montage_Play(IdleAnim);
	}
	else
	{
		if (HasIdleAnimation() == false && HasAttackAnimationOrAttackWarmupAnimation() == true)
		{
			GetAnimInstance()->OnMontageEnded.AddDynamic(this, &UBuildingAttackComponent_SK::ClientOnMontageEnded);
		}
		
		if (IdleAnim == nullptr)
		{
			/* Clients do not need tick always on. They will need to turn it on to play animation though */
			SetComponentAnimationTickingEnabled(false);
		}
		else
		{
			GetAnimInstance()->Montage_Play(IdleAnim);
		}
	}
}

bool UBuildingAttackComponent_SK::CheckAnimBlueprint()
{
#if WITH_EDITOR
	if (GetAnimInstance() != nullptr)
	{
		if (GetAnimInstance()->IsA(UBuildingTurretCompAnimInstance::StaticClass()) == false)
		{
			UE_LOG(RTSLOG, Fatal, TEXT("[%s]: must reparent anim blueprint [%s] to derive from UBuildingTurretCompAnimInstance"),
				*GetClass()->GetName(), *GetAnimInstance()->GetClass()->GetName());
			return false;
		}
	}
#endif

	return true;
}

bool UBuildingAttackComponent_SK::CheckAnimNotifies()
{
#if WITH_EDITOR
	if (AttackAnim != nullptr)
	{
		FString AttackFuncName = GET_FUNCTION_NAME_STRING_CHECKED(UBuildingTurretCompAnimInstance, AnimNotify_FireWeapon);
		// Remove the "AnimNotify_" part at the start
		AttackFuncName = AttackFuncName.RightChop(11);
		static const FName AttackFuncFName = *AttackFuncName;

		for (const auto & AnimNotify : AttackAnim->Notifies)
		{
			if (AnimNotify.NotifyName == AttackFuncFName)
			{
				return true;
			}
		}

		/* Did not find anim notify with the name we were searching for */
		UE_LOG(RTSLOG, Fatal, TEXT("[%s]: no anim notify with name [%s] found on AttackAnim"),
			*GetClass()->GetName(), *AttackFuncName);

		return false;
	}
#endif

	return true;
}

void UBuildingAttackComponent_SK::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction * ThisTickFunction)
{
	if (GetWorld()->IsServer())
	{
		// Try attack target.
		if (HasAttackWarmup())
		{
			if (HasAttackWarmupAnimation())
			{
				if (IsPlayingAttackOrWarmupAnimation() == false)
				{
					TimeRemainingTillAttackOffCooldown -= DeltaTime;
					TimeRemainingTillAttackOffCooldown = FMath::Max(0.f, TimeRemainingTillAttackOffCooldown);
					if (TimeRemainingTillAttackOffCooldown <= 0.f)
					{
						/* The anim notifies will handle doing the attack */
						ServerStartAttackWarmupAnimation();
					}
				}
			}
			else
			{
				// Check if attack is ready
				if (HasAttackFullyWarmedUp())
				{
					if (HasAttackAnimation())
					{
						if (IsPlayingAttackAnimation() == false)
						{
							if (GetCurrentTargetTargetingStatus() == ETargetTargetingCheckResult::CanAttack)
							{
								ServerStartAttackAnimation();
							}
							else
							{
								SetTarget(nullptr);
							}
						}
					}
					else
					{
						if (GetCurrentTargetTargetingStatus() == ETargetTargetingCheckResult::CanAttack)
						{
							ServerDoAttack();
						}
						else
						{
							SetTarget(nullptr);
						}
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
		}
		else
		{
			if (HasAttackAnimation())
			{
				if (IsPlayingAttackAnimation() == false)
				{
					TimeRemainingTillAttackOffCooldown -= DeltaTime;
					if (TimeRemainingTillAttackOffCooldown <= 0.f)
					{
						const ETargetTargetingCheckResult TargetingStatus = GetCurrentTargetTargetingStatus();
						if (TargetingStatus == ETargetTargetingCheckResult::CanAttack)
						{
							TimeAttackWarmupOrAnimStarted = GetWorld()->GetTimeSeconds();
							ServerStartAttackAnimation();
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
	}
	else
	{
		/* Currently tick is only enabled for clients when either: 
		- we're warming up attack and will play an attack anim soon 
		- are playing an anim 
		This branch should only be hit if we're warming up attack and will play an attack anim soon. 
		If I ever just enable tick 24/7 for clients then this increment should probably not 
		always happen */

		/* If warming up attack (and warmup doesn't use warmup anim)... */
		if (TimeSpentWarmingUpAttack >= 0.f)
		{
			TimeSpentWarmingUpAttack += DeltaTime;
			if (TimeSpentWarmingUpAttack >= AttackWarmupTime)
			{
				ClientPlayAttackAnimation(TimeSpentWarmingUpAttack - AttackWarmupTime);
				TimeSpentWarmingUpAttack = -1.f;
			}
		}
	}

	/* Super will tick animations */
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UBuildingAttackComponent_SK::ServerDoAttack()
{
	/* Kind of not really ment to be called if got anims. Call ServerAnimNotify_DoAttack instead */
	assert(HasAttackWarmupAnimation() == false && HasAttackAnimation() == false);

	const FVector MuzzleLocation = GetSocketLocation(AttackAttributes.GetMuzzleSocket());
	const FRotator MuzzleRotation = GetSocketRotation(AttackAttributes.GetMuzzleSocket());

	// Fire projectile or projectiles - clive pugh
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
		TimeAttackWarmupOrAnimStarted = -1.f;
		TimeSpentWarmingUpAttack = -1.f;

		// Don't stop the warm up particles or sound. It is assumed their lengths are the 
		// same length as the warm up duration so they'll stop on their own
	}
}

void UBuildingAttackComponent_SK::ClientDoAttack(AActor * AttackTarget)
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

void UBuildingAttackComponent_SK::ServerAnimNotify_DoAttack()
{
	const FVector MuzzleLocation = GetSocketLocation(AttackAttributes.GetMuzzleSocket());
	const FRotator MuzzleRotation = GetSocketRotation(AttackAttributes.GetMuzzleSocket());

	TimeRemainingTillAttackOffCooldown += AttackAttributes.GetAttackRate();

	PoolingManager->Server_FireProjectileAtTarget(OwningBuilding, AttackAttributes.GetProjectileBP(),
		AttackAttributes.GetDamageInfo(), AttackAttributes.GetAttackRange(), Team,
		MuzzleLocation, Target.Get(), 0.f);

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
		TimeAttackWarmupOrAnimStarted = -1.f;
		TimeSpentWarmingUpAttack = -1.f;

		// Don't stop the warm up particles or sound. It is assumed their lengths are the 
		// same length as the warm up duration so they'll stop on their own
	}
}

void UBuildingAttackComponent_SK::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	DOREPLIFETIME(UBuildingAttackComponent_SK, TimeAttackWarmupOrAnimStarted);
}

UMeshComponent * UBuildingAttackComponent_SK::GetAsMeshComponent()
{
	return this;
}

void UBuildingAttackComponent_SK::SetupAttackComp(ABuilding * InOwningBuilding, ARTSGameState * GameState, AObjectPoolingManager * InPoolingManager, uint8 InUniqueID, IBuildingAttackComp_TurretsBase * InTurretRotatingBase)
{
	OwningBuilding = InOwningBuilding;
	GS = GameState;
	PoolingManager = InPoolingManager;
	AttackCompUniqueID = InUniqueID;
	SetRotatingBase(InTurretRotatingBase);
}

void UBuildingAttackComponent_SK::ServerSetupAttackCompMore(ETeam InTeam, const FVisibilityInfo * InTeamsVisibilityInfo, FCollisionObjectQueryParams InQueryParams)
{
	Team = InTeam;
	TeamVisibilityInfo = InTeamsVisibilityInfo;
	QueryParams = InQueryParams;
}

void UBuildingAttackComponent_SK::ClientSetupAttackCompMore(ETeam InTeam)
{
	Team = InTeam;
}

FOverlapDelegate * UBuildingAttackComponent_SK::GetTargetingTraceDelegate()
{
	return &TargetingTraceDelegate;
}

FCollisionObjectQueryParams UBuildingAttackComponent_SK::GetTargetingQueryParams() const
{
	return QueryParams;
}

FVector UBuildingAttackComponent_SK::GetTargetingSweepOrigin() const
{
	return GetComponentLocation();
}

float UBuildingAttackComponent_SK::GetTargetingSweepRadius() const
{
	return AttackAttributes.GetAttackRange();
}

void UBuildingAttackComponent_SK::SetTaskManagerBucketIndices(uint8 BucketIndex, int16 ArrayIndex)
{
	TaskManagerBucket = FTaskManagerBucketInfo(BucketIndex, ArrayIndex);
}

void UBuildingAttackComponent_SK::SetTaskManagerArrayIndex(int16 ArrayIndex)
{
	TaskManagerBucket.ArrayIndex = ArrayIndex;
}

void UBuildingAttackComponent_SK::GrabTaskManagerBucketIndices(uint8 & OutBucketIndex, int16 & OutArrayIndex) const
{
	OutBucketIndex = TaskManagerBucket.BucketIndex;
	OutArrayIndex = TaskManagerBucket.ArrayIndex;
}

void UBuildingAttackComponent_SK::OnParentBuildingExitFogOfWar(bool bOnServer)
{
	if (bOnServer == false)
	{
		/* Enable animation ticking if an animation should be playing */
		if (HasIdleAnimation() || IsPlayingAttackOrWarmupAnimation())
		{
			SetComponentAnimationTickingEnabled(true);
		}
	}
}

void UBuildingAttackComponent_SK::SetRotatingBase(IBuildingAttackComp_TurretsBase * InTurretRotatingBase)
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

bool UBuildingAttackComponent_SK::HasRotatingBase() const
{
	return RotatingBase != nullptr;
}

bool UBuildingAttackComponent_SK::HasIdleAnimation() const
{
	return IdleAnim != nullptr;
}

bool UBuildingAttackComponent_SK::HasAttackAnimationOrAttackWarmupAnimation() const
{
	return AttackAnim != nullptr;
}

bool UBuildingAttackComponent_SK::HasAttackWarmupAnimation() const
{
	return AttackAnim != nullptr && bUseAttackAnimForWarmupToo;
}

bool UBuildingAttackComponent_SK::HasAttackAnimation() const
{
	return AttackAnim != nullptr && !bUseAttackAnimForWarmupToo;
}

bool UBuildingAttackComponent_SK::IsPlayingAttackAnimation() const
{
	assert(HasAttackAnimation());
	return GetAnimInstance()->Montage_IsPlaying(AttackAnim);
}

bool UBuildingAttackComponent_SK::IsPlayingAttackOrWarmupAnimation() const
{
	return GetAnimInstance()->Montage_IsPlaying(AttackAnim);
}

bool UBuildingAttackComponent_SK::IsAttackOffCooldown() const
{
	return TimeRemainingTillAttackOffCooldown <= 0.f;
}

bool UBuildingAttackComponent_SK::HasAttackWarmup() const
{
	return AttackWarmupTime > 0.f;
}

bool UBuildingAttackComponent_SK::HasAttackFullyWarmedUp() const
{
	return TimeSpentWarmingUpAttack >= AttackWarmupTime;
}

bool UBuildingAttackComponent_SK::IsAttackWarmingUp() const
{
	return TimeSpentWarmingUpAttack >= 0.f && HasAttackFullyWarmedUp() == false;
}

void UBuildingAttackComponent_SK::OnSweepForTargetsComplete(const FTraceHandle & TraceHandle, FOverlapDatum & TraceData)
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

bool UBuildingAttackComponent_SK::CanSweptActorBeAquiredAsTarget(AActor * Actor) const
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

bool UBuildingAttackComponent_SK::CanSweptActorBeAquiredAsTarget(AActor * Actor, float & OutRotationRequiredToFace) const
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

bool UBuildingAttackComponent_SK::IsCurrentTargetStillAquirable() const
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

UBuildingAttackComponent_SK::ETargetTargetingCheckResult UBuildingAttackComponent_SK::GetCurrentTargetTargetingStatus() const
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

bool UBuildingAttackComponent_SK::IsActorInRange_NoLenience(AActor * Actor) const
{
	// Probably want to take bounds into account here, and maybe throw in some leneince too 
	const float DistanceSqr = Statics::GetDistance2DSquared(GetComponentLocation(), Actor->GetActorLocation());
	return DistanceSqr <= FMath::Square(AttackAttributes.GetAttackRange());
}

bool UBuildingAttackComponent_SK::IsActorInRange_UseLeneince(AActor * Actor) const
{
	const float DistanceSqr = Statics::GetDistance2DSquared(GetComponentLocation(), Actor->GetActorLocation());
	return DistanceSqr <= FMath::Square(AttackAttributes.GetAttackRange() * RangeLenienceMultiplier);
}

float UBuildingAttackComponent_SK::GetYawToActor(AActor * Actor) const
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

float UBuildingAttackComponent_SK::GetYawToActor(AActor * Actor, FRotator & OutLookAtRot) const
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

float UBuildingAttackComponent_SK::GetEffectiveComponentYawRotation() const
{
	return RotatingBase->GetEffectiveComponentYawRotation();
}

void UBuildingAttackComponent_SK::SetTarget(AActor * InTarget)
{
	SERVER_CHECK;

	Target = InTarget;

	if (HasAttackWarmupAnimation())
	{
		if (Target == nullptr)
		{
			if (IsPlayingAttackOrWarmupAnimation())
			{
				ServerStopAttackOrWarmupAnimation();
			}
		}
		else
		{
			if (IsAttackOffCooldown() == true && IsPlayingAttackOrWarmupAnimation() == false)
			{
				ServerStartAttackWarmupAnimation();
			}
		}
	}
	else if (HasAttackAnimation())
	{
		if (HasAttackWarmup())
		{
			if (Target == nullptr)
			{
				if (IsPlayingAttackAnimation())
				{
					ServerStopAttackAnimation();
				}
				else if (IsAttackWarmingUp())
				{
					ServerStopAttackWarmUp();
				}
			}
			else
			{
				if (IsAttackOffCooldown() == true && IsAttackWarmingUp() == false && IsPlayingAttackAnimation() == false)
				{
					StartAttackWarmUp();
				}
			}
		}
		else
		{
			if (Target == nullptr)
			{
				if (IsPlayingAttackAnimation())
				{
					ServerStopAttackAnimation();
				}
			}
		}
	}
	else
	{
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
}

AActor * UBuildingAttackComponent_SK::GetTarget() const
{
	return Target.Get();
}

void UBuildingAttackComponent_SK::StartAttackWarmUp()
{
	assert(HasAttackWarmupAnimation() == false);

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

	TimeAttackWarmupOrAnimStarted = GetWorld()->GetTimeSeconds();
	TimeSpentWarmingUpAttack = 0.f;
}

void UBuildingAttackComponent_SK::PlayAttackWarmupParticlesAndSound(float StartTime)
{
	// Particles
	if (AttackPreparationParticles != nullptr)
	{
		WarmupParticlesComp = Statics::SpawnFogParticlesAttached(GS, AttackPreparationParticles, this,
			AttackAttributes.GetMuzzleSocket(), StartTime);
	}

	// Sound
	if (AttackAttributes.GetPreparationSound() != nullptr)
	{
		if (Statics::IsValid(WarmupAudioComp))
		{
			// Re-use already spawned audio comp to avoid having to spawn another
			WarmupAudioComp->PlaySound(GS, WarmupAudioComp->Sound, StartTime, Team, ESoundFogRules::DynamicExceptForInstigatorsTeam);
		}
		else
		{
			WarmupAudioComp = Statics::SpawnSoundAttached(AttackAttributes.GetPreparationSound(),
				this, ESoundFogRules::DynamicExceptForInstigatorsTeam, AttackAttributes.GetMuzzleSocket(),
				FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset,
				false, 1.f, 1.f, StartTime);
		}
	}
}

void UBuildingAttackComponent_SK::ServerStopAttackWarmUp()
{
	assert(HasAttackWarmupAnimation() == false);

	TimeAttackWarmupOrAnimStarted = -1.f;
	TimeSpentWarmingUpAttack = -1.f;

	/* Just null checks here, not IsValid also. But if I get problems here then may need to
	switch to IsValid checks */

	// Stop attack warm up particles
	if (WarmupParticlesComp != nullptr)
	{
		StopWarmupParticles();
	}

	// Stop attack warm up sound
	if (WarmupAudioComp != nullptr)
	{
		WarmupAudioComp->Stop();
	}
}

void UBuildingAttackComponent_SK::ClientStopAttackWarmUp()
{
	/* Just null checks here, not IsValid also. But if I get problems here then may need to
	switch to IsValid checks */

	// Stop attack warm up particles
	if (WarmupParticlesComp != nullptr)
	{
		StopWarmupParticles();
	}

	// Stop attack warm up sound
	if (WarmupAudioComp != nullptr)
	{
		WarmupAudioComp->Stop();
	}
}

void UBuildingAttackComponent_SK::StopWarmupParticles()
{
	// Hoping the particle comp gets GCed eventually if I call DeactivateSystem
	// Although I should change this to KillParticlesForced() or something that flat out 
	// ends the particles instead of just ending the emitting. The reason for this is because 
	// when OnRep_TimeAttackWarmupOrAnimStarted comes in the client may have completely missed the partcles 
	//WarmupParticlesComp->DeactivateSystem();
	WarmupParticlesComp->KillParticlesForced();
}

void UBuildingAttackComponent_SK::ServerStartAttackWarmupAnimation()
{
	TimeAttackWarmupOrAnimStarted = GetWorld()->GetTimeSeconds();

	assert(AttackAnim != nullptr);
	GetAnimInstance()->Montage_Play(AttackAnim);
}

void UBuildingAttackComponent_SK::ServerStartAttackAnimation()
{
	/* Doing this when the attack anim notify goes off instead */
	//TimeRemainingTillAttackOffCooldown += AttackAttributes.GetAttackRate();

	assert(AttackAnim != nullptr);
	GetAnimInstance()->Montage_Play(AttackAnim);
}

void UBuildingAttackComponent_SK::ServerStopAttackOrWarmupAnimation()
{
	TimeAttackWarmupOrAnimStarted = -1.f;
	TimeSpentWarmingUpAttack = -1.f;

	assert(AttackAnim != nullptr);
	GetAnimInstance()->Montage_Stop(0.f, AttackAnim);
}

void UBuildingAttackComponent_SK::ServerStopAttackAnimation()
{
	TimeAttackWarmupOrAnimStarted = -1.f;
	TimeSpentWarmingUpAttack = -1.f;

	assert(AttackAnim != nullptr);
	GetAnimInstance()->Montage_Stop(0.f, AttackAnim);
}

void UBuildingAttackComponent_SK::ClientPlayAttackWarmupAnimation(float AnimStartTime)
{
	SetComponentAnimationTickingEnabled(true);

	assert(AttackAnim != nullptr);
	GetAnimInstance()->Montage_Play(AttackAnim, 1.f, EMontagePlayReturnType::MontageLength, AnimStartTime);
}

void UBuildingAttackComponent_SK::ClientPlayAttackAnimation(float AnimStartTime)
{
	SetComponentAnimationTickingEnabled(true);

	assert(AttackAnim != nullptr);
	GetAnimInstance()->Montage_Play(AttackAnim, 1.f, EMontagePlayReturnType::MontageLength, AnimStartTime);
}

void UBuildingAttackComponent_SK::ClientStopAttackWarmupAnimation()
{
	assert(AttackAnim != nullptr);
	GetAnimInstance()->Montage_Stop(0.f, AttackAnim);

	SetComponentAnimationTickingEnabled(false);
}

void UBuildingAttackComponent_SK::SetComponentAnimationTickingEnabled(bool bEnabled)
{
	SetComponentTickEnabled(bEnabled);
}

void UBuildingAttackComponent_SK::ClientOnMontageEnded(UAnimMontage * Montage, bool bInterrupted)
{
	/* Ther only other anim is the idle anim and that should never stop */
	assert(Montage == AttackAnim);

	SetComponentAnimationTickingEnabled(false);
}

void UBuildingAttackComponent_SK::OnRep_TimeAttackWarmupOrAnimStarted()
{
	if (HasAttackWarmupAnimation())
	{
		// <0 is code for animation has stopped
		if (TimeAttackWarmupOrAnimStarted < 0.f)
		{
			ClientStopAttackWarmupAnimation();
		}
		else
		{
			const float HowLongAgoAnimStarted = GS->GetServerWorldTimeSeconds() - TimeAttackWarmupOrAnimStarted;
			ClientPlayAttackWarmupAnimation(HowLongAgoAnimStarted);
		}
	}
	else if (HasAttackAnimation())
	{
		if (HasAttackWarmup())
		{
			// <0 is code for warmup and anim are both stopped
			if (TimeAttackWarmupOrAnimStarted < 0.f)
			{
				ClientStopAttackWarmUp();
				ClientStopAttackWarmupAnimation();
			}
			else
			{
				const float HowLongAgoWarmupOrAnimStarted = GS->GetServerWorldTimeSeconds() - TimeAttackWarmupOrAnimStarted;
				/* Check what phase the whole attack prodecure is in: either the warmup phase 
				or playing the attack anim phase */
				if (HowLongAgoWarmupOrAnimStarted <= AttackWarmupTime)
				{
					PlayAttackWarmupParticlesAndSound(HowLongAgoWarmupOrAnimStarted);

					TimeSpentWarmingUpAttack = HowLongAgoWarmupOrAnimStarted;

					/* Enable tick to monitor when to start playing attack anim */
					SetComponentAnimationTickingEnabled(true);
				}
				else
				{
					/* Missed the attack warmup. Not going to bother playing the warmup particles 
					or sound */
					const float TimeIntoAnim = HowLongAgoWarmupOrAnimStarted - AttackWarmupTime;
					ClientPlayAttackAnimation(TimeIntoAnim);
				}
			}
		}
		else
		{
			const float HowLongAgoAnimStarted = GS->GetServerWorldTimeSeconds() - TimeAttackWarmupOrAnimStarted;
			/* This func should make sure to not actually play the anim at all if we've missed it 
			e.g. start time = 3 but it's only a 2 sec duration animation */
			ClientPlayAttackAnimation(HowLongAgoAnimStarted);
		}
	}
	else
	{
		// <0 is code for warmup is stopped
		if (TimeAttackWarmupOrAnimStarted < 0.f)
		{
			ClientStopAttackWarmUp();
		}
		else
		{
			const float HowLongAgoWarmupStarted = GS->GetServerWorldTimeSeconds() - TimeAttackWarmupOrAnimStarted;
			PlayAttackWarmupParticlesAndSound(HowLongAgoWarmupStarted);
		}
	}
}

#if WITH_EDITOR
void UBuildingAttackComponent_SK::PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent)
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

	bReplicates = (AttackWarmupTime > 0.f) || HasAttackAnimationOrAttackWarmupAnimation();
}
#endif
