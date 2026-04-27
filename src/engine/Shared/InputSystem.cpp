#include "InputSystem.h"

#include <array>
#include <bitset>

#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_mouse.h>

#include "PlayerAction.h"
#include "Log.h"

BE_USING_NAMESPACE

//-----------------------------------------------------------------------------
// Const
//-----------------------------------------------------------------------------

constexpr int SDL_SCANCODE_MOUSE_LEFT = (SDL_BUTTON_LEFT + SDL_SCANCODE_MOUSE_OFFSET);
constexpr int SDL_SCANCODE_MOUSE_MIDDLE = (SDL_BUTTON_MIDDLE + SDL_SCANCODE_MOUSE_OFFSET);
constexpr int SDL_SCANCODE_MOUSE_RIGHT = (SDL_BUTTON_RIGHT + SDL_SCANCODE_MOUSE_OFFSET);
constexpr int SDL_SCANCODE_MOUSE4 = (SDL_BUTTON_X1 + SDL_SCANCODE_MOUSE_OFFSET);
constexpr int SDL_SCANCODE_MOUSE5 = (SDL_BUTTON_X2 + SDL_SCANCODE_MOUSE_OFFSET);
constexpr int SDL_SCANCODE_WHEEL_UP = (6 + SDL_SCANCODE_MOUSE_OFFSET);
constexpr int SDL_SCANCODE_WHEEL_DOWN = (7 + SDL_SCANCODE_MOUSE_OFFSET);

constexpr int INPUT_MAX_ACTION_PER_KEY = 10;

//-----------------------------------------------------------------------------
// Variable
//-----------------------------------------------------------------------------

// How many keys being pressed from the same action
static std::array<int, PlayerAction::PLAYER_ACTION_COUNT> m_actionLayer;
static std::bitset<PlayerAction::PLAYER_ACTION_COUNT> m_newActions;

// Reset actions that bind to wheel
static bool m_wheelupReset;
static bool m_wheeldownReset;

static float m_mouseX;
static float m_mouseY;
static float m_mouseRelX;
static float m_mouseRelY;

static uint8_t m_keyMap[SDL_SCANCODE_COUNT][INPUT_MAX_ACTION_PER_KEY];

//-----------------------------------------------------------------------------
// Private Decl
//-----------------------------------------------------------------------------

static int GetKeyFromName(const char *name);

//-----------------------------------------------------------------------------
// Public
//-----------------------------------------------------------------------------

void InputSystem::Init()
{
	ResetKeyBind();
}

void InputSystem::Prepare()
{
	m_newActions.reset();

	if (m_wheelupReset)
	{
		KeyInput(SDL_SCANCODE_WHEEL_UP, -1);
		m_wheelupReset = false;
	}
	else if (m_wheeldownReset)
	{
		KeyInput(SDL_SCANCODE_WHEEL_DOWN, -1);
		m_wheeldownReset = false;
	}
}

void InputSystem::KeyInput(int scancode, int value)
{
	// Update all the actions that bind to this key
	for (int i = 0; i < INPUT_MAX_ACTION_PER_KEY; ++i)
	{
		uint8_t action = m_keyMap[scancode][i];
		if (action != PlayerAction::PLAYER_ACTION_NONE)
		{
			m_actionLayer[action] = std::max(0, m_actionLayer[action] + value);
			m_newActions[action] = m_newActions[action] || (value == 1);

			// TODO:
			// Register the new action in case the key release before UpdatePackedAction()
		}
	}
}

void InputSystem::PollMouseInput()
{
	float oldMouseX{m_mouseX};
	float oldMouseY{m_mouseY};

	static SDL_MouseButtonFlags oldButtonFlag{0};
	SDL_MouseButtonFlags buttonFlag = SDL_GetGlobalMouseState(&m_mouseX, &m_mouseY);
	SDL_MouseButtonFlags changedButton = oldButtonFlag ^ buttonFlag;

	m_mouseRelX = m_mouseX - oldMouseX;
	m_mouseRelY = m_mouseY - oldMouseY;

	if (changedButton & SDL_BUTTON_LMASK) KeyInput(SDL_SCANCODE_MOUSE_LEFT, (buttonFlag & SDL_BUTTON_LMASK) ? 1 : -1);
	if (changedButton & SDL_BUTTON_MMASK) KeyInput(SDL_SCANCODE_MOUSE_MIDDLE, (buttonFlag & SDL_BUTTON_MMASK) ? 1 : -1);
	if (changedButton & SDL_BUTTON_RMASK) KeyInput(SDL_SCANCODE_MOUSE_RIGHT, (buttonFlag & SDL_BUTTON_RMASK) ? 1 : -1);
	if (changedButton & SDL_BUTTON_X1MASK) KeyInput(SDL_SCANCODE_MOUSE4, (buttonFlag & SDL_BUTTON_X1MASK) ? 1 : -1);
	if (changedButton & SDL_BUTTON_X2MASK) KeyInput(SDL_SCANCODE_MOUSE5, (buttonFlag & SDL_BUTTON_X2MASK) ? 1 : -1);

	oldButtonFlag = buttonFlag;
}

void InputSystem::WheelInput(float scrollY)
{
	if (scrollY == 0) return;
	KeyInput(scrollY > 0 ? SDL_SCANCODE_WHEEL_UP : SDL_SCANCODE_WHEEL_DOWN, 1);

	(scrollY > 0 ? m_wheelupReset : m_wheeldownReset) = true;
}

void InputSystem::ResetKeyBind()
{
	memset(m_keyMap, 0xFF, sizeof(m_keyMap));

	m_keyMap[SDL_SCANCODE_W][0] = PlayerAction::forward;
	m_keyMap[SDL_SCANCODE_S][0] = PlayerAction::backward;
	m_keyMap[SDL_SCANCODE_A][0] = PlayerAction::left;
	m_keyMap[SDL_SCANCODE_D][0] = PlayerAction::right;

	m_keyMap[SDL_SCANCODE_SPACE][0] = PlayerAction::jump;
	m_keyMap[SDL_SCANCODE_LCTRL][0] = PlayerAction::duck;
	m_keyMap[SDL_SCANCODE_LSHIFT][0] = PlayerAction::sprint;
	m_keyMap[SDL_SCANCODE_MOUSE_LEFT][0] = PlayerAction::attack;
	m_keyMap[SDL_SCANCODE_MOUSE_RIGHT][0] = PlayerAction::interact;

	m_keyMap[SDL_SCANCODE_TAB][0] = PlayerAction::inventory;
	m_keyMap[SDL_SCANCODE_ESCAPE][0] = PlayerAction::menu;
	m_keyMap[SDL_SCANCODE_RETURN][0] = PlayerAction::enter;
	m_keyMap[SDL_SCANCODE_GRAVE][0] = PlayerAction::console;

	m_keyMap[SDL_SCANCODE_WHEEL_UP][0] = PlayerAction::wheelup;
	m_keyMap[SDL_SCANCODE_WHEEL_DOWN][0] = PlayerAction::wheeldown;

	m_keyMap[SDL_SCANCODE_1][0] = PlayerAction::slot1;
	m_keyMap[SDL_SCANCODE_2][0] = PlayerAction::slot2;
	m_keyMap[SDL_SCANCODE_3][0] = PlayerAction::slot3;
	m_keyMap[SDL_SCANCODE_4][0] = PlayerAction::slot4;
	m_keyMap[SDL_SCANCODE_5][0] = PlayerAction::slot5;
	m_keyMap[SDL_SCANCODE_6][0] = PlayerAction::slot6;
	m_keyMap[SDL_SCANCODE_7][0] = PlayerAction::slot7;
	m_keyMap[SDL_SCANCODE_8][0] = PlayerAction::slot8;
	m_keyMap[SDL_SCANCODE_9][0] = PlayerAction::slot9;
	m_keyMap[SDL_SCANCODE_0][0] = PlayerAction::slot10;
}

void InputSystem::UnbindKey(const std::string &key, const std::string &action)
{
	bool allKey = key == "all";

	int scancode;
	if (!allKey)
	{
		scancode = GetKeyFromName(key.c_str());
		if (!scancode)
		{
			Log::Warn("Unknown key \"{}\"", key);
			return;
		}
	}

	bool allAction = action == "all";

	uint8_t act;
	if (!allAction)
	{
		act = PlayerAction::GetActionFromName(action);
		if (act == PlayerAction::PLAYER_ACTION_NONE)
		{
			Log::Warn("Unknown action \"{}\"", action);
			return;
		}
	}

	if (allKey)
	{
		for (int i = 0; i < SDL_SCANCODE_COUNT; ++i)
		{
			for (int j = 0; j < INPUT_MAX_ACTION_PER_KEY; ++j)
			{
				if (allAction || m_keyMap[i][j] == act)
				{
					m_keyMap[i][j] = PlayerAction::PLAYER_ACTION_NONE;
				}
			}
		}
	}
	else
	{
		for (int j = 0; j < INPUT_MAX_ACTION_PER_KEY; ++j)
		{
			if (allAction || m_keyMap[scancode][j] == act)
			{
				m_keyMap[scancode][j] = PlayerAction::PLAYER_ACTION_NONE;
			}
		}
	}
}

void InputSystem::BindKey(const std::string &key, const std::string &action)
{
	int scancode = GetKeyFromName(key.c_str());
	if (!scancode)
	{
		Log::Warn("Unknown key \"{}\"", key);
		return;
	}

	uint8_t act = PlayerAction::GetActionFromName(action);
	if (act == PlayerAction::PLAYER_ACTION_NONE)
	{
		Log::Warn("Unknown action \"{}\"", action);
		return;
	}

	for (int i = 0; i < INPUT_MAX_ACTION_PER_KEY; ++i)
	{
		if (m_keyMap[scancode][i] == PlayerAction::PLAYER_ACTION_NONE)
		{
			m_keyMap[scancode][i] = act;
			return;
		}
	}

	Log::Warn("Can not bind more than {} actions to one key", INPUT_MAX_ACTION_PER_KEY);
}

bool InputSystem::ActionState(uint8_t action) { return m_actionLayer[action]; }
bool InputSystem::ActionJustStarted(uint8_t action) { return m_newActions[action]; }
float InputSystem::GetMouseX() { return m_mouseX; }
float InputSystem::GetMouseY() { return m_mouseY; }
float InputSystem::GetMouseRelativeX() { return m_mouseRelX; }
float InputSystem::GetMouseRelativeY() { return m_mouseRelY; }

//-----------------------------------------------------------------------------
// Private
//-----------------------------------------------------------------------------

int GetKeyFromName(const char *name)
{
	int scancode = SDL_GetScancodeFromName(name);

	if (scancode) return scancode;

	if (!SDL_strcasecmp(name, "mouse1"))
	{
		scancode = SDL_SCANCODE_MOUSE_LEFT;
	}
	else if (!SDL_strcasecmp(name, "mouse2"))
	{
		scancode = SDL_SCANCODE_MOUSE_RIGHT;
	}
	else if (!SDL_strcasecmp(name, "mouse3"))
	{
		scancode = SDL_SCANCODE_MOUSE_MIDDLE;
	}
	else if (!SDL_strcasecmp(name, "mouse4"))
	{
		scancode = SDL_SCANCODE_MOUSE4;
	}
	else if (!SDL_strcasecmp(name, "mouse5"))
	{
		scancode = SDL_SCANCODE_MOUSE5;
	}
	else if (!SDL_strcasecmp(name, "wheelup"))
	{
		scancode = SDL_SCANCODE_WHEEL_UP;
	}
	else if (!SDL_strcasecmp(name, "wheeldown"))
	{
		scancode = SDL_SCANCODE_WHEEL_DOWN;
	}

	return scancode;
}
