#pragma once

#include "PropertyValidatorBase.h"

#include "StructValidators.generated.h"

UCLASS(Abstract)
class ASSETVALIDATION_API UStructValidator: public UPropertyValidatorBase
{
	GENERATED_BODY()
public:
	UStructValidator();

	//~Begin PropertyValidatorBase
	virtual bool CanValidateProperty(FProperty* Property) const override;
	virtual bool CanValidatePropertyValue(FProperty* Property, void* Value) const override;
	//~End PropertyValidatorBase
	
protected:

	FString CppType = "";
};

UCLASS()
class ASSETVALIDATION_API UStructValidator_GameplayTag: public UStructValidator
{
	GENERATED_BODY()
public:
	UStructValidator_GameplayTag();

	//~Begin PropertyValidatorBase
	virtual void ValidateProperty(void* Container, FProperty* Property, FPropertyValidationContext& ValidationContext) const override;
	virtual void ValidatePropertyValue(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const override;
	//~End PropertyValidatorBase
};

UCLASS()
class ASSETVALIDATION_API UStructValidator_GameplayTagContainer: public UStructValidator
{
	GENERATED_BODY()
public:
	UStructValidator_GameplayTagContainer();

	//~Begin PropertyValidatorBase
	virtual void ValidateProperty(void* Container, FProperty* Property, FPropertyValidationContext& ValidationContext) const override;
	virtual void ValidatePropertyValue(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const override;
	//~End PropertyValidatorBase
};

UCLASS()
class UStructValidator_GameplayAttribute: public UStructValidator
{
	GENERATED_BODY()
public:
	UStructValidator_GameplayAttribute();
	
	//~Begin PropertyValidatorBase
	virtual void ValidateProperty(void* Container, FProperty* Property, FPropertyValidationContext& ValidationContext) const override;
	virtual void ValidatePropertyValue(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const override;
	//~End PropertyValidatorBase
};

UCLASS()
class ASSETVALIDATION_API UStructValidator_DataTableRowHandle: public UStructValidator
{
	GENERATED_BODY()
public:
	UStructValidator_DataTableRowHandle();

	//~Begin PropertyValidatorBase
	virtual void ValidateProperty(void* Container, FProperty* Property, FPropertyValidationContext& ValidationContext) const override;
	virtual void ValidatePropertyValue(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const override;
	//~End PropertyValidatorBase
};