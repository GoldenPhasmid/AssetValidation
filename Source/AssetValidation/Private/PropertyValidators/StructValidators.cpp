#include "PropertyValidators/StructValidators.h"

#include "AttributeSet.h"
#include "GameplayTagContainer.h"
#include "Engine/AssetManager.h"

#include "PropertyValidators/PropertyValidation.h"
#include "Windows/Accessibility/WindowsUIAPropertyGetters.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

UStructValidator::UStructValidator()
{
	PropertyClass = FStructProperty::StaticClass();
}

UScriptStruct* UStructValidator::GetNativeScriptStruct(FName StructName)
{
	static UPackage* CoreUObjectPkg = FindObjectChecked<UPackage>(nullptr, TEXT("/Script/CoreUObject"));
	return (UScriptStruct*)StaticFindObjectFastInternal(UScriptStruct::StaticClass(), CoreUObjectPkg, StructName, false, RF_NoFlags, EInternalObjectFlags::None);
}

bool UStructValidator::CanValidateProperty(const FProperty* Property, FMetaDataSource& MetaData) const
{
	return Super::CanValidateProperty(Property, MetaData) && Property->GetCPPType().Equals(CppType);
}

UStructValidator_SoftObjectPath::UStructValidator_SoftObjectPath()
{
	CppType = GetNativeScriptStruct(TEXT("SoftObjectPath"))->GetStructCPPName();
}

void UStructValidator_SoftObjectPath::ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FMetaDataSource& MetaData, FPropertyValidationContext& ValidationContext) const
{
	const FSoftObjectPath* ObjectPath = ConvertStructMemory<FSoftObjectPath>(PropertyMemory);
	check(ObjectPath);
	
	if (ObjectPath->IsNull())
	{
		ValidationContext.PropertyFails(Property, LOCTEXT("SoftObjectPath_Null", "Soft object path not set."));
	}
	else if (ObjectPath->IsAsset())
	{
		if (const FString PackageName = ObjectPath->GetLongPackageName(); !FPackageName::DoesPackageExist(PackageName))
		{
			const FText FailReason = FText::Format(LOCTEXT("SoftObjectPath_NotExists", "Soft object path {0}: package {1} doesn't exist on disk."),
				FText::FromString(ObjectPath->ToString()), FText::FromString(PackageName));
			ValidationContext.PropertyFails(Property, FailReason);
		}
	}
}

UStructValidator_SoftClassPath::UStructValidator_SoftClassPath()
{
	CppType = GetNativeScriptStruct(TEXT("SoftClassPath"))->GetStructCPPName();
}

void UStructValidator_SoftClassPath::ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FMetaDataSource& MetaData, FPropertyValidationContext& ValidationContext) const
{
	const FSoftClassPath* ClassPath = ConvertStructMemory<FSoftClassPath>(PropertyMemory);
	check(ClassPath);
	
	if (ClassPath->IsNull())
	{
		ValidationContext.PropertyFails(Property, LOCTEXT("SoftClassPath_Null", "Soft class path not set."));
	}
	// same as FSoftClassPath::TryLoadClass<T> but with LOAD_Quiet to silence the warning
	else if (UObject*	Class = LoadClass<UObject>(nullptr, *ClassPath->ToString(), nullptr, LOAD_Quiet);
						Class == nullptr)
	{
		const FText FailReason = FText::Format(LOCTEXT("SoftClassPath_NotExists", "Soft class path {0}: failed to load class."), FText::FromString(ClassPath->ToString()));
		ValidationContext.PropertyFails(Property, FailReason);
	}
}

UStructValidator_GameplayTag::UStructValidator_GameplayTag()
{
	CppType = GetStructCppName<FGameplayTag>();
}

void ValidateGameplayTag(const FGameplayTag& Tag, const FProperty* Property, FPropertyValidationContext& ValidationContext)
{
	const FGameplayTag NewTag = FGameplayTag::RequestGameplayTag(Tag.GetTagName(), false);
	if (!NewTag.IsValid())
	{
		ValidationContext.PropertyFails(Property, FText::Format(LOCTEXT("GameplayTag_Invalid", "Gameplay tag with name {0} no longer exists."), FText::FromName(NewTag.GetTagName())));
	}
}

void UStructValidator_GameplayTag::ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FMetaDataSource& MetaData, FPropertyValidationContext& ValidationContext) const
{
	const FGameplayTag* GameplayTag = ConvertStructMemory<FGameplayTag>(PropertyMemory);
	check(GameplayTag);

	if (!GameplayTag->IsValid())
	{
		ValidationContext.PropertyFails(Property, LOCTEXT("GameplayTag_Empty", "Gameplay tag property not set."));
	}
	else
	{
		ValidateGameplayTag(*GameplayTag, Property, ValidationContext);
	}
}

UStructValidator_GameplayTagContainer::UStructValidator_GameplayTagContainer()
{
	CppType = GetStructCppName<FGameplayTagContainer>();
}

void UStructValidator_GameplayTagContainer::ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FMetaDataSource& MetaData, FPropertyValidationContext& ValidationContext) const
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
	CppType = GetStructCppName<FGameplayTagQuery>();
}

void UStructValidator_GameplayTagQuery::ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FMetaDataSource& MetaData, FPropertyValidationContext& ValidationContext) const
{
	const FGameplayTagQuery* TagQuery = ConvertStructMemory<FGameplayTagQuery>(PropertyMemory);
	check(TagQuery);

	for (const FGameplayTag& GameplayTag: TagQuery->GetGameplayTagArray())
	{
		ValidateGameplayTag(GameplayTag, Property, ValidationContext);
	}
}

UStructValidator_GameplayAttribute::UStructValidator_GameplayAttribute()
{
	CppType = GetStructCppName<FGameplayAttribute>();
}

void UStructValidator_GameplayAttribute::ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FMetaDataSource& MetaData, FPropertyValidationContext& ValidationContext) const
{
	const FGameplayAttribute* GameplayAttribute = ConvertStructMemory<FGameplayAttribute>(PropertyMemory);
	check(GameplayAttribute);

	// @todo: additional attribute validation in case property is renamed
	if (!GameplayAttribute->IsValid())
	{
		ValidationContext.PropertyFails(Property, LOCTEXT("GameplayAttribute_Empty", "Gameplay attribute property is not set."));
	}
	else if (UClass* AttributeSet = GameplayAttribute->GetAttributeSetClass(); AttributeSet == nullptr)
	{
		ValidationContext.PropertyFails(Property, FText::Format(LOCTEXT("GameplayAttribute_Invalid", "Gameplay attribute with name {0} no longer exists."), FText::FromString(GameplayAttribute->AttributeName)));
	}
}

UStructValidator_DataTableRowHandle::UStructValidator_DataTableRowHandle()
{
	CppType = GetStructCppName<FDataTableRowHandle>();
}

void UStructValidator_DataTableRowHandle::ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FMetaDataSource& MetaData, FPropertyValidationContext& ValidationContext) const
{
	const FDataTableRowHandle* DataTableRow = ConvertStructMemory<FDataTableRowHandle>(PropertyMemory);
	check(DataTableRow);

	if (DataTableRow->DataTable == nullptr || DataTableRow->RowName == NAME_None)
	{
		ValidationContext.PropertyFails(Property, LOCTEXT("DataTableRow_Empty", "Data table row property is not set."));
	}
	else if (!DataTableRow->DataTable->GetRowMap().Find(DataTableRow->RowName))
	{
		const FText FailReason = FText::Format(LOCTEXT("DataTableRow_Invalid", "Invalid row name {0} for data table row property."), FText::FromName(DataTableRow->RowName));
		ValidationContext.PropertyFails(Property, FailReason);
	}
}

UStructValidator_DirectoryPath::UStructValidator_DirectoryPath()
{
	CppType = GetNativeScriptStruct(TEXT("DirectoryPath"))->GetStructCPPName();
}

void UStructValidator_DirectoryPath::ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FMetaDataSource& MetaData, FPropertyValidationContext& ValidationContext) const
{
	const FDirectoryPath* DirectoryPath = ConvertStructMemory<FDirectoryPath>(PropertyMemory);
	check(DirectoryPath);

	FString RelativePath = DirectoryPath->Path;
	if (RelativePath.IsEmpty())
	{
		ValidationContext.PropertyFails(Property, LOCTEXT("DirectoryPath_Empty", "Directory path is empty."));
		return;
	}
	
	if (FPackageName::IsValidLongPackageName(DirectoryPath->Path))
	{
		FPackageName::TryConvertGameRelativePackagePathToLocalPath(DirectoryPath->Path, RelativePath);
	}

	FString FullPath = RelativePath;
	if (FPaths::IsRelative(RelativePath))
	{
		FullPath = FPaths::ConvertRelativePathToFull(RelativePath);
	}
	
	if (!FPaths::DirectoryExists(FullPath))
	{
		const FText FailReason =	FText::Format(LOCTEXT("DirectoryPath", "Failed to convert file path {0}: disk path {1} is invalid or doesn't exist."),
									FText::FromString(RelativePath), FText::FromString(FullPath));
		ValidationContext.PropertyFails(Property, FailReason);
	}
}

UStructValidator_FilePath::UStructValidator_FilePath()
{
	CppType = GetNativeScriptStruct(TEXT("FilePath"))->GetStructCPPName();
}

void UStructValidator_FilePath::ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FMetaDataSource& MetaData, FPropertyValidationContext& ValidationContext) const
{
	const FFilePath* FilePath = ConvertStructMemory<FFilePath>(PropertyMemory);
	check(FilePath);
	
	if (FilePath->FilePath.IsEmpty())
	{
		ValidationContext.PropertyFails(Property, LOCTEXT("FilePath_Empty", "File path is invalid."));
	}
	else if (!FPackageName::DoesPackageExist(FilePath->FilePath))
	{
		const FText FailReason = FText::Format(LOCTEXT("FilePath_NotExists", "File path {0} doesn't exist on disk."), FText::FromString(FilePath->FilePath));
		ValidationContext.PropertyFails(Property, FailReason);
	}
}

UStructValidator_PrimaryAssetId::UStructValidator_PrimaryAssetId()
{
	CppType = GetNativeScriptStruct(TEXT("PrimaryAssetId"))->GetStructCPPName();
}

void UStructValidator_PrimaryAssetId::ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FMetaDataSource& MetaData, FPropertyValidationContext& ValidationContext) const
{
	const FPrimaryAssetId* AssetID = ConvertStructMemory<FPrimaryAssetId>(PropertyMemory);
	check(AssetID);

	if (!AssetID->IsValid())
	{
		ValidationContext.PropertyFails(Property, LOCTEXT("PrimaryAsset_NotSet", "Primary asset property is not set."));
	}
	else if (UAssetManager* AssetManager = UAssetManager::GetIfInitialized())
	{
		FAssetData AssetData;
		if (!AssetManager->GetPrimaryAssetData(*AssetID, AssetData))
		{
			ValidationContext.PropertyFails(Property, LOCTEXT("PrimaryAsset_Invalid", "Primary asset property {0} stores invalid value."));
		}
	}
}

UStructValidator_Key::UStructValidator_Key()
{
	CppType = GetStructCppName<FKey>();
}

void UStructValidator_Key::ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FMetaDataSource& MetaData, FPropertyValidationContext& ValidationContext) const
{
	const FKey* Key = ConvertStructMemory<FKey>(PropertyMemory);
	check(Key);

	ValidationContext.FailOnCondition(!Key->IsValid(), Property, LOCTEXT("Key", "Key property is not set."));
}

#undef LOCTEXT_NAMESPACE
