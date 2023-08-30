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

FPropertyValidationResult UPropertyValidatorSubsystem::IsPropertyContainerValid(UObject* Object) const
{
	if (!IsValid(Object))
	{
		// count invalid objects as valid in property validation. 
		return FPropertyValidationResult{EDataValidationResult::Valid};
	}

	FPropertyValidationContext ValidationContext(this);
	IsPropertyContainerValid(static_cast<void*>(Object), Object->GetClass(), ValidationContext);

	return ValidationContext.MakeValidationResult();
}

FPropertyValidationResult UPropertyValidatorSubsystem::IsPropertyValid(UObject* Object, FProperty* Property) const
{
	if (!IsValid(Object) || Property == nullptr)
	{
		// count invalid objects or properties as valid in property validation. 
		return FPropertyValidationResult{EDataValidationResult::Valid};
	}

	FPropertyValidationContext ValidationContext(this);
	IsPropertyValid(static_cast<void*>(Object), Property, ValidationContext);

	return ValidationContext.MakeValidationResult();
}

void UPropertyValidatorSubsystem::IsPropertyContainerValid(void* Container, UStruct* Struct, FPropertyValidationContext& ValidationContext) const
{
	while (Struct && ShouldValidatePackage(Struct->GetPackage()))
	{
		if (bSkipBlueprintGeneratedClasses && IsBlueprintGenerated(Struct->GetPackage()))
		{
			Struct = Struct->GetSuperStruct();
			continue;
		}

		// EFieldIterationFlags::None because we look only at given Struct valid properties
		for (TFieldIterator<FProperty> It(Struct, EFieldIterationFlags::None); It; ++It)
		{
			IsPropertyValid(Container, *It, ValidationContext);
		}
		
		Struct = Struct->GetSuperStruct();
	}
}

void UPropertyValidatorSubsystem::IsPropertyValid(void* Container, FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	// do not validate transient or deprecated properties
	// only validate properties that we can actually edit in editor
	if (Property->HasAnyPropertyFlags(EPropertyFlags::CPF_Transient) || !Property->HasAnyPropertyFlags(EPropertyFlags::CPF_Edit))
	{
		return;
	}
		
	for (UPropertyValidatorBase* Validator: Validators)
	{
		if (Validator->CanValidateProperty(Property))
		{
			Validator->ValidateProperty(Container, Property, ValidationContext);
		}
	}
}

void UPropertyValidatorSubsystem::IsPropertyValueValid(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const
{
	// do not check for property flags for ParentProperty or ValueProperty.
	// ParentProperty has already been checked and ValueProperty is set by container so it doesn't have metas or required property flags
	for (UPropertyValidatorBase* Validator: Validators)
	{
		if (Validator->CanValidatePropertyValue(ValueProperty, Value))
		{
			Validator->ValidatePropertyValue(Value, ParentProperty, ValueProperty, ValidationContext);
		}
	}
}

bool UPropertyValidatorSubsystem::ShouldValidatePackage(UPackage* Package) const
{
	if (IsBlueprintGenerated(Package))
	{
		return true;
	}
	
	const FString PackageName = Package->GetName();

	// allow validation for project package
	const FString ProjectPackage = FString::Printf(TEXT("/Script/%s"), FApp::GetProjectName());
	if (PackageName.StartsWith(ProjectPackage))
	{
		return true;
	}
	
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
	// package is blueprint generated if it is either in Content folder or Plugins/Content folder
	return PackageName.StartsWith(TEXT("/Game/")) || !PackageName.StartsWith(TEXT("/Script"));
}


