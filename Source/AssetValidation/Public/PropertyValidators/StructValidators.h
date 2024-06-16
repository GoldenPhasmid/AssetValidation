#pragma once

#include "PropertyValidatorBase.h"
#include "ContainerValidators/PropertyContainerValidator.h"
#include "ContainerValidators/StructContainerValidator.h"

#include "StructValidators.generated.h"

UCLASS(Abstract)
class ASSETVALIDATION_API UStructValidator: public UPropertyValidatorBase
{
	GENERATED_BODY()
public:
	UStructValidator();

protected:

	/** @return script struct generated from a native non UHT compliant cpp struct */
	static UScriptStruct* GetNativeScriptStruct(FName StructName);
};

UCLASS()
class ASSETVALIDATION_API UStructValidator_SoftObjectPath: public UStructValidator
{
	GENERATED_BODY()
public:
	UStructValidator_SoftObjectPath();
	
	//~Begin PropertyValidatorBase
	virtual void ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FMetaDataSource& MetaData, FPropertyValidationContext& ValidationContext) const override;
	//~End PropertyValidatorBase
};

UCLASS()
class ASSETVALIDATION_API UStructValidator_SoftClassPath: public UStructValidator
{
	GENERATED_BODY()
public:
	UStructValidator_SoftClassPath();
	
	//~Begin PropertyValidatorBase
	virtual void ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FMetaDataSource& MetaData, FPropertyValidationContext& ValidationContext) const override;
	//~End PropertyValidatorBase
};


UCLASS()
class ASSETVALIDATION_API UStructValidator_GameplayTag: public UStructValidator
{
	GENERATED_BODY()
public:
	UStructValidator_GameplayTag();

	//~Begin PropertyValidatorBase
	virtual void ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FMetaDataSource& MetaData, FPropertyValidationContext& ValidationContext) const override;
	//~End PropertyValidatorBase
};

UCLASS()
class ASSETVALIDATION_API UStructValidator_GameplayTagContainer: public UStructValidator
{
	GENERATED_BODY()
public:
	UStructValidator_GameplayTagContainer();

	//~Begin PropertyValidatorBase
	virtual void ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FMetaDataSource& MetaData, FPropertyValidationContext& ValidationContext) const override;
	//~End PropertyValidatorBase
};

UCLASS()
class ASSETVALIDATION_API UStructValidator_GameplayTagQuery: public UStructValidator
{
	GENERATED_BODY()
public:
	UStructValidator_GameplayTagQuery();

	//~Begin PropertyValidatorBase
	virtual void ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FMetaDataSource& MetaData, FPropertyValidationContext& ValidationContext) const override;
	//~End PropertyValidatorBase
};

UCLASS()
class UStructValidator_GameplayAttribute: public UStructValidator
{
	GENERATED_BODY()
public:
	UStructValidator_GameplayAttribute();
	
	//~Begin PropertyValidatorBase
	virtual void ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FMetaDataSource& MetaData, FPropertyValidationContext& ValidationContext) const override;
	//~End PropertyValidatorBase
};

UCLASS()
class ASSETVALIDATION_API UStructValidator_DataTableRowHandle: public UStructValidator
{
	GENERATED_BODY()
public:
	UStructValidator_DataTableRowHandle();

	//~Begin PropertyValidatorBase
	virtual void ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FMetaDataSource& MetaData, FPropertyValidationContext& ValidationContext) const override;
	//~End PropertyValidatorBase
};

UCLASS()
class ASSETVALIDATION_API UStructValidator_DirectoryPath: public UStructValidator
{
	GENERATED_BODY()
public:
	UStructValidator_DirectoryPath();

	//~Begin PropertyValidatorBase
	virtual void ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FMetaDataSource& MetaData, FPropertyValidationContext& ValidationContext) const override;
	//~End PropertyValidatorBase
};

UCLASS()
class ASSETVALIDATION_API UStructValidator_FilePath: public UStructValidator
{
	GENERATED_BODY()
public:
	UStructValidator_FilePath();

	//~Begin PropertyValidatorBase
	virtual void ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FMetaDataSource& MetaData, FPropertyValidationContext& ValidationContext) const override;
	//~End PropertyValidatorBase
};

UCLASS()
class ASSETVALIDATION_API UStructValidator_PrimaryAssetId: public UStructValidator
{
	GENERATED_BODY()
public:
	UStructValidator_PrimaryAssetId();

	//~Begin PropertyValidatorBase
	virtual void ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FMetaDataSource& MetaData, FPropertyValidationContext& ValidationContext) const override;
	//~End PropertyValidatorBase
};

UCLASS()
class ASSETVALIDATION_API UStructValidator_Key: public UStructValidator
{
	GENERATED_BODY()
public:
	UStructValidator_Key();

	//~Begin PropertyValidatorBase
	virtual void ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FMetaDataSource& MetaData, FPropertyValidationContext& ValidationContext) const override;
	//~End PropertyValidatorBase
};

UCLASS()
class ASSETVALIDATION_API UStructValidator_InstancedStruct: public UStructValidator
{
	GENERATED_BODY()
public:
	UStructValidator_InstancedStruct();

	//~Begin PropertyValidatorBase
	virtual void ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FMetaDataSource& MetaData, FPropertyValidationContext& ValidationContext) const override;
	//~End PropertyValidatorBase
};

UCLASS(Config = Editor)
class ASSETVALIDATION_API UContainerValidator_InstancedStruct: public UStructContainerValidator
{
	GENERATED_BODY()
public:
	UContainerValidator_InstancedStruct();

	virtual void ValidateStructAsContainer(TNonNullPtr<const uint8> PropertyMemory, const FStructProperty* StructProperty, FMetaDataSource& MetaData, FPropertyValidationContext& ValidationContext) const override;

	UPROPERTY(Config)
	bool bAlwaysValidateInnerPropertyValue = true;
};