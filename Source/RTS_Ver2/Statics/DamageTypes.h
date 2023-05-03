// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DamageType.h"
#include "DamageTypes.generated.h"

/** 
 *-----------------------------------------------------------------------------------------------
 *	A file to put damage type classes in, but you can put them in any file you want.
 *-----------------------------------------------------------------------------------------------
 *
 *	To add a damage type:
 *
 *	Through editor:
 *	1. Create a new blueprint class that childs RTSDamageType.
 *	2. In your game instance blueprint, in the TMap DamageMultipliers add a new entry and populate it
 *	with your newly created damage type class.
 *	
 *	Through C++:
 *	1. Close the editor if it is open (very important to avoid BP data loss bug).
 *	2. Copy and paste a damage type declaration from below, then change the class name to the name 
 *	you want. You are free to name it anything and do not have to follow any convention.
 *	3. Compile project in visual studio and wait for it to complete.
 *	4. Startup the editor. In your game instance BP your new damage type should be listed under
 *	DamageMultipliers. If not then just add an entry for it.
 *
 *	
 *	To remove a damage type:
 *	If the damage type class was created via editor then simply delete its blueprint.
 *	If the damage type class was added via C++ then close the editor if it is open, remove 
 *	its declaration below, then compile. When compiling has completed you can safely open the 
 *	editor.
 */


/** 
 *	Base class for all user created damage types 
 *
 *	Implementation notes: if new damage types aren't being automatically added to GI BP then this 
 *	class may need to lose the abstract UCLASS specifier
 */
UCLASS(const, Abstract)
class RTS_VER2_API URTSDamageType : public UDamageType
{
	GENERATED_BODY()

public:

	URTSDamageType();
};


//-----------------------------------------------------------------------------------------------
//===============================================================================================
//	Add new damage classes below here 
//===============================================================================================
//-----------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class RTS_VER2_API UDamageType_Default final : public URTSDamageType
{
	GENERATED_BODY()
};

UCLASS(NotBlueprintable)
class RTS_VER2_API UDamageType_Bullet final : public URTSDamageType
{
	GENERATED_BODY()
};

UCLASS(NotBlueprintable)
class RTS_VER2_API UDamageType_SniperRound final : public URTSDamageType
{
	GENERATED_BODY()
};

UCLASS(NotBlueprintable)
class RTS_VER2_API UDamageType_Explosive final : public URTSDamageType
{
	GENERATED_BODY()
};

UCLASS(NotBlueprintable)
class RTS_VER2_API UDamageType_BiologicalHeal final : public URTSDamageType
{
	GENERATED_BODY()
};

UCLASS(NotBlueprintable)
class RTS_VER2_API UDamageType_Shadow final : public URTSDamageType
{
	GENERATED_BODY()
};

UCLASS(NotBlueprintable)
class RTS_VER2_API UDamageType_Lightning final : public URTSDamageType
{
	GENERATED_BODY()
};

UCLASS(NotBlueprintable)
class RTS_VER2_API UDamageType_Frost final : public URTSDamageType
{
	GENERATED_BODY()
};

UCLASS(NotBlueprintable)
class RTS_VER2_API UDamageType_Nuke final : public URTSDamageType
{
	GENERATED_BODY()
};

/* The ammo fired from the warthog */
UCLASS(NotBlueprintable)
class RTS_VER2_API UDamageType_GAU8ArmourPiercing final : public URTSDamageType
{
	GENERATED_BODY()
};

/* The ammo fired from the warthog */
UCLASS(NotBlueprintable)
class RTS_VER2_API UDamageType_GAU8HighExplosive final : public URTSDamageType
{
	GENERATED_BODY()
};
