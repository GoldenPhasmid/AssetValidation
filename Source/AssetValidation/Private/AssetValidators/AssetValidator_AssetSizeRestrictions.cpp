#include "AssetValidators/AssetValidator_AssetSizeRestrictions.h"

#include "AssetDependencyTree.h"
#include "AssetValidationDefines.h"
#include "AssetValidationStatics.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Misc/EnumerateRange.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

UAssetValidator_AssetSizeRestrictions::UAssetValidator_AssetSizeRestrictions()
{
	bIsConfigDisabled = false; // enabled by default

	bCanRunParallelMode		= false;
	bRequiresLoadedAsset	= true;
	bRequiresTopLevelAsset	= true;
	bCanValidateActors		= false;
}

void UAssetValidator_AssetSizeRestrictions::PostInitProperties()
{
	Super::PostInitProperties();

	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		BuildAssetTypeMap();
	}
}

void UAssetValidator_AssetSizeRestrictions::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	BuildAssetTypeMap();
}

bool UAssetValidator_AssetSizeRestrictions::IsEnabled() const
{
	return Super::IsEnabled() && !AssetTypes.IsEmpty();
}

bool UAssetValidator_AssetSizeRestrictions::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const
{
	if (!Super::CanValidateAsset_Implementation(InAssetData, InObject, InContext))
	{
		return false;
	}

	check(InObject);
	if (AssetMap.Contains(TSoftObjectPtr{InObject}.ToSoftObjectPath()))
	{
		return true;
	}
	
	const UClass* AssetClass = GetCppAssetClass(InObject);
	if (AssetClass != nullptr && AssetTypeMap.Contains(AssetClass->GetClassPathName()))
	{
		return true;
	}

	return false;
}

EDataValidationResult UAssetValidator_AssetSizeRestrictions::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL(UAssetValidator_AssetSizeRestrictions, AssetValidationChannel)

	EDataValidationResult Result = EDataValidationResult::Valid;
	
	const FText ClassNameText = FText::FromString(GetClass()->GetName());
	if (!AssetExceptions.IsEmpty() && AssetExceptions.Contains(TSoftObjectPtr{InAsset}))
	{
		const FText Message = FText::Format(LOCTEXT("AssetSizeRestrictions_Exception", "{0}: Asset is listed as an exception and will be skipped."), ClassNameText);
		UE::AssetValidation::AddTokenMessage(Context, EMessageSeverity::Info, InAssetData, Message);
		
		return Result;
	}
	
	// @todo: reuse dependency tree between calls
	FAssetDependencyTree AssetTree{};

	FAssetAuditResult AuditResult{};
	if (!AssetTree.AuditAsset(*IAssetRegistry::Get(), InAssetData, AuditResult))
	{
		UE::AssetValidation::AddTokenMessage(Context, EMessageSeverity::Error, InAssetData, LOCTEXT("AssetSizeRestrictions_AuditFailed", "Failed to audit asset. Unknown error."));
		return EDataValidationResult::Invalid;
	}

	constexpr float MB = 1.0 / 1024.0 / 1024.0;
	const float Memory = AuditResult.TotalMemorySizeBytes * MB;
	const float DiskMemory = AuditResult.TotalDiskSizeBytes * MB;

	float MaxMemory = 0.f, MaxDiskMemory = 0.f;
	GetAssetRestrictions(InAsset, MaxMemory, MaxDiskMemory);

	if (Memory > MaxMemory)
	{
		const FText Message = FText::Format(LOCTEXT("AssetSizeRestrictions_MemoryExceeded", "{0}: Asset memory size exceeds a hard set limit: {1}MB > {2}MB. "
																					  "To fix the issue, remove hard dependencies to other blueprints (most likely caused by blueprint casts), store asset resources by soft pointers  or consult a programming team."),
			ClassNameText, FText::FromString(FString::Printf(TEXT("%.2f"), Memory)), FText::FromString(FString::FromInt(MaxMemory)));
		UE::AssetValidation::AddTokenMessage(Context, EMessageSeverity::Error, InAssetData, Message);
		
		Result &= EDataValidationResult::Invalid;
	}
	
	if (DiskMemory > MaxDiskMemory)
	{
		const FText Message = FText::Format(LOCTEXT("AssetSizeRestrictions_MemoryExceeded", "{0}: Asset disk size exceeds a hard set limit: {1}MB > {2}MB. "
																					  "To fix the issue, remove hard dependencies to other assets (most likely caused by blueprint casts), store asset resources by soft pointers or consult a programming team."),
			ClassNameText, FText::FromString(FString::Printf(TEXT("%.2f"), DiskMemory)), FText::FromString(FString::FromInt(MaxDiskMemory)));
		UE::AssetValidation::AddTokenMessage(Context, EMessageSeverity::Error, InAssetData, Message);
		
		Result &= EDataValidationResult::Invalid;
	}

	return Result;
}

UClass* UAssetValidator_AssetSizeRestrictions::GetCppAssetClass(const UObject* Asset) const
{
	if (const UBlueprint* Blueprint = Cast<UBlueprint>(Asset))
	{
		return UE::AssetValidation::GetCppBaseClass(Blueprint->GeneratedClass);
	}

	return Asset->GetClass();
}

void UAssetValidator_AssetSizeRestrictions::BuildAssetTypeMap()
{
	AssetMap.Reset();
	for (TConstEnumerateRef<FAssetSizeRestriction> Entry: EnumerateRange(Assets))
	{
		AssetMap.Add(Entry->Asset, Entry.GetIndex());
	}
	
	AssetTypeMap.Reset();
	for (TConstEnumerateRef<FAssetTypeSizeRestriction> Entry: EnumerateRange(AssetTypes))
	{
		AssetTypeMap.Add(Entry->ClassFilter.GetAssetPath(), Entry.GetIndex());
	}
}

void UAssetValidator_AssetSizeRestrictions::GetAssetRestrictions(const UObject* Asset, float& OutMemory, float& OutDiskMemory) const
{
	check(Asset);
	if (const int32* Index = AssetMap.Find(Asset))
	{
		OutMemory = Assets[*Index].MaxMemorySizeMegaBytes;
		OutDiskMemory = Assets[*Index].MaxDiskSizeMegaBytes;
		return;
	}

	if (UClass* Class = GetCppAssetClass(Asset))
	{
		if (const int32* Index = AssetTypeMap.Find(Class->GetClassPathName()))
		{
			OutMemory = AssetTypes[*Index].MaxMemorySizeMegaBytes;
			OutDiskMemory = AssetTypes[*Index].MaxDiskSizeMegaBytes;
			return;
		}
	}

	OutMemory = OutDiskMemory = 0.f;
}

#undef LOCTEXT_NAMESPACE
