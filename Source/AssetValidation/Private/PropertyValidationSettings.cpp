#include "PropertyValidationSettings.h"

#include "Engine/UserDefinedStruct.h"

void UPropertyValidationSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	
	for (FClassExternalValidationData& ClassDesc: ExternalClasses)
	{
		for (FPropertyExternalValidationData& PropertyData: ClassDesc.Properties)
		{
			PropertyData.Struct = ClassDesc.Class.Get();
		}
	}

	for (FStructExternalValidationData& StructDesc: ExternalStructs)
	{
		for (FPropertyExternalValidationData& PropertyData: StructDesc.Properties)
		{
			PropertyData.Struct = StructDesc.StructClass;
		}
	}
}

const TArray<FPropertyExternalValidationData>* UPropertyValidationSettings::GetMutableValidationData(const UStruct* Struct) const
{
	const auto Settings = Get();
	if (const UUserDefinedStruct* UserDefinedStruct = Cast<UUserDefinedStruct>(Struct))
	{
		FSoftObjectPath StructPath = TSoftObjectPtr{UserDefinedStruct}.ToSoftObjectPath();
		if (const FStructExternalValidationData* Entry = Settings->UserDefinedStructs.Find(StructPath))
		{
			return &Entry->Properties;
		}
	}
	else if (const UClass* Class = Cast<UClass>(Struct))
	{
		if (const FClassExternalValidationData* Entry = Settings->ExternalClasses.FindByKey(Class))
		{
			return &Entry->Properties;
		}
	}
	else if (const UScriptStruct* ScriptStruct = Cast<UScriptStruct>(Struct))
	{
		if (const FStructExternalValidationData* Entry = Settings->ExternalStructs.FindByKey(ScriptStruct))
		{
			return &Entry->Properties;
		}
	}

	return nullptr;
}

const TArray<FPropertyExternalValidationData>& UPropertyValidationSettings::GetExternalValidationData(const UStruct* Struct)
{
	const auto Settings = Get();
	if (const TArray<FPropertyExternalValidationData>* StructData = Settings->GetMutableValidationData(Struct))
	{
		return *StructData;
	}
	
	static TArray<FPropertyExternalValidationData> EmptyData;
	return EmptyData;
}

FPropertyExternalValidationData UPropertyValidationSettings::GetPropertyExternalValidationData(UStruct* Struct, FProperty* Property)
{
	const TArray<FPropertyExternalValidationData>& Properties = GetExternalValidationData(Struct);
	auto Pred = [Property](const FPropertyExternalValidationData& PropertyData)
	{
		return PropertyData.GetProperty() == Property;
	};
	
	if (const FPropertyExternalValidationData* PropertyData = Properties.FindByPredicate(Pred))
	{
		return *PropertyData;
	}
	return FPropertyExternalValidationData{Struct, Property};
}

void UPropertyValidationSettings::UpdatePropertyExternalValidationData(const FPropertyExternalValidationData& InData)
{
	if (const TArray<FPropertyExternalValidationData>* StructData = Settings->GetMutableValidationData(Struct))
	{
		
	}
}
