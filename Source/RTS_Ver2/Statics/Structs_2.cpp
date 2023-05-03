// Fill out your copyright notice in the Description page of Project Settings.

#include "Structs_2.h"

#include "Statics/DevelopmentStatics.h"
#include "MapElements/Projectiles/NoCollisionTimedProjectile.h"
#include "Miscellaneous/CPUPlayerAIController.h"
#include "MapElements/RTSPlayerStart.h"
#include "MapElements/AIControllers/InfantryController.h"
#include "GameFramework/Selectable.h"
#include "GameFramework/RTSPlayerController.h"
#include "UI/RTSHUD.h"
#include "Statics/Statics.h"
#include "GameFramework/RTSPlayerState.h"


//=============================================================================================
//	>FUnifiedMouseFocusImage
//=============================================================================================

FUnifiedMouseFocusImage::FUnifiedMouseFocusImage() 
	: bEnabled(false) 
{
}


//=============================================================================================
//	>FVisibilityInfo
//=============================================================================================

void FVisibilityInfo::AddToMap(AActor * Actor)
{
	assert(Actor != nullptr);
	/* TODO: false is the correct value. Selectables need to spawn as
	'hidden' and then become revealed very quickly by fog of war manager */
	Actors.Emplace(Actor, false);
}

void FVisibilityInfo::RemoveFromMap(AActor * Actor)
{
	assert(Actor != nullptr);
	Actors.Remove(Actor);
}

bool FVisibilityInfo::IsVisible(const AActor * Actor) const
{
	assert(Actor != nullptr);

	return Actors[Actor];
}

void FVisibilityInfo::SetVisibility(const AActor * Actor, bool bNewVisibility)
{
	assert(Actor != nullptr);

	Actors[Actor] = bNewVisibility;
}


//=============================================================================================
//	>FAttackAttributes
//=============================================================================================

void FAttackAttributes::TellHUDAboutChange_ImpactDamage(ISelectable * Owner)
{
	Owner->GetLocalPC()->GetHUDWidget()->Selected_OnImpactDamageChanged(Owner, DamageInfo.ImpactDamage,
		Owner->GetAttributesBase().IsPrimarySelected());
}

void FAttackAttributes::TellHUDAboutChange_ImpactDamageType(ISelectable * Owner)
{
	/* For now we just do nothing since the HUD does not display any information about damage 
	types because there is currently no information on them other than their damage multipliers 
	against different armor types. Wouldn't be too hard to add extra info though. 
	Could just add more fields to that DamageMultipliers TMap in GI such as a FText 
	display name and UTexture2D */
}

void FAttackAttributes::TellHUDAboutChange_AttackRate(ISelectable * Owner)
{
	Owner->GetLocalPC()->GetHUDWidget()->Selected_OnAttackRateChanged(Owner, AttackRate,
		Owner->GetLocalPC()->GetCurrentSelected() == Owner);
}

void FAttackAttributes::TellHUDAboutChange_AttackRange(ISelectable * Owner)
{
	Owner->GetLocalPC()->GetHUDWidget()->Selected_OnAttackRangeChanged(Owner, AttackRange,
		Owner->GetLocalPC()->GetCurrentSelected() == Owner);
}

FAttackAttributes::FAttackAttributes()
{
	bOverrideProjectileDamageValues = false;
	AttackRate = 1.f;
	AttackRange = 600.f;
	// Note if wanting to change this then edit next 2 lines  
	AcceptableTargetTypes.Emplace(ETargetingType::Default);
	AcceptableTargetFNames.Emplace(Statics::GetTargetingType(ETargetingType::Default));
	bCanAttackAir = false;
	Projectile_BP = ANoCollisionTimedProjectile::StaticClass();
	MuzzleShakeRadius = 300.f;
	MuzzleShakeFalloff = 1.f;
	MuzzleSocket = FName("Muzzle");

	DefaultAttackRate = AttackRate;
	DefaultAttackRange = AttackRange;
}

void FAttackAttributes::SetValues(bool bInOverrideProjectileDamageValues, float InImpactDamage, 
	TSubclassOf<URTSDamageType> InImpactDamageType, float InImpactRandomDamageFactor,
	float InAoEDamage, TSubclassOf < URTSDamageType > InAoEDamageType, float InAoERandomDamageFactor,
	TSubclassOf<AProjectileBase> InProjectileBP)
{
	bOverrideProjectileDamageValues = bInOverrideProjectileDamageValues;
	DamageInfo.SetValues(InImpactDamage, InImpactDamageType, InImpactRandomDamageFactor,
		InAoEDamage, InAoEDamageType, InAoERandomDamageFactor);
	Projectile_BP = InProjectileBP;

	DefaultImpactDamage = InImpactDamage;
	DefaultAoEDamage = InAoEDamage;
}

void FAttackAttributes::SetValues(float InImpactDamage, TSubclassOf < URTSDamageType > InImpactDamageType, float InImpactRandomDamageFactor,
	float InAoEDamage, TSubclassOf < URTSDamageType > InAoEDamageType, float InAoERandomDamageFactor)
{
	DamageInfo.SetValues(InImpactDamage, InImpactDamageType, InImpactRandomDamageFactor, 
		InAoEDamage, InAoEDamageType, InAoERandomDamageFactor);

	DefaultImpactDamage = InImpactDamage;
	DefaultAoEDamage = InAoEDamage;
}

void FAttackAttributes::SetDamageValues(const FUnitInfo * UnitInfo)
{
	DamageInfo.SetValues(UnitInfo->GetImpactDamage(), UnitInfo->GetImpactDamageType(), UnitInfo->GetImpactRandomDamageFactor(), 
		UnitInfo->GetAoEDamage(), UnitInfo->GetAoEDamageType(), UnitInfo->GetAoERandomDamageFactor());

	DefaultImpactDamage = DamageInfo.ImpactDamage;
	DefaultAoEDamage = DamageInfo.AoEDamage;
}

const TSet <FName> & FAttackAttributes::GetAcceptableTargetTypes() const
{
	return AcceptableTargetFNames;
}

UParticleSystem * FAttackAttributes::GetMuzzleParticles() const
{
	return MuzzleParticles;
}

TSubclassOf<AProjectileBase> FAttackAttributes::GetProjectileBP() const
{
	return Projectile_BP;
}

bool FAttackAttributes::ShouldOverrideProjectileDamageValues() const
{
	return bOverrideProjectileDamageValues;
}

const FName & FAttackAttributes::GetMuzzleSocket() const
{
	return MuzzleSocket;
}

TSubclassOf<UCameraShake> FAttackAttributes::GetMuzzleCameraShakeBP() const
{
	return MuzzleCameraShake_BP;
}

float FAttackAttributes::GetMuzzleShakeRadius() const
{
	return MuzzleShakeRadius;
}

float FAttackAttributes::GetMuzzleShakeFalloff() const
{
	return MuzzleShakeFalloff;
}

USoundBase * FAttackAttributes::GetPreparationSound() const
{
	return PreparationSound;
}

USoundBase * FAttackAttributes::GetAttackMadeSound() const
{
	return AttackMadeSound;
}

float FAttackAttributes::GetImpactDamage() const
{
	return DamageInfo.ImpactDamage;
}

float FAttackAttributes::GetDefaultImpactDamage() const
{
	return DefaultImpactDamage;
}

void FAttackAttributes::SetImpactDamage(float NewDamage, ISelectable * Owner)
{
	if (NumTempImpactDamageModifiers > 0)
	{
		const float Percentage = NewDamage / DefaultImpactDamage;
		DamageInfo.ImpactDamage *= Percentage;
	}
	else
	{
		DamageInfo.ImpactDamage = NewDamage;
	}

	DefaultImpactDamage = NewDamage;

	// Tell HUD about change
	TellHUDAboutChange_ImpactDamage(Owner);
}

void FAttackAttributes::SetImpactDamageViaMultiplier(float Multiplier, ISelectable * Owner)
{
	DamageInfo.ImpactDamage *= Multiplier;
	DefaultImpactDamage *= Multiplier;

	// Tell HUD about change
	TellHUDAboutChange_ImpactDamage(Owner);
}

float FAttackAttributes::ApplyTempImpactDamageModifierViaMultiplier(float Multiplier, ISelectable * Owner)
{
	NumTempImpactDamageModifiers++;

	DamageInfo.ImpactDamage *= Multiplier;
	
	// Tell HUD about change
	TellHUDAboutChange_ImpactDamage(Owner);

	return DamageInfo.ImpactDamage;
}

float FAttackAttributes::RemoveTempImpactDamageModifierViaMultiplier(float Multiplier, ISelectable * Owner)
{
	NumTempImpactDamageModifiers--;
	assert(NumTempImpactDamageModifiers >= 0);

	if (NumTempImpactDamageModifiers > 0)
	{
		DamageInfo.ImpactDamage *= Multiplier;
	}
	else
	{
		DamageInfo.ImpactDamage = DefaultImpactDamage;
	}
	
	// Tell HUD about change
	TellHUDAboutChange_ImpactDamage(Owner);

	return DamageInfo.ImpactDamage;
}

const TSubclassOf<URTSDamageType>& FAttackAttributes::GetImpactDamageType() const
{
	return DamageInfo.ImpactDamageType;
}

void FAttackAttributes::SetImpactDamageType(TSubclassOf<URTSDamageType>& NewDamageType, ISelectable * Owner)
{
	DamageInfo.ImpactDamageType = NewDamageType;

	// Tell HUD about change
	TellHUDAboutChange_ImpactDamageType(Owner);
}

float FAttackAttributes::GetImpactRandomDamageFactor() const
{
	return DamageInfo.ImpactRandomDamageFactor;
}

float FAttackAttributes::GetAoEDamage() const
{
	return DamageInfo.AoEDamage;
}

const TSubclassOf<URTSDamageType>& FAttackAttributes::GetAoEDamageType() const
{
	return DamageInfo.AoEDamageType;
}

float FAttackAttributes::GetAoERandomDamageFactor() const
{
	return DamageInfo.AoERandomDamageFactor;
}

bool FAttackAttributes::CanAttackAir() const
{
	return bCanAttackAir;
}

void FAttackAttributes::SetCanAttackAir(bool bNewValue)
{
	bCanAttackAir = bNewValue;
}

float FAttackAttributes::GetAttackRate() const
{
	return AttackRate;
}

float FAttackAttributes::GetDefaultAttackRate() const
{
	return DefaultAttackRate;
}

void FAttackAttributes::SetAttackRate(float NewAttackRate, ISelectable * Owner)
{
	if (NumTempAttackRateModifiers > 0)
	{
		const float Percentage = NewAttackRate / DefaultAttackRate;
		AttackRate *= Percentage;
	}
	else
	{
		AttackRate = NewAttackRate;
	}

	DefaultAttackRate = NewAttackRate;

	/* Tell HUD about change */
	TellHUDAboutChange_AttackRate(Owner);
}

void FAttackAttributes::SetAttackRateViaMultiplier(float Multiplier, ISelectable * Owner)
{
	AttackRate *= Multiplier;
	DefaultAttackRate *= Multiplier;

	/* Tell HUD about change */
	TellHUDAboutChange_AttackRate(Owner);
}

float FAttackAttributes::ApplyTempAttackRateModifierViaMultiplier(float Multiplier, ISelectable * Owner)
{
	NumTempAttackRateModifiers++;

	AttackRate *= Multiplier;
	
	/* Tell HUD about change */
	TellHUDAboutChange_AttackRate(Owner);

	return GetAttackRate();
}

float FAttackAttributes::RemoveTempAttackRateModifierViaMultiplier(float Multiplier, ISelectable * Owner)
{
	NumTempAttackRateModifiers--;
	assert(NumTempAttackRateModifiers >= 0);

	if (NumTempAttackRateModifiers > 0)
	{
		AttackRate *= Multiplier;
	}
	else
	{
		AttackRate = DefaultAttackRate;
	}
	
	/* Tell HUD about change */
	TellHUDAboutChange_AttackRate(Owner);

	return AttackRate;
}

float FAttackAttributes::GetAttackRange() const
{
	return AttackRange;
}

float FAttackAttributes::GetDefaultAttackRange() const
{
	return DefaultAttackRange;
}

void FAttackAttributes::SetAttackRange(float NewAttackRange, AInfantryController * AIController, 
	ISelectable * Owner)
{
	if (NumTempAttackRangeModifiers > 0)
	{
		const float Percentage = NewAttackRange / DefaultAttackRange;
		AttackRange *= Percentage;
	}
	else
	{
		AttackRange = NewAttackRange;
	}

	DefaultAttackRange = NewAttackRange;

	AIController->OnUnitRangeChanged(AttackRange);

	TellHUDAboutChange_AttackRange(Owner);
}

void FAttackAttributes::SetAttackRangeViaMultiplier(float Multiplier, AInfantryController * AIController,
	ISelectable * Owner)
{
	AttackRange *= Multiplier;
	DefaultAttackRange *= Multiplier;

	AIController->OnUnitRangeChanged(AttackRange);

	TellHUDAboutChange_AttackRange(Owner);
}

float FAttackAttributes::ApplyTempAttackRangeModifierViaMultiplier(float Multiplier, 
	AInfantryController * AIController, ISelectable * Owner)
{
	NumTempAttackRangeModifiers++;

	AttackRange *= Multiplier;
	
	AIController->OnUnitRangeChanged(AttackRange);

	TellHUDAboutChange_AttackRange(Owner);

	return GetAttackRange();
}

float FAttackAttributes::RemoveTempAttackRangeModifierViaMultiplier(float Multiplier, 
	AInfantryController * AIController, ISelectable * Owner)
{
	NumTempAttackRangeModifiers--;
	assert(NumTempAttackRangeModifiers >= 0);

	if (NumTempAttackRangeModifiers > 0)
	{
		AttackRange *= Multiplier;
	}
	else
	{
		AttackRange = GetDefaultAttackRange();
	}

	AIController->OnUnitRangeChanged(AttackRange);

	TellHUDAboutChange_AttackRange(Owner);

	return GetAttackRange();
}

#if WITH_EDITOR
void FAttackAttributes::OnPostEdit()
{
	bCanEditCameraShakeOptions = MuzzleCameraShake_BP;

	const ETargetingType MaxEnumValue = static_cast<ETargetingType>(static_cast<int32>(ETargetingType::z_ALWAYS_LAST_IN_ENUM) + 1);
	AcceptableTargetFNames.Empty();
	for (const auto & Elem : AcceptableTargetTypes)
	{
		if (Elem != ETargetingType::None && Elem != MaxEnumValue)
		{
			AcceptableTargetFNames.Emplace(Statics::GetTargetingType(Elem));
		}
	}

	DefaultImpactDamage = DamageInfo.ImpactDamage;
	DefaultAoEDamage = DamageInfo.AoEDamage;
	DefaultAttackRate = AttackRate;
	DefaultAttackRange = AttackRange;
}
#endif // WITH_EDITOR


//=============================================================================================
//	>FDamageMultipliers
//=============================================================================================

FDamageMultipliers::FDamageMultipliers()
{
	for (uint8 i = 0; i < Statics::NUM_ARMOUR_TYPES; ++i)
	{
		Multipliers.Emplace(Statics::ArrayIndexToArmourType(i), 1.f);
	}
}

float FDamageMultipliers::GetMultiplier(EArmourType ArmourType) const
{
	return Multipliers[ArmourType];
}


//=============================================================================================
//	>FSelectionDecalInfo
//=============================================================================================

FSelectionDecalInfo::FSelectionDecalInfo() 
	: Decal(nullptr) 
	, ParamName(FName("RTS_Opacity")) 
	, MouseoverParamValue(0.5f)
{
}

UMaterialInterface * FSelectionDecalInfo::GetDecal() const
{
	return Decal;
}

const FName & FSelectionDecalInfo::GetParamName() const
{
	return ParamName;
}

bool FSelectionDecalInfo::IsParamNameValid() const
{
	return bIsParamNameValid;
}

void FSelectionDecalInfo::SetIsParamNameValid(bool bNewValue)
{
	bIsParamNameValid = bNewValue;
}

float FSelectionDecalInfo::GetMouseoverParamValue() const
{
	return MouseoverParamValue;
}

float FSelectionDecalInfo::GetOriginalParamValue() const
{
	return OriginalParamValue;
}

void FSelectionDecalInfo::SetOriginalParamValue(float InValue)
{
	OriginalParamValue = InValue;
}

bool FSelectionDecalInfo::RequiresCreatingMaterialInstanceDynamic() const
{
	return (MouseoverParamValue != OriginalParamValue);
}


//=========================================================================================
//	>FCPUDifficultyInfo
//=========================================================================================

FCPUDifficultyInfo::FCPUDifficultyInfo()
{
	static uint64 Count = 0;
	Name = FString("Default Name ") + FString::FromInt(Count++);
	Icon = nullptr;
	AIController_BP = ACPUPlayerAIController::StaticClass();
}

FCPUDifficultyInfo::FCPUDifficultyInfo(const FString & InName) 
	: Name(InName)
	, Icon(nullptr) 
	, AIController_BP(ACPUPlayerAIController::StaticClass())
{
}

const FString & FCPUDifficultyInfo::GetName() const
{
	return Name;
}

UTexture2D * FCPUDifficultyInfo::GetIcon() const
{
	return Icon;
}

TSubclassOf<ACPUPlayerAIController> FCPUDifficultyInfo::GetControllerBP() const
{
	return AIController_BP;
}


//=========================================================================================
//	>FPlayerStartDistanceInfo
//=========================================================================================

FPlayerStartDistanceInfo::FPlayerStartDistanceInfo()
{
}

FPlayerStartDistanceInfo::FPlayerStartDistanceInfo(uint8 InID, float InDistance)
{
	PlayerStartID = InID;
	Distance = InDistance;
}

uint8 FPlayerStartDistanceInfo::GetID() const
{
	return PlayerStartID;
}

float FPlayerStartDistanceInfo::GetDistance() const
{
	return Distance;
}

float FPlayerStartDistanceInfo::GetDistanceBetweenSpots(const FPlayerStartInfo & Info1, const FPlayerStartInfo & Info2)
{
	/* Going to actually use 3D distance not 2D */
	return (Info1.GetLocation() - Info2.GetLocation()).Size();
}

bool operator<(const FPlayerStartDistanceInfo & Info1, const FPlayerStartDistanceInfo & Info2)
{
	return Info1.GetDistance() < Info2.GetDistance();
}


//=========================================================================================
//	>FPlayerStartInfo
//=========================================================================================

FPlayerStartInfo::FPlayerStartInfo()
{
}

FPlayerStartInfo::FPlayerStartInfo(ARTSPlayerStart * PlayerStart)
{
	UniqueID = PlayerStart->GetUniqueID();
	
	/* May want to change this to a ray trace for the Z axis value because player starts are 
	slightly in the air due to being a capsule. */
	Location = PlayerStart->GetActorLocation();
	
	Rotation = PlayerStart->GetActorRotation();
}

FPlayerStartInfo::FPlayerStartInfo(uint8 InUniqueID, const FVector & InLoc, const FRotator & InRot)
{
	UniqueID = InUniqueID;
	Location = InLoc;
	Rotation = InRot;
}

void FPlayerStartInfo::EmptyNearbyPlayerStarts()
{
	NearbyPlayerStarts.Empty();
}

void FPlayerStartInfo::AddNearbyPlayerStart(const FPlayerStartDistanceInfo & OthersInfo)
{
	NearbyPlayerStarts.Emplace(OthersInfo);
}

void FPlayerStartInfo::SortNearbyPlayerStarts()
{
	NearbyPlayerStarts.Sort();
}

const TArray<FPlayerStartDistanceInfo>& FPlayerStartInfo::GetNearbyPlayerStartsSorted() const
{
	return NearbyPlayerStarts;
}


//=========================================================================================
//	>FMapInfo
//=========================================================================================

void FMapInfo::GetPlayerStartTransform(int16 InPlayerStartID, FVector & OutLocation, 
	FRotator & OutRotation) const
{
	UE_CLOG(InPlayerStartID == -1, RTSLOG, Fatal, TEXT("Starting spot ID is -1 which implies "
		"game never tried to assign spot to them. \nMake sure URTSGameInstance::AssignOptimalStartingSpots "
		"changes any -1s to -2s to show it at least considered player"));
	
	if (InPlayerStartID == -2)
	{
		// Could not assign spot. Default to origin
		OutLocation = FVector::ZeroVector;
		OutRotation = FRotator::ZeroRotator;
	}
	else
	{
		OutLocation = PlayerStartPoints[InPlayerStartID].GetLocation();
		OutRotation = PlayerStartPoints[InPlayerStartID].GetRotation();
	}
}


//=========================================================================================
//	>FStartingResourceConfig
//=========================================================================================

FStartingResourceConfig::FStartingResourceConfig()
{
	static uint64 SomeInt = 1;

	/* These need to be unique. For one, the match rules widget will use this name to map to
	enum value. But that's not a very good scheme so should change that TODO */
	Name = FText::FromString("Default name " + FString::FromInt(SomeInt++));

	Amounts.Reserve(Statics::NUM_RESOURCE_TYPES);
	for (uint8 i = 0; i < Statics::NUM_RESOURCE_TYPES; ++i)
	{
		Amounts.Emplace(Statics::ArrayIndexToResourceType(i), 10000);
	}
}

#if WITH_EDITOR
FStartingResourceConfig::FStartingResourceConfig(const TMap<EResourceType, int32> & InResourceMap)
{
	Name = FText::FromString("Development settings config");

	// Make copy of TMap param
	// This syntax doesn't work for me. Actually try it again, issue may have been something else
	//Amounts = InResourceMap;
	Amounts.Empty();
	for (const auto & Elem : InResourceMap)
	{
		Amounts.Emplace(Elem.Key, Elem.Value);
	}
}

FString FStartingResourceConfig::ToString() const
{
	FString String;
	
	String += "Name: " + Name.ToString();
	
	for (const auto & Elem : Amounts)
	{
		String += "\n" + ENUM_TO_STRING(EResourceType, Elem.Key) + ": " + FString::FromInt(Elem.Value);
	}

	return String;
}
#endif // WITH_EDITOR

const FText & FStartingResourceConfig::GetName() const
{
	return Name;
}

const TArray<int32> FStartingResourceConfig::GetResourceArraySlow() const
{
	TArray < int32 > Array;
	Array.Init(0, Statics::NUM_RESOURCE_TYPES);
	for (const auto & Elem : Amounts)
	{
		Array[Statics::ResourceTypeToArrayIndex(Elem.Key)] = Elem.Value;
	}

	return Array;
}


//=========================================================================================
//	>FPlayerInfo
//=========================================================================================

FPlayerInfo::FPlayerInfo(ARTSPlayerState * InPlayerState, ETeam InTeam, EFaction InFaction, 
	int16 InStartingSpot)
{
	PlayerType = ELobbySlotStatus::Human;
	PlayerState = InPlayerState;
	CPUDifficulty = ECPUDifficulty::None;
	Team = InTeam;
	Faction = InFaction;
	StartingSpotID = InStartingSpot;

	// Added this for PIE match setup
	if (InPlayerState->bIsABot)
	{
		CPUPlayerState = InPlayerState;
	}
}

FPlayerInfo::FPlayerInfo(ECPUDifficulty InCPUDifficulty, ETeam InTeam, EFaction InFaction, 
	int16 InStartingSpot)
{
	PlayerType = ELobbySlotStatus::CPU;
	PlayerState = nullptr;
	CPUDifficulty = InCPUDifficulty;
	Team = InTeam;
	Faction = InFaction;
	StartingSpotID = InStartingSpot;
}


//=========================================================================================
//	>FMatchInfo
//=========================================================================================

void FMatchInfo::SetMap(const FName & InMapName, FMapID InMapID, const FText & InDisplayName)
{
	MapFName = InMapName;
	MapID = InMapID;
	MapDisplayName = InDisplayName;
}


//=========================================================================================
//	>FStaticBuffOrDebuffInfo
//=========================================================================================

FStaticBuffOrDebuffInfo::FStaticBuffOrDebuffInfo()
	: TryApplyToPtr(nullptr)
	, OnRemovedPtr(nullptr)
	, Type(EBuffOrDebuffType::Buff)
	, DisplayName(FText::FromString("Default name")) 
	, ParticlesTemplate(nullptr) 
	, ParticlesAttachPoint(ESelectableBodySocket::None)
{
}

FStaticBuffOrDebuffInfo::FStaticBuffOrDebuffInfo(EStaticBuffAndDebuffType InSpecificType)
	: SpecificType(InSpecificType)
	, TryApplyToPtr(nullptr) 
	, OnRemovedPtr(nullptr)
	, Type(EBuffOrDebuffType::Buff)
	, DisplayName(FText::FromString("Default name"))
	, ParticlesTemplate(nullptr)
	, ParticlesAttachPoint(ESelectableBodySocket::None)
{
}

void FStaticBuffOrDebuffInfo::SetFunctionPointers(Static_FunctionPtr_TryApplyTo InTryApplyToPtr,
	Static_FunctionPtr_OnRemoved InOnRemovedPtr)
{
	TryApplyToPtr = InTryApplyToPtr;
	OnRemovedPtr = InOnRemovedPtr;
}

bool FStaticBuffOrDebuffInfo::IsBuff() const
{
	return Type == EBuffOrDebuffType::Buff;
}

bool FStaticBuffOrDebuffInfo::IsDebuff() const
{
	return Type == EBuffOrDebuffType::Debuff;
}

EBuffOrDebuffApplicationOutcome FStaticBuffOrDebuffInfo::ExecuteTryApplyBehavior(AActor * BuffOrDebuffInstigator,
	ISelectable * InstigatorAsSelectable, ISelectable * Target) const
{
	// Call function pointed to by function pointer
	return (*(TryApplyToPtr))(BuffOrDebuffInstigator, InstigatorAsSelectable, Target, *this);
}

EBuffOrDebuffRemovalOutcome FStaticBuffOrDebuffInfo::ExecuteOnRemovedBehavior(ISelectable * Target, const FStaticBuffOrDebuffInstanceInfo & StateInfo,
	AActor * Remover, ISelectable * RemoverAsSelectable, EBuffAndDebuffRemovalReason RemovalReason) const
{ 
	/* Call function pointed to by function pointer */
	return (*(OnRemovedPtr))(Target, StateInfo, Remover, RemoverAsSelectable, RemovalReason, *this);
}

const FText & FStaticBuffOrDebuffInfo::GetDisplayName() const
{
	return DisplayName;
}

const FSlateBrush & FStaticBuffOrDebuffInfo::GetDisplayImage() const
{
	return DisplayImage;
}

EStaticBuffAndDebuffType FStaticBuffOrDebuffInfo::GetSpecificType() const
{
	return SpecificType;
}

EBuffAndDebuffSubType FStaticBuffOrDebuffInfo::GetSubType() const
{
	return SubType;
}

#if WITH_EDITOR
void FStaticBuffOrDebuffInfo::SetSpecificType(EStaticBuffAndDebuffType InSpecificType)
{
	SpecificType = InSpecificType;
}
#endif


//=========================================================================================
//	>FTickableBuffOrDebuffInfo
//=========================================================================================

FTickableBuffOrDebuffInfo::FTickableBuffOrDebuffInfo()
	: TickInterval(3.f)
	, NumberOfTicks(2)
	, TryApplyToPtr(nullptr)
	, DoTickPtr(nullptr)
	, OnRemovedPtr(nullptr)
	, Type(EBuffOrDebuffType::Buff) 
	, SubType(EBuffAndDebuffSubType::Default)
	, DisplayName(FText::FromString("Default name"))
	, DisplayImage(nullptr) 
	, ParticlesTemplate(nullptr)
	, ParticlesAttachPoint(ESelectableBodySocket::None)
{
}

FTickableBuffOrDebuffInfo::FTickableBuffOrDebuffInfo(ETickableBuffAndDebuffType InSpecificType)
	: TickInterval(3.f)
	, NumberOfTicks(2)
	, SpecificType(InSpecificType)
	, TryApplyToPtr(nullptr)
	, DoTickPtr(nullptr)
	, OnRemovedPtr(nullptr)
	, Type(EBuffOrDebuffType::Buff)
	, SubType(EBuffAndDebuffSubType::Default)
	, DisplayName(FText::FromString("Default name"))
	, DisplayImage(nullptr)
	, ParticlesTemplate(nullptr)
	, ParticlesAttachPoint(ESelectableBodySocket::None)
{
}

void FTickableBuffOrDebuffInfo::SetFunctionPointers(Tickable_FunctionPtr_TryApplyTo InTryApplyToPtr,
	FunctionPtr_DoTick InDoTickPtr, Tickable_FunctionPtr_OnRemoved InOnRemovedPtr)
{
	TryApplyToPtr = InTryApplyToPtr;
	DoTickPtr = InDoTickPtr;
	OnRemovedPtr = InOnRemovedPtr;
}

float FTickableBuffOrDebuffInfo::GetTickInterval() const
{
	return TickInterval;
}

uint8 FTickableBuffOrDebuffInfo::GetNumberOfTicks() const
{
	return NumberOfTicks;
}

bool FTickableBuffOrDebuffInfo::ExpiresOverTime() const
{
	return NumberOfTicks > 0;
}

float FTickableBuffOrDebuffInfo::GetFullDuration() const
{
	return NumberOfTicks * TickInterval;
}

bool FTickableBuffOrDebuffInfo::IsBuff() const
{
	return Type == EBuffOrDebuffType::Buff;
}

bool FTickableBuffOrDebuffInfo::IsDebuff() const
{
	return Type == EBuffOrDebuffType::Debuff;
}

EBuffOrDebuffApplicationOutcome FTickableBuffOrDebuffInfo::ExecuteTryApplyBehavior(AActor * BuffOrDebuffInstigator,
	ISelectable * InstigatorAsSelectable, ISelectable * Target) const
{
	UE_CLOG(TryApplyToPtr == nullptr, RTSLOG, Fatal, TEXT("Buff/debuff \"%s\" does not have its "
		"TryApplyTo behavior set. Need to add an entry for this in "
		"URTSGameInstance::SetupBuffsAndDebuffInfos()"),
		TO_STRING(ETickableBuffAndDebuffType, SpecificType));
	
	/* Call function pointed to by function pointer */
	return (*(TryApplyToPtr))(BuffOrDebuffInstigator, InstigatorAsSelectable, Target, *this);
}

EBuffOrDebuffTickOutcome FTickableBuffOrDebuffInfo::ExecuteDoTickBehavior(ISelectable * Target, const FTickableBuffOrDebuffInstanceInfo & StateInfo) const
{
	UE_CLOG(DoTickPtr == nullptr, RTSLOG, Fatal, TEXT("Buff/debuff \"%s\" does not have its "
		"DoTick behavior set. Need to add an entry for this in "
		"URTSGameInstance::SetupBuffsAndDebuffInfos()"), 
		TO_STRING(ETickableBuffAndDebuffType, SpecificType));
	
	/* Call function pointed to by function pointer */
	return (*(DoTickPtr))(Target, StateInfo);
}

EBuffOrDebuffRemovalOutcome FTickableBuffOrDebuffInfo::ExecuteOnRemovedBehavior(ISelectable * Target, const FTickableBuffOrDebuffInstanceInfo & StateInfo,
	AActor * Remover, ISelectable * RemoverAsSelectable, EBuffAndDebuffRemovalReason RemovalReason) const
{
	UE_CLOG(OnRemovedPtr == nullptr, RTSLOG, Fatal, TEXT("Buff/debuff \"%s\" does not have its "
		"OnRemoved behavior set. Need to add an entry for this in "
		"URTSGameInstance::SetupBuffsAndDebuffInfos()"),
		TO_STRING(ETickableBuffAndDebuffType, SpecificType));
	
	/* Call function pointed to by function pointer */
	return (*(OnRemovedPtr))(Target, StateInfo, Remover, RemoverAsSelectable, RemovalReason, *this);
}

const FText & FTickableBuffOrDebuffInfo::GetDisplayName() const
{
	return DisplayName;
}

UTexture2D * FTickableBuffOrDebuffInfo::GetDisplayImage() const
{
	return DisplayImage;
}

ETickableBuffAndDebuffType FTickableBuffOrDebuffInfo::GetSpecificType() const
{
	/* Should have been set in post edit or something */
	assert(SpecificType != ETickableBuffAndDebuffType::None);
	
	return SpecificType;
}

EBuffAndDebuffSubType FTickableBuffOrDebuffInfo::GetSubType() const
{
	return SubType;
}

#if WITH_EDITOR
void FTickableBuffOrDebuffInfo::SetSpecificType(ETickableBuffAndDebuffType InSpecificType)
{
	SpecificType = InSpecificType;
}
#endif


//=========================================================================================
//	>FBuffOrDebuffSubTypeInfo
//=========================================================================================

FBuffOrDebuffSubTypeInfo::FBuffOrDebuffSubTypeInfo() 
	: Texture(nullptr) 
	, BorderBrushColor(FLinearColor::Red)
{
}

UTexture2D * FBuffOrDebuffSubTypeInfo::GetTexture() const
{
	return Texture;
}

const FLinearColor & FBuffOrDebuffSubTypeInfo::GetBrushColor() const
{
	return BorderBrushColor;
}



