// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "EditorValidatorBase.h"

#include "AssetValidator.generated.h"

UCLASS(Abstract, Blueprintable)
class ASSETVALIDATION_API UAssetValidator: public UEditorValidatorBase
{
	GENERATED_BODY()
public:
	virtual bool CanValidateAsset_Implementation(UObject* InAsset) const override;
};
