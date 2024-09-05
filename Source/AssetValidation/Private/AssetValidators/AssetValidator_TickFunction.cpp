#include "AssetValidators/AssetValidator_TickFunction.h"

#include "AssetValidationDefines.h"
#include "AssetValidationStatics.h"
#include "ScriptBlueprint.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

UAssetValidator_TickFunction::UAssetValidator_TickFunction()
{
	bIsConfigDisabled = true; // disabled by default

	bCanRunParallelMode = true;
	bRequiresLoadedAsset = true;
	bRequiresTopLevelAsset = true;
	bCanValidateActors = false;
}

bool UAssetValidator_TickFunction::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const
{
	if (!Super::CanValidateAsset_Implementation(InAssetData, InObject, InContext))
	{
		return false;
	}

	if (const UScriptBlueprint* Blueprint = Cast<UScriptBlueprint>(InObject))
	{
		const UObject* DefaultObject = Blueprint->GeneratedClass->GetDefaultObject();
		if (DefaultObject->IsA<AActor>() || DefaultObject->IsA<UActorComponent>())
		{
			return true;
		}
	}

	return false;
}

EDataValidationResult UAssetValidator_TickFunction::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL(UAssetValidator_TickFunction, AssetValidationChannel);

	EDataValidationResult Result = EDataValidationResult::Valid;

	const UBlueprint* Blueprint = CastChecked<UBlueprint>(InAsset);
	const UObject* DefaultObject = Blueprint->GeneratedClass->GetDefaultObject();
	check(DefaultObject);
	
	const FTickFunction* TickFunction = nullptr;
	if (const AActor* Actor = Cast<AActor>(DefaultObject))
	{
		TickFunction = &Actor->PrimaryActorTick;
	}
	else if (const UActorComponent* ActorComponent = Cast<UActorComponent>(DefaultObject))
	{
		TickFunction = &ActorComponent->PrimaryComponentTick;
	}
	
	if (TickFunction->bStartWithTickEnabled)
	{
		UE::AssetValidation::AddTokenMessage(Context, EMessageSeverity::Error,
			InAssetData, LOCTEXT("TickFunction_TickEnabled", "StartWithTickEnabled is set to true. "
			"If your blueprint requires tick from the start, use SetActorTickEnabled/SetComponentTickEnabled during BeginPlay.\n"
			"Disable StartWithTickEnabled to fix this error.")
		);
		Result &= EDataValidationResult::Invalid;
	}
	
	if (TickFunction->bCanEverTick)
	{
		static FName TickFunctionName{TEXT("ReceiveTick")};
		
		if (const UFunction* Func = Blueprint->GeneratedClass->FindFunctionByName(TickFunctionName))
		{
			if (const UObject* Outer = Func->GetOuter(); Outer && Outer->IsA<UBlueprintGeneratedClass>())
			{
				UE::AssetValidation::AddTokenMessage(Context, EMessageSeverity::Error,
					InAssetData, LOCTEXT("TickFunction_CanEverTick", "CanEverTick is set to false but Tick event is implemented.\n"
					"Either delete Tick event or set CanEverTick to true to fix this error.")
				);
				Result &= EDataValidationResult::Invalid;
			}
		}
	}
	
	if (Result != EDataValidationResult::Invalid)
	{
		AssetPasses(InAsset);
	}

	return Result;
}

#undef LOCTEXT_NAMESPACE