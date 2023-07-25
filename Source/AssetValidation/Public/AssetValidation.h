// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
struct FPropertyValidationResult;
class FToolBarBuilder;
class FMenuBuilder;


class UPropertyValidatorBase;

class ASSETVALIDATION_API IAssetValidationModule: public IModuleInterface
{
public:

	static IAssetValidationModule& Get()
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_IAssetValidationModule_Get);
		static IAssetValidationModule& Module = FModuleManager::Get().LoadModuleChecked<IAssetValidationModule>("AssetValidation");
		return Module;
	}

	static bool IsLoaded()
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_IAssetValidationModule_IsAvailable);
		return FModuleManager::Get().IsModuleLoaded("AssetValidation");
	}

	virtual void ValidateProperty(FProperty* Property, FPropertyValidationResult& OutValidationResult) const = 0;
};