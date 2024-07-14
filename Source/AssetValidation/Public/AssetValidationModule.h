#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

struct FPropertyValidationResult;
class FToolBarBuilder;
class FMenuBuilder;

namespace UE::AssetValidation
{
	class ISourceControlProxy;
}

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
	
	virtual TSharedRef<UE::AssetValidation::ISourceControlProxy> GetSourceControlProxy() const = 0;
};