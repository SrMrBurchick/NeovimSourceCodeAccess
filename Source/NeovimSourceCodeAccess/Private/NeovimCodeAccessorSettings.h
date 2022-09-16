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
	//~ UObject interface
	virtual bool CanEditChange(const FProperty* InProperty) const override;
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;

	//~ UDeveloperSettings interface
	virtual FText GetSectionText() const override;
#endif

	/** The neovim server address that the code accessor should bind to */
	UPROPERTY(config, EditAnywhere, Category = Neovim, meta = (ConfigRestartRequired=true, DisplayName = "Neovim remote server url"))
	FString RemoteExecutionURL;
	UPROPERTY(config, EditAnywhere, Category = Terminal, meta = (DisplayName = "Start in terminal"))
	bool bStartInTerminal;
	UPROPERTY(config, EditAnywhere, Category = Terminal, AdvancedDisplay, meta = (ConfigRestartRequired=true, DisplayName = "Terminal path"))
	FString RemoteExecutionTerminal;
	UPROPERTY(config, EditAnywhere, Category = Terminal, AdvancedDisplay, meta = (ConfigRestartRequired=true, DisplayName = "Terminal options to execute command"))
	FString RemoteExecutionTerminalOpts;


};
