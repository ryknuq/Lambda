#pragma once

#include "..\..\includes.hpp"
#include "..\..\sdk\structs.hpp"
#include <Sdk/math/Vector.hpp>

enum
{
	MAIN,
	NONE,
	FIRST,
	SECOND,
	THIRD,
	HALF_FIRST,
	HALF_SECOND
};

enum
{
	MIDDLE_SIDE,
	LEFT_SIDE,
	RIGHT_SIDE
};

enum resolver_type
{
	ORIGINAL,
	BRUTEFORCE,
	LBY,
	TRACE,
	DIRECTIONAL,
	ANIM_s,
	ANIM_m,
	ANIM_l,
	LOCKED_SIDE,
	HISTORY_SIDE,
	ANIMATION,
	HIGH_DELTA,
	SHOT,
	LAYER
};

enum resolver_history
{
	HISTORY_UNKNOWN = -1,
	HISTORY_ORIGINAL,
	HISTORY_ZERO,
	HISTORY_DEFAULT,
	HISTORY_LOW
};

enum layers_t
{
	zero_side,
	left_side,
	right_side,
	original_side
};

enum resolver_side
{
	RESOLVER_ORIGINAL,
	RESOLVER_ZERO,
	RESOLVER_FIRST,
	RESOLVER_SECOND,
	RESOLVER_LOW_FIRST,
	RESOLVER_LOW_SECOND,
	BRUTFORC,
	RESOLVER_HIGH_FIRST,
	RESOLVER_HIGH_SECOND,
	RESOLVER_DESYNC_FIRST,
	RESOLVER_DESYNC_SECOND,
	RESOLVER_LEFT,
	RESOLVER_RIGHT,
	RESOLVER_SIDE_UNKNOWN
};

enum animstate_layer_t
{
	ANIMATION_LAYER_AIMMATRIX = 0, // matrix that be aimed
	ANIMATION_LAYER_WEAPON_ACTION, // defusing bomb / reloading / ducking / planting bomb / throwing grenade
	ANIMATION_LAYER_WEAPON_ACTION_RECROUCH,// ducking && defusing bomb / ducking && reloading / ducking && ducking / ducking && planting bomb / ducking && throwing grenade
	ANIMATION_LAYER_ADJUST, // breaking lowerbody yaw
	ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL, // jumping or falling / landing
	ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, // landing / climb
	ANIMATION_LAYER_MOVEMENT_MOVE, // moving
	ANIMATION_LAYER_MOVEMENT_STRAFECHANGE, // strafing
	ANIMATION_LAYER_WHOLE_BODY, // whole body hitbox adjusting
	ANIMATION_LAYER_FLASHED, // player flashed
	ANIMATION_LAYER_FLINCH, // player flinching // flicking lby
	ANIMATION_LAYER_ALIVELOOP, // player alive
	ANIMATION_LAYER_LEAN, // body lean
	ANIMATION_LAYER_COUNT, // layers count
};

static constexpr int resolver_candidate_count = 9;
static constexpr int resolver_body_yaw_pose = 11;

inline constexpr float resolver_candidate_scale[resolver_candidate_count] =
{
	0.0f, 1.0f, -1.0f, 0.5f, -0.5f, 0.75f, -0.75f, 0.25f, -0.25f
};

inline constexpr int resolver_candidate_matrix[resolver_candidate_count] =
{
	NONE, FIRST, SECOND, HALF_FIRST, HALF_SECOND, -1, -1, -1, -1
};

inline constexpr resolver_side resolver_candidate_sides[resolver_candidate_count] =
{
	RESOLVER_ZERO, RESOLVER_FIRST, RESOLVER_SECOND, RESOLVER_LOW_FIRST, RESOLVER_LOW_SECOND,
	RESOLVER_HIGH_FIRST, RESOLVER_HIGH_SECOND, RESOLVER_DESYNC_FIRST, RESOLVER_DESYNC_SECOND
};

inline constexpr int resolver_candidate_order[resolver_candidate_count] =
{
	2, 6, 4, 8, 0, 7, 3, 5, 1
};

inline constexpr animstate_layer_t resolver_scored_layers[] =
{
	ANIMATION_LAYER_AIMMATRIX,
	ANIMATION_LAYER_ADJUST,
	ANIMATION_LAYER_MOVEMENT_MOVE,
	ANIMATION_LAYER_MOVEMENT_STRAFECHANGE,
	ANIMATION_LAYER_WHOLE_BODY,
	ANIMATION_LAYER_ALIVELOOP,
	ANIMATION_LAYER_LEAN
};

static constexpr int resolver_memory_slots = 32;

inline float resolver_max_delta(player_t* e)
{
	if (!e)
		return 0.0f;

	return std::clamp(std::fabs(e->get_max_desync_delta()), 10.0f, 60.0f);
}

struct resolver_memory
{
	uint64_t identity = 0ull;
	float weight[resolver_candidate_count] = { };
	int samples = 0;
	float touched = 0.0f;
	float updated = 0.0f;
};

struct matrixes
{
	matrix3x4_t main[MAXSTUDIOBONES];
	matrix3x4_t zero[MAXSTUDIOBONES];
	matrix3x4_t first[MAXSTUDIOBONES];
	matrix3x4_t second[MAXSTUDIOBONES];
	matrix3x4_t negative[MAXSTUDIOBONES];
	matrix3x4_t positive[MAXSTUDIOBONES];
	matrix3x4_t third[MAXSTUDIOBONES];
};

class adjust_data;

class resolver
{
	player_t* player = nullptr;
	adjust_data* player_record = nullptr;
	adjust_data* previous_player_record = nullptr;

	float original_pitch = 0.0f;

public:

	void initialize(player_t* e, adjust_data* record, const float& goal_feet_yaw, const float& pitch);
	void reset();
	void resolve();
	void BuildMoveYaw(player_t* player, float& foot_yaw);

	float goal_feet_yaw = 0.0f;
	AnimationLayer previous_layers[13];

	float original_goal_feet_yaw = 0.0f;

	resolver_side last_side = RESOLVER_ORIGINAL;

	float last_resolved_yaw = 0.0f;
	float last_resolved_time = 0.0f;

	float last_error = 0.0f;
	float last_confidence = 0.0f;
	int stable_ticks = 0;
	int tried_mask = 0;
	int ruled_out_mask = 0;
	int preferred_mask = 0;
	float mask_time = 0.0f;
	int last_missed = 0;

	int flip_mode = 0;
	float flip_time = 0.0f;
	float last_eye_yaw = 0.0f;
};

class adjust_data
{
public:
	player_t* player;
	int i;
	AnimationLayer layers[13];
	matrixes matrixes_data;
	AnimationLayer resolver_layers[resolver_candidate_count][15];
	float resolver_poses[resolver_candidate_count][24];
	float network_poses[24];
	resolver_type type;
	resolver_side side;

	bool moving_resolver_active;
	bool high_desync_resolver_active;
	bool resolver_confident;
	float resolver_confidence;

	bool invalid;
	bool immune;
	bool dormant;
	bool bot;
	bool shot;
	bool exploited;
	bool teleport_break;

	bool hittable;
	bool selected;
	int hittable_damage;
	int hittable_hitbox;
	int hittable_tick;

	int flags;
	int bone_count;
	int ammo_count;
	int tickbase;
	int weapon_sequence;

	float simulation_time;
	float duck_amount;
	float lby;
	float last_shot_time;
	float weapon_cycle;

	Vector angles;
	Vector abs_angles;
	Vector velocity;
	Vector origin;
	Vector mins;
	Vector maxs;
	Vector hittable_point;

	adjust_data()
	{
		reset();
	}

	void reset()
	{
		player = nullptr;
		i = -1;

		type = ORIGINAL;
		side = RESOLVER_ORIGINAL;

		moving_resolver_active = false;
		high_desync_resolver_active = false;
		resolver_confident = false;
		resolver_confidence = 0.0f;

		invalid = false;
		immune = false;
		dormant = false;
		bot = false;
		shot = false;
		exploited = false;
		teleport_break = false;
		hittable = false;
		selected = false;
		hittable_damage = 0;
		hittable_hitbox = -1;
		hittable_tick = INT_MIN;

		flags = 0;
		bone_count = 0;
		ammo_count = -1;
		tickbase = 0;
		weapon_sequence = -1;

		std::memset(&matrixes_data, 0, sizeof(matrixes_data));
		std::memset(resolver_layers, 0, sizeof(resolver_layers));
		std::memset(resolver_poses, 0, sizeof(resolver_poses));
		std::memset(network_poses, 0, sizeof(network_poses));

		simulation_time = 0.0f;
		duck_amount = 0.0f;
		lby = 0.0f;
		last_shot_time = 0.0f;
		weapon_cycle = 0.0f;

		angles.Zero();
		abs_angles.Zero();
		velocity.Zero();
		origin.Zero();
		mins.Zero();
		maxs.Zero();
		hittable_point.Zero();
	}

	adjust_data(player_t* e, bool store = true)
	{
		type = ORIGINAL;
		side = RESOLVER_ORIGINAL;

		moving_resolver_active = false;
		high_desync_resolver_active = false;
		resolver_confident = false;
		resolver_confidence = 0.0f;

		hittable = false;
		selected = false;
		hittable_damage = 0;
		hittable_hitbox = -1;
		hittable_tick = INT_MIN;
		hittable_point.Zero();

		invalid = false;
		teleport_break = false;
		store_data(e, store);
	}

	void store_data(player_t* e, bool store = true)
	{
		if (!e->is_alive())
			return;

		player = e;
		i = player->EntIndex();

		bone_count = std::clamp(player->m_CachedBoneData().Count(), 0, MAXSTUDIOBONES);

		if (store)
		{
			memcpy(layers, e->get_animlayers(), e->animlayer_count() * sizeof(AnimationLayer));
			memcpy(matrixes_data.main, player->m_CachedBoneData().Base(), bone_count * sizeof(matrix3x4_t));
		}

		immune = player->m_bGunGameImmunity() || player->m_fFlags() & FL_FROZEN;
		dormant = player->IsDormant();

		player_info_t info{};
		bot = m_engine()->GetPlayerInfo(i, &info) && info.fakeplayer;

		flags = player->m_fFlags();
		tickbase = player->m_nTickBase();

		simulation_time = player->m_flSimulationTime();
		duck_amount = player->m_flDuckAmount();
		lby = player->m_flLowerBodyYawTarget();

		auto weapon = player->m_hActiveWeapon().Get();
		if (weapon)
		{
			last_shot_time = weapon->m_fLastShotTime();
			ammo_count = weapon->m_iClip1();

			auto layers = player->get_animlayers();
			if (layers)
			{
				weapon_sequence = layers[1].m_nSequence;
				weapon_cycle = layers[1].m_flCycle;
			}
		}

		angles = player->m_angEyeAngles();
		abs_angles = player->GetAbsAngles();
		velocity = player->m_vecVelocity();
		origin = player->m_vecOrigin();
		mins = player->GetCollideable()->OBBMins();
		maxs = player->GetCollideable()->OBBMaxs();
	}

	void adjust_player()
	{
		if (!valid(false))
			return;

		memcpy(player->get_animlayers(), layers, player->animlayer_count() * sizeof(AnimationLayer));
		auto count = player->m_CachedBoneData().Count() < bone_count ? player->m_CachedBoneData().Count() : bone_count;
		count = std::clamp(count, 0, MAXSTUDIOBONES);
		memcpy(player->m_CachedBoneData().Base(), matrixes_data.main, count * sizeof(matrix3x4_t));

		player->m_fFlags() = flags;
		player->m_CachedBoneData().m_Size = count;

		player->m_flSimulationTime() = simulation_time;
		player->m_flDuckAmount() = duck_amount;
		player->m_flLowerBodyYawTarget() = lby;

		player->m_angEyeAngles() = angles;
		player->set_abs_angles(abs_angles);
		player->m_vecVelocity() = velocity;
		player->m_vecOrigin() = origin;
		player->set_abs_origin(origin);
		player->GetCollideable()->OBBMins() = mins;
		player->GetCollideable()->OBBMaxs() = maxs;
	}

	bool valid(bool extra_checks = true)
	{
		if (i > 0)
			player = (player_t*)m_entitylist()->GetClientEntity(i);

		if (!player)
			return false;

		if (player->m_lifeState() != LIFE_ALIVE)
			return false;

		if (immune)
			return false;

		if (dormant)
			return false;

		if (!extra_checks)
			return true;

		if (invalid)
			return false;

		auto net_channel_info = m_engine()->GetNetChannelInfo();

		if (!net_channel_info)
			return false;

		static auto sv_maxunlag = m_cvar()->FindVar(crypt_str("sv_maxunlag"));

		auto outgoing = net_channel_info->GetLatency(FLOW_OUTGOING);
		auto incoming = net_channel_info->GetLatency(FLOW_INCOMING);

		auto unlag_buffer = 0.01f;
		auto max_unlag = sv_maxunlag->GetFloat() - unlag_buffer;

		auto correct = math::clamp(outgoing + incoming + util::get_interpolation(), 0.0f, max_unlag);

		auto curtime = g_ctx.local()->is_alive() ? TICKS_TO_TIME(g_ctx.globals.fixed_tickbase) : m_globals()->m_curtime;
		auto delta_time = correct - (curtime - simulation_time);

		if (fabs(delta_time) > max_unlag)
			return false;

		auto extra_choke = 0;

		if (g_ctx.globals.fakeducking)
			extra_choke = 14 - m_clientstate()->iChokedCommands;

		auto server_tickcount = extra_choke + m_globals()->m_tickcount + TIME_TO_TICKS(outgoing + incoming);
		auto dead_time = TICKS_TO_TIME(server_tickcount) - max_unlag;

		if (simulation_time < dead_time)
			return false;

		return true;
	}

	bool usable()
	{
		if (valid(true))
			return true;

		return bone_count > 0 && valid(false);
	}
};

struct player_settings
{
	__int64 id;
	resolver_history res_type;
	bool faking;
	bool low_move;
	bool low_stand;
	int neg;
	int pos;

	player_settings(__int64 id, resolver_history res_type, bool faking, int left, int right) noexcept : id(id), res_type(res_type), faking(faking), low_move(false), low_stand(false), neg(left), pos(right) { }
};

class optimized_adjust_data
{
public:
	int i;
	player_t* player;

	float simulation_time;
	float duck_amount;

	bool speed;
	bool shot;

	Vector angles;
	Vector origin;

	optimized_adjust_data()
	{
		reset();
	}

	void reset()
	{
		i = 0;
		player = nullptr;

		simulation_time = 0.0f;
		duck_amount = 0.0f;

		angles.Zero();
		origin.Zero();
	}
};

extern std::deque <adjust_data> player_records[65];

class lagcompensation : public singleton <lagcompensation>
{
public:

	struct LagRecord_t {
		LagRecord_t() = default;

		player_t* m_pEntity;
		int m_iEntIndex;

		matrixes m_pMatrix;
		matrix3x4_t* m_pBoneCache;

		bool m_bValid;
		bool m_bDormant;

		Vector m_vecVelocity;
		Vector m_vecOrigin;
		Vector m_vecAbsOrigin;
		Vector m_vecMins;
		Vector m_vecMaxs;

		AnimationLayer m_pLayers[13];
		AnimationLayer m_pResolveLayers[3][15];
		resolver_side m_ResolverSide;
		float m_pPoses[24];

		c_baseplayeranimationstate* m_pState;

		float m_flSimulationTime;
		float m_flInterpTime;
		float m_flDuck;
		float m_flLowerBodyYawTarget;
		float m_flLastShotTime;
		float m_flSpawnTime;

		Vector m_angLastReliableAngle;
		Vector m_angEyeAngles;
		Vector m_angAbsAngles;

		CBaseHandle m_ulEntHandle;

		int m_fFlags;
		int m_iEFlags;
		int m_iEffects;
		int m_iChoked;

		bool m_bDidShot;
		bool m_bPrefer = false;
		bool m_bAllowAnimationUpdate;
		bool m_bAnimatePlayer;
	};

	void fsn(ClientFrameStage_t stage);
	void upd_nw(player_t* m_pPlayer);
	bool valid(int i, player_t* e);
	void do_anim_event(player_t* pl, c_baseplayeranimationstate* state, int order, int activity);
	void extrapolation(player_t* player, Vector& origin, Vector& velocity, int& flags, bool on_ground);
	void extrapolate(player_t* player, Vector& origin, Vector& velocity, int& flags, bool wasonground);
	void update_player_animations(player_t* e);

	bool is_unsafe_tick(player_t* player);

	void mark_lagcomp_break(int index);
	bool is_breaking_lagcomp(int index);

	resolver_memory* resolver_memory_for(player_t* e);
	void resolver_feedback(int index, resolver_side side, bool hit);
	void resolver_shot_feedback(int index, const Vector& start, const Vector& end, bool hurt, int hitgroup);

	resolver player_resolver[65];
	resolver_memory resolver_memories[resolver_memory_slots];
	std::vector<player_settings> player_sets;

	bool is_dormant[65];
	float previous_goal_feet_yaw[65];
	float feet_delta[65];

	float lagcomp_break_time[65];
	int lagcomp_break_count[65];
	float last_simulation_time[65];
};
