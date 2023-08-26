#include "StructValidatorTests.h"

#include "Misc/AutomationTest.h"

#include "PropertyValidatorSubsystem.h"
#include "PropertyValidators/PropertyValidation.h"

class FStructValidatorAutomationTest: public FAutomationTestBase
{
public:

	FStructValidatorAutomationTest( const FString& InName, const bool bInComplexTask)
		: FAutomationTestBase(InName, bInComplexTask)
	{ }
	
	template <typename TObjectClass>
	bool ValidateObject(int32 ExpectedErrors)
	{
		UPropertyValidatorSubsystem* Subsystem = GEditor->GetEditorSubsystem<UPropertyValidatorSubsystem>();
		
		UObject* Object = NewObject<TObjectClass>();
		
		FPropertyValidationResult Result = Subsystem->IsPropertyContainerValid(Object);
		UTEST_EQUAL(TEXT("ValidationResult"), Result.ValidationResult, EDataValidationResult::Invalid)
		UTEST_EQUAL(TEXT("NumErrors"), Result.Errors.Num(), ExpectedErrors);

		Object->MarkAsGarbage();

		return true;
	}
};

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FAutomationTest_PropertyValidators, FStructValidatorAutomationTest, "Editor.PropertyValidation.PropertyValidators", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)

bool FAutomationTest_PropertyValidators::RunTest(const FString& Parameters)
{
	return ValidateObject<UValidationTestObject_PropertyValidators>(8);
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FAutomationTest_GameplayTag, FStructValidatorAutomationTest, "Editor.PropertyValidation.StructValidators.GameplayTag", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)

bool FAutomationTest_GameplayTag::RunTest(const FString& Parameters)
{
	return ValidateObject<UValidationTestObject_GameplayTag>(3);
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FAutomationTest_GameplayTagContainer, FStructValidatorAutomationTest, "Editor.PropertyValidation.StructValidators.GameplayTagContainer", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)

bool FAutomationTest_GameplayTagContainer::RunTest(const FString& Parameters)
{
	return ValidateObject<UValidationTestObject_GameplayTagContainer>(3);
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FAutomationTest_GameplayAttribute, FStructValidatorAutomationTest, "Editor.PropertyValidation.StructValidators.GameplayAttribute", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)

bool FAutomationTest_GameplayAttribute::RunTest(const FString& Parameters)
{
	return ValidateObject<UValidationTestObject_GameplayAttribute>(3);
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FAutomationTest_DataTableRow, FStructValidatorAutomationTest, "Editor.PropertyValidation.StructValidators.DataTableRow", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)

bool FAutomationTest_DataTableRow::RunTest(const FString& Parameters)
{
	return ValidateObject<UValidationTestObject_DataTableRow>(3);
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FAutomationTest_DirectoryPath, FStructValidatorAutomationTest, "Editor.PropertyValidation.StructValidators.DirectoryPath", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)

bool FAutomationTest_DirectoryPath::RunTest(const FString& Parameters)
{
	return ValidateObject<UValidationTestObject_DirectoryPath>(4);
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FAutomationTest_FilePath, FStructValidatorAutomationTest, "Editor.PropertyValidation.StructValidators.FilePath", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)

bool FAutomationTest_FilePath::RunTest(const FString& Parameters)
{
	return ValidateObject<UValidationTestObject_FilePath>(4);
}













