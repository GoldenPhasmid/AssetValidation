﻿#pragma once

#include "CoreMinimal.h"

#include "AVCommandletAction.generated.h"

struct FAssetData;

USTRUCT()
struct FAVCommandletActionResultBase
{
	GENERATED_BODY()

	FAVCommandletActionResultBase() = default;
	explicit FAVCommandletActionResultBase(const FAssetData& AssetData);
	
	UPROPERTY()
	FString AssetName;

	UPROPERTY()
	FString AssetPath;
};


UCLASS(Abstract, EditInlineNew, DefaultToInstanced)
class ASSETVALIDATION_API UAVCommandletAction: public UObject
{
	GENERATED_BODY()
public:

	/**
	 * Parse commandline parameters into action properties
	 */
	virtual void InitFromCommandlet(const TArray<FString>& Switches, const TMap<FString, FString>& Params);

	/**
	 * Action implementation should live here
	 * @return true if action succeeded at completing the operation, false otherwise
	 */
	virtual bool Run(const TArray<FAssetData>& Assets) { return false; }
	
};
