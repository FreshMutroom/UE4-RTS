// Fill out your copyright notice in the Description page of Project Settings.

#if MULTITHREADED_FOG_OF_WAR

#include "MultithreadedFogOfWar.h"
#include "HAL/RunnableThread.h"


// TODO garrisoned infantry should not reveal any tiles. Remember they update the 
// building when they enter it. Possibly do nothing in this class and instead in the 
// OnGameThreadTick func do not add units to the containers if they are inside a garrison

// TODO the fog manager should not try to hide/reveal enemy units inside garrisons

//--------------------------------------------------------------------------------------------
//============================================================================================
//	------- Fog Manager Class -------
//============================================================================================
//--------------------------------------------------------------------------------------------

// Is this ok?
MultithreadedFogOfWarManager MultithreadedFogOfWarManager::FogManager;

void MultithreadedFogOfWarManager::Setup(UWorld * World, ARTSLevelVolume * FogVolume,  uint8 InNumTeams,
	uint8 InLocalPlayersTeamIndex, UMaterialInterface * FogOfWarMaterial)
{
	bIsServer = World->IsServer();
	LocalPlayersTeamIndex = InLocalPlayersTeamIndex;
	NumTeams = InNumTeams;

	//-----------------------------------------------------------
	//	Set map info using fog volume
	//-----------------------------------------------------------

	FBoxSphereBounds Bounds = FogVolume->GetMapBounds();

	MapCenter = FVector2D(Bounds.Origin.X, Bounds.Origin.Y);

	const float X = Bounds.BoxExtent.X * 2;
	const float Y = Bounds.BoxExtent.Y * 2;

	const FVector2D MapDimensions = FVector2D(X, Y);

	MapDimensionsInverse = FVector2D::UnitVector / MapDimensions;

	/* Round up if needed */
	const int32 NumTilesX = FMath::CeilToFloat(MapDimensions.X / FogOfWarOptions::FOG_TILE_SIZE);
	const int32 NumTilesY = FMath::CeilToFloat(MapDimensions.Y / FogOfWarOptions::FOG_TILE_SIZE);

	MapTileDimensions = FIntPoint(NumTilesX, NumTilesY);

	const int32 NumTiles = NumTilesX * NumTilesY;

	UE_CLOG(FogOfWarMaterial == nullptr, RTSLOG, Fatal, TEXT("Fog of war is enabled for project "
		"but no fog of war material set in game instance, need to set fog material"));

	APostProcessVolume * PostProcessVolume = nullptr;
	for (TActorIterator<APostProcessVolume> Iter(World); Iter; ++Iter)
	{
		PostProcessVolume = *Iter;
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
	/* This param may not actually be needed */
	FogOfWarMaterialInstance->SetScalarParameterValue(FName("OneOverTileSize"), 1.0f / MapTileDimensions.X);
	FogOfWarMaterialInstance->SetVectorParameterValue(FName("CenterOfVolume"), FLinearColor(MapCenter.X, MapCenter.Y, 0, 0));

	PostProcessVolume->AddOrUpdateBlendable(FogOfWarMaterialInstance);

	TextureBuffer = new uint8[MapTileDimensions.X * MapTileDimensions.Y * 4];

	//---------------------------------------------------------
	//	Filling Containers
	//---------------------------------------------------------

	if (bIsServer)
	{
		// Fill TilesArrays
		for (int32 i = 0; i < NumTeams; ++i)
		{
			TilesArrays[i].Array.Init(EFogStatus::Hidden, NumTiles);
		}

		// Fill TeamTilesShared
		for (int32 i = 0; i < NumTeams; ++i)
		{
			TeamTilesShared[i].Array.Init(EFogStatus::Hidden, NumTiles);
		}
	}
	else
	{
		// Fill TilesArrays
		TilesArrays[LocalPlayersTeamIndex].Array.Init(EFogStatus::Hidden, NumTiles);

		// Fill TeamTilesShared
		TeamTilesShared[LocalPlayersTeamIndex].Array.Init(EFogStatus::Hidden, NumTiles);
	}

	//-----------------------------------------------------
	//	Creating threads
	//-----------------------------------------------------

	const int32 NumThreads = GetNumFogOfWarThreadsToCreate();
	if (bIsServer && false)// Using && false to force server to actually use client threads at the moment just for testing
	{
		//ServerIdleThreads.Array.Reserve(NumThreads);
		//for (int32 i = 0; i < NumThreads; ++i)
		//{
		//	ServerIdleThreads.Array.Emplace(new ServerFogOfWarThread());
		//}
	}
	else
	{
		ClientIdleThreads.Array.Reserve(NumThreads);
		for (int32 i = 0; i < NumThreads; ++i)
		{
			ClientIdleThreads.Array.Emplace(new ClientFogOfWarThread());
		}
	}

	//-------------------------------------------------------
	//	Done
	//-------------------------------------------------------

	assert(!bHasSetup);
	bHasSetup = true;
}

int32 MultithreadedFogOfWarManager::GetNumFogOfWarThreadsToCreate() const
{
	/* Might wanna factor these in at some point */
	//const int32 NumCores = FPlatformMisc::NumberOfCores();
	//const int32 NumLogicalCores = FPlatformMisc::NumberOfCoresIncludingHyperthreads();
	
	// Both 1 for testing
	if (bIsServer)
	{
		return 1;
		//return FMath::Max(NumTeams / 2, 2);
	}
	else
	{
		return 1;
	}
}

void MultithreadedFogOfWarManager::OnGameThreadTick(float InDeltaTime)
{
	if (!bHasSetup)
	{
		return;
	}

	if (bIsServer && false) // && false added so server does client logic, just for testing
	{
		// TODO
	}
	else
	{
		// If choosing to hide/reveal selectables in a batch then do that now. Or 
		// can do it anywhere really.
		
		bool bIssuedTask = false;
		
		ClientIdleThreads.Lock.Lock();
		if (ClientIdleThreads.Array.Num() > 0)
		{
			ClientIdleThreads.Lock.Unlock();

			CalculationProgress & Progress = TeamProgress[LocalPlayersTeamIndex];

			Progress.Lock.Lock();

			//----------------------------------------------
			//	Begin critical section
			//----------------------------------------------

			/* We do a lot of work here, but there are actually no other threads contending for 
			this lock right now so it's ok */
			if (Progress.NextTask == EMulthreadedFogTask::Finished)
			{
				bIssuedTask = true;
				
				/* Tell a thread to reset the tiles array. While it does this we will fill the 
				infantry/buildings arrays */
				ClientIdleThreads.Array.Last()->ResetTileInfo(LocalPlayersTeamIndex);

				Progress.NextTask = EMulthreadedFogTask::RevealTilesFromBuildings;

				//--------------------------------------------------------------
				//	Begin critical section
				//--------------------------------------------------------------
				BuildingStatesShared[LocalPlayersTeamIndex].ArrayLock.Lock();

				// Copy array from game state into fog manager
				TArray<BuildingFogInfo> & NewlyCreatedBuildings = RecentlyCreatedBuildings[LocalPlayersTeamIndex].Array;
				TArray<BuildingFogInfo> & BuildingsArray = BuildingStatesShared[LocalPlayersTeamIndex].Array;
				BuildingsArray.Reserve(BuildingsArray.Num() + NewlyCreatedBuildings.Num());
				for (const auto & Elem : NewlyCreatedBuildings)
				{
					BuildingsArray.Emplace(Elem);
				}

				// Copy array from game state into fog manager
				TArray<BuildingFogInfo> & BuildingsUpdated = RecentlyModifiedBuildings[LocalPlayersTeamIndex].Array;
				for (auto & Elem : BuildingsUpdated)
				{
					for (const auto & Entry : BuildingsArray)
					{
						if (Elem.SelectableID == Entry.SelectableID)
						{
							// We're assuming it's either one of sight radius or stealth detection radius by just copying those two values
							Elem.SightRadius = Entry.SightRadius;
							Elem.StealthRevealRadius = Entry.StealthRevealRadius;
							break;
						}
					}
				}

				// Copy array from game state into fog manager
				TArray<uint8> & DestroyedBuildingsArray = RecentlyDestroyedBuildings[LocalPlayersTeamIndex].Array;
				DestroyedBuildingArray & DestroyedBuildingsStruct = DestroyedBuildingsShared[LocalPlayersTeamIndex];
				for (const auto & Elem : DestroyedBuildingsArray)
				{
					DestroyedBuildingsStruct.SetBitToRecentlyDestroyed(Elem);
				}

				InfantryStatesShared[LocalPlayersTeamIndex].ArrayLock.Lock();
				BuildingStatesShared[LocalPlayersTeamIndex].ArrayLock.Unlock();
				//--------------------------------------------------------------
				//	End critical section and begin new one
				//--------------------------------------------------------------

				TArray <InfantryFogInfo> & InfantryArray = InfantryStatesShared[LocalPlayersTeamIndex].Array;
				TArray <AInfantry*> & OtherArray = TeamsInfantry[LocalPlayersTeamIndex].Array;
				threadAssert(InfantryArray.Num() == 0);
				InfantryArray.Reserve(OtherArray.Num());
				for (const auto & Infantry : OtherArray)
				{
					InfantryArray.Emplace(InfantryFogInfo(Infantry));
				}

				TemporaryRevealEffectsShared[LocalPlayersTeamIndex].ArrayLock.Lock();
				InfantryStatesShared[LocalPlayersTeamIndex].ArrayLock.Unlock();
				//--------------------------------------------------------------
				//	End critical section and begin new one
				//--------------------------------------------------------------

				TArray<FTemporaryFogRevealEffectInfo> & TempEffectsArray = RecentlyCreatedTemporaryRevealEffects[LocalPlayersTeamIndex].Array;
				TArray<FTemporaryFogRevealEffectInfo> & SharedTempEffectArray = TemporaryRevealEffectsShared[LocalPlayersTeamIndex].Array;
				SharedTempEffectArray.Append(TempEffectsArray);

				DeltaTime = DeltaTimeNotThreadSafe + InDeltaTime; 

				TemporaryRevealEffectsShared[LocalPlayersTeamIndex].ArrayLock.Unlock();

				/* Reset these arrays outside the critical sections so fog threads do not have to wait
				on us doing it. But it might be slightly slower this way for the game thread.
				It's kind of a trade off: do we want to improve fog thread performance or game thread
				performance? Maybe framerates will be better with these instead the critical sections.
				Will have to test. But only the infantry array will really be big */
				TempEffectsArray.Reset();
				DestroyedBuildingsArray.Reset();
				BuildingsUpdated.Reset();
				NewlyCreatedBuildings.Reset();
			}

			//----------------------------------------------
			//	End critical section
			//----------------------------------------------

			// Maybe this can be released earlier... it may be the locks like the building/infantry etc that threads will wait on
			Progress.Lock.Unlock();

			if (bIssuedTask)
			{
				DeltaTimeNotThreadSafe = 0.f;

				/* Now while threads are doing the infantry, building and temp effects revealing, 
				we begin filling the hostile team arrays. */

				for (int32 i = 0; i < NumTeams; ++i)
				{
					// Skip our own team; already did them above
					if (i == LocalPlayersTeamIndex)
					{
						continue;
					}

					InfantryFogInfoBasicArray & InfantryArrayStruct = InfantryStatesBasicShared[i];

					// Infantry lock is the only lock we'll use here. Whoever has it has 
					// access to both the infantry array and building array
					InfantryArrayStruct.ArrayLock.Lock();

					//----------------------------------------------
					//	Begin critical section
					//----------------------------------------------
					TArray<InfantryFogInfoBasic> & InfantryArray = InfantryArrayStruct.Array;
					threadAssert(InfantryArray.Num() == 0);
					TArray<AInfantry*> & OtherArray = TeamsInfantry[i].Array;
					InfantryArray.Reserve(OtherArray.Num());
					for (const auto & Infantry : OtherArray)
					{
						InfantryArray.Emplace(InfantryFogInfoBasic(Infantry));
					}

					TArray<BuildingFogInfo> & BuildingsArray = BuildingStatesShared[i].Array;
					TArray<BuildingFogInfo> & NewBuildings = RecentlyCreatedBuildings[i].Array;
					BuildingsArray.Reserve(BuildingsArray.Num() + NewBuildings.Num());
					for (const auto & Elem : NewBuildings)
					{
						BuildingsArray.Emplace(Elem);
					}

					DestroyedBuildingArray & DestroyedBuildingsStruct = DestroyedBuildingsShared[i];
					threadAssert(DestroyedBuildingsStruct.AreAllFlagsReset());
					TArray<uint8> & AnotherArray = RecentlyDestroyedBuildings[i].Array;
					for (const auto & Elem : AnotherArray)
					{
						DestroyedBuildingsStruct.SetBitToRecentlyDestroyed(Elem);
					}

					//----------------------------------------------
					//	End critical section
					//----------------------------------------------

					InfantryArrayStruct.ArrayLock.Unlock();
				}
			}
			else
			{
				DeltaTimeNotThreadSafe += InDeltaTime;
			}
		}
		else
		{
			ClientIdleThreads.Lock.Unlock();
			DeltaTimeNotThreadSafe += InDeltaTime;
		}
	}
}

void MultithreadedFogOfWarManager::OnBuildingDestroyed(ABuilding * Building)
{
	RecentlyDestroyedBuildings[Building->GetTeamIndex()].Array.Emplace(Building->GetSelectableID());
}

void MultithreadedFogOfWarManager::RegisterTeamTemporaryRevealEffect(const FTemporaryFogRevealEffectInfo & Effect, FVector2D WorldLocation, ETeam Team)
{
	const int32 Index = Statics::TeamToArrayIndex(Team);
	RecentlyCreatedTemporaryRevealEffects[Index].Array.Emplace(FTemporaryFogRevealEffectInfo(Effect, WorldLocation));
}

void MultithreadedFogOfWarManager::RevealTilesAroundLocation(const FVector2D & Location, float SightRadius, float StealthRevealRadius, TArray<EFogStatus>& Tiles)
{
	const int32 SightRadiusInTiles = FMath::FloorToInt(SightRadius / FogOfWarOptions::FOG_TILE_SIZE);
	const int32 StealthSightRadiusInTiles = FMath::FloorToInt(StealthRevealRadius / FogOfWarOptions::FOG_TILE_SIZE);
	const int32 HighestRadiusInTiles = SightRadiusInTiles > StealthSightRadiusInTiles ? SightRadiusInTiles : StealthSightRadiusInTiles;

	const FIntPoint TileLocation = GetFogGridCoords(Location);

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

EFogStatus MultithreadedFogOfWarManager::GetLocationFogStatus(const FVector2D & Location, const TArray<EFogStatus>& Tiles) const
{
	return Tiles[WorldLocationToTilesIndex(Location)];
}

void MultithreadedFogOfWarManager::OnAllTasksComplete(uint8 TeamIndex)
{
	TeamProgress[TeamIndex].NextTeamIndexForStoringSelectableVisibilityInfo = 0;
}

void MultithreadedFogOfWarManager::AddRecentlyCreatedBuilding(ABuilding * Building)
{
	RecentlyCreatedBuildings[Building->GetTeamIndex()].Array.Emplace(BuildingFogInfo(Building));
}

int32 MultithreadedFogOfWarManager::WorldLocationToTilesIndex(const FVector2D & WorldLocation) const
{
	FIntPoint Loc = GetFogGridCoords(WorldLocation);
	return GetTileIndex(Loc.X, Loc.Y);
}

FIntPoint MultithreadedFogOfWarManager::GetFogGridCoords(const FVector & WorldLocation) const
{
	return GetFogGridCoords(FVector2D(WorldLocation));
}

FIntPoint MultithreadedFogOfWarManager::GetFogGridCoords(const FVector2D & WorldLocation) const
{
	FVector2D Vector = WorldLocation - MapCenter;
	Vector *= MapDimensionsInverse;
	Vector += FVector2D(0.5f, 0.5f);
	Vector *= MapTileDimensions;

	const FIntPoint TileCoords = FIntPoint(FMath::FloorToInt(Vector.X),
		FMath::FloorToInt(Vector.Y));

	return TileCoords;
}

EFogStatus MultithreadedFogOfWarManager::BitwiseOrOperator(const EFogStatus & Enum1, const EFogStatus & Enum2)
{
	return static_cast<EFogStatus>(static_cast<uint8>(Enum1) | static_cast<uint8>(Enum2));
}

EFogStatus MultithreadedFogOfWarManager::BitwiseOrOperator(const EFogStatus & Enum1, const uint8 & Enum2AsInt)
{
	return static_cast<EFogStatus>(static_cast<uint8>(Enum1) | Enum2AsInt);
}

bool MultithreadedFogOfWarManager::IsLocationLocallyVisible(const FVector & Location) const
{
	const FIntPoint Loc = GetFogGridCoords(FVector2D(Location));
	const int32 TilesIndex = GetTileIndex(Loc.X, Loc.Y);
	const EFogStatus TileStatus = TilesArrays[LocalPlayersTeamIndex].Array[TilesIndex];
	return TileStatus == EFogStatus::Revealed || TileStatus == EFogStatus::StealthRevealed;
}

bool MultithreadedFogOfWarManager::IsLocationVisibleNotChecked(const FVector & Location, ETeam Team) const
{
	const FIntPoint GridCoords = GetFogGridCoords(Location);

	const int32 Index = Statics::TeamToArrayIndex(Team);
	const TArray<EFogStatus> & TeamsArray = TilesArrays[Index].Array;
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

bool MultithreadedFogOfWarManager::IsSelectableOutsideFog(const ISelectable * Selectable, ETeam Team) const
{
	const int32 CheckingTeamIndex = Statics::TeamToArrayIndex(Team);
	const EFogStatus Status = TeamVisibilityInfo[CheckingTeamIndex][Selectable->GetTeamIndex()].Array[Selectable->GetSelectableID()];
	return Status == EFogStatus::Revealed || Status == EFogStatus::StealthRevealed;
}

int32 MultithreadedFogOfWarManager::GetTileIndex(int32 X, int32 Y) const
{
	// Ummm I think this is the wrong way round for good cache locality. 
	// Regular AFogOfWarManager has it round the wrong way too I think
	return X + Y * MapTileDimensions.X; 
}


//--------------------------------------------------------------------------------------------
//============================================================================================
//	------- Client Thread Class -------
//============================================================================================
//--------------------------------------------------------------------------------------------

ClientFogOfWarThread::ClientFogOfWarThread()
{
	FRunnableThread::Create(this, TEXT("Fog of war thread"), 0, TPri_BelowNormal);
}

ClientFogOfWarThread::~ClientFogOfWarThread()
{
	// Possibly want some stuff here
}

bool ClientFogOfWarThread::Init()
{
	return true;
}

uint32 ClientFogOfWarThread::Run()
{	
	return 0;
}

void ClientFogOfWarThread::Stop()
{
}

void ClientFogOfWarThread::Exit()
{
}

void ClientFogOfWarThread::ResetTileInfo(uint8 TeamIndex)
{
	TArray<EFogStatus> & Tiles = MultithreadedFogOfWarManager::Get().TeamTilesShared[TeamIndex].Array;
	for (auto & Elem : Tiles)
	{
		Elem = EFogStatus::Hidden;
	}

	//-----------------------------------------------------------
	//	Move on to next task
	//-----------------------------------------------------------

	CalculateTeamVisibilityFromBuildings(TeamIndex, Tiles);
}

void ClientFogOfWarThread::CalculateTeamVisibilityFromBuildings(uint8 TeamIndex, TArray<EFogStatus> & Tiles)
{
	MultithreadedFogOfWarManager & FogManager = MultithreadedFogOfWarManager::Get();
	
	BuildingFogInfoArray & BuildingsStruct = FogManager.BuildingStatesShared[TeamIndex];

	BuildingsStruct.ArrayLock.Lock();

	/* Buildings are a little more complicated than infantry. Because buildings do not move the 
	game thread did not have to send the location of the building - we already have it stored 
	in non-game thread land. For the hell of it we also store the sight radius and stealth reveal 
	radius on. */

	/* Start off by adding any recently created buildings to the array. Actually the 
	game thread should do this for us. Actually same with sight/stealth radius too */

	TArray<BuildingFogInfo> & BuildingsArray = BuildingsStruct.Array;
	DestroyedBuildingArray & RecentlyDestroyedBuildingsStruct = FogManager.DestroyedBuildingsShared[TeamIndex];

	/* Loop through array and reveal tiles */
	for (int32 i = BuildingsArray.Num() - 1; i >= 0; --i)
	{
		BuildingFogInfo & Elem = BuildingsArray[i];
		
		if (RecentlyDestroyedBuildingsStruct.WasRecentlyDestroyed(Elem))
		{
			RecentlyDestroyedBuildingsStruct.SetBitToNotRecentlyDestroyed(Elem);
			
			/* If it was recently destroyed then remove it from array */
			BuildingsArray.RemoveAtSwap(i, 1, false);
		}
		else
		{
			FogManager.RevealTilesAroundLocation(Elem.WorldLocation, Elem.SightRadius,
				Elem.StealthRevealRadius, Tiles);
		}
	}

	BuildingsStruct.ArrayLock.Unlock();

	//-----------------------------------------------------------
	//	Move on to next task
	//-----------------------------------------------------------

	CalculateTeamVisibilityFromInfantry(TeamIndex, Tiles);
}

void ClientFogOfWarThread::CalculateTeamVisibilityFromInfantry(uint8 TeamIndex, TArray<EFogStatus> & Tiles)
{
	InfantryFogInfoArray & InfantryArray = MultithreadedFogOfWarManager::Get().InfantryStatesShared[TeamIndex];

	InfantryArray.ArrayLock.Lock();

	for (const auto & Elem : InfantryArray.Array)
	{
		MultithreadedFogOfWarManager::Get().RevealTilesAroundLocation(Elem.WorldLocation, Elem.SightRadius,
			Elem.StealthRevealRadius, Tiles);
	}

	/* Do this now otherwise game thread has to do it. */
	InfantryArray.Array.Reset();

	/* This could actually be moved to right under the ArrayLock() call. We just wait on the message
	from game thread that it has populated array then we can go. The game thread will never
	try and contend for the lock again until the whole fog process has finished so yeah whatever
	we'll unlock here though */
	InfantryArray.ArrayLock.Unlock();

	//-----------------------------------------------------------
	//	Move on to next task
	//-----------------------------------------------------------

	TickAndCalculateTeamVisibilityFromTemporaryRevealEffects(TeamIndex, Tiles);
}

void ClientFogOfWarThread::TickAndCalculateTeamVisibilityFromTemporaryRevealEffects(uint8 TeamIndex, TArray<EFogStatus>& Tiles)
{
	MultithreadedFogOfWarManager & FogManager = MultithreadedFogOfWarManager::Get();
	TempRevealEffectArray & Struct = FogManager.TemporaryRevealEffectsShared[TeamIndex];
	
	Struct.ArrayLock.Lock();

	const float DeltaTime = FogManager.DeltaTime;

	for (int32 i = Struct.Array.Num() - 1; i >= 0; --i)
	{
		FTemporaryFogRevealEffectInfo & Elem = Struct.Array[i];

		float FogRevealRadius, StealthRevealRadius;
		Elem.Tick(DeltaTime, FogRevealRadius, StealthRevealRadius);

		FogManager.RevealTilesAroundLocation(Elem.GetLocation(), FogRevealRadius,
			StealthRevealRadius, Tiles);

		if (Elem.HasCompleted())
		{
			Struct.Array.RemoveAtSwap(i, 1, false);
		}
	}

	Struct.ArrayLock.Unlock();

	//-----------------------------------------------------------
	//	Move on to next task
	//-----------------------------------------------------------

	// Should ALWAYS aquire this lock first time: there should be no other threads running
	FogManager.ClientIdleThreads.Lock.Lock();

	TArray<ClientFogOfWarThread*> & IdleThreadArray = FogManager.ClientIdleThreads.Array;
	CalculationProgress & Progress = FogManager.TeamProgress[TeamIndex];

	/* Get a thread to queue render command */
	if (IdleThreadArray.Num() > 0)
	{
		IdleThreadArray.Pop()->QueueRenderFogCommand(TeamIndex, Tiles);
	}

	/* Tell any other threads (they will all be idle) to start working on storing the 
	selectable visibility info. */
	while (IdleThreadArray.Num() > 0 && Progress.NextTeamIndexForStoringSelectableVisibilityInfo < FogManager.NumTeams)
	{
		// Skip our own team: they are always visible to us
		if (Progress.NextTeamIndexForStoringSelectableVisibilityInfo == TeamIndex)
		{
			Progress.NextTeamIndexForStoringSelectableVisibilityInfo++;
		}

		IdleThreadArray.Pop()->CalculateAndStoreSelectableVisibilityInfo(TeamIndex, Progress.GetNextTeamIndexForStoringSelectableVisibilityInfo(), Tiles);
	}

	// If there's still idle threads assign one to projectiles
	if (IdleThreadArray.Num() > 0)
	{
		IdleThreadArray.Pop()->CalculateAndStoreProjectileVisibilityInfo(TeamIndex, Tiles);
	}

	// If there's still idle threads assign one to particle systems
	if (IdleThreadArray.Num() > 0)
	{
		IdleThreadArray.Pop()->CalculateAndStoreParticleSystemVisibilityInfo(TeamIndex, Tiles);
	}

	// If there's still idle threads assign one to inventory items
	if (IdleThreadArray.Num() > 0)
	{
		IdleThreadArray.Pop()->CalculateAndStoreInventoryItemVisibilityInfo(TeamIndex, Tiles);
	}

	FogManager.ClientIdleThreads.Lock.Unlock();

	// Now this thread will store tile visibility info
	StoreTileVisibilityInfo(TeamIndex, Tiles);
}

// Perhaps do this right before/after the enqueue render command because cache will be 
// fresh with tiles array
void ClientFogOfWarThread::StoreTileVisibilityInfo(uint8 TeamIndex, const TArray<EFogStatus> & Tiles)
{
	MultithreadedFogOfWarManager & FogManager = MultithreadedFogOfWarManager::Get();

	// Copy tiles array from here into
	FogManager.TilesArrays[TeamIndex].Lock.Lock();
	
	// Just a thought: could we perhaps get away with using the same array for all teams by 
	// like packing the bits into certain places hmmm... we need at least 2 bits for each 
	// tile per team so the array could store uint16, this would allow for up to 8 teams. 
	// Don't need to do it for clients though - just something server can consider

	// Game thread could be waiting on us - be quick
	// Some examples of when gamethread might we wanting to access the tiles array: 
	// - an ability wants to know whether a location is inside fog
	FMemory::Memcpy(&FogManager.TilesArrays[TeamIndex].Array, &FogManager.TeamTilesShared[TeamIndex].Array, FogManager.TilesArrays[TeamIndex].Array.Num());

	FogManager.TilesArrays[TeamIndex].Lock.Unlock();

	//-----------------------------------------------------------
	//	Move on to next task
	//-----------------------------------------------------------

	FogManager.ClientIdleThreads.Lock.Lock(); 

	/* Make sure this function automatically skips over our own team and checks whether 
	all teams have been done */
	const EMulthreadedFogTask NextTask = FogManager.TeamProgress[TeamIndex].GetNextTask();

	switch (NextTask)
	{
		// Finished is code for 'all done'
		case EMulthreadedFogTask::Finished:
		{
			// Go idle
			FogManager.ClientIdleThreads.Array.Emplace(this);
			FogManager.ClientIdleThreads.Lock.Unlock();
			break;
		}
	
		case EMulthreadedFogTask::QueueUpRenderingFogOfWar:
		{
			FogManager.TeamProgress[TeamIndex].AdvanceNextTask(TeamIndex, FogManager);
			FogManager.ClientIdleThreads.Lock.Unlock();

			QueueRenderFogCommand(TeamIndex, Tiles);
			break;
		}

		case EMulthreadedFogTask::StoreHostileTeamSelectableVisibility:
		{
			const uint8 EnemyTeamIndex = FogManager.TeamProgress[TeamIndex].GetNextTeamIndexForStoringSelectableVisibilityInfo();
			
			FogManager.TeamProgress[TeamIndex].AdvanceNextTask(TeamIndex, FogManager);
			FogManager.ClientIdleThreads.Lock.Unlock();

			CalculateAndStoreSelectableVisibilityInfo(TeamIndex, EnemyTeamIndex, Tiles);
			break;
		}

		case EMulthreadedFogTask::StoreProjectilesVisibility:
		{
			FogManager.TeamProgress[TeamIndex].AdvanceNextTask(TeamIndex, FogManager);
			FogManager.ClientIdleThreads.Lock.Unlock();
			
			CalculateAndStoreProjectileVisibilityInfo(TeamIndex, Tiles);
			break;
		}

		case EMulthreadedFogTask::StoreParticleSystemsVisibility:
		{
			FogManager.TeamProgress[TeamIndex].AdvanceNextTask(TeamIndex, FogManager);
			FogManager.ClientIdleThreads.Lock.Unlock();
			
			CalculateAndStoreParticleSystemVisibilityInfo(TeamIndex, Tiles);
			break;
		}

		case EMulthreadedFogTask::StoreInventoryItemVisibility:
		{
			FogManager.TeamProgress[TeamIndex].AdvanceNextTask(TeamIndex, FogManager);
			FogManager.ClientIdleThreads.Lock.Unlock();
			
			CalculateAndStoreInventoryItemVisibilityInfo(TeamIndex, Tiles);
			break;
		}

		default:
		{
			// Should not have got here
			threadAssert(0);
#if !UE_BUILD_SHIPPING
			FogManager.ClientIdleThreads.Lock.Unlock();
#endif
			break;
		}
	}
}

void ClientFogOfWarThread::QueueRenderFogCommand(uint8 TeamIndex, const TArray<EFogStatus> & Tiles)
{
	//--------------------------------------------------------
	//	Fill texture buffer
	//--------------------------------------------------------

	MultithreadedFogOfWarManager & FogManager = MultithreadedFogOfWarManager::Get();

	uint8 * TextureBuffer = FogManager.TextureBuffer;
	const int32 TileDimensions_X = FogManager.MapTileDimensions.X;
	const int32 TileDimensions_Y = FogManager.MapTileDimensions.Y;
	
	for (int32 Y = 0; Y < TileDimensions_Y; ++Y)
	{
		for (int32 X = 0; X < TileDimensions_X; ++X)
		{
			const int32 Index = FogManager.GetTileIndex(X, Y);

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

	//------------------------------------------------------
	//	Queue render command
	//------------------------------------------------------

	/* This *may* need to be queued from the game thread instead of a fog thread. Commenting 
	it out for now because I get crashes with it (which may possibly be because of what I 
	just stated) */

	//struct FUpdateTextureRegionsData
	//{
	//	FTexture2DResource * Texture2DResource;
	//	int32 MipIndex;
	//	uint32 NumRegions;
	//	FUpdateTextureRegion2D * Regions;
	//	uint32 SrcPitch;
	//	uint32 SrcBpp;
	//	uint8 * SrcData;
	//};
	//
	//FUpdateTextureRegionsData * RegionData = new FUpdateTextureRegionsData;
	//
	//RegionData->Texture2DResource = (FTexture2DResource*)FogManager->FogTexture->Resource;
	//RegionData->MipIndex = 0;
	//RegionData->NumRegions = 1;
	//RegionData->Regions = FogManager->TextureRegions;
	//RegionData->SrcPitch = FogManager->MapTileDimensions.X * 4;
	//RegionData->SrcBpp = (uint32)4;
	//RegionData->SrcData = TextureBuffer;
	//const bool bFreeData = false;
	//
	//ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
	//	UpdateTextureRegionsData,
	//	FUpdateTextureRegionsData*, RegionData, RegionData,
	//	bool, bFreeData, bFreeData,
	//	{
	//		for (uint32 RegionIndex = 0; RegionIndex < RegionData->NumRegions; ++RegionIndex)
	//		{
	//			int32 CurrentFirstMip = RegionData->Texture2DResource->GetCurrentFirstMip();
	//			if (RegionData->MipIndex >= CurrentFirstMip)
	//			{
	//				RHIUpdateTexture2D(
	//					RegionData->Texture2DResource->GetTexture2DRHI(),
	//					RegionData->MipIndex - CurrentFirstMip,
	//					RegionData->Regions[RegionIndex],
	//					RegionData->SrcPitch,
	//					RegionData->SrcData
	//					+ RegionData->Regions[RegionIndex].SrcY * RegionData->SrcPitch
	//					+ RegionData->Regions[RegionIndex].SrcX * RegionData->SrcBpp
	//				);
	//			}
	//		}
	//if (bFreeData)
	//{
	//	FMemory::Free(RegionData->Regions);
	//	FMemory::Free(RegionData->SrcData);
	//}
	//delete RegionData;
	//	});
	//
	// Does this need to happen every frame or just once? Try it out
	//static const FName VisMaskFName = FName("VisibilityMask");
	//FogManager->FogOfWarMaterialInstance->SetTextureParameterValue(VisMaskFName, FogManager->FogTexture);

	//-----------------------------------------------------------
	//	Move on to next task
	//-----------------------------------------------------------

	/* Help with storing selectable visibility info */
	// Check if any teams are not being done

	// If all teams are already being done then we can go idle

	FogManager.ClientIdleThreads.Lock.Lock();

	// Should I aquire another lock here?
	const EMulthreadedFogTask NextTask = FogManager.TeamProgress[TeamIndex].GetNextTask();

	switch (NextTask)
	{
		// Finished is code for 'all done'
		case EMulthreadedFogTask::Finished:
		{
			// Go idle
			FogManager.ClientIdleThreads.Array.Emplace(this);
			FogManager.ClientIdleThreads.Lock.Unlock();
			break;
		}

		case EMulthreadedFogTask::StoreHostileTeamSelectableVisibility:
		{
			const uint8 EnemyTeamIndex = FogManager.TeamProgress[TeamIndex].GetNextTeamIndexForStoringSelectableVisibilityInfo();

			FogManager.TeamProgress[TeamIndex].AdvanceNextTask(TeamIndex, FogManager);
			FogManager.ClientIdleThreads.Lock.Unlock();

			CalculateAndStoreSelectableVisibilityInfo(TeamIndex, EnemyTeamIndex, Tiles);
			break;
		}

		case EMulthreadedFogTask::StoreProjectilesVisibility:
		{
			FogManager.TeamProgress[TeamIndex].AdvanceNextTask(TeamIndex, FogManager);
			FogManager.ClientIdleThreads.Lock.Unlock();

			CalculateAndStoreProjectileVisibilityInfo(TeamIndex, Tiles);
			break;
		}

		case EMulthreadedFogTask::StoreParticleSystemsVisibility:
		{
			FogManager.TeamProgress[TeamIndex].AdvanceNextTask(TeamIndex, FogManager);
			FogManager.ClientIdleThreads.Lock.Unlock();

			CalculateAndStoreParticleSystemVisibilityInfo(TeamIndex, Tiles);
			break;
		}

		case EMulthreadedFogTask::StoreInventoryItemVisibility:
		{
			FogManager.TeamProgress[TeamIndex].AdvanceNextTask(TeamIndex, FogManager);
			FogManager.ClientIdleThreads.Lock.Unlock();

			CalculateAndStoreInventoryItemVisibilityInfo(TeamIndex, Tiles);
			break;
		}

		default:
		{
			// Should not have got here
			threadAssert(0);
#if !UE_BUILD_SHIPPING
			FogManager.ClientIdleThreads.Lock.Unlock();
#endif
			break;
		}
	}
}

void ClientFogOfWarThread::CalculateAndStoreSelectableVisibilityInfo(uint8 TeamWithVisionIndex, uint8 OtherTeamIndex, const TArray<EFogStatus> & Tiles)
{
	MultithreadedFogOfWarManager & FogManager = MultithreadedFogOfWarManager::Get();
	
	TArray<InfantryFogInfoBasic> & InfantryArray = FogManager.InfantryStatesBasicShared[OtherTeamIndex].Array;
	
	for (auto & Elem : InfantryArray)
	{
		Elem.FogStatus = FogManager.GetLocationFogStatus(Elem.WorldLocation, Tiles);
	}

	// Now transfer the data to a container the game thread will access
	FogManager.TeamVisibilityInfo[TeamWithVisionIndex][OtherTeamIndex].ArrayLock.Lock();
	for (const auto & Elem : InfantryArray)
	{
		FogManager.TeamVisibilityInfo[TeamWithVisionIndex][OtherTeamIndex].Array[Elem.SelectableID] = Elem.FogStatus;
	}
	FogManager.TeamVisibilityInfo[TeamWithVisionIndex][OtherTeamIndex].ArrayLock.Unlock();

	InfantryArray.Reset();

	// Now do buildings too
	
	// We might need to process the building updates such as new/destroyed and sight radius changes first

	TArray<BuildingFogInfo> & HostileBuildings = FogManager.BuildingStatesShared[OtherTeamIndex].Array;

	for (auto & Elem : HostileBuildings)
	{
		const TArray<int32> & BuildingLocations = Elem.FogLocationsIndices;
		bool bFoundVisibleTile = false;

		/* Buildings can be present at multiple locations. Have to check each one. If at least 
		one tile is visible we'll say that the building is visible */
		for (int32 i = 0; i < BuildingLocations.Num(); ++i)
		{
			const EFogStatus TileStatus = Tiles[BuildingLocations[i]];
			/* We treat Revealed and StealthRevealed as the same thing since buildings cannot 
			go stealth */
			if (TileStatus == EFogStatus::Revealed || TileStatus == EFogStatus::StealthRevealed)
			{
				Elem.FogStatus = EFogStatus::Revealed;
				bFoundVisibleTile = true;
				break;
			}
		}

		if (!bFoundVisibleTile)
		{
			Elem.FogStatus = EFogStatus::Hidden;
		}
	}

	// Now transfer the data to a container the game thread will access
	FogManager.TeamVisibilityInfo[TeamWithVisionIndex][OtherTeamIndex].ArrayLock.Lock();
	for (const auto & Elem : HostileBuildings)
	{
		FogManager.TeamVisibilityInfo[TeamWithVisionIndex][OtherTeamIndex].Array[Elem.SelectableID] = Elem.FogStatus;
	}
	FogManager.TeamVisibilityInfo[TeamWithVisionIndex][OtherTeamIndex].ArrayLock.Unlock();

	//-----------------------------------------------------------
	//	Move on to next task
	//-----------------------------------------------------------

	FogManager.ClientIdleThreads.Lock.Lock();

	// Should I aquire another lock here?
	const EMulthreadedFogTask NextTask = FogManager.TeamProgress[TeamWithVisionIndex].GetNextTask();

	switch (NextTask)
	{
		// Finished is code for 'all done'
		case EMulthreadedFogTask::Finished:
		{
			// Go idle
			FogManager.ClientIdleThreads.Array.Emplace(this);
			FogManager.ClientIdleThreads.Lock.Unlock();
			break;
		}

		case EMulthreadedFogTask::StoreHostileTeamSelectableVisibility:
		{
			const uint8 EnemyTeamIndex = FogManager.TeamProgress[TeamWithVisionIndex].GetNextTeamIndexForStoringSelectableVisibilityInfo();

			FogManager.TeamProgress[TeamWithVisionIndex].AdvanceNextTask(TeamWithVisionIndex, FogManager);
			FogManager.ClientIdleThreads.Lock.Unlock();

			CalculateAndStoreSelectableVisibilityInfo(TeamWithVisionIndex, EnemyTeamIndex, Tiles);
			break;
		}

		case EMulthreadedFogTask::StoreProjectilesVisibility:
		{
			FogManager.TeamProgress[TeamWithVisionIndex].AdvanceNextTask(TeamWithVisionIndex, FogManager);
			FogManager.ClientIdleThreads.Lock.Unlock();

			CalculateAndStoreProjectileVisibilityInfo(TeamWithVisionIndex, Tiles);
			break;
		}

		case EMulthreadedFogTask::StoreParticleSystemsVisibility:
		{
			FogManager.TeamProgress[TeamWithVisionIndex].AdvanceNextTask(TeamWithVisionIndex, FogManager);
			FogManager.ClientIdleThreads.Lock.Unlock();

			CalculateAndStoreParticleSystemVisibilityInfo(TeamWithVisionIndex, Tiles);
			break;
		}

		case EMulthreadedFogTask::StoreInventoryItemVisibility:
		{
			FogManager.TeamProgress[TeamWithVisionIndex].AdvanceNextTask(TeamWithVisionIndex, FogManager);
			FogManager.ClientIdleThreads.Lock.Unlock();

			CalculateAndStoreInventoryItemVisibilityInfo(TeamWithVisionIndex, Tiles);
			break;
		}

		default:
		{
		// Should not have got here
		threadAssert(0);
		break;
		}
	}
}

void ClientFogOfWarThread::CalculateAndStoreProjectileVisibilityInfo(uint8 TeamIndex, const TArray<EFogStatus>& Tiles)
{
}

void ClientFogOfWarThread::CalculateAndStoreParticleSystemVisibilityInfo(uint8 TeamIndex, const TArray<EFogStatus>& Tiles)
{
}

void ClientFogOfWarThread::CalculateAndStoreInventoryItemVisibilityInfo(uint8 TeamIndex, const TArray<EFogStatus>& Tiles)
{
}

void CalculationProgress::AdvanceNextTask(uint8 TeamIndex, MultithreadedFogOfWarManager & FogManager)
{
	if (NextTask == EMulthreadedFogTask::StoreHostileTeamSelectableVisibility)
	{
		NextTeamIndexForStoringSelectableVisibilityInfo++;
		if (NextTeamIndexForStoringSelectableVisibilityInfo == FogManager.NumTeams)
		{
			NextTask = EMulthreadedFogTask::StoreProjectilesVisibility;
		}
		else if (NextTeamIndexForStoringSelectableVisibilityInfo == TeamIndex)
		{
			NextTeamIndexForStoringSelectableVisibilityInfo++;
			if (NextTeamIndexForStoringSelectableVisibilityInfo == FogManager.NumTeams)
			{
				NextTask = EMulthreadedFogTask::StoreProjectilesVisibility;
			}
		}
	}
	else
	{
		NextTask = static_cast<EMulthreadedFogTask>(static_cast<uint8>(NextTask) << 1);
	}
}

#endif // MULTITHREADED_FOG_OF_WAR
