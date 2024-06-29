#include "PropertyValidationSettings.h"

#include "PropertyExtensionTypes.h"
#include "PropertyValidators/PropertyValidation.h"

UPropertyValidationSettings::UPropertyValidationSettings(const FObjectInitializer& Initializer): Super(Initializer)
{
	
}

void UPropertyValidationSettings::PostInitProperties()
{
	Super::PostInitProperties();

	LoadConfig();
}

void UPropertyValidationSettings::PostReloadConfig(FProperty* PropertyThatWasLoaded)
{
	Super::PostReloadConfig(PropertyThatWasLoaded);

	if (PropertyThatWasLoaded->GetFName() == GET_MEMBER_NAME_CHECKED(ThisClass, PropertyExtensions))
	{
		LoadConfig();
	}
}

const TArray<FEnginePropertyExtension>& UPropertyValidationSettings::GetPropertyExtensions(const UStruct* Struct)
{
	auto Settings = Get();
	Settings->UpdatePropertyExtensionMap();
	
	const TArray<FEnginePropertyExtension> EmptyArray;
	return Settings->PropertyExtensionMap.FindOrAdd(FSoftObjectPath{Struct});
}

bool UPropertyValidationSettings::ShouldSkipPackage(const UPackage* Package)
{
	check(Package);
	return Get()->bSkipBlueprintGeneratedClasses && UE::AssetValidation::IsBlueprintGeneratedPackage(Package->GetName());
}

bool UPropertyValidationSettings::ShouldIgnorePackage(const UPackage* Package)
{
	check(Package);
	const FString PackageName = Package->GetName();
	// always allow validation for project package
	const FString ProjectPackage = FString::Printf(TEXT("/Script/%s"), FApp::GetProjectName());
	if (PackageName.StartsWith(ProjectPackage))
	{
		return false;
	}
	
#if WITH_ASSET_VALIDATION_TESTS
	if (PackageName.StartsWith("/Script/AssetValidation") || PackageName.StartsWith("/AssetValidation"))
	{
		return false;
	}
#endif
	
	// package is blueprint generated if it is either in Content folder or Plugins/Content folder
	return Get()->PackagesToIgnore.ContainsByPredicate([PackageName](const FString& ModulePath)
	{
		return PackageName.StartsWith(ModulePath);
	});
}

bool UPropertyValidationSettings::ShouldIteratePackage(const UPackage* Package)
{
	check(Package);
	const FString PackageName = Package->GetName();
	// allow validation for project package
	const FString ProjectPackage = FString::Printf(TEXT("/Script/%s"), FApp::GetProjectName());
	if (PackageName.StartsWith(ProjectPackage))
	{
		return true;
	}
	
#if WITH_ASSET_VALIDATION_TESTS
	if (PackageName.StartsWith("/Script/AssetValidation") || PackageName.StartsWith("/AssetValidation"))
	{
		return true;
	}
#endif
	
	return Get()->PackagesToIterate.ContainsByPredicate([PackageName](const FString& ModulePath)
	{
		return PackageName.StartsWith(ModulePath);
	});
}

void UPropertyValidationSettings::LoadConfig()
{
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		auto FindOrAdd = []<typename ContainerType, typename PredType>(ContainerType& Container, PredType&& Pred) -> typename ContainerType::ElementType&
		{
			if (typename ContainerType::ElementType* ElemPtr = Container.FindByPredicate(Pred))
			{
				return *ElemPtr;
			}

			return Container.AddDefaulted_GetRef();
		};

		ClassExtensions.Reset();
		StructExtensions.Reset();
		
		for (const FPropertyExtensionConfig& Elem: PropertyExtensions)
		{
			FEnginePropertyExtension PropertyExtension{Elem};
			if (!PropertyExtension.IsValid())
			{
				continue;
			}

			if (UScriptStruct* ScriptStruct = Cast<UScriptStruct>(PropertyExtension.Struct))
			{
				FEngineStructExtension& StructExtension = FindOrAdd(StructExtensions,
					[ScriptStruct](const FEngineStructExtension& Extension) { return Extension.Struct == ScriptStruct; });
				StructExtension.Struct = ScriptStruct;
				StructExtension.Properties.Add(PropertyExtension);
			}
			else if (UClass* Class = Cast<UClass>(PropertyExtension.Struct))
			{
				FEngineClassExtension& ClassExtension = FindOrAdd(ClassExtensions,
					[Class](const FEngineClassExtension& Extension) { return Extension.Class == Class; });
				ClassExtension.Class = Class;
				ClassExtension.Properties.Add(PropertyExtension);
			}
		}
	}

	MarkExtensionMapDirty();
}

void UPropertyValidationSettings::StoreConfig()
{
	PropertyExtensions.Reset(ClassExtensions.Num() + StructExtensions.Num());

	auto IsExtensionValid = [](const FEnginePropertyExtension& Extension) { return Extension.IsValid(); };
	for (const FEngineClassExtension& ClassExtension: ClassExtensions)
	{
		if (ClassExtension.IsValid())
		{
			Algo::CopyIf(ClassExtension.Properties, PropertyExtensions, IsExtensionValid);
		}
	}

	for (const FEngineStructExtension& StructExtension: StructExtensions)
	{
		if (StructExtension.IsValid())
		{
			Algo::CopyIf(StructExtension.Properties, PropertyExtensions, IsExtensionValid);
		}
	}

	MarkExtensionMapDirty();
	TryUpdateDefaultConfigFile();
}

void UPropertyValidationSettings::UpdatePropertyExtensionMap() const
{
	if (!bExtensionMapDirty)
	{
		return;
	}
	
	bExtensionMapDirty = false;
	PropertyExtensionMap.Reset();

	auto IsExtensionValid = [](const FEnginePropertyExtension& Extension) { return Extension.IsValid(); };
	for (const FEngineClassExtension& ClassExtension: ClassExtensions)
	{
		if (ClassExtension.IsValid())
		{
			TArray<FEnginePropertyExtension>& Extensions = PropertyExtensionMap.Add(FSoftObjectPath{ClassExtension.Class});
			Extensions.Reserve(ClassExtension.Properties.Num());
			
			Algo::CopyIf(ClassExtension.Properties, Extensions, IsExtensionValid);
		}
	}

	for (const FEngineStructExtension& StructExtension: StructExtensions)
	{
		if (StructExtension.IsValid())
		{
			TArray<FEnginePropertyExtension>& Extensions = PropertyExtensionMap.Add(FSoftObjectPath{StructExtension.Struct});
			Extensions.Reserve(StructExtension.Properties.Num());
			
			Algo::CopyIf(StructExtension.Properties, Extensions, IsExtensionValid);
		}
	}
}


void UPropertyValidationSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// @todo: move to detail customization instead
	for (FEngineClassExtension& ClassExtension: ClassExtensions)
	{
		for (FEnginePropertyExtension& PropertyData: ClassExtension.Properties)
		{
			PropertyData.Struct = ClassExtension.Class.Get();
		}
	}

	// @todo: move to detail customization instead
	for (FEngineStructExtension& StructExtension: StructExtensions)
	{
		for (FEnginePropertyExtension& PropertyData: StructExtension.Properties)
		{
			PropertyData.Struct = StructExtension.Struct;
		}
	}

	const FName PropertyName = PropertyChangedEvent.GetMemberPropertyName();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, ClassExtensions) || PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, StructExtensions))
	{
		StoreConfig();
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, PackagesToIgnore) ||
			PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, PackagesToIterate) ||
			PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, bSkipBlueprintGeneratedClasses))
	{
		OnPackageTraitsChanged.Broadcast();
	}
}
