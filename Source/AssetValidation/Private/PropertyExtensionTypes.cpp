#include "PropertyExtensionTypes.h"

#include "AssetValidationDefines.h"
#include "PropertyValidators/PropertyValidation.h"

FPropertyExtensionConfig::FPropertyExtensionConfig(const FEnginePropertyExtension& Extension)
	: Class(Extension.Struct)
	, Property(Extension.GetProperty()->GetFName())
{
	auto MakeMessage = [](const FString& Key, const FString& Value)
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
	};
	
	auto MetaDataMap = Extension.MetaDataMap;
	for (const FName& MetaKey: UE::AssetValidation::GetMetaKeys())
	{
		if (const FString* MetaValue = MetaDataMap.Find(MetaKey))
		{
			MetaData += MakeMessage(MetaKey.ToString(), *MetaValue);
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
		MetaData += MakeMessage(MetaKey.ToString(), MetaValue);
	}

	MetaData.RemoveFromEnd(TEXT(";"));
}

FEnginePropertyExtension::FEnginePropertyExtension(const FPropertyExtensionConfig& Config)
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
