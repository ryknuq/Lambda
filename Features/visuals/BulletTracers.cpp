#include "BulletTracers.h"
#include "..\..\sdk\misc\BeamInfo_t.hpp"
#include "..\ragebot\aim.h"
#include "..\..\..\utils\ctx\ctx.hpp"
#include "..\misc\logs.h"

void bullettracers::events(IGameEvent* event)
{
	auto event_name = event->GetName();

	if (!strcmp(event_name, crypt_str("bullet_impact")))
	{
		auto user_id = event->GetInt(crypt_str("userid"));
		auto user = m_engine()->GetPlayerForUserID(user_id);

		auto e = static_cast<player_t*>(m_entitylist()->GetClientEntity(user));

		if (e->valid(false))
		{
			if (e == g_ctx.local())
			{
				auto new_record = true;
				Vector position(event->GetFloat(crypt_str("x")), event->GetFloat(crypt_str("y")), event->GetFloat(crypt_str("z")));

				for (auto& current : impacts)
				{
					if (e == current.e)
					{
						new_record = false;

						current.impact_position = position;
						current.time = m_globals()->m_curtime;
					}
				}

				if (new_record)
					impacts.push_back(
						impact_data
						{
							e,
							position,
							m_globals()->m_curtime
						});
			}
			else if (e->m_iTeamNum() != g_ctx.local()->m_iTeamNum())
			{
				auto new_record = true;
				Vector position(event->GetFloat(crypt_str("x")), event->GetFloat(crypt_str("y")), event->GetFloat(crypt_str("z")));

				for (auto& current : impacts)
				{
					if (e == current.e)
					{
						new_record = false;

						current.impact_position = position;
						current.time = m_globals()->m_curtime;
					}
				}

				if (new_record)
					impacts.push_back(
						impact_data
						{
							e,
							position,
							m_globals()->m_curtime
						});
			}
		}
	}
}

void bullettracers::draw_beams()
{
	if (impacts.empty())
		return;

	while (!impacts.empty())
	{
		if (impacts.begin()->impact_position.IsZero())
		{
			impacts.erase(impacts.begin());
			continue;
		}

		if (fabs(m_globals()->m_curtime - impacts.begin()->time) > 4.0f)
		{
			impacts.erase(impacts.begin());
			continue;
		}

		if (!impacts.begin()->e->valid(false))
		{
			impacts.erase(impacts.begin());
			continue;
		}

		if (TIME_TO_TICKS(m_globals()->m_curtime) > TIME_TO_TICKS(impacts.begin()->time))
		{
			auto color = cfg.esp.enemy_bullet_tracer_color;

			if (impacts.begin()->e == g_ctx.local())
			{
				if (!cfg.esp.bullet_tracer)
				{
					impacts.erase(impacts.begin());
					continue;
				}

				color = cfg.esp.bullet_tracer_color;
			}
			else if (!cfg.esp.enemy_bullet_tracer)
			{
				impacts.erase(impacts.begin());
				continue;
			}

			m_debugoverlay()->AddLineOverlayAlpha(impacts.begin()->e == g_ctx.local() ? aim::get().last_shoot_position : impacts.begin()->e->get_shoot_position(), impacts.begin()->impact_position, (float)color.r(), (float)color.g(), (float)color.b(), 255, false, 4);
			impacts.erase(impacts.begin());
			continue;
		}

		break;
	}
}