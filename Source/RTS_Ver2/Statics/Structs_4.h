// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Structs_4.generated.h"


//******************************************************************************
//	This file should contain wrappers only!
//
//******************************************************************************

//==============================================================================================
/**
 *	This file contains structs that are used as wrappers of basic primitive types. They are
 *	commonly sent over the wire so to reduce bandwidth they are kept as small as possible.
 *	Using a wrapper like this means users can easily change the amount of bytes sent if they 
 *	need to.
 *
 *	A cleaner alternative to doing this is to do a #define MY_TYPE uint8, but UHT runs before 
 *	all user made #define directives so that isn't possible. 
 *
 *	Each struct has a brief explanation explaining when it is good to increase/decrease the 
 *	wrapped integer used. In that explanation when I say MAX I just mean the max value of whatever 
 *	the wrapped integer used is e.g. if it's a uint16 then it's 65535.
 */
//==============================================================================================


/**
 *	Wrapper for the integer we use to identify inventory items. Every item that appears in the 
 *	world needs one of these assigned to it. This includes items that start in the world at the
 *	start of the match. 
 *
 *-----------------------------------------------------------------------------------------------
 *	Good criteria for adjusting this value: 
 *	If you anticipate that more than MAX number of items will ever be spawned (not just exist in 
 *	the world at the same time but actually spawned throughout the lifetime of your match) then 
 *	you may want to increase this. Tbh I should probably make this a uint32 by default.
 *-----------------------------------------------------------------------------------------------
 */
USTRUCT()
struct FInventoryItemID
{
	GENERATED_BODY()

	UPROPERTY()
	uint16 ID;

	FInventoryItemID() : ID(0) { }
	FInventoryItemID(uint16 InID) : ID(InID) { }

	FString ToString() const
	{
		return FString::FromInt(ID);
	}

	//==========================================================================================
	//	Operator overloads. Hope I did these right
	//==========================================================================================

	// Pre increment
	FInventoryItemID & operator++()
	{
		++ID;
		return *this;
	}

	// Post increment
	FInventoryItemID operator++(int)
	{
		FInventoryItemID Temp(*this);
		++(*this);
		return Temp;
	}

	// Hash
	friend uint32 GetTypeHash(const FInventoryItemID & IDStruct)
	{
		return GetTypeHash(IDStruct.ID); // This is GetTypeHash(uint16)
	}

	/* Equality. Commented because I get compile errors with it, and I oddly got them after 
	doing something completely unrelated */
	//bool operator==(const FInventoryItemID & RHS) const
	//{
	//	return (this->ID == RHS.ID);
	//}

	// TSet/TMap appears to use this one
	friend bool operator==(const FInventoryItemID & LHS, const FInventoryItemID & RHS)
	{
		return (LHS.ID == RHS.ID);
	}
};


/** 
 *	Wrapper for a unique ID for maps.
 *
 *	----------------------------------------------------------------------------------------------
 *	If your map pool is at the limit and you need more maps then increase this 
 *	----------------------------------------------------------------------------------------------
 */
USTRUCT()
struct FMapID
{
	GENERATED_BODY()

	UPROPERTY()
	uint8 ID;

	FMapID() : ID(0) { }
	FMapID(uint8 InID) : ID(InID) { }

	FString ToString() const
	{
		return FString::FromInt(ID);
	}

	// For hash
	friend bool operator==(const FMapID & ID_1, const FMapID & ID_2)
	{
		return ID_1.ID == ID_2.ID;
	}

	friend uint32 GetTypeHash(const FMapID & InID)
	{
		return GetTypeHash(InID.ID);
	}
};


/** 
 *	Wrapper for the level or rank of a selectable. 
 *	
 *	----------------------------------------------------------------------------------------------
 *	If selectables can reach levels higher than MAX then increase this. 
 *	----------------------------------------------------------------------------------------------
 */
USTRUCT()
struct FSelectableRankInt
{
	GENERATED_BODY()

	UPROPERTY()
	uint8 Level;

	FSelectableRankInt() : Level(0) { }
	FSelectableRankInt(uint8 InLevel) : Level(InLevel) { }

	// For hash
	friend bool operator==(const FSelectableRankInt & Elem_1, const FSelectableRankInt & Elem_2)
	{
		return Elem_1.Level == Elem_2.Level;
	}

	friend uint32 GetTypeHash(const FSelectableRankInt & Elem)
	{
		return GetTypeHash(Elem.Level);
	}
};