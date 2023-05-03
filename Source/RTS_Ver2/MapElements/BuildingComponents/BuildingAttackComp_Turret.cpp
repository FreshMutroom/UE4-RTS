// Fill out your copyright notice in the Description page of Project Settings.


#include "BuildingAttackComp_Turret.h"


void BuldingTurretStatics::SelectTargetFromOverlapTest(IBuildingAttackComp_Turret * TurretComp,
	FOverlapDatum & TraceData, ETargetAquireMethodPriorties TargetAquireMethod, 
	EDistanceCheckMethod DistanceCheckMethod, bool bCanAssignNullToTarget)
{
	switch (TargetAquireMethod)
	{
	case ETargetAquireMethodPriorties::None:
	{
		for (const auto & Elem : TraceData.OutOverlaps)
		{
			AActor * Actor = Elem.GetActor();
			if (TurretComp->CanSweptActorBeAquiredAsTarget(Actor))
			{
				TurretComp->SetTarget(Actor);
				return;
			}
		}

		if (bCanAssignNullToTarget)
		{
			TurretComp->SetTarget(nullptr);
		}

		break;
	}

	case ETargetAquireMethodPriorties::LeastRotationRequired:
	{
		AActor * BestTarget = nullptr;
		float BestRotationAmount = FLT_MAX;
		float RotationRequired;
		for (const auto & Elem : TraceData.OutOverlaps)
		{
			AActor * Actor = Elem.GetActor();
			if (TurretComp->CanSweptActorBeAquiredAsTarget(Actor, RotationRequired))
			{
				if (RotationRequired < BestRotationAmount)
				{
					BestRotationAmount = RotationRequired;
					BestTarget = Actor;
				}
			}
		}
		
		if (bCanAssignNullToTarget)
		{
			TurretComp->SetTarget(BestTarget);
		}
		else
		{
			if (BestTarget != nullptr)
			{
				TurretComp->SetTarget(BestTarget);
			}
		}

		break;
	}

	case ETargetAquireMethodPriorties::HasAttack:
	{
		AActor * BestTarget = nullptr;
		for (const auto & Elem : TraceData.OutOverlaps)
		{
			AActor * Actor = Elem.GetActor();
			if (TurretComp->CanSweptActorBeAquiredAsTarget(Actor))
			{
				if (Statics::HasAttack(Actor))
				{
					TurretComp->SetTarget(Actor);
					return;
				}
				else
				{
					BestTarget = Actor;
				}
			}
		}

		if (bCanAssignNullToTarget)
		{
			TurretComp->SetTarget(BestTarget);
		}
		else
		{
			if (BestTarget != nullptr)
			{
				TurretComp->SetTarget(BestTarget);
			}
		}

		break;
	}

	case ETargetAquireMethodPriorties::Distance:
	{
		/* Both loops are similar. I've just hoisted an if statement out */
		if (DistanceCheckMethod == EDistanceCheckMethod::Closest)
		{
			AActor * NewTarget = nullptr;
			float BestDistanceSqr = FLT_MAX;
			for (const auto & Elem : TraceData.OutOverlaps)
			{
				AActor * Actor = Elem.GetActor();
				if (TurretComp->CanSweptActorBeAquiredAsTarget(Actor))
				{
					const float DistanceSqr = Statics::GetDistance2DSquared(TraceData.Pos, Actor->GetActorLocation());
					if (DistanceSqr < BestDistanceSqr)
					{
						NewTarget = Actor;
						BestDistanceSqr = DistanceSqr;
					}
				}
			}

			if (bCanAssignNullToTarget)
			{
				TurretComp->SetTarget(NewTarget);
			}
			else
			{
				if (NewTarget != nullptr)
				{
					TurretComp->SetTarget(NewTarget);
				}
			}
		}
		else // Assumed Furtherest
		{
			AActor * NewTarget = nullptr;
			float BestDistanceSqr = -FLT_MAX;
			for (const auto & Elem : TraceData.OutOverlaps)
			{
				AActor * Actor = Elem.GetActor();
				if (TurretComp->CanSweptActorBeAquiredAsTarget(Actor))
				{
					const float DistanceSqr = Statics::GetDistance2DSquared(TraceData.Pos, Actor->GetActorLocation());
					if (DistanceSqr > BestDistanceSqr)
					{
						NewTarget = Actor;
						BestDistanceSqr = DistanceSqr;
					}
				}
			}

			if (bCanAssignNullToTarget)
			{
				TurretComp->SetTarget(NewTarget);
			}
			else
			{
				if (NewTarget != nullptr)
				{
					TurretComp->SetTarget(NewTarget);
				}
			}
		}

		break;
	}

	case ETargetAquireMethodPriorties::HasAttack_LeastRotationRequired:
	{
		float BestRotationAmount_TargetWithAttack = FLT_MAX;
		AActor * BestTarget_TargetWithAttack = nullptr;
		float BestRotationAmount_TargetWithoutAttack = FLT_MAX;
		AActor * BestTarget_TargetWithoutAttack = nullptr;
		float RotationRequired;
		for (const auto & Elem : TraceData.OutOverlaps)
		{
			AActor * Actor = Elem.GetActor();
			if (TurretComp->CanSweptActorBeAquiredAsTarget(Actor, RotationRequired))
			{
				if (Statics::HasAttack(Actor))
				{
					if (RotationRequired < BestRotationAmount_TargetWithAttack)
					{
						BestRotationAmount_TargetWithAttack = RotationRequired;
						BestTarget_TargetWithAttack = Actor;
					}
				}
				else
				{
					if (RotationRequired < BestRotationAmount_TargetWithoutAttack)
					{
						BestRotationAmount_TargetWithoutAttack = RotationRequired;
						BestTarget_TargetWithoutAttack = Actor;
					}
				}
			}
		}

		if (bCanAssignNullToTarget)
		{
			if (BestTarget_TargetWithAttack != nullptr)
			{
				TurretComp->SetTarget(BestTarget_TargetWithAttack);
			}
			else
			{
				TurretComp->SetTarget(BestTarget_TargetWithoutAttack);
			}
		}
		else
		{
			if (BestTarget_TargetWithAttack != nullptr)
			{
				TurretComp->SetTarget(BestTarget_TargetWithAttack);
			}
			else if (BestTarget_TargetWithoutAttack != nullptr)
			{
				TurretComp->SetTarget(BestTarget_TargetWithoutAttack);
			}
		}

		break;
	}

	case ETargetAquireMethodPriorties::HasAttack_Distance:
	{
		// Both the if and else are similar - I've just hoiested an if out 
		if (DistanceCheckMethod == EDistanceCheckMethod::Closest)
		{
			float BestDistanceSqr_TargetWithAttack = FLT_MAX;
			AActor * BestTarget_TargetWithAttack = nullptr;
			float BestDistanceSqr_TargetWithoutAttack = FLT_MAX;
			AActor * BestTarget_TargetWithoutAttack = nullptr;
			for (const auto & Elem : TraceData.OutOverlaps)
			{
				AActor * Actor = Elem.GetActor();
				if (TurretComp->CanSweptActorBeAquiredAsTarget(Actor))
				{
					const float DistanceSqr = Statics::GetDistance2DSquared(TraceData.Pos, Actor->GetActorLocation());
					if (Statics::HasAttack(Actor))
					{
						if (DistanceSqr < BestDistanceSqr_TargetWithAttack)
						{
							BestDistanceSqr_TargetWithAttack = DistanceSqr;
							BestTarget_TargetWithAttack = Actor;
						}
					}
					else
					{
						if (DistanceSqr < BestDistanceSqr_TargetWithoutAttack)
						{
							BestDistanceSqr_TargetWithoutAttack = DistanceSqr;
							BestTarget_TargetWithoutAttack = Actor;
						}
					}
				}
			}

			if (bCanAssignNullToTarget)
			{
				if (BestTarget_TargetWithAttack != nullptr)
				{
					TurretComp->SetTarget(BestTarget_TargetWithAttack);
				}
				else if (BestTarget_TargetWithoutAttack != nullptr)
				{
					TurretComp->SetTarget(BestTarget_TargetWithoutAttack);
				}
			}
			else
			{
				if (BestTarget_TargetWithAttack != nullptr)
				{
					TurretComp->SetTarget(BestTarget_TargetWithAttack);
				}
				else
				{
					TurretComp->SetTarget(BestTarget_TargetWithoutAttack);
				}
			}
		}
		else
		{
			float BestDistanceSqr_TargetWithAttack = -FLT_MAX;
			AActor * BestTarget_TargetWithAttack = nullptr;
			float BestDistanceSqr_TargetWithoutAttack = -FLT_MAX;
			AActor * BestTarget_TargetWithoutAttack = nullptr;
			for (const auto & Elem : TraceData.OutOverlaps)
			{
				AActor * Actor = Elem.GetActor();
				if (TurretComp->CanSweptActorBeAquiredAsTarget(Actor))
				{
					const float DistanceSqr = Statics::GetDistance2DSquared(TraceData.Pos, Actor->GetActorLocation());
					if (Statics::HasAttack(Actor))
					{
						if (DistanceSqr > BestDistanceSqr_TargetWithAttack)
						{
							BestDistanceSqr_TargetWithAttack = DistanceSqr;
							BestTarget_TargetWithAttack = Actor;
						}
					}
					else
					{
						if (DistanceSqr < BestDistanceSqr_TargetWithoutAttack)
						{
							BestDistanceSqr_TargetWithoutAttack = DistanceSqr;
							BestTarget_TargetWithoutAttack = Actor;
						}
					}
				}
			}

			if (bCanAssignNullToTarget)
			{
				if (BestTarget_TargetWithAttack != nullptr)
				{
					TurretComp->SetTarget(BestTarget_TargetWithAttack);
				}
				else if (BestTarget_TargetWithoutAttack != nullptr)
				{
					TurretComp->SetTarget(BestTarget_TargetWithoutAttack);
				}
			}
			else
			{
				if (BestTarget_TargetWithAttack != nullptr)
				{
					TurretComp->SetTarget(BestTarget_TargetWithAttack);
				}
				else
				{
					TurretComp->SetTarget(BestTarget_TargetWithoutAttack);
				}
			}
		}

		break;
	}
	} // End switch (TargetAquireMethod)
}
