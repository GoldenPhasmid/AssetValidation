#include "PropertyValidationSettings.h"

#include "AssetValidationDefines.h"
#include "PropertyExtensionTypes.h"

UPropertyValidationSettings::UPropertyValidationSettings(const FObjectInitializer& Initializer): Super(Initializer)
{
	UE_LOG(LogAssetValidation, Warning, TEXT(""));
}

void UPropertyValidationSettings::PostInitProperties()
{
	Super::PostInitProperties();

	ConvertConfig();
}

void UPropertyValidationSettings::PostReloadConfig(FProperty* PropertyThatWasLoaded)
{
	Super::PostReloadConfig(PropertyThatWasLoaded);

	if (PropertyThatWasLoaded->GetFName() == GET_MEMBER_NAME_CHECKED(ThisClass, PropertyExtensions))
	{
		ConvertConfig();
	}
}

void UPropertyValidationSettings::ConvertConfig()
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
}

void UPropertyValidationSettings::StoreConfig()
{
	PropertyExtensions.Reset();
	
	for (FEngineClassExtension& ClassExtension: ClassExtensions)
	{
		Algo::CopyIf(ClassExtension.Properties, PropertyExtensions, [](const FEnginePropertyExtension& Prop) { return Prop.IsValid(); });
	}

	for (FEngineStructExtension& StructExtension: StructExtensions)
	{
		Algo::CopyIf(StructExtension.Properties, PropertyExtensions, [](const FEnginePropertyExtension& Prop) { return Prop.IsValid(); });
	}

	TryUpdateDefaultConfigFile();
}


void UPropertyValidationSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	
	for (FEngineClassExtension& ClassExtension: ClassExtensions)
	{
		for (FEnginePropertyExtension& PropertyData: ClassExtension.Properties)
		{
			PropertyData.Struct = ClassExtension.Class.Get();
		}
	}

	for (FEngineStructExtension& StructExtension: StructExtensions)
	{
		for (FEnginePropertyExtension& PropertyData: StructExtension.Properties)
		{
			PropertyData.Struct = StructExtension.Struct;
		}
	}

	FName PropertyName = PropertyChangedEvent.GetMemberPropertyName();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, ClassExtensions) || PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, StructExtensions))
	{
		StoreConfig();
	}
}
