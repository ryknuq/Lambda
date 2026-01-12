#pragma once

#include "..\..\includes.hpp"
#include "..\..\sdk\structs.hpp"
struct m_indicator
{
	std::string m_text;
	std::string gamesense;
	Color m_color;

	m_indicator(const char* text, Color color) :
		m_text(text), m_color(color)
	{

	}
	m_indicator(std::string text, Color color) :
		m_text(text), m_color(color)
	{

	}

};
class otheresp : public singleton< otheresp >
{
	float radius = 23.f;
	float rotation_step = 0.08f;
	float current_rotation = 0.0f;
	Vector current_peek_position = Vector(0, 0, 0);
public:
	void penetration_reticle();
	void damage_marker_paint();
	void hitmarker_paint();
	void indicators();
	void draw_indicators();
	void nibm(player_t* WhoUseThisBone = g_ctx.local(), int hitbox_id = HITBOX_HEAD);
	void holopanel(player_t* WhoUseThisBone = g_ctx.local(), int hitbox_id = HITBOX_RIGHT_FOREARM, bool autodir = true);
	void spread_crosshair(LPDIRECT3DDEVICE9 device);
	void automatic_peek_indicator();

	void peek_indicator();

	struct Hitmarker
	{
		float hurt_time = FLT_MIN;
		Color hurt_color = Color::White;
		Vector point = ZERO;
	} hitmarker;

	struct Damage_marker
	{
		Vector position = ZERO;
		float hurt_time = FLT_MIN;
		Color hurt_color = Color::White;
		int damage = -1;
		int hitgroup = -1;

		void reset()
		{
			position.Zero();
			hurt_time = FLT_MIN;
			hurt_color = Color::White;
			damage = -1;
			hitgroup = -1;
		}
	} damage_marker[65];
	std::vector<m_indicator> m_indicators;

	vgui::HFont IndShadow;
};