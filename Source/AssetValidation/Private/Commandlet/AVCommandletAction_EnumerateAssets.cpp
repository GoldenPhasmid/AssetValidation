#include "Commandlet/AVCommandletAction_EnumerateAssets.h"

#include "AssetValidationDefines.h"

bool UAVCommandletAction_EnumerateAssets::Run(const TArray<FAssetData>& Assets)
{
	UE_LOG(LogAssetValidation, Display, TEXT("UAssetValidationCommandlet: Found %d assets."), Assets.Num());

	if (bLogAssets)
	{
		for (const FAssetData& Asset: Assets)
		{
			UE_LOG(LogAssetValidation, Display, TEXT("UAssetValidationCommandlet: Found asset %s, class %s."), *Asset.AssetName.ToString(), *Asset.AssetClassPath.ToString());
		}
	}
	
	return true;
}
