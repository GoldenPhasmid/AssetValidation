#include "PropertyValidatorSubsystem.h"

#include "PropertyValidators/PropertyValidatorBase.h"

bool UPropertyValidatorSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	TArray<UClass*> ChildClasses;
	GetDerivedClasses(GetClass(), ChildClasses, false);

	// add chance to override this subsystem with derived class
	return ChildClasses.Num() == 0;
}

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

void UPropertyValidatorSubsystem::IsPropertyContainerValid(void* Container, UStruct* Struct, FPropertyValidationResult& ValidationResult) const
{
	UPackage* Package = Struct->GetPackage();

	while (Struct && CanValidatePackage(Package))
	{
		if (bSkipBlueprintGeneratedClasses && IsBlueprintGenerated(Package))
		{
			Struct = Struct->GetSuperStruct();
			continue;
		}

		for (TFieldIterator<FProperty> It(Struct, EFieldIterationFlags::None); It; ++It)
		{
			FProperty* Property = *It;
			// do not validate transient or deprecated properties
			// only validate properties that we can actually edit in editor
			if (!Property->HasAnyPropertyFlags(EPropertyFlags::CPF_Transient) && Property->HasAnyPropertyFlags(EPropertyFlags::CPF_Edit))
			{
				FPropertyValidationResult Result;
				IsPropertyValid(Container, Property, Result);
				ValidationResult.Append(Result);
			}
		}
		
		Struct = Struct->GetSuperStruct();
	}
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

bool UPropertyValidatorSubsystem::CanValidatePackage(UPackage* Package) const
{
	if (IsBlueprintGenerated(Package))
	{
		return true;
	}
	
	const FString PackageName = Package->GetName();

#if WITH_EDITOR
	if (PackageName.StartsWith("/Script/AssetValidation"))
	{
		return true;
	}
#endif
	
	return PackagesToValidate.ContainsByPredicate([PackageName](const FString& ModulePath)
	{
		return PackageName.StartsWith(ModulePath);
	});
}

bool UPropertyValidatorSubsystem::IsBlueprintGenerated(UPackage* Package) const
{
	const FString PackageName = Package->GetName();
	return PackageName.StartsWith(TEXT("/Game/"));
}
