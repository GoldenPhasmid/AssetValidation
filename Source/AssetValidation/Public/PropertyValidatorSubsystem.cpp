#include "PropertyValidatorSubsystem.h"

#include "PropertyValidators/PropertyValidatorBase.h"

void UPropertyValidatorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UPropertyValidatorSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

FPropertyValidationResult UPropertyValidatorSubsystem::ValidateProperty(FProperty* Property) const
{
	return {};
}
