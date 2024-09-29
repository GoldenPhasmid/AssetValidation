#include "PropertyValidatorSubsystem.h"

#include "PropertyValidationSettings.h"
#include "ContainerValidators/ContainerValidator.h"
#include "Editor/MetaDataSource.h"
#include "Editor/ValidationEditorExtensionManager.h"
#include "PropertyValidators/PropertyValidatorBase.h"
#include "PropertyValidators/PropertyValidation.h"

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

			auto& MapContainer = Validator->IsA<UContainerValidator>() ? ContainerValidators : PropertyValidators;

			if (auto Descriptor = Validator->GetDescriptor();
				ensureAlwaysMsgf(Descriptor.IsValid(), TEXT("%s: validator %s has uninitialized descriptor."), *FString(__FUNCTION__), *GetNameSafe(Validator)))
			{
				MapContainer.Add(Descriptor, Validator);
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
	LazyEmpty(PropertyValidators, ContainerValidators, AllValidators);
	
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

	TRACE_CPUPROFILER_EVENT_SCOPE(UPropertyValidatorSubsystem::ValidateObject);

	// package check happens inside WithContext call
	FPropertyValidationContext ValidationContext(this, Object);
	ValidateContainerWithContext(reinterpret_cast<const uint8*>(Object), Object->GetClass(), ValidationContext);

	return ValidationContext.MakeValidationResult();
}

FPropertyValidationResult UPropertyValidatorSubsystem::ValidateStruct(const UObject* OwningObject, const UScriptStruct* ScriptStruct, const uint8* StructData)
{
	if (!IsValid(OwningObject) || ScriptStruct == nullptr || StructData == nullptr)
	{
		// count invalid objects as valid in property validation. 
		return FPropertyValidationResult{EDataValidationResult::Valid};
	}
	
	TRACE_CPUPROFILER_EVENT_SCOPE(UPropertyValidatorSubsystem::ValidateStruct);

	// package check happens inside WithContext call
	FPropertyValidationContext ValidationContext(this, OwningObject);
	ValidateContainerWithContext(StructData, ScriptStruct, ValidationContext);
	
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
	TRACE_CPUPROFILER_EVENT_SCOPE(UPropertyValidatorSubsystem::ValidateObjectProperty);
	
	// explicitly check for object package
	FPropertyValidationContext ValidationContext(this, Object);
	if (ShouldIgnorePackage(Object->GetPackage()))
	{
		return FPropertyValidationResult{EDataValidationResult::Valid};
	}

	UE::AssetValidation::FMetaDataSource MetaData{Property};
	ValidatePropertyWithContext(reinterpret_cast<const uint8*>(Object), Property, MetaData, ValidationContext);

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

	UE::AssetValidation::FMetaDataSource MetaData{Property};
	ValidatePropertyWithContext(StructData, Property, MetaData, ValidationContext);

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
	if (PackageName.StartsWith("/Script/AssetValidation") || PackageName.StartsWith("/AssetValidation"))
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
	if (PackageName.StartsWith("/Script/AssetValidation") || PackageName.StartsWith("/AssetValidation"))
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
		
		if (ShouldIteratePackageProperties(Package))
		{
			// EFieldIterationFlags::None because we look only at this Struct properties
			for (FProperty* Property: TFieldRange<FProperty>(Struct, EFieldIterationFlags::None))
			{
				UE::AssetValidation::FMetaDataSource MetaData{Property};
				ValidatePropertyWithContext(ContainerMemory, Property, MetaData, ValidationContext);
			}
		}

		// query property extensions for current Struct and run validation on them
		for (const FEnginePropertyExtension& Extension: UPropertyValidationSettings::GetExtensions(Struct))
		{
			UE::AssetValidation::FMetaDataSource MetaData{Extension};
			ValidatePropertyWithContext(ContainerMemory, Extension.GetProperty(), MetaData, ValidationContext);
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
	if (UPropertyValidationSettings::Get()->bReportIncorrectMetaUsage)
	{
		// check whether metadata is valid
		UE::AssetValidation::CheckPropertyMetaData(Property, MetaData, true);
	}
	
	// check whether we should validate property at all
	if (!ShouldValidateProperty(Property, MetaData, ValidationContext))
	{
		return;
	}
	
	UStruct* Struct = Property->GetOwnerStruct();
	// @todo: move to ShouldValidateProperty
	if (UE::AssetValidation::PassesEditCondition(Struct, ContainerMemory, Property) == false)
	{
		return;
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
	if (Property->HasAnyPropertyFlags(EPropertyFlags::CPF_Deprecated | EPropertyFlags::CPF_SkipSerialization))
	{
		return false;
	}

	// for some reason blueprint created components doesn't have CPF_Edit, only CPF_BlueprintVisible. So we don't require CPF_Edit to be on a component property
	// it is not required for component properties to be editable, as we want to validate their properties recursively
	// Also, engine properties can have Transient and we still want to validate them, like UUserWidget::WidgetTree
	return true;
}

bool UPropertyValidatorSubsystem::ShouldValidateProperty(const FProperty* Property, UE::AssetValidation::FMetaDataSource& MetaData, FPropertyValidationContext& ValidationContext) const
{
	if (!CanEverValidateProperty(Property))
	{
		return false;
	}
	
	bool bTransient = MetaData.IsType<FProperty>() && Property->HasAnyPropertyFlags(EPropertyFlags::CPF_Transient);
	if (bTransient)
	{
		// do not validate transient properties, unless it is property extension
		return false;
	}

	if (UPropertyValidationSettings::Get()->bRequirePropertyBlueprintVisibility)
	{
		bool bVisibleInBlueprint = false;
		// engine property extension ignore visibility requirements
		bVisibleInBlueprint |= MetaData.IsType<FEnginePropertyExtension>();
		// property is editable in blueprints
		bVisibleInBlueprint |= Property->HasAnyPropertyFlags(EPropertyFlags::CPF_Edit) && !Property->HasAnyPropertyFlags(EPropertyFlags::CPF_EditConst);
		// blueprint created components doesn't have CPF_Edit property specifier, while cpp defined components have
		// we want to validate blueprint components as well, so we check for Owner to be a blueprint generated class
		// and property to be object property derived from actor component
		bVisibleInBlueprint |= UE::AssetValidation::IsBlueprintComponentProperty(Property);
		if (!bVisibleInBlueprint)
		{
			return false;
		}
	}

	const UObject* SourceObject = ValidationContext.GetSourceObject();
	const bool bAsset = UE::AssetValidation::IsAssetOrAssetFragment(SourceObject);
	
	// assets ignore EditDefaultsOnly and EditInstanceOnly specifics
	if (bAsset == true)
	{
		return true;
	}
	
	constexpr EObjectFlags TemplateFlags = RF_ArchetypeObject | RF_ClassDefaultObject;
	// don't use IsTemplate, as it checks outer chain as well
	// we're only interested whether this object is template or not
	const bool bTemplate = SourceObject->HasAnyFlags(TemplateFlags);
		
	// user can disable property validation on template
	// @todo: sometimes template related-checks will skip editable properties, because engine is inconsistent with its template flag
	if (MetaData.IsType<FEnginePropertyExtension>() && MetaData.HasMetaData(UE::AssetValidation::DisableEditOnTemplate) && bTemplate)
	{
		return false;
	}
		
	// EditDefaultsOnly property for instance object (not template and not asset)
	if (Property->HasAnyPropertyFlags(EPropertyFlags::CPF_DisableEditOnInstance) && !bTemplate)
	{
		return false;
	}
		
	// EditInstanceOnly property for template object
	if (Property->HasAnyPropertyFlags(EPropertyFlags::CPF_DisableEditOnTemplate) && bTemplate)
	{
		return false;
	}

	return true;
}

const UPropertyValidatorBase* UPropertyValidatorSubsystem::FindPropertyValidator(const FProperty* PropertyType) const
{
	return FindValidator(PropertyValidators, PropertyType);
}

const UPropertyValidatorBase* UPropertyValidatorSubsystem::FindContainerValidator(const FProperty* PropertyType) const
{
	return FindValidator(ContainerValidators, PropertyType);
}

const UPropertyValidatorBase* UPropertyValidatorSubsystem::FindValidator(const TMap<FPropertyValidatorDescriptor, UPropertyValidatorBase*>& Container, const FProperty* PropertyType)
{
	check(!Container.IsEmpty()); // we don't expect validator container to be empty, so this is probably a bug
	
	const FFieldClass* PropertyClass = PropertyType->GetClass();
	// find suitable validator for property value
	if (PropertyType->IsA<FStructProperty>())
	{
		FPropertyValidatorDescriptor Descriptor{PropertyType->GetClass(), FName{PropertyType->GetCPPType()}};
		if (auto ValidatorPtr = Container.Find(Descriptor))
		{
			return *ValidatorPtr;
		}
	}

	while (PropertyClass && !PropertyClass->HasAnyClassFlags(CLASS_Abstract))
	{
		FPropertyValidatorDescriptor Descriptor{PropertyClass};
		if (auto ValidatorPtr = Container.Find(Descriptor))
		{
			return *ValidatorPtr;
		}
		
		PropertyClass = PropertyClass->GetSuperClass();
	}

	return nullptr;
}
