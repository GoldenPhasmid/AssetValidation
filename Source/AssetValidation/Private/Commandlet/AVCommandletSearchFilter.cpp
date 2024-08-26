#include "Commandlet/AVCommandletSearchFilter.h"

#include "Commandlet/AssetValidationCommandlet.h"

void UAVCommandletSearchFilter::InitFromCommandlet(const TArray<FString>& Switches, const TMap<FString, FString>& Params)
{
	UAssetValidationCommandlet::ParseCommandlineParams(this, Switches, Params);
}
