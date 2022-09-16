#include "NeovimSourceCodeAccessor.h"
#include "NeovimCodeAccessorSettings.h"
#include "NeovimSourceCodeAccessModule.h"
#include "ISourceCodeAccessModule.h"
#include "Modules/ModuleManager.h"
#include "DesktopPlatformModule.h"
#include "Misc/Paths.h"
#include "Misc/ScopeLock.h"
#include "Misc/UProjectInfo.h"
#include "Misc/App.h"

#if PLATFORM_WINDOWS
	#include "Windows/AllowWindowsPlatformTypes.h"
#endif
#include "Internationalization/Regex.h"

DEFINE_LOG_CATEGORY_STATIC(LogNeovimCodeAccessor, Log, All);

#define LOCTEXT_NAMESPACE "NeovimSourceCodeAccessor"

FNeovimSourceCodeAccessor::FNeovimSourceCodeAccessor()
	:bIsRemoteRunning(false)
{
	RemoteServerURL =
		GetDefault<UNeovimCodeAccessorSettings>()->RemoteExecutionURL;

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

/** save all open documents in visual studio, when recompiling */
static void OnModuleCompileStarted(bool bIsAsyncCompile)
{
	UE_LOG(LogNeovimCodeAccessor, Log, TEXT("OnModuleCompileStarted"));
	FNeovimSourceCodeAccessModule& NeovimSourceCodeAccessModule =
		FModuleManager::LoadModuleChecked<FNeovimSourceCodeAccessModule>(TEXT("NeovimSourceCodeAccess"));
	NeovimSourceCodeAccessModule.GetAccessor().SaveAllOpenDocuments();
}

void FNeovimSourceCodeAccessor::Startup()
{
	UE_LOG(LogNeovimCodeAccessor, Log, TEXT("Startup"));
	GetSolutionPath();
	RefreshAvailability();
}

void FNeovimSourceCodeAccessor::RefreshAvailability()
{
	TArray<FString> Args = { TEXT(":echo \"Test\"<CR>") };
#if PLATFORM_WINDOWS
	/* TODO */
	Location.URL = TEXT("nvim.exe");

#elif PLATFORM_LINUX
	FString OutURL;
	int32 ReturnCode = -1;

	FPlatformProcess::ExecProcess(TEXT("/bin/bash"), TEXT("-c \"type -p nvim\""),
								&ReturnCode, &OutURL, nullptr);

	if (ReturnCode == 0)
	{
		Location.URL = OutURL.TrimStartAndEnd();
	}
	else
	{
		// Fallback to default install location
		FString URL = TEXT("/usr/bin/nvim");
		if (FPaths::FileExists(URL))
		{
			Location.URL = URL;
		}
	}

#elif PLATFORM_MAC
	/* TODO */
	Location.URL = TEXT("nvim");
#endif

	bIsRemoteRunning = SendRemote(Args);

}

void FNeovimSourceCodeAccessor::Shutdown()
{
	if (bIsRemoteRunning) {
		TArray<FString> Args = { TEXT(":qa!<CR>") };
		SaveAllOpenDocuments();
		Launch(Args);
		bIsRemoteRunning = false;
	}
}

bool FNeovimSourceCodeAccessor::OpenSourceFiles(
	const TArray<FString>& AbsoluteSourcePaths)
{
	bool result = false;
	if (Location.IsValid())
	{
		FString SolutionDir = GetSolutionPath();
		TArray<FString> Args;

		Args.Add(TEXT(":"));
		if (!bIsRemoteRunning) {
			OpenSolution();
		}

		for (const FString& SourcePath : AbsoluteSourcePaths)
		{
			Args.Add(TEXT("e "));
			Args.Add(SourcePath);
			Args.Add(TEXT("|"));
		}

		Args.Add(TEXT("<CR>"));

		result = Launch(Args);

		/* If remote accessor was closed must be reopend */
		if (!result && !bIsRemoteRunning) {
			return OpenSourceFiles(AbsoluteSourcePaths);
		}
	}

	return result;
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
	/* TODO: Implement ability to open file at specified line */
	return false;
}

bool FNeovimSourceCodeAccessor::CanAccessSourceCode() const
{
	// True if we have any versions of VS installed
	return Location.IsValid();
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
	if (Location.IsValid())
	{
		return OpenSolutionAtPath(GetSolutionPath());
	}

	return false;
}

bool FNeovimSourceCodeAccessor::OpenSolutionAtPath(const FString& InSolutionPath)
{
	if (Location.IsValid())
	{
		FString SolutionPath = InSolutionPath;

		TArray<FString> Args;

		Args.Add(FString::Printf(TEXT(":cd %s <CR>"), *InSolutionPath));
		return Launch(Args);
	}

	return false;
}

bool FNeovimSourceCodeAccessor::IsRunInTerminal() const
{
	return GetDefault<UNeovimCodeAccessorSettings>()->bStartInTerminal
			&& !GetDefault<UNeovimCodeAccessorSettings>()->RemoteExecutionTerminal.IsEmpty()
			&& !GetDefault<UNeovimCodeAccessorSettings>()->RemoteExecutionTerminalOpts.IsEmpty();
}

bool FNeovimSourceCodeAccessor::DoesSolutionExist() const
{
	return FPaths::FileExists(GetSolutionPath());
}

bool FNeovimSourceCodeAccessor::SaveAllOpenDocuments() const
{
	if (bIsRemoteRunning) {
		TArray<FString> Args = { TEXT(":wa<CR>") };
		return const_cast<FNeovimSourceCodeAccessor*>(this)->Launch(Args);
	}

	return false;
}

FString FNeovimSourceCodeAccessor::PrepareRemoteCommand()
{
	FString ArgsString;

	if (RemoteServerURL.IsEmpty()) {
		UE_LOG(LogNeovimCodeAccessor, Error,
		 TEXT("Remote server not specified!"));

		return ArgsString;
	}
	ArgsString.Append(FString::Printf(TEXT("--server %s"), *RemoteServerURL));

	ArgsString.Append(TEXT(" "));
	ArgsString.Append(TEXT("--remote-send"));
	ArgsString.Append(TEXT(" "));

	return ArgsString;
}

bool FNeovimSourceCodeAccessor::SendRemote(const TArray<FString>& InArgs)
{
	FString OutURL;
	int32 ReturnCode = -1;
	FString RemoteCommand = PrepareRemoteCommand();

	if (Location.IsValid() && !RemoteCommand.IsEmpty())
	{
		FString ArgsString;
		ArgsString.Append(RemoteCommand);

		ArgsString.Append(TEXT("\""));
		for (const FString& Arg : InArgs)
		{
			ArgsString.Append(Arg);
		}
		ArgsString.Append(TEXT("\""));

		FPlatformProcess::ExecProcess(*Location.URL, *ArgsString, &ReturnCode,
									&OutURL, nullptr);

		bIsRemoteRunning = ReturnCode == 0;
	}

	return bIsRemoteRunning;
}

void FNeovimSourceCodeAccessor::StartRemoteNeovimServer()
{
	FString RemoteServer =
		GetDefault<UNeovimCodeAccessorSettings>()->RemoteExecutionURL;
	FString RemoteTerminal =
		GetDefault<UNeovimCodeAccessorSettings>()->RemoteExecutionTerminal;
	/* Terminal opts that provides ability to run command with terminal opening */
	FString RemoteTerminalOpts =
		GetDefault<UNeovimCodeAccessorSettings>()->RemoteExecutionTerminalOpts;


	if (Location.IsValid() && !RemoteServer.IsEmpty())
	{
		FString ArgsString;
		bool bSuccess = false;

		Location.URL = RemoteTerminal;

		ArgsString.Append(RemoteTerminalOpts);
		ArgsString.Append(TEXT(" "));
		ArgsString.Append(TEXT("nvim"));
		ArgsString.Append(TEXT(" "));
		ArgsString.Append(FString::Printf(TEXT("--listen %s"), *RemoteServer));

		FProcHandle WorkerHandle = FPlatformProcess::CreateProc(*Location.URL,
																*ArgsString,
																true, false,
																false, nullptr,
																0, nullptr,
																nullptr);
		bSuccess = WorkerHandle.IsValid();
		FPlatformProcess::CloseProc(WorkerHandle);

		bIsRemoteRunning = bSuccess;
		RefreshAvailability();
	}
}

bool FNeovimSourceCodeAccessor::Launch(const TArray<FString>& InArgs)
{
	/* Is remote not runing and terminal command has provided then remote server must be started */
	if (!bIsRemoteRunning && IsRunInTerminal()) {
		StartRemoteNeovimServer();
	}

	return SendRemote(InArgs);
}

#undef LOCTEXT_NAMESPACE
