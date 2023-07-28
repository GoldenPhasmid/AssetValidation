#include "PropertyValidatorSubsystem.h"

#include "PropertyValidators/PropertyValidatorBase.h"
#include "PropertyValidators/PropertyValidation.h"

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
					GroupedValidators.Add(Validator->GetPropertyClass(), Validator);
				}
			}
		}
	}
}

void UPropertyValidatorSubsystem::Deinitialize()
{
	Validators.Empty();
	GroupedValidators.Empty();
	
	Super::Deinitialize();
}

FPropertyValidationResult UPropertyValidatorSubsystem::IsPropertyContainerValid(UObject* Object) const
{
	if (!IsValid(Object))
	{
		return {};
	}

	FPropertyValidationContext ValidationContext(this);
	IsPropertyContainerValid(static_cast<void*>(Object), Object->GetClass(), ValidationContext);

	return ValidationContext.MakeValidationResult();
}

FPropertyValidationResult UPropertyValidatorSubsystem::IsPropertyValid(UObject* Object, FProperty* Property) const
{
	if (!IsValid(Object) || Property == nullptr)
	{
		return {};
	}

	FPropertyValidationContext ValidationContext(this);
	IsPropertyValid(static_cast<void*>(Object), Property, ValidationContext);

	return ValidationContext.MakeValidationResult();
}

void UPropertyValidatorSubsystem::IsPropertyContainerValid(void* Container, UStruct* Struct, FPropertyValidationContext& ValidationContext) const
{
	while (Struct && CanValidatePackage(Struct->GetPackage()))
	{
		if (bSkipBlueprintGeneratedClasses && IsBlueprintGenerated(Struct->GetPackage()))
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
				IsPropertyValid(Container, Property, ValidationContext);
			}
		}
		
		Struct = Struct->GetSuperStruct();
	}
}

void UPropertyValidatorSubsystem::IsPropertyValid(void* Container, FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	const FFieldClass* PropertyClass = Property->GetClass();

	UPropertyValidatorBase* const* PropertyValidatorPtr = GroupedValidators.Find(PropertyClass);
	while (PropertyValidatorPtr == nullptr)
	{
		PropertyValidatorPtr = GroupedValidators.Find(PropertyClass);
		PropertyClass = PropertyClass->GetSuperClass();
	}

	if (PropertyValidatorPtr)
	{
		const UPropertyValidatorBase* PropertyValidator = *PropertyValidatorPtr;
		if (PropertyValidator->CanValidateProperty(Property))
		{
			PropertyValidator->ValidateProperty(Property, Container, ValidationContext);
		}
	}
}

void UPropertyValidatorSubsystem::IsPropertyValueValid(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const
{
	const FFieldClass* PropertyClass = ValueProperty->GetClass();

	UPropertyValidatorBase* const* PropertyValidatorPtr = GroupedValidators.Find(PropertyClass);
	while (PropertyValidatorPtr == nullptr)
	{
		PropertyValidatorPtr = GroupedValidators.Find(PropertyClass);
		PropertyClass = PropertyClass->GetSuperClass();
	}

	const UPropertyValidatorBase* PropertyValidator = *PropertyValidatorPtr;
	if (PropertyValidator && PropertyValidator->CanValidateProperty(ValueProperty))
	{
		PropertyValidator->ValidatePropertyValue(Value, ParentProperty, ValueProperty, ValidationContext);
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


