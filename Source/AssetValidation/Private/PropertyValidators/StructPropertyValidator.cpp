#include "PropertyValidators/StructPropertyValidator.h"

#include "AssetValidationStatics.h"
#include "PropertyValidatorSubsystem.h"

bool UStructPropertyValidator::CanValidateProperty(FProperty* Property) const
{
	return AssetValidationStatics::CanValidateProperty(Property) && Property->IsA<FStructProperty>();
}

bool UStructPropertyValidator::CanValidatePropertyValue(FProperty* ParentProperty, FProperty* ValueProperty) const
{
	return AssetValidationStatics::CanValidateProperty(ValueProperty) && ValueProperty->IsA<FStructProperty>();
}

void UStructPropertyValidator::ValidateProperty(FProperty* Property, void* BasePointer, FPropertyValidationResult& OutValidationResult) const
{
	UPropertyValidatorSubsystem* PropertyValidators = GEditor->GetEditorSubsystem<UPropertyValidatorSubsystem>();
	check(PropertyValidators);
	
	const FStructProperty* StructProperty = CastFieldChecked<FStructProperty>(Property);

	void* ContainerPtr = StructProperty->ContainerPtrToValuePtr<void*>(BasePointer);
	PropertyValidators->IsPropertyContainerValid(ContainerPtr, StructProperty->Struct, OutValidationResult);
}

void UStructPropertyValidator::ValidatePropertyValue(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationResult& OutValidationResult) const
{
	UPropertyValidatorSubsystem* PropertyValidators = GEditor->GetEditorSubsystem<UPropertyValidatorSubsystem>();
	check(PropertyValidators);

	PropertyValidators->IsPropertyValueValid(Value, ParentProperty, ValueProperty, OutValidationResult);
}
