#include "Commandlet/AssetValidationCommandlet.h"

#include "AssetValidationDefines.h"
#include "AssetValidationSettings.h"
#include "WidgetBlueprint.h"
#include "Algo/RemoveIf.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Commandlet/AVCommandletAction_ValidateAssets.h"
#include "Commandlet/AVCommandletSearchFilter.h"
#include "Commandlet/AVCommandletSearchFilter.h"
#include "Engine/UserDefinedEnum.h"
#include "Engine/UserDefinedStruct.h"
#include "Sound/SoundClass.h"


namespace UE::AssetValidation
{
	static const TCHAR* Separator{TEXT(",")};

	static const FString CommandletAction{TEXT("Action")};
	static const FString CommandletFilter{TEXT("Filter")};
	
	static constexpr int32 RESULT_FAIL = 2;
	static constexpr int32 RESULT_SUCCESS = 0;
}

int32 UAssetValidationCommandlet::Main(const FString& Commandline)
{
	UE_LOG(LogAssetValidation, Display, TEXT("Running UAssetValidationCommandlet..."));
	
	TArray<FString> Tokens, Switches;
	TMap<FString, FString> Params;
	ParseCommandLine(*Commandline, Tokens, Switches, Params);

	auto FindClass = [&Params](const FString& ParamName, UClass* DefaultValue) -> TPair<UClass*, FString>
	{
		if (const FString* ParamValue = Params.Find(ParamName))
		{
			UClass* ClassValue = UClass::TryFindTypeSlow<UClass>(*ParamValue, EFindFirstObjectOptions::ExactClass);
			return {ClassValue, *ParamValue};
		}
		
		return {DefaultValue, FString{}};
	};
	
	const auto& [FilterClass, FoundFilterName] = FindClass(UE::AssetValidation::CommandletFilter, UAssetValidationSettings::Get()->CommandletDefaultFilter);
	if (FilterClass == nullptr)
	{
		UE_LOG(LogAssetValidation, Error, TEXT("Failed to find Commandlet Filter: [%s]. Aborting commandlet..."), *FoundFilterName);
		return UE::AssetValidation::RESULT_FAIL;
	}

	const auto& [ActionClass, FoundActionName] = FindClass(UE::AssetValidation::CommandletAction, UAssetValidationSettings::Get()->CommandletDefaultAction);
	if (ActionClass == nullptr)
	{
		UE_LOG(LogAssetValidation, Error, TEXT("Failed to find Commandlet Action: [%s]. Aborting commandlet..."), *FoundActionName);
		return UE::AssetValidation::RESULT_FAIL;
	}
	
	UAVCommandletSearchFilter* SearchFilter = NewObject<UAVCommandletSearchFilter>(this, FilterClass);
	check(SearchFilter);

	SearchFilter->InitFromCommandlet(Switches, Params);
	
	TArray<FAssetData> Assets;
	if (!SearchFilter->GetAssets(Assets) || Assets.IsEmpty())
	{
		return UE::AssetValidation::RESULT_FAIL;
	}

	UAVCommandletAction* Action = NewObject<UAVCommandletAction>(this, ActionClass);
	check(Action);

	Action->InitFromCommandlet(Switches, Params);
	if (!Action->Run(Assets))
	{
		return UE::AssetValidation::RESULT_FAIL;
	}
	
	return UE::AssetValidation::RESULT_SUCCESS;
}
