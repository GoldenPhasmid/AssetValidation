#pragma once


#include "IDetailCustomization.h"

class UPropertyValidationSettings;

class FPropertyValidationSettingsDetails: public IDetailCustomization
{
	using ThisClass = FPropertyValidationSettingsDetails;
public:
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

	TSharedRef<SWidget> GetMenuContent() const;
	FText GetClassName() const;

private:
	void OnClassPicked(UClass* PickedClass) const;
	
	TWeakObjectPtr<UPropertyValidationSettings> Settings;
	/** Class property handle */
	TSharedPtr<IPropertyHandle> ClassPropertyHandle;
	/** Class combo button */
	TSharedPtr<SComboButton> ClassComboButton;
};
