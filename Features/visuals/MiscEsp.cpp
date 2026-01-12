#include "MiscEsp.h"
#include "../../menu/UI.h"
#include "../../utils/Render/Render.h"

void miscesp::damage_marker_paint()
{
	for (auto i = 1; i < m_globals()->m_maxclients; i++) //-V807
	{
		if (damage_marker[i].hurt_time + 2.0f > m_globals()->m_curtime)
		{
			Vector screen;

			if (!math::world_to_screen(damage_marker[i].position, screen))
				continue;

			auto alpha = (int)((damage_marker[i].hurt_time + 2.0f - m_globals()->m_curtime) * 127.5f);
			damage_marker[i].hurt_color.SetAlpha(alpha);

			g_Render->DrawString(screen.x, screen.y -= 0.5 * (127.5 - alpha), damage_marker[i].hurt_color, render2::centered_x | render2::centered_y | render2::outline, c_menu::get().OpenSansBold, "- %i", damage_marker[i].damage);
		}
	}
}

void miscesp::nibm(player_t* WhoUseThisBone, int hitbox_id) 
{
	if (cfg.esp.nimb)
		return;

	auto bone_pos = WhoUseThisBone->hitbox_position(hitbox_id);
	Vector angle;
	if (key_binds::get().get_key_bind_state(17))
	{
		if (math::world_to_screen(bone_pos, angle))
		{
			bone_pos.z = bone_pos.z + 10;
			render::get().Draw3DCircle(bone_pos, 5, Color(255, 255, 255, 255));

		}
	}
}

void miscesp::hitmarker_paint()
{
	float m_hit_start, m_hit_end, m_hit_duration;

	static auto nl = Color(230, 230, 230);

	if (!cfg.esp.hitmarker[0] && !cfg.esp.hitmarker[1])
	{
		hitmarker.hurt_time = FLT_MIN;
		hitmarker.point = ZERO;
		return;
	}

	if (!g_ctx.local()->is_alive())
	{
		hitmarker.hurt_time = FLT_MIN;
		hitmarker.point = ZERO;
		return;
	}

	if (hitmarker.hurt_time + 0.7f > m_globals()->m_curtime)
	{
		if (cfg.esp.hitmarker[0])
		{
			static int width, height;
			m_engine()->GetScreenSize(width, height);
			auto alpha = (int)((hitmarker.hurt_time + 1.0f - m_globals()->m_curtime) * 255.0f);
		    nl.SetAlpha(alpha);

			static auto cross = m_cvar()->FindVar("weapon_debug_spread_show");

			float complete = 4;
			int x = width / 2, y = height / 2, alpha2 = (1.f - complete) * 240;

			constexpr int line{ 6 };

			g_Render->DrawLine(x - line, y - line, x - (line / 4), y - (line / 4), { 200, 200, 200, alpha2 });
			g_Render->DrawLine(x - line, y + line, x - (line / 4), y + (line / 4), { 200, 200, 200, alpha2 });
			g_Render->DrawLine(x + line, y + line, x + (line / 4), y + (line / 4), { 200, 200, 200, alpha2 });
			g_Render->DrawLine(x + line, y - line, x + (line / 4), y - (line / 4), { 200, 200, 200, alpha2 });
		}

		if (cfg.esp.hitmarker[1])
		{
			Vector world;

			if (math::world_to_screen(hitmarker.point, world))
			{
				auto alpha = (int)((hitmarker.hurt_time + 1.0f - m_globals()->m_curtime) * 255.0f);
				nl.SetAlpha(alpha);

				static auto cross = m_cvar()->FindVar("weapon_debug_spread_show");
				//cross->SetValue(cfg.esp.hitmarker_test && !g_ctx.local()->m_bIsScoped());

				float complete = 4;
				int x = world.x / 2, y = world.y / 2, alpha2 = (1.f - complete) * 240;

				constexpr int line{ 6 };

				g_Render->DrawLine(x - line, y - line, x - (line / 4), y - (line / 4), { 200, 200, 200, alpha2 });
				g_Render->DrawLine(x - line, y + line, x - (line / 4), y + (line / 4), { 200, 200, 200, alpha2 });
				g_Render->DrawLine(x + line, y + line, x + (line / 4), y + (line / 4), { 200, 200, 200, alpha2 });
				g_Render->DrawLine(x + line, y - line, x + (line / 4), y - (line / 4), { 200, 200, 200, alpha2 });
			}
		}
	}
}