#pragma once

#include "CoreMinimal.h"
#include "WorkflowOrientedApp/WorkflowTabFactory.h"

class FBlueprintEditor;

namespace UE::AssetValidation
{

class FPropertyValidationTabSummoner: public FWorkflowTabFactory
{
public:
	FPropertyValidationTabSummoner(TSharedPtr<FBlueprintEditor> InBlueprintEditor);

	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override;

protected:
	TWeakPtr<FBlueprintEditor> BlueprintEditor;
};
	
}

