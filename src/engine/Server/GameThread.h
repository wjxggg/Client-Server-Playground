#pragma once

#include <cstdint>

constexpr uint32_t DEFAULT_TPS = 20;

namespace GameThread
{

void Start();
void End();

void SetTps(uint32_t tps);

float GetTimeFactor();
float GetElapsedTime();

};
