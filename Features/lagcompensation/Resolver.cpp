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

	this->goal_feet_yaw = original_goal_feet_yaw;
	side = false;
	fake = false;
}

void resolver::reset()
{
	player = nullptr;
	player_record = nullptr;
	previous_player_record = nullptr;

	goal_feet_yaw = 0.0f;
	original_goal_feet_yaw = 0.0f;

	last_resolved_yaw = 0.0f;
	last_resolved_time = 0.0f;

	last_side = RESOLVER_ORIGINAL;
	side = false;
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

	goal_feet_yaw = original_goal_feet_yaw;
	state->m_flGoalFeetYaw = original_goal_feet_yaw;

	if (player_record->bot || player->m_iTeamNum() == g_ctx.local()->m_iTeamNum() ||
		player->get_move_type() == MOVETYPE_LADDER || player->get_move_type() == MOVETYPE_NOCLIP)
		return;

	const auto eye_yaw = math::normalize_yaw(player->m_angEyeAngles().y);
	const auto max_delta = std::clamp(std::fabs(player->get_max_desync_delta()), 25.0f, 60.0f);

	float candidate_yaw[resolver_candidate_count];

	for (auto i = 0; i < resolver_candidate_count; ++i)
		candidate_yaw[i] = math::normalize_yaw(eye_yaw + max_delta * resolver_candidate_scale[i]);

	auto commit = [&](resolver_side selected, float yaw)
	{
		player_record->side = selected;
		goal_feet_yaw = math::normalize_yaw(yaw);

		state->m_flGoalFeetYaw = goal_feet_yaw;
		state->m_flCurrentFeetYaw = goal_feet_yaw;

		last_resolved_yaw = goal_feet_yaw;
		last_resolved_time = player_record->simulation_time;
	};

	auto nearest_side = [&](float yaw)
	{
		auto best = 0;
		auto best_error = FLT_MAX;

		for (auto i = 0; i < resolver_candidate_count; ++i)
		{
			const auto error = std::fabs(math::normalize_yaw(candidate_yaw[i] - yaw));

			if (error < best_error)
			{
				best_error = error;
				best = i;
			}
		}

		return resolver_candidate_sides[best];
	};

	const auto speed = player_record->velocity.Length2D();
	const auto on_ground = (player_record->flags & FL_ONGROUND) != 0;
	const auto standing = on_ground && speed < 5.0f;

	if (player_record->shot)
	{
		commit(RESOLVER_ZERO, eye_yaw);
		return;
	}

	const auto fresh_update = !previous_player_record ||
		player_record->simulation_time > previous_player_record->simulation_time;

	const auto lby_fresh = fresh_update && previous_player_record && previous_player_record->valid(false) &&
		std::fabs(math::normalize_yaw(player_record->lby - previous_player_record->lby)) > 0.25f;

	float error[resolver_candidate_count] = { };
	auto terms = 0.0f;

	auto accumulate = [&](const float* simulated, float network, bool circular, float gate)
	{
		auto distance = [&](float first, float second)
		{
			const auto raw = std::fabs(first - second);
			return circular && raw > 0.5f ? 1.0f - raw : raw;
		};

		auto spread = 0.0f;

		for (auto i = 0; i < resolver_candidate_count; ++i)
		{
			for (auto j = i + 1; j < resolver_candidate_count; ++j)
				spread = max(spread, distance(simulated[i], simulated[j]));
		}

		if (spread < gate)
			return;

		auto closest = FLT_MAX;

		for (auto i = 0; i < resolver_candidate_count; ++i)
			closest = min(closest, distance(simulated[i], network));

		if (closest > spread * 2.0f)
			return;

		for (auto i = 0; i < resolver_candidate_count; ++i)
			error[i] += min(distance(simulated[i], network) / spread, 1.0f);

		terms += 1.0f;
	};

	for (auto layer : resolver_scored_layers)
	{
		for (auto field = 0; field < 3; ++field)
		{
			float gathered[resolver_candidate_count];

			for (auto i = 0; i < resolver_candidate_count; ++i)
			{
				const auto& entry = player_record->resolver_layers[i][layer];
				gathered[i] = field == 0 ? entry.m_flWeight : field == 1 ? entry.m_flPlaybackRate : entry.m_flCycle;
			}

			const auto& network = player_record->layers[layer];
			const auto network_value = field == 0 ? network.m_flWeight : field == 1 ? network.m_flPlaybackRate : network.m_flCycle;

			accumulate(gathered, network_value, field == 2, 0.002f);
		}

		auto sequence_spread = false;
		auto sequence_match = false;

		for (auto i = 0; i < resolver_candidate_count; ++i)
		{
			if (player_record->resolver_layers[i][layer].m_nSequence != player_record->resolver_layers[0][layer].m_nSequence)
				sequence_spread = true;

			if (player_record->resolver_layers[i][layer].m_nSequence == player_record->layers[layer].m_nSequence)
				sequence_match = true;
		}

		if (!sequence_spread || !sequence_match)
			continue;

		for (auto i = 0; i < resolver_candidate_count; ++i)
		{
			if (player_record->resolver_layers[i][layer].m_nSequence != player_record->layers[layer].m_nSequence)
				error[i] += 1.0f;
		}

		terms += 1.0f;
	}

	for (auto pose = 0; pose < 24; ++pose)
	{
		float gathered[resolver_candidate_count];

		for (auto i = 0; i < resolver_candidate_count; ++i)
			gathered[i] = player_record->resolver_poses[i][pose];

		accumulate(gathered, player_record->network_poses[pose], false, 0.005f);
	}

	if (lby_fresh)
	{
		for (auto i = 0; i < resolver_candidate_count; ++i)
			error[i] += 2.0f * min(std::fabs(math::normalize_yaw(candidate_yaw[i] - player_record->lby)) / max_delta, 1.0f);

		terms += 2.0f;
	}

	if (last_resolved_time > 0.0f && player_record->simulation_time - last_resolved_time <= TICKS_TO_TIME(2))
	{
		for (auto i = 0; i < resolver_candidate_count; ++i)
			error[i] += 0.5f * min(std::fabs(math::normalize_yaw(candidate_yaw[i] - last_resolved_yaw)) / max_delta, 1.0f);

		terms += 0.5f;
	}

	auto has_fallback = false;
	auto fallback_side = RESOLVER_ORIGINAL;
	auto fallback_yaw = 0.0f;

	if (terms > 0.0f)
	{
		auto best = 0;

		for (auto i = 1; i < resolver_candidate_count; ++i)
		{
			if (error[i] < error[best])
				best = i;
		}

		auto runner_up = -1;

		for (auto i = 0; i < resolver_candidate_count; ++i)
		{
			if (i != best && (runner_up < 0 || error[i] < error[runner_up]))
				runner_up = i;
		}

		auto position = -1;

		for (auto i = 0; i < resolver_candidate_count; ++i)
		{
			if (resolver_candidate_order[i] == best)
				position = i;
		}

		auto refined = candidate_yaw[best];

		if (position > 0 && position < resolver_candidate_count - 1)
		{
			const auto low = error[resolver_candidate_order[position - 1]];
			const auto high = error[resolver_candidate_order[position + 1]];
			const auto denominator = low - 2.0f * error[best] + high;

			if (denominator > 0.0001f)
			{
				const auto step = max_delta * 0.25f;
				const auto offset = std::clamp(0.5f * (low - high) / denominator * step, -step, step);

				refined = math::normalize_yaw(candidate_yaw[best] + offset);
			}
		}

		if (runner_up >= 0 && (error[runner_up] - error[best]) / terms > 0.035f)
		{
			player_record->moving_resolver_active = !standing;
			commit(resolver_candidate_sides[best], refined);
			return;
		}

		has_fallback = true;
		fallback_side = resolver_candidate_sides[best];
		fallback_yaw = refined;
	}

	if (lby_fresh)
	{
		commit(nearest_side(player_record->lby), player_record->lby);
		return;
	}

	if (standing)
	{
		const auto lby_delta = math::normalize_yaw(player_record->lby - eye_yaw);

		if (std::fabs(lby_delta) > 5.0f)
		{
			const auto resolved = std::clamp(lby_delta, -max_delta, max_delta);

			commit(nearest_side(eye_yaw + resolved), eye_yaw + resolved);
			return;
		}
	}

	const auto first_visible = util::visible(g_ctx.globals.eye_pos,
		player->hitbox_position_matrix(HITBOX_HEAD, player_record->matrixes_data.first), player, g_ctx.local());
	const auto second_visible = util::visible(g_ctx.globals.eye_pos,
		player->hitbox_position_matrix(HITBOX_HEAD, player_record->matrixes_data.second), player, g_ctx.local());

	if (first_visible != second_visible)
	{
		const auto hidden = first_visible ? 2 : 1;

		commit(resolver_candidate_sides[hidden], candidate_yaw[hidden]);
		return;
	}

	if (has_fallback)
	{
		player_record->moving_resolver_active = !standing;
		commit(fallback_side, fallback_yaw);
		return;
	}

	const auto original_delta = math::normalize_yaw(original_goal_feet_yaw - eye_yaw);

	if (std::fabs(original_delta) > max_delta)
	{
		const auto resolved = std::clamp(original_delta, -max_delta, max_delta);

		commit(nearest_side(eye_yaw + resolved), eye_yaw + resolved);
	}
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
