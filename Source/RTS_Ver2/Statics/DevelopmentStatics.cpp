// Fill out your copyright notice in the Description page of Project Settings.

#include "DevelopmentStatics.h"
#include "Engine/LocalPlayer.h"
#include "Widgets/SViewport.h"


DEFINE_LOG_CATEGORY(RTSLOG);

void DevelopmentStatics::BreakpointMessage()
{
#if PRINT_DEBUG_MESSAGES

	FColor Color = FColor(0, 0, FMath::RandRange(128, 255), 255);
	GEngine->AddOnScreenDebugMessage(-1, 20.f, Color, TEXT("######### Breakpoint reached #########"));

#endif
}

void DevelopmentStatics::Message(const FString & Message)
{
	PrintToScreenAndLog(Message);
}

void DevelopmentStatics::Message(const FString & Message, const FString & Message2)
{
	const FString FullMessage = Message + ": " + Message2;

	PrintToScreenAndLog(FullMessage);
}

void DevelopmentStatics::Message(const FString & Message, const FText & Value)
{
	const FString FullMessage = Message + ": " + Value.ToString();

	PrintToScreenAndLog(FullMessage);
}

void DevelopmentStatics::Message(const FString & Message, float Value)
{
	FString FullMessage = Message + FString::Printf(TEXT(": %.3f"), Value);

	PrintToScreenAndLog(FullMessage);
}


void DevelopmentStatics::Message(const FString & Message, int32 Value)
{
	FString FullMessage = Message + FString::Printf(TEXT(": %d"), Value);

	PrintToScreenAndLog(FullMessage);
}

void DevelopmentStatics::Message(const FString & Message, uint32 Value)
{
	FString FullMessage = Message + FString::Printf(TEXT(": %u"), Value);

	PrintToScreenAndLog(FullMessage);
}

void DevelopmentStatics::Message(const FString & Message, uint64 Value)
{
	FString FullMessage = Message + FString::Printf(TEXT(": %u"), Value);

	PrintToScreenAndLog(FullMessage);
}

void DevelopmentStatics::Message(const FString & Message, bool Value)
{
	FString FullMessage = Message + FString::Printf(TEXT(": %s"), Value ? *FString("true") : *FString("false"));

	PrintToScreenAndLog(FullMessage);
}

void DevelopmentStatics::Message(const FString & Message, const FName & Value)
{
	FString FullMessage = Message + FString::Printf(TEXT(": %s"), *Value.ToString());

	PrintToScreenAndLog(FullMessage);
}

void DevelopmentStatics::Message(const FString & Message, const FVector & Vector)
{
	/* Floats have 1 decimal place */
	//FString FullMessage = Message + FString::Printf(TEXT(": ( %.1f , %.1f , %.1f )"), Vector.X, Vector.Y, Vector.Z);
	FString FullMessage = Message + FString::Printf(TEXT(": ( %.6f , %.6f , %.6f )"), Vector.X, Vector.Y, Vector.Z);

	PrintToScreenAndLog(FullMessage);
}

void DevelopmentStatics::Message(const FString & Message, const FRotator & Rotator)
{
	/* Floats have 1 decimal place */
	//FString FullMessage = Message + FString::Printf(TEXT(": ( %.1f , %.1f , %.1f )"), Rotator.Pitch, Rotator.Yaw, Rotator.Roll);
	FString FullMessage = Message + FString::Printf(TEXT(": (P: %.3f , Y: %.3f , R: %.3f )"), Rotator.Pitch, Rotator.Yaw, Rotator.Roll);

	PrintToScreenAndLog(FullMessage);
}

void DevelopmentStatics::Message(const FString & Message, const FQuat & Quat)
{
	FString FullMessage = Message + ": " + Quat.ToString();

	PrintToScreenAndLog(FullMessage);
}

void DevelopmentStatics::Message(const FString & Message, const FVector2D & Vector2D)
{
	/* Floats have 1 decimal place */
	FString FullMessage = Message + FString::Printf(TEXT(": ( %.1f , %.1f )"), Vector2D.X, Vector2D.Y);

	PrintToScreenAndLog(FullMessage);
}

void DevelopmentStatics::Message(const FString & Message, const FIntPoint & IntPoint)
{
	FString FullMessage = Message + FString::Printf(TEXT(": ( %d , %d )"), IntPoint.X, IntPoint.Y);

	PrintToScreenAndLog(FullMessage);
}

void DevelopmentStatics::Message(const FString & Message, const FLinearColor & Color)
{
	FString FullMessage = Message + FString::Printf(TEXT(": %s"), *Color.ToString());

	PrintToScreenAndLog(FullMessage);
}

bool DevelopmentStatics::IsViewportFocused(const UObject * WorldContextObject)
{
	/* Well this works. Relies on keyboard focus though */
	TSharedPtr<SWidget> ActiveViewport = FSlateApplication::Get().GetKeyboardFocusedWidget();
	return ActiveViewport == WorldContextObject->GetWorld()->GetFirstLocalPlayerFromController()->ViewportClient->GetGameViewportWidget();
}

void DevelopmentStatics::CurrentPIEInstanceMessage(const UObject * WorldContextObject, const FString & InMessage)
{
	if (IsViewportFocused(WorldContextObject))
	{
		Message(InMessage);
	}
}

void DevelopmentStatics::CurrentPIEInstanceMessage(const UObject * WorldContextObject, const FString & InMessage, const FString & Message2)
{
	if (IsViewportFocused(WorldContextObject))
	{
		Message(InMessage, Message2);
	}
}

void DevelopmentStatics::CurrentPIEInstanceMessage(const UObject * WorldContextObject, const FString & InMessage, const FText & Value)
{
	if (IsViewportFocused(WorldContextObject))
	{
		Message(InMessage, Value);
	}
}

void DevelopmentStatics::CurrentPIEInstanceMessage(const UObject * WorldContextObject, const FString & InMessage, float Value)
{
	if (IsViewportFocused(WorldContextObject))
	{
		Message(InMessage, Value);
	}
}

void DevelopmentStatics::CurrentPIEInstanceMessage(const UObject * WorldContextObject, const FString & InMessage, int32 Value)
{
	if (IsViewportFocused(WorldContextObject))
	{
		Message(InMessage, Value);
	}
}

void DevelopmentStatics::CurrentPIEInstanceMessage(const UObject * WorldContextObject, const FString & InMessage, uint32 Value)
{
	if (IsViewportFocused(WorldContextObject))
	{
		Message(InMessage, Value);
	}
}

void DevelopmentStatics::CurrentPIEInstanceMessage(const UObject * WorldContextObject, const FString & InMessage, uint64 Value)
{
	if (IsViewportFocused(WorldContextObject))
	{
		Message(InMessage, Value);
	}
}

void DevelopmentStatics::CurrentPIEInstanceMessage(const UObject * WorldContextObject, const FString & InMessage, bool Value)
{
	if (IsViewportFocused(WorldContextObject))
	{
		Message(InMessage, Value);
	}
}

void DevelopmentStatics::CurrentPIEInstanceMessage(const UObject * WorldContextObject, const FString & InMessage, const FName & Value)
{
	if (IsViewportFocused(WorldContextObject))
	{
		Message(InMessage, Value);
	}
}

void DevelopmentStatics::CurrentPIEInstanceMessage(const UObject * WorldContextObject, const FString & InMessage, const FVector & Value)
{
	if (IsViewportFocused(WorldContextObject))
	{
		Message(InMessage, Value);
	}
}

void DevelopmentStatics::CurrentPIEInstanceMessage(const UObject * WorldContextObject, const FString & InMessage, const FRotator & Value)
{
	if (IsViewportFocused(WorldContextObject))
	{
		Message(InMessage, Value);
	}
}

void DevelopmentStatics::CurrentPIEInstanceMessage(const UObject * WorldContextObject, const FString & InMessage, const FVector2D & Value)
{
	if (IsViewportFocused(WorldContextObject))
	{
		Message(InMessage, Value);
	}
}

void DevelopmentStatics::CurrentPIEInstanceMessage(const UObject * WorldContextObject, const FString & InMessage, const FIntPoint & Value)
{
	if (IsViewportFocused(WorldContextObject))
	{
		Message(InMessage, Value);
	}
}

void DevelopmentStatics::CurrentPIEInstanceMessage(const UObject * WorldContextObject, const FString & InMessage, const FLinearColor & Value)
{
	if (IsViewportFocused(WorldContextObject))
	{
		Message(InMessage, Value);
	}
}

bool DevelopmentStatics::IsValid(AActor * Actor)
{
	/* Check an actor is either NULL or has had Destroy()
	called on it. N.B. checking is Actor == nullptr is
	not enough. */

	return Actor != nullptr && !Actor->IsActorBeingDestroyed();
}

void DevelopmentStatics::PrintToScreenAndLog(const FString & Message)
{
#if PRINT_DEBUG_MESSAGES

	assert(GEngine != nullptr);

	GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::Red, Message);
	LogMessage(Message);

#endif
}

void DevelopmentStatics::LogMessage(const FString & Message)
{
#if PRINT_DEBUG_MESSAGES

	SET_WARN_COLOR(COLOR_CYAN);
	UE_LOG(RTSLOG, Warning, TEXT("%s"), *Message);
	CLEAR_WARN_COLOR();

#endif
}
