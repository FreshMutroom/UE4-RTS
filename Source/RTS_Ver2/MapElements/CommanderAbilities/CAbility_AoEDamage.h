// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MapElements/CommanderAbilities/CommanderAbilityBase.h"

#include "Statics/Structs_5.h"
#include "CAbility_AoEDamage.generated.h"

class UParticleSystem;
struct FTimerHandle;
class UCurveFloat;


/**
 *	- Deals damage in an AoE
 *	- Shows particles
 */
UCLASS(Abstract, Blueprintable)
class RTS_VER2_API UCAbility_AoEDamage : public UCommanderAbilityBase
{
	GENERATED_BODY()

public:

	UCAbility_AoEDamage();

	//~ Begin UCommanderAbilityBase
	virtual void FinalSetup(URTSGameInstance * GameInst, ARTSGameState * GameState, ARTSPlayerState * LocalPlayersPlayerState) override;
	virtual void Server_Execute(FUNCTION_SIGNATURE_ServerExecute) override;
	virtual void Client_Execute(FUNCTION_SIGNATURE_ClientExecute) override;
	//~ End UCommanderAbilityBase

protected:

	void DealDamageAfterDelay(const FVector & Location, ETeam InstigatorsTeam);
	
	UFUNCTION()
	void DealDamage(const FVector & TargetLocation, ETeam InstigatorsTeam);
	
	/**
	 *	Return how much damage a selectable should take from being hit by this ability 
	 *	
	 *	@param TargetLocation - world location where the ability was used 
	 *	@return - final outgoing damage
	 */
	float CalculateDamage(const FVector & TargetLocation, AActor * HitActor) const;

	void ShowTargetLocationParticles(const FVector & TargetLocation);

	/* Check the curve assets are usable and if not then null them and log something */
	void CheckCurveAssets();

	void Delay(FTimerHandle & TimerHandle, void(UCAbility_AoEDamage:: *Function)(), float DelayAmount);

	//----------------------------------------------------------
	//	Data
	//----------------------------------------------------------

	/* AoE radius */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Damage", meta = (ClampMin = "0.0001"))
	float Radius;
	
	/* Delay from when ability is used to when damage should be dealt */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Damage", meta = (ClampMin = 0))
	float DealDamageDelay;

	/* Damage of AoE */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Damage")
	FBasicDamageInfo DamageInfo;

	/**
	 *	The curve to use for damage falloff. If no curve is specified then there will be no damage
	 *	falloff - every selectable hit will take full damage.
	 *
	 *	X axis = range from center. Larger implies further from ability center. Axis range: [0, 1]
	 *	Y axis = normalized percentage of BaseDamage to deal. Probably want range [0, 1]
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Damage")
	UCurveFloat * DamageFalloffCurve;

	UPROPERTY(EditDefaultsOnly, Category = "RTS|Damage")
	uint8 bCanHitEnemies : 1;

	UPROPERTY(EditDefaultsOnly, Category = "RTS|Damage")
	uint8 bCanHitFriendlies : 1;

	UPROPERTY(EditDefaultsOnly, Category = "RTS|Damage")
	uint8 bCanHitFlying : 1;

	/* Particle system to spawn at use location */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Particles")
	UParticleSystem * TargetLocationParticles;

	/** 
	 *	Scale of particles. Don't usually expose this cause sometimes particles can scale 
	 *	incorrectly. In this case you need to create another particle system asset. 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Particles")
	float TargetLocationParticlesScale;
};
