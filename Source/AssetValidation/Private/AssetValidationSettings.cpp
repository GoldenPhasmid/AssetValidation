#include "AssetValidationSettings.h"

#include "Commandlet/AVCommandletAction_ValidateAssets.h"
#include "Commandlet/AVCommandletAssetSearchFilter.h"

UAssetValidationSettings::UAssetValidationSettings(const FObjectInitializer& Initializer): Super(Initializer)
{
	CommandletDefaultFilter = UAVCommandletAssetSearchFilter::StaticClass();
	CommandletDefaultAction = UAVCommandletAction_ValidateAssets::StaticClass();
}
