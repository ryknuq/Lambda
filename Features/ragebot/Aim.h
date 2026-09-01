#pragma once
#include "..\..\includes.hpp"
#include "..\lagcompensation\animation_system.h"

class target
{
public:
	player_t* e;

	adjust_data* last_record;
	adjust_data* history_record;

	std::vector <adjust_data*> records;

	target()
	{
		e = nullptr;

		last_record = nullptr;
		history_record = nullptr;
	}

	target(player_t* e, adjust_data* last_record, adjust_data* history_record)
	{
		this->e = e;

		this->last_record = last_record;
		this->history_record = history_record;
	}
};

class scan_point
{
public:
	Vector point;
	int hitbox;
	bool center;
	float safe;

	scan_point()
	{
		point.Zero();
		hitbox = -1;
		center = false;
		safe = 0.0f;
	}

	scan_point(const Vector& point, const int& hitbox, const bool& center)
	{
		this->point = point;
		this->hitbox = hitbox;
		this->center = center;
		this->safe = 0.0f;
	}

	void reset()
	{
		point.Zero();
		hitbox = -1;
		center = false;
		safe = 0.0f;
	}
};

class scan_data
{
public:
	scan_point point;
	bool visible;
	int damage;
	int hitbox;

	scan_data()
	{
		reset();
	}

	void reset()
	{
		point.reset();
		visible = false;
		damage = -1;
		hitbox = -1;
	}

	bool valid()
	{
		return damage >= 1 && hitbox != -1;
	}
};

struct Last_target
{
	adjust_data record;
	scan_data data;
	float distance;
};

class scanned_target
{
public:
	adjust_data* record;
	scan_data data;

	float fov;
	float distance;
	int health;

	scanned_target()
	{
		reset();
	}

	scanned_target(const scanned_target& data) //-V688
	{
		this->record = data.record;
		this->data = data.data;
		this->fov = data.fov;
		this->distance = data.distance;
		this->health = data.health;
	}

	scanned_target& operator=(const scanned_target& data) //-V688
	{
		this->record = data.record;
		this->data = data.data;
		this->fov = data.fov;
		this->distance = data.distance;
		this->health = data.health;

		return *this;
	}

	scanned_target(adjust_data* record, const scan_data& data) //-V688 //-V818
	{
		this->record = record;
		this->data = data;

		Vector viewangles;
		m_engine()->GetViewAngles(viewangles);

		auto aim_angle = math::calculate_angle(g_ctx.globals.eye_pos, data.point.point); //-V688
		auto fov = math::get_fov(viewangles, aim_angle); //-V688

		this->fov = fov;
		this->distance = g_ctx.globals.eye_pos.DistTo(data.point.point);
		this->health = record->player->m_iHealth();
	}

	void reset()
	{
		record = nullptr;
		data.reset();

		fov = 0.0f;
		distance = 0.0f;
		health = 0;
	}
};

class aim : public singleton <aim>
{
	void automatic_revolver(CUserCmd* cmd);
	void prepare_targets();
	adjust_data* get_record(std::deque <adjust_data>* records, bool history);
	void collect_records(target& current);
	float probe_record(adjust_data* record);
	float rate_record(adjust_data* record, scan_data& data, float newest_time);
	void select_record(target& current);
	int get_minimum_damage(bool visible, int health);
	void scan_targets();
	bool automatic_stop(CUserCmd* cmd);
	bool standing_in_fire();
	void find_best_target();
	void automatic_scope(CUserCmd* cmd);
	void fire(CUserCmd* cmd);
	void build_seed_table();
	int hitchance(const Vector& aim_angle);
	float final_hitchance;
	std::vector <scanned_target> scanned_targets;
	scanned_target final_target;
public:
	int backtrack_window();
	void run(CUserCmd* cmd);
	float bodyscale(player_t* e);
	float GetHeadScale(player_t* e);
	void scan(adjust_data* record, scan_data& data, const Vector& shoot_position = g_ctx.globals.eye_pos);
	std::vector<int> get_hitboxes(adjust_data* record);
	void update_peek_state();
	std::vector <scan_point> get_points(adjust_data* record, int hitbox, bool from_aim = true);
	bool hitbox_intersection(player_t* e, matrix3x4_t* matrix, int hitbox, const Vector& start, const Vector& end, float* safe = nullptr);
	float safe_point_margin(adjust_data* record, int hitbox, const Vector& shoot_position, const Vector& point);
	float point_clearance(adjust_data* record, const scan_point& point, const Vector& shoot_position, float spread, float damage_floor);
	bool fallback_point(scanned_target& candidate);

	std::vector <target> targets;
	std::vector <adjust_data> backup;

	int last_target_index = -1;
	Last_target last_target[65];

	bool fire_data_saved;

	float weapon_range;

	bool r8_fire = false;

	Vector last_shoot_position;
	bool should_stop;
};
