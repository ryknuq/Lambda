#include "antiaim.h"
#include "knifebot.h"
#include "zeusbot.h"
#include "..\misc\fakelag.h"
#include "..\misc\prediction_system.h"
#include "..\misc\misc.h"
#include "..\lagcompensation\local_animations.h"
#include "..\lagcompensation\animation_system.h"

void antiaim::create_move(CUserCmd* m_pcmd)
{
	auto velocity = g_ctx.local()->m_vecVelocity().Length();

	type = ANTIAIM_STAND;

	if (cfg.antiaim.antiaim_type)
		type = ANTIAIM_LEGIT;

	if (condition(m_pcmd))
		return;

	if ((type == ANTIAIM_LEGIT ? cfg.antiaim.desync : cfg.antiaim.type[type].desync) && (type == ANTIAIM_LEGIT ? !cfg.antiaim.legit_lby_type : !cfg.antiaim.lby_type) && !cfg.misc.fast_stop && (!g_ctx.globals.weapon->is_grenade() || cfg.esp.on_click && !(m_pcmd->m_buttons & IN_ATTACK) && !(m_pcmd->m_buttons & IN_ATTACK2)) && engineprediction::get().backup_data.velocity.Length2D() <= 20.0f)
	{
		auto speed = 1.01f;

		if (m_pcmd->m_buttons & IN_DUCK || g_ctx.globals.fakeducking)
			speed *= 2.94117647f;

		static auto switch_move = false;

		if (switch_move)
			m_pcmd->m_sidemove += speed;
		else
			m_pcmd->m_sidemove -= speed;

		switch_move = !switch_move;
	}

	if (type != ANTIAIM_LEGIT)
		m_pcmd->m_viewangles.x = get_pitch(m_pcmd);

	m_pcmd->m_viewangles.y = get_yaw(m_pcmd);

	switch (cfg.antiaim.extended_desync_type)
	{
	case 1:
		m_pcmd->m_viewangles.z = flip == true ? -60.0f : 60.0f;
		break;
	case 2:
		m_pcmd->m_viewangles.z = flip == true ? -cfg.antiaim.extended_desync_int : cfg.antiaim.extended_desync_int;
		break;
	}

	if (cfg.antiaim.roll_enable)
		m_pcmd->m_viewangles.z = flip == true ? -cfg.antiaim.roll_int : cfg.antiaim.roll_int;
}

float antiaim::get_pitch(CUserCmd* m_pcmd)
{
	static auto invert_jitter = false;
	static auto should_invert = false;

	if (g_ctx.send_packet)
		should_invert = true;
	else if (!g_ctx.send_packet && should_invert)
	{
		should_invert = false;
		invert_jitter = !invert_jitter;
	}

	auto pitch = m_pcmd->m_viewangles.x;

	switch (cfg.antiaim.type[type].pitch)
	{
	case 1:
		pitch = 89.0f;
		break;
	case 2:
		pitch = -89.0f;
		break;
	case 3:
		pitch = 0.0f;
		break;
	}

	return pitch;
}

float antiaim::get_yaw(CUserCmd* m_pcmd) // fixed by semmxz
{
	static auto invert_jitter = false;
	static auto should_invert = false;

	if (g_ctx.send_packet)
		should_invert = true;
	else if (!g_ctx.send_packet && should_invert)
	{
		should_invert = false;
		invert_jitter = !invert_jitter;
	}

	auto max_desync_delta = g_ctx.local()->get_max_desync_delta();

	auto yaw = 0.0f;
	auto lby_type = 0;

	if (type == ANTIAIM_LEGIT)
	{
		yaw = m_pcmd->m_viewangles.y;

		if (!cfg.antiaim.desync)
			return yaw;

		if (cfg.antiaim.desync != 2 && (cfg.antiaim.flip_desync.key <= KEY_NONE || cfg.antiaim.flip_desync.key >= KEY_MAX))
			flip = automatic_direction();
		else if (cfg.antiaim.desync == 1)
			flip = key_binds::get().get_key_bind_state(16);
		else if (cfg.antiaim.desync == 2)
			flip = invert_jitter;

		desync_angle = max_desync_delta;

		if (cfg.antiaim.legit_lby_type && g_ctx.local()->m_vecVelocity().Length() < 5.0f && g_ctx.local()->m_fFlags() & FL_ONGROUND && engineprediction::get().backup_data.flags & FL_ONGROUND)
			desync_angle *= 2.0f;

		if (flip)
		{
			desync_angle = -desync_angle;
			max_desync_delta = -max_desync_delta;
		}

		yaw -= desync_angle;
		lby_type = cfg.antiaim.legit_lby_type;
	}
	else
	{
		final_manual_side = manual_side;

		if (manual_side == SIDE_NONE && key_binds::get().get_key_bind_state(25))
			freestanding(m_pcmd);

		auto reference_angle = m_pcmd->m_viewangles.y;

		if (cfg.antiaim.type[type].base_angle == 1)
			reference_angle = at_targets() - 180.0f;

		auto base_angle = reference_angle + 180.0f;

		switch (final_manual_side)
		{
		case SIDE_LEFT:
			base_angle = reference_angle + 90.0f;
			break;
		case SIDE_RIGHT:
			base_angle = reference_angle - 90.0f;
			break;
		case SIDE_BACK:
			base_angle = reference_angle + 180.0f;
			break;
		case SIDE_FORWARD:
			base_angle = reference_angle;
			break;
		default:
			break;
		}

		if (cfg.antiaim.type[type].desync != 2 && (cfg.antiaim.flip_desync.key <= KEY_NONE || cfg.antiaim.flip_desync.key >= KEY_MAX))
		{
			if (final_manual_side == SIDE_LEFT)
				flip = true;
			else if (final_manual_side == SIDE_RIGHT)
				flip = false;
			else
				flip = automatic_direction();
		}
		else if (cfg.antiaim.type[type].desync == 1)
			flip = key_binds::get().get_key_bind_state(16);

		auto yaw_angle = 0.0f;

		switch (cfg.antiaim.type[type].yaw)
		{
		case 1:
			yaw_angle = invert_jitter ? (float)cfg.antiaim.type[type].range * -0.5f : (float)cfg.antiaim.type[type].range * 0.5f;
			break;
		case 2:
		{
			if (flip)
			{
				auto start_angle = (float)cfg.antiaim.type[type].range * 0.5f;
				auto end_angle = (float)cfg.antiaim.type[type].range * -0.5f;

				static auto angle = start_angle;

				auto angle_add_amount = (float)cfg.antiaim.type[type].speed * 0.5f;

				if (angle - angle_add_amount >= end_angle)
					angle -= angle_add_amount;
				else
					angle = start_angle;

				yaw_angle = angle;
			}
			else
			{
				auto start_angle = (float)cfg.antiaim.type[type].range * -0.5f;
				auto end_angle = (float)cfg.antiaim.type[type].range * 0.5f;

				static auto angle = start_angle;

				auto angle_add_amount = (float)cfg.antiaim.type[type].speed * 0.5f;

				if (angle + angle_add_amount <= end_angle)
					angle += angle_add_amount;
				else
					angle = start_angle;

				yaw_angle = angle;
			}
		}
		break;
		}

		desync_angle = 0.0f;

		if (cfg.antiaim.type[type].desync)
		{
			if (cfg.antiaim.type[type].desync == 2)
				flip = invert_jitter;

			auto desync_delta = max_desync_delta;

			if (type == ANTIAIM_STAND && cfg.antiaim.lby_type)
				desync_delta *= 2.0f;
			else
			{
				if (!flip)
					desync_delta = min(desync_delta, (float)cfg.antiaim.type[type].desync_range);
				else
					desync_delta = min(desync_delta, (float)cfg.antiaim.type[type].inverted_desync_range);
			}

			if (!flip)
			{
				desync_delta = -desync_delta;
				max_desync_delta = -max_desync_delta;
			}

			base_angle -= desync_delta;

			if (type != ANTIAIM_STAND && type != ANTIAIM_LEGIT || !cfg.antiaim.lby_type)
			{
				if (!flip)
					base_angle += desync_delta * (float)cfg.antiaim.type[type].body_lean * 0.01f;
				else
					base_angle += desync_delta * (float)cfg.antiaim.type[type].inverted_body_lean * 0.01f;
			}

			desync_angle = desync_delta;
		}

		yaw = base_angle + yaw_angle;

		if (!desync_angle)
			return yaw;

		lby_type = cfg.antiaim.lby_type;
	}

	static auto sway_counter = 0;
	static auto force_choke = false;

	if (should_break_lby(m_pcmd, lby_type))
	{
		auto speed = 1.01f;

		if (m_pcmd->m_buttons & IN_DUCK || g_ctx.globals.fakeducking)
			speed *= 2.94117647f;

		static auto switch_move = false;

		if (switch_move)
			m_pcmd->m_sidemove += speed;
		else
			m_pcmd->m_sidemove -= speed;

		switch_move = !switch_move;

		if (lby_type != 2 || sway_counter > 3)
		{
			if (desync_angle > 0.0f)
				yaw -= 179.0f;
			else
				yaw += 179.0f;
		}

		if (sway_counter < 8)
			++sway_counter;
		else
			sway_counter = 0;

		breaking_lby = true;
		force_choke = true;
		g_ctx.send_packet = false;

		return yaw;
	}

	if (force_choke)
	{
		force_choke = false;

		if (m_clientstate()->iChokedCommands < 14)
		{
			g_ctx.send_packet = false;
			return yaw;
		}
	}

	if (g_ctx.send_packet)
		yaw += desync_angle;

	return yaw;
}

bool antiaim::condition(CUserCmd* m_pcmd, bool dynamic_check)
{
	if (!m_pcmd)
		return true;

	if (!g_ctx.available())
		return true;

	if (!cfg.antiaim.enable)
		return true;

	if (!g_ctx.local()->is_alive())
		return true;

	if (g_ctx.local()->m_bGunGameImmunity() || g_ctx.local()->m_fFlags() & FL_FROZEN)
		return true;

	if (g_ctx.local()->get_move_type() == MOVETYPE_NOCLIP || g_ctx.local()->get_move_type() == MOVETYPE_LADDER)
		return true;

	if (g_ctx.globals.aimbot_working)
		return true;

	auto weapon = g_ctx.local()->m_hActiveWeapon().Get();

	if (!weapon)
		return true;

	if (m_pcmd->m_buttons & IN_ATTACK && weapon->m_iItemDefinitionIndex() != WEAPON_REVOLVER && !weapon->is_non_aim())
		return true;

	auto revolver_shoot = weapon->m_iItemDefinitionIndex() == WEAPON_REVOLVER && !g_ctx.globals.revolver_working && (m_pcmd->m_buttons & IN_ATTACK || m_pcmd->m_buttons & IN_ATTACK2);

	if (revolver_shoot)
		return true;

	if ((m_pcmd->m_buttons & IN_ATTACK || m_pcmd->m_buttons & IN_ATTACK2) && weapon->is_knife())
		return true;

	if (dynamic_check && freeze_check)
		return true;

	if (dynamic_check && m_pcmd->m_buttons & IN_USE && !cfg.antiaim.antiaim_type)
		return true;

	if (dynamic_check && weapon->is_grenade() && weapon->m_fThrowTime())
		return true;

	return false;
}

bool antiaim::should_break_lby(CUserCmd* m_pcmd, int lby_type)
{
	if (!lby_type)
		return false;

	if (g_ctx.globals.fakeducking && m_clientstate()->iChokedCommands > 12)
		return false;

	if (!g_ctx.globals.fakeducking && m_clientstate()->iChokedCommands > 14)
	{
		g_ctx.send_packet = true;
		fakelag::get().started_peeking = false;

		return false;
	}

	auto animstate = g_ctx.local()->get_animation_state();

	if (!animstate)
		return false;

	if (animstate->m_velocity > 0.1f || fabs(animstate->flUpVelocity) > 100.0f)
		g_ctx.globals.next_lby_update = TICKS_TO_TIME(g_ctx.globals.fixed_tickbase + 14);
	else
	{
		if (TICKS_TO_TIME(g_ctx.globals.fixed_tickbase) > g_ctx.globals.next_lby_update)
		{
			g_ctx.globals.next_lby_update = 0.0f;
			return true;
		}
	}

	return false;
}

float antiaim::at_targets()
{
	static auto sticky_index = -1;

	Vector view_angles;
	m_engine()->GetViewAngles(view_angles);

	auto aim_angle_to = [](player_t* e)
	{
		auto position = e->GetAbsOrigin();
		auto records = &player_records[e->EntIndex()];

		if (!records->empty() && records->front().valid())
			position = records->front().origin;

		return math::calculate_angle(g_ctx.globals.eye_pos, position + e->m_vecViewOffset());
	};

	player_t* target = nullptr;
	auto best_fov = FLT_MAX;
	Vector best_angle(0.0f, 0.0f, 0.0f);

	auto sticky = static_cast<player_t*>(m_entitylist()->GetClientEntity(sticky_index));

	if (sticky && sticky->valid(true))
	{
		best_angle = aim_angle_to(sticky);
		best_fov = math::get_fov(view_angles, best_angle) * 0.85f;
	}
	else
	{
		sticky = nullptr;
		sticky_index = -1;
	}

	for (auto i = 1; i <= m_globals()->m_maxclients; i++)
	{
		auto e = static_cast<player_t*>(m_entitylist()->GetClientEntity(i));

		if (!e || e == sticky || !e->valid(true))
			continue;

		auto angle = aim_angle_to(e);
		auto fov = math::get_fov(view_angles, angle);

		if (fov >= best_fov)
			continue;

		best_fov = fov;
		best_angle = angle;
		target = e;
	}

	if (target)
		sticky_index = target->EntIndex();

	if (!target && !sticky)
		return g_ctx.get_command()->m_viewangles.y + 180.0f;

	return best_angle.y + 180.0f;
}

bool antiaim::cover_scores(float& left, float& right)
{
	player_t* target = nullptr;
	auto best_distance = FLT_MAX;

	for (auto i = 1; i <= m_globals()->m_maxclients; i++)
	{
		auto e = static_cast<player_t*>(m_entitylist()->GetClientEntity(i));

		if (!e || !e->valid(true))
			continue;

		const auto distance = g_ctx.globals.eye_pos.DistTo(e->GetAbsOrigin());

		if (distance >= best_distance)
			continue;

		best_distance = distance;
		target = e;
	}

	if (!target)
		return false;

	auto source = target->get_eye_pos();
	auto records = &player_records[target->EntIndex()];

	if (!records->empty() && records->front().valid())
		source = records->front().origin + target->m_vecViewOffset();

	auto to_us = g_ctx.globals.eye_pos - source;
	to_us.z = 0.0f;

	const auto length = to_us.Length();

	if (length < 1.0f)
		return false;

	to_us /= length;

	const Vector perpendicular(-to_us.y, to_us.x, 0.0f);

	const auto origin_z = g_ctx.local()->GetAbsOrigin().z;

	const float heights[3] = { g_ctx.globals.eye_pos.z, origin_z + 50.0f, origin_z + 36.0f };
	const float weights[3] = { 3.0f, 1.0f, 1.0f };

	auto covered = [&](float offset)
	{
		auto score = 0.0f;

		for (auto i = 0; i < 3; i++)
		{
			const Vector end(g_ctx.globals.eye_pos.x + perpendicular.x * offset,
				g_ctx.globals.eye_pos.y + perpendicular.y * offset, heights[i]);

			trace_t trace;
			Ray_t ray;
			ray.Init(source, end);

			CTraceFilter filter;
			filter.pSkip = g_ctx.local();

			g_ctx.globals.autowalling = true;
			m_trace()->TraceRay(ray, MASK_SOLID & ~CONTENTS_MONSTER, &filter, &trace);
			g_ctx.globals.autowalling = false;

			if (trace.fraction < 0.97f)
				score += weights[i];
		}

		return score;
	};

	Vector forward, right_vector, up, view_angles;
	m_engine()->GetViewAngles(view_angles);
	view_angles.x = 0.0f;

	math::angle_vectors(view_angles, &forward, &right_vector, &up);

	const auto positive_is_left = perpendicular.x * right_vector.x + perpendicular.y * right_vector.y < 0.0f;

	const auto positive = covered(16.0f);
	const auto negative = covered(-16.0f);

	left = positive_is_left ? positive : negative;
	right = positive_is_left ? negative : positive;

	return true;
}

bool antiaim::automatic_direction()
{
	if (!cfg.antiaim.automatic_direction)
		return false;

	auto left = 0.0f;
	auto right = 0.0f;

	if (!cover_scores(left, right))
		return flip;

	static auto left_ticks = 0;
	static auto right_ticks = 0;

	if (left - right > 0.5f)
		++left_ticks;
	else
		left_ticks = 0;

	if (right - left > 0.5f)
		++right_ticks;
	else
		right_ticks = 0;

	if (left_ticks > 6)
		return true;
	else if (right_ticks > 6)
		return false;

	return flip;
}


void antiaim::freestanding(CUserCmd* m_pcmd)
{
	static auto last_side = (int)SIDE_NONE;

	auto left = 0.0f;
	auto right = 0.0f;

	if (!cover_scores(left, right))
	{
		final_manual_side = last_side;
		return;
	}

	static auto left_ticks = 0;
	static auto right_ticks = 0;
	static auto back_ticks = 0;

	if (left - right > 0.5f)
		++left_ticks;
	else
		left_ticks = 0;

	if (right - left > 0.5f)
		++right_ticks;
	else
		right_ticks = 0;

	if (left <= 0.0f && right <= 0.0f)
		++back_ticks;
	else
		back_ticks = 0;

	if (left_ticks > 6)
		last_side = SIDE_LEFT;
	else if (right_ticks > 6)
		last_side = SIDE_RIGHT;
	else if (back_ticks > 6)
		last_side = SIDE_BACK;

	final_manual_side = last_side;
}
