// Fill out your copyright notice in the Description page of Project Settings.

#include "Structs_1.h"
#include "Animation/AnimMontage.h"
#include "Particles/ParticleSystemComponent.h" // Need this for assigning to a TWeakObjectPtr<UParticleSystemComponent>

#include "Statics/DevelopmentStatics.h"
#include "Statics/Statics.h"
#include "UI/RTSHUD.h"
#include "GameFramework/RTSPlayerState.h"
#include "GameFramework/FactionInfo.h"
#include "Managers/UpgradeManager.h"
#include "MapElements/Building.h"
#include "GameFramework/RTSPlayerController.h"
#include "Miscellaneous/CPUPlayerAIController.h"
#include "MapElements/Projectiles/ProjectileBase.h"
#include "GameFramework/RTSGameState.h"
#include "GameFramework/RTSGameInstance.h"
#include "UI/WorldWidgets/SelectableWidgetComponent.h"
#include "MapElements/Infantry.h"
#include "MapElements/InventoryItem.h"
#include "MapElements/BuildingComponents/BuildingAttackComp_Turret.h"
#include "MapElements/BuildingTargetingAbilities//BuildingTargetingAbilityBase.h"


//==========================================================================================
//	>FFactionBuildingTypePair
//==========================================================================================

FFactionBuildingTypePair::FFactionBuildingTypePair()
	: Faction(EFaction::None)
	, BuildingType(EBuildingType::NotBuilding)
{
}


//======================================================================================
//	>FMeshInfoBasic
//======================================================================================

FMeshInfoBasic::FMeshInfoBasic()
	: Mesh(nullptr)
	, Location(FVector::ZeroVector) 
	, Rotation(FRotator::ZeroRotator)
	, Scale(FVector::OneVector) 
{
#if WITH_EDITORONLY_DATA
	WORKAROUND_MeshPath = FString();
#endif
}

bool FMeshInfoBasic::IsMeshSet() const
{
	return Mesh != nullptr;
}

bool FMeshInfoBasic::IsStaticMesh() const
{
	return (Mesh->GetClass() == UStaticMesh::StaticClass());
}

bool FMeshInfoBasic::IsSkeletalMesh() const
{
	return (Mesh->GetClass() == USkeletalMesh::StaticClass());
}

UStaticMesh * FMeshInfoBasic::GetMeshAsStaticMesh() const
{
	return CastChecked<UStaticMesh>(Mesh);
}

USkeletalMesh * FMeshInfoBasic::GetMeshAsSkeletalMesh() const
{
	return CastChecked<USkeletalMesh>(Mesh);
}

FVector FMeshInfoBasic::GetLocation() const
{
	return Location;
}

FRotator FMeshInfoBasic::GetRotation() const
{
	return Rotation;
}

FVector FMeshInfoBasic::GetScale3D() const
{
	return Scale;
}

#if WITH_EDITOR
void FMeshInfoBasic::OnPostEdit()
{
	if (WORKAROUND_MeshPath != FString())
	{
		Mesh = LoadObject<UObject>(nullptr, *WORKAROUND_MeshPath);
	}
}
#endif


//======================================================================================
//	>FCursorInfo
//======================================================================================

FCursorInfo::FCursorInfo()
{
	CursorPath = FName();
	HotSpot = FVector2D::ZeroVector;
	FullPath = FName();
}

FCursorInfo::FCursorInfo(const FString & InFullPath, FVector2D InHotSpot) 
	: HotSpot(InHotSpot) 
	, FullPath(*InFullPath)
{
}

void FCursorInfo::SetFullPath()
{
#if WITH_EDITOR
	/* Make sure we haven't already done this - no point calling it more than once */
	//assert(bHasSetFullPath == false);
	/* Commented assert above. I hit it all the time. I'm not 100% sure what the issue is. It's just 
	a efficiency thing so it's no biggie. I call SetFullPath on all entries in GI::MouseCursors. 
	Then when I call SetFullPath on the specific FCursorInfos on GI I have already apparently 
	called it. However the ones on the FI seem to not crash. It's as if the ones on GI are 
	pointers or something while the ones on FI are actual values TODO */
	bHasSetFullPath = true;
#endif

	/* Editor sets default value as "None" and it actually is that string, which is unexpected, so
	need this check */
	if (CursorPath.ToString() == "None")
	{
		FullPath = Statics::HARDWARE_CURSOR_PATH;
	}
	else
	{
		FullPath = *FString(Statics::HARDWARE_CURSOR_PATH.ToString() + CursorPath.ToString());
	}
}

bool FCursorInfo::ContainsCustomCursor() const
{
	return FullPath != Statics::HARDWARE_CURSOR_PATH;
}


//======================================================================================
//	>FContextCommand
//======================================================================================

FContextCommand::FContextCommand(bool bUseSameGroup, const FContextButton & Button) 
	: bSameGroup(bUseSameGroup) 
{
	SetupTypes(Button);
}

FContextCommand::FContextCommand(bool bUseSameGroup, EContextButton AbilityType) 
	: bSameGroup(bUseSameGroup)
{
	SetupTypes(AbilityType);
}

FContextCommand::FContextCommand()
{
	/* These appear to get called with RPCs. Have changed to using intializer lists. Will 
	see if that's still the case */
	//assert(0);
}

void FContextCommand::AddSelectable(uint8 SelectableID)
{
	AffectedSelectables.Emplace(SelectableID);
}

void FContextCommand::SetupTypes(EContextButton AbilityType)
{
	/* If thrown then you will want to call the FContextButton param version instead */
	assert(AbilityType != EContextButton::BuildBuilding && AbilityType != EContextButton::Train
		&& AbilityType != EContextButton::Upgrade);
	
	Type = Statics::ContextButtonToArrayIndex(AbilityType);
}

void FContextCommand::SetupTypes(const FContextButton & Button)
{
	Type = Statics::ContextButtonToArrayIndex(Button.GetButtonType());

	switch (Button.GetButtonType())
	{
	// Does case EContextButton::BuildBuilding also need to be here?
	case EContextButton::Train:
	{
		SubType = Statics::UnitTypeToArrayIndex(Button.GetUnitType());
		break;
	}
	case EContextButton::Upgrade:
	{
		SubType = static_cast<uint8>(Button.GetUpgradeType());
		break;
	}
	default:
	{
		break;
	}
	}
}


//======================================================================================
//	>FContextCommandWithLocation
//======================================================================================

FContextCommandWithLocation::FContextCommandWithLocation()
{
	// Commented because crash here on startup after migrating to 4.21
	//assert(0);
}

FContextCommandWithLocation::FContextCommandWithLocation(EContextButton AbilityType,
	bool bUseSameGroup, const FVector & ClickLoc) 
	: Super(bUseSameGroup, AbilityType)
	, ClickLocation(ClickLoc)
{
}


//======================================================================================
//	>FContextCommandWithTarget
//======================================================================================

FContextCommandWithTarget::FContextCommandWithTarget(EContextButton AbilityType,
	bool bUseSameGroup, const FVector & ClickLoc, AActor * ClickTarget) 
	: Super(bUseSameGroup, AbilityType)
	, ClickLocation(ClickLoc) 
	, Target(ClickTarget)
{
}

/* Don't think this is ever called */
FContextCommandWithTarget::FContextCommandWithTarget()
{
	// Commented because crash here on startup after migrating to 4.21
	//assert(0);
	Target = nullptr;
}


//======================================================================================
//	>FRightClickCommandBase
//======================================================================================

FRightClickCommandBase::FRightClickCommandBase()
{
	/* Not expected to be called, but will be for RPCs */
}

FRightClickCommandBase::FRightClickCommandBase(const FVector & ClickLoc) 
	: ClickLocation(ClickLoc)
{
}

void FRightClickCommandBase::ReserveAffectedSelectables(int32 Num)
{
	AffectedSelectables.Reserve(Num);
}

void FRightClickCommandBase::AddSelectable(uint8 SelectableID)
{
	assert(SelectableID != 0);

	AffectedSelectables.Emplace(SelectableID);
}

void FRightClickCommandBase::SetUseSameGroup(bool bValue)
{
	bSameGroup = bValue;
}

bool FRightClickCommandBase::UseSameGroup() const
{
	return bSameGroup;
}

const TArray<uint8>& FRightClickCommandBase::GetAffectedSelectables() const
{
	return AffectedSelectables;
}

const FVector & FRightClickCommandBase::GetClickLocation() const
{
	return ClickLocation;
}


//======================================================================================
//	>FRightClickCommandWithTarget
//======================================================================================

FRightClickCommandWithTarget::FRightClickCommandWithTarget(const FVector & ClickLoc, AActor * InTarget) 
	: Super(ClickLoc) 
	, Target(InTarget)
{
}

FRightClickCommandWithTarget::FRightClickCommandWithTarget()
{
	/* Not expected to be called, but will be for RPCs */
}

AActor * FRightClickCommandWithTarget::GetTarget() const
{
	return Target;
}


//======================================================================================
//	>FRightClickCommandOnInventoryItem
//======================================================================================

FRightClickCommandOnInventoryItem::FRightClickCommandOnInventoryItem()
{
}

FRightClickCommandOnInventoryItem::FRightClickCommandOnInventoryItem(const FVector & ClickLoc, AInventoryItem * InClickedItem) 
	: Super(ClickLoc)
	, ItemID(InClickedItem->GetUniqueID())
{
}

AInventoryItem * FRightClickCommandOnInventoryItem::GetClickedInventoryItem(ARTSGameState * GameState) const
{	
	return GameState->GetInventoryItemFromID(ItemID);
}


//======================================================================================
//	>FContextButton
//======================================================================================

FContextButton::FContextButton(EBuildingType InBuildingType)
{
	ButtonType = EContextButton::BuildBuilding;
	BuildingType = InBuildingType;
	UnitType = EUnitType::NotUnit;
	UpgradeType = EUpgradeType::None;
	BuildInfo = nullptr;
}

FContextButton::FContextButton(EUnitType InUnitType)
{
	ButtonType = EContextButton::Train;
	BuildingType = EBuildingType::NotBuilding;
	UnitType = InUnitType;
	UpgradeType = EUpgradeType::None;
	BuildInfo = nullptr;
}

FContextButton::FContextButton(EUpgradeType InUpgradeType)
{
	ButtonType = EContextButton::Upgrade;
	BuildingType = EBuildingType::NotBuilding;
	UnitType = EUnitType::NotUnit;
	UpgradeType = InUpgradeType;
	BuildInfo = nullptr;
}

FContextButton::FContextButton(EContextButton InButtonType)
{
	assert(ButtonType != EContextButton::BuildBuilding);
	assert(ButtonType != EContextButton::Train);
	assert(ButtonType != EContextButton::Upgrade);

	ButtonType = InButtonType;
	BuildingType = EBuildingType::NotBuilding;
	UnitType = EUnitType::NotUnit;
	UpgradeType = EUpgradeType::None;
	BuildInfo = nullptr;
}

FContextButton::FContextButton(const FContextCommand & Command)
{
	ButtonType = Statics::ArrayIndexToContextButton(Command.Type);
	if (ButtonType == EContextButton::Train)
	{
		BuildingType = EBuildingType::NotBuilding;
		UnitType = Statics::ArrayIndexToUnitType(Command.SubType);
		UpgradeType = EUpgradeType::None;
	}
	else if (ButtonType == EContextButton::Upgrade)
	{
		BuildingType = EBuildingType::NotBuilding;
		UpgradeType = static_cast<EUpgradeType>(Command.SubType);
		UnitType = EUnitType::NotUnit;
	}
	else if (ButtonType == EContextButton::BuildBuilding)
	{
		BuildingType = static_cast<EBuildingType>(Command.SubType);
		UpgradeType = EUpgradeType::None;
		UnitType = EUnitType::NotUnit;
	}
	else
	{
		BuildingType = EBuildingType::NotBuilding;
		UnitType = EUnitType::NotUnit;
		UpgradeType = EUpgradeType::None;
	}
}

FContextButton::FContextButton(const FBuildInfo * InBuildInfo)
{
	if (InBuildInfo->GetSelectableType() == ESelectableType::Building)
	{
		ButtonType = EContextButton::BuildBuilding;
		BuildingType = InBuildInfo->GetBuildingType();
		UnitType = EUnitType::NotUnit;
		UpgradeType = EUpgradeType::None;
	}
	else if (InBuildInfo->GetSelectableType() == ESelectableType::Unit)
	{
		ButtonType = EContextButton::Train;
		BuildingType = EBuildingType::NotBuilding;
		UnitType = InBuildInfo->GetUnitType();
		UpgradeType = EUpgradeType::None;
	}
	else
	{
		UE_LOG(RTSLOG, Fatal, TEXT("Unknown selectable type"));
	}

	SetHUDPersistentTabButtonOrdering(InBuildInfo->GetHUDPersistentTabOrdering());
	BuildInfo = InBuildInfo;
}

FContextButton::FContextButton(const FTrainingInfo & InTrainingInfo)
{
	if (InTrainingInfo.IsForUnit())
	{
		ButtonType = EContextButton::Train;
		BuildingType = EBuildingType::NotBuilding;
		UnitType = InTrainingInfo.GetUnitType();
		UpgradeType = EUpgradeType::None;
	}
	else if (InTrainingInfo.IsProductionForBuilding())
	{
		ButtonType = EContextButton::BuildBuilding;
		BuildingType = InTrainingInfo.GetBuildingType();
		UnitType = EUnitType::NotUnit;
		UpgradeType = EUpgradeType::None;
	}
	else
	{
		assert(InTrainingInfo.IsForUpgrade());
		ButtonType = EContextButton::Upgrade;
		BuildingType = EBuildingType::NotBuilding;
		UnitType = EUnitType::NotUnit;
		UpgradeType = InTrainingInfo.GetUpgradeType();
	}
}

FContextButton::FContextButton()
{
	ButtonType = EContextButton::None;
	BuildingType = EBuildingType::NotBuilding;
	UnitType = EUnitType::NotUnit;
	UpgradeType = EUpgradeType::None;
	BuildInfo = nullptr;
}

#if !UE_BUILD_SHIPPING
FString FContextButton::ToString() const
{
	FString S;
	S.Append(FString(*TEXT("Button type: %s | "), TO_STRING(EContextButton, ButtonType)));
	S.Append(FString(*TEXT("Building type: %s | "), TO_STRING(EBuildingType, BuildingType)));
	S.Append(FString(*TEXT("Unit type: %s | "), TO_STRING(EUnitType, UnitType)));
	S.Append(FString(*TEXT("Upgrade type: %s"), TO_STRING(EUpgradeType, UpgradeType)));

	return S;
}
#endif


//======================================================================================
//	>FTrainingInfo
//======================================================================================

bool FTrainingInfo::IsProductionForBuilding() const
{
	return BuildingType != EBuildingType::NotBuilding;
}

bool FTrainingInfo::IsForUnit() const
{
	return UnitType != EUnitType::NotUnit;
}

bool FTrainingInfo::IsForUpgrade() const
{
	return UpgradeType != EUpgradeType::None;
}

FTrainingInfo::FTrainingInfo(const FContextButton & InButton)
{
	BuildingType = InButton.GetBuildingType();
	UnitType = InButton.GetUnitType();
	UpgradeType = InButton.GetUpgradeType();

#if WITH_EDITOR
	/* Make sure values are what I expect */
	if (BuildingType != EBuildingType::NotBuilding)
	{
		assert(UnitType == EUnitType::NotUnit && UpgradeType == EUpgradeType::None);
	}
	else if (UnitType != EUnitType::NotUnit)
	{
		assert(BuildingType == EBuildingType::NotBuilding && UpgradeType == EUpgradeType::None);
	}
	else // Assumed upgrade
	{
		assert(UpgradeType != EUpgradeType::None);
		assert(BuildingType == EBuildingType::NotBuilding && UnitType == EUnitType::NotUnit);
	}
#endif
}

FTrainingInfo::FTrainingInfo(EBuildingType InBuildingType)
{
	BuildingType = InBuildingType;
	UnitType = EUnitType::NotUnit;
	UpgradeType = EUpgradeType::None;
}

FTrainingInfo::FTrainingInfo(EUnitType InUnitType)
{
	BuildingType = EBuildingType::NotBuilding;
	UnitType = InUnitType;
	UpgradeType = EUpgradeType::None;
}

FTrainingInfo::FTrainingInfo(EUpgradeType InUpgradeType)
{
	BuildingType = EBuildingType::NotBuilding;
	UnitType = EUnitType::NotUnit;
	UpgradeType = InUpgradeType;
}

/* So passing this struct as a param in an RPC will call this. Assert commented
for now. Uncomment it to see stuff crash in Building.cpp when calling RPCs */
FTrainingInfo::FTrainingInfo()
{
	/* Throw error */
	//assert(nullptr);
}

/* For TMap */
bool operator==(const FTrainingInfo & Info1, const FTrainingInfo & Info2)
{
	return Info1.GetBuildingType() == Info2.GetBuildingType()
		&& Info1.GetUnitType() == Info2.GetUnitType()
		&& Info1.GetUpgradeType() == Info2.GetUpgradeType();
}

uint32 GetTypeHash(const FTrainingInfo & Info)
{
	return static_cast<int32>(Info.GetBuildingType())
		+ static_cast<int32>(Info.GetUnitType()) * 256
		+ static_cast<int32>(Info.GetUpgradeType()) * FMath::Square<int32>(256);
}

#if !UE_BUILD_SHIPPING
FString FTrainingInfo::ToString() const
{
	FString S; 
	S += "Training info: ";
	S += ENUM_TO_STRING(EBuildingType, BuildingType);
	S += ", " + ENUM_TO_STRING(EUnitType, UnitType);
	S += ", " + ENUM_TO_STRING(EUpgradeType, UpgradeType);
	
	return S;
}
#endif // !UE_BUILD_SHIPPING

EBuildingType FTrainingInfo::GetBuildingType() const
{
	return BuildingType;
}

EUnitType FTrainingInfo::GetUnitType() const
{
	return UnitType;
}

EUpgradeType FTrainingInfo::GetUpgradeType() const
{
	return UpgradeType;
}


//======================================================================================
//	>FCPUPlayerTrainingInfo
//======================================================================================

FCPUPlayerTrainingInfo::FCPUPlayerTrainingInfo()
{
	// Commented because crash here on startup after migrating to 4.21
	//assert(0);
}

FCPUPlayerTrainingInfo::FCPUPlayerTrainingInfo(EBuildingType InBuildingType, const FMacroCommandReason & CommandReason) 
	: FTrainingInfo(InBuildingType)
{
	ActualCommandType = CommandReason.GetActualCommand();
	AuxilleryInfo = CommandReason.GetRawAuxilleryReason();

	OriginalCommand_ReasonForProduction = CommandReason.GetOriginalCommandMainReason();
	OriginalCommand_AuxilleryInfo = CommandReason.GetOriginalCommandAuxilleryData();
}

FCPUPlayerTrainingInfo::FCPUPlayerTrainingInfo(EUnitType InUnitType, const FMacroCommandReason & CommandReason)
	: FTrainingInfo(InUnitType)
{
	ActualCommandType = CommandReason.GetActualCommand();
	AuxilleryInfo = CommandReason.GetRawAuxilleryReason();

	OriginalCommand_ReasonForProduction = CommandReason.GetOriginalCommandMainReason();
	OriginalCommand_AuxilleryInfo = CommandReason.GetOriginalCommandAuxilleryData();
}

FCPUPlayerTrainingInfo::FCPUPlayerTrainingInfo(EUpgradeType InUpgradeType, const FMacroCommandReason & CommandReason)
	: FTrainingInfo(InUpgradeType)
{
	ActualCommandType = CommandReason.GetActualCommand();
	AuxilleryInfo = CommandReason.GetRawAuxilleryReason();

	OriginalCommand_ReasonForProduction = CommandReason.GetOriginalCommandMainReason();
	OriginalCommand_AuxilleryInfo = CommandReason.GetOriginalCommandAuxilleryData();
}

EMacroCommandSecondaryType FCPUPlayerTrainingInfo::GetActualCommand() const
{
	return ActualCommandType;
}

uint8 FCPUPlayerTrainingInfo::GetRawAuxilleryInfo() const
{
	return AuxilleryInfo;
}

EMacroCommandType FCPUPlayerTrainingInfo::GetOriginalCommandReasonForProduction() const
{
	return OriginalCommand_ReasonForProduction;
}

uint8 FCPUPlayerTrainingInfo::GetOriginalCommandAuxilleryInfo() const
{
	return OriginalCommand_AuxilleryInfo;
}


//======================================================================================
//	>FBasicDecalInfo
//======================================================================================

FBasicDecalInfo::FBasicDecalInfo() 
	: Decal(nullptr) 
	, Radius(300.f)
{
}

FBasicDecalInfo::FBasicDecalInfo(UMaterialInterface * InDecal, float InRadius) 
	: Decal(InDecal) 
	, Radius(InRadius)
{
}

UMaterialInterface * FBasicDecalInfo::GetDecal() const
{
	return Decal;
}

float FBasicDecalInfo::GetRadius() const
{
	return Radius;
}


//======================================================================================
//	>FContextButtonInfo
//======================================================================================

FContextButtonInfo::FContextButtonInfo(const FText & InName, EContextButton ButtonType,
	bool bIsIssuedInstantly, bool bIsIssuedToAllSelected, bool bInIsRangeRequirementStrict, 
	bool bInCommandStopsMovement, bool bInUseAnimation, EAnimation InAnimationType, 
	EAbilityMouseAppearance InMouseAppearance, bool bInRequiresSelectableTarget, 
	bool bInCanTargetEnemies, bool bInCanTargetFriendlies, bool bInCanTargetSelf, float Cooldown, 
	float MaxRange, UMaterialInterface  * AcceptableLocDecal, float AcceptableDecalRadius, 
	TSubclassOf <AAbilityBase> Effect_BP)
{
	Name = InName;
	HoveredSound = nullptr;
	PressedByLMBSound = nullptr;
	PressedByRMBSound = nullptr;
	this->ButtonType = ButtonType;
	this->bIsIssuedInstantly = bIsIssuedInstantly;
	this->bIsIssuedToAllSelected = bIsIssuedToAllSelected;
	bIsRangeRequirementStrict = bInIsRangeRequirementStrict;
	bCommandStopsMovement = true;/*bInCommandStopsMovement*/ // While not supported
	bUseAnimation = bInUseAnimation;
	AnimationType = InAnimationType;
	MouseAppearance = InMouseAppearance;
	bRequiresSelectableTarget = bInRequiresSelectableTarget;
	bCanTargetEnemies = bInCanTargetEnemies;
	bCanTargetFriendlies = bInCanTargetFriendlies;
	bCanTargetSelf = bInCanTargetSelf;
	AcceptableTargets.Emplace(ETargetingType::Default);
	this->Cooldown = Cooldown;
	InitialCooldown = 0.f;
	this->MaxRange = MaxRange;
	AcceptableLocationDecalInfo = FBasicDecalInfo(AcceptableLocDecal, AcceptableDecalRadius);
	this->Effect_BP = Effect_BP;

#if WITH_EDITOR
	OnPostEdit(ButtonType);
#endif
}

FContextButtonInfo::FContextButtonInfo(EContextButton InType)
{
	ButtonType = InType;
	Name = FText::FromString(ENUM_TO_STRING(EContextButton, InType));
	MostlyDefaultValues();
}

FContextButtonInfo::FContextButtonInfo()
{
	ButtonType = EContextButton::None;
	MostlyDefaultValues();
}

void FContextButtonInfo::MostlyDefaultValues()
{
	if (Name.EqualTo(FText::GetEmpty()))
	{
		Name = FText::FromString("Default name");
	}
	HoveredSound = nullptr;
	PressedByLMBSound = nullptr;
	PressedByRMBSound = nullptr;
	Description = FText::FromString("Default description");
	bIsIssuedInstantly = false;
	bIsIssuedToAllSelected = false;
	bCommandStopsMovement = true;
	bUseAnimation = false;
	bUsePreparationAnimation = false;
	AnimationType = EAnimation::None;
	PreparationAnimationType_Building = EBuildingAnimation::None;
	AnimationType_Building = EBuildingAnimation::None;
	MouseAppearance = EAbilityMouseAppearance::NoChange;
	bRequiresSelectableTarget = false;
	bCanTargetEnemies = true;
	bCanTargetFriendlies = false;
	bCanTargetSelf = true;
	AcceptableTargets.Emplace(ETargetingType::Default);
	Cooldown = 5.f;
	InitialCooldown = 0.f;
	PreparationAnimPlayPoint = 3.f;
	MaxRange = 1500.f;
	Effect_BP = nullptr;
	AcceptableLocationDecalInfo = FBasicDecalInfo();
	UnusableLocationDecalInfo = FBasicDecalInfo();

#if WITH_EDITOR
	OnPostEdit(ButtonType);
#endif
}

void FContextButtonInfo::SetInitialType(EContextButton InType)
{
	ButtonType = InType;
}

void FContextButtonInfo::AddAcceptableTargetFName(const FName & InName)
{
	AcceptableTargetFNames.Emplace(InName);
}

void FContextButtonInfo::SetupHardwareCursors(URTSGameInstance * GameInst)
{
	if (!bIsIssuedInstantly && MouseAppearance == EAbilityMouseAppearance::CustomMouseCursor)
	{
		const EMouseCursorType MaxEnumValue = static_cast<EMouseCursorType>(static_cast<int32>(EMouseCursorType::z_ALWAYS_LAST_IN_ENUM) + 1);
		
		/* Use the default cursor specified in the game instance if no cursor is set */
		if (DefaultMouseCursor == EMouseCursorType::None || DefaultMouseCursor == MaxEnumValue)
		{
			DefaultMouseCursor = GameInst->GetAbilityDefaultMouseCursor_Default();
		}
		if (AcceptableTargetMouseCursor == EMouseCursorType::None || AcceptableTargetMouseCursor == MaxEnumValue)
		{
			AcceptableTargetMouseCursor = GameInst->GetAbilityDefaultMouseCursor_AcceptableTarget();
		}
		if (UnacceptableTargetMouseCursor == EMouseCursorType::None || UnacceptableTargetMouseCursor == MaxEnumValue)
		{
			UnacceptableTargetMouseCursor = GameInst->GetAbilityDefaultMouseCursor_UnacceptableTarget();
		}

		DefaultMouseCursor_Info = GameInst->GetMouseCursorInfo(DefaultMouseCursor);
		AcceptableTargetMouseCursor_Info = GameInst->GetMouseCursorInfo(AcceptableTargetMouseCursor);
		UnacceptableTargetMouseCursor_Info = GameInst->GetMouseCursorInfo(UnacceptableTargetMouseCursor);

		DefaultMouseCursor_Info.SetFullPath();
		AcceptableTargetMouseCursor_Info.SetFullPath();
		UnacceptableTargetMouseCursor_Info.SetFullPath();
	}
}

bool operator<(const FContextButtonInfo & Info1, const FContextButtonInfo & Info2)
{
	const uint8 Int1 = static_cast<uint8>(Info1.ButtonType);
	const uint8 Int2 = static_cast<uint8>(Info2.ButtonType);

	return Int1 < Int2;
}

const FText & FContextButtonInfo::GetName() const
{
	/* Buttons like these should display the name of the thing being produced instead */
	assert(ButtonType != EContextButton::BuildBuilding && ButtonType != EContextButton::Train
		&& ButtonType != EContextButton::Upgrade);
	return Name;
}

bool FContextButtonInfo::PassesCommandTimeRangeCheck(ISelectable * AbilityUser, const FVector & TargetLocation) const
{
	return !bIsRangeRequirementStrict || MaxRange <= 0.f || Statics::IsSelectableInRangeForAbility(*this, AbilityUser, TargetLocation);
}

bool FContextButtonInfo::PassesCommandTimeRangeCheck(ISelectable * AbilityUser, ISelectable * Target) const
{
	return !bIsRangeRequirementStrict || MaxRange <= 0.f || Statics::IsSelectableInRangeForAbility(*this, AbilityUser, Target);
}

bool FContextButtonInfo::PassesAnimNotifyTargetVisibilityCheck(AActor * AbilityTarget) const
{
	/* For now this always passes. Tbh though we should really check with fog manager. 
	Remember this func is ment to take into account both fog and stealth. Also I have not 
	added any variables to this struct for this function */
	return true;
}

AAbilityBase * FContextButtonInfo::GetEffectActor(ARTSGameState * GameState) const
{
	return GameState->GetAbilityEffectActor(ButtonType);
}


//======================================================================================
//	>FCommanderAbilityInfo
//======================================================================================

FCommanderAbilityInfo::FCommanderAbilityInfo() 
	: Name(FText::FromString("Default name")) 
	, NormalImage(FSlateBrush()) 
	, HoveredImage(FSlateBrush())
	, PressedImage(FSlateBrush())
	, HighlightedImage(FSlateBrush())
	, HoveredSound(nullptr) 
	, PressedByLMBSound(nullptr) 
	, PressedByRMBSound(nullptr) 
	, Description(FText::FromString("Default description")) 
	, Type(ECommanderAbility::None) 
	, TargetingMethod(EAbilityTargetingMethod::DoesNotRequireAnyTarget) 
	, bCanTargetInsideFog(true) 
	, bCanTargetEnemies(true) 
	, bCanTargetFriendlies(false) 
	, bCanTargetSelf(false) 
	, Cooldown(2.f) 
	, InitialCooldown(0.f) 
	, NumCharges(-1) 
	, Effect_BP(nullptr) 
	, MouseAppearance(EAbilityMouseAppearance::NoChange) 
	, DefaultMouseCursor(EMouseCursorType::None) 
	, AcceptableTargetMouseCursor(EMouseCursorType::None)
	, UnacceptableTargetMouseCursor(EMouseCursorType::None) 
	, AcceptableLocationDecalInfo(FBasicDecalInfo()) 
	, UnusableLocationDecalInfo(FBasicDecalInfo())
{
#if WITH_EDITOR
	OnPostEdit();
#endif
}

void FCommanderAbilityInfo::SetType(ECommanderAbility InAbilityType)
{
	Type = InAbilityType;
}

void FCommanderAbilityInfo::PopulateAcceptableTargetFNames()
{
	if (TargetingMethod == EAbilityTargetingMethod::RequiresSelectable
		|| TargetingMethod == EAbilityTargetingMethod::RequiresWorldLocationOrSelectable)
	{
		AcceptableSelectableTargetFNames.Reserve(AcceptableSelectableTargets.Num());
		for (const auto & Elem : AcceptableSelectableTargets)
		{
			AcceptableSelectableTargetFNames.Emplace(Statics::GetTargetingType(Elem));
		}
	}
}

void FCommanderAbilityInfo::SetupHardwareCursors(URTSGameInstance * GameInst)
{
	if (MouseAppearance == EAbilityMouseAppearance::CustomMouseCursor 
		&& 
		(TargetingMethod == EAbilityTargetingMethod::RequiresSelectable 
		|| TargetingMethod == EAbilityTargetingMethod::RequiresWorldLocation 
		|| TargetingMethod == EAbilityTargetingMethod::RequiresWorldLocationOrSelectable))
	{
		const EMouseCursorType MaxEnumValue = static_cast<EMouseCursorType>(static_cast<int32>(EMouseCursorType::z_ALWAYS_LAST_IN_ENUM) + 1);

		/* Use the default cursor specified in the game instance if no cursor is set */
		if (DefaultMouseCursor == EMouseCursorType::None || DefaultMouseCursor == MaxEnumValue)
		{
			DefaultMouseCursor = GameInst->GetAbilityDefaultMouseCursor_Default();
		}
		if (AcceptableTargetMouseCursor == EMouseCursorType::None || AcceptableTargetMouseCursor == MaxEnumValue)
		{
			AcceptableTargetMouseCursor = GameInst->GetAbilityDefaultMouseCursor_AcceptableTarget();
		}
		if (UnacceptableTargetMouseCursor == EMouseCursorType::None || UnacceptableTargetMouseCursor == MaxEnumValue)
		{
			UnacceptableTargetMouseCursor = GameInst->GetAbilityDefaultMouseCursor_UnacceptableTarget();
		}

		DefaultMouseCursor_Info = GameInst->GetMouseCursorInfo(DefaultMouseCursor);
		AcceptableTargetMouseCursor_Info = GameInst->GetMouseCursorInfo(AcceptableTargetMouseCursor);
		UnacceptableTargetMouseCursor_Info = GameInst->GetMouseCursorInfo(UnacceptableTargetMouseCursor);

		DefaultMouseCursor_Info.SetFullPath();
		AcceptableTargetMouseCursor_Info.SetFullPath();
		UnacceptableTargetMouseCursor_Info.SetFullPath();
	}
}

#if WITH_EDITOR
void FCommanderAbilityInfo::OnPostEdit()
{
	bCanEditWorldLocationTargetingProperties = (TargetingMethod == EAbilityTargetingMethod::RequiresWorldLocation)
		|| (TargetingMethod == EAbilityTargetingMethod::RequiresWorldLocationOrSelectable);

	bCanEditSingleTargetProperties = (TargetingMethod == EAbilityTargetingMethod::RequiresPlayer)
		|| (TargetingMethod == EAbilityTargetingMethod::RequiresSelectable)
		|| (TargetingMethod == EAbilityTargetingMethod::RequiresWorldLocationOrSelectable);

	bCanEditSelectableTargetingProperties = (TargetingMethod == EAbilityTargetingMethod::RequiresSelectable) 
		|| (TargetingMethod == EAbilityTargetingMethod::RequiresWorldLocationOrSelectable);

	bCanEditMouseCursorProperties = (MouseAppearance == EAbilityMouseAppearance::CustomMouseCursor);

	bCanEditDecalProperties = (MouseAppearance == EAbilityMouseAppearance::HideAndShowDecal);
}
#endif

UCommanderAbilityBase * FCommanderAbilityInfo::GetAbilityObject(ARTSGameState * GameState) const
{
	return GameState->GetCommanderAbilityEffectObject(Type);
}


//======================================================================================
//	>FCommanderSkillTreePrerequisiteArrayEntry
//======================================================================================

FString FCommanderSkillTreePrerequisiteArrayEntry::ToString() const
{
	return "Type: " + ENUM_TO_STRING(ECommanderSkillTreeNodeType, PrerequisiteType)
		+ ", rank required: " + FString::FromInt(PrerequisiteRank);
}


//======================================================================================
//	>FCommanderAbilityTreeNodeSingleRankInfo
//======================================================================================

FCommanderAbilityTreeNodeSingleRankInfo::FCommanderAbilityTreeNodeSingleRankInfo()
	: AbilityType(ECommanderAbility::None) 
	, UnlockRank(1)
	, Cost(1) 
	, AbilityInfo(nullptr)
{
}


//======================================================================================
//	>FCommanderAbilityTreeNodeInfo
//======================================================================================

FCommanderAbilityTreeNodeInfo::FCommanderAbilityTreeNodeInfo()
	: Type(ECommanderSkillTreeNodeType::None) 
	, bOnlyExecuteOnAquired(false)
{
}


void FCommanderAbilityTreeNodeInfo::SetupInfo(URTSGameInstance * GameInst, ECommanderSkillTreeNodeType InNodeType)
{
	// Set this cause I think I don't do it anywhere else
	Type = InNodeType;
	
	uint8 LastAbilityRankUnlockRank = 0;
	for (int32 i = 0; i < Abilities.Num(); ++i)
	{
		UE_CLOG(Abilities[i].GetUnlockRank() < LastAbilityRankUnlockRank, RTSLOG, Fatal,
			TEXT("Commander ability node [%s]'s Abilities array entry at index [%d] has an unlock "
				"rank lower than the previous entry. Please make it equal to or greater than the "
				"previous array entry"), TO_STRING(ECommanderSkillTreeNodeType, InNodeType), i);

		LastAbilityRankUnlockRank = Abilities[i].GetUnlockRank();

		Abilities[i].SetAbilityInfo(&GameInst->GetCommanderAbilityInfo(Abilities[i].GetAbilityType()));
	}
}


//======================================================================================
//	>FUnaquiredCommanderAbilityState
//======================================================================================

FUnaquiredCommanderAbilityState::FUnaquiredCommanderAbilityState(const FCommanderAbilityTreeNodeInfo * InNodeInfo)
{
	NodeInfo = InNodeInfo;
	
	// Fill prerequisites container
	MissingPrerequisites.Reserve(InNodeInfo->GetPrerequisites().Num());
	for (const auto & Elem : InNodeInfo->GetPrerequisites())
	{
		MissingPrerequisites.Emplace(Elem);
	}
}

void FUnaquiredCommanderAbilityState::OnAnotherAbilityAquired(const FCommanderSkillTreePrerequisiteArrayEntry & AquiredAbilityTuple)
{
	MissingPrerequisites.Remove(AquiredAbilityTuple);
}


//======================================================================================
//	>FAttachInfo
//======================================================================================

FAttachInfo::FAttachInfo()
{
	ComponentName = NAME_None;
	SceneComponent = nullptr;
	SocketName = NAME_None;
}

USceneComponent * FAttachInfo::GetComponent() const
{
	return SceneComponent;
}

const FName & FAttachInfo::GetSocketName() const
{
	return SocketName;
}

const FTransform & FAttachInfo::GetAttachTransform() const
{
	return AttachTransform;
}

#if WITH_EDITOR
void FAttachInfo::OnPostEdit(AActor * InOwningActor)
{
	/* Search for the component by name and set UPROP pointer */

	USceneComponent * Comp = nullptr;

	if (ComponentName != NAME_None)
	{
		// Search by comparing FNames (INTs), not strings
		for (TFieldIterator<UObjectPropertyBase>It(InOwningActor->GetClass()); It; ++It)
		{
			if (It->GetFName() == ComponentName)
			{
				Comp = Cast<USceneComponent>(It->GetObjectPropertyValue_InContainer(InOwningActor));
				break;
			}
		}
	}

	/* Use root component if no component found */
	if (Comp == nullptr)
	{
		Comp = InOwningActor->GetRootComponent();
	}

	SceneComponent = Comp;
}
#endif


//======================================================================================
//	>FInventoryItemInfo
//======================================================================================

FInventoryItemInfo::FInventoryItemInfo()
	: DisplayName(FText::FromString("SHOULD NEVER SEE THIS"))
{
	//assert(0); // Call the ctor with params instead. Apparently though this is called at some 
	// point. Perhaps look into this. Possibly nothing I can do about it
}

FInventoryItemInfo::FInventoryItemInfo(const FString & InDisplayName) 
	: DisplayName(FText::FromString(InDisplayName)) 
	, Sound_Hovered(nullptr)
	, Sound_PressedByLMB(nullptr)
	, Sound_PressedByRMB(nullptr)
	, Description(FText::FromString("Default description")) 
	, StatChangeText(FText::FromString("Increases nothing by anything"))
	, FlavorText(FText::FromString("Default flavour text"))
	, AquireSound(nullptr) 
	, OnDroppedAnim(nullptr)
	, StackQuantityLimit(0) 
	, MaxNumPerStack(1) 
	, OnAquiredFunctionPtr(nullptr) 
	, OnRemovedFunctionPtr(nullptr)
	, bDropsOnDeath(false) 
	, bCanBeExplicitlyDropped(true) 
	, bCanBeSold(true) 
	, bIsUsable(false) 
	, UseAbilityType(EContextButton::None) 
	, NumCharges(1) 
	, NumChargesChangedBehavior(EInventoryItemNumChargesChangedBehavior::DestroyAtZeroCharges)
{
	// Possibly call post edit here
}

void FInventoryItemInfo::ClearIngredients()
{
	IngredientsTMap.Empty();
}

void FInventoryItemInfo::AddCombinationResult(EInventoryItem InItemType)
{
	WhatThisCanMake.AddUnique(InItemType);
}

void FInventoryItemInfo::SetBehaviorFunctionPointers(InventoryItem_OnAquired OnAquiredPtr, 
	InventoryItem_OnRemoved OnRemovedPtr, InventoryItem_OnNumChargesChanged OnNumChargesChangedPtr)
{
	OnAquiredFunctionPtr = OnAquiredPtr;
	OnRemovedFunctionPtr = OnRemovedPtr;
	OnNumChargesChangedFunctionPtr = OnNumChargesChangedPtr;

	/* If the user has specified they want custom on zero charge behavior then the zero charge 
	function pointer better have been non-null. If it is null then we will default zero charge 
	behavior to "Destroy" */
	if (OnNumChargesChangedFunctionPtr == nullptr)
	{
		if (NumChargesChangedBehavior == EInventoryItemNumChargesChangedBehavior::CustomBehavior)
		{
			NumChargesChangedBehavior = EInventoryItemNumChargesChangedBehavior::DestroyAtZeroCharges;
		}
	}
	else
	{
		if (NumChargesChangedBehavior != EInventoryItemNumChargesChangedBehavior::CustomBehavior)
		{
			/* Maybe log something saying like "Function pointer being set but will not be used 
			because zero charge behavior is not set to CustomBehavior" */
		}
	}
}

void FInventoryItemInfo::SetUseAbilityInfo(const FContextButtonInfo * AbilityInfo)
{
	UseAbilityInfo = AbilityInfo;
}

void FInventoryItemInfo::RunOnAquiredLogic(ISelectable * Owner, int32 Quantity, EItemAquireReason AquireReason) const
{
	/* Eh, we will bracnh on null. Null basically means the item has no OnAquired logic e.g. clarity 
	potions in dota */
	if (OnAquiredFunctionPtr != nullptr)
	{
		for (int32 i = 0; i < Quantity; ++i)
		{
			// Call function pointed to by function pointer
			(*(OnAquiredFunctionPtr))(Owner, AquireReason);
		}
	}
}

void FInventoryItemInfo::RunOnRemovedLogic(ISelectable * Owner, int32 Quantity, EItemRemovalReason RemovalReason) const
{
	/* Just like OnAquired we allow null and take it to mean "do nothing" */
	if (OnRemovedFunctionPtr != nullptr)
	{
		for (int32 i = 0; i < Quantity; ++i)
		{
			// Call function pointed to by function pointer
			(*(OnRemovedFunctionPtr))(Owner, RemovalReason);
		}
	}
}

void FInventoryItemInfo::RunCustomOnNumChargesChangedLogic(ISelectable * Owner, FInventory * Inventory,
	FInventorySlotState & InvSlot, int16 ChangeAmount, EItemChangesNumChargesReason ChangeReason) const
{
	// Call function pointed to by function pointer
	(*(OnNumChargesChangedFunctionPtr))(Owner, Inventory, InvSlot, ChangeAmount, ChangeReason);
}

const FText & FInventoryItemInfo::GetDisplayName() const
{
	return DisplayName;
}

const FText & FInventoryItemInfo::GetDescription() const
{
	return Description;
}

const FText & FInventoryItemInfo::GetStatChangeText() const
{
	return StatChangeText;
}

const FText & FInventoryItemInfo::GetFlavorText() const
{
	return FlavorText;
}

const FSlateBrush & FInventoryItemInfo::GetDisplayImage() const
{
	return DisplayImage;
}

const FSlateBrush & FInventoryItemInfo::GetDisplayImage_Hovered() const
{
	return DisplayImage_Hovered;
}

const FSlateBrush & FInventoryItemInfo::GetDisplayImage_Pressed() const
{
	return DisplayImage_Pressed;
}

const FSlateBrush & FInventoryItemInfo::GetDisplayImage_Highlighted() const
{
	return DisplayImage_Highlighted;
}

USoundBase * FInventoryItemInfo::GetUISound_Hovered() const
{
	return Sound_Hovered;
}

USoundBase * FInventoryItemInfo::GetUISound_PressedByLMB() const
{
	return Sound_PressedByLMB;
}

USoundBase * FInventoryItemInfo::GetUISound_PressedByRMB() const
{
	return Sound_PressedByRMB;
}

USoundBase * FInventoryItemInfo::GetAquireSound() const
{
	return AquireSound;
}

const FParticleInfo_Attach & FInventoryItemInfo::GetAquireParticles() const
{
	return AquireParticles;
}

UAnimMontage * FInventoryItemInfo::GetOnDroppedAnim() const
{
	return OnDroppedAnim;
}

const FMeshInfoBasic & FInventoryItemInfo::GetMeshInfo() const
{
	return MeshInfo;
}

const TArray<int32>& FInventoryItemInfo::GetDefaultCost() const
{
	return DefaultCostArray;
}

EGameWarning FInventoryItemInfo::IsSelectablesTypeAcceptable(ISelectable * Selectable) const
{
	if (bRestrictedToCertainTypes)
	{
		return TypesAllowedToPickUp.Contains(Selectable->GetAttributesBase().GetUnitType()) ? EGameWarning::None : EGameWarning::TypeCannotAquireItem;
	}
	else
	{
		return EGameWarning::None;
	}
}

bool FInventoryItemInfo::CanStack() const
{
	return MaxNumPerStack != 1;
}

int32 FInventoryItemInfo::GetNumStacksLimit() const
{
	return StackQuantityLimit;
}

int32 FInventoryItemInfo::GetStackSize() const
{
	return MaxNumPerStack;
}

bool FInventoryItemInfo::HasNumberOfStacksLimit() const
{
	return (StackQuantityLimit != 0);
}

bool FInventoryItemInfo::HasNumberInStackLimit() const
{
	return (MaxNumPerStack != 0);
}

bool FInventoryItemInfo::IsUsable() const
{	
	/* This might throw if you have flagged an item as usable in editor but did not set its 
	use ability type. Tbh this is kind of aggressive. Could replace return as 
	return bIsUsable && UseAbilityType != EContextButton::None */
	UE_CLOG(bIsUsable && UseAbilityType == EContextButton::None, RTSLOG, Fatal, TEXT("Inventory item type "
		"%s is flagged as usable but its use ability is \'None\'. Set Use Ability Type to something "
		"or set Is Usable to false if it is not a usable item"), TO_STRING(EInventoryItem, ItemType));
	
	return bIsUsable;
}

int32 FInventoryItemInfo::GetNumCharges() const
{
	return NumCharges;
}

bool FInventoryItemInfo::HasUnlimitedNumberOfUsesChecked() const
{
	// What this function returns is only relevant if this is usable
	assert(bIsUsable);
	
	return (NumCharges == -1);
}

bool FInventoryItemInfo::HasUnlimitedNumberOfUses() const
{
	return (NumCharges == -1);
}

const TMap<EInventoryItem, FAtLeastOneInt16>& FInventoryItemInfo::GetIngredients() const
{
	return IngredientsTMap;
}

TMap<EInventoryItem, FAtLeastOneInt16> FInventoryItemInfo::GetIngredientsByValue() const
{
	return IngredientsTMap;
}

const TArray<EInventoryItem>& FInventoryItemInfo::GetItemsMadeFromThis() const
{
	return WhatThisCanMake;
}

#if WITH_EDITOR
void FInventoryItemInfo::OnPostEdit(EInventoryItem ThisItemsType)
{
	// Populate cost array 
	DefaultCostArray.Init(0, Statics::NUM_RESOURCE_TYPES);
	for (const auto & Elem : DefaultCost)
	{
		if (Elem.Key != EResourceType::None)
		{
			DefaultCostArray[Statics::ResourceTypeToArrayIndex(Elem.Key)] = Elem.Value;
		}
	}

	/* Make sure the IngredientsTMap does not contain the item itself */
	if (IngredientsTMap.Contains(ThisItemsType))
	{
		IngredientsTMap.FindAndRemoveChecked(ThisItemsType);
	}
	
	/* What I want to achieve with these is that if an item is usable and can stack then it 
	must have 1 charge and remove itself on zero charges */
	bCanEditMaxNumPerStack = !bIsUsable || (bIsUsable && NumCharges == 1 && NumChargesChangedBehavior == EInventoryItemNumChargesChangedBehavior::DestroyAtZeroCharges);
	bCanEditNumCharges = bIsUsable && (MaxNumPerStack == 1);
	bCanEditZeroChargeBehavior = bIsUsable && (MaxNumPerStack == 1);

	MeshInfo.OnPostEdit();
}
#endif


//======================================================================================
//	>FItemOnDisplayInShopSlot
//======================================================================================

FItemOnDisplayInShopSlot::FItemOnDisplayInShopSlot() 
	: ItemOnDisplay(EInventoryItem::None) 
	, bIsForSale(true) 
	, PurchasesRemaining(-1)
	, QuantityBoughtPerPurchase(1) 
{
}

EInventoryItem FItemOnDisplayInShopSlot::GetItemType() const
{
	return ItemOnDisplay;
}

bool FItemOnDisplayInShopSlot::IsForSale() const
{
	return bIsForSale;
}

int32 FItemOnDisplayInShopSlot::GetQuantityBoughtPerPurchase() const
{
	return QuantityBoughtPerPurchase;
}

int32 FItemOnDisplayInShopSlot::GetPurchasesRemaining() const
{
	return PurchasesRemaining;
}

bool FItemOnDisplayInShopSlot::HasUnlimitedPurchases() const
{
	return (PurchasesRemaining == -1);
}

void FItemOnDisplayInShopSlot::OnPurchase()
{
	if (HasUnlimitedPurchases() == false)
	{
		PurchasesRemaining--;
	}
}


//======================================================================================
//	>FShopInfo
//======================================================================================

FShopInfo::FShopInfo() 
	: WhoCanShopHere(EAffiliation::Owned) 
	, bAcceptsRefunds(true)
	, ShoppingRange(800.f)
{
}

const TArray<FItemOnDisplayInShopSlot>& FShopInfo::GetItems() const
{
	return ItemsOnDisplay;
}

bool FShopInfo::SellsAtLeastOneItem() const
{
	return bAtLeastOneItemOnDisplayIsSold;
}

bool FShopInfo::AcceptsRefunds() const
{
	return SellsAtLeastOneItem() && bAcceptsRefunds;
}

const FItemOnDisplayInShopSlot & FShopInfo::GetSlotInfo(int32 Index) const
{
	return ItemsOnDisplay[Index];
}

bool FShopInfo::CanShopHere(EAffiliation Affiliation) const
{
	return (Affiliation <= WhoCanShopHere);
}

void FShopInfo::GetSelectablesInRangeOfShopOnTeam(UWorld * World, ARTSGameState * GameState,
	TArray < FHitResult > & OutHits, const FVector & ShopLocation, ETeam Team) const
{
	FCollisionObjectQueryParams QueryParams;
	QueryParams.AddObjectTypesToQuery(GameState->GetTeamCollisionChannel(Team));
	Statics::CapsuleSweep(World, OutHits, ShopLocation, QueryParams, ShoppingRange);
}

void FShopInfo::OnPurchase(int32 SlotIndex)
{
	ItemsOnDisplay[SlotIndex].OnPurchase();
}

#if WITH_EDITOR
void FShopInfo::OnPostEdit(ISelectable * OwningSelectable)
{
	/* Limit the number of displayable items to 256 to reduce bandwidth when requesting to 
	buy something */
	const int32 SizeLimit = (int32)UINT8_MAX + 1;
	if (ItemsOnDisplay.Num() > SizeLimit)
	{
		// Remove elements from the highest indices. Haven't really tested whether this code 
		// does what I think it should
		ItemsOnDisplay.RemoveAt(SizeLimit, ItemsOnDisplay.Num() - SizeLimit, true);
	}

	// Set bAtLeastOneItemOnDisplayIsSold
	bAtLeastOneItemOnDisplayIsSold = false;
	for (const auto & Elem : ItemsOnDisplay)
	{
		if (Elem.IsForSale())
		{
			bAtLeastOneItemOnDisplayIsSold = true;
			break;
		}
	}

	bCanEditMostProperties = (ItemsOnDisplay.Num() > 0);
}
#endif


//======================================================================================
//	>FInventorySlotState
//======================================================================================

FInventorySlotState::FInventorySlotState() 
	: ItemType(EInventoryItem::None)
	, NumInStackOrNumCharges(-1)
{
}

FInventorySlotState::FInventorySlotState(uint8 InStartingIndex) 
	: ItemType(EInventoryItem::None)
	, NumInStackOrNumCharges(-1)
{
}

EInventoryItem FInventorySlotState::GetItemType() const
{
	return ItemType;
}

bool FInventorySlotState::HasItem() const
{
	return (ItemType != EInventoryItem::None);
}

bool FInventorySlotState::CanItemStack() const
{
	return bCanItemStack;
}

bool FInventorySlotState::DropsOnZeroHealth() const
{
	return bItemDropsOnDeath;
}

bool FInventorySlotState::IsItemUsable() const
{
	return bIsItemUsable;
}

bool FInventorySlotState::HasUnlimitedCharges() const
{
	return bHasUnlimitedCharges;
}

int16 FInventorySlotState::GetNumInStack() const
{	
	/* Added this after I had used this function everywhere.
	
	Here's the deal with this function: if this throws then either check before calling this 
	fucn whether the item can stack 
	OR 
	can remove this assert and use the commented return statement below instead, which is 
	what I have done. Just leaving old code behind for no reason  */
	//assert(bCanItemStack);
	
	//return NumInStackOrNumCharges;
	return bCanItemStack ? NumInStackOrNumCharges : 1;
}

int16 FInventorySlotState::GetNumCharges() const
{
	return NumInStackOrNumCharges;
}

void FInventorySlotState::PutItemIn(EInventoryItem InItem, int32 Quantity, const FInventoryItemInfo & ItemsInfo)
{
	ItemType = InItem;
	NumInStackOrNumCharges = ItemsInfo.IsUsable() && !ItemsInfo.CanStack() ? ItemsInfo.GetNumCharges() : Quantity;

	// Set flags. Probably want to add more as time goes by
	bCanItemStack = ItemsInfo.CanStack();
	bItemDropsOnDeath = ItemsInfo.DropsOnDeath();
	bIsItemUsable = ItemsInfo.IsUsable();
	bHasUnlimitedCharges = ItemsInfo.HasUnlimitedNumberOfUses();
}

void FInventorySlotState::PutItemInForReversal(EInventoryItem InItem, int32 Quantity)
{
	ItemType = InItem;
	NumInStackOrNumCharges = Quantity;
}

void FInventorySlotState::AdjustAmount(int16 AmountToAdjustBy)
{
	NumInStackOrNumCharges += AmountToAdjustBy;

	UE_CLOG(NumInStackOrNumCharges < 0, RTSLOG, Fatal, TEXT("NumInStackOrNumCharges: %d, " 
		"AmountToAdjustBy: %d"), NumInStackOrNumCharges, AmountToAdjustBy);
}

void FInventorySlotState::RemoveAmount(int16 AmountToRemove)
{
	assert(bCanItemStack);
	
	NumInStackOrNumCharges -= AmountToRemove;
	
	// Assumed we've already checked that we didn't either start below 0 or go below 0
	assert(NumInStackOrNumCharges >= 0);
}

void FInventorySlotState::RemoveCharge()
{
	assert(!bHasUnlimitedCharges);
	
	NumInStackOrNumCharges--;
}

void FInventorySlotState::SetItemTypeToNone()
{
	ItemType = EInventoryItem::None;
}

void FInventorySlotState::ReduceQuantityToZero()
{
	ItemType = EInventoryItem::None;
	NumInStackOrNumCharges = 0;
}

bool FInventorySlotState::IsOnCooldown(const FTimerManager & WorldTimerManager) const
{
	return WorldTimerManager.GetTimerRemaining(TimerHandle_ItemUseCooldown) > 0.f;
}

float FInventorySlotState::GetUseCooldownRemaining(FTimerManager & WorldTimerManager) const
{
	// FTimerManager::IsTimerPending is unexpectedly not const
	if (WorldTimerManager.IsTimerPending(TimerHandle_ItemUseCooldown))
	{
		return WorldTimerManager.GetTimerRemaining(TimerHandle_ItemUseCooldown);
	}
	else
	{
		return 0.f;
	}
}

float FInventorySlotState::GetUseCooldownRemainingChecked(const FTimerManager & WorldTimerManager) const
{
	return WorldTimerManager.GetTimerRemaining(TimerHandle_ItemUseCooldown);
}

FString FInventorySlotState::ToString() const
{
	FString S;
	
	S += "Item type: " + ENUM_TO_STRING(EInventoryItem, ItemType);
	S += "\nNum in stack or quantity: " + FString::FromInt(NumInStackOrNumCharges);

	return S;
}


//======================================================================================
//	>FInventoryItemQuantity
//======================================================================================

FInventoryItemQuantity::FInventoryItemQuantity()
{
	// Commented because crash here on startup after migrating to 4.21
	//assert(0);
}

FInventoryItemQuantity::FInventoryItemQuantity(int16 InStackSize)
	: NumStacks(1) 
	, Num(InStackSize)
{
}

uint8 FInventoryItemQuantity::GetNumStacks() const
{
	return NumStacks;
}

int16 FInventoryItemQuantity::GetTotalNum() const
{
	return Num;
}

void FInventoryItemQuantity::IncrementNumStacksAndAdjustNum(int16 AdjustAmount)
{
	Num += AdjustAmount;
	NumStacks++;
}

void FInventoryItemQuantity::AdjustAmount(int16 AmountToAdjustBy)
{
	Num += AmountToAdjustBy;
}

void FInventoryItemQuantity::DecrementNumStacksAndAdjustNum(int16 AmountLost)
{
	Num -= AmountLost;
	NumStacks--;
}


//======================================================================================
//	>FInventoryItemQuantityPair
//======================================================================================

FInventoryItemQuantityPair::FInventoryItemQuantityPair()
{
	// Commented because crash here on startup after migrating to 4.21
	//assert(false);
}


// Just a random TODO: nothing for instigating dropping inventory items has been implemented
// I'm thinking either a drag and drop or possibly right click button, although right click 
// is sell to shop... could make right click drop and double right click sell to shop


//======================================================================================
//	>FInventory
//======================================================================================

bool FInventory::AtNumStacksLimit(EInventoryItem ItemType, const FInventoryItemInfo & Item) const
{
	if (Item.HasNumberOfStacksLimit())
	{
		// Here we assume that a quantity limit of 0 is not possible, minimum is 1
		return Quantities.Contains(ItemType) ? (Item.GetNumStacksLimit() == Quantities[ItemType].GetNumStacks()) : false;
	}
	else
	{
		return false;
	}
}

bool FInventory::AtTotalNumLimit(EInventoryItem ItemType, int32 Quantity, const FInventoryItemInfo & Item) const
{
	if (Item.HasNumberOfStacksLimit())
	{
		if (Item.HasNumberInStackLimit())
		{
			if (Quantities.Contains(ItemType))
			{
				return (Quantities[ItemType].GetTotalNum() + Quantity) <= (Item.GetNumStacksLimit() * Item.GetStackSize());
			}
			else
			{
				return (Quantity) <= (Item.GetNumStacksLimit() * Item.GetStackSize());
			}
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}

void FInventory::RunOnItemAquiredLogic(ISelectable * InventoryOwner, EInventoryItem ItemAquired, 
	int32 Quantity, EItemAquireReason AquireReason, const FInventoryItemInfo & ItemsInfo)
{
	ItemsInfo.RunOnAquiredLogic(InventoryOwner, Quantity, AquireReason);
}

void FInventory::RunOnItemRemovedLogic(ISelectable * InventoryOwner, EInventoryItem ItemRemoved, 
	int32 Quantity, EItemRemovalReason RemovalReason, const FInventoryItemInfo & ItemsInfo)
{
	ItemsInfo.RunOnRemovedLogic(InventoryOwner, Quantity, RemovalReason);
}

void FInventory::RunOnNumChargesChangedLogic(ISelectable * InventoryOwner, EInventoryItem ItemType, 
	FInventorySlotState & Slot, int16 ChargeChangeAmount, EItemChangesNumChargesReason ReasonForChangingNumCharges,
	const FInventoryItemInfo & ItemsInfo, URTSHUD * HUDWidget)
{
	if (ItemsInfo.GetNumChargesChangedBehavior() == EInventoryItemNumChargesChangedBehavior::DestroyAtZeroCharges)
	{
		if (ItemsInfo.GetNumCharges() == 1)
		{
			/* Item has now magically been destroyed */
			
			/* Check if stack is now empty. Because we know each item in the stack only has 1 
			charge this is an ok way to check it */
			if (Slot.GetNumCharges() == 0)
			{
				NumSlotsOccupied--;
				Quantities[ItemType].DecrementNumStacksAndAdjustNum(ChargeChangeAmount);
				Slot.SetItemTypeToNone();
			}
			else
			{
				Quantities[ItemType].AdjustAmount(ChargeChangeAmount);
			}
			
			/* Run the OnRemoved logic, once for each charge removed because each item only has 
			1 charge */
			for (int32 i = 0; i < -ChargeChangeAmount; ++i)
			{
				RunOnItemRemovedLogic(InventoryOwner, ItemType, -ChargeChangeAmount,
					EItemRemovalReason::RemovedOnZeroCharges, ItemsInfo);
			}
		}
		else
		{
			if (Slot.GetNumCharges() == 0)
			{
				NumSlotsOccupied--;
				Quantities[ItemType].DecrementNumStacksAndAdjustNum(ChargeChangeAmount);
				Slot.SetItemTypeToNone();

				/* Run the OnRemoved logic */
				RunOnItemRemovedLogic(InventoryOwner, ItemType, 1, EItemRemovalReason::RemovedOnZeroCharges,
					ItemsInfo);
			}
		}
	}
	else if (ItemsInfo.GetNumChargesChangedBehavior() == EInventoryItemNumChargesChangedBehavior::CustomBehavior)
	{
		/* It's important that this function updates this FInventory struct and the 
		Slot param */
		ItemsInfo.RunCustomOnNumChargesChangedLogic(InventoryOwner, this, Slot, ChargeChangeAmount, 
			ReasonForChangingNumCharges);
	}
	else // Assumed always do nothing
	{
		// Nothing to do
	}
}

FInventory::FInventory()
{
#if WITH_EDITORONLY_DATA
	Capacity = 0;
#endif
	NumSlotsOccupied = 0;
}

uint8 FInventory::GetLocalIndexFromServerIndex(int32 ServerIndex) const
{
	return ServerIndexToLocalIndex[ServerIndex]; 
}

const FInventorySlotState & FInventory::GetSlotForDisplayAtIndex(int32 RawIndex) const
{
	return SlotsArray[ServerIndexToLocalIndex[RawIndex]];
}

int32 FInventory::GetCapacity() const
{
	/* Because we always keep the array full. If there's no item in the slot then there is a 
	blank type entry */
	return SlotsArray.Num();
}

int32 FInventory::GetNumSlotsOccupied() const
{
	return NumSlotsOccupied;
}

bool FInventory::AreAllSlotsOccupied() const
{
	return (GetNumSlotsOccupied() == GetCapacity());
}

FInventory::TryAddItemResult FInventory::TryPutItemInInventory(ISelectable * InventoryOwner, 
	bool bIsOwnerSelected, bool bIsOwnerCurrentSelected, EInventoryItem Item, int32 Quantity, 
	const FInventoryItemInfo & ItemsInfo, EItemAquireReason ReasonForAquiring, 
	const URTSGameInstance * GameInst, URTSHUD * HUDWidget)
{
	/* The case where the quantity is larger than the stack size has not been implemented. 
	It probably wouldn't be too hard to implement though. 
	A common time this assert may fire is if you've placed an item on the map but made it's 
	quantity bigger than its stack size */
	assert(ItemsInfo.HasNumberInStackLimit() == false || Quantity <= ItemsInfo.GetStackSize());
	
	/* This happening now means even if selectable has no inventory then we get this warning. No biggie I guess though */
	if (ItemsInfo.IsSelectablesTypeAcceptable(InventoryOwner) == EGameWarning::TypeCannotAquireItem)
	{
		return TryAddItemResult(EGameWarning::TypeCannotAquireItem, 0, Item);
	}

	/* How many SlotsArray elements to iterate over. Even if we reorder the array locally 
	SlotsArray shouls still never have any gaps in it. But if this ever changes then change 
	this value to SlotsArray.Num(). 
	Actually I think this has to be changed because holes can be formed in SlotsArray from 
	removing ingredients */
	const int32 ArraySize = SlotsArray.Num();
	
	if (ItemsInfo.GetItemsMadeFromThis().Num() == 0)
	{
		//========================================================================================
		// Easier case: item does not combine to create anything. 
		//========================================================================================

		// ------ I have copied this whole block from CanEnterInventory ------ 

		if (ItemsInfo.GetStackSize() == 1)
		{
			if (AreAllSlotsOccupied())
			{
				return FInventory::TryAddItemResult(EGameWarning::InventoryFull, 0, Item);
			}
			else
			{
				if (AtNumStacksLimit(Item, ItemsInfo) == true)
				{
					return FInventory::TryAddItemResult(EGameWarning::CannotCarryAnymoreOfItem, 0, Item);
				}
				else
				{
					// First free slot is where we will put this item

					for (int32 i = 0; i < SlotsArray.Num(); ++i)
					{
						const FInventorySlotState & Elem = SlotsArray[i];

						if (Elem.HasItem() == false)
						{
							PutItemInSlotChecked(InventoryOwner, true, bIsOwnerSelected, bIsOwnerCurrentSelected, Item, Quantity, i, ItemsInfo, ReasonForAquiring, HUDWidget);
							return FInventory::TryAddItemResult(EGameWarning::None, i, Item);
						}
					}

					// Did not find free slot but we checked previously that our inventory was not full
					assert(0);

					// Not ment to ever get here
					return FInventory::TryAddItemResult(EGameWarning::InventoryFull, 0, Item);
				}
			}
		}
		else
		{
			const bool bHasStackSizeLimit = ItemsInfo.HasNumberInStackLimit();

			if (ItemsInfo.HasNumberOfStacksLimit() == false)
			{
				if (AreAllSlotsOccupied() == true)
				{
					/* Find a stack to put item onto */

					for (int32 i = 0; i < ArraySize; ++i)
					{
						const FInventorySlotState & Elem = SlotsArray[i];

						if (Elem.GetItemType() == Item)
						{
							// See if this stack can accept adding this purchase onto it
							if (bHasStackSizeLimit)
							{
								if (Elem.GetNumInStack() + Quantity <= ItemsInfo.GetStackSize())
								{
									PutItemInSlotChecked(InventoryOwner, true, bIsOwnerSelected, bIsOwnerCurrentSelected, Item, Quantity, i, ItemsInfo, ReasonForAquiring, HUDWidget);
									return FInventory::TryAddItemResult(EGameWarning::None, i, Item);
								}
							}
							else
							{
								PutItemInSlotChecked(InventoryOwner, true, bIsOwnerSelected, bIsOwnerCurrentSelected, Item, Quantity, i, ItemsInfo, ReasonForAquiring, HUDWidget);
								return FInventory::TryAddItemResult(EGameWarning::None, i, Item);
							}
						}
					}

					// Could not find a stack to put item on to
					return FInventory::TryAddItemResult(EGameWarning::InventoryFull, 0, Item);
				}
				else
				{
					/* Find a slot to put item in. We will put the item on top of an already existing
					stack if it will fit. Otherwise we will put it in the first empty slot */

					int32 FreeIndex = -1;

					for (int32 i = 0; i < SlotsArray.Num(); ++i)
					{
						const FInventorySlotState & Elem = SlotsArray[i];

						if (Elem.GetItemType() == Item)
						{
							// See if this stack can accept adding this purchase onto it
							if (bHasStackSizeLimit)
							{
								if (Elem.GetNumInStack() + Quantity <= ItemsInfo.GetStackSize())
								{
									PutItemInSlotChecked(InventoryOwner, true, bIsOwnerSelected, bIsOwnerCurrentSelected, Item, Quantity, i, ItemsInfo, ReasonForAquiring, HUDWidget);
									return FInventory::TryAddItemResult(EGameWarning::None, i, Item);
								}
							}
							else
							{
								PutItemInSlotChecked(InventoryOwner, true, bIsOwnerSelected, bIsOwnerCurrentSelected, Item, Quantity, i, ItemsInfo, ReasonForAquiring, HUDWidget);
								return TryAddItemResult(EGameWarning::None, i, Item);
							}
						}
						else if (Elem.HasItem() == false)
						{
							if (FreeIndex == -1)
							{
								FreeIndex = i;
							}
						}
					}

					/* If here then none of the stacks we already have of this item can accept the item
					so put the item in a new stack */

					// We already checked the inventory wasn't full so we should not get this
					assert(FreeIndex != -1);

					PutItemInSlotChecked(InventoryOwner, true, bIsOwnerSelected, bIsOwnerCurrentSelected, Item, Quantity, FreeIndex, ItemsInfo, ReasonForAquiring, HUDWidget);
					return FInventory::TryAddItemResult(EGameWarning::None, FreeIndex, Item);
				}
			}
			else
			{
				if (AtNumStacksLimit(Item, ItemsInfo))
				{
					// Must look for an existing stack to put this item on
					for (int32 i = 0; i < ArraySize; ++i)
					{
						const FInventorySlotState & Elem = SlotsArray[i];

						if (Elem.GetItemType() == Item)
						{
							if (bHasStackSizeLimit)
							{
								if (Elem.GetNumInStack() + Quantity <= ItemsInfo.GetStackSize())
								{
									PutItemInSlotChecked(InventoryOwner, true, bIsOwnerSelected, bIsOwnerCurrentSelected, Item, Quantity, i, ItemsInfo, ReasonForAquiring, HUDWidget);
									return FInventory::TryAddItemResult(EGameWarning::None, i, Item);
								}
							}
							else
							{
								PutItemInSlotChecked(InventoryOwner, true, bIsOwnerSelected, bIsOwnerCurrentSelected, Item, Quantity, i, ItemsInfo, ReasonForAquiring, HUDWidget);
								return FInventory::TryAddItemResult(EGameWarning::None, i, Item);
							}
						}
					}

					/* If here then could not find a stack to put item on top of */
					return FInventory::TryAddItemResult(EGameWarning::CannotCarryAnymoreOfItem, 0, Item);
				}
				else
				{
					if (AreAllSlotsOccupied())
					{
						// Must look for an existing stack to put item on 
						for (int32 i = 0; i < ArraySize; ++i)
						{
							const FInventorySlotState & Elem = SlotsArray[i];

							if (Elem.GetItemType() == Item)
							{
								if (bHasStackSizeLimit)
								{
									if (Elem.GetNumInStack() + Quantity <= ItemsInfo.GetStackSize())
									{
										PutItemInSlotChecked(InventoryOwner, true, bIsOwnerSelected, bIsOwnerCurrentSelected, Item, Quantity, i, ItemsInfo, ReasonForAquiring, HUDWidget);
										return FInventory::TryAddItemResult(EGameWarning::None, i, Item);
									}
								}
								else
								{
									PutItemInSlotChecked(InventoryOwner, true, bIsOwnerSelected, bIsOwnerCurrentSelected, Item, Quantity, i, ItemsInfo, ReasonForAquiring, HUDWidget);
									return FInventory::TryAddItemResult(EGameWarning::None, i, Item);
								}
							}
						}

						/* If here then could not find a stack to put item on top of */
						return FInventory::TryAddItemResult(EGameWarning::InventoryFull, 0, Item);
					}
					else
					{
						/* Look for an existing stack to put item on. If none can be found then
						place item in a new slot */

						int32 FreeIndex = -1;

						for (int32 i = 0; i < SlotsArray.Num(); ++i)
						{
							const FInventorySlotState & Elem = SlotsArray[i];

							if (Elem.GetItemType() == Item)
							{
								if (bHasStackSizeLimit)
								{
									if (Elem.GetNumInStack() + Quantity <= ItemsInfo.GetStackSize())
									{
										PutItemInSlotChecked(InventoryOwner, true, bIsOwnerSelected, bIsOwnerCurrentSelected, Item, Quantity, i, ItemsInfo, ReasonForAquiring, HUDWidget);
										return FInventory::TryAddItemResult(EGameWarning::None, i, Item);
									}
								}
								else
								{
									PutItemInSlotChecked(InventoryOwner, true, bIsOwnerSelected, bIsOwnerCurrentSelected, Item, Quantity, i, ItemsInfo, ReasonForAquiring, HUDWidget);
									return FInventory::TryAddItemResult(EGameWarning::None, i, Item);
								}
							}
							else if (Elem.HasItem() == false)
							{
								if (FreeIndex == -1)
								{
									FreeIndex = i;
								}
							}
						}

						// We already checked inventory was not full so this should not throw
						assert(FreeIndex != -1);

						PutItemInSlotChecked(InventoryOwner, true, bIsOwnerSelected, bIsOwnerCurrentSelected, Item, Quantity, FreeIndex, ItemsInfo, ReasonForAquiring, HUDWidget);
						return FInventory::TryAddItemResult(EGameWarning::None, FreeIndex, Item);
					}
				}
			}
		}
	}
	else
	{
		//========================================================================================
		//	Hard case: item is an ingredient
		//----------------------------------------------------------------------------------------
		//	Here we need to check if we can make anything.
		//
		//	I can't remember how dota did it. I think you needed a spare inventory slot if you 
		//	wanted to buy an item that would combine into something. e.g. no matter what if your 
		//	inventory is full you cannot buy an item even if the purchase would result in a new 
		//	item being created and 2+ items being removed. 
		//
		//	The way I have implemented this here is that if the result after creating a 
		//	combination item means your inventory stays at or below capacity then it is allowed. 
		//	But the inventory must be at or below capacity after each item creation.
		//
		//	Also Item will need to be put into a slot first before we can create combination 
		//	items meaning that even if your inventory is full and you know your purchase will 
		//	result in the load staying below capacity you cannot do that.
		//======================================================================================== 
		
		/* Start off by putting the item into inventory. This massive chunk of code is an exact 
		replica of all the code above here, but does not return and instead sets the value of 
		AddItemResult and goes a goto */

		FInventory::TryAddItemResult AddItemResult;

		//---------------------------------------------------------------------------------------
		//	------ Begin mostly copied code ------

		if (ItemsInfo.GetStackSize() == 1)
		{
			if (AreAllSlotsOccupied())
			{
				AddItemResult = TryAddItemResult(EGameWarning::InventoryFull, 0, Item);
				goto Label;
			}
			else
			{
				if (AtNumStacksLimit(Item, ItemsInfo) == true)
				{
					AddItemResult = TryAddItemResult(EGameWarning::CannotCarryAnymoreOfItem, 0, Item);
					goto Label;
				}
				else
				{
					// First free slot is where we will put this item

					for (int32 i = 0; i < SlotsArray.Num(); ++i)
					{
						const FInventorySlotState & Elem = SlotsArray[i];

						if (Elem.HasItem() == false)
						{
							PutItemInSlotChecked(InventoryOwner, true, bIsOwnerSelected, bIsOwnerCurrentSelected, Item, Quantity, i, ItemsInfo, ReasonForAquiring, HUDWidget);
							AddItemResult = TryAddItemResult(EGameWarning::None, i, Item);
							goto Label;
						}
					}

					// Did not find free slot but we checked previously that our inventory was not full
					assert(0);
				}
			}
		}
		else
		{
			const bool bHasStackSizeLimit = ItemsInfo.HasNumberInStackLimit();

			if (ItemsInfo.HasNumberOfStacksLimit() == false)
			{
				if (AreAllSlotsOccupied() == true)
				{
					/* Find a stack to put item onto */

					for (int32 i = 0; i < ArraySize; ++i)
					{
						const FInventorySlotState & Elem = SlotsArray[i];

						if (Elem.GetItemType() == Item)
						{
							// See if this stack can accept adding this purchase onto it
							if (bHasStackSizeLimit)
							{
								if (Elem.GetNumInStack() + Quantity <= ItemsInfo.GetStackSize())
								{
									PutItemInSlotChecked(InventoryOwner, true, bIsOwnerSelected, bIsOwnerCurrentSelected, Item, Quantity, i, ItemsInfo, ReasonForAquiring, HUDWidget);
									AddItemResult = TryAddItemResult(EGameWarning::None, i, Item);
									goto Label;
								}
							}
							else
							{
								PutItemInSlotChecked(InventoryOwner, true, bIsOwnerSelected, bIsOwnerCurrentSelected, Item, Quantity, i, ItemsInfo, ReasonForAquiring, HUDWidget);
								AddItemResult = TryAddItemResult(EGameWarning::None, i, Item);
								goto Label;
							}
						}
					}

					// Could not find a stack to put item on to
					AddItemResult = TryAddItemResult(EGameWarning::InventoryFull, 0, Item);
					goto Label;
				}
				else
				{
					/* Find a slot to put item in. We will put the item on top of an already existing
					stack if it will fit. Otherwise we will put it in the first empty slot */

					int32 FreeIndex = -1;

					for (int32 i = 0; i < SlotsArray.Num(); ++i)
					{
						const FInventorySlotState & Elem = SlotsArray[i];

						if (Elem.GetItemType() == Item)
						{
							// See if this stack can accept adding this purchase onto it
							if (bHasStackSizeLimit)
							{
								if (Elem.GetNumInStack() + Quantity <= ItemsInfo.GetStackSize())
								{
									PutItemInSlotChecked(InventoryOwner, true, bIsOwnerSelected, bIsOwnerCurrentSelected, Item, Quantity, i, ItemsInfo, ReasonForAquiring, HUDWidget);
									AddItemResult = TryAddItemResult(EGameWarning::None, i, Item);
									goto Label;
								}
							}
							else
							{
								PutItemInSlotChecked(InventoryOwner, true, bIsOwnerSelected, bIsOwnerCurrentSelected, Item, Quantity, i, ItemsInfo, ReasonForAquiring, HUDWidget);
								AddItemResult = TryAddItemResult(EGameWarning::None, i, Item);
								goto Label;
							}
						}
						else if (Elem.HasItem() == false)
						{
							if (FreeIndex == -1)
							{
								FreeIndex = i;
							}
						}
					}

					/* If here then none of the stacks we already have of this item can accept the item
					so put the item in a new stack */

					// We already checked the inventory wasn't full so we should not get this
					assert(FreeIndex != -1);

					PutItemInSlotChecked(InventoryOwner, true, bIsOwnerSelected, bIsOwnerCurrentSelected, Item, Quantity, FreeIndex, ItemsInfo, ReasonForAquiring, HUDWidget);
					AddItemResult = TryAddItemResult(EGameWarning::None, FreeIndex, Item);
					goto Label;
				}
			}
			else
			{
				if (AtNumStacksLimit(Item, ItemsInfo))
				{
					// Must look for an existing stack to put this item on
					for (int32 i = 0; i < ArraySize; ++i)
					{
						const FInventorySlotState & Elem = SlotsArray[i];

						if (Elem.GetItemType() == Item)
						{
							if (bHasStackSizeLimit)
							{
								if (Elem.GetNumInStack() + Quantity <= ItemsInfo.GetStackSize())
								{
									PutItemInSlotChecked(InventoryOwner, true, bIsOwnerSelected, bIsOwnerCurrentSelected, Item, Quantity, i, ItemsInfo, ReasonForAquiring, HUDWidget);
									AddItemResult = TryAddItemResult(EGameWarning::None, i, Item);
									goto Label;
								}
							}
							else
							{
								PutItemInSlotChecked(InventoryOwner, true, bIsOwnerSelected, bIsOwnerCurrentSelected, Item, Quantity, i, ItemsInfo, ReasonForAquiring, HUDWidget);
								AddItemResult = TryAddItemResult(EGameWarning::None, i, Item);
								goto Label;
							}
						}
					}

					/* If here then could not find a stack to put item on top of */
					AddItemResult = TryAddItemResult(EGameWarning::CannotCarryAnymoreOfItem, 0, Item);
					goto Label;
				}
				else
				{
					if (AreAllSlotsOccupied())
					{
						// Must look for an existing stack to put item on 
						for (int32 i = 0; i < ArraySize; ++i)
						{
							const FInventorySlotState & Elem = SlotsArray[i];

							if (Elem.GetItemType() == Item)
							{
								if (bHasStackSizeLimit)
								{
									if (Elem.GetNumInStack() + Quantity <= ItemsInfo.GetStackSize())
									{
										PutItemInSlotChecked(InventoryOwner, true, bIsOwnerSelected, bIsOwnerCurrentSelected, Item, Quantity, i, ItemsInfo, ReasonForAquiring, HUDWidget);
										AddItemResult = TryAddItemResult(EGameWarning::None, i, Item);
										goto Label;
									}
								}
								else
								{
									PutItemInSlotChecked(InventoryOwner, true, bIsOwnerSelected, bIsOwnerCurrentSelected, Item, Quantity, i, ItemsInfo, ReasonForAquiring, HUDWidget);
									AddItemResult = TryAddItemResult(EGameWarning::None, i, Item);
									goto Label;
								}
							}
						}

						/* If here then could not find a stack to put item on top of */
						AddItemResult = TryAddItemResult(EGameWarning::InventoryFull, 0, Item);
						goto Label;
					}
					else
					{
						/* Look for an existing stack to put item on. If none can be found then
						place item in a new slot */

						int32 FreeIndex = -1;

						for (int32 i = 0; i < SlotsArray.Num(); ++i)
						{
							const FInventorySlotState & Elem = SlotsArray[i];

							if (Elem.GetItemType() == Item)
							{
								if (bHasStackSizeLimit)
								{
									if (Elem.GetNumInStack() + Quantity <= ItemsInfo.GetStackSize())
									{
										PutItemInSlotChecked(InventoryOwner, true, bIsOwnerSelected, bIsOwnerCurrentSelected, Item, Quantity, i, ItemsInfo, ReasonForAquiring, HUDWidget);
										AddItemResult = TryAddItemResult(EGameWarning::None, i, Item);
										goto Label;
									}
								}
								else
								{
									PutItemInSlotChecked(InventoryOwner, true, bIsOwnerSelected, bIsOwnerCurrentSelected, Item, Quantity, i, ItemsInfo, ReasonForAquiring, HUDWidget);
									AddItemResult = TryAddItemResult(EGameWarning::None, i, Item);
									goto Label;
								}
							}
							else if (Elem.HasItem() == false)
							{
								if (FreeIndex == -1)
								{
									FreeIndex = i;
								}
							}
						}

						// We already checked inventory was not full so this should not throw
						assert(FreeIndex != -1);

						PutItemInSlotChecked(InventoryOwner, true, bIsOwnerSelected, bIsOwnerCurrentSelected, Item, Quantity, FreeIndex, ItemsInfo, ReasonForAquiring, HUDWidget);
						AddItemResult = TryAddItemResult(EGameWarning::None, FreeIndex, Item);
						goto Label;
					}
				}
			}
		}

		//	------ End mostly copied code ------
		//----------------------------------------------------------------------------------------

		/* Check if we successfully added Item */
Label:	if (AddItemResult.Result == EGameWarning::None)
		{
			/* Successful. Now check if we can create any combination items */
			return TryCreateCombinationItem(InventoryOwner, bIsOwnerSelected, bIsOwnerCurrentSelected, 
				AddItemResult, ItemsInfo, GameInst, HUDWidget);
		}
		else // Not successful
		{
			return AddItemResult;
		}
	}
}

FInventory::TryAddItemResult FInventory::TryCreateCombinationItem(ISelectable * InventoryOwner, 
	bool bIsOwnerSelected, bool bIsOwnerCurrentSelected, TryAddItemResult ItemJustAdded, 
	const FInventoryItemInfo & ItemsInfo, const URTSGameInstance * GameInst, URTSHUD * HUDWidget)
{
	for (const auto & CombinationItem : ItemsInfo.GetItemsMadeFromThis())
	{
		const FInventoryItemInfo * CombinationItemInfo = GameInst->GetInventoryItemInfo(CombinationItem);

		/* Cannot create item if its 'uniqueness' rules would be broken. So we skip trying to. 
		This means that in order for it to be created later on the player would have to drop then 
		re-aquire the item. */
		if (AtTotalNumLimit(CombinationItem, 1, *CombinationItemInfo) == false)
		{
			if (AreAllIngredientsPresent(CombinationItem, *CombinationItemInfo) == true)
			{
				//=========================================================================
				//	Removing the ingredients
				//=========================================================================

				/* Most of the time after we remove the ingredients we will have room
				to put the new combination item. But not always. If the quantity bought
				from the shop is greater than 1 then sometimes we will not have room.
				Therefore we need to first check BEFORE removing any ingredients that:
				- if we remove the ingredients will we have room for the combination item?
				If not then we cannot create the combination item */

				/* Here we remove all the ingredients, but we need to remember what we removed
				in case we find out that the combination item will not fit in inventory and
				therefore need to put everything back. */

				const int32 PopulatedArraySize = SlotsArray.Num();

				// Return by value is correct here - we want a local copy of it
				TMap < EInventoryItem, FAtLeastOneInt16 > QuantitysRequired = CombinationItemInfo->GetIngredientsByValue();

				/* Array that keeps track of the ingredients we remove in case we need
				to put them back again. Maps index in SlotsArray to the item we removed
				and how many we removed */
				TMap < uint8, FInventoryItemQuantityPair > RemovedItems;
				RemovedItems.Reserve(QuantitysRequired.Num());

				for (int32 i = 0; i < PopulatedArraySize; ++i)
				{
					FInventorySlotState & InventorySlot = SlotsArray[i];
					const EInventoryItem ItemType = InventorySlot.GetItemType();

					if (QuantitysRequired.Contains(ItemType))
					{
						const int16 NumToRemove = QuantitysRequired[ItemType].GetInteger();

						/* Remove ingredients from stack. This isn't final and can be reversed 
						later so we do not update HUD yet */
						if (InventorySlot.GetNumInStack() >= NumToRemove)
						{
							QuantitysRequired.FindAndRemoveChecked(ItemType);

							InventorySlot.AdjustAmount(-NumToRemove);

							/* Check if the stack was emptied and update variable if so */
							if (InventorySlot.GetNumInStack() == 0)
							{
								InventorySlot.SetItemTypeToNone();
								
								NumSlotsOccupied--;

								Quantities[ItemType].DecrementNumStacksAndAdjustNum(NumToRemove);
							}
							else
							{
								Quantities[ItemType].AdjustAmount(-NumToRemove);
							}

							// Note down we did this in case we need to reverse it later 
							RemovedItems.Emplace(i, FInventoryItemQuantityPair(ItemType, NumToRemove));
						}
						else
						{
							/* Stack does not contain enough of the ingredient for combination
							item. Remove as many as we can from stack */

							const int16 NumActuallyRemoved = InventorySlot.GetNumInStack();

							QuantitysRequired[ItemType] -= NumActuallyRemoved;

							InventorySlot.ReduceQuantityToZero();

							NumSlotsOccupied--;

							Quantities[ItemType].AdjustAmount(-NumActuallyRemoved);

							// Note down we did this in case we need to reverse it later
							RemovedItems.Emplace(i, FInventoryItemQuantityPair(ItemType, NumActuallyRemoved));
						}
					}
				}

				// Assert we found every ingredient. If this throws then as long as all values are 0 this is ok
				assert(QuantitysRequired.Num() == 0);

				//========================================================================
				//	Putting combination item into inventory if there's room
				//========================================================================

				const bool bHasStackSizeLimit = CombinationItemInfo->HasNumberInStackLimit();
				int32 FreeIndex = -1;

				/* Find index of stack in StackArray to put combination item, prefering an
				already existing stack of the same type over an empty one.
				Currently this logic requires putting the full Quantity amount in one
				stack. We could make it "better" and have it try and split it between
				stacks if that is what it will take to get the combination item into
				the inventory */
				if (CombinationItemInfo->CanStack())
				{
					for (int32 i = 0; i < SlotsArray.Num(); ++i)
					{
						const FInventorySlotState & InventorySlot = SlotsArray[i];

						if (InventorySlot.GetItemType() == CombinationItem)
						{
							if (bHasStackSizeLimit) 
							{
								if (InventorySlot.GetNumInStack() < CombinationItemInfo->GetStackSize())
								{
									FreeIndex = i;
									break;
								}
							}
							else
							{
								FreeIndex = i;
								break;
							}
						}
						else if (InventorySlot.HasItem() == false)
						{
							if (FreeIndex == -1)
							{
								FreeIndex = i;

								/* No break here - keep checking if a slot of the same type
								exists that we can add to */
							}
						}
					}
				}
				else // Cannot stack
				{
					// Just put it in first empty slot
					for (int32 i = 0; i < SlotsArray.Num(); ++i)
					{
						if (SlotsArray[i].HasItem() == false)
						{
							FreeIndex = i;
							break;
						}
					}
				}

				const bool bCanAddCombinationItem = (FreeIndex != -1);
				if (bCanAddCombinationItem)
				{
					/* Run all the OnRemove logic for all the ingredients we removed. */
					for (const auto & Elem : RemovedItems)
					{
						// Param2 = item type, param3 = how many times to run it
						RunOnItemRemovedLogic(InventoryOwner, Elem.Value.ItemType, Elem.Value.Quantity, 
							EItemRemovalReason::Ingredient, *GameInst->GetInventoryItemInfo(Elem.Value.ItemType));
					}

					/* Notify HUD about removal */
					if (bIsOwnerSelected)
					{
						HUDWidget->Selected_OnItemsRemovedFromInventory(InventoryOwner, this, 
							RemovedItems, bIsOwnerCurrentSelected);
					}

					PutItemInSlotChecked(InventoryOwner, false, bIsOwnerSelected, bIsOwnerCurrentSelected, 
						CombinationItem, 1, FreeIndex, *CombinationItemInfo, EItemAquireReason::CombinedFromOthers, HUDWidget);

					SwapJustCreatedCombinationItemIntoLowestDisplayIndex(FreeIndex, InventoryOwner,
						bIsOwnerSelected, bIsOwnerCurrentSelected, HUDWidget);

					/* Now see if creating the combination item in turn causes more
					combination items to be created */
					return TryCreateCombinationItem(InventoryOwner, bIsOwnerSelected, bIsOwnerCurrentSelected, 
						FInventory::TryAddItemResult(EGameWarning::None, FreeIndex, CombinationItem),
						*CombinationItemInfo, GameInst, HUDWidget);
				}
				else
				{
					/* Could not add combination item to inventory. Roll back changes. */

					for (const auto & Elem : RemovedItems)
					{
						const uint8 ArrayIndex = Elem.Key;
						const EInventoryItem ItemTypeRemoved = Elem.Value.ItemType;
						const int16 AmountRemoved = Elem.Value.Quantity;

						FInventorySlotState & Slot = SlotsArray[ArrayIndex];

						// Put item back
						if (Slot.HasItem() == false)
						{
							/* Note here: the flags in the struct I think were never adjusted so 
							we do not need to re-set them */
							Slot.PutItemInForReversal(ItemTypeRemoved, AmountRemoved);
							NumSlotsOccupied++;

							Quantities[ItemTypeRemoved].IncrementNumStacksAndAdjustNum(AmountRemoved);
						}
						else
						{
							assert(Slot.GetItemType() == ItemTypeRemoved);

							Slot.AdjustAmount(AmountRemoved);

							Quantities[ItemTypeRemoved].AdjustAmount(AmountRemoved);
						}
					}

					/* At this point we could continue with the for loop and try another
					combination item but I think we will just stop here */
					break;
				}
			}
		}
	}

	/* If here then no combination item was created this call */

	/* Update HUD now. We did not do it in TryPutItemInInventory. 
	I actually added this code because I thought it was needed. Turns out this code wasn't 
	what was required to fix what I wanted, but it may actually be needed later on. For now 
	I have commented it since I'm busy testing something else so cannot check right now. 
	Actually I do not think it is needed */
	//if (bIsOwnerSelected)
	//{
	//	HUDWidget->Selected_OnItemAddedToInventory(InventoryOwner, GetLocalIndexFromServerIndex(ItemJustAdded.RawIndex),
	//		SlotsArray[ItemJustAdded.RawIndex], bIsOwnerCurrentSelected);
	//}

	return ItemJustAdded;
}

bool FInventory::AreAllIngredientsPresent(EInventoryItem Item, const FInventoryItemInfo & ItemsInfo) const
{
	for (const auto & Elem : ItemsInfo.GetIngredients())
	{
		if (Quantities.Contains(Elem.Key) == false || Quantities[Elem.Key].GetTotalNum() < Elem.Value.GetInteger())
		{
			return false;
		}
	}

	return true;
}

void FInventory::SwapJustCreatedCombinationItemIntoLowestDisplayIndex(int32 SlotsArrayIndexForCombinationItem, 
	ISelectable * InventoryOwner, bool bIsOwnerSelected, bool bIsOwnerCurrentSelected, URTSHUD * HUDWidget)
{
	const uint8 ComboItemDisplayIndex = ServerIndexToLocalIndex[SlotsArrayIndexForCombinationItem];
	
	// Check quick case
	if (AreAllSlotsOccupied())
	{
		// Still have to update HUD 
		if (bIsOwnerSelected)
		{
			HUDWidget->Selected_OnItemAddedToInventory(InventoryOwner, ComboItemDisplayIndex,
				SlotsArray[SlotsArrayIndexForCombinationItem], bIsOwnerCurrentSelected);
		}
		
		return;
	}

	int16 ServerIndex = INT16_MAX;
	int16 LowestEmptyDisplayIndex = INT16_MAX;

	for (int32 i = 0; i < SlotsArray.Num(); ++i)
	{
		if (SlotsArray[i].HasItem() == false)
		{
			const uint8 DisplayIndex = ServerIndexToLocalIndex[i];
			
			if ((DisplayIndex < LowestEmptyDisplayIndex) && (DisplayIndex < ComboItemDisplayIndex))
			{
				ServerIndex = i;
				LowestEmptyDisplayIndex = DisplayIndex;
			}
		}
	}

	if (LowestEmptyDisplayIndex == INT16_MAX)
	{
		/* None of the empty slots were at a lower local postion so no swap will happen */

		// Still have to update HUD 
		if (bIsOwnerSelected)
		{
			HUDWidget->Selected_OnItemAddedToInventory(InventoryOwner, ComboItemDisplayIndex,
				SlotsArray[SlotsArrayIndexForCombinationItem], bIsOwnerCurrentSelected);
		}
	}
	else
	{
		// Need to swap just created combination item into this index
		SwapSlotPositions_LocalIndicies(ComboItemDisplayIndex, LowestEmptyDisplayIndex);

		// Update HUD
		if (bIsOwnerSelected)
		{
			HUDWidget->Selected_OnInventoryPositionsSwapped(InventoryOwner, ComboItemDisplayIndex,
				LowestEmptyDisplayIndex, bIsOwnerCurrentSelected);
		}

		/* Whenever two indices swap we need to tell the PC about it. The reason for this is 
		that there may be a pending item use action that has a pointer to the inventory slot, 
		so we need to make it point to the correct slot after the swap. 
		However we only need to do this if we own the selectable, and also because item uses 
		are only issued to the primary selected we can branch on that too */
		if (bIsOwnerCurrentSelected && InventoryOwner->GetAttributesBase().GetAffiliation() == EAffiliation::Owned)
		{
			InventoryOwner->GetLocalPC()->OnInventoryIndicesOfPrimarySelectedSwapped(SlotsArrayIndexForCombinationItem, ServerIndex);
		}
	}
}

void FInventory::SwapSlotPositions_ServerIndicies(int32 ServerIndex_1, int32 ServerIndex_2)
{
	uint8 Temp;
	
	// Swap ServerIndexToLocalIndex entries
	Temp = ServerIndexToLocalIndex[ServerIndex_1];
	ServerIndexToLocalIndex[ServerIndex_1] = ServerIndexToLocalIndex[ServerIndex_2];
	ServerIndexToLocalIndex[ServerIndex_2] = Temp;
	
	// Swap LocalIndexToServerIndex entries
	Temp = LocalIndexToServerIndex[ServerIndexToLocalIndex[ServerIndex_1]];
	LocalIndexToServerIndex[ServerIndexToLocalIndex[ServerIndex_1]] = LocalIndexToServerIndex[ServerIndexToLocalIndex[ServerIndex_2]];
	LocalIndexToServerIndex[ServerIndexToLocalIndex[ServerIndex_2]] = Temp;
}

void FInventory::SwapSlotPositions_LocalIndicies(int32 LocalIndex_1, int32 LocalIndex_2)
{
	uint8 Temp;
	
	// Swap LocalIndexToServerIndex entries
	Temp = LocalIndexToServerIndex[LocalIndex_1];
	LocalIndexToServerIndex[LocalIndex_1] = LocalIndexToServerIndex[LocalIndex_2];
	LocalIndexToServerIndex[LocalIndex_2] = Temp;

	// Swap ServerIndexToLocalIndex entries
	Temp = ServerIndexToLocalIndex[LocalIndexToServerIndex[LocalIndex_1]];
	ServerIndexToLocalIndex[LocalIndexToServerIndex[LocalIndex_1]] = ServerIndexToLocalIndex[LocalIndexToServerIndex[LocalIndex_2]];
	ServerIndexToLocalIndex[LocalIndexToServerIndex[LocalIndex_2]] = Temp;
}

EGameWarning FInventory::CanItemEnterInventory(EInventoryItem Item, int32 Quantity,
	const FInventoryItemInfo & ItemsInfo, ISelectable * InventoryOwner) const
{
	/* This function just checks whether the item can be added. It does not remember the index 
	to add at. We could do that though and pass it on to the server. It would save the server 
	some work and can't really be exploited */
	
	/* This function doesn't take into account whether the item will combine into another item
	and therefore you will have room. So if you have a full inventory and you know something you
	buy will combine and make something else then you will need to drop an item first */

	/* This happening now means even if selectable has no inventory then we get this warning. No biggie I guess though */
	if (ItemsInfo.IsSelectablesTypeAcceptable(InventoryOwner) == EGameWarning::TypeCannotAquireItem)
	{
		return EGameWarning::TypeCannotAquireItem;
	}

	if (ItemsInfo.GetStackSize() == 1)
	{
		if (AreAllSlotsOccupied())
		{
			return EGameWarning::InventoryFull;
		}
		else
		{
			if (AtNumStacksLimit(Item, ItemsInfo) == true)
			{
				return EGameWarning::CannotCarryAnymoreOfItem;
			}
			else
			{
				return EGameWarning::None;
			}
		}
	}
	else
	{
		/* An item that can have multiple of it in a stack */

		const bool bHasStackSizeLimit = ItemsInfo.HasNumberInStackLimit();

		if (ItemsInfo.HasNumberOfStacksLimit() == false)
		{
			if (AreAllSlotsOccupied() == false)
			{
				return EGameWarning::None;
			}
			else
			{
				// Search for a slot that we can add this item on to

				for (int32 i = 0; i < SlotsArray.Num(); ++i)
				{
					const FInventorySlotState & InventorySlot = SlotsArray[i];

					if (InventorySlot.GetItemType() == Item)
					{
						if (bHasStackSizeLimit)
						{
							if (InventorySlot.GetNumInStack() + Quantity <= ItemsInfo.GetStackSize())
							{
								// Found a stack we can add the purchase onto
								return EGameWarning::None;
							}
						}
						else
						{
							// Found a stack we can add the purchase onto
							return EGameWarning::None;
						}
					}
				}

				// Did not find a stack we can add this purchase onto
				return EGameWarning::InventoryFull;
			}
		}
		else
		{
			if (AreAllSlotsOccupied() == false)
			{
				if (AtNumStacksLimit(Item, ItemsInfo) == false)
				{
					return EGameWarning::None;
				}
				else
				{
					// Check if an existing stack can accept this item

					for (int32 i = 0; i < SlotsArray.Num(); ++i)
					{
						const FInventorySlotState & Elem = SlotsArray[i];

						if (Elem.GetItemType() == Item)
						{
							if (bHasStackSizeLimit)
							{
								if (Elem.GetNumInStack() + Quantity <= ItemsInfo.GetStackSize())
								{
									return EGameWarning::None;
								}
							}
							else
							{
								return EGameWarning::None;
							}
						}
					}

					// Could not find a stack to add item on to
					return EGameWarning::CannotCarryAnymoreOfItem;
				}
			}
			else
			{
				// Check if an existing stack can accept this item

				for (int32 i = 0; i < SlotsArray.Num(); ++i)
				{
					const FInventorySlotState & Elem = SlotsArray[i];

					if (Elem.GetItemType() == Item)
					{
						if (bHasStackSizeLimit)
						{
							if (Elem.GetNumInStack() + Quantity <= ItemsInfo.GetStackSize())
							{
								return EGameWarning::None;
							}
						}
						else
						{
							return EGameWarning::None;
						}
					}
				}

				// Could not find a stack to add item on to
				return EGameWarning::InventoryFull;
			}
		}
	}
}

EGameWarning FInventory::CanItemEnterInventory(AInventoryItem * InventoryItem, ISelectable * InventoryOwner) const
{
	return CanItemEnterInventory(InventoryItem->GetType(), InventoryItem->GetItemQuantity(), *InventoryItem->GetItemInfo(), InventoryOwner);
}

void FInventory::PutItemInSlotChecked(ISelectable * InventoryOwner, bool bUpdateHUD, bool bIsOwnerSelected,
	bool bIsOwnerCurrentSelected, EInventoryItem ItemType, int32 Quantity, int32 SlotIndex, 
	const FInventoryItemInfo & ItemsInfo, EItemAquireReason AquireReason, URTSHUD * HUDWidget)
{
	FInventorySlotState & Slot = SlotsArray[SlotIndex];

	if (Slot.HasItem())
	{
		// Assert we are not trying to have 2+ different item types in the same stack
		assert(Slot.GetItemType() == ItemType);

		Slot.AdjustAmount(Quantity);

		Quantities[ItemType].AdjustAmount(Quantity);
	}
	else
	{
		NumSlotsOccupied++;

		/* This func should set the item type and quantity */
		Slot.PutItemIn(ItemType, Quantity, ItemsInfo);

		if (Quantities.Contains(ItemType))
		{
			Quantities[ItemType].IncrementNumStacksAndAdjustNum(Quantity);
		}
		else
		{
			Quantities.Emplace(ItemType, FInventoryItemQuantity(Quantity));
		}
	}

	// Apply the effects of aquiring the item
	RunOnItemAquiredLogic(InventoryOwner, ItemType, Quantity, AquireReason, ItemsInfo);

	// Update HUD 
	if (bUpdateHUD)
	{
		if (bIsOwnerSelected)
		{
			HUDWidget->Selected_OnItemAddedToInventory(InventoryOwner, ServerIndexToLocalIndex[SlotIndex],
				Slot, bIsOwnerCurrentSelected); 
		}
	}
}

void FInventory::OnItemUsed(UWorld * World, ISelectable * Owner, EAffiliation OwnersAffiliation, 
	uint8 ServerSlotIndex, const FContextButtonInfo & AbilityInfo, URTSGameInstance * GameInstance, 
	URTSHUD * HUDWidget)
{
	FInventorySlotState & Slot = SlotsArray[ServerSlotIndex];
	const EInventoryItem ItemType = Slot.GetItemType();

	/* Start cooldown timer handle. Will only do for server and owning client but nothing wrong
	with doing it for everyone */
	if (AbilityInfo.GetCooldown() > 0.f && (World->IsServer() || OwnersAffiliation == EAffiliation::Owned))
	{
		/* Start timer handle. This func should be really basic. We're calling it through Owner 
		because timer handles need a UObject to run. All this func needs to do is start the timer 
		handle and call a call a OnInventoryItemUseCooldownTimerHandleExpired func which will 
		then make sure to update the HUD */
		Owner->StartInventoryItemUseCooldownTimerHandle(Slot.GetItemUseCooldownTimerHandle(), 
			ServerSlotIndex, AbilityInfo.GetCooldown());
	}

	if (Slot.HasUnlimitedCharges() == false)
	{
		const FInventoryItemInfo * ItemsInfo = GameInstance->GetInventoryItemInfo(ItemType);
		
		if (Slot.CanItemStack())
		{
			Slot.RemoveAmount(1);
		}
		else
		{
			// Remove 1 charge
			Slot.RemoveCharge();
		}
	
		/* This function should branch on the 3 different 'OnNumChargesChanged' functions. 
		It will also need to take into account whether the item can stack or not */
		RunOnNumChargesChangedLogic(Owner, ItemType, Slot, -1, EItemChangesNumChargesReason::Use,
			*ItemsInfo, HUDWidget);
	}

	/* Update HUD. We potentially started a timer handle and removed a charge/item. 
	If the user chose custom OnZeroCharges behavior then we have no way of knowing what they did. 
	It is up to users to make sure any logic they run updates UI if necessary.
	This HUD func just updates the affected slot's quantity/charges and cooldown */
	HUDWidget->Selected_OnInventoryItemUsed(Owner, GetLocalIndexFromServerIndex(ServerSlotIndex),
		AbilityInfo.GetCooldown(), Owner->GetAttributesBase().IsPrimarySelected()); 
}

void FInventory::OnItemSold(ISelectable * Owner, uint8 ServerSlotIndex, 
	int16 AmountSold, const FInventoryItemInfo & ItemsInfo, URTSHUD * HUDWidget)
{
	FInventorySlotState & Slot = SlotsArray[ServerSlotIndex];
	const EInventoryItem ItemType = Slot.GetItemType();

	Slot.AdjustAmount(-AmountSold);

	/* Check if the slot is now empty. If it can't stack then it must be empty */
	if (Slot.CanItemStack() == false || Slot.GetNumInStack() == 0)
	{
		Slot.SetItemTypeToNone();
		NumSlotsOccupied--;
		Quantities[ItemType].DecrementNumStacksAndAdjustNum(AmountSold);
	}
	else
	{
		Quantities[ItemType].AdjustAmount(AmountSold);
	}

	// Run OnRemoved logic
	RunOnItemRemovedLogic(Owner, ItemType, AmountSold, EItemRemovalReason::Sold, ItemsInfo);
	
	// Update HUD
	if (Owner->GetAttributesBase().IsSelected())
	{
		HUDWidget->Selected_OnInventoryItemSold(Owner, GetLocalIndexFromServerIndex(ServerSlotIndex),
			Owner->GetAttributesBase().IsPrimarySelected());
	}
}

uint32 FInventory::OnOwnerZeroHealth(UWorld * World, URTSGameInstance * GameInst, ARTSGameState * GameState,
	ISelectable * Owner, const FVector & OwnerLocation, float OwnerYawRotation)
{
	//==========================================================================================
	//	Variables users may want to adjust to change the behavior of function
	//==========================================================================================
	
	/* How far away from the selectable we should try drop the item */
	const float DropCircleRadius = Owner->GetBoundsLength() + 100.f;
	/* How many items to drop per circle. Should be at least 1 */
	const uint32 DropsPerCircle = 6;
	/* After DropsPerCircle how much to increase radius by */
	const float CircleRadiusIncreaseAmount = 80.f;
	/* If true then try not to drop the item on a cliff. Modify 
	OnOwnerZeroHealth_IsLocationConsideredACliff to adjust this logic. */
	const bool bTryToNotDropOnCliff = false;
	const int32 NumCliffTracesToTry = 3;

	/* Half height of line trace. It's actually important we NEVER fail a trace because the 
	ground around us is too high/low so this should be set with that in mind */
	const float TraceHalfHeight = 2000.f;

	/* Whether to remove the item from the inventory. If you implement logic that respawns your 
	selectable later on (like in dota 2 when your hero dies) then you would want this true. 
	Turning this off is an optimization */
	const bool bUpdateInventory = true;

	/* 
	----------------------------------------------------------------------------------------------
	Some quick notes about how we choose where to drop the items: 
	
	- We try to drop all items in a circle of radius DropDistance around the dead selectable. 
	- If bTryToNotDropOnCliff is true then if the original location is considered a cliff then 
	we will try drop the item at locations that are less than DropDistance away until we find 
	one.  
	- The angle between each drop is DegreesBetweenEachDrop. If we drop enough items that we 
	have made a complete circle of items then the drop distance will we increased slightly, 
	and we will drop another circle of items with a larger radius. Also the angle will be offset 
	by half. I quick diagram of how this may look: 
	
	DropsPerCircle = 4

	O = where selectable died. Had 6 items which need to be dropped and was facing east.
	1,2,3,4,5,6 = items that were dropped in order

	                                             5
									  	   2     
								               
								      4	   O->  1
									            
									       3    
										         6

	But what if 2 circles worth is not enough? In that case we just keep increasing the radius 
	and will end up using the same angles from the first circle.

	The cliff logic may cause items to drop on top of each other if it is the second circle's 
	worth. It won't be very smart for the sake of performance because we'll be doing a trace 
	for every location.
	----------------------------------------------------------------------------------------------
	*/


	//==========================================================================================
	//	Part of function that does stuff
	//==========================================================================================

	// Wonder if this is calculated at compile time
	const float RadiansBetweenEachDrop = FMath::DegreesToRadians(360.f / DropsPerCircle);

	float CurrentCircleRadius = DropCircleRadius;
	float RotationOffset = 0.f;
	uint32 NumItemsDropped = 0;

	/* Drop whatever items are ment to drop on death. */
	for (int32 i = SlotsArray.Num() - 1; i >= 0; --i)
	{
		FInventorySlotState & Elem = SlotsArray[i];

		// Consider adding a uint8 for flags about item like drops on death, can be explicitly dropped, etc 
		// Also perhaps the int32 in FInventorySlotState can be changed to an int16
		if (Elem.HasItem() && Elem.DropsOnZeroHealth())
		{
			/* Drop the item in front of the unit, so the direction they were facing when they 
			reached zero health matters. */

			/* Here I would like to avoid dropping the item on a high up cliff nearby. The location 
			and yaw rot params will be identical on both server and client, but I'm not too sure 
			whether ray traces are deternimistic across different CPUs/GPUs. For the channel it traces 
			against nothing moves around so there's that but still. 
			
			There are ways around this that would work even if ray tracing is not deterministic. 
			One way would be to just keep moving in closer and closer to OwnerLocation until we 
			consider that we are no longer on a cliff. It may mean client and server locations 
			are slightly off but hopefully not enough that it matters. 
			Eh technically if traces are not deterministic then there is potential for trouble 
			if the trace doesn't hit on one client but does on the other. The solution is to 
			make the trace height large enough that this is basically impossible */

			/* Here make sure to alternate between left and right instead of dropping
			the first item in front of us then just going around the circle in the same
			direction */
			const int32 RotationMuliplier = ((NumItemsDropped % DropsPerCircle) % 2 == 1)
				? ((NumItemsDropped % DropsPerCircle) / 2) + 1
				: ((NumItemsDropped % DropsPerCircle) / 2) * (-1);

			const float YawRotationInRadians = FMath::DegreesToRadians<float>(OwnerYawRotation) + (RotationMuliplier * RadiansBetweenEachDrop) + RotationOffset;
			// Possibly clamp the rotation to between [0, 2PI)? or is it [-PI, PI)?

			// Create vector out of rotation
			float SY, CY;
			FMath::SinCos(&SY, &CY, YawRotationInRadians);
			FVector UnitVector = FVector(CY, SY, 0.f);
			UnitVector.Normalize();

			bool bFoundSuitableLocation = false;
			FHitResult HitResult;
			if (bTryToNotDropOnCliff)
			{
				/* Haven't really thought about this too much */
				
				/* Basically just do closer and closer to location unit reached zero health until 
				we find a spot that's considered suitable */
				for (int32 NumTracesDone = 0; NumTracesDone < NumCliffTracesToTry; ++NumTracesDone)
				{
					const FVector TraceMiddle = OwnerLocation + UnitVector * CurrentCircleRadius * ((NumCliffTracesToTry - NumTracesDone) / (float)NumCliffTracesToTry);
					const FVector TraceStart = TraceMiddle + FVector(0.f, 0.f, TraceHalfHeight);
					const FVector TraceEnd = TraceMiddle + FVector(0.f, 0.f, -TraceHalfHeight);

					if (World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, GROUND_CHANNEL))
					{
						/* Check if the location is considered a cliff */
						if (OnOwnerZeroHealth_IsLocationConsideredACliff(OwnerLocation, HitResult.ImpactPoint) == false)
						{
							bFoundSuitableLocation = true;
							break;
						}
					}
					else
					{
						// Silently fail... not good
					}
				}
			}
			else
			{
				const FVector TraceMiddle = OwnerLocation + UnitVector * CurrentCircleRadius;
				const FVector TraceStart = TraceMiddle + FVector(0.f, 0.f, TraceHalfHeight);
				const FVector TraceEnd = TraceMiddle + FVector(0.f, 0.f, -TraceHalfHeight);
				
				if (World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, GROUND_CHANNEL))
				{
					bFoundSuitableLocation = true;
				}
				else
				{
					// Silently fail... not good
				}
			}

			if (bFoundSuitableLocation)
			{
				// Put item at hit location
				GameState->PutInventoryItemInWorld(Elem.GetItemType(), Elem.GetNumInStack(), 
					Elem.GetNumCharges(), HitResult.ImpactPoint, FRotator(0.f, OwnerYawRotation, 0.f), 
					EItemEntersWorldReason::DroppedOnDeath);

				if (bUpdateInventory)
				{
					/* The code in this block removes the item from the inventory array. The 
					owner is dead and can no longer be selected but a reason you may want this 
					code to run is if you implement some logic that respawns your unit later 
					(like in dota when your hero respawns) with their inventory the way 
					it was when they died (minus the items that drop on death of course) */

					const FInventoryItemInfo * ItemsInfo = GameInst->GetInventoryItemInfo(Elem.GetItemType());

					// Run item's OnDropped logic. Probably don't need to update HUD after this logic has run
					RunOnItemRemovedLogic(Owner, Elem.GetItemType(), Elem.GetNumInStack(),
						EItemRemovalReason::DroppedOnDeath, *ItemsInfo);

					NumSlotsOccupied--;
					Quantities[Elem.GetItemType()].DecrementNumStacksAndAdjustNum(Elem.GetNumInStack());
					Elem.ReduceQuantityToZero();
				}
				
				NumItemsDropped++;

				if (NumItemsDropped % DropsPerCircle == 0)
				{
					/* Have dropped all the items we want to for this circle. Increase the
					radius for the next group of drops */
					CurrentCircleRadius += CircleRadiusIncreaseAmount;

					/* Offset angle by half. If I find that the second circle of items drop more
					to one side then change that 0.5f to a -0.5f */
					RotationOffset = (RotationOffset == 0.f) ? RadiansBetweenEachDrop * -0.5f : 0.f;
				}
			}
		}
	}

	return NumItemsDropped;
}

bool FInventory::OnOwnerZeroHealth_IsLocationConsideredACliff(const FVector & SelectableLocation, const FVector & Location) const
{
	/* Two things are considered here: 
	- pitch between SelectableLocation and Location 
	- Z axis difference between SelectableLocation and Location. 
	
	You'll likely want to adjust this to your specific game though 
	
	Also this isn't very good because the SelectableLocation is often the center of the selectable 
	so has half it's height added on to it. Would be good to know the selectable's height when 
	running this function */

	const bool bIsHeightDifferenceOK = FMath::Abs<float>(Location.Z - SelectableLocation.Z) < 300.f;
	if (bIsHeightDifferenceOK)
	{
		/* Now check pitch between the two vectors */
		const float Hypotenuse = (Location - SelectableLocation).Size();
		const float Adjacent = (FVector2D(Location) - FVector2D(SelectableLocation)).Size();
		const float Pitch = FMath::RadiansToDegrees(FMath::Acos(Adjacent / Hypotenuse));

		return (Pitch > 60.f);
	}

	return true;
}

#if WITH_EDITOR
void FInventory::OnPostEdit(AActor * InOwningActor)
{
	/* We always keep the array full */
	SlotsArray.Empty();
	SlotsArray.Reserve(Capacity);
	for (uint8 i = 0; i < Capacity; ++i)
	{
		SlotsArray.Emplace(FInventorySlotState(i));
	}

	// Populate these 2 arrays.
	LocalIndexToServerIndex.Empty();
	LocalIndexToServerIndex.Reserve(SlotsArray.Num());
	ServerIndexToLocalIndex.Empty();
	ServerIndexToLocalIndex.Reserve(SlotsArray.Num());
	for (int32 i = 0; i < SlotsArray.Num(); ++i)
	{
		LocalIndexToServerIndex.Emplace(i);
		ServerIndexToLocalIndex.Emplace(i);
	}

	/* If we're allowing units to start with items in their inventory then make sure 
	NumSlotsOccupied is set at some point */
}
#endif // WITH_EDITOR


//======================================================================================
//	>FSelectableResourceInfo
//======================================================================================

/* For performance's sake it is good to keep this as a power of two */
static_assert(Statics::IsPowerOfTwo(FSelectableResourceInfo::MULTIPLIER),
	"FSelectableResourceInfo::MULTIPLIER is not a power of two. For performance's sake it is best "
	"to keep this as a power of two");

int32 FSelectableResourceInfo::AmountUndividedToAmount() const
{
	return AmountUndivided / MULTIPLIER;
}

#if WITH_EDITOR
int32 FSelectableResourceInfo::CalculateRegenRatePerGameTick(int32 InRegenRatePerMinute)
{
	/* We have the regen rate per minute. Convert that to a regen rate per second */
	const float AsFloat = (InRegenRatePerMinute * MULTIPLIER) / (60.f / ProjectSettings::GAME_TICK_RATE);
	// Rounding behavior we choose is round to nearest integer
	return FMath::RoundToInt(AsFloat);
}
#endif

FSelectableResourceInfo::FSelectableResourceInfo()
{
	Type = ESelectableResourceType::None;
	StartingAmount = 50;
	MaxAmount = 300;
	AmountUndivided = StartingAmount * MULTIPLIER;
	Amount = AmountUndividedToAmount();
#if WITH_EDITOR
	RegenRatePerMinute = 100;

	// Could possibly add another if statement like if (bNotActuallyInPIEOrStandalone)
	OnPostEdit();
#endif
}

ESelectableResourceType FSelectableResourceInfo::GetType() const
{
	return Type;
}

void FSelectableResourceInfo::SetNumTicksAhead(int8 NumTicks)
{
	NumTicksAhead = NumTicks;
}

void FSelectableResourceInfo::SetNumTicksAheadGivenStuff(uint8 GameTickCount, uint8 InstigatingEffectTickCount)
{
	NumTicksAhead = (int32)InstigatingEffectTickCount - ((int32)GameTickCount + (int32)NumTicksAhead);
}

void FSelectableResourceInfo::SetAmountInternal(int32 NewAmountUndivided,
	uint8 GameStatesTickCount, uint8 InstigatingEffectsTickCount, bool bUpdateUI, URTSHUD * HUDWidget, 
	ISelectable * Owner, USelectableWidgetComponent * PersistentWorldWidget, 
	USelectableWidgetComponent * SelectedWorldWidget, bool bIsSelected, bool bIsCurrentSelected)
{
	if (NewAmountUndivided != AmountUndivided)
	{
		//---------------------------------------------------------------------------------------
		//	Bring current amount up to date
		SetNumTicksAheadGivenStuff(GameStatesTickCount, InstigatingEffectsTickCount);
		AmountUndivided += (-NumTicksAhead) * RegenRatePerGameTickUndivided;
		AmountUndivided = FMath::Clamp<int32>(AmountUndivided, 0, MaxAmount * MULTIPLIER);
		Amount = AmountUndividedToAmount();

		//--------------------------------------------------------------------------------------
		// Now apply the change

		AmountUndivided = NewAmountUndivided;
		AmountUndivided = FMath::Clamp<int32>(AmountUndivided, 0, MaxAmount * MULTIPLIER);
		Amount = AmountUndividedToAmount();

		//---------------------------------------------------------------------------------------
		//	Update UI

		if (bUpdateUI)
		{
			// Update HUD 
			HUDWidget->Selected_OnSelectableResourceCurrentAmountChanged(Owner, GetAmountAsFloatForDisplay(),
				MaxAmount, bIsCurrentSelected);

			// Update the world widgets 
			PersistentWorldWidget->OnSelectableResourceAmountChanged(Amount, MaxAmount);
			SelectedWorldWidget->OnSelectableResourceAmountChanged(Amount, MaxAmount);
		}
	}
}

void FSelectableResourceInfo::SetAmount(int32 NewAmount, uint8 GameStatesTickCount, 
	uint8 InstigatingEffectsTickCount, bool bUpdateUI, URTSHUD * HUDWidget, ISelectable * Owner, 
	USelectableWidgetComponent * PersistentWorldWidget, USelectableWidgetComponent * SelectedWorldWidget, 
	bool bIsSelected, bool bIsCurrentSelected)
{
	SetAmountInternal(NewAmount * MULTIPLIER, GameStatesTickCount, InstigatingEffectsTickCount,
		bUpdateUI, HUDWidget, Owner, PersistentWorldWidget, SelectedWorldWidget, bIsSelected,
		bIsCurrentSelected);
}

void FSelectableResourceInfo::SetAmountViaMultiplier(float Multiplier, uint8 GameStatesTickCount,
	uint8 InstigatingEffectsTickCount, bool bUpdateUI, URTSHUD * HUDWidget, ISelectable * Owner, 
	USelectableWidgetComponent * PersistentWorldWidget, USelectableWidgetComponent * SelectedWorldWidget, 
	bool bIsSelected, bool bIsCurrentSelected)
{
	SetAmountInternal(AmountUndivided * Multiplier, GameStatesTickCount, InstigatingEffectsTickCount,
		bUpdateUI, HUDWidget, Owner, PersistentWorldWidget, SelectedWorldWidget, bIsSelected,
		bIsCurrentSelected);
}

void FSelectableResourceInfo::AdjustAmount(int32 AdjustAmount, uint8 GameStatesTickCount,
	uint8 InstigatingEffectsTickCount, bool bUpdateUI, URTSHUD * HUDWidget, ISelectable * Owner,
	USelectableWidgetComponent * PersistentWorldWidget, USelectableWidgetComponent * SelectedWorldWidget, 
	bool bIsSelected, bool bIsCurrentSelected)
{
	SetAmountInternal(AmountUndivided + (AdjustAmount * MULTIPLIER), GameStatesTickCount, InstigatingEffectsTickCount,
		bUpdateUI, HUDWidget, Owner, PersistentWorldWidget, SelectedWorldWidget, bIsSelected,
		bIsCurrentSelected);
}

void FSelectableResourceInfo::AdjustAmount(int32 AdjustAmount, uint8 GameStatesTickCount, 
	uint8 InstigatingEffectsTickCount, ISelectable * Owner, bool bUpdateUI)
{
	FSelectableResourceInfo::AdjustAmount(AdjustAmount, GameStatesTickCount, InstigatingEffectsTickCount, bUpdateUI,
		Owner->GetLocalPC()->GetHUDWidget(), Owner, Owner->GetPersistentWorldWidget(),
		Owner->GetSelectedWorldWidget(), Owner->GetAttributesBase().IsSelected(),
		Owner->GetAttributesBase().IsPrimarySelected());
}

int32 FSelectableResourceInfo::SetMaxAmount(int32 NewMaxAmount, uint8 GameStatesTickCount,
	uint8 InstigatingEffectsTickCount, bool bUpdateUI, URTSHUD * HUDWidget, ISelectable * Owner, 
	USelectableWidgetComponent * PersistentWorldWidget, USelectableWidgetComponent * SelectedWorldWidget, bool bIsSelected, 
	bool bIsCurrentSelected, EAttributeAdjustmentRule CurrentAmountAdjustmentRule)
{
	/* Do not allow setting max amount less than 1 to avoid dividing by 0 when showing
	percentage in UI */
	const int32 NewMaxAmountClamped = (NewMaxAmount < 1) ? 1 : NewMaxAmount;
	
	if (NewMaxAmountClamped != MaxAmount)
	{
		/* First bring the current amount up to date in regards to game ticks. Only client 
		will ever get values not equal to zero here */
		SetNumTicksAheadGivenStuff(GameStatesTickCount, InstigatingEffectsTickCount);
		AmountUndivided += (-NumTicksAhead) * RegenRatePerGameTickUndivided;
		AmountUndivided = FMath::Clamp<int32>(AmountUndivided, 0, MaxAmount * MULTIPLIER);
		Amount = AmountUndividedToAmount();

		//--------------------------------------------------------------------------------------
		// Now apply the change

		const int32 Difference = NewMaxAmountClamped - MaxAmount;
		const float PercentageDiff = FMath::RoundToInt(((float)NewMaxAmountClamped / MaxAmount));
		
		MaxAmount = NewMaxAmountClamped;

		// To know whether we should try update UI for the current amount too
		bool bChangedCurrentAmountToo = false;

		/* Adjust the current amount */
		switch (CurrentAmountAdjustmentRule)
		{
			case EAttributeAdjustmentRule::Percentage:
			{
				AmountUndivided *= PercentageDiff;
				AmountUndivided = FMath::Clamp<int32>(AmountUndivided, 0, MaxAmount * MULTIPLIER);
				Amount = AmountUndividedToAmount();

				bChangedCurrentAmountToo = true;

				break;
			}
			case EAttributeAdjustmentRule::Absolute:
			{
				if (PercentageDiff > 1.f)
				{
					AmountUndivided += Difference * MULTIPLIER;
					Amount = AmountUndividedToAmount();

					bChangedCurrentAmountToo = true;
				}
				else
				{
					if (AmountUndivided > MaxAmount * MULTIPLIER)
					{
						AmountUndivided = MaxAmount * MULTIPLIER;
						Amount = AmountUndividedToAmount();

						bChangedCurrentAmountToo = true;
					}
				}
			
				break;
			}
			case EAttributeAdjustmentRule::NoChange:
			{
				if (AmountUndivided > MaxAmount * MULTIPLIER)
				{
					AmountUndivided = MaxAmount * MULTIPLIER;
					Amount = AmountUndividedToAmount();

					bChangedCurrentAmountToo = true;
				}
			
				break;
			}
			default:
			{
				assert(0);
				break;
			}
		}

		//---------------------------------------------------------------------------------------
		//	Update UI

		if (bUpdateUI)
		{
			if (bChangedCurrentAmountToo)
			{
				// Update HUD
				HUDWidget->Selected_OnSelectableResourceCurrentAndMaxAmountsChanged(Owner, Amount,
					MaxAmount, bIsCurrentSelected);

				// Update world widgets
				PersistentWorldWidget->OnSelectableResourceAmountChanged(Amount, MaxAmount);
				SelectedWorldWidget->OnSelectableResourceAmountChanged(Amount, MaxAmount);
			}
			else
			{
				// Update HUD
				HUDWidget->Selected_OnSelectableResourceMaxAmountChanged(Owner, Amount, MaxAmount,
					bIsCurrentSelected);

				// Update world widgets
				PersistentWorldWidget->OnSelectableResourceAmountChanged(Amount, MaxAmount);
				SelectedWorldWidget->OnSelectableResourceAmountChanged(Amount, MaxAmount);
			}
		}
	}

	return MaxAmount;
}

int32 FSelectableResourceInfo::SetMaxAmountViaMultiplier(float Multiplier, uint8 GameStatesTickCount,
	uint8 InstigatingEffectsTickCount, bool bUpdateUI, URTSHUD * HUDWidget, ISelectable * Owner, 
	USelectableWidgetComponent * PersistentWorldWidget, USelectableWidgetComponent * SelectedWorldWidget, 
	bool bIsSelected, bool bIsCurrentSelected, EAttributeAdjustmentRule CurrentAmountAdjustmentRule)
{
	/* Probably lose a few CPU cycles by bubbling this instead of writing it out */
	SetMaxAmount(FMath::RoundToInt(MaxAmount * Multiplier), GameStatesTickCount, InstigatingEffectsTickCount, 
		bUpdateUI, HUDWidget, Owner, PersistentWorldWidget, SelectedWorldWidget, bIsSelected, 
		bIsCurrentSelected, CurrentAmountAdjustmentRule);

	return MaxAmount;
}

int32 FSelectableResourceInfo::AdjustMaxAmount(int32 AdjustAmount, uint8 GameStatesTickCount,
	uint8 InstigatingEffectsTickCount, bool bUpdateUI, URTSHUD * HUDWidget, ISelectable * Owner, 
	USelectableWidgetComponent * PersistentWorldWidget, USelectableWidgetComponent * SelectedWorldWidget, 
	bool bIsSelected, bool bIsCurrentSelected, EAttributeAdjustmentRule CurrentAmountAdjustmentRule)
{
	SetMaxAmount(MaxAmount + AdjustAmount, GameStatesTickCount, InstigatingEffectsTickCount,
		bUpdateUI, HUDWidget, Owner, PersistentWorldWidget, SelectedWorldWidget, bIsSelected,
		bIsCurrentSelected, CurrentAmountAdjustmentRule);

	return MaxAmount;
}

float FSelectableResourceInfo::AdjustRegenRate(float AdjustAmount, uint8 GameStateTickCount,
	uint8 InstigatingEffectTickCount, ISelectable * Owner, bool bUpdateUI, URTSHUD * HUDWidget,
	USelectableWidgetComponent * PersistentWorldWidget, USelectableWidgetComponent * SelectedWorldWidget, 
	bool bIsSelected, bool bIsCurrentSelected, UWorld * World, ARTSGameState * GameState)
{
	if (AdjustAmount != 0.f)
	{
		SetRegenRate(GetRegenRatePerSecond() + AdjustAmount, GameStateTickCount, InstigatingEffectTickCount,
			Owner, bUpdateUI, HUDWidget, PersistentWorldWidget, SelectedWorldWidget, bIsSelected, 
			bIsCurrentSelected, World, GameState);
	}
	
	return GetRegenRatePerSecond();
}

float FSelectableResourceInfo::SetRegenRate(float NewRegenRatePerSecond, uint8 GameStateTickCount,
	uint8 InstigatingEffectTickCount, ISelectable * Owner, bool bUpdateUI, URTSHUD * HUDWidget,
	USelectableWidgetComponent * PersistentWorldWidget, USelectableWidgetComponent * SelectedWorldWidget, 
	bool bIsSelected, bool bIsCurrentSelected, UWorld * World, ARTSGameState * GameState)
{
	//------------------------------------------------------------------------------------------
	//	Bring current amount up to date 
	
	SetNumTicksAheadGivenStuff(GameStateTickCount, InstigatingEffectTickCount);
	AmountUndivided += (-NumTicksAhead) * RegenRatePerGameTickUndivided;
	AmountUndivided = FMath::Clamp<int32>(AmountUndivided, 0, MaxAmount * MULTIPLIER);
	Amount = AmountUndividedToAmount();

	//------------------------------------------------------------------------------------------
	//	Apply change

	/* Before doing this we need to note down whether we currently regen */
	const bool bIsRegener = RegensOverTime();

	RegenRatePerGameTickUndivided = FMath::RoundToInt(NewRegenRatePerSecond * ProjectSettings::GAME_TICK_RATE);

	/* Check if our regen rate either: 
	- went from 0 to non 0 
	- went from non 0 to 0 
	If either of those cases happen then we need to add/remove this owner from an array in GS */
	if (bIsRegener && !RegensOverTime())
	{
		/* We don't *have* to remove it from the array but we will anyway */
		if (World->IsServer())
		{
			GameState->Server_UnregisterSelectableResourceRegener(Owner);
		}
		else
		{
			// No casting overhead right?
			GameState->Client_UnregisterSelectableResourceRegener(CastChecked<AActor>(Owner));
		}

		UE_LOG(RTSLOG, Log, TEXT("FSelectableResourceInfo::SetRegenRate: a selectable's regen was disabled"));
	}
	else if (!bIsRegener && RegensOverTime())
	{
		if (World->IsServer())
		{
			// This func will just add it to array, but should assert that it's not there first
			GameState->Server_RegisterSelectableResourceRegener(Owner);
		}
		else
		{
			// No casting overhead right?
			GameState->Client_RegisterSelectableResourceRegener(CastChecked<AActor>(Owner));
		}

		UE_LOG(RTSLOG, Log, TEXT("FSelectableResourceInfo::SetRegenRate: a selectable became a regener"));
	}

	//---------------------------------------------------------------------------------------
	//	Update UI

	if (bUpdateUI)
	{
		// Update HUD
		HUDWidget->Selected_OnSelectableResourceRegenRateChanged(Owner, RegenRatePerGameTickUndivided,
			bIsCurrentSelected);
	}

	return GetRegenRatePerSecond();
}

float FSelectableResourceInfo::SetRegenRateViaMultiplier(float Multiplier, uint8 GameStateTickCount,
	uint8 InstigatingEffectTickCount, ISelectable * Owner, bool bUpdateUI, URTSHUD * HUDWidget,
	USelectableWidgetComponent * PersistentWorldWidget, USelectableWidgetComponent * SelectedWorldWidget,
	bool bIsSelected, bool bIsCurrentSelected, UWorld * World, ARTSGameState * GameState)
{
	if (Multiplier != 1.f)
	{
		SetRegenRate(GetRegenRatePerSecond() * Multiplier, GameStateTickCount, InstigatingEffectTickCount,
			Owner, bUpdateUI, HUDWidget, PersistentWorldWidget, SelectedWorldWidget, bIsSelected, 
			bIsCurrentSelected, World, GameState);
	}

	return GetRegenRatePerSecond();
}

void FSelectableResourceInfo::SetStartingAmount(int32 NewStartingAmount, uint8 GameStateTickCount, 
	uint8 InstigatingEffectTickCount, ISelectable * Owner, bool bUpdateUI, URTSHUD * HUDWidget, 
	USelectableWidgetComponent * PersistentWorldWidget, USelectableWidgetComponent * SelectedWorldWidget, 
	bool bIsSelected, bool bIsCurrentSelected, EAttributeAdjustmentRule CurrentAmountAdjustmentRule)
{
	/* Cannot set a negative starting amount since we do not allow negative resource values. 
	Setting a value above max doesn't really work right now with how upgrades are applied */
	assert(NewStartingAmount >= 0 && NewStartingAmount <= MaxAmount);
	
	assert(0); // Crash because I haven't implemented it
	// TODO
}

void FSelectableResourceInfo::SetStartingAmountViaMultiplier(float Multiplier, uint8 GameStateTickCount, 
	uint8 InstigatingEffectTickCount, ISelectable * Owner, bool bUpdateUI, URTSHUD * HUDWidget, 
	USelectableWidgetComponent * PersistentWorldWidget, USelectableWidgetComponent * SelectedWorldWidget, 
	bool bIsSelected, bool bIsCurrentSelected, EAttributeAdjustmentRule CurrentAmountAdjustmentRule)
{
	SetStartingAmount(FMath::RoundToInt(StartingAmount * Multiplier), GameStateTickCount, InstigatingEffectTickCount,
		Owner, bUpdateUI, HUDWidget, PersistentWorldWidget, SelectedWorldWidget, bIsSelected,
		bIsCurrentSelected, CurrentAmountAdjustmentRule);
}

void FSelectableResourceInfo::AdjustStartingAmount(int32 AdjustAmount, uint8 GameStateTickCount,
	uint8 InstigatingEffectTickCount, ISelectable * Owner, bool bUpdateUI, URTSHUD * HUDWidget,
	USelectableWidgetComponent * PersistentWorldWidget, USelectableWidgetComponent * SelectedWorldWidget, 
	bool bIsSelected, bool bIsCurrentSelected, EAttributeAdjustmentRule CurrentAmountAdjustmentRule)
{
	SetStartingAmount(StartingAmount + AdjustAmount, GameStateTickCount, InstigatingEffectTickCount,
		Owner, bUpdateUI, HUDWidget, PersistentWorldWidget, SelectedWorldWidget, bIsSelected,
		bIsCurrentSelected, CurrentAmountAdjustmentRule);
}

void FSelectableResourceInfo::RegenFromGameTicks(int8 NumGameTicks, URTSHUD * HUDWidget,
	ISelectable * Owner, ARTSPlayerController * LocalPlayerController, 
	USelectableWidgetComponent * PersistentWorldWidget, USelectableWidgetComponent * SelectedWorldWidget)
{
	/* If false then why are we calling this? */
	assert(NumGameTicks > 0);
	/* If 0 then there's no point running any of the logic in this function. We should have 
	checked for this before setting it to be updated each game tick */
	assert(RegenRatePerGameTickUndivided != 0);

	/* Basically here we do nothing if we are still ahead */
	NumTicksAhead -= NumGameTicks;
	if (NumTicksAhead < 0)
	{
		/* Regenerate how much we are ment to regenerate. Or if we are ahead then we will
		regenerate in the oppisite direction actually */
		AmountUndivided += (-NumTicksAhead) * RegenRatePerGameTickUndivided;
		AmountUndivided = FMath::Clamp<int32>(AmountUndivided, 0, MaxAmount * MULTIPLIER);
		Amount = AmountUndividedToAmount();

		NumTicksAhead = 0;

		/* Update HUD if we're the CurrentSelected */
		HUDWidget->Selected_OnSelectableResourceCurrentAmountChanged(Owner, GetAmountAsFloatForDisplay(), 
			MaxAmount,
			LocalPlayerController->GetCurrentSelected() == Owner);

		/* Update the selected and persistent world widgets */
		SelectedWorldWidget->OnSelectableResourceAmountChanged(Amount, MaxAmount);
		PersistentWorldWidget->OnSelectableResourceAmountChanged(Amount, MaxAmount);
	}

	/* When we call this for every selectable resource user we update a lot of widgets. See if 
	there's some way of optimizing it */
}

int32 FSelectableResourceInfo::GetAmount() const
{
	return Amount;
}

float FSelectableResourceInfo::GetAmountAsFloatForDisplay() const
{ 
	return (float)AmountUndivided * (1.f / MULTIPLIER);
}

float FSelectableResourceInfo::GetRegenRatePerSecond() const
{
	return (RegenRatePerGameTickUndivided * (1.f / ProjectSettings::GAME_TICK_RATE)) / MULTIPLIER;
}

float FSelectableResourceInfo::GetRegenRatePerSecondForDisplay() const
{
	return GetRegenRatePerSecond();
}

int32 FSelectableResourceInfo::GetMaxAmount() const
{
	return MaxAmount;
}

bool FSelectableResourceInfo::RegensOverTime() const
{
	return (RegenRatePerGameTickUndivided != 0);
}

#if WITH_EDITOR
void FSelectableResourceInfo::OnPostEdit()
{
	bCanEditProperties = (Type != ESelectableResourceType::None);
	
	AmountUndivided = StartingAmount * MULTIPLIER;
	Amount = AmountUndividedToAmount();
	
	RegenRatePerGameTickUndivided = CalculateRegenRatePerGameTick(RegenRatePerMinute);
}
#endif


//======================================================================================
//	>FGatheredResourceMeshInfo
//======================================================================================

FGatheredResourceMeshInfo::FGatheredResourceMeshInfo()
{
	Mesh = nullptr;
	SocketName = NAME_None;
}


//======================================================================================
//	>FResourceCollectionAttribute
//======================================================================================

FResourceCollectionAttribute::FResourceCollectionAttribute()
{
	GatherTime = 2.f;
	Capacity = 50;
}


//======================================================================================
//	>FResourceGatheringProperties
//======================================================================================

FResourceGatheringProperties::FResourceGatheringProperties()
{
	MovementSpeedPenaltyForHoldingResources = 1.f;
}

float FResourceGatheringProperties::GetMoveSpeedPenaltyForHoldingResources() const
{
	return MovementSpeedPenaltyForHoldingResources;
}

bool FResourceGatheringProperties::IsCollector() const
{
	return ResourceGatheringProperties.Num() > 0;
}

bool FResourceGatheringProperties::CanGatherResource(EResourceType ResourceType) const
{
	return ResourceGatheringProperties.Contains(ResourceType);
}

float FResourceGatheringProperties::GetGatherTime(EResourceType ResourceType) const
{
	return ResourceGatheringProperties[ResourceType].GetGatherTime();
}

uint32 FResourceGatheringProperties::GetCapacity(EResourceType ResourceType) const
{
	return ResourceGatheringProperties[ResourceType].GetCapacity();
}

const FGatheredResourceMeshInfo & FResourceGatheringProperties::GetGatheredMeshProperties(EResourceType ResourceType) const
{
	return ResourceGatheringProperties[ResourceType].GetMeshInfo();
}

const TMap<EResourceType, FResourceCollectionAttribute>& FResourceGatheringProperties::GetTMap() const
{
	return ResourceGatheringProperties;
}

TMap<EResourceType, FResourceCollectionAttribute>& FResourceGatheringProperties::GetAllAttributesModifiable()
{
	return ResourceGatheringProperties;
}


//======================================================================================
//	>FSelectableAttributesBase
//======================================================================================

FSelectableAttributesBase::FSelectableAttributesBase()
	: Name(FText())
	, SelectionSound(nullptr) 
	, ContextMenu(FContextMenuButtons()) 
	, bIsSelected(false) 
	, bIsPrimarySelected(false) 
	, Team(ETeam::Uninitialized) 
	, Affiliation(EAffiliation::Unknown) 
	, VisionState(EFogStatus::Hidden) 
	, SelectionDecalSetup(ESelectionDecalSetup::Unknown) 
	, SelectionDecalMID(nullptr)  
	, ParticleSize(0)
{
}

const FName & FSelectableAttributesBase::GetOwnerID() const
{
	assert(PlayerID != FName());
	return PlayerID;
}

void FSelectableAttributesBase::SetupBasicTypeInfo(ESelectableType InSelectableType, 
	EBuildingType InBuildingType, EUnitType InUnitType)
{
	SelectableType = InSelectableType;
	BuildingType = InBuildingType;
	UnitType = InUnitType;
}

void FSelectableAttributesBase::SetupSelectionInfo(const FName & InOwnerID, ETeam InTeam)
{
	PlayerID = InOwnerID;
	Team = InTeam;
}


//======================================================================================
//	>FSelectableAttributes
//======================================================================================

FSelectableAttributesBasic::FSelectableAttributesBasic() 
{
	BuildTime = 0.2f;
	MaxHealth = 100.f;
	TargetingType = ETargetingType::Default;
	ArmourType = EArmourType::Default;
	DefenseMultiplier = 1.f;
	SightRange = 1200.f;
	StealthRevealRange = 0.f;
	ExperienceBounty = 60.f;
	HUDPersistentTabCategory = EHUDPersistentTabType::None;
	HUDPersistentTabButtonOrdering = 0;
	NumRequiredReppedVariablesForSetup = 4; // Owner, SelectableID, creation method, game tick counter
	NumReppedVariablesReceived = 0;
	CreationMethod = ESelectableCreationMethod::Uninitialized;

	DefaultMaxHealth = MaxHealth;
	DefaultDefenseMultiplier = DefenseMultiplier;
}

FSelectableAttributesBasic::~FSelectableAttributesBasic()
{
}

USoundBase * FSelectableAttributesBasic::GetCommandSound(EContextButton Command) const
{
	if (ContextCommandSounds.Contains(Command))
	{
		return ContextCommandSounds[Command];
	}
	else
	{
		return nullptr;
	}
}

const FAttachInfo & FSelectableAttributesBasic::GetBodyAttachPointInfo(ESelectableBodySocket AttachPoint) const
{
	/* Use user defined entry if there is one. Otherwise default to root component */
	if (BodyAttachPoints.Contains(AttachPoint))
	{
		return BodyAttachPoints[AttachPoint];
	}
	else
	{
		return DefaultAttachInfo;
	}
}

bool FSelectableAttributesBasic::IsAStartingSelectable() const
{
	/* Recently added this, may need to relax it */
	assert(CreationMethod != ESelectableCreationMethod::Uninitialized);
	
	return (CreationMethod == ESelectableCreationMethod::StartingSelectable);
}

ESelectableCreationMethod FSelectableAttributesBasic::GetCreationMethod() const
{
	return CreationMethod;
}

void FSelectableAttributesBasic::SetCreationMethod(ESelectableCreationMethod InMethod)
{
	CreationMethod = InMethod;
}

void FSelectableAttributesBasic::SetNumCustomGameTicksAhead(int8 NumAhead)
{
	SelectableResource_1.SetNumTicksAhead(NumAhead);
}

bool FSelectableAttributesBasic::HasASelectableResource() const
{
	return (SelectableResource_1.GetType() != ESelectableResourceType::None);
}

bool FSelectableAttributesBasic::HasASelectableResourceThatRegens() const
{
	return HasASelectableResource() && SelectableResource_1.RegensOverTime();
}

ESelectableResourceType FSelectableAttributesBasic::GetSelectableResourceType() const
{
	return SelectableResource_1.GetType();
}

void FSelectableAttributesBasic::AdjustCurrentHealthAfterMaxHealthChange(ISelectable * Owner,
	float PreviousMaxHealth, EAttributeAdjustmentRule CurrentHealthAdjustmentRule)
{
	float & CurrentHealth = Owner->GetHealthRef();
	
	if (CurrentHealthAdjustmentRule == EAttributeAdjustmentRule::Absolute)
	{
		const float Difference = MaxHealth - PreviousMaxHealth;
		
		if (Difference > 0.f)
		{
			CurrentHealth += Difference;

			TellHUDAboutChange_MaxHealthAndCurrentHealth(Owner, CurrentHealth);
		}
		else if (CurrentHealth > MaxHealth)
		{
			CurrentHealth = MaxHealth;

			TellHUDAboutChange_MaxHealthAndCurrentHealth(Owner, CurrentHealth);
		}
		else
		{
			TellHUDAboutChange_MaxHealth(Owner, CurrentHealth);
		}
	}
	else if (CurrentHealthAdjustmentRule == EAttributeAdjustmentRule::Percentage)
	{
		const float Percentage = MaxHealth / PreviousMaxHealth;
		
		CurrentHealth *= Percentage;

		TellHUDAboutChange_MaxHealthAndCurrentHealth(Owner, CurrentHealth);
	}
	else // Assuming NoChange
	{
		assert(CurrentHealthAdjustmentRule == EAttributeAdjustmentRule::NoChange);

		if (CurrentHealth > MaxHealth)
		{
			CurrentHealth = MaxHealth;

			TellHUDAboutChange_MaxHealthAndCurrentHealth(Owner, CurrentHealth);
		}
		else
		{
			TellHUDAboutChange_MaxHealth(Owner, CurrentHealth);
		}
	}
}

void FSelectableAttributesBasic::TellHUDAboutChange_MaxHealth(ISelectable * Owner, float CurrentHealth)
{
	Owner->GetLocalPC()->GetHUDWidget()->Selected_OnMaxHealthChanged(Owner, MaxHealth, CurrentHealth,
		Owner->GetLocalPC()->GetCurrentSelected() == Owner);
}

void FSelectableAttributesBasic::TellHUDAboutChange_MaxHealthAndCurrentHealth(ISelectable * Owner, float CurrentHealth)
{
	Owner->GetLocalPC()->GetHUDWidget()->Selected_OnCurrentAndMaxHealthChanged(Owner, MaxHealth, 
		CurrentHealth, Owner->GetLocalPC()->GetCurrentSelected() == Owner);
}

void FSelectableAttributesBasic::TellHUDAboutChange_DefenseMultiplier(ISelectable * Owner)
{
	Owner->GetLocalPC()->GetHUDWidget()->Selected_OnDefenseMultiplierChanged(Owner, DefenseMultiplier,
		Owner->GetLocalPC()->GetCurrentSelected() == Owner);
}

float FSelectableAttributesBasic::GetDefaultMaxHealth() const
{
	return DefaultMaxHealth;
}

void FSelectableAttributesBasic::SetMaxHealth(float NewMaxHealth, ISelectable * Owner,
	EAttributeAdjustmentRule CurrentHealthAdjustmentRule)
{
	const float PreviousMaxHealth = MaxHealth;

	if (NumTempMaxHealthModifiers > 0)
	{
		const float Percentage = NewMaxHealth / DefaultMaxHealth;
		MaxHealth *= Percentage;
	}
	else
	{
		MaxHealth = NewMaxHealth;
	}

	DefaultMaxHealth = NewMaxHealth;

	/* Adjust current health and tell HUD about change */
	AdjustCurrentHealthAfterMaxHealthChange(Owner, PreviousMaxHealth, CurrentHealthAdjustmentRule);
}

void FSelectableAttributesBasic::SetMaxHealthViaMultiplier(float Multiplier, ISelectable * Owner,
	EAttributeAdjustmentRule CurrentHealthAdjustmentRule)
{
	const float PreviousMaxHealth = MaxHealth;
	
	MaxHealth *= Multiplier;
	DefaultMaxHealth *= Multiplier;

	/* Adjust current health and tell HUD about change */
	AdjustCurrentHealthAfterMaxHealthChange(Owner, PreviousMaxHealth, CurrentHealthAdjustmentRule);
}

float FSelectableAttributesBasic::ApplyTempMaxHealthModifierViaMultiplier(float Multiplier, ISelectable * Owner,
	EAttributeAdjustmentRule CurrentHealthAdjustmentRule)
{
	const float PreviousMaxHealth = MaxHealth;
	
	NumTempMaxHealthModifiers++;

	MaxHealth *= Multiplier;
	
	/* Adjust current health and tell HUD about change */
	AdjustCurrentHealthAfterMaxHealthChange(Owner, PreviousMaxHealth, CurrentHealthAdjustmentRule);

	return MaxHealth;
}

float FSelectableAttributesBasic::RemoveTempMaxHealthModifierViaMultiplier(float Multiplier, ISelectable * Owner,
	EAttributeAdjustmentRule CurrentHealthAdjustmentRule)
{
	const float PreviousMaxHealth = MaxHealth;
	
	NumTempMaxHealthModifiers--;
	assert(NumTempMaxHealthModifiers >= 0);

	if (NumTempMaxHealthModifiers > 0)
	{
		MaxHealth *= Multiplier;
	}
	else
	{
		MaxHealth = DefaultMaxHealth;
	}

	/* Adjust current health and tell HUD about change */
	AdjustCurrentHealthAfterMaxHealthChange(Owner, PreviousMaxHealth, CurrentHealthAdjustmentRule);

	return MaxHealth;
}

float FSelectableAttributesBasic::GetDefaultDefenseMultiplier() const
{
	return DefaultDefenseMultiplier;
}

void FSelectableAttributesBasic::SetDefenseMultiplier(float NewDefenseMultiplier, ISelectable * Owner)
{
	if (NumTempDefenseMultiplierModifiers > 0)
	{
		const float Percentage = NewDefenseMultiplier / DefaultDefenseMultiplier;
		DefenseMultiplier *= Percentage;
	}
	else
	{
		DefenseMultiplier = NewDefenseMultiplier;
	}

	DefaultDefenseMultiplier = NewDefenseMultiplier;

	// Tell HUD about change
	TellHUDAboutChange_DefenseMultiplier(Owner);
}

void FSelectableAttributesBasic::SetDefenseMultiplierViaMultiplier(float Multiplier, ISelectable * Owner)
{
	DefenseMultiplier *= Multiplier;
	DefaultDefenseMultiplier *= Multiplier;

	// Tell HUD about change
	TellHUDAboutChange_DefenseMultiplier(Owner);
}

float FSelectableAttributesBasic::ApplyTempDefenseMultiplierViaMultiplier(float Multiplier, ISelectable * Owner)
{
	/* This change is irreversable (would require dividing by zero) and therefore not allowed. 
	If you would like to make a selectable invulnerable use ISelectable::SetIsInvulnerable */
	assert(Multiplier != 0.f);
	
	NumTempDefenseMultiplierModifiers++;

	DefenseMultiplier *= Multiplier;
	
	// Tell HUD about change
	TellHUDAboutChange_DefenseMultiplier(Owner);

	return DefenseMultiplier;
}

float FSelectableAttributesBasic::RemoveTempDefenseMultiplierViaMultiplier(float Multiplier, ISelectable * Owner)
{
	NumTempDefenseMultiplierModifiers--;
	assert(NumTempDefenseMultiplierModifiers >= 0);

	if (NumTempDefenseMultiplierModifiers > 0)
	{
		DefenseMultiplier *= Multiplier;
	}
	else
	{
		DefenseMultiplier = DefaultDefenseMultiplier;
	}

	// Tell HUD about change
	TellHUDAboutChange_DefenseMultiplier(Owner);

	return DefenseMultiplier;
}

void FSelectableAttributesBasic::IncrementNumReppedVariablesReceived()
{
	NumReppedVariablesReceived++;
	assert(NumReppedVariablesReceived <= NumRequiredReppedVariablesForSetup);
}

bool FSelectableAttributesBasic::CanSetup() const
{
	return (NumReppedVariablesReceived == NumRequiredReppedVariablesForSetup);
}

#if WITH_EDITOR
void FSelectableAttributesBasic::OnPostEdit(AActor * InOwningActor)
{
	/* I'm getting some inconsistent results with post edit being called, even taking into account
	the fact that PostEditChain gets called for structs only. Might have to test it more */

	/* Pretty inefficient way of doing this. Better to get the property edited and decide if
	anything needs to be done. */

	TArray < FContextButton > & ButtonArray = ContextMenu.GetButtonsArray();

	if (ContextMenu.GetButtons().Num() < ContextMenu.GetButtonsArray().Num())
	{
		for (int32 i = ButtonArray.Num() - 1; i >= 0; --i)
		{
			FContextButton & Elem = ButtonArray[i];

			Elem.UpdateFieldVisibilitys();

			if (!ContextMenu.GetButtons().Contains(Elem))
			{
				ButtonArray.RemoveAt(i);
			}
		}
	}
	else if (ContextMenu.GetButtons().Num() > ContextMenu.GetButtonsArray().Num())
	{
		for (auto & Elem : ContextMenu.GetButtons())
		{
			Elem.UpdateFieldVisibilitys();

			if (!ButtonArray.Contains(Elem))
			{
				ButtonArray.Emplace(Elem);
			}
		}
	}
	else // Set.Num() == Array.Num()
	{
		/* This is pretty inefficient O(n^2) */

		int32 IndexToReplace = -1;

		/* Find index of item that doesn't belong in array if any */
		for (const auto & Elem : ButtonArray)
		{
			if (!ContextMenu.GetButtons().Contains(Elem))
			{
				IndexToReplace = ButtonArray.Find(Elem);
			}
		}

		/* Update field visibilities and replace item in array if any */
		for (auto & Elem : ContextMenu.GetButtons())
		{
			Elem.UpdateFieldVisibilitys();

			if (!ButtonArray.Contains(Elem))
			{
				// Pretty vague message
				UE_CLOG(IndexToReplace == -1, RTSLOG, Fatal, TEXT("Button not found: %s"),
					*Elem.ToString());
				
				ButtonArray[IndexToReplace] = Elem;
			}
		}
	}

	/* Check that context menu TSet and TArray match */
	UE_CLOG(ContextMenu.GetButtons().Num() != ContextMenu.GetButtonsArray().Num(), RTSLOG, Fatal,
		TEXT("Context menu TArray and TSet do not match. Size of TArray: %d, size of TSet: %d"),
		ContextMenu.GetButtonsArray().Num(), ContextMenu.GetButtons().Num());
	for (const auto & Elem : ContextMenu.GetButtons())
	{
		UE_CLOG(ContextMenu.GetButtonsArray().Contains(Elem) == false, RTSLOG, Fatal,
			TEXT("Context menu TArray and TSet do not match. Missing element: %s"), *Elem.ToString());
	}

	/* FName to component setup */
	for (auto & Elem : BodyAttachPoints)
	{
		Elem.Value.OnPostEdit(InOwningActor);
	}

	/* Remove any duplicates from Prerequisites array since they will do nothing and only 
	slow down performance. Also we maintain array order because it could get confusing for 
	users if their entries shuffle around */
	TSet < EBuildingType > NoDuplicates;
	NoDuplicates.Reserve(BuildingPrerequisites.Num());
	TArray < EBuildingType > Copy;
	Copy.Reserve(BuildingPrerequisites.Num());
	for (int32 i = BuildingPrerequisites.Num() - 1; i >= 0; --i)
	{
		EBuildingType Elem = BuildingPrerequisites[i];

		if (NoDuplicates.Contains(Elem) == false)
		{
			NoDuplicates.Emplace(Elem);
			Copy.Emplace(Elem);
		}
		
		BuildingPrerequisites.RemoveAt(i);
	}
	for (int32 i = Copy.Num() - 1; i >= 0; --i)
	{
		BuildingPrerequisites.Emplace(Copy[i]);
	}

	// This doesn't actually need to happen every post edit, just once
	DefaultAttachInfo.OnPostEdit(InOwningActor);

	DefaultMaxHealth = MaxHealth;
	/* DefenseMultiplier isn't exposed to editor so it will never change but adding this line 
	in case it becomes exposed in the future */
	DefaultDefenseMultiplier = DefenseMultiplier;

	SelectableResource_1.OnPostEdit();
}
#endif // WITH_EDITOR


//======================================================================================
//	>FBuildingNetworkState
//======================================================================================

bool FBuildingNetworkState::HasEnoughCapacityToAcceptUnit(AInfantry * UnitThatWantsToEnter) const
{
	return SlotUsage + UnitThatWantsToEnter->GetInfantryAttributes()->GarrisonAttributes.GetBuildingGarrisonSlotUsage() <= SlotCapacity;
}

int32 FBuildingNetworkState::ServerOnUnitEntered(AInfantry * Unit)
{
	OnUnitAdded_UpdateSlotUsage(Unit->GetInfantryAttributes()->GarrisonAttributes);

	const int32 Index = GarrisonedUnits.Emplace(Unit);

	return Index;
}

int32 FBuildingNetworkState::ClientOnUnitEntered(AInfantry * Unit)
{
	OnUnitAdded_UpdateSlotUsage(Unit->GetInfantryAttributes()->GarrisonAttributes);

	const int32 Index = GarrisonedUnits.Emplace(Unit);

	return Index;
}

int32 FBuildingNetworkState::ServerOnUnitExited(AInfantry * Unit)
{
	OnUnitRemoved_UpdateSlotUsage(Unit->GetInfantryAttributes()->GarrisonAttributes);

	const int32 Index = GarrisonedUnits.Find(Unit);
	GarrisonedUnits.RemoveAt(Index, 1, false);

	return Index;
}

int32 FBuildingNetworkState::ClientOnUnitExited(AInfantry * Unit)
{
	OnUnitRemoved_UpdateSlotUsage(Unit->GetInfantryAttributes()->GarrisonAttributes);

	const int32 Index = GarrisonedUnits.Find(Unit);
	GarrisonedUnits.RemoveAt(Index, 1, false);

	return Index;
}

int32 FBuildingNetworkState::OnGarrisonedUnitZeroHealth(AInfantry * Unit)
{
	OnUnitRemoved_UpdateSlotUsage(Unit->GetInfantryAttributes()->GarrisonAttributes);
	
	const int32 Index = GarrisonedUnits.Find(Unit);
	GarrisonedUnits.RemoveAt(Index, 1, false);

	return Index;
}

void FBuildingNetworkState::OnUnitAdded_UpdateSlotUsage(const FEnteringGarrisonAttributes & AddedUnitsAttributes)
{
	SlotUsage += AddedUnitsAttributes.GetBuildingGarrisonSlotUsage();
	assert(SlotUsage >= 0 && SlotUsage <= SlotCapacity);
}

void FBuildingNetworkState::OnUnitRemoved_UpdateSlotUsage(const FEnteringGarrisonAttributes & RemovedUnitsAttributes)
{
	SlotUsage -= RemovedUnitsAttributes.GetBuildingGarrisonSlotUsage();
	assert(SlotUsage >= 0 && SlotUsage <= SlotCapacity);
}


//======================================================================================
//	>FPlayersBuildingNetworksState
//======================================================================================

FBuildingNetworkState * FPlayersBuildingNetworksState::GetNetworkInfo(EBuildingNetworkType NetworkType)
{
	const int32 Index = Statics::BuildingNetworkTypeToArrayIndex(NetworkType);
	return &NetworksContainer[Index];
}


//======================================================================================
//	>FBuildingGarrisonAttributes
//======================================================================================

void FBuildingGarrisonAttributes::OnOwningBuildingSetup(ARTSPlayerState * OwningBuildingsOwner)
{
	if (NetworkType != EBuildingNetworkType::None)
	{
		GarrisonNetworkInfo = OwningBuildingsOwner->GetBuildingGarrisonNetworkInfo(NetworkType);
	}
}

void FBuildingGarrisonAttributes::ServerOnUnitEntered(ARTSGameState * GameState, ABuilding * ThisBuilding, AInfantry * Infantry, URTSHUD * HUD)
{
	FEnteringGarrisonAttributes & InfantrysGarrisonAttributes = Infantry->GetInfantryAttributesModifiable()->GarrisonAttributes;
	
	int32 Index;
	if (NetworkType == EBuildingNetworkType::None)
	{
		OnUnitAdded_UpdateSlotUsage(InfantrysGarrisonAttributes);

		assert(GarrisonedUnits.Contains(Infantry) == false);
		Index = GarrisonedUnits.Emplace(Infantry);
	}
	else
	{
		Index = GarrisonNetworkInfo->ServerOnUnitEntered(Infantry);
	}

	/* Update the building's vision radius if the garrisoned units are allowed to see out of it */
	if (bCanSeeOutOf)
	{
		float InfantrysSightRadius;
		float InfantrysStealthRevealRadius;
		Infantry->GetVisionInfo(InfantrysSightRadius, InfantrysStealthRevealRadius);
		/* Check if we've gone over the building's effect sight range and update it if so */
		if (InfantrysSightRadius > ThisBuilding->GetBuildingAttributesModifiable()->GetEffectiveSightRange())
		{
			HightestSightRadiusContainerContributor = Infantry;
			ThisBuilding->GetBuildingAttributesModifiable()->SetEffectiveSightRange(InfantrysSightRadius, Infantry);
		}
		/* Same deal for stealth reveal radius */
		if (InfantrysStealthRevealRadius > ThisBuilding->GetBuildingAttributesModifiable()->GetEffectiveStealthRevealRange())
		{
			HighestStealthRevealRadiusContainerContributor = Infantry;
			ThisBuilding->GetBuildingAttributesModifiable()->SetEffectiveStealthRevealRange(InfantrysStealthRevealRadius, Infantry);
		}
	}

	/* Play sound */
	if (EnterSound != nullptr)
	{
		Statics::SpawnSoundAtLocation(GameState, Infantry->GetTeam(), EnterSound, ThisBuilding->GetActorLocation(),
			ESoundFogRules::DecideOnSpawn);
	}

	/* Tell HUD */
	HUD->OnUnitEnteredBuildingGarrison(ThisBuilding, *this, Infantry, Index);
}

void FBuildingGarrisonAttributes::ClientOnUnitEntered(ARTSGameState * GameState, ABuilding * ThisBuilding, AInfantry * Infantry, URTSHUD * HUD)
{
	FEnteringGarrisonAttributes & InfantrysGarrisonAttributes = Infantry->GetInfantryAttributesModifiable()->GarrisonAttributes;
	
	int32 Index;
	if (NetworkType == EBuildingNetworkType::None)
	{
		OnUnitAdded_UpdateSlotUsage(InfantrysGarrisonAttributes);

		assert(GarrisonedUnits.Contains(Infantry) == false);
		Index = GarrisonedUnits.Emplace(Infantry);
	}
	else
	{
		Index = GarrisonNetworkInfo->ClientOnUnitEntered(Infantry);
	}

	/* Update the building's vision radius if the garrisoned units are allowed to see out of it */
	if (bCanSeeOutOf)
	{
		float InfantrysSightRadius;
		float InfantrysStealthRevealRadius;
		Infantry->GetVisionInfo(InfantrysSightRadius, InfantrysStealthRevealRadius);
		/* Check if we've gone over the building's effect sight range and update it if so */
		if (InfantrysSightRadius > ThisBuilding->GetBuildingAttributesModifiable()->GetEffectiveSightRange())
		{
			HightestSightRadiusContainerContributor = Infantry;
			ThisBuilding->GetBuildingAttributesModifiable()->SetEffectiveSightRange(InfantrysSightRadius, Infantry);
		}
		/* Same deal for stealth reveal radius */
		if (InfantrysStealthRevealRadius > ThisBuilding->GetBuildingAttributesModifiable()->GetEffectiveStealthRevealRange())
		{
			HighestStealthRevealRadiusContainerContributor = Infantry;
			ThisBuilding->GetBuildingAttributesModifiable()->SetEffectiveStealthRevealRange(InfantrysStealthRevealRadius, Infantry);
		}
	}

	/* Play sound */
	if (EnterSound != nullptr)
	{
		Statics::SpawnSoundAtLocation(GameState, Infantry->GetTeam(), EnterSound, ThisBuilding->GetActorLocation(),
			ESoundFogRules::DecideOnSpawn);
	}

	/* Tell HUD */
	HUD->OnUnitEnteredBuildingGarrison(ThisBuilding, *this, Infantry, Index);
}

void FBuildingGarrisonAttributes::ServerUnloadSingleUnit(ARTSGameState * GameState, ABuilding * ThisBuilding, AActor * UnitToUnload, URTSHUD * HUD)
{
	TArray<TRawPtr(AActor)> & GarrisonedUnitsArray = NetworkType == EBuildingNetworkType::None ? GarrisonedUnits : GarrisonNetworkInfo->GetGarrisonedUnits();
	for (int32 i = GarrisonedUnitsArray.Num() - 1; i >= 0; --i)
	{
		if (GarrisonedUnitsArray[i].Get() == UnitToUnload)
		{
			/* Just going to use the 'evac all' struct - it should work fine. Maybe not the 
			*most* efficient for just a single unit but very close */
			FGarrisonEvacHistory_UnloadAllAtOnce_Grid EvacHistory = FGarrisonEvacHistory_UnloadAllAtOnce_Grid(ThisBuilding->Selectable_GetGI(), ThisBuilding, this);
			/* Calculate location unit should appear at */
			FVector EvacLocation;
			FQuat EvacRotation;
			const bool bSuccess = GetSuitableEvacuationTransform(UnitToUnload->GetWorld(), ThisBuilding,
				UnitToUnload, EvacHistory, EvacLocation, EvacRotation);

			if (bSuccess)
			{
				AInfantry * Infantry = CastChecked<AInfantry>(UnitToUnload);

				if (NetworkType == EBuildingNetworkType::None)
				{
					/* Remove from container. Not using RemoveAtSwap because it will reorder the
					order they are displayed on HUD most likely when I implement it in the future */
					GarrisonedUnitsArray.RemoveAt(i, 1, false);

					/* Update slot usage */
					OnUnitRemoved_UpdateSlotUsage(Infantry->GetInfantryAttributes()->GarrisonAttributes);
				}
				else
				{
					GarrisonNetworkInfo->ServerOnUnitExited(Infantry);
				}

				/* Update sight radius */
				OnUnitRemoved_UpdateSightAndStealthRadius(ThisBuilding, Infantry);

				/* Notify unit */
				Infantry->ServerEvacuateGarrison(EvacLocation, EvacRotation);

				/* Play sound */
				if (EvacuateSound != nullptr)
				{
					PlayEvacSound(GameState, ThisBuilding);
				}

				/* Update HUD */
				HUD->OnUnitExitedBuildingGarrison(ThisBuilding, *this, i);
			}

#if !UE_BUILD_SHIPPING
			/* Logging */
			if (bSuccess)
			{
				UE_LOG(RTSLOG, Log, TEXT("[%s]: successfully evaced unit from garrison. Unit: [%s], "
					"Evac location: [%s]"), *ThisBuilding->GetName(), *UnitToUnload->GetName(), *EvacLocation.ToCompactString());
			}
			else
			{
				UE_LOG(RTSLOG, Log, TEXT("[%s]: failed to evac unit from garrion. Unit: [%s]"),
					*ThisBuilding->GetName(), *UnitToUnload->GetName());
			}
#endif

			break;
		}
	}
}

void FBuildingGarrisonAttributes::ServerUnloadAll(ARTSGameState * GameState, ABuilding * ThisBuilding, URTSHUD * HUD)
{
	switch (UnloadAllMethod)
	{
	case EGarrisonUnloadAllMethod::AllAtOnce_Grid:
	{
		const int16 NumInGarrisonBeforeRemovals = GetNumUnitsInGarrison();

		FVector EvacLocation;
		FQuat EvacRotation;
		FGarrisonEvacHistory_UnloadAllAtOnce_Grid EvacHistory = FGarrisonEvacHistory_UnloadAllAtOnce_Grid(ThisBuilding->Selectable_GetGI(), ThisBuilding, this);
		TArray<TRawPtr(AActor)> & GarrisonedUnitsArray = NetworkType == EBuildingNetworkType::None ? GarrisonedUnits : GarrisonNetworkInfo->GetGarrisonedUnits();
		for (int32 i = GarrisonedUnitsArray.Num() - 1; i >= 0; --i)
		{
			AActor * Unit = GarrisonedUnitsArray[i].Get();
			/* Calculate location unit should appear at */
			const bool bSuccess = GetSuitableEvacuationTransform(Unit->GetWorld(), ThisBuilding,
				Unit, EvacHistory, EvacLocation, EvacRotation);

			if (bSuccess)
			{
				AInfantry * Infantry = CastChecked<AInfantry>(Unit);
				
				if (NetworkType == EBuildingNetworkType::None)
				{
					/* Remove from container. Not using RemoveAtSwap because it will reorder the
					order they are displayed on HUD most likely when I implement it in the future */
					GarrisonedUnitsArray.RemoveAt(i, 1, false);

					/* Update slot usage */
					OnUnitRemoved_UpdateSlotUsage(Infantry->GetInfantryAttributes()->GarrisonAttributes);
				}
				else
				{
					GarrisonNetworkInfo->ServerOnUnitExited(Infantry);
				}

				/* Update sight radius */
				OnUnitRemoved_UpdateSightAndStealthRadius(ThisBuilding, Infantry);
				
				/* Notify unit */
				Infantry->ServerEvacuateGarrison(EvacLocation, EvacRotation);
			}

#if !UE_BUILD_SHIPPING
			/* Logging */
			if (bSuccess)
			{
				UE_LOG(RTSLOG, Log, TEXT("[%s]: successfully evaced unit from garrison. Unit: [%s], "
					"Evac location: [%s]"), *ThisBuilding->GetName(), *Unit->GetName(), *EvacLocation.ToCompactString());
			}
			else
			{
				UE_LOG(RTSLOG, Log, TEXT("[%s]: failed to evac unit from garrion. Unit: [%s]"),
					*ThisBuilding->GetName(), *Unit->GetName());
			}
#endif
		}

		if (EvacuateSound != nullptr)
		{
			/* Play sound 1 time for each unit that leaves. 
			Possible alternatives: just play the sound once but increase it's volume multiplier 
			based on how many units leave. This is easy to do here. There is one problem 
			with clients though and that is that updates about when units leave come through 
			via OnRep funcs so you would have to remember how many leave in the frame, then 
			at the end of the frame play the sound then. I looked at UWorld::Tick and it 
			appears net drivers are always ticked as the last things in the frame no exceptions 
			so I think clients would always have a 1 frame delay on playing the sound */
			for (int32 i = 0; i < NumInGarrisonBeforeRemovals; ++i)
			{
				PlayEvacSound(GameState, ThisBuilding);
			}
		}

		/* Update HUD */
		HUD->OnBuildingGarrisonMultipleUnitsEnteredOrExited(ThisBuilding, *this, GarrisonedUnitsArray);

		break;
	}
	default:
	{
		UE_LOG(RTSLOG, Fatal, TEXT("Unaccounted for enum value: [%s]"), TO_STRING(EGarrisonUnloadAllMethod, UnloadAllMethod));
	}
	}
}

void FBuildingGarrisonAttributes::ClientOnUnitExited(ARTSGameState * GameState, ABuilding * ThisBuilding, AInfantry * ExitingInfantry, URTSHUD * HUD)
{
	FEnteringGarrisonAttributes & InfantrysGarrisonAttributes = ExitingInfantry->GetInfantryAttributesModifiable()->GarrisonAttributes;
	
	int32 Index;
	if (NetworkType == EBuildingNetworkType::None)
	{
		OnUnitRemoved_UpdateSlotUsage(InfantrysGarrisonAttributes);

		/* Not using RemoveAtSwap because when I implement showing what units are inside
		garrisons it will likely make the ordering unnatural */
		Index = GarrisonedUnits.Find(ExitingInfantry);
		GarrisonedUnits.RemoveAt(Index, 1, false);
	}
	else
	{
		Index = GarrisonNetworkInfo->ClientOnUnitExited(ExitingInfantry);
	}

	/* Update building's sight radius */
	OnUnitRemoved_UpdateSightAndStealthRadius(ThisBuilding, ExitingInfantry);
	
	if (EvacuateSound != nullptr)
	{
		PlayEvacSound(GameState, ThisBuilding);
	}

	/* Tell HUD */
	HUD->OnUnitExitedBuildingGarrison(ThisBuilding, *this, Index);
}

int32 FBuildingGarrisonAttributes::OnGarrisonedUnitZeroHealth(ABuilding * ThisBuilding, AInfantry * Unit)
{
	FEnteringGarrisonAttributes & InfantrysGarrisonAttributes = Unit->GetInfantryAttributesModifiable()->GarrisonAttributes;

	int32 Index;
	if (NetworkType == EBuildingNetworkType::None)
	{
		/* Remove it's slot usage */
		OnUnitRemoved_UpdateSlotUsage(InfantrysGarrisonAttributes);

		/* Remove unit from container. Not going to use RemoveAtSwap because that will likely
		look unnatural on the HUD when I implement showing garrisoned units on HUD */
		Index = GarrisonedUnits.Find(Unit);
		GarrisonedUnits.RemoveAt(Index, 1, false);
	}
	else
	{
		Index = GarrisonNetworkInfo->OnGarrisonedUnitZeroHealth(Unit);
	}

	/* Update sight radius */
	OnUnitRemoved_UpdateSightAndStealthRadius(ThisBuilding, Unit);

	return Index;
}


const FVector FGarrisonEvacHistory_UnloadAllAtOnce_Grid::LOCATION_INVALID = FVector(FLT_MAX);

FGarrisonEvacHistory_UnloadAllAtOnce_Grid::FGarrisonEvacHistory_UnloadAllAtOnce_Grid(URTSGameInstance * GameInst,
	ABuilding * Building, const FBuildingGarrisonAttributes * BuildingsGarrisonAttributes)
	: NumUnitsEvacced(0) 
	, bHasGivenOutFirstLocation(false) 
{
	/* The unit vector facing away from the building */
	FVector BuildingsFacingAwayVector = Building->GetActorRightVector();
	/* Check if the building is near the map bounds. If yes then we wanna choose a
	side that is not going to have units colliding with the map bounds. */
	const float MapBoundsCheckVectorLength = Building->GetBoundsLength() + 600.f;
	int32 NumFailedTests = 0;
	for (int32 i = 3; i >= 0; --i)
	{
		FVector MapBoundsCheckVector = Building->GetActorLocation() + (BuildingsFacingAwayVector * MapBoundsCheckVectorLength);
		if (GameInst->IsLocationInsideMapBounds(MapBoundsCheckVector))
		{
			FoursSidesInfo[i] = FBuildingSideInfo(BuildingsFacingAwayVector);
		}
		else
		{
			/* This side is not usable. Put LOCATION_INVALID in there to signal this */
			FoursSidesInfo[i] = FBuildingSideInfo(LOCATION_INVALID);
			NumFailedTests++;
		}

		BuildingsFacingAwayVector = BuildingsFacingAwayVector.RotateAngleAxis(90.f, FVector::UpVector);
	}

	/* It is possible that all 4 side's check failed. If that's the case then make the
	last vector tested not the invalid vector but the actual vector. If all 4 are
	flagged invalid then later on there will be nothing to try */
	if (NumFailedTests == 4)
	{
		FoursSidesInfo[0] = FBuildingSideInfo(BuildingsFacingAwayVector);
		CurrentFoursSidesInfoIndex = 0;
	}
	else
	{
		/* Assign value to CurrentFoursSidesInfoIndex */
		for (int32 i = 3; i >= 0; --i)
		{
			if (FoursSidesInfo[i].FacingAwayUnitVector != LOCATION_INVALID)
			{
				CurrentFoursSidesInfoIndex = i;
				break;
			}
		}
	}
}

FTransform FGarrisonEvacHistory_UnloadAllAtOnce_Grid::GetInitialTransformToTry(const UWorld * World, ABuilding * BuildingBeingEvaced,
	const FBuildingGarrisonAttributes * BuildingsGarrisonAttributes, ISelectable * EvacingSelectable, 
	const FSelectableRootComponent2DShapeInfo & UnitsCollisionInfo)
{
	FVector Location;
	FRotator Rotation;

	/* 
	//TODO this uses the building's GetBoundsLength() because I think I have collision 
	//on the building's mesh not it's root component so GetRootComponent2DCollisionInfo 
	//would return data for something with no collision. */
	//const float Distance = BuildingBeingEvaced->GetBoundsLength()
	//	+ UnitsCollisionInfo.GetXAxisHalfDistance()
	//	+ 100.f; // +100 so the unit isn't right up against the building
	
	{
		FBuildingSideInfo & CurrentSideInfo = FoursSidesInfo[CurrentFoursSidesInfoIndex];

		const FVector FacingAwayFromBuildingVector = CurrentSideInfo.FacingAwayUnitVector;

		/* Set rotation */
		Rotation = FacingAwayFromBuildingVector.Rotation();

		//-----------------------------------------------------------
		//	Now do location 
		//-----------------------------------------------------------
				
		const FVector PerpendicularToBuildingsFacingAwayVector = FVector::CrossProduct(FacingAwayFromBuildingVector, FVector::UpVector);
			
		if (CurrentSideInfo.NumOnCurrentRow == 0) /* First unit on row */
		{
			/* This might want to go even further though or even shift slightly
			left/right depending on what previous tests passed/failed */
			Location = FacingAwayFromBuildingVector * CurrentSideInfo.GetRecommendedShiftAwayDistance(UnitsCollisionInfo);
			Location += BuildingBeingEvaced->GetActorLocation();
		}
		else
		{
			/* Choose direction to put unit either left or right.
			We alternate between going left and right. However if one side fails to put
			a unit there then we just abandon trying to use it and go with the other side
			unless both sides have failed */
			if (CurrentSideInfo.NumLeftSideFails > CurrentSideInfo.NumRightSideFails)
			{
				CurrentSideInfo.CurrentRowDirection = EDirection::Right;
			}
			else if (CurrentSideInfo.NumLeftSideFails < CurrentSideInfo.NumRightSideFails)
			{
				CurrentSideInfo.CurrentRowDirection = EDirection::Left;
			}
			else
			{
				/* Alternate between going left and right */
				CurrentSideInfo.CurrentRowDirection = (NumUnitsEvacced % 2 == 0) ? EDirection::Left : EDirection::Right;
			}

			if (CurrentSideInfo.CurrentRowDirection == EDirection::Left)
			{
				/* Take into account the previous evacced units and failed overlap tests */

				/* Check if need to start on a new row of the grid */
				if (CurrentSideInfo.NeedToStartOnNewRow(UnitsCollisionInfo, CurrentSideInfo.CurrentRowDirection))
				{
					CurrentSideInfo.NumRows++;
					CurrentSideInfo.NumOnCurrentRow = 0;
					CurrentSideInfo.CurrentRowShiftLeftAmount = 0.f;
					CurrentSideInfo.CurrentRowShiftRightAmount = 0.f;

					/* This might want to go even further though or even shift slightly
					left/right depending on what previous tests passed/failed */
					Location = FacingAwayFromBuildingVector * CurrentSideInfo.GetRecommendedShiftAwayDistance(UnitsCollisionInfo);
					Location += BuildingBeingEvaced->GetActorLocation();
				}
				else
				{
					/* Shift away from building */
					Location = FacingAwayFromBuildingVector * CurrentSideInfo.GetRecommendedShiftAwayDistance(UnitsCollisionInfo);
					/* Shift left (parallel to building) */
					Location += (PerpendicularToBuildingsFacingAwayVector * CurrentSideInfo.GetRecommendedShiftLeftDistance(UnitsCollisionInfo));
					Location += BuildingBeingEvaced->GetActorLocation();
				}
			}
			else
			{
				/* Go right.
				Take into account the previous evacced units and failed overlap tests */

				if (CurrentSideInfo.NeedToStartOnNewRow(UnitsCollisionInfo, CurrentSideInfo.CurrentRowDirection))
				{
					CurrentSideInfo.NumRows++;
					CurrentSideInfo.NumOnCurrentRow = 0;
					CurrentSideInfo.CurrentRowShiftLeftAmount = 0.f;
					CurrentSideInfo.CurrentRowShiftRightAmount = 0.f;
					CurrentSideInfo.CurrentRowDirection = EDirection::Left;

					/* This might want to go even further though or even shift slightly
					left/right depending on what previous tests passed/failed */
					Location = FacingAwayFromBuildingVector * CurrentSideInfo.GetRecommendedShiftAwayDistance(UnitsCollisionInfo);
					Location += BuildingBeingEvaced->GetActorLocation();
				}
				else
				{
					/* Shift away from building */
					Location = FacingAwayFromBuildingVector * CurrentSideInfo.GetRecommendedShiftAwayDistance(UnitsCollisionInfo);
					/* Shift right (parallel to building) */
					Location += (PerpendicularToBuildingsFacingAwayVector * CurrentSideInfo.GetRecommendedShiftRightDistance(UnitsCollisionInfo));
					Location += BuildingBeingEvaced->GetActorLocation();
				}
			}

			/* Resolve overlaps with successful/failed spots. Relative to doing
			an overlap test with the physics system i.e. UWorld::OverlapSingleByChannel
			these checks are much, much faster.
			It may be possible to do this even faster by making stuff more event driven.
			I'm not a geometry pro so this will have to do */
			/* Idea for this func: try shift left. When that fails then go back
			to OriginalLocation and shift that away. If that fails then maybe just
			give up.
			If this func fails to find a location then set Location to
			LOCATION_INVALID */
			Location = TryResolveOverlapsWithPreviousLocations(CurrentSideInfo, Location, UnitsCollisionInfo, CurrentSideInfo.CurrentRowDirection);
			if (Location != LOCATION_INVALID)
			{
				/* Line trace so it is on ground. Otherwise unit could spawn in mid air or below landscape */
				Location = PutLocationOnGround(World, Location);
			}
		}
	}
	
	return FTransform(Rotation, Location);
}

FVector FGarrisonEvacHistory_UnloadAllAtOnce_Grid::PutLocationOnGround(const UWorld * World, const FVector & Location)
{
	FHitResult HitResult;
	const FVector TraceStart = Location + FVector(0.f, 0.f, 1500.f);
	const FVector TraceEnd = Location + FVector(0.f, 0.f, -1500.f);
	const bool bResult = Statics::LineTraceSingleByChannel(World, HitResult, TraceStart, TraceEnd, ENVIRONMENT_CHANNEL);
	assert(bResult);
	return HitResult.ImpactPoint;
}

FGarrisonEvacHistory_UnloadAllAtOnce_Grid::FUnitBasic2DCollisionInfo::FUnitBasic2DCollisionInfo(const FSelectableRootComponent2DShapeInfo & InUnitsCollisionInfo) 
	: X(InUnitsCollisionInfo.Data.X) 
	, Y(InUnitsCollisionInfo.Data.Y)
{
}

bool FGarrisonEvacHistory_UnloadAllAtOnce_Grid::FBuildingSideInfo::NeedToStartOnNewRow(const FSelectableRootComponent2DShapeInfo & UnitsCollisionInfo, EDirection InDirection) const
{
	/* 5 units on each row. But may wanna take into account unit's fatness */
	return NumOnCurrentRow == 5;
}

float FGarrisonEvacHistory_UnloadAllAtOnce_Grid::FBuildingSideInfo::GetRecommendedShiftAwayDistance(const FSelectableRootComponent2DShapeInfo & UnitsCollisionInfo) const
{
	/* Gap of 200 between rows but this is very basic TODO */
	return 200.f * (NumRows + 1);
}

float FGarrisonEvacHistory_UnloadAllAtOnce_Grid::FBuildingSideInfo::GetRecommendedShiftLeftDistance(const FSelectableRootComponent2DShapeInfo & UnitsCollisionInfo) const
{
	/* +50 for a gap of 50 between each unit */
	return CurrentRowShiftLeftAmount + UnitsCollisionInfo.GetYAxisHalfDistance() * 2.f + 50.f;
}

float FGarrisonEvacHistory_UnloadAllAtOnce_Grid::FBuildingSideInfo::GetRecommendedShiftRightDistance(const FSelectableRootComponent2DShapeInfo & UnitsCollisionInfo) const
{
	/* +50 for a gap of 50 between each unit. 
	Negative to it shifts in opposite direction to left */
	return -(CurrentRowShiftRightAmount + UnitsCollisionInfo.GetYAxisHalfDistance() * 2.f + 50.f);
}

void FGarrisonEvacHistory_UnloadAllAtOnce_Grid::NotifyOfFailedOverlapTest(ENextTest & InOutState, const FSelectableRootComponent2DShapeInfo & UnitsCollisionInfo,
	FVector & InOutLocation, FQuat & InOutRotation, const FVector & ProposedAdjustment, 
	const FVector & OriginalLocation)
{
	/* Keep a record of failed tests */
	FoursSidesInfo[CurrentFoursSidesInfoIndex].PreviousOverlapTests.Emplace(FOverlapTestResult(InOutLocation, UnitsCollisionInfo));

	/* Tell the function caller what to do next */
	switch (InOutState)
	{
	case FGarrisonEvacHistory_UnloadAllAtOnce_Grid::ENextTest::InitialTest:
	{
		if (ProposedAdjustment.IsNearlyZero())
		{
			OnAllAdjustmentsTriedForLocation(InOutState, UnitsCollisionInfo, InOutLocation, InOutRotation, OriginalLocation);
		}
		else if (FMath::IsNearlyZero(ProposedAdjustment.Z) == false)
		{
			InOutLocation.Z += ProposedAdjustment.Z;
			InOutState = FGarrisonEvacHistory_UnloadAllAtOnce_Grid::ENextTest::JustZ;
		}
		else if (FMath::IsNearlyZero(ProposedAdjustment.X) == false || FMath::IsNearlyZero(ProposedAdjustment.Y) == false)
		{
			InOutLocation.X += ProposedAdjustment.X;
			InOutLocation.Y += ProposedAdjustment.Y;
			InOutState = FGarrisonEvacHistory_UnloadAllAtOnce_Grid::ENextTest::XY;
		}
		else
		{
			InOutLocation = OriginalLocation + ProposedAdjustment;
			InOutState = FGarrisonEvacHistory_UnloadAllAtOnce_Grid::ENextTest::FullAdjustment;
		}
		break;
	}

	case FGarrisonEvacHistory_UnloadAllAtOnce_Grid::ENextTest::JustZ:
	{
		InOutLocation = OriginalLocation;
		if (FMath::IsNearlyZero(ProposedAdjustment.X) == false || FMath::IsNearlyZero(ProposedAdjustment.Y) == false)
		{
			InOutLocation.X += ProposedAdjustment.X;
			InOutLocation.Y += ProposedAdjustment.Y;
			InOutState = FGarrisonEvacHistory_UnloadAllAtOnce_Grid::ENextTest::XY;
		}
		else
		{
			InOutLocation = OriginalLocation + ProposedAdjustment;
			InOutState = FGarrisonEvacHistory_UnloadAllAtOnce_Grid::ENextTest::FullAdjustment;
		}
		break;
	}

	case FGarrisonEvacHistory_UnloadAllAtOnce_Grid::ENextTest::XY:
	{
		if (FMath::IsNearlyZero(ProposedAdjustment.Z) == false)
		{
			InOutLocation.Z += ProposedAdjustment.Z;
			InOutState = FGarrisonEvacHistory_UnloadAllAtOnce_Grid::ENextTest::XYZ;
		}
		else
		{
			InOutLocation = OriginalLocation + ProposedAdjustment;
			InOutState = FGarrisonEvacHistory_UnloadAllAtOnce_Grid::ENextTest::FullAdjustment;
		}
		break;
	}

	case FGarrisonEvacHistory_UnloadAllAtOnce_Grid::ENextTest::XYZ:
	{
		InOutLocation = OriginalLocation + ProposedAdjustment;
		InOutState = FGarrisonEvacHistory_UnloadAllAtOnce_Grid::ENextTest::FullAdjustment;
		break;
	}

	case FGarrisonEvacHistory_UnloadAllAtOnce_Grid::ENextTest::FullAdjustment:
	{
		OnAllAdjustmentsTriedForLocation(InOutState, UnitsCollisionInfo, InOutLocation, InOutRotation, OriginalLocation);
		break;
	}

	default:
	{
		assert(0); // Unaccounted for enum value
		break;
	}
	}

	/* Was doing the FullAdjustment overlap test redundent? 
	Search comment 'Now try full adjustment' in source code. I'm not 100% sure what it means 
	by 'because we did it right above'. Was it done inside that func or before calling it 
	during some SpawnActor code somewhere? */
}

void FGarrisonEvacHistory_UnloadAllAtOnce_Grid::NotifyOfSuccessfulOverlapTest(const FSelectableRootComponent2DShapeInfo & UnitsCollisionInfo,
	const FVector & Location, const FQuat & Rotation)
{
	FBuildingSideInfo & CurrentSideInfo = FoursSidesInfo[CurrentFoursSidesInfoIndex];
	CurrentSideInfo.PreviousOverlapTests.Emplace(FOverlapTestResult(Location, UnitsCollisionInfo));
	CurrentSideInfo.NumOnCurrentRow++;
	
	/* Increment of CurrentRowShiftLeft/RightAmount isn't 100% true, needs fixing TODO */
	if (CurrentSideInfo.CurrentRowDirection == EDirection::Left)
	{
		CurrentSideInfo.CurrentRowShiftLeftAmount += UnitsCollisionInfo.GetYAxisHalfDistance() * 2.f + 50.f;
	}
	else
	{
		CurrentSideInfo.CurrentRowShiftRightAmount += UnitsCollisionInfo.GetYAxisHalfDistance() * 2.f + 50.f;
	}
	NumUnitsEvacced++;
}

void FGarrisonEvacHistory_UnloadAllAtOnce_Grid::OnAllAdjustmentsTriedForLocation(ENextTest & InOutState, const FSelectableRootComponent2DShapeInfo & UnitsCollisionInfo,
	FVector & InOutLocation, FQuat & InOutRotation, const FVector & OriginalLocation)
{
	/* Could possibly find another location. Then would wanna put that into InOutLocation 
	and do InOutState = InitialTest. Also I might wanna add a variable that keeps track 
	of how many times we do this, cause you might wanna stop after a certain amount. 
	For now though we just give up */
	InOutState = ENextTest::GiveUp;
}

FVector FGarrisonEvacHistory_UnloadAllAtOnce_Grid::TryResolveOverlapsWithPreviousLocations(
	FGarrisonEvacHistory_UnloadAllAtOnce_Grid::FBuildingSideInfo & CurrentSideInfo, 
	FVector Location, 
	const FSelectableRootComponent2DShapeInfo & UnitsCollisionInfo, 
	EDirection PrefferedDirection)
{
	/* TODO want to take into account previous failed/successful locations */
	return Location;
}

bool FBuildingGarrisonAttributes::GetSuitableEvacuationTransform(const UWorld * World, ABuilding * ThisBuilding, 
	AActor * EvacingUnit, FGarrisonEvacHistory_UnloadAllAtOnce_Grid & EvacHistory, FVector & OutLocation, FQuat & OutRotation) const
{
	// Idea for anim appearing like it should when the units leaves:
	// play the idle anim the moment it enters, or perhaps make sure it doesnt blend or something
	
	/* The code here will be based off the code SpawnActor uses to avoid collisions */
	/* Another note: it assumes that previously evacced units are moved to their evac location 
	before we calculate the next unit's evac location. What I mean by this is if 5 units are 
	being evacced in the same frame then as each location is returned from this that unit then 
	needs to be moved to that location with SetActorLocationAndRotation or something similar 
	before we calculate the location for the next unit because we need it there so we make sure 
	the next unit isn't colliding with the previous ones */

	UPrimitiveComponent * RootPrimitive = CastChecked<UPrimitiveComponent>(EvacingUnit->GetRootComponent());
	ISelectable * EvacingSelectable = CastChecked<ISelectable>(EvacingUnit);
	const FSelectableRootComponent2DShapeInfo SelectablesCollisionInfo = EvacingSelectable->GetRootComponent2DCollisionInfo();

	const FTransform Transform = EvacHistory.GetInitialTransformToTry(World, ThisBuilding, this, EvacingSelectable, SelectablesCollisionInfo);
	FVector LocationToTry = Transform.GetLocation();
	FQuat RotationToTry = Transform.GetRotation();

	if (LocationToTry == FGarrisonEvacHistory_UnloadAllAtOnce_Grid::LOCATION_INVALID)
	{
		/* GetInitialTransformToTry thinks there are no locations that won't cause a collision */
		return false;
	}

	const FVector OriginalLocation = LocationToTry;

	FVector ProposedAdjustment;
	/* Try initial test */
	if (ComponentEncroachesBlockingGeometry_WithAdjustment(World, EvacingUnit, RootPrimitive, FTransform(RotationToTry, LocationToTry), ProposedAdjustment))
	{
		/* Initial test failed. Begin loop where we ask the EvacHistory what to do */
		FGarrisonEvacHistory_UnloadAllAtOnce_Grid::ENextTest NextAction = FGarrisonEvacHistory_UnloadAllAtOnce_Grid::ENextTest::InitialTest;
		while (true)
		{
			EvacHistory.NotifyOfFailedOverlapTest(NextAction, SelectablesCollisionInfo, LocationToTry, RotationToTry, ProposedAdjustment, OriginalLocation);
			if (NextAction != FGarrisonEvacHistory_UnloadAllAtOnce_Grid::ENextTest::GiveUp)
			{
				if (ComponentEncroachesBlockingGeometry_NoAdjustment(World, EvacingUnit, RootPrimitive, FTransform(RotationToTry, LocationToTry)) == false)
				{
					/* Success */
					EvacHistory.NotifyOfSuccessfulOverlapTest(SelectablesCollisionInfo, LocationToTry, RotationToTry);
					OutLocation = LocationToTry;
					OutRotation = RotationToTry;
					return true;
				}
			}
			else 
			{
				/* Give up */
				return false;
			}
		}
	}
	else
	{
		/* Success */
		EvacHistory.NotifyOfSuccessfulOverlapTest(SelectablesCollisionInfo, LocationToTry, RotationToTry);
		OutLocation = LocationToTry;
		OutRotation = RotationToTry;
		return true;
	}
}

bool FBuildingGarrisonAttributes::ComponentEncroachesBlockingGeometry_WithAdjustment(const UWorld * World, 
	const AActor * TestActor, const UPrimitiveComponent * PrimComp, 
	const FTransform & TestWorldTransform, FVector & OutProposedAdjustment)
{
	const float Epsilon = 0.15f; //CVarEncroachEpsilon.GetValueOnGameThread();

	TArray<FOverlapResult> Overlaps;
	const ECollisionChannel BlockingChannel = PrimComp->GetCollisionObjectType();
	const FCollisionShape CollisionShape = PrimComp->GetCollisionShape(-Epsilon);

	FCollisionQueryParams Params = FCollisionQueryParams(NAME_None, false, TestActor);
	FCollisionResponseParams ResponseParams;
	PrimComp->InitSweepCollisionParams(Params, ResponseParams);
	bool bFoundBlockingHit = World->OverlapMultiByChannel(Overlaps, TestWorldTransform.GetLocation(),
		TestWorldTransform.GetRotation(), BlockingChannel, CollisionShape, Params, ResponseParams);

	/* Compute adjustment */
	if (bFoundBlockingHit)
	{
		/* If encroaching, add up all the MTDs of overlapping shapes */
		FMTDResult MTDResult;
		uint32 NumBlockingHits = 0;
		for (int32 HitIdx = 0; HitIdx < Overlaps.Num(); HitIdx++)
		{
			UPrimitiveComponent * const OverlapComponent = Overlaps[HitIdx].Component.Get();
			/* First determine closest impact point along each axis */
			if (OverlapComponent != nullptr && OverlapComponent->GetCollisionResponseToChannel(BlockingChannel) == ECR_Block)
			{
				NumBlockingHits++;
				const FCollisionShape NonShrunkenCollisionShape = PrimComp->GetCollisionShape();
				bool bSuccess = OverlapComponent->ComputePenetration(MTDResult, NonShrunkenCollisionShape, TestWorldTransform.GetLocation(), TestWorldTransform.GetRotation());
				if (bSuccess)
				{
					OutProposedAdjustment += MTDResult.Direction * MTDResult.Distance;
				}
				else
				{
					UE_LOG(RTSLOG, Log, TEXT("OverlapTest says we are overlapping, yet MTD says we're not. Something is wrong"));
					OutProposedAdjustment = FVector::ZeroVector;
					return true;
				}

				// #hack: sometimes for boxes, physx returns a 0 MTD even though it reports a contact (returns true)
				// to get around this, let's go ahead and test again with the epsilon-shrunken collision shape to see if we're really in 
				// the clear.
				if (bSuccess && FMath::IsNearlyZero(MTDResult.Distance))
				{
					const FCollisionShape ShrunkenCollisionShape = PrimComp->GetCollisionShape(-Epsilon);
					bSuccess = OverlapComponent->ComputePenetration(MTDResult, ShrunkenCollisionShape, TestWorldTransform.GetLocation(), TestWorldTransform.GetRotation());
					if (bSuccess)
					{
						OutProposedAdjustment += MTDResult.Direction * MTDResult.Distance;
					}
					else
					{
						/* Ignore this overlap. */
						UE_LOG(RTSLOG, Log, TEXT("OverlapTest says we are overlapping, yet MTD says we're not (with smaller shape). Ignoring this overlap."));
						NumBlockingHits--;
						continue;
					}
				}
			}
		}

		// See if we chose to invalidate all of our supposed "blocking hits".
		if (NumBlockingHits == 0)
		{
			OutProposedAdjustment = FVector::ZeroVector;
			bFoundBlockingHit = false;
		}
	}

	return bFoundBlockingHit;
}

bool FBuildingGarrisonAttributes::ComponentEncroachesBlockingGeometry_NoAdjustment(const UWorld * World, const AActor * TestActor, const UPrimitiveComponent * PrimComp, const FTransform & TestWorldTransform)
{
	const float Epsilon = 0.15f; //CVarEncroachEpsilon.GetValueOnGameThread();

	const ECollisionChannel BlockingChannel = PrimComp->GetCollisionObjectType();
	const FCollisionShape CollisionShape = PrimComp->GetCollisionShape(-Epsilon);

	FCollisionQueryParams Params = FCollisionQueryParams(NAME_None, false, TestActor);
	FCollisionResponseParams ResponseParams;
	PrimComp->InitSweepCollisionParams(Params, ResponseParams);
	return World->OverlapBlockingTestByChannel(TestWorldTransform.GetLocation(), TestWorldTransform.GetRotation(), BlockingChannel, CollisionShape, Params, ResponseParams);
}

void FBuildingGarrisonAttributes::OnUnitAdded_UpdateSlotUsage(const FEnteringGarrisonAttributes & AddedUnitsAttributes)
{
	/* For Nones only */
	assert(NetworkType == EBuildingNetworkType::None);

	SlotUsage += AddedUnitsAttributes.GetBuildingGarrisonSlotUsage();
	assert(SlotUsage >= 0 && SlotUsage <= NumGarrisonSlots);
}

void FBuildingGarrisonAttributes::OnUnitRemoved_UpdateSlotUsage(const FEnteringGarrisonAttributes & RemovedUnitsAttributes)
{
	/* For Nones only */
	assert(NetworkType == EBuildingNetworkType::None);

	SlotUsage -= RemovedUnitsAttributes.GetBuildingGarrisonSlotUsage();
	assert(SlotUsage >= 0 && SlotUsage <= NumGarrisonSlots);
}

void FBuildingGarrisonAttributes::OnUnitRemoved_UpdateSightAndStealthRadius(ABuilding * ThisBuilding, AInfantry * RemovedUnit)
{
	if (HightestSightRadiusContainerContributor == RemovedUnit && HighestStealthRevealRadiusContainerContributor == RemovedUnit)
	{
		FBuildingAttributes * BuildingAttributes = ThisBuilding->GetBuildingAttributesModifiable();

		/* Find new unit that has highest radius for both sight range and steal reveal range */
		HightestSightRadiusContainerContributor = nullptr;
		float CurrentHighestSightRadius = BuildingAttributes->GetSightRange();
		HighestStealthRevealRadiusContainerContributor = nullptr;
		float CurrentHighestStealthRevealRadius = BuildingAttributes->GetStealthRevealRange();

		for (int32 i = 0; i < GarrisonedUnits.Num(); ++i)
		{
			ISelectable * LoopUnit = CastChecked<ISelectable>(GarrisonedUnits[i]);

			float SightRange, StealthRevealRange;
			LoopUnit->GetVisionInfo(SightRange, StealthRevealRange);
			if (SightRange > CurrentHighestSightRadius)
			{
				CurrentHighestSightRadius = SightRange;
				HightestSightRadiusContainerContributor = LoopUnit;
			}
			if (StealthRevealRange > CurrentHighestStealthRevealRadius)
			{
				CurrentHighestStealthRevealRadius = StealthRevealRange;
				HighestStealthRevealRadiusContainerContributor = LoopUnit;
			}
		}

		BuildingAttributes->SetEffectiveSightRange(CurrentHighestSightRadius, HightestSightRadiusContainerContributor);
		BuildingAttributes->SetEffectiveStealthRevealRange(CurrentHighestStealthRevealRadius, HighestStealthRevealRadiusContainerContributor);
	}
	else if (HightestSightRadiusContainerContributor == RemovedUnit)
	{
		FBuildingAttributes * BuildingAttributes = ThisBuilding->GetBuildingAttributesModifiable();

		/* Find new unit that has the highest radius */
		HightestSightRadiusContainerContributor = nullptr;
		float CurrentHighestSightRadius = BuildingAttributes->GetSightRange();

		for (int32 i = 0; i < GarrisonedUnits.Num(); ++i)
		{
			ISelectable * LoopUnit = CastChecked<ISelectable>(GarrisonedUnits[i]);

			float SightRange, StealthRevealRange;
			LoopUnit->GetVisionInfo(SightRange, StealthRevealRange);
			if (SightRange > CurrentHighestSightRadius)
			{
				CurrentHighestSightRadius = SightRange;
				HightestSightRadiusContainerContributor = LoopUnit;
			}
		}

		BuildingAttributes->SetEffectiveSightRange(CurrentHighestSightRadius, HightestSightRadiusContainerContributor);
	}
	else if (HighestStealthRevealRadiusContainerContributor == RemovedUnit)
	{
		FBuildingAttributes * BuildingAttributes = ThisBuilding->GetBuildingAttributesModifiable();

		HighestStealthRevealRadiusContainerContributor = nullptr;
		float CurrentHighestStealthRevealRadius = BuildingAttributes->GetStealthRevealRange();

		for (int32 i = 0; i < GarrisonedUnits.Num(); ++i)
		{
			ISelectable * LoopUnit = CastChecked<ISelectable>(GarrisonedUnits[i]);

			float SightRange, StealthRevealRange;
			LoopUnit->GetVisionInfo(SightRange, StealthRevealRange);
			if (StealthRevealRange > CurrentHighestStealthRevealRadius)
			{
				CurrentHighestStealthRevealRadius = StealthRevealRange;
				HighestStealthRevealRadiusContainerContributor = LoopUnit;
			}
		}

		BuildingAttributes->SetEffectiveStealthRevealRange(CurrentHighestStealthRevealRadius, HighestStealthRevealRadiusContainerContributor);
	}
}

void FBuildingGarrisonAttributes::PlayEvacSound(ARTSGameState * GameState, ABuilding * ThisBuilding)
{
	Statics::SpawnSoundAtLocation(GameState, ThisBuilding->GetTeam(), EvacuateSound,
		ThisBuilding->GetActorLocation(), ESoundFogRules::DecideOnSpawn);
}

int16 FBuildingGarrisonAttributes::GetSlotUsage() const
{
	return SlotUsage;
}

int16 FBuildingGarrisonAttributes::GetNumUnitsInGarrison() const
{
	return (NetworkType == EBuildingNetworkType::None) ? GarrisonedUnits.Num() : GarrisonNetworkInfo->GetGarrisonedUnits().Num();
}

const TArray<TRawPtr(AActor)>& FBuildingGarrisonAttributes::GetGarrisonedUnitsContainerTakingIntoAccountNetworkType() const
{
	return (NetworkType == EBuildingNetworkType::None) ? GarrisonedUnits : GarrisonNetworkInfo->GetGarrisonedUnits();
}

const FCursorInfo & FBuildingGarrisonAttributes::GetCanEnterMouseCursor(URTSGameInstance * GameInst) const
{
	return GameInst->GetMouseCursorInfo(MouseCursor_CanEnter);
}

bool FBuildingGarrisonAttributes::HasEnoughCapacityToAcceptUnit(AInfantry * UnitThatWantsToEnter) const
{
	if (NetworkType == EBuildingNetworkType::None)
	{
		return GetSlotUsage() + UnitThatWantsToEnter->GetInfantryAttributes()->GarrisonAttributes.GetBuildingGarrisonSlotUsage() <= GetTotalNumGarrisonSlots();
	}
	else
	{
		return GarrisonNetworkInfo->HasEnoughCapacityToAcceptUnit(UnitThatWantsToEnter);
	}
}


//======================================================================================
//	>FBuildingAttributes
//======================================================================================

FBuildingAttributes::FBuildingAttributes() 
	/* These two lines get done in OnPostEdit. When I do OnPostEdit inside this ctor then
	I can delete these */
	: EffectiveSightRange(SightRange) 
	, EffectiveStealthRevealRange(StealthRevealRange) 
	, ProductionQueueLimit(5) 
	, ProducedUnitInitialBehavior(EUnitInitialSpawnBehavior::CollectFromNearestResourceSpot)
	, BuildMethodOverride(EBuildingBuildMethod::None)
	, ConstructTime(0.5f)
	, ResourceDropRadius(400.f)
	, FoundationsRadius(400.f)
{
	/* Account for the fact we do not wait on a GameTickCountOnCreation variable */
	NumRequiredReppedVariablesForSetup--;
}

void FBuildingAttributes::SetEffectiveSightRange(float NewValue, ISelectable * Contributor)
{
	EffectiveSightRange = NewValue;
	GarrisonAttributes.HightestSightRadiusContainerContributor = Contributor;
}

void FBuildingAttributes::SetEffectiveStealthRevealRange(float NewValue, ISelectable * Contributor)
{
	EffectiveStealthRevealRange = NewValue;
	GarrisonAttributes.HighestStealthRevealRadiusContainerContributor = Contributor;
}

#if ALLOW_BUFFS_AND_DEBUFFS_ON_BUILDINGS 

float FBuildingAttributes::GetTimeIntoZeroHealthAnimThatAnimNotifyIsSlow() const
{
	if (IsAnimationValid(EBuildingAnimation::Destroyed))
	{
		UAnimMontage * Montage = GetAnimation(EBuildingAnimation::Destroyed);
		static const FName NotifyName = FName("OnZeroHealthAnimationFinished");

		for (const auto & Notify : Montage->Notifies)
		{
			if (Notify.NotifyName == NotifyName)
			{
				return Notify.GetTriggerTime();
			}
		}
	}

	return 0.f;
}

TArray<FStaticBuffOrDebuffInstanceInfo>& FBuildingAttributes::GetStaticBuffs()
{
	return StaticBuffs;
}

const TArray<FStaticBuffOrDebuffInstanceInfo>& FBuildingAttributes::GetStaticBuffs() const
{
	return StaticBuffs;
}

TArray<FStaticBuffOrDebuffInstanceInfo>& FBuildingAttributes::GetStaticDebuffs()
{
	return StaticDebuffs;
}

const TArray<FStaticBuffOrDebuffInstanceInfo>& FBuildingAttributes::GetStaticDebuffs() const
{
	return StaticDebuffs;
}

#if TICKABLE_BUFF_AND_DEBUFF_ENABLED_GAME

TArray<FTickableBuffOrDebuffInstanceInfo>& FBuildingAttributes::GetTickableBuffs()
{
	return TickableBuffs;
}

const TArray<FTickableBuffOrDebuffInstanceInfo>& FBuildingAttributes::GetTickableBuffs() const
{
	return TickableBuffs;
}

TArray<FTickableBuffOrDebuffInstanceInfo>& FBuildingAttributes::GetTickableDebuffs()
{
	return TickableDebuffs;
}

const TArray<FTickableBuffOrDebuffInstanceInfo>& FBuildingAttributes::GetTickableDebuffs() const
{
	return TickableDebuffs;
}

#endif // TICKABLE_BUFF_AND_DEBUFF_ENABLED_GAME

#endif // ALLOW_BUFFS_AND_DEBUFFS_ON_BUILDINGS

#if WITH_EDITOR
void FBuildingAttributes::OnPostEdit(AActor * InOwningActor)
{
	Super::OnPostEdit(InOwningActor);

	EffectiveSightRange = SightRange;
	EffectiveStealthRevealRange = StealthRevealRange;

	ABuilding * OwnerAsBuilding = CastChecked<ABuilding>(InOwningActor);

	BuildingType = OwnerAsBuilding->GetType();

	// Try this without IsTemplate check and check BlueprintCreatedComponents.Num()
	if (OwnerAsBuilding->IsTemplate() == false)
	{
		/* Set value of bIsDefenseBuilding */
		bIsDefenseBuilding = false; // Initially reset to false
		for (UActorComponent * Comp : OwnerAsBuilding->BlueprintCreatedComponents)
		{
			IBuildingAttackComp_Turret * AttackComp = Cast<IBuildingAttackComp_Turret>(Comp);
			if (AttackComp != nullptr)
			{
				bIsDefenseBuilding = true;
				break;
			}
		}
	}
	
	/* Set value of bCanProduce and bCanEditProductionProperties */
	if (ProductionQueueLimit > 0)
	{
		bCanEditProductionProperties = true;
		
		bool bFoundTrainOrUpgrade = false;

		/* Check if a train or upgrade button is present in context menu */
		for (const auto & Button : ContextMenu.GetButtonsArray())
		{
			if (Button.GetButtonType() == EContextButton::Train
				|| Button.GetButtonType() == EContextButton::Upgrade)
			{
				bFoundTrainOrUpgrade = true;
				break;
			}
		}

		bCanProduce = bFoundTrainOrUpgrade;
	}
	else
	{
		bCanEditProductionProperties = false;
		bCanProduce = false;
	}

	ProductionQueue.OnPostEdit(ProductionQueueLimit);
	PersistentProductionQueue.OnPostEdit(NumPersistentBuildSlots);

	// Populate housing resource array
	HousingResourcesProvidedArray.Init(0, Statics::NUM_HOUSING_RESOURCE_TYPES);
	for (const auto & Elem : HousingResourcesProvided)
	{
		/* This check avoids crashes when adding pairs to TMap */
		if (Elem.Key != EHousingResourceType::None)
		{
			HousingResourcesProvidedArray[Statics::HousingResourceTypeToArrayIndex(Elem.Key)] = Elem.Value;
		}
	}

	ShoppingInfo.OnPostEdit(OwnerAsBuilding);
	GarrisonAttributes.OnPostEdit();
}
#endif // WITH_EDITOR


//======================================================================================
//	>FInfantryAttributes
//======================================================================================

FInfantryAttributes::FInfantryAttributes()
{
	bCanEverEnterStealth = false;
	bSpawnStealthed = false;
	bInToggleStealthMode = bSpawnStealthed;
	StealthRevealTime = 2.f;
	StealthMaterialParamName = FName("RTS_StealthOpacity");
	InStealthParamValue = 0.5f;
	
	bCanGainExperience = true;
	// Populate experience required TMap and TArray with default values
	float EXPReq = 100.f;
	const uint32 NumContainerEnteries = LevelingUpOptions::MAX_LEVEL - LevelingUpOptions::STARTING_LEVEL;
	ExperienceRequirementsTMap.Reserve(NumContainerEnteries);
	for (uint32 i = LevelingUpOptions::STARTING_LEVEL + 1; i <= LevelingUpOptions::MAX_LEVEL; ++i)
	{
		ExperienceRequirementsTMap.Emplace(i, EXPReq);
		EXPReq *= 1.5f;

		// TArray needs populating too. It will be done in OnPostEdit
	}

	ExperienceGainMultiplier = 1.f;

	DefaultExperienceGainMultiplier = ExperienceGainMultiplier;
}

void FInfantryAttributes::TellHUDAboutChange_ExperienceGainMultiplier(ISelectable * Owner)
{
	/* We currently do not display experience gain multiplier on HUD so this is a noop */
}

bool FInfantryAttributes::ShouldSpawnStealthed() const
{
	// If thrown likely an error in post edit or you just need to set this true in editor
	UE_CLOG(bCanEverEnterStealth == false, RTSLOG, Fatal, TEXT("Unit needs bCanEverEnterStealth "
		"set to true on it "));
	
	return bSpawnStealthed;
}

float FInfantryAttributes::GetInStealthParamValue() const
{
	return InStealthParamValue;
}

USoundBase * FInfantryAttributes::GetCommandSound(EContextButton Command) const
{
	if (ContextCommandSounds.Contains(Command) && ContextCommandSounds[Command] != nullptr)
	{
		// Use sound specific to command if set
		return ContextCommandSounds[Command];
	}
	else
	{
		// Default to using move command sound
		return MoveCommandSound;
	}
}

int32 FInfantryAttributes::LevelToArrayIndex(uint32 Level) const
{
	return Level - LevelingUpOptions::STARTING_LEVEL - 1;
}

float FInfantryAttributes::GetTotalExperienceRequirementForLevel(FSelectableRankInt Level) const
{
	// Crash here? Possibly because post edit hasn't run 
	return ExperienceRequirementsArray[LevelToArrayIndex(Level.Level)];
}

TArray<FStaticBuffOrDebuffInstanceInfo>& FInfantryAttributes::GetStaticBuffs()
{
	return StaticBuffs;
}

const TArray<FStaticBuffOrDebuffInstanceInfo>& FInfantryAttributes::GetStaticBuffs() const
{
	return StaticBuffs;
}

TArray<FStaticBuffOrDebuffInstanceInfo>& FInfantryAttributes::GetStaticDebuffs()
{
	return StaticDebuffs;
}

const TArray<FStaticBuffOrDebuffInstanceInfo>& FInfantryAttributes::GetStaticDebuffs() const
{
	return StaticDebuffs;
}

#if TICKABLE_BUFF_AND_DEBUFF_ENABLED_GAME
TArray<FTickableBuffOrDebuffInstanceInfo>& FInfantryAttributes::GetTickableBuffs()
{
	return TickableBuffs;
}

const TArray<FTickableBuffOrDebuffInstanceInfo>& FInfantryAttributes::GetTickableBuffs() const
{
	return TickableBuffs;
}

TArray<FTickableBuffOrDebuffInstanceInfo>& FInfantryAttributes::GetTickableDebuffs()
{
	return TickableDebuffs;
}

const TArray<FTickableBuffOrDebuffInstanceInfo>& FInfantryAttributes::GetTickableDebuffs() const
{
	return TickableDebuffs;
}

float FInfantryAttributes::GetExperienceGainMultiplier() const
{
	return ExperienceGainMultiplier;
}

float FInfantryAttributes::GetDefaultExperienceGainMultiplier() const
{
	return DefaultExperienceGainMultiplier;
}

void FInfantryAttributes::SetExperienceGainMultiplier(float NewExperienceGainMultiplier, ISelectable * Owner)
{
	if (NumTempExperienceGainModifiers > 0)
	{
		const float Percentage = NewExperienceGainMultiplier / DefaultExperienceGainMultiplier;
		ExperienceGainMultiplier *= Percentage;
	}
	else
	{
		ExperienceGainMultiplier = NewExperienceGainMultiplier;
	}

	DefaultExperienceGainMultiplier = NewExperienceGainMultiplier;

	// Tell HUD about change
	TellHUDAboutChange_ExperienceGainMultiplier(Owner);
}

void FInfantryAttributes::SetExperienceGainMultiplierViaMultiplier(float Multiplier, ISelectable * Owner)
{
	ExperienceGainMultiplier *= Multiplier;
	DefaultExperienceGainMultiplier *= Multiplier;

	// Tell HUD about change
	TellHUDAboutChange_ExperienceGainMultiplier(Owner);
}

float FInfantryAttributes::ApplyTempExperienceGainMultiplierViaMultiplier(float Multiplier, ISelectable * Owner)
{
	NumTempExperienceGainModifiers++;
	
	ExperienceGainMultiplier *= Multiplier;
	
	// Tell HUD about change
	TellHUDAboutChange_ExperienceGainMultiplier(Owner);

	return ExperienceGainMultiplier;
}

float FInfantryAttributes::RemoveTempExperienceGainMultiplierViaMultiplier(float Multiplier, ISelectable * Owner)
{
	NumTempExperienceGainModifiers--;
	assert(NumTempExperienceGainModifiers >= 0);
	
	if (NumTempExperienceGainModifiers > 0)
	{
		ExperienceGainMultiplier *= Multiplier;
	}
	else
	{
		ExperienceGainMultiplier = DefaultExperienceGainMultiplier;
	}

	// Tell HUD about change
	TellHUDAboutChange_ExperienceGainMultiplier(Owner);

	return ExperienceGainMultiplier;
}
#endif // TICKABLE_BUFF_AND_DEBUFF_ENABLED_GAME

#if WITH_EDITOR
void FInfantryAttributes::OnPostEdit(AActor * InOwningActor)
{
	Super::OnPostEdit(InOwningActor);

	AInfantry * OwnerAsInfantry = CastChecked<AInfantry>(InOwningActor);

	UnitType = OwnerAsInfantry->GetType();

	// Fix for when sometimes adding values to EBuildingType and BuildingType gets changed to 
	// something other than NotBuilding
	BuildingType = EBuildingType::NotBuilding;

	/* Does not need to happen each post edit obviously. Although if this is placed in the ctor 
	unreal actually lets you manually turn on/off the editability in the editor. Must be 
	that the ctor runs before regular preproc. */
	bCanEditExperienceProperties = EXPERIENCE_ENABLED_GAME && bCanGainExperience;

	/* Set value for bCanBuildBuildings. Base it off a BuildBuilding button existing in 
	context menu */
	bCanBuildBuildings = false; // Initially reset to false
	for (const auto & Button : ContextMenu.GetButtonsArray())
	{
		if (Button.IsForBuildBuilding())
		{
			bCanBuildBuildings = true;
			break;
		}
	}

	/* Stealthing variables */
	if (bSpawnStealthed == true)
	{
		bCanEverEnterStealth = true;
	}
	bInToggleStealthMode = bSpawnStealthed;

	// Populate experience required TArray
	ExperienceRequirementsArray.Init(-1.f, ExperienceRequirementsTMap.Num());
	for (const auto & Elem : ExperienceRequirementsTMap)
	{
		const int32 Index = LevelToArrayIndex(Elem.Key);
		ExperienceRequirementsArray[Index] = Elem.Value;
	}

	/* DefaultExperienceGainMultiplier is not a UPROPERTY. So does this save? */
	DefaultExperienceGainMultiplier = ExperienceGainMultiplier;

	Inventory.OnPostEdit(InOwningActor);
}
#endif // WITH_EDITOR


//======================================================================================
//	>FAquiredCommanderAbilityState
//======================================================================================

FAquiredCommanderAbilityState::FAquiredCommanderAbilityState(ARTSPlayerState * AbilityOwner, const FCommanderAbilityInfo * InAbilityInfo)
	: AbilityInfo(InAbilityInfo)
	, RankAquired(0)
	, NumTimesUsed(0) 
	, GlobalSkillsPanelButtonIndex(-1)
{
	assert(AbilityOwner->GetWorld()->IsServer() || AbilityOwner->BelongsToLocalPlayer());
	
	if (InAbilityInfo->GetInitialCooldown() > 0.f)
	{
		/* Start the initial cooldown */
		AbilityOwner->StartAbilitysCooldownTimer(TimerHandle_CooldownRemaining, this, InAbilityInfo->GetInitialCooldown());
	}
}

float FAquiredCommanderAbilityState::GetCooldownRemaining(const FTimerManager & TimerManager) const
{
	return TimerManager.GetTimerRemaining(TimerHandle_CooldownRemaining);
}

float FAquiredCommanderAbilityState::GetCooldownPercent(const FTimerManager & TimerManager) const
{
	const float CooldownRemaining = GetCooldownRemaining(TimerManager);
	if (CooldownRemaining == 0.f)
	{
		return 0.f;
	}
	else
	{
		return (NumTimesUsed == 0) ? CooldownRemaining / AbilityInfo->GetInitialCooldown() : CooldownRemaining / AbilityInfo->GetCooldown();
	}
}

int32 FAquiredCommanderAbilityState::GetGlobalSkillsPanelButtonIndex() const
{
	assert(GlobalSkillsPanelButtonIndex != -1);
	return GlobalSkillsPanelButtonIndex;
}

void FAquiredCommanderAbilityState::OnAbilityUsed(ARTSPlayerState * AbilityInstigator,
	const FCommanderAbilityInfo & ThisAbilityInfo, URTSHUD * HUD)
{
	assert(AbilityInstigator->GetWorld()->IsServer() || AbilityInstigator->BelongsToLocalPlayer());
	
	NumTimesUsed++;
	assert(ThisAbilityInfo.HasUnlimitedUses() || NumTimesUsed <= ThisAbilityInfo.GetNumUses());

	/* Whether the ability has used up all its uses */
	const bool bHaveUsedAllCharges = (NumTimesUsed == ThisAbilityInfo.GetNumUses());
	
	if (!bHaveUsedAllCharges && ThisAbilityInfo.GetCooldown() > 0.f)
	{
		/* Put on cooldown */
		AbilityInstigator->StartAbilitysCooldownTimer(TimerHandle_CooldownRemaining, this, ThisAbilityInfo.GetCooldown());
	}
	
	/* Tell HUD if it was us */
	if (AbilityInstigator->BelongsToLocalPlayer())
	{
		HUD->OnCommanderAbilityUsed(ThisAbilityInfo, GlobalSkillsPanelButtonIndex, bHaveUsedAllCharges);
	}
}


//==========================================================================================
//	>FBuildingTargetingAbilityStaticInfo
//==========================================================================================

FBuildingTargetingAbilityStaticInfo::FBuildingTargetingAbilityStaticInfo()
	: AcceptableTargetMouseCursor(EMouseCursorType::None) 
	, Range(300.f)
	, bUsesAnimation(false)
	, Animation(EAnimation::None) 
	, bConsumeInstigator(true)
{
}

void FBuildingTargetingAbilityStaticInfo::InitialSetup(EBuildingTargetingAbility InType)
{
	AbilityType = InType;

	UE_CLOG(Effect_BP == nullptr, RTSLOG, Fatal, TEXT("Effect_BP for building targeting ability "
		"[%s] is null"), TO_STRING(EBuildingTargetingAbility, InType));

	EffectObject = NewObject<UBuildingTargetingAbilityBase>(GetTransientPackage(), *Effect_BP);
	EffectObject->InitialSetup();
}

void FBuildingTargetingAbilityStaticInfo::SetupMouseCursors(URTSGameInstance * GameInst, EMouseCursorType MaxEnumValue)
{
	if (AcceptableTargetMouseCursor != EMouseCursorType::None && AcceptableTargetMouseCursor != MaxEnumValue)
	{
		AcceptableTargetMouseCursor_Info = GameInst->GetMouseCursorInfo(AcceptableTargetMouseCursor);
	}
	AcceptableTargetMouseCursor_Info.SetFullPath();
}


//==========================================================================================
//	>FBuildingTargetingAbilityPerSelectableInfo
//==========================================================================================

FBuildingTargetingAbilityPerSelectableInfo::FBuildingTargetingAbilityPerSelectableInfo()
	: AbilityType(EBuildingTargetingAbility::None) 
	, bUsableAgainstOwnedBuildings(false)
	, bUsableAgainstAlliedBuildings(false)
	, bUsableAgainstHostileBuildings(true)
{
}

const FCursorInfo & FBuildingTargetingAbilityPerSelectableInfo::GetAcceptableTargetMouseCursor(URTSGameInstance * GameInst) const
{
	return GameInst->GetBuildingTargetingAbilityInfo(AbilityType).GetAcceptableTargetMouseCursor();
}

bool FBuildingTargetingAbilityPerSelectableInfo::IsAffiliationTargetable(EAffiliation Affiliation) const
{
	switch (Affiliation)
	{
	case EAffiliation::Hostile:
		return bUsableAgainstHostileBuildings;
	case EAffiliation::Allied:
		return bUsableAgainstAlliedBuildings;
	case EAffiliation::Owned:
		return bUsableAgainstOwnedBuildings;
	default:
		UE_LOG(RTSLOG, Fatal, TEXT("Unexpected affiliation value: [%s]"), TO_STRING(EAffiliation, Affiliation));
		return false;
	}
}


//======================================================================================
//	>FBuildingTargetingAbilitiesAttributes
//======================================================================================

const FBuildingTargetingAbilityPerSelectableInfo * FBuildingTargetingAbilitiesAttributes::GetAbilityInfo(EFaction BuildingsFaction, EBuildingType BuildingType, EAffiliation BuildingsAffiliation) const
{
	/* First check if unit has an ability that can target every building */
	const FFactionBuildingTypePair AllBuildingsTMapKey = FFactionBuildingTypePair(EFaction::None, EBuildingType::NotBuilding);
	if (BuildingTargetingAbilities.Contains(AllBuildingsTMapKey)
		&& BuildingTargetingAbilities[AllBuildingsTMapKey].IsAffiliationTargetable(BuildingsAffiliation))
	{
		return &BuildingTargetingAbilities[AllBuildingsTMapKey];
	}

	const FFactionBuildingTypePair TMapKey = FFactionBuildingTypePair(BuildingsFaction, BuildingType);
	if (BuildingTargetingAbilities.Contains(TMapKey)
		&& BuildingTargetingAbilities[TMapKey].IsAffiliationTargetable(BuildingsAffiliation))
		//&& BuildingTargetingAbilities[TMapKey].IsBuildingTypeTargetable(BuildingsFaction, BuildingType))
	{
		return &BuildingTargetingAbilities[TMapKey];
	}

	return nullptr;
}


//======================================================================================
//	>FProductionQueue
//======================================================================================

FProductionQueue::FProductionQueue()
{
	bHasCompletedEarly = false;
	bHasCompletedBuildsInTab = false;
	Type = EProductionQueueType::None;
}

void FProductionQueue::SetupForHumanOwner(EProductionQueueType InType, int32 InCapacity)
{
	// Assert this has not been called before
	assert(Type == EProductionQueueType::None);

	Type = InType;
	Capacity = InCapacity;
}

void FProductionQueue::SetupForCPUOwner(EProductionQueueType InType, int32 InCapacity)
{
	// Assert this has not been called before
	assert(Type == EProductionQueueType::None);
	
	Type = InType;
	Capacity = InCapacity;
}

bool FProductionQueue::HasRoom(ARTSPlayerState * OwningPlayer, bool bShowHUDWarning) const
{
	/* Check not more in queue already than there should be */
	assert(Queue.Num() <= Capacity);

	const bool bHasRoom = Queue.Num() < Capacity;

	if (bShowHUDWarning && !bHasRoom)
	{
		OwningPlayer->OnGameWarningHappened(EGameWarning::TrainingQueueFull);
	}

	return bHasRoom;
}

int32 FProductionQueue::GetCapacity() const
{
	return Capacity;
}

const FTrainingInfo & FProductionQueue::Peek() const
{
	assert(Queue.Num() > 0);
	return Queue.Last();
}

void FProductionQueue::Client_OnProductionComplete()
{
	bHasCompletedEarly = true;
}

void FProductionQueue::Server_CancelProductionOfFrontItem()
{
	// TODO
	// const uint8 ActualIndex = ProductionQueue.Num() - Index - 1;
	// Some things to do: Instantly return if nothing being produced
	// refund resources
	// Check prereqs are met for stuff next in queue
	// Call UpgradeManager->OnUpgradeCancelled
	// RPC to client to have them modify their version
}

void FProductionQueue::AddToQueue(const FTrainingInfo & TrainingInfo)
{
	Queue.Insert(TrainingInfo, 0);
}

void FProductionQueue::SetBuildingBeingProduced(ABuilding * InBuildingWeAreProducing)
{
	BuildingBeingProduced = InBuildingWeAreProducing;
}

const FTimerHandle & FProductionQueue::GetTimerHandle() const
{
	return TimerHandle_FrontOfQueue;
}

FTimerHandle & FProductionQueue::GetTimerHandle()
{
	return TimerHandle_FrontOfQueue;
}

float FProductionQueue::GetPercentageCompleteForUI(const UWorld * World) const
{
	if (bHasCompletedEarly)
	{
		assert(World->IsServer() == false);

		return 1.f;
	}
	else
	{
		if (World->GetTimerManager().IsTimerActive(TimerHandle_FrontOfQueue))
		{
			const float TimeElapsed = World->GetTimerManager().GetTimerElapsed(TimerHandle_FrontOfQueue);
			const float TimeRemaining = World->GetTimerManager().GetTimerRemaining(TimerHandle_FrontOfQueue);

			return TimeElapsed / (TimeElapsed + TimeRemaining);
		}
		else
		{
			return 0.f;
		}
	}
}

void FProductionQueue::Pop()
{
	Queue.Pop(false);
}

void FProductionQueue::Insert(const FTrainingInfo & Item, int32 Index)
{
	Queue.Insert(Item, Index);
}

EUnitType FProductionQueue::GetUnitAtFront(bool bIsOwnedByCPUPlayer) const
{
	return bIsOwnedByCPUPlayer ? AICon_Queue.Last().GetUnitType() : Queue.Last().GetUnitType();
}

TWeakObjectPtr<ABuilding>& FProductionQueue::GetBuildingBeingProduced()
{
	return BuildingBeingProduced;
}

int32 FProductionQueue::Num() const
{
	assert(Queue.Num() >= 0);
	return Queue.Num();
}

const FTrainingInfo & FProductionQueue::operator[](int32 Index) const
{
	return Queue[Index];
}

void FProductionQueue::SetHasCompletedEarly(bool bNewValue)
{
	bHasCompletedEarly = bNewValue;
}

void FProductionQueue::SetHasCompletedBuildsInTab(bool bNewValue)
{
	bHasCompletedBuildsInTab = bNewValue;
}

bool FProductionQueue::HasCompletedBuildsInTab() const
{
	return bHasCompletedBuildsInTab;
}

void FProductionQueue::OnBuildsInTabBuildingPlaced()
{
	bHasCompletedBuildsInTab = false;
	Pop();
}

EProductionQueueType FProductionQueue::GetType() const
{
	return Type;
}

#if WITH_EDITOR
void FProductionQueue::OnPostEdit(int32 NewCapacity)
{
	Capacity = NewCapacity;
}
#endif

int32 FProductionQueue::AICon_Num() const
{
	assert(AICon_Queue.Num() >= 0);
	return AICon_Queue.Num();
}

FCPUPlayerTrainingInfo FProductionQueue::AICon_Pop()
{
	return AICon_Queue.Pop(false);
}

bool FProductionQueue::AICon_HasRoom() const
{
	return AICon_Queue.Num() < Capacity;
}

void FProductionQueue::AICon_Insert(const FCPUPlayerTrainingInfo & Item, int32 Index)
{
	AICon_Queue.Insert(Item, Index);
}

const FCPUPlayerTrainingInfo & FProductionQueue::AICon_Last() const
{
	return AICon_Queue.Last();
}

const FCPUPlayerTrainingInfo & FProductionQueue::AICon_BracketOperator(int32 Index) const
{
	return AICon_Queue[Index];
}


//==========================================================================================
//	>FDisplayInfoBase
//==========================================================================================

FDisplayInfoBase::FDisplayInfoBase() 
	: Name(FText::FromString("Default Name")) 
	, HoveredButtonSound(nullptr) 
	, PressedByLMBSound(nullptr) 
	, PressedByRMBSound(nullptr) 
	, Description(FText::FromString("Default description"))
	, JustBuiltSound(nullptr)
	, TrainTime(0.2f) 
	, HUDPersistentTabType(EHUDPersistentTabType::None)
{
}

void FDisplayInfoBase::SetupCostsArray(const TMap<EResourceType, int32>& InCosts)
{
	Costs.Init(0, Statics::NUM_RESOURCE_TYPES);

	/* Move values from map into array */
	for (const auto & Elem : InCosts)
	{
		/* Crashes here indicate the resource type could be None */
		Costs[Statics::ResourceTypeToArrayIndex(Elem.Key)] = Elem.Value;
	}
}

void FDisplayInfoBase::SetPrerequisites(const TArray <EBuildingType> & ToClone_Buildings, const TArray<EUpgradeType> & ToClone_Upgrades)
{
	Prerequisites_Buildings = ToClone_Buildings;
	Prerequisites_Upgrades = ToClone_Upgrades;
}

void FDisplayInfoBase::CreatePrerequisitesText(const AFactionInfo * FactionInfo)
{
	FString String;
	
	for (const auto & Elem : Prerequisites_Buildings)
	{
		String += FactionInfo->GetBuildingInfo(Elem)->GetName().ToString() + ", ";
	}

	String = String.LeftChop(2);

	PrerequisitesText = FText::FromString(String);
}

#if WITH_EDITOR
void FDisplayInfoBase::OnPostEdit()
{
}
#endif


//==========================================================================================
//	>FUpgradeInfo
//==========================================================================================

FUpgradeInfo::FUpgradeInfo()
{
	TrainTime = 2.f;

#if WITH_EDITORONLY_DATA
	bIsUnitSetEditable = !bAffectsAllUnitTypes;
	bIsBuildingSetEditable = !bAffectsAllBuildingTypes;
#endif
}

bool FUpgradeInfo::AffectsUnitType(EUnitType UnitType) const
{
	return UnitsAffected.Contains(UnitType);
}

bool FUpgradeInfo::AffectsBuildingType(EBuildingType BuildingType) const
{
	return BuildingsAffected.Contains(BuildingType);
}

void FUpgradeInfo::AddTechTreeParent(EBuildingType BuildingType)
{
	expensiveAssert(!TechTreeParents.Contains(BuildingType));

	TechTreeParents.Emplace(BuildingType);
}

#if WITH_EDITOR
void FUpgradeInfo::OnPostEdit()
{
	Super::OnPostEdit();

	bIsUnitSetEditable = !bAffectsAllUnitTypes;
	bIsBuildingSetEditable = !bAffectsAllBuildingTypes;

	/* Populate costs array */
	Costs.Init(0, Statics::NUM_RESOURCE_TYPES);
	for (const auto & Elem : UpgradeCosts)
	{
		if (Elem.Key != EResourceType::None)
		{
			Costs[Statics::ResourceTypeToArrayIndex(Elem.Key)] = Elem.Value;
		}
	}
}
#endif // WITH_EDITOR

EBuildingType FUpgradeInfo::Random_GetTechTreeParent() const
{
	if (TechTreeParents.Num() == 0)
	{
		return EBuildingType::NotBuilding;
	}
	else
	{
		const int32 RandomIndex = FMath::RandRange(0, TechTreeParents.Num() - 1);
		return TechTreeParents[RandomIndex];
	}
}


//==========================================================================================
//	>FBuildInfo
//==========================================================================================

bool operator<(const FBuildInfo & Info1, const FBuildInfo & Info2)
{
	/* This operator is used for sorting buildings and units.
	Make sure they are the same type */
	assert(Info1.SelectableType == Info2.SelectableType);

	if (Info1.SelectableType == ESelectableType::Building)
	{
		/* Make sure they are not the same type */
		assert(Info1.BuildingType != Info2.BuildingType);

		/* Make sure unit type is NotUnit for both */
		assert(Info1.UnitType == EUnitType::NotUnit);
		assert(Info2.UnitType == EUnitType::NotUnit);

		const uint8 Int1 = static_cast<uint8>(Info1.BuildingType);
		const uint8 Int2 = static_cast<uint8>(Info2.BuildingType);

		return Int1 < Int2;
	}
	else // Assumed unit
	{
		assert(Info1.SelectableType == ESelectableType::Unit);

		/* Make sure they are not the same type */
		assert(Info1.UnitType != Info2.UnitType);

		/* Make sure building type is NotBuilding for both */
		assert(Info1.BuildingType == EBuildingType::NotBuilding);
		assert(Info2.BuildingType == EBuildingType::NotBuilding);

		const uint8 Int1 = static_cast<uint8>(Info1.UnitType);
		const uint8 Int2 = static_cast<uint8>(Info2.UnitType);

		return Int1 < Int2;
	}
}

FContextButton FBuildInfo::ConstructBuildButton() const
{
	return FContextButton(this);
}


//==========================================================================================
//	>FBuildingInfo
//==========================================================================================

FBuildingInfo::FBuildingInfo()
{
	BuildingBuildMethod = EBuildingBuildMethod::None;
}

void FBuildingInfo::AddTechTreeParent(EBuildingType BuildingThatCanBuildThis)
{
	expensiveAssert(!TechTreeParentBuildings.Contains(BuildingThatCanBuildThis));

	TechTreeParentBuildings.Emplace(BuildingThatCanBuildThis);
}

void FBuildingInfo::AddTechTreeParent(EUnitType UnitThatCanBuildThis)
{
	expensiveAssert(!TechTreeParentUnits.Contains(UnitThatCanBuildThis));

	TechTreeParentUnits.Emplace(UnitThatCanBuildThis);
}

bool FBuildingInfo::IsAConstructionYardType() const
{
	return NumPersistentQueues > 0;
}

bool FBuildingInfo::IsABarracksType() const
{
	return bIsBarracksType;
}

bool FBuildingInfo::IsABaseDefenseType() const
{
	return bIsBaseDefenseType;
}

bool FBuildingInfo::CanBeBuiltByConstructionYard() const
{
	return TechTreeParentBuildings.Num() > 0 && IsBuildMethodSupportedByConstructionYards();
}

bool FBuildingInfo::IsBuildMethodSupportedByConstructionYards() const
{
	return BuildingBuildMethod == EBuildingBuildMethod::BuildsInTab 
		|| BuildingBuildMethod == EBuildingBuildMethod::BuildsItself;
}

bool FBuildingInfo::CanBeBuiltByWorkers() const
{
	return TechTreeParentUnits.Num() > 0 && IsBuildMethodSupportedByUnits();
}

bool FBuildingInfo::IsBuildMethodSupportedByUnits() const
{
	return !IsBuildMethodSupportedByConstructionYards();
}

const TArray<EBuildingType>& FBuildingInfo::GetTechTreeParentBuildings() const
{
	return TechTreeParentBuildings;
}

const TArray<EUnitType>& FBuildingInfo::GetTechTreeParentUnits() const
{
	return TechTreeParentUnits;
}

EBuildingType FBuildingInfo::Random_GetTechTreeParentBuilding() const
{
	if (TechTreeParentBuildings.Num() == 0)
	{
		return EBuildingType::NotBuilding;
	}
	else
	{
		const int32 RandomIndex = FMath::RandRange(0, TechTreeParentBuildings.Num() - 1);
		return TechTreeParentBuildings[RandomIndex];
	}
}

EUnitType FBuildingInfo::Random_GetTechTreeParentUnit() const
{
	if (TechTreeParentUnits.Num() == 0)
	{
		return EUnitType::None;
	}
	else
	{
		const int32 RandomIndex = FMath::RandRange(0, TechTreeParentUnits.Num() - 1);
		return TechTreeParentUnits[RandomIndex];
	}
}


//==========================================================================================
//	>FUnitInfo
//==========================================================================================

void FUnitInfo::SetupHousingCostsArray(const TMap<EHousingResourceType, int16>& InCostMap)
{
	HousingCosts.Init(0, Statics::NUM_HOUSING_RESOURCE_TYPES);

	for (const auto & Elem : InCostMap)
	{
		if (Elem.Key != EHousingResourceType::None)
		{
			HousingCosts[Statics::HousingResourceTypeToArrayIndex(Elem.Key)] = Elem.Value;
		}
	}
}

const TArray<int16>& FUnitInfo::GetHousingCosts() const
{
	return HousingCosts;
}

float FUnitInfo::GetImpactDamage() const
{
	return ImpactDamage;
}

TSubclassOf<URTSDamageType> FUnitInfo::GetImpactDamageType() const
{
	return ImpactDamageType;
}

float FUnitInfo::GetImpactRandomDamageFactor() const
{
	return ImpactRandomDamageFactor;
}

float FUnitInfo::GetAoEDamage() const
{
	return AoEDamage;
}

TSubclassOf<URTSDamageType> FUnitInfo::GetAoEDamageType() const
{
	return AoEDamageType;
}

float FUnitInfo::GetAoERandomDamageFactor() const
{
	return AoERandomDamageFactor;
}

void FUnitInfo::SetDamageValues(UWorld * World, const FAttackAttributes & AttackAttributes)
{
	if (AttackAttributes.ShouldOverrideProjectileDamageValues())
	{
		ImpactDamage = AttackAttributes.GetImpactDamage();
		ImpactDamageType = AttackAttributes.GetImpactDamageType();
		ImpactRandomDamageFactor = AttackAttributes.GetImpactRandomDamageFactor();
		AoEDamage = AttackAttributes.GetAoEDamage();
		AoEDamageType = AttackAttributes.GetAoEDamageType();
		AoERandomDamageFactor = AttackAttributes.GetAoERandomDamageFactor();
	}
	else
	{
		/* Need to spawn the projectile and store damage values from it. Calling SpawnActorDeferred 
		BeginPlay isn't run and therefore it is not added to object pool, but I guess we could 
		allow that if it works */
		AProjectileBase * Projectile = World->SpawnActorDeferred<AProjectileBase>(AttackAttributes.GetProjectileBP(), 
			FTransform(FRotator::ZeroRotator, Statics::POOLED_ACTOR_SPAWN_LOCATION, FVector::OneVector));
		
		ImpactDamage = Projectile->GetImpactDamage();
		ImpactDamageType = Projectile->GetImpactDamageType();
		ImpactRandomDamageFactor = Projectile->GetImpactRandomDamageFactor();
		AoEDamage = Projectile->GetAoEDamage();
		AoEDamageType = Projectile->GetAoEDamageType();
		AoERandomDamageFactor = Projectile->GetAoERandomDamageFactor();

		/* Projectile no longer needed */
		Projectile->Destroy();
	}
}

void FUnitInfo::AddTechTreeParent(EBuildingType Building)
{
	expensiveAssert(!TechTreeParents.Contains(Building));

	TechTreeParents.Emplace(Building);
}

void FUnitInfo::SetIsACollectorType(bool bInValue)
{
	bIsCollectorType = bInValue;
}

void FUnitInfo::SetIsAWorkerType(bool bInValue)
{
	bIsWorkerType = bInValue;
}

void FUnitInfo::SetIsAAttackingType(bool bInValue)
{
	bIsArmyType = bInValue;
}

bool FUnitInfo::IsTrainable() const
{
	return TechTreeParents.Num() > 0;
}

bool FUnitInfo::IsACollectorType() const
{
	return bIsCollectorType;
}

bool FUnitInfo::IsAWorkerType() const
{
	return bIsWorkerType;
}

bool FUnitInfo::IsAArmyUnitType() const
{
	return bIsArmyType;
}

const TArray<EBuildingType>& FUnitInfo::GetTechTreeParents() const
{
	return TechTreeParents;
}

EBuildingType FUnitInfo::Random_GetTechTreeParent() const
{
	if (TechTreeParents.Num() == 0)
	{
		return EBuildingType::NotBuilding;
	}
	else
	{
		const int32 RandomIndex = FMath::RandRange(0, TechTreeParents.Num() - 1);
		return TechTreeParents[RandomIndex];
	}
}


//==========================================================================================
//	>StaticBuffOrDebuffInstanceInfo
//==========================================================================================

FStaticBuffOrDebuffInstanceInfo::FStaticBuffOrDebuffInstanceInfo() 
	: Info(nullptr) 
	, SpecificType(EStaticBuffAndDebuffType::None) 
	, Instigator(nullptr)
	, InstigatorAsSelectable(nullptr)
{
}

FStaticBuffOrDebuffInstanceInfo::FStaticBuffOrDebuffInstanceInfo(const FStaticBuffOrDebuffInfo * InInfo,
	AActor * InInstigator, ISelectable * InInstigatorAsSelectable) 
	: Info(InInfo) 
	, SpecificType(InInfo->GetSpecificType())
	, Instigator(InInstigator) 
	, InstigatorAsSelectable(InInstigatorAsSelectable)
{
}

AActor * FStaticBuffOrDebuffInstanceInfo::GetInstigator() const
{
	return Instigator.Get();
}

ISelectable * FStaticBuffOrDebuffInstanceInfo::GetInstigatorAsSelectable() const
{
	return InstigatorAsSelectable;
}

const FStaticBuffOrDebuffInfo * FStaticBuffOrDebuffInstanceInfo::GetInfoStruct() const
{
	return Info;
}

EStaticBuffAndDebuffType FStaticBuffOrDebuffInstanceInfo::GetSpecificType() const
{
	return SpecificType;
}


//==========================================================================================
//	>TickableBuffOrDebuffInstanceInfo
//==========================================================================================

FTickableBuffOrDebuffInstanceInfo::FTickableBuffOrDebuffInstanceInfo()
	: Info(nullptr)
	, SpecificType(ETickableBuffAndDebuffType::None) 
	, TickCount(0) 
	, TimeRemainingTillNextTick(0.f) 
	, Instigator(nullptr) 
	, InstigatorAsSelectable(nullptr)
{
}

FTickableBuffOrDebuffInstanceInfo::FTickableBuffOrDebuffInstanceInfo(const FTickableBuffOrDebuffInfo * InInfo,
	AActor * InInstigator, ISelectable * InInstigatorAsSelectable)
	: Info(InInfo) 
	, SpecificType(InInfo->GetSpecificType())
	, TickCount(0)
	, TimeRemainingTillNextTick(InInfo->GetTickInterval())
	, Instigator(InInstigator)
	, InstigatorAsSelectable(InInstigatorAsSelectable)
{
}

void FTickableBuffOrDebuffInstanceInfo::ResetDuration()
{
	/* Not logical to call this if the buff/debuff never expires */
	assert(Info->ExpiresOverTime());
	
	TickCount = 0;
	TimeRemainingTillNextTick = Info->GetTickInterval();
}

uint8 FTickableBuffOrDebuffInstanceInfo::GetTickCount() const
{
	/* If this buff/debuff never expires on its own then it is quite possible the value returned 
	by this could have overflown since it is only 1 byte. Therefore infinite duration buffs/debuffs 
	should not call this */
	assert(Info->ExpiresOverTime());
	
	return TickCount;
}

AActor * FTickableBuffOrDebuffInstanceInfo::GetInstigator() const
{
	return Instigator.Get();
}

ISelectable * FTickableBuffOrDebuffInstanceInfo::GetInstigatorAsSelectable() const
{
	return InstigatorAsSelectable;
}

const FTickableBuffOrDebuffInfo * FTickableBuffOrDebuffInstanceInfo::GetInfoStruct() const
{
	return Info;
}

ETickableBuffAndDebuffType FTickableBuffOrDebuffInstanceInfo::GetSpecificType() const
{
	assert(SpecificType != ETickableBuffAndDebuffType::None);

	return SpecificType;
}

void FTickableBuffOrDebuffInstanceInfo::DecrementDeltaTime(float DeltaTime)
{
	TimeRemainingTillNextTick -= DeltaTime;
}

float FTickableBuffOrDebuffInstanceInfo::GetTimeRemainingTillNextTick() const
{
	return TimeRemainingTillNextTick;
}

void FTickableBuffOrDebuffInstanceInfo::IncreaseTimeRemainingTillNextTick(float Amount)
{
	TimeRemainingTillNextTick += Amount;
}

void FTickableBuffOrDebuffInstanceInfo::IncrementTickCount()
{
	TickCount++;
}

float FTickableBuffOrDebuffInstanceInfo::CalculateDurationRemaining() const
{
	/* Maybe able to structure this struct a bit differently to make this more efficient 
	e.g. store NumTicksRemaining instead of TickCount */
	
	const uint8 NumTicksRemaining = Info->GetNumberOfTicks() - TickCount;
	
	/* Buff/debuff should have been removed if it has no ticks remaining. Maybe will need to 
	relax this if we do end up needing to call this even if they have expired */
	assert(NumTicksRemaining > 0);

	return ((NumTicksRemaining - 1) * Info->GetTickInterval()) + TimeRemainingTillNextTick;
}


//==========================================================================================
//	>FPlayerStateArray
//==========================================================================================

void FPlayerStateArray::AddToArray(ARTSPlayerState * PlayerState)
{
	assert(PlayerState != nullptr);
	PlayerStates.Emplace(PlayerState);
}

const TArray <ARTSPlayerState *> & FPlayerStateArray::GetPlayerStates() const
{
	return PlayerStates;
}


//==========================================================================================
//	>FParticleInfo
//==========================================================================================

FParticleInfo::FParticleInfo()
{
	Template = nullptr;
	Transform = FTransform::Identity;
}

UParticleSystem * FParticleInfo::GetTemplate() const
{
	return Template;
}

const FTransform & FParticleInfo::GetTransform() const
{
	return Transform;
}


//=========================================================================================
//	>FAttachedParticleInfo
//=========================================================================================

FAttachedParticleInfo::FAttachedParticleInfo()
{
	// Should never be called
	//assert(0);
}

FAttachedParticleInfo::FAttachedParticleInfo(UParticleSystemComponent * InPSC, const FTickableBuffOrDebuffInfo & BuffOrDebuffInfo)
{
	assert(InPSC != nullptr);
	PSC = InPSC;
	UniqueID = GenerateUniqueID(BuffOrDebuffInfo);
}

FAttachedParticleInfo::FAttachedParticleInfo(UParticleSystemComponent * InPSC, const FStaticBuffOrDebuffInfo & BuffOrDebuffInfo)
{
	assert(InPSC != nullptr);
	PSC = InPSC;
	UniqueID = GenerateUniqueID(BuffOrDebuffInfo);
}

FAttachedParticleInfo::FAttachedParticleInfo(UParticleSystemComponent * InPSC, const FContextButtonInfo & AbilityInfo, uint32 Index)
{
	assert(InPSC != nullptr);
	PSC = InPSC;
	UniqueID = GenerateUniqueID(AbilityInfo, Index);
}

FAttachedParticleInfo::FAttachedParticleInfo(const FTickableBuffOrDebuffInfo & BuffOrDebuffInfo)
{
	UniqueID = GenerateUniqueID(BuffOrDebuffInfo);
}

FAttachedParticleInfo::FAttachedParticleInfo(const FStaticBuffOrDebuffInfo & BuffOrDebuffInfo)
{
	UniqueID = GenerateUniqueID(BuffOrDebuffInfo);
}

FAttachedParticleInfo::FAttachedParticleInfo(const FContextButtonInfo & AbilityInfo, uint32 Index)
{
	UniqueID = GenerateUniqueID(AbilityInfo, Index);
}

uint32 FAttachedParticleInfo::GenerateUniqueID(const FTickableBuffOrDebuffInfo & BuffOrDebuffInfo)
{
	return static_cast<uint32>(BuffOrDebuffInfo.GetSpecificType());
}

uint32 FAttachedParticleInfo::GenerateUniqueID(const FStaticBuffOrDebuffInfo & BuffOrDebuffInfo)
{
	return static_cast<uint32>(BuffOrDebuffInfo.GetSpecificType());
}

uint32 FAttachedParticleInfo::GenerateUniqueID(const FContextButtonInfo & AbilityInfo, uint32 Index)
{
	// 256 skips past buffs/debuffs.
	return 256 + static_cast<uint32>(AbilityInfo.GetButtonType()) * 8 + Index;
}


//==========================================================================================
//	>FDecalInfo
//==========================================================================================

FDecalInfo::FDecalInfo() 
	: Decal(nullptr) 
	, InitialDuration(1.f) 
	, FadeDuration(1.f)
{
}

UMaterialInterface * FDecalInfo::GetDecal() const
{
	return Decal;
}

float FDecalInfo::GetInitialDuration() const
{
	return InitialDuration;
}

float FDecalInfo::GetFadeDuration() const
{
	return FadeDuration;
}


//==========================================================================================
//	>FStartingSelectables
//==========================================================================================

void FStartingSelectables::AddStartingSelectable(EBuildingType InBuildingType)
{
	assert(InBuildingType != EBuildingType::NotBuilding);
	Buildings.Emplace(InBuildingType);
}

void FStartingSelectables::AddStartingSelectable(EUnitType InUnitType)
{
	assert(InUnitType != EUnitType::None && InUnitType != EUnitType::NotUnit);
	Units.Emplace(InUnitType);
}

void FStartingSelectables::SetStartingBuildings(const TArray<EBuildingType>& SelectableTypes)
{
	Buildings = SelectableTypes;
}

void FStartingSelectables::SetStartingUnits(const TArray<EUnitType>& SelectableTypes)
{
	Units = SelectableTypes;
}

const TArray<EBuildingType>& FStartingSelectables::GetBuildings() const
{
	return Buildings;
}

const TArray<EUnitType>& FStartingSelectables::GetUnits() const
{
	return Units;
}


