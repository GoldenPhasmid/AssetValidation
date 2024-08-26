#include "Commandlet/AVCommandletAction_FixupRedirectors.h"

#include "AssetToolsModule.h"
#include "AssetValidationDefines.h"
#include "IAssetTools.h"

UAVCommandletAction_FixupRedirectors::UAVCommandletAction_FixupRedirectors()
{
	FixupMode = ERedirectFixupMode::DeleteFixedUpRedirectors;
}

bool UAVCommandletAction_FixupRedirectors::Run(const TArray<FAssetData>& Assets)
{
	static const FTopLevelAssetPath RedirectorClass = UObjectRedirector::StaticClass()->GetClassPathName();

	TArray<UObjectRedirector*> Redirectors;
	for (const FAssetData& AssetData: Assets)
	{
		if (AssetData.AssetClassPath != RedirectorClass)
		{
			continue;
		}

		if (UObjectRedirector* Redirector = CastChecked<UObjectRedirector>(AssetData.GetAsset(), ECastCheckedType::NullAllowed))
		{
			Redirectors.Add(Redirector);
		}
	}

	if (!Redirectors.IsEmpty())
	{
		UE_LOG(LogAssetValidation, Display, TEXT("Found %d redirectors. Performing fixup..."), Redirectors.Num());

		const FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		AssetToolsModule.Get().FixupReferencers(Redirectors, bPromptCheckout, FixupMode);
	}

	UE_LOG(LogAssetValidation, Display, TEXT("Failed to find redirectors."));

	return true;
}
