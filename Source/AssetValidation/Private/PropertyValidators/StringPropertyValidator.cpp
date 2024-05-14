#include "PropertyValidators/StringPropertyValidator.h"

#include "PropertyValidators/PropertyValidation.h"

UStringPropertyValidator::UStringPropertyValidator()
{
	PropertyClass = FStrProperty::StaticClass();
}

void UStringPropertyValidator::ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FMetaDataSource& MetaData, FPropertyValidationContext& ValidationContext) const
{
	const FString* Str = GetPropertyValuePtr<FStrProperty>(PropertyMemory, Property);
	check(Str);

	ValidationContext.FailOnCondition(Str->IsEmpty(), Property, NSLOCTEXT("AssetValidation", "StrProperty", "String property not set."));
}

