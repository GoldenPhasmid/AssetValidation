#pragma once

#include "ISourceControlState.h"

namespace UE::AssetValidation
{
/**
 * Source Control Proxy
 * Abstract away some implementation details when asset validation systems interact with different SC implementations
 */
class ISourceControlProxy: public TSharedFromThis<ISourceControlProxy, ESPMode::ThreadSafe>
{
public:
    virtual ~ISourceControlProxy() {}
	virtual void GetOpenedFiles(TArray<FSourceControlStateRef>& OutOpenedFiles) {}
};

class FNullSourceControlProxy: public ISourceControlProxy
{
public:
	virtual ~FNullSourceControlProxy() override {}
	virtual void GetOpenedFiles(TArray<FSourceControlStateRef>& OutOpenedFiles) override {}
};

class FGitSourceControlProxy: public ISourceControlProxy
{
public:
	virtual ~FGitSourceControlProxy() override {}
	virtual void GetOpenedFiles(TArray<FSourceControlStateRef>& OutOpenedFiles) override;
};

class FPerforceSourceControlProxy: public ISourceControlProxy
{
public:
	virtual ~FPerforceSourceControlProxy() override {}
	virtual void GetOpenedFiles(TArray<FSourceControlStateRef>& OutOpenedFiles) override;
};
	
} // UE::AssetValidation


