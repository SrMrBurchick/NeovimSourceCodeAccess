#pragma once

#include "ISourceCodeAccessor.h"

class FNeovimSourceCodeAccessor : public ISourceCodeAccessor
{
public:
	FNeovimSourceCodeAccessor();

	/** Initialise internal systems, register delegates etc. */
	void Startup();

	/** Shut down internal systems, unregister delegates etc. */
	void Shutdown();

	/** ISourceCodeAccessor implementation */
	virtual void RefreshAvailability() override;
	virtual bool CanAccessSourceCode() const override;
	virtual FName GetFName() const override;
	virtual FText GetNameText() const override;
	virtual FText GetDescriptionText() const override;
	virtual bool OpenSolution() override;
	virtual bool OpenSolutionAtPath(const FString& InSolutionPath) override;
	virtual bool DoesSolutionExist() const override;
	virtual bool OpenFileAtLine(const FString& FullPath, int32 LineNumber, int32 ColumnNumber = 0) override;
	virtual bool OpenSourceFiles(const TArray<FString>& AbsoluteSourcePaths) override;
	virtual bool AddSourceFiles(const TArray<FString>& AbsoluteSourcePaths, const TArray<FString>& AvailableModules) override;
	virtual bool SaveAllOpenDocuments() const override;
	virtual void Tick(const float DeltaTime) override;

private:
	/** Wrapper for vscode executable launch information */
	struct FLocation
	{
		bool IsValid() const
		{
			return URL.Len() > 0;
		}

		FString URL;
	};

	/** Location instance */
	FLocation Location;

	/** String storing the solution path obtained from the module manager to avoid having to use it on a thread */
	mutable FString CachedSolutionPath;

	/** Critical section for updating SolutionPath */
	mutable FCriticalSection CachedSolutionPathCriticalSection;

	/** Flag of the neovim server running */
	bool bIsRemoteRunning;
	/** Neovim --server flag argument. The neovim running server url */
	FString RemoteServerURL;

	/** Accessor for SolutionPath. Will try to update it when called from the game thread, otherwise will use the cached value */
	FString GetSolutionPath() const;

	/** Helper function for sending commands to the running neovim server */
	bool SendRemote(const TArray<FString>& InArgs);
	/** Checks if terminal app and opts was provided */
	bool IsRunInTerminal() const;
	/** Runs neovim server in the new terminal window */
	void StartRemoteNeovimServer();
	/** Helper function that prepares basic neovim remote command options */
	FString PrepareRemoteCommand();

	/** Helper function for launching the neovim instance with the given list of arguments */
	bool Launch(const TArray<FString>& InArgs);
};
