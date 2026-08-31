// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

/*

        By Semmxz

*/

#include "aim.h"
#include "..\misc\misc.h"
#include "..\misc\logs.h"
#include "..\autowall\autowall.h"
#include "..\misc\prediction_system.h"
#include "..\fakewalk\slowwalk.h"
#include "..\lagcompensation\local_animations.h"
#include "RageBackTracking.h"

void aim::run(CUserCmd* cmd)
{
    backup.clear();
    targets.clear();
    scanned_targets.clear();
    final_target.reset();
    should_stop = false;

    if (!cfg.ragebot.enable)
    {
        // Clear player records when ragebot is disabled to prevent memory accumulation
        for (auto i = 1; i <= m_globals()->m_maxclients; i++)
            player_records[i].clear();
        return;
    }

    // Limit player records size to prevent memory leak
    for (auto i = 1; i <= m_globals()->m_maxclients; i++)
    {
        if (player_records[i].size() > 64)
        {
            // Keep only the most recent 32 records
            while (player_records[i].size() > 32)
                player_records[i].pop_back();
        }
    }

    automatic_revolver(cmd);
    prepare_targets();

    auto restore_players = [&]()
    {
        for (auto& record : backup)
            record.adjust_player();
    };

    // Safety check: ensure weapon is valid
    if (!g_ctx.globals.weapon)
        return;

    if (g_ctx.globals.weapon->is_non_aim())
        return;

    if (g_ctx.globals.current_weapon == -1)
        return;

    scan_targets();

    if (!should_stop && cfg.ragebot.weapon[g_ctx.globals.current_weapon].autostop_modifiers[AUTOSTOP_PREDICTIVE])
    {
        auto max_speed = 260.0f;
        auto weapon_info = g_ctx.globals.weapon->get_csweapon_info();

        // Safety check before accessing weapon_info
        if (weapon_info)
            max_speed = g_ctx.globals.scoped ? weapon_info->flMaxPlayerSpeedAlt : weapon_info->flMaxPlayerSpeed;

        auto ticks_to_stop = math::clamp(engineprediction::get().backup_data.velocity.Length2D() / max_speed * 3.0f, 0.0f, 4.0f);
        auto predicted_eye_pos = g_ctx.globals.eye_pos + engineprediction::get().backup_data.velocity * m_globals()->m_intervalpertick * ticks_to_stop;

        for (auto& target : targets)
        {
            if (!target.last_record || !target.last_record->valid())
                continue;

            scan_data last_data;

            target.last_record->adjust_player();
            scan(target.last_record, last_data, predicted_eye_pos);

            if (!last_data.valid())
                continue;

            should_stop = true;
            break;
        }
    }

    automatic_scope(cmd);

    restore_players();

    if (!automatic_stop(cmd))
        return;

    if (scanned_targets.empty())
        return;

    find_best_target();

    if (!final_target.data.valid())
    {
        restore_players();
        return;
    }

    fire(cmd);
    restore_players();

}

float m_flMatVal[3][4];

inline void SetOrigin(Vector const& p)
{
    m_flMatVal[0][3] = p.x;
    m_flMatVal[1][3] = p.y;
    m_flMatVal[2][3] = p.z;
}
inline Vector GetOrigin()
{
    return Vector(m_flMatVal[0][3], m_flMatVal[1][3], m_flMatVal[2][3]);
}

/*void aim::automatic_revolver(CUserCmd* cmd)
{
    if (!cfg.ragebot.enable)
        return;

    if (g_ctx.globals.weapon->m_iItemDefinitionIndex() != WEAPON_REVOLVER)
        return;

    if (!m_engine()->IsActiveApp())
        return;

    if (g_ctx.local()->m_flNextAttack() > TICKS_TO_TIME(g_ctx.globals.backup_tickbase))
        return;

    static auto last_checked = 0;
    static auto last_spawn_time = 0.f;
    static auto tick_cocked = 0;
    static auto tick_strip = 0;
    auto time = TICKS_TO_TIME(g_ctx.globals.backup_tickbase);

    const auto max_ticks = TIME_TO_TICKS(.25f) - 1;
    const auto tick_base = TIME_TO_TICKS(time);

    if (g_ctx.local()->m_flSpawnTime() != last_spawn_time) {
        tick_cocked = tick_base;
        tick_strip = tick_base - max_ticks - 1;
        last_spawn_time = g_ctx.local()->m_flSpawnTime();
    }

    if (g_ctx.globals.weapon->m_flNextPrimaryAttack() > time) {
        cmd->m_buttons &= ~IN_ATTACK;
        r8_fire = false;
        return;
    }

    if (last_checked == tick_base)
        return;

    last_checked = tick_base;
    r8_fire = false;

    if (tick_base - tick_strip > 2 && tick_base - tick_strip < 14)
        r8_fire = true;

    if (cmd->m_buttons & IN_ATTACK && r8_fire)
        return;

    cmd->m_buttons |= IN_ATTACK;

    if (g_ctx.globals.weapon->m_flNextSecondaryAttack() >= time)
        cmd->m_buttons |= IN_ATTACK2;

    if (tick_base - tick_cocked > max_ticks * 2 + 1) {
        tick_cocked = tick_base;
        tick_strip = tick_base - max_ticks - 1;
    }

    const auto cock_limit = tick_base - tick_cocked >= max_ticks;
    const auto after_strip = tick_base - tick_strip <= max_ticks;

    if (cock_limit || after_strip) {
        tick_cocked = tick_base;
        cmd->m_buttons &= ~IN_ATTACK;

        if (cock_limit)
            tick_strip = tick_base;
    }
}*/

void aim::automatic_revolver(CUserCmd* cmd)
{
    if (!cfg.ragebot.enable)
        return;

    if (g_ctx.globals.weapon->m_iItemDefinitionIndex() != WEAPON_REVOLVER)
        return;

    static auto fix = 0.0f;

    static auto tick = 0.2f;

    if (cmd->m_buttons &= ~IN_ATTACK)
        return;

    cmd->m_buttons &= ~IN_ATTACK2;

    if (g_ctx.globals.weapon->can_fire(false))
    {
        if (fix <= TICKS_TO_TIME(g_ctx.globals.backup_tickbase) + tick)
        {
            if (g_ctx.globals.weapon->m_flNextSecondaryAttack() <= TICKS_TO_TIME(g_ctx.globals.backup_tickbase))
                fix = TICKS_TO_TIME(g_ctx.globals.backup_tickbase) + 0.2f;
            else
                cmd->m_buttons &= ~IN_ATTACK2;
        }
        else
        {
            cmd->m_buttons |= IN_ATTACK;
        }
    }
    else
    {
        fix = TICKS_TO_TIME(g_ctx.globals.backup_tickbase) + 0.2f;
        cmd->m_buttons &= ~IN_ATTACK;
    }

    g_ctx.globals.revolver_working = true;
}

void aim::AdjustRevolverData(CUserCmd* cmd, int32_t nCommand, int32_t nButtons)
{
    if (g_ctx.local()->m_fFlags() & FL_FROZEN || antiaim::get().freeze_check)
        return;

    int nn;

    auto pCombatWeapon = g_ctx.local()->m_hActiveWeapon().Get();
}

float aim::LerpTime() {
    static auto cl_interp = m_cvar()->FindVar(crypt_str("cl_interp"));
    static auto cl_updaterate = m_cvar()->FindVar(crypt_str("cl_updaterate"));
    static auto cl_interp_ratio = m_cvar()->FindVar(crypt_str("cl_interp_ratio"));

    const auto a2 = cl_updaterate->GetFloat();
    const auto a1 = cl_interp->GetFloat();
    const auto v2 = cl_interp_ratio->GetFloat() / a2;

    return fmaxf(a1, v2);
};

void aim::prepare_targets()
{
    for (auto i = 1; i <= m_globals()->m_maxclients; i++)
    {
        auto e = (player_t*)m_entitylist()->GetClientEntity(i);

        if (!e->valid(true, false))
            continue;

        if (e->m_flSimulationTime() < e->m_flOldSimulationTime())
            continue;

        auto records = &player_records[i];

        if (records->empty())
            continue;

        auto latest = get_record(records, false);
        auto history = get_record(records, true);

        if (!latest)
            continue;

        targets.emplace_back(target(e, latest, history));
    }

    // Limit targets to reasonable count without expensive sorting
    if (targets.size() > 5)
    {
        Vector engine_angles;
        m_engine()->GetViewAngles(engine_angles);

        // Simple distance-based culling instead of O(n²)
        std::sort(targets.begin(), targets.end(), [&engine_angles](const target& a, const target& b) {
            auto dist_a = a.e->GetAbsOrigin().DistTo(g_ctx.local()->GetAbsOrigin());
            auto dist_b = b.e->GetAbsOrigin().DistTo(g_ctx.local()->GetAbsOrigin());
            return dist_a < dist_b;
            });

        targets.resize(5);
    }

    for (auto& target : targets)
        backup.emplace_back(adjust_data(target.e));
}

static bool compare_records(const optimized_adjust_data& first, const optimized_adjust_data& second)
{
    auto first_pitch = math::normalize_pitch(first.angles.x);
    auto second_pitch = math::normalize_pitch(second.angles.x);

    if (fabs(first_pitch - second_pitch) > 15.0f)
        return fabs(first_pitch) < fabs(second_pitch);
    else if (first.duck_amount != second.duck_amount) //-V550
        return first.duck_amount < second.duck_amount;
    else if (first.origin != second.origin)
        return first.origin.DistTo(g_ctx.local()->GetAbsOrigin()) < second.origin.DistTo(g_ctx.local()->GetAbsOrigin());

    return first.simulation_time > second.simulation_time;
}

adjust_data* aim::get_record(std::deque <adjust_data>* records, bool history)
{
	if (!records || records->empty())
		return nullptr;

	const auto start = history ? size_t{ 1 } : size_t{ 0 };
	if (start >= records->size())
		return nullptr;

	if (history)
	{
		for (auto i = start; i < records->size(); ++i)
		{
			auto record = &records->at(i);
			if (record->valid() && record->shot)
				return record;
		}
	}

	for (auto i = start; i < records->size(); ++i)
	{
		auto record = &records->at(i);
		if (record->valid())
			return record;
	}

    return nullptr;
}

int aim::get_minimum_damage(bool visible, int health)
{
    auto minimum_damage = 1;
    if (visible)
    {
        if (cfg.ragebot.weapon[g_ctx.globals.current_weapon].minimum_visible_damage > 100)
            minimum_damage = health + cfg.ragebot.weapon[g_ctx.globals.current_weapon].minimum_visible_damage - 100;
        else
            minimum_damage = math::clamp(cfg.ragebot.weapon[g_ctx.globals.current_weapon].minimum_visible_damage, 1, health);
    }
    else
    {
        if (cfg.ragebot.weapon[g_ctx.globals.current_weapon].minimum_damage > 100)
            minimum_damage = health + cfg.ragebot.weapon[g_ctx.globals.current_weapon].minimum_damage - 100;
        else
            minimum_damage = math::clamp(cfg.ragebot.weapon[g_ctx.globals.current_weapon].minimum_damage, 1, health);
    }

    if (g_ctx.globals.current_weapon != -1 && key_binds::get().get_key_bind_state(4 + g_ctx.globals.current_weapon))
    {
        if (cfg.ragebot.weapon[g_ctx.globals.current_weapon].minimum_override_damage > 100)
            minimum_damage = health + cfg.ragebot.weapon[g_ctx.globals.current_weapon].minimum_override_damage - 100;
        else
            minimum_damage = math::clamp(cfg.ragebot.weapon[g_ctx.globals.current_weapon].minimum_override_damage, 1, health);
    }

    return minimum_damage;
}

int aim::get_adaptive_minimum_damage(bool visible, int health, int hitbox)
{
    return get_minimum_damage(visible, health);
}

void aim::scan_targets()
{
    if (targets.empty())
        return;

    for (auto& target : targets)
    {
        if (!target.last_record || !target.last_record->valid())
            continue;

        if (target.history_record && target.history_record != target.last_record && target.history_record->valid())
        {
            scan_data last_data;

            target.last_record->adjust_player();
            scan(target.last_record, last_data);

            scan_data history_data;

            target.history_record->adjust_player();
            scan(target.history_record, history_data);

            if (last_data.valid() && history_data.valid())
            {
                if (last_data.damage + 5 >= history_data.damage || (last_data.visible && !history_data.visible))
                    scanned_targets.emplace_back(scanned_target(target.last_record, last_data));
                else
                    scanned_targets.emplace_back(scanned_target(target.history_record, history_data));
            }
            else if (last_data.valid())
                scanned_targets.emplace_back(scanned_target(target.last_record, last_data));
            else if (history_data.valid())
                scanned_targets.emplace_back(scanned_target(target.history_record, history_data));
        }
        else
        {
            scan_data last_data;

            target.last_record->adjust_player();
            scan(target.last_record, last_data);

            if (!last_data.valid())
                continue;

            scanned_targets.emplace_back(scanned_target(target.last_record, last_data));
        }
    }
}

bool aim::automatic_stop(CUserCmd* cmd)
{
    static auto last_stop_tick = INT_MIN;

    if (!cfg.ragebot.weapon[g_ctx.globals.current_weapon].autostop)
        return true;

    if (should_stop)
        last_stop_tick = m_globals()->m_tickcount;

    if (!should_stop)
    {
        auto ticks_since_stop = m_globals()->m_tickcount - last_stop_tick;

        if (targets.empty() || ticks_since_stop < 0 || ticks_since_stop > 5)
            return true;
    }

    if (g_ctx.globals.slowwalking || g_ctx.globals.fakeducking || misc::get().recharging_double_tap)
        return true;

    if (!(g_ctx.local()->m_fFlags() & FL_ONGROUND && engineprediction::get().backup_data.flags & FL_ONGROUND))
        return true;

    if (g_ctx.globals.weapon->is_empty())
        return true;

    if (!cfg.ragebot.weapon[g_ctx.globals.current_weapon].autostop_modifiers[AUTOSTOP_BETWEEN_SHOTS] && !g_ctx.globals.weapon->can_fire(false))
        return true;

    auto animlayer = g_ctx.local()->get_animlayers()[1];

    if (animlayer.m_nSequence)
    {
        auto activity = g_ctx.local()->sequence_activity(animlayer.m_nSequence);

        if (activity == ACT_CSGO_RELOAD && animlayer.m_flWeight > 0.0f)
            return true;
    }

    auto weapon_info = g_ctx.globals.weapon->get_csweapon_info();

    if (!weapon_info)
        return true;

    auto max_speed = 0.33f * (g_ctx.globals.scoped ? weapon_info->flMaxPlayerSpeedAlt : weapon_info->flMaxPlayerSpeed);

    if (engineprediction::get().backup_data.velocity.Length2D() < max_speed)
        slowwalk::get().create_move(cmd);
    else
    {
        Vector direction;
        Vector real_view;

        math::vector_angles(engineprediction::get().backup_data.velocity, direction);
        m_engine()->GetViewAngles(real_view);

        direction.x = 0.0f;
        direction.y = real_view.y - direction.y;

        Vector forward;
        math::angle_vectors(direction, forward);

        static auto cl_forwardspeed = m_cvar()->FindVar(crypt_str("cl_forwardspeed"));
        static auto cl_sidespeed = m_cvar()->FindVar(crypt_str("cl_sidespeed"));

        auto negative_forward_speed = -cl_forwardspeed->GetFloat();
        auto negative_side_speed = -cl_sidespeed->GetFloat();

        auto negative_forward_direction = forward * negative_forward_speed;
        auto negative_side_direction = forward * negative_side_speed;

        cmd->m_forwardmove = negative_forward_direction.x;
        cmd->m_sidemove = negative_side_direction.y;

        if (cfg.ragebot.weapon[g_ctx.globals.current_weapon].autostop_modifiers[AUTOSTOP_FORCE_ACCURACY])
            return false;
    }

    return true;
}

static bool compare_points(const scan_point& first, const scan_point& second)
{
    return !first.center && first.hitbox == second.hitbox;
}

void aim::scan(adjust_data* record, scan_data& data, const Vector& shoot_position)
{
    auto weapon = g_ctx.globals.weapon;

    if (!weapon)
        return;

    auto weapon_info = weapon->get_csweapon_info();

    if (!weapon_info)
        return;

    auto hitboxes = get_hitboxes(record);

    if (hitboxes.empty())
        return;

    auto& weapon_cfg = cfg.ragebot.weapon[g_ctx.globals.current_weapon];

    auto safe_points_key = key_binds::get().get_key_bind_state(3);
    auto force_safe_points = safe_points_key || weapon_cfg.max_misses && g_ctx.globals.missed_shots[record->i] >= weapon_cfg.max_misses_amount;
    auto best_damage = 0;

    auto health = record->player->m_iHealth();

    auto visible_minimum_damage = get_adaptive_minimum_damage(true, health, HITBOX_HEAD);
    auto occluded_minimum_damage = get_adaptive_minimum_damage(false, health, HITBOX_HEAD);
    auto damage_floor = (float)min(visible_minimum_damage, occluded_minimum_damage);

	std::vector <scan_point> points;

	for (auto& hitbox : hitboxes)
	{
		auto current_points = get_points(record, hitbox, true);

		for (auto& point : current_points)
		{
			if (!hitbox_intersection(record->player, record->matrixes_data.main, hitbox, shoot_position, point.point))
				continue;

			if (!record->bot)
			{
				auto safe = 1.0f;

				if (record->matrixes_data.zero[0].GetOrigin() == record->matrixes_data.first[0].GetOrigin() || record->matrixes_data.zero[0].GetOrigin() == record->matrixes_data.second[0].GetOrigin() || record->matrixes_data.first[0].GetOrigin() == record->matrixes_data.second[0].GetOrigin())
					safe = 0.0f;
				else if (!hitbox_intersection(record->player, record->matrixes_data.zero, hitbox, shoot_position, point.point, &safe))
					safe = 0.0f;
				else if (!hitbox_intersection(record->player, record->matrixes_data.first, hitbox, shoot_position, point.point, &safe))
					safe = 0.0f;
				else if (!hitbox_intersection(record->player, record->matrixes_data.second, hitbox, shoot_position, point.point, &safe))
					safe = 0.0f;

				point.safe = safe;
			}
			else
				point.safe = 1.0f;

			if (!safe_points_key || weapon_cfg.prefer_safe_points || point.safe)
			{
				points.emplace_back(point);
			};
		}
	}

	if (points.empty())
		return;

	auto body_hitboxes = true;
	float best_score = -1.0f;

	auto& body_aim_modifiers = weapon_cfg.body_aim_modifiers;

	auto force_body_aim = body_aim_modifiers[BAIM_PREFER]
		|| body_aim_modifiers[BAIM_AIR] && !(record->flags & FL_ONGROUND)
		|| body_aim_modifiers[BAIM_HIGH_VELOCITY] && record->velocity.Length2D() > 85.0f
		|| body_aim_modifiers[BAIM_DOUBLE_TAP] && g_ctx.globals.double_tap_aim
		|| body_aim_modifiers[BAIM_UNRESOLVED] && !record->bot && !record->resolver_confident;

	auto lethal_body = body_aim_modifiers[BAIM_LETHAL];

	scan_point lethal_body_point;

	auto lethal_body_damage = 0;
	auto lethal_body_hitbox = -1;
	auto lethal_body_visible = false;
	float lethal_body_score = -1.0f;

	for (auto& point : points)
	{
		auto is_body = point.hitbox >= HITBOX_PELVIS && point.hitbox <= HITBOX_UPPER_CHEST;

		if (body_hitboxes && !is_body)
		{
			body_hitboxes = false;

			if (key_binds::get().get_key_bind_state(22))
				break;

			if (best_damage >= health)
				break;

			if ((force_body_aim || weapon_cfg.prefer_body_aim) && best_damage >= 1)
				break;
		}

		auto fire_data = autowall::get().wall_penetration(shoot_position, point.point, record->player, damage_floor);

		if (!fire_data.valid)
			continue;

		if (fire_data.damage < 1)
			continue;

		if (!fire_data.visible && !cfg.ragebot.enable)
			continue;

		auto current_minimum_damage = fire_data.visible ? visible_minimum_damage : occluded_minimum_damage;

		auto effective_damage = min(fire_data.damage, health);

		float current_score = static_cast<float>(effective_damage);

		if (effective_damage >= health)
			current_score += 40.0f;

		if (fire_data.visible)
			current_score += 12.0f;

		current_score += point.center ? 2.0f : 0.0f;
		current_score += point.safe ? 1.0f : 0.0f;

		if (lethal_body && is_body && effective_damage >= health)
			current_score += 8.0f;

		if (current_score >= best_score && fire_data.damage >= current_minimum_damage)
		{
			if (!should_stop)
			{
				should_stop = true;

				if (weapon_cfg.autostop_modifiers[AUTOSTOP_LETHAL] && fire_data.damage < health)
					should_stop = false;
				else if (weapon_cfg.autostop_modifiers[AUTOSTOP_VISIBLE] && !fire_data.visible)
					should_stop = false;
				else if (weapon_cfg.autostop_modifiers[AUTOSTOP_CENTER] && !point.center)
					should_stop = false;
			}

			if (force_safe_points && !point.safe)
				continue;

			if (lethal_body && is_body && fire_data.damage >= health && current_score > lethal_body_score)
			{
				lethal_body_score = current_score;
				lethal_body_damage = fire_data.damage;
				lethal_body_hitbox = fire_data.hitbox;
				lethal_body_visible = fire_data.visible;
				lethal_body_point = point;
			}

			best_damage = fire_data.damage;
			best_score = current_score;

			data.point = point;
			data.visible = fire_data.visible;
			data.damage = fire_data.damage;
			data.hitbox = fire_data.hitbox;

			if (fire_data.visible && point.center && point.safe && fire_data.damage >= health)
				break;
		}
	}

	if (lethal_body_hitbox >= 0 && lethal_body_damage >= data.damage)
	{
		data.point = lethal_body_point;
		data.visible = lethal_body_visible;
		data.damage = lethal_body_damage;
		data.hitbox = lethal_body_hitbox;
	}
}

std::vector <int> aim::get_hitboxes(adjust_data* record)
{
    std::vector <int> hitboxes;

    if (cfg.ragebot.weapon[g_ctx.globals.current_weapon].hitboxes.at(1))
        hitboxes.emplace_back(HITBOX_UPPER_CHEST);

    if (cfg.ragebot.weapon[g_ctx.globals.current_weapon].hitboxes.at(2))
        hitboxes.emplace_back(HITBOX_CHEST);

    if (cfg.ragebot.weapon[g_ctx.globals.current_weapon].hitboxes.at(3))
        hitboxes.emplace_back(HITBOX_LOWER_CHEST);

    if (cfg.ragebot.weapon[g_ctx.globals.current_weapon].hitboxes.at(4))
        hitboxes.emplace_back(HITBOX_STOMACH);

    if (cfg.ragebot.weapon[g_ctx.globals.current_weapon].hitboxes.at(5))
        hitboxes.emplace_back(HITBOX_PELVIS);

    if (cfg.ragebot.weapon[g_ctx.globals.current_weapon].hitboxes.at(0))
        hitboxes.emplace_back(HITBOX_HEAD);

    if (cfg.ragebot.weapon[g_ctx.globals.current_weapon].hitboxes.at(6))
    {
        hitboxes.emplace_back(HITBOX_RIGHT_UPPER_ARM);
        hitboxes.emplace_back(HITBOX_LEFT_UPPER_ARM);
    }

    if (cfg.ragebot.weapon[g_ctx.globals.current_weapon].hitboxes.at(7))
    {
        hitboxes.emplace_back(HITBOX_RIGHT_THIGH);
        hitboxes.emplace_back(HITBOX_LEFT_THIGH);

        hitboxes.emplace_back(HITBOX_RIGHT_CALF);
        hitboxes.emplace_back(HITBOX_LEFT_CALF);
    }

    if (cfg.ragebot.weapon[g_ctx.globals.current_weapon].hitboxes.at(8))
    {
        hitboxes.emplace_back(HITBOX_RIGHT_FOOT);
        hitboxes.emplace_back(HITBOX_LEFT_FOOT);
    }

    return hitboxes;
}


void aim::update_peek_state()
{
    g_ctx.globals.m_Peek.m_bIsPeeking = false;
    if (!cfg.ragebot.enable && (!cfg.antiaim.fakelag || !cfg.antiaim.triggers_fakelag_amount > 2))
        return;

    if (g_ctx.local()->m_vecVelocity().Length2D() > 5.0f)
        return;

    // predpos
    Vector predicted_eye_pos = g_ctx.globals.eye_pos + (engineprediction::get().backup_data.velocity * m_globals()->m_intervalpertick);

    for (auto i = 1; i <= m_globals()->m_maxclients; i++)
    {
        auto e = static_cast<player_t*>(m_entitylist()->GetClientEntity(i));
        if (!e->valid(true))
            continue;

        auto records = &player_records[i];
        if (records->empty())
            continue;

        auto record = &records->front();
        if (!record->valid())
            continue;

        // apply player animated data
        record->adjust_player();

        // look all ticks for get first hitable
        for (int next_chock = 1; next_chock <= m_clientstate()->iChokedCommands; ++next_chock)
        {
            predicted_eye_pos *= next_chock;

            auto fire_data = autowall::get().wall_penetration(predicted_eye_pos, e->hitbox_position_matrix(HITBOX_HEAD, record->matrixes_data.first), e);
            if (!fire_data.valid || fire_data.damage < 1)
                continue;

            g_ctx.globals.m_Peek.m_bIsPeeking = true;
            m_debugoverlay()->AddBoxOverlay(predicted_eye_pos, Vector(-0.7f, -0.7f, -0.7f), Vector(0.7f, 0.7f, 0.7f), Vector(0.f, 0.f, 0.f), 0, 255, 0, 100, m_globals()->m_intervalpertick * 2);
        }
    }
}

float aim::bodyscale(player_t* e) // 
{
    if (!(e->m_fFlags() & FL_ONGROUND))
        return 0.f;

    if (cfg.ragebot.weapon[g_ctx.globals.current_weapon].static_point_scale)
    {
        auto scale = cfg.ragebot.weapon[g_ctx.globals.current_weapon].body_scale;
        if (scale > 1.0f)
            scale *= 0.01f;
        return std::clamp(scale, 0.0f, 0.75f);
    }

    // Calculate dynamic body scale based on distance
    auto distance = e->m_vecOrigin().DistTo(g_ctx.globals.eye_pos);
    auto max_distance = weapon_range / 4.f;
    
    float factor = 0.f;
    if (distance < max_distance)
    {
        // Sigmoid curve for smooth scale transition
        float normalized = (distance / max_distance);
        factor = 1.f - 1.f / (1.f + pow(2.f, -(normalized * 2.f - 1.f) / 0.115f));
    }

    if (g_ctx.globals.weapon->is_sniper() && g_ctx.globals.scoped)
        factor = 0.f;

    if (e->m_flDuckAmount() >= 0.9f && !cfg.misc.fakeduck_key.key)
        return 0.65f;

    return std::clamp(factor, 0.f, 0.75f);
}

float aim::GetHeadScale(player_t* e)
{
    if (cfg.ragebot.weapon[g_ctx.globals.current_weapon].static_point_scale)
    {
        auto scale = cfg.ragebot.weapon[g_ctx.globals.current_weapon].head_scale;
        if (scale > 1.0f)
            scale *= 0.01f;
        return std::clamp(scale, 0.0f, 0.80f);
    }

    if (cfg.misc.fakeduck_key.key)
        return 0.70f;

    if (e->m_fFlags() & FL_ONGROUND)
        return bodyscale(e);
    else
        return 0.75f;
}

std::vector <scan_point> aim::get_points(adjust_data* record, int hitbox, bool from_aim)
{
    std::vector <scan_point> points;

    if (!record || !record->player || record->bone_count <= 0)
        return points;

    auto model = record->player->GetModel();

    if (!model)
        return points;

    auto hdr = m_modelinfo()->GetStudioModel(model);

    if (!hdr)
        return points;

    auto set = hdr->pHitboxSet(record->player->m_nHitboxSet());

    if (!set)
        return points;

    mstudiobbox_t* bbox = set->pHitbox(hitbox);
    if (!bbox || bbox->bone < 0 || bbox->bone >= record->bone_count)
        return points;

    Vector minimum;
    Vector maximum;
    math::vector_transform(bbox->bbmin, record->matrixes_data.main[bbox->bone], minimum);
    math::vector_transform(bbox->bbmax, record->matrixes_data.main[bbox->bone], maximum);
    const auto center = (minimum + maximum) * 0.5f;

    if (bbox->radius <= 0.0f)
    {
        auto rotation_matrix = math::angle_matrix(bbox->rotation);

        matrix3x4_t matrix;
        math::concat_transforms(record->matrixes_data.main[bbox->bone], rotation_matrix, matrix);

        if (hitbox == HITBOX_RIGHT_FOOT || hitbox == HITBOX_LEFT_FOOT)
        {
            const auto local_center = (bbox->bbmin + bbox->bbmax) * 0.5f;
            Vector local_points[3] =
            {
                local_center,
                Vector(local_center.x + (bbox->bbmin.x - local_center.x) * 0.75f, local_center.y, local_center.z),
                Vector(local_center.x + (bbox->bbmax.x - local_center.x) * 0.75f, local_center.y, local_center.z)
            };

            for (int i = 0; i < 3; ++i)
            {
                Vector world;
                math::vector_transform(local_points[i], matrix, world);
                points.emplace_back(scan_point(world, hitbox, i == 0));
            }
        }

        return points;
    }

    auto scale = hitbox == HITBOX_HEAD ? GetHeadScale(record->player) : bodyscale(record->player);
    if (!cfg.ragebot.weapon[g_ctx.globals.current_weapon].static_point_scale)
    {
        const auto spread = fmaxf(g_ctx.globals.spread + g_ctx.globals.inaccuracy, 0.0f);
        const auto spread_radius = center.DistTo(g_ctx.globals.eye_pos) * std::tan(spread);
        scale = std::clamp((bbox->radius - spread_radius) / bbox->radius, 0.0f,
            hitbox == HITBOX_HEAD ? 0.80f : 0.75f);
    }

    points.emplace_back(scan_point(center, hitbox, true));

    if (scale <= 0.0f)
        return points;

    const auto radius = bbox->radius * scale;
    Vector forward;
    Vector right;
    Vector up;
    math::angle_vectors(math::calculate_angle(g_ctx.globals.eye_pos, center), &forward, &right, &up);

    if (hitbox == HITBOX_HEAD)
    {
        points.emplace_back(scan_point(center + right * radius, hitbox, false));
        points.emplace_back(scan_point(center - right * radius, hitbox, false));
        points.emplace_back(scan_point(center + up * radius, hitbox, false));
    }
    else if (hitbox >= HITBOX_PELVIS && hitbox <= HITBOX_UPPER_CHEST)
    {
        points.emplace_back(scan_point(center + right * radius, hitbox, false));
        points.emplace_back(scan_point(center - right * radius, hitbox, false));
    }
    else if (from_aim && (hitbox == HITBOX_RIGHT_CALF || hitbox == HITBOX_LEFT_CALF ||
        hitbox == HITBOX_RIGHT_UPPER_ARM || hitbox == HITBOX_LEFT_UPPER_ARM))
    {
        points.emplace_back(scan_point(center + right * radius * 0.5f, hitbox, false));
    }

    return points;
}

static bool compare_targets(const scanned_target& first, const scanned_target& second)
{
    if (cfg.player_list.high_priority[first.record->i] != cfg.player_list.high_priority[second.record->i])
        return cfg.player_list.high_priority[first.record->i];

    return first.data.damage > second.data.damage;

}

void aim::find_best_target()
{
    if (cfg.ragebot.weapon[g_ctx.globals.current_weapon].selection_type)
        std::sort(scanned_targets.begin(), scanned_targets.end(), compare_targets);

    for (auto& target : scanned_targets)
    {
        final_target = target;
        final_target.record->adjust_player();
        break;
    }
}

bool IsTickValid(float simTime)
{
    static auto cl_interp_ratio = m_cvar()->FindVar(crypt_str("cl_interp_ratio"));
    static auto sv_client_min_interp_ratio = m_cvar()->FindVar(crypt_str("sv_client_min_interp_ratio"));
    static auto sv_client_max_interp_ratio = m_cvar()->FindVar(crypt_str("sv_client_max_interp_ratio"));
    static auto sv_maxunlag = m_cvar()->FindVar(crypt_str("sv_maxunlag"));
    auto lerp_ratio = math::clamp(cl_interp_ratio->GetFloat(), sv_client_min_interp_ratio->GetFloat(), sv_client_max_interp_ratio->GetFloat());
    INetChannelInfo* nci = m_engine()->GetNetChannelInfo();

    if (!nci)
        return false;

    auto lerpTicks = TIME_TO_TICKS(lerp_ratio);

    int predCmdArrivTick = m_globals()->m_tickcount + 1 + 
        TIME_TO_TICKS(nci->GetAvgLatency(FLOW_INCOMING) + 
                      nci->GetAvgLatency(FLOW_OUTGOING));

    float outgoingLatency = nci->GetLatency(FLOW_OUTGOING);
    float clampedLatency = math::clamp(lerp_ratio + outgoingLatency, 0.f, sv_maxunlag->GetFloat());

    int timeDelta = TIME_TO_TICKS(simTime) + TIME_TO_TICKS(lerp_ratio);
    float correction = clampedLatency - TICKS_TO_TIME(predCmdArrivTick + lerpTicks - timeDelta);

    return abs(correction) < sv_maxunlag->GetFloat();
}

bool aim::calculate_hitchance(const Vector& aim_angle, player_t* ent, int& final_hitchance)
{
    build_seed_table();

    static int iTotalSeeds;

    auto weapon = g_ctx.local()->m_hActiveWeapon();
    if (!weapon)
        return false;

    auto weapon_info = weapon->get_csweapon_info();
    if (!weapon_info)
        return false;

    Vector angles;

    float flChance = cfg.ragebot.weapon[g_ctx.globals.current_weapon].hitchance ? cfg.ragebot.weapon[g_ctx.globals.current_weapon].hitchance_amount : 0;

    Vector fw, rw, uw;
    math::aim::AngleVectors(angles, fw, rw, uw);

    int hits = 0;
    int needed_hits = static_cast<int>(iTotalSeeds * (flChance / 100.f));

    float weapon_spread = weapon->get_spread();
    float weapon_inaccuracy = weapon->get_inaccuracy();

    Vector src = g_ctx.local()->get_eye_pos();

    for (int i = 0; i < iTotalSeeds; i++) {

        float a = math::random_float(0.f, 1.f);
        float b = math::random_float(0.f, M_PI * 2.f);
        float c = math::random_float(0.f, 1.f);
        float d = math::random_float(0.f, M_PI * 2.f);
        float inaccuracy = a * weapon_inaccuracy;
        float spread = c * weapon_spread;

        if (weapon->m_iItemDefinitionIndex() == 64) {
            if (g_ctx.get_command()->m_buttons & IN_ATTACK2) {
                a = 1.f - a * a;
                c = 1.f - c * c;
            }
        }

        Vector spread_view((cos(b) * inaccuracy) + (cos(d) * spread), (sin(b) * inaccuracy) + (sin(d) * spread), 0);
        Vector direction;

        direction.x = fw.x + (spread_view.x * rw.x) + (spread_view.y * uw.x);
        direction.y = fw.y + (spread_view.x * rw.y) + (spread_view.y * uw.y);
        direction.z = fw.z + (spread_view.x * rw.z) + (spread_view.y * uw.z);
        direction.Normalized();

        Vector viewangles_spread;
        Vector view_forward;

        math::aim::Vector_Angles(direction, uw, viewangles_spread);
        viewangles_spread.Normalize();
        math::aim::AngleVectors2(viewangles_spread, view_forward);

        view_forward.NormalizeInPlace();
        view_forward = src + (view_forward * weapon_info->flRange);

        trace_t tr;
        Ray_t ray;

        ray.Init(src, view_forward);
        m_trace()->ClipRayToEntity(ray, MASK_SHOT, ent, &tr);

        if (tr.hit_entity == ent)
            hits++;

        if (static_cast<int>((static_cast<float>(hits) / iTotalSeeds) * 100.f) >= flChance)
            return true;

        if ((iTotalSeeds - i + hits) < needed_hits)
            return false;
    }
    final_hitchance = ((float)hits / (float)iTotalSeeds);
    return false;
}

int aim::calc_bt_ticks()
{
    auto records = &player_records[final_target.record->player->EntIndex()];

    for (auto i = 0; i < records->size(); i++)
    {
        auto record = &records->at(i);

        if (record->simulation_time == final_target.record->simulation_time)
            return i;
    }
}

void aim::automatic_scope(CUserCmd* cmd)
{
    if (!cfg.ragebot.autoscope)
        return;

    if (!g_ctx.globals.weapon)
        return;

    auto index = g_ctx.globals.weapon->m_iItemDefinitionIndex();

    if (index != WEAPON_SCAR20 && index != WEAPON_G3SG1 && index != WEAPON_SSG08 && index != WEAPON_AWP && index != WEAPON_AUG && index != WEAPON_SG553)
        return;

    if (g_ctx.globals.weapon->m_zoomLevel())
        return;

    if (cmd->m_buttons & IN_ATTACK2)
        return;

    if (g_ctx.globals.weapon->is_empty())
        return;

    if (g_ctx.local()->m_bIsDefusing())
        return;

    if (!scanned_targets.empty())
    {
        cmd->m_buttons |= IN_ATTACK2;
        return;
    }

    if (targets.empty())
        return;

    auto weapon_info = g_ctx.globals.weapon->get_csweapon_info();

    if (!weapon_info)
        return;

    auto max_speed = g_ctx.globals.scoped ? weapon_info->flMaxPlayerSpeedAlt : weapon_info->flMaxPlayerSpeed;
    auto velocity = engineprediction::get().backup_data.velocity;

    auto ticks_to_stop = math::clamp(velocity.Length2D() / max_speed * 3.0f, 0.0f, 4.0f);
    auto lookahead = math::clamp((float)m_clientstate()->iChokedCommands + 6.0f + ticks_to_stop, 1.0f, 16.0f);

    auto predicted_eye_pos = g_ctx.globals.eye_pos + velocity * (m_globals()->m_intervalpertick * lookahead);

    for (auto& target : targets)
    {
        if (!target.last_record || !target.last_record->valid())
            continue;

        if (predicted_eye_pos.DistTo(target.last_record->origin) > weapon_info->flRange)
            continue;

        scan_data predicted_data;

        target.last_record->adjust_player();
        scan(target.last_record, predicted_data, predicted_eye_pos);

        if (!predicted_data.valid())
            continue;

        cmd->m_buttons |= IN_ATTACK2;
        break;
    }
}

void aim::fire(CUserCmd* cmd)
{
    if (!g_ctx.globals.weapon)
        return;

    if (g_ctx.globals.current_weapon == -1)
        return;

    if (!g_ctx.globals.weapon->can_fire(true))
        return;

    auto weapon_info = g_ctx.globals.weapon->get_csweapon_info();
    if (!weapon_info)
        return;

    auto aim_angle = math::calculate_angle(g_ctx.globals.eye_pos, final_target.data.point.point).Clamp();

    if (!cfg.ragebot.enable)
        m_engine()->SetViewAngles(aim_angle);

    if (!cfg.ragebot.enable && !(cmd->m_buttons & IN_ATTACK))
        return;


    auto hitchance_amount = cfg.ragebot.weapon[g_ctx.globals.current_weapon].hitchance_amount;

    auto is_valid_hitchance = hitchance(aim_angle);

    if (cfg.ragebot.weapon[g_ctx.globals.current_weapon].hitchance)
    {
        if (!is_valid_hitchance || final_hitchance < hitchance_amount)
        {
            auto is_zoomable_weapon = g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_SCAR20 || g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_G3SG1 || g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_SSG08 || g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_AWP || g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_AUG || g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_SG553;

            if (cfg.ragebot.autoscope && is_zoomable_weapon && !g_ctx.globals.weapon->m_zoomLevel())
                cmd->m_buttons |= IN_ATTACK2;

            return;
        }
    }


    // auto final_hitchance = 0;

    if (!g_ctx.globals.double_tap_aim && cfg.ragebot.weapon[g_ctx.globals.current_weapon].hitchance)
        hitchance_amount = cfg.ragebot.weapon[g_ctx.globals.current_weapon].hitchance_amount;

    if (!is_valid_hitchance)
        return;

    auto backtrack_ticks = 0;
    auto net_channel_info = m_engine()->GetNetChannelInfo();

    if (net_channel_info)
    {
        auto original_tickbase = g_ctx.globals.backup_tickbase;

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
        };

    player_info_t player_info;
    m_engine()->GetPlayerInfo(final_target.record->i, &player_info);

    cmd->m_viewangles = aim_angle;
    cmd->m_buttons |= IN_ATTACK;
    
    // Use tick-accurate value for lag compensation
    cmd->m_tickcount = TIME_TO_TICKS(final_target.record->simulation_time + util::get_interpolation());

    last_target_index = final_target.record->i;
    last_shoot_position = g_ctx.globals.eye_pos;
    last_target[last_target_index] = Last_target{
        *final_target.record, final_target.data, final_target.distance
    };

    // Limit shots array to prevent unbounded memory growth
    const int MAX_SHOTS = 256;
    if (g_ctx.shots.size() >= MAX_SHOTS)
    {
        g_ctx.shots.erase(g_ctx.shots.begin());
    }

    auto shot = &g_ctx.shots.emplace_back();
    shot->last_target = last_target_index;
    shot->side = final_target.record->side;
    shot->fire_tick = m_globals()->m_tickcount;
    shot->shot_info.target_name = player_info.szName;
    shot->shot_info.client_hitbox = get_hitbox_name(final_target.data.hitbox, true);
    shot->shot_info.client_damage = final_target.data.damage;
    shot->shot_info.hitchance = final_hitchance;
    shot->shot_info.backtrack_ticks = backtrack_ticks;
    shot->shot_info.aim_point = final_target.data.point.point;
    shot->shot_info.distance_to_target = final_target.distance;

    // Capture weapon name from weapon index
    shot->shot_info.weapon_name = g_ctx.globals.weapon->get_name();

    // Capture target position at fire time for prediction error detection
    shot->target_position_at_fire = final_target.record->player->GetAbsOrigin();
	shot->shoot_position = g_ctx.globals.eye_pos;

    // Get network latency
    auto net_channel = m_engine()->GetNetChannelInfo();
    if (net_channel && !net_channel->IsLoopback() && !m_engine()->IsPlayingDemo())
    {
        auto latency = net_channel->GetLatency(FLOW_OUTGOING) + net_channel->GetLatency(FLOW_INCOMING);
        shot->shot_info.network_latency_ms = static_cast<int>(latency * 1000.0f);
    }
	else
		shot->shot_info.network_latency_ms = 0;

    // Capture target animation data
    auto target_player = final_target.record->player;
    if (target_player)
    {
        auto animlayers = target_player->get_animlayers();
        if (animlayers)
        {
            shot->shot_info.target_animation_sequence = animlayers[0].m_nSequence;
            shot->shot_info.target_animation_cycle = animlayers[0].m_flCycle;
        }
    }

    // Capture current map
    if (g_ctx.globals.current_map == crypt_str("unknown"))
    {
        const char* map_name = m_engine()->GetLevelName();
        if (map_name)
        {
            std::string map_str(map_name);
            // Extract just the map name from path (e.g., "maps/de_dust2" -> "de_dust2")
            size_t pos = map_str.find_last_of("/\\");
            if (pos != std::string::npos)
                map_str = map_str.substr(pos + 1);
            g_ctx.globals.current_map = map_str;
        }
    }

    g_ctx.globals.aimbot_working = true;
    g_ctx.globals.revolver_working = false;
    g_ctx.globals.last_aimbot_shot = m_globals()->m_tickcount;
}

static std::vector<std::tuple<float, float, float>> precomputed_seeds = {};

__forceinline void aim::build_seed_table()
{
    if (!precomputed_seeds.empty())
        return;

    for (auto i = 0; i < 255; i++)
    {
        math::random_seed(i + 1);

        const auto pi_seed = math::random_float(0.f, twopi);

        precomputed_seeds.emplace_back(math::random_float(0.f, 1.f),
            sin(pi_seed), cos(pi_seed));
    }
}

static const int iTotalSeeds = 255;
static std::vector<std::tuple<float, float, float>> PreComputedSeeds = {};
void BulidSeedTable() {

    if (!PreComputedSeeds.empty()) return;

    for (auto i = 0; i < iTotalSeeds; i++) {
        math::random_seed(i + 1);

        const auto pi_seed = math::random_float(0.f, M_PI * 2);

        PreComputedSeeds.emplace_back(math::random_float(0.f, 1.f), sin(pi_seed), cos(pi_seed));
    }
}

int aim::hitchance(const Vector& aim_angle)
{
    // Use member variable instead of local
    final_hitchance = 0;
    auto weapon_info = g_ctx.globals.weapon->get_csweapon_info();

    if (!weapon_info)
        return final_hitchance;

    if ((g_ctx.globals.eye_pos - final_target.data.point.point).Length() > weapon_info->flRange)
        return final_hitchance;

    auto forward = ZERO;
    auto right = ZERO;
    auto up = ZERO;

    math::angle_vectors(aim_angle, &forward, &right, &up);

    math::fast_vec_normalize(forward);
    math::fast_vec_normalize(right);
    math::fast_vec_normalize(up);

    // Use weapon's actual calculated inaccuracy instead of static values
    auto real_inaccuracy = g_ctx.globals.weapon->get_inaccuracy();
    
    // Edge case: ladder movement has special inaccuracy
    if (g_ctx.local()->get_move_type() == MOVETYPE_LADDER)
        real_inaccuracy = max(real_inaccuracy, weapon_info->flInaccuracyLadder);

    auto is_special_weapon = g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_AWP || g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_G3SG1 || g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_SCAR20 || g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_SSG08;
    auto base_inaccuracy = weapon_info->flInaccuracyStand;

    if (g_ctx.local()->m_fFlags() & FL_DUCKING)
    {
        if (is_special_weapon)
            base_inaccuracy = weapon_info->flInaccuracyCrouchAlt;
        else
            base_inaccuracy = weapon_info->flInaccuracyCrouch;
    }
    else if (is_special_weapon)
        base_inaccuracy = weapon_info->flInaccuracyStandAlt;

    auto is_head = final_target.data.hitbox == HITBOX_HEAD;
    auto high_accuracy_weapon = g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_AWP || g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_SSG08;

    auto ring_spread = g_ctx.globals.spread + g_ctx.globals.inaccuracy;

    auto ring_intersections = 0;
    auto ring_reaching = 0;
    auto damage = 0;

    for (auto i = 1; i <= 6; ++i)
    {
        for (auto j = 0; j < 8; ++j)
        {
            auto current_spread = ring_spread * ((float)i / 6.0f);

            auto direction_cos = 0.0f;
            auto direction_sin = 0.0f;

            DirectX::XMScalarSinCos(&direction_cos, &direction_sin, (float)j / 8.0f * DirectX::XM_2PI);

            auto spread_x = direction_cos * current_spread;
            auto spread_y = direction_sin * current_spread;

            auto direction = ZERO;

            direction.x = forward.x + spread_x * right.x + spread_y * up.x;
            direction.y = forward.y + spread_x * right.y + spread_y * up.y;
            direction.z = forward.z + spread_x * right.z + spread_y * up.z;

            auto end = g_ctx.globals.eye_pos + direction * weapon_info->flRange;

            if (!hitbox_intersection(final_target.record->player, final_target.record->matrixes_data.main, final_target.data.hitbox, g_ctx.globals.eye_pos, end))
                continue;

            ++ring_intersections;

            auto fire_data = autowall::get().wall_penetration(g_ctx.globals.eye_pos, end, final_target.record->player);

            if (!fire_data.valid || fire_data.damage < 1)
                continue;

            ++ring_reaching;

            if (is_head && fire_data.hitbox != HITBOX_HEAD)
                continue;

            damage += high_accuracy_weapon ? fire_data.damage : 1;
        }
    }

    auto occlusion_scale = 1.0f;

    if (ring_intersections)
    {
        if (!ring_reaching)
            return final_hitchance;

        occlusion_scale = (float)ring_reaching / (float)ring_intersections;
    }

    if (real_inaccuracy - 0.000001f < base_inaccuracy)
        final_hitchance = 101;
    else
    {
        static auto setup_spread_values = true;
        static float spread_values[256][6];

        if (setup_spread_values)
        {
            setup_spread_values = false;

            for (auto i = 0; i < 256; ++i)
            {
                math::random_seed(i + 1);

                auto a = math::random_float(0.0f, 1.0f);
                auto b = math::random_float(0.0f, DirectX::XM_2PI);
                auto c = math::random_float(0.0f, 1.0f);
                auto d = math::random_float(0.0f, DirectX::XM_2PI);

                spread_values[i][0] = a;
                spread_values[i][1] = c;

                auto sin_b = 0.0f, cos_b = 0.0f;
                DirectX::XMScalarSinCos(&sin_b, &cos_b, b);

                auto sin_d = 0.0f, cos_d = 0.0f;
                DirectX::XMScalarSinCos(&sin_d, &cos_d, d);

                spread_values[i][2] = sin_b;
                spread_values[i][3] = cos_b;
                spread_values[i][4] = sin_d;
                spread_values[i][5] = cos_d;
            }
        }

        auto hits = 0;

        for (auto i = 0; i < 256; ++i)
        {
            auto inaccuracy = spread_values[i][0] * g_ctx.globals.inaccuracy;
            auto spread = spread_values[i][1] * g_ctx.globals.spread;

            auto spread_x = spread_values[i][3] * inaccuracy + spread_values[i][5] * spread;
            auto spread_y = spread_values[i][2] * inaccuracy + spread_values[i][4] * spread;

            auto direction = ZERO;

            direction.x = forward.x + right.x * spread_x + up.x * spread_y;
            direction.y = forward.y + right.y * spread_x + up.y * spread_y;
            direction.z = forward.z + right.z * spread_x + up.z * spread_y;

            auto end = g_ctx.globals.eye_pos + direction * weapon_info->flRange;

            if (hitbox_intersection(final_target.record->player, final_target.record->matrixes_data.main, final_target.data.hitbox, g_ctx.globals.eye_pos, end))
                ++hits;
        }

        final_hitchance = (int)((float)hits / 2.56f);
    }

    final_hitchance = (int)((float)final_hitchance * occlusion_scale);

    if (g_ctx.globals.double_tap_aim)
        return final_hitchance;

    if (high_accuracy_weapon)
        return (float)damage / 48.0f >= get_minimum_damage(final_target.data.visible, final_target.health) ? final_hitchance : 0;

    return final_hitchance;
}

static int clip_ray_to_hitbox(const Ray_t& ray, mstudiobbox_t* hitbox, matrix3x4_t& matrix, trace_t& trace)
{
    static auto fn = util::FindSignature(crypt_str("client.dll"), crypt_str("55 8B EC 83 E4 F8 F3 0F 10 42"));

    trace.fraction = 1.0f;
    trace.startsolid = false;

    return reinterpret_cast <int(__fastcall*)(const Ray_t&, mstudiobbox_t*, matrix3x4_t&, trace_t&)> (fn)(ray, hitbox, matrix, trace);
}

bool aim::hitbox_intersection(player_t* e, matrix3x4_t* matrix, int hitbox, const Vector& start, const Vector& end, float* safe)
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

/*
            TODO // ĶÅ ÄĪĻČŃĄĶĄß ×ĄŃŅÜ
*/
