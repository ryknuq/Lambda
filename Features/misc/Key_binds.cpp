#include "key_binds.h"
#include "..\..\includes.hpp"
#include "misc.h"

void key_binds::update_key_bind(key_bind* key_bind, int key_bind_id)
{
	auto is_button_down = util::is_button_down(key_bind->key);
	bool active = false;

	switch (key_bind->mode)
	{
	case HOLD_ON:
		active = is_button_down;
		break;
	case HOLD_OFF:
		active = !is_button_down;
		break;
	case TOGGLE:
		if (!key_bind->holding && is_button_down)
		{
			keys[key_bind_id] = !keys[key_bind_id];
			key_bind->holding = true;
		}
		else if (key_bind->holding && !is_button_down)
		{
			key_bind->holding = false;
		}

		active = keys[key_bind_id];
		break;

	case ALWAYS_ON:
		active = true;
		break;
	}

	// Update the global state
	switch (key_bind_id)
	{
	case 2: // Double tap
		if (misc::get().recharging_double_tap && key_bind->mode != ALWAYS_ON)
			break;

		misc::get().double_tap_key = active;
		if (misc::get().double_tap_key && cfg.ragebot.double_tap_key.key != cfg.antiaim.hide_shots_key.key)
			misc::get().hide_shots_key = false;
		break;
	case 12: // Hide shots
		misc::get().hide_shots_key = active;
		if (misc::get().hide_shots_key && cfg.antiaim.hide_shots_key.key != cfg.ragebot.double_tap_key.key)
			misc::get().double_tap_key = false;
		break;
	case 13: // Manual back
		if (active) antiaim::get().manual_side = SIDE_BACK;
		else if (antiaim::get().manual_side == SIDE_BACK) antiaim::get().manual_side = SIDE_NONE;
		break;
	case 14: // Manual left
		if (active) antiaim::get().manual_side = SIDE_LEFT;
		else if (antiaim::get().manual_side == SIDE_LEFT) antiaim::get().manual_side = SIDE_NONE;
		break;
	case 15: // Manual right
		if (active) antiaim::get().manual_side = SIDE_RIGHT;
		else if (antiaim::get().manual_side == SIDE_RIGHT) antiaim::get().manual_side = SIDE_NONE;
		break;
	default:
		keys[key_bind_id] = active;
		break;
	}

	mode[key_bind_id] = key_bind->mode;
}

void key_binds::initialize_key_binds()
{
	for (auto i = 0; i < 23; i++)
	{
		keys[i] = false;
		if (i == 2 || i >= 12 && i <= 17)
			mode[i] = TOGGLE;
		else
			mode[i] = HOLD_ON;
	}
}

void key_binds::update_key_binds()
{
	update_key_bind(&cfg.ragebot.double_tap_key, 2);
	update_key_bind(&cfg.ragebot.safe_point_key, 3);

	for (auto i = 0; i < 8; i++)
		update_key_bind(&cfg.ragebot.weapon[i].damage_override_key, 4 + i);

	update_key_bind(&cfg.antiaim.hide_shots_key, 12);
	update_key_bind(&cfg.antiaim.manual_back, 13);
	update_key_bind(&cfg.antiaim.manual_left, 14);
	update_key_bind(&cfg.antiaim.manual_right, 15);
	update_key_bind(&cfg.antiaim.flip_desync, 16);
	update_key_bind(&cfg.misc.thirdperson_toggle, 17);
	update_key_bind(&cfg.misc.automatic_peek, 18);
	update_key_bind(&cfg.misc.edge_jump, 19);
	update_key_bind(&cfg.misc.fakeduck_key, 20);
	update_key_bind(&cfg.misc.slowwalk_key, 21);
	update_key_bind(&cfg.ragebot.body_aim_key, 22);
}

bool key_binds::get_key_bind_state(int key_bind_id)
{
	auto key = keys.find(key_bind_id);

	if (key == keys.end())
		return false;

	return key->second;
}

bool key_binds::get_key_bind_state_lua(int key_bind_id)
{
	if (key_bind_id < 0 || key_bind_id > 22)
		return false;

	switch (key_bind_id)
	{
	case 2:
		return misc::get().double_tap_key;
	case 4:
		if (g_ctx.globals.current_weapon < 0)
			return false;

		return get_key_bind_state(4 + g_ctx.globals.current_weapon);
	case 12:
		return misc::get().hide_shots_key;
	case 13:
		return antiaim::get().manual_side == SIDE_BACK;
	case 14:
		return antiaim::get().manual_side == SIDE_LEFT;
	case 15:
		return antiaim::get().manual_side == SIDE_RIGHT;
	default:
		return get_key_bind_state(key_bind_id);
	}
}

bool key_binds::get_key_bind_mode(int key_bind_id)
{
	auto found = mode.find(key_bind_id);

	if (found == mode.end())
		return HOLD_ON;

	return found->second;
}