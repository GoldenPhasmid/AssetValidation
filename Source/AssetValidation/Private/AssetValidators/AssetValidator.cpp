#include "AssetValidators/AssetValidator.h"

#include "Misc/DataValidation.h"
#include "AssetValidationDefines.h"
#include "AssetValidationSettings.h"


UAssetValidator::UAssetValidator()
{
	bCanRunParallelMode		= false;
	bRequiresLoadedAsset	= true;
	bRequiresTopLevelAsset	= true;
	bCanValidateActors		= false;
}

void UAssetValidator::PostLoad()
{
	Super::PostLoad();

	bIsEnabled = !bIsConfigDisabled;
	bOnlyPrintCustomMessage = bLogCustomMessageOnly;
}

void UAssetValidator::PostInitProperties()
{
	Super::PostInitProperties();

	bIsEnabled = !bIsConfigDisabled;
	bOnlyPrintCustomMessage = bLogCustomMessageOnly;
}

void UAssetValidator::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.GetMemberPropertyName();
	if (PropertyName == TEXT("bIsEnabled"))
	{
		bIsConfigDisabled = !bIsEnabled;
		TryUpdateDefaultConfigFile();
	}
	else if (PropertyName == TEXT("bOnlyPrintCustomMessage"))
	{
		bLogCustomMessageOnly = bOnlyPrintCustomMessage;
		TryUpdateDefaultConfigFile();
	}
}

bool UAssetValidator::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const
{
	if (bRequiresLoadedAsset && InObject == nullptr)
	{
		return false;
	}

	if (bRequiresTopLevelAsset && !bCanValidateActors && !InAssetData.IsTopLevelAsset())
	{
		return false;
	}

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
	static const UAssetValidationSettings& Settings = *UAssetValidationSettings::Get();
	if (Settings.bEnabledDetailedAssetLogging && AssetData.IsValid())
	{
		// can't use UEditorValidator::AssetMessage because AssetValidator resets its validation state when doing recursive validation
		Context.AddMessage(AssetData, EMessageSeverity::Info, NSLOCTEXT("AssetValidation", "ValidatingAsset", "Validating asset"));
	}
}
