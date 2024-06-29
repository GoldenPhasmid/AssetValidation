#include "ContainerValidators/ArrayContainerValidator.h"

#include "PropertyValidatorSubsystem.h"
#include "Editor/MetaDataSource.h"
#include "PropertyValidators/PropertyValidation.h"

UArrayContainerValidator::UArrayContainerValidator()
{
	Descriptor = FArrayProperty::StaticClass();
}

bool UArrayContainerValidator::CanValidateProperty(const FProperty* Property, FMetaDataSource& MetaData) const
{
	if (Super::CanValidateProperty(Property, MetaData))
	{
		// cast checked because Super call checks for property compatibility
		const FArrayProperty* ArrayProperty = CastFieldChecked<FArrayProperty>(Property);
		const FProperty* InnerProperty = ArrayProperty->Inner;
		
		if (InnerProperty->IsA<FStructProperty>())
		{
			// allow struct properties to be recursively validated without meta specifier
			return true;
		}

		const UPropertyValidatorSubsystem* ValidatorSubsystem = UPropertyValidatorSubsystem::Get();
		if (MetaData.HasMetaData(UE::AssetValidation::Validate) && ValidatorSubsystem->HasValidatorForPropertyType(InnerProperty))
		{
			// in general, array contents should be validated if Validate meta is present
			return true;
		}
	}

	return false;
}

void UArrayContainerValidator::ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FMetaDataSource& MetaData, FPropertyValidationContext& ValidationContext) const
{
	const FArrayProperty* ArrayProperty = CastFieldChecked<FArrayProperty>(Property);
	const FProperty* ValueProperty = ArrayProperty->Inner;

	const FScriptArray* Array = ArrayProperty->GetPropertyValuePtr(PropertyMemory);

	const uint32 Num = Array->Num();
	const uint32 Stride = ValueProperty->ElementSize;

	const uint8* Data = static_cast<const uint8*>(Array->GetData());
	for (uint32 Index = 0; Index < Num; ++Index)
	{
		// add scoped array property prefix
		const FString Prefix = UE::AssetValidation::GetPropertyDisplayName(ArrayProperty) + "[" + FString::FromInt(Index) + "]";
		FPropertyValidationContext::FConditionalPrefix ScopedPrefix{ValidationContext, Prefix, UE::AssetValidation::IsBlueprintVisibleProperty(ArrayProperty)};
		// validate property value
		ValidationContext.IsPropertyValueValid(Data, ValueProperty, MetaData);

		Data += Stride;
	}
}
