// Fill out your copyright notice in the Description page of Project Settings.

#include "InventoryItem.h"
#include "Components/DecalComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Animation/AnimMontage.h"

#include "Statics/DevelopmentStatics.h"
#include "Statics/Statics.h"
#include "GameFramework/RTSPlayerController.h"
#include "GameFramework/RTSGameInstance.h"
#include "GameFramework/FactionInfo.h"
#include "GameFramework/RTSGameState.h"
#include "UI/RTSHUD.h"


AInventoryItem::AInventoryItem()
{
	PrimaryActorTick.bCanEverTick = false; 
	PrimaryActorTick.bStartWithTickEnabled = false;

	/* Side note: something I learning about decal components: if the decal material is null 
	it still renders something */
	SelectionDecalComp = CreateDefaultSubobject<UDecalComponent>(TEXT("Selection Decal"));
	SelectionDecalComp->bVisible = false;
	SelectionDecalComp->bAbsoluteRotation = true;
	SelectionDecalComp->bDestroyOwnerAfterFade = false;
	SelectionDecalComp->RelativeRotation = FRotator(-90.f, 0.f, 0.f);
	SelectionDecalComp->PrimaryComponentTick.bCanEverTick = false;
	SelectionDecalComp->PrimaryComponentTick.bStartWithTickEnabled = false;
	SelectionDecalComp->RelativeScale3D = FVector(0.1f, 0.3f, 0.3f);
	// SelectionDecalComp collision? Or by default it's already off?

	SelectionParticlesComp = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("Selection Particles"));
	SelectionParticlesComp->bAutoActivate = false;
	SelectionParticlesComp->bAbsoluteRotation = true;
	SelectionParticlesComp->SetCollisionProfileName(FName("NoCollision"));
	SelectionParticlesComp->SetGenerateOverlapEvents(false);
	SelectionParticlesComp->SetCanEverAffectNavigation(false);
	SelectionParticlesComp->CanCharacterStepUpOn = ECB_No;
	SelectionParticlesComp->bReceivesDecals = false;
	SelectionParticlesComp->PrimaryComponentTick.bCanEverTick = true;
	SelectionParticlesComp->PrimaryComponentTick.bStartWithTickEnabled = false;

	bReplicates = false;

	// Populate AActor::Tags. Users can edit these in editor which we don't want so perhaps defer 
	// doing this until BeginPlay
	assert(Tags.Num() == 0);
	Tags.Reserve(Statics::NUM_ACTOR_TAGS);
	Tags.Emplace(Statics::NeutralID);
	Tags.Emplace(Statics::NEUTRAL_TEAM_TAG);
	Tags.Emplace(Statics::UNTARGETABLE_TAG);
	Tags.Emplace(Statics::NotAirTag);
	Tags.Emplace(Statics::InventoryItemTag);
	Tags.Emplace(Statics::NotHasAttackTag);
	Tags.Emplace(Statics::HasZeroHealthTag);
	Tags.Emplace(Statics::NotHasInventoryTag);
	assert(Tags.Num() == Statics::NUM_ACTOR_TAGS);

	Attributes.SetupSelectionInfo(Statics::NeutralID, ETeam::Neutral);
	Attributes.SetupBasicTypeInfo(ESelectableType::InventoryItem, EBuildingType::NotBuilding, EUnitType::NotUnit);
	Type = EInventoryItem::None;
	Quantity = 1;
	AcceptanceRadius = 250.f;
}

void AInventoryItem::BeginPlay()
{
	/*--------------------------------------------------------------------------- 
	3 possible times this code is executed:
	- item already placed on map at start of match
	- item being spawned for the initial object pool quota at start of match
	- item being spawned during match because pool is empty 
	-----------------------------------------------------------------------------*/

#if !UE_BUILD_SHIPPING

	/* Previously this code was in PostLoad, but PostLoad is also called when the editor
	starts up and therefore this warning can show then even though it isn't really relevant, 
	so it has been moved to here */
	
	/* Check if this is the default class (i.e. non-blueprint class). If it's the default 
	class then we are spawning this actor for pools */
	if (IsBlueprintClass())
	{
		if (Type == EInventoryItem::None)
		{
			UE_LOG(RTSLOG, Warning, TEXT("Inventory item %s placed on map does not have its type set. "
				"For this session it will be destroyed. In future either set its type or remove it "
				"from the map."), *GetClass()->GetName());

			Destroy();

			return;
		}
	}
	
#endif

	Super::BeginPlay();
}

void AInventoryItem::Setup()
{
}

void AInventoryItem::SetupStuff(const URTSGameInstance * GameInstance, ARTSGameState * GameState,
	bool bMakeHidden)
{
	// Overridden by child classes
}

void AInventoryItem::SetupSomeStuff(const URTSGameInstance * GameInstance)
{
	UWorld * World = GetWorld();
	ARTSPlayerController * LocalPlayCon = CastChecked<ARTSPlayerController>(World->GetFirstPlayerController());
	AFactionInfo * LocalPlayersFactionInfo = LocalPlayCon->GetFactionInfo();

	// Set affiliation. Will either be Neutral or Observed
	if (LocalPlayCon->IsObserver())
	{
		Attributes.SetAffiliation(EAffiliation::Observed);
	}
	else
	{
		Attributes.SetAffiliation(EAffiliation::Neutral);
	}

	SetupSelectionDecal(LocalPlayersFactionInfo, GameInstance);
	SetupParticles(LocalPlayersFactionInfo);
}

void AInventoryItem::SetupSelectionDecal(AFactionInfo * LocalPlayersFactionInfo, const URTSGameInstance * GameInst)
{
	if (Attributes.GetAffiliation() == EAffiliation::Observed)
	{
		SelectionDecalInfo = &GameInst->GetObserverSelectionDecal();
	}
	else // Assumed Neutral
	{
		assert(Attributes.GetAffiliation() == EAffiliation::Neutral);

		SelectionDecalInfo = &LocalPlayersFactionInfo->GetSelectionDecalInfo(Attributes.GetAffiliation());
	}

	/* Easy TODO: CreateDynamicMaterialInstance call also does SetDecalMaterial, so we do 
	it twice which is unneccessary. Just move this into the UsesNonDynamicMaterial else statement. 
	Do this for all other selectables too (Ainfantry, ABuilding, etc). Actually it requires a 
	bit more than that because the if statement right after this will instead need to check 
	if SelectionDecalInfo->GetDecal() != nullptr */
	SelectionDecalComp->SetDecalMaterial(SelectionDecalInfo->GetDecal());

	if (SelectionDecalComp->GetDecalMaterial() != nullptr)
	{
		/* Create a material instance dynamic, but only if we have to */
		if (SelectionDecalInfo->RequiresCreatingMaterialInstanceDynamic())
		{
			Attributes.SetSelectionDecalMaterial(SelectionDecalComp->CreateDynamicMaterialInstance());

			Attributes.SetSelectionDecalSetup(ESelectionDecalSetup::UsesDynamicMaterial);
		}
		else
		{
			Attributes.SetSelectionDecalSetup(ESelectionDecalSetup::UsesNonDynamicMaterial);
		}
	}
	else
	{
		Attributes.SetSelectionDecalSetup(ESelectionDecalSetup::DoesNotUse);
	}
}

void AInventoryItem::SetupParticles(AFactionInfo * LocalPlayersFactionInfo)
{
	SelectionParticlesTemplate = LocalPlayersFactionInfo->GetSelectionParticles(GetAttributesBase().GetAffiliation(),
		GetAttributesBase().GetParticleSize());

	RightClickParticlesTemplate = LocalPlayersFactionInfo->GetRightClickParticles(GetAttributesBase().GetAffiliation(),
		GetAttributesBase().GetParticleSize());
}

void AInventoryItem::ExitPool_BasicStuff(ARTSGameState * GameState, EInventoryItem InItemType, 
	int16 InQuantity, int16 InNumItemCharges, const FInventoryItemInfo & InItemsInfo)
{
	Type = InItemType;
	Quantity = InQuantity;
	NumItemCharges = InNumItemCharges;
	UniqueID = GameState->GenerateInventoryItemUniqueID(this);
	ItemInfo = &InItemsInfo;

	Attributes.SetName(ItemInfo->GetDisplayName());
	
	/* So creating a copy of FSlateBrush here, struct isn't TOO big. Do it 3 times */
	Attributes.SetHUDImage_Normal(ItemInfo->GetDisplayImage());
	Attributes.SetHUDImage_Hovered(ItemInfo->GetDisplayImage_Hovered());
	Attributes.SetHUDImage_Pressed(ItemInfo->GetDisplayImage_Pressed());
}

void AInventoryItem::ExitPool_FinalStuff(ARTSGameState * GameState)
{
	/* Basically what we are doing here is disabling the particles from emitting and destroying
	them outright. We do this because when we enable tick at the next line if the particles were
	in the middle of emitting when the actor was placed in object pool then they will just continue
	where they left off which we don't want. There may be a better func in UParticleSystemComponent
	for this but I have not found one (I haven't searched that hard though).
	This could alternatively be done when the actor is added to the object pool instead of now */
	SelectionParticlesComp->ResetParticles(false);

	// Enable tick on particle system component
	SelectionParticlesComp->SetComponentTickEnabled(true);

	GameState->PutInventoryItemActorInArray(this);
}

bool AInventoryItem::IsBlueprintClass() const
{
	/* I do not know if this is correct. If it's not then you can always fallback to something 
	like return GetClass() != AInventoryItem_SM::StaticClass() && GetClass() != AInventoryItem_SK::StaticClass(); */
	return HasAnyFlags(RF_ArchetypeObject);
}

void AInventoryItem::SetVis(bool bIsVisible, bool bAffectsSelectionDecal)
{
	// Would really like to call SetActorHiddenInGame here instead. In fact can avoid the 
	// virtual call and just expand AActor::SetActorHiddenInGame here instead. 
	// And also can make this func non-virtual too after
	
	SelectionParticlesComp->SetVisibility(bIsVisible, false);

	if (bAffectsSelectionDecal)
	{
		SelectionDecalComp->SetVisibility(bIsVisible, false);
	}
}

FInventoryItemID AInventoryItem::GetUniqueID() const
{
	return UniqueID;
}

EInventoryItem AInventoryItem::GetType() const
{
	return Type;
}

int16 AInventoryItem::GetItemQuantity() const
{ 
	return Quantity;
}

int16 AInventoryItem::GetNumItemCharges() const
{
	return NumItemCharges;
}

const FInventoryItemInfo * AInventoryItem::GetItemInfo() const
{
	return ItemInfo;
}

bool AInventoryItem::IsInObjectPool() const
{
	return (Quantity == -1);
}

void AInventoryItem::SetupForEnteringObjectPool(const URTSGameInstance * GameInstance)
{
	SetupSomeStuff(GameInstance);
	
	SetVis(false, true);
	Quantity = -1;
}

void AInventoryItem::OnPickedUp(ISelectable * SelectableThatPickedUsUp, ARTSGameState * GameState,
	ARTSPlayerController * LocalPlayCon)
{
	LocalPlayCon->NotifyOfInventoryItemBeingPickedUp(this, Attributes.IsSelected());
	
	Attributes.SetIsSelected(false);
	Attributes.SetIsPrimarySelected(false);
	
	/* Set up for putting back into object pool */
	SetVis(false, true);
	SelectionParticlesComp->SetComponentTickEnabled(false);
	Quantity = -1; // -1 is code for "IsInPool". We query this at times to know whether its in pool or not
}

#if !UE_BUILD_SHIPPING
bool AInventoryItem::IsFitForEnteringObjectPool() const
{
	/* Some of this stuff I never toggle but is good to check in case users change the default 
	values */
	
	const FString MessageStart = TEXT("AInventoryItem::IsFitForEnteringObjectPool() failed because ");

	if (PrimaryActorTick.IsTickFunctionEnabled() == true)
	{
		UE_LOG(RTSLOG, Fatal, TEXT("%s actor tick is enabled"), *MessageStart);
		return false;
	}
	if (SelectionParticlesComp->IsComponentTickEnabled() == true)
	{
		UE_LOG(RTSLOG, Fatal, TEXT("%s SelectionParticlesComp tick is enabled"), *MessageStart);
		return false;
	}
	if (SelectionDecalComp->IsComponentTickEnabled() == true)
	{
		UE_LOG(RTSLOG, Fatal, TEXT("%s SelectionDecalComp tick is enabled"), *MessageStart);
		return false;
	}
	if (SelectionDecalComp->IsVisible() == true)
	{
		UE_LOG(RTSLOG, Fatal, TEXT("%s SelectionDecalComp is visible"), *MessageStart);
		return false;
	}
	if (SelectionParticlesComp->IsVisible() == true)
	{
		UE_LOG(RTSLOG, Fatal, TEXT("%s SelectionParticlesComp is visible"), *MessageStart);
		return false;
	}
	if (Quantity != -1)
	{
		UE_LOG(RTSLOG, Fatal, TEXT("%s Quantity is not set to -1"), *MessageStart);
		return false;
	}
	
	return true;
}
#endif

void AInventoryItem::SetVisibilityFromFogManager(bool bInIsVisible)
{
	// Do quick check to see if vis has changed (for performance)
	if (SelectionParticlesComp->bVisible != bInIsVisible)
	{
		SetVis(bInIsVisible);
	}
}

const FSelectableAttributesBase & AInventoryItem::GetAttributesBase() const
{
	return Attributes;
}

FSelectableAttributesBase & AInventoryItem::GetAttributesBase()
{
	return Attributes;
}

const FSelectableAttributesBasic * AInventoryItem::GetAttributes() const
{
	return nullptr;
}

void AInventoryItem::OnMouseHover(ARTSPlayerController * LocalPlayCon)
{
#if INSTANT_SHOWING_SELECTABLE_TOOLTIPS
	// Show tooltip widget
	LocalPlayCon->GetHUDWidget()->ShowTooltipWidget_InventoryItemInWorld(this);
#endif

	if (Attributes.IsSelected())
	{
		return;
	}

	if (Attributes.GetSelectionDecalSetup() == ESelectionDecalSetup::UsesDynamicMaterial)
	{
		Attributes.GetSelectionDecalMID()->SetScalarParameterValue(SelectionDecalInfo->GetParamName(),
			SelectionDecalInfo->GetMouseoverParamValue());

		SelectionDecalComp->SetVisibility(true);
	}
	else if (Attributes.GetSelectionDecalSetup() == ESelectionDecalSetup::UsesNonDynamicMaterial)
	{
		SelectionDecalComp->SetVisibility(true);
	}
}

void AInventoryItem::OnMouseUnhover(ARTSPlayerController * LocalPlayCon)
{
	// Hide tooltip widget
	LocalPlayCon->GetHUDWidget()->HideTooltipWidget_InventoryItemInWorld(this);

	/* Do nothing if selected */
	if (Attributes.IsSelected())
	{
		return;
	}

	/* Not sure how beneficial it is to do this check first, if any */
	if (Attributes.GetSelectionDecalSetup() == ESelectionDecalSetup::UsesNonDynamicMaterial
		|| Attributes.GetSelectionDecalSetup() == ESelectionDecalSetup::UsesDynamicMaterial)
	{
		SelectionDecalComp->SetVisibility(false);
	}
}

void AInventoryItem::OnSingleSelect()
{
	Attributes.SetIsSelected(true);
	Attributes.SetIsPrimarySelected(false);

	// Show selection decal
	if (Attributes.GetSelectionDecalSetup() == ESelectionDecalSetup::UsesDynamicMaterial)
	{
		Attributes.GetSelectionDecalMID()->SetScalarParameterValue(SelectionDecalInfo->GetParamName(),
			SelectionDecalInfo->GetOriginalParamValue());

		SelectionDecalComp->SetVisibility(true);
	}
	else if (Attributes.GetSelectionDecalSetup() == ESelectionDecalSetup::UsesNonDynamicMaterial)
	{
		SelectionDecalComp->SetVisibility(true);
	}

	//-------------------------------------------------
	// Play selection particles
	//-------------------------------------------------

	/* Here I've copied AInfantry code and I check if the template is different */
	if (SelectionParticlesComp->Template != SelectionParticlesTemplate)
	{
		SelectionParticlesComp->SetTemplate(SelectionParticlesTemplate);
	}

	// Reset particles to play from start
	SelectionParticlesComp->ResetParticles(true);
	SelectionParticlesComp->ActivateSystem(false);
}

uint8 AInventoryItem::OnDeselect()
{
	Attributes.SetIsSelected(false);
	Attributes.SetIsPrimarySelected(false);

	// Hide selection decal
	if (Attributes.GetSelectionDecalSetup() == ESelectionDecalSetup::UsesDynamicMaterial 
		|| Attributes.GetSelectionDecalSetup() == ESelectionDecalSetup::UsesNonDynamicMaterial)
	{
		SelectionDecalComp->SetVisibility(false);
	}
	
	// Stop selection particles
	SelectionParticlesComp->ResetParticles(true);

	return 0;
}

bool AInventoryItem::Security_CanBeClickedOn(ETeam ClickersTeam, const FName & ClickersTeamTag, ARTSGameState * GameState) const
{
	return true;
}

void AInventoryItem::OnRightClick()
{
	//-----------------------------------------------------------
	//	Show right click particles
	//-----------------------------------------------------------
	
	if (SelectionParticlesComp->Template != RightClickParticlesTemplate)
	{
		SelectionParticlesComp->SetTemplate(RightClickParticlesTemplate);
	}

	// Reset particles to play from start
	SelectionParticlesComp->ResetParticles(true);
	SelectionParticlesComp->ActivateSystem(false);
}

void AInventoryItem::ShowTooltip(URTSHUD * HUDWidget) const
{
	HUDWidget->ShowTooltipWidget_InventoryItemInWorld(this);
}

bool AInventoryItem::HasAttack() const
{
	return false;
}

bool AInventoryItem::CanClassGainExperience() const
{
	return false;
}

const FShopInfo * AInventoryItem::GetShopAttributes() const
{
	return nullptr;
}

const FInventory * AInventoryItem::GetInventory() const
{
	return nullptr;
}

#if WITH_EDITOR
bool AInventoryItem::PIE_IsForCPUPlayer() const
{
	return false;
}

int32 AInventoryItem::PIE_GetHumanOwnerIndex() const
{
	return -2;
}
#endif // WITH_EDITOR

float AInventoryItem::TakeDamage(float DamageAmount, FDamageEvent const & DamageEvent, AController * EventInstigator, AActor * DamageCauser)
{
	return 0.f;
}

#if WITH_EDITOR
void AInventoryItem::PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);
	
	// GetWorld() and GetGameInstance() both return null. Just an FYI about post edit
}
#endif // WITH_EDITOR


//=============================================================================================
//	Static Mesh Version
//=============================================================================================

AInventoryItem_SM::AInventoryItem_SM()
{
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionProfileName(FName("InventoryItem"));
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->CanCharacterStepUpOn = ECB_No;
	Mesh->SetCanEverAffectNavigation(false);
	Mesh->bReceivesDecals = false;
	// Trying if turning tick off is ok
	Mesh->PrimaryComponentTick.bCanEverTick = false;
	Mesh->PrimaryComponentTick.bStartWithTickEnabled = false;

	/* We'll make the mesh a cube by default for no reason whatsoever */
#if WITH_EDITOR // Why am I wrapping this in WITH_EDITOR? Because I'm worried a potentially 
	// costly operation like finding this cube object could run in a packaged game, but 
	// I'm pretty sure that won't happen. WITH_EDITOR is pretty much implied in ctor I think.
	auto CubeMesh = ConstructorHelpers::FObjectFinder<UStaticMesh>(TEXT("StaticMesh'/Engine/BasicShapes/Cube.Cube'"));
	if (CubeMesh.Object != nullptr)
	{
		Mesh->SetStaticMesh(CubeMesh.Object);
	}
#endif // WITH_EDITOR

	// Attach the parent's components to the mesh
	SelectionDecalComp->SetupAttachment(Mesh);
	SelectionParticlesComp->SetupAttachment(Mesh); 
}

void AInventoryItem_SM::SetupStuff(const URTSGameInstance * GameInstance, ARTSGameState * GameState, bool bMakeHidden)
{
	UE_CLOG(Type == EInventoryItem::None, RTSLOG, Fatal, TEXT("Inventory item actor [%s] has type "
		"\"None\". Set its type to something else"), *GetClass()->GetName());

	ItemInfo = GameInstance->GetInventoryItemInfo(Type);
	NumItemCharges = ItemInfo->GetNumCharges();

#if !UE_BUILD_SHIPPING
	/* Check that the mesh is a static mesh. If it isn't then the user has assigned the wrong
	item type to this item actor. There are two choices:
	1. We just destroy this actor and log a message
	2. We destroy this actor, log a message and then respawn a skeletal mesh version of
	this actor.

	Option 2 possibly could work but just to be safe I'm not going to spawn an actor. */
	if (ItemInfo->GetMeshInfo().IsMeshSet() && ItemInfo->GetMeshInfo().IsStaticMesh() == false)
	{
		Destroy();

		UE_LOG(RTSLOG, Warning, TEXT("Inventory item actor %s has item type \"%s\". This uses a "
			"skeletal mesh however you have created a static mesh actor for it. Change the actor "
			"to inherit from AInventoryItem_SK instead of AInventoryItem_SM. Actor has been destroyed "
			"for this session"), *GetClass()->GetName(), TO_STRING(EInventoryItem, Type));

		return;
	}
#endif
	
	/* Could pretty much just put this in base class and call Super */
	
	SetupSomeStuff(GameInstance);

	UniqueID = GameState->GenerateInventoryItemUniqueID(this);

	/* Using ItemInfo set some of the values in our attribute struct. The HUD will use them at
	some point */
	Attributes.SetName(ItemInfo->GetDisplayName());
	
	/* So creating a copy of FSlateBrush here, struct isn't TOO big */
	Attributes.SetHUDImage_Normal(ItemInfo->GetDisplayImage());
	Attributes.SetHUDImage_Hovered(ItemInfo->GetDisplayImage_Hovered());
	Attributes.SetHUDImage_Pressed(ItemInfo->GetDisplayImage_Pressed());

	/* Set mesh appearance */
	if (ItemInfo->GetMeshInfo().IsMeshSet())
	{
		Mesh->SetStaticMesh(ItemInfo->GetMeshInfo().GetMeshAsStaticMesh());

		// Think this is the right func, but have not tested thoroughly
		AddActorLocalTransform(FTransform(ItemInfo->GetMeshInfo().GetRotation(),
			ItemInfo->GetMeshInfo().GetLocation(), ItemInfo->GetMeshInfo().GetScale3D()));
	}

	if (bMakeHidden)
	{
		SetVis(false);
	}

	SelectionParticlesComp->SetComponentTickEnabled(true);

	GameState->PutInventoryItemActorInArray(this);

	/* Now could be a time to trace against the ground to make sure item starts on the ground,
	or do we not really care - let users place items however they want? Yeah let them place
	them how they want */
}

void AInventoryItem_SM::SetupForEnteringObjectPool(const URTSGameInstance * GameInstance)
{
	Super::SetupForEnteringObjectPool(GameInstance);

	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AInventoryItem_SM::SetVis(bool bIsVisible, bool bAffectsSelectionDecal)
{
	Super::SetVis(bIsVisible, bAffectsSelectionDecal);

	Mesh->SetVisibility(bIsVisible, false);
}

void AInventoryItem_SM::ExitPool(ARTSGameState * GameState, EInventoryItem InItemType, int16 InQuantity,
	int16 InNumItemCharges, const FInventoryItemInfo & InItemsInfo, const FVector & Location, 
	const FRotator & Rotation, EItemEntersWorldReason ReasonForSpawning, bool bMakeVisible)
{
	ExitPool_BasicStuff(GameState, InItemType, InQuantity, InNumItemCharges, InItemsInfo);

#if UE_BUILD_SHIPPING
	// Set mesh
	Mesh->SetStaticMesh(ItemInfo->GetMeshInfo().GetMeshAsStaticMesh());

	// Set location/rotation/scale
	const FVector FinalLocation = Location + ItemInfo->GetMeshInfo().GetTransform().GetLocation();
	const FRotator FinalRotation = Rotation + ItemInfo->GetMeshInfo().GetTransform().GetRotation().Rotator();
	const FVector FinalScale = ItemInfo->GetMeshInfo().GetTransform().GetScale3D();
	SetActorTransform(FTransform(FinalRotation, FinalLocation, FinalScale));
#else

	if (ItemInfo->GetMeshInfo().IsMeshSet() == false)
	{
		/* No mesh set. Use engine cube as mesh */
		UStaticMesh * EngineCubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("StaticMesh'/Engine/BasicShapes/Cube.Cube'"));
		Mesh->SetStaticMesh(EngineCubeMesh);
	}
	else
	{
		Mesh->SetStaticMesh(ItemInfo->GetMeshInfo().GetMeshAsStaticMesh());

		// Set location/rotation/scale
		const FVector FinalLocation = Location + ItemInfo->GetMeshInfo().GetLocation();
		const FRotator FinalRotation = Rotation + ItemInfo->GetMeshInfo().GetRotation();
		const FVector FinalScale = ItemInfo->GetMeshInfo().GetScale3D();
		SetActorTransform(FTransform(FinalRotation, FinalLocation, FinalScale));
	}

#endif // !UE_BUILD_SHIPPING

	// Re-enable collision so item responds to player's mouse
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	// Set visibility. Because we're leaving pool we know we're already hidden
	if (bMakeVisible)
	{
		SetVis(bMakeVisible);
	}

	ExitPool_FinalStuff(GameState);
}

void AInventoryItem_SM::ExitPool_FreshlySpawnedActor(URTSGameInstance * GameInst, ARTSGameState * GameState, 
	EInventoryItem InItemType, int16 InQuantity, int16 InNumItemCharges, const FInventoryItemInfo & InItemsInfo, 
	EItemEntersWorldReason ReasonForSpawning, bool bMakeHidden)
{
	SetupSomeStuff(GameInst);

	// Mostly copy of other overload, just changing location/rotation setting values
	// plus the vis setting logic is slightly different 

	ExitPool_BasicStuff(GameState, InItemType, InQuantity, InNumItemCharges, InItemsInfo);

#if UE_BUILD_SHIPPING
	// Set mesh
	Mesh->SetStaticMesh(ItemInfo->GetMeshInfo().GetMeshAsStaticMesh());

	/* Set location/rotation. Commented because we now set
	it in AObjectPoolingManager::PutItemInWorld for SpawnActor call */
	//AddActorLocalTransform(ItemInfo->GetMeshInfo().GetTransform());
#else
	if (ItemInfo->GetMeshInfo().IsMeshSet() == false)
	{
		/* No mesh set. Use engine cube as mesh */
		UStaticMesh * EngineCubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("StaticMesh'/Engine/BasicShapes/Cube.Cube'"));
		Mesh->SetStaticMesh(EngineCubeMesh);
	}
	else
	{
		Mesh->SetStaticMesh(ItemInfo->GetMeshInfo().GetMeshAsStaticMesh());

		/* Set location/rotation. Have not tested whether this does what I want. Commented because we now set
		it in AObjectPoolingManager::PutItemInWorld for SpawnActor call */
		//AddActorLocalTransform(ItemInfo->GetMeshInfo().GetTransform());
	}
#endif // !UE_BUILD_SHIPPING

	// Re-enable collision so item responds to player's mouse
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	if (!bMakeHidden)
	{
		SetVis(!bMakeHidden);
	}

	ExitPool_FinalStuff(GameState);
}

void AInventoryItem_SM::OnPickedUp(ISelectable * SelectableThatPickedUsUp, ARTSGameState * GameState, 
	ARTSPlayerController * LocalPlayCon)
{
	/* Set up mesh for putting back into object pool */
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Super::OnPickedUp(SelectableThatPickedUsUp, GameState, LocalPlayCon);

	// Put in object pool
	GameState->PutInventoryItemInObjectPool(this);
}

#if !UE_BUILD_SHIPPING
bool AInventoryItem_SM::IsFitForEnteringObjectPool() const
{
	if (Mesh->GetCollisionEnabled() != ECollisionEnabled::NoCollision)
	{
		UE_LOG(RTSLOG, Fatal, TEXT("AInventoryItem::IsFitForEnteringObjectPool() failed because mesh collision is enabled"));
		return false;
	}
	if (Mesh->IsVisible() == true)
	{
		UE_LOG(RTSLOG, Fatal, TEXT("AInventoryItem::IsFitForEnteringObjectPool() failed because mesh is visible"));
		return false;
	}
	if (Mesh->IsComponentTickEnabled() == true)
	{
		UE_LOG(RTSLOG, Fatal, TEXT("AInventoryItem::IsFitForEnteringObjectPool() failed because mesh tick is enabled"));
		return false;
	}

	return Super::IsFitForEnteringObjectPool();
}
#endif


//=============================================================================================
//	Skeletal Mesh Version
//=============================================================================================

AInventoryItem_SK::AInventoryItem_SK()
{
	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionProfileName(FName("InventoryItem"));
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->CanCharacterStepUpOn = ECB_No;
	Mesh->SetCanEverAffectNavigation(false);
	Mesh->bReceivesDecals = false;
	/* Just some notes about tick: 
	The component's tick does indeed need to be on for animations to play. But there appears 
	to be some kind of bug with setting bStartWithTickEnabled to false in this ctor. From my 
	testing it just doesn't seem to work, but disabling tick during BeginPlay does. 
	See https://answers.unrealengine.com/questions/601078/cannot-disable-actorcomponent-ticking.html 
	Setting bCanEverTick does seem to work in the ctor though */
	Mesh->PrimaryComponentTick.bCanEverTick = true;
	Mesh->PrimaryComponentTick.bStartWithTickEnabled = true;

	// Attach the parent's components to the mesh
	SelectionDecalComp->SetupAttachment(Mesh);
	SelectionParticlesComp->SetupAttachment(Mesh);
}

void AInventoryItem_SK::SetupStuff(const URTSGameInstance * GameInstance, ARTSGameState * GameState, bool bMakeHidden)
{
	UE_CLOG(Type == EInventoryItem::None, RTSLOG, Fatal, TEXT("Inventory item actor [%s] has type "
		"\"None\". Set its type to something else"), *GetClass()->GetName());

	ItemInfo = GameInstance->GetInventoryItemInfo(Type);
	NumItemCharges = ItemInfo->GetNumCharges();

#if !UE_BUILD_SHIPPING
	/* Check if skeletal mesh is not null. We don't do this check for the static mesh version of 
	this actor because we can use a cube if it's null. But having invisible items in the world 
	just seems wrong so if no mesh is set we destroy this actor */
	if (ItemInfo->GetMeshInfo().IsMeshSet() == false)
	{
		Destroy();

		/* Kind of doubling up on warnings with this one since GI will show a warning about 
		mesh being null during InitInventoryItemInfo */
		//UE_LOG(RTSLOG, Warning, TEXT("Inventory item actor %s has item type \"%s\". This item type "
		//	"does not have a mesh set for it in the GI blueprint. Therefore it has been destroyed"),
		//	*GetClass()->GetName(), TO_STRING(EInventoryItem, Type));

		return;
	}
	
	// Check mesh for this item type is a skeletal mesh just like we do for AInventoryItem_SM
	if (ItemInfo->GetMeshInfo().IsSkeletalMesh() == false)
	{
		Destroy();

		UE_LOG(RTSLOG, Warning, TEXT("Inventory item actor %s has item type \"%s\". This uses a "
			"static mesh however you have created a skeleta mesh actor for it. Change the actor "
			"to inherit from AInventoryItem_SM instead of AInventoryItem_SK. Actor has been destroyed "
			"for this session"), *GetClass()->GetName(), TO_STRING(EInventoryItem, Type));

		return;
	}
#endif

	/* Could pretty much just put this in base class and call Super */

	SetupSomeStuff(GameInstance);

	UniqueID = GameState->GenerateInventoryItemUniqueID(this);

	/* Using ItemInfo set some of the values in our attribute struct. The HUD will use them at
	some point */
	Attributes.SetName(ItemInfo->GetDisplayName());
	
	/* So creating a copy of FSlateBrush here, struct isn't TOO big */
	Attributes.SetHUDImage_Normal(ItemInfo->GetDisplayImage());
	Attributes.SetHUDImage_Hovered(ItemInfo->GetDisplayImage_Hovered());
	Attributes.SetHUDImage_Pressed(ItemInfo->GetDisplayImage_Pressed());

	/* Set mesh appearance */
	if (ItemInfo->GetMeshInfo().IsMeshSet())
	{
		/* Not 100% sure what 2nd param should be */
		Mesh->SetSkeletalMesh(ItemInfo->GetMeshInfo().GetMeshAsSkeletalMesh(), true);

		/* At this point we could play an idle anim, but we just assume that the item does not 
		have an idle anim (at least one that actually animates). So disable ticking on the mesh 
		because by default it is enabled. But if it does (i.e. the OnDropped anim's looping 
		part actually has animated stuff) then we will need to add another anim like IdleAnim 
		to FInventoryItemInfo and then instead of disabling tick here we play that idle anim 
		if the IdleAnim isn't null. If it is then we just disable tick like we did now */
		Mesh->SetComponentTickEnabled(false);

		// Think this is the right func, but have not tested thoroughly
		AddActorLocalTransform(FTransform(ItemInfo->GetMeshInfo().GetRotation(),
			ItemInfo->GetMeshInfo().GetLocation(), ItemInfo->GetMeshInfo().GetScale3D()));
	}

	if (bMakeHidden)
	{
		SetVis(false);
	}

	SelectionParticlesComp->SetComponentTickEnabled(true);

	GameState->PutInventoryItemActorInArray(this);

	/* Now could be a time to trace against the ground to make sure item starts on the ground,
	or do we not really care - let users place items however they want? Yeah let them place
	them how they want */
}

void AInventoryItem_SK::SetupForEnteringObjectPool(const URTSGameInstance * GameInstance)
{
	Super::SetupForEnteringObjectPool(GameInstance);

	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetComponentTickEnabled(false);
}

void AInventoryItem_SK::SetVis(bool bIsVisible, bool bAffectsSelectionDecal)
{
	Super::SetVis(bIsVisible, bAffectsSelectionDecal);

	Mesh->SetVisibility(bIsVisible, false);
}

void AInventoryItem_SK::ExitPool(ARTSGameState * GameState, EInventoryItem InItemType, int16 InQuantity,
	int16 InNumItemCharges, const FInventoryItemInfo & InItemsInfo, const FVector & Location, 
	const FRotator & Rotation, EItemEntersWorldReason ReasonForSpawning, bool bMakeVisible)
{
	ExitPool_BasicStuff(GameState, InItemType, InQuantity, InNumItemCharges, InItemsInfo);

	// Set mesh. Once again unsure what 2nd param should be. 1st param should not be null - we checked this in pooling manager
	Mesh->SetSkeletalMesh(ItemInfo->GetMeshInfo().GetMeshAsSkeletalMesh(), true);

	if (InItemsInfo.GetOnDroppedAnim() != nullptr /* && (ReasonForSpawning == EItemEntersWorldReason::DroppedOnDeath || ReasonForSpawning == EItemEntersWorldReason::ExplicitlyDropped) */)
	{
		// Enable tick so animation will play
		Mesh->SetComponentTickEnabled(true);
		
		Mesh->PlayAnimation(InItemsInfo.GetOnDroppedAnim(), false);
		//Mesh->GetAnimInstance()->Montage_Play(InItemsInfo.GetOnDroppedAnim());
	}

	// Set location/rotation/scale
	const FVector FinalLocation = Location + ItemInfo->GetMeshInfo().GetLocation();
	const FRotator FinalRotation = Rotation + ItemInfo->GetMeshInfo().GetRotation();
	const FVector FinalScale = ItemInfo->GetMeshInfo().GetScale3D();
	SetActorTransform(FTransform(FinalRotation, FinalLocation, FinalScale));

	// Re-enable collision so item responds to player's mouse
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	// Set visibility. Because we're leaving pool we know we're already hidden
	if (bMakeVisible)
	{
		SetVis(bMakeVisible);
	}

	ExitPool_FinalStuff(GameState);
}

void AInventoryItem_SK::ExitPool_FreshlySpawnedActor(URTSGameInstance * GameInst, ARTSGameState * GameState, 
	EInventoryItem InItemType, int16 InQuantity, int16 InNumItemCharges, const FInventoryItemInfo & InItemsInfo, 
	EItemEntersWorldReason ReasonForSpawning, bool bMakeHidden)
{
	SetupSomeStuff(GameInst);

	// Mostly copy of other overload, just changing location/rotation setting values
	// plus the vis setting logic is slightly different 

	ExitPool_BasicStuff(GameState, InItemType, InQuantity, InNumItemCharges, InItemsInfo);

	// Set mesh. Unsure what 2nd param should be. Note 1st param will not be null because we checked in pooling manager
	Mesh->SetSkeletalMesh(ItemInfo->GetMeshInfo().GetMeshAsSkeletalMesh(), true);

	/* Play 'OnDropped' animation if one is set. The stuff after &&... we'll leave that out for 
	now and assume that every time the item enters the world we should try play anim. Can change in 
	future though */
	if (InItemsInfo.GetOnDroppedAnim() != nullptr /* && (ReasonForSpawning == EItemEntersWorldReason::DroppedOnDeath || ReasonForSpawning == EItemEntersWorldReason::ExplicitlyDropped) */)
	{
		Mesh->PlayAnimation(InItemsInfo.GetOnDroppedAnim(), false);
		//Mesh->GetAnimInstance()->Montage_Play(InItemsInfo.GetOnDroppedAnim());
	}
	else
	{
		// Disable tick. By default it is enabled but because there's no anim we don't need it 
		Mesh->SetComponentTickEnabled(false);
	}

	/* Set location/rotation. Commented because we now set
	it in AObjectPoolingManager::PutItemInWorld for SpawnActor call */
	//AddActorLocalTransform(ItemInfo->GetMeshInfo().GetTransform());

	// Re-enable collision so item responds to player's mouse
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	if (!bMakeHidden)
	{
		SetVis(!bMakeHidden);
	}

	ExitPool_FinalStuff(GameState);
}

void AInventoryItem_SK::OnPickedUp(ISelectable * SelectableThatPickedUsUp, ARTSGameState * GameState, 
	ARTSPlayerController * LocalPlayCon)
{
	/* Set up mesh for putting back into object pool */
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetComponentTickEnabled(false);

	Super::OnPickedUp(SelectableThatPickedUsUp, GameState, LocalPlayCon);

	// Put in object pool
	GameState->PutInventoryItemInObjectPool(this);
}

#if !UE_BUILD_SHIPPING
bool AInventoryItem_SK::IsFitForEnteringObjectPool() const
{
	if (Mesh->GetCollisionEnabled() != ECollisionEnabled::NoCollision)
	{
		UE_LOG(RTSLOG, Fatal, TEXT("AInventoryItem::IsFitForEnteringObjectPool() failed because mesh collision is enabled"));
		return false;
	}
	if (Mesh->IsVisible() == true)
	{
		UE_LOG(RTSLOG, Fatal, TEXT("AInventoryItem::IsFitForEnteringObjectPool() failed because mesh is visible"));
		return false;
	}
	if (Mesh->IsComponentTickEnabled() == true)
	{
		UE_LOG(RTSLOG, Fatal, TEXT("AInventoryItem::IsFitForEnteringObjectPool() failed because mesh tick is enabled"));
		return false;
	}

	return Super::IsFitForEnteringObjectPool();
}
#endif
