#include "PropertyValidationSettings.h"

bool UPropertyValidationSettings::CanValidatePackage(const FString& PackageName)
{
	return Get()->PackagesToValidate.ContainsByPredicate([PackageName](const FString& ModulePath)
	{
		return PackageName.StartsWith(ModulePath);
	});
}

void UPropertyValidationSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	
	for (FEngineClassDescription& ClassDesc: EngineClasses)
	{
		for (FEngineVariableDescription& PropertyDesc: ClassDesc.Properties)
		{
			PropertyDesc.Struct = ClassDesc.EngineClass.Get();
		}
	}

	for (FEngineStructDescription& StructDesc: EngineStructs)
	{
		for (FEngineVariableDescription& PropertyDesc: StructDesc.Properties)
		{
			PropertyDesc.Struct = StructDesc.StructClass;
		}
	}
}
