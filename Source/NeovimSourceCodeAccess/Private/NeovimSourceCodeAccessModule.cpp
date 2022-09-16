#include "NeovimSourceCodeAccessModule.h"
#include "Features/IModularFeatures.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_MODULE(FNeovimSourceCodeAccessModule, NeovimSourceCodeAccess);

#define LOCTEXT_NAMESPACE "NeovimSourceCodeAccessor"

FNeovimSourceCodeAccessModule::FNeovimSourceCodeAccessModule()
	: NeovimSourceCodeAccessor(MakeShareable(new FNeovimSourceCodeAccessor()))
{
}

void FNeovimSourceCodeAccessModule::StartupModule()
{
	NeovimSourceCodeAccessor->Startup();

	// Bind our source control provider to the editor
	IModularFeatures::Get().RegisterModularFeature(TEXT("SourceCodeAccessor"),
												&NeovimSourceCodeAccessor.Get());
}

void FNeovimSourceCodeAccessModule::ShutdownModule()
{
	// unbind provider from editor
	IModularFeatures::Get().UnregisterModularFeature(TEXT("SourceCodeAccessor"),
												&NeovimSourceCodeAccessor.Get());

	NeovimSourceCodeAccessor->Shutdown();
}

FNeovimSourceCodeAccessor& FNeovimSourceCodeAccessModule::GetAccessor()
{
	return NeovimSourceCodeAccessor.Get();
}

#undef LOCTEXT_NAMESPACE
