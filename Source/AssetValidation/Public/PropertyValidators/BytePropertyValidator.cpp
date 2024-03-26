#include "BytePropertyValidator.h"

#include "PropertyValidation.h"

UBytePropertyValidator::UBytePropertyValidator()
{
	PropertyClass = FByteProperty::StaticClass();
}

bool UBytePropertyValidator::CanValidateProperty(const FProperty* Property) const
{
	if (Super::CanValidateProperty(Property))
	{
		const FByteProperty* ByteProperty = CastFieldChecked<FByteProperty>(Property);
		return ByteProperty->IsEnum();
	}

	return false;
}

void UBytePropertyValidator::ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	const uint8* ValuePtr = GetPropertyValuePtr<FByteProperty>(PropertyMemory, Property);
	check(ValuePtr);
	
	ValidationContext.FailOnCondition(*ValuePtr == 0, Property, NSLOCTEXT("AssetValidation", "ByteProperty", "Enum property not set"));
}
