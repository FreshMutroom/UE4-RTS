// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
//#include "InventoryItems.generated.h" // Cannot get project to compile with this.
										// Hmmm added this file using VS class wizard NOT through 
										// UE4 editor, wonder if that has something to do with it
class ISelectable;
enum class EItemAquireReason : uint8;
enum class EItemRemovalReason : uint8;
enum class EItemZeroChargesReachedReason : uint8;


/**
*	This class is only here as a place to put functions that define the behavior for inventory
*	items.
*/
class RTS_VER2_API InventoryItemBehavior
{
	//===========================================================================================
	//	Function pointer typedefs
	//===========================================================================================

#define FUNCTION_SIGNATURE_OnAquired ISelectable * ItemOwner, EItemAquireReason AquireReason
#define FUNCTION_SIGNATURE_OnRemoved ISelectable * ItemOwner, EItemRemovalReason RemovalReason
#define FUNCTION_SIGNATURE_OnZeroChargesReached ISelectable *, EItemZeroChargesReachedReason ZeroChargeReason

	/* OnAquired function */
	typedef void (*InventoryItem_OnAquired)(FUNCTION_SIGNATURE_OnAquired);

	/* OnRemoved function */
	typedef void (*InventoryItem_OnRemoved)(FUNCTION_SIGNATURE_OnRemoved);

	/* OnZeroChargesReached function - the function that can be called when the item reaches
	zero charges */
	typedef void (*InventoryItem_OnZeroChargesReached)(FUNCTION_SIGNATURE_OnZeroChargesReached);


//================================================================================================

public:

	//===========================================================================================
	//	Function declarations
	//===========================================================================================

	static void Shoes_OnAquired(FUNCTION_SIGNATURE_OnAquired);
	static void Shoes_OnRemoved(FUNCTION_SIGNATURE_OnRemoved);

	static void Bangle_OnAquired(FUNCTION_SIGNATURE_OnAquired);
	static void Bangle_OnRemoved(FUNCTION_SIGNATURE_OnRemoved);

	static void RedGem_OnAquired(FUNCTION_SIGNATURE_OnAquired);
	static void RedGem_OnRemoved(FUNCTION_SIGNATURE_OnRemoved);

	static void GreenGem_OnAquired(FUNCTION_SIGNATURE_OnAquired);
	static void GreenGem_OnRemoved(FUNCTION_SIGNATURE_OnRemoved);

	static void GoldenDagger_OnAquired(FUNCTION_SIGNATURE_OnAquired);
	static void GoldenDagger_OnRemoved(FUNCTION_SIGNATURE_OnRemoved);

	static void Necklace_OnAquired(FUNCTION_SIGNATURE_OnAquired);
	static void Necklace_OnRemoved(FUNCTION_SIGNATURE_OnRemoved);

	static void GoldenCrown_OnAquired(FUNCTION_SIGNATURE_OnAquired);
	static void GoldenCrown_OnRemoved(FUNCTION_SIGNATURE_OnRemoved);

	static void StrongSniperRifle_OnAquired(FUNCTION_SIGNATURE_OnAquired);
	static void StrongSniperRifle_OnRemoved(FUNCTION_SIGNATURE_OnRemoved);

	static void RottenPumpkin_OnAquired(FUNCTION_SIGNATURE_OnAquired);
	static void RottenPumpkin_OnRemoved(FUNCTION_SIGNATURE_OnRemoved);
};
