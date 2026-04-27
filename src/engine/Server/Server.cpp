#include <filesystem>
#include <system_error>

#include "ErrorHandle.h"
#include "GameThread.h"
#include "InputSystem.h"
#include "Log.h"
#include "RenderThread.h"
#include "Timer.h"
#include "Window.h"

BE_USING_NAMESPACE

int main()
{
	// Start
    Log::Init();

	std::string workingDirectory{"./Server"};
	std::filesystem::create_directory(workingDirectory);

	std::error_code errorCode;
	std::filesystem::current_path(workingDirectory, errorCode);

	if (errorCode)
	{
		Log::Critical("Failed to set working directory \"{}\" ({})", workingDirectory, errorCode.message());
		ErrorHandle::PauseThenAbort();
	}

    Window::Init();
	InputSystem::Init();

    Timer::Init();

    GameThread::Start();
    RenderThread::Start();

    // End
    RenderThread::End();
    GameThread::End();

    Window::Shutdown();

    return 0;
}