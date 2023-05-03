// Fill out your copyright notice in the Description page of Project Settings.

#include "Ability_Nuke.h"

#include "MapElements/Projectiles/LeaveThenComeBackProjectile.h"


AAbility_Nuke::AAbility_Nuke()
{
	//~ Begin AAbilityBase interface
	bHasMultipleOutcomes =			EUninitializableBool::False;
	bCallAoEStartFunction =			EUninitializableBool::False;
	bAoEHitsHaveMultipleOutcomes =	EUninitializableBool::False;
	bRequiresTargetOtherThanSelf =	EUninitializableBool::False;
	bRequiresLocation =				EUninitializableBool::True;
	bHasRandomBehavior =			EUninitializableBool::False;
	bRequiresTickCount =			EUninitializableBool::False;
	//~ End AAbilityBase interface
	
	LaunchBodyLocation = ESelectableBodySocket::None;
	DamageInfo.ImpactDamage = 0.f;
	DamageInfo.AoEDamage = 1000.f;
	DamageInfo.AoERandomDamageFactor = 0.15f;
	ShowDecalDelay = 1.f;
}

void AAbility_Nuke::BeginPlay()
{
	UE_CLOG(Projectile_BP == nullptr, RTSLOG, Fatal, TEXT("Nuke ability object [%s] does not have "
		"a projectile blueprint set. Need to set one."), *GetClass()->GetName());
	
	Super::BeginPlay();

	PoolingManager = GS->GetObjectPoolingManager();

	if (!bOverrideProjectileDamageValues)
	{
		/* This will spawn a projectile if pool is empty */
		AProjectileBase * Projectile = PoolingManager->GetProjectileReference(Projectile_BP);
		DamageInfo.SetValuesFromProjectile(Projectile);
	}
}

void AAbility_Nuke::Server_Begin(FUNCTION_SIGNATURE_ServerBegin)
{
	SuperServerBegin;

	Begin(EffectInstigator, InstigatorAsSelectable, Target, TargetAsSelectable, InstigatorsTeam, Location, true);
}

void AAbility_Nuke::Client_Begin(FUNCTION_SIGNATURE_ClientBegin)
{
	SuperClientBegin;

	Begin(EffectInstigator, InstigatorAsSelectable, Target, TargetAsSelectable, InstigatorsTeam, Location, false);
}

void AAbility_Nuke::Begin(AActor * EffectInstigator, ISelectable * InstigatorAsSelectable, AActor * Target,
	ISelectable * TargetAsSelectable, ETeam InstigatorsTeam, const FVector & Location, bool bIsServer)
{
	ARTSPlayerState * LocalPlayersPlayerState = CastChecked<ARTSPlayerState>(GetWorld()->GetFirstPlayerController()->PlayerState);
	ARTSPlayerState * InstigatorsPlayerState = CastChecked<ARTSPlayerState>(EffectInstigator->GetOwner());

	RevealFogAtTargetLocation(Location, InstigatorsTeam, bIsServer, LocalPlayersPlayerState);
	
	NotifyLocalPlayerOfLaunch(InstigatorsPlayerState, InstigatorsTeam, LocalPlayersPlayerState);

	// The actor where the missle launches from
	AActor * LaunchingActor = GetLaunchActor(EffectInstigator, Target);
	ISelectable * LaunchingSelectable = GetLaunchSelectable(InstigatorAsSelectable, TargetAsSelectable);

	const FAttachInfo * BodyLocInfo = LaunchingSelectable->GetBodyLocationInfo(LaunchBodyLocation);
	const FVector LaunchLocation = BodyLocInfo->GetComponent()->GetComponentLocation() + BodyLocInfo->GetAttachTransform().GetLocation();
	const FRotator LaunchRotation = BodyLocInfo->GetComponent()->GetComponentRotation() + BodyLocInfo->GetAttachTransform().GetRotation().Rotator();

	ShowLaunchLocationParticles(LaunchLocation, LaunchRotation);
	PlayLaunchLocationSound(LaunchLocation, InstigatorsTeam);

	LaunchProjectile(LaunchingActor, InstigatorsTeam, LaunchLocation, Location);

	if (ShowDecalDelay > 0.f)
	{
		SpawnDecalAtTargetLocationAfterDelay(Location, InstigatorsPlayerState, InstigatorsTeam, LocalPlayersPlayerState);
	}
	else
	{
		SpawnDecalAtTargetLocation(Location, InstigatorsPlayerState, InstigatorsTeam, LocalPlayersPlayerState);
	}
}

void AAbility_Nuke::NotifyLocalPlayerOfLaunch(ARTSPlayerState * InstigatorsOwner, ETeam InstigatorsTeam, 
	ARTSPlayerState * LocalPlayersPlayerState)
{
	EGameNotification NotificationType;
	if (InstigatorsOwner == LocalPlayersPlayerState)
	{
		NotificationType = EGameNotification::NukeLaunchedByOurselves;
	}
	else if (InstigatorsTeam == LocalPlayersPlayerState->GetTeam())
	{
		NotificationType = EGameNotification::NukeLaunchedByAllies;
	}
	else
	{
		NotificationType = EGameNotification::NukeLaunchedByHostiles;
	}

	LocalPlayersPlayerState->OnGameEventHappened(NotificationType);
}

AActor * AAbility_Nuke::GetLaunchActor(AActor * EffectInstigator, AActor * Target) const
{
	return bLaunchesFromAbilityTarget ? Target : EffectInstigator;
}

ISelectable * AAbility_Nuke::GetLaunchSelectable(ISelectable * InstigatorAsSelectable, ISelectable * TargetAsSelectable) const
{
	return bLaunchesFromAbilityTarget ? TargetAsSelectable : InstigatorAsSelectable;
}

void AAbility_Nuke::ShowLaunchLocationParticles(const FVector & LaunchLocation, const FRotator & LaunchRotation)
{
	if (LaunchLocationParticles != nullptr)
	{
		Statics::SpawnFogParticles(LaunchLocationParticles, GS, LaunchLocation, LaunchRotation, FVector::OneVector);
	}
}

void AAbility_Nuke::PlayLaunchLocationSound(const FVector & LaunchLocation, ETeam InstigatorsTeam)
{
	if (LaunchLocationSound != nullptr)
	{
		Statics::SpawnSoundAtLocation(GS, InstigatorsTeam, LaunchLocationSound, LaunchLocation, 
			ESoundFogRules::DynamicExceptForInstigatorsTeam);
	}
}

void AAbility_Nuke::LaunchProjectile(AActor * ProjectileLauncher, ETeam LaunchersTeam, 
	const FVector & LaunchLocation, const FVector & TargetLocation)
{
	/* Maybe could be non-valid if world is getting torn down or something? */
	if (PoolingManager.IsValid())
	{
		/* This will differ across server/clients. Does it really matter though? Can easily 
		solve this by choosing to send the random seed but I don't think it's that important */
		const float RandomRollRotation = FMath::RandRange(0.f, 360.f);

		/* Just need to fire projectile and we are done - the projectile class handles the rest */
		if (GetWorld()->IsServer())
		{
			PoolingManager->Server_FireProjectileAtLocation(ProjectileLauncher, Projectile_BP, 
				DamageInfo, 0.f, LaunchersTeam, LaunchLocation, TargetLocation, RandomRollRotation);
		}
		else
		{
			PoolingManager->Client_FireProjectileAtLocation(Projectile_BP, DamageInfo, 0.f, 
				LaunchersTeam, LaunchLocation, TargetLocation, RandomRollRotation);
		}
	}
}

void AAbility_Nuke::RevealFogAtTargetLocation(const FVector & TargetLocation, ETeam InstigatorsTeam, 
	bool bIsServer, ARTSPlayerState * LocalPlayersPlayerState)
{
#if GAME_THREAD_FOG_OF_WAR

	TargetLocationTemporaryFogReveal.CreateForTeam(GS->GetFogManager(), FVector2D(TargetLocation), 
		InstigatorsTeam, bIsServer, LocalPlayersPlayerState->GetTeam() == InstigatorsTeam);

#elif MULTITHREADED_FOG_OF_WAR

	TargetLocationTemporaryFogReveal.CreateForTeam(FVector2D(TargetLocation),
		InstigatorsTeam, bIsServer, LocalPlayersPlayerState->GetTeam() == InstigatorsTeam);

#endif
}

void AAbility_Nuke::SpawnDecalAtTargetLocation(const FVector & TargetLocation, ARTSPlayerState * InstigatorsOwner, 
	ETeam InstigatorsTeam, ARTSPlayerState * LocalPlayersPlayerState)
{
	// Spawn decal for just the instigator to see. Can easily be changed to include teammates too
	if (InstigatorsOwner == LocalPlayersPlayerState)
	{
		Statics::SpawnDecalAtLocation(this, TargetLocationDecalInfo, TargetLocation);
	}
}

void AAbility_Nuke::SpawnDecalAtTargetLocationAfterDelay(const FVector & TargetLocation, ARTSPlayerState * InstigatorsOwner,
	ETeam InstigatorsTeam, ARTSPlayerState * LocalPlayersPlayerState)
{
	assert(ShowDecalDelay > 0.f);
	
	FTimerDelegate TimerDel;
	FTimerHandle TimerHandle;

	TimerDel.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(AAbility_Nuke, SpawnDecalAtTargetLocation),
		TargetLocation, InstigatorsOwner, InstigatorsTeam, LocalPlayersPlayerState);
	GetWorldTimerManager().SetTimer(TimerHandle, TimerDel, ShowDecalDelay, false);
}

#if WITH_EDITOR
void AAbility_Nuke::PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	bRequiresTargetOtherThanSelf = bLaunchesFromAbilityTarget ? EUninitializableBool::True : EUninitializableBool::False;

	TargetLocationTemporaryFogReveal.OnPostEdit();
}
#endif
