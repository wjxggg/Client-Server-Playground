#include "Window.h"

#include "ImGui/imgui_impl_sdl3.h"
#include "ImGui/imgui_impl_sdlrenderer3.h"

#include "ErrorHandle.h"
#include "Log.h"

BE_USING_NAMESPACE

//-----------------------------------------------------------------------------
// Const
//-----------------------------------------------------------------------------

constexpr int WINDOW_DEFAULT_WIDTH = 800;
constexpr int WINDOW_DEFAULT_HEIGHT = 600;

//-----------------------------------------------------------------------------
// Variable
//-----------------------------------------------------------------------------

static bool m_shouldClose{false};

static SDL_Window *m_pWindow;
static int m_windowWidth{WINDOW_DEFAULT_WIDTH};
static int m_windowHeight{WINDOW_DEFAULT_HEIGHT};
static SDL_WindowFlags m_windowFlags;

static SDL_Renderer *m_pRenderer;

//-----------------------------------------------------------------------------
// Private Decl
//-----------------------------------------------------------------------------

static void InitImGui();
static void ShutdownImGui();

//-----------------------------------------------------------------------------
// Public
//-----------------------------------------------------------------------------

void Window::Init()
{
	Log::Trace("Window initializing...");

	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		Log::Critical("Failed to init SDL systems ({})", SDL_GetError());
		ErrorHandle::PauseThenAbort();
	}

	m_pWindow = SDL_CreateWindow("Game", WINDOW_DEFAULT_WIDTH, WINDOW_DEFAULT_HEIGHT, SDL_WINDOW_RESIZABLE);
	if (!m_pWindow)
	{
		Log::Critical("Failed to create window ({})", SDL_GetError());
		ErrorHandle::PauseThenAbort();
	}

	m_pRenderer = SDL_CreateRenderer(m_pWindow, NULL);
	if (!m_pRenderer)
	{
		Log::Critical("Failed to create renderer ({})", SDL_GetError());
		ErrorHandle::PauseThenAbort();
	}

	InitImGui();

	Log::Trace("Window init success");
}

void Window::Shutdown()
{
	ShutdownImGui();
	SDL_DestroyWindow(m_pWindow);
	SDL_Quit();
}

void Window::CloseWindow() { m_shouldClose = true; }
bool Window::ShouldClose() { return m_shouldClose; }

void Window::UpdateWindowSize(int width, int height) { m_windowWidth = width; m_windowHeight = height; }
void Window::UpdateWindowFlags() { m_windowFlags = SDL_GetWindowFlags(m_pWindow); }

SDL_Window *Window::GetWindow() { return m_pWindow; }
int Window::GetWindowWidth() { return m_windowWidth; }
int Window::GetWindowHeight() { return m_windowHeight; }
bool Window::IsMinimized() { return m_windowFlags & SDL_WINDOW_MINIMIZED; }

SDL_Renderer *Window::GetRenderer() { return m_pRenderer; }

//-----------------------------------------------------------------------------
// Private
//-----------------------------------------------------------------------------

void InitImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO &io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("font/JetBrainsMapleMono-Regular-NR.ttf", 16.0f);
	io.IniFilename = nullptr;

	ImGui_ImplSDL3_InitForSDLRenderer(m_pWindow, m_pRenderer);
	ImGui_ImplSDLRenderer3_Init(m_pRenderer);

    ImGui::StyleColorsDark();
}

void ShutdownImGui()
{
	ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}
