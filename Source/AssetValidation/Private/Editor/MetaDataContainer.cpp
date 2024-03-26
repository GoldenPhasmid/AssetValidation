#include "MetaDataContainer.h"

namespace UE::AssetValidation
{
bool FMetaDataSource::HasMetaData(const FName& Key) const
{
	if (auto PropertyPtr = Variant.TryGet<FProperty*>())
	{
		return (*PropertyPtr)->HasMetaData(Key);
	}
	if (auto ExternalData = Variant.TryGet<FPropertyExternalValidationData>())
	{
		return ExternalData->HasMetaData(Key);
	}

	checkNoEntry();
	return false;
}

FString FMetaDataSource::GetMetaData(const FName& Key) const
{
	if (auto PropertyPtr = Variant.TryGet<FProperty*>())
	{
		return (*PropertyPtr)->GetMetaData(Key);
	}
	if (auto ExternalData = Variant.TryGet<FPropertyExternalValidationData>())
	{
		return ExternalData->GetMetaData(Key);
	}

	checkNoEntry();
	return {};
}

void FMetaDataSource::SetMetaData(const FName& Key, const FString& Value)
{
	if (auto PropertyPtr = Variant.TryGet<FProperty*>())
	{
		return (*PropertyPtr)->SetMetaData(Key, *Value);
	}
	if (auto ExternalData = Variant.TryGet<FPropertyExternalValidationData>())
	{
		return ExternalData->SetMetaData(Key, Value);
	}
	
	checkNoEntry();
}

bool FPropertyMetaDataContainer::HasMetaData(const FName& Key) const
{
	check(Property);
	return Property->HasMetaData(Key);
}

FString FPropertyMetaDataContainer::GetMetaData(const FName& Key) const
{
	check(Property);
	return Property->GetMetaData(Key);
}

void FPropertyMetaDataContainer::SetMetaData(const FName& Key, const FString& Value)
{
	check(Property);
	return Property->SetMetaData(Key, *Value);
}

bool FExternalPropertyMetaDataContainer::HasMetaData(const FName& Key) const
{
	check(PropertyData.IsValid());
	return PropertyData.HasMetaData(Key);
}

FString FExternalPropertyMetaDataContainer::GetMetaData(const FName& Key) const
{
	check(PropertyData.IsValid());
	return PropertyData.GetMetaData(Key);
}

void FExternalPropertyMetaDataContainer::SetMetaData(const FName& Key, const FString& Value)
{
	check(PropertyData.IsValid());
	PropertyData.SetMetaData(Key, Value);
}

bool FBPVariableMetaDataContainer::HasMetaData(const FName& Key) const
{
	return Desc.HasMetaData(Key);
}

FString FBPVariableMetaDataContainer::GetMetaData(const FName& Key) const
{
	return Desc.GetMetaData(Key);
}

void FBPVariableMetaDataContainer::SetMetaData(const FName& Key, const FString& Value)
{
	Desc.SetMetaData(Key, Value);
}
	
}

