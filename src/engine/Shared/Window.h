#pragma once

#include <SDL3/SDL.h>

namespace Window
{

void Init();
void Shutdown();

void CloseWindow();
bool ShouldClose();

void UpdateWindowSize(int width, int height);
void UpdateWindowFlags();

SDL_Window *GetWindow();
int GetWindowWidth();
int GetWindowHeight();
bool IsMinimized();

SDL_Renderer *GetRenderer();

};
