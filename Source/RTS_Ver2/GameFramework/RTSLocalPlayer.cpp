// Fill out your copyright notice in the Description page of Project Settings.

#include "RTSLocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

#include "Statics/Statics.h"
#include "Statics/DevelopmentStatics.h"
#include "Settings/RTSGameUserSettings.h"

URTSLocalPlayer::URTSLocalPlayer()
{
}

FString URTSLocalPlayer::GetGameLoginOptions() const
{
	// Counting how many times this is called, just curious
	static uint64 CallCount = 0;
	if (++CallCount == 500)
	{
		UE_LOG(RTSLOG, Warning, TEXT("URTSLocalPlayer::GetGameLoginOptions() called 500 times"));
	}

	// Super just returns blank text

	FString Options;

	/* Add on password */
	Options += TEXT("?password=") + Password;

	/* Add nickname. Not needed I think, is done automatically by source
	@See UPendingNetGame::NotifyControlMessage line 240. GetNickname() is used which returns
	the steam username I think, so the name defined in game user settings is irrelevant */
	//Options += TEXT("?Name=") + GetNickname();

	URTSGameUserSettings * Settings = CastChecked<URTSGameUserSettings>(UGameUserSettings::GetGameUserSettings());

	/* Add on the faction we want to be set to by default when we join. I think this will be
	called before game user settings has a chance to set default values which happens in
	GI::Initialize so will have to allow EFaction::None through. This I think should only
	be needed for starting game not for trying to create/join server, so what this func returns
	then doesn't really matter, and probably only an issue for the very first time starting game */

	FString FactionString;

	if (Settings->GetDefaultFaction() == EFaction::None)
	{
		// Use first value in EFaction enum
		FactionString = Statics::FactionToString(Statics::ArrayIndexToFaction(0));
	}
	else
	{
		FactionString = FString::FromInt(Statics::FactionToArrayIndex(Settings->GetDefaultFaction()));
	}

	Options += TEXT("?faction=") + FactionString;

	return Options;
}

void URTSLocalPlayer::SetPassword(const FString & InPassword)
{
	Password = InPassword;
}
