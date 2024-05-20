
#include "AttributeSet.h"
#include "GameplayTagContainer.h"
#include "PropertyValidators/PropertyValidation.h"
#include "AutomationHelpers.h"

using UE::AssetValidation::AutomationFlags;

template <typename TPropertyType>
TPropertyType* ConstructPropertyBase(EObjectFlags InFlags)
{
	static int32 Counter = 0;
	const FFieldClass* FieldClass = TPropertyType::StaticClass();

	const FString PropertyName = FString::Printf(TEXT("%s_%d"), *FieldClass->GetName(), ++Counter);
	TPropertyType* Property = CastFieldChecked<TPropertyType>(FieldClass->Construct(nullptr, FName{PropertyName}, InFlags));

	return Property;
}

template <typename TPropertyType, typename ...TArgs>
TPropertyType* ConstructPropertyUnsafe(TArgs... Args)
{
	return ConstructPropertyBase<TPropertyType>(RF_NoFlags);
}

template <>
FByteProperty* ConstructPropertyUnsafe<>(UEnum* Enum)
{
	FByteProperty* ByteProperty{ConstructPropertyBase<FByteProperty>(RF_NoFlags)};
	ByteProperty->Enum = Enum;

	return ByteProperty;
}

template <>
FStructProperty* ConstructPropertyUnsafe<>(UScriptStruct* Struct)
{
	FStructProperty* StructProperty{ConstructPropertyBase<FStructProperty>(RF_NoFlags)};
	StructProperty->Struct = Struct;

	return StructProperty;
}

template <>
FArrayProperty* ConstructPropertyUnsafe<>(FProperty* InnerProperty)
{
	FArrayProperty* ArrayProperty{ConstructPropertyBase<FArrayProperty>(RF_NoFlags)};
	ArrayProperty->Inner = InnerProperty;

	return ArrayProperty;
}

template <>
FSetProperty* ConstructPropertyUnsafe<>(FProperty* ElemProperty)
{
	FSetProperty* SetProperty{ConstructPropertyBase<FSetProperty>(RF_NoFlags)};
	SetProperty->ElementProp = ElemProperty;

	return SetProperty;
}

template <>
FMapProperty* ConstructPropertyUnsafe<>(FProperty* KeyProp, FProperty* ValueProp)
{
	FMapProperty* MapProperty{ConstructPropertyBase<FMapProperty>(RF_NoFlags)};
	MapProperty->KeyProp = KeyProp;
	MapProperty->ValueProp = ValueProp;

	return MapProperty;
}

template <typename TPropertyType, typename ...TArgs>
FProperty* ConstructFPropertyUnsafe(TArgs&&... Args)
{
	return (FProperty*)ConstructPropertyUnsafe<TPropertyType>(Forward<TArgs>(Args)...);
}

template <typename TPropertyType, typename ...TArgs>
TSharedPtr<TPropertyType> ConstructProperty(TArgs... Args)
{
	return TSharedPtr<TPropertyType>{ConstructPropertyUnsafe<TPropertyType>(Forward<TArgs>(Args)...)};
}

template <typename TPropertyType, typename TPred, typename ...TArgs>
bool CanApplyToProperty(TPred&& Pred, TArgs&&... Args)
{
	TSharedPtr<TPropertyType> Property = ConstructProperty<TPropertyType>(Forward<TArgs>(Args)...);
	return Pred(Property.Get());
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAutomationTest_MetaSpecifiers, "PropertyValidation.MetaSpecifiers", AutomationFlags)

bool FAutomationTest_MetaSpecifiers::RunTest(const FString& Parameters)
{
	using namespace UE::AssetValidation;
	
	/**
	 * ------------ VALIDATE ------------
	 * meta = ("Validate") can be placed on a property if underlying property has a property validator,
	 * or property is a container property and element property has a property validator
	 */
	
	using FuncType = bool(*)(const FProperty* Property);
	FuncType CanApplyMeta = CanApplyMeta_Validate;

	FString What{TEXT("CanApplyMeta_Validate returns true for correct properties")};

	// engine properties
	TestEqual(What, CanApplyToProperty<FProperty>(CanApplyMeta),		false);
	TestEqual(What, CanApplyToProperty<FBoolProperty>(CanApplyMeta),	false);
	TestEqual(What, CanApplyToProperty<FByteProperty>(CanApplyMeta, nullptr), false);
	TestEqual(What, CanApplyToProperty<FNumericProperty>(CanApplyMeta), false);
	TestEqual(What, CanApplyToProperty<FObjectPropertyBase>(CanApplyMeta), false);
	
	TestEqual(What, CanApplyToProperty<FEnumProperty>(CanApplyMeta),	true);
	// byte property validation is not fully supported due to different interpretations of an Invalid value
#if 0
	TestEqual(What, CanApplyToProperty<FByteProperty>(CanApplyMeta, UEnum::StaticClass()->GetDefaultObject<UEnum>()), true);
#endif
	TestEqual(What, CanApplyToProperty<FNameProperty>(CanApplyMeta),	true);
	TestEqual(What, CanApplyToProperty<FStrProperty>(CanApplyMeta), true);
	TestEqual(What, CanApplyToProperty<FTextProperty>(CanApplyMeta),	true);
	TestEqual(What, CanApplyToProperty<FObjectProperty>(CanApplyMeta), true);
	TestEqual(What, CanApplyToProperty<FClassProperty>(CanApplyMeta), true);
	TestEqual(What, CanApplyToProperty<FSoftObjectProperty>(CanApplyMeta), true);
	TestEqual(What, CanApplyToProperty<FSoftClassProperty>(CanApplyMeta), true);

	// struct properties
	TestEqual(What, CanApplyToProperty<FStructProperty>(CanApplyMeta, FStructWithoutValidator::StaticStruct()), false);

	TestEqual(What, CanApplyToProperty<FStructProperty>(CanApplyMeta, FGameplayTag::StaticStruct()), true);
	TestEqual(What, CanApplyToProperty<FStructProperty>(CanApplyMeta, FGameplayTagContainer::StaticStruct()), true);
	TestEqual(What, CanApplyToProperty<FStructProperty>(CanApplyMeta, FGameplayTagQuery::StaticStruct()), true);
	TestEqual(What, CanApplyToProperty<FStructProperty>(CanApplyMeta, FGameplayAttribute::StaticStruct()), true);
	TestEqual(What, CanApplyToProperty<FStructProperty>(CanApplyMeta, FGameplayTag::StaticStruct()), true);
	TestEqual(What, CanApplyToProperty<FStructProperty>(CanApplyMeta, FDataTableRowHandle::StaticStruct()), true);

	// container properties
	{
		// cannot validate engine property value inside container
		auto ConstructProp = [] { return ConstructFPropertyUnsafe<FNumericProperty>(); };
		TestEqual(What, CanApplyToProperty<FArrayProperty>(CanApplyMeta, ConstructProp()), false);
		TestEqual(What, CanApplyToProperty<FSetProperty>(CanApplyMeta, ConstructProp()), false);
		TestEqual(What, CanApplyToProperty<FMapProperty>(CanApplyMeta, ConstructProp(), ConstructProp()), false);
	}

	{
		// can validate name value inside container
		auto ConstructProp = [] { return ConstructFPropertyUnsafe<FNameProperty>(); };
		TestEqual(What, CanApplyToProperty<FArrayProperty>(CanApplyMeta, ConstructProp()), true);
		TestEqual(What, CanApplyToProperty<FSetProperty>(CanApplyMeta, ConstructProp()), true);
		TestEqual(What, CanApplyToProperty<FMapProperty>(CanApplyMeta, ConstructProp(), ConstructProp()), true);
	}

	{
		// cannot validate struct value inside container
		auto ConstructProp = [] { return ConstructFPropertyUnsafe<FStructProperty>(FStructWithoutValidator::StaticStruct()); };
		TestEqual(What, CanApplyToProperty<FArrayProperty>(CanApplyMeta, ConstructProp()), false);
		TestEqual(What, CanApplyToProperty<FSetProperty>(CanApplyMeta, ConstructProp()), false);
		TestEqual(What, CanApplyToProperty<FMapProperty>(CanApplyMeta, ConstructProp(), ConstructProp()), false);
	}
	
	{
		// validate struct value
		auto ConstructProp = [] { return ConstructFPropertyUnsafe<FStructProperty>(FGameplayTag::StaticStruct()); };
		TestEqual(What, CanApplyToProperty<FArrayProperty>(CanApplyMeta, ConstructProp()), true);
		TestEqual(What, CanApplyToProperty<FSetProperty>(CanApplyMeta, ConstructProp()), true);
		TestEqual(What, CanApplyToProperty<FMapProperty>(CanApplyMeta, ConstructProp(), ConstructProp()), true);
	}

	/** 
	 * ------------ VALIDATE KEY ------------
	 * meta = ("ValidateKey") can be placed specifically on map property, that has a key property that satisfies meta = ("Validate") requirements
	 */

	CanApplyMeta = CanApplyMeta_ValidateKey;
	What = TEXT("CanApplyMeta_ValidateKey returns true for correct properties");

	TestEqual(What, CanApplyToProperty<FMapProperty>(CanApplyMeta, ConstructFPropertyUnsafe<FNameProperty>(), ConstructFPropertyUnsafe<FNameProperty>()), true);
	TestEqual(What, CanApplyToProperty<FMapProperty>(CanApplyMeta, ConstructFPropertyUnsafe<FNameProperty>(), ConstructFPropertyUnsafe<FNumericProperty>()), true);
	TestEqual(What, CanApplyToProperty<FMapProperty>(CanApplyMeta, ConstructFPropertyUnsafe<FNumericProperty>(), ConstructFPropertyUnsafe<FNameProperty>()), false);
	TestEqual(What, CanApplyToProperty<FMapProperty>(CanApplyMeta, ConstructFPropertyUnsafe<FNumericProperty>(), ConstructFPropertyUnsafe<FNumericProperty>()), false);

	/**
	 * ------------ VALIDATE VALUE ------------
	 * meta = ("ValidateValue") can be placed specifically on map property, that has a value property that satisfies meta = ("Validate") requirements
	 */

	CanApplyMeta = CanApplyMeta_ValidateValue;
	What = TEXT("CanApplyMeta_ValidateValue returns true for correct properties");

	TestEqual(What, CanApplyToProperty<FMapProperty>(CanApplyMeta, ConstructFPropertyUnsafe<FNameProperty>(), ConstructFPropertyUnsafe<FNameProperty>()), true);
	TestEqual(What, CanApplyToProperty<FMapProperty>(CanApplyMeta, ConstructFPropertyUnsafe<FNameProperty>(), ConstructFPropertyUnsafe<FNumericProperty>()), false);
	TestEqual(What, CanApplyToProperty<FMapProperty>(CanApplyMeta, ConstructFPropertyUnsafe<FNumericProperty>(), ConstructFPropertyUnsafe<FNameProperty>()), true);
	TestEqual(What, CanApplyToProperty<FMapProperty>(CanApplyMeta, ConstructFPropertyUnsafe<FNumericProperty>(), ConstructFPropertyUnsafe<FNumericProperty>()), false);
	

	
	/**
	 * ------------ VALIDATE RECURSIVE ------------
	 * meta = ("ValidateRecursive") can be placed either on object property, soft object property, or struct property,
	 * both as a plain property and a part of some container property (array, set, map).
	 * Struct property allow meta specifier, but don't require it by default.
	 * @todo: disallow "ValidateRecursive" for FClassProperty and FObjectPropertyBase.
	 */
	
	CanApplyMeta = CanApplyMeta_ValidateRecursive;
	What = TEXT("CanApplyMeta_ValidateRecursive returns true for correct properties");
	
	TestEqual(What, CanApplyToProperty<FTextProperty>(CanApplyMeta), false);
	TestEqual(What, CanApplyToProperty<FNameProperty>(CanApplyMeta), false);
	TestEqual(What, CanApplyToProperty<FObjectPropertyBase>(CanApplyMeta), false);
	TestEqual(What, CanApplyToProperty<FClassProperty>(CanApplyMeta), false);
	TestEqual(What, CanApplyToProperty<FSoftClassProperty>(CanApplyMeta), false);

	TestEqual(What, CanApplyToProperty<FObjectProperty>(CanApplyMeta), true);
	TestEqual(What, CanApplyToProperty<FSoftObjectProperty>(CanApplyMeta), true);
	TestEqual(What, CanApplyToProperty<FStructProperty>(CanApplyMeta, FStructWithoutValidator::StaticStruct()), true);
	TestEqual(What, CanApplyToProperty<FStructProperty>(CanApplyMeta, FGameplayTag::StaticStruct()), true);

	// container properties

	{
		auto ConstructProp = [] { return ConstructFPropertyUnsafe<FTextProperty>(); };
		TestEqual(What, CanApplyToProperty<FArrayProperty>(CanApplyMeta, ConstructProp()), false);
		TestEqual(What, CanApplyToProperty<FSetProperty>(CanApplyMeta, ConstructProp()), false);
		TestEqual(What, CanApplyToProperty<FMapProperty>(CanApplyMeta, ConstructProp(), ConstructProp()), false);
	}
	
	{
		auto ConstructProp = [] { return ConstructFPropertyUnsafe<FObjectProperty>(); };
		TestEqual(What, CanApplyToProperty<FArrayProperty>(CanApplyMeta, ConstructProp()), true);
		TestEqual(What, CanApplyToProperty<FSetProperty>(CanApplyMeta, ConstructProp()), true);
		TestEqual(What, CanApplyToProperty<FMapProperty>(CanApplyMeta, ConstructProp(), ConstructProp()), true);
	}

	{
		auto ConstructProp = [] { return ConstructFPropertyUnsafe<FSoftObjectProperty>(); };
		TestEqual(What, CanApplyToProperty<FArrayProperty>(CanApplyMeta, ConstructProp()), true);
		TestEqual(What, CanApplyToProperty<FSetProperty>(CanApplyMeta, ConstructProp()), true);
		TestEqual(What, CanApplyToProperty<FMapProperty>(CanApplyMeta, ConstructProp(), ConstructProp()), true);
	}

	{
		auto ConstructProp = [] { return ConstructFPropertyUnsafe<FStructProperty>(FStructWithoutValidator::StaticStruct()); };
		TestEqual(What, CanApplyToProperty<FArrayProperty>(CanApplyMeta, ConstructProp()), true);
		TestEqual(What, CanApplyToProperty<FSetProperty>(CanApplyMeta, ConstructProp()), true);
		TestEqual(What, CanApplyToProperty<FMapProperty>(CanApplyMeta, ConstructProp(), ConstructProp()), true);
	}

	{
		auto ConstructProp = [] { return ConstructFPropertyUnsafe<FStructProperty>(FGameplayTag::StaticStruct()); };
		TestEqual(What, CanApplyToProperty<FArrayProperty>(CanApplyMeta, ConstructProp()), true);
		TestEqual(What, CanApplyToProperty<FSetProperty>(CanApplyMeta, ConstructProp()), true);
		TestEqual(What, CanApplyToProperty<FMapProperty>(CanApplyMeta, ConstructProp(), ConstructProp()), true);
	}
	
	return !HasAnyErrors();
}
