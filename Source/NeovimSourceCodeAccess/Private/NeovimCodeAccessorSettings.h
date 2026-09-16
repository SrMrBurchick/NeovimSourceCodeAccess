#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Engine/DeveloperSettings.h"

#include "NeovimCodeAccessorSettings.generated.h"

/**
 * Configure the Neovim plug-in.
 */
UCLASS(config = Engine, defaultconfig)
class UNeovimCodeAccessorSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UNeovimCodeAccessorSettings();

#if WITH_EDITOR
	//~ UDeveloperSettings interface
	virtual FText GetSectionText() const override;
#endif

	/** Neovide executable name or absolute path. */
	UPROPERTY(config, EditAnywhere, Category = Neovim, meta = (DisplayName = "Neovide executable"))
	FString NeovideExecutable;
	/** Optional RPC address override. Empty creates a stable per-project socket/pipe. */
	UPROPERTY(config, EditAnywhere, Category = Neovim, AdvancedDisplay, meta = (DisplayName = "Neovide RPC address"))
	FString NeovideRPCAddress;
};
