#include "Commandlet/AVCommandletAction_ValidateAssets.h"

#include "AssetValidationSettings.h"
#include "AssetValidationStatics.h"
#include "EditorValidatorBase.h"

namespace UE::AssetValidation
{
	static const TCHAR* Separator{TEXT(",")};
	/** Enable detailed logging during validation */
	static const FString DetailedLog{TEXT("DetailedLog")};
	/** Parameter, disable one or more editor validators */
	static const FString DisableValidators{TEXT("DisableValidators")};
}

UAVCommandletAction_ValidateAssets::UAVCommandletAction_ValidateAssets()
{
	ValidationUsecase = EDataValidationUsecase::None;
}

void UAVCommandletAction_ValidateAssets::InitFromCommandlet(const TArray<FString>& Switches, const TMap<FString, FString>& Params)
{
	ValidationUsecase = EDataValidationUsecase::Commandlet;
	bDetailedLog = Switches.Contains(UE::AssetValidation::DetailedLog);
	
	if (const FString* Values = Params.Find(UE::AssetValidation::DisableValidators))
	{
		TArray<FString> ClassNames;
		Values->ParseIntoArray(ClassNames, UE::AssetValidation::Separator);

		Algo::Transform(ClassNames, DisabledValidators, [](const FString& ClassName)
		{
			return TSoftClassPtr<UClass>{ClassName};
		});
	}
}

bool UAVCommandletAction_ValidateAssets::Run(const TArray<FAssetData>& Assets)
{
	if (Assets.IsEmpty())
	{
		return true;
	}
	
	UAssetValidationSettings& ProjectSettings = *UAssetValidationSettings::GetMutable();
	const bool bCachedDetailedLog = ProjectSettings.bEnabledDetailedAssetLogging;
	// enable or disable verbose asset logging depending on a switch
	ProjectSettings.bEnabledDetailedAssetLogging = bDetailedLog;
	
	FValidateAssetsSettings Settings;
	Settings.bSkipExcludedDirectories = bSkipExcludedDirectories;
	Settings.bShowIfNoFailures = true;
	Settings.ValidationUsecase = EDataValidationUsecase::Commandlet;
	DisableValidators();

	const UAssetValidationSubsystem* Subsystem = GEditor->GetEditorSubsystem<UAssetValidationSubsystem>();
	
	FValidateAssetsResults Results;
	Subsystem->ValidateAssetsWithSettings(Assets, Settings, Results);

	ProjectSettings.bEnabledDetailedAssetLogging = bCachedDetailedLog;
	
	return Results.NumInvalid > 0;
}

void UAVCommandletAction_ValidateAssets::DisableValidators()
{
	TSet<UClass*> ValidatorClasses;
	for (TSoftClassPtr<UClass> Validator: DisabledValidators)
	{
		if (UClass* ValidatorClass = Validator.LoadSynchronous())
		{
			ValidatorClasses.Add(ValidatorClass);
		}
	}

	UAssetValidationSubsystem* Subsystem = UAssetValidationSubsystem::Get();
	Subsystem->ForEachEnabledValidator([&ValidatorClasses](UEditorValidatorBase* EditorValidator)
	{
		if (ValidatorClasses.Contains(EditorValidator->GetClass()))
		{
			UE::AssetValidation::SetValidatorEnabled(EditorValidator, false);
		}
		return true;
	});
}
