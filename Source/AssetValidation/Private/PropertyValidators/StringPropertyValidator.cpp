#include "PropertyValidators/StringPropertyValidator.h"

#include "AssetValidationStatics.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

bool UStringPropertyValidator::CanValidateProperty(FProperty* Property) const
{
	return AssetValidationStatics::CanValidateProperty(Property) && Property->IsA<FStrProperty>();
}

bool UStringPropertyValidator::CanValidatePropertyValue(FProperty* ParentProperty, FProperty* ValueProperty) const
{
	return AssetValidationStatics::CanValidateProperty(ParentProperty) && ValueProperty->IsA<FStrProperty>();
}

void UStringPropertyValidator::ValidateProperty(FProperty* Property, void* BasePointer, FPropertyValidationResult& OutValidationResult) const
{
	const FString* Str = Property->ContainerPtrToValuePtr<FString>(BasePointer);
	check(Str);

	if (Str->IsEmpty())
	{
		OutValidationResult.PropertyFails(Property, LOCTEXT("AssetValidation_StrProperty", "String property not set"));
	}
}

void UStringPropertyValidator::ValidatePropertyValue(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationResult& OutValidationResult) const
{
	const FString* Str = ValueProperty->ContainerPtrToValuePtr<FString>(Value);
	check(Str);

	if (Str->IsEmpty())
	{
		OutValidationResult.PropertyFails(ValueProperty, LOCTEXT("AssetValidation_StrPropertyValue", "String value not set"));
	}
}

#undef LOCTEXT_NAMESPACE
