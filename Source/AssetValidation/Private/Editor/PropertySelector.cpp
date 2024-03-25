#include "PropertySelector.h"

#include "PropertyWidget.h"
#include "Widgets/PropertyViewer/SPropertyViewer.h"

TArray<FFieldVariant> FFieldIterator_Properties::GetFields(const UStruct* Struct) const
{
	TArray<FFieldVariant> Result;
	for (TFieldIterator<FProperty> It{Struct, EFieldIterationFlags::None}; It; ++It)
	{
		
	}
}

void SPropertySelector::Construct(const FArguments& Args)
{
	ChildSlot
	[
		SNew(SBox)
		.MinDesiredWidth(200.f)
		[
			SAssignNew(ComboButton, SComboButton)
			.OnGetMenuContent(this, &SPropertySelector::GetMenuContent)
			.ButtonContent()
			[
				SNew(SPropertyWidget)
			]
		]
	];
}

TSharedRef<SWidget> SPropertySelector::GetMenuContent() const
{
	PropertyViewer = SNew(SPropertyViewer)
}
