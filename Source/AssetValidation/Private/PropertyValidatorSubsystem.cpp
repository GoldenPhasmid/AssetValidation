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

bool UPropertyValidatorSubsystem::IsContainerProperty(const FProperty* Property)
{
	return Property != nullptr && (Property->IsA(FArrayProperty::StaticClass()) || Property->IsA(FMapProperty::StaticClass()) || Property->IsA(FSetProperty::StaticClass()));
}

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

	for (const UClass* ValidatorClass: ValidatorClasses)
	{
		if (!ValidatorClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated))
		{
			UPropertyValidatorBase* Validator = NewObject<UPropertyValidatorBase>(GetTransientPackage(), ValidatorClass);
			if (Validator->IsA<UPropertyContainerValidator>())
			{
				ContainerValidators.Add(Validator);
			}
			else
			{
				PropertyValidators.Add(Validator);
			}
		}
	}
}

void UPropertyValidatorSubsystem::Deinitialize()
{
	PropertyValidators.Empty();
	
	Super::Deinitialize();
}

FPropertyValidationResult UPropertyValidatorSubsystem::ValidateObject(const UObject* Object) const
{
	if (!IsValid(Object))
	{
		// count invalid objects as valid in property validation. 
		return FPropertyValidationResult{EDataValidationResult::Valid};
	}

	// package check happens inside WithContext call
	FPropertyValidationContext ValidationContext(this, Object);
	ValidateContainerWithContext(reinterpret_cast<const uint8*>(Object), Object->GetClass(), ValidationContext);

	return ValidationContext.MakeValidationResult();
}

FPropertyValidationResult UPropertyValidatorSubsystem::ValidateStruct(const UObject* OwningObject, const UScriptStruct* ScriptStruct, const uint8* Data)
{
	if (!IsValid(OwningObject) || ScriptStruct == nullptr || Data == nullptr)
	{
		// count invalid objects as valid in property validation. 
		return FPropertyValidationResult{EDataValidationResult::Valid};
	}

	// package check happens inside WithContext call
	FPropertyValidationContext ValidationContext(this, OwningObject);
	ValidateContainerWithContext(Data, ScriptStruct, ValidationContext);
	
	return ValidationContext.MakeValidationResult();
}

FPropertyValidationResult UPropertyValidatorSubsystem::ValidateObjectProperty(const UObject* Object, const FProperty* Property) const
{
	if (!IsValid(Object) || Property == nullptr)
	{
		// count invalid objects or properties as valid in property validation. 
		return FPropertyValidationResult{EDataValidationResult::Valid};
	}
	// sanity check that property belongs to object class
	check(Object->IsA(Property->GetOwner<UClass>()));
	
	// explicitly check for object package
	FPropertyValidationContext ValidationContext(this, Object);
	if (!CanValidatePackage(Object->GetPackage()))
	{
		return FPropertyValidationResult{EDataValidationResult::Valid};
	}
	
	ValidatePropertyWithContext(reinterpret_cast<const uint8*>(Object), Property, ValidationContext);

	return ValidationContext.MakeValidationResult();
}

FPropertyValidationResult UPropertyValidatorSubsystem::ValidateStructProperty(const UObject* OwningObject, const UScriptStruct* ScriptStruct, FProperty* Property, const uint8* StructData)
{
	if (!IsValid(OwningObject) || ScriptStruct == nullptr || Property == nullptr || StructData == nullptr)
	{
		// count invalid objects or properties as valid in property validation. 
		return FPropertyValidationResult{EDataValidationResult::Valid};
	}

	// explicitly check for object package
	FPropertyValidationContext ValidationContext(this, OwningObject);
	if (!CanValidatePackage(OwningObject->GetPackage()))
	{
		return FPropertyValidationResult{EDataValidationResult::Valid};
	}

	ValidatePropertyWithContext(StructData, Property, ValidationContext);

	return ValidationContext.MakeValidationResult();
}

bool UPropertyValidatorSubsystem::CanValidatePackage(const UPackage* Package) const
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

void UPropertyValidatorSubsystem::ValidateContainerWithContext(TNonNullPtr<const uint8> ContainerMemory, const UStruct* Struct, FPropertyValidationContext& ValidationContext) const
{
	const bool bIsStruct = Cast<UScriptStruct>(Struct) != nullptr;
	const UPackage* Package = Struct->GetPackage();
	
	while (Struct && (bIsStruct || CanValidatePackage(Package)))
	{
		if (bSkipBlueprintGeneratedClasses && IsBlueprintGenerated(Package))
		{
			Struct = Struct->GetSuperStruct();
			continue;
		}

		// EFieldIterationFlags::None because we look only at given Struct valid properties
		for (TFieldIterator<FProperty> It(Struct, EFieldIterationFlags::None); It; ++It)
		{
			ValidatePropertyWithContext(ContainerMemory, *It, ValidationContext);
		}
		
		Struct = Struct->GetSuperStruct();
	}
}

void UPropertyValidatorSubsystem::ValidatePropertyWithContext(TNonNullPtr<const uint8> ContainerMemory, const FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	// check whether we should validate property at all
	if (!ShouldValidateProperty(Property, ValidationContext))
	{
		return;
	}

	TNonNullPtr<const uint8> PropertyMemory{Property->ContainerPtrToValuePtr<uint8>(ContainerMemory)};
	for (const UPropertyValidatorBase* Validator: PropertyValidators)
	{
		if (Validator->CanValidateProperty(Property))
		{
			Validator->ValidateProperty(PropertyMemory, Property, ValidationContext);
			break;
		}
	}
	
	for (const UPropertyValidatorBase* Validator: ContainerValidators)
	{
		if (Validator->CanValidateProperty(Property))
		{
			Validator->ValidateProperty(PropertyMemory, Property, ValidationContext);
			break;
		}
	}
}

void UPropertyValidatorSubsystem::ValidatePropertyValueWithContext(TNonNullPtr<const uint8> PropertyMemory, const FProperty* ParentProperty, const FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const
{
	// do not check for property flags for ParentProperty or ValueProperty.
	// ParentProperty has already been checked and ValueProperty is set by container so it doesn't have metas or required property flags
	
	for (const UPropertyValidatorBase* Validator: PropertyValidators)
	{
		if (Validator->CanValidateProperty(ValueProperty))
		{
			Validator->ValidateProperty(PropertyMemory, ValueProperty, ValidationContext);
			break;
		}
	}
	
	for (const UPropertyValidatorBase* Validator: ContainerValidators)
	{
		if (Validator->CanValidateProperty(ValueProperty))
		{
			Validator->ValidateProperty(PropertyMemory, ValueProperty, ValidationContext);
			break;
		}
	}
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

bool UPropertyValidatorSubsystem::IsBlueprintGenerated(const UPackage* Package) const
{
	const FString PackageName = Package->GetName();
	// package is blueprint generated if it is either in Content folder or Plugins/Content folder
	return PackageName.StartsWith(TEXT("/Game/")) || !PackageName.StartsWith(TEXT("/Script"));
}


