#include "NeovimSourceCodeAccessModule.h"
#include "Features/IModularFeatures.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_MODULE(FNeovimSourceCodeAccessModule, NeovimSourceCodeAccess);

DEFINE_LOG_CATEGORY_STATIC(LogNeovimSourceCodeAccessModule, Log, All);

#define LOCTEXT_NAMESPACE "NeovimSourceCodeAccessor"

FNeovimSourceCodeAccessModule::FNeovimSourceCodeAccessModule()
	: NeovimSourceCodeAccessor(MakeShareable(new FNeovimSourceCodeAccessor()))
{
}

void FNeovimSourceCodeAccessModule::StartupModule()
{
	UE_LOG(LogNeovimSourceCodeAccessModule, Log, TEXT("[NeovimSourceCodeAccess] StartupModule BEGIN"));
	NeovimSourceCodeAccessor->Startup();

	// Bind our source control provider to the editor
	UE_LOG(LogNeovimSourceCodeAccessModule, Log, TEXT("[NeovimSourceCodeAccess] Registering accessor BEGIN"));
	IModularFeatures::Get().RegisterModularFeature(TEXT("SourceCodeAccessor"),
											&NeovimSourceCodeAccessor.Get());
	UE_LOG(LogNeovimSourceCodeAccessModule, Log, TEXT("[NeovimSourceCodeAccess] Registering accessor END"));
	UE_LOG(LogNeovimSourceCodeAccessModule, Log, TEXT("[NeovimSourceCodeAccess] StartupModule END"));
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
