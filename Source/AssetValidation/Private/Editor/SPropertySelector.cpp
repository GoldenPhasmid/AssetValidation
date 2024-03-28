#include "SPropertySelector.h"

#include "DetailLayoutBuilder.h"
#include "PropertyValidatorSubsystem.h"
#include "Widgets/PropertyViewer/SFieldName.h"
#include "Widgets/PropertyViewer/SPropertyViewer.h"

namespace UE::AssetValidation
{
	
class FFieldIterator_EditableProperties: public UE::PropertyViewer::IFieldIterator
{
public:
	FFieldIterator_EditableProperties(UStruct* InBaseStruct, UPropertyValidatorSubsystem* InValidationSubsystem)
		: BaseStruct(InBaseStruct)
		, ValidationSubsystem(InValidationSubsystem)
	{}

	static TUniquePtr<UE::PropertyViewer::IFieldIterator> Create(UStruct* BaseStruct)
	{
		return MakeUnique<FFieldIterator_EditableProperties>(BaseStruct, UPropertyValidatorSubsystem::Get());
	}

	//~Begin IFieldIterator interface
	virtual TArray<FFieldVariant> GetFields(const UStruct* Struct) const override
	{
		if (!BaseStruct.IsValid() || !ValidationSubsystem.IsValid() || Struct != BaseStruct.Get())
		{
			return {};
		}
    
		TArray<FFieldVariant> Result;
		for (const FProperty* Property: TFieldRange<FProperty>{Struct, EFieldIterationFlags::None})
		{
			if (ValidationSubsystem->CanEverValidateProperty(Property) && (CanValidatePropertyValue(Property) || CanValidatePropertyRecursively(Property)))
			{
				Result.Add(FFieldVariant{Property});
			}
		}

		return Result;
	}
	virtual ~FFieldIterator_EditableProperties() override {}
	//~End IFieldIterator interface

	private:

	TWeakObjectPtr<UStruct> BaseStruct;
	TWeakObjectPtr<UPropertyValidatorSubsystem> ValidationSubsystem;
};

class FFieldExpander_DontExpand: public UE::PropertyViewer::FFieldExpander_Default
{
public:
	static TUniquePtr<UE::PropertyViewer::IFieldExpander> Create()
	{
		TUniquePtr Result{MakeUnique<FFieldExpander_DontExpand>()};
		Result->SetExpandScriptStruct(false);
		Result->SetExpandObject(EObjectExpandFlag::None);
		Result->SetExpandFunction(EFunctionExpand::None);

		return Result;
	}
};

void SPropertySelector::Construct(const FArguments& Args)
{
	OnGetStruct = Args._OnGetStruct;
	OnGetPropertyPath = Args._OnGetPropertyPath;
	OnPropertySelectionChanged = Args._OnPropertySelectionChanged;

	ChildSlot
	[
		SNew(SBox)
		.MinDesiredWidth(200.f)
		[
			SAssignNew(ComboButton, SComboButton)
			.OnGetMenuContent(this, &SPropertySelector::GetMenuContent)
			.ButtonContent()
			[
				SNew(STextBlock)
				.Text(this, &SPropertySelector::GetPropertyName)
				.Font(IDetailLayoutBuilder::GetDetailFont())
			]
		]
	];
}

TSharedRef<SWidget> SPropertySelector::GetMenuContent()
{
	UStruct* Struct = nullptr;
	if (OnGetStruct.IsBound())
	{
		Struct = OnGetStruct.Execute();
	}
	FieldIterator = FFieldIterator_EditableProperties::Create(Struct);
	FieldExpander = FFieldExpander_DontExpand::Create();

	PropertyViewer = SNew(SPropertyViewer)
	.FieldIterator(FieldIterator.Get())
	.FieldExpander(FieldExpander.Get())
	.SelectionMode(ESelectionMode::Single)
	.bShowFieldIcon(true)
	.bSanitizeName(true)
	.bShowSearchBox(true)
	.PropertyVisibility(SPropertyViewer::EPropertyVisibility::Hidden)
	.OnGetPreSlot(this, &SPropertySelector::HandleGetPreSlot)
	.OnGenerateContainer(this, &SPropertySelector::HandleGenerateContainer)
	.OnSelectionChanged(this, &SPropertySelector::HandlePropertySelectionChanged);

	if (Struct)
	{
		if (const UClass* Class = Cast<UClass>(Struct))
		{
			PropertyViewer->AddContainer(Class);
		}
		else if (const UScriptStruct* ScriptStruct = Cast<UScriptStruct>(Struct))
		{
			PropertyViewer->AddContainer(ScriptStruct);
		}
	}

	return PropertyViewer.ToSharedRef();
}

FText SPropertySelector::GetPropertyName() const
{
	if (OnGetPropertyPath.IsBound() && OnGetStruct.IsBound())
	{
		UStruct* Struct = OnGetStruct.Execute();
		TFieldPath<FProperty> PropertyPath = OnGetPropertyPath.Execute();
		if (const FProperty* Property = PropertyPath.Get(Struct))
		{
			return Property->GetDisplayNameText();
		}
	}

	return NSLOCTEXT("AssetValidation", "PropertySelectionEmptyLabel", "No property selected");
}

void SPropertySelector::HandlePropertySelectionChanged(SPropertyViewer::FHandle Handle, TArrayView<const FFieldVariant> FieldPath, ESelectInfo::Type SelectionType)
{
	ComboButton->SetIsOpen(false);

	if (FieldPath.Num() == 1)
	{
		const FFieldVariant& FieldVariant = FieldPath[0];
		if (FProperty* Property = FieldVariant.Get<FProperty>())
		{
			OnPropertySelectionChanged.ExecuteIfBound(TFieldPath<FProperty>{Property});
		}
	}
}

TSharedRef<SWidget> SPropertySelector::HandleGenerateContainer(SPropertyViewer::FHandle Handle, TOptional<FText> DisplayName)
{
	// Implementation copy from FPropertyViewerImpl::HandleGenerateRow to avoid ContainerPin->IsValid() check
	// which would fail for UScriptStruct not being trashed
	// @todo: remove after epic's bug is fixed
	TSharedPtr<SWidget> ItemWidget = SNullWidget::NullWidget;
	if (OnGetStruct.IsBound())
	{
		const UStruct* Struct = OnGetStruct.Execute();
		if (const UClass* Class = Cast<const UClass>(Struct))
		{
			return SNew(UE::PropertyViewer::SFieldName, Class)
			.bShowIcon(true)
			.bSanitizeName(true)
			.OverrideDisplayName(DisplayName);
		}
		if (const UScriptStruct* ScriptStruct = Cast<const UScriptStruct>(Struct))
		{
			return SNew(UE::PropertyViewer::SFieldName, ScriptStruct)
			.bShowIcon(true)
			.bSanitizeName(true)
			.OverrideDisplayName(DisplayName);
		}
	}

	return ItemWidget.ToSharedRef();
}

TSharedPtr<SWidget> SPropertySelector::HandleGetPreSlot(SPropertyViewer::FHandle, TArrayView<const FFieldVariant> FieldPath)
{
	if (OnGetStruct.IsBound())
	{
		if (UScriptStruct* Struct = Cast<UScriptStruct>(OnGetStruct.Execute()))
		{
			// add STRUCT_Trashed flag to struct container, so that FContainer::IsValid check succeeds for script structs
			// adding here, because delegate conveniently happens before any FContainer::IsValid checks
			// @todo: remove after epic's bug is fixed
			Struct->SetStructTrashed(true);
		}
	}

	return nullptr;
}

void SPropertySelector::Tick(float DeltaTime)
{
	if (OnGetStruct.IsBound())
	{
		if (UScriptStruct* Struct = Cast<UScriptStruct>(OnGetStruct.Execute()))
		{
			// clear STRUCT_Trashed flag from struct container.
			// We can't set STRUCT_Trashed flag in OnGetPreSlot and clear in OnGetPostSlot,
			// because adding OnGetPostSlot delegate inevitably fucks up visualization
			// (as no one uses it I suppose that's another bug on epic's side)
			// @todo: remove after epic's bug is fixed
			Struct->SetStructTrashed(false);
		}
	}
}
	
}