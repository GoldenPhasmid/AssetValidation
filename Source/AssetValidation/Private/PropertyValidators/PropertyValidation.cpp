#include "PropertyValidators/PropertyValidation.h"

#include "AssetValidationDefines.h"
#include "PropertyValidatorSubsystem.h"
#include "EditCondition/EditConditionContext.h"
#include "EditCondition/EditConditionParser.h"
#include "Editor/MetaDataSource.h"
#include "Editor/PropertyEditor/Private/EditConditionParser.h"
#include "Kismet2/BlueprintEditorUtils.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

template <uint32 NumElements>
struct TMetaKeysWrapper
{
	template <typename ...ArgTypes>
	TMetaKeysWrapper(ArgTypes&&... Args)
	{
		int32 Index = 0;
		((MetaKeys[Index] = Args, ++Index), ...);
	}
		
	TStaticArray<FName, NumElements> MetaKeys;
};

const TStaticArray<FName, 6>& UE::AssetValidation::GetMetaKeys()
{
	static TMetaKeysWrapper<6> Wrapper{Validate, ValidateKey, ValidateValue, ValidateRecursive, FailureMessage, DisableEditOnTemplate};
	return Wrapper.MetaKeys;
}

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
		return Func(MapProperty->KeyProp) || Func(MapProperty->ValueProp);
	}

	return Func(Property);
}

bool UE::AssetValidation::PassesEditCondition(UStruct* Struct, TNonNullPtr<const uint8> Container, const FProperty* Property)
{
	const FString EditConditionString = Property->GetMetaData(TEXT("EditCondition"));
	if (EditConditionString.IsEmpty())
	{
		return true;
	}

	static UE::AssetValidation::FEditConditionParser Parser;
	
	if (TSharedPtr<UE::AssetValidation::FEditConditionExpression> Expression = Parser.Parse(EditConditionString))
	{
		FEditConditionContext Context{Struct, Container, Property};
		if (auto Result = Parser.Evaluate(*Expression, Context); Result.HasValue())
		{
			return Result.GetValue();
		}
	}

	return false;
}

bool UE::AssetValidation::CheckPropertyMetaData(const FProperty* Property, const FMetaDataSource& MetaData, bool bLoggingEnabled)
{
	static TMap<const FProperty*, bool> CheckedProperties;
	if (const bool* Found = CheckedProperties.Find(Property))
	{
		return *Found;
	}
	
	const FString Pattern{TEXT("{0} : {1} property type does not support \"{2}\" meta specifier. Please update source code to fix incorrect meta usage.")};
	
	const FString CppPropertyName = GetNameSafe(Property->GetOwnerUObject()) + TEXT(".") + Property->GetNameCPP();
	const FString CppType = Property->GetCPPType();

	bool bPropertyValid = true;
	{
		const FName MetaName = UE::AssetValidation::Validate;
		// check "Validate" meta specifier
		const bool bUsedOnStructProperty = ApplyToNonContainerProperty(Property, [](const FProperty* Property) { return Property->IsA<FStructProperty>(); });
		// struct properties can have "Validate" meta all they want, even if struct value validator doesn't exist
		const bool bMetaAllowed = !MetaData.HasMetaData(MetaName) || bUsedOnStructProperty || CanApplyMeta_Validate(Property);
		UE_CLOG(!bMetaAllowed, LogAssetValidation, Error, TEXT("%s"), *FString::Format(*Pattern, {CppPropertyName, CppType, MetaName.ToString()}));
		bPropertyValid &= bMetaAllowed;
	}

	{
		const FName MetaName = UE::AssetValidation::ValidateKey;
		// check "ValidateKey" meta specifier
		const bool bMetaAllowed = !MetaData.HasMetaData(MetaName) || CanApplyMeta_ValidateKey(Property);
		UE_CLOG(!bMetaAllowed && bLoggingEnabled, LogAssetValidation, Error, TEXT("%s"), *FString::Format(*Pattern, {CppPropertyName, CppType, MetaName.ToString()}));
		bPropertyValid &= bMetaAllowed;
	}

	{
		const FName MetaName = UE::AssetValidation::ValidateValue;
		// check "ValidateValue" meta specifier
		const bool bMetaAllowed = !MetaData.HasMetaData(MetaName) || CanApplyMeta_ValidateValue(Property);
		UE_CLOG(!bMetaAllowed && bLoggingEnabled, LogAssetValidation, Error, TEXT("%s"), *FString::Format(*Pattern, {CppPropertyName, CppType, MetaName.ToString()}));
		bPropertyValid &= bMetaAllowed;
	}

	{
		const FName MetaName = UE::AssetValidation::ValidateRecursive;
		// check "ValidateRecursive" meta specifier
		const bool bMetaAllowed = !MetaData.HasMetaData(MetaName) || CanApplyMeta_ValidateRecursive(Property);
		UE_CLOG(!bMetaAllowed && bLoggingEnabled, LogAssetValidation, Error, TEXT("%s"), *FString::Format(*Pattern, {CppPropertyName, CppType, MetaName.ToString()}));
		bPropertyValid &= bMetaAllowed;
	}

	{
		const FName MetaName = UE::AssetValidation::FailureMessage;
		// check "FailureMessage" meta specifier
		const bool bMetaAllowed = !MetaData.HasMetaData(MetaName) || CanApplyMeta(Property, MetaName);
		UE_CLOG(!bMetaAllowed && bLoggingEnabled, LogAssetValidation, Error, TEXT("%s"), *FString::Format(*Pattern, {CppPropertyName, CppType, MetaName.ToString()}));
		bPropertyValid &= bMetaAllowed;
	}

	CheckedProperties.Add(Property, bPropertyValid);
	
	return bPropertyValid;
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
		// property is an object property, soft object property, or a struct property
		const FFieldClass* PropertyClass = Property->GetClass();
		return PropertyClass == FObjectProperty::StaticClass() || PropertyClass == FSoftObjectProperty::StaticClass() || Property->IsA<FStructProperty>();
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
		// @todo: replace with "has any valid meta specifier AND it can be applied"
		return CanApplyMeta_Validate(Property) || CanApplyMeta_ValidateKey(Property) || CanApplyMeta_ValidateValue(Property);
	}

	return true;
}

bool UE::AssetValidation::CanValidatePropertyValue(const FProperty* Property)
{
	return CanApplyMeta_Validate(Property) || CanApplyMeta_ValidateKey(Property) || CanApplyMeta_ValidateValue(Property);
}

bool UE::AssetValidation::CanValidatePropertyRecursively(const FProperty* Property)
{
	return CanApplyMeta_ValidateRecursive(Property);
}

bool UE::AssetValidation::IsBlueprintComponentProperty(const FProperty* Property)
{
	if (const FObjectProperty* ObjectProperty = CastField<FObjectProperty>(Property))
	{
		if (const UObject* PropertyOwner = Property->GetOwnerUObject(); PropertyOwner->IsA<UBlueprintGeneratedClass>())
		{
			return ObjectProperty->PropertyClass->IsChildOf<UActorComponent>();
		}
	}

	return false;
}

bool UE::AssetValidation::IsBlueprintVisibleProperty(const FProperty* Property)
{
	constexpr EPropertyFlags VisibleFlags = CPF_Edit | CPF_BlueprintVisible;
	return Property && Property->HasAnyPropertyFlags(VisibleFlags);
}

FString UE::AssetValidation::GetPropertyDisplayName(const FProperty* Property)
{
	check(Property);
	return Property->GetDisplayNameText().ToString();
}

FString UE::AssetValidation::GetPropertyTypeName(const FProperty* Property)
{
	check(Property);
	if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
	{
		return EnumProperty->GetEnum()->GetName();
	}
	if (const FByteProperty* ByteProperty = CastField<FByteProperty>(Property))
	{
		const UEnum* EnumType = ByteProperty->GetIntPropertyEnum();
		if (EnumType != nullptr)
		{
			return EnumType->GetName();
		}
	}

	return Property->GetCPPType();
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

FPropertyValidationContext::FPropertyValidationContext(const UPropertyValidatorSubsystem* OwningSubsystem, const UObject* InSourceObject)
	: Subsystem(OwningSubsystem)
{
	// obtain object's package. It can be either outermost package or external package (in case of external actors)
	const UPackage* Package = InSourceObject->GetPackage();
	check(Package);

	// construct outer chain until we meet a package
	TArray<const UObject*, TInlineAllocator<4>> Outers;
	Outers.Add(InSourceObject);
	for (UObject* Outer = InSourceObject->GetOuter(); Outer && !Outer->IsA<UPackage>() && Outer != Package; Outer = Outer->GetOuter())
	{
		Outers.Add(Outer);
	}

	// push first scoped object
	PushSource(InSourceObject);
	// push outer chain as a validation prefix
	// explicitly exclude last outer, as it is probably a context that user can understand (blueprint, map, etc.)
	for (int32 Index = Outers.Num() - 2; Index >= 0; --Index)
	{
		PushPrefix(GetBeautifiedName(Outers[Index]));
	}
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
	
	if (const FString* CustomMsg = Property->FindMetaData(UE::AssetValidation::FailureMessage); CustomMsg && !CustomMsg->IsEmpty())
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

FString FPropertyValidationContext::GetBeautifiedName(const UObject* Object) const
{
	if (const AActor* Actor = Cast<AActor>(Object))
	{
		// display actor label instead of actor name. In editor world it is impossible to find an actor by its name
		return Actor->GetActorNameOrLabel();
	}

	return Object->GetName();
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