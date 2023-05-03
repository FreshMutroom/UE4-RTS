// Fill out your copyright notice in the Description page of Project Settings.

#include "DamageTypes.h"

#include "Statics/Statics.h"
#include "Statics/DevelopmentStatics.h"

URTSDamageType::URTSDamageType()
{
	/* This stops the base class URTSDamageType from appearing in editor by default which is
	what we want */
	if (GetClass() == StaticClass())
	{
		return;
	}

	/* Ensure only one of these is ever created */
	UE_CLOG(Statics::DamageTypes.Contains(GetClass()), RTSLOG, Fatal, 
		TEXT("Attempted to add RTSDamageType class \'%s\' to Statics::DamageTypes but the array "
			"already contains it"), *GetClass()->GetName());
		
	Statics::DamageTypes.Add(GetClass());
}
