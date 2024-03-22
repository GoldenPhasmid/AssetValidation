#include "ArrayContainerValidator.h"

#include "PropertyValidationSettings.h"
#include "PropertyValidatorSubsystem.h"
#include "PropertyValidators/PropertyValidation.h"

UArrayContainerValidator::UArrayContainerValidator()
{
	PropertyClass = FArrayProperty::StaticClass();
}

bool UArrayContainerValidator::CanValidateProperty(const FProperty* Property) const
{
	if (Super::CanValidateProperty(Property))
	{
		// cast checked because Super call checks for property compatibility
		const FArrayProperty* ArrayProperty = CastFieldChecked<FArrayProperty>(Property);
		const FProperty* InnerProperty = ArrayProperty->Inner;
		
		if (UPropertyValidationSettings::Get()->bAutoValidateStructInnerProperties && InnerProperty->IsA<FStructProperty>())
		{
			// allow struct properties to be recursively validated without meta specifier
			return true;
		}

		const UPropertyValidatorSubsystem* ValidatorSubsystem = UPropertyValidatorSubsystem::Get();
		if (Property->HasMetaData(UE::AssetValidation::Validate) && ValidatorSubsystem->HasValidatorForPropertyType(InnerProperty))
		{
			// in general, array contents should be validated if Validate meta is present
			return true;
		}
	}

	return false;
}

void UArrayContainerValidator::ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	const FArrayProperty* ArrayProperty = CastFieldChecked<FArrayProperty>(Property);
	const FProperty* ValueProperty = ArrayProperty->Inner;

	const FScriptArray* Array = ArrayProperty->GetPropertyValuePtr(PropertyMemory);

	const uint32 Num = Array->Num();
	const uint32 Stride = ValueProperty->ElementSize;

	const uint8* Data = static_cast<const uint8*>(Array->GetData());
	for (uint32 Index = 0; Index < Num; ++Index)
	{
		ValidationContext.PushPrefix(Property->GetName() + "[" + FString::FromInt(Index) + "]");
		// validate property value
		ValidationContext.IsPropertyValueValid(Data, ArrayProperty, ValueProperty);
		ValidationContext.PopPrefix();

		Data += Stride;
	}
}
