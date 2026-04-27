#include "QuickStart.h"

#include <filesystem>

#define WIN32_LEAN_AND_MEAN
#define NOGDI
#include <Windows.h>

#include "ErrorHandle.h"
#include "Log.h"

BE_USING_NAMESPACE

static PROCESS_INFORMATION serverProcess;

void QuickStart::StartSinglePlayer()
{
	std::string workingDirectory{"../"};
	std::error_code errorCode;
	std::filesystem::current_path(workingDirectory, errorCode);

	if (errorCode)
	{
		Log::Critical("Failed to set working directory \"{}\" ({})", workingDirectory, errorCode.message());
		ErrorHandle::PauseThenAbort();
	}

	STARTUPINFO startupInfo;

	memset(&startupInfo, 0, sizeof(startupInfo));
	memset(&serverProcess, 0, sizeof(serverProcess));

	char serverExecutable[] = "Server.exe";

	CreateProcessA(
		NULL,
		serverExecutable,
		NULL,
		NULL,
		false,
		CREATE_NEW_CONSOLE,
		NULL,
		NULL,
		&startupInfo,
		&serverProcess
	);

	workingDirectory = "./Client";
	std::filesystem::current_path(workingDirectory, errorCode);

	if (errorCode)
	{
		Log::Critical("Failed to set working directory \"{}\" ({})", workingDirectory, errorCode.message());
		ErrorHandle::PauseThenAbort();
	}
}

void QuickStart::ShutdownServer()
{
	TerminateProcess(serverProcess.hProcess, 0);
}
