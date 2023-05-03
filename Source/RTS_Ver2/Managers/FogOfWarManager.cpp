// Fill out your copyright notice in the Description page of Project Settings.

#include "FogOfWarManager.h"
#include "Components/BrushComponent.h"
#include "Engine/PostProcessVolume.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture2D.h"
#include "Particles/ParticleSystemComponent.h"
#include "Public/EngineUtils.h"

#include "Statics/DevelopmentStatics.h"
#include "MapElements/Invisible/RTSLevelVolume.h"
#include "GameFramework/RTSPlayerState.h"
#include "GameFramework/RTSGameState.h"
#include "MapElements/Projectiles/ProjectileBase.h"
#include "Statics/Statics.h"
#include "Settings/ProjectSettings.h"
#include "MapElements/Building.h"
#include "MapElements/Infantry.h"
#include "Audio/FogObeyingAudioComponent.h"
#include "MapElements/InventoryItem.h"

// TODO something that would help fog managers out: when units enter a garrison move them 
// to their own container that way fog manager won't even iterate over them. I might 
// want like 3 containers on PS: AllUnits, FogRelevantUnits and GarrisonedUnits (or NotFogRelevantUnits) 
// or something.


AFogOfWarManager::AFogOfWarManager()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	bReplicates = bAlwaysRelevant = false;
}

void AFogOfWarManager::Initialize(ARTSLevelVolume * FogVolume,  uint8 InNumTeams, ETeam InLocalPlayersTeam, 
	UMaterialInterface * InFogOfWarMaterial)
{
	NumTeams = InNumTeams;
	LocalPlayersTeam = InLocalPlayersTeam;

	SetupReferences();

	SetupMapInfo(FogVolume);

	SetupTileInfo(InNumTeams);

	SetupRenderingReferences(InFogOfWarMaterial);

	SetupTeamTempRevealEffects();
}

void AFogOfWarManager::SetupReferences()
{
	APlayerController * const PlayerController = GetWorld()->GetFirstPlayerController();

	PS = CastChecked<ARTSPlayerState>(PlayerController->PlayerState);

	GS = CastChecked<ARTSGameState>(GetWorld()->GetGameState());
}

void AFogOfWarManager::SetupMapInfo(ARTSLevelVolume * FogVolume)
{
	// Take fog info from the volume
 
	FBoxSphereBounds Bounds = FogVolume->GetMapBounds();

	MapCenter = FVector2D(Bounds.Origin.X, Bounds.Origin.Y);

	const float X = Bounds.BoxExtent.X * 2;
	const float Y = Bounds.BoxExtent.Y * 2;

	MapDimensions = FVector2D(X, Y);

	MapDimensionsInverse = FVector2D::UnitVector / MapDimensions;
}

void AFogOfWarManager::SetupTileInfo(uint8 InNumTeams)
{
	/* Round up if needed */
	const int32 NumTilesX = FMath::CeilToFloat(MapDimensions.X / FogOfWarOptions::FOG_TILE_SIZE);
	const int32 NumTilesY = FMath::CeilToFloat(MapDimensions.Y / FogOfWarOptions::FOG_TILE_SIZE);

	MapTileDimensions = FIntPoint(NumTilesX, NumTilesY);

	const int32 NumTiles = NumTilesX * NumTilesY;

	/* Only server really needs to init all arrays - clients just update their own teams array */

	for (uint8 i = 0; i < InNumTeams; ++i)
	{
		TeamTiles.Emplace(FTileArray());
	}

	for (auto & Array : TeamTiles)
	{
		Array.GetTiles().Init(EFogStatus::Hidden, NumTiles);
	}
}

void AFogOfWarManager::SetupRenderingReferences(UMaterialInterface * FogOfWarMaterial)
{
	UE_CLOG(FogOfWarMaterial == nullptr, RTSLOG, Fatal, TEXT("Fog of war is enabled for project "
		"but no fog of war material set in game instance, need to set fog material"));

	/* Assign first found post process volume */
	for (TActorIterator<APostProcessVolume> It(GetWorld()); It; ++It)
	{
		PostProcessVolume = *It;
		break;
	}
	
	UE_CLOG(PostProcessVolume == nullptr, RTSLOG, Fatal, TEXT("Fog of war is enabled for project "
		"but no post process volume is found on map, need to add a post process volume to map"));

	FogTexture = UTexture2D::CreateTransient(MapTileDimensions.X, MapTileDimensions.Y);
	FogTexture->UpdateResource();
	/* Crashes on the line above could mean fog of war volume is glitched and doesn't have any
	dimensions. Need to remove it and add another */

	TextureRegions = new FUpdateTextureRegion2D(0, 0, 0, 0, MapTileDimensions.X, MapTileDimensions.Y);

	FogOfWarMaterialInstance = UMaterialInstanceDynamic::Create(FogOfWarMaterial, nullptr);
	assert(FogOfWarMaterialInstance != nullptr);
	FogOfWarMaterialInstance->SetTextureParameterValue(FName("VisibilityMask"), FogTexture);
	FogOfWarMaterialInstance->SetScalarParameterValue(FName("OneOverWorldSize"), 1.0f / MapDimensions.X);
	/* This param may not actullay be needed */
	FogOfWarMaterialInstance->SetScalarParameterValue(FName("OneOverTileSize"), 1.0f / MapTileDimensions.X);
	FogOfWarMaterialInstance->SetVectorParameterValue(FName("CenterOfVolume"), FLinearColor(MapCenter.X, MapCenter.Y, 0, 0));

	PostProcessVolume->AddOrUpdateBlendable(FogOfWarMaterialInstance);

	TextureBuffer = new uint8[MapTileDimensions.X * MapTileDimensions.Y * 4];
}

void AFogOfWarManager::SetupTeamTempRevealEffects()
{
	TeamTempRevealEffects.Init(FTemporaryFogRevealEffectsArray(16), NumTeams);
}

void AFogOfWarManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	assert(GS != nullptr);

	/* Since these are spawned by clients HasAuthority() will not distinguish
	between server and client */
	if (GetWorld()->IsServer())
	{
		/* For each team... */
		for (uint8 i = 0; i < NumTeams; ++i)
		{
			const ETeam Team = Statics::ArrayIndexToTeam(i);

			ComputeTeamVisibility(Team, DeltaTime);

			/* Render fog + hide/reveal enemy selectables if it is our own team */
			if (Team == PS->GetTeam())
			{
				/* Also does StoreTeamVisibility */
				Server_HideAndRevealSelectables(Team);
				Server_StoreResourceSpotVisInfo(Team);
				HideAndRevealTemporaries(Team);
				Server_HideAndRevealInventoryItems(Team);
				MuteAndUnmuteAudio(Team);
				RenderFogOfWar(Team);	// TODO do before HideAndRevealSelectables for performance?
			}
			else
			{
				StoreTeamVisibility(Team);
				Server_StoreResourceSpotVisInfo(Team);
				Server_StoreNonLocalTeamsInventoryItemVisInfo(Team);
			}
		}
	}
	else
	{
		const ETeam Team = PS->GetTeam();

		ComputeTeamVisibility(Team, DeltaTime);
		Client_HideAndRevealSelectables(Team);
		HideAndRevealTemporaries(Team);
		Client_HideAndRevealInventoryItems(Team);
		MuteAndUnmuteAudio(Team);
		RenderFogOfWar(Team);	// TODO do before HideAndRevealSelectables for performance?
	}
}

void AFogOfWarManager::BeginDestroy()
{
	delete TextureBuffer;
	delete TextureRegions;

	Super::BeginDestroy();
}

FIntPoint AFogOfWarManager::GetGridCoords(const AActor * Actor) const
{
	return (GetGridCoords(Actor->GetActorLocation()));
}

FIntPoint AFogOfWarManager::GetGridCoords(const FVector & WorldCoords) const
{
	return GetGridCoords(FVector2D(WorldCoords.X, WorldCoords.Y));
}

FIntPoint AFogOfWarManager::GetGridCoords(const FVector2D & World2DCoords) const
{
	FVector2D Vector = World2DCoords - MapCenter;
	Vector *= MapDimensionsInverse;
	Vector += FVector2D(0.5f, 0.5f);
	Vector *= MapTileDimensions;

	const FIntPoint TileCoords = FIntPoint(FMath::FloorToInt(Vector.X),
		FMath::FloorToInt(Vector.Y));

	return TileCoords;
}

EFogStatus AFogOfWarManager::GetFogStatusForLocation(const AActor * Actor, const TArray <EFogStatus> & TilesArray) const
{
	assert(Statics::IsValid(Actor));
	return GetFogStatusForLocation(Actor->GetActorLocation(), TilesArray);
}

EFogStatus AFogOfWarManager::GetFogStatusForLocation(const FVector & WorldCoords, const TArray <EFogStatus> & TilesArray) const
{
	return GetFogStatusForLocation(FVector2D(WorldCoords.X, WorldCoords.Y), TilesArray);
}

EFogStatus AFogOfWarManager::GetFogStatusForLocation(const FVector2D & World2DCoords, const TArray <EFogStatus> & TilesArray) const
{
	const FVector2D GridCoords = GetGridCoords(World2DCoords);
	const int32 Index = GetTileIndex(GridCoords.X, GridCoords.Y);

	UE_CLOG(!TilesArray.IsValidIndex(Index), RTSLOG, Fatal, TEXT("Fog manager: Tiles index %d not valid. "
		"World coords were: %s"), Index, *World2DCoords.ToString());

	return TilesArray[Index];
}

int32 AFogOfWarManager::GetTileIndex(int32 X, int32 Y) const
{
	return X + Y * MapTileDimensions.X;
}

void AFogOfWarManager::ResetTeamVisibility(ETeam Team)
{
	const uint8 Index = Statics::TeamToArrayIndex(Team);

	for (auto & Elem : TeamTiles[Index].GetTiles())
	{
		Elem = EFogStatus::Hidden;
	}
}

void AFogOfWarManager::ComputeTeamVisibility(ETeam Team, float DeltaTime)
{
	/* TODO: Maybe change buildings container from set to array in player state */

	assert(GS != nullptr);

	ResetTeamVisibility(Team);

	const uint8 Index = Statics::TeamToArrayIndex(Team);
	TArray<EFogStatus> & Tiles = TeamTiles[Index].GetTiles();

	// For each player state of team
	//		For each building and unit of team
	//			Set visible for every tile within range
	for (ARTSPlayerState * PlayerState : GS->GetTeamPlayerStates(Team))
	{
		/* TODO: check if possibly only related to the issue of player
		state needing a delay on initial replication therefore may be
		able to remove this check */
		if (!Statics::IsValid(PlayerState))
		{
			continue;
		}

		//-------------------------------------------------------------
		//	Units
		//-------------------------------------------------------------

		for (AInfantry * Unit : PlayerState->GetUnits())
		{
			if (!Statics::IsValid(Unit))
			{
				continue;
			}

			/* Skip if inside garrison. Units update the building's sight/stealth radius 
			when they enter it if it can be seen out of */
			if (Unit->IsInsideGarrison())
			{
				continue;
			}

			float ActorSightRadius;
			float ActorStealthRevealRadius;
			Unit->GetVisionInfo(ActorSightRadius, ActorStealthRevealRadius);

			RevealFogAroundLocation(FVector2D(Unit->GetActorLocation()), ActorSightRadius,
				ActorStealthRevealRadius, Tiles);
		}
		
		//-----------------------------------------------------------
		//	Buildings - same deal as units
		//-----------------------------------------------------------

		for (ABuilding * Building : PlayerState->GetBuildings())
		{
			if (!Statics::IsValid(Building))
			{
				continue;
			}

			float ActorSightRadius;
			float ActorStealthRevealRadius;
			Building->GetVisionInfo(ActorSightRadius, ActorStealthRevealRadius);

			RevealFogAroundLocation(FVector2D(Building->GetActorLocation()), ActorSightRadius,
				ActorStealthRevealRadius, Tiles);
		}
	}

	//------------------------------------------------------------
	//	Team's Temporary Effects
	//------------------------------------------------------------

	TArray <FTemporaryFogRevealEffectInfo> & TempEffectArray = TeamTempRevealEffects[Index].Array;
	for (int32 i = TempEffectArray.Num() - 1; i >= 0; --i)
	{
		FTemporaryFogRevealEffectInfo & Elem = TempEffectArray[i];

		float FogRevealRadius, StealthRevealRadius;
		Elem.Tick(DeltaTime, FogRevealRadius, StealthRevealRadius);

		RevealFogAroundLocation(Elem.GetLocation(), FogRevealRadius, StealthRevealRadius, Tiles);

		if (Elem.HasCompleted())
		{
			TempEffectArray.RemoveAtSwap(i, 1, false);
		}
	}
}

void AFogOfWarManager::Client_HideAndRevealSelectables(ETeam Team)
{
	const uint8 IndexToAvoid = Statics::TeamToArrayIndex(Team);

	for (int32 i = 0; i < NumTeams; ++i)
	{
		/* Avoid trying to hide/reveal selectables from our own team - they
		are always revealed */
		if (i != IndexToAvoid)
		{
			const uint8 Index = Statics::TeamToArrayIndex(Team);
			const TArray <EFogStatus> & Tiles = TeamTiles[Index].GetTiles();

			for (ARTSPlayerState * PlayerState : GS->GetTeams()[i].GetPlayerStates())
			{
#if WITH_EDITOR
				// Null sometimes in 2+ player PIE
				if (!Statics::IsValid(PlayerState))
				{
					continue;
				}
#endif

				/* Check units */
				for (AInfantry * Unit : PlayerState->GetUnits())
				{
					if (Unit->IsInsideGarrison())
					{
						/* Skip trying to hide/reveal units inside garrisons */
						continue;
					}

					Unit->UpdateFogStatus(GetFogStatusForLocation(Unit, Tiles));
				}
				const TArray<ABuilding*> & BuildingsArray = PlayerState->GetBuildings();
				/* Do same for buildings but only pass in Hidden or Revealed. 
				Iterate backwards because removals can happen along the way */
				for (int32 j = BuildingsArray.Num() - 1; j >= 0; --j)
				{
					ABuilding * Building = BuildingsArray[j];
					
					bool bCanBeSeen = false;

					for (const auto & TilesIndex : BuildingTileIndices[Building].GetArray())
					{
						if (Tiles[TilesIndex] == EFogStatus::Revealed
							|| Tiles[TilesIndex] == EFogStatus::StealthRevealed)
						{
							/* As long as one point can be seen then whole building is considered
							revealed */
							bCanBeSeen = true;
							break;
						}
					}

					Building->UpdateFogStatus(bCanBeSeen ? EFogStatus::Revealed : EFogStatus::Hidden);
				}
			}
		}
	}
}

void AFogOfWarManager::Server_HideAndRevealSelectables(ETeam Team)
{
	const uint8 IndexToAvoid = Statics::TeamToArrayIndex(Team);

	for (int32 i = 0; i < NumTeams; ++i)
	{
		/* Avoid trying to hide/reveal selectables from our own team - they
		are always revealed */
		if (i != IndexToAvoid)
		{
			const uint8 Index = Statics::TeamToArrayIndex(Team);
			const TArray <EFogStatus> & Tiles = TeamTiles[Index].GetTiles();

			FVisibilityInfo & TeamVisibilityInfo = GS->GetTeamVisibilityInfo(Team);

			for (ARTSPlayerState * PlayerState : GS->GetTeams()[i].GetPlayerStates())
			{
				/* Check units */
				for (AInfantry * Unit : PlayerState->GetUnits())
				{
					if (Unit->IsInsideGarrison())
					{
						/* Skip trying to hide/reveal units inside garrisons */
						continue;
					}
					
					const FIntPoint GridCoords = GetGridCoords(Unit);
					const int32 TilesIndex = GetTileIndex(GridCoords.X, GridCoords.Y);

					const bool bCanBeAttacked = Unit->UpdateFogStatus(Tiles[TilesIndex]);
					TeamVisibilityInfo.SetVisibility(Unit, bCanBeAttacked);
				}
				/* Do same for buildings but only pass in Hidden or Revealed */
				for (ABuilding * Building : PlayerState->GetBuildings())
				{
					if (Statics::IsValid(Building) == false)
					{
						// Do we want to remove it too?
						continue;
					}
					
					bool bCanBeSeen = false;

					for (const auto & TilesIndex : BuildingTileIndices[Building].GetArray())
					{
						if (Tiles[TilesIndex] == EFogStatus::Revealed
							|| Tiles[TilesIndex] == EFogStatus::StealthRevealed)
						{
							/* As long as one point can be seen then whole building is considered
							revealed */
							bCanBeSeen = true;
							break;
						}
					}

					// Get crashes here when building destroys
					TeamVisibilityInfo.SetVisibility(Building, bCanBeSeen);
					Building->UpdateFogStatus(bCanBeSeen ? EFogStatus::Revealed : EFogStatus::Hidden);
				}
			}
		}
	}
}

void AFogOfWarManager::Server_StoreResourceSpotVisInfo(ETeam Team)
{ 
	// Commented for now since don't really need it. Can uncomment later if needed

	//FVisibilityInfo & TeamVisibilityInfo = GS->GetTeamVisibilityInfo(Team);
	//
	//for (const auto & ResourceSpot : GS->GetAllResourceSpots())
	//{
	//	// May want to use array of points instead of just GetActorLocation
	//	const FIntPoint GridCoords = GetGridCoords(ResourceSpot->GetActorLocation());
	//	const int32 TilesIndex = GetTileIndex(GridCoords.X, GridCoords.Y);
	//
	//	const bool bOutsideFog = IsTileVisible(TilesIndex, Team);
	//
	//	TeamVisibilityInfo.SetVisibility(ResourceSpot, bOutsideFog);
	//}

	/* Do we even really care about storing whether players can see resource spots or not? 
	We allow clicks on them even if they are inside fog. 
	The function ISelectable::CanBeClickedOn was created because resource spots have different 
	behavior than other selectables (they CAN be clicked on when inside fog). An alternative 
	to using this virtual func is to just do FVisibilityInfo::SetVisibility(Spot, true) 
	once when the resource spot sets up, then never call this function. */
}

void AFogOfWarManager::HideAndRevealTemporaries(ETeam Team)
{
	/* Here we iterate backwards and remove nulls since they don't remove themselves
	when they are destroyed. Most likely gonna need locks on these arrays if switching
	to multi-threading */

	TArray <AProjectileBase *> & TempFogProjectiles = GS->GetTemporaryFogProjectiles();
	for (int32 i = TempFogProjectiles.Num() - 1; i >= 0; --i)
	{
		AProjectileBase * const Projectile = TempFogProjectiles[i];

		if (!Statics::IsValid(Projectile))
		{
			TempFogProjectiles.RemoveAtSwap(i, 1, false);
			continue;
		}

		const bool bCanBeSeen = IsProjectileVisible(Projectile, Team);

		/* TODO: need to override this for replicating projectiles and avoid toggling bHidden */
		Projectile->SetActorHiddenInGame(!bCanBeSeen);
	}

	TArray <UParticleSystemComponent *> & TempFogParticles = GS->GetTemporaryFogParticles();

	for (int32 i = TempFogParticles.Num() - 1; i >= 0; --i)
	{
		UParticleSystemComponent * const Comp = TempFogParticles[i];

		if (!Statics::IsValid(Comp))
		{
			TempFogParticles.RemoveAtSwap(i, 1, false);
			continue;
		}

		const FIntPoint GridCoords = GetGridCoords(Comp->GetComponentLocation());
		const int32 TilesIndex = GetTileIndex(GridCoords.X, GridCoords.Y);

		const bool bCanBeSeen = IsTileVisibleNotChecked(TilesIndex, Team);

		Comp->SetHiddenInGame(!bCanBeSeen);
	}
}

void AFogOfWarManager::Server_HideAndRevealInventoryItems(ETeam Team)
{
	FVisibilityInfo & TeamVisibilityInfo = GS->GetTeamVisibilityInfo(Team);
	
	for (const auto & ItemActor : GS->GetInventoryItemsInWorld())
	{
		const FIntPoint GridCoords = GetGridCoords(ItemActor->GetActorLocation());
		const int32 TilesIndex = GetTileIndex(GridCoords.X, GridCoords.Y);

		const bool bCanBeSeen = IsTileVisible(TilesIndex, Team);

		TeamVisibilityInfo.SetVisibility(ItemActor, bCanBeSeen);

		ItemActor->SetVisibilityFromFogManager(bCanBeSeen);
	}	
}

void AFogOfWarManager::Server_StoreNonLocalTeamsInventoryItemVisInfo(ETeam Team)
{
	FVisibilityInfo & TeamVisibilityInfo = GS->GetTeamVisibilityInfo(Team);

	for (const auto & ItemActor : GS->GetInventoryItemsInWorld())
	{
		const FIntPoint GridCoords = GetGridCoords(ItemActor->GetActorLocation());
		const int32 TilesIndex = GetTileIndex(GridCoords.X, GridCoords.Y);

		const bool bCanBeSeen = IsTileVisible(TilesIndex, Team);

		TeamVisibilityInfo.SetVisibility(ItemActor, bCanBeSeen);
	}
}

void AFogOfWarManager::Client_HideAndRevealInventoryItems(ETeam Team)
{
	for (const auto & ItemActor : GS->GetInventoryItemsInWorld())
	{
		const FIntPoint GridCoords = GetGridCoords(ItemActor->GetActorLocation());
		const int32 TilesIndex = GetTileIndex(GridCoords.X, GridCoords.Y);

		const bool bCanBeSeen = IsTileVisible(TilesIndex, Team);

		ItemActor->SetVisibilityFromFogManager(bCanBeSeen);
	}
}

void AFogOfWarManager::MuteAndUnmuteAudio(ETeam Team)
{
	// Alternative that might be faster: have two containers: one for muted and one for unmuted
	for (const auto & AudioComp : GS->FogSoundsContainer_Dynamic.GetArray())
	{
		// Hmmm... getting crashes here. Not 100% sure why.
		assert(AudioComp != nullptr);
		assert(AudioComp->IsPendingKill() == false);
		
		if (AudioComp->IsMuted())
		{
			if (IsLocationVisibleNotChecked(AudioComp->GetComponentLocation(), Team))
			{
				AudioComp->OnExitFogOfWar_Dynamic(GS);
			}
		}
		else
		{
			if (!IsLocationVisibleNotChecked(AudioComp->GetComponentLocation(), Team))
			{
				AudioComp->OnEnterFogOfWar_Dynamic(GS);
			}
		}
	}

	for (int32 i = GS->FogSoundsContainer_AlwaysKnownOnceHeard.Num() - 1; i >= 0; --i)
	{
		UFogObeyingAudioComponent * AudioComp = GS->FogSoundsContainer_AlwaysKnownOnceHeard.GetArray()[i];

		if (IsLocationVisibleNotChecked(AudioComp->GetComponentLocation(), Team))
		{
			AudioComp->OnExitFogOfWar_AlwaysKnownOnceHeard(GS);
		}
	}

	// These types of sounds can actually use the Dynamic container instead
	for (const auto & AudioComp : GS->FogSoundsContainer_DynamicExceptForInstigatorsTeam.GetArray())
	{
		// For debugging. May possibly have to just check for them and remove as iterating
		assert(AudioComp);

		if (AudioComp->IsMuted())
		{
			if (IsLocationVisibleNotChecked(AudioComp->GetComponentLocation(), Team))
			{
				AudioComp->OnExitFogOfWar_DynamicExceptForInstigatorsTeam(GS);
			}
		}
		else
		{
			if (!IsLocationVisibleNotChecked(AudioComp->GetComponentLocation(), Team))
			{
				AudioComp->OnEnterFogOfWar_DynamicExceptForInstigatorsTeam(GS);
			}
		}
	}
}

void AFogOfWarManager::RevealFogAroundLocation(FVector2D Location, float FogRevealRadius, 
	float StealthRevealRadius, TArray <EFogStatus> & Tiles)
{
	const int32 SightRadiusInTiles = FMath::FloorToInt(FogRevealRadius / FogOfWarOptions::FOG_TILE_SIZE);
	const int32 StealthSightRadiusInTiles = FMath::FloorToInt(StealthRevealRadius / FogOfWarOptions::FOG_TILE_SIZE);
	const int32 HighestRadiusInTiles = SightRadiusInTiles > StealthSightRadiusInTiles ? SightRadiusInTiles : StealthSightRadiusInTiles;

	const FIntPoint TileLocation = GetGridCoords(Location);

	/* Hoisting this out now in case compiler does not do it */
	if (StealthRevealRadius <= 0.f)
	{
		for (int32 Radius_Y = -HighestRadiusInTiles; Radius_Y <= HighestRadiusInTiles; ++Radius_Y)
		{
			for (int32 Radius_X = -HighestRadiusInTiles; Radius_X <= HighestRadiusInTiles; ++Radius_X)
			{
				const int32 Tile_X = TileLocation.X + Radius_X;
				const int32 Tile_Y = TileLocation.Y + Radius_Y;

				/* Check if tile is within range */
				if (Tile_X >= 0 && Tile_Y >= 0 && Tile_X < MapTileDimensions.X
					&& Tile_Y < MapTileDimensions.Y
					&& FMath::Square(Radius_X) + FMath::Square(Radius_Y) < FMath::Square(SightRadiusInTiles))
				{
					const int32 TileIndex = GetTileIndex(Tile_X, Tile_Y);

					/* Reveal tile using bitwise or operator */
					Tiles[TileIndex] = BitwiseOrOperator(Tiles[TileIndex], EFogStatus::Revealed);
					/* Get rid of this case now. It's the same as Hidden */
					if (Tiles[TileIndex] == EFogStatus::StealthDetected)
					{
						Tiles[TileIndex] = EFogStatus::Hidden;
					}
				}
			}
		}
	}
	else
	{
		for (int32 Radius_Y = -HighestRadiusInTiles; Radius_Y <= HighestRadiusInTiles; ++Radius_Y)
		{
			for (int32 Radius_X = -HighestRadiusInTiles; Radius_X <= HighestRadiusInTiles; ++Radius_X)
			{
				const int32 Tile_X = TileLocation.X + Radius_X;
				const int32 Tile_Y = TileLocation.Y + Radius_Y;

				/* Check if tile is within range */
				if (Tile_X >= 0 && Tile_Y >= 0 && Tile_X < MapTileDimensions.X
					&& Tile_Y < MapTileDimensions.Y)
				{
					/* Assuming static_cast<uint8>(SomeBool) assigns 0x01 for true and 0x00 for false */
					const uint8 bInSightRange = FMath::Square(Radius_X) + FMath::Square(Radius_Y) < FMath::Square(SightRadiusInTiles);
					const uint8 bInStealthRevealRange = FMath::Square(Radius_X) + FMath::Square(Radius_Y) < FMath::Square(StealthSightRadiusInTiles);
					/* Least significant bit = revealed
					2nd least sginificant bit = reveals stealth.
					Both have to be true to actually reveal stealth selectables */
					const uint8 FinalTileStatus = bInSightRange | (bInStealthRevealRange << 1);

					const int32 TileIndex = GetTileIndex(Tile_X, Tile_Y);

					/* Reveal tile using bitwise or operator */
					Tiles[TileIndex] = BitwiseOrOperator(Tiles[TileIndex], FinalTileStatus);
					/* Get rid of this case now. It's the same as Hidden */
					if (Tiles[TileIndex] == EFogStatus::StealthDetected)
					{
						Tiles[TileIndex] = EFogStatus::Hidden;
					}
				}
			}
		}
	}
}

void AFogOfWarManager::StoreTeamVisibility(ETeam Team)
{
	const uint8 IndexToAvoid = Statics::TeamToArrayIndex(Team);

	for (int32 i = 0; i < NumTeams; ++i)
	{
		/* Avoid updating status for selectables on our own team - they are always visible */
		if (i != IndexToAvoid)
		{
			const uint8 Index = Statics::TeamToArrayIndex(Team);
			const TArray <EFogStatus> & Tiles = TeamTiles[Index].GetTiles();

			FVisibilityInfo & TeamVisibilityInfo = GS->GetTeamVisibilityInfo(Team);

			for (ARTSPlayerState * PlayerState : GS->GetTeams()[i].GetPlayerStates())
			{
				/* Check units */
				for (AInfantry * Unit : PlayerState->GetUnits())
				{
					const FIntPoint GridCoords = GetGridCoords(Unit);
					const int32 TilesIndex = GetTileIndex(GridCoords.X, GridCoords.Y);

					if (Tiles[TilesIndex] == EFogStatus::StealthRevealed)
					{
						TeamVisibilityInfo.SetVisibility(Unit, true);
					}
					else if (Tiles[TilesIndex] == EFogStatus::Revealed)
					{
						const bool bCanBeSeen = !Unit->IsInStealthMode();
						TeamVisibilityInfo.SetVisibility(Unit, bCanBeSeen);
					}
					else
					{
						TeamVisibilityInfo.SetVisibility(Unit, false);
					}
				}
				/* Do same for buildings. For now buildings cannot be stealth so don't bother
				checking if they are */
				for (ABuilding * Building : PlayerState->GetBuildings())
				{
					bool bCanBeSeen = false;

					for (const auto & TilesIndex : BuildingTileIndices[Building].GetArray())
					{
						if (Tiles[TilesIndex] == EFogStatus::Revealed
							|| Tiles[TilesIndex] == EFogStatus::StealthRevealed)
						{
							/* As long as one point can be seen then whole building is considered
							revealed */
							bCanBeSeen = true;
							break;
						}
					}

					TeamVisibilityInfo.SetVisibility(Building, bCanBeSeen);
				}
			}
		}
	}
}

void AFogOfWarManager::RenderFogOfWar(ETeam Team)
{
	FillTextureBuffer(Team);

	/* Currently fog of war volume needs have equal width and length (or at least have the same
	amount of tiles for width and length) */
	assert(MapTileDimensions.X == MapTileDimensions.Y);

	UpdateTextureRegions(FogTexture, 0, 1, TextureRegions, /*Proportional to TileSize (512 previous hardcoded value)*/ MapTileDimensions.X * 4, (uint32)4, TextureBuffer, false);

	/* TODO: make sure this and all other FNames are defined at statics
	in the headers or something  */
	FogOfWarMaterialInstance->SetTextureParameterValue(FName("VisibilityMask"), FogTexture);
}

void AFogOfWarManager::FillTextureBuffer(ETeam Team)
{
	const uint8 TeamIndex = Statics::TeamToArrayIndex(Team);
	const TArray <EFogStatus> & Tiles = TeamTiles[TeamIndex].GetTiles();

	for (int32 Y = 0; Y < MapTileDimensions.Y; ++Y)
	{
		for (int32 X = 0; X < MapTileDimensions.X; ++X)
		{
			const int32 Index = GetTileIndex(X, Y);

			if (static_cast<uint8>(Tiles[Index]) & 0x01) /* Translation: if tile revealed */
			{
				TextureBuffer[Index * 4] = 0;
				TextureBuffer[Index * 4 + 1] = 0;
				TextureBuffer[Index * 4 + 2] = 255;
				TextureBuffer[Index * 4 + 3] = 0;
			}
			else
			{
				TextureBuffer[Index * 4] = 0;
				TextureBuffer[Index * 4 + 1] = 255;
				TextureBuffer[Index * 4 + 2] = 0;
				TextureBuffer[Index * 4 + 3] = 0;
			}
		}
	}
}

/* Taken from wiki.unrealengine.com/Dynamic_Textures */
void AFogOfWarManager::UpdateTextureRegions(UTexture2D * Texture, int32 MipIndex, uint32 NumRegions, FUpdateTextureRegion2D * Regions, uint32 SrcPitch, uint32 SrcBpp, uint8 * SrcData, bool bFreeData)
{
	assert(Texture != nullptr);
	assert(Texture->Resource != nullptr);

	struct FUpdateTextureRegionsData
	{
		FTexture2DResource * Texture2DResource;
		int32 MipIndex;
		uint32 NumRegions;
		FUpdateTextureRegion2D * Regions;
		uint32 SrcPitch;
		uint32 SrcBpp;
		uint8* SrcData;
	};

	FUpdateTextureRegionsData * RegionData = new FUpdateTextureRegionsData;

	RegionData->Texture2DResource = (FTexture2DResource*)Texture->Resource;
	RegionData->MipIndex = MipIndex;
	RegionData->NumRegions = NumRegions;
	RegionData->Regions = Regions;
	RegionData->SrcPitch = SrcPitch;
	RegionData->SrcBpp = SrcBpp;
	RegionData->SrcData = SrcData;

	ENQUEUE_RENDER_COMMAND(UpdateTextureRegionsData)(
		[RegionData, bFreeData](FRHICommandListImmediate& RHICommandList) // 4.22: how this macro works changed and now I have to specifiy a FRHICommandList& param
																		  // Just FRHICommandList or FRHICommandListImmediate ? Does it matter?
		{
			for (uint32 RegionIndex = 0; RegionIndex < RegionData->NumRegions; ++RegionIndex)
			{
				int32 CurrentFirstMip = RegionData->Texture2DResource->GetCurrentFirstMip();
				if (RegionData->MipIndex >= CurrentFirstMip)
				{
					RHIUpdateTexture2D(
						RegionData->Texture2DResource->GetTexture2DRHI(),
						RegionData->MipIndex - CurrentFirstMip,
						RegionData->Regions[RegionIndex],
						RegionData->SrcPitch,
						RegionData->SrcData
						+ RegionData->Regions[RegionIndex].SrcY * RegionData->SrcPitch
						+ RegionData->Regions[RegionIndex].SrcX * RegionData->SrcBpp
					);
				}
			}
	if (bFreeData)
	{
		FMemory::Free(RegionData->Regions);
		FMemory::Free(RegionData->SrcData);
	}
	delete RegionData;
		});
}

uint8 AFogOfWarManager::IsTileVisible(int32 TileIndex, ETeam Team) const
{
	const uint8 Index = Statics::TeamToArrayIndex(Team);

	return static_cast<uint8>(TeamTiles[Index].GetTiles()[TileIndex]) & 0x01;
}

uint8 AFogOfWarManager::IsTileVisibleNotChecked(int32 TileIndex, ETeam Team) const
{
	const uint8 Index = Statics::TeamToArrayIndex(Team);

	const TArray<EFogStatus> & Array = TeamTiles[Index].GetTiles();
	return Array.IsValidIndex(TileIndex) ? static_cast<uint8>(Array[TileIndex]) & 0x01 : 0; // Return 0 for EFogStatus::Hidden
}

EFogStatus AFogOfWarManager::BitwiseOrOperator(const EFogStatus & Enum1, const EFogStatus & Enum2) const
{
	return static_cast<EFogStatus>(static_cast<uint8>(Enum1) | static_cast<uint8>(Enum2));
}

EFogStatus AFogOfWarManager::BitwiseOrOperator(const EFogStatus & Enum1, const uint8 & Enum2AsInt) const
{
	return static_cast<EFogStatus>(static_cast<uint8>(Enum1) | Enum2AsInt);
}

bool AFogOfWarManager::IsLocationVisible(const FVector & Location, ETeam Team) const
{
	const FIntPoint GridCoords = GetGridCoords(Location);

	return IsTileVisible(GetTileIndex(GridCoords.X, GridCoords.Y), Team);
}

bool AFogOfWarManager::IsLocationVisibleNotChecked(const FVector & Location, ETeam Team) const
{
	const FIntPoint GridCoords = GetGridCoords(Location);

	const uint8 Index = Statics::TeamToArrayIndex(Team);
	const TArray<EFogStatus> & TeamsArray = TeamTiles[Index].GetTiles();
	const int32 ArrayIndex = GetTileIndex(GridCoords.X, GridCoords.Y);
	if (TeamsArray.IsValidIndex(ArrayIndex))
	{
		return static_cast<uint8>(TeamsArray[ArrayIndex]) & 0x01;
	}
	else
	{
		return false;
	}
}

bool AFogOfWarManager::IsLocationLocallyVisible(const FVector & Location) const
{
	return IsLocationVisible(Location, LocalPlayersTeam);
}

bool AFogOfWarManager::IsLocationLocallyVisibleNotChecked(const FVector & Location) const
{
	return IsLocationVisibleNotChecked(Location, LocalPlayersTeam);
}

EFogStatus AFogOfWarManager::GetLocationVisibilityStatus(const FVector & Location, ETeam Team) const
{
	const FIntPoint GridCoords = GetGridCoords(Location);
	const int32 TileIndex = GetTileIndex(GridCoords.X, GridCoords.Y);
	const uint8 TeamIndex = Statics::TeamToArrayIndex(Team);
	return TeamTiles[TeamIndex].GetTiles()[TileIndex];
}

EFogStatus AFogOfWarManager::GetLocationVisibilityStatusLocally(const FVector & Location) const
{
	return GetLocationVisibilityStatus(Location, LocalPlayersTeam);
}

EFogStatus AFogOfWarManager::GetLocationVisibilityStatusNotChecked(const FVector & Location, ETeam Team) const
{
	const FIntPoint GridCoords = GetGridCoords(Location);
	const int32 TileIndex = GetTileIndex(GridCoords.X, GridCoords.Y);
	const uint8 TeamIndex = Statics::TeamToArrayIndex(Team);
	const TArray<EFogStatus> & TilesArray = TeamTiles[TeamIndex].GetTiles();
	if (TilesArray.IsValidIndex(TileIndex))
	{
		return TilesArray[TileIndex];
	}
	else
	{
		return EFogStatus::Hidden;
	}
}

EFogStatus AFogOfWarManager::GetLocationVisibilityStatusLocallyNotChecked(const FVector & Location) const
{
	return GetLocationVisibilityStatusNotChecked(Location, LocalPlayersTeam);
}

bool AFogOfWarManager::IsProjectileVisible(const AProjectileBase * Projectile, ETeam Team) const
{
	TArray <FVector, TInlineAllocator<AProjectileBase::NUM_FOG_LOCATIONS>> Locations;
	Projectile->GetFogLocations(Locations);
	/* If any location is revealed we will reveal the projectile just like buildings */
	bool bCanBeSeen = false;
	for (const auto & Loc : Locations)
	{
		const FIntPoint GridCoords = GetGridCoords(Loc);
		const int32 TilesIndex = GetTileIndex(GridCoords.X, GridCoords.Y);

		bCanBeSeen = IsTileVisibleNotChecked(TilesIndex, Team);
		if (bCanBeSeen)
		{
			break;
		}
	}

	return bCanBeSeen;
}

void AFogOfWarManager::OnBuildingPlaced(ABuilding * InBuilding, const FVector & BuildingLocation,
	const FRotator & BuildingRotation)
{
	BuildingTileIndices.Emplace(InBuilding, FIntegerArray(InBuilding->GetFogLocations().Num()));

	// Calculate and store each grid location building occupies
	for (int32 i = 0; i < InBuilding->GetFogLocations().Num(); ++i)
	{
		const FVector2D Elem = InBuilding->GetFogLocations()[i];

		// Apply buildings rotation and location
		FVector2D Loc = Elem.GetRotated(BuildingRotation.Yaw);
		Loc += FVector2D(BuildingLocation.X, BuildingLocation.Y);

		/* Convert world location to tile location. Worthwhile since building cannot move
		(cannot move 2D coord-wise) and saves having to do it on every tick */
		const FIntPoint GridCoords = GetGridCoords(Loc);
		const int32 TilesIndex = GetTileIndex(GridCoords.X, GridCoords.Y);

		BuildingTileIndices[InBuilding].Emplace(TilesIndex);
	}
}

void AFogOfWarManager::OnBuildingDestroyed(ABuilding * InBuilding)
{
	// Remove entry from building fog locations. This frees TMap value memory right?
	BuildingTileIndices.Remove(InBuilding);
}

void AFogOfWarManager::CreateTeamTemporaryRevealEffect(const FTemporaryFogRevealEffectInfo & RevealEffect, FVector2D Location, ETeam Team)
{
	const int32 Index = Statics::TeamToArrayIndex(Team);
	TeamTempRevealEffects[Index].Array.Emplace(FTemporaryFogRevealEffectInfo(RevealEffect, Location));
}
