#include "AssetValidators/AssetValidator_ExternalObjects.h"

#include "AssetValidationSubsystem.h"
#include "AssetValidationModule.h"
#include "AssetValidationStatics.h"
#include "Misc/DataValidation.h"

bool UAssetValidator_ExternalObjects::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& InContext) const
{
	return CurrentExternalAsset != InAsset && InContext.GetAssociatedExternalObjects().Num();
}

EDataValidationResult UAssetValidator_ExternalObjects::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& InContext)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL(UAssetValidator_ExternalObjects, AssetValidationChannel);
	
	UAssetValidationSubsystem* ValidationSubsystem = UAssetValidationSubsystem::Get();
	check(ValidationSubsystem);

	EDataValidationResult Result = EDataValidationResult::NotValidated;
	for (const FAssetData& AssetData: InContext.GetAssociatedExternalObjects())
	{
		// gather error and warning messages produced by loading an external asset
		UE::AssetValidation::FLogMessageGatherer Gatherer;
		if (UObject* Asset = AssetData.GetAsset({ULevel::LoadAllExternalObjectsTag}))
		{
			TGuardValue AssetGuard{CurrentExternalAsset, Asset};
			LogValidatingAssetMessage(AssetData, InContext);
			
			Result &= ValidationSubsystem->IsAssetValidWithContext(AssetData, InContext);
		}
		else
		{
			UE::AssetValidation::AppendValidationMessages(InContext, AssetData, EMessageSeverity::Error, {TEXT("Failed to load object")});
		}

		UE::AssetValidation::AppendValidationMessages(InContext, AssetData, Gatherer);
	}

	return Result;
}
