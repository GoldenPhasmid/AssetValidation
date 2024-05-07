#include "SourceControlProxy.h"

#include "ISourceControlModule.h"
#include "ISourceControlProvider.h"
#include "SourceControlOperations.h"

namespace UE::AssetValidation
{
	/** update source control file state with update operation */
	bool UpdateOpenedOnlyFiles(ISourceControlProvider& SSCProvider)
	{
		TSharedRef<FUpdateStatus> UpdateOperation = ISourceControlOperation::Create<FUpdateStatus>();
		// affects only opened/edited files, basically a git status operation on root folder
		UpdateOperation->SetGetOpenedOnly(true);
		
		ECommandResult::Type Result = SSCProvider.Execute(UpdateOperation, EConcurrency::Synchronous);
		return Result == ECommandResult::Succeeded;
	}
	
	void FGitSourceControlProxy::GetOpenedFiles(TArray<FSourceControlStateRef>& OutOpenedFiles)
	{
		check(ISourceControlModule::Get().IsEnabled());
		
		ISourceControlProvider& SSCProvider = ISourceControlModule::Get().GetProvider();
		UpdateOpenedOnlyFiles(SSCProvider);
			
		TArray<FSourceControlStateRef> FileStates = SSCProvider.GetCachedStateByPredicate([](const FSourceControlStateRef& State)
		{
			// in git, Modified means any state except Unchanged, NotControlled and Ignored
			return State->IsModified();
		});
		OutOpenedFiles.Append(FileStates);
	}

	void FPerforceSourceControlProxy::GetOpenedFiles(TArray<FSourceControlStateRef>& OutOpenedFiles)
	{
		check(ISourceControlModule::Get().IsEnabled());
		
		ISourceControlProvider& SSCProvider = ISourceControlModule::Get().GetProvider();
		UpdateOpenedOnlyFiles(SSCProvider);
			
		TArray<FSourceControlStateRef> FileStates = SSCProvider.GetCachedStateByPredicate([](const FSourceControlStateRef& State)
		{
			// perforce query version
			return State->IsCheckedOut() || State->IsAdded() || State->IsDeleted();
		});
		OutOpenedFiles.Append(FileStates);
	}
}
