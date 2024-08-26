#include "Commandlet/AVCommandletAction.h"

#include "Commandlet/AssetValidationCommandlet.h"

void UAVCommandletAction::InitFromCommandlet(const TArray<FString>& Switches, const TMap<FString, FString>& Params)
{
	UAssetValidationCommandlet::ParseCommandlineParams(this, Switches, Params);
}
