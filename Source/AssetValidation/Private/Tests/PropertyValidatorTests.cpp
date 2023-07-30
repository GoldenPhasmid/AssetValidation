#include "PropertyValidatorTests.h"

#include "Misc/AutomationTest.h"
#include "PropertyValidatorSubsystem.h"
#include "PropertyValidators/PropertyValidation.h"

static TArray<TPair<FName, EDataValidationResult>> PropertyNames
{
	{"NoMetaProperty",			EDataValidationResult::Valid},
	{"NoMetaEditableProperty",	EDataValidationResult::Valid},
	{"NonEditableProperty",		EDataValidationResult::Valid},
	{"TransientProperty",			EDataValidationResult::Valid},
	{"TransientEditableProperty",	EDataValidationResult::Valid},
	{"EditDefaultOnlyProperty",	EDataValidationResult::Invalid},
	{"EditAnywhereProperty",		EDataValidationResult::Invalid},
	{"EditInstanceOnlyProperty",	EDataValidationResult::Invalid}
};

BEGIN_DEFINE_SPEC(FAutomationSpec_ValidationConditions, "Editor.PropertyValidation.Conditions", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)
	UObject* TestObject;
	UPropertyValidatorSubsystem* ValidationSubsystem;
END_DEFINE_SPEC(FAutomationSpec_ValidationConditions)
void FAutomationSpec_ValidationConditions::Define()
{
	BeforeEach([this]()
	{
		TestObject = NewObject<UValidationTestObject_ValidationConditions>(GetTransientPackage());
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
#if 0
		TestObject->RemoveFromRoot();
		TestObject = nullptr;
#endif
		ValidationSubsystem = nullptr;
	});
}

BEGIN_DEFINE_SPEC(FAutomationSpec_ContainerProperties, "Editor.PropertyValidation", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)
	UValidationTestObject_ContainerProperties* TestObject;
	UPropertyValidatorSubsystem* ValidationSubsystem;
	FProperty* TestProperty;
END_DEFINE_SPEC(FAutomationSpec_ContainerProperties)
void FAutomationSpec_ContainerProperties::Define()
{
	BeforeEach([this]()
	{
		TestObject = NewObject<UValidationTestObject_ContainerProperties>(GetTransientPackage());
		TestObject->AddToRoot();
		
		ValidationSubsystem = GEditor->GetEditorSubsystem<UPropertyValidatorSubsystem>();
	});

	Describe("ArrayProperty", [this]()
	{
		BeforeEach([this]()
		{
			TestProperty = TestObject->GetClass()->FindPropertyByName("ObjectArray");
		});
		
		It("empty array should be valid", [this]()
		{
			FPropertyValidationResult Result = ValidationSubsystem->IsPropertyValid(TestObject, TestProperty);
			TestEqual("ValidationResult", Result.ValidationResult, EDataValidationResult::Valid);
		});

		It("array with at least one null object should be invalid", [this]()
		{
			TestObject->ObjectArray.Append({NewObject<UEmptyObject>(), nullptr, nullptr});
			
			FPropertyValidationResult Result = ValidationSubsystem->IsPropertyValid(TestObject, TestProperty);
			TestEqual("ValidationResult", Result.ValidationResult, EDataValidationResult::Invalid);
			TestEqual("NumErrors", Result.Errors.Num(), 2);
		});

		// it is questionable whether we should check for this. Not picked up by default because IsValidLowLevel doesn't check for PendingKill? weird
		xIt("array with at least one garbage object should be invalid", [this]()
		{
			UObject* GarbageObject = NewObject<UEmptyObject>();
			GarbageObject->MarkAsGarbage();
			TestObject->ObjectArray.Append({NewObject<UEmptyObject>(), NewObject<UEmptyObject>(), GarbageObject});

			FPropertyValidationResult Result = ValidationSubsystem->IsPropertyValid(TestObject, TestProperty);
			TestEqual("ValidationResult", Result.ValidationResult, EDataValidationResult::Invalid);
			TestEqual("NumErrors", Result.Errors.Num(), 1);
		});

		It("array with valid objects should be valid", [this]()
		{
			TestObject->ObjectArray.Append({NewObject<UEmptyObject>(), NewObject<UEmptyObject>(), NewObject<UEmptyObject>()});

			FPropertyValidationResult Result = ValidationSubsystem->IsPropertyValid(TestObject, TestProperty);
			TestEqual("ValidationResult", Result.ValidationResult, EDataValidationResult::Valid);
		});

		AfterEach([this]()
		{
			TestProperty = nullptr;
		});
	});

	Describe("SetProperty", [this]()
	{
		BeforeEach([this]()
		{
			TestProperty = TestObject->GetClass()->FindPropertyByName("ObjectSet");
		});

		It("empty set should be valid", [this]()
		{
			FPropertyValidationResult Result = ValidationSubsystem->IsPropertyValid(TestObject, TestProperty);
			TestEqual("ValidationResult", Result.ValidationResult, EDataValidationResult::Valid);
		});

		It("set with a null object should be invalid", [this]()
		{
			TestObject->ObjectSet.Append({NewObject<UEmptyObject>(), nullptr});

			FPropertyValidationResult Result = ValidationSubsystem->IsPropertyValid(TestObject, TestProperty);
			TestEqual("ValidationResult", Result.ValidationResult, EDataValidationResult::Invalid);
			TestEqual("NumErrors", Result.Errors.Num(), 1);
		});

		// it is questionable whether we should check for this. Not picked up by default because IsValidLowLevel doesn't check for PendingKill? weirdx
		xIt("set with a garbage object should be invalid", [this]()
		{
			UObject* GarbageObject = NewObject<UEmptyObject>();
			GarbageObject->MarkAsGarbage();
			TestObject->ObjectSet.Append({NewObject<UEmptyObject>(), NewObject<UEmptyObject>(), GarbageObject});

			FPropertyValidationResult Result = ValidationSubsystem->IsPropertyValid(TestObject, TestProperty);
			TestEqual("ValidationResult", Result.ValidationResult, EDataValidationResult::Invalid);
			TestEqual("NumErrors", Result.Errors.Num(), 1);
		});

		It("set with valid objects should be valid", [this]()
		{
			TestObject->ObjectSet.Append({NewObject<UEmptyObject>(), NewObject<UEmptyObject>(), NewObject<UEmptyObject>()});

			FPropertyValidationResult Result = ValidationSubsystem->IsPropertyValid(TestObject, TestProperty);
			TestEqual("ValidationResult", Result.ValidationResult, EDataValidationResult::Valid);
		});

		AfterEach([this]()
		{
			TestProperty = nullptr;
		});
	});

	Describe("MapProperty", [this]()
	{
		BeforeEach([this]()
		{
			TestProperty = TestObject->GetClass()->FindPropertyByName("ObjectMap");
		});

		It("empty map should be valid", [this]()
		{
			FPropertyValidationResult Result = ValidationSubsystem->IsPropertyValid(TestObject, TestProperty);
			TestEqual("ValidationResult", Result.ValidationResult, EDataValidationResult::Valid);
		});

		It("map with at least one null object key or null value should be invalid", [this]()
		{
			TestObject->ObjectMap.Add(nullptr, NewObject<UEmptyObject>());

			FPropertyValidationResult Result = ValidationSubsystem->IsPropertyValid(TestObject, TestProperty);
			TestEqual("ValidationResult", Result.ValidationResult, EDataValidationResult::Invalid);
			TestEqual("NumErrors", Result.Errors.Num(), 1);

			TestObject->ObjectMap.Add(NewObject<UEmptyObject>(), nullptr);

			Result = ValidationSubsystem->IsPropertyValid(TestObject, TestProperty);
			TestEqual("ValidationResult", Result.ValidationResult, EDataValidationResult::Invalid);
			TestEqual("NumErrors", Result.Errors.Num(), 2);
		});

		It("map with valid keys and values should be valid", [this]()
		{
			TestObject->ObjectMap.Add(NewObject<UEmptyObject>(), NewObject<UEmptyObject>());
			TestObject->ObjectMap.Add(NewObject<UEmptyObject>(), NewObject<UEmptyObject>());

			FPropertyValidationResult Result = ValidationSubsystem->IsPropertyValid(TestObject, TestProperty);
			TestEqual("ValidationResult", Result.ValidationResult, EDataValidationResult::Valid);
		});

		AfterEach([this]()
		{
			TestProperty = nullptr;
		});
	});
	
	AfterEach([this]()
	{
		TestObject->ObjectArray.Empty();
		TestObject->ObjectMap.Empty();
		TestObject->ObjectSet.Empty();
		
		TestObject->RemoveFromRoot();
		TestObject = nullptr;
		ValidationSubsystem = nullptr;
	});
}

BEGIN_DEFINE_SPEC(FAutomationSpec_ValidateMetas, "Editor.PropertyValidation.ValidationMetas", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)
	UValidationTestObject_ValidationMetas* TestObject;
	UPropertyValidatorSubsystem* ValidationSubsystem;
	FProperty* TestProperty;
END_DEFINE_SPEC(FAutomationSpec_ValidateMetas)
void FAutomationSpec_ValidateMetas::Define()
{
	BeforeEach([this]()
	{
		TestObject = NewObject<UValidationTestObject_ValidationMetas>(GetTransientPackage());
		TestObject->AddToRoot();
			
		ValidationSubsystem = GEditor->GetEditorSubsystem<UPropertyValidatorSubsystem>();
	});

	It("property with Validate meta should be validated", [this]()
	{
		FProperty* Property = TestObject->GetClass()->FindPropertyByName("Validate");

		FPropertyValidationResult Result = ValidationSubsystem->IsPropertyValid(TestObject, Property);
		TestEqual("ValidationResult", Result.ValidationResult, EDataValidationResult::Invalid);
	});

	It("property with ValidateRecursive meta should validate nested properties", [this]()
	{
		UNestedObject* NestedObject = NewObject<UNestedObject>();
		FPropertyValidationResult NestedResult = ValidationSubsystem->IsPropertyContainerValid(NestedObject);

		FProperty* Property = TestObject->GetClass()->FindPropertyByName("ValidateRecursive");
		FPropertyValidationResult Result = ValidationSubsystem->IsPropertyValid(TestObject, Property);
		TestEqual("ValidationResult", Result.ValidationResult, EDataValidationResult::Valid);

		TestObject->ValidateRecursive = NestedObject;
		Result = ValidationSubsystem->IsPropertyValid(TestObject, Property);
		
		TestEqual("ValidationResult", Result.ValidationResult, EDataValidationResult::Invalid);
		TestEqual("NumErrors", Result.Errors.Num(), NestedResult.Errors.Num());
	});

	It("property with custom FailureMessage", [this]()
	{
		FProperty* Property = TestObject->GetClass()->FindPropertyByName("ValidateWithCustomMessage");
		FPropertyValidationResult Result = ValidationSubsystem->IsPropertyValid(TestObject, Property);

		FString CustomMessage = Property->GetMetaData(ValidationNames::ValidationFailureMessage);
		TestEqual("ValidationResult", Result.ValidationResult, EDataValidationResult::Invalid);
		TestEqual("NumErrors", Result.Errors.Num(), 1);
		TestTrue("Message", Result.Errors[0].ToString().Contains(CustomMessage));
	});

	It("map property with ValidateKey meta", [this]()
	{
		TestObject->ValidateKey.Add(NewObject<UEmptyObject>(), nullptr);
		TestObject->ValidateKey.Add(NewObject<UEmptyObject>(), nullptr);
		TestObject->ValidateKey.Add(nullptr, nullptr);

		FProperty* Property = TestObject->GetClass()->FindPropertyByName("ValidateKey");
		FPropertyValidationResult Result = ValidationSubsystem->IsPropertyValid(TestObject, Property);

		TestEqual("ValidationResult", Result.ValidationResult, EDataValidationResult::Invalid);
		TestEqual("NumErrors", Result.Errors.Num(), 1);
	});

	It("map property with ValidateValue meta", [this]()
	{
		TestObject->ValidateValue.Add(NewObject<UEmptyObject>(), nullptr);
		TestObject->ValidateValue.Add(NewObject<UEmptyObject>(), nullptr);
		TestObject->ValidateValue.Add(nullptr, nullptr);

		FProperty* Property = TestObject->GetClass()->FindPropertyByName("ValidateValue");
		FPropertyValidationResult Result = ValidationSubsystem->IsPropertyValid(TestObject, Property);

		TestEqual("ValidationResult", Result.ValidationResult, EDataValidationResult::Invalid);
		TestEqual("NumErrors", Result.Errors.Num(), 3);
	});
	
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

