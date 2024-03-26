#pragma once

#include "Framework/PropertyViewer/IFieldExpander.h"
#include "Framework/PropertyViewer/IFieldIterator.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/PropertyViewer/SPropertyViewer.h"

namespace UE::PropertyViewer
{
	class SPropertyViewer;
	class IFieldIterator;
	class IFieldExpander;
}
using SPropertyViewer = UE::PropertyViewer::SPropertyViewer;

DECLARE_DELEGATE_RetVal(TFieldPath<FProperty>, FGetPropertyPathEvent);
DECLARE_DELEGATE_RetVal(UStruct*, FGetStructEvent);
DECLARE_DELEGATE_OneParam(FPropertySelectionEvent, TFieldPath<FProperty>);

class SPropertySelector: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SPropertySelector)
	{}
	SLATE_EVENT(FGetStructEvent, OnGetStruct)
	SLATE_EVENT(FGetPropertyPathEvent, OnGetPropertyPath)
	SLATE_EVENT(FPropertySelectionEvent, OnPropertySelectionChanged)
	SLATE_END_ARGS()

	void Construct(const FArguments& Args);

	TSharedRef<SWidget> GetMenuContent();
	FText GetPropertyName() const;
	void HandlePropertySelectionChanged(SPropertyViewer::FHandle Handle, TArrayView<const FFieldVariant> FieldPath, ESelectInfo::Type SelectionType);

private:
	TSharedPtr<SComboButton> ComboButton;
	TSharedPtr<SPropertyViewer> PropertyViewer;

	FGetPropertyPathEvent OnGetPropertyPath;
	FGetStructEvent OnGetStruct;
	FPropertySelectionEvent OnPropertySelectionChanged;

	TUniquePtr<UE::PropertyViewer::IFieldIterator> FieldIterator;
	TUniquePtr<UE::PropertyViewer::IFieldExpander> FieldExpander;
};
