#pragma once

#include "CustomizationTarget.h"
#include "IDetailCustomization.h"
#include "IDetailCustomNodeBuilder.h"
#include "Kismet2/StructureEditorUtils.h"

namespace UE::AssetValidation
{
	class FMetaDataSource;
}

struct FPropertyExternalValidationData;
class UUserDefinedStruct;

/**
 * Validation tab used for UDS editor
 */
class SStructureEditorValidationTab: public SCompoundWidget, public FStructureEditorUtils::INotifyOnStructChanged
{
public:
	SLATE_BEGIN_ARGS(SStructureEditorValidationTab)
	{}
	SLATE_ARGUMENT(UUserDefinedStruct*, Struct)
	SLATE_END_ARGS()

	void Construct(const FArguments& Args);

	//~Begin INotifyOnStructChanged
	virtual void PreChange(const class UUserDefinedStruct* Struct, FStructureEditorUtils::EStructureEditorChangeInfo Info) override;
	virtual void PostChange(const class UUserDefinedStruct* Struct, FStructureEditorUtils::EStructureEditorChangeInfo Info) override;
	//~End INotifyOnStructChanged
private:
	/** Struct that widget represents */
	TWeakObjectPtr<UUserDefinedStruct> UserDefinedStruct;
	/** Struct scope */
	TSharedPtr<FStructOnScope> StructScope;
	/** Details view */
	TSharedPtr<IDetailsView> DetailsView;
};


/**
 * Represents validation dock tab in UDS editor
 */
class FStructureEditorValidationTabLayout: public IDetailCustomization
{
	using ThisClass = FStructureEditorValidationTabLayout;
public:

	FStructureEditorValidationTabLayout(TSharedPtr<FStructOnScope> InStructScope)
		: StructScope(InStructScope)
	{}

	static TSharedRef<IDetailCustomization> MakeInstance(TSharedPtr<FStructOnScope> StructScope)
	{
		return MakeShared<ThisClass>(StructScope);
	}
	
	virtual void CustomizeDetails(class IDetailLayoutBuilder& DetailLayout) override;
	virtual ~FStructureEditorValidationTabLayout() override;
private:
	
	/** Customized user defined struct */
	TWeakObjectPtr<UUserDefinedStruct> UserDefinedStruct;
	/** Struct scope */
	TSharedPtr<FStructOnScope> StructScope;
};

