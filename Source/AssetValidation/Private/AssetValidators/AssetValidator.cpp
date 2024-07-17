#include "AssetValidators/AssetValidator.h"

#include "Misc/DataValidation.h"
#include "AssetValidationDefines.h"
#include "AssetValidationSettings.h"


UAssetValidator::UAssetValidator()
{
}

bool UAssetValidator::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const
{
	return true;
}

EDataValidationResult UAssetValidator::ValidateAsset(const FAssetData& InAssetData, FDataValidationContext& InContext)
{
	EDataValidationResult Result = EDataValidationResult::NotValidated;
	
	ResetValidationState();
	if (CanValidateAsset_Implementation(InAssetData, nullptr, InContext))
	{
		Result &= ValidateAsset_Implementation(InAssetData, InContext);
	}
	
	Result &= ExtractValidationState(InContext);
	return Result;
}

void UAssetValidator::LogValidatingAssetMessage(const FAssetData& AssetData, FDataValidationContext& Context)
{
	if (UAssetValidationSettings::Get()->bEnabledDetailedAssetLogging && AssetData.IsValid())
	{
		// can't use UEditorValidator::AssetMessage because AssetValidator resets its validation state when doing recursive validation
		Context.AddMessage(AssetData, EMessageSeverity::Info, NSLOCTEXT("AssetValidation", "ValidatingAsset", "Validating asset"));
	}
}
