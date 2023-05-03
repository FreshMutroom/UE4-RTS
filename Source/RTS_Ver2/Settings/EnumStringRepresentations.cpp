// Fill out your copyright notice in the Description page of Project Settings.


#include "EnumStringRepresentations.h"

#include "Statics/Statics.h"
#include "Statics/DevelopmentStatics.h"
#include "Settings/ProjectSettings.h"


#if WITH_EDITORONLY_DATA
UEnumStringRepresentations::UEnumStringRepresentations()
	: EnumToString_EditorPlaySkippingOption(InitializeEnumToString_EditorPlaySkippingOption()) 
	, EnumToString_DefeatConditions(InitializeEnumToString_DefeatConditions()) 
	, EnumToString_InvalidOwnerIndexAction(InitializeEnumToString_InvalidOwnerIndexAction()) 
	, EnumToString_Faction(InitializeEnumToString_Faction()) 
	, EnumToString_Team(InitializeEnumToString_Team()) 
	, EnumToString_CPUDifficulty(InitializeEnumToString_CPUDifficulty())
{
}

TMap<EEditorPlaySkippingOption, FString> UEnumStringRepresentations::InitializeEnumToString_EditorPlaySkippingOption()
{
	TMap<EEditorPlaySkippingOption, FString> Map;
	Map.Reserve(Statics::NUM_EDITOR_PLAY_SKIP_OPTIONS);

	Map.Emplace(EEditorPlaySkippingOption::SkipNothing, "Skip Nothing");
	Map.Emplace(EEditorPlaySkippingOption::SkipOpeningCutsceneOnly, "Skip Opening Cutscene Only");
	Map.Emplace(EEditorPlaySkippingOption::SkipMainMenu, "Play Match Straight Away");

	return Map;
}

TMap<EDefeatCondition, FString> UEnumStringRepresentations::InitializeEnumToString_DefeatConditions()
{
	TMap<EDefeatCondition, FString> Map;
	Map.Reserve(Statics::NUM_DEFEAT_CONDITIONS);

	for (int32 i = 0; i < Statics::NUM_DEFEAT_CONDITIONS; ++i)
	{
		const EDefeatCondition EnumValue = Statics::ArrayIndexToDefeatCondition(i);
		Map.Emplace(EnumValue, ENUM_TO_STRING(EDefeatCondition, EnumValue));
	}

	/* You can hardcode values here if you want if the default value isn't good enough */

	return Map;
}

TMap<EInvalidOwnerIndexAction, FString> UEnumStringRepresentations::InitializeEnumToString_InvalidOwnerIndexAction()
{
	TMap<EInvalidOwnerIndexAction, FString> Map;
	Map.Reserve(Statics::NUM_PIE_SESSION_INVALID_OWNER_RULES);

	for (int32 i = 0; i < Statics::NUM_PIE_SESSION_INVALID_OWNER_RULES; ++i)
	{
		const EInvalidOwnerIndexAction EnumValue = Statics::ArrayIndexToPIESeshInvalidOwnerRule(i);
		Map.Emplace(EnumValue, ENUM_TO_STRING(EInvalidOwnerIndexAction, EnumValue));
	}

	return Map;
}

TMap<EFaction, FString> UEnumStringRepresentations::InitializeEnumToString_Faction()
{
	TMap<EFaction, FString> Map;
	Map.Reserve(Statics::NUM_FACTIONS);
	for (int32 i = 0; i < Statics::NUM_FACTIONS; ++i)
	{
		const EFaction EnumValue = Statics::ArrayIndexToFaction(i);
		Map.Emplace(EnumValue, ENUM_TO_STRING(EFaction, EnumValue));
	}
	
	return Map;
}

TMap<ETeam, FString> UEnumStringRepresentations::InitializeEnumToString_Team()
{
	TMap<ETeam, FString> Map;
	Map.Reserve(ProjectSettings::MAX_NUM_TEAMS);
	for (int32 i = 0; i < ProjectSettings::MAX_NUM_TEAMS; ++i)
	{
		const ETeam EnumValue = Statics::ArrayIndexToTeam(i);
		Map.Emplace(EnumValue, ENUM_TO_STRING(ETeam, EnumValue));
	}
	Map.Emplace(ETeam::Observer, ENUM_TO_STRING(ETeam, ETeam::Observer));

	return Map;
}

TMap<ECPUDifficulty, FString> UEnumStringRepresentations::InitializeEnumToString_CPUDifficulty()
{
	TMap<ECPUDifficulty, FString> Map;
	Map.Reserve(Statics::NUM_CPU_DIFFICULTIES + 1);
	Map.Emplace(ECPUDifficulty::DoesNothing, "Does Nothing");
	for (int32 i = 0; i < Statics::NUM_CPU_DIFFICULTIES; ++i)
	{
		const ECPUDifficulty EnumValue = Statics::ArrayIndexToCPUDifficulty(i);
		Map.Emplace(EnumValue, ENUM_TO_STRING(ECPUDifficulty, EnumValue));
	}

	return Map;
}

const UEnumStringRepresentations * UEnumStringRepresentations::Get()
{
	return GetDefault<UEnumStringRepresentations>();
}

FString UEnumStringRepresentations::GetString(EEditorPlaySkippingOption EnumValue) const
{
	return EnumToString_EditorPlaySkippingOption[EnumValue];
}

FString UEnumStringRepresentations::GetString(EDefeatCondition EnumValue) const
{
	return EnumToString_DefeatConditions[EnumValue];
}

FString UEnumStringRepresentations::GetString(EInvalidOwnerIndexAction EnumValue) const
{
	return  EnumToString_InvalidOwnerIndexAction[EnumValue];
}

FString UEnumStringRepresentations::GetString(EFaction EnumValue) const
{
	return EnumToString_Faction[EnumValue];
}

FString UEnumStringRepresentations::GetString(ETeam EnumValue) const
{
	UE_CLOG(EnumToString_Team.Contains(EnumValue) == false, RTSLOG, Fatal, TEXT("Container entry "
		"for team [%s] not present"), TO_STRING(ETeam, EnumValue));
	return EnumToString_Team[EnumValue];
}

FString UEnumStringRepresentations::GetString(ECPUDifficulty EnumValue) const
{
	return EnumToString_CPUDifficulty[EnumValue];
}

bool UEnumStringRepresentations::GetEnumValueFromStringSlow_EditorPlaySkippingOption(const FString & String, EEditorPlaySkippingOption & OutEnumValue) const
{
	for (const auto & Pair : EnumToString_EditorPlaySkippingOption)
	{
		if (Pair.Value == String)
		{
			OutEnumValue = Pair.Key;
			return true;
		}
	}
	return false;
}

bool UEnumStringRepresentations::GetEnumValueFromStringSlow_DefeatCondition(const FString & String, EDefeatCondition & OutEnumValue) const
{
	for (const auto & Pair : EnumToString_DefeatConditions)
	{
		if (Pair.Value == String)
		{
			OutEnumValue = Pair.Key;
			return true;
		}
	}
	return false;
}

bool UEnumStringRepresentations::GetEnumValueFromStringSlow_PIEPlayInvalidOwnerRule(const FString & String, EInvalidOwnerIndexAction & OutEnumValue) const
{
	for (const auto & Pair : EnumToString_InvalidOwnerIndexAction)
	{
		if (Pair.Value == String)
		{
			OutEnumValue = Pair.Key;
			return true;
		}
	}
	return false;
}
bool UEnumStringRepresentations::GetEnumValueFromStringSlow_Faction(const FString & String, EFaction & OutEnumValue) const
{
	for (const auto & Pair : EnumToString_Faction)
	{
		if (Pair.Value == String)
		{
			OutEnumValue = Pair.Key;
			return true;
		}
	}
	return false;
}

bool UEnumStringRepresentations::GetEnumValueFromStringSlow_Team(const FString & String, ETeam & OutEnumValue) const
{
	for (const auto & Pair : EnumToString_Team)
	{
		if (Pair.Value == String)
		{
			OutEnumValue = Pair.Key;
			return true;
		}
	}
	return false;
}

bool UEnumStringRepresentations::GetEnumValueFromStringSlow_CPUDifficulty(const FString & String, ECPUDifficulty & OutEnumValue) const
{
	for (const auto & Pair : EnumToString_CPUDifficulty)
	{
		if (Pair.Value == String)
		{
			OutEnumValue = Pair.Key;
			return true;
		}
	}
	return false;
}

#endif//WITH_EDITORONLY_DATA
