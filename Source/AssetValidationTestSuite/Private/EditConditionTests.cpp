#include "EditConditionTests.h"

#include "AutomationHelpers.h"
#include "PropertyValidators/PropertyValidation.h"

using UE::AssetValidation::AutomationFlags;

BEGIN_DEFINE_SPEC(FAutomationSpec_EditConditionValue, "PropertyValidation.EditConditionValue", AutomationFlags)
protected:
	void TestEditCondition(UObject* Object, FName PropertyName, bool bExpected);
	void TestValidationResult(UObject* Object, FName PropertyName, bool bExpectedConditionResult);

	UObject* TestObject = nullptr;
	UPropertyValidatorSubsystem* Subsystem = nullptr;
	static TArray<TPair<FName, bool>> PropertyNames;
END_DEFINE_SPEC(FAutomationSpec_EditConditionValue)

TArray<TPair<FName, bool>> FAutomationSpec_EditConditionValue::PropertyNames
{
		{"BoolConditionFalse",		false},
		{"BoolConditionTrue",			true},
		{"EnumConditionFalse",		false},
		{"EnumConditionTrue",			true},
		{"IntConditionFalse",			false},
		{"IntConditionTrue",			true},
		{"PointerConditionFalse",		false},
		{"PointerConditionTrue",		true},
		{"ComplexConditionFalse",		false},
		{"ComplexConditionTrue",		true}
};

void FAutomationSpec_EditConditionValue::TestEditCondition(UObject* Object, FName PropertyName, bool bExpected)
{
	UClass* Class = Object->GetClass();
	FProperty* Property = Class->FindPropertyByName(PropertyName);
	bool bActual = UE::AssetValidation::PassesEditCondition(Class, reinterpret_cast<const uint8*>(Object), Property);

	// @todo: ensure that property is suitable for validation e.g. check(ShouldValidateProperty == true)
	TestEqual(FString::Printf(TEXT("EditCondition matches for property %s"), *PropertyName.ToString()), bActual, bExpected);
}

void FAutomationSpec_EditConditionValue::TestValidationResult(UObject* Object, FName PropertyName, bool bExpectedConditionResult)
{
	FProperty* Property = Object->GetClass()->FindPropertyByName(PropertyName);

	// if EditCondition evaluates to True, this means property should be validated and validation result for these properties is always false
	EDataValidationResult Expected = bExpectedConditionResult == true ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
	FPropertyValidationResult Result = Subsystem->ValidateObjectProperty(Object, Property);

	TestEqual(FString::Printf(TEXT("Validation matches EditCondition result for property %s"), *PropertyName.ToString()), Result.ValidationResult, Expected);
}

void FAutomationSpec_EditConditionValue::Define()
{
	BeforeEach([this]
	{
		Subsystem = UPropertyValidatorSubsystem::Get();
		TestObject = NewObject<UValidationTestObject_EditCondition>(GetTransientPackage());
	});

	Describe("For Each Property", [this]
	{
		for (const auto& Pair : PropertyNames)
		{
			const auto& PropertyName = Pair.Key;
			const auto& ExpectedResult = Pair.Value;

			{
				FString Desc = FString::Printf(TEXT("Condition Value: %s"), *PropertyName.ToString());
				It(Desc, [this, PropertyName, ExpectedResult]
				{
					TestEditCondition(TestObject, PropertyName, ExpectedResult);
				});
			}

			{
				FString Desc = FString::Printf(TEXT("Validation Result: %s"), *PropertyName.ToString());
				It(Desc, [this, PropertyName, ExpectedResult]
				{
					TestValidationResult(TestObject, PropertyName, ExpectedResult);
				});
			}
		}
	});

	It("Total Errors", [this]
	{
		FPropertyValidationResult Result = Subsystem->ValidateObject(TestObject);
		TestEqual("ValidationResult", Result.ValidationResult, EDataValidationResult::Invalid);
		TestEqual("NumErrors", Result.Errors.Num(), 5);
	});
	
	AfterEach([this]
	{
		Subsystem	= nullptr;
		TestObject	= nullptr;
	});
}
