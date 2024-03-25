#pragma once

#include "PropertySelector.h"
#include "Widgets/SCompoundWidget.h"

class SPropertyWidget: public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPropertyWidget)
	{}
	SLATE_EVENT(FGetPropertyPathEvent, OnGetPropertyPath)
	SLATE_END_ARGS()

	void Construct(const FArguments& Args);

	int32 GetDisplayIndex() const;
};
