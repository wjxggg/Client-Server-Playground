#pragma once

#include <string>

// Set our SDL_Scancode for mouse action because SDL_Scancode and SDL_MouseButtonFlags are incompatible.
constexpr int SDL_SCANCODE_MOUSE_OFFSET = 300;

namespace InputSystem
{

void Init();
// You should call Prepare() before receive window event
void Prepare();

// value: down = 1, up = -1
void KeyInput(int scancode, int value);
void PollMouseInput();
void WheelInput(float scrollY);

void ResetKeyBind();
void UnbindKey(const std::string &key, const std::string &action);
void BindKey(const std::string &key, const std::string &action);

bool ActionState(uint8_t action);
bool ActionJustStarted(uint8_t action);
float GetMouseX();
float GetMouseY();
float GetMouseRelativeX();
float GetMouseRelativeY();

};