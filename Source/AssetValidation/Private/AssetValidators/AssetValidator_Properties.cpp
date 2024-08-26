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
	
	UPropertyValidatorSubsystem* PropertyValidators = GEditor->GetEditorSubsystem<UPropertyValidatorSubsystem>();
	check(PropertyValidators);

	UClass* Class = InAsset->GetClass();
	UObject* Object = InAsset;
	
	if (UBlueprint* Blueprint = Cast<UBlueprint>(InAsset))
	{
		Class = Blueprint->GeneratedClass;
		Object = Class->GetDefaultObject();
	}

	check(Class && Object);
	
	FPropertyValidationResult Result = PropertyValidators->ValidateObject(Object);
	
	UE::AssetValidation::AppendAssetValidationMessages(Context, InAssetData, EMessageSeverity::Error, Result.Errors);
	UE::AssetValidation::AppendAssetValidationMessages(Context, InAssetData, EMessageSeverity::Warning, Result.Warnings);
	
	return Result.ValidationResult;
}
