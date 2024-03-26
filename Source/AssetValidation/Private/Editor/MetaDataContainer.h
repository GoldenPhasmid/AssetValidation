#pragma once

#include "PropertyValidationSettings.h"
#include "Engine/Blueprint.h"

namespace UE::AssetValidation
{

class FMetaDataSource
{
public:
	FMetaDataSource() = default;
	FMetaDataSource(FProperty* Property)
	{
		Variant.Set<FProperty*>(Property);
	}
	FMetaDataSource(const FProperty* Property)
	{
		Variant.Set<FProperty*>(const_cast<FProperty*>(Property)); // @todo: remove const_cast after everything clears up
	}
	FMetaDataSource(const FPropertyExternalValidationData& PropertyData)
	{
		Variant.Set<FPropertyExternalValidationData>(PropertyData);
	}

	void SetProperty(FProperty* Property)
	{
		Variant.Set<FProperty*>(Property);
	}

	void SetExternalData(const FPropertyExternalValidationData& PropertyData)
	{
		Variant.Set<FPropertyExternalValidationData>(PropertyData);
	}
	
	bool HasMetaData(const FName& Key) const;
	FString GetMetaData(const FName& Key) const;
	void SetMetaData(const FName& Key, const FString& Value);

private:
	TVariant<FProperty*, FPropertyExternalValidationData> Variant;
};
	
class IMetaDataContainer
{
public:
	virtual ~IMetaDataContainer() {}
	virtual bool HasMetaData(const FName& Key) const = 0;
	virtual FString GetMetaData(const FName& Key) const = 0;
	virtual void SetMetaData(const FName& Key, const FString& Value) = 0;

	void SetMetaData(const FName& Key)
	{
		SetMetaData(Key, {});
	}
};


class FPropertyMetaDataContainer: public IMetaDataContainer
{
public:
	FPropertyMetaDataContainer() = default;
	FPropertyMetaDataContainer(FProperty* InProperty)
		: Property(InProperty)
	{}
	FPropertyMetaDataContainer(const FProperty* Property)
		: Property(const_cast<FProperty*>(Property)) // @todo: remove const_cast after everything clears up
	{}
	
	void SetProperty(FProperty* InProperty)
	{
		Property = InProperty;
	}

	//~Begin IMetaDataContainer interface
	virtual bool HasMetaData(const FName& Key) const override;
	virtual FString GetMetaData(const FName& Key) const override;
	virtual void SetMetaData(const FName& Key, const FString& Value) override;
	virtual ~FPropertyMetaDataContainer() override {}
	//~End IMetaDataContainer interface
	
private:
	FProperty* Property = nullptr;
};

class FExternalPropertyMetaDataContainer: public IMetaDataContainer
{
public:
	FExternalPropertyMetaDataContainer() = default;
	FExternalPropertyMetaDataContainer(const FPropertyExternalValidationData& InPropertyData)
		: PropertyData(InPropertyData)
	{}

	void SetPropertyData(const FPropertyExternalValidationData& InPropertyData)
	{
		PropertyData = InPropertyData;
	}

	//~Begin IMetaDataContainer interface
	virtual bool HasMetaData(const FName& Key) const override;
	virtual FString GetMetaData(const FName& Key) const override;
	virtual void SetMetaData(const FName& Key, const FString& Value) override;
	virtual ~FExternalPropertyMetaDataContainer() override {}
	//~End IMetaDataContainer interface
	
private:
	FPropertyExternalValidationData PropertyData;
};

class FBPVariableMetaDataContainer: public IMetaDataContainer
{
public:
	FBPVariableMetaDataContainer(const FBPVariableDescription& InDesc)
		: Desc(InDesc)
	{}
	
	
	//~Begin IMetaDataContainer interface
	virtual bool HasMetaData(const FName& Key) const override;
	virtual FString GetMetaData(const FName& Key) const override;
	virtual void SetMetaData(const FName& Key, const FString& Value) override;
	virtual ~FBPVariableMetaDataContainer() override {}
	//~End IMetaDataContainer interface
	
private:
	FBPVariableDescription Desc;
};

}

