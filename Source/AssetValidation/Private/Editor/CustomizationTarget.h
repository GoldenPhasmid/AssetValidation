#pragma once

#include "CoreMinimal.h"

struct EVisibility;
class FDetailWidgetRow;
enum class ECheckBoxState : uint8;

namespace UE::AssetValidation
{
	class FMetaDataSource;
}


namespace UE::AssetValidation
{

class ICustomizationTarget: public TSharedFromThis<ICustomizationTarget>
{
public:

	static void CustomizeForObject(TSharedPtr<ICustomizationTarget> Target, TFunctionRef<FDetailWidgetRow&(const FText&)> GetCustomRow);

	FORCEINLINE ECheckBoxState GetMetaState(FName MetaKey) const
	{
		FString MetaValue;
		return HandleGetMetaState(MetaKey, MetaValue) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}

	FORCEINLINE FText GetMetaValue(FName MetaKey) const
	{
		FString MetaValue;
		HandleGetMetaState(MetaKey, MetaValue);

		return FText::FromString(MetaValue);
	}

	FORCEINLINE void SetMetaState(ECheckBoxState NewState, FName MetaKey)
	{
		HandleMetaStateChanged(NewState == ECheckBoxState::Checked, MetaKey);
	}

	FORCEINLINE void SetMetaValue(const FText& NewValue, ETextCommit::Type, FName MetaKey)
	{
		HandleMetaStateChanged(true, MetaKey, NewValue.ToString());
	}

	FORCEINLINE EVisibility GetMetaVisibility(FName MetaKey) const
	{
		return HandleIsMetaVisible(MetaKey) ? EVisibility::Visible : EVisibility::Collapsed;
	}

	FORCEINLINE bool IsMetaEditable(FName MetaKey) const
	{
		return HandleIsMetaEditable(MetaKey);
	}

	virtual ~ICustomizationTarget() {}

protected:
	/** shared implementation of whether property meta should be visible */
	bool IsPropertyMetaVisible(const FProperty* Property, const FName& MetaKey) const;
private:

	virtual bool HandleIsMetaVisible(const FName& MetaKey) const = 0;
	virtual bool HandleIsMetaEditable(FName MetaKey) const = 0;
	virtual bool HandleGetMetaState(const FName& MetaKey, FString& OutValue) const = 0;
	virtual void HandleMetaStateChanged(bool NewMetaState, const FName& MetaKey, FString MetaValue = {}) = 0;
};

}
