#include "MetaDataSource.h"

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

void FMetaDataSource::RemoveMetaData(const FName& Key)
{
	if (auto PropertyPtr = Variant.TryGet<FProperty*>())
	{
		return (*PropertyPtr)->RemoveMetaData(Key);
	}
	if (auto ExternalData = Variant.TryGet<FPropertyExternalValidationData>())
	{
		return ExternalData->RemoveMetaData(Key);
	}
	checkNoEntry();
}

}

