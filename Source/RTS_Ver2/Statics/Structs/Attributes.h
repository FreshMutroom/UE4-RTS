// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Attributes.generated.h"

class ISelectable;
class URTSHUD;


/** 
 *==============================================================================================
 *	A single floating point attribute. 
 *
 *	Can be modified and will always return to its exact float value eventually i.e. it is 
 *	not suspectable to floating point errors in the long run.
 *
 *	Can be modified by either adding a flat value e.g. add 20 to attack range, 
 *	or using a multiplier e.g. increase attack range by 10%. Multipliers can be applied in 
 *	either additive or multiplicative ways 
 *	e.g. attack range is 500
 *	We get a buff that increases it by 10%. We now have 550 attack range
 *	We get another buff that increases attack range by 50%. We can choose to either apply it 
 *	additively so we now have a 60% boost and have 800 attack range 
 *	or it can be done multiplicatively and the boost is now 66% and attack range is 830.
 *
 *	Addition bonuses are always applied first then the multiplier is applied. 
 *	Allowing it the other way around I think will require adding more variables.
 *==============================================================================================
 */
USTRUCT()
struct FFloatAttributeBase
{
	GENERATED_BODY()

protected:

	/* The value of Value that is considered the base value */
	UPROPERTY()
	float DefaultValue;

	/* Don't think this is needed if we disallow additive multiplier adjustments. */
	UPROPERTY()
	float DefaultMultiplier;

	/* If (NumTempValueModifiers == 0) then this will equal DefaultValue */
	UPROPERTY()
	float Value;

	/* If (NumTempMultiplierModifiers == 0) then this will equal DefaultMultiplier */
	UPROPERTY()
	float Multiplier;

	/* How many "temporary" modifiers are applied to the attribute */
	int16 NumTempValueModifiers;
	int16 NumTempMultiplierModifiers;

public:

	FFloatAttributeBase()
		: DefaultValue(-1.f) 
		, DefaultMultiplier(1.f)
		, Value(DefaultValue)
		, Multiplier(DefaultMultiplier)
		, NumTempValueModifiers(0) 
		, NumTempMultiplierModifiers(0)
	{
	}

	explicit FFloatAttributeBase(float InValue) 
		: DefaultValue(InValue)
		, DefaultMultiplier(1.f)
		, Value(DefaultValue)
		, Multiplier(DefaultMultiplier)
		, NumTempValueModifiers(0)
		, NumTempMultiplierModifiers(0)
	{
	}

	/* Return the final value after taking into account multiplier */
	float GetFinalValue() const
	{
		return Value * Multiplier;
	}

#if WITH_EDITOR
	void SetValuesOnPostEdit(float InValue)
	{
		DefaultValue = InValue;
		Value = InValue;
	}
#endif

protected:

	//------------------------------------------------------------------------------------------
	//=========================================================================================
	//	------- Modification Functions -------
	//=========================================================================================
	//------------------------------------------------------------------------------------------

	//=========================================================================================
	//	------- Permanent modification functions -------
	//-----------------------------------------------------------------------------------------
	//	These are for permanent changes such as upgrades and leveling up bonuses
	//=========================================================================================

	/** 
	 *	@param TAttribute - type of attribute
	 *	@param Owner - the selectable that this attribute belongs to. 
	 *	@return - final value after modification 
	 */
	template <typename TAttribute>
	void SetDefaultValue(float NewValue, bool bUpdateUI, ISelectable * Owner = nullptr)
	{
		const float PercentageDiff = NewValue / DefaultValue;
		DefaultValue = NewValue;
		Value *= PercentageDiff;

		CheckIfSightRangeOrStealthRevealRangeChanged<TAttribute>(Owner);

		if (bUpdateUI)
		{
			UpdateUI<TAttribute>(Owner);
		}
	}
	
	template <typename TAttribute>
	void AdjustDefaultValue(float AdjustmentAmount, bool bUpdateUI, ISelectable * Owner = nullptr)
	{
		DefaultValue += AdjustmentAmount;
		Value += AdjustmentAmount;

		CheckIfSightRangeOrStealthRevealRangeChanged<TAttribute>(Owner);

		if (bUpdateUI)
		{
			UpdateUI<TAttribute>(Owner);
		}
	}
	
	template <typename TAttribute>
	void AdjustDefaultMultiplier_Addition(float MultiplierDelta, bool bUpdateUI, ISelectable * Owner = nullptr)
	{
		DefaultMultiplier += MultiplierDelta;
		Multiplier += MultiplierDelta;

		CheckIfSightRangeOrStealthRevealRangeChanged<TAttribute>(Owner);

		if (bUpdateUI)
		{
			UpdateUI<TAttribute>(Owner);
		}
	}

	template <typename TAttribute>
	void AdjustDefaultMultiplier_Multiplication(float MultiplierMultiplier, bool bUpdateUI, ISelectable * Owner = nullptr)
	{
		DefaultMultiplier *= MultiplierMultiplier;
		Multiplier *= MultiplierMultiplier;

		CheckIfSightRangeOrStealthRevealRangeChanged<TAttribute>(Owner);

		if (bUpdateUI)
		{
			UpdateUI<TAttribute>(Owner);
		}
	}

	//=========================================================================================
	//	------- Temporary modification functions -------
	//-----------------------------------------------------------------------------------------
	//	These are for temporary things like buff/debuffs and inventory item bonuses
	//=========================================================================================

	template <typename TAttribute>
	void ApplyTempAbsoluteValueChange_Addition(float ValueDelta, bool bUpdateUI, ISelectable * Owner = nullptr)
	{
		NumTempValueModifiers++;

		Value += ValueDelta;

		CheckIfSightRangeOrStealthRevealRangeChanged<TAttribute>(Owner);

		if (bUpdateUI)
		{
			UpdateUI<TAttribute>(Owner->GetLocalPC()->GetHUDWidget(), Owner);
		}

		return GetFinalValue();
	}
	
	template <typename TAttribute>
	void ApplyTempMultiplierChange_Addition(float MultiplierDelta, bool bUpdateUI, ISelectable * Owner = nullptr)
	{
		NumTempMultiplierModifiers++;

		Multiplier += MultiplierDelta;

		CheckIfSightRangeOrStealthRevealRangeChanged<TAttribute>(Owner);

		if (bUpdateUI)
		{
			UpdateUI<TAttribute>(Owner);
		}

		return GetFinalValue();
	}
	
	template <typename TAttribute>
	void ApplyTempMultiplierChange_Multiplication(float MultiplierMultiplier, bool bUpdateUI, ISelectable * Owner = nullptr)
	{
		/* 0 is irreversable and therefore not allowed */
		assert(MultiplierMultiplier != 0.f);

		NumTempMultiplierModifiers++;

		Multiplier *= MultiplierMultiplier;

		CheckIfSightRangeOrStealthRevealRangeChanged<TAttribute>(Owner);

		if (bUpdateUI)
		{
			UpdateUI<TAttribute>(Owner);
		}

		return GetFinalValue();
	}
	
	template <typename T>
	void RemoveTempAbsoluteValueChange_Addition(float ValueDelta, bool bUpdateUI, ISelectable * Owner = nullptr)
	{
		NumTempValueModifiers--;
		assert(NumTempValueModifiers >= 0);
		if (NumTempValueModifiers == 0)
		{
			Value = DefaultValue;
		}
		else
		{
			Value += ValueDelta;
		}

		CheckIfSightRangeOrStealthRevealRangeChanged<TAttribute>(Owner);

		if (bUpdateUI)
		{
			UpdateUI<TAttribute>(Owner);
		}

		return GetFinalValue();
	}
	
	template <typename TAttribute>
	void RemoveTempMultiplierChange_Addition(float MultiplierDelta, bool bUpdateUI, ISelectable * Owner = nullptr)
	{
		NumTempMultiplierModifiers--;
		assert(NumTempMultiplierModifiers >= 0);
		if (NumTempMultiplierModifiers == 0)
		{
			Multiplier = DefaultMultiplier;
		}
		else
		{
			Multiplier += MultiplierDelta;
		}

		CheckIfSightRangeOrStealthRevealRangeChanged<TAttribute>(Owner);

		if (bUpdateUI)
		{
			UpdateUI<TAttribute>(Owner);
		}

		return GetFinalValue();
	}
	
	template <typename TAttribute>
	void RemoveTempMultiplierChange_Multiplication(float MultiplierMultiplier, bool bUpdateUI, ISelectable * Owner = nullptr)
	{
		NumTempMultiplierModifiers--;
		assert(NumTempMultiplierModifiers >= 0);
		if (NumTempMultiplierModifiers == 0)
		{
			Multiplier = DefaultMultiplier;
		}
		else
		{
			Multiplier *= MultiplierMultiplier;
		}

		CheckIfSightRangeOrStealthRevealRangeChanged<TAttribute>(Owner);

		if (bUpdateUI)
		{
			UpdateUI<TAttribute>(Owner);
		}

		return GetFinalValue();
	}

private:

	/**
	 *	If either sight range or stealth reveal range change then we need to check if the 
	 *	selectable is inside a garrison. If yes then we need to let it know
	 */
	template <typename TAttribute>
	void CheckIfSightRangeOrStealthRevealRangeChanged(ISelectable * Owner)
	{
		/* Check if this struct is for sight range for a building */
		if constexpr(switch_value<TAttribute>::Value == 3)
		{
			/* Update effective sight radius if required. */
			if (BuildingsSightRadiusWentDown && EffectiveSightRadiusContributorWasBuilding)
			{
				a;
			}
			else if (BuildingsSightRadiusWentUp && EffectiveSightRadiusContributorWasNOTBuilding)
			{
				a;
			}
		}
		/* Check if this struct is for stealth reveal range for a building */
		else if constexpr(switch_value<TAttribute>::Value == 4)
		{
			/* Update effective sight radius if required. Same deal as above with sight radius 
			except this is for stealth reveal radius */
			a;
		}
		/* Check if for sight range for a unit */
		else if constexpr(switch_value<TAttribute>::Value == 5)
		{
			/* Haven't actually implemented any of these next 3 funcs I think */
			if (Owner->SelectableIsInsideGarrison())
			{
				Owner->GetTheGarrisonedSeletable()->OnGarrisonedUnitSightRangeChanged();
			}
		}
		/* Check if for stealth reveal range for a unit */
		else if constexpr(switch_value<TAttribute>::Value == 6)
		{
			/* Haven't actually implemented any of these next 3 funcs I think */
			if (Owner->SelectableIsInsideGarrison())
			{
				Owner->GetTheGarrisonedSeletable()->OnGarrisonedUnitStealthRevealRangeChanged();
			}
		}
	}

	//=========================================================================================
	//	Updating HUD code
	//=========================================================================================

	template<typename T>
	struct switch_value {};

	template<>
	struct switch_value<struct FAttribute_AttackSpeed>	{ enum { value = 1 }; };

	template<>
	struct switch_value<struct FAttribute_AttackRange> { enum { value = 2 }; };
	
	template<> 
	struct switch_value<struct FAttribute_Building_SightRange> { enum { value = 3 }; };
	
	template<>
	struct switch_value<struct FAttribute_Building_StealthRevealRange> { enum { value = 4 }; };
	
	template<>
	struct switch_value<struct FAttribute_Unit_SightRange> { enum { value = 5 }; };

	template<>
	struct switch_value<struct FAttribute_Unit_StealthRevealRange> { enum { value = 6 }; };

	template<>
	struct switch_value<struct FAttribute_Defense> { enum { value = 7 }; };
	
	template<>
	struct switch_value<struct FAttribute_ExperienceBounty> { enum { value = 8 }; };

	/** 
	 *	Updates the HUD to the current value. 
	 *	
	 *	@param T - the type of attribute 
	 *	@param HUDWidget - HUD widget for local player 
	 */
	template <typename T>
	void UpdateUI(ISelectable * AttributeOwner)
	{
		const FSelectableAttributesBase & Attributes = Owner->GetAttributesBase();
		if (Attributes.IsSelected())
		{
			URTSHUD * HUDWidget = AttributeOwner->GetLocalPC()->GetHUDWidget();
			
			if constexpr(switch_value<T>::Value == 1)
			{
				HUDWidget->Selected_OnAttackRateChanged(AttributeOwner, FinalValue, Attributes.IsPrimarySelected());
			}
			else if constexpr(switch_value<T>::Value == 2)
			{
				HUDWidget->Selected_OnAttackRangeChanged(AttributeOwner, FinalValue, Attributes.IsPrimarySelected());
			}
			else if constexpr(switch_value<T>::Value == 3)
			{
				a;
			}
			else if constexpr(switch_value<T>::Value == 4)
			{
				a;
			}
			else if constexpr(switch_value<T>::Value == 5)
			{
				HUDWidget->
			}
			else if constexpr(switch_value<T>::Value == 6)
			{
				HUDWidget->
			}
			else if constexpr(switch_value<T>::Value == 7)
			{
				HUDWidget->
			}
			else if constexpr(switch_value<T>::Value == 8)
			{
				HUDWidget->
			}
			else
			{
				static_assert(0, "Unhandled attribute type. Make sure to declare a switch_value template specialization for it");
			}
		}
	}
};


USTRUCT()
struct FAttribute_AttackSpeed : public FFloatAttributeBase
{
	GENERATED_BODY()

protected:

#if WITH_EDITORONLY_DATA
	/**
	 *	Amount of time between point when attack animation fires projectile to the time when the
	 *	attack animation can start again.
	 *
	 *	Lower = attack faster.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = "0.05"))
	float AttackSpeed;
#endif 

public:

	FAttribute_AttackSpeed()
		: Super(1.f) 
		, AttackSpeed(1.f)
	{
	}
};


USTRUCT()
struct FAttribute_AttackRange : public FFloatAttributeBase
{
	GENERATED_BODY()

protected:

#if WITH_EDITORONLY_DATA
	/* The variable exposed to editor that users can edit */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 0))
	float AttackRange;
#endif 

public:

	FAttribute_AttackRange()
		: Super(600.f)
		, AttackRange(600.f)
	{
	}
};


/* Sight range for a building */
USTRUCT()
struct FAttribute_Building_SightRange : public FFloatAttributeBase
{
	GENERATED_BODY()

protected:

#if WITH_EDITORONLY_DATA
	/* Fog of war reveal radius */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 0))
	float SightRange;
#endif

	UPROPERTY()
	float EffectiveSightRange;

	// Might want a contributor variable here

public:

	FAttribute_Building_SightRange()
		: Super(1200.f)
		, SightRange(1200.f) 
		, EffectiveSightRange(1200.f)
	{
	}
};


USTRUCT()
struct FAttribute_Building_StealthRevealRange : public FFloatAttributeBase
{
	GENERATED_BODY()

protected:

#if WITH_EDITORONLY_DATA
	/* Range that selectable can reveal other selectables in stealth mode */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 0))
	float StealthRevealRange;
#endif

	UPROPERTY()
	float EffectiveStealthRevealRange;

	// Might want a contributor variable here

public:

	FAttribute_Building_StealthRevealRange()
		: Super(0.f)
		, StealthRevealRange(0.f) 
		, EffectiveStealthRevealRange(0.f)
	{
	}
};


/* Sight range for a unit */
USTRUCT()
struct FAttribute_Unit_SightRange : public FFloatAttributeBase
{
	GENERATED_BODY()

protected:

#if WITH_EDITORONLY_DATA
	/* Fog of war reveal radius */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 0))
	float SightRange;
#endif

public:

	FAttribute_Unit_SightRange()
		: Super(1200.f)
		, SightRange(1200.f)
	{
	}
};


/* Stealth reveal range for a unit */
USTRUCT()
struct FAttribute_Unit_StealthRevealRange : public FFloatAttributeBase
{
	GENERATED_BODY()

protected:

#if WITH_EDITORONLY_DATA
	/* Range that selectable can reveal other selectables in stealth mode */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 0))
	float StealthRevealRange;
#endif

public:

	FAttribute_Unit_StealthRevealRange()
		: Super(0.f)
		, StealthRevealRange(0.f)
	{
	}
};


UENUM()
enum class EImcomingDamageReductionRule : uint8
{
	/**
	 *	e.g. 
	 *	Damage multiplier = 0.9, damage reduction amount = 10 
	 *	Incoming damage = 100. 
	 *	Damage = 100 * 0.9 - 10 = 80 
	 */
	MultiplyThenAdd, 
	
	/** 
	 *	e.g.
	 *	Damage multiplier = 0.9, damage reduction amount = 10
	 *	Incoming damage = 100.
	 *	Damage = 100 - 10 * 0.9 = 81 
	 */
	AddThenMultiply
};


/** 
 *	Value = how much to subtract from each instance of incoming damage 
 *	Multiplier = how much to multiply incoming damage
 *	
 *	Which happens first can be decided by enum value. 
 */
USTRUCT()
struct FAttribute_Defense : public FFloatAttributeBase
{
	GENERATED_BODY()

	static constexpr EImcomingDamageReductionRule ReductionRule = EImcomingDamageReductionRule::AddThenMultiply;

public:

	FAttribute_Defense()
		: Super(0.f)
	{
	}
};


USTRUCT()
struct FAttribute_ExperienceBounty : public FFloatAttributeBase
{
	GENERATED_BODY()

protected:

#if WITH_EDITORONLY_DATA
	/* How much base experience is awarded to an enemy when they destroy this */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 0))
	float ExperienceBounty;
#endif

public:

	FAttribute_ExperienceBounty()
		: Super(40.f)
		, ExperienceBounty(40.f)
	{
	}
};


// Havent injected these into code at all. Should start removing all the attributes
// and start adding these
