#pragma once

#include "AttributeSet.h"
#include "GameplayTagContainer.h"

#include "StructValidatorTests.generated.h"

UENUM()
enum class EValidationEnum: uint8
{
	None = 0,
	One = 1,
	Two = 2
};

UCLASS()
class UValidationTestObject_PropertyValidators: public UObject
{
	GENERATED_BODY()
public:

	UPROPERTY(EditAnywhere, meta = (Validate))
	TObjectPtr<UObject> ObjectProperty;

	UPROPERTY(EditAnywhere, meta = (Validate))
	TSubclassOf<UObject> ClassProperty;

	UPROPERTY(EditAnywhere, meta = (Validate))
	TSoftObjectPtr<UObject> SoftObjectProperty;

	UPROPERTY(EditAnywhere, meta = (Validate))
	TSoftClassPtr<UClass> SoftClassProperty;

	UPROPERTY(EditAnywhere, meta = (Validate))
	FName Name;
	
	UPROPERTY(EditAnywhere, meta = (Validate))
	FString Str;
	
	UPROPERTY(EditAnywhere, meta = (Validate))
	FText Text;

	UPROPERTY(EditAnywhere, meta = (Validate))
	EValidationEnum Enum = EValidationEnum::None;
};


USTRUCT()
struct FGameplayTagStruct
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, meta = (Validate))
	FGameplayTag EmptyTag = FGameplayTag::EmptyTag;
};

UCLASS()
class UValidationTestObject_GameplayTag: public UObject
{
	GENERATED_BODY()
	
	UValidationTestObject_GameplayTag()
	{
		EmptyTagArray.AddDefaulted();
	}

	UPROPERTY(EditAnywhere, meta = (Validate))
	FGameplayTag EmptyTag = FGameplayTag::EmptyTag;

	UPROPERTY(EditAnywhere, meta = (Validate))
	TArray<FGameplayTag> EmptyTagArray;

	UPROPERTY(EditAnywhere, meta = (Validate))
	FGameplayTagStruct Struct;
};

USTRUCT()
struct FGameplayTagContainerStruct
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, meta = (Validate))
	FGameplayTagContainer EmptyTags;
};

UCLASS()
class UValidationTestObject_GameplayTagContainer: public UObject
{
	GENERATED_BODY()

	UValidationTestObject_GameplayTagContainer()
	{
		EmptyTagsArray.AddDefaulted();
	}
	
	UPROPERTY(EditAnywhere, meta = (Validate))
	FGameplayTagContainer EmptyTags;

	UPROPERTY(EditAnywhere, meta = (Validate))
	TArray<FGameplayTagContainer> EmptyTagsArray;

	UPROPERTY(EditAnywhere, meta = (Validate))
	FGameplayTagContainerStruct Struct;
};

USTRUCT()
struct FGameplayAttributeStruct
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, meta = (Validate))
	FGameplayAttribute EmptyAttribute;
};

UCLASS()
class UValidationTestObject_GameplayAttribute: public UObject
{
	GENERATED_BODY()

	UValidationTestObject_GameplayAttribute()
	{
		EmptyAttributeArray.AddDefaulted();
	}

	UPROPERTY(EditAnywhere, meta = (Validate))
	FGameplayAttribute EmptyAttribute;

	UPROPERTY(EditAnywhere, meta = (Validate))
	TArray<FGameplayAttribute> EmptyAttributeArray;

	UPROPERTY(EditAnywhere, meta = (Validate))
	FGameplayAttributeStruct Struct;
};

USTRUCT()
struct FDataTableRowStruct
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, meta = (Validate))
	FDataTableRowHandle EmptyRow;
};

UCLASS()
class UValidationTestObject_DataTableRow: public UObject
{
	GENERATED_BODY()

	UValidationTestObject_DataTableRow()
	{
		EmptyRowArray.AddDefaulted();
	}

	UPROPERTY(EditAnywhere, meta = (Validate))
	FDataTableRowHandle EmptyRow;

	UPROPERTY(EditAnywhere, meta = (Validate))
	TArray<FDataTableRowHandle> EmptyRowArray;

	UPROPERTY(EditAnywhere, meta = (Validate))
	FDataTableRowStruct Struct;
};