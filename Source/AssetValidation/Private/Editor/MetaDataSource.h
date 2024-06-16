#pragma once

#include "PropertyExtensionTypes.h"
#include "Engine/Blueprint.h"

namespace UE::AssetValidation
{
	
/**
 *  Variant structure that represents either property or its extension
 *  Provides access to meta data map for a single property
 *  Not a part of FPropertyValidationContext, because multiple properties may be validated as part of a single context
 */
class ASSETVALIDATION_API FMetaDataSource
{
public:
	FMetaDataSource() = default;
	FMetaDataSource(FProperty* Property)
	{
		Variant.Set<FProperty*>(Property);
	}
	FMetaDataSource(const FProperty* Property)
	{
		// @todo: remove const_cast after everything clears up
		Variant.Set<FProperty*>(const_cast<FProperty*>(Property)); 
	}
	FMetaDataSource(const FEnginePropertyExtension& Extension)
	{
		Variant.Set<FEnginePropertyExtension>(Extension);
	}

	template <typename T>
	bool IsType() const;

	FProperty* GetProperty() const
	{
		return Variant.Get<FProperty*>();
	}

	FEnginePropertyExtension GetExtension() const
	{
		return Variant.Get<FEnginePropertyExtension>();
	}

	void SetProperty(FProperty* Property)
	{
		Variant.Set<FProperty*>(Property);
	}

	void SetExtension(const FEnginePropertyExtension& PropertyData)
	{
		Variant.Set<FEnginePropertyExtension>(PropertyData);
	}

	bool IsValid() const;
	
	FString GetMetaData(const FName& Key) const;
	bool HasMetaData(const FName& Key) const;
	void SetMetaData(const FName& Key);
	void SetMetaData(const FName& Key, const FString& Value);
	void RemoveMetaData(const FName& Key);

private:
	TVariant<FEmptyVariantState, FProperty*, FEnginePropertyExtension> Variant;
};
	
template <>
FORCEINLINE bool FMetaDataSource::IsType<FProperty>() const
{
	return Variant.IsType<FProperty*>();
}

template <>
FORCEINLINE bool FMetaDataSource::IsType<FEnginePropertyExtension>() const
{
	return Variant.IsType<FEnginePropertyExtension>();
}


}

