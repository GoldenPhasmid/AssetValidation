// Copyright Epic Games, Inc. All Rights Reserved.

#include "AssetValidators/AssetValidator.h"

bool UAssetValidator::CanValidateAsset_Implementation(UObject* InAsset) const
{
	return true;
}

EDataValidationResult UAssetValidator::ValidateAsset(const FAssetData& AssetData, TArray<FText>& ValidationErrors)
{
	return EDataValidationResult::NotValidated;
}
