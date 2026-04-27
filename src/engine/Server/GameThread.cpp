#include "GameThread.h"

#include <thread>

#include "Log.h"
#include "Timer.h"
#include "Window.h"

BE_USING_NAMESPACE

//-----------------------------------------------------------------------------
// Variable
//-----------------------------------------------------------------------------

static std::thread m_thread;

static Timer::DelayTimer m_timer{DEFAULT_TPS};

//-----------------------------------------------------------------------------
// Private Decl
//-----------------------------------------------------------------------------

static void GameLoop();

//-----------------------------------------------------------------------------
// Public
//-----------------------------------------------------------------------------

void GameThread::Start()
{
    m_thread = std::thread{GameLoop};
}

void GameThread::End()
{
    m_thread.join();
}

void GameThread::SetTps(uint32_t tps) { m_timer.SetTps(tps); }

float GameThread::GetTimeFactor() { return m_timer.GetTimeFactor(); }
float GameThread::GetElapsedTime() { return m_timer.GetElapsedTime(); }

//-----------------------------------------------------------------------------
// Private
//-----------------------------------------------------------------------------

void GameLoop()
{
    while (!Window::ShouldClose())
    {
        m_timer.UpdateTime();



        m_timer.WaitUntilNextTick();
    }
}
