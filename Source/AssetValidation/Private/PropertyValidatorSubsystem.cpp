#include "PropertyValidatorSubsystem.h"

#include "PropertyValidators/PropertyValidatorBase.h"

void UPropertyValidatorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	TArray<UClass*> ValidatorClasses;
	GetDerivedClasses(UPropertyValidatorBase::StaticClass(), ValidatorClasses, true);

	for (UClass* ValidatorClass: ValidatorClasses)
	{
		if (!ValidatorClass->HasAnyClassFlags(CLASS_Abstract))
		{
			if (const UPackage* ClassPackage = ValidatorClass->GetOuterUPackage())
			{
				const FName ModuleName = FPackageName::GetShortFName(ClassPackage->GetFName());
				if (FModuleManager::Get().IsModuleLoaded(ModuleName))
				{
					UPropertyValidatorBase* Validator = NewObject<UPropertyValidatorBase>(GetTransientPackage(), ValidatorClass);
					Validators.Add(Validator);
				}
			}
		}
	}
}

void UPropertyValidatorSubsystem::Deinitialize()
{
	Validators.Empty();
	
	Super::Deinitialize();
}

void UPropertyValidatorSubsystem::IsPropertyValid(void* Container, FProperty* Property, FPropertyValidationResult& ValidationResult) const
{
	if (!Property->HasMetaData(ValidationNames::Validate))
	{
		return;
	}

	for (const auto PropertyValidator: Validators)
	{
		if (PropertyValidator->CanValidateProperty(Property))
		{
			PropertyValidator->ValidateProperty(Property, Container, ValidationResult);
			break;
		}
	}
	
	return;
}

void UPropertyValidatorSubsystem::IsPropertyValueValid(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationResult& ValidationResult)
{
	for (const auto PropertyValidator: Validators)
	{
		if (PropertyValidator->CanValidatePropertyValue(ParentProperty, ValueProperty))
		{
			PropertyValidator->ValidatePropertyValue(Value, ParentProperty, ValueProperty, ValidationResult);
			break;
		}
	}
}
