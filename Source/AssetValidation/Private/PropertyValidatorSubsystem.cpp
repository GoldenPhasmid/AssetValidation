#include "PropertyValidatorSubsystem.h"

#include "PropertyValidationSettings.h"
#include "ContainerValidators/PropertyContainerValidator.h"
#include "Editor/MetaDataSource.h"
#include "Editor/ValidationEditorExtensionManager.h"
#include "PropertyValidators/PropertyValidatorBase.h"
#include "PropertyValidators/PropertyValidation.h"
#include "PropertyValidators/StructValidators.h"

#ifndef WITH_ASSET_VALIDATION_TESTS
#define WITH_ASSET_VALIDATION_TESTS 1
#endif

UPropertyValidatorSubsystem* UPropertyValidatorSubsystem::Get()
{
	return GEditor->GetEditorSubsystem<UPropertyValidatorSubsystem>();
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
	
	Collection.InitializeDependency<UAssetEditorSubsystem>();

	// cache property validator classes
	TArray<UClass*> ValidatorClasses;
	GetDerivedClasses(UPropertyValidatorBase::StaticClass(), ValidatorClasses, true);

	for (const UClass* ValidatorClass: ValidatorClasses)
	{
		// group validators by their base class to speed up property validation
		if (!ValidatorClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated))
		{
			UPropertyValidatorBase* Validator = NewObject<UPropertyValidatorBase>(GetTransientPackage(), ValidatorClass);
			if (Validator->IsA<UPropertyContainerValidator>())
			{
				ContainerValidators.Add(Validator->GetPropertyClass(), Validator);
			}
			else if (Validator->IsA<UStructValidator>())
			{
				StructValidators.Add(CastChecked<UStructValidator>(Validator)->GetCppType(), Validator);
			}
			else
			{
				PropertyValidators.Add(Validator->GetPropertyClass(), Validator);
			}
			AllValidators.Add(Validator);
		}
	}

	ExtensionManager = NewObject<UValidationEditorExtensionManager>(this);
	ExtensionManager->Initialize();
}

template <typename ...Types>
void LazyEmpty(Types&&... Vals)
{
	(Vals.Empty(), ...);
}

void UPropertyValidatorSubsystem::Deinitialize()
{
	LazyEmpty(PropertyValidators, ContainerValidators, StructValidators, AllValidators);

	ExtensionManager->Cleanup();
	ExtensionManager = nullptr;
	
	Super::Deinitialize();
}

FPropertyValidationResult UPropertyValidatorSubsystem::ValidateObject(const UObject* Object) const
{
	if (!IsValid(Object))
	{
		// count invalid objects as valid in property validation. 
		return FPropertyValidationResult{EDataValidationResult::Valid};
	}

	if (UClass* Class = Object->GetClass(); Class->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated))
	{
		// do not validate abstract or deprecated classes.
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

	if (UClass* Class = Object->GetClass(); Class->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated))
	{
		// do not validate abstract or deprecated classes.
		return FPropertyValidationResult{EDataValidationResult::Valid};
	}
	
	// sanity check that property belongs to object class
	check(Object->IsA(Property->GetOwner<UClass>()));

	
	// explicitly check for object package
	FPropertyValidationContext ValidationContext(this, Object);
	if (ShouldIgnorePackage(Object->GetPackage()))
	{
		return FPropertyValidationResult{EDataValidationResult::Valid};
	}

	TSharedRef MetaData = MakeShared<UE::AssetValidation::FMetaDataSource>(Property);
	ValidatePropertyWithContext(reinterpret_cast<const uint8*>(Object), Property, *MetaData, ValidationContext);

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
	if (ShouldIgnorePackage(OwningObject->GetPackage()))
	{
		return FPropertyValidationResult{EDataValidationResult::Valid};
	}

	TSharedRef MetaData = MakeShared<UE::AssetValidation::FMetaDataSource>(Property);
	ValidatePropertyWithContext(StructData, Property, *MetaData, ValidationContext);

	return ValidationContext.MakeValidationResult();
}

bool UPropertyValidatorSubsystem::ShouldIgnorePackage(const UPackage* Package) const
{
	const FString PackageName = Package->GetName();

	// allow validation for project package
	const FString ProjectPackage = FString::Printf(TEXT("/Script/%s"), FApp::GetProjectName());
	if (PackageName.StartsWith(ProjectPackage))
	{
		return false;
	}
	
#if WITH_ASSET_VALIDATION_TESTS
	if (PackageName.StartsWith("/Script/AssetValidation"))
	{
		return false;
	}
#endif

	auto Settings = UPropertyValidationSettings::Get();
	// package is blueprint generated if it is either in Content folder or Plugins/Content folder
	return Settings->PackagesToIgnore.ContainsByPredicate([PackageName](const FString& ModulePath)
	{
		return PackageName.StartsWith(ModulePath);
	});
}

bool UPropertyValidatorSubsystem::ShouldIteratePackageProperties(const UPackage* Package) const
{
	const FString PackageName = Package->GetName();
	// allow validation for project package
	const FString ProjectPackage = FString::Printf(TEXT("/Script/%s"), FApp::GetProjectName());
	if (PackageName.StartsWith(ProjectPackage))
	{
		return true;
	}
	
#if WITH_ASSET_VALIDATION_TESTS
	if (PackageName.StartsWith("/Script/AssetValidation"))
	{
		return true;
	}
#endif
	
	auto Settings = UPropertyValidationSettings::Get();
	return Settings->PackagesToIterate.ContainsByPredicate([PackageName](const FString& ModulePath)
	{
		return PackageName.StartsWith(ModulePath);
	});
}

bool UPropertyValidatorSubsystem::ShouldSkipPackage(const UPackage* Package) const
{
	auto Settings = UPropertyValidationSettings::Get();
	return Settings->bSkipBlueprintGeneratedClasses && UE::AssetValidation::IsBlueprintGeneratedPackage(Package->GetName());
}

bool UPropertyValidatorSubsystem::HasValidatorForPropertyType(const FProperty* PropertyType) const
{
	// attempt to find property validator for given property type
	if (FindPropertyValidator(PropertyType) != nullptr)
	{
		return true;
	}
	
	if (!UE::AssetValidation::IsContainerProperty(PropertyType))
	{
		// if we failed to find a property validator for a given property type, it is probably a struct and we can't validate the value.
		// Validate means "validate container data" for other "containers", while for "object" and "struct" container validate means the value itself
		return false;
	}
	
	// property type is a container
	if (FindContainerValidator(PropertyType) != nullptr)
	{
		return true;
	}

	return false;
}

void UPropertyValidatorSubsystem::ValidateContainerWithContext(TNonNullPtr<const uint8> ContainerMemory, const UStruct* Struct, FPropertyValidationContext& ValidationContext) const
{
	const bool bIsScriptStruct = Cast<UScriptStruct>(Struct) != nullptr;
	const UPackage* Package = Struct->GetPackage();
	
	while (Struct && (bIsScriptStruct || !ShouldIgnorePackage(Package)))
	{
		if (ShouldSkipPackage(Package))
		{
			Struct = Struct->GetSuperStruct();
			continue;
		}

		TSharedRef MetaData = MakeShared<UE::AssetValidation::FMetaDataSource>();
		if (ShouldIteratePackageProperties(Package))
		{
			// EFieldIterationFlags::None because we look only at Struct type properties
			for (FProperty* Property: TFieldRange<FProperty>(Struct, EFieldIterationFlags::None))
			{
				MetaData->SetProperty(Property);
				ValidatePropertyWithContext(ContainerMemory, Property, *MetaData, ValidationContext);
			}
		}
		
		for (const FPropertyExternalValidationData& ExternalData: UPropertyValidationSettings::GetExternalValidationData(Struct))
		{
			MetaData->SetExternalData(ExternalData);
			ValidatePropertyWithContext(ContainerMemory, ExternalData.GetProperty(), *MetaData, ValidationContext);
		}
		
		Struct = Struct->GetSuperStruct();
		if (Struct)
		{
			Package = Struct->GetPackage();
		}
	}
}

void UPropertyValidatorSubsystem::ValidatePropertyWithContext(TNonNullPtr<const uint8> ContainerMemory, const FProperty* Property, FMetaDataSource& MetaData, FPropertyValidationContext& ValidationContext) const
{
	// check whether we should validate property at all
	if (!ShouldValidateProperty(Property, ValidationContext))
	{
		return;
	}

	if (UPropertyValidationSettings::Get()->bReportIncorrectMetaUsage)
	{
		UE::AssetValidation::CheckPropertyMetaData(Property, MetaData, true);
	}
	
	TNonNullPtr<const uint8> PropertyMemory{Property->ContainerPtrToValuePtr<uint8>(ContainerMemory)};
	// validate property value
	if (auto PropertyValidator = FindPropertyValidator(Property))
	{
		if (PropertyValidator->CanValidateProperty(Property, MetaData))
		{
			PropertyValidator->ValidateProperty(PropertyMemory, Property, MetaData, ValidationContext);
		}
	}

	// validate property as a container
	if (auto ContainerValidator = FindContainerValidator(Property))
	{
		if (ContainerValidator->CanValidateProperty(Property, MetaData))
		{
			ContainerValidator->ValidateProperty(PropertyMemory, Property, MetaData, ValidationContext);
		}
	}
}

void UPropertyValidatorSubsystem::ValidatePropertyValueWithContext(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FMetaDataSource& MetaData, FPropertyValidationContext& ValidationContext) const
{
	// do not check for property flags for ParentProperty or ValueProperty.
	// ParentProperty has already been checked and ValueProperty is set by container so it doesn't have metas or required property flags

	if (auto Validator = FindPropertyValidator(Property))
	{
		if (Validator->CanValidateProperty(Property, MetaData))
		{
			Validator->ValidateProperty(PropertyMemory, Property, MetaData, ValidationContext);
		}
	}

	if (auto Validator = FindContainerValidator(Property))
	{
		if (Validator->CanValidateProperty(Property, MetaData))
		{
			Validator->ValidateProperty(PropertyMemory, Property, MetaData, ValidationContext);
		}
	}
}

bool UPropertyValidatorSubsystem::CanEverValidateProperty(const FProperty* Property) const
{
	if (Property->HasAnyPropertyFlags(EPropertyFlags::CPF_Deprecated | EPropertyFlags::CPF_Transient | EPropertyFlags::CPF_SkipSerialization))
	{
		return false;
	}

	// for some reason blueprint created components doesn't have CPF_Edit, only CPF_BlueprintVisible. So we don't require CPF_Edit to be on a component property
	// it is not required for component properties to be editable, as we want to validate their properties recursively
	return true;
}

bool UPropertyValidatorSubsystem::ShouldValidateProperty(const FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	if (!CanEverValidateProperty(Property))
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
	else if (const UObject* PropertyOwner = Property->GetOwnerUObject(); PropertyOwner->IsA<UBlueprintGeneratedClass>())
	{
		// blueprint created components doesn't have CPF_Edit property specifier, while cpp defined components have
		// we want to validate blueprint components as well, so we check for Owner to be a blueprint generated class
		// and property to be object property derived from actor component
		if (const FObjectProperty* ObjectProperty = CastField<FObjectProperty>(Property))
		{
			return ObjectProperty->PropertyClass->IsChildOf<UActorComponent>();
		}
	}

	return false;
}

const UPropertyValidatorBase* UPropertyValidatorSubsystem::FindPropertyValidator(const FProperty* PropertyType) const
{
	const UPropertyValidatorBase* PropertyValidator = nullptr;
	// find suitable validator for property value
	if (PropertyType->IsA<FStructProperty>())
	{
		const FString Key = PropertyType->GetCPPType();
		if (auto ValidatorPtr = StructValidators.Find(Key))
		{
			PropertyValidator = *ValidatorPtr;
		}
	}
	else if (const FByteProperty* ByteProperty = CastField<FByteProperty>(PropertyType))
	{
		// @todo:  This is ugly. Byte property can represent either byte or enum. We don't want byte but want enum.
		if (ByteProperty->IsEnum())
		{
			if (auto ValidatorPtr = PropertyValidators.Find(FByteProperty::StaticClass()))
			{
				PropertyValidator = *ValidatorPtr;
			}
		}
	}
	else
	{
		const FFieldClass* PropertyClass = PropertyType->GetClass();
		while (PropertyClass && !PropertyClass->HasAnyClassFlags(CLASS_Abstract))
		{
			if (auto ValidatorPtr = PropertyValidators.Find(PropertyClass))
			{
				PropertyValidator = *ValidatorPtr;
				break;
			}
			else
			{
				PropertyClass = PropertyClass->GetSuperClass();
			}
		}
	}

	return PropertyValidator;
}

const UPropertyValidatorBase* UPropertyValidatorSubsystem::FindContainerValidator(const FProperty* PropertyType) const
{
	const FFieldClass* PropertyClass = PropertyType->GetClass();
	while (PropertyClass && !PropertyClass->HasAnyClassFlags(CLASS_Abstract))
	{
		if (auto ValidatorPtr = ContainerValidators.Find(PropertyClass))
		{
			return *ValidatorPtr;
		}
		else
		{
			PropertyClass = PropertyClass->GetSuperClass();
		}
	}

	return nullptr;
}

#undef WITH_ASSET_VALIDATION_TESTS
