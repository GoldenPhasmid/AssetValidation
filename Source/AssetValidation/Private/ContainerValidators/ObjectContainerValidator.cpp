#include "ObjectContainerValidator.h"

#include "BehaviorTree/BTNode.h"
#include "Components/Widget.h"

#include "PropertyValidators/PropertyValidation.h"
#include "Editor/MetaDataSource.h"

UObjectContainerValidator::UObjectContainerValidator()
{
	PropertyClass = FObjectPropertyBase::StaticClass();
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
		const FString ObjectName = FindObjectDisplayName(Object);
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

FString UObjectContainerValidator::FindObjectDisplayName(const UObject* Object) const
{
	if (const UBTNode* BTNode = Cast<UBTNode>(Object))
	{
		return BTNode->NodeName.IsEmpty() ? Object->GetClass()->GetName() : BTNode->NodeName;
	}
	else if (const AActor* Actor = Cast<AActor>(Object))
	{
		return Actor->GetActorNameOrLabel();
	}
	else if (const UWidget* Widget = Cast<UWidget>(Object))
	{
		if (FString DisplayLabel = Widget->GetDisplayLabel(); !DisplayLabel.IsEmpty())
		{
			return DisplayLabel;
		}
		
		return Widget->GetName();
	}

	return Object->GetName();
}
