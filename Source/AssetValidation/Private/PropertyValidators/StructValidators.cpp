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

void ValidateGameplayTag(const FGameplayTag& Tag, const FProperty* Property, FPropertyValidationContext& ValidationContext)
{
	const FGameplayTag NewTag = FGameplayTag::RequestGameplayTag(Tag.GetTagName());
	if (!NewTag.IsValid())
	{
		ValidationContext.PropertyFails(Property, FText::Format(LOCTEXT("GameplayTag_Invalid", "Gameplay tag with name %s no longer exists."), FText::FromName(NewTag.GetTagName())));
	}
}

void UStructValidator_GameplayTag::ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	const FGameplayTag* GameplayTag = ConvertStructMemory<FGameplayTag>(PropertyMemory);
	check(GameplayTag);

	if (!GameplayTag->IsValid())
	{
		ValidationContext.PropertyFails(Property, LOCTEXT("GameplayTag_Empty", "Gameplay tag property not set"));
	}
	else
	{
		ValidateGameplayTag(*GameplayTag, Property, ValidationContext);
	}
}

UStructValidator_GameplayTagContainer::UStructValidator_GameplayTagContainer()
{
	CppType = StaticStruct<FGameplayTagContainer>()->GetStructCPPName();
}

void UStructValidator_GameplayTagContainer::ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	const FGameplayTagContainer* GameplayTags = ConvertStructMemory<FGameplayTagContainer>(PropertyMemory);
	check(GameplayTags);

	for (const FGameplayTag& GameplayTag: *GameplayTags)
	{
		ValidateGameplayTag(GameplayTag, Property, ValidationContext);
	}
}

UStructValidator_GameplayTagQuery::UStructValidator_GameplayTagQuery()
{
	CppType = StaticStruct<FGameplayTagQuery>()->GetStructCPPName();
}

void UStructValidator_GameplayTagQuery::ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	const FGameplayTagQuery* TagQuery = ConvertStructMemory<FGameplayTagQuery>(PropertyMemory);
	check(TagQuery);

	for (const FGameplayTag& GameplayTag: *TagQuery)
	{
		ValidateGameplayTag(GameplayTag, Property, ValidationContext);
	}
}

UStructValidator_GameplayAttribute::UStructValidator_GameplayAttribute()
{
	CppType = StaticStruct<FGameplayAttribute>()->GetStructCPPName();
}

void UStructValidator_GameplayAttribute::ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	const FGameplayAttribute* GameplayAttribute = ConvertStructMemory<FGameplayAttribute>(PropertyMemory);
	check(GameplayAttribute);

	// @todo: additional attribute validation in case of property renaming
	if (!GameplayAttribute->IsValid())
	{
		ValidationContext.PropertyFails(Property, LOCTEXT("GameplayAttribute_Empty", "Gameplay attribute property is not set"));
	}
	else if (UClass* AttributeSet = GameplayAttribute->GetAttributeSetClass(); AttributeSet == nullptr)
	{
		ValidationContext.PropertyFails(Property, FText::Format(LOCTEXT("GameplayAttribute_Invalid", "Gameplay attribute with name %s no longer exists."), FText::FromString(GameplayAttribute->AttributeName)));
	}
}

UStructValidator_DataTableRowHandle::UStructValidator_DataTableRowHandle()
{
	CppType = StaticStruct<FDataTableRowHandle>()->GetStructCPPName();
}

void UStructValidator_DataTableRowHandle::ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	const FDataTableRowHandle* DataTableRow = ConvertStructMemory<FDataTableRowHandle>(PropertyMemory);
	check(DataTableRow);

	if (DataTableRow->DataTable == nullptr || DataTableRow->RowName == NAME_None)
	{
		ValidationContext.PropertyFails(Property, LOCTEXT("DataTableRow_Empty", "Data table row property is not set"));
	}
	else if (!DataTableRow->DataTable->GetRowMap().Find(DataTableRow->RowName))
	{
		ValidationContext.PropertyFails(Property, FText::Format(LOCTEXT("DataTableRow_Invalid", "Invalid row name %s for data table row property"), FText::FromName(DataTableRow->RowName)));
	}
}

UStructValidator_DirectoryPath::UStructValidator_DirectoryPath()
{
	CppType = GetNativeScriptStruct(TEXT("DirectoryPath"))->GetStructCPPName();
}

void UStructValidator_DirectoryPath::ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	const FDirectoryPath* DirectoryPath = ConvertStructMemory<FDirectoryPath>(PropertyMemory);
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

void UStructValidator_FilePath::ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	const FFilePath* FilePath = ConvertStructMemory<FFilePath>(PropertyMemory);
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

void UStructValidator_PrimaryAssetId::ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	const FPrimaryAssetId* AssetID = ConvertStructMemory<FPrimaryAssetId>(PropertyMemory);
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

UStructValidator_Key::UStructValidator_Key()
{
	CppType = StaticStruct<FKey>()->GetStructCPPName();
}

void UStructValidator_Key::ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	const FKey* Key = ConvertStructMemory<FKey>(PropertyMemory);
	check(Key);

	ValidationContext.FailOnCondition(!Key->IsValid(), Property, LOCTEXT("Key", "Key property is not set"));
}

#undef LOCTEXT_NAMESPACE
