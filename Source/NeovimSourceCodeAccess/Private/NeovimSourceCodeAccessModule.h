#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "NeovimSourceCodeAccessor.h"

class FNeovimSourceCodeAccessModule : public IModuleInterface
{
public:
	FNeovimSourceCodeAccessModule();

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	FNeovimSourceCodeAccessor& GetAccessor();

private:
	TSharedRef<FNeovimSourceCodeAccessor> NeovimSourceCodeAccessor;
};
