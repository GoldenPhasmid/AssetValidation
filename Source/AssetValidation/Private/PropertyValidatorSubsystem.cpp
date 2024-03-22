#include "PropertyValidatorSubsystem.h"

#include "BlueprintEditorModule.h"
#include "PropertyValidationSettings.h"
#include "PropertyValidationVariableDetailCustomization.h"
#include "SubobjectDataSubsystem.h"
#include "ContainerValidators/PropertyContainerValidator.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "PropertyValidators/PropertyValidatorBase.h"
#include "PropertyValidators/PropertyValidation.h"
#include "PropertyValidators/StructValidators.h"

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

	// cache property validator classes
	TArray<UClass*> ValidatorClasses;
	GetDerivedClasses(UPropertyValidatorBase::StaticClass(), ValidatorClasses, true);

	for (const UClass* ValidatorClass: ValidatorClasses)
	{
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

	// Register bp variable customization
	FBlueprintEditorModule& BlueprintEditorModule = FModuleManager::LoadModuleChecked<FBlueprintEditorModule>("Kismet");
	VariableCustomizationHandle = BlueprintEditorModule.RegisterVariableCustomization(FProperty::StaticClass(), FOnGetVariableCustomizationInstance::CreateStatic(&FPropertyValidationVariableDetailCustomization::MakeInstance));

	FCoreUObjectDelegates::OnObjectModified.AddUObject(this, &ThisClass::PreBlueprintChange);
	
	USubobjectDataSubsystem* SubobjectSubsystem = GEngine->GetEngineSubsystem<USubobjectDataSubsystem>();
	SubobjectSubsystem->OnNewSubobjectAdded().AddUObject(this, &ThisClass::HandleBlueprintComponentAdded);
}

template <typename ...Types>
void LazyEmpty(Types&&... Vals)
{
	(Vals.Empty(), ...);
}

void UPropertyValidatorSubsystem::Deinitialize()
{
	LazyEmpty(PropertyValidators, ContainerValidators, StructValidators, AllValidators);
	
	// Unregister bp variable customization
	FBlueprintEditorModule& BlueprintEditorModule = FModuleManager::LoadModuleChecked<FBlueprintEditorModule>("Kismet");
	BlueprintEditorModule.UnregisterVariableCustomization(FProperty::StaticClass(), VariableCustomizationHandle);

	if (!IsEngineExitRequested())
	{
		if (USubobjectDataSubsystem* SubobjectSubsystem = GEngine->GetEngineSubsystem<USubobjectDataSubsystem>())
		{
			SubobjectSubsystem->OnNewSubobjectAdded().RemoveAll(this);
		}
	}
	
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

	return UPropertyValidationSettings::CanValidatePackage(PackageName);
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
	const bool bIsStruct = Cast<UScriptStruct>(Struct) != nullptr;
	const UPackage* Package = Struct->GetPackage();
	
	while (Struct && (bIsStruct || CanValidatePackage(Package)))
	{
		if (UPropertyValidationSettings::Get()->bSkipBlueprintGeneratedClasses && IsBlueprintGenerated(Package))
		{
			Struct = Struct->GetSuperStruct();
			continue;
		}

		// EFieldIterationFlags::None because we look only at Struct type properties
		for (const FProperty* Property: TFieldRange<FProperty>(Struct, EFieldIterationFlags::None))
		{
			ValidatePropertyWithContext(ContainerMemory, Property, ValidationContext);
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
	// validate property value
	if (auto PropertyValidator = FindPropertyValidator(Property))
	{
		if (PropertyValidator->CanValidateProperty(Property))
		{
			PropertyValidator->ValidateProperty(PropertyMemory, Property, ValidationContext);
		}
	}

	// validate property as a container
	if (auto ContainerValidator = FindContainerValidator(Property))
	{
		if (ContainerValidator->CanValidateProperty(Property))
		{
			ContainerValidator->ValidateProperty(PropertyMemory, Property, ValidationContext);
		}
	}
}

void UPropertyValidatorSubsystem::ValidatePropertyValueWithContext(TNonNullPtr<const uint8> PropertyMemory, const FProperty* ParentProperty, const FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const
{
	// do not check for property flags for ParentProperty or ValueProperty.
	// ParentProperty has already been checked and ValueProperty is set by container so it doesn't have metas or required property flags

	if (auto Validator = FindPropertyValidator(ValueProperty))
	{
		if (Validator->CanValidateProperty(ValueProperty))
		{
			Validator->ValidateProperty(PropertyMemory, ValueProperty, ValidationContext);
		}
	}

	if (auto Validator = FindContainerValidator(ValueProperty))
	{
		if (Validator->CanValidateProperty(ValueProperty))
		{
			Validator->ValidateProperty(PropertyMemory, ValueProperty, ValidationContext);
		}
	}
}

bool UPropertyValidatorSubsystem::CanEverValidateProperty(const FProperty* Property) const
{
	if (Property->HasAnyPropertyFlags(EPropertyFlags::CPF_Deprecated | EPropertyFlags::CPF_Transient | EPropertyFlags::CPF_SkipSerialization))
	{
		return false;
	}

	return Property->HasAnyPropertyFlags(EPropertyFlags::CPF_Edit);
}

bool UPropertyValidatorSubsystem::ShouldValidateProperty(const FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	if (CanEverValidateProperty(Property))
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
	else
	{
		const FFieldClass* Key = PropertyType->GetClass();
		while (Key && !Key->HasAnyClassFlags(CLASS_Abstract))
		{
			if (auto ValidatorPtr = PropertyValidators.Find(Key))
			{
				PropertyValidator = *ValidatorPtr;
				break;
			}
			else
			{
				Key = Key->GetSuperClass();
			}
		}
	}

	return PropertyValidator;
}

const UPropertyValidatorBase* UPropertyValidatorSubsystem::FindContainerValidator(const FProperty* PropertyType) const
{
	if (auto ValidatorPtr = ContainerValidators.Find(PropertyType->GetClass()))
	{
		return *ValidatorPtr;
	}

	return nullptr;
}

void UPropertyValidatorSubsystem::PreBlueprintChange(UObject* ModifiedObject)
{
	if (UBlueprint* Blueprint = Cast<UBlueprint>(ModifiedObject))
	{
		if (!CachedBlueprints.Contains(Blueprint))
		{
			FBlueprintVariableData BlueprintData{Blueprint};
			BlueprintData.OnChangedHandle = Blueprint->OnChanged().AddUObject(this, &ThisClass::PostBlueprintChange);
			CachedBlueprints.Add(MoveTemp(BlueprintData));
		}
	}
}

void UPropertyValidatorSubsystem::PostBlueprintChange(UBlueprint* Blueprint)
{
	const int32 Index = CachedBlueprints.IndexOfByKey(Blueprint);
	check(Index != INDEX_NONE);

	FBlueprintVariableData OldData = CachedBlueprints[Index];
	// remove cached data
	CachedBlueprints.RemoveAt(Index);
	Blueprint->OnChanged().Remove(OldData.OnChangedHandle);
	
	static bool bUpdatingVariables = false;
	TGuardValue UpdateVariablesGuard{bUpdatingVariables, true};

	TMap<FGuid, int32> NewVariables;
	for (int32 VarIndex = 0; VarIndex < Blueprint->NewVariables.Num(); ++VarIndex)
	{
		NewVariables.Add(Blueprint->NewVariables[VarIndex].VarGuid, VarIndex);
	}

	TMap<FGuid, int32> OldVariables;
	for (int32 VarIndex = 0; VarIndex < OldData.Variables.Num(); ++VarIndex)
	{
		OldVariables.Add(OldData.Variables[VarIndex].VarGuid, VarIndex);
	}

	for (const FBPVariableDescription& OldVariable: OldData.Variables)
	{
		if (!NewVariables.Contains(OldVariable.VarGuid))
		{
			HandleVariableRemoved(Blueprint, OldVariable.VarName);
		}
	}

	for (const FBPVariableDescription& NewVariable: Blueprint->NewVariables)
	{
		if (!OldVariables.Contains(NewVariable.VarGuid))
		{
			HandleVariableAdded(Blueprint, NewVariable.VarName);
			continue;
		}

		const int32 OldVarIndex = OldVariables.FindChecked(NewVariable.VarGuid);
		const FBPVariableDescription& OldVariable = OldData.Variables[OldVarIndex];
		if (OldVariable.VarName != NewVariable.VarName)
		{
			HandleVariableRenamed(Blueprint, OldVariable.VarName, NewVariable.VarName);
		}
		if (OldVariable.VarType != NewVariable.VarType)
		{
			HandleVariableTypeChanged(Blueprint, NewVariable.VarName, OldVariable.VarType, NewVariable.VarType);
		}
	}
}

void UPropertyValidatorSubsystem::HandleVariableAdded(UBlueprint* Blueprint, const FName& VarName)
{
	if (!UPropertyValidationSettings::Get()->bAddMetaToNewBlueprintVariables)
	{
		return;
	}

	UpdateBlueprintVariableMetaData(Blueprint, VarName, true);
}

void UPropertyValidatorSubsystem::HandleVariableTypeChanged(UBlueprint* Blueprint, const FName& VarName, FEdGraphPinType OldPinType, FEdGraphPinType NewPinType)
{
	UpdateBlueprintVariableMetaData(Blueprint, VarName, false);
}

void UPropertyValidatorSubsystem::UpdateBlueprintVariableMetaData(UBlueprint* Blueprint, const FName& VarName, bool bAddIfPossible)
{
	const FProperty* VarProperty = FindFProperty<FProperty>(Blueprint->SkeletonGeneratedClass, VarName);
	check(VarProperty);

	using namespace UE::AssetValidation;
	bool bHasFailureMessage = UpdateBlueprintVarMetaData(Blueprint, VarProperty, VarName, Validate, bAddIfPossible);
	bHasFailureMessage |= UpdateBlueprintVarMetaData(Blueprint, VarProperty, VarName, ValidateKey, bAddIfPossible);
	bHasFailureMessage |= UpdateBlueprintVarMetaData(Blueprint, VarProperty, VarName, ValidateValue, bAddIfPossible);
	if (bHasFailureMessage)
	{
		UpdateBlueprintVarMetaData(Blueprint, VarProperty, VarName, FailureMessage, bAddIfPossible);
	}
	UpdateBlueprintVarMetaData(Blueprint, VarProperty, VarName, ValidateRecursive, bAddIfPossible);
}

#if 0
void UPropertyValidatorSubsystem::HandleObjectModified(UObject* ModifiedObject) const
{
	if (const USCS_Node* SCSNode = Cast<USCS_Node>(ModifiedObject))
	{
		if (UBlueprintGeneratedClass* BlueprintClass = SCSNode->GetTypedOuter<UBlueprintGeneratedClass>())
		{
			if (UBlueprint* Blueprint = Cast<UBlueprint>(BlueprintClass->ClassGeneratedBy))
			{
				
			}
		}
	}
}
#endif

void UPropertyValidatorSubsystem::HandleBlueprintComponentAdded(const FSubobjectData& NewSubobjectData)
{
	if (!UPropertyValidationSettings::Get()->bAddMetaToNewBlueprintComponents)
	{
		// component automatic validation is disabled
		return;
	}

	if (NewSubobjectData.IsComponent() && !NewSubobjectData.IsInheritedComponent())
	{
		if (UBlueprint* Blueprint = NewSubobjectData.GetBlueprint())
		{
			const FName VariableName = NewSubobjectData.GetVariableName();

			const FString ComponentNotValid = FString::Printf(TEXT("Corrupted component property of name %s"), *VariableName.ToString());
			FBlueprintEditorUtils::SetBlueprintVariableMetaData(Blueprint, VariableName, nullptr, UE::AssetValidation::Validate, {});
			FBlueprintEditorUtils::SetBlueprintVariableMetaData(Blueprint, VariableName, nullptr, UE::AssetValidation::ValidateRecursive, {});
			FBlueprintEditorUtils::SetBlueprintVariableMetaData(Blueprint, VariableName, nullptr, UE::AssetValidation::FailureMessage, ComponentNotValid);

			if (const UClass* SkeletonClass = Blueprint->SkeletonGeneratedClass)
			{
				const FProperty* ComponentProperty = FindFProperty<FProperty>(SkeletonClass, VariableName);
				check(ComponentProperty);
			}
		}
	}
}



