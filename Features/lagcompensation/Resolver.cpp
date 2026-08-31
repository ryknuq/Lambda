#include "animation_system.h"
#include "..\ragebot\aim.h"
#include "..\ragebot\antiaim.h"
#include "..\misc\logs.h"

Vector player_t::get_eye_pos()
{
	return m_vecOrigin() + m_vecViewOffset();
}

void resolver::initialize(player_t* e, adjust_data* record, const float& goal_feet_yaw, const float& pitch)
{
	player = e;
	player_record = record;

	auto records = &player_records[e->EntIndex()];
	previous_player_record = records->size() >= 2 ? &records->at(1) : nullptr;

	original_goal_feet_yaw = math::normalize_yaw(goal_feet_yaw);
	original_pitch = math::normalize_pitch(pitch);
	side = RESOLVER_ORIGINAL;
	fake = false;
}

void resolver::reset()
{
	player = nullptr;
	player_record = nullptr;
	previous_player_record = nullptr;
	side = RESOLVER_ORIGINAL;
	fake = false;
}

void resolver::resolve()
{
	if (!player || !player_record || !player->is_alive())
		return;

	auto state = player->get_animation_state();
	if (!state)
		return;

	player_record->side = RESOLVER_ORIGINAL;
	player_record->moving_resolver_active = false;
	player_record->high_desync_resolver_active = false;

	if (player_record->bot || player->m_iTeamNum() == g_ctx.local()->m_iTeamNum() ||
		player->get_move_type() == MOVETYPE_LADDER || player->get_move_type() == MOVETYPE_NOCLIP)
	{
		state->m_flGoalFeetYaw = original_goal_feet_yaw;
		return;
	}

	const auto eye_yaw = math::normalize_yaw(player->m_angEyeAngles().y);
	const auto delta = std::clamp(std::fabs(player->get_max_desync_delta()), 25.0f, 60.0f);

	auto apply = [&](resolver_side selected)
	{
		player_record->side = selected;

		switch (selected)
		{
		case RESOLVER_ZERO:
			state->m_flGoalFeetYaw = eye_yaw;
			break;
		case RESOLVER_FIRST:
			state->m_flGoalFeetYaw = math::normalize_yaw(eye_yaw + delta);
			break;
		case RESOLVER_SECOND:
			state->m_flGoalFeetYaw = math::normalize_yaw(eye_yaw - delta);
			break;
		case RESOLVER_LOW_FIRST:
			state->m_flGoalFeetYaw = math::normalize_yaw(eye_yaw + delta * 0.5f);
			break;
		case RESOLVER_LOW_SECOND:
			state->m_flGoalFeetYaw = math::normalize_yaw(eye_yaw - delta * 0.5f);
			break;
		default:
			state->m_flGoalFeetYaw = original_goal_feet_yaw;
			break;
		}
	};

	if (player_record->shot)
	{
		apply(RESOLVER_ZERO);
		return;
	}

	const auto misses = g_ctx.globals.missed_shots[player->EntIndex()] > 0 ?
		g_ctx.globals.missed_shots[player->EntIndex()] : 0;
	if (misses > 0)
	{
		static constexpr resolver_side sequence[] =
		{
			RESOLVER_FIRST,
			RESOLVER_SECOND,
			RESOLVER_ZERO,
			RESOLVER_LOW_FIRST,
			RESOLVER_LOW_SECOND
		};

		apply(sequence[(misses - 1) % (sizeof(sequence) / sizeof(sequence[0]))]);
		return;
	}

	const auto speed = player_record->velocity.Length2D();
	if (speed > 5.0f && player_record->flags & FL_ONGROUND)
	{
		const auto network_rate = player_record->layers[ANIMATION_LAYER_MOVEMENT_MOVE].m_flPlaybackRate;
		float errors[3] =
		{
			std::fabs(player_record->resolver_layers[0][ANIMATION_LAYER_MOVEMENT_MOVE].m_flPlaybackRate - network_rate),
			std::fabs(player_record->resolver_layers[1][ANIMATION_LAYER_MOVEMENT_MOVE].m_flPlaybackRate - network_rate),
			std::fabs(player_record->resolver_layers[2][ANIMATION_LAYER_MOVEMENT_MOVE].m_flPlaybackRate - network_rate)
		};

		int best = 0;
		if (errors[1] < errors[best])
			best = 1;
		if (errors[2] < errors[best])
			best = 2;

		player_record->moving_resolver_active = true;
		apply(best == 0 ? RESOLVER_ZERO : best == 1 ? RESOLVER_FIRST : RESOLVER_SECOND);
		return;
	}

	const auto lby_delta = math::normalize_yaw(player_record->lby - eye_yaw);
	if (previous_player_record && previous_player_record->valid() &&
		std::fabs(math::normalize_yaw(player_record->lby - previous_player_record->lby)) > 20.0f)
	{
		if (std::fabs(lby_delta) < delta * 0.35f)
			apply(RESOLVER_ZERO);
		else
			apply(lby_delta > 0.0f ? RESOLVER_FIRST : RESOLVER_SECOND);
		return;
	}

	const auto first_visible = util::visible(g_ctx.globals.eye_pos,
		player->hitbox_position_matrix(HITBOX_HEAD, player_record->matrixes_data.first), player, g_ctx.local());
	const auto second_visible = util::visible(g_ctx.globals.eye_pos,
		player->hitbox_position_matrix(HITBOX_HEAD, player_record->matrixes_data.second), player, g_ctx.local());

	if (first_visible != second_visible)
	{
		apply(first_visible ? RESOLVER_FIRST : RESOLVER_SECOND);
		return;
	}

	if (last_side == RESOLVER_FIRST || last_side == RESOLVER_SECOND ||
		last_side == RESOLVER_LOW_FIRST || last_side == RESOLVER_LOW_SECOND || last_side == RESOLVER_ZERO)
		apply(last_side);
	else
		apply(RESOLVER_ZERO);
}

void resolver::update_animation_layers(player_t*)
{
}

void resolver::BuildMoveYaw(player_t* e, float& foot_yaw)
{
	if (!e)
		return;

	if (e->m_vecVelocity().Length2D() > 0.1f)
		foot_yaw = math::normalize_yaw(RAD2DEG(std::atan2(-e->m_vecVelocity().y, -e->m_vecVelocity().x)));
	else
		foot_yaw = math::normalize_yaw(e->m_angEyeAngles().y);
}
