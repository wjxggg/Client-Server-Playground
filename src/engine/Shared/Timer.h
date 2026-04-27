#pragma once

#include <chrono>
#include <thread>

namespace Timer
{

void Init();

// Control the frequency of the thread
class DelayTimer
{
public:
	DelayTimer(uint32_t tps);

	inline void UpdateTime();
	inline void WaitUntilNextTick() const;

	void SetTps(uint32_t tps) { m_tickTime = tps > 0 ? std::chrono::microseconds{1000000ull /*MS_PER_SECOND*/ / tps} : std::chrono::microseconds{UINT32_MAX}; }

	std::chrono::steady_clock::time_point GetPrevTickStartTime() const { return m_prevTime; }
	std::chrono::steady_clock::time_point GetTickStartTime() const { return m_startTime; }
	float GetElapsedTime() const { return m_elapsedTimeSecond; }

	// Time factor: Percentage of elapsed time relative to the total tick time
	// You should store the time factor
	// so the functions that are supposed to run simultaneously
	// can access the same value.
	inline float GetTimeFactor() const;

private:
	std::chrono::microseconds m_tickTime;
	std::chrono::steady_clock::time_point m_currentTime;
	std::chrono::steady_clock::time_point m_startTime;
	std::chrono::steady_clock::time_point m_prevTime;
	std::chrono::microseconds m_elapsedTimeMS;
	float m_elapsedTimeSecond;

};

// Try executing at the specified frequency
class RepeatTimer
{
public:
	RepeatTimer(uint32_t tps);

	inline bool TickFinished();
	// Trying to keep up with TPS
	inline bool TickFinishedFixed();

	void SetTps(uint32_t tps) { m_tickTime = tps > 0 ? std::chrono::microseconds{1000000ull /*MS_PER_SECOND*/ / tps} : std::chrono::microseconds{UINT32_MAX}; }

	float GetElapsedTime() const { return m_elapsedTime; }

private:
	std::chrono::microseconds m_tickTime;
	std::chrono::steady_clock::time_point m_targetTime;
	std::chrono::steady_clock::time_point m_startTime;
	float m_elapsedTime{0};

};

inline void DelayTimer::UpdateTime()
{
	m_currentTime = std::chrono::steady_clock::now();

	m_elapsedTimeMS = duration_cast<std::chrono::microseconds>(m_currentTime - m_startTime);
	m_elapsedTimeSecond = static_cast<float>(m_elapsedTimeMS.count()) / 1000000ull /*MS_PER_SECOND*/;

	m_prevTime = m_startTime;
	m_startTime = m_currentTime;
}

inline void DelayTimer::WaitUntilNextTick() const
{
	auto targetTime = m_currentTime + m_tickTime;
	if (std::chrono::steady_clock::now() < targetTime)
	{
		std::this_thread::sleep_until(targetTime - std::chrono::milliseconds{2});
		while (std::chrono::steady_clock::now() < targetTime);
	}
}

inline float DelayTimer::GetTimeFactor() const
{
	// ((steady_clock::now() - m_elapsedTimeMS) - m_prevTime) / (m_startTime - m_prevTime)

	auto t = std::chrono::steady_clock::now() - m_elapsedTimeMS;

	if (t >= m_startTime) return 1.0f;

	float num = std::chrono::duration<float, std::micro>(t - m_prevTime).count();
	return num / m_elapsedTimeMS.count();
}

inline bool RepeatTimer::TickFinished()
{
	auto currentTime = std::chrono::steady_clock::now();
	m_elapsedTime = static_cast<float>(duration_cast<std::chrono::microseconds>(currentTime - m_startTime).count()) / 1000000ull /*MS_PER_SECOND*/;

	if (currentTime < m_targetTime) return false;

	m_targetTime = currentTime + m_tickTime;
	m_startTime = currentTime;

	return true;
}

inline bool RepeatTimer::TickFinishedFixed()
{
	auto currentTime = std::chrono::steady_clock::now();
	m_elapsedTime = static_cast<float>(duration_cast<std::chrono::microseconds>(currentTime - m_startTime).count()) / 1000000ull /*MS_PER_SECOND*/;

	if (currentTime < m_targetTime) return false;

	if (m_targetTime + m_tickTime > currentTime)
	{
		m_targetTime += m_tickTime;
	}
	else
	{
		m_targetTime = currentTime + m_tickTime;
	}

	m_startTime = currentTime;

	return true;
}

}
