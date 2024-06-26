#include "StructValidatorTests.h"

#include "Misc/AutomationTest.h"

#include "PropertyValidatorSubsystem.h"
#include "PropertyValidatorTests.h"
#include "AutomationHelpers.h"

using UE::AssetValidation::AutomationFlags;

class FStructValidatorAutomationTest : public FAutomationTestBase
{
public:
	FStructValidatorAutomationTest(const FString& InName, const bool bInComplexTask)
		: FAutomationTestBase(InName, bInComplexTask)
	{
	}

	template <typename TObjectClass>
	bool ValidateObject(int32 ExpectedErrors)
	{
		UPropertyValidatorSubsystem* Subsystem = GEditor->GetEditorSubsystem<UPropertyValidatorSubsystem>();

		UObject* Object = NewObject<TObjectClass>();

		FPropertyValidationResult Result = Subsystem->ValidateObject(Object);
		UTEST_EQUAL(TEXT("ValidationResult"), Result.ValidationResult, EDataValidationResult::Invalid)
		UTEST_EQUAL(TEXT("NumErrors"), Result.Errors.Num(), ExpectedErrors);

		Object->MarkAsGarbage();

		return true;
	}
};

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FAutomationTest_StructProperties, FStructValidatorAutomationTest,
                                        "PropertyValidation.SubsystemAPI", AutomationFlags)

bool FAutomationTest_StructProperties::RunTest(const FString& Parameters)
{
	// ensures that ValidateObjectProperty, ValidateNestedStruct and ValidateNestedStructProperty work as expected in relation to one another
	UPropertyValidatorSubsystem* Subsystem = GEditor->GetEditorSubsystem<UPropertyValidatorSubsystem>();
	check(Subsystem);

	UObject* Object = NewObject<UValidationTestObject_StructValidation>();
	const UScriptStruct* StructType = FValidationStruct::StaticStruct();
	const FProperty* StructProperty = Object->GetClass()->FindPropertyByName("Struct");

	{
		// validate struct as an object's property
		FPropertyValidationResult Result = Subsystem->ValidateObjectProperty(Object, StructProperty);
		UTEST_EQUAL(TEXT("ValidationResult"), Result.ValidationResult, EDataValidationResult::Invalid);
		UTEST_EQUAL(TEXT("NumErrors"), Result.Errors.Num(), 1);
	}

	const uint8* StructMemory = StructProperty->ContainerPtrToValuePtr<uint8>(Object);
	{
		// validate struct in a separate "nested struct" flow
		FPropertyValidationResult Result = Subsystem->ValidateStruct(Object, StructType, StructMemory);
		UTEST_EQUAL(TEXT("ValidationResult"), Result.ValidationResult, EDataValidationResult::Invalid);
		UTEST_EQUAL(TEXT("NumErrors"), Result.Errors.Num(), 1);
	}

	FProperty* NameProperty = StructType->FindPropertyByName("TagToValidate");
	{
		// validate single "TagToValidate" property inside same struct
		FPropertyValidationResult Result = Subsystem->ValidateStructProperty(
			Object, StructType, NameProperty, StructMemory);
		UTEST_EQUAL(TEXT("ValidationResult"), Result.ValidationResult, EDataValidationResult::Invalid);
		UTEST_EQUAL(TEXT("NumErrors"), Result.Errors.Num(), 1);
	}

	// @todo: currently ValidateStruct ignores the actual struct and does only property validation. https://github.com/GoldenPhasmid/AssetValidation/issues/20
	// We should also check if there's a struct validator for this struct type and if the struct value is valid
#if 0
	{
		// validate "FGameplayTag: TagToValidate" directly as a double nested struct inside an object
		FPropertyValidationResult Result = Subsystem->ValidateStruct(Object, FGameplayTag::StaticStruct(), NameProperty->ContainerPtrToValuePtr<uint8>(StructMemory));
		UTEST_EQUAL(TEXT("ValidationResult"), Result.ValidationResult, EDataValidationResult::Invalid);
		UTEST_EQUAL(TEXT("NumErrors"), Result.Errors.Num(), 1);
	}
#endif

	return !HasAnyErrors();
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FAutomationTest_PropertyTypes, FStructValidatorAutomationTest,
                                        "PropertyValidation.PropertyTypes", AutomationFlags)

bool FAutomationTest_PropertyTypes::RunTest(const FString& Parameters)
{
	return ValidateObject<UValidationTestObject_PropertyTypes>(8);
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FAutomationTest_StructValidation, FStructValidatorAutomationTest,
                                        "PropertyValidation.StructProperties", AutomationFlags)

bool FAutomationTest_StructValidation::RunTest(const FString& Parameters)
{
	// struct properties should be validated automatically, either as a part of UObject storage or container storage
	return ValidateObject<UValidationTestObject_StructValidation>(5);
}

UValidationTestObject_SoftObjectPath::UValidationTestObject_SoftObjectPath()
{
	EmptyPathArray.AddDefaulted();
	EmptyPathArray2.AddDefaulted();
	BadPath			= TEXT("/Temp/Path/That/Doesnt/Exist");
	Struct.BadPath	= TEXT("/Temp/Path/That/Doesnt/Exist");
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FAutomationTest_SoftObjectPath, FStructValidatorAutomationTest,
                                        "PropertyValidation.StructValidators.SoftObjectPath", AutomationFlags)

bool FAutomationTest_SoftObjectPath::RunTest(const FString& Parameters)
{
	// SoftObjectPath struct value should be validated
	return ValidateObject<UValidationTestObject_SoftObjectPath>(5);
}

UValidationTestObject_SoftClassPath::UValidationTestObject_SoftClassPath()
{
	EmptyPathArray.AddDefaulted();
	BadPath			= FString{TEXT("/Script/Class/That/Doesnt/Exist.Name")};
	Struct.BadPath	= FString{TEXT("/Script/Class/That/Doesnt/Exist.Name")};
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FAutomationTest_SoftClassPath, FStructValidatorAutomationTest,
										"PropertyValidation.StructValidators.SoftClassPath", AutomationFlags)

bool FAutomationTest_SoftClassPath::RunTest(const FString& Parameters)
{
	// SoftObjectPath struct value should be validated
	return ValidateObject<UValidationTestObject_SoftClassPath>(5);
}

FGameplayTag CreateInvalidTag()
{
	FGameplayTag Result;

	const FNameProperty* NameProperty = CastFieldChecked<FNameProperty>(
		FGameplayTag::StaticStruct()->FindPropertyByName(TEXT("TagName")));
	NameProperty->SetPropertyValue(NameProperty->ContainerPtrToValuePtr<void>(&Result), TEXT("Tag.Invalid"));

	check(Result.GetTagName() == TEXT("Tag.Invalid"));
	return Result;
}

UValidationTestObject_GameplayTag::UValidationTestObject_GameplayTag()
{
	EmptyTagArray.AddDefaulted();
	BadTag = CreateInvalidTag();
	Struct.BadTag = CreateInvalidTag();
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FAutomationTest_GameplayTag, FStructValidatorAutomationTest,
                                        "PropertyValidation.StructValidators.GameplayTag", AutomationFlags)

bool FAutomationTest_GameplayTag::RunTest(const FString& Parameters)
{
	// GameplayTag struct value should be validated
	return ValidateObject<UValidationTestObject_GameplayTag>(5);
}

UValidationTestObject_GameplayTagContainer::UValidationTestObject_GameplayTagContainer()
{
	const FGameplayTag InvalidTag = CreateInvalidTag();

	BadTags.AddTag(InvalidTag);
	Struct.BadTags.AddTag(InvalidTag);
	BadTagsArray.Add(FGameplayTagContainer{InvalidTag});
}


IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FAutomationTest_GameplayTagContainer, FStructValidatorAutomationTest,
                                        "PropertyValidation.StructValidators.GameplayTagContainer",
                                        AutomationFlags)

bool FAutomationTest_GameplayTagContainer::RunTest(const FString& Parameters)
{
	// GameplayTagContainer struct value should be validated
	return ValidateObject<UValidationTestObject_GameplayTagContainer>(3);
}

UValidationTestObject_GameplayTagQuery::UValidationTestObject_GameplayTagQuery()
{
	const FGameplayTag InvalidTag = CreateInvalidTag();
	FGameplayTagQuery InvalidQuery = FGameplayTagQuery::MakeQuery_MatchTag(InvalidTag);

	BadQuery = InvalidQuery;
	Struct.BadQuery = InvalidQuery;
	BadQueryArray.Add(InvalidQuery);
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FAutomationTest_GameplayTagQuery, FStructValidatorAutomationTest,
                                        "PropertyValidation.StructValidators.GameplayTagQuery", AutomationFlags)

bool FAutomationTest_GameplayTagQuery::RunTest(const FString& Parameters)
{
	// GameplayTagQuery struct value should be validated
	return ValidateObject<UValidationTestObject_GameplayTagQuery>(3);
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FAutomationTest_GameplayAttribute, FStructValidatorAutomationTest,
                                        "PropertyValidation.StructValidators.GameplayAttribute", AutomationFlags)

bool FAutomationTest_GameplayAttribute::RunTest(const FString& Parameters)
{
	// GameplayAttribute struct value should be validated
	return ValidateObject<UValidationTestObject_GameplayAttribute>(3);
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FAutomationTest_DataTableRow, FStructValidatorAutomationTest,
                                        "PropertyValidation.StructValidators.DataTableRow", AutomationFlags)

bool FAutomationTest_DataTableRow::RunTest(const FString& Parameters)
{
	// DataTableRowHandle struct value should be validated
	return ValidateObject<UValidationTestObject_DataTableRow>(3);
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FAutomationTest_DirectoryPath, FStructValidatorAutomationTest,
                                        "PropertyValidation.StructValidators.DirectoryPath", AutomationFlags)

bool FAutomationTest_DirectoryPath::RunTest(const FString& Parameters)
{
	// DirectoryPath struct value should be validated
	return ValidateObject<UValidationTestObject_DirectoryPath>(4);
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FAutomationTest_FilePath, FStructValidatorAutomationTest,
                                        "PropertyValidation.StructValidators.FilePath", AutomationFlags)

bool FAutomationTest_FilePath::RunTest(const FString& Parameters)
{
	// FilePath struct value should be validated
	return ValidateObject<UValidationTestObject_FilePath>(4);
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FAutomationTest_PrimaryAssetId, FStructValidatorAutomationTest,
                                        "PropertyValidation.StructValidators.PrimaryAssetId", AutomationFlags)

bool FAutomationTest_PrimaryAssetId::RunTest(const FString& Parameters)
{
	// PrimaryAssetID struct value should be validated
	return ValidateObject<UValidationTestObject_PrimaryAssetId>(6);
}

UValidationTestObject_InstancedStruct::UValidationTestObject_InstancedStruct()
{
	EmptyStructArray.AddDefaulted();
	CompositeArray.AddDefaulted();

	InvalidStruct = FInstancedStruct::Make<FGameplayTag>(CreateInvalidTag());
	InvalidStructArray.Add(InvalidStruct);
	InvalidComposite.InstancedStruct = InvalidStruct;
	InvalidCompositeArray.Add(InvalidComposite);
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FAutomationTest_InstancedStruct, FStructValidatorAutomationTest,
										"PropertyValidation.StructValidators.InstancedStruct", AutomationFlags)

bool FAutomationTest_InstancedStruct::RunTest(const FString& Parameters)
{
	// PrimaryAssetID struct value should be validated
	return ValidateObject<UValidationTestObject_InstancedStruct>(8);
}
