#include "AssetValidationSettingsCustomization.h"

#include "AssetValidationSettings.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "EditorValidatorBase.h"
#include "AssetRegistry/AssetRegistryModule.h"

namespace UE::AssetValidation
{

FAssetValidationSettingsCustomization::~FAssetValidationSettingsCustomization()
{
	if (IAssetRegistry* AssetRegistry = IAssetRegistry::Get())
	{
		AssetRegistry->OnAssetAdded().RemoveAll(this);
		AssetRegistry->OnAssetRenamed().RemoveAll(this);
		AssetRegistry->OnAssetRemoved().RemoveAll(this);
	}
}

void FAssetValidationSettingsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	DetailBuilder.EditCategory(UAssetValidationSettings::StaticClass()->GetFName(), FText::GetEmpty(), ECategoryPriority::Important);
	
	FARFilter Filter;
	Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
	IAssetRegistry& AssetRegistry = IAssetRegistry::GetChecked();
	
	TArray<FAssetData> BlueprintAssets;
	AssetRegistry.GetAssetsByClass(UBlueprint::StaticClass()->GetClassPathName(), BlueprintAssets, true);

	TArray<UClass*> ValidatorClasses;
	// gather blueprint validators
	for (const FAssetData& BlueprintAsset: BlueprintAssets)
	{
		if (UClass* ValidatorClass = GetAssetValidatorClassFromAsset(BlueprintAsset))
		{
			ValidatorClasses.Add(ValidatorClass);
		}
	}

	// gather native validators
	GetDerivedClasses(UEditorValidatorBase::StaticClass(), ValidatorClasses, true);

	// filter validators: remove abstract, deprecated and validators without config properties.
	ValidatorClasses.RemoveAll([](const UClass* Class)
	{
		if (Class->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
		{
			return true;
		}
		
		for (const FProperty* Property: TFieldRange<FProperty>{Class, EFieldIterationFlags::Default})
		{
			if (Property->HasAnyPropertyFlags(CPF_Config))
			{
				return false;
			}
		}

		return true;
	});

	IDetailCategoryBuilder& Category = DetailBuilder.EditCategory(
		"AssetValidators", NSLOCTEXT("AssetValidation", "AssetValidatorsCategoryTitle", "Asset Validators"));
	
	for (const UClass* ValidatorClass: ValidatorClasses)
	{
		UObject* DefaultValidator = ValidatorClass->GetDefaultObject();

		IDetailPropertyRow* Row = Category.AddExternalObjects({DefaultValidator}, EPropertyLocation::Default, FAddPropertyParams{}.UniqueId(ValidatorClass->GetFName()));
		check(Row);

		Row->CustomWidget()
		.NameContent()
		[
			SNew(STextBlock).Text(ValidatorClass->GetDisplayNameText())
		];
	}
	
	AssetRegistry.OnAssetAdded().AddSP(this, &ThisClass::OnAssetAdded);
	AssetRegistry.OnAssetRenamed().AddSP(this, &ThisClass::OnAssetRenamed);
	AssetRegistry.OnAssetRemoved().AddSP(this, &ThisClass::OnAssetRemoved);
}

void FAssetValidationSettingsCustomization::CustomizeDetails(const TSharedPtr<IDetailLayoutBuilder>& DetailBuilder)
{
	CachedDetailBuilder = DetailBuilder;
	CustomizeDetails(*DetailBuilder);
}

void FAssetValidationSettingsCustomization::OnAssetAdded(const FAssetData& AssetData)
{
	RebuildDetails(AssetData, false);
}

void FAssetValidationSettingsCustomization::OnAssetRemoved(const FAssetData& AssetData)
{
	RebuildDetails(AssetData, true);
}

void FAssetValidationSettingsCustomization::OnAssetRenamed(const FAssetData& AssetData, const FString& NewName)
{
	RebuildDetails(AssetData, true);
}

void FAssetValidationSettingsCustomization::RebuildDetails(const FAssetData& AssetData, bool bRemoved)
{
	if (UClass* ValidatorClass = GetAssetValidatorClassFromAsset(AssetData))
	{
		if (CachedDetailBuilder.IsValid())
		{
			CachedDetailBuilder.Pin()->ForceRefreshDetails();
		}
	}
}

UClass* FAssetValidationSettingsCustomization::GetAssetValidatorClassFromAsset(const FAssetData& AssetData) const
{
	FString NativeClassPath{};
	if (!AssetData.GetTagValue(FBlueprintTags::NativeParentClassPath, NativeClassPath))
	{
		AssetData.GetTagValue(FBlueprintTags::ParentClassPath, NativeClassPath);
	}

	if (!NativeClassPath.IsEmpty())
	{
		if (UClass* ParentClass = FindObjectSafe<UClass>(nullptr, *NativeClassPath, true); ParentClass->IsChildOf<UEditorValidatorBase>())
		{
			UBlueprint* Blueprint = CastChecked<UBlueprint>(AssetData.GetAsset());
			return Blueprint->GeneratedClass;
		}
	}

	return nullptr;
}
}
