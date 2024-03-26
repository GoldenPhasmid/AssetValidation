#pragma once

#include "CoreMinimal.h"
#include "WorkflowOrientedApp/WorkflowTabFactory.h"

class FBlueprintEditor;

class FPropertyValidationTabSummoner: public FWorkflowTabFactory
{
public:
	FPropertyValidationTabSummoner(TSharedPtr<FBlueprintEditor> InBlueprintEditor);

	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override;

protected:
	TWeakPtr<FBlueprintEditor> BlueprintEditor;
};
