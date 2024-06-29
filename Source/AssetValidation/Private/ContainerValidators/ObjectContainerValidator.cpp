#include "ContainerValidators/ObjectContainerValidator.h"

#include "PropertyValidators/PropertyValidation.h"
#include "Editor/MetaDataSource.h"

UObjectContainerValidator::UObjectContainerValidator()
{
	Descriptor = FObjectPropertyBase::StaticClass();
}

bool UObjectContainerValidator::CanValidateProperty(const FProperty* Property, FMetaDataSource& MetaData) const
{
	return Super::CanValidateProperty(Property, MetaData) && MetaData.HasMetaData(UE::AssetValidation::ValidateRecursive);
}

void UObjectContainerValidator::ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FMetaDataSource& MetaData, FPropertyValidationContext& ValidationContext) const
{
	const FObjectPropertyBase* ObjectProperty = CastFieldChecked<FObjectPropertyBase>(Property);

	if (const UObject* Object = ObjectProperty->LoadObjectPropertyValue(PropertyMemory))
	{
		FPropertyValidationContext::FScopedSourceObject ScopedObject{ValidationContext, Object};
		
		// push either property prefix or object prefix, depending on whether property is visible
		const FString ObjectName = UE::AssetValidation::ResolveObjectDisplayName(Object, ValidationContext);
		if (UE::AssetValidation::IsBlueprintVisibleProperty(ObjectProperty))
		{
			// push property prefix
			ValidationContext.PushPrefix(UE::AssetValidation::GetPropertyDisplayName(ObjectProperty)/* + TEXT(".") + ObjectDisplayName*/);
		}
		else
		{
			// push object prefix
			ValidationContext.PushPrefix(ObjectName);
		}
		
		FPropertyValidationContext::FScopedSourceObject ScopedSource{ValidationContext, Object};
		// validate underlying object recursively
		ValidationContext.IsPropertyContainerValid(reinterpret_cast<const uint8*>(Object), Object->GetClass());
		ValidationContext.PopPrefix();
	}
}
