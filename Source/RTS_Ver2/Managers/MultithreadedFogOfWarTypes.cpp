// Fill out your copyright notice in the Description page of Project Settings.

#if MULTITHREADED_FOG_OF_WAR

#include "MultithreadedFogOfWarTypes.h"


InfantryFogInfo::InfantryFogInfo(AInfantry * Infantry)
	: WorldLocation(FVector2D(Infantry->GetActorLocation()))
	, SightRadius(Infantry->GetAttributes()->GetSightRange())
	, StealthRevealRadius(Infantry->GetAttributes()->GetStealthRevealRange())
{
}

InfantryFogInfoBasic::InfantryFogInfoBasic(AInfantry * Infantry)
	: WorldLocation(Infantry->GetActorLocation())
	, SelectableID(Infantry->GetSelectableID())
	, FogStatus(EFogStatus::Hidden)
{
}

BuildingFogInfo::BuildingFogInfo(ABuilding * Building)
	: WorldLocation(FVector2D(Building->GetActorLocation()))
	, SightRadius(Building->GetAttributes()->GetSightRange())
	, StealthRevealRadius(Building->GetAttributes()->GetStealthRevealRange())
	, SelectableID(Building->GetSelectableID())
{
}

bool DestroyedBuildingArray::WasRecentlyDestroyed(const BuildingFogInfo & Elem) const
{
	return Array[Elem.SelectableID] == true;
}

void DestroyedBuildingArray::SetBitToRecentlyDestroyed(uint8 BuildingsSelectableID)
{
	Array[BuildingsSelectableID] = true;
}

void DestroyedBuildingArray::SetBitToNotRecentlyDestroyed(const BuildingFogInfo & Elem)
{
	Array[Elem.SelectableID] = false;
}

bool DestroyedBuildingArray::AreAllFlagsReset() const
{
	for (int32 i = 0; i < ProjectSettings::MAX_NUM_SELECTABLES_PER_PLAYER; ++i)
	{
		if (Array[i] == true)
		{
			return false;
		}
	}

	return true;
}

#endif // MULTITHREADED_FOG_OF_WAR
