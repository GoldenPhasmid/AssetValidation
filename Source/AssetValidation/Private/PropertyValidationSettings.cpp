#include "PropertyValidationSettings.h"

bool UPropertyValidationSettings::CanValidatePackage(const FString& PackageName)
{
	return Get()->PackagesToValidate.ContainsByPredicate([PackageName](const FString& ModulePath)
	{
		return PackageName.StartsWith(ModulePath);
	});
}
