#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "PropertyValidationSettings.generated.h"

struct FPropertyMetaDataExtensionConfig;
struct FPropertyMetaDataExtension;
struct FUClassMetaDataExtension;
struct FUScriptStructMetaDataExtension;

UCLASS(Config = Editor, DefaultConfig, DisplayName = "Property Validation")
class ASSETVALIDATION_API UPropertyValidationSettings: public UDeveloperSettings
{
	GENERATED_BODY()
public:

	UPropertyValidationSettings(const FObjectInitializer& Initializer);

	FORCEINLINE static const UPropertyValidationSettings* Get()		{ return GetDefault<UPropertyValidationSettings>(); }
	FORCEINLINE static UPropertyValidationSettings* GetMutable()	{ return GetMutableDefault<UPropertyValidationSettings>(); }
	
	virtual void PostInitProperties() override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	/**
	 * List of packages asset property validator will ignore
	 * Can be any content that doesn't have properties (e.g. /Game/Meshes, /Game/Materials, etc.)
	 */
	UPROPERTY(EditAnywhere, Config, Category = "Settings", meta = (Validate))
	TArray<FString> PackagesToIgnore;

	/**
	 * List of packages that should be validated using class property iteration
	 * Include any C++/Blueprint packages that your team can edit (project, project plugins, blueprints)
	 * For validating packages that you don't have edit access (engine code & engine plugins), see @ExternalClasses/@ExternalStructs
	 */
	UPROPERTY(EditAnywhere, Config, Category = "Settings", meta = (Validate))
	TArray<FString> PackagesToIterate;

	UPROPERTY(EditAnywhere, Config, Category = "Settings", meta = (Validate, ConfigRestartRequired = "true"))
	TArray<FString> PropertyExtensionPaths;

	/** If set, project module path is included to the list of paths for property validation */
	UPROPERTY(EditAnywhere, Config, Category = "Settings")
	bool bValidateProject = true;

	/** If set, project plugins paths is included  to the list of paths for property validation */
	UPROPERTY(EditAnywhere, Config, Category = "Settings")
	bool bValidateProjectPlugins = true;
	
	/**
	 * if set to true, allows unwrapping and iterating over struct inner properties without "ValidateRecursive" meta specifier
	 * Desired default behavior. It means users will require placing any metas on struct properties only if they want to validate an actual value, e.g. FGameplayTag or FGameplayAttribute
	 * It allows to omit requirement to create ValidateRecursive chains, where child most property can be validated only if all parents have ValidateRecursive
	 * Enabled by default
	 */
	UPROPERTY(EditAnywhere, Config, Category = "Settings")
	bool bAutoValidateStructInnerProperties = true;

	/**
	 * If set to true, will skip ALL blueprint generated classes, meaning all blueprints. Validation will start from first cpp base.
	 * Can slightly speed up validation if project doesn't use blueprint property validation that much, or uses deep blueprint hierarchies
	 * Disabled by default
	 */
	UPROPERTY(EditAnywhere, Config, Category = "Settings")
	bool bSkipBlueprintGeneratedClasses = false;

	/**
	 * If set to true, will automatically add "Validate" and "ValidateRecursive" meta specifiers when new blueprint component is added to a component tree
	 * Enabled by default
	 */
	UPROPERTY(EditAnywhere, Config, Category = "Settings")
	bool bAddMetaToNewBlueprintComponents = true;

	/**
	 * If set to true, will automatically add "Validate" and "ValidateRecursive" meta specifiers to newly created blueprint variables, if type supports
	 * Enabled by default
	 */
	UPROPERTY(EditAnywhere, Config, Category = "Settings")
	bool bAddMetaToNewBlueprintVariables = true;

	/**
	 * If set to true, property is validated only if it is blueprint visible
	 * Enabled by default
	 */
	UPROPERTY(EditAnywhere, Config, Category = "Settings")
	bool bRequirePropertyBlueprintVisibility = true;

	/**
	 * If set to true, will report incorrect meta usage for C++ and Blueprint properties.
	 * Enabled by default
	 */
	UPROPERTY(EditAnywhere, Config, Category = "Settings")
	bool bReportIncorrectMetaUsage = true;
	
};
