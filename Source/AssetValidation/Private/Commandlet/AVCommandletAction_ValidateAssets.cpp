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

	CommandletDisabledValidators.Reset();
	if (const FString* Values = Params.Find(UE::AssetValidation::DisableValidators))
	{
		TArray<FString> ClassNames;
		Values->ParseIntoArray(ClassNames, UE::AssetValidation::Separator);

		Algo::Transform(ClassNames, CommandletDisabledValidators, [](const FString& ClassName)
		{
			return FName{ClassName};
		});
	}

	for (TSoftClassPtr<UAssetValidator> Validator: DisabledValidators)
	{
		CommandletDisabledValidators.Add(Validator->GetFName());;
	}

}

bool UAVCommandletAction_ValidateAssets::Run(const TArray<FAssetData>& Assets)
{
	if (Assets.IsEmpty())
	{
		return true;
	}
	
	UAssetValidationSettings& ProjectSettings = *UAssetValidationSettings::GetMutable();
	// override detailed asset logging switch
	TGuardValue Guard{ProjectSettings.bEnabledDetailedAssetLogging, bDetailedLog};
	
	FValidateAssetsSettings Settings;
	Settings.bSkipExcludedDirectories = bSkipExcludedDirectories;
	Settings.bShowIfNoFailures = true;
	Settings.ValidationUsecase = EDataValidationUsecase::Commandlet;

	TArray<UEditorValidatorBase*> TempDisabledValidators;
	DisableValidators(TempDisabledValidators);

	const UAssetValidationSubsystem* Subsystem = GEditor->GetEditorSubsystem<UAssetValidationSubsystem>();
	
	FValidateAssetsResults Results;
	Subsystem->ValidateAssetsWithSettings(Assets, Settings, Results);

	EnableValidators(TempDisabledValidators);
	
	return Results.NumInvalid > 0;
}

void UAVCommandletAction_ValidateAssets::DisableValidators(TArray<UEditorValidatorBase*>& OutDisabledValidators)
{
	UAssetValidationSubsystem* Subsystem = UAssetValidationSubsystem::Get();
	Subsystem->ForEachEnabledValidator([this, &OutDisabledValidators](UEditorValidatorBase* EditorValidator)
	{
		if (CommandletDisabledValidators.Contains(EditorValidator->GetClass()->GetFName()))
		{
			OutDisabledValidators.Add(EditorValidator);
			UE::AssetValidation::SetValidatorEnabled(EditorValidator, false);
		}
		return true;
	});
}

void UAVCommandletAction_ValidateAssets::EnableValidators(TArrayView<UEditorValidatorBase*> Validators)
{
	for (UEditorValidatorBase* EditorValidator: Validators)
	{
		check(EditorValidator);
		// ForEachEnabledValidator always skips disabled validators, so we just have to re-enable those that we disabled
		UE::AssetValidation::SetValidatorEnabled(EditorValidator, true);
	}
}
