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
	THIRD
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

	bool side = false;
	bool fake = false;
	bool got = false;
	bool was_first_bruteforce = false;
	bool was_second_bruteforce = false;
	bool was_first_low_bruteforce = false;
	bool was_second_low_bruteforce = false;
	int RightSide;
	int LeftSide;
	int last_delta1 = false;
	int last_delta2 = false;
	int last_delta3 = false;
	int resolving_way = false;
	float lock_side = 0.0f;
	int m_side;
	int m_way = 1;
	int FreestandSide[64];
	float original_pitch = 0.0f;
	float angle = 0.f;

	int pside = 0;
	int plast_side = 0;

public:

	void initialize(player_t* e, adjust_data* record, const float& goal_feet_yaw, const float& pitch);
	void reset();
	//float BuildMoveYaw(player_t* e, float flFootYaw);
	//float BuildMoveYaw(player_t* e, float flFootYaw);
	void resolve();
	void update_animation_layers(player_t* player);
	void BuildMoveYaw(player_t* player, float& foot_yaw);
	float resolve_pitch();
	//bool NoSpreadResolver();
	float goal_feet_yaw = 0.0f;
	float zero_feet_yaw = 0.0f;
	AnimationLayer resolver_layers[3][13];
	AnimationLayer previous_layers[13];
	AnimationLayer prev_server_layers[6];
	AnimationLayer layers[12];
	AnimationLayer moveLayers[3][13];

	float original_goal_feet_yaw = 0.0f;

	resolver_side last_side = RESOLVER_ORIGINAL;
	resolver_side way = RESOLVER_LEFT;

	float gfy_default = 0.0f;
	float positive_side = 0.0f;
	float negative_side = 0.0f;

	int bruteforce_ticks = 0;
	int freestand_side = 0;
};

class adjust_data
{
public:
	player_t* player;
	int i;
	AnimationLayer right_layers[13];
	AnimationLayer left_layers[13];
	AnimationLayer center_layers[13];
	AnimationLayer layers[13];
	AnimationLayer Animlayers[4][13];
	matrixes matrixes_data;
	AnimationLayer resolver_layers[3][15];
	AnimationLayer m_pResolveLayers[3][15];
	AnimationLayer pre_orig[13] = {};
	resolver_type type;
	resolver_side side;
	layers_t lay;


	float left_side = 0.f, right_side = 0.f;
	bool moving_resolver_active;
	bool high_desync_resolver_active;

	bool invalid;
	bool immune;
	bool dormant;
	bool bot;
	bool shot;

	int flags;
	int bone_count;

	float simulation_time;
	float duck_amount;
	float lby;

	Vector angles;
	Vector abs_angles;
	Vector velocity;
	Vector origin;
	Vector mins;
	Vector maxs;

	matrix3x4_t leftmatrixes[128] = {};
	matrix3x4_t rightmatrixes[128] = {};

	std::array<float, 24> left_poses = {};
	std::array<float, 24> right_poses = {};

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

		invalid = false;
		immune = false;
		dormant = false;
		bot = false;

		flags = 0;
		bone_count = 0;

		std::memset(this->pre_orig, 0, sizeof(pre_orig));

		simulation_time = 0.0f;
		duck_amount = 0.0f;
		lby = 0.0f;

		angles.Zero();
		abs_angles.Zero();
		velocity.Zero();
		origin.Zero();
		mins.Zero();
		maxs.Zero();
	}

	adjust_data(player_t* e, bool store = true)
	{
		type = ORIGINAL;
		side = RESOLVER_ORIGINAL;

		invalid = false;
		store_data(e, store);
	}

	void store_data(player_t* e, bool store = true)
	{
		if (!e->is_alive())
			return;

		player = e;
		i = player->EntIndex();

		if (store)
		{
			memcpy(layers, e->get_animlayers(), e->animlayer_count() * sizeof(AnimationLayer));
			memcpy(matrixes_data.main, player->m_CachedBoneData().Base(), player->m_CachedBoneData().Count() * sizeof(matrix3x4_t));
		}

		immune = player->m_bGunGameImmunity() || player->m_fFlags() & FL_FROZEN;
		dormant = player->IsDormant();

		bot = false;

		flags = player->m_fFlags();
		bone_count = player->m_CachedBoneData().Count();

		simulation_time = player->m_flSimulationTime();
		duck_amount = player->m_flDuckAmount();
		lby = player->m_flLowerBodyYawTarget();

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
		memcpy(player->m_CachedBoneData().Base(), matrixes_data.main, player->m_CachedBoneData().Count() * sizeof(matrix3x4_t));

		player->m_fFlags() = flags;
		player->m_CachedBoneData().m_Size = bone_count;

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
		if (!this)
			return false;

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

		auto correct = math::clamp(outgoing + incoming + util::get_interpolation(), 0.0f, sv_maxunlag->GetFloat());

		auto curtime = g_ctx.local()->is_alive() ? TICKS_TO_TIME(g_ctx.globals.fixed_tickbase) : m_globals()->m_curtime;
		auto delta_time = correct - (curtime - simulation_time);

		if (fabs(delta_time) > 0.2f)
			return false;

		auto extra_choke = 0;

		if (g_ctx.globals.fakeducking)
			extra_choke = 14 - m_clientstate()->iChokedCommands;

		auto server_tickcount = extra_choke + m_globals()->m_tickcount + TIME_TO_TICKS(outgoing + incoming);
		auto dead_time = (int)(TICKS_TO_TIME(server_tickcount) - sv_maxunlag->GetFloat());

		if (simulation_time < (float)dead_time)
			return false;

		return true;
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

	player_settings(__int64 id, resolver_history res_type, bool faking, int left, int right) noexcept : id(id), res_type(res_type), faking(faking), neg(neg), pos(pos) { }
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

class lagdata
{
	c_baseplayeranimationstate* animstate;
public:
	float side;
	float realtime = animstate->m_flLastClientSideAnimationUpdateTime;
	float resolving_way;
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

	resolver player_resolver[65];
	std::vector<player_settings> player_sets;

	bool is_dormant[65];
	float previous_goal_feet_yaw[65];
	float feet_delta[65];
};