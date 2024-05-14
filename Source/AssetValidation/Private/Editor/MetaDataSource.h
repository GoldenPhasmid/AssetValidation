#pragma once

#include "PropertyValidationSettings.h"
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
	FMetaDataSource(const FPropertyExternalValidationData& PropertyData)
	{
		Variant.Set<FPropertyExternalValidationData>(PropertyData);
	}

	FProperty* GetProperty() const
	{
		return Variant.Get<FProperty*>();
	}

	FPropertyExternalValidationData GetExternalData() const
	{
		return Variant.Get<FPropertyExternalValidationData>();
	}

	void SetProperty(FProperty* Property)
	{
		Variant.Set<FProperty*>(Property);
	}

	void SetExternalData(const FPropertyExternalValidationData& PropertyData)
	{
		Variant.Set<FPropertyExternalValidationData>(PropertyData);
	}

	bool IsValid() const;

	FString GetMetaData(const FName& Key) const;
	bool HasMetaData(const FName& Key) const;
	void SetMetaData(const FName& Key, const FString& Value);
	void RemoveMetaData(const FName& Key);

private:
	TVariant<FEmptyVariantState, FProperty*, FPropertyExternalValidationData> Variant;
};

}

