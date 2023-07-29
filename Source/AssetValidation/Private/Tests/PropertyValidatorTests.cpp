#include "PropertyValidatorTests.h"

#include "Misc/AutomationTest.h"
#include "PropertyValidatorSubsystem.h"
#include "PropertyValidators/PropertyValidation.h"

static TArray<TPair<FName, EDataValidationResult>> PropertyNames
{
	{"NonEditableProperty",		EDataValidationResult::Valid},
	{"TransientProperty",			EDataValidationResult::Valid},
	{"TransientEditableProperty",	EDataValidationResult::Valid},
	{"EditDefaultOnlyProperty",	EDataValidationResult::Invalid},
	{"EditAnywhereProperty",		EDataValidationResult::Invalid},
	{"EditInstanceOnlyProperty",	EDataValidationResult::Invalid}
};

BEGIN_DEFINE_SPEC(FAutomationSpec_ValidationConditions, "Editor.PropertyValidation.Conditions", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask);
	UObject* TestObject;
	UPropertyValidatorSubsystem* ValidationSubsystem;
END_DEFINE_SPEC(FAutomationSpec_ValidationConditions)
void FAutomationSpec_ValidationConditions::Define()
{
	BeforeEach([this]()
	{
		TestObject = NewObject<UTestObject_ValidationConditions>(GetTransientPackage());
		TestObject->AddToRoot();
		
		ValidationSubsystem = GEditor->GetEditorSubsystem<UPropertyValidatorSubsystem>();
	});
	
	for (auto [PropertyName, ExpectedResult]: PropertyNames)
	{
		It(FString::Printf(TEXT("should validate %s"), *PropertyName.ToString()),
			[this, PropertyName, ExpectedResult]()
		{
			FProperty* Property = TestObject->GetClass()->FindPropertyByName(PropertyName);
			FPropertyValidationResult Result = ValidationSubsystem->IsPropertyValid(TestObject, Property);

			TestEqual("ValidationResult", Result.ValidationResult, ExpectedResult);
		});	
	}

	AfterEach([this]()
	{
		TestObject->RemoveFromRoot();
		TestObject = nullptr;
		ValidationSubsystem = nullptr;
	});
}

#if 0
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FComplexAutomationTest_ValidationConditions, "Editor.PropertyValidation.ComplexConditions", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask);
void FComplexAutomationTest_ValidationConditions::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	int32 Index = 0;
	for (auto [PropertyName, ExpectedResult]: PropertyNames)
	{
		OutBeautifiedNames.Add(FString::Printf(TEXT("should validate %s"), *PropertyName.ToString()));
		OutTestCommands.Add(FString::FromInt(Index));
		++Index;
	}
}

bool FComplexAutomationTest_ValidationConditions::RunTest(const FString& Parameters)
{
	int32 Index = FCString::Atoi(*Parameters);

	UObject* TestObject = NewObject<UTestObject_ValidationConditions>(GetTransientPackage());
	UPropertyValidatorSubsystem* ValidationSubsystem = GEditor->GetEditorSubsystem<UPropertyValidatorSubsystem>();
	
	auto [PropertyName, ExpectedResult] = PropertyNames[Index];
	FProperty* Property = TestObject->GetClass()->FindPropertyByName(PropertyName);
	FPropertyValidationResult Result = ValidationSubsystem->IsPropertyValid(TestObject, Property);

	UTEST_EQUAL("ValidationResult", Result.ValidationResult, ExpectedResult);

	return true;
}
#endif

