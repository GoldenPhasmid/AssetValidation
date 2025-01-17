#pragma once

#include "CoreMinimal.h"
#include "AVCommandletAction.h"

#include "AVCommandletAction_MigrateBlueprintComponent.generated.h"

/**
 * Migrate blueprint created component to an existing C++ component
 * 1. Copies properties
 * 2. Fixes Getters/Setters for old component
 * 3. Removes blueprint created component
 * New component preferably should be AT LEAST of old component type or a child of it,
 * otherwise Function Calls for old component type will fail to compile
 */
UCLASS(DisplayName = "Migrate Blueprint Component")
class UAVCommandletAction_MigrateBlueprintComponent: public UAVCommandletAction
{
	GENERATED_BODY()
public:
	virtual bool Run(const TArray<FAssetData>& Assets) override;

	/**
	 * Performs full copy from one component to another, either blueprint or C++
	 * 1. Copies property information using CopyPropertiesForUnrelatedObjects
	 * 2. Reroutes K2 Nodes (Get/Set) to a new component
	 * 3. Reattaches any attached SCS components from old to new component
	 * 4. Marks blueprint as structurally modified
	 * @param Blueprint blueprint
	 * @param OldComponentName old component name
	 * @param NewComponentName new component name
	 * @return if successfully copied data from old to a new component 
	 */
	UFUNCTION(BlueprintCallable, Category = "Comino|Utilities")
	static bool CopyComponentProperties(UBlueprint* Blueprint, FName OldComponentName, FName NewComponentName);

	/** old component property name, component object should exist */
	UPROPERTY(EditAnywhere, Category = "Action")
	FName OldComponentName = NAME_None;

	/** new component property name, component object should exist */
	UPROPERTY(EditAnywhere, Category = "Action")
	FName NewComponentName = NAME_None;

	/** whether to delete old component if it was created via construction script */
	UPROPERTY(EditAnywhere, Category = "Action")
	bool bRemoveOldComponent = true;
};
