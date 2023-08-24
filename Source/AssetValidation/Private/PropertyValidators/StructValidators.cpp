#include "PropertyValidators/StructValidators.h"

#include "AttributeSet.h"
#include "GameplayTagContainer.h"

#include "PropertyValidators/PropertyValidation.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

UStructValidator::UStructValidator()
{
	PropertyClass = FStructProperty::StaticClass();
}

bool UStructValidator::CanValidateProperty(FProperty* Property) const
{
	return Super::CanValidateProperty(Property) && Property->GetCPPType().Equals(CppType);
}

bool UStructValidator::CanValidatePropertyValue(FProperty* Property, void* Value) const
{
	return Super::CanValidatePropertyValue(Property, Value) && Property->GetCPPType().Equals(CppType);
}

UStructValidator_GameplayTag::UStructValidator_GameplayTag()
{
	CppType = StaticStruct<FGameplayTag>()->GetStructCPPName();
}

void UStructValidator_GameplayTag::ValidateProperty(void* Container, FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	FGameplayTag* GameplayTag = Property->ContainerPtrToValuePtr<FGameplayTag>(Container);
	check(GameplayTag);

	if (!GameplayTag->IsValid())
	{
		ValidationContext.PropertyFails(Property, LOCTEXT("AssetValidation_GameplayTag", "Gameplay tag property not set"), Property->GetDisplayNameText());
	}
}

void UStructValidator_GameplayTag::ValidatePropertyValue(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const
{
	FGameplayTag* GameplayTag = static_cast<FGameplayTag*>(Value);
	check(GameplayTag);

	if (!GameplayTag->IsValid())
	{
		ValidationContext.PropertyFails(ParentProperty, LOCTEXT("AssetValidation_GameplayTagValue", "Gameplay tag not set"));
	}
}

UStructValidator_GameplayTagContainer::UStructValidator_GameplayTagContainer()
{
	CppType = StaticStruct<FGameplayTagContainer>()->GetStructCPPName();
}

void UStructValidator_GameplayTagContainer::ValidateProperty(void* Container, FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	FGameplayTagContainer* GameplayTags = Property->ContainerPtrToValuePtr<FGameplayTagContainer>(Container);
	check(GameplayTags);

	if (GameplayTags->IsEmpty())
	{
		ValidationContext.PropertyFails(Property, LOCTEXT("AssetValidation_GameplayTagContainer", "Gameplay tag container property is empty"), Property->GetDisplayNameText());
	}
}

void UStructValidator_GameplayTagContainer::ValidatePropertyValue(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const
{
	FGameplayTagContainer* GameplayTags = static_cast<FGameplayTagContainer*>(Value);
	check(GameplayTags);

	if (GameplayTags->IsEmpty())
	{
		ValidationContext.PropertyFails(ParentProperty, LOCTEXT("AssetValidation_GameplayTagContainerValue", "Gameplay tag container is empty"));
	}
}

UStructValidator_GameplayAttribute::UStructValidator_GameplayAttribute()
{
	CppType = StaticStruct<FGameplayAttribute>()->GetStructCPPName();
}

void UStructValidator_GameplayAttribute::ValidateProperty(void* Container, FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	FGameplayAttribute* GameplayAttribute = Property->ContainerPtrToValuePtr<FGameplayAttribute>(Container);
	check(GameplayAttribute);

	if (!GameplayAttribute->IsValid())
	{
		ValidationContext.PropertyFails(Property, LOCTEXT("AssetValidation_GameplayAttribute", "Gameplay attribute property is not set"), Property->GetDisplayNameText());
	}
}

void UStructValidator_GameplayAttribute::ValidatePropertyValue(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const
{
	FGameplayAttribute* GameplayAttribute = static_cast<FGameplayAttribute*>(Value);
	check(GameplayAttribute);

	if (!GameplayAttribute->IsValid())
	{
		ValidationContext.PropertyFails(ParentProperty, LOCTEXT("AssetValidation_GameplayAttributeValue", "Gameplay attribute is not set"));
	}
}

UStructValidator_DataTableRowHandle::UStructValidator_DataTableRowHandle()
{
	CppType = StaticStruct<FDataTableRowHandle>()->GetStructCPPName();
}

void UStructValidator_DataTableRowHandle::ValidateProperty(void* Container, FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	FDataTableRowHandle* DataTableRow = Property->ContainerPtrToValuePtr<FDataTableRowHandle>(Container);
	check(DataTableRow);

	if (DataTableRow->IsNull())
	{
		ValidationContext.PropertyFails(Property, LOCTEXT("AssetValidation_DataTableRow", "Data table row property is not set"), Property->GetDisplayNameText());
	}
}

void UStructValidator_DataTableRowHandle::ValidatePropertyValue(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const
{
	FDataTableRowHandle* DataTableRow = static_cast<FDataTableRowHandle*>(Value);
	check(DataTableRow);

	if (DataTableRow->IsNull())
	{
		ValidationContext.PropertyFails(ParentProperty, LOCTEXT("AssetValidation_DataTableRowValue", "Data table row is not set"));
	}
}

UStructValidator_DirectoryPath::UStructValidator_DirectoryPath()
{
	static UPackage* CoreUObjectPkg = FindObjectChecked<UPackage>(nullptr, TEXT("/Script/CoreUObject"));

	UScriptStruct* Result = (UScriptStruct*)StaticFindObjectFastInternal(UScriptStruct::StaticClass(), CoreUObjectPkg, TEXT("DirectoryPath"), false, RF_NoFlags, EInternalObjectFlags::None);
	check(Result);
	
	CppType = Result->GetStructCPPName();
}

void UStructValidator_DirectoryPath::ValidateProperty(void* Container, FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	FDirectoryPath* DirectoryPath = Property->ContainerPtrToValuePtr<FDirectoryPath>(Container);
	check(DirectoryPath);

	if (DirectoryPath->Path.IsEmpty() || !IFileManager::Get().DirectoryExists(*DirectoryPath->Path))
	{
		ValidationContext.PropertyFails(Property, LOCTEXT("AssetValidation_DirectoryPath", "Directory path is invalid"));
	}
}

void UStructValidator_DirectoryPath::ValidatePropertyValue(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const
{
	FDirectoryPath* DirectoryPath = static_cast<FDirectoryPath*>(Value);
	check(DirectoryPath);

	if (DirectoryPath->Path.IsEmpty() || !IFileManager::Get().DirectoryExists(*DirectoryPath->Path))
	{
		ValidationContext.PropertyFails(ParentProperty, LOCTEXT("AssetValidation_DirectoryPath", "Directory path is invalid"));
	}
}

#undef LOCTEXT_NAMESPACE
