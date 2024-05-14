#include "PropertyValidators/BytePropertyValidator.h"

#include "PropertyValidators/PropertyValidation.h"

UBytePropertyValidator::UBytePropertyValidator()
{
	PropertyClass = FByteProperty::StaticClass();
}

bool UBytePropertyValidator::CanValidateProperty(const FProperty* Property, FMetaDataSource& MetaData) const
{
	if (Super::CanValidateProperty(Property, MetaData))
	{
		const FByteProperty* ByteProperty = CastFieldChecked<FByteProperty>(Property);
		return ByteProperty->IsEnum();
	}

	return false;
}

void UBytePropertyValidator::ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FMetaDataSource& MetaData, FPropertyValidationContext& ValidationContext) const
{
	const uint8* ValuePtr = GetPropertyValuePtr<FByteProperty>(PropertyMemory, Property);
	check(ValuePtr);
	
	ValidationContext.FailOnCondition(*ValuePtr == 0, Property, NSLOCTEXT("AssetValidation", "ByteProperty", "Enum property not set."));
}
