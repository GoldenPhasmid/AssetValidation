#include "EditConditionTests.h"

#include "AutomationFlags.h"
#include "PropertyValidators/PropertyValidation.h"

using UE::AssetValidation::AutomationFlags;

static TArray<TPair<FName, bool>> PropertyNames
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


BEGIN_DEFINE_SPEC(FAutomationSpec_EditConditionValue, "PropertyValidation.EditConditionValue", AutomationFlags)
	void TestEditCondition(UObject* Object, FName PropertyName, bool bExpected);
	UObject* TestObject = nullptr;
END_DEFINE_SPEC(FAutomationSpec_EditConditionValue)

void FAutomationSpec_EditConditionValue::TestEditCondition(UObject* Object, FName PropertyName, bool bExpected)
{
	UClass* Class = Object->GetClass();
	FProperty* Property = Class->FindPropertyByName(PropertyName);
	bool bActual = UE::AssetValidation::PassesEditCondition(Class, reinterpret_cast<const uint8*>(Object), Property);

	TestEqual(FString::Printf(TEXT("EditCondition matches for property %s"), *PropertyName.ToString()), bActual, bExpected);
}

void FAutomationSpec_EditConditionValue::Define()
{
	BeforeEach([this]
	{
		TestObject = NewObject<UValidationTestObject_EditCondition>(GetTransientPackage());
	});

	Describe("For Each Property", [this]
	{
		for (const auto& Pair : PropertyNames)
		{
			const auto& PropertyName = Pair.Key;
			const auto& ExpectedResult = Pair.Value;
			
			It(FString::Printf(TEXT("%s"), *PropertyName.ToString()), [this, PropertyName, ExpectedResult]
			{
				TestEditCondition(TestObject, PropertyName, ExpectedResult);
			});
		}
	});
	
	AfterEach([this]
	{
		TestObject = nullptr;
	});
}
