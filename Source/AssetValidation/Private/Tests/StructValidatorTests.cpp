#include "StructValidatorTests.h"

#include "Misc/AutomationTest.h"

#include "PropertyValidatorSubsystem.h"
#include "PropertyValidators/PropertyValidation.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAutomationTest_GameplayTag, "Editor.PropertyValidation.StructValidators.GameplayTag", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)

bool FAutomationTest_GameplayTag::RunTest(const FString& Parameters)
{
	UPropertyValidatorSubsystem* Subsystem = GEditor->GetEditorSubsystem<UPropertyValidatorSubsystem>();
	UObject* Object = NewObject<UValidationTestObject_GameplayTag>();

	FPropertyValidationResult Result = Subsystem->IsPropertyContainerValid(Object);
	UTEST_EQUAL(TEXT("ValidationResult"), Result.ValidationResult, EDataValidationResult::Invalid)
	UTEST_EQUAL(TEXT("NumErrors"), Result.Errors.Num(), 3);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAutomationTest_GameplayTagContainer, "Editor.PropertyValidation.StructValidators.GameplayTagContainer", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)

bool FAutomationTest_GameplayTagContainer::RunTest(const FString& Parameters)
{
	UPropertyValidatorSubsystem* Subsystem = GEditor->GetEditorSubsystem<UPropertyValidatorSubsystem>();
	UObject* Object = NewObject<UValidationTestObject_GameplayTagContainer>();
	
	FPropertyValidationResult Result = Subsystem->IsPropertyContainerValid(Object);
	UTEST_EQUAL(TEXT("ValidationResult"), Result.ValidationResult, EDataValidationResult::Invalid)
	UTEST_EQUAL(TEXT("NumErrors"), Result.Errors.Num(), 3);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAutomationTest_GameplayAttribute, "Editor.PropertyValidation.StructValidators.GameplayAttribute", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)

bool FAutomationTest_GameplayAttribute::RunTest(const FString& Parameters)
{
	UPropertyValidatorSubsystem* Subsystem = GEditor->GetEditorSubsystem<UPropertyValidatorSubsystem>();
	UObject* Object = NewObject<UValidationTestObject_GameplayAttribute>();
	
	FPropertyValidationResult Result = Subsystem->IsPropertyContainerValid(Object);
	UTEST_EQUAL(TEXT("ValidationResult"), Result.ValidationResult, EDataValidationResult::Invalid)
	UTEST_EQUAL(TEXT("NumErrors"), Result.Errors.Num(), 3);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAutomationTest_DataTableRow, "Editor.PropertyValidation.StructValidators.DataTableRow", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)

bool FAutomationTest_DataTableRow::RunTest(const FString& Parameters)
{
	UPropertyValidatorSubsystem* Subsystem = GEditor->GetEditorSubsystem<UPropertyValidatorSubsystem>();
	UObject* Object = NewObject<UValidationTestObject_DataTableRow>();
	
	FPropertyValidationResult Result = Subsystem->IsPropertyContainerValid(Object);
	UTEST_EQUAL(TEXT("ValidationResult"), Result.ValidationResult, EDataValidationResult::Invalid)
	UTEST_EQUAL(TEXT("NumErrors"), Result.Errors.Num(), 3);

	return true;
}















