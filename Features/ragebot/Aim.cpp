#include "aim.h"
#include "..\misc\misc.h"
#include "..\misc\logs.h"
#include "..\autowall\autowall.h"
#include "..\misc\prediction_system.h"
#include "..\fakewalk\slowwalk.h"
#include "..\lagcompensation\local_animations.h"

void aim::run(CUserCmd* cmd)
{
    backup.clear();
    targets.clear();
    scanned_targets.clear();
    final_target.reset();
    should_stop = false;
    g_ctx.globals.autostop_fakeduck = false;

    if (!cfg.ragebot.enable)
    {
        for (auto i = 1; i <= m_globals()->m_maxclients; i++)
        {
            player_records[i].clear();

            g_ctx.globals.backtrack_time[i] = 0.0f;
            g_ctx.globals.backtrack_ticks[i] = 0;
        }

        return;
    }

    automatic_revolver(cmd);
    prepare_targets();

    auto restore_players = [&]()
    {
        for (auto& record : backup)
            record.adjust_player();
    };

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

void aim::automatic_revolver(CUserCmd* cmd)
{
    if (!cfg.ragebot.enable)
        return;

    if (g_ctx.globals.weapon->m_iItemDefinitionIndex() != WEAPON_REVOLVER)
        return;

    static auto fix = 0.0f;

    static auto tick = 0.2f;

    if (!(cmd->m_buttons & IN_ATTACK))
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

int aim::backtrack_window()
{
    static auto sv_maxunlag = m_cvar()->FindVar(crypt_str("sv_maxunlag"));

    auto max_unlag = sv_maxunlag ? sv_maxunlag->GetFloat() : 0.2f;
    auto latency = 0.0f;

    auto net_channel = m_engine()->GetNetChannelInfo();

    if (net_channel)
        latency = net_channel->GetLatency(FLOW_OUTGOING) + net_channel->GetLatency(FLOW_INCOMING);

    auto usable = max(max_unlag - 0.01f - latency, 0.0f);
    auto server_limit = (int)(usable / m_globals()->m_intervalpertick);

    if (server_limit < 1)
        server_limit = 1;

    auto configured = cfg.ragebot.backtrack_ticks;

    if (configured < 1)
        configured = 1;

    return min(configured, server_limit);
}

void aim::collect_records(target& current)
{
    auto records = &player_records[current.e->EntIndex()];

    if (!current.last_record)
        return;

    const auto window = backtrack_window();
    const auto newest_tick = TIME_TO_TICKS(current.last_record->simulation_time);

    auto previous_tick = INT_MIN;

    current.records.reserve(min(records->size(), (size_t)(window + 1)));

    for (auto& record : *records)
    {
        const auto tick = TIME_TO_TICKS(record.simulation_time);

        if (newest_tick - tick > window)
            break;

        if (tick == previous_tick)
            continue;

        if (&record != current.last_record && !record.valid())
            continue;

        previous_tick = tick;

        record.hittable = false;
        record.selected = false;
        record.hittable_damage = 0;
        record.hittable_hitbox = -1;
        record.hittable_tick = m_globals()->m_tickcount;
        record.hittable_point.Zero();

        current.records.emplace_back(&record);

        if (current.records.size() >= 24)
            break;
    }
}

static float target_threat(player_t* e, const Vector& viewangles, const Vector& eye_pos)
{
    auto head = e->hitbox_position(HITBOX_HEAD);

    if (head.IsZero())
        head = e->GetAbsOrigin() + e->m_vecViewOffset();

    const auto distance = eye_pos.DistTo(head);
    const auto fov = math::get_fov(viewangles, math::calculate_angle(eye_pos, head));

    auto score = 1024.0f;

    score -= math::clamp(fov, 0.0f, 180.0f) * 4.0f;
    score -= math::clamp(distance, 0.0f, 4096.0f) * 0.06f;
    score -= (float)math::clamp(e->m_iHealth(), 0, 100) * 1.2f;

    auto weapon = e->m_hActiveWeapon().Get();

    if (weapon)
    {
        if (weapon->is_non_aim())
            score -= 160.0f;
        else
        {
            const auto reverse = math::get_fov(e->m_angEyeAngles(), math::calculate_angle(head, eye_pos));

            if (reverse < 30.0f)
                score += (30.0f - reverse) * 3.5f;
        }
    }

    return score;
}

void aim::prepare_targets()
{
    Vector engine_angles;
    m_engine()->GetViewAngles(engine_angles);

    for (auto i = 1; i <= m_globals()->m_maxclients; i++)
    {
        auto e = (player_t*)m_entitylist()->GetClientEntity(i);

        if (!e)
            continue;

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

    if (targets.size() > 5)
    {
        std::sort(targets.begin(), targets.end(), [&engine_angles](const target& a, const target& b) {
            return target_threat(a.e, engine_angles, g_ctx.globals.eye_pos) > target_threat(b.e, engine_angles, g_ctx.globals.eye_pos);
            });

        targets.resize(5);
    }

    for (auto& target : targets)
    {
        collect_records(target);
        backup.emplace_back(adjust_data(target.e));
    }
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

float aim::probe_record(adjust_data* record)
{
    static const int probe_hitboxes[] = { HITBOX_HEAD, HITBOX_UPPER_CHEST, HITBOX_CHEST, HITBOX_STOMACH, HITBOX_PELVIS };

    auto health = record->player->m_iHealth();
    auto best = -1.0f;

    for (auto hitbox : probe_hitboxes)
    {
        auto position = record->player->hitbox_position_matrix(hitbox, record->matrixes_data.main);

        if (position.IsZero())
            continue;

        if (!hitbox_intersection(record->player, record->matrixes_data.main, hitbox, g_ctx.globals.eye_pos, position))
            continue;

        auto fire_data = autowall::get().wall_penetration(g_ctx.globals.eye_pos, position, record->player);

        if (!fire_data.valid || fire_data.damage < 1)
            continue;

        auto rating = (float)min(fire_data.damage, health);

        if (fire_data.damage >= health)
            rating += 40.0f;

        if (fire_data.visible)
            rating += 12.0f;

        if (record->shot)
            rating += 22.0f;

        rating += record->resolver_confidence * 16.0f;

        if (fire_data.damage > record->hittable_damage)
        {
            record->hittable = true;
            record->hittable_damage = fire_data.damage;
            record->hittable_hitbox = hitbox;
            record->hittable_point = position;
        }

        best = max(best, rating);
    }

    return best;
}

float aim::rate_record(adjust_data* record, scan_data& data, float newest_time)
{
    auto health = record->player->m_iHealth();
    auto rating = (float)min(data.damage, health);

    auto trust = record->bot ? 1.0f : math::clamp(record->resolver_confidence, 0.0f, 1.0f);
    auto unresolved = trust < 0.55f;

    if (data.damage >= health)
        rating += 60.0f;

    if (data.visible)
        rating += 30.0f;

    if (data.point.safe > 0.0f)
        rating += unresolved ? 26.0f : 6.0f;

    if (data.point.center)
        rating += 3.0f;

    if (record->shot)
        rating += 22.0f;

    rating += trust * 18.0f;

    auto age = TIME_TO_TICKS(newest_time - record->simulation_time);

    if (age > 0)
    {
        auto window = max(backtrack_window(), 1);
        auto fraction = min((float)age / (float)window, 1.0f);

        rating -= (float)age + 16.0f * fraction * fraction;
    }

    return rating;
}

void aim::select_record(target& current)
{
    const auto index = current.e->EntIndex();
    const auto newest_time = current.last_record->simulation_time;
    const auto health = current.e->m_iHealth();

    g_ctx.globals.backtrack_time[index] = 0.0f;
    g_ctx.globals.backtrack_ticks[index] = 0;

    adjust_data* best_record = nullptr;
    scan_data best_data;

    auto best_rating = -FLT_MAX;

    auto consider = [&](adjust_data* record)
    {
        scan_data data;

        record->adjust_player();
        scan(record, data);

        if (!data.valid())
            return false;

        if (data.damage > record->hittable_damage)
        {
            record->hittable = true;
            record->hittable_damage = data.damage;
            record->hittable_hitbox = data.hitbox;
            record->hittable_point = data.point.point;
        }

        auto rating = rate_record(record, data, newest_time);

        if (rating > best_rating)
        {
            best_rating = rating;
            best_record = record;
            best_data = data;
        }

        return true;
    };

    consider(current.last_record);

    auto flawless = best_record && best_data.visible && best_data.damage >= health
        && (best_data.point.safe > 0.0f || current.last_record->bot);

    if (!flawless && current.records.size() > 1)
    {
        struct probe_t
        {
            adjust_data* record;
            float rating;
        };

        probe_t probes[24];
        auto probe_count = 0;

        for (auto record : current.records)
        {
            if (record == current.last_record)
                continue;

            if (probe_count >= 24)
                break;

            record->adjust_player();

            auto rating = probe_record(record);

            if (rating < 0.0f)
                continue;

            probes[probe_count].record = record;
            probes[probe_count].rating = rating;

            ++probe_count;
        }

        std::sort(probes, probes + probe_count, [](const probe_t& first, const probe_t& second) {
            return first.rating > second.rating;
            });

        auto scans = min(probe_count, 3);

        for (auto i = 0; i < scans; i++)
            consider(probes[i].record);
    }

    if (!best_record)
        return;

    best_record->selected = true;

    g_ctx.globals.backtrack_time[index] = best_record->simulation_time;
    g_ctx.globals.backtrack_ticks[index] = max(TIME_TO_TICKS(newest_time - best_record->simulation_time), 0);

    scanned_targets.emplace_back(scanned_target(best_record, best_data));
}

void aim::scan_targets()
{
    if (targets.empty())
        return;

    for (auto& target : targets)
    {
        if (!target.last_record || !target.last_record->valid())
            continue;

        if (target.records.empty())
            target.records.emplace_back(target.last_record);

        select_record(target);
    }
}

bool aim::standing_in_fire()
{
    if (!g_ctx.local())
        return false;

    auto origin = g_ctx.local()->GetAbsOrigin();

    for (auto i = m_globals()->m_maxclients + 1; i <= m_entitylist()->GetHighestEntityIndex(); i++)
    {
        auto e = static_cast<entity_t*>(m_entitylist()->GetClientEntity(i));

        if (!e || e->IsDormant())
            continue;

        auto client_class = e->GetClientClass();

        if (!client_class || client_class->m_ClassID != CInferno)
            continue;

        auto renderable = e->GetClientRenderable();

        if (!renderable)
            continue;

        Vector mins, maxs;
        renderable->GetRenderBounds(mins, maxs);

        auto fire_origin = e->GetAbsOrigin();

        mins += fire_origin;
        maxs += fire_origin;

        if (origin.x < mins.x - 24.0f || origin.x > maxs.x + 24.0f)
            continue;

        if (origin.y < mins.y - 24.0f || origin.y > maxs.y + 24.0f)
            continue;

        if (origin.z < mins.z - 64.0f || origin.z > maxs.z + 64.0f)
            continue;

        return true;
    }

    return false;
}

bool aim::automatic_stop(CUserCmd* cmd)
{
    static auto last_stop_tick = INT_MIN;
    static auto withheld_ticks = 0;

    auto& weapon_config = cfg.ragebot.weapon[g_ctx.globals.current_weapon];

    auto release = [&]()
    {
        withheld_ticks = 0;
        return true;
    };

    if (!weapon_config.autostop)
        return release();

    if (should_stop)
        last_stop_tick = m_globals()->m_tickcount;

    if (!should_stop)
    {
        auto ticks_since_stop = m_globals()->m_tickcount - last_stop_tick;

        if (targets.empty() || ticks_since_stop < 0 || ticks_since_stop > 5)
            return release();
    }

    if (g_ctx.globals.slowwalking || misc::get().recharging_double_tap)
        return release();

    if (key_binds::get().get_key_bind_state(20))
        return release();

    if (g_ctx.globals.weapon->is_empty())
        return release();

    if (!weapon_config.autostop_modifiers[AUTOSTOP_BETWEEN_SHOTS] && !g_ctx.globals.weapon->can_fire(false))
        return release();

    auto animlayer = g_ctx.local()->get_animlayers()[1];

    if (animlayer.m_nSequence)
    {
        auto activity = g_ctx.local()->sequence_activity(animlayer.m_nSequence);

        if (activity == ACT_CSGO_RELOAD && animlayer.m_flWeight > 0.0f)
            return release();
    }

    auto weapon_info = g_ctx.globals.weapon->get_csweapon_info();

    if (!weapon_info)
        return release();

    auto index = g_ctx.globals.weapon->m_iItemDefinitionIndex();
    auto alt_accuracy = index == WEAPON_AWP || index == WEAPON_G3SG1 || index == WEAPON_SCAR20 || index == WEAPON_SSG08;

    if (!(g_ctx.local()->m_fFlags() & FL_ONGROUND && engineprediction::get().backup_data.flags & FL_ONGROUND))
    {
        if (!weapon_config.autostop_modifiers[AUTOSTOP_JUMP_SCOUT] || index != WEAPON_SSG08)
            return release();

        auto air_penalty = max(g_ctx.globals.inaccuracy, g_ctx.globals.weapon->get_inaccuracy()) - weapon_info->flInaccuracyStandAlt;

        if (air_penalty > weapon_info->flInaccuracyJumpAlt * 0.2f)
            return false;

        return release();
    }

    if (weapon_config.autostop_modifiers[AUTOSTOP_IGNORE_MOLOTOV] && standing_in_fire())
        return release();

    auto max_speed = g_ctx.globals.scoped ? weapon_info->flMaxPlayerSpeedAlt : weapon_info->flMaxPlayerSpeed;
    auto stop_speed = 0.24f * max_speed;
    auto velocity = engineprediction::get().backup_data.velocity.Length2D();

    if (weapon_config.autostop_modifiers[AUTOSTOP_DUCK])
        cmd->m_buttons |= IN_DUCK;

    if (weapon_config.autostop_modifiers[AUTOSTOP_FAKE_DUCK])
        g_ctx.globals.autostop_fakeduck = true;

    if (velocity <= stop_speed)
        slowwalk::get().autostop(cmd);
    else if (weapon_config.autostop_modifiers[AUTOSTOP_SLOW_MOTION])
        slowwalk::get().autostop(cmd, 0.22f);
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
    }

    if (velocity > stop_speed)
    {
        if (withheld_ticks < 32)
        {
            ++withheld_ticks;
            return false;
        }

        return release();
    }

    withheld_ticks = 0;

    if (weapon_config.autostop_modifiers[AUTOSTOP_FORCE_ACCURACY])
    {
        auto baseline = weapon_info->flInaccuracyStand;

        if (g_ctx.local()->m_fFlags() & FL_DUCKING)
            baseline = alt_accuracy ? weapon_info->flInaccuracyCrouchAlt : weapon_info->flInaccuracyCrouch;
        else if (alt_accuracy)
            baseline = weapon_info->flInaccuracyStandAlt;

        if (max(g_ctx.globals.inaccuracy, g_ctx.globals.weapon->get_inaccuracy()) > baseline * 1.15f)
            return false;
    }

    return true;
}

float aim::safe_point_margin(adjust_data* record, int hitbox, const Vector& shoot_position, const Vector& point)
{
    matrix3x4_t* candidates[5] =
    {
        record->matrixes_data.zero,
        record->matrixes_data.first,
        record->matrixes_data.second,
        record->matrixes_data.positive,
        record->matrixes_data.negative
    };

    Vector origin[5];
    auto count = 0;

    for (auto i = 0; i < 5; ++i)
    {
        auto position = record->player->hitbox_position_matrix(hitbox, candidates[i]);

        if (position.IsZero())
        {
            if (i < 3)
                return 0.0f;

            continue;
        }

        candidates[count] = candidates[i];
        origin[count] = position;

        ++count;
    }

    if (count < 3)
        return 0.0f;

    auto distinct = false;

    for (auto i = 0; i < count && !distinct; ++i)
    {
        for (auto j = i + 1; j < count; ++j)
        {
            if (origin[i].DistTo(origin[j]) > 0.05f)
            {
                distinct = true;
                break;
            }
        }
    }

    if (!distinct)
        return 0.0f;

    auto margin = 0.0f;

    for (auto i = 0; i < count; ++i)
    {
        auto distance = 0.0f;

        if (!hitbox_intersection(record->player, candidates[i], hitbox, shoot_position, point, &distance))
            return 0.0f;

        margin = max(margin, distance);
    }

    return max(margin, 0.01f);
}

float aim::point_clearance(adjust_data* record, const scan_point& point, const Vector& shoot_position, float spread, float damage_floor)
{
    auto distance = point.point.DistTo(shoot_position);

    if (distance < 1.0f)
        return 1.0f;

    auto radius = distance * std::tan(spread);

    if (radius < 1.0f)
        return 1.0f;

    auto forward = ZERO;
    auto right = ZERO;
    auto up = ZERO;

    math::angle_vectors(math::calculate_angle(shoot_position, point.point), &forward, &right, &up);

    math::fast_vec_normalize(right);
    math::fast_vec_normalize(up);

    const auto diagonal = radius * 0.70710678f;

    const Vector shift[8] =
    {
        right * radius,
        right * -radius,
        up * radius,
        up * -radius,
        right * diagonal + up * diagonal,
        right * diagonal + up * -diagonal,
        right * -diagonal + up * diagonal,
        right * -diagonal + up * -diagonal
    };

    auto extension = 1.0f + 24.0f / distance;
    auto passing = 0;

    for (auto& current : shift)
    {
        auto target = point.point + current;

        if (!hitbox_intersection(record->player, record->matrixes_data.main, point.hitbox, shoot_position, shoot_position + (target - shoot_position) * extension))
            continue;

        auto fire_data = autowall::get().wall_penetration(shoot_position, target, record->player, damage_floor);

        if (!fire_data.valid || fire_data.damage < 1)
            continue;

        ++passing;
    }

    return (float)passing * 0.125f;
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

    auto trust = record->bot ? 1.0f : math::clamp(record->resolver_confidence, 0.0f, 1.0f);
    auto unresolved = trust < 0.55f;

    auto safe_points_key = key_binds::get().get_key_bind_state(3);
    auto force_safe_points = safe_points_key || weapon_cfg.max_misses && g_ctx.globals.missed_shots[record->i] >= weapon_cfg.max_misses_amount;
    auto best_damage = 0;

    auto health = record->player->m_iHealth();

    auto visible_minimum_damage = get_minimum_damage(true, health);
    auto occluded_minimum_damage = get_minimum_damage(false, health);
    auto damage_floor = (float)min(visible_minimum_damage, occluded_minimum_damage);
    auto cone_spread = fmaxf(g_ctx.globals.spread + g_ctx.globals.inaccuracy, 0.0f);

	std::vector <scan_point> points;

	for (auto& hitbox : hitboxes)
	{
		auto current_points = get_points(record, hitbox, true);

		for (auto& point : current_points)
		{
			if (!hitbox_intersection(record->player, record->matrixes_data.main, hitbox, shoot_position, point.point))
				continue;

			if (!record->bot)
				point.safe = safe_point_margin(record, hitbox, shoot_position, point.point);
			else
				point.safe = 1.0f;

			if (!safe_points_key || point.safe)
				points.emplace_back(point);
		}
	}

	if (points.empty())
		return;

	std::stable_partition(points.begin(), points.end(), [](const scan_point& point)
		{
			return point.hitbox >= HITBOX_PELVIS && point.hitbox <= HITBOX_UPPER_CHEST;
		});

	if (unresolved || weapon_cfg.prefer_safe_points)
	{
		auto safe_available = false;

		for (auto& point : points)
		{
			if (point.safe > 0.0f)
			{
				safe_available = true;
				break;
			}
		}

		if (safe_available)
			points.erase(std::remove_if(points.begin(), points.end(), [](const scan_point& point)
				{
					return point.safe <= 0.0f;
				}), points.end());
	}

	auto body_hitboxes = true;
	float best_score = -1.0f;

	auto& body_aim_modifiers = weapon_cfg.body_aim_modifiers;

	auto force_body_aim = body_aim_modifiers[BAIM_PREFER]
		|| body_aim_modifiers[BAIM_AIR] && !(record->flags & FL_ONGROUND)
		|| body_aim_modifiers[BAIM_HIGH_VELOCITY] && record->velocity.Length2D() > 85.0f
		|| body_aim_modifiers[BAIM_DOUBLE_TAP] && g_ctx.globals.double_tap_aim
		|| body_aim_modifiers[BAIM_UNRESOLVED] && unresolved;

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

			if (lethal_body && best_damage >= health)
				break;

			if ((force_body_aim || weapon_cfg.prefer_body_aim) && best_damage >= 1)
				break;
		}

		auto fire_data = autowall::get().wall_penetration(shoot_position, point.point, record->player, damage_floor);

		if (!fire_data.valid)
			continue;

		if (fire_data.damage < 1)
			continue;

		auto current_minimum_damage = fire_data.visible ? visible_minimum_damage : occluded_minimum_damage;

		auto effective_damage = min(fire_data.damage, health);

		if (!fire_data.visible && effective_damage < health)
		{
			auto marginal = (float)fire_data.damage < (float)current_minimum_damage * 1.35f;

			if (marginal && !point.center)
				continue;
		}

		float current_score = static_cast<float>(effective_damage);

		if (effective_damage >= health)
			current_score += 40.0f;

		if (fire_data.visible)
			current_score += 25.0f;

		current_score += point.center ? 2.0f : 0.0f;
		current_score += point.safe ? (1.0f + (1.0f - trust) * 26.0f) : 0.0f;

		if (lethal_body && is_body && effective_damage >= health)
			current_score += 8.0f;

		if (cone_spread > 0.0f && current_score >= best_score)
		{
			auto clearance = point_clearance(record, point, shoot_position, cone_spread, damage_floor);

			if (clearance <= 0.0f)
				continue;

			current_score -= (1.0f - clearance) * 45.0f;
		}

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

    if (!cfg.ragebot.enable && (!cfg.antiaim.fakelag || cfg.antiaim.triggers_fakelag_amount <= 2))
        return;

    if (!g_ctx.local() || g_ctx.globals.current_weapon == -1)
        return;

    if (g_ctx.local()->m_vecVelocity().Length2D() > 5.0f)
        return;

    const auto velocity = engineprediction::get().backup_data.velocity;
    const auto chokes = max(m_clientstate()->iChokedCommands, 1);

    for (auto i = 1; i <= m_globals()->m_maxclients; i++)
    {
        auto e = static_cast<player_t*>(m_entitylist()->GetClientEntity(i));

        if (!e || !e->valid(true))
            continue;

        auto records = &player_records[i];

        if (records->empty())
            continue;

        auto record = &records->front();

        if (!record->valid())
            continue;

        record->adjust_player();

        auto head = e->hitbox_position_matrix(HITBOX_HEAD, record->matrixes_data.main);

        if (head.IsZero())
            continue;

        for (auto next_choke = 1; next_choke <= chokes; ++next_choke)
        {
            const auto predicted_eye_pos = g_ctx.globals.eye_pos + velocity * (m_globals()->m_intervalpertick * (float)next_choke);

            auto fire_data = autowall::get().wall_penetration(predicted_eye_pos, head, e);

            if (!fire_data.valid || fire_data.damage < 1)
                continue;

            g_ctx.globals.m_Peek.m_bIsPeeking = true;
            break;
        }

        if (g_ctx.globals.m_Peek.m_bIsPeeking)
            break;
    }
}

float aim::bodyscale(player_t* e)
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

    auto distance = e->m_vecOrigin().DistTo(g_ctx.globals.eye_pos);
    auto max_distance = weapon_range / 4.f;

    float factor = 0.f;
    if (distance < max_distance)
    {
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

        const auto local_center = (bbox->bbmin + bbox->bbmax) * 0.5f;

        Vector world_center;
        math::vector_transform(local_center, matrix, world_center);

        points.emplace_back(scan_point(world_center, hitbox, true));

        if (hitbox == HITBOX_RIGHT_FOOT || hitbox == HITBOX_LEFT_FOOT)
        {
            Vector local_points[2] =
            {
                Vector(local_center.x + (bbox->bbmin.x - local_center.x) * 0.75f, local_center.y, local_center.z),
                Vector(local_center.x + (bbox->bbmax.x - local_center.x) * 0.75f, local_center.y, local_center.z)
            };

            for (auto& local : local_points)
            {
                Vector world;
                math::vector_transform(local, matrix, world);
                points.emplace_back(scan_point(world, hitbox, false));
            }
        }

        return points;
    }

    auto scale = hitbox == HITBOX_HEAD ? GetHeadScale(record->player) : bodyscale(record->player);

    if (!cfg.ragebot.weapon[g_ctx.globals.current_weapon].static_point_scale)
    {
        const auto spread = fmaxf(g_ctx.globals.spread + g_ctx.globals.inaccuracy, 0.0f);
        const auto spread_radius = center.DistTo(g_ctx.globals.eye_pos) * std::tan(spread);
        const auto spread_scale = std::clamp((bbox->radius - spread_radius) / bbox->radius, 0.0f,
            hitbox == HITBOX_HEAD ? 0.80f : 0.75f);

        scale = min(scale, spread_scale);
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

    const auto lethal_first = first.data.damage >= first.health;
    const auto lethal_second = second.data.damage >= second.health;

    if (lethal_first != lethal_second)
        return lethal_first;

    switch (cfg.ragebot.weapon[g_ctx.globals.current_weapon].selection_type)
    {
    case 1:
        return first.fov < second.fov;
    case 2:
        return first.distance < second.distance;
    case 3:
        return first.health < second.health;
    }

    return first.data.damage > second.data.damage;
}

void aim::find_best_target()
{
    if (scanned_targets.empty())
        return;

    std::sort(scanned_targets.begin(), scanned_targets.end(), compare_targets);

    final_target = scanned_targets.front();
    final_target.record->adjust_player();
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

bool aim::fallback_point(scanned_target& candidate)
{
    static const int wide_hitboxes[] = { HITBOX_STOMACH, HITBOX_CHEST, HITBOX_LOWER_CHEST, HITBOX_PELVIS, HITBOX_UPPER_CHEST };

    auto record = candidate.record;

    if (!record || !record->player)
        return false;

    auto hitboxes = get_hitboxes(record);

    if (hitboxes.empty())
        return false;

    auto& weapon_cfg = cfg.ragebot.weapon[g_ctx.globals.current_weapon];

    auto force_safe_points = key_binds::get().get_key_bind_state(3)
        || weapon_cfg.max_misses && g_ctx.globals.missed_shots[record->i] >= weapon_cfg.max_misses_amount;

    auto health = record->player->m_iHealth();

    auto visible_minimum_damage = get_minimum_damage(true, health);
    auto occluded_minimum_damage = get_minimum_damage(false, health);
    auto damage_floor = (float)min(visible_minimum_damage, occluded_minimum_damage);

    for (auto hitbox : wide_hitboxes)
    {
        if (hitbox == candidate.data.point.hitbox)
            continue;

        if (std::find(hitboxes.begin(), hitboxes.end(), hitbox) == hitboxes.end())
            continue;

        auto position = record->player->hitbox_position_matrix(hitbox, record->matrixes_data.main);

        if (position.IsZero())
            continue;

        if (!hitbox_intersection(record->player, record->matrixes_data.main, hitbox, g_ctx.globals.eye_pos, position))
            continue;

        auto fire_data = autowall::get().wall_penetration(g_ctx.globals.eye_pos, position, record->player, damage_floor);

        if (!fire_data.valid || fire_data.damage < 1)
            continue;

        if (fire_data.damage < (fire_data.visible ? visible_minimum_damage : occluded_minimum_damage))
            continue;

        auto point = scan_point(position, hitbox, true);

        point.safe = record->bot ? 1.0f : safe_point_margin(record, hitbox, g_ctx.globals.eye_pos, position);

        if (force_safe_points && point.safe <= 0.0f)
            continue;

        candidate.data.point = point;
        candidate.data.visible = fire_data.visible;
        candidate.data.damage = fire_data.damage;
        candidate.data.hitbox = fire_data.hitbox;

        return true;
    }

    return false;
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

    auto hitchance_enabled = cfg.ragebot.weapon[g_ctx.globals.current_weapon].hitchance;
    auto hitchance_amount = cfg.ragebot.weapon[g_ctx.globals.current_weapon].hitchance_amount;

    auto aim_angle = math::calculate_angle(g_ctx.globals.eye_pos, final_target.data.point.point).Clamp();

    if (!cfg.ragebot.enable)
        m_engine()->SetViewAngles(aim_angle);

    if (!cfg.ragebot.enable && !(cmd->m_buttons & IN_ATTACK))
        return;

    auto solved = false;
    auto attempts = 0;

    for (auto& candidate : scanned_targets)
    {
        if (!candidate.data.valid() || !candidate.record || !candidate.record->player)
            continue;

        for (auto pass = 0; pass < 2 && attempts < 6; ++pass)
        {
            if (pass && !fallback_point(candidate))
                break;

            final_target = candidate;
            final_target.record->adjust_player();

            aim_angle = math::calculate_angle(g_ctx.globals.eye_pos, final_target.data.point.point).Clamp();

            if (!hitchance_enabled)
            {
                final_hitchance = 100;
                solved = true;
                break;
            }

            ++attempts;

            if (hitchance(aim_angle) && final_hitchance >= hitchance_amount)
            {
                solved = true;
                break;
            }
        }

        if (solved || attempts >= 6)
            break;
    }

    if (!solved)
    {
        auto is_zoomable_weapon = g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_SCAR20 || g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_G3SG1 || g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_SSG08 || g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_AWP || g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_AUG || g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_SG553;

        if (cfg.ragebot.autoscope && is_zoomable_weapon && !g_ctx.globals.weapon->m_zoomLevel())
            cmd->m_buttons |= IN_ATTACK2;

        return;
    }


    auto backtrack_ticks = 0;

    if (final_target.record->player)
        backtrack_ticks = g_ctx.globals.backtrack_ticks[final_target.record->player->EntIndex()];


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

    cmd->m_viewangles = aim_angle;
    cmd->m_buttons |= IN_ATTACK;

    cmd->m_tickcount = TIME_TO_TICKS(final_target.record->simulation_time) + TIME_TO_TICKS(util::get_interpolation());

    last_target_index = final_target.record->i;
    last_shoot_position = g_ctx.globals.eye_pos;
    last_target[last_target_index] = Last_target{
        *final_target.record, final_target.data, final_target.distance
    };

    if (g_ctx.shots.size() >= 256)
        g_ctx.shots.erase(g_ctx.shots.begin());

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
    shot->shot_info.fire_inaccuracy = g_ctx.globals.inaccuracy + g_ctx.globals.spread;
    shot->shot_info.was_visible_when_fired = final_target.data.visible;
    shot->shot_info.point_was_safe = final_target.data.point.safe > 0.0f;

    shot->shot_info.weapon_name = g_ctx.globals.weapon->get_name();

    shot->target_position_at_fire = final_target.record->origin;
	shot->shoot_position = g_ctx.globals.eye_pos;

    auto net_channel = m_engine()->GetNetChannelInfo();
    if (net_channel && !net_channel->IsLoopback() && !m_engine()->IsPlayingDemo())
    {
        auto latency = net_channel->GetLatency(FLOW_OUTGOING) + net_channel->GetLatency(FLOW_INCOMING);
        shot->shot_info.network_latency_ms = static_cast<int>(latency * 1000.0f);
    }
	else
		shot->shot_info.network_latency_ms = 0;

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

    if (g_ctx.globals.current_map == crypt_str("unknown"))
    {
        const char* map_name = m_engine()->GetLevelName();
        if (map_name)
        {
            std::string map_str(map_name);
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

struct spread_sample_t
{
    float inaccuracy_scale;
    float spread_scale;
    float inaccuracy_sin;
    float inaccuracy_cos;
    float spread_sin;
    float spread_cos;
};

static spread_sample_t spread_samples[256];
static auto spread_samples_ready = false;

void aim::build_seed_table()
{
    if (spread_samples_ready)
        return;

    spread_samples_ready = true;

    for (auto i = 0; i < 256; ++i)
    {
        math::random_seed(i + 1);

        const auto inaccuracy_scale = math::random_float(0.0f, 1.0f);
        const auto inaccuracy_angle = math::random_float(0.0f, DirectX::XM_2PI);
        const auto spread_scale = math::random_float(0.0f, 1.0f);
        const auto spread_angle = math::random_float(0.0f, DirectX::XM_2PI);

        auto inaccuracy_sin = 0.0f, inaccuracy_cos = 0.0f;
        DirectX::XMScalarSinCos(&inaccuracy_sin, &inaccuracy_cos, inaccuracy_angle);

        auto spread_sin = 0.0f, spread_cos = 0.0f;
        DirectX::XMScalarSinCos(&spread_sin, &spread_cos, spread_angle);

        spread_samples[i].inaccuracy_scale = inaccuracy_scale;
        spread_samples[i].spread_scale = spread_scale;
        spread_samples[i].inaccuracy_sin = inaccuracy_sin;
        spread_samples[i].inaccuracy_cos = inaccuracy_cos;
        spread_samples[i].spread_sin = spread_sin;
        spread_samples[i].spread_cos = spread_cos;
    }
}

int aim::hitchance(const Vector& aim_angle)
{
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

    auto real_inaccuracy = max(g_ctx.globals.inaccuracy, g_ctx.globals.weapon->get_inaccuracy());

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

    const auto minimum_damage = (float)min(get_minimum_damage(true, final_target.health), get_minimum_damage(false, final_target.health));

    if (g_ctx.globals.spread + g_ctx.globals.inaccuracy <= 0.0f && real_inaccuracy - 0.000001f < base_inaccuracy)
    {
        auto end = g_ctx.globals.eye_pos + forward * weapon_info->flRange;

        if (!hitbox_intersection(final_target.record->player, final_target.record->matrixes_data.main, final_target.data.hitbox, g_ctx.globals.eye_pos, end))
            return final_hitchance;

        auto fire_data = autowall::get().wall_penetration(g_ctx.globals.eye_pos, end, final_target.record->player, minimum_damage);

        if (!fire_data.valid || (float)fire_data.damage < minimum_damage)
            return final_hitchance;

        final_hitchance = 101;
        return final_hitchance;
    }

    build_seed_table();

    const auto configured = cfg.ragebot.weapon[g_ctx.globals.current_weapon].hitchance
        ? math::clamp(cfg.ragebot.weapon[g_ctx.globals.current_weapon].hitchance_amount, 1, 100) : 1;

    const auto needed = configured * 256 / 100;

    auto intersecting = 0;
    auto evaluated = 0;
    auto reaching = 0;
    auto budget = 48;
    auto sampled = 0;

    for (auto i = 0; i < 256; ++i)
    {
        if (256 - i + intersecting < needed)
            break;

        ++sampled;

        const auto& sample = spread_samples[i];

        auto inaccuracy = sample.inaccuracy_scale * g_ctx.globals.inaccuracy;
        auto spread = sample.spread_scale * g_ctx.globals.spread;

        auto spread_x = sample.inaccuracy_cos * inaccuracy + sample.spread_cos * spread;
        auto spread_y = sample.inaccuracy_sin * inaccuracy + sample.spread_sin * spread;

        auto direction = ZERO;

        direction.x = forward.x + right.x * spread_x + up.x * spread_y;
        direction.y = forward.y + right.y * spread_x + up.y * spread_y;
        direction.z = forward.z + right.z * spread_x + up.z * spread_y;

        auto end = g_ctx.globals.eye_pos + direction * weapon_info->flRange;

        if (!hitbox_intersection(final_target.record->player, final_target.record->matrixes_data.main, final_target.data.hitbox, g_ctx.globals.eye_pos, end))
            continue;

        ++intersecting;

        if (budget <= 0)
            continue;

        --budget;
        ++evaluated;

        auto fire_data = autowall::get().wall_penetration(g_ctx.globals.eye_pos, end, final_target.record->player, minimum_damage);

        if (fire_data.valid && (float)fire_data.damage >= minimum_damage)
            ++reaching;
    }

    if (evaluated > 0 && reaching == 0)
        return final_hitchance;

    const auto reach = evaluated > 0 ? (float)reaching / (float)evaluated : 1.0f;
    const auto bounded = intersecting + (256 - sampled);

    final_hitchance = (int)((float)bounded / 2.56f * reach);

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
