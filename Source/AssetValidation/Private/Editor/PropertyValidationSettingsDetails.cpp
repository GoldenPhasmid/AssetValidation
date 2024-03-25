#include "PropertyValidationSettingsDetails.h"

#include "ClassViewerFilter.h"
#include "ClassViewerModule.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "PropertyValidationSettings.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

namespace UE::AssetValidation
{
	class FAssetClassFilter: public IClassViewerFilter
	{
		static constexpr EClassFlags DisallowedClassFlags = CLASS_Deprecated | CLASS_NewerVersionExists | CLASS_Hidden | CLASS_HideDropDown;
	public:
		FAssetClassFilter() = default;

		virtual bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, TSharedRef<FClassViewerFilterFuncs> InFilterFuncs) override
		{
			return InClass && !InClass->HasAnyClassFlags(DisallowedClassFlags);
		}
		virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const TSharedRef<const IUnloadedBlueprintData> InUnloadedClassData, TSharedRef<FClassViewerFilterFuncs> InFilterFuncs) override
		{
			return !InUnloadedClassData->HasAnyClassFlags(DisallowedClassFlags);
		}
	};
}

TSharedRef<IDetailCustomization> FPropertyValidationSettingsDetails::MakeInstance()
{
	return MakeShared<FPropertyValidationSettingsDetails>();
}

void FPropertyValidationSettingsDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> Objects;
	DetailBuilder.GetObjectsBeingCustomized(Objects);

	if (Objects.Num() == 0)
	{
		return;	
	}
	
	if (Settings = Cast<UPropertyValidationSettings>(Objects[0]); !Settings.IsValid())
	{
		return;
	}

	DetailBuilder.EditCategory(TEXT("Settings"), LOCTEXT("SettingsCategoryLabel", "Settings"), ECategoryPriority::Important)
	.InitiallyCollapsed(false);
	
	ClassPropertyHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UPropertyValidationSettings, CustomizedObjectClass));
	ClassPropertyHandle->MarkHiddenByCustomization();

	const FSlateFontInfo DetailFont = IDetailLayoutBuilder::GetDetailFont();

	IDetailCategoryBuilder& Category = DetailBuilder.EditCategory(TEXT("ValidateEngineClasses"), LOCTEXT("EngineClassesCategoryLabel", "Validate Engine Classes"))
	.InitiallyCollapsed(false);
	
	Category.AddCustomRow(FText::GetEmpty())
	.NameContent()
	[
		ClassPropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(1.f)
		[
			SAssignNew(ClassComboButton, SComboButton)
			.IsEnabled(true)
			.OnGetMenuContent(this, &ThisClass::GetMenuContent)
			.ButtonContent()
			[
				SNew(STextBlock)
				.Text(this, &ThisClass::GetClassName)
				.Font(DetailFont)
			]
		]
	];
}

TSharedRef<SWidget> FPropertyValidationSettingsDetails::GetMenuContent() const
{
	FClassViewerInitializationOptions InitOptions;
	InitOptions.Mode = EClassViewerMode::ClassPicker;
	InitOptions.DisplayMode = EClassViewerDisplayMode::TreeView;
	InitOptions.NameTypeToDisplay = EClassViewerNameTypeToDisplay::Dynamic;
	InitOptions.PropertyHandle = ClassPropertyHandle;
	InitOptions.bShowBackgroundBorder = true;
	InitOptions.ClassFilters.Add(MakeShared<UE::AssetValidation::FAssetClassFilter>());

	FClassViewerModule& ClassViewer = FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer");
	TSharedRef<SWidget> ClassPicker = ClassViewer.CreateClassViewer(InitOptions, FOnClassPicked::CreateSP(this, &ThisClass::OnClassPicked));

	return SNew(SBox)
	.WidthOverride(280.f)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.MaxHeight(500.f)
		.AutoHeight()
		[
			ClassPicker
		]
	];
}

FText FPropertyValidationSettingsDetails::GetClassName() const
{
	UObject* OutValue = nullptr;
	if (FPropertyAccess::Result AccessResult = ClassPropertyHandle->GetValue(OutValue); AccessResult == FPropertyAccess::Success)
	{
		if (const UClass* Class = CastChecked<UClass>(OutValue, ECastCheckedType::NullAllowed))
		{
			return Class->GetDisplayNameText();	
		}
	}

	return FText::GetEmpty();
}

void FPropertyValidationSettingsDetails::OnClassPicked(UClass* PickedClass) const
{
	ClassComboButton->SetIsOpen(false);
	ClassPropertyHandle->SetValue(PickedClass);
}


#undef LOCTEXT_NAMESPACE
