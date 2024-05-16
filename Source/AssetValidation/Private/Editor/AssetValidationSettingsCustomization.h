#pragma once

#include "IDetailCustomization.h"


namespace UE::AssetValidation
{
	
class FAssetValidationSettingsCustomization: public IDetailCustomization
{
	using ThisClass = FAssetValidationSettingsCustomization;
public:
	static TSharedRef<IDetailCustomization> MakeInstance()
	{
		return MakeShareable(new FAssetValidationSettingsCustomization());
	}

	virtual ~FAssetValidationSettingsCustomization() override;
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
	virtual void CustomizeDetails(const TSharedPtr<IDetailLayoutBuilder>& DetailBuilder) override;

protected:

	void OnAssetAdded(const FAssetData& AssetData);
	void OnAssetRemoved(const FAssetData& AssetData);
	void OnAssetRenamed(const FAssetData& AssetData, const FString& NewName);
	
	void RebuildDetails(const FAssetData& AssetData, bool bRemoved);
	UClass* GetAssetValidatorClassFromAsset(const FAssetData& AssetData) const;
	
	/** Cached details builder to rebuild details when a new asset is added */
	TWeakPtr<IDetailLayoutBuilder> CachedDetailBuilder;
};
	
}

