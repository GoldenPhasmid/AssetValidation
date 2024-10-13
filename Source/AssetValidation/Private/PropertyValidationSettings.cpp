#include "PropertyValidationSettings.h"

UPropertyValidationSettings::UPropertyValidationSettings(const FObjectInitializer& Initializer): Super(Initializer)
{
	PackagesToIterate.Add(TEXT("/Game/"));
	PackagesToIgnore.Add(TEXT("/Game/Developers/"));
	PropertyExtensionPaths.Add(TEXT("/AssetValidation/"));
}

void UPropertyValidationSettings::PostInitProperties()
{
	Super::PostInitProperties();
}

void UPropertyValidationSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
