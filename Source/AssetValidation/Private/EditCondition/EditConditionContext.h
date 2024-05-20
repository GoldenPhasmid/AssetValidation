#pragma once

#include "CoreMinimal.h"
#include "UObject/WeakObjectPtr.h"
#include "Templates/NonNullPointer.h"

class UFunction;
class FProperty;
class FEditConditionExpression;

namespace UE::AssetValidation
{

class FEditConditionExpression;
	
class IEditConditionContext
{
public:
	virtual ~IEditConditionContext() {}
	
	virtual FName GetContextName() const = 0;

	virtual TOptional<bool> GetBoolValue(const FString& PropertyName, TWeakObjectPtr<UFunction> CachedFunction = nullptr) const = 0;
	virtual TOptional<int64> GetIntegerValue(const FString& PropertyName, TWeakObjectPtr<UFunction> CachedFunction = nullptr) const = 0;
	virtual TOptional<double> GetNumericValue(const FString& PropertyName, TWeakObjectPtr<UFunction> CachedFunction = nullptr) const = 0;
	virtual TOptional<FString> GetEnumValue(const FString& PropertyName, TWeakObjectPtr<UFunction> CachedFunction = nullptr) const = 0;
	virtual TOptional<UObject*> GetPointerValue(const FString& PropertyName, TWeakObjectPtr<UFunction> CachedFunction = nullptr) const = 0;
	virtual TOptional<FString> GetTypeName(const FString& PropertyName, TWeakObjectPtr<UFunction> CachedFunction = nullptr) const = 0;
	virtual TOptional<int64> GetIntegerValueOfEnum(const FString& EnumType, const FString& EnumValue) const = 0;

	/**
	 * Get field as a function, avoiding multiple function searches
	 *
	 * @TODO: Get property in general, not just for functions. See: UE-175891.
	 *
	 * @param FieldName Name of the field to try to interpret as a function
	 * @return Function if found or already cached, else
	 */
	virtual const TWeakObjectPtr<UFunction> GetFunction(const FString& FieldName) const = 0;
};
	
class FEditConditionContext : public IEditConditionContext
{
public:
	FEditConditionContext(UStruct* Struct, TNonNullPtr<const uint8> InContainer, const FProperty* InSourceProperty);
	virtual ~FEditConditionContext() override {}

	virtual FName GetContextName() const override;

	virtual TOptional<bool> GetBoolValue(const FString& PropertyName, TWeakObjectPtr<UFunction> CachedFunction = nullptr) const override;
	virtual TOptional<int64> GetIntegerValue(const FString& PropertyName, TWeakObjectPtr<UFunction> CachedFunction = nullptr) const override;
	virtual TOptional<double> GetNumericValue(const FString& PropertyName, TWeakObjectPtr<UFunction> CachedFunction = nullptr) const override;
	virtual TOptional<FString> GetEnumValue(const FString& PropertyName, TWeakObjectPtr<UFunction> CachedFunction = nullptr) const override;
	virtual TOptional<UObject*> GetPointerValue(const FString& PropertyName, TWeakObjectPtr<UFunction> CachedFunction = nullptr) const override;
	virtual TOptional<FString> GetTypeName(const FString& PropertyName, TWeakObjectPtr<UFunction> CachedFunction = nullptr) const override;
	virtual TOptional<int64> GetIntegerValueOfEnum(const FString& EnumType, const FString& EnumValue) const override;

	virtual const TWeakObjectPtr<UFunction> GetFunction(const FString& FieldName) const override;
	
	FORCEINLINE bool IsValid() const { return SourceStruct.IsValid() && SourceProperty.IsValid(); }
	FORCEINLINE FProperty* GetProperty() const { return SourceProperty.Get(); }
	FORCEINLINE UStruct* GetClass() const { return SourceStruct.Get(); }
private:
	
	TNonNullPtr<const uint8>			Container;
	TWeakObjectPtr<UStruct>		SourceStruct;
	TWeakFieldPtr<FProperty>	SourceProperty;
};

}

