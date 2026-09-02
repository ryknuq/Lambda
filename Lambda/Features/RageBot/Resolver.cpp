#include "Resolver.h"

#include <algorithm>
#include "../../SDK/Interfaces.h"
#include "../../SDK/Misc/CBasePlayer.h"
#include "LagCompensation.h"
#include "../../SDK/Globals.h"
#include "AnimationSystem.h"


#include "../../Utils/Console.h"


CResolver* Resolver = new CResolver;

float CResolver::GetTime() {
	if (!Cheat.LocalPlayer)
		return GlobalVars->curtime;

	return TICKS_TO_TIME(Cheat.LocalPlayer->m_nTickBase());
}

float FindAvgYaw(const std::deque<LagRecord>& records) {
	float sin_sum = 0.f;
	float cos_sum = 0.f;

	for (int i = records.size() - 2; i > records.size() - 6; i--) {
		const LagRecord* record = &records.at(i);
		float eyeYaw = record->m_angEyeAngles.yaw;

		sin_sum += std::sinf(DEG2RAD(eyeYaw));
		cos_sum += std::cosf(DEG2RAD(eyeYaw));
	}

	return RAD2DEG(std::atan2f(sin_sum, cos_sum));
}

void CResolver::Reset(CBasePlayer* pl) {
	if (pl) {
		resolver_data[pl->EntIndex()].reset();
		return;
	}

	for (int i = 0; i < 64; ++i) {
		resolver_data[i].reset();
	}
}

R_PlayerState CResolver::DetectPlayerState(CBasePlayer* player, AnimationLayer* animlayers) {
	if (!(player->m_fFlags() & FL_ONGROUND))
		return R_PlayerState::AIR;

	CCSGOPlayerAnimationState* animstate = player->GetAnimstate();

	if (player->m_vecVelocity().Length2DSqr() > 256.f && animstate->flWalkToRunTransition > 0.8f && animlayers[ANIMATION_LAYER_MOVEMENT_MOVE].m_flPlaybackRate > 0.0001f)
		return R_PlayerState::MOVING;

	return R_PlayerState::STANDING;
}

R_AntiAimType CResolver::DetectAntiAim(CBasePlayer* player, const std::deque<LagRecord>& records) {
	if (records.size() < 12)
		return R_AntiAimType::NONE;

	int jitteredRecords = 0;
	int staticRecords = 0;
	float avgDelta = 0.f;
	float prevEyeYaw = player->m_angEyeAngles().yaw;

	for (int i = records.size() - 2; i > records.size() - 10; i--) {
		const LagRecord* record = &records.at(i);
		float eyeYaw = record->m_angEyeAngles.yaw;

		float delta = std::abs(Math::AngleDiff(eyeYaw, prevEyeYaw));

		avgDelta += delta;

		if (delta > 32.f)
			jitteredRecords++;
		else
			staticRecords++;

		prevEyeYaw = eyeYaw;
	}

	if (jitteredRecords > staticRecords)
		return R_AntiAimType::JITTER;

	if (avgDelta * 0.5f < 30.f)
		return R_AntiAimType::STATIC;

	return R_AntiAimType::UNKNOWN;
}

float ValveAngleDiff(float destAngle, float srcAngle)
{
	float delta;

	delta = fmodf(destAngle - srcAngle, 360.0f);
	if (destAngle > srcAngle)
	{
		if (delta >= 180)
			delta -= 360;
	}
	else
	{
		if (delta <= -180)
			delta += 360;
	}
	return delta;
}


void CResolver::SetupLayer(LagRecord* record, int idx, float delta) {
	CCSGOPlayerAnimationState* animstate = record->player->GetAnimstate();

	QAngle angles = record->player->m_angEyeAngles();

	animstate->flFootYaw = Math::AngleNormalize(angles.yaw + delta);
	
	Vector vel = record->player->m_vecVelocity();
	float flRawYawIdeal = atan2(-vel.y, -vel.x) * 180.f / M_PI;
	if (flRawYawIdeal < 0)
		flRawYawIdeal += 360;

	animstate->flMoveYaw = Math::AngleNormalize(ValveAngleDiff(flRawYawIdeal, animstate->flFootYaw));
	if (record->prev_record)
		memcpy(record->player->GetAnimlayers(), record->prev_record->animlayers, sizeof(AnimationLayer) * 13);

	animstate->ForceUpdate();
	animstate->Update(angles);
	
	auto& resolver_layer = record->resolver_data.layers[idx];
	resolver_layer.desync = delta;
	resolver_layer.delta = std::abs(record->animlayers[ANIMATION_LAYER_MOVEMENT_MOVE].m_flPlaybackRate - record->player->GetAnimlayers()[ANIMATION_LAYER_MOVEMENT_MOVE].m_flPlaybackRate) * 1000.f;

	*animstate = *AnimationSystem->GetUnupdatedAnimstate(record->player->EntIndex());
}

void CResolver::SetupResolverLayers(CBasePlayer* player, LagRecord* record) {
	SetupLayer(record, 0, 0.f);
	SetupLayer(record, 1, record->resolver_data.max_desync_delta);
	SetupLayer(record, 2, -record->resolver_data.max_desync_delta);
	SetupLayer(record, 3, record->resolver_data.max_desync_delta * 0.6f);
	SetupLayer(record, 4, -record->resolver_data.max_desync_delta * 0.6f);
}

void CResolver::DetectFreestand(CBasePlayer* player, LagRecord* record, const std::deque<LagRecord>& records) {
	if (records.size() < 16)
		return;

	Vector eyePos = player->m_vecOrigin() + Vector(0, 0, 64 - player->m_flDuckAmount() * 16.f);

	Vector forward = (Cheat.LocalPlayer->m_vecOrigin() - player->m_vecOrigin()).Normalized();

	float notModifiedYaw = player->m_angEyeAngles().yaw;

	if (record->resolver_data.antiaim_type != R_AntiAimType::STATIC)
		notModifiedYaw = FindAvgYaw(records);

	Vector right = Math::AngleVectors(QAngle(5.f, notModifiedYaw + 90.f, 0));

	Vector negPos = eyePos - right * 23.f;
	Vector posPos = eyePos + right * 23.f;

	CTraceFilterWorldAndPropsOnly filter;
	Ray_t rayNeg(negPos, negPos + forward * 128.f);
	Ray_t rayPos(posPos, posPos + forward * 128.f);
	CGameTrace negTrace, posTrace;

	EngineTrace->TraceRay(rayNeg, MASK_SHOT_HULL | CONTENTS_GRATE, &filter, &negTrace);
	EngineTrace->TraceRay(rayPos, MASK_SHOT_HULL | CONTENTS_GRATE, &filter, &posTrace);

	if (negTrace.startsolid && posTrace.startsolid) {
		record->resolver_data.side = 0;
		record->resolver_data.resolver_type = ResolverType::NONE;
		return;
	}
	else if (negTrace.startsolid) {
		record->resolver_data.side = -1;
		record->resolver_data.resolver_type = ResolverType::FREESTAND;
		return;
	}
	else if (posTrace.startsolid) {
		record->resolver_data.side = 1;
		record->resolver_data.resolver_type = ResolverType::FREESTAND;
		return;
	}

	if (negTrace.fraction == 1.f && posTrace.fraction == 1.f) {
		record->resolver_data.side = 0;
		record->resolver_data.resolver_type = ResolverType::NONE;
		return;
	}
	record->resolver_data.side = negTrace.fraction < posTrace.fraction ? -1 : 1;
	record->resolver_data.resolver_type = ResolverType::FREESTAND;
}

void CResolver::Apply(LagRecord* record) {
	if (record->resolver_data.side != 0) {
		float body_yaw = record->resolver_data.max_desync_delta * record->resolver_data.side;
		auto state = record->player->GetAnimstate();

		state->flFootYaw = Math::AngleNormalize(state->flEyeYaw + body_yaw);
	}
}

void CResolver::Run(CBasePlayer* player, LagRecord* record, std::deque<LagRecord>& records) {
	if (GameRules()->IsFreezePeriod() || player->m_fFlags() & FL_FROZEN || !Cheat.LocalPlayer->IsAlive() || record->shooting)
		return;

	LagRecord* prevRecord = record->prev_record;

	record->resolver_data.max_desync_delta = player->GetMaxDesyncDelta();

//#ifndef RESOLVER_DEBUG
	if (!record->m_nChokedTicks || player->m_bIsDefusing()) {
		record->resolver_data.side = 0;
		record->resolver_data.resolver_type = ResolverType::NONE;
		return;
	}
//#endif

	record->resolver_data.player_state = DetectPlayerState(player, record->animlayers);
	record->resolver_data.antiaim_type = DetectAntiAim(player, records);

	SetupResolverLayers(player, record);

	record->resolver_data.resolver_type = ResolverType::NONE;

	float min_delta = 1000.f;
	for (auto& layer : record->resolver_data.layers) {
		if (layer.delta < min_delta) {
			min_delta = layer.delta;
			if (layer.desync != 0.f) {
				record->resolver_data.side = layer.desync < 0.f ? -1 : 1;
				record->resolver_data.resolver_type = ResolverType::ANIM;
			}
		}
	}
	
	if (min_delta > 10.f)
		record->resolver_data.resolver_type = ResolverType::NONE;

	float vel_sqr = player->m_vecVelocity().LengthSqr();

	if (vel_sqr < 256.f || record->animlayers[ANIMATION_LAYER_MOVEMENT_MOVE].m_flWeight <= 0.f || record->resolver_data.resolver_type == ResolverType::NONE || !(player->m_fFlags() & FL_ONGROUND)) {
		if (record->resolver_data.antiaim_type == R_AntiAimType::JITTER && records.size() > 16) {
			float eyeYaw = player->m_angEyeAngles().yaw;
			float prevEyeYaw = FindAvgYaw(records);
			float delta = Math::AngleDiff(eyeYaw, prevEyeYaw);

			if (delta < 0.f)
				record->resolver_data.side = 1;
			else
				record->resolver_data.side = -1;

			record->resolver_data.resolver_type = ResolverType::LOGIC;
		}
		else {
			DetectFreestand(player, record, records);
		}
	}

	auto res_data = &resolver_data[player->EntIndex()];
	float curtime = GetTime();

	if (record->resolver_data.resolver_type != ResolverType::NONE) {
		res_data->last_resolved = curtime;
		res_data->last_side = record->resolver_data.side;
		res_data->res_type_last = record->resolver_data.resolver_type;
	}
	else {
		record->resolver_data.resolver_type = ResolverType::MEMORY;
		record->resolver_data.side = res_data->last_side;
	}

	if (curtime - res_data->brute_time < 5.f && record->resolver_data.resolver_type != ResolverType::LOGIC) {
		record->resolver_data.side = res_data->brute_side;
		record->resolver_data.resolver_type = ResolverType::BRUTEFORCE;
	}

	if (record->resolver_data.side == 0) {
		record->resolver_data.side = -1;
		record->resolver_data.resolver_type = ResolverType::DEFAULT;
	}

	Apply(record);
}

void CResolver::OnMiss(CBasePlayer* player, LagRecord* record) {
	auto bf_data = &resolver_data[player->EntIndex()];

	if (bf_data->missed_shots % 3 || bf_data->missed_shots == 0)
		bf_data->brute_side = (record->resolver_data.side == 0) ? -1 : -record->resolver_data.side;
	bf_data->brute_time = GetTime();
	bf_data->missed_shots++;
}

void CResolver::OnHit(CBasePlayer* player, LagRecord* record) {
	auto bf_data = &resolver_data[player->EntIndex()];

	bf_data->brute_side = record->resolver_data.side;
	bf_data->brute_time = GetTime();
	bf_data->missed_shots = 0;
}