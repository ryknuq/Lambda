#include "knifebot.h"
#include "..\misc\misc.h"
#include "..\misc\logs.h"
#include "..\autowall\autowall.h"
#include "..\misc\prediction_system.h"
#include "..\fakewalk\slowwalk.h"
#include "..\lagcompensation\local_animations.h"
#include <random>

void knifebot::run(CUserCmd* cmd)
{
	final_target.reset();

	if (!cfg.ragebot.enable)
		return;

	if (!cfg.ragebot.knife_bot)
		return;

	if (!g_ctx.globals.weapon->is_knife())
		return;

	scan_targets();

	if (!final_target.record)
		return;

	fire(cmd);
}

void knifebot::scan_targets()
{
	if (aim::get().targets.empty())
		return;

	for (auto& target : aim::get().targets)
	{
		if (target.history_record && target.history_record->valid())
		{
			if (target.last_record && target.last_record->valid())
			{
				auto last_distance = g_ctx.globals.eye_pos.DistTo(target.last_record->origin);
				auto history_distance = g_ctx.globals.eye_pos.DistTo(target.history_record->origin);

				final_target.record = last_distance > history_distance ? target.history_record : target.last_record;
				final_target.record->adjust_player();
			}
			else
			{
				final_target.record = target.history_record;
				final_target.record->adjust_player();
			}
		}
		else
		{
			if (!target.last_record || !target.last_record->valid())
				continue;

			final_target.record = target.last_record;
			final_target.record->adjust_player();
		}
	}
}

void knifebot::fire(CUserCmd* cmd)
{
	if (!g_ctx.globals.weapon->can_fire(false))
		return;

	auto vecOrigin = final_target.record->player->m_vecOrigin();

	auto vecOBBMins = final_target.record->player->GetCollideable()->OBBMins();
	auto vecOBBMaxs = final_target.record->player->GetCollideable()->OBBMaxs();

	auto vecMins = vecOBBMins + vecOrigin;
	auto vecMaxs = vecOBBMaxs + vecOrigin;

	auto vecEyePos = final_target.record->player->get_shoot_position();

	auto closest = Vector(
		math::clamp(vecEyePos.x, vecMins.x, vecMaxs.x),
		math::clamp(vecEyePos.y, vecMins.y, vecMaxs.y),
		math::clamp(g_ctx.globals.eye_pos.z, vecMins.z, vecMaxs.z));

	auto vecDelta = closest - g_ctx.globals.eye_pos;

	if (vecDelta.Length() > 60.0f)
		return;

	vecDelta.Normalize();
	auto delta = fabs(math::normalize_yaw(final_target.record->angles.y - math::calculate_angle(final_target.record->player->get_shoot_position(), g_ctx.local()->GetAbsOrigin()).y));

	auto stab = final_target.record->player->m_iHealth() > 46 && delta < 120.0f && determinate_hit_type(1, vecDelta);

	if (!stab && !determinate_hit_type(0, vecDelta))
		return;

	cmd->m_viewangles = vecDelta.ToEulerAngles();
	cmd->m_buttons |= stab ? IN_ATTACK2 : IN_ATTACK;
	cmd->m_tickcount = TIME_TO_TICKS(final_target.record->simulation_time) + TIME_TO_TICKS(util::get_interpolation());
}

int knifebot::determinate_hit_type(bool stab_type, const Vector& delta)
{
	auto minimum_distance = stab_type ? 32.0f : 48.0f;
	auto end = g_ctx.globals.eye_pos + delta * minimum_distance;

	CTraceFilter filter;
	filter.pSkip = g_ctx.local();

	trace_t trace;
	Ray_t ray;

	ray.Init(g_ctx.globals.eye_pos, end, Vector(-16.0f, -16.0f, -18.0f), Vector(16.0f, 16.0f, 18.0f));
	m_trace()->TraceRay(ray, 0x200400B, &filter, &trace);

	if (trace.hit_entity != final_target.record->player)
		return 0;

	auto cos_pitch = cos(DEG2RAD(final_target.record->angles.x));

	auto sin_yaw = 0.0f;
	auto cos_yaw = 0.0f;

	DirectX::XMScalarSinCos(&sin_yaw, &cos_yaw, DEG2RAD(final_target.record->angles.y));

	auto final_delta = final_target.record->origin - g_ctx.globals.eye_pos;
	return (int)(cos_yaw * cos_pitch * final_delta.x + sin_yaw * cos_pitch * final_delta.y >= 0.475f) + 1;
}