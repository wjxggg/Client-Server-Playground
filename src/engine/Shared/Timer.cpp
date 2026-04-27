#include "Timer.h"

#ifdef _WIN32
#include <windows.h>
#endif

using namespace Timer;

void Timer::Init()
{
#ifdef _WIN32
	timeBeginPeriod(1);
#endif
}

DelayTimer::DelayTimer(uint32_t tps)
{
	SetTps(tps);
	m_currentTime = std::chrono::steady_clock::now();
	m_startTime = m_currentTime;
	m_prevTime = m_currentTime;
}

RepeatTimer::RepeatTimer(uint32_t tps)
{
	SetTps(tps);
	m_startTime = std::chrono::steady_clock::now();
	m_targetTime = m_startTime + m_tickTime;
}
