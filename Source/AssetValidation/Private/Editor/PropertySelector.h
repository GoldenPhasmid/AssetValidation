#pragma once

#include "Framework/PropertyViewer/IFieldIterator.h"
#include "Widgets/SCompoundWidget.h"

class SPropertyViewer;

DECLARE_DELEGATE_RetVal(TFieldPath<FProperty>, FGetPropertyPathEvent);

class FFieldIterator_Properties: public UE::PropertyViewer::IFieldIterator
{
	virtual TArray<FFieldVariant> GetFields(const UStruct* Struct) const override;
	virtual ~FFieldIterator_Properties() override {}
};

class SPropertySelector: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SPropertySelector)
	{}
	SLATE_EVENT(FGetPropertyPathEvent, OnGetPropertyPath)
	SLATE_ARGUMENT(TSharedPtr<IPropertyHandle>, Property)
	SLATE_ARGUMENT(TObjectPtr<UStruct>, OwningStruct)
	SLATE_END_ARGS()

	void Construct(const FArguments& Args);

	TSharedRef<SWidget> GetMenuContent() const;

private:
	TSharedPtr<SComboButton> ComboButton;
	TSharedPtr<SPropertyViewer> PropertyViewer;
};
