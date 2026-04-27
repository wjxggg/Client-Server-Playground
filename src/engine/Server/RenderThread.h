#pragma once

#include <cstdint>

constexpr uint32_t DEFAULT_FPS = 240;
constexpr uint32_t DEFAULT_FPS_UI = 60;

namespace RenderThread
{

void Start();
void End();

void SetFps(uint32_t fps);
void SetFps_UI(uint32_t fps);

float GetElapsedTime();

};
