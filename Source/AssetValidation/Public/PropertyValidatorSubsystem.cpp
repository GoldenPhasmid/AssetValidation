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

FPropertyValidationResult UPropertyValidatorSubsystem::IsPropertyValid(UObject* InObject, FProperty* Property) const
{
	FPropertyValidationResult ValidationResult;
	
	if (!Property->HasMetaData(ValidationNames::Validate))
	{
		return ValidationResult;
	}

	for (const auto PropertyValidator: Validators)
	{
		if (PropertyValidator->CanValidateProperty(Property))
		{
			PropertyValidator->ValidateProperty(Property, static_cast<void*>(InObject), ValidationResult);
			break;
		}
	}
	
	return ValidationResult;
}

FPropertyValidationResult UPropertyValidatorSubsystem::IsPropertyValueValid(UObject* InObject, FProperty* ParentProperty, FProperty* ValueProperty)
{
	FPropertyValidationResult ValidationResult;
	
	for (const auto PropertyValidator: Validators)
	{
		if (PropertyValidator->CanValidatePropertyValue(ParentProperty, ValueProperty))
		{
			PropertyValidator->ValidatePropertyValue(static_cast<void*>(InObject), ParentProperty, ValueProperty, ValidationResult);
			break;
		}
	}

	return ValidationResult;
}
