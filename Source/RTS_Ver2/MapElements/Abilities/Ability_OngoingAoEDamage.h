// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MapElements/Abilities/AbilityBase.h"
#include "Ability_OngoingAoEDamage.generated.h"

class URTSDamageType;
class UCurveFloat;


USTRUCT()
struct FOngoingAoEDamageInstance
{
	GENERATED_BODY()

public:

	FOngoingAoEDamageInstance() 
	{
	}

	FOngoingAoEDamageInstance(AActor * InInstigator, int32 InNumTicksRemaining, ETeam InInstigatorsTeam)
		: AbilityEpicenter(InInstigator->GetActorLocation())
		, AbilityInstigator(InInstigator) 
		, NumTicksRemaining(InNumTicksRemaining)
		, InstigatorsTeam(InInstigatorsTeam)
	{
	}

	FVector GetAbilityUseLocation() const { return AbilityEpicenter; }
	void DecrementNumTicksRemaining() { NumTicksRemaining--; }
	int32 GetNumTicksRemaining() const { return NumTicksRemaining; }
	FTimerHandle & GetTimerHandle() { return TimerHandle_DoDamageTick; }

protected:

	FVector AbilityEpicenter;

	TWeakObjectPtr<AActor> AbilityInstigator;

	FTimerHandle TimerHandle_DoDamageTick;

	int32 NumTicksRemaining;

	ETeam InstigatorsTeam;
};


/**
 *	Deals damage over time in an area of effect. 
 *	By default does not use use a world location so not ideal for an ability that targets a 
 *	world location.
 */
UCLASS(Abstract, Blueprintable)
class RTS_VER2_API AAbility_OngoingAoEDamage : public AAbilityBase
{
	GENERATED_BODY()
	
public:

	AAbility_OngoingAoEDamage();

protected:

	virtual void BeginPlay() override;

	void CheckCurves();

	//~ Begin AAbilityBase interface
	virtual void Server_Begin(FUNCTION_SIGNATURE_ServerBegin) override;
	virtual void Client_Begin(FUNCTION_SIGNATURE_ClientBegin) override;
	//~ End AAbilityBase interface

	void SpawnEpicenterParticles(AActor * AbilityInstigator, ISelectable * InstigatorAsSelectable);
	void PlayEpicenterSound(AActor * AbilityInstigator, ETeam InstigatorsTeam);

	UFUNCTION()
	void DoDamageTick(AActor * AbilityInstigator, ETeam InstigatorsTeam, uint32 UniqueID);

	void DoInitialDamageTickAfterDelay(AActor * AbilityInstigator, ETeam InstigatorsTeam, uint32 UniqueID);

	/* Call DoDamageTick after a delay */
	void DoDamageTickAfterDelay(AActor * AbilityInstigator, ETeam InstigatorsTeam, uint32 UniqueID, float DelayAmount);

	/** 
	 *	Calculate how much damage to deal to an actor hit by the ability 
	 *	
	 *	@param Location - where the epicenter of the ability is 
	 *	@param HitActor - actor that was hit by ability 
	 *	@return - final damage amount to pass into HitActor->TakeDamage 
	 */
	float CalculateDamage(const FVector & Location, AActor * HitActor) const;

	//----------------------------------------------------------------
	//	Data
	//----------------------------------------------------------------

	// Maps unique ID to info about the state of the ability
	UPROPERTY()
	TMap<uint32, FOngoingAoEDamageInstance> ActiveEffectInstances;

	/** 
	 *	Whether the effect attaches to the instigator. If true the ability "follows" its user. 
	 *	If false it will stay in the same spot which is the place it was used at, and if the 
	 *	user moves it will not move with them 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Behavior")
	bool bAttachesToInstigator;

	/** 
	 *	The time between when this ability is used and the first damage tick happens. 
	 *	0 = no delay 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Duration", meta = (ClampMin = 0))
	float FirstTickDelay;

	/* How many damage ticks for this ability */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Duration", meta = (ClampMin = 1))
	int32 NumTicks;

	/* Time between damage ticks */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Duration", meta = (ClampMin = "0.05"))
	float TimeBetweenTicks;

	/* Radius */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Damage", meta = (ClampMin = "0.001"))
	float Radius;

	/* How much base damage is dealt each tick */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Damage")
	float BaseDamagePerTick;

	UPROPERTY(EditDefaultsOnly, NoClear, Category = "RTS|Damage")
	TSubclassOf<URTSDamageType> DamageType;

	/**
	 *	Setting this value > 0 adds some randomness to damage
	 *	OutgoingDamage = Damage * FMath::RandRange(1.f - RandomDamageFactor, 1.f + RandomDamageFactor)
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Damage", meta = (ClampMin = "0", ClampMax = "0.999"))
	float RandomDamageFactor;

	/**
	 *	The curve to use for damage falloff. If no curve is specified then there will be no damage
	 *	falloff - every selectable hit will take full damage.
	 *
	 *	X axis = range from center. Larger implies further from ability center. Axis range: [0, 1]
	 *	Y axis = normalized percentage of BaseDamage to deal. Probably want range [0, 1]
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Damage")
	UCurveFloat * DamageFalloffCurve;

	/** Whether it can hit enemies */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Targeting")
	bool bCanHitEnemies;

	/**
	 *	Whether it can hit friendlies. Note if this and bCanHitEnemies are both false then this
	 *	ability cannot hit anything!
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Targeting")
	bool bCanHitFriendlies;

	/* Whether this ability can damage the instigator */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Targeting", meta = (EditCondition = bCanHitFriendlies))
	bool bCanHitSelf;

	/* Whether it can hit flying units */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Targeting")
	bool bCanHitFlying;

	/* Optional particle system to show at target location */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Particle Systems")
	UParticleSystem * TargetLocationParticles;

	/* The part of the instigator that the effect's particles attaches/spawns at. 
	EditCondition = (TargetLocationParticles != nullptr) */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Particle Systems")
	ESelectableBodySocket BodySpawnLocation;

	/* Optional sound to play at the target location */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Audio")
	USoundBase * TargetLocationSound;
};
