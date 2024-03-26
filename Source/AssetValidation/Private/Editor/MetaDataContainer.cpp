#include "MetaDataContainer.h"

namespace UE::AssetValidation
{
	
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

bool FEngineVariableMetaDataContainer::HasMetaData(const FName& Key) const
{
	check(Desc.IsValid());
	return Desc.HasMetaData(Key);
}

FString FEngineVariableMetaDataContainer::GetMetaData(const FName& Key) const
{
	check(Desc.IsValid());
	return Desc.GetMetaData(Key);
}

void FEngineVariableMetaDataContainer::SetMetaData(const FName& Key, const FString& Value)
{
	check(Desc.IsValid());
	Desc.SetMetaData(Key, Value);
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

