#include "PropertyValidationSettings.h"

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

const TArray<FPropertyExternalValidationData>& UPropertyValidationSettings::GetExternalValidationData(const UStruct* Struct)
{
	const auto Settings = Get();
	if (const UClass* Class = Cast<UClass>(Struct))
	{
		if (const FClassExternalValidationData* Entry = Settings->ExternalClasses.FindByKey(Class))
		{
			return Entry->Properties;
		}
	}
	else if (const UScriptStruct* ScriptStruct = Cast<UScriptStruct>(Struct))
	{
		if (const FStructExternalValidationData* Entry = Settings->ExternalStructs.FindByKey(ScriptStruct))
		{
			return Entry->Properties;
		}
	}

	static TArray<FPropertyExternalValidationData> EmptyData;
	return EmptyData;
}
