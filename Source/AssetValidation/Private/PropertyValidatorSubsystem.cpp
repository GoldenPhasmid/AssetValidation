#include "PropertyValidatorSubsystem.h"

#include "ContainerValidators/PropertyContainerValidator.h"
#include "PropertyValidators/PropertyValidatorBase.h"
#include "PropertyValidators/PropertyValidation.h"

bool GValidateStructPropertiesWithoutMeta = true;
FAutoConsoleVariableRef ValidateStructPropertiesWithoutMeta(
	TEXT("PropertyValidation.ValidateStructsWithoutMeta"),
	GValidateStructPropertiesWithoutMeta,
	TEXT("")
);

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
		if (!ValidatorClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated))
		{
			UPropertyValidatorBase* Validator = NewObject<UPropertyValidatorBase>(GetTransientPackage(), ValidatorClass);
			PropertyValidators.Add(Validator);
		}
	}

	TArray<UClass*> ContainerClasses;
	GetDerivedClasses(UPropertyContainerValidator::StaticClass(), ContainerClasses, true);

	for (UClass* ContainerClass: ContainerClasses)
	{
		if (!ContainerClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated))
		{
			UPropertyContainerValidator* Validator = NewObject<UPropertyContainerValidator>(GetTransientPackage(), ContainerClass);
			ContainerValidators.Add(Validator);
		}
	}
}

void UPropertyValidatorSubsystem::Deinitialize()
{
	PropertyValidators.Empty();
	
	Super::Deinitialize();
}

FPropertyValidationResult UPropertyValidatorSubsystem::IsPropertyContainerValid(const UObject* Object) const
{
	if (!IsValid(Object))
	{
		// count invalid objects as valid in property validation. 
		return FPropertyValidationResult{EDataValidationResult::Valid};
	}

	// package check happens inside WithContext call
	FPropertyValidationContext ValidationContext(this, Object);
	IsPropertyContainerValidWithContext(static_cast<const void*>(Object), Object->GetClass(), ValidationContext);

	return ValidationContext.MakeValidationResult();
}

FPropertyValidationResult UPropertyValidatorSubsystem::IsPropertyContainerValid(const UObject* OwningObject, const UScriptStruct* ScriptStruct, void* Data)
{
	if (!IsValid(OwningObject) || ScriptStruct == nullptr || Data == nullptr)
	{
		// count invalid objects as valid in property validation. 
		return FPropertyValidationResult{EDataValidationResult::Valid};
	}

	// package check happens inside WithContext call
	FPropertyValidationContext ValidationContext(this, OwningObject);
	IsPropertyContainerValidWithContext(Data, ScriptStruct, ValidationContext);
	
	return ValidationContext.MakeValidationResult();
}

FPropertyValidationResult UPropertyValidatorSubsystem::IsPropertyValid(const UObject* Object, FProperty* Property) const
{
	if (!IsValid(Object) || Property == nullptr)
	{
		// count invalid objects or properties as valid in property validation. 
		return FPropertyValidationResult{EDataValidationResult::Valid};
	}

	// explicitly check for object package
	FPropertyValidationContext ValidationContext(this, Object);
	if (!ShouldValidatePackage(Object->GetPackage(), ValidationContext))
	{
		return FPropertyValidationResult{EDataValidationResult::Valid};
	}
	
	IsPropertyValidWithContext(static_cast<const void*>(Object), Property, ValidationContext);

	return ValidationContext.MakeValidationResult();
}

FPropertyValidationResult UPropertyValidatorSubsystem::IsPropertyValid(const UObject* OwningObject, const UScriptStruct* ScriptStruct, FProperty* Property, void* Data)
{
	if (!IsValid(OwningObject) || ScriptStruct == nullptr || Property == nullptr || Data == nullptr)
	{
		// count invalid objects or properties as valid in property validation. 
		return FPropertyValidationResult{EDataValidationResult::Valid};
	}

	// explicitly check for object package
	FPropertyValidationContext ValidationContext(this, OwningObject);
	if (!ShouldValidatePackage(OwningObject->GetPackage(), ValidationContext))
	{
		return FPropertyValidationResult{EDataValidationResult::Valid};
	}

	IsPropertyValidWithContext(Data, Property, ValidationContext);

	return ValidationContext.MakeValidationResult();
}

void UPropertyValidatorSubsystem::IsPropertyContainerValidWithContext(const void* Container, const UStruct* Struct, FPropertyValidationContext& ValidationContext) const
{
	while (Struct && ShouldValidatePackage(Struct->GetPackage(), ValidationContext))
	{
		if (bSkipBlueprintGeneratedClasses && IsBlueprintGenerated(Struct->GetPackage()))
		{
			Struct = Struct->GetSuperStruct();
			continue;
		}

		// EFieldIterationFlags::None because we look only at given Struct valid properties
		for (TFieldIterator<FProperty> It(Struct, EFieldIterationFlags::None); It; ++It)
		{
			IsPropertyValidWithContext(Container, *It, ValidationContext);
		}
		
		Struct = Struct->GetSuperStruct();
	}
}

void UPropertyValidatorSubsystem::IsPropertyValidWithContext(const void* Container, const FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	// check whether we should validate property at all
	if (!ShouldValidateProperty(Property, ValidationContext))
	{
		return;
	}
		
	for (UPropertyValidatorBase* Validator: PropertyValidators)
	{
		if (Validator->CanValidateProperty(Property))
		{
			Validator->ValidateProperty(Container, Property, ValidationContext);
			break;
		}
	}

	const void* PropertyMemory = Property->ContainerPtrToValuePtr<void>(Container);
	for (UPropertyContainerValidator* Validator: ContainerValidators)
	{
		if (Validator->CanValidateContainerProperty(Property))
		{
			Validator->ValidateContainerProperty(PropertyMemory, Property, ValidationContext);
			break;
		}
	}
}

void UPropertyValidatorSubsystem::IsPropertyValueValidWithContext(const void* Value, const FProperty* ParentProperty, const FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const
{
	// do not check for property flags for ParentProperty or ValueProperty.
	// ParentProperty has already been checked and ValueProperty is set by container so it doesn't have metas or required property flags
	for (UPropertyValidatorBase* Validator: PropertyValidators)
	{
		if (Validator->CanValidatePropertyValue(ValueProperty, Value))
		{
			Validator->ValidatePropertyValue(Value, ParentProperty, ValueProperty, ValidationContext);
			break;
		}
	}

	const void* PropertyMemory = Value;
	for (UPropertyContainerValidator* Validator: ContainerValidators)
	{
		if (Validator->CanValidateContainerProperty(ValueProperty))
		{
			Validator->ValidateContainerProperty(PropertyMemory, ValueProperty, ValidationContext);
			break;
		}
	}
}

bool UPropertyValidatorSubsystem::ShouldValidatePackage(UPackage* Package, FPropertyValidationContext& ValidationContext) const
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

bool UPropertyValidatorSubsystem::ShouldValidateProperty(const FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	if (Property->HasAnyPropertyFlags(EPropertyFlags::CPF_Deprecated | EPropertyFlags::CPF_Transient | EPropertyFlags::CPF_SkipSerialization))
	{
		return false;
	}
	
	if (Property->HasAnyPropertyFlags(EPropertyFlags::CPF_Edit))
	{
		const UObject* SourceObject = ValidationContext.GetSourceObject();
		if (SourceObject->IsAsset())
		{
			// assets ignore EditDefaultsOnly and EditInstanceOnly specifics
			return true;
		}

		if (Property->HasAnyPropertyFlags(EPropertyFlags::CPF_DisableEditOnInstance) && !SourceObject->IsTemplate())
		{
			// EditDefaultsOnly property for instance object (not template and not asset)
			return false;
		}
		if (Property->HasAnyPropertyFlags(EPropertyFlags::CPF_DisableEditOnTemplate) && SourceObject->IsTemplate())
		{
			// EditInstanceOnly property for template object
			return false;
		}
		

		return true;
	}

	return false;
}

bool UPropertyValidatorSubsystem::IsBlueprintGenerated(UPackage* Package) const
{
	const FString PackageName = Package->GetName();
	// package is blueprint generated if it is either in Content folder or Plugins/Content folder
	return PackageName.StartsWith(TEXT("/Game/")) || !PackageName.StartsWith(TEXT("/Script"));
}


