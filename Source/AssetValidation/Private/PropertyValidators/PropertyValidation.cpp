#include "PropertyValidators/PropertyValidation.h"

#include "PropertyValidationSettings.h"
#include "PropertyValidatorSubsystem.h"
#include "Editor/MetaDataContainer.h"
#include "Kismet2/BlueprintEditorUtils.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

bool UE::AssetValidation::IsContainerProperty(const FProperty* Property)
{
	return Property != nullptr && (Property->IsA<FArrayProperty>() || Property->IsA<FMapProperty>() || Property->IsA<FSetProperty>());
}

bool UE::AssetValidation::IsBlueprintGeneratedPackage(const FString& PackageName)
{
	// package is blueprint generated if it is either in Content folder or Plugins/Content folder
	return PackageName.StartsWith(TEXT("/Game/")) || !PackageName.StartsWith(TEXT("/Script"));
}

template <typename TPropPred>
bool ApplyToNonContainerProperty(const FProperty* Property, TPropPred&& Func)
{
	// if property is a container property, predicate is applied to its inner property/properties
	if (const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
	{
		return Func(ArrayProperty->Inner);
	}
	if (const FSetProperty* SetProperty = CastField<FSetProperty>(Property))
	{
		return Func(SetProperty->ElementProp);
	}
	if (const FMapProperty* MapProperty = CastField<FMapProperty>(Property))
	{
		return Func(MapProperty->KeyProp) && Func(MapProperty->ValueProp);
	}

	return Func(Property);
}

bool UE::AssetValidation::CanApplyMeta_Validate(const FProperty* Property)
{
	// check if validator exists for a property, unwrapping container property types
	return ApplyToNonContainerProperty(Property, [](const FProperty* Property)
	{
		if (const UPropertyValidatorSubsystem* ValidatorSubsystem = UPropertyValidatorSubsystem::Get())
		{
			return ValidatorSubsystem->HasValidatorForPropertyType(Property);
		}
		
		return false;
	});
}

bool UE::AssetValidation::CanApplyMeta_ValidateRecursive(const FProperty* Property)
{
	// check if property is an object property (or struct property without auto validation), unwrapping container property types
	return ApplyToNonContainerProperty(Property, [](const FProperty* Property)
	{
		// property is an object property, soft object property, or auto unwrapping for struct properties is disabled
		const FFieldClass* PropertyClass = Property->GetClass();
		return PropertyClass == FObjectProperty::StaticClass() || PropertyClass == FSoftObjectProperty::StaticClass() || (!UPropertyValidationSettings::Get()->bAutoValidateStructInnerProperties && Property->IsA<FStructProperty>());
	});
}

bool UE::AssetValidation::CanApplyMeta_ValidateKey(const FProperty* Property)
{
	// "ValidateKey" can be set for map properties if key property has validator
	if (const FMapProperty* MapProperty = CastField<FMapProperty>(Property))
	{
		if (const UPropertyValidatorSubsystem* ValidatorSubsystem = UPropertyValidatorSubsystem::Get())
		{
			return ValidatorSubsystem->HasValidatorForPropertyType(MapProperty->KeyProp);
		}
	}

	return false;
}

bool UE::AssetValidation::CanApplyMeta_ValidateValue(const FProperty* Property)
{
	// "ValidateValue" can be set for map properties if value property has validator
	if (const FMapProperty* MapProperty = CastField<FMapProperty>(Property))
	{
		if (const UPropertyValidatorSubsystem* ValidatorSubsystem = UPropertyValidatorSubsystem::Get())
		{
			return ValidatorSubsystem->HasValidatorForPropertyType(MapProperty->ValueProp);
		}
	}

	return false;
}

bool UE::AssetValidation::CanApplyMeta(const FProperty* Property, const FName& MetaName)
{
	// ugh, it should have been a switch statement
	if (MetaName == UE::AssetValidation::Validate)
	{
		return CanApplyMeta_Validate(Property);
	}
	else if (MetaName == UE::AssetValidation::ValidateRecursive)
	{
		return CanApplyMeta_ValidateRecursive(Property);
	}
	else if (MetaName == UE::AssetValidation::ValidateKey)
	{
		return CanApplyMeta_ValidateKey(Property);
	}
	else if (MetaName == UE::AssetValidation::ValidateValue)
	{
		return CanApplyMeta_ValidateValue(Property);
	}
	else if (MetaName == UE::AssetValidation::FailureMessage)
	{
		return CanApplyMeta_Validate(Property) || CanApplyMeta_ValidateKey(Property) || CanApplyMeta_ValidateValue(Property);
	}
	else
	{
		checkNoEntry();
	}

	return false;
}

bool UE::AssetValidation::UpdateBlueprintVarMetaData(UBlueprint* Blueprint, const FProperty* Property, const FName& VarName, const FName& MetaName, bool bAddIfPossible)
{
	FString OutValue{};
	const bool bHasMetaData = FBlueprintEditorUtils::GetBlueprintVariableMetaData(Blueprint, VarName, nullptr, MetaName, OutValue);

	if (CanApplyMeta(Property, MetaName))
	{
		if ((bAddIfPossible && !bHasMetaData) || bHasMetaData)
		{
			FBlueprintEditorUtils::SetBlueprintVariableMetaData(Blueprint, VarName, nullptr, MetaName, {});
		}
		return true;
	}
	else if (bHasMetaData)
	{
		FBlueprintEditorUtils::RemoveBlueprintVariableMetaData(Blueprint, VarName, nullptr, MetaName);
	}

	return false;
}

FPropertyValidationResult FPropertyValidationContext::MakeValidationResult() const
{
	FPropertyValidationResult Result;
	Result.Errors.Reserve(Issues.Num());
	Result.Warnings.Reserve(Issues.Num());
	
	for (const FIssue& Issue: Issues)
	{
		switch (Issue.Severity)
		{
		case EMessageSeverity::Error:
			Result.Errors.Add(Issue.Message);
			break;
		case EMessageSeverity::Warning:
			Result.Warnings.Add(Issue.Message);
		default:
			checkNoEntry();
		}
	}
	Result.ValidationResult = Result.Errors.IsEmpty() ? EDataValidationResult::Valid : EDataValidationResult::Invalid;
	return Result;
}

void FPropertyValidationContext::PropertyFails(const FProperty* Property, const FText& DefaultFailureMessage)
{
	FProperty* OwnerProperty = Property->GetOwner<FProperty>();
	const bool bContainerProperty = UE::AssetValidation::IsContainerProperty(OwnerProperty);

	FIssue Issue;
	Issue.IssueProperty = bContainerProperty ? OwnerProperty : Property;
	Issue.Severity = EMessageSeverity::Error;

	FText PropertyPrefix = FText::GetEmpty();
	if (!bContainerProperty)
	{
		PropertyPrefix = Property->GetDisplayNameText();
	}
	
	if (const FString* CustomMsg = Property->FindMetaData(UE::AssetValidation::FailureMessage))
	{
		Issue.Message = MakeFullMessage(FText::FromString(*CustomMsg), PropertyPrefix);
	}
	else
	{
		Issue.Message = MakeFullMessage(DefaultFailureMessage, PropertyPrefix);
	}

	Issues.Add(Issue);
}

void FPropertyValidationContext::IsPropertyContainerValid(TNonNullPtr<const uint8> ContainerMemory, const UStruct* Struct)
{
	Subsystem->ValidateContainerWithContext(ContainerMemory, Struct, *this);
}

void FPropertyValidationContext::IsPropertyValid(TNonNullPtr<const uint8> ContainerMemory, const FProperty* Property, UE::AssetValidation::FMetaDataSource& MetaData)
{
	Subsystem->ValidatePropertyWithContext(ContainerMemory, Property, MetaData, *this);
}

void FPropertyValidationContext::IsPropertyValueValid(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, UE::AssetValidation::FMetaDataSource& MetaData)
{
	Subsystem->ValidatePropertyValueWithContext(PropertyMemory, Property, MetaData, *this);
}

FText FPropertyValidationContext::MakeFullMessage(const FText& FailureMessage, const FText& PropertyPrefix) const
{
	FString CorrectContext = ContextString;
	if (PropertyPrefix.IsEmpty())
	{
		// remove last dot
		CorrectContext = CorrectContext.LeftChop(1);
	}
	else
	{
		// append property prefix
		CorrectContext += PropertyPrefix.ToString();
	}
	
	FFormatNamedArguments NamedArguments;
	NamedArguments.Add(TEXT("Context"), FText::FromString(CorrectContext));
	NamedArguments.Add(TEXT("FailureMessage"), FailureMessage);

	return FText::Format(FTextFormat(LOCTEXT("AssetValidation_Message", "{Context}: {FailureMessage}")), NamedArguments);
}

#undef LOCTEXT_NAMESPACE