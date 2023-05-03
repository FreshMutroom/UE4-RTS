// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Structs_8.generated.h"


UENUM()
enum class EKeyModifiers : uint8
{
	None = 0,
	CTRL = 1,
	ALT = 2,
	SHIFT = 4,
	CTRL_ALT = 3,
	CTRL_SHIFT = 5,
	ALT_SHIFT = 6,
	CTRL_ALT_SHIFT = 7
};


USTRUCT()
struct FKeyWithModifiers
{
	GENERATED_BODY()

public:

	// This ctor was added for code generation
	FKeyWithModifiers()
		: Key(EKeys::Invalid)
		, Modifiers(EKeyModifiers::None)
	{
	}

	explicit FKeyWithModifiers(const FKey & InKey, EKeyModifiers InKeyModifiers)
		: Key(InKey)
		, Modifiers(InKeyModifiers)
	{
	}

	explicit FKeyWithModifiers(const FKey & InKey)
		: Key(InKey)
		, Modifiers(EKeyModifiers::None)
	{
	}

	UPROPERTY()
	FKey Key;

	UPROPERTY()
	EKeyModifiers Modifiers;

	bool HasSHIFTModifier() const { return static_cast<uint8>(Modifiers) & static_cast<uint8>(EKeyModifiers::SHIFT); }
	bool HasCTRLModifier() const { return static_cast<uint8>(Modifiers) & static_cast<uint8>(EKeyModifiers::CTRL); }
	bool HasALTModifier() const { return static_cast<uint8>(Modifiers) & static_cast<uint8>(EKeyModifiers::ALT); }
	bool HasCMDModifier() const { return false; }

	friend bool operator==(const FKeyWithModifiers & Struct_1, const FKeyWithModifiers & Struct_2)
	{
		return Struct_1.Key == Struct_2.Key && Struct_1.Modifiers == Struct_2.Modifiers;
	}

	friend uint32 GetTypeHash(const FKeyWithModifiers & Struct)
	{
		return GetTypeHash(Struct.Key);
	}

	FString ToString() const
	{
		return FString()
			+ (HasCTRLModifier() ? "CTRL + " : FString())
			+ (HasALTModifier() ? "ALT + " : FString())
			+ (HasSHIFTModifier() ? "SHIFT + " : FString())
			+ Key.ToString();
	}
};
