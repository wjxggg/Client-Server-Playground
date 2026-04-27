#include "RenderThread.h"

#include "SDL3/SDL_mouse.h"

#include <SDL3/SDL_events.h>

#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_sdl3.h"
#include "ImGui/imgui_impl_sdlrenderer3.h"

#include "Console.h"
#include "GameThread.h"
#include "InputSystem.h"
#include "Log.h"
#include "PlayerAction.h"
#include "Timer.h"
#include "Utils.h"
#include "Window.h"

BE_USING_NAMESPACE

//-----------------------------------------------------------------------------
// Variable
//-----------------------------------------------------------------------------

static SDL_Event m_event;

static Timer::DelayTimer m_timer{DEFAULT_FPS};
static Timer::RepeatTimer m_timer_ui{DEFAULT_FPS_UI};

static float m_gameThreadTimeFactor{0.0f};

//-----------------------------------------------------------------------------
// Private Decl
//-----------------------------------------------------------------------------

static void RenderLoop();

static void WaitIfMinimized();
static void PollEvent();

static void DrawDebugInfo();

//-----------------------------------------------------------------------------
// Public
//-----------------------------------------------------------------------------

void RenderThread::Start()
{
    RenderLoop();
}

void RenderThread::End()
{

}

void RenderThread::SetFps(uint32_t fps) { m_timer.SetTps(fps > 0 ? fps : UINT32_MAX); }
void RenderThread::SetFps_UI(uint32_t fps) { m_timer_ui.SetTps(fps > 0 ? fps : UINT32_MAX); }

float RenderThread::GetElapsedTime() { return m_timer.GetElapsedTime(); }

//-----------------------------------------------------------------------------
// Private
//-----------------------------------------------------------------------------

void RenderLoop()
{
    while (!Window::ShouldClose())
    {
        WaitIfMinimized();

		m_timer.UpdateTime();
		m_gameThreadTimeFactor = GameThread::GetTimeFactor();

        InputSystem::Prepare();

        PollEvent();

        if (InputSystem::ActionJustStarted(PlayerAction::console))
        {
            Console::ToggleConsole();
        }

		SDL_SetRenderDrawColor(Window::GetRenderer(), 0, 0, 0, 255);
		SDL_RenderClear(Window::GetRenderer());

		SDL_SetRenderDrawColor(Window::GetRenderer(), 255, 255, 255, 255);
		SDL_FRect rect{100, 100, 200, 200};
		SDL_RenderFillRect(Window::GetRenderer(), &rect);

        // Render UI
        if (m_timer_ui.TickFinishedFixed())
        {
            ImGui_ImplSDL3_NewFrame();
			ImGui_ImplSDLRenderer3_NewFrame();
            ImGui::NewFrame();

            DrawDebugInfo();

            Console::RenderFrame();

            ImGui::Render();
        }

        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), Window::GetRenderer());
		SDL_RenderPresent(Window::GetRenderer());

        m_timer.WaitUntilNextTick();
    }
}

void WaitIfMinimized()
{
	while (Window::IsMinimized())
	{
		SDL_WaitEvent(&m_event);

		switch (m_event.type)
		{
			case SDL_EVENT_QUIT:
				Window::CloseWindow();
				break;
			case SDL_EVENT_WINDOW_RESIZED:
				Window::UpdateWindowSize(m_event.window.data1, m_event.window.data2);
				break;
			case SDL_EVENT_WINDOW_MINIMIZED:
			case SDL_EVENT_WINDOW_MAXIMIZED:
			case SDL_EVENT_WINDOW_RESTORED:
				Window::UpdateWindowFlags();
				break;
			default:
				break;
		}
	}
}

void PollEvent()
{
	while (SDL_PollEvent(&m_event))
	{
		switch (m_event.type)
		{
			case SDL_EVENT_QUIT:
				Window::CloseWindow();
				break;
			case SDL_EVENT_WINDOW_RESIZED:
				Window::UpdateWindowSize(m_event.window.data1, m_event.window.data2);
				break;
			case SDL_EVENT_WINDOW_MAXIMIZED:
			case SDL_EVENT_WINDOW_MINIMIZED:
			case SDL_EVENT_WINDOW_RESTORED:
				Window::UpdateWindowFlags();
				break;
			case SDL_EVENT_KEY_DOWN:
				if (!m_event.key.repeat)
				{
					InputSystem::KeyInput(m_event.key.scancode, 1);
				}
				break;
			case SDL_EVENT_KEY_UP:
				InputSystem::KeyInput(m_event.key.scancode, -1);
				break;
			case SDL_EVENT_MOUSE_WHEEL:
				InputSystem::WheelInput(m_event.wheel.y);
				break;
			default:
				break;
		}

		ImGui_ImplSDL3_ProcessEvent(&m_event);
	}

	InputSystem::PollMouseInput();
}

void DrawDebugInfo()
{
    ImGui::Begin(
        "Info",
        nullptr,
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoInputs
    );

    ImGui::SetWindowPos({0, 0});

    ImGui::Text("%ufps %.2fms", static_cast<uint32_t>(round(1.0 / RenderThread::GetElapsedTime())), RenderThread::GetElapsedTime() * 1000.0);
    ImGui::Text("%utps %.2fms", static_cast<uint32_t>(round(1.0 / GameThread::GetElapsedTime())), GameThread::GetElapsedTime() * 1000.0);

    //ImGui::Text("pos(%.2f, %.2f, %.2f)", g_player.position.x, g_player.position.y, g_player.position.z);

    static auto lastTime = std::chrono::steady_clock::now();
    auto currentTime = std::chrono::steady_clock::now();
    auto elapsedTime = currentTime - lastTime;

    static uint32_t fps_ui_count = 0;
    static uint32_t fps_ui = 0;

    ++fps_ui_count;

    if (elapsedTime > std::chrono::milliseconds{1000})
    {
        fps_ui = fps_ui_count;
        fps_ui_count = 0;
        lastTime = currentTime;
    }

    ImGui::Text("%ufps(UI) %.2fms", fps_ui, 1000.0f / fps_ui);

    for (uint8_t i = 0; i <= PlayerAction::interact; ++i)
    {
        ImGui::Text("%s%s", InputSystem::ActionState(i) ? "+" : "-", PlayerAction::ToString(i).data());
    }

    ImGui::End();
}
