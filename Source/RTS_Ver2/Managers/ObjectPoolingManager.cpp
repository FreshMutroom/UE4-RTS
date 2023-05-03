// Fill out your copyright notice in the Description page of Project Settings.

#include "ObjectPoolingManager.h"
#include "Classes/Engine/World.h"

#include "MapElements/Projectiles/ProjectileBase.h"
#include "GameFramework/RTSGameState.h"
#include "Statics/Statics.h"
#include "Statics/DevelopmentStatics.h"
#include "GameFramework/RTSGameInstance.h"


//==============================================================================================
//	Structs
//==============================================================================================

FProjectileArray::FProjectileArray(int32 ReserveSize)
{
	assert(ReserveSize >= 0);

	Array.Reserve(ReserveSize);
}

FProjectileArray::FProjectileArray()
{
	/* Not intended to be called. Crash */
	// Commented because I crash here after migrating to 4.21
	//assert(0);
}

void FProjectileArray::AddToStack(AProjectileBase * NewProjectile)
{
	/* Potentially heavy operation */
	assert(!Array.Contains(NewProjectile));
	UE_CLOG(NewProjectile->IsFitForEnteringObjectPool() == false, RTSLOG, Fatal, 
		TEXT("Projectile [%s] failed fitness test to enter object pool"), *NewProjectile->GetName());

	Array.Push(NewProjectile);
}

AProjectileBase * FProjectileArray::Pop()
{
	assert(Array.Num() > 0);
	AProjectileBase * Projectile = Array.Pop(false);
	assert(Projectile != nullptr);
	return Projectile;
}

AProjectileBase * FProjectileArray::Last() const
{
	AProjectileBase * Projectile = Array.Last();
	assert(Projectile != nullptr);
	return Projectile;
}


//==============================================================================================
//	Object Pooling Manager
//==============================================================================================

AObjectPoolingManager::AObjectPoolingManager()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void AObjectPoolingManager::BeginPlay()
{
	Super::BeginPlay();

	GS = CastChecked<ARTSGameState>(GetWorld()->GetGameState());
}

int32 AObjectPoolingManager::GetNumForPool(TSubclassOf<AProjectileBase> ProjectileBP) const
{
	/* TODO: return values that isn't hardcoded */
#if WITH_EDITOR
	return 2;
#else
	return 32;
#endif
}

int32 AObjectPoolingManager::GetInventoryItemInitialPoolSize_SM(URTSGameInstance * GameInst) const
{
	/* If nothing has an inventory then pool can be 0. Otherwise make it the magic number 8, 
	but can be whatever you like */
	return (GameInst->GetLargestInventoryCapacity() > 0) ? 8 : 0;
}

int32 AObjectPoolingManager::GetInventoryItemInitialPoolSize_SK(URTSGameInstance * GameInst) const
{
	return (GameInst->GetLargestInventoryCapacity() > 0) ? 8 : 0;
}

bool AObjectPoolingManager::IsFitForEnteringPool(AInventoryItem_SM * InventoryItemActor)
{
	return InventoryItemActor->IsFitForEnteringObjectPool();
}

bool AObjectPoolingManager::IsFitForEnteringPool(AInventoryItem_SK * InventoryItemActor)
{
	return InventoryItemActor->IsFitForEnteringObjectPool();
}

void AObjectPoolingManager::Server_FireProjectileAtTarget(AActor * Firer, const TSubclassOf<AProjectileBase> & ProjectileBP, 
	const FBasicDamageInfo & DamageInfo, float AttackRange, ETeam Team, const FVector & MuzzleLoc, AActor * Target, float Roll)
{
	SERVER_CHECK;

	/* Fire no projectile if either shooter or target do not exist. Maybe this check
	isn't needed on server */
	if (!Statics::IsValid(Firer) || !Statics::IsValid(Target))
	{
		return;
	}

	/* Get and remove projectile from pool */
	AProjectileBase * Projectile = GetProjectileFromPoolOrSpawnIfNeeded(ProjectileBP);

	/* Fire projectile. It will handle adding itself to the pool when done */
	Projectile->FireAtTarget(Firer, DamageInfo, AttackRange, Team, MuzzleLoc, Target, Roll);
}

void AObjectPoolingManager::Client_FireProjectileAtTarget(const TSubclassOf<AProjectileBase> & ProjectileBP, 
	const FBasicDamageInfo & DamageInfo, float AttackRange, ETeam Team, const FVector & MuzzleLoc, AActor * Target, float Roll)
{
	CLIENT_CHECK;
	assert(ProjectileBP != nullptr);

	/* Get projectile from pool */
	AProjectileBase * Projectile = GetProjectileFromPoolOrSpawnIfNeeded(ProjectileBP);

	/* Fire projectile. It will handle adding itself to the pool when done.
	Do not need instigator reference since clients cannot deal damage */
	Projectile->FireAtTarget(nullptr, DamageInfo, AttackRange, Team, MuzzleLoc, Target, Roll);
}

void AObjectPoolingManager::Server_FireProjectileAtLocation(AActor * InInstigator, const TSubclassOf<AProjectileBase> & ProjectileBP, 
	const FBasicDamageInfo & DamageInfo, float AttackRange, ETeam Team, const FVector & StartLoc, const FVector & TargetLoc,
	float Roll, AAbilityBase * ListeningAbility, int32 ListeningAbilityUniqueID)
{
	SERVER_CHECK;
	assert(ProjectileBP != nullptr);

	/* Get and remove projectile from pool */
	AProjectileBase * Projectile = GetProjectileFromPoolOrSpawnIfNeeded(ProjectileBP);

	/* Fire projectile. It will handle adding itself to the pool when done */
	Projectile->FireAtLocation(InInstigator, DamageInfo, AttackRange, Team, StartLoc, TargetLoc, Roll,
		ListeningAbility, ListeningAbilityUniqueID);
}

void AObjectPoolingManager::Client_FireProjectileAtLocation(const TSubclassOf<AProjectileBase> & ProjectileBP, 
	const FBasicDamageInfo & DamageInfo, float AttackRange, ETeam Team, const FVector & StartLoc, const FVector & TargetLoc, float Roll,
	AAbilityBase * ListeningAbility, int32 ListeningAbilityUniqueID)
{
	CLIENT_CHECK;

	/* Get and remove projectile from pool */
	AProjectileBase * const Projectile = GetProjectileFromPoolOrSpawnIfNeeded(ProjectileBP);

	/* Fire projectile. It will handle adding itself to the pool when done */
	Projectile->FireAtLocation(nullptr, DamageInfo, AttackRange, Team, StartLoc, TargetLoc, Roll,
		ListeningAbility, ListeningAbilityUniqueID);
}

void AObjectPoolingManager::Server_FireProjectileInDirection(AActor * InInstigator, 
	const TSubclassOf<AProjectileBase>& ProjectileBP, const FBasicDamageInfo & DamageInfo, 
	float AttackRange, ETeam Team, const FVector & StartLoc, const FRotator & Direction,
	AAbilityBase * ListeningAbility, int32 ListeningAbilityUniqueID)
{
	SERVER_CHECK;
	
	AProjectileBase * Projectile = GetProjectileFromPoolOrSpawnIfNeeded(ProjectileBP);
	Projectile->FireInDirection(InInstigator, DamageInfo, AttackRange, Team, StartLoc, Direction, ListeningAbility, ListeningAbilityUniqueID);
}

void AObjectPoolingManager::Client_FireProjectileInDirection(const TSubclassOf<AProjectileBase>& ProjectileBP, 
	const FBasicDamageInfo & DamageInfo, float AttackRange, ETeam Team, const FVector & StartLoc,
	const FRotator & Direction, AAbilityBase * ListeningAbility, int32 ListeningAbilityUniqueID)
{
	CLIENT_CHECK;

	AProjectileBase * Projectile = GetProjectileFromPoolOrSpawnIfNeeded(ProjectileBP);
	Projectile->FireInDirection(nullptr, DamageInfo, AttackRange, Team, StartLoc, Direction, ListeningAbility, ListeningAbilityUniqueID);
}

void AObjectPoolingManager::AddToPool(AProjectileBase * Projectile, TSubclassOf <AProjectileBase> ProjectileBP)
{
	assert(ProjectileBP != nullptr);

	GS->UnregisterFogProjectile(Projectile);

	Pools[ProjectileBP].AddToStack(Projectile);
}

void AObjectPoolingManager::AddProjectileBP(TSubclassOf<AProjectileBase> ProjectileBP)
{
	/* TODO: change this to an assert when all selectable classes are
	returning not null for GetProjectileBP() maybe. Depends on how pool
	size decisions are implemented. */
	/* Do not crash on null BPs which indicate selectable cannot attack */
	if (ProjectileBP != nullptr)
	{
		Projectile_BPs.Emplace(ProjectileBP);
	}
}

void AObjectPoolingManager::CreatePools()
{
	UWorld * World = GetWorld();
	
	//-----------------------------------------------
	//	Projectile pools
	//-----------------------------------------------
	
	for (const auto & BP : Projectile_BPs)
	{
		/* Number to be in pool for this particular blueprint */
		const int32 NumInPool = GetNumForPool(BP);

		Pools.Emplace(BP, FProjectileArray(NumInPool));

		/* Spawn lots of projectiles. They should have collision disabled by default
		otherwise there will be crashes */
		for (int32 i = 0; i < NumInPool; ++i)
		{
			AProjectileBase * Projectile = World->SpawnActor<AProjectileBase>(
				BP, Statics::POOLED_ACTOR_SPAWN_LOCATION, FRotator::ZeroRotator);
			assert(Projectile->PrimaryActorTick.bStartWithTickEnabled == false);
			
			Projectile->SetProjectileBP(BP);
			Projectile->SetPoolingManager(this);

			Pools[BP].AddToStack(Projectile);
		}
	}

	/* Free up unused memory */
	Projectile_BPs.Empty();

	//--------------------------------------------------
	//	Inventory item pools
	//--------------------------------------------------

	URTSGameInstance * GameInst = CastChecked<URTSGameInstance>(World->GetGameInstance());

	InventoryItems_SM.Reserve(GetInventoryItemInitialPoolSize_SM(GameInst));
	for (int32 i = 0; i < GetInventoryItemInitialPoolSize_SM(GameInst); ++i)
	{
		AInventoryItem_SM * Item = World->SpawnActor<AInventoryItem_SM>(Statics::POOLED_ACTOR_SPAWN_LOCATION,
			FRotator::ZeroRotator);

		// Make hidden. CDO is likely visible + do other stuff
		Item->SetupForEnteringObjectPool(GameInst);

		PutInventoryItemInPool(Item);
	}

	InventoryItems_SK.Reserve(GetInventoryItemInitialPoolSize_SK(GameInst));
	for (int32 i = 0; i < GetInventoryItemInitialPoolSize_SK(GameInst); ++i)
	{
		AInventoryItem_SK * Item = World->SpawnActor<AInventoryItem_SK>(Statics::POOLED_ACTOR_SPAWN_LOCATION, 
			FRotator::ZeroRotator);

		Item->SetupForEnteringObjectPool(GameInst);

		PutInventoryItemInPool(Item);
	}

	//--------------------------------------------------
	//	End
	//--------------------------------------------------

	assert(!bHasCreatedPools);
	bHasCreatedPools = true;
}

const AProjectileBase * AObjectPoolingManager::GetProjectileChecked(const TSubclassOf<AProjectileBase> ProjectileBP) const
{
	return Pools[ProjectileBP].Last();
}

AProjectileBase * AObjectPoolingManager::GetProjectileReference(const TSubclassOf<AProjectileBase> & ProjectileBP)
{
	if (Pools.Contains(ProjectileBP) == false)
	{
		Pools.Emplace(ProjectileBP, FProjectileArray(16));
	}
 
	FProjectileArray & PoolsStruct = Pools[ProjectileBP];
	if (PoolsStruct.IsPoolEmpty())
	{
		// Spawn projectile first
		AProjectileBase * Projectile = GetWorld()->SpawnActor<AProjectileBase>(ProjectileBP, 
			Statics::POOLED_ACTOR_SPAWN_LOCATION, FRotator::ZeroRotator);

		Projectile->SetProjectileBP(ProjectileBP);
		Projectile->SetPoolingManager(this);

		PoolsStruct.AddToStack(Projectile);
	}

	return PoolsStruct.Last();
}

AProjectileBase * AObjectPoolingManager::GetProjectileFromPoolOrSpawnIfNeeded(const TSubclassOf<AProjectileBase> & ProjectileBP)
{
	/* Some abilities will need this. Could add a virtual to AAbilityBase and UCommanderAbilityBase 
	like GetProjectileBlueprints(TArray<TSubclassOf<AprojecitleBase>> & OutArray) that gathers 
	all the projectile blueprints they need and call it during setup to avoid this ever being hit */
	if (Pools.Contains(ProjectileBP) == false)
	{
		Pools.Emplace(ProjectileBP, FProjectileArray(16));
	}

	FProjectileArray & Pool = Pools[ProjectileBP];
	if (Pool.IsPoolEmpty())
	{
		// Spawn a projectile
		AProjectileBase * Projectile = GetWorld()->SpawnActor<AProjectileBase>(ProjectileBP,
			Statics::POOLED_ACTOR_SPAWN_LOCATION, FRotator::ZeroRotator);

		Projectile->SetProjectileBP(ProjectileBP);
		Projectile->SetPoolingManager(this);

#if WITH_EDITOR
		// Note down we spawned a projectile
		if (CurrentFrameNumber != GFrameNumber)
		{
			// First spawn of this frame. Check if the previous time a projectile was spawned 
			// was considered the worst frame
			if (NumProjectilesSpawnedThisFrame.Num() > WorstProjectileFrame.Num())
			{
				// Hoping this copies the whole TMap into WorstFrame
				WorstProjectileFrame = NumProjectilesSpawnedThisFrame;
				NumProjectilesSpawnedThisFrame.Reset();

				WorstProjectileFrameNumber = CurrentFrameNumber;
			}

			CurrentFrameNumber = GFrameNumber;

			NumProjectilesSpawnedThisFrame.Emplace(ProjectileBP, 1);
		}
		else // Not the first projectile we have spawned this frame
		{
			if (NumProjectilesSpawnedThisFrame.Contains(ProjectileBP))
			{
				NumProjectilesSpawnedThisFrame[ProjectileBP] += 1;
			}
			else
			{
				NumProjectilesSpawnedThisFrame.Emplace(ProjectileBP, 1);
			}
		}
#endif

		return Projectile;
	}
	else
	{
		return Pool.Pop();
	}
}

#if WITH_EDITOR
void AObjectPoolingManager::NotifyOfSuggestProjectileVelocityFailing(AProjectileBase * Projectile, const FVector & StartLoc, const FVector & TargetLoc)
{
	if (SuggestProjectileVelocityFails.Contains(Projectile->GetClass()) == false)
	{
		SuggestProjectileVelocityFails.Emplace(Projectile->GetClass(), FSuggestProjectileVelocityEntryContainer());
	}

	/* Do not add anymore than 50 entries per projectile class. 50 is enough to conclude 
	the projectile needs tweaking */
	if (SuggestProjectileVelocityFails[Projectile->GetClass()].Array.Num() < FSuggestProjectileVelocityEntryContainer::MAX_CONTAINER_ENTRIES)
	{
		SuggestProjectileVelocityFails[Projectile->GetClass()].Array.Emplace(FSuggestProjectileVelocityEntry(StartLoc, TargetLoc));
	}
}

void AObjectPoolingManager::LogWorstProjectileFrame()
{
	/* 3 may be annoying. Maybe like 10 would be a number people care about */
	if (WorstProjectileFrame.Num() >= 3)
	{
		FString String;
		
		String +=	" \n"
					"------------------------------------------------------------------ \n"
					"		------- Projectile Spawns During Match Summary: ------- \n"
					"------------------------------------------------------------------ \n";
		
		String += "Projectiles spawned during worst frame (frame ";
		String += FString::FromInt(WorstProjectileFrameNumber);
		String += "): \n";

		int32 TotalSpawnsInFrame = 0;
		for (const auto & Pair : WorstProjectileFrame)
		{
			String += Pair.Key->GetName();
			String += ": ";
			String += FString::FromInt(Pair.Value);
			String += "\n";

			TotalSpawnsInFrame += Pair.Value;
		}

		String += "Total projectiles spawned in frame: " + FString::FromInt(TotalSpawnsInFrame);

		UE_LOG(RTSLOG, Warning, TEXT("%s"), *String);
	}
}

void AObjectPoolingManager::LogSuggestProjectileVelocityFails()
{
	if (SuggestProjectileVelocityFails.Num() > 0)
	{
		/* How many classes to log verbosly. The rest get faster less verbose logging */
		const int32 VerboseLoggingLimit = 10;

		FString String;

		String +=	" \n"
					"------------------------------------------------------------------ \n"
					"------- Projectile SuggestProjectileVelocity Fail Summary: ------- \n"
					"------------------------------------------------------------------ \n";

		int32 NumProjectileTypesLogged = 0;
		for (auto & Pair : SuggestProjectileVelocityFails)
		{
			if (NumProjectileTypesLogged < VerboseLoggingLimit)
			{
				Pair.Value.Array.Sort();
				
				String += Pair.Key.Get()->GetName() + ": ";
				String += "number of fails: " + FString::FromInt(Pair.Value.Array.Num());
					
				if (Pair.Value.Array.Num() == FSuggestProjectileVelocityEntryContainer::MAX_CONTAINER_ENTRIES)
				{
					String += "+";
				}
				
				if (Pair.Value.Array.Num() == 1)
				{
					String += FString::Printf(TEXT("  |  Fail distance was: %.1f"), Pair.Value.Array.Last().GetDistance());
				}
				else
				{
					String += FString::Printf(TEXT("  |  Greatest fail distance was: %.1f"), Pair.Value.Array.Last().GetDistance());
					String += FString::Printf(TEXT("  |  Shortest fail distance was: %.1f"), Pair.Value.Array[0].GetDistance());
				}
				
				String += "\n";
			}
			else
			{
				/* After logging 10 classes just show the class + how many times it failed.
				I want to limit the amount of array sorting just cause it could slow down
				how fast PIE exits (cause this is called when PIE exits) */
				
				if (NumProjectileTypesLogged == VerboseLoggingLimit)
				{
					String += " + some other classes: \n";
				}
				
				String += Pair.Key.Get()->GetName() + ": ";
				String += "number of fails: " + (Pair.Value.Array.Num() == FSuggestProjectileVelocityEntryContainer::MAX_CONTAINER_ENTRIES)
					? FString::FromInt(FSuggestProjectileVelocityEntryContainer::MAX_CONTAINER_ENTRIES) + "+"
					: FString::FromInt(Pair.Value.Array.Num());
				String += "\n";
			}

			NumProjectileTypesLogged++;
		}

		String += "To avoid this try increasing the InitialSpeed of the movment comp or reducing "
			"how far the projectiles have to travel. Alternatively you can "
			"set Arc Calculation Method to ChooseArc which (I think) never fails ";

		UE_LOG(RTSLOG, Warning, TEXT("%s"), *String);
	}
}
#endif

void AObjectPoolingManager::PutInventoryItemInPool(AInventoryItem_SM * InventoryItem)
{
	assert(IsFitForEnteringPool(InventoryItem));
	
	InventoryItems_SM.Emplace(InventoryItem);
}

void AObjectPoolingManager::PutInventoryItemInPool(AInventoryItem_SK * InventoryItem)
{
	assert(IsFitForEnteringPool(InventoryItem));

	InventoryItems_SK.Emplace(InventoryItem);
}

AInventoryItem * AObjectPoolingManager::PutItemInWorld(EInventoryItem ItemType, int16 ItemQuantity, 
	int16 ItemNumCharges, const FInventoryItemInfo & ItemsInfo, const FVector & Location, 
	const FRotator & Rotation, EItemEntersWorldReason ReasonForSpawning, AFogOfWarManager * FogManager)
{
	/* May want to change this in the future to use fog manager to decide initial visibility 
	e.g. something like bInitialVisibility = FogManager->IsLocationVisible(Location or something) */
	const bool bInitialVisibility = false;

	/* Check if it's either a static mesh or a skeletal mesh the user wants. If mesh is null 
	we will just default to using the static mesh version */
	if (ItemsInfo.GetMeshInfo().IsMeshSet() && ItemsInfo.GetMeshInfo().IsSkeletalMesh())
	{
		// ----- Skeletal mesh case -----

		AInventoryItem_SK * ItemActor;

		if (InventoryItems_SK.Num() > 0)
		{
			ItemActor = InventoryItems_SK.Pop(false);

			ItemActor->ExitPool(GS, ItemType, ItemQuantity, ItemNumCharges, ItemsInfo, Location, 
				Rotation, ReasonForSpawning, bInitialVisibility);
		}
		else
		{
			UWorld * World = GetWorld();

			// Pool empty. We have to spawn a new actor
			const FTransform SpawnTransform(Rotation + ItemsInfo.GetMeshInfo().GetRotation(),
				Location + ItemsInfo.GetMeshInfo().GetLocation(),
				ItemsInfo.GetMeshInfo().GetScale3D());
			ItemActor = World->SpawnActor<AInventoryItem_SK>(AInventoryItem_SK::StaticClass(), SpawnTransform);

			URTSGameInstance * GameInst = CastChecked<URTSGameInstance>(World->GetGameInstance());
			ItemActor->ExitPool_FreshlySpawnedActor(GameInst, GS, ItemType, ItemQuantity, ItemNumCharges, 
				ItemsInfo, ReasonForSpawning, bInitialVisibility);
		}

		return ItemActor;
	}
	else
	{
		// ----- Static mesh case -----
		
		AInventoryItem_SM * ItemActor;

		if (InventoryItems_SM.Num() > 0)
		{
			ItemActor = InventoryItems_SM.Pop(false);

			ItemActor->ExitPool(GS, ItemType, ItemQuantity, ItemNumCharges, ItemsInfo, Location, 
				Rotation, ReasonForSpawning, bInitialVisibility);
		}
		else
		{
			UWorld * World = GetWorld();

			// Pool empty. We have to spawn a new actor
			const FTransform SpawnTransform(Rotation + ItemsInfo.GetMeshInfo().GetRotation(),
				Location + ItemsInfo.GetMeshInfo().GetLocation(),
				ItemsInfo.GetMeshInfo().GetScale3D());
			ItemActor = World->SpawnActor<AInventoryItem_SM>(AInventoryItem_SM::StaticClass(), SpawnTransform);

			URTSGameInstance * GameInst = CastChecked<URTSGameInstance>(World->GetGameInstance());
			ItemActor->ExitPool_FreshlySpawnedActor(GameInst, GS, ItemType, ItemQuantity, ItemNumCharges, 
				ItemsInfo, ReasonForSpawning, bInitialVisibility);
		}

		return ItemActor;
	}
}
