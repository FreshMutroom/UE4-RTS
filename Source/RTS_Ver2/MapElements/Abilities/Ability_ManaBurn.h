// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MapElements/Abilities/AbilityBase.h"
#include "Ability_ManaBurn.generated.h"

class URTSDamageType;


UENUM()
enum class EDamageDealingRule : uint8
{
	AbsoluteAmount, 
	PercentageOfSelectableResourceAmountDrained
};


/**
 *	Single target. 
 *	Modifies target's selectable resource (mana) and optionally deals damage. Amount of 
 *	damage dealt can be based on the amount of mana burned.
 */
UCLASS(Blueprintable)
class RTS_VER2_API AAbility_ManaBurn : public AAbilityBase
{
	GENERATED_BODY()
	
public:

	AAbility_ManaBurn();

protected:

	//~ Begin AAbilityBase interface
	virtual bool IsUsable_TargetChecks(ISelectable * AbilityInstigator, ISelectable * Target, EAbilityRequirement & OutMissingRequirement) const override;
	virtual void Server_Begin(FUNCTION_SIGNATURE_ServerBegin) override;
	virtual void Client_Begin(FUNCTION_SIGNATURE_ClientBegin) override;
	//~ End AAbilityBase interface

	/** 
	 *	Get how much damage to pass into Target->TakeDamage. 
	 *	
	 *	@param ResourceAmountDrain - how much of the selectable resource was drained. Negative means 
	 *	it was gained 
	 */
	float GetOutgoingDamage(int32 ResourceAmountDrained) const;

	void ShowTargetParticles(ISelectable * TargetAsSelectable);
	void PlayTargetSound(AActor * Target, ETeam InstigatorsTeam);

	//---------------------------------------------------------------
	//	Data
	//---------------------------------------------------------------

	//===========================================================================================
	//	Resource Drain and Damage
	//===========================================================================================

	/** The selectable resource to burn */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Resource Drain")
	ESelectableResourceType SelectableResourceType;

	/** 
	 *	The type of message to show if the target does not have the selectable resource type 
	 *	we are trying to modify. 
	 *
	 *	My notes: maybe this could be defined somewhere where we map ESelectableResourceType 
	 *	to EAbilityRequirement. For now users have to do it manually 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Resource Drain", meta = (EditCondition = bCanEditMostProperties))
	EAbilityRequirement MissingResourceMessageType;

	/** Amount of selectable resource to burn. Negative values = target gains selectable resource */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Resource Drain", meta = (EditCondition = bCanEditMostProperties))
	int32 ResourceDrainAmount;

	/* How damage is calculated */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Damage")
	EDamageDealingRule DamageDealingRule;

	/* Percentage of selectable resource amount drained to deal in damage */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Damage", meta = (EditCondition = bCanEditPercentageDamageProperties))
	float DamagePercentage;

	/** If using absolute damage the amount to deal */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Damage", meta = (EditCondition = bCanEditAbsoluteDamageProperties))
	float AbsoluteDamageAmount;

	/* Type of damage to deal */
	UPROPERTY(EditDefaultsOnly, NoClear, Category = "RTS|Damage")
	TSubclassOf<URTSDamageType> DamageType;

	/**
	 *	Setting this value > 0 adds some randomness to damage
	 *	OutgoingDamage = Damage * FMath::RandRange(1.f - RandomDamageFactor, 1.f + RandomDamageFactor)
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Damage", meta = (ClampMin = "0", ClampMax = "0.999"))
	float RandomDamageFactor;

	//===========================================================================================
	//	Visuals
	//===========================================================================================

	/* Optional particle system template */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Particle Systems")
	UParticleSystem * TargetParticles_Template;

	/**
	 *	Where on the selectable the particles should try attach to.
	 *	EditCondition = (TargetParticles_Template != nullptr)
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Particle Systems")
	ESelectableBodySocket TargetParticles_AttachPoint;

	//============================================================================================
	//	Audio
	//============================================================================================

	/* Optional sound to play at target location */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Audio")
	USoundBase * TargetSound;


	//============================================================================================
	//	Editor Only Variables
	//============================================================================================

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	bool bCanEditMostProperties;

	UPROPERTY()
	bool bCanEditPercentageDamageProperties;

	UPROPERTY()
	bool bCanEditAbsoluteDamageProperties;
#endif

#if WITH_EDITOR
	virtual void PostEditChangeChainProperty(struct FPropertyChangedChainEvent & PropertyChangedEvent) override;
#endif
};
