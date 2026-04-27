#pragma once

#include <string>

#include <glm/glm.hpp>

#include <magic_enum/magic_enum.hpp>

struct PlayerAction
{
	enum Action : uint8_t
	{
		forward,
		backward,
		left,
		right,

		jump,
		duck,
		sprint,
		attack,
		interact,

		inventory,
		menu,
		enter,
		console,

		wheelup,
		wheeldown,

		slot1,
		slot2,
		slot3,
		slot4,
		slot5,
		slot6,
		slot7,
		slot8,
		slot9,
		slot10,

		PLAYER_ACTION_COUNT,

		PLAYER_ACTION_NONE = 0xFF
	};

	static std::string ToString(uint8_t playerAction)
	{
		if (playerAction >= PLAYER_ACTION_COUNT) return "UNKNOWN_ACTION";

		return std::string{magic_enum::enum_name(static_cast<Action>(playerAction))};
	}

	static uint8_t GetActionFromName(const std::string &name)
	{
		auto action = magic_enum::enum_cast<Action>(name);

		return action.has_value() ? action.value() : PLAYER_ACTION_NONE;
	}
};

// Actions that send to the server
struct PackedAction
{
	// 0 bit ~ 8 bit: forward ~ interact
	uint32_t moveAction;
	glm::vec3 rotation;
	// Held item

	void Reset()
	{
		moveAction = 0;
		rotation = {0.0f, 0.0f, 0.0f};
	}

	bool ActionState(uint8_t action) const
	{
		return moveAction & (1 << action);
	}
};
