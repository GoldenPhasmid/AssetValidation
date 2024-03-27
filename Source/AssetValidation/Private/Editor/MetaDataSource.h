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

	void SetProperty(FProperty* Property)
	{
		Variant.Set<FProperty*>(Property);
	}

	void SetExternalData(const FPropertyExternalValidationData& PropertyData)
	{
		Variant.Set<FPropertyExternalValidationData>(PropertyData);
	}
	
	bool HasMetaData(const FName& Key) const;
	FString GetMetaData(const FName& Key) const;
	void SetMetaData(const FName& Key, const FString& Value);
	void RemoveMetaData(const FName& Key);

private:
	TVariant<FProperty*, FPropertyExternalValidationData> Variant;
};

}

