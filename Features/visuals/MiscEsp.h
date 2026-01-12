#pragma once

#include "..\..\includes.hpp"
#include "..\..\sdk\structs.hpp"


class miscesp : public singleton< miscesp >
{
public:
	void damage_marker_paint();
	void nibm(player_t* WhoUseThisBone = g_ctx.local(), int hitbox_id = HITBOX_HEAD);
	void hitmarker_paint();

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
};

