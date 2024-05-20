#include "EditConditionContext.h"
#include "EditConditionParser.h"

namespace UE::AssetValidation
{

FEditConditionContext::FEditConditionContext(UStruct* Struct, TNonNullPtr<const uint8> InContainer, const FProperty* InSourceProperty)
	: Container(InContainer)
	, SourceStruct(Struct)
	, SourceProperty(const_cast<FProperty*>(InSourceProperty))
{
	
}

FName FEditConditionContext::GetContextName() const
{
	return IsValid() ? SourceStruct->GetFName() : NAME_None;
}
	
const TWeakObjectPtr<UFunction> FEditConditionContext::GetFunction(const FString& FieldName) const
{
	if (!IsValid())
	{
		return nullptr;
	}

	const FProperty* Property = SourceProperty.Get();
	if (Property == nullptr)
	{
		return nullptr;
	}

	TWeakObjectPtr<UFunction> Function = FindUField<UFunction>(Property->GetOwnerStruct(), *FieldName);

	if (Function == nullptr && FieldName.Contains(TEXT(".")))
	{
		// Function not found in struct, try to see if this is a static function
		Function = FindObject<UFunction>(nullptr, *FieldName, true);

		if (Function.IsValid() && !Function->HasAnyFunctionFlags(EFunctionFlags::FUNC_Static))
		{
			Function = nullptr;
		}
	}

	return Function;
}
	
TOptional<bool> FEditConditionContext::GetBoolValue(const FString& PropertyName, TWeakObjectPtr<UFunction> CachedFunction) const
{
	if (!IsValid() || CachedFunction.IsValid())
	{
		return {};
	}
	
	const FProperty* Property = SourceStruct->FindPropertyByName(FName{PropertyName});
	if (const FBoolProperty* BoolProperty = CastField<FBoolProperty>(Property))
	{
		return BoolProperty->GetPropertyValue(BoolProperty->ContainerPtrToValuePtr<void>(Container));
	}
	return {};
}

TOptional<int64> FEditConditionContext::GetIntegerValue(const FString& PropertyName, TWeakObjectPtr<UFunction> CachedFunction) const
{
	if (!IsValid() || CachedFunction.IsValid())
	{
		return {};
	}

	const FProperty* Property = SourceStruct->FindPropertyByName(FName{PropertyName});
	const FNumericProperty* NumericProperty = CastField<FNumericProperty>(Property);
	if (NumericProperty == nullptr)
	{
		// Retry with an enum and its underlying property
		if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
		{
			NumericProperty = EnumProperty->GetUnderlyingProperty();
		}
	}

	return NumericProperty->GetSignedIntPropertyValue(NumericProperty->ContainerPtrToValuePtr<void>(Container));
}

TOptional<double> FEditConditionContext::GetNumericValue(const FString& PropertyName, TWeakObjectPtr<UFunction> CachedFunction) const
{
	if (!IsValid() || CachedFunction.IsValid())
	{
		return {};
	}

	const FProperty* Property = SourceStruct->FindPropertyByName(FName{PropertyName});
	const FNumericProperty* NumericProperty = CastField<FNumericProperty>(Property);
	if (NumericProperty == nullptr)
	{
		return {};
	}

	const void* ValuePtr = NumericProperty->ContainerPtrToValuePtr<void>(Container);
	TOptional<double> Result{};
	if (NumericProperty->IsInteger())
	{
		Result = static_cast<double>(NumericProperty->GetSignedIntPropertyValue(ValuePtr));
	}
	else if (NumericProperty->IsFloatingPoint())
	{
		Result = NumericProperty->GetFloatingPointPropertyValue(ValuePtr);
	}

	return Result;
}

TOptional<FString> FEditConditionContext::GetEnumValue(const FString& PropertyName, TWeakObjectPtr<UFunction> CachedFunction) const
{
	if (!IsValid() || CachedFunction.IsValid())
	{
		return {};
	}

	const FProperty* Property = SourceStruct->FindPropertyByName(FName{PropertyName});
	const FNumericProperty* NumericProperty = CastField<FNumericProperty>(Property);
	
	const UEnum* EnumType = nullptr;
	if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
	{
		NumericProperty = EnumProperty->GetUnderlyingProperty();
		EnumType = EnumProperty->GetEnum();
	}
	else if (const FByteProperty* ByteProperty = CastField<FByteProperty>(Property))
	{
		NumericProperty = ByteProperty;
		EnumType = ByteProperty->GetIntPropertyEnum();
	}

	if (EnumType == nullptr || NumericProperty == nullptr || !NumericProperty->IsInteger())
	{
		return {};
	}

	const void* ValuePtr = NumericProperty->ContainerPtrToValuePtr<void>(Container);
	const int64 Value = NumericProperty->GetSignedIntPropertyValue(ValuePtr);
	return EnumType->GetNameStringByValue(Value);
}

TOptional<UObject*> FEditConditionContext::GetPointerValue(const FString& PropertyName, TWeakObjectPtr<UFunction> CachedFunction) const
{
	if (!IsValid() || CachedFunction.IsValid())
	{
		return {};
	}

	const FProperty* Property = SourceStruct->FindPropertyByName(FName{PropertyName});
	const FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property);
	if (ObjectProperty == nullptr)
	{
		return {};
	}

	const void* ValuePtr = ObjectProperty->ContainerPtrToValuePtr<void>(Container);
	return ObjectProperty->GetObjectPropertyValue(ValuePtr);
}

TOptional<FString> FEditConditionContext::GetTypeName(const FString& PropertyName, TWeakObjectPtr<UFunction> CachedFunction) const
{
	if (!IsValid())
	{
		return {};
	}

	auto GetPropertyName = [](const FProperty* Property) -> TOptional<FString>
	{
		if (Property == nullptr)
		{
			return {};
		}

		if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
		{
			return EnumProperty->GetEnum()->GetName();
		}
		else if (const FByteProperty* ByteProperty = CastField<FByteProperty>(Property))
		{
			const UEnum* EnumType = ByteProperty->GetIntPropertyEnum();
			if (EnumType != nullptr)
			{
				return EnumType->GetName();
			}
		}

		return Property->GetCPPType();
	};

	if (const UFunction* Function = CachedFunction.Get())
	{
		return GetPropertyName(Function->GetReturnProperty());
	}

	const FProperty* Property = SourceStruct->FindPropertyByName(FName{PropertyName});

	return GetPropertyName(Property);
}

TOptional<int64> FEditConditionContext::GetIntegerValueOfEnum(const FString& EnumTypeName, const FString& MemberName) const
{
	const UEnum* EnumType = UClass::TryFindTypeSlow<UEnum>(EnumTypeName, EFindFirstObjectOptions::ExactClass);
	if (EnumType == nullptr)
	{
		return {};
	}

	const int64 EnumValue = EnumType->GetValueByName(FName(*MemberName));
	if (EnumValue == INDEX_NONE)
	{
		return {};
	}

	return EnumValue;
}

} // UE::AssetValidation