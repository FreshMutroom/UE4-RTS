// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnumStringRepresentations.generated.h"

enum class EEditorPlaySkippingOption : uint8;
enum class EDefeatCondition : uint8;
enum class EInvalidOwnerIndexAction : uint8;


/**
 *	This class contains mapping from enum to the string form for it so it can be shown 
 *	on UI. Static variables aren't good enough because I want to default fill the containers 
 *	but I don't think UENUM exist yet so it doesn't work (thinking about it I thought UHT 
 *	runs first, generates code, so UENUM info should be available). Anyway I never create 
 *	this class at anytime - just using the default object you can get the info you want. 
 *
 *	In the future perhaps I could spawn this class sometime when the editor starts and that 
 *	way users can edit strings without having to recompile C++. Then you would wanna make 
 *	sure to not get the default object but get that other one.
 *
 *	This class was added because sometimes I need enum string representations but the game 
 *	instance isn't alive e.g. for editor utility widgets
 *
 *	Also most of this class is wrapped in EDITOR_ONLY preproc. There's nothing stop
 */
UCLASS()
class RTS_VER2_API UEnumStringRepresentations : public UObject
{
	GENERATED_BODY()
	
#if WITH_EDITORONLY_DATA

public:

	UEnumStringRepresentations();

	/* Get the object you can query for strings/enum.values */
	static const UEnumStringRepresentations * Get();

	/* Functions to get a user-defined string representation of an enum value */
	FString GetString(EEditorPlaySkippingOption EnumValue) const;
	FString GetString(EDefeatCondition EnumValue) const;
	FString GetString(EInvalidOwnerIndexAction EnumValue) const;
	FString GetString(EFaction EnumValue) const;
	FString GetString(ETeam EnumValue) const;
	FString GetString(ECPUDifficulty EnumValue) const;

	/**
	 *	These functions do the reverse: they take a string and try convert it to an enum value.
	 *
	 *	@return - true if the string corrisponds to an enum value
	 */
	bool GetEnumValueFromStringSlow_EditorPlaySkippingOption(const FString & String, EEditorPlaySkippingOption & OutEnumValue) const;
	bool GetEnumValueFromStringSlow_DefeatCondition(const FString & String, EDefeatCondition & OutEnumValue) const;
	bool GetEnumValueFromStringSlow_PIEPlayInvalidOwnerRule(const FString & String, EInvalidOwnerIndexAction & OutEnumValue) const;
	bool GetEnumValueFromStringSlow_Faction(const FString & String, EFaction & OutEnumValue) const;
	bool GetEnumValueFromStringSlow_Team(const FString & String, ETeam & OutEnumValue) const;
	bool GetEnumValueFromStringSlow_CPUDifficulty(const FString & String, ECPUDifficulty & OutEnumValue) const;

protected:

	//-------------------------------------------------------
	//	Data
	//-------------------------------------------------------

	/*-------------------------------------------------------------------------------------------
	*	Structures that map enum values to string/text representations. Usually this stuff
	*	would be on the game instance but I sometimes need to access this stuff at times when
	*	the GI isn't alive e.g. editor utility widgets. So essentially there are 2 copies
	*	of this data: one here and one on GI which is bad.
	---------------------------------------------------------------------------------------------*/
	/* I use CDO of this class and that's it. In future if that changes then make these non-const, 
	and only populate them if we're NOT the CDO cause the CDO will be ignored. This will 
	save on memory */

	const TMap<EEditorPlaySkippingOption, FString> EnumToString_EditorPlaySkippingOption;
	const TMap<EDefeatCondition, FString> EnumToString_DefeatConditions;
	const TMap<EInvalidOwnerIndexAction, FString> EnumToString_InvalidOwnerIndexAction;
	const TMap<EFaction, FString> EnumToString_Faction;
	const TMap<ETeam, FString> EnumToString_Team;
	const TMap<ECPUDifficulty, FString> EnumToString_CPUDifficulty;

	static TMap<EEditorPlaySkippingOption, FString> InitializeEnumToString_EditorPlaySkippingOption();
	static TMap<EDefeatCondition, FString> InitializeEnumToString_DefeatConditions();
	static TMap<EInvalidOwnerIndexAction, FString> InitializeEnumToString_InvalidOwnerIndexAction();
	static TMap<EFaction, FString> InitializeEnumToString_Faction();
	static TMap<ETeam, FString> InitializeEnumToString_Team();
	static TMap<ECPUDifficulty, FString> InitializeEnumToString_CPUDifficulty();

#endif // WITH_EDITORONLY_DATA
};
