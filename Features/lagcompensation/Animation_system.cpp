#include "animation_system.h"
#include "..\misc\misc.h"
#include "..\misc\logs.h"

std::deque <adjust_data> player_records[65];


void lagcompensation::fsn(ClientFrameStage_t stage) // Паста
{
	if (stage != FRAME_NET_UPDATE_END)
		return;
	for (auto i = 1; i <= m_globals()->m_maxclients; i++) //-V807
	{
		auto e = static_cast<player_t*>(m_entitylist()->GetClientEntity(i));

		if (e == g_ctx.local())
			continue;

		if (!valid(i, e))
			continue;

		auto time_delta = abs(TIME_TO_TICKS(e->m_flSimulationTime()) - m_globals()->m_tickcount);

		if (time_delta > 1.0f / m_globals()->m_intervalpertick)
			continue;

		auto update = player_records[i].empty() || e->m_flSimulationTime() != e->m_flOldSimulationTime(); //-V550

		if (update && !player_records[i].empty())
		{
			auto server_tick = m_clientstate()->m_iServerTick - i % m_globals()->m_timestamprandomizewindow;
			auto current_tick = server_tick - server_tick % m_globals()->m_timestampnetworkingbase;

			if (TIME_TO_TICKS(e->m_flOldSimulationTime()) < current_tick && TIME_TO_TICKS(e->m_flSimulationTime()) == current_tick)
			{
				auto layer = &e->get_animlayers()[11];
				auto previous_layer = &player_records[i].front().layers[11];

				if (layer->m_flCycle == previous_layer->m_flCycle) //-V550
				{
					e->m_flSimulationTime() = e->m_flOldSimulationTime();
					update = false;
				}
			}
		}

		if (update) //-V550
		{
			if (!player_records[i].empty() && (e->m_vecOrigin() - player_records[i].front().origin).LengthSqr() > 4096.0f)
				for (auto& record : player_records[i])
					record.invalid = true;

			player_records[i].emplace_front(adjust_data());
			update_player_animations(e);

			// IMPORTANT: Limit record size to prevent unbounded memory growth
			// Keep max 32 records per player
			while (player_records[i].size() > 32)
				player_records[i].pop_back();
		}
	}
}

void lagcompensation::upd_nw(player_t* m_pPlayer)
{
	float m_flSimulationTime = 0.0f;

	if (m_pPlayer->EntIndex() >= 64)
		return;

	int m_iNextSimulationTick = m_flSimulationTime / m_globals()->m_intervalpertick + 1;

	g_ctx.globals.updating_animation = true;

	if (m_pPlayer->get_animation_state()->m_iLastClientSideAnimationUpdateFramecount >= m_iNextSimulationTick)
		m_pPlayer->get_animation_state()->m_iLastClientSideAnimationUpdateFramecount = m_iNextSimulationTick - 1;

	m_pPlayer->m_bClientSideAnimation() = true;
	m_pPlayer->update_clientside_animation();

	g_ctx.globals.updating_animation = false;
}

void lagcompensation::extrapolate(player_t* player, Vector& origin, Vector& velocity, int& flags, bool wasonground)
{
	static auto sv_gravity = m_cvar()->FindVar(crypt_str("sv_gravity"));
	static auto sv_jump_impulse = m_cvar()->FindVar(crypt_str("sv_jump_impulse"));

	// Apply gravity if airborne
	if (!(flags & FL_ONGROUND))
		velocity.z -= (m_globals()->m_frametime * sv_gravity->GetFloat());
	else if (wasonground)
		velocity.z = sv_jump_impulse->GetFloat();

	const Vector mins = player->GetCollideable()->OBBMins();
	const Vector max = player->GetCollideable()->OBBMaxs();

	const Vector src = origin;
	Vector end = src + (velocity * m_globals()->m_frametime);

	Ray_t ray;
	ray.Init(src, end, mins, max);

	trace_t trace;
	CTraceFilter filter;
	filter.pSkip = (void*)(player);

	m_trace()->TraceRay(ray, MASK_PLAYERSOLID, &filter, &trace);

	if (trace.fraction != 1.f)
	{
		// Multi-bounce collision resolution (up to 2 bounces)
		for (int i = 0; i < 2; i++)
		{
			// Remove velocity in collision normal direction
			velocity -= trace.plane.normal * velocity.Dot(trace.plane.normal);

			const float dot = velocity.Dot(trace.plane.normal);
			if (dot < 0.f)
			{
				velocity.x -= dot * trace.plane.normal.x;
				velocity.y -= dot * trace.plane.normal.y;
				velocity.z -= dot * trace.plane.normal.z;
			}

			// Continue movement from collision point
			end = trace.endpos + (velocity * (m_globals()->m_intervalpertick * (1.f - trace.fraction)));

			ray.Init(trace.endpos, end, mins, max);
			m_trace()->TraceRay(ray, MASK_PLAYERSOLID, &filter, &trace);

			if (trace.fraction == 1.f)
				break;
		}
	}

	origin = trace.endpos;
	end = trace.endpos;
	end.z -= 2.f;

	// Ground check
	ray.Init(origin, end, mins, max);
	m_trace()->TraceRay(ray, MASK_PLAYERSOLID, &filter, &trace);

	flags &= ~FL_ONGROUND;

	// Valid ground surface check (slope > 0.7 = 45°)
	if (trace.DidHit() && trace.plane.normal.z > 0.7f)
		flags |= FL_ONGROUND;
}

// Enhanced extrapolation with multiple tick simulation
void lagcompensation::extrapolation(player_t* player, Vector& origin, Vector& velocity, int& flags, bool on_ground)
{
	// Don't extrapolate if already accurate
	if (player->m_flSimulationTime() >= m_globals()->m_curtime)
		return;
	
	// Calculate ticks to extrapolate
	float time_delta = m_globals()->m_curtime - player->m_flSimulationTime();
	int ticks_to_extrapolate = TIME_TO_TICKS(time_delta);
	
	// Limit extrapolation to reasonable amount (prevent extreme prediction)
	static auto sv_maxunlag = m_cvar()->FindVar(crypt_str("sv_maxunlag"));
	int max_ticks = TIME_TO_TICKS(sv_maxunlag->GetFloat());
	
	if (ticks_to_extrapolate > max_ticks || ticks_to_extrapolate < 1)
		return;
	
	// Clamp to prevent server rejection (max 64 ticks = ~1 second)
	ticks_to_extrapolate = math::clamp(ticks_to_extrapolate, 1, 64);
	
	// Simulate physics for each tick
	for (int i = 0; i < ticks_to_extrapolate; ++i)
	{
		bool was_on_ground = (flags & FL_ONGROUND) != 0;
		extrapolate(player, origin, velocity, flags, was_on_ground);
	}
}

bool lagcompensation::valid(int i, player_t* e)
{
	if (!cfg.ragebot.enable || !e || !e->valid(false))
	{
		auto keep_records = false;

		if (!e || !e->is_alive())
		{
			is_dormant[i] = false;
			player_resolver[i].reset();

			g_ctx.globals.fired_shots[i] = 0;
			g_ctx.globals.missed_shots[i] = 0;
			g_ctx.globals.backtrack_time[i] = 0.0f;
			g_ctx.globals.backtrack_ticks[i] = 0;
		}
		else if (e->IsDormant())
		{
			is_dormant[i] = true;
			keep_records = cfg.ragebot.enable && cfg.misc.extended_backtack;
		}

		if (!keep_records)
			player_records[i].clear();

		return false;
	}

	return true;
}

void lagcompensation::update_player_animations(player_t* e)
{
	auto animstate = e->get_animation_state();

	if (!animstate)
		return;

	player_info_t player_info;

	if (!m_engine()->GetPlayerInfo(e->EntIndex(), &player_info))
		return;

	auto records = &player_records[e->EntIndex()]; //-V826

	if (records->empty())
		return;

	adjust_data* previous_record = nullptr;
	if (records->size() >= 2)
		previous_record = &records->at(1);

	auto record = &records->front();
	record->store_data(e, false);
	record->bot = player_info.fakeplayer;

	auto weapon = e->m_hActiveWeapon().Get();
	if (previous_record && weapon && !weapon->is_grenade() && !weapon->is_knife())
	{
		const auto shot_time = weapon->m_fLastShotTime();
		record->shot = shot_time > previous_record->simulation_time && shot_time <= record->simulation_time;
	}

	AnimationLayer animlayers[13];
	float pose_parametrs[24];

	memcpy(pose_parametrs, &e->m_flPoseParameter(), 24 * sizeof(float));
	memcpy(record->network_poses, pose_parametrs, 24 * sizeof(float));
	memcpy(animlayers, e->get_animlayers(), e->animlayer_count() * sizeof(AnimationLayer));
	memcpy(record->layers, animlayers, e->animlayer_count() * sizeof(AnimationLayer));
	memcpy(record->left_layers, animlayers, e->animlayer_count() * sizeof(AnimationLayer));
	memcpy(record->right_layers, animlayers, e->animlayer_count() * sizeof(AnimationLayer));
	memcpy(record->center_layers, animlayers, e->animlayer_count() * sizeof(AnimationLayer));

	auto backup_lower_body_yaw_target = e->m_flLowerBodyYawTarget();
	auto backup_duck_amount = e->m_flDuckAmount();
	auto backup_flags = e->m_fFlags();
	auto backup_eflags = e->m_iEFlags();
	auto backup_velocity = e->m_vecVelocity();
	auto backup_abs_velocity = e->m_vecAbsVelocity();
	auto backup_abs_origin = e->GetAbsOrigin();
	auto backup_abs_angles = e->GetAbsAngles();

	auto backup_curtime = m_globals()->m_curtime; //-V807
	auto backup_frametime = m_globals()->m_frametime;
	auto backup_realtime = m_globals()->m_realtime;
	auto backup_framecount = m_globals()->m_framecount;
	auto backup_tickcount = m_globals()->m_tickcount;
	auto backup_interpolation_amount = m_globals()->m_interpolation_amount;

	m_globals()->m_curtime = e->m_flSimulationTime();
	m_globals()->m_frametime = m_globals()->m_intervalpertick;

	if (previous_record)
	{
		auto was_in_air = !(e->m_fFlags() & FL_ONGROUND) || !(previous_record->flags & FL_ONGROUND);

		auto time_difference = max(m_globals()->m_intervalpertick, e->m_flSimulationTime() - previous_record->simulation_time);
		auto origin_delta = e->m_vecOrigin() - previous_record->origin;

		auto animation_speed = 0.0f;

		if (!origin_delta.IsZero() && TIME_TO_TICKS(time_difference) > 0)
		{
			e->m_vecVelocity() = origin_delta * (1.0f / time_difference);

			if (e->m_fFlags() & FL_ONGROUND && animlayers[6].m_flWeight >= 0.1f)
			{
				auto weapon = e->m_hActiveWeapon().Get();

				if (weapon)
				{
					auto max_speed = 260.0f;
					auto weapon_info = weapon->get_csweapon_info();

					if (weapon_info)
						max_speed = e->m_bIsScoped() ? weapon_info->flMaxPlayerSpeedAlt : weapon_info->flMaxPlayerSpeed;

					float weight = animlayers[6].m_flWeight;
					float previous_weight = previous_record->layers[6].m_flWeight;
					float loop_weight = animlayers[11].m_flWeight;

					float length_2d = e->m_vecVelocity().Length2D();

					if ((loop_weight <= 0.f || loop_weight >= 1.f) && length_2d >= 0.1f)
					{
						bool valid_6th_layer = weight > 0.f && weight < 1.f && (weight >= previous_weight);

						if (valid_6th_layer)
						{
							float closest_speed_to_minimal = max_speed * 0.34f;
							float speed_multiplier = std::fmaxf(0.f, (max_speed * 0.52f) - (max_speed * 0.34f));

							auto v208 = 1.f - record->duck_amount;
							if (length_2d > 0.0f)
							{
								auto speed_via_6th_layer = (((v208 * speed_multiplier) + closest_speed_to_minimal) * weight) / length_2d;
								animation_speed = speed_via_6th_layer;
							}
						}
					}
				}
			}

			if (animation_speed > 0.0f)
			{
				auto vel_length = e->m_vecVelocity().Length2D();
				if (vel_length > 0.0f)
				{
					animation_speed /= vel_length;

					e->m_vecVelocity().x *= animation_speed;
					e->m_vecVelocity().y *= animation_speed;
				}
			}

			if (records->size() >= 3 && time_difference > m_globals()->m_intervalpertick)
			{
				auto previous_velocity = (previous_record->origin - records->at(2).origin) * (1.0f / time_difference);

				if (!previous_velocity.IsZero() && !was_in_air)
				{
					auto current_direction = math::normalize_yaw(RAD2DEG(atan2(e->m_vecVelocity().y, e->m_vecVelocity().x)));
					auto previous_direction = math::normalize_yaw(RAD2DEG(atan2(previous_velocity.y, previous_velocity.x)));

					auto average_direction = current_direction - previous_direction;
					average_direction = DEG2RAD(math::normalize_yaw(current_direction + average_direction * 0.5f));

					auto direction_cos = cos(average_direction);
					auto dirrection_sin = sin(average_direction);

					auto velocity_speed = e->m_vecVelocity().Length2D();

					e->m_vecVelocity().x = direction_cos * velocity_speed;
					e->m_vecVelocity().y = dirrection_sin * velocity_speed;
				}
			}

			if (!(e->m_fFlags() & FL_ONGROUND))
			{
				static auto sv_gravity = m_cvar()->FindVar(crypt_str("sv_gravity"));

				auto fixed_timing = math::clamp(time_difference, m_globals()->m_intervalpertick, 1.0f);
				e->m_vecVelocity().z -= sv_gravity->GetFloat() * fixed_timing * 0.5f;
			}
			else
				e->m_vecVelocity().z = 0.0f;
		}
	}

	e->m_iEFlags() &= ~0x1000;

	if (e->m_fFlags() & FL_ONGROUND && e->m_vecVelocity().Length() > 0.0f && animlayers[6].m_flWeight <= 0.0f)
		e->m_vecVelocity().Zero();

	e->m_vecAbsVelocity() = e->m_vecVelocity();
	e->m_bClientSideAnimation() = true;

	if (is_dormant[e->EntIndex()])
	{
		is_dormant[e->EntIndex()] = false;

		if (e->m_fFlags() & FL_ONGROUND)
		{
			animstate->m_bOnGround = true;
			animstate->m_bInHitGroundAnimation = false;
		}

		animstate->m_flGoalFeetYaw = math::normalize_yaw(e->m_angEyeAngles().y);

		// PVS Fix: Initialize shot detection values to prevent false positives
		auto weapon = e->m_hActiveWeapon().Get();
		if (weapon)
		{
			record->last_shot_time = weapon->m_fLastShotTime();
			record->ammo_count = weapon->m_iClip1();
		}
	}

	auto updated_animations = false;

	c_baseplayeranimationstate state;
	memcpy(&state, animstate, sizeof(c_baseplayeranimationstate));

	c_baseplayeranimationstate final_state;
	AnimationLayer final_layers[15];
	float final_poses[24];

	auto final_time = e->m_flSimulationTime();
	auto final_ticks = TIME_TO_TICKS(final_time);
	auto final_flags = e->m_fFlags();
	auto final_duck = e->m_flDuckAmount();
	auto final_lby = e->m_flLowerBodyYawTarget();
	auto have_final = false;

	auto ticks_chocked = 1;
	if (previous_record)
	{
		memcpy(e->get_animlayers(), previous_record->layers, e->animlayer_count() * sizeof(AnimationLayer));
		memcpy(&e->m_flPoseParameter(), pose_parametrs, 24 * sizeof(float));

		auto simulation_ticks = TIME_TO_TICKS(e->m_flSimulationTime() - previous_record->simulation_time);

		if (simulation_ticks > 0 && simulation_ticks < 31)
			ticks_chocked = simulation_ticks;

		if (ticks_chocked > 1)
		{
			auto land_time = 0.0f;
			auto land_in_cycle = false;
			auto is_landed = false;
			auto on_ground = false;

			if (animlayers[4].m_flCycle < 0.5f && (!(e->m_fFlags() & FL_ONGROUND) || !(previous_record->flags & FL_ONGROUND)))
			{
				land_time = e->m_flSimulationTime() - animlayers[4].m_flPlaybackRate * animlayers[4].m_flCycle;
				land_in_cycle = land_time >= previous_record->simulation_time;
			}

			auto duck_amount_per_tick = (e->m_flDuckAmount() - previous_record->duck_amount) / ticks_chocked;

			for (auto i = 1; i <= ticks_chocked; ++i)
			{

				auto lby_delta = fabs(math::normalize_yaw(e->m_flLowerBodyYawTarget() - previous_record->lby));

				if (lby_delta > 0.0f && e->m_vecVelocity().Length() < 5.0f)
				{
					auto delta = ticks_chocked - i;
					auto use_new_lby = true;

					if (lby_delta < 1.0f)
						use_new_lby = !delta; //-V547
					else
						use_new_lby = delta < 2;

					e->m_flLowerBodyYawTarget() = use_new_lby ? backup_lower_body_yaw_target : previous_record->lby;
				}

				auto simulated_time = previous_record->simulation_time + TICKS_TO_TIME(i);

				if (duck_amount_per_tick) //-V550
					e->m_flDuckAmount() = previous_record->duck_amount + duck_amount_per_tick * (float)i;

				on_ground = e->m_fFlags() & FL_ONGROUND;

				if (land_in_cycle && !is_landed)
				{
					if (land_time <= simulated_time)
					{
						is_landed = true;
						on_ground = true;
					}
					else
						on_ground = previous_record->flags & FL_ONGROUND;
				}

				if (on_ground)
					e->m_fFlags() |= FL_ONGROUND;
				else
					e->m_fFlags() &= ~FL_ONGROUND;

				auto simulated_ticks = TIME_TO_TICKS(simulated_time);

				m_globals()->m_realtime = simulated_time;
				m_globals()->m_curtime = simulated_time;
				m_globals()->m_framecount = simulated_ticks;
				m_globals()->m_tickcount = simulated_ticks;
				m_globals()->m_interpolation_amount = 0.0f;

				if (i == ticks_chocked)
				{
					memcpy(&final_state, animstate, sizeof(c_baseplayeranimationstate));
					memcpy(final_layers, e->get_animlayers(), e->animlayer_count() * sizeof(AnimationLayer));
					memcpy(final_poses, &e->m_flPoseParameter(), 24 * sizeof(float));

					final_time = simulated_time;
					final_ticks = simulated_ticks;
					final_flags = e->m_fFlags();
					final_duck = e->m_flDuckAmount();
					final_lby = e->m_flLowerBodyYawTarget();
					have_final = true;
				}

				g_ctx.globals.updating_animation = true;
				e->update_clientside_animation();
				g_ctx.globals.updating_animation = false;

				m_globals()->m_realtime = backup_realtime;
				m_globals()->m_curtime = backup_curtime;
				m_globals()->m_framecount = backup_framecount;
				m_globals()->m_tickcount = backup_tickcount;
				m_globals()->m_interpolation_amount = backup_interpolation_amount;

				updated_animations = true;
			}
		}
	}

	if (!updated_animations)
	{
		memcpy(&final_state, animstate, sizeof(c_baseplayeranimationstate));
		memcpy(final_layers, e->get_animlayers(), e->animlayer_count() * sizeof(AnimationLayer));
		memcpy(final_poses, &e->m_flPoseParameter(), 24 * sizeof(float));

		final_flags = e->m_fFlags();
		final_duck = e->m_flDuckAmount();
		final_lby = e->m_flLowerBodyYawTarget();
		have_final = true;

		g_ctx.globals.updating_animation = true;
		e->update_clientside_animation();
		g_ctx.globals.updating_animation = false;
	}

	memcpy(animstate, &state, sizeof(c_baseplayeranimationstate));

	AnimationLayer baseline_layers[15];
	memcpy(baseline_layers, e->get_animlayers(), e->animlayer_count() * sizeof(AnimationLayer));

	auto setup_matrix = [&](player_t* e, AnimationLayer* layers, const int& matrix) -> bool
		{
			e->invalidate_physics_recursive(8);

			AnimationLayer backup_layers[15];
			memcpy(backup_layers, e->get_animlayers(), e->animlayer_count() * sizeof(AnimationLayer));
			memcpy(e->get_animlayers(), layers, e->animlayer_count() * sizeof(AnimationLayer));

			bool result = false;
			switch (matrix)
			{
			case MAIN:
				result = e->setup_bones_fixed(record->matrixes_data.main, BONE_USED_BY_ANYTHING);
				break;
			case NONE:
				result = e->setup_bones_fixed(record->matrixes_data.zero, BONE_USED_BY_HITBOX);
				break;
			case FIRST:
				result = e->setup_bones_fixed(record->matrixes_data.first, BONE_USED_BY_HITBOX);
				break;
			case SECOND:
				result = e->setup_bones_fixed(record->matrixes_data.second, BONE_USED_BY_HITBOX);
				break;
			case THIRD:
				result = e->setup_bones_fixed(record->matrixes_data.third, BONE_USED_BY_HITBOX);
				break;
			}

			memcpy(e->get_animlayers(), backup_layers, e->animlayer_count() * sizeof(AnimationLayer));
			return result;
		};

	if (!player_info.fakeplayer && g_ctx.local()->is_alive() && e->m_iTeamNum() != g_ctx.local()->m_iTeamNum() && ticks_chocked >= 1)
	{
		animstate->m_flGoalFeetYaw = previous_goal_feet_yaw[e->EntIndex()]; //-V807

		g_ctx.globals.updating_animation = true;
		e->update_clientside_animation();
		g_ctx.globals.updating_animation = false;

		feet_delta[e->EntIndex()] = math::angle_difference(record->angles.y, animstate->m_flGoalFeetYaw);
		previous_goal_feet_yaw[e->EntIndex()] = animstate->m_flGoalFeetYaw;
		memcpy(animstate, &state, sizeof(c_baseplayeranimationstate));

		const auto max_delta = std::clamp(std::fabs(e->get_max_desync_delta()), 25.0f, 60.0f);
		const auto eye_yaw = e->m_angEyeAngles().y;

		auto simulate_side = [&](float yaw, int matrix, int slot)
		{
			memcpy(animstate, have_final ? &final_state : &state, sizeof(c_baseplayeranimationstate));
			memcpy(&e->m_flPoseParameter(), have_final ? final_poses : pose_parametrs, 24 * sizeof(float));
			memcpy(e->get_animlayers(), have_final ? final_layers : baseline_layers, e->animlayer_count() * sizeof(AnimationLayer));

			if (have_final)
			{
				e->m_fFlags() = final_flags;
				e->m_flDuckAmount() = final_duck;
				e->m_flLowerBodyYawTarget() = final_lby;
			}

			animstate->m_flGoalFeetYaw = math::normalize_yaw(yaw);
			animstate->m_flCurrentFeetYaw = animstate->m_flGoalFeetYaw;

			const auto entry_realtime = m_globals()->m_realtime;
			const auto entry_curtime = m_globals()->m_curtime;
			const auto entry_framecount = m_globals()->m_framecount;
			const auto entry_tickcount = m_globals()->m_tickcount;
			const auto entry_interpolation = m_globals()->m_interpolation_amount;

			if (have_final)
			{
				m_globals()->m_realtime = final_time;
				m_globals()->m_curtime = final_time;
				m_globals()->m_framecount = final_ticks;
				m_globals()->m_tickcount = final_ticks;
				m_globals()->m_interpolation_amount = 0.0f;
			}

			g_ctx.globals.updating_animation = true;
			e->update_clientside_animation();
			g_ctx.globals.updating_animation = false;

			m_globals()->m_realtime = entry_realtime;
			m_globals()->m_curtime = entry_curtime;
			m_globals()->m_framecount = entry_framecount;
			m_globals()->m_tickcount = entry_tickcount;
			m_globals()->m_interpolation_amount = entry_interpolation;

			memcpy(record->resolver_poses[slot], &e->m_flPoseParameter(), 24 * sizeof(float));
			memcpy(record->resolver_layers[slot], e->get_animlayers(), sizeof(AnimationLayer) * 13);

			if (matrix >= 0)
				setup_matrix(e, animlayers, matrix);
		};

		for (auto slot = 0; slot < resolver_candidate_count; ++slot)
			simulate_side(eye_yaw + max_delta * resolver_candidate_scale[slot], resolver_candidate_matrix[slot], slot);

		memcpy(animstate, &state, sizeof(c_baseplayeranimationstate));
		memcpy(&e->m_flPoseParameter(), pose_parametrs, 24 * sizeof(float));
		memcpy(e->get_animlayers(), baseline_layers, e->animlayer_count() * sizeof(AnimationLayer));

		player_resolver[e->EntIndex()].initialize(e, record, previous_goal_feet_yaw[e->EntIndex()], e->m_angEyeAngles().x);
		player_resolver[e->EntIndex()].resolve();
	}

	switch (record->type)
	{
	case ANIMATION:
		e->get_animation_state()->m_flLastClientSideAnimationUpdateTime = math::AngleNormalize(e->m_angEyeAngles().y);
		break;
	}

	g_ctx.globals.updating_animation = true;
	e->update_clientside_animation();
	g_ctx.globals.updating_animation = false;

	record->invalid = !setup_matrix(e, animlayers, MAIN);
	const auto cached_count = std::clamp(e->m_CachedBoneData().Count(), 0, MAXSTUDIOBONES);
	memcpy(e->m_CachedBoneData().Base(), record->matrixes_data.main, cached_count * sizeof(matrix3x4_t));

	m_globals()->m_curtime = backup_curtime;
	m_globals()->m_frametime = backup_frametime;
	m_globals()->m_realtime = backup_realtime;
	m_globals()->m_framecount = backup_framecount;
	m_globals()->m_tickcount = backup_tickcount;
	m_globals()->m_interpolation_amount = backup_interpolation_amount;

	e->m_flLowerBodyYawTarget() = backup_lower_body_yaw_target;
	e->m_flDuckAmount() = backup_duck_amount;
	e->m_fFlags() = backup_flags;
	e->m_iEFlags() = backup_eflags;
	e->m_vecVelocity() = backup_velocity;
	e->m_vecAbsVelocity() = backup_abs_velocity;
	e->set_abs_origin(backup_abs_origin);
	e->set_abs_angles(backup_abs_angles);

	memcpy(e->get_animlayers(), animlayers, e->animlayer_count() * sizeof(AnimationLayer));
	memcpy(&e->m_flPoseParameter(), pose_parametrs, 24 * sizeof(float));
	memcpy(player_resolver[e->EntIndex()].previous_layers, animlayers, sizeof(AnimationLayer) * 13);
	record->store_data(e, false);

	if (e->m_flSimulationTime() < e->m_flOldSimulationTime())
		record->invalid = true;

	e->invalidate_physics_recursive(8);
	e->invalidate_bone_cache();
}

bool lagcompensation::is_unsafe_tick(player_t* player)
{
	auto records = &player_records[player->EntIndex()];

	if (records->empty())
		return true; //no records, then skip.

	adjust_data* previous_record = nullptr;

	if (records->size() >= 2)
		previous_record = &records->at(1);

	auto record = &records->front();

	auto ticks = TIME_TO_TICKS(player->m_flSimulationTime() - player->m_flOldSimulationTime());
	if (ticks < 1 && !previous_record) return false; //no previous record, ticks is below 1, we can safely proceed. so let's cache this entity.

	if (previous_record)
	{
		int old_tick = TIME_TO_TICKS(record->simulation_time - previous_record->simulation_time);
		if (ticks < 1 && old_tick < ticks)
			return false; //both records are 0/1 or 0/0

		if (ticks < 1 && old_tick > 2)
		{
			record->invalid = true;
			return true;
		}
		else if (ticks < 2 && old_tick > 0)
			return false;
	}

	return ticks < 2;
}
