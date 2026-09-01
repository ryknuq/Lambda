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
	player_record->resolver_confident = false;

	goal_feet_yaw = original_goal_feet_yaw;
	state->m_flGoalFeetYaw = original_goal_feet_yaw;

	if (player_record->bot || player->m_iTeamNum() == g_ctx.local()->m_iTeamNum() ||
		player->get_move_type() == MOVETYPE_LADDER || player->get_move_type() == MOVETYPE_NOCLIP)
		return;

	const auto eye_yaw = math::normalize_yaw(player->m_angEyeAngles().y);
	const auto max_delta = std::clamp(std::fabs(player->get_max_desync_delta()), 25.0f, 60.0f);
	const auto missed = std::clamp(g_ctx.globals.missed_shots[player_record->i], 0, 3);

	float candidate_yaw[resolver_candidate_count];

	for (auto i = 0; i < resolver_candidate_count; ++i)
		candidate_yaw[i] = math::normalize_yaw(eye_yaw + max_delta * resolver_candidate_scale[i]);

	auto trusted = false;

	auto commit = [&](resolver_side selected, float yaw, bool confident = false)
	{
		player_record->side = selected;
		player_record->resolver_confident = confident && (trusted || !missed);

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

	auto body_delta = FLT_MAX;

	{
		const auto network = player_record->network_poses[resolver_body_yaw_pose];

		float sample[resolver_candidate_count];
		float sample_delta[resolver_candidate_count];

		for (auto i = 0; i < resolver_candidate_count; ++i)
		{
			const auto candidate = resolver_candidate_order[i];

			sample[i] = player_record->resolver_poses[candidate][resolver_body_yaw_pose];
			sample_delta[i] = max_delta * resolver_candidate_scale[candidate];
		}

		const auto lowest = sample[0];
		const auto highest = sample[resolver_candidate_count - 1];

		if (std::fabs(highest - lowest) > 0.02f)
		{
			for (auto i = 1; i < resolver_candidate_count; ++i)
			{
				const auto low = min(sample[i - 1], sample[i]);
				const auto high = max(sample[i - 1], sample[i]);

				if (high - low < 0.0005f || network < low || network > high)
					continue;

				const auto fraction = (network - sample[i - 1]) / (sample[i] - sample[i - 1]);

				body_delta = sample_delta[i - 1] + (sample_delta[i] - sample_delta[i - 1]) * fraction;
				break;
			}

			if (body_delta == FLT_MAX)
			{
				const auto rising = highest > lowest;

				auto direction = 0.0f;

				if (network > max(lowest, highest))
					direction = rising ? 1.0f : -1.0f;
				else if (network < min(lowest, highest))
					direction = rising ? -1.0f : 1.0f;

				if (direction != 0.0f) //-V550
				{
					const auto analytic = std::clamp(network * 120.0f - 60.0f, -60.0f, 60.0f);

					body_delta = analytic * direction > max_delta ? analytic : direction * max_delta;
				}
			}
		}
	}

	const auto body_solved = body_delta != FLT_MAX; //-V550
	const auto body_yaw = math::normalize_yaw(eye_yaw + (body_solved ? body_delta : 0.0f));

	if (player_record->shot)
	{
		trusted = true;

		if (body_solved && std::fabs(body_delta) > 4.0f)
			commit(nearest_side(body_yaw), body_yaw, true);
		else
			commit(RESOLVER_ZERO, eye_yaw, true);

		return;
	}

	const auto fresh_update = !previous_player_record ||
		player_record->simulation_time > previous_player_record->simulation_time;

	const auto lby_fresh = standing && fresh_update && previous_player_record && previous_player_record->valid(false) &&
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

	float pose_estimates[24];
	auto pose_estimate_count = 0;

	for (auto pose = 0; pose < 24; ++pose)
	{
		if (pose == resolver_body_yaw_pose)
			continue;

		float sorted[resolver_candidate_count];
		float sorted_delta[resolver_candidate_count];

		for (auto i = 0; i < resolver_candidate_count; ++i)
		{
			const auto candidate = resolver_candidate_order[i];

			sorted[i] = player_record->resolver_poses[candidate][pose];
			sorted_delta[i] = max_delta * resolver_candidate_scale[candidate];
		}

		auto rising = 0;
		auto falling = 0;

		for (auto i = 1; i < resolver_candidate_count; ++i)
		{
			if (sorted[i] > sorted[i - 1])
				++rising;
			else if (sorted[i] < sorted[i - 1])
				++falling;
		}

		if (rising && falling)
			continue;

		if (std::fabs(sorted[resolver_candidate_count - 1] - sorted[0]) < 0.02f)
			continue;

		const auto network = player_record->network_poses[pose];
		auto estimate = FLT_MAX;

		for (auto i = 1; i < resolver_candidate_count; ++i)
		{
			const auto low = min(sorted[i - 1], sorted[i]);
			const auto high = max(sorted[i - 1], sorted[i]);

			if (high - low < 0.0001f || network < low || network > high)
				continue;

			const auto fraction = (network - sorted[i - 1]) / (sorted[i] - sorted[i - 1]);

			estimate = sorted_delta[i - 1] + (sorted_delta[i] - sorted_delta[i - 1]) * fraction;
			break;
		}

		if (estimate == FLT_MAX)
			continue;

		pose_estimates[pose_estimate_count++] = estimate;
	}

	auto pose_delta = 0.0f;
	auto pose_solved = false;
	auto pose_confident = false;

	if (pose_estimate_count)
	{
		auto sum = 0.0f;
		auto lowest = FLT_MAX;
		auto highest = -FLT_MAX;

		for (auto i = 0; i < pose_estimate_count; ++i)
		{
			sum += pose_estimates[i];
			lowest = min(lowest, pose_estimates[i]);
			highest = max(highest, pose_estimates[i]);
		}

		pose_delta = std::clamp(sum / (float)pose_estimate_count, -max_delta, max_delta);
		pose_solved = true;
		pose_confident = pose_estimate_count >= 2 && highest - lowest <= max_delta * 0.2f;

		const auto pose_yaw = math::normalize_yaw(eye_yaw + pose_delta);

		for (auto i = 0; i < resolver_candidate_count; ++i)
			error[i] += 1.5f * min(std::fabs(math::normalize_yaw(candidate_yaw[i] - pose_yaw)) / max_delta, 1.0f);

		terms += 1.5f;
	}

	if (body_solved)
	{
		for (auto i = 0; i < resolver_candidate_count; ++i)
			error[i] += 2.5f * min(std::fabs(math::normalize_yaw(candidate_yaw[i] - body_yaw)) / max_delta, 1.0f);

		terms += 2.5f;

		const auto pose_agrees = pose_solved && std::fabs(pose_delta - body_delta) <= max_delta * 0.25f;
		const auto lby_agrees = lby_fresh && std::fabs(math::normalize_yaw(player_record->lby - body_yaw)) <= max_delta * 0.25f;

		const auto repeating = missed >= 1 && last_side != RESOLVER_ORIGINAL && nearest_side(body_yaw) == last_side;

		if (!repeating && std::fabs(body_delta) > 4.0f && (pose_agrees || lby_agrees))
		{
			player_record->moving_resolver_active = !standing;
			player_record->high_desync_resolver_active = std::fabs(body_delta) > max_delta * 0.7f;

			commit(nearest_side(body_yaw), body_yaw, true);
			return;
		}
	}

	if (lby_fresh)
	{
		for (auto i = 0; i < resolver_candidate_count; ++i)
			error[i] += 2.0f * min(std::fabs(math::normalize_yaw(candidate_yaw[i] - player_record->lby)) / max_delta, 1.0f);

		terms += 2.0f;
	}

	const auto eye_swing = previous_player_record ?
		std::fabs(math::normalize_yaw(eye_yaw - math::normalize_yaw(previous_player_record->angles.y))) : 0.0f;

	if (last_resolved_time > 0.0f && eye_swing < 30.0f)
	{
		const auto elapsed = TIME_TO_TICKS(player_record->simulation_time - last_resolved_time);

		if (elapsed >= 0 && elapsed <= 6)
		{
			const auto weight = 0.5f * (1.0f - (float)elapsed / 7.0f);

			for (auto i = 0; i < resolver_candidate_count; ++i)
				error[i] += weight * min(std::fabs(math::normalize_yaw(candidate_yaw[i] - last_resolved_yaw)) / max_delta, 1.0f);

			terms += weight;
		}
	}

	if (missed > 0 && last_side != RESOLVER_ORIGINAL)
	{
		auto missed_scale = FLT_MAX;

		for (auto i = 0; i < resolver_candidate_count; ++i)
		{
			if (resolver_candidate_sides[i] == last_side)
				missed_scale = resolver_candidate_scale[i];
		}

		if (missed_scale != FLT_MAX) //-V550
		{
			const auto weight = 2.8f * (float)missed;

			for (auto i = 0; i < resolver_candidate_count; ++i)
				error[i] += weight * max(0.0f, 1.0f - std::fabs(resolver_candidate_scale[i] - missed_scale) * 0.5f);

			terms += weight;
		}
	}

	const auto memory = lagcompensation::get().resolver_memory_for(player);

	auto prior_best = 0.0f;
	auto prior_span = 0.0f;

	if (memory && memory->samples >= 4)
	{
		auto highest = -FLT_MAX;
		auto lowest = FLT_MAX;

		for (auto i = 0; i < resolver_candidate_count; ++i)
		{
			highest = max(highest, memory->weight[i]);
			lowest = min(lowest, memory->weight[i]);
		}

		if (highest - lowest > 0.5f)
		{
			prior_best = highest;
			prior_span = highest - lowest;

			const auto weight = 0.6f;

			for (auto i = 0; i < resolver_candidate_count; ++i)
				error[i] += weight * (prior_best - memory->weight[i]) / prior_span;

			terms += weight;
		}
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

		auto decisive = runner_up >= 0 && (error[runner_up] - error[best]) / terms > 0.12f;

		if (!decisive && memory && prior_span > 0.0f && memory->weight[best] >= prior_best - prior_span * 0.15f)
			decisive = true;

		if (decisive)
		{
			auto resolved = refined;

			if (body_solved && std::fabs(math::normalize_yaw(body_yaw - refined)) <= max_delta * 0.35f)
				resolved = body_yaw;
			else if (pose_solved)
			{
				const auto pose_yaw = math::normalize_yaw(eye_yaw + pose_delta);

				if (std::fabs(math::normalize_yaw(pose_yaw - refined)) <= max_delta * 0.35f)
					resolved = pose_yaw;
			}

			player_record->moving_resolver_active = !standing;
			player_record->high_desync_resolver_active = std::fabs(math::normalize_yaw(resolved - eye_yaw)) > max_delta * 0.7f;

			commit(resolver_candidate_sides[best], resolved, true);
			return;
		}

		has_fallback = true;
		fallback_side = resolver_candidate_sides[best];
		fallback_yaw = refined;
	}

	if (pose_confident)
	{
		const auto resolved = math::normalize_yaw(eye_yaw + pose_delta);

		player_record->moving_resolver_active = !standing;
		player_record->high_desync_resolver_active = std::fabs(pose_delta) > max_delta * 0.7f;

		commit(nearest_side(resolved), resolved, true);
		return;
	}

	if (lby_fresh)
	{
		commit(nearest_side(player_record->lby), player_record->lby, true);
		return;
	}

	if (standing)
	{
		const auto lby_delta = math::normalize_yaw(player_record->lby - eye_yaw);

		if (std::fabs(lby_delta) > 5.0f)
		{
			const auto resolved = std::clamp(lby_delta, -max_delta, max_delta);

			commit(nearest_side(eye_yaw + resolved), eye_yaw + resolved, true);
			return;
		}
	}

	if (has_fallback)
	{
		auto resolved = fallback_yaw;

		if (body_solved && std::fabs(math::normalize_yaw(body_yaw - fallback_yaw)) <= max_delta * 0.35f)
			resolved = body_yaw;
		else if (pose_solved)
		{
			const auto pose_yaw = math::normalize_yaw(eye_yaw + pose_delta);

			if (std::fabs(math::normalize_yaw(pose_yaw - fallback_yaw)) <= max_delta * 0.35f)
				resolved = pose_yaw;
		}

		player_record->moving_resolver_active = !standing;
		player_record->high_desync_resolver_active = std::fabs(math::normalize_yaw(resolved - eye_yaw)) > max_delta * 0.7f;

		commit(fallback_side, resolved);
		return;
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

resolver_memory* lagcompensation::resolver_memory_for(player_t* e)
{
	if (!e)
		return nullptr;

	const auto index = e->EntIndex();

	if (index < 1 || index > 64)
		return nullptr;

	player_info_t info{ };

	if (!m_engine()->GetPlayerInfo(index, &info) || info.fakeplayer)
		return nullptr;

	const auto identity = (uint64_t)info.steamID64;

	if (!identity)
		return nullptr;

	for (auto& entry : resolver_memories)
	{
		if (entry.identity == identity)
		{
			entry.touched = m_globals()->m_curtime;
			return &entry;
		}
	}

	auto slot = &resolver_memories[0];

	for (auto& entry : resolver_memories)
	{
		if (!entry.identity)
		{
			slot = &entry;
			break;
		}

		if (entry.touched < slot->touched)
			slot = &entry;
	}

	slot->identity = identity;
	slot->samples = 0;
	slot->touched = m_globals()->m_curtime;

	for (auto& value : slot->weight)
		value = 0.0f;

	return slot;
}

void lagcompensation::resolver_feedback(int index, resolver_side side, bool hit)
{
	if (index < 1 || index > 64)
		return;

	auto reference = -1;

	for (auto i = 0; i < resolver_candidate_count; ++i)
	{
		if (resolver_candidate_sides[i] == side)
			reference = i;
	}

	if (reference < 0)
		return;

	const auto memory = resolver_memory_for((player_t*)m_entitylist()->GetClientEntity(index));

	if (!memory)
		return;

	for (auto i = 0; i < resolver_candidate_count; ++i)
	{
		memory->weight[i] *= 0.985f;

		const auto affinity = max(0.0f, 1.0f - std::fabs(resolver_candidate_scale[i] - resolver_candidate_scale[reference]));

		if (affinity <= 0.0f)
			continue;

		memory->weight[i] = std::clamp(memory->weight[i] + (hit ? 1.0f : -0.7f) * affinity, -6.0f, 6.0f);
	}

	memory->samples = min(memory->samples + 1, 4096);
}
