#include "AssetValidators/AssetValidator_ExternalObjects.h"

#include "AssetValidationSubsystem.h"
#include "AssetValidationModule.h"
#include "AssetValidationStatics.h"
#include "Misc/DataValidation.h"

bool UAssetValidator_ExternalObjects::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& InContext) const
{
	return CurrentExternalAsset != InAsset && InContext.GetAssociatedExternalObjects().Num();
}

EDataValidationResult UAssetValidator_ExternalObjects::ValidateAsset_Implementation(const FAssetData& InAssetData, FDataValidationContext& InContext)
{
	return ValidateAssetInternal(InAssetData, InContext);
}

EDataValidationResult UAssetValidator_ExternalObjects::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& InContext)
{
	return ValidateAssetInternal(InAssetData, InContext);
}

EDataValidationResult UAssetValidator_ExternalObjects::ValidateAssetInternal(const FAssetData& InAssetData, FDataValidationContext& InContext)
{
	using namespace UE::AssetValidation;
	TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL(UAssetValidator_ExternalObjects, AssetValidationChannel);
	
	UAssetValidationSubsystem* ValidationSubsystem = UAssetValidationSubsystem::Get();
	check(ValidationSubsystem);

	const uint32 NumValidationErrors = InContext.GetNumErrors();
	EDataValidationResult Result = EDataValidationResult::NotValidated;
	for (const FAssetData& AssetData: InContext.GetAssociatedExternalObjects())
	{
		// gather error and warning messages produced by loading an external asset
		FLogMessageGatherer Gatherer;
		if (UObject* Asset = AssetData.GetAsset({ULevel::LoadAllExternalObjectsTag}))
		{
			TGuardValue AssetGuard{CurrentExternalAsset, Asset};
			LogValidatingAssetMessage(AssetData, InContext);
			
			Result &= ValidationSubsystem->IsAssetValidWithContext(AssetData, InContext);
		}
		else
		{
			AppendAssetValidationMessages(InContext, AssetData, EMessageSeverity::Error, {TEXT("Failed to load object")});
		}

		AppendAssetValidationMessages(InContext, AssetData, Gatherer);
	}

	if (Result == EDataValidationResult::Invalid)
	{
		check(InContext.GetNumErrors() > NumValidationErrors);
		const FText FailReason = FText::Format(NSLOCTEXT("AssetValidation", "AssetCheckFailed", "{0} is not valid. See AssetCheck log for more details"), FText::FromName(InAssetData.AssetName));
		InContext.AddMessage(InAssetData, EMessageSeverity::Error, FailReason);
	}
	
	return Result;
}
