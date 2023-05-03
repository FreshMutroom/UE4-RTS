// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class AActor;


/* There are some parts of code that may call things like Contains on large arrays or whatnot. 
Once my project is completed I should turn this off because the areas where these are are 
not a problem */
#define DO_EXPENSIVE_CHECKS 1


#define PRINT_DEBUG_MESSAGES !UE_BUILD_SHIPPING

/** 
 *	How thorough development checks should be.
 *	0 = No checks. Not recommended at all.
 *	1 = Standard. Everything that is required for the game to run properly will be checked.
 *	2 = More thorough. Extra things that are expected about appearance will be checked. Good to
 *	compile and test a bit with this before shipping, but perhaps too strict for incremental 
 *	development 
 */
#define ASSERT_VERBOSITY 1

/*  Macro for assert that will cause crash. Copy of engine's check macro */
#if UE_BUILD_SHIPPING || ASSERT_VERBOSITY < 1
#define assert(expr)
#else
#define assert(expr) check(expr)
#endif

#if UE_BUILD_SHIPPING || ASSERT_VERBOSITY < 2
#define verboseAssert(expr)
#else
#define verboseAssert(expr) assert(expr)
#endif

/* Assert for something that is considered slow like Contains on a massive array. Can change 
DO_EXPENSIVE_CHECKS to false if no asserts are being thrown for expensive checks */
#if UE_BUILD_SHIPPING || ASSERT_VERBOSITY < 1 || !DO_EXPENSIVE_CHECKS
#define expensiveAssert(expr)
#else
#define expensiveAssert(expr) assert(expr)
#endif


/** 
 *	Convert enum value to string. 
 *	
 *	@param T - the UENUM class 
 *	@param EnumValue - enum value to convert to string 
 */
#define ENUM_TO_STRING(T, EnumValue) DevelopmentStatics::EnumToString<T>(TEXT(#T), EnumValue) 

 /* Same as ENUM_TO_STRING but does operator* too */
#define TO_STRING(T, EnumValue) *DevelopmentStatics::EnumToString<T>(TEXT(#T), EnumValue)


/* To reduce typing */
#define MESSAGE DevelopmentStatics::Message


/* To reduce even more typing */
#define MSG MESSAGE


/* To reduce typing */
#define PMESSAGE DevelopmentStatics::CurrentPIEInstanceMessage


/* Show a kind of break point message for fast debugging */
#define BPM DevelopmentStatics::BreakpointMessage();


/** 
 *	The first time this macro is encountered the function it resides in will return and call 
 *	itself again after a delay at which point this will be ignored. This flip flops between 
 *	returning and running.
 *
 *	The following must be true:
 *	- class calling this is a UObject
 *	- its GetWorld() returns non-null
 *	- function is paramless
 *	- function returns void
 *	
 *	@param Function - name of function including namespace e.g. ISelectable::GetHealth 
 *	@param DelayAmount - how long to wait before calling function again 
 */
#define RECALL(Function, DelayAmount) \
static bool bRecall = true; \
if (bRecall) \
{ \
	FTimerHandle TimerHandle_Dummy; \
	GetWorld()->GetTimerManager().SetTimer(TimerHandle_Dummy, this, &Function, DelayAmount, false); \
	bRecall = false; \
	return; \
} \
else \
{ \
	bRecall = true; \
} 


// Log category to use
#define RTSLOG LogRTS

//~ All for logging
DECLARE_LOG_CATEGORY_EXTERN(RTSLOG, Log, All);
// Name of class and function 
#define CUR_CLASS_FUNC (FString(__FUNCTION__))
#define CUR_LINE (FString::FromInt(__LINE__))


/* Simple macro to assert we are on server */
#define SERVER_CHECK assert(GetWorld()->IsServer());

/* Simple macro to assert we are on remote client */
#define CLIENT_CHECK assert(!GetWorld()->IsServer());


/**
 *	Functions and variables that are to aid in development and have no effect on a final shipping 
 *	build
 */
class RTS_VER2_API DevelopmentStatics
{
public:
	
	/* Show a message colored in blue that says "***BREAKPOINT REACHED*** or something like that" */
	static void BreakpointMessage();

	/* Print a red debug message on screen for 20 seconds
	@param Message the message to print */
	static void Message(const FString & Message);

	/* Print message plus FString
	@param Message - the first message
	@param Message2 - the second message */
	static void Message(const FString & Message, const FString & Message2);

	/* Print message plus FText
	@param Message - the first message
	@param Value - FText to print */
	static void Message(const FString & Message, const FText & Value);

	/* Print message plus integer value
	@param Message - the message to print
	@param Value - the value to print after message */
	static void Message(const FString & Message, float Value);

	/* Print message plus integer value
	@param Message the message to print
	@param Value the value to print after message */
	static void Message(const FString & Message, int32 Value);

	/* Print message plus integer value
	@param Message - the message to print
	@param Value - the value to print after message */
	static void Message(const FString & Message, uint32 Value);

	/* Print message plus integer value
	@param Message - the message to print
	@param Value - the value to print after message */
	static void Message(const FString & Message, uint64 Value);

	/* Print message plus boolean value
	@param Message - the message to print
	@param Value - the boolean value to print */
	static void Message(const FString & Message, bool Value);

	/* Print message plus FName
	@param Message - the message to print
	@param Value - the FName to print */
	static void Message(const FString & Message, const FName & Value);

	/* Print message plus vector values
	@param Message - the message to print
	@param Vector - the vector whose values to print */
	static void Message(const FString & Message, const FVector & Vector);

	/* Print message plus rotator values
	@param Message - the message to print
	@param Rotator - the rotator whose values to print */
	static void Message(const FString & Message, const FRotator & Rotator);
	static void Message(const FString & Message, const FQuat & Quat);

	/* Print message plus vector values
	@param Message - the message to print
	@param Vector2D - the vector whose values to print */
	static void Message(const FString & Message, const FVector2D & Vector2D);

	/* Print message plus FIntPoint values
	@param Message - the message to print
	@param Vector2D - the vector whose values to print */
	static void Message(const FString & Message, const FIntPoint & IntPoint);

	static void Message(const FString & Message, const FLinearColor & Color);

protected:

	/* Returns true if the WorldContextObject's viewport is the one currently focused by 
	the player */
	static bool IsViewportFocused(const UObject * WorldContextObject);

public:

	/** 
	 *	Print a message only if the current focused PIE window executes it. Useful for debugging 
	 *	PIE multiplayer.
	 */
	static void CurrentPIEInstanceMessage(const UObject * WorldContextObject, const FString & InMessage);
	static void CurrentPIEInstanceMessage(const UObject * WorldContextObject, const FString & InMessage, const FString & Message2);
	static void CurrentPIEInstanceMessage(const UObject * WorldContextObject, const FString & InMessage, const FText & Value);
	static void CurrentPIEInstanceMessage(const UObject * WorldContextObject, const FString & InMessage, float Value);
	static void CurrentPIEInstanceMessage(const UObject * WorldContextObject, const FString & InMessage, int32 Value);
	static void CurrentPIEInstanceMessage(const UObject * WorldContextObject, const FString & InMessage, uint32 Value);
	static void CurrentPIEInstanceMessage(const UObject * WorldContextObject, const FString & InMessage, uint64 Value);
	static void CurrentPIEInstanceMessage(const UObject * WorldContextObject, const FString & InMessage, bool Value);
	static void CurrentPIEInstanceMessage(const UObject * WorldContextObject, const FString & InMessage, const FName & Value);
	static void CurrentPIEInstanceMessage(const UObject * WorldContextObject, const FString & InMessage, const FVector & Value);
	static void CurrentPIEInstanceMessage(const UObject * WorldContextObject, const FString & InMessage, const FRotator & Value);
	static void CurrentPIEInstanceMessage(const UObject * WorldContextObject, const FString & InMessage, const FVector2D & Value);
	static void CurrentPIEInstanceMessage(const UObject * WorldContextObject, const FString & InMessage, const FIntPoint & Value);
	static void CurrentPIEInstanceMessage(const UObject * WorldContextObject, const FString & InMessage, const FLinearColor & Value);

	/* Check if actor is valid. Still runs in packaged game
	@param Actor the actor to check
	@return the actor is valid */
	static bool IsValid(AActor * Actor);

	/* Print on screen message and to log */
	static void PrintToScreenAndLog(const FString & Message);

	/* Print something to log */
	static void LogMessage(const FString & Message);

	/* Converts a UEnum value to FString */
	template <typename T>
	static FString EnumToString(const FString & EnumName, T EnumValue);
};


//-------------------------------------------
//	Templated function definitions
//-------------------------------------------

template<typename T>
inline FString DevelopmentStatics::EnumToString(const FString & EnumName, T EnumValue)
{
	UEnum * Enum = FindObject<UEnum>(ANY_PACKAGE, *EnumName, true);

	return Enum != nullptr ? Enum->GetNameStringByValue(static_cast<uint8>(EnumValue)) : "Unknown";
}
