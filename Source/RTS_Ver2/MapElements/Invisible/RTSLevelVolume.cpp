// Fill out your copyright notice in the Description page of Project Settings.

#include "RTSLevelVolume.h"
#include "Components/BoxComponent.h"
#include "Components/SceneCaptureComponent2D.h" 
#include "Engine/TextureRenderTarget2D.h"
#include "Public/EngineUtils.h"
//#include "Classes/GameMapSettings.h"

#include "MapElements/RTSPlayerStart.h"
#include "Statics/DevelopmentStatics.h"


const float ARTSLevelVolume::WALL_THICKNESS = 200.f;

// Sets default values
ARTSLevelVolume::ARTSLevelVolume()
{
#if WITH_EDITORONLY_DATA

	BoxComp = CreateEditorOnlyDefaultSubobject<UBoxComponent>(TEXT("Level Bounds"));
	if (BoxComp != nullptr)
	{
		SetRootComponent(BoxComp);
		BoxComp->SetMobility(EComponentMobility::Static);
		BoxComp->PrimaryComponentTick.bCanEverTick = false;
		BoxComp->SetGenerateOverlapEvents(false);
		BoxComp->SetCollisionProfileName(FName("NoCollision"));
		BoxComp->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
		BoxComp->SetCanEverAffectNavigation(false);

		BoxComp->SetBoxExtent(FVector(8192.f, 8192.f, 200.f), false);
	}

	MinimapSceneCapture = CreateEditorOnlyDefaultSubobject<USceneCaptureComponent2D>(TEXT("Minimap Scene Capture"));
	if (MinimapSceneCapture != nullptr)
	{
		MinimapSceneCapture->SetMobility(EComponentMobility::Static);
		
		MinimapSceneCapture->SetupAttachment(BoxComp);
		MinimapSceneCapture->PrimaryComponentTick.bCanEverTick = false;
		MinimapSceneCapture->SetCanEverAffectNavigation(false);
		MinimapSceneCapture->bCaptureEveryFrame = false;
		MinimapSceneCapture->ProjectionType = ECameraProjectionMode::Orthographic;
		MinimapSceneCapture->OrthoWidth = CalcSceneCaptureOrthoWidth();
		/* This is an important one. Without it I just get a black line when trying to show
		texture with widget. */
		MinimapSceneCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
		// Put in air
		MinimapSceneCapture->RelativeLocation = FVector(0.f, 0.f, 10000.f);
		// Make comp look downwards. I'm assuming it looks forward by default 
		MinimapSceneCapture->RelativeRotation = FRotator(-90.f, 0.f, 0.f);
	}

#endif // WITH_EDITORONLY_DATA

	/* Possibly might need to make a dummy root component to add these to (every other 
	component I made is actually editor only). 
	Another thing: there is a engine actor ALevelBounds. Could that be a better performing 
	alternative? */
	MapWall_North = CreateDefaultSubobject<UBoxComponent>(TEXT("NorthWall"));
	MapWall_North->SetupAttachment(RootComponent);
	MapWall_East = CreateDefaultSubobject<UBoxComponent>(TEXT("EastWall"));
	MapWall_East->SetupAttachment(RootComponent);
	MapWall_South = CreateDefaultSubobject<UBoxComponent>(TEXT("SouthWall"));
	MapWall_South->SetupAttachment(RootComponent);
	MapWall_West = CreateDefaultSubobject<UBoxComponent>(TEXT("WestWall"));
	MapWall_West->SetupAttachment(RootComponent);
	SetMapWallConstructorValues(MapWall_North);
	SetMapWallConstructorValues(MapWall_East);
	SetMapWallConstructorValues(MapWall_South);
	SetMapWallConstructorValues(MapWall_West);
	MapWall_East->AddRelativeRotation(FRotator(0.f, 0.f, 90.f));
	MapWall_West->AddRelativeRotation(FRotator(0.f, 0.f, 90.f));

	/* This stuff doesn't really matter, actor gets destroyed before match starts anyway. At 
	least that's the plan unless I decide to put the camera blocking bounds on this actor 
	in which case it will not */

	bReplicates = bReplicateMovement = false;

	bCanBeDamaged = false;

	PrimaryActorTick.bCanEverTick = PrimaryActorTick.bStartWithTickEnabled = false;
	SetActorHiddenInGame(true);
}

#if WITH_EDITOR
void ARTSLevelVolume::OnConstruction(const FTransform & Transform)
{
	Super::OnConstruction(Transform);

	MapName = GetWorld()->GetMapName();
}

void ARTSLevelVolume::Destroyed()
{
	Super::Destroyed();
}

void ARTSLevelVolume::SetMapWallConstructorValues(UBoxComponent * InBoxComp)
{
	InBoxComp->SetMobility(EComponentMobility::Static);
	InBoxComp->PrimaryComponentTick.bCanEverTick = false;
	InBoxComp->SetCanEverAffectNavigation(false);
	InBoxComp->SetGenerateOverlapEvents(false);
	InBoxComp->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	InBoxComp->SetCollisionProfileName(FName("MapWall"));

	InBoxComp->bAbsoluteScale = true;

	InBoxComp->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
}

float ARTSLevelVolume::CalcSceneCaptureOrthoWidth() const
{
	/* Use X coord which is good enough since it is assumed that X == Y */
	return BoxComp->CalcBounds(BoxComp->GetComponentTransform()).BoxExtent.X * 2;
}

void ARTSLevelVolume::StoreMapInfo()
{
	CreateMinimapTexture();
	StorePlayerStartInfo();
	StoreMapBounds();
	WriteMapInfoToDisk();
	AdjustWallDimensions();
}
#endif // WITH_EDITOR

UTexture2D * ARTSLevelVolume::GetMinimapTexture()
{
	if (MinimapTexture == nullptr)
	{
		UE_CLOG(MinimapTextureSoftPtr.IsNull(), RTSLOG, Fatal, TEXT("Minimap texture soft ptr "
			"doesn't point to a valid object"));
		
		// Need to load it
		MinimapTexture = MinimapTextureSoftPtr.LoadSynchronous();

		// TODO try and get this working
		//UE_CLOG(MinimapTexture == nullptr, RTSLOG, Fatal, TEXT("Minimap texture still null after "
		//	"trying to load from disk"));
	}
	
	return MinimapTexture;
}

const FBoxSphereBounds & ARTSLevelVolume::GetMapBounds() const
{
	return MapBounds;
}

const TArray<FPlayerStartInfo>& ARTSLevelVolume::GetPlayerStarts() const
{
	return PlayerStarts;
}

#if WITH_EDITORONLY_DATA 
void ARTSLevelVolume::CreateMinimapTexture()
{
	/* Just one problem: if user edits their map appearance the minimap texture will not
	update. Workaround is to just make an edit on this to trigger post edit */

	/* Create one of these if not already created */
	if (MinimapSceneCapture->TextureTarget == nullptr)
	{
		MinimapSceneCapture->TextureTarget = NewObject<UTextureRenderTarget2D>(this);

		/* Got to set these values since they aren't doen by default yet if creating one in
		editor they are. Maybe should just force users to use an editor made one cause maybe
		I've missed a value that would normally be done for an editor created one */
		MinimapSceneCapture->TextureTarget->AddressX = TA_Wrap;
		MinimapSceneCapture->TextureTarget->AddressY = TA_Wrap;
		MinimapSceneCapture->TextureTarget->ClearColor = FLinearColor::Black;
		MinimapSceneCapture->TextureTarget->bAutoGenerateMips = false;
		MinimapSceneCapture->TextureTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA16f;
		MinimapSceneCapture->TextureTarget->TargetGamma = 0.f;
		MinimapSceneCapture->TextureTarget->bGPUSharedFlag = false;
		MinimapSceneCapture->TextureTarget->LODBias = 0;
		MinimapSceneCapture->TextureTarget->MipGenSettings = TMGS_FromTextureGroup;
		MinimapSceneCapture->TextureTarget->bPreserveBorder = false;
		MinimapSceneCapture->TextureTarget->NumCinematicMipLevels = 0;
		MinimapSceneCapture->TextureTarget->LODGroup = TEXTUREGROUP_RenderTarget;
		MinimapSceneCapture->TextureTarget->ResizeTarget(256, 256);
		MinimapSceneCapture->TextureTarget->UpdateResourceImmediate();
	}

	MinimapSceneCapture->CaptureScene();

	/* Create a UTexture2D from the UTextureRenderTarget2D */
	static uint64 Count = 0;
	MinimapTexture = MinimapSceneCapture->TextureTarget->ConstructTexture2D(this,
		MapName + "_Minimap_" + FString::FromInt(Count++), RF_NoFlags);
	assert(MinimapTexture != nullptr);

	// Save soft reference 
	MinimapTextureSoftPtr = MinimapTexture;
}

void ARTSLevelVolume::StorePlayerStartInfo()
{
	/* Added this check for 'no edit GI BP workaround'. If in a editor tab then return */
	if (GetWorld() == nullptr)
	{
		return;
	}
	
	PlayerStarts.Empty();

	uint8 ID = 0;
	for (TActorIterator<ARTSPlayerStart> Iter(GetWorld()); Iter; ++Iter)
	{
		ARTSPlayerStart * PlayerStart = *Iter;

		const uint8 UniqueID = ID++;

		// Assign unique ID. Not sure this is even necessary
		PlayerStart->SetUniqueID(UniqueID);

		PlayerStarts.Emplace(FPlayerStartInfo(PlayerStart));
	}

	/* Now go through the player starts array and add the distance each other player start is 
	from it. This helps game choose a starting spot for players that weren't assigned one 
	either in lobby or in development settings. Doing it now to avoid doing it at runtime */
	for (auto & PlayerStart : PlayerStarts)
	{
		PlayerStart.EmptyNearbyPlayerStarts();
		
		for (const auto & OtherPlayerStart : PlayerStarts)
		{
			if (OtherPlayerStart.GetUniqueID() != PlayerStart.GetUniqueID())
			{
				const float Distance = FPlayerStartDistanceInfo::GetDistanceBetweenSpots(OtherPlayerStart, PlayerStart);
				PlayerStart.AddNearbyPlayerStart(FPlayerStartDistanceInfo(OtherPlayerStart.GetUniqueID(), Distance));
			}
		}

		// Now sort array based on distance
		PlayerStart.SortNearbyPlayerStarts();
	}

	/* Originally tried to modify the game instance blueprint but doesn't work so have chosen
	to write to ini file and have GI read from it. Would like to try this method again though
	to avoid potentially having to load strings from disk on every PIE session */

	// Also need to add PublicDependencyModule EngineSettings in build.cs inside editor conditional
	//#include "Kismet/BlueprintMapLibrary.h"
	//#include "GameFramework/RTSGameInstance.h"

	//// Not sure if GetDefault gets 'current' or 'default' project settings, hoping current
	//const UGameMapsSettings * MapSettings = GetDefault<UGameMapsSettings>();
	//
	//// Get reference to game instance blueprint set in project settings
	//UClass * GameInstanceBlueprint = MapSettings->GameInstanceClass.TryLoadClass<URTSGameInstance>();
	//
	//// Get reference to the TMap UPROPERTY called "MapPool"
	//static const FName PropertyName = FName("MapPool");
	//UProperty * MapInfoProperty = GameInstanceBlueprint->FindPropertyByName(PropertyName);
	//UMapProperty * MapProperty = CastChecked<UMapProperty>(MapInfoProperty);
	//
	//UObject * GameInstanceObj = GameInstanceBlueprint->GetArchetypeForCDO();
	//
	//const void * Address = MapProperty->ContainerPtrToValuePtr<void>(GameInstanceObj);
	////FScriptMapHelper MapHelper = FScriptMapHelper(MapProperty, Address);
	//
	//void * ValuePtr = nullptr;
	//const bool bContainsKey = UBlueprintMapLibrary::GenericMap_Find(Address, MapProperty, 
	//	(void*)(&MapPath), ValuePtr);
	//
	//if (ValuePtr == nullptr)
	//{
	//	// Add key value pair
	//	//UBlueprintMapLibrary::GenericMap_Add(Address, MapProperty, void*)(&MapPath), )
	//}
	//
	//return;
	//
	//TMap <FString, FMapInfo> * TMapPtr = MapInfoProperty->ContainerPtrToValuePtr<TMap<FString, FMapInfo>>(GameInstanceObj);
	//
	//if (!TMapPtr->Contains(MapPath))
	//{
	//	/* Player has not added an entry for the map so create one */
	//	TMapPtr->Emplace(MapPath, FMapInfo());
	//}
	//
	//FMapInfo & ValueEntry = (*TMapPtr)[MapPath];
	//
	///* Emtpy the player start array */
	//ValueEntry.EmptyPlayerStartPoints();
	//
	//// Add entry for each player start on map
	//for (TActorIterator<APlayerStart> Iter(GetWorld()); Iter; ++Iter)
	//{
	//	APlayerStart * PlayerStart = *Iter;
	//
	//	ValueEntry.AddPlayerStartPoint(PlayerStart->GetActorTransform());
	//}
}

void ARTSLevelVolume::StoreMapBounds()
{
	MapBounds = BoxComp->CalcBounds(BoxComp->GetComponentTransform());
}

void ARTSLevelVolume::WriteMapInfoToDisk()
{
}

void ARTSLevelVolume::AdjustWallDimensions()
{
	// Set their locations
	MapWall_North->SetRelativeLocation(FVector(MapBounds.BoxExtent.X + WALL_THICKNESS, 0.f, 0.f));
	MapWall_East->SetRelativeLocation(FVector(0.f, MapBounds.BoxExtent.Y + WALL_THICKNESS, 0.f));
	MapWall_South->SetRelativeLocation(FVector(-MapBounds.BoxExtent.X - WALL_THICKNESS, 0.f, 0.f));
	MapWall_West->SetRelativeLocation(FVector(0.f, -MapBounds.BoxExtent.Y - WALL_THICKNESS, 0.f));

	const float RootScaledX = BoxComp->GetScaledBoxExtent().X;
	const float RootScaledY = BoxComp->GetScaledBoxExtent().Y;

	// Make them the correct length. X = wall height, Y = length, Z = wall thickness
	MapWall_North->SetBoxExtent(FVector(4000.f, RootScaledX + WALL_THICKNESS, WALL_THICKNESS));
	MapWall_East->SetBoxExtent(FVector(4000.f, RootScaledY + WALL_THICKNESS, WALL_THICKNESS));
	MapWall_South->SetBoxExtent(FVector(4000.f, RootScaledX + WALL_THICKNESS, WALL_THICKNESS));
	MapWall_West->SetBoxExtent(FVector(4000.f, RootScaledY + WALL_THICKNESS, WALL_THICKNESS));
}

void ARTSLevelVolume::PostEditChangeProperty(FPropertyChangedEvent & PropertyChangedEvent)
{
	MinimapSceneCapture->OrthoWidth = CalcSceneCaptureOrthoWidth();

	StoreMapInfo();

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void ARTSLevelVolume::PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent)
{
	MinimapSceneCapture->OrthoWidth = CalcSceneCaptureOrthoWidth();

	StoreMapInfo();

	Super::PostEditChangeChainProperty(PropertyChangedEvent);
}

void ARTSLevelVolume::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);

	if (bFinished)
	{
		MinimapSceneCapture->OrthoWidth = CalcSceneCaptureOrthoWidth();

		StoreMapInfo();
	}
}
#endif // WITH_EDITORONLY_DATA
