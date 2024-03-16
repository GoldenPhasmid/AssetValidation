#include "PropertyValidators/StringPropertyValidator.h"

#include "PropertyValidators/PropertyValidation.h"

UStringPropertyValidator::UStringPropertyValidator()
{
	PropertyClass = FStrProperty::StaticClass();
}

void UStringPropertyValidator::ValidateProperty(const void* PropertyMemory, const FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	const FString* Str = GetPropertyValuePtr<FStrProperty>(PropertyMemory, Property);
	check(Str);

	ValidationContext.FailOnCondition(Str->IsEmpty(), Property, NSLOCTEXT("AssetValidation", "StrProperty", "String property not set"));
}

