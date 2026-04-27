#include "Command.h"
#include "Commands.h"

#include <cfloat>
#include <unordered_map>

#include "GameThread.h"
#include "InputSystem.h"
#include "Log.h"
#include "RenderThread.h"
#include "Window.h"

BE_USING_NAMESPACE

static const Command quit{
	"quit",
	"",
	Window::CloseWindow,
	{

	}
};

static const Command tps{
	"tps",
	"Set max tps",
	GameThread::SetTps,
	{
		ConVar{.name = "tps", .desc = "Ticks per second", .uint32Default = DEFAULT_TPS, .uint32Min = 0, .uint32Max = UINT32_MAX}
	}
};

static const Command fps{
	"fps",
	"Set max fps",
	RenderThread::SetFps,
	{
		ConVar{.name = "fps", .desc = "Frames per second, set to 0 to not limit the fps", .uint32Default = DEFAULT_FPS, .uint32Min = 0, .uint32Max = UINT32_MAX}
	}
};

static const Command fps_ui{
	"fps_ui",
	"Set max ui fps",
	RenderThread::SetFps_UI,
	{
		ConVar{.name = "fps", .desc = "Frames per second, set to 0 to not limit the fps", .uint32Default = DEFAULT_FPS_UI, .uint32Min = 0, .uint32Max = UINT32_MAX}
	}
};

static const Command bind{
	"bind",
	"Bind action to key",
	InputSystem::BindKey,
	{
		ConVar{.name = "key", .desc = "", .stringDefault = "default"},
		ConVar{.name = "action", .desc = "", .stringDefault = "default"}
	}
};

static const Command unbind{
	"unbind",
	"Unbind action from key",
	InputSystem::UnbindKey,
	{
		ConVar{.name = "key", .desc = "Set to \"all\" to unbind all the keys", .stringDefault = "default"},
		ConVar{.name = "action", .desc = "Set to \"all\" to unbind all the actions", .stringDefault = "default"}
	}
};

static const std::unordered_map<std::string, const ICommand *> m_commands{
	{quit.GetName(), &quit},
	{tps.GetName(), &tps},
	{fps.GetName(), &fps},
	{fps_ui.GetName(), &fps_ui},
	{bind.GetName(), &bind},
	{unbind.GetName(), &unbind}
};

void Commands::Run(std::span<std::string> args)
{
	// args[0] command
	// args[1 ...n] args
	if (!m_commands.contains(args[0]))
	{
		Log::Warn("Unknown command \"{}\"", args[0]);
		return;
	}

	// Call the command
	m_commands.at(args[0])->Run(std::span<std::string>{args.begin() + 1, args.end()});
}
