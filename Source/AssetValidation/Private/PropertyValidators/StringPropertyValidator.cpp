#include "PropertyValidators/StringPropertyValidator.h"

#include "PropertyValidators/PropertyValidation.h"

#define LOCTEXT_NAMESPACE "AssetValidation"


UStringPropertyValidator::UStringPropertyValidator()
{
	PropertyClass = FStrProperty::StaticClass();
}

const FText EmptyStringFailure = LOCTEXT("AssetValidation_StrProperty", "String property not set");

void UStringPropertyValidator::ValidateProperty(const void* Container, const FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	const FString* Str = Property->ContainerPtrToValuePtr<FString>(Container);
	check(Str);

	ValidationContext.FailOnCondition(Str->IsEmpty(), Property, EmptyStringFailure, Property->GetDisplayNameText());
}

void UStringPropertyValidator::ValidatePropertyValue(const void* Value, const FProperty* ParentProperty, const FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const
{
	const FString* Str = ValueProperty->ContainerPtrToValuePtr<FString>(Value);
	check(Str);

	ValidationContext.FailOnCondition(Str->IsEmpty(), ParentProperty, EmptyStringFailure);
}

#undef LOCTEXT_NAMESPACE
