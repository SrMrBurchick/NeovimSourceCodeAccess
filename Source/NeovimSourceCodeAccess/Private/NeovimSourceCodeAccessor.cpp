#include "NeovimSourceCodeAccessor.h"
#include "NeovimCodeAccessorSettings.h"
#include "Misc/Paths.h"
#include "Misc/ScopeLock.h"
#include "Misc/SecureHash.h"
#include "Async/Async.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"

DEFINE_LOG_CATEGORY_STATIC(LogNeovimCodeAccessor, Log, All);

#define LOCTEXT_NAMESPACE "NeovimSourceCodeAccessor"

namespace
{
	FString QuoteProcessArgument(const FString& Argument)
	{
		FString Escaped = Argument.Replace(TEXT("\""), TEXT("\\\""));
		return FString::Printf(TEXT("\"%s\""), *Escaped);
	}

	FString FindExecutableOnPath(const FString& ConfiguredExecutable)
	{
		if (FPaths::FileExists(ConfiguredExecutable))
		{
			return FPaths::ConvertRelativePathToFull(ConfiguredExecutable);
		}

		FString ExecutableName = ConfiguredExecutable;
#if PLATFORM_WINDOWS
		if (FPaths::GetExtension(ExecutableName).IsEmpty())
		{
			ExecutableName += TEXT(".exe");
		}
#endif

		TArray<FString> SearchDirectories;
		FPlatformMisc::GetEnvironmentVariable(TEXT("PATH")).ParseIntoArray(
			SearchDirectories,
#if PLATFORM_WINDOWS
			TEXT(";"),
#else
			TEXT(":"),
#endif
			true);

		for (const FString& Directory : SearchDirectories)
		{
			const FString Candidate = FPaths::Combine(Directory, ExecutableName);
			if (FPaths::FileExists(Candidate))
			{
				return Candidate;
			}
		}

		return FString();
	}
}

FNeovimSourceCodeAccessor::FNeovimSourceCodeAccessor()
	: bIsAvailable(true)
	, bIsShuttingDown(false)
{
	const UNeovimCodeAccessorSettings* Settings = GetDefault<UNeovimCodeAccessorSettings>();
	NeovideExecutable = Settings->NeovideExecutable;
	NeovideRPCAddress = Settings->NeovideRPCAddress;
}

FString FNeovimSourceCodeAccessor::GetSolutionPath() const
{
	FScopeLock Lock(&CachedSolutionPathCriticalSection);

	if (IsInGameThread())
	{
		CachedSolutionPath =
			FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
	}

	return CachedSolutionPath;
}

void FNeovimSourceCodeAccessor::Startup()
{
	UE_LOG(LogNeovimCodeAccessor, Log, TEXT("[NeovimSourceCodeAccess] Accessor Startup BEGIN"));
	GetSolutionPath();
	if (NeovideRPCAddress.IsEmpty())
	{
		const FString ProjectKey = FMD5::HashAnsiString(*GetSolutionPath()).Left(16);
#if PLATFORM_WINDOWS
		NeovideRPCAddress = FString::Printf(TEXT("//./pipe/neovide-unreal-%s"), *ProjectKey);
#else
		FString RuntimeDirectory = FPlatformMisc::GetEnvironmentVariable(TEXT("XDG_RUNTIME_DIR"));
		if (RuntimeDirectory.IsEmpty())
		{
			RuntimeDirectory = FPlatformProcess::UserTempDir();
		}
		NeovideRPCAddress = FPaths::Combine(
			RuntimeDirectory, FString::Printf(TEXT("neovide-unreal-%s.sock"), *ProjectKey));
#endif
	}
	// Do not probe or launch external processes while Unreal is loading
	// Default-phase modules. Availability is refreshed asynchronously on demand.
	UE_LOG(LogNeovimCodeAccessor, Log, TEXT("[NeovimSourceCodeAccess] Accessor Startup END"));
}

void FNeovimSourceCodeAccessor::RefreshAvailability()
{
	UE_LOG(LogNeovimCodeAccessor, Log, TEXT("[NeovimSourceCodeAccess] RefreshAvailability BEGIN"));
	// Availability is intentionally optimistic. Executable discovery and RPC
	// probing are lazy and happen only in response to an open request.
	bIsAvailable.Store(true);
	UE_LOG(LogNeovimCodeAccessor, Log, TEXT("[NeovimSourceCodeAccess] RefreshAvailability END"));
}

void FNeovimSourceCodeAccessor::Shutdown()
{
	bIsShuttingDown.Store(true);
	TArray<TFuture<void>> TasksToJoin;
	{
		FScopeLock TasksLock(&PendingTasksCriticalSection);
		TasksToJoin = MoveTemp(PendingTasks);
	}
	for (TFuture<void>& Task : TasksToJoin)
	{
		Task.Wait();
	}
}

bool FNeovimSourceCodeAccessor::OpenSourceFiles(
	const TArray<FString>& AbsoluteSourcePaths)
{
	bool bAcceptedAll = true;
	for (const FString& SourcePath : AbsoluteSourcePaths)
	{
		bAcceptedAll &= OpenFileAtLine(SourcePath, 1, 1);
	}
	return bAcceptedAll;
}

bool FNeovimSourceCodeAccessor::AddSourceFiles(
	const TArray<FString>& AbsoluteSourcePaths,
	const TArray<FString>& AvailableModules)
{
	// Neovim doesn't need to do anything when new files are added
	return true;
}

bool FNeovimSourceCodeAccessor::OpenFileAtLine(const FString& FullPath,
	int32 LineNumber, int32 ColumnNumber)
{
	const FString AbsolutePath = FPaths::ConvertRelativePathToFull(FullPath);
	TSharedRef<FNeovimSourceCodeAccessor> Self = AsShared();
	FScopeLock TasksLock(&PendingTasksCriticalSection);
	if (bIsShuttingDown.Load())
	{
		return false;
	}
	PendingTasks.Add(Async(EAsyncExecution::ThreadPool, [Self, AbsolutePath, LineNumber, ColumnNumber]()
	{
		FScopeLock Lock(&Self->RemoteOperationCriticalSection);
		if (Self->bIsShuttingDown.Load())
		{
			return;
		}
		Self->OpenFileAtLineBlocking(AbsolutePath, LineNumber, ColumnNumber);
	}));

	// The request was accepted. RPC and process launch work is deliberately
	// performed off the editor thread.
	return true;
}

bool FNeovimSourceCodeAccessor::CanAccessSourceCode() const
{
	return bIsAvailable.Load();
}

FName FNeovimSourceCodeAccessor::GetFName() const
{
	return FName("Neovim");
}

FText FNeovimSourceCodeAccessor::GetNameText() const
{
	return LOCTEXT("NeovimDisplayName", "Neovim");
}

FText FNeovimSourceCodeAccessor::GetDescriptionText() const
{
	return LOCTEXT("NeovimDisplayDesc", "Open source code files in Neovim");
}

void FNeovimSourceCodeAccessor::Tick(const float DeltaTime)
{
}

bool FNeovimSourceCodeAccessor::OpenSolution()
{
	return false;
}

bool FNeovimSourceCodeAccessor::OpenSolutionAtPath(const FString& InSolutionPath)
{
	return false;
}

bool FNeovimSourceCodeAccessor::DoesSolutionExist() const
{
	return FPaths::DirectoryExists(GetSolutionPath());
}

bool FNeovimSourceCodeAccessor::SaveAllOpenDocuments() const
{
	return false;
}

FString FNeovimSourceCodeAccessor::GetNeovideRPCAddress() const
{
	return NeovideRPCAddress;
}

bool FNeovimSourceCodeAccessor::EnsureNeovimExecutable()
{
	if (!Location.IsValid())
	{
		Location.URL = FindExecutableOnPath(TEXT("nvim"));
	}
	bIsAvailable.Store(Location.IsValid());
	return Location.IsValid();
}

bool FNeovimSourceCodeAccessor::RunProcessWithTimeout(const FString& Executable, const FString& Arguments,
	double TimeoutSeconds, int32& OutReturnCode) const
{
	OutReturnCode = -1;
	FProcHandle Process = FPlatformProcess::CreateProc(*Executable, *Arguments, true, true, true,
		nullptr, 0, nullptr, nullptr);
	if (!Process.IsValid())
	{
		return false;
	}

	const double Deadline = FPlatformTime::Seconds() + TimeoutSeconds;
	while (FPlatformProcess::IsProcRunning(Process) && FPlatformTime::Seconds() < Deadline)
	{
		FPlatformProcess::Sleep(0.02f);
	}

	if (FPlatformProcess::IsProcRunning(Process))
	{
		UE_LOG(LogNeovimCodeAccessor, Warning,
			TEXT("[NeovimSourceCodeAccess] Process timed out; terminating %s"), *Executable);
		FPlatformProcess::TerminateProc(Process, true);
		FPlatformProcess::CloseProc(Process);
		return false;
	}

	const bool bGotReturnCode = FPlatformProcess::GetProcReturnCode(Process, &OutReturnCode);
	FPlatformProcess::CloseProc(Process);
	return bGotReturnCode;
}

bool FNeovimSourceCodeAccessor::IsNeovideServerAlive()
{
	if (!EnsureNeovimExecutable())
	{
		return false;
	}

	int32 ReturnCode = -1;
	const FString Arguments = FString::Printf(TEXT("--server %s --remote-expr %s"),
		*QuoteProcessArgument(GetNeovideRPCAddress()), *QuoteProcessArgument(TEXT("1")));
	return RunProcessWithTimeout(Location.URL, Arguments, 1.0, ReturnCode) && ReturnCode == 0;
}

bool FNeovimSourceCodeAccessor::LaunchNeovide()
{
	const FString ResolvedNeovideExecutable = FindExecutableOnPath(NeovideExecutable);
	if (ResolvedNeovideExecutable.IsEmpty())
	{
		UE_LOG(LogNeovimCodeAccessor, Error,
			TEXT("[NeovimSourceCodeAccess] Cannot find Neovide executable '%s'"), *NeovideExecutable);
		return false;
	}

	const FString RPCAddress = GetNeovideRPCAddress();
#if !PLATFORM_WINDOWS
	// A crashed server can leave a socket node behind. It is safe to remove only
	// after the RPC probe has failed and before starting this project's server.
	IFileManager::Get().Delete(*RPCAddress, false, true);
#endif

	const FString Arguments = FString::Printf(TEXT("-- --listen %s"), *QuoteProcessArgument(RPCAddress));
	UE_LOG(LogNeovimCodeAccessor, Log,
		TEXT("[NeovimSourceCodeAccess] Launching Neovide (Executable=%s, Server=%s)"),
		*ResolvedNeovideExecutable, *RPCAddress);
	FProcHandle Process = FPlatformProcess::CreateProc(*ResolvedNeovideExecutable, *Arguments, true, false, false,
		nullptr, 0, *GetSolutionPath(), nullptr);
	const bool bStarted = Process.IsValid();
	if (bStarted)
	{
		FPlatformProcess::CloseProc(Process);
	}
	return bStarted;
}

bool FNeovimSourceCodeAccessor::OpenFileAtLineBlocking(const FString& FullPath, int32 LineNumber, int32 ColumnNumber)
{
	UE_LOG(LogNeovimCodeAccessor, Log,
		TEXT("[NeovimSourceCodeAccess] OpenFileAtLine worker BEGIN (File=%s, Line=%d, Column=%d)"),
		*FullPath, LineNumber, ColumnNumber);

	if (!IsNeovideServerAlive())
	{
		if (!LaunchNeovide())
		{
			return false;
		}

		const double Deadline = FPlatformTime::Seconds() + 10.0;
		bool bServerReady = false;
		while (!bIsShuttingDown.Load() && FPlatformTime::Seconds() < Deadline)
		{
			bServerReady = IsNeovideServerAlive();
			if (bServerReady)
			{
				break;
			}
			FPlatformProcess::Sleep(0.1f);
		}
		if (!bServerReady)
		{
			UE_LOG(LogNeovimCodeAccessor, Error,
				TEXT("[NeovimSourceCodeAccess] Neovide RPC endpoint did not become ready: %s"),
				*GetNeovideRPCAddress());
			return false;
		}
	}

	int32 ReturnCode = -1;
	const FString OpenArguments = FString::Printf(TEXT("--server %s --remote %s"),
		*QuoteProcessArgument(GetNeovideRPCAddress()), *QuoteProcessArgument(FullPath));
	if (!RunProcessWithTimeout(Location.URL, OpenArguments, 2.0, ReturnCode) || ReturnCode != 0)
	{
		UE_LOG(LogNeovimCodeAccessor, Error,
			TEXT("[NeovimSourceCodeAccess] Failed to open file in Neovide (ReturnCode=%d)"), ReturnCode);
		return false;
	}

	const int32 SafeLine = FMath::Max(1, LineNumber);
	const int32 SafeColumn = FMath::Max(1, ColumnNumber);
	const FString CursorExpression = FString::Printf(TEXT("cursor(%d,%d)"), SafeLine, SafeColumn);
	const FString CursorArguments = FString::Printf(TEXT("--server %s --remote-expr %s"),
		*QuoteProcessArgument(GetNeovideRPCAddress()), *QuoteProcessArgument(CursorExpression));
	const bool bMovedCursor = RunProcessWithTimeout(Location.URL, CursorArguments, 2.0, ReturnCode)
		&& ReturnCode == 0;
	UE_LOG(LogNeovimCodeAccessor, Log,
		TEXT("[NeovimSourceCodeAccess] OpenFileAtLine worker END (Success=%s)"),
		bMovedCursor ? TEXT("true") : TEXT("false"));
	return bMovedCursor;
}

#undef LOCTEXT_NAMESPACE
