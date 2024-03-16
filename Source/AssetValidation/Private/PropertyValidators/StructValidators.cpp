#include "PropertyValidators/StructValidators.h"

#include "AttributeSet.h"
#include "GameplayTagContainer.h"
#include "Engine/AssetManager.h"

#include "PropertyValidators/PropertyValidation.h"
#include "Windows/Accessibility/WindowsUIAPropertyGetters.h"

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

UStructValidator_GameplayTag::UStructValidator_GameplayTag()
{
	CppType = StaticStruct<FGameplayTag>()->GetStructCPPName();
}

void UStructValidator_GameplayTag::ValidateProperty(const void* PropertyMemory, const FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	const FGameplayTag* GameplayTag = static_cast<const FGameplayTag*>(PropertyMemory);
	check(GameplayTag);

	// @todo: validate gameplay tag string
	ValidationContext.FailOnCondition(!GameplayTag->IsValid(), Property, LOCTEXT("GameplayTag", "Gameplay tag property not set"));
}

UStructValidator_GameplayTagContainer::UStructValidator_GameplayTagContainer()
{
	CppType = StaticStruct<FGameplayTagContainer>()->GetStructCPPName();
}

void UStructValidator_GameplayTagContainer::ValidateProperty(const void* PropertyMemory, const FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	const FGameplayTagContainer* GameplayTags = static_cast<const FGameplayTagContainer*>(PropertyMemory);
	check(GameplayTags);

	// @todo: validate gameplay tag string
	ValidationContext.FailOnCondition(GameplayTags->IsEmpty(), Property, LOCTEXT("GameplayTagContainer", "Gameplay tag container property is empty"));
}

UStructValidator_GameplayAttribute::UStructValidator_GameplayAttribute()
{
	CppType = StaticStruct<FGameplayAttribute>()->GetStructCPPName();
}

void UStructValidator_GameplayAttribute::ValidateProperty(const void* PropertyMemory, const FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	const FGameplayAttribute* GameplayAttribute = static_cast<const FGameplayAttribute*>(PropertyMemory);
	check(GameplayAttribute);

	// @todo: additional attribute validation in case of property renaming
	ValidationContext.FailOnCondition(!GameplayAttribute->IsValid(), Property, LOCTEXT("GameplayAttribute", "Gameplay attribute property is not set"));
}

UStructValidator_DataTableRowHandle::UStructValidator_DataTableRowHandle()
{
	CppType = StaticStruct<FDataTableRowHandle>()->GetStructCPPName();
}

void UStructValidator_DataTableRowHandle::ValidateProperty(const void* PropertyMemory, const FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	const FDataTableRowHandle* DataTableRow = static_cast<const FDataTableRowHandle*>(PropertyMemory);
	check(DataTableRow);

	if (DataTableRow->DataTable == nullptr || DataTableRow->RowName == NAME_None)
	{
		ValidationContext.PropertyFails(Property, LOCTEXT("DataTableRow_Empty", "Data table row property is not set"));
	}
	else if (!DataTableRow->DataTable->GetRowMap().Find(DataTableRow->RowName))
	{
		ValidationContext.PropertyFails(Property, LOCTEXT("DataTableRow_Invalid", "Invalid row name for data table row property"));
	}
}

UStructValidator_DirectoryPath::UStructValidator_DirectoryPath()
{
	CppType = GetNativeScriptStruct(TEXT("DirectoryPath"))->GetStructCPPName();
}

void UStructValidator_DirectoryPath::ValidateProperty(const void* PropertyMemory, const FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	const FDirectoryPath* DirectoryPath = static_cast<const FDirectoryPath*>(PropertyMemory);
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
		ValidationContext.PropertyFails(Property, LOCTEXT("DirectoryPath", "Directory path is invalid"));
	}
}

UStructValidator_FilePath::UStructValidator_FilePath()
{
	CppType = GetNativeScriptStruct(TEXT("FilePath"))->GetStructCPPName();
}

void UStructValidator_FilePath::ValidateProperty(const void* PropertyMemory, const FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	const FFilePath* FilePath = static_cast<const FFilePath*>(PropertyMemory);
	check(FilePath);
	
	if (FilePath->FilePath.IsEmpty() || !FPackageName::DoesPackageExist(FilePath->FilePath))
	{
		ValidationContext.PropertyFails(Property, LOCTEXT("FilePath", "File path is invalid"));
	}
}

UStructValidator_PrimaryAssetId::UStructValidator_PrimaryAssetId()
{
	CppType = GetNativeScriptStruct(TEXT("PrimaryAssetId"))->GetStructCPPName();
}

void UStructValidator_PrimaryAssetId::ValidateProperty(const void* PropertyMemory, const FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	const FPrimaryAssetId* AssetID = static_cast<const FPrimaryAssetId*>(PropertyMemory);
	check(AssetID);

	if (!AssetID->IsValid())
	{
		ValidationContext.PropertyFails(Property, LOCTEXT("PrimaryAsset_NotSet", "Primary asset property is not set"));
	}
	else if (UAssetManager* AssetManager = UAssetManager::GetIfInitialized())
	{
		FAssetData AssetData;
		if (!AssetManager->GetPrimaryAssetData(*AssetID, AssetData))
		{
			ValidationContext.PropertyFails(Property, LOCTEXT("PrimaryAsset_Invalid", "Primary asset property stores invalid value"));
		}
	}
}

#undef LOCTEXT_NAMESPACE
