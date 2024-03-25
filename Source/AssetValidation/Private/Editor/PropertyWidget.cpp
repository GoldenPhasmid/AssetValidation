#include "PropertyWidget.h"

#include "Widgets/Layout/SWidgetSwitcher.h"

void SPropertyWidget::Construct(const FArguments& Args)
{
	ChildSlot
	[
		SNew(SWidgetSwitcher)
		.WidgetIndex(this, &SPropertyWidget::GetDisplayIndex)
		+ SWidgetSwitcher::Slot()
		.Padding(0.f, 0.f, 8.f, 0.f)
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		[

		]
		+ SWidgetSwitcher::Slot()
		[
			SNew(SBox)
			.Padding(FMargin(8.0f, 0.0f, 8.0f, 0.0f))
			[
				SNew(STextBlock)
				.TextStyle(FAppStyle::Get(), "HintText")
				.Text(NSLOCTEXT("AssetValidation", "None", "No property selected"))
			]
		]
	];
}

int32 SPropertyWidget::GetDisplayIndex() const
{
	return 1;
}
