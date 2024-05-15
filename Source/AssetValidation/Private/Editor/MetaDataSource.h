#pragma once

#include "PropertyExtensionTypes.h"
#include "Engine/Blueprint.h"

namespace UE::AssetValidation
{

class FMetaDataSource
{
public:
	FMetaDataSource() = default;
	FMetaDataSource(FProperty* Property)
	{
		Variant.Set<FProperty*>(Property);
	}
	FMetaDataSource(const FProperty* Property)
	{
		Variant.Set<FProperty*>(const_cast<FProperty*>(Property)); // @todo: remove const_cast after everything clears up
	}
	FMetaDataSource(const FEnginePropertyExtension& PropertyData)
	{
		Variant.Set<FEnginePropertyExtension>(PropertyData);
	}

	FProperty* GetProperty() const
	{
		return Variant.Get<FProperty*>();
	}

	FEnginePropertyExtension GetExternalData() const
	{
		return Variant.Get<FEnginePropertyExtension>();
	}

	void SetProperty(FProperty* Property)
	{
		Variant.Set<FProperty*>(Property);
	}

	void SetExternalData(const FEnginePropertyExtension& PropertyData)
	{
		Variant.Set<FEnginePropertyExtension>(PropertyData);
	}

	bool IsValid() const;

	FString GetMetaData(const FName& Key) const;
	bool HasMetaData(const FName& Key) const;
	void SetMetaData(const FName& Key, const FString& Value);
	void RemoveMetaData(const FName& Key);

private:
	TVariant<FEmptyVariantState, FProperty*, FEnginePropertyExtension> Variant;
};

}

