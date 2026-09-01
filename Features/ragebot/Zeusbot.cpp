#include "zeusbot.h"
#include "..\misc\misc.h"
#include "..\misc\logs.h"
#include "..\autowall\autowall.h"
#include "..\misc\prediction_system.h"
#include "..\fakewalk\slowwalk.h"
#include "..\lagcompensation\local_animations.h"
#include <random>

void zeusbot::run(CUserCmd* cmd)
{
	scanned_targets.clear();
	final_target.reset();

	if (!cfg.ragebot.enable)
		return;

	if (!cfg.ragebot.zeus_bot)
		return;

	if (!g_ctx.globals.weapon)
		return;

	if (g_ctx.globals.weapon->m_iItemDefinitionIndex() != WEAPON_TASER)
		return;

	if (!g_ctx.globals.weapon->can_fire(false))
		return;

	scan_targets();

	auto restore_players = [&]()
	{
		for (auto& record : aim::get().backup)
			record.adjust_player();
	};

	if (scanned_targets.empty())
	{
		restore_players();
		return;
	}

	if (cfg.ragebot.weapon[1].autostop && cfg.ragebot.weapon[1].autostop_modifiers[AUTOSTOP_TASER])
		slowwalk::get().autostop(cmd);

	find_best_target();

	if (!final_target.data.valid())
	{
		restore_players();
		return;
	}

	fire(cmd);
	restore_players();
}

void zeusbot::scan_targets()
{
	if (aim::get().targets.empty())
		return;

	for (auto& target : aim::get().targets)
	{
		if (target.history_record && target.history_record->valid())
		{
			scan_data last_data;

			if (target.last_record && target.last_record->valid())
			{
				target.last_record->adjust_player();
				scan(target.last_record, last_data);
			}

			scan_data history_data;

			target.history_record->adjust_player();
			scan(target.history_record, history_data);

			if (last_data.valid() && last_data.damage > history_data.damage)
				scanned_targets.emplace_back(scanned_target(target.last_record, last_data));
			else if (history_data.valid())
				scanned_targets.emplace_back(scanned_target(target.history_record, history_data));
		}
		else
		{
			if (!target.last_record || !target.last_record->valid())
				continue;

			scan_data last_data;

			target.last_record->adjust_player();
			scan(target.last_record, last_data);

			if (!last_data.valid())
				continue;

			scanned_targets.emplace_back(scanned_target(target.last_record, last_data));
		}
	}
}

static constexpr auto zeus_fallback_range = 180.0f;

static float zeus_range()
{
	auto weapon_info = g_ctx.globals.weapon->get_csweapon_info();

	if (!weapon_info || weapon_info->flRange <= 0.0f)
		return zeus_fallback_range;

	return weapon_info->flRange;
}

void zeusbot::scan(adjust_data* record, scan_data& data)
{
	auto max_range = zeus_range();
	auto best_distance = FLT_MAX;

	for (auto hitbox = (int)HITBOX_PELVIS; hitbox <= (int)HITBOX_UPPER_CHEST; ++hitbox)
	{
		auto point = scan_point(record->player->hitbox_position_matrix(hitbox, record->matrixes_data.main), hitbox, true);

		if (point.point.IsZero())
			continue;

		auto distance = g_ctx.globals.eye_pos.DistTo(point.point);

		if (distance > max_range || distance >= best_distance)
			continue;

		auto fire_data = autowall::get().wall_penetration(g_ctx.globals.eye_pos, point.point, record->player);

		if (!fire_data.valid || !fire_data.visible || fire_data.damage < 1)
			continue;

		best_distance = distance;

		data.point = point;
		data.visible = fire_data.visible;
		data.damage = fire_data.damage;
		data.hitbox = fire_data.hitbox;
	}
}

static bool compare_targets(const scanned_target& first, const scanned_target& second)
{
	return first.distance < second.distance;
}

void zeusbot::find_best_target()
{
	std::sort(scanned_targets.begin(), scanned_targets.end(), compare_targets);

	for (auto& target : scanned_targets)
	{
		final_target = target;
		final_target.record->adjust_player();
		break;
	}
}

void zeusbot::fire(CUserCmd* cmd)
{
	if (!g_ctx.globals.weapon->can_fire(true))
		return;

	auto aim_angle = math::calculate_angle(g_ctx.globals.eye_pos, final_target.data.point.point).Clamp();

	auto final_hitchance = hitchance(aim_angle);

	if (final_hitchance < 50)
		return;

	auto backtrack_ticks = 0;
	auto net_channel_info = m_engine()->GetNetChannelInfo();

	if (net_channel_info)
	{
		auto original_tickbase = g_ctx.globals.backup_tickbase;

		if (misc::get().double_tap_enabled && misc::get().double_tap_key)
			original_tickbase = g_ctx.globals.backup_tickbase + g_ctx.globals.weapon->get_max_tickbase_shift();

		static auto sv_maxunlag = m_cvar()->FindVar(crypt_str("sv_maxunlag"));

		auto correct = math::clamp(net_channel_info->GetLatency(FLOW_OUTGOING) + net_channel_info->GetLatency(FLOW_INCOMING) + util::get_interpolation(), 0.0f, sv_maxunlag->GetFloat());
		auto delta_time = correct - (TICKS_TO_TIME(original_tickbase) - final_target.record->simulation_time);

		backtrack_ticks = TIME_TO_TICKS(fabs(delta_time));
	}

	static auto get_hitbox_name = [](int hitbox, bool shot_info = false) -> std::string
	{
		switch (hitbox)
		{
		case HITBOX_HEAD:
			return shot_info ? crypt_str("Head") : crypt_str("head");
		case HITBOX_LOWER_CHEST:
			return shot_info ? crypt_str("Lower chest") : crypt_str("lower chest");
		case HITBOX_CHEST:
			return shot_info ? crypt_str("Chest") : crypt_str("chest");
		case HITBOX_UPPER_CHEST:
			return shot_info ? crypt_str("Upper chest") : crypt_str("upper chest");
		case HITBOX_STOMACH:
			return shot_info ? crypt_str("Stomach") : crypt_str("stomach");
		case HITBOX_PELVIS:
			return shot_info ? crypt_str("Pelvis") : crypt_str("pelvis");
		case HITBOX_RIGHT_UPPER_ARM:
		case HITBOX_RIGHT_FOREARM:
		case HITBOX_RIGHT_HAND:
			return shot_info ? crypt_str("Left arm") : crypt_str("left arm");
		case HITBOX_LEFT_UPPER_ARM:
		case HITBOX_LEFT_FOREARM:
		case HITBOX_LEFT_HAND:
			return shot_info ? crypt_str("Right arm") : crypt_str("right arm");
		case HITBOX_RIGHT_THIGH:
		case HITBOX_RIGHT_CALF:
			return shot_info ? crypt_str("Left leg") : crypt_str("left leg");
		case HITBOX_LEFT_THIGH:
		case HITBOX_LEFT_CALF:
			return shot_info ? crypt_str("Right leg") : crypt_str("right leg");
		case HITBOX_RIGHT_FOOT:
			return shot_info ? crypt_str("Left foot") : crypt_str("left foot");
		case HITBOX_LEFT_FOOT:
			return shot_info ? crypt_str("Right foot") : crypt_str("right foot");
		}

		return shot_info ? crypt_str("Generic") : crypt_str("generic");
	};

	player_info_t player_info;
	m_engine()->GetPlayerInfo(final_target.record->i, &player_info);

#if BETA
	std::stringstream log;

	log << crypt_str("Fired shot at ") + (std::string)player_info.szName + crypt_str(": ");
	log << crypt_str("hitchance: ") + (final_hitchance == 101 ? crypt_str("MA") : std::to_string(final_hitchance)) + crypt_str(", ");
	log << crypt_str("hitbox: ") + get_hitbox_name(final_target.data.hitbox) + crypt_str(", ");
	log << crypt_str("damage: ") + std::to_string(final_target.data.damage) + crypt_str(", ");
	log << crypt_str("backtrack: ") + std::to_string(backtrack_ticks);

	if (cfg.misc.events_to_log[EVENTLOG_HIT])
		eventlogs::get().add(log.str());
#endif
	cmd->m_viewangles = aim_angle;
	cmd->m_buttons |= IN_ATTACK;
	cmd->m_tickcount = TIME_TO_TICKS(final_target.record->simulation_time) + TIME_TO_TICKS(util::get_interpolation());

	g_ctx.globals.aimbot_working = true;
	g_ctx.globals.last_aimbot_shot = m_globals()->m_tickcount;
}

static const float zeus_ring_x[8] = { 1.0f, 0.70710678f, 0.0f, -0.70710678f, -1.0f, -0.70710678f, 0.0f, 0.70710678f };
static const float zeus_ring_y[8] = { 0.0f, 0.70710678f, 1.0f, 0.70710678f, 0.0f, -0.70710678f, -1.0f, -0.70710678f };

int zeusbot::hitchance(const Vector& aim_angle)
{
	auto max_range = zeus_range();
	auto distance = g_ctx.globals.eye_pos.DistTo(final_target.data.point.point);

	if (distance > max_range)
		return 0;

	auto forward = ZERO;
	auto right = ZERO;
	auto up = ZERO;

	math::angle_vectors(aim_angle, &forward, &right, &up);

	math::fast_vec_normalize(forward);
	math::fast_vec_normalize(right);
	math::fast_vec_normalize(up);

	auto end = g_ctx.globals.eye_pos + forward * max_range;

	if (!hitbox_intersection(final_target.record->player, final_target.record->matrixes_data.main, final_target.data.hitbox, g_ctx.globals.eye_pos, end))
		return 0;

	auto drift = final_target.record->player->m_vecVelocity().Length2D() * (util::get_interpolation() + m_globals()->m_intervalpertick);

	if (drift <= 0.0f || distance <= 1.0f)
		return 101;

	auto tolerance = drift / distance;

	auto intersecting = 0;
	auto samples = 0;

	for (auto ring = 1; ring <= 3; ++ring)
	{
		auto radius = tolerance * ((float)ring / 3.0f);

		for (auto step = 0; step < 8; ++step)
		{
			auto direction = forward + right * (zeus_ring_x[step] * radius) + up * (zeus_ring_y[step] * radius);
			auto sample_end = g_ctx.globals.eye_pos + direction * max_range;

			++samples;

			if (hitbox_intersection(final_target.record->player, final_target.record->matrixes_data.main, final_target.data.hitbox, g_ctx.globals.eye_pos, sample_end))
				++intersecting;
		}
	}

	if (intersecting == samples)
		return 101;

	return (int)((float)intersecting / (float)samples * 100.0f);
}

static int clip_ray_to_hitbox(const Ray_t& ray, mstudiobbox_t* hitbox, matrix3x4_t& matrix, trace_t& trace)
{
	static auto fn = util::FindSignature(crypt_str("client.dll"), crypt_str("55 8B EC 83 E4 F8 F3 0F 10 42"));

	trace.fraction = 1.0f;
	trace.startsolid = false;

	return reinterpret_cast <int(__fastcall*)(const Ray_t&, mstudiobbox_t*, matrix3x4_t&, trace_t&)> (fn)(ray, hitbox, matrix, trace);
}

bool zeusbot::hitbox_intersection(player_t* e, matrix3x4_t* matrix, int hitbox, const Vector& start, const Vector& end, float* safe)
{
	auto model = e->GetModel();

	if (!model)
		return false;

	auto studio_model = m_modelinfo()->GetStudioModel(model);

	if (!studio_model)
		return false;

	auto studio_set = studio_model->pHitboxSet(e->m_nHitboxSet());

	if (!studio_set)
		return false;

	auto studio_hitbox = studio_set->pHitbox(hitbox);

	if (!studio_hitbox)
		return false;

	if (studio_hitbox->bone < 0 || studio_hitbox->bone >= MAXSTUDIOBONES)
		return false;

	trace_t trace;

	Ray_t ray;
	ray.Init(start, end);

	auto intersected = clip_ray_to_hitbox(ray, studio_hitbox, matrix[studio_hitbox->bone], trace) >= 0;

	if (!safe)
		return intersected;

	Vector min, max;

	math::vector_transform(studio_hitbox->bbmin, matrix[studio_hitbox->bone], min);
	math::vector_transform(studio_hitbox->bbmax, matrix[studio_hitbox->bone], max);

	auto center = (min + max) * 0.5f;
	auto distance = center.DistTo(end);

	if (distance > *safe)
		*safe = distance;

	return intersected;
}