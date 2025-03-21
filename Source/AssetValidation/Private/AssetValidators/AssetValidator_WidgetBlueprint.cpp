#include "AssetValidators/AssetValidator_WidgetBlueprint.h"

#include "AssetValidationDefines.h"
#include "AssetValidationStatics.h"
#include "PropertyValidatorSubsystem.h"
#include "Editor/UMGEditor/Public/WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
bool UAssetValidator_WidgetBlueprint::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const
{
	return Super::CanValidateAsset_Implementation(InAssetData, InObject, InContext) && InObject && InObject->IsA<UWidgetBlueprint>();
}

EDataValidationResult UAssetValidator_WidgetBlueprint::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL(UAssetValidator_WidgetBlueprint, AssetValidationChannel);

	UPropertyValidatorSubsystem* PropertyValidators = GEditor->GetEditorSubsystem<UPropertyValidatorSubsystem>();
	check(PropertyValidators);

	const UWidgetBlueprint* WidgetBlueprint = CastChecked<UWidgetBlueprint>(InAsset);

	FPropertyValidationResult Result = PropertyValidators->ValidateObject(WidgetBlueprint->WidgetTree);
	
	UE::AssetValidation::AppendMessages(Context, InAssetData, EMessageSeverity::Error, Result.Errors);
	UE::AssetValidation::AppendMessages(Context, InAssetData, EMessageSeverity::Warning, Result.Warnings);

	return Result.ValidationResult;
}
