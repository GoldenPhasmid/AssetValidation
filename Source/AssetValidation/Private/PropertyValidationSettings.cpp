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
