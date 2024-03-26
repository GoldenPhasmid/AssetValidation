#pragma once

#include "PropertyValidationSettings.h"
#include "Engine/Blueprint.h"

namespace UE::AssetValidation
{
	
class IMetaDataContainer
{
public:
	virtual ~IMetaDataContainer() {}
	virtual bool HasMetaData(const FName& Key) const = 0;
	virtual FString GetMetaData(const FName& Key) const = 0;
	virtual void SetMetaData(const FName& Key, const FString& Value) = 0;

	FORCEINLINE void SetMetaData(const FName& Key)
	{
		SetMetaData(Key, {});
	}
};


class FPropertyMetaDataContainer: public IMetaDataContainer
{
public:
	FPropertyMetaDataContainer(FProperty* InProperty)
		: Property(InProperty)
	{}
	virtual ~FPropertyMetaDataContainer() override {}

	//~Begin IMetaDataContainer interface
	virtual bool HasMetaData(const FName& Key) const override;
	virtual FString GetMetaData(const FName& Key) const override;
	virtual void SetMetaData(const FName& Key, const FString& Value) override;
	//~End IMetaDataContainer interface
	
private:
	FProperty* Property = nullptr;
};

class FEngineVariableMetaDataContainer: public IMetaDataContainer
{
public:
	FEngineVariableMetaDataContainer(const FEngineVariableDescription& InDesc)
		: Desc(InDesc)
	{}
	virtual ~FEngineVariableMetaDataContainer() override {}

	//~Begin IMetaDataContainer interface
	virtual bool HasMetaData(const FName& Key) const override;
	virtual FString GetMetaData(const FName& Key) const override;
	virtual void SetMetaData(const FName& Key, const FString& Value) override;
	//~End IMetaDataContainer interface

private:
	FEngineVariableDescription Desc;
};

class FBPVariableMetaDataContainer: public IMetaDataContainer
{
public:
	FBPVariableMetaDataContainer(const FBPVariableDescription& InDesc)
		: Desc(InDesc)
	{}
	virtual ~FBPVariableMetaDataContainer() override {}
	
	//~Begin IMetaDataContainer interface
	virtual bool HasMetaData(const FName& Key) const override;
	virtual FString GetMetaData(const FName& Key) const override;
	virtual void SetMetaData(const FName& Key, const FString& Value) override;
	//~End IMetaDataContainer interface

private:
	FBPVariableDescription Desc;
};

}

