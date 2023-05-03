// Fill out your copyright notice in the Description page of Project Settings.

#include "InventoryItems.h"

#include "GameFramework/Selectable.h"
#include "Statics/Structs_1.h"
#include "Statics/Structs_2.h"


void InventoryItemBehavior::Shoes_OnAquired(FUNCTION_SIGNATURE_OnAquired)
{
	// Apply a 20% move speed increase
	ItemOwner->ApplyTempMoveSpeedMultiplier(1.2f);
}

void InventoryItemBehavior::Shoes_OnRemoved(FUNCTION_SIGNATURE_OnRemoved)
{
	// Remove the 20% move speed increase
	ItemOwner->RemoveTempMoveSpeedMultiplier(1.f / 1.2f);
}

void InventoryItemBehavior::Bangle_OnAquired(FUNCTION_SIGNATURE_OnAquired)
{
	// Apply 25% damage increase
	if (ItemOwner->GetAttackAttributesModifiable() != nullptr)
	{
		ItemOwner->GetAttackAttributesModifiable()->ApplyTempImpactDamageModifierViaMultiplier(1.25f, ItemOwner);
	}
}

void InventoryItemBehavior::Bangle_OnRemoved(FUNCTION_SIGNATURE_OnRemoved)
{
	if (ItemOwner->GetAttackAttributesModifiable() != nullptr)
	{
		ItemOwner->GetAttackAttributesModifiable()->RemoveTempImpactDamageModifierViaMultiplier(1.f / 1.25f, ItemOwner);
	}
}

void InventoryItemBehavior::RedGem_OnAquired(FUNCTION_SIGNATURE_OnAquired)
{
	// 30% move speed increase
	ItemOwner->ApplyTempMoveSpeedMultiplier(1.3f);

	// Take 50% less damage
	ItemOwner->GetAttributesModifiable()->ApplyTempDefenseMultiplierViaMultiplier(0.5f, ItemOwner);
}

void InventoryItemBehavior::RedGem_OnRemoved(FUNCTION_SIGNATURE_OnRemoved)
{
	// Remove 30% move speed increase
	ItemOwner->RemoveTempMoveSpeedMultiplier(1.f / 1.3f);

	// Remove 50% incoming damage reduction
	ItemOwner->GetAttributesModifiable()->RemoveTempDefenseMultiplierViaMultiplier(1.f / 0.5f, ItemOwner);
}

void InventoryItemBehavior::GreenGem_OnAquired(FUNCTION_SIGNATURE_OnAquired)
{
	// Increase max health by 50%
	ItemOwner->GetAttributesModifiable()->ApplyTempMaxHealthModifierViaMultiplier(1.5f, ItemOwner,
		EAttributeAdjustmentRule::Percentage);
}

void InventoryItemBehavior::GreenGem_OnRemoved(FUNCTION_SIGNATURE_OnRemoved)
{
	// Remove the 50% max health increase
	ItemOwner->GetAttributesModifiable()->RemoveTempMaxHealthModifierViaMultiplier(1.f / 1.5f, ItemOwner,
		EAttributeAdjustmentRule::Percentage);
}

void InventoryItemBehavior::GoldenDagger_OnAquired(FUNCTION_SIGNATURE_OnAquired)
{
	// Apply 100% damage increase
	if (ItemOwner->GetAttackAttributesModifiable() != nullptr)
	{
		ItemOwner->GetAttackAttributesModifiable()->ApplyTempImpactDamageModifierViaMultiplier(2.f, ItemOwner);
	}
}

void InventoryItemBehavior::GoldenDagger_OnRemoved(FUNCTION_SIGNATURE_OnRemoved)
{
	if (ItemOwner->GetAttackAttributesModifiable() != nullptr)
	{
		ItemOwner->GetAttackAttributesModifiable()->RemoveTempImpactDamageModifierViaMultiplier(1.f / 2.f, ItemOwner);
	}
}

void InventoryItemBehavior::Necklace_OnAquired(FUNCTION_SIGNATURE_OnAquired)
{
}

void InventoryItemBehavior::Necklace_OnRemoved(FUNCTION_SIGNATURE_OnRemoved)
{
}

void InventoryItemBehavior::GoldenCrown_OnAquired(FUNCTION_SIGNATURE_OnAquired)
{
}

void InventoryItemBehavior::GoldenCrown_OnRemoved(FUNCTION_SIGNATURE_OnRemoved)
{
}

void InventoryItemBehavior::StrongSniperRifle_OnAquired(FUNCTION_SIGNATURE_OnAquired)
{
	FAttackAttributes * AttackAttributes = ItemOwner->GetAttackAttributesModifiable();
	
	AttackAttributes->ApplyTempImpactDamageModifierViaMultiplier(2.f, ItemOwner);
	AttackAttributes->ApplyTempAttackRateModifierViaMultiplier(1.f / 1.5f, ItemOwner);
}

void InventoryItemBehavior::StrongSniperRifle_OnRemoved(FUNCTION_SIGNATURE_OnRemoved)
{
	FAttackAttributes * AttackAttributes = ItemOwner->GetAttackAttributesModifiable();

	AttackAttributes->RemoveTempImpactDamageModifierViaMultiplier(1.f / 2.f, ItemOwner);
	AttackAttributes->RemoveTempAttackRateModifierViaMultiplier(1.5f / 1.f, ItemOwner);
}

void InventoryItemBehavior::RottenPumpkin_OnAquired(FUNCTION_SIGNATURE_OnAquired)
{
	// Reduce max health by 20%
	ItemOwner->GetAttributesModifiable()->ApplyTempMaxHealthModifierViaMultiplier(0.8f, ItemOwner,
		EAttributeAdjustmentRule::Percentage);
}

void InventoryItemBehavior::RottenPumpkin_OnRemoved(FUNCTION_SIGNATURE_OnRemoved)
{
	// Remove the 20% max health reduction
	ItemOwner->GetAttributesModifiable()->RemoveTempMaxHealthModifierViaMultiplier(1.f / 0.8f, ItemOwner,
		EAttributeAdjustmentRule::Percentage);
}
