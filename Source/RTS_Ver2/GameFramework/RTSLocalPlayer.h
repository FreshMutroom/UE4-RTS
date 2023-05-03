// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/LocalPlayer.h"

#include "RTSLocalPlayer.generated.h"

/**
 *	Here for one reason:
 *	- to pass data such as password to the server when connecting to an online session
 *
 *	How to set custom ULocalPlayer class: edit DefaultEngine.ini
 */
UCLASS()
class RTS_VER2_API URTSLocalPlayer : public ULocalPlayer
{
	GENERATED_BODY()

public:

	URTSLocalPlayer();

protected:

	/* Password to send to server when trying to connect to networked lobby */
	UPROPERTY()
	FString Password;

public:

	/* Return the string to send into AGameModeBase::PreLogin which in turn calls
	AGameSession::ApproveLogin. This is here for passwords and for setting our desired faction
	when we join a networked game */
	virtual FString GetGameLoginOptions() const override;

	/* Store the password to send to server when trying to join */
	void SetPassword(const FString & InPassword);
};
