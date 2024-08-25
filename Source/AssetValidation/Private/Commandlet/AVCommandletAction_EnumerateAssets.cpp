#include "Commandlet/AVCommandletAction_EnumerateAssets.h"

#include "AssetValidationDefines.h"

bool UAVCommandletAction_EnumerateAssets::Run(const TArray<FAssetData>& Assets)
{
	UE_LOG(LogAssetValidation, Display, TEXT("UAssetValidationCommandlet: Found %d assets."), Assets.Num());
	return true;
}
