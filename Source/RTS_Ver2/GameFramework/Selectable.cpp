// Fill out your copyright notice in the Description page of Project Settings.

#include "Selectable.h"

/* TODO: remove this include when GetProductionQueue is made pure virtual */
#include "MapElements/Building.h"
#include "Statics/DevelopmentStatics.h"

/* These are all non pure virtual functions. Since this interface
is shared by both units and buildings most of them will do nothing
and should be overridden when needed. Some will return values
that are expected because the child did not override them e.g.
GetBuildingType will return EBuildingType::NotBuilding assuming
it was a unit that chose not to override the function */

void ISelectable::Setup()
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

const FSelectableAttributesBasic * ISelectable::GetAttributes() const
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return nullptr;
}

const FBuildingAttributes * ISelectable::GetBuildingAttributes() const
{
	return nullptr;
}

const FInfantryAttributes * ISelectable::GetInfantryAttributes() const
{
	return nullptr;
}

const FAttackAttributes * ISelectable::GetAttackAttributes() const
{
	return nullptr;
}

const FContextMenuButtons * ISelectable::GetContextMenu() const
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return nullptr;
}

FBuildingAttributes * ISelectable::GetBuildingAttributesModifiable()
{
	return nullptr;
}

FSelectableAttributesBasic * ISelectable::GetAttributesModifiable()
{
	return nullptr;
}

FAttackAttributes * ISelectable::GetAttackAttributesModifiable()
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return nullptr;
}

void ISelectable::SetupBuildInfo(FBuildInfo & OutInfo, const AFactionInfo * FactionInfo) const
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

void ISelectable::SetupBuildInfo(FUnitInfo & OutInfo, const AFactionInfo * FactionInfo) const
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

bool ISelectable::CanClassGainExperience() const
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return false;
}

uint8 ISelectable::GetRank() const
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return 0;
}

float ISelectable::GetCurrentRankExperience() const
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return -1.0f;
}

float ISelectable::GetTotalExperienceRequiredForNextLevel() const
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return -1.0f;
}

float ISelectable::GetTotalExperienceRequiredForLevel(FSelectableRankInt Level) const
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return -1.0f;
}

void ISelectable::GainExperience(float ExperienceBounty, bool bApplyRandomness)
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

UBoxComponent * ISelectable::GetBounds() const
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return nullptr;
}

FSelectableRootComponent2DShapeInfo ISelectable::GetRootComponent2DCollisionInfo() const
{
	NO_OVERRIDE_SO_FATAL_LOG;
	/* Return garbage */
	return FSelectableRootComponent2DShapeInfo(0.f);
}

void ISelectable::OnSingleSelect()
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

EUnitType ISelectable::OnMarqueeSelect(uint8 & SelectableID)
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return EUnitType::NotUnit;
}

EUnitType ISelectable::OnCtrlGroupSelect(uint8 & OutSelectableID)
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return EUnitType::NotUnit;
}

uint8 ISelectable::GetSelectableID() const
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return 0;
}

uint8 ISelectable::GetOwnersID() const
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return UINT8_MAX;
}

uint8 ISelectable::OnDeselect()
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return 0;
}

void ISelectable::OnRightClick()
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

void ISelectable::OnMouseHover(ARTSPlayerController * LocalPlayCon)
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

void ISelectable::OnMouseUnhover(ARTSPlayerController * LocalPlayCon)
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

void ISelectable::OnEnterMarqueeBox(ARTSPlayerController * LocalPlayCon)
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

void ISelectable::OnExitMarqueeBox(ARTSPlayerController * LocalPlayCon)
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

float ISelectable::GetHealth() const
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return -1.0f;
}

float & ISelectable::GetHealthRef()
{
	NO_OVERRIDE_SO_FATAL_LOG;
	static float Dummy = 0.f;
	return Dummy;
}

void ISelectable::SetHealth(float NewValue)
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

ETeam ISelectable::GetTeam() const
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return ETeam::Uninitialized;
}

uint8 ISelectable::GetTeamIndex() const
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return UINT8_MAX;
}

void ISelectable::GetInfo(TMap < EResourceType, int32 > & OutCosts, float & OutBuildTime, FContextMenuButtons & OutContextMenu) const
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

TArray<EBuildingType> ISelectable::GetPrerequisites() const
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return TArray<EBuildingType>();
}

float ISelectable::GetCooldownRemaining(const FContextButton & Button)
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return 0.0f;
}

const TMap<FContextButton, FTimerHandle>* ISelectable::GetContextCooldowns() const
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return nullptr;
}

bool ISelectable::HasEnoughSelectableResources(const FContextButtonInfo & AbilityInfo) const
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return false;
}

uint8 ISelectable::GetAppliedGameTickCount() const
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return UINT8_MAX;
}

void ISelectable::RegenSelectableResourcesFromGameTicks(uint8 NumGameTicksWorth, URTSHUD * HUDWidget, ARTSPlayerController * LocalPlayerController)
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

void ISelectable::OnAbilityUse(const FContextButtonInfo & AbilityInfo, uint8 ServerTickCountAtTimeOfAbility)
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

float ISelectable::GetSightRadius() const
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return -1.f;
}

void ISelectable::GetVisionInfo(float & SightRadius, float & StealthRevealRadius) const
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

float ISelectable::GetBoundsLength() const
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return -1.f;
}

bool ISelectable::UpdateFogStatus(EFogStatus FogStatus)
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return false;
}

bool ISelectable::Security_CanBeClickedOn(ETeam ClickersTeam, const FName & ClickersTeamTag, ARTSGameState * GameState) const
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return false;
}

bool ISelectable::IsInStealthMode() const
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return false;
}

void ISelectable::OnUpgradeComplete(EUpgradeType UpgradeType, uint8 UpgradeLevel)
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

TSubclassOf<AProjectileBase> ISelectable::GetProjectileBP() const
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return nullptr;
}

ARTSPlayerController * ISelectable::GetLocalPC() const
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return nullptr;
}

bool ISelectable::HasAttack() const
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return false;
}

void ISelectable::SetIsInvulnerable(bool bInvulnerable)
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

FVector ISelectable::GetMuzzleLocation() const
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return FVector();
}

bool ISelectable::CanAquireTarget(AActor * PotentialTargetAsActor, ISelectable * PotentialTarget) const
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return false;
}

const FBuildingTargetingAbilityPerSelectableInfo * ISelectable::GetSpecialRightClickActionTowardsBuildingInfo(ABuilding * Building, EFaction BuildingsFaction, EBuildingType BuildingsType, EAffiliation BuildingsAffiliation) const
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return nullptr;
}

const FCursorInfo * ISelectable::GetSelectedMouseCursor_CanAttackHostileUnit(URTSGameInstance * GameInst, AActor * HostileAsActor, ISelectable * HostileUnit) const
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return nullptr;
}

const FCursorInfo * ISelectable::GetSelectedMouseCursor_CanAttackFriendlyUnit(URTSGameInstance * GameInst, AActor * FriendlyAsActor, ISelectable * FriendlyUnit, EAffiliation UnitsAffiliation) const
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return nullptr;
}

const FCursorInfo * ISelectable::GetSelectedMouseCursor_CanAttackHostileBuilding(URTSGameInstance * GameInst, ABuilding * HostileBuilding) const
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return nullptr;
}

const FCursorInfo * ISelectable::GetSelectedMouseCursor_CanAttackFriendlyBuilding(URTSGameInstance * GameInst, ABuilding * FriendlyBuilding, EAffiliation BuildingsAffiliation) const
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return nullptr;
}

void ISelectable::OnRightClickCommand(const FVector & Location)
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

void ISelectable::OnRightClickCommand(ISelectable * TargetAsSelectable, const FSelectableAttributesBase & TargetInfo)
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

void ISelectable::IssueCommand_PickUpItem(const FVector & SomeLocation, AInventoryItem * TargetItem)
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

void ISelectable::IssueCommand_MoveTo(const FVector & Location)
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

void ISelectable::IssueCommand_RightClickOnResourceSpot(AResourceSpot * ResourceSpot)
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

void ISelectable::IssueCommand_SpecialBuildingTargetingAbility(ABuilding * TargetBuilding, const FBuildingTargetingAbilityPerSelectableInfo * AbilityInfo)
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

void ISelectable::IssueCommand_EnterGarrison(ABuilding * TargetBuilding, const FBuildingGarrisonAttributes & GarrisonAttributes)
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

void ISelectable::OnLayFoundationCommand(EBuildingType BuildingType, const FVector & Location, const FRotator & Rotation)
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

void ISelectable::OnContextCommand(const FContextButtonInfo & AbilityInfo, const FVector & ClickLoc, ISelectable * Target, const FSelectableAttributesBase & TargetInfo)
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

void ISelectable::OnContextCommand(const FContextButtonInfo & AbilityInfo, const FVector & ClickLoc)
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

void ISelectable::OnContextCommand(const FContextButton & Button)
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

void ISelectable::IssueCommand_UseInventoryItem(uint8 InventorySlotIndex,
	EInventoryItem ItemType, const FContextButtonInfo & AbilityInfo)
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

void ISelectable::IssueCommand_UseInventoryItem(uint8 InventorySlotIndex, EInventoryItem ItemType,
	const FContextButtonInfo & AbilityInfo, const FVector & Location)
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

void ISelectable::IssueCommand_UseInventoryItem(uint8 InventorySlotIndex, EInventoryItem ItemType,
	const FContextButtonInfo & AbilityInfo, ISelectable * Target)
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

void ISelectable::OnEnemyDestroyed(const FSelectableAttributesBasic & EnemyAttributes)
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

void ISelectable::Client_OnLevelGained(uint8 NewLevel)
{
	UE_LOG(RTSLOG, Fatal, TEXT("ISelectable virtual function not overridden"));
}

const FProductionQueue * ISelectable::GetProductionQueue() const
{
	return nullptr;
}

void ISelectable::OnProductionCancelled(uint8 Index)
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

float ISelectable::GetBuildingQueuePercentage() const
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return 0.0f;
}

void ISelectable::SetOnSpawnValues(ESelectableCreationMethod InCreationMethod, ABuilding * BuilderBuilding)
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

void ISelectable::SetSelectableIDAndGameTickCount(uint8 InID, uint8 GameTickCounter)
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

const FAttachInfo * ISelectable::GetBodyLocationInfo(ESelectableBodySocket BodyLocationType) const
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return nullptr;
}

void ISelectable::AttachParticles(const FContextButtonInfo & AbilityInfo, UParticleSystem * Template,
	ESelectableBodySocket AttachLocation, uint32 ParticleIndex)
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

void ISelectable::AttachParticles(const FTickableBuffOrDebuffInfo & BuffOrDebuffInfo)
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

void ISelectable::AttachParticles(const FStaticBuffOrDebuffInfo & BuffOrDebuffInfo)
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

void ISelectable::RemoveAttachedParticles(const FTickableBuffOrDebuffInfo & BuffOrDebuffInfo)
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

void ISelectable::RemoveAttachedParticles(const FStaticBuffOrDebuffInfo & BuffOrDebuffInfo)
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

void ISelectable::OnContextMenuPlaceBuildingResult(ABuilding * PlacedBuilding, const FBuildingInfo & BuildingInfo, 
	EBuildingBuildMethod BuildMethod)
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

void ISelectable::OnWorkedOnBuildingConstructionComplete()
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

void ISelectable::Server_OnOwningPlayerDefeated()
{
	/* Override in derived class. Make selectable have zero health */
	NO_OVERRIDE_SO_FATAL_LOG;
}

FVector ISelectable::GetActorLocationSelectable() const
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return FVector();
}

float ISelectable::GetDistanceFromAnotherForAbilitySquared(const FContextButtonInfo & AbilityInfo, ISelectable * Target) const
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return 0.0f;
}

float ISelectable::GetDistanceFromLocationForAbilitySquared(const FContextButtonInfo & AbilityInfo, const FVector & Location) const
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return 0.0f;
}

void ISelectable::Consume_BuildingTargetingAbilityInstigator()
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

bool ISelectable::CanClassAcceptBuffsAndDebuffs() const
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return false;
}

const TArray<FStaticBuffOrDebuffInstanceInfo>* ISelectable::GetStaticBuffArray() const
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return nullptr;
}

const TArray<FTickableBuffOrDebuffInstanceInfo>* ISelectable::GetTickableBuffArray() const
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return nullptr;
}

const TArray<FStaticBuffOrDebuffInstanceInfo>* ISelectable::GetStaticDebuffArray() const
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return nullptr;
}

const TArray<FTickableBuffOrDebuffInstanceInfo>* ISelectable::GetTickableDebuffArray() const
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return nullptr;
}

//====================================================================================
//	Buffs and Debuffs
//====================================================================================

FStaticBuffOrDebuffInstanceInfo * ISelectable::GetBuffState(EStaticBuffAndDebuffType BuffType)
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return nullptr;
}

FTickableBuffOrDebuffInstanceInfo * ISelectable::GetBuffState(ETickableBuffAndDebuffType BuffType)
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return nullptr;
}

FStaticBuffOrDebuffInstanceInfo * ISelectable::GetDebuffState(EStaticBuffAndDebuffType DebuffType)
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return nullptr;
}

FTickableBuffOrDebuffInstanceInfo * ISelectable::GetDebuffState(ETickableBuffAndDebuffType DebuffType)
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return nullptr;
}

void ISelectable::RegisterBuff(EStaticBuffAndDebuffType BuffType, AActor * BuffInstigator, ISelectable * InstigatorAsSelectable)
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

void ISelectable::RegisterDebuff(EStaticBuffAndDebuffType DebuffType, AActor * DebuffInstigator, ISelectable * InstigatorAsSelectable)
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

void ISelectable::RegisterBuff(ETickableBuffAndDebuffType BuffType, AActor * BuffInstigator, ISelectable * InstigatorAsSelectable)
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

void ISelectable::RegisterDebuff(ETickableBuffAndDebuffType DebuffType, AActor * DebuffInstigator, ISelectable * InstigatorAsSelectable)
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

EBuffOrDebuffRemovalOutcome ISelectable::RemoveBuff(EStaticBuffAndDebuffType BuffType, AActor * RemovalInstigator,
	ISelectable * InstigatorAsSelectable, EBuffAndDebuffRemovalReason RemovalReason)
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return EBuffOrDebuffRemovalOutcome::NotPresent;
}

EBuffOrDebuffRemovalOutcome ISelectable::RemoveDebuff(EStaticBuffAndDebuffType DebuffType, AActor * RemovalInstigator,
	ISelectable * InstigatorAsSelectable, EBuffAndDebuffRemovalReason RemovalReason)
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return EBuffOrDebuffRemovalOutcome::NotPresent;
}

EBuffOrDebuffRemovalOutcome ISelectable::RemoveBuff(ETickableBuffAndDebuffType BuffType, AActor * RemovalInstigator,
	ISelectable * InstigatorAsSelectable, EBuffAndDebuffRemovalReason RemovalReason)
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return EBuffOrDebuffRemovalOutcome::NotPresent;
}

EBuffOrDebuffRemovalOutcome ISelectable::RemoveDebuff(ETickableBuffAndDebuffType DebuffType, AActor * RemovalInstigator,
	ISelectable * InstigatorAsSelectable, EBuffAndDebuffRemovalReason RemovalReason)
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return EBuffOrDebuffRemovalOutcome::NotPresent;
}

void ISelectable::Client_RemoveBuffGivenOutcome(EStaticBuffAndDebuffType BuffType, AActor * RemovalInstigator, ISelectable * InstigatorAsSelectable, EBuffOrDebuffRemovalOutcome Outcome)
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

void ISelectable::Client_RemoveDebuffGivenOutcome(EStaticBuffAndDebuffType DebuffType, AActor * RemovalInstigator, ISelectable * InstigatorAsSelectable, EBuffOrDebuffRemovalOutcome Outcome)
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

void ISelectable::Client_RemoveBuffGivenOutcome(ETickableBuffAndDebuffType BuffType, AActor * RemovalInstigator, ISelectable * InstigatorAsSelectable, EBuffOrDebuffRemovalOutcome Outcome)
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

void ISelectable::Client_RemoveDebuffGivenOutcome(ETickableBuffAndDebuffType DebuffType, AActor * RemovalInstigator, ISelectable * InstigatorAsSelectable, EBuffOrDebuffRemovalOutcome Outcome)
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

float ISelectable::TakeDamageSelectable(float DamageAmount, FDamageEvent const & DamageEvent, AController * EventInstigator, AActor * DamageCauser)
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return 0.0f;
}

bool ISelectable::HasZeroHealth() const
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return false;
}

float ISelectable::ApplyTempMoveSpeedMultiplier(float Multiplier)
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return 0.0f;
}

float ISelectable::RemoveTempMoveSpeedMultiplier(float Multiplier)
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return 0.0f;
}

bool ISelectable::ApplyTempStealthModeEffect()
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return false;
}

bool ISelectable::RemoveTempStealthModeEffect()
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return false;
}

URTSGameInstance * ISelectable::Selectable_GetGI() const
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return nullptr;
}

ARTSPlayerState * ISelectable::Selectable_GetPS() const
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return nullptr;
}

void ISelectable::ShowTooltip(URTSHUD * HUDWidget) const
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

USelectableWidgetComponent * ISelectable::GetPersistentWorldWidget() const
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return nullptr;
}

USelectableWidgetComponent * ISelectable::GetSelectedWorldWidget() const
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return nullptr;
}

void ISelectable::AdjustPersistentWorldWidgetForNewCameraZoomAmount(float CameraZoomAmount, float CameraPitchAmount)
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

void ISelectable::AdjustSelectedWorldWidgetForNewCameraZoomAmount(float CameraZoomAmount, float CameraPitchAmount)
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

const FShopInfo * ISelectable::GetShopAttributes() const
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return nullptr;
}

void ISelectable::OnItemPurchasedFromHere(int32 ShopSlotIndex, ISelectable * ItemRecipient, bool bTryUpdateHUD)
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

bool ISelectable::IsAShopInRange() const
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return false;
}

const FInventory * ISelectable::GetInventory() const
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return nullptr;
}

FInventory * ISelectable::GetInventoryModifiable()
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return nullptr;
}

void ISelectable::OnInventoryItemUse(uint8 ServerInventorySlotIndex, const FContextButtonInfo & AbilityInfo, uint8 ServerTickCountAtTimeOfAbility)
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

void ISelectable::StartInventoryItemUseCooldownTimerHandle(FTimerHandle & TimerHandle, uint8 ServerInventorySlotIndex, float Duration)
{
	NO_OVERRIDE_SO_FATAL_LOG;
}

#if WITH_EDITOR
bool ISelectable::PIE_IsForCPUPlayer() const
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return false;
}

int32 ISelectable::PIE_GetHumanOwnerIndex() const
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return -1;
}

int32 ISelectable::PIE_GetCPUOwnerIndex() const
{
	NO_OVERRIDE_SO_FATAL_LOG;
	return -1;
}
#endif // WITH_EDITOR

void ISelectable::AdjustForUpgrades()
{
	NO_OVERRIDE_SO_FATAL_LOG;
}
