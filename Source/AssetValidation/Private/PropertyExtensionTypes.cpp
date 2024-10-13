#include "PropertyExtensionTypes.h"

#include "AssetValidationDefines.h"
#include "PropertyValidators/PropertyValidation.h"

FString MakeMetaDataMessage(const FString& Key, const FString& Value)
{
	FString MetaData{};
	if (Value.IsEmpty())
	{
		MetaData += Key;
	}
	else
	{
		MetaData += Key + TEXT("=") + Value;
	}
	MetaData += TEXT(";");
	return MetaData;
}

FPropertyMetaDataExtensionConfig::FPropertyMetaDataExtensionConfig(const FPropertyMetaDataExtension& Extension)
	: Class(Extension.Struct)
	, Property(Extension.GetProperty()->GetFName())
{
	auto MetaDataMap = Extension.MetaDataMap;
	for (const FName& MetaKey: UE::AssetValidation::GetMetaKeys())
	{
		if (const FString* MetaValue = MetaDataMap.Find(MetaKey))
		{
			MetaData += MakeMetaDataMessage(MetaKey.ToString(), *MetaValue);
			MetaDataMap.Remove(MetaKey);
		}
	}
	
	TArray<TPair<FName, FString>> MetaDataArray = MetaDataMap.Array();
	// sort by name to guarantee the same order
	Algo::SortBy(MetaDataArray,
		[](const TPair<FName, FString>& Pair) { return Pair.Key; },
		[](const FName& Lhs, const FName& Rhs) { return Lhs.Compare(Rhs) < 0; }
	);

	// append remaining metadata which is not related to asset validation meta keys.
	for (auto& [MetaKey, MetaValue]: MetaDataArray)
	{
		MetaData += MakeMetaDataMessage(MetaKey.ToString(), MetaValue);
	}

	MetaData.RemoveFromEnd(TEXT(";"));
}

FPropertyMetaDataExtension::FPropertyMetaDataExtension(const FPropertyMetaDataExtensionConfig& Config)
{
	Struct = Cast<UStruct>(Config.Class.TryLoad());
	if (Struct == nullptr)
	{
		UE_LOG(LogAssetValidation, Error,	TEXT("%s: Failed to load class %s. Update PropertyExtensions config."),
											*FString(__FUNCTION__), *Config.Class.GetAssetPath().ToString());
		return;
	}

	PropertyPath = Struct->FindPropertyByName(Config.Property);
	if (PropertyPath == nullptr)
	{
		UE_LOG(LogAssetValidation, Error,	TEXT("%s: Failed to find property %s from class %s. Update PropertyExtensions config."),
											*FString(__FUNCTION__), *Config.Property.ToString(), *GetNameSafe(Struct));
		return;
	}

	TArray<FString> MetaDataArray;
	Config.MetaData.ParseIntoArray(MetaDataArray, TEXT(";"));

	const FName* Data = UE::AssetValidation::GetMetaKeys().GetData();
	TArrayView<const FName> MetaKeys{Data, UE::AssetValidation::GetMetaKeys().Num()};
	
	for (const FString& MetaData: MetaDataArray)
	{
		if (MetaData.IsEmpty())
		{
			continue;
		}
		
		if (MetaKeys.Contains(MetaData))
		{
			MetaDataMap.Add(FName{MetaData});
		}
		else if (FString OutKey{}, OutValue{}; MetaData.Split("=", &OutKey, &OutValue))
		{
			MetaDataMap.Add(FName{OutKey}, OutValue);
		}
		else
		{
			UE_LOG(LogAssetValidation, Error,	TEXT("%s: Failed to parse meta data %s for class %s. Update PropertyExtensions config."),
												*FString(__FUNCTION__), *MetaData, *GetNameSafe(Struct));
		}
	}

	// @todo: this happens too early for validation to properly validate it
#if 0
	const FProperty* Property = GetProperty();
	for (auto It = MetaDataMap.CreateIterator(); It; ++It)
	{
		if (UE::AssetValidation::CanApplyMeta(GetProperty(), It->Key) == false)
		{
			UE_LOG(LogAssetValidation, Error,	TEXT("%s: meta %s can't be applied to property %s:%s. Update PropertyExtensions config."),
												*FString(__FUNCTION__), *It->Key.ToString(), *Struct->GetName(), *Property->GetName());
			It.RemoveCurrent();
		}
	}
#endif
}

FSimpleDelegate UPropertyMetaDataExtensionSet::OnPropertyMetaDataChanged{};

void UPropertyMetaDataExtensionSet::PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.GetPropertyName();
	const FName MemberName = PropertyChangedEvent.GetMemberPropertyName();

	if (PropertyName == GET_MEMBER_NAME_CHECKED(FUClassMetaDataExtension, Class))
	{
		const int32 Index = PropertyChangedEvent.GetArrayIndex(GET_MEMBER_NAME_CHECKED(ThisClass, ClassExtensions).ToString());
		check(ClassExtensions.IsValidIndex(Index));
		
		ClassExtensions[Index].Properties.Reset();
	}

	if (PropertyName == GET_MEMBER_NAME_CHECKED(FUScriptStructMetaDataExtension, Struct))
	{
		const int32 Index = PropertyChangedEvent.GetArrayIndex(GET_MEMBER_NAME_CHECKED(ThisClass, StructExtensions).ToString());
		check(StructExtensions.IsValidIndex(Index));
		
		StructExtensions[Index].Properties.Reset();
	}

	// @todo: move to detail customization instead
	for (FUClassMetaDataExtension& ClassExtension: ClassExtensions)
	{
		for (FPropertyMetaDataExtension& PropertyExtension: ClassExtension.Properties)
		{
			PropertyExtension.Struct = ClassExtension.Class.Get();
		}
	}
	
	// @todo: move to detail customization instead
	for (FUScriptStructMetaDataExtension& StructExtension: StructExtensions)
	{
		for (FPropertyMetaDataExtension& PropertyExtension: StructExtension.Properties)
		{
			PropertyExtension.Struct = StructExtension.Struct;
		}
	}

	OnPropertyMetaDataChanged.ExecuteIfBound();
}

void UPropertyMetaDataExtensionSet::FillPropertyMap(TMap<FSoftObjectPath, TArray<FPropertyMetaDataExtension>>& ExtensionMap)
{
	auto IsPropertyExtValid = [](const FPropertyMetaDataExtension& Extension) { return Extension.IsValid(); };
	for (const FUClassMetaDataExtension& ClassExtension: ClassExtensions)
	{
		if (ClassExtension.IsValid())
		{
			TArray<FPropertyMetaDataExtension>& Extensions = ExtensionMap.Add(FSoftObjectPath{ClassExtension.Class});
			Extensions.Reserve(ClassExtension.Properties.Num());
			
			Algo::CopyIf(ClassExtension.Properties, Extensions, IsPropertyExtValid);
		}
	}

	for (const FUScriptStructMetaDataExtension& StructExtension: StructExtensions)
	{
		if (StructExtension.IsValid())
		{
			TArray<FPropertyMetaDataExtension>& Extensions = ExtensionMap.Add(FSoftObjectPath{StructExtension.Struct});
			Extensions.Reserve(StructExtension.Properties.Num());
			
			Algo::CopyIf(StructExtension.Properties, Extensions, IsPropertyExtValid);
		}
	}
}
