// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Statics/Structs_2.h"
#include "RTSLevelVolume.generated.h"

class USceneCaptureComponent2D;
class ARTSPlayerStart;
class UBoxComponent;


/* If true then use code for 'no edit GI BP' workaround. Changing this to 0 will not remove 
all code for it though - some parts I didn't wrap in this TODO remove need for this */
#define USING_RTSLEVELVOLUME_WORKAROUND 1


/**
 *	A volume to be placed on the map. It defines: 
 *	- the area of fog of war that is revealable
 *	- the area of the map that should be shown on the minimap 
 *	- It also stores the locations of RTS player starts placed on map 
 *	- the area the player's camera is allowed to visit
 */
UCLASS()
class RTS_VER2_API ARTSLevelVolume : public AActor
{
	GENERATED_BODY()
	
public:	
	
	ARTSLevelVolume();

#if WITH_EDITOR
	virtual void OnConstruction(const FTransform & Transform) override;
#endif

	virtual void Destroyed() override;

protected:

#if WITH_EDITORONLY_DATA

	/** 
	 *	Box that defines the area of the map where fog of war can be toggled and the area of the 
	 *	map that should be used as the minimap image 
	 */
	UPROPERTY(VisibleDefaultsOnly)
	UBoxComponent * BoxComp;

	/**
	 *	The scene capture component that captures an image to use as a texture for minimap
	 */
	UPROPERTY(VisibleDefaultsOnly)
	USceneCaptureComponent2D * MinimapSceneCapture;

	/* Name of map this volume was placed on */
	FString MapName;

#endif // WITH_EDITORONLY_DATA

	/* 4 box components. Each one is a wall that represents the edge of the map and should 
	block at least the player's camera. 
	Possibly we could use just one box component instead that is hollow but I do not know 
	how to do that or if it's possible or if it will even improve performance so for now I 
	will just use 4 box components. 
	I know I could just add a single ABrush actor to scene and make it hollow but I want 
	the user to not have to worry about adding 2 actors (actually a possible way to do this 
	could be to spawn an ABrush every time this actor is moved/placed into world, so users 
	only have to worry about 1 actor, but still don't know whether it will imporve performance) 
	Anyway novel complete */

	// How thick the walls are 
	static const float WALL_THICKNESS;

	UPROPERTY()
	UBoxComponent * MapWall_North;
	UPROPERTY()
	UBoxComponent * MapWall_East;
	UPROPERTY()
	UBoxComponent * MapWall_South;
	UPROPERTY()
	UBoxComponent * MapWall_West;

	// Sets up a map wall box comp in ctor
	static void SetMapWallConstructorValues(UBoxComponent * InBoxComp);

	/**
	 *	The minimap texture created while in editor. You can look but cannot touch. 
	 *	Actually for the meanwhile this has been changed from VisibleAnywhere to EditDefaultsOnly 
	 *	(this and PlayerStarts and MapBounds - for 'no edit GI BP' workaround)
	 */
	UPROPERTY(EditAnywhere /*VisibleAnywhere*/)
	UTexture2D * MinimapTexture;

	/** This is a path reference to the minimap texture. Basically without this the UTexture2D 
	created while in editor will be null although I can't get it working with this */
	UPROPERTY()
	TSoftObjectPtr < UTexture2D > MinimapTextureSoftPtr;

	/**
	 *	Array of all RTSPlayerStarts on map. 
	 */
	UPROPERTY(EditAnywhere)
	TArray < FPlayerStartInfo > PlayerStarts;

	/** Bounds of the map this volume was placed in */
	UPROPERTY(EditDefaultsOnly)
	FBoxSphereBounds MapBounds;

#if WITH_EDITOR

	/* Return what ortho widget should be set to on minimap scene campture comp */
	float CalcSceneCaptureOrthoWidth() const;

public:

	/* Create minimap texture and also store transforms of player starts */
	void StoreMapInfo();

#endif // WITH_EDITOR

public:

	//--------------------------------------------------------------------------------------
	//	These 3 functions are here until I find a way to edit the game instance blueprint 
	//	from this class in C++
	//--------------------------------------------------------------------------------------

	/**
	 *	Get the texture to use as the background of the minimap. Not really 100% necessary to
	 *	have this, the data is also stored on disk or at least should be stored elsewhere on
	 *	post edit
	 */
	UTexture2D * GetMinimapTexture();

	/** Get bounds of map */
	const FBoxSphereBounds & GetMapBounds() const;

	/* Get player starts array */
	const TArray < FPlayerStartInfo > & GetPlayerStarts() const;

#if WITH_EDITORONLY_DATA

protected:

	/* If one doesn't exist then creates a texture to be used as the minimap for the current
	map */
	void CreateMinimapTexture();

	/* Store the transforms of all the player starts */
	void StorePlayerStartInfo();

	/* Store the bounds for the map */
	void StoreMapBounds();

	/* Writes all the data about the map to disk so game instance can access it later */
	void WriteMapInfoToDisk();

	/* Adjusts the dimensions of each of the map wall box comps to whatever they should be now */
	void AdjustWallDimensions();

	/* Don't think this is called when user changes proportions with editor scaling tools */
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent & PropertyChangedEvent) override;
	virtual void PostEditChangeChainProperty(struct FPropertyChangedChainEvent & PropertyChangedEvent) override;
	virtual void PostEditMove(bool bFinished) override;

#endif // WITH_EDITORONLY_DATA

};
