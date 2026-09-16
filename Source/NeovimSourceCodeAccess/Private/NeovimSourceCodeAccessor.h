#pragma once

#include "ISourceCodeAccessor.h"
#include "Async/Future.h"

class FNeovimSourceCodeAccessor : public ISourceCodeAccessor, public TSharedFromThis<FNeovimSourceCodeAccessor>
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

	/** Cached accessor availability; updated by background probes. */
	TAtomic<bool> bIsAvailable;
	TAtomic<bool> bIsShuttingDown;
	/** Serializes endpoint probing, Neovide launch, and remote open requests. */
	FCriticalSection RemoteOperationCriticalSection;
	/** Keeps background work alive and joinable during module shutdown. */
	FCriticalSection PendingTasksCriticalSection;
	TArray<TFuture<void>> PendingTasks;
	FString NeovideExecutable;
	FString NeovideRPCAddress;

	/** Accessor for SolutionPath. Will try to update it when called from the game thread, otherwise will use the cached value */
	FString GetSolutionPath() const;

	FString GetNeovideRPCAddress() const;
	bool EnsureNeovimExecutable();
	bool IsNeovideServerAlive();
	bool LaunchNeovide();
	bool OpenFileAtLineBlocking(const FString& FullPath, int32 LineNumber, int32 ColumnNumber);
	bool RunProcessWithTimeout(const FString& Executable, const FString& Arguments, double TimeoutSeconds, int32& OutReturnCode) const;
};
