#pragma once

#include "CoreMinimal.h"
#include "UObject/WeakObjectPtr.h"
#include "Templates/NonNullPointer.h"

class UFunction;
class FProperty;

namespace UE::AssetValidation
{
class FEditConditionExpression;
	
/**
 * Context required to evaluate edit condition expression
 * @todo: add support for UFunctions in EditCondition expressions
 */
class FEditConditionContext
{
public:
	FEditConditionContext(UStruct* Struct, TNonNullPtr<const uint8> InContainer, const FProperty* InSourceProperty);

	FName GetContextName() const;

	TOptional<bool> GetBoolValue(const FString& PropertyName) const;
	TOptional<int64> GetIntegerValue(const FString& PropertyName) const;
	TOptional<double> GetNumericValue(const FString& PropertyName) const;
	TOptional<FString> GetEnumValue(const FString& PropertyName) const;
	TOptional<UObject*> GetObjectValue(const FString& PropertyName) const;
	TOptional<FString> GetTypeName(const FString& PropertyName) const;
	TOptional<int64> GetIntegerValueOfEnum(const FString& EnumType, const FString& EnumValue) const;
	
	FORCEINLINE bool IsValid() const { return SourceStruct.IsValid() && SourceProperty.IsValid(); }
	FORCEINLINE FProperty* GetProperty() const { return SourceProperty.Get(); }
	FORCEINLINE UStruct* GetClass() const { return SourceStruct.Get(); }
private:
	
	TNonNullPtr<const uint8>	Container;
	TWeakObjectPtr<UStruct>		SourceStruct;
	TWeakFieldPtr<FProperty>	SourceProperty;
};

}

