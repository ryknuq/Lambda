#include "otheresp.h"
#include "..\autowall\autowall.h"
#include "..\ragebot\antiaim.h"
#include "..\misc\logs.h"
#include "..\misc\misc.h"
#include "..\lagcompensation\local_animations.h"
#include "../../menu/UI.h"
#include "../../utils/Render/Render.h"

bool can_penetrate(weapon_t* weapon)
{
	auto weapon_info = weapon->get_csweapon_info();

	if (!weapon_info)
		return false;

	Vector view_angles;
	m_engine()->GetViewAngles(view_angles);

	Vector direction;
	math::angle_vectors(view_angles, direction);

	CTraceFilter filter;
	filter.pSkip = g_ctx.local();

	trace_t trace;
	util::trace_line(g_ctx.globals.eye_pos, g_ctx.globals.eye_pos + direction * weapon_info->flRange, MASK_SHOT_HULL | CONTENTS_HITBOX, &filter, &trace);

	if (trace.fraction == 1.0f)
		return false;

	auto eye_pos = g_ctx.globals.eye_pos;
	auto hits = 1;
	auto damage = (float)weapon_info->iDamage;
	auto penetration_power = weapon_info->flPenetration;

	static auto damageReductionBullets = m_cvar()->FindVar(crypt_str("ff_damage_reduction_bullets"));
	static auto damageBulletPenetration = m_cvar()->FindVar(crypt_str("ff_damage_bullet_penetration"));

	return autowall::get().handle_bullet_penetration(weapon_info, trace, eye_pos, direction, hits, damage, penetration_power, damageReductionBullets->GetFloat(), damageBulletPenetration->GetFloat(), trace.hit_entity);
}

void otheresp::penetration_reticle()
{
	if (!cfg.player.enable)
		return;

	if (!cfg.esp.penetration_reticle)
		return;

	if (!g_ctx.local()->is_alive())
		return;

	auto weapon = g_ctx.local()->m_hActiveWeapon().Get();

	if (!weapon)
		return;

	auto color = Color::Red;

	if (!weapon->is_non_aim() && weapon->m_iItemDefinitionIndex() != WEAPON_TASER && can_penetrate(weapon))
		color = Color::Green;

	static int width, height;
	m_engine()->GetScreenSize(width, height);

	trace_t enterTrace;
	CTraceFilter filter;
	Ray_t ray;
	auto weapon_info = weapon->get_csweapon_info();
	if (!weapon_info)
		return;
	Vector viewangles; m_engine()->GetViewAngles(viewangles);
	Vector direction; math::angle_vectors(viewangles, direction);
	Vector start = g_ctx.globals.eye_pos;
	auto m_flMaxRange = weapon_info->flRange * 2;
	Vector end = start + (direction * m_flMaxRange);

	filter.pSkip = g_ctx.local();
	ray.Init(start, end);
	m_trace()->TraceRay(ray, MASK_SHOT | CONTENTS_GRATE, &filter, &enterTrace);

	float anglez = math::dot_product(Vector(0, 0, 1), enterTrace.plane.normal);
	float invanglez = math::dot_product(Vector(0, 0, -1), enterTrace.plane.normal);
	float angley = math::dot_product(Vector(0, 1, 0), enterTrace.plane.normal);
	float invangley = math::dot_product(Vector(0, -1, 0), enterTrace.plane.normal);
	float anglex = math::dot_product(Vector(1, 0, 0), enterTrace.plane.normal);
	float invanglex = math::dot_product(Vector(-1, 0, 0), enterTrace.plane.normal);

	if (anglez > 0.5 || invanglez > 0.5)
		g_Render->filled_rect_world(enterTrace.endpos, Vector2D(3, 3), Color(color.r(), color.g(), color.b(), 100), 0, int(can_penetrate(weapon)));
	else if (angley > 0.5 || invangley > 0.5)
		g_Render->filled_rect_world(enterTrace.endpos, Vector2D(3, 3), Color(color.r(), color.g(), color.b(), 100), 1, int(can_penetrate(weapon)));
	else if (anglex > 0.5 || invanglex > 0.5)
		g_Render->filled_rect_world(enterTrace.endpos, Vector2D(3, 3), Color(color.r(), color.g(), color.b(), 100), 2, int(can_penetrate(weapon)));
	Vector pos2d;
}

void otheresp::damage_marker_paint()
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

			g_Render->DrawString(screen.x, screen.y -= 0.5 * (127.5 - alpha), damage_marker[i].hurt_color, render2::centered_x | render2::centered_y | render2::outline, c_menu::get().g_cxm, "- %i", damage_marker[i].damage);
		}
	}
}

void otheresp::hitmarker_paint()
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

void otheresp::indicators()
{
	if (!g_ctx.local()->is_alive())
		return;

	auto weapon = g_ctx.local()->m_hActiveWeapon().Get();

	if (!weapon)
		return;

	if (cfg.esp.indicators[INDICATOR_FAKE] && (antiaim::get().type == ANTIAIM_LEGIT || cfg.antiaim.type[antiaim::get().type].desync))
	{
		auto color = Color(130, 20, 20);
		auto animstate = g_ctx.local()->get_animation_state();

		if (animstate && local_animations::get().local_data.animstate)
		{
			auto delta = fabs(math::normalize_yaw(animstate->m_flGoalFeetYaw - local_animations::get().local_data.animstate->m_flGoalFeetYaw));
			auto desync_delta = max(g_ctx.local()->get_max_desync_delta(), 58.0f);

			color = Color(130, 20 + (int)(min(delta / desync_delta, 1.0f) * 150.0f), 20);
		}


		//m_indicators.push_back(m_indicator("FAKE", color));
	}

	auto choke_indicator = false;

	if (cfg.esp.indicators[INDICATOR_CHOKE] && !fakelag::get().condition && !misc::get().double_tap_enabled && !misc::get().hide_shots_enabled)
	{
		m_indicators.push_back(m_indicator("FAKELAG", Color(230, 230, 230)));
		choke_indicator = true;
	}

	if (cfg.esp.indicators[INDICATOR_DAMAGE] && g_ctx.globals.current_weapon != -1 && key_binds::get().get_key_bind_state(4 + g_ctx.globals.current_weapon) && !weapon->is_non_aim())
	{
		if (cfg.ragebot.weapon[g_ctx.globals.current_weapon].minimum_override_damage > 100)
			m_indicators.push_back(m_indicator(("" + std::to_string(cfg.ragebot.weapon[g_ctx.globals.current_weapon].minimum_override_damage - 100)), Color(255, 255, 255)));
		else
			m_indicators.push_back(m_indicator(("" + std::to_string(cfg.ragebot.weapon[g_ctx.globals.current_weapon].minimum_override_damage)), Color(255, 255, 255)));
	}

	if (key_binds::get().get_key_bind_state(18))
		m_indicators.push_back(m_indicator("PEEK", Color(230, 230, 230)));

	if (cfg.ragebot.roll)
		m_indicators.push_back(m_indicator("ROLL FIX", Color(230, 230, 230)));

	if (cfg.esp.indicators[INDICATOR_SAFE_POINTS] && key_binds::get().get_key_bind_state(3) && !weapon->is_non_aim())
		m_indicators.push_back(m_indicator("SAFE", Color(20, 255, 20)));

	if (cfg.esp.indicators[INDICATOR_SAFE_POINTS] && key_binds::get().get_key_bind_state(23) && !weapon->is_non_aim())
		m_indicators.push_back(m_indicator("EDGE BUG", Color(20, 20, 255)));

	if (cfg.esp.indicators[INDICATOR_SAFE_POINTS] && key_binds::get().get_key_bind_state(13) && !weapon->is_non_aim())
		m_indicators.push_back(m_indicator("EDGE JUMP", Color(20, 20, 255)));

	if (cfg.esp.indicators[INDICATOR_SAFE_POINTS] && key_binds::get().get_key_bind_state(24) && !weapon->is_non_aim())
		m_indicators.push_back(m_indicator("JUMP BUG", Color(20, 20, 255)));

	if (cfg.esp.indicators[INDICATOR_BODY_AIM] && key_binds::get().get_key_bind_state(22) && !weapon->is_non_aim())
		m_indicators.push_back(m_indicator("BAIM", Color(255, 0, 0)));

	if (cfg.esp.indicators[INDICATOR_DESYNC_SIDE] && (antiaim::get().type == ANTIAIM_LEGIT && cfg.antiaim.desync == 1 || antiaim::get().type != ANTIAIM_LEGIT && cfg.antiaim.type[antiaim::get().type].desync == 1) && !antiaim::get().condition(g_ctx.get_command()))
	{
		auto side = antiaim::get().desync_angle > 0.0f ? "AA RIGHT" : "AA LEFT";

		if (antiaim::get().type == ANTIAIM_LEGIT)
			side = antiaim::get().desync_angle > 0.0f ? "AA LEFT" : "AA RIGHT";

		m_indicators.push_back(m_indicator(side, Color(255, 255, 255)));
	}

	if (cfg.esp.indicators[INDICATOR_DT] && cfg.ragebot.double_tap && cfg.ragebot.double_tap_key.key > KEY_NONE && cfg.ragebot.double_tap_key.key < KEY_MAX && misc::get().double_tap_key)
		m_indicators.push_back(m_indicator("DT", !g_ctx.local()->m_bGunGameImmunity() && !(g_ctx.local()->m_fFlags() & FL_FROZEN) && !antiaim::get().freeze_check && misc::get().double_tap_enabled && !weapon->is_grenade() && weapon->m_iItemDefinitionIndex() != WEAPON_TASER && weapon->m_iItemDefinitionIndex() != WEAPON_REVOLVER && weapon->can_fire(false) ? Color(230, 230, 230) : Color(130, 20, 20)));

	if (cfg.esp.indicators[INDICATOR_HS] && misc::get().hide_shots_key)
		m_indicators.push_back(m_indicator("ON-SHOT", !g_ctx.local()->m_bGunGameImmunity() && !(g_ctx.local()->m_fFlags() & FL_FROZEN) && !antiaim::get().freeze_check && misc::get().hide_shots_enabled ? Color(230, 230, 230) : Color(130, 20, 20)));
}

void otheresp::draw_indicators()
{
	if (!g_ctx.local()->is_alive())
		return;

	static int width, height;
	m_engine()->GetScreenSize(width, height);

	auto h = height / 2 + 100;

	for (auto& indicator : m_indicators)
	{
		render::get().gradient(5, h - 15, 30, 30, Color(0, 0, 0, 220), Color(0, 0, 0, 200), GRADIENT_HORIZONTAL);
		render::get().gradient(35, h - 15, 30, 30, Color(0, 0, 0, 200), Color(0, 0, 0, 0), GRADIENT_HORIZONTAL);
		render::get().text(fonts[SEMXXZ], 10, h, indicator.m_color, HFONT_CENTERED_Y, indicator.m_text.c_str());
		h -= 35;
	}

	m_indicators.clear();
}

void draw_circe(float x, float y, float radius, int resolution, DWORD color, DWORD color2, LPDIRECT3DDEVICE9 device);

void otheresp::spread_crosshair(LPDIRECT3DDEVICE9 device)
{
	if (!cfg.player.enable)
		return;

	if (!cfg.esp.show_spread)
		return;

	if (!g_ctx.local()->is_alive())
		return;

	auto weapon = g_ctx.local()->m_hActiveWeapon().Get();

	if (weapon->is_non_aim())
		return;

	int w, h;
	m_engine()->GetScreenSize(w, h);

	draw_circe((float)w * 0.5f, (float)h * 0.5f, g_ctx.globals.inaccuracy * 500.0f, 50, D3DCOLOR_RGBA(cfg.esp.show_spread_color.r(), cfg.esp.show_spread_color.g(), cfg.esp.show_spread_color.b(), cfg.esp.show_spread_color.a()), D3DCOLOR_RGBA(0, 0, 0, 0), device);
}

void draw_circe(float x, float y, float radius, int resolution, DWORD color, DWORD color2, LPDIRECT3DDEVICE9 device)
{
	LPDIRECT3DVERTEXBUFFER9 g_pVB2 = nullptr;
	std::vector <CUSTOMVERTEX2> circle(resolution + 2);

	circle[0].x = x;
	circle[0].y = y;
	circle[0].z = 0.0f;

	circle[0].rhw = 1.0f;
	circle[0].color = color2;

	for (auto i = 1; i < resolution + 2; i++)
	{
		circle[i].x = (float)(x - radius * cos(D3DX_PI * ((i - 1) / (resolution / 2.0f))));
		circle[i].y = (float)(y - radius * sin(D3DX_PI * ((i - 1) / (resolution / 2.0f))));
		circle[i].z = 0.0f;

		circle[i].rhw = 1.0f;
		circle[i].color = color;
	}

	device->CreateVertexBuffer((resolution + 2) * sizeof(CUSTOMVERTEX2), D3DUSAGE_WRITEONLY, D3DFVF_XYZRHW | D3DFVF_DIFFUSE, D3DPOOL_DEFAULT, &g_pVB2, nullptr);

	if (!g_pVB2)
		return;

	void* pVertices;

	g_pVB2->Lock(0, (resolution + 2) * sizeof(CUSTOMVERTEX2), (void**)&pVertices, 0);
	memcpy(pVertices, &circle[0], (resolution + 2) * sizeof(CUSTOMVERTEX2));
	g_pVB2->Unlock();

	device->SetTexture(0, nullptr);
	device->SetPixelShader(nullptr);
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

	device->SetStreamSource(0, g_pVB2, 0, sizeof(CUSTOMVERTEX2));
	device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
	device->DrawPrimitive(D3DPT_TRIANGLEFAN, 0, resolution);

	g_pVB2->Release();
}

void otheresp::peek_indicator()
{
	auto weapon = g_ctx.local()->m_hActiveWeapon().Get();

	for (int i = 1; i <= m_entitylist()->GetHighestEntityIndex(); i++) {
		auto e = static_cast<entity_t*>(m_entitylist()->GetClientEntity(i));

		if (!e)
			continue;

		if (e->is_player())
			continue;

		if (e->IsDormant())
			continue;

		auto client_class = e->GetClientClass();

		if (!client_class)
			continue;

		if (client_class->m_ClassID != CSmokeGrenadeProjectile)
			continue;

	}

	if (!weapon)
		return;

	static auto position = ZERO;

	if (!g_ctx.globals.start_position.IsZero())
		position = g_ctx.globals.start_position;

	if (position.IsZero())
		return;

	static auto alpha = 0.0f;

	if (!weapon->is_non_aim() && key_binds::get().get_key_bind_state(18) || alpha)
	{
		if (!weapon->is_non_aim() && key_binds::get().get_key_bind_state(18))
			alpha += 3.0f * m_globals()->m_frametime;
		else
			alpha -= 3.0f * m_globals()->m_frametime;

		alpha = math::clamp(alpha, 0.0f, 1.0f);
		float rad = max(2, 24 * alpha) - 1.f;
		for (int i = 1; i < max(2, 24 * alpha); i++) {
			if (rad > 2.f)
				g_Render->DrawRing3D1(position.x, position.y, position.z, round(rad), 180, Color(0, 00, 0, 0), Color(cfg.misc.automatic_peek_color.r(), cfg.misc.automatic_peek_color.g(), cfg.misc.automatic_peek_color.b(), int(i * alpha)), 0, 0.f);

			rad -= 1.f;
		}
	}
}