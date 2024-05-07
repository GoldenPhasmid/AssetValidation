#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "Editor/PropertyValidationVariableDetailCustomization.h"
#include "PropertyValidators/PropertyValidationResult.h"
#include "Templates/NonNullPointer.h"

#include "PropertyValidatorSubsystem.generated.h"

namespace UE::AssetValidation
{
	class FMetaDataSource;
}
using FMetaDataSource = UE::AssetValidation::FMetaDataSource;

class FFieldClass;
class UPropertyValidatorBase;
class FPropertyValidationContext;
class UValidationEditorExtensionManager;
struct FPropertyValidationResult;
struct FSubobjectData;

/**
 *
 */
UCLASS()
class ASSETVALIDATION_API UPropertyValidatorSubsystem: public UEditorSubsystem
{
	GENERATED_BODY()

	friend class FPropertyValidationContext;
public:

	static UPropertyValidatorSubsystem* Get();

	//~Begin USubsystem interface
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~End USubsystem interface

	/**
	 * @return validation result for given object
	 * @param Object object to perform full validation on
	 */
	FPropertyValidationResult ValidateObject(const UObject* Object) const;

	/**
	 * @return property validation result for given struct inside another object
	 * @param OwningObject logically owns script struct (doesn't mean that struct data is a part of object's memory) and indicates object of validation
	 * @param ScriptStruct struct type to perform full validation
	 * @param StructData memory that represents @ScriptStruct
	 */
	FPropertyValidationResult ValidateStruct(const UObject* OwningObject, const UScriptStruct* ScriptStruct, const uint8* StructData);

	template <typename TStructType>
	FPropertyValidationResult ValidateStruct(const UObject* OwningObject, const TStructType& Value)
	{
		return ValidateStruct(OwningObject, TStructType::StaticStruct(), reinterpret_cast<const uint8*>(&Value));
	}

	/**
	 * @return validation result for given property that is part of given object
	 * @param Object object that property is part of and located in
	 * @param Property property to validate
	 */
	FPropertyValidationResult ValidateObjectProperty(const UObject* Object, const FProperty* Property) const;

	/**
	 * @return validation result for given property that belongs to struct type and stored inside struct data
	 * @param OwningObject logically owns script struct (doesn't mean that struct data is a part of object's memory) and indicates object of validation
	 * @param ScriptStruct struct type given property belongs to
	 * @param Property property to validate
	 * @param StructData a region of memory that holds @ScriptStruct object type
	 */
	FPropertyValidationResult ValidateStructProperty(const UObject* OwningObject, const UScriptStruct* ScriptStruct, FProperty* Property, const uint8* StructData);

	template <typename TStructType>
	FPropertyValidationResult ValidateStructProperty(const UObject* OwningObject, FProperty* Property, const TStructType& Value)
	{
		return ValidateStructProperty(OwningObject, TStructType::StaticStruct(), Property, reinterpret_cast<const uint8*>(&Value));
	}

	/** @return whether validator subsystem can run validators on a given object */
	bool ShouldIgnorePackage(const UPackage* Package) const;
	
	/** @return whether package should be skipped during validation */
	bool ShouldSkipPackage(const UPackage* Package) const;

	/** @return whether validator subsystem should analyze all package's properties */
	bool ShouldIteratePackageProperties(const UPackage* Package) const;

	/** @return whether subsystem can validate following property type */
	bool HasValidatorForPropertyType(const FProperty* PropertyType) const;

	/** @return whether property can be ever validated based on its property flags */
	bool CanEverValidateProperty(const FProperty* Property) const;

protected:
	
	/**
	 * @brief validate all properties in @Container
	 * @param ContainerMemory container to get data from
	 * @param Struct struct to retrieve
	 * @param ValidationContext provided validation context
	 */
	virtual void ValidateContainerWithContext(TNonNullPtr<const uint8> ContainerMemory, const UStruct* Struct, FPropertyValidationContext& ValidationContext) const;

	/**
	 * @brief validate @Property in @Container
	 * @param ContainerMemory container to get data from
	 * @param Property property to validate
	 * @param MetaData ref that describes property meta data. Should be used instead of querying property meta data directly
	 * @param ValidationContext provided validation context
	 */
	virtual void ValidatePropertyWithContext(TNonNullPtr<const uint8> ContainerMemory, const FProperty* Property, UE::AssetValidation::FMetaDataSource& MetaData, FPropertyValidationContext& ValidationContext) const;
	
	/**
	 * @brief validate given property value
	 * @param PropertyMemory property value
	 * @param Property property to validate
	 * @param MetaData ref that describes property meta data. Should be used instead of querying property meta data directly
	 * @param ValidationContext provided validation context
	 */
	virtual void ValidatePropertyValueWithContext(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, UE::AssetValidation::FMetaDataSource& MetaData, FPropertyValidationContext& ValidationContext) const;
	
	/** @return whether property should be validated for given @ValidationContext */
	bool ShouldValidateProperty(const FProperty* Property, FPropertyValidationContext& ValidationContext) const;

	/** @return property validator for a given property type */
	const UPropertyValidatorBase* FindPropertyValidator(const FProperty* PropertyType) const;
	/** @return container  validator for a given property type */
	const UPropertyValidatorBase* FindContainerValidator(const FProperty* PropertyType) const;
	
	/** property validators mapped by their respective use */
	TMap<FFieldClass*, UPropertyValidatorBase*> ContainerValidators;
	TMap<FFieldClass*, UPropertyValidatorBase*> PropertyValidators;
	TMap<FString, UPropertyValidatorBase*> StructValidators;

	/** List of all active validators */
	UPROPERTY(Transient)
	TArray<UPropertyValidatorBase*> AllValidators;

	UPROPERTY(Transient)
	TObjectPtr<UValidationEditorExtensionManager> ExtensionManager;
};
