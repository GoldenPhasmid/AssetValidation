#include "AssetValidators/AssetValidator_Properties.h"

#include "AssetValidationStatics.h"
#include "PropertyValidatorSubsystem.h"
#include "Engine/UserDefinedEnum.h"
#include "Engine/UserDefinedStruct.h"

UAssetValidator_Properties::UAssetValidator_Properties()
{
	bIsConfigDisabled = false; // enabled by default

	bCanRunParallelMode = true;
	bRequiresLoadedAsset = true;
	bRequiresTopLevelAsset = false;
	bCanValidateActors = true; // works on actors
}

bool UAssetValidator_Properties::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const
{
	return InObject != nullptr && !InObject->IsA<UUserDefinedStruct>() && !InObject->IsA<UUserDefinedEnum>();
}

EDataValidationResult UAssetValidator_Properties::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL(UAssetValidator_Properties, AssetValidationChannel);
	
	UPropertyValidatorSubsystem* ValidatorSubsystem = GEditor->GetEditorSubsystem<UPropertyValidatorSubsystem>();
	check(ValidatorSubsystem);

	UClass* Class = InAsset->GetClass();
	UObject* Object = InAsset;

	EDataValidationResult Result = EDataValidationResult::Valid;
	if (UBlueprint* Blueprint = Cast<UBlueprint>(InAsset))
	{
		// for blueprint assets, validate SCS and other engine properties with metadata extension
		FPropertyValidationResult OutResult = ValidatorSubsystem->ValidateObject(Blueprint);
		Result &= OutResult.ValidationResult;

		UE::AssetValidation::AppendMessages(Context, InAssetData, EMessageSeverity::Error, OutResult.Errors);
		UE::AssetValidation::AppendMessages(Context, InAssetData, EMessageSeverity::Warning, OutResult.Warnings);

		Class = Blueprint->GeneratedClass;
        Object = Class->GetDefaultObject();
	}

	check(Class && Object);

	// validate default object
	FPropertyValidationResult OutResult = ValidatorSubsystem->ValidateObject(Object);
	Result &= OutResult.ValidationResult;
	
	UE::AssetValidation::AppendMessages(Context, InAssetData, EMessageSeverity::Error, OutResult.Errors);
	UE::AssetValidation::AppendMessages(Context, InAssetData, EMessageSeverity::Warning, OutResult.Warnings);
	
	return Result;
}
