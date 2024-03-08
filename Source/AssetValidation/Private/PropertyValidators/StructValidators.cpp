#include "PropertyValidators/StructValidators.h"

#include "AttributeSet.h"
#include "GameplayTagContainer.h"
#include "Engine/AssetManager.h"

#include "PropertyValidators/PropertyValidation.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

UScriptStruct* GetNativeScriptStruct(FName StructName)
{
	static UPackage* CoreUObjectPkg = FindObjectChecked<UPackage>(nullptr, TEXT("/Script/CoreUObject"));
	return (UScriptStruct*)StaticFindObjectFastInternal(UScriptStruct::StaticClass(), CoreUObjectPkg, StructName, false, RF_NoFlags, EInternalObjectFlags::None);
}

UStructValidator::UStructValidator()
{
	PropertyClass = FStructProperty::StaticClass();
}

bool UStructValidator::CanValidateProperty(const FProperty* Property) const
{
	return Super::CanValidateProperty(Property) && Property->GetCPPType().Equals(CppType);
}

bool UStructValidator::CanValidatePropertyValue(const FProperty* Property, const void* Value) const
{
	return Super::CanValidatePropertyValue(Property, Value) && Property->GetCPPType().Equals(CppType);
}

UStructValidator_GameplayTag::UStructValidator_GameplayTag()
{
	CppType = StaticStruct<FGameplayTag>()->GetStructCPPName();
}

void UStructValidator_GameplayTag::ValidateProperty(const void* Container, const FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	const FGameplayTag* GameplayTag = Property->ContainerPtrToValuePtr<FGameplayTag>(Container);
	check(GameplayTag);

	if (!GameplayTag->IsValid())
	{
		ValidationContext.PropertyFails(Property, LOCTEXT("AssetValidation_GameplayTag", "Gameplay tag property not set"), Property->GetDisplayNameText());
	}
}

void UStructValidator_GameplayTag::ValidatePropertyValue(const void* Value, const FProperty* ParentProperty, const FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const
{
	const FGameplayTag* GameplayTag = static_cast<const FGameplayTag*>(Value);
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

void UStructValidator_GameplayTagContainer::ValidateProperty(const void* Container, const FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	const FGameplayTagContainer* GameplayTags = Property->ContainerPtrToValuePtr<FGameplayTagContainer>(Container);
	check(GameplayTags);

	if (GameplayTags->IsEmpty())
	{
		ValidationContext.PropertyFails(Property, LOCTEXT("AssetValidation_GameplayTagContainer", "Gameplay tag container property is empty"), Property->GetDisplayNameText());
	}
}

void UStructValidator_GameplayTagContainer::ValidatePropertyValue(const void* Value, const FProperty* ParentProperty, const FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const
{
	const FGameplayTagContainer* GameplayTags = static_cast<const FGameplayTagContainer*>(Value);
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

void UStructValidator_GameplayAttribute::ValidateProperty(const void* Container, const FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	const FGameplayAttribute* GameplayAttribute = Property->ContainerPtrToValuePtr<FGameplayAttribute>(Container);
	check(GameplayAttribute);

	if (!GameplayAttribute->IsValid())
	{
		ValidationContext.PropertyFails(Property, LOCTEXT("AssetValidation_GameplayAttribute", "Gameplay attribute property is not set"), Property->GetDisplayNameText());
	}
}

void UStructValidator_GameplayAttribute::ValidatePropertyValue(const void* Value, const FProperty* ParentProperty, const FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const
{
	const FGameplayAttribute* GameplayAttribute = static_cast<const FGameplayAttribute*>(Value);
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

void UStructValidator_DataTableRowHandle::ValidateProperty(const void* Container, const FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	const FDataTableRowHandle* DataTableRow = Property->ContainerPtrToValuePtr<FDataTableRowHandle>(Container);
	check(DataTableRow);

	if (DataTableRow->DataTable == nullptr || DataTableRow->RowName == NAME_None)
	{
		ValidationContext.PropertyFails(Property, LOCTEXT("AssetValidation_DataTableRow", "Data table row property is not set"), Property->GetDisplayNameText());
	}
	else if (!DataTableRow->DataTable->GetRowNames().Contains(DataTableRow->RowName))
	{
		ValidationContext.PropertyFails(Property, LOCTEXT("AssetValidation_DataTableRowName", "Invalid row name for data table row property"), Property->GetDisplayNameText());
	}
}

void UStructValidator_DataTableRowHandle::ValidatePropertyValue(const void* Value, const FProperty* ParentProperty, const FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const
{
	const FDataTableRowHandle* DataTableRow = static_cast<const FDataTableRowHandle*>(Value);
	check(DataTableRow);

	if (DataTableRow->DataTable == nullptr || DataTableRow->RowName == NAME_None || !DataTableRow->DataTable->GetRowNames().Contains(DataTableRow->RowName))
	{
		ValidationContext.PropertyFails(ParentProperty, LOCTEXT("AssetValidation_DataTableRowValue", "Data table row is not set"));
	}
	else if (!DataTableRow->DataTable->GetRowNames().Contains(DataTableRow->RowName))
	{
		ValidationContext.PropertyFails(ParentProperty, LOCTEXT("AssetValidation_DataTableRowValueName", "Invalid row name for data table row property"));
	}
}

UStructValidator_DirectoryPath::UStructValidator_DirectoryPath()
{
	CppType = GetNativeScriptStruct(TEXT("DirectoryPath"))->GetStructCPPName();
}

void UStructValidator_DirectoryPath::ValidateProperty(const void* Container, const FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	const FDirectoryPath* DirectoryPath = Property->ContainerPtrToValuePtr<FDirectoryPath>(Container);
	check(DirectoryPath);

	FString RelativePath = DirectoryPath->Path;
	if (FPackageName::IsValidLongPackageName(DirectoryPath->Path))
	{
		FPackageName::TryConvertGameRelativePackagePathToLocalPath(DirectoryPath->Path, RelativePath);
	}

	FString FullPath = RelativePath;
	if (FPaths::IsRelative(RelativePath))
	{
		FullPath = FPaths::ConvertRelativePathToFull(RelativePath);
	}
	
	if (DirectoryPath->Path.IsEmpty() || !FPaths::DirectoryExists(FullPath))
	{
		ValidationContext.PropertyFails(Property, LOCTEXT("AssetValidation_DirectoryPath", "Directory path is invalid"));
	}
}

void UStructValidator_DirectoryPath::ValidatePropertyValue(const void* Value, const FProperty* ParentProperty, const FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const
{
	const FDirectoryPath* DirectoryPath = static_cast<const FDirectoryPath*>(Value);
	check(DirectoryPath);

	FString RelativePath = DirectoryPath->Path;
	if (FPackageName::IsValidLongPackageName(DirectoryPath->Path))
	{
		FPackageName::TryConvertGameRelativePackagePathToLocalPath(DirectoryPath->Path, RelativePath);
	}

	FString FullPath = RelativePath;
	if (FPaths::IsRelative(RelativePath))
	{
		FullPath = FPaths::ConvertRelativePathToFull(RelativePath);
	}
	
	if (DirectoryPath->Path.IsEmpty() || !FPaths::DirectoryExists(FullPath))
	{
		ValidationContext.PropertyFails(ParentProperty, LOCTEXT("AssetValidation_DirectoryPathValue", "Directory path is invalid"));
	}
}

UStructValidator_FilePath::UStructValidator_FilePath()
{
	CppType = GetNativeScriptStruct(TEXT("FilePath"))->GetStructCPPName();
}

void UStructValidator_FilePath::ValidateProperty(const void* Container, const FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	const FFilePath* FilePath = Property->ContainerPtrToValuePtr<FFilePath>(Container);
	check(FilePath);
	
	if (FilePath->FilePath.IsEmpty() || !FPackageName::DoesPackageExist(FilePath->FilePath))
	{
		ValidationContext.PropertyFails(Property, LOCTEXT("AssetValidation_FilePath", "File path is invalid"));
	}
}

void UStructValidator_FilePath::ValidatePropertyValue(const void* Value, const FProperty* ParentProperty, const FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const
{
	const FFilePath* FilePath = static_cast<const FFilePath*>(Value);
	check(FilePath);
	
	if (FilePath->FilePath.IsEmpty() || !FPackageName::DoesPackageExist(FilePath->FilePath))
	{
		ValidationContext.PropertyFails(ParentProperty, LOCTEXT("AssetValidation_FilePathValue", "File path is invalid"));
	}
}

UStructValidator_PrimaryAssetId::UStructValidator_PrimaryAssetId()
{
	CppType = GetNativeScriptStruct(TEXT("PrimaryAssetId"))->GetStructCPPName();
}

void UStructValidator_PrimaryAssetId::ValidateProperty(const void* Container, const FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	const FPrimaryAssetId* AssetID = Property->ContainerPtrToValuePtr<FPrimaryAssetId>(Container);
	check(AssetID);

	FAssetData AssetData;
	if (!AssetID->IsValid() || (UAssetManager::IsInitialized() && !UAssetManager::Get().GetPrimaryAssetData(*AssetID, AssetData)))
	{
		ValidationContext.PropertyFails(Property, LOCTEXT("AssetValidation_PrimaryAsset", "Primary asset is invalid"));
	}
}

void UStructValidator_PrimaryAssetId::ValidatePropertyValue(const void* Value, const FProperty* ParentProperty, const FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const
{
	const FPrimaryAssetId* AssetID = static_cast<const FPrimaryAssetId*>(Value);
	check(AssetID);

	FAssetData AssetData;
	if (!AssetID->IsValid() || (UAssetManager::IsInitialized() && !UAssetManager::Get().GetPrimaryAssetData(*AssetID, AssetData)))
	{
		ValidationContext.PropertyFails(ParentProperty, LOCTEXT("AssetValidation_PrimaryAssetValue", "Primary asset is invalid"));
	}
}

#undef LOCTEXT_NAMESPACE
