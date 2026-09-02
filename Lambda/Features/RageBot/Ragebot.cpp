#include "Ragebot.h"
#include "AutoWall.h"
#include "../Misc/Prediction.h"
#include "../../SDK/Interfaces.h"
#include "../../SDK/Globals.h"
#include "../../Utils/Utils.h"
#include <algorithm>
#include "../Misc/AutoPeek.h"
#include "Exploits.h"
#include "../../Utils/Console.h"
#include "../ShotManager/ShotManager.h"
#include "../Visuals/Chams.h"
#include "../Visuals/ESP.h"

void CRagebot::CalcSpreadValues() {
	for (int i = 0; i < 100; i++) {
		Utils::RandomSeed(i);

		float a = Utils::RandomFloat(0.f, 1.f);
		float b = Utils::RandomFloat(0.f, 6.2831853071795864f);
		float c = Utils::RandomFloat(0.f, 1.f);
		float d = Utils::RandomFloat(0.f, 6.2831853071795864f);

		spread_values[i].a = a;
		spread_values[i].bcos = std::cos(b);
		spread_values[i].bsin = std::sin(b);
		spread_values[i].c = c;
		spread_values[i].dcos = std::cos(d);
		spread_values[i].dsin = std::sin(d);
	}
}

weapon_settings_t CRagebot::GetWeaponSettings(int weaponId) {
	weapon_settings_t settings = config.ragebot.weapons.global;

	switch (weaponId) {
	case Scar20:
	case G3SG1:
		settings = config.ragebot.weapons.autosniper;
		break;
	case Ssg08:
		settings = config.ragebot.weapons.scout;
		break;
	case Awp:
		settings = config.ragebot.weapons.awp;
		break;
	case Deagle:
		settings = config.ragebot.weapons.deagle;
		break;
	case Revolver:
		settings = config.ragebot.weapons.revolver;
		break;
	case Fiveseven:
	case Glock:
	case Usp_s:
	case Tec9:
	case P250:
	case Hkp2000:
	case Elite:
		settings = config.ragebot.weapons.pistol;
		break;
	}

	return settings;
}

void CRagebot::UpdateUI(int idx) {
	if (!Cheat.InGame || !Cheat.LocalPlayer || !Cheat.LocalPlayer->IsAlive() || !ctx.active_weapon)
		return;

	int ui_id = 0;
	if (idx == -1)
		idx = ctx.active_weapon->m_iItemDefinitionIndex();

	switch (idx) {
	case Awp:
		ui_id = 1;
		break;
	case Scar20:
	case G3SG1:
		ui_id = 2;
		break;
	case Ssg08:
		ui_id = 3;
		break;
	case Deagle:
		ui_id = 4;
		break;
	case Revolver:
		ui_id = 5;
		break;
	case Fiveseven:
	case Glock:
	case Usp_s:
	case Tec9:
	case P250:
		ui_id = 6;
		break;
	}

	config.ragebot.selected_weapon->set(ui_id);
}

float CRagebot::CalcMinDamage(CBasePlayer* player) {
	int minimum_damage = settings.minimum_damage->get();
	if (config.ragebot.aimbot.minimum_damage_override_key->get())
		minimum_damage = settings.minimum_damage_override->get();

	if (minimum_damage >= 100) {
		return minimum_damage - 100 + player->m_iHealth();
	}
	else {
		return min(minimum_damage, player->m_iHealth() + 1);
	}
}

void CRagebot::AutoStop(bool predict) {
	if (ctx.cmd->buttons & IN_DUCK && (Cheat.LocalPlayer->m_fFlags() & FL_ONGROUND))
		return;

	if (!(Cheat.LocalPlayer->m_fFlags() & FL_ONGROUND) && !settings.auto_stop->get(2))
		return;

	Vector vec_speed = Cheat.LocalPlayer->m_vecVelocity();
	QAngle direction = Math::VectorAngles(vec_speed);

	float target_speed = ctx.active_weapon->MaxSpeed() * 0.2f;

	if (vec_speed.LengthSqr() < ((target_speed + 1.f) * (target_speed + 1.f))) {
		float cmd_speed = std::sqrt(ctx.cmd->forwardmove * ctx.cmd->forwardmove + ctx.cmd->sidemove * ctx.cmd->sidemove);
	
		if (cmd_speed > 2.f) {
			float factor = target_speed / cmd_speed;
			ctx.cmd->forwardmove *= factor;
			ctx.cmd->sidemove *= factor;
		}

		return;
	}

	QAngle view; EngineClient->GetViewAngles(view);
	direction.yaw = predict ? direction.yaw : (view.yaw - direction.yaw);
	direction.Normalize();

	Vector forward;
	Math::AngleVectors(direction, forward);

	float wish_speed = std::clamp(vec_speed.Length2D(), 0.f, 450.f);

	if (!(Cheat.LocalPlayer->m_fFlags() & FL_ONGROUND)) {
		static ConVar* sv_airaccelerate = CVar->FindVar("sv_airaccelerate");

		wish_speed = std::clamp((vec_speed.Length2D() * 0.936f) / (sv_airaccelerate->GetFloat() * GlobalVars->frametime), 0.f, 450.f);
	}

	Vector nigated_direction = forward * -wish_speed;

	ctx.cmd->sidemove = nigated_direction.y;
	ctx.cmd->forwardmove = nigated_direction.x;
}

float CRagebot::FastHitchance(LagRecord* target, float inaccuracy, int hitbox_radius) {
	if (inaccuracy == -1.f)
		inaccuracy = std::tan(EnginePrediction->WeaponInaccuracy());

	return min(hitbox_radius / ((ctx.shoot_position - (target->m_vecOrigin + Vector(0, 0, 32))).Length() * inaccuracy), 1.f);
}

float CRagebot::CalcHitchance(QAngle angles, LagRecord* target, int hitbox) {
	if (hitbox == HITBOX_LEFT_FOOT || hitbox == HITBOX_RIGHT_FOOT)
		return FastHitchance(target, -1, 3.f);

	int damagegroup = HitboxToDamagegroup(hitbox);

	Vector forward, right, up;
	Math::AngleVectors(angles, forward, right, up);

	int hits = 0;

	for (int i = 0; i < 100; i++)
	{
		SpreadValues_t& s_val = spread_values[i];

		float a = s_val.a;
		float c = s_val.c;

		float inaccuracy = a * EnginePrediction->WeaponInaccuracy();
		float spread = c * EnginePrediction->WeaponSpread();

		if (ctx.active_weapon->m_iItemDefinitionIndex() == 64)
		{
			a = 1.f - a * a;
			a = 1.f - c * c;
		}

		Vector direction = forward + (right * (s_val.bcos * inaccuracy + s_val.dcos * spread)) + (up * (s_val.bsin * inaccuracy + s_val.dsin * spread));
		direction.Normalized();

		if (!EngineTrace->RayIntersectPlayer(ctx.shoot_position, ctx.shoot_position + (direction * 8192.f), target->player, target->clamped_matrix, damagegroup))
			continue;

			hits++;
	}

	return hits * 0.01f;
}

bool CRagebot::CompareRecords(LagRecord* a, LagRecord* b) {
	const Vector vec_diff = a->m_vecOrigin - b->m_vecOrigin;

	if (vec_diff.LengthSqr() > 1.f)
		return false;

	QAngle angle_diff = a->m_angEyeAngles - b->m_angEyeAngles;
	angle_diff.Normalize();

	if (angle_diff.yaw > 90.f)
		return false;

	if (angle_diff.pitch > 10.f)
		return false;

	if (a->breaking_lag_comp != b->breaking_lag_comp)
		return false;

	if (a->shifting_tickbase != b->shifting_tickbase)
		return false;

	return true;
}

void CRagebot::SelectRecords(CBasePlayer* player, std::queue<LagRecord*>& target_records) {
	auto& records = LagCompensation->records(player->EntIndex());

	if (records.empty())
		return;

	if (Exploits->IsShifting() || !cvars.cl_lagcompensation->GetInt()) {
		LagRecord* record = &records.back();

		float latency = 0.f;
		if (INetChannelInfo* nci = EngineClient->GetNetChannelInfo()) {
			latency += nci->GetLatency(FLOW_INCOMING) + nci->GetLatency(FLOW_OUTGOING);

			int pred_ticks = TIME_TO_TICKS(latency);

			int next_update_tick = record->update_tick + record->m_nChokedTicks;

			if (GlobalVars->tickcount + pred_ticks >= next_update_tick)
				record = LagCompensation->ExtrapolateRecord(record, pred_ticks);
		}

		target_records.push(record);
		return;
	}

	bool ex_back = config.ragebot.aimbot.extended_backtrack->get() && !frametime_issues;

	LagRecord* last_valid_record = nullptr;
	for (int i = records.size() - 1; i >= 0; i--) {
		const auto record = &records[i];

		if (!LagCompensation->ValidRecord(record)) {
			WorldESP->AddDebugMessage(std::string(("CRageBot::SelectRecords")).append((" -> !LagCompensation->ValidRecord |")).append((" L") + std::to_string(__LINE__)));

			if (record->breaking_lag_comp || record->invalid)
				break;

			continue;
		}

		if (!target_records.empty() && CompareRecords(record, target_records.back()))
			continue;

		if (ex_back) {
			target_records.push(record);
			continue;
		}

		if (target_records.empty()) {
			target_records.push(record);
		} else {
			if (record->shooting)
				target_records.push(record);

			last_valid_record = record;
		}
	}

	if (!ex_back && last_valid_record && last_valid_record != &records.back()) {
		target_records.push(last_valid_record);
	}

	if (target_records.empty()) {
		// TODO: should be extrapolation here
		auto last_record = &records.back();
		if (!last_record->shifting_tickbase && !last_record->invalid)
			target_records.push(last_record);
	}
}

void CRagebot::AddPoint(AimPoint_t&& point) {
	std::lock_guard<std::mutex> lock(scan_mutex);
	thread_work.emplace(point);
	selected_points++;
	scan_condition.notify_one();
}

void CRagebot::GetMultipoints(LagRecord* record, int hitbox_id, float scale) {
	studiohdr_t* studiomodel = ModelInfoClient->GetStudioModel(record->player->GetModel());

	if (!studiomodel)
		return;

	mstudiobbox_t* hitbox = studiomodel->GetHitboxSet(record->player->m_nHitboxSet())->GetHitbox(hitbox_id);

	if (!hitbox)
		return;

	if (hitbox->flCapsuleRadius <= 0) // do not scan multipoints for feet
		return;

	matrix3x4_t boneMatrix = record->clamped_matrix[hitbox->bone];

	Vector mins, maxs;
	Math::VectorTransform(hitbox->bbmin, boneMatrix, &mins);
	Math::VectorTransform(hitbox->bbmax, boneMatrix, &maxs);

	Vector center = (mins + maxs) * 0.5f;
	Vector sideDirection = center - mins;
	const float width = sideDirection.Normalize() + hitbox->flCapsuleRadius;
	const float radius = hitbox->flCapsuleRadius;

	Vector verts[]{
		Vector(0, radius * scale, 0),
		Vector(0, -radius * scale, 0),
		Vector(0, 0, width * scale),
		Vector(0, 0, -width * scale),
	};

	switch (hitbox_id) {
	case HITBOX_HEAD:
		for (auto& vert : verts)
			AddPoint(AimPoint_t{ Math::VectorTransform(vert, boneMatrix), hitbox_id, true, dont_shoot_next_points });
		if (record->m_angEyeAngles.pitch > 85.f && center.z >= ctx.shoot_position.z - 16.f) // extra point on top of head
			AddPoint(AimPoint_t{ Vector(center.x, center.y, center.z + width * scale * 0.89f), hitbox_id, true, dont_shoot_next_points });
		break;
	case HITBOX_STOMACH:
	case HITBOX_PELVIS:
		AddPoint(AimPoint_t{ Math::VectorTransform(verts[1], boneMatrix), hitbox_id, true, dont_shoot_next_points });
		[[ fallthrough ]];
	case HITBOX_CHEST:
	case HITBOX_UPPER_CHEST:
		AddPoint(AimPoint_t{ Math::VectorTransform(verts[2], boneMatrix), hitbox_id, true, dont_shoot_next_points });
		AddPoint(AimPoint_t{ Math::VectorTransform(verts[3], boneMatrix), hitbox_id, true, dont_shoot_next_points });
		break;
	}
}

bool CRagebot::IsArmored(int hitbox) {
	switch (hitbox)
	{
	case HITBOX_LEFT_CALF:
	case HITBOX_LEFT_FOOT:
	case HITBOX_LEFT_THIGH:
	case HITBOX_RIGHT_THIGH:
	case HITBOX_RIGHT_FOOT:
	case HITBOX_RIGHT_CALF:
		return false;
	default:
		return true;
	}
}

void CRagebot::SelectPoints(LagRecord* record) {
	for (int hitbox = 0; hitbox < HITBOX_MAX; hitbox++) {
		if (!hitbox_enabled(hitbox) || (cvars.mp_damage_headshot_only->GetInt() > 0 && hitbox != HITBOX_HEAD) || (hitbox == HITBOX_HEAD && config.ragebot.aimbot.force_body_aim->get()))
			continue;

		float max_possible_damage = ctx.weapon_info->iDamage;
		record->player->ScaleDamage(HitboxToHitgroup(hitbox), ctx.weapon_info, max_possible_damage);

		int min_damage = CalcMinDamage(record->player);
		if (!settings.auto_stop->get(0) && max_possible_damage < min_damage)
			continue;

		dont_shoot_next_points = max_possible_damage < min_damage;

		{
			std::lock_guard<std::mutex> lock(scan_mutex);
			thread_work.emplace(record->player->GetHitboxCenter(hitbox, record->clamped_matrix), hitbox, false, dont_shoot_next_points);
			selected_points++;
			scan_condition.notify_one();
		}

		if (multipoints_enabled(hitbox) && !Exploits->IsShifting())
			GetMultipoints(record, hitbox, hitbox == HITBOX_HEAD ? settings.head_point_scale->get() * 0.01f : settings.body_point_scale->get() * 0.01f);

		dont_shoot_next_points = false;
	}
}

void CRagebot::SelectBestPoint(ScannedTarget_t* target) {
	ScannedPoint_t best_body_point;
	ScannedPoint_t best_head_point;

	for (const auto& point : target->points) {
		if (point.hitbox == HITBOX_HEAD) {
			if (best_head_point.damage < target->minimum_damage || (point.priority > best_head_point.priority && point.damage > target->minimum_damage))
				best_head_point = point;
		}
		else if (point.hitbox != HITBOX_HEAD) {
			if (best_body_point.damage < target->minimum_damage || (point.priority > best_body_point.priority && point.damage > target->minimum_damage))
				best_body_point = point;
		}
	}

	int& delay = delayed_ticks[target->player->EntIndex()];
	if ((best_body_point.damage > 1.f && !best_body_point.dont_shoot) && best_head_point.damage > 1.f && settings.delay_shot->get() > 0) {
		if (delay < settings.delay_shot->get())
			best_head_point.damage = -1.f;
		delay++;
	}
	else {
		delay = 0;
	}

	if (best_body_point.damage < target->player->m_iHealth() && best_head_point.damage > best_body_point.damage && (best_head_point.record->shooting || (settings.aim_head_if_safe->get() && best_head_point.safe_point))) {
		target->best_point = best_head_point;
		return;
	}

	if ((target->player != last_target && ctx.tickbase_shift == 0 && config.ragebot.aimbot.doubletap->get() && !config.ragebot.aimbot.force_teleport->get())) {
		if (best_body_point.damage > target->player->m_iHealth() || best_head_point.damage < target->player->m_iHealth()) {
			target->best_point = best_body_point;
			return;
		}

		target->best_point = best_head_point;
		return;
	}

	target->best_point = (best_body_point.damage > target->minimum_damage) ? best_body_point : best_head_point;
}

void CRagebot::CreateThreads() {
	for (int i = 0; i < GetRagebotThreads(); i++) {
		threads[i] = Utils::CreateSimpleThread(&CRagebot::ThreadScan, (void*)i);
		inited_threads++;
	}
}

void CRagebot::TerminateThreads() {
	remove_threads = true;
	scan_condition.notify_all();

	for (int i = 0; i < inited_threads; i++) {
		Utils::ThreadJoin(threads[i], 1000000);
		threads[i] = 0;
	}
	inited_threads = 0;
	remove_threads = false;
}

void CRagebot::ScanTargets() {
	scanned_targets.clear();

	for (int i = 0; i < ClientState->m_nMaxClients; i++) {
		CBasePlayer* player = reinterpret_cast<CBasePlayer*>(EntityList->GetClientEntity(i));

		if (!player || !player->IsAlive() || player->IsTeammate() || player->m_bDormant() || player->m_bGunGameImmunity())
			continue;

		if (frametime_issues && (i + ctx.cmd->command_number % 2) % 2 == 0)
			continue;

		ScanTarget(player);
	}
}

uintptr_t CRagebot::ThreadScan(int threadId) {
	while (true) {
		std::unique_lock<std::mutex> scan_lock(Ragebot->scan_mutex);

		Ragebot->scan_condition.wait(scan_lock, []() { return !Ragebot->thread_work.empty() || Ragebot->remove_threads; });

		if (Ragebot->remove_threads)
			break;

		AimPoint_t point = Ragebot->thread_work.front();
		Ragebot->thread_work.pop();
		scan_lock.unlock();

		if (config.ragebot.aimbot.show_aimpoints->get())
			DebugOverlay->AddBoxOverlay(point.point, Vector(-1, -1, -1), Vector(1, 1, 1), QAngle(0, 0, 0), 255, 255, 255, 200, GlobalVars->interval_per_tick * 2);

		CBasePlayer* target = Ragebot->current_record->player;

		if (config.ragebot.aimbot.force_safepoint->get() &&
			!(EngineTrace->RayIntersectPlayer(ctx.shoot_position, point.point, target, Ragebot->current_record->opposite_matrix, HitboxToDamagegroup(point.hitbox)) &&
				EngineTrace->RayIntersectPlayer(ctx.shoot_position, point.point, target, Ragebot->current_record->safe_matrix, HitboxToDamagegroup(point.hitbox)))) {

			std::lock_guard completed_lock(Ragebot->completed_mutex);

			Ragebot->scanned_points++;
			if (Ragebot->scanned_points >= Ragebot->selected_points)
				Ragebot->result_condition.notify_all();
			continue;
		}

		FireBulletData_t bullet;
		if (AutoWall->FireBullet(Cheat.LocalPlayer, ctx.shoot_position, point.point, bullet, target) && bullet.enterTrace.hitgroup == HitboxToHitgroup(point.hitbox)) {
			int priority = point.multipoint ? 0 : 1;
			bool jitter_safe = false;

			if (!config.ragebot.aimbot.force_safepoint->get() &&
				EngineTrace->RayIntersectPlayer(ctx.shoot_position, point.point, target, Ragebot->current_record->opposite_matrix, HitboxToDamagegroup(point.hitbox)) &&
				EngineTrace->RayIntersectPlayer(ctx.shoot_position, point.point, target, Ragebot->current_record->safe_matrix, HitboxToDamagegroup(point.hitbox))) {

				priority += 2;
				jitter_safe = true;
			}

			if (point.hitbox >= HITBOX_PELVIS && point.hitbox <= HITBOX_UPPER_CHEST)
				priority += 2;

			if (Ragebot->current_record->shooting)
				priority += 2;
			else if (Ragebot->current_record->m_angEyeAngles.pitch < 10.f) // aim at shitty defensive aa
				priority += 1;

			if (GlobalVars->tickcount - Ragebot->current_record->update_tick < 12)
				priority += 1;

			ScannedPoint_t sc_point;
			sc_point.damage = bullet.damage;
			sc_point.hitbox = point.hitbox;
			std::memcpy(sc_point.impacts, bullet.impacts, sizeof(Vector) * 5);
			sc_point.num_impacts = bullet.num_impacts;
			sc_point.point = point.point;
			sc_point.priority = priority;
			sc_point.record = Ragebot->current_record;
			sc_point.safe_point = jitter_safe;
			sc_point.dont_shoot = point.dont_shoot;

			if (bullet.damage > 5.f)
				Exploits->block_charge = true;

			std::lock_guard<std::mutex> result_lock(Ragebot->result_mutex);
			Ragebot->result_target->points.push_back(sc_point);
		}

		std::lock_guard completed_lock(Ragebot->completed_mutex);			

		Ragebot->scanned_points++;
		if (Ragebot->scanned_points >= Ragebot->selected_points)
			Ragebot->result_condition.notify_all();
	}

	return 0;
}

void CRagebot::ScanTarget(CBasePlayer* target) {
	// Add debug message to indicate starting of ScanTarget function

	std::queue<LagRecord*> records;
	SelectRecords(target, records);

	if (records.empty()) {
		// Add debug message for empty records
		WorldESP->AddDebugMessage(std::string(("CRageBot::ScanTarget")).append((" -> Records Empty |")).append((" L") + std::to_string(__LINE__)));
		return;
	}

	float minimum_damage = CalcMinDamage(target);

	ScannedTarget_t* result = &scanned_targets.emplace_back();
	result->player = target;
	result->minimum_damage = minimum_damage;

	LagRecord* backup_record = LagCompensation->BackupData(target);

	while (records.size() > 0) {
		LagRecord* record = records.front();
		records.pop();

		LagCompensation->BacktrackEntity(record, false);
		record->BuildMatrix();

		memcpy(target->GetCachedBoneData().Base(), record->clamped_matrix, sizeof(matrix3x4_t) * target->GetCachedBoneData().Count());
		target->ForceBoneCache();

		current_record = record;
		result_target = result;
		selected_points = 0;
		scanned_points = 0;

		while (!thread_work.empty())
			thread_work.pop();

		SelectPoints(record);

		std::unique_lock<std::mutex> lock(completed_mutex);
		result_condition.wait(lock, [this]() {
			return scanned_points >= selected_points;
		});
	}

	LagCompensation->BacktrackEntity(backup_record);
	delete backup_record;

	SelectBestPoint(result);
	if (!result->best_point.record || result->best_point.damage < 2) {
		// Add debug message if best point is not selected or damage is low
		WorldESP->AddDebugMessage(std::string(("CRageBot::ScanTarget")).append((" -> !SelectBestPoint |")).append((" L") + std::to_string(__LINE__)));
		return;
	}

	result->angle = Math::VectorAngles_p(result->best_point.point - ctx.shoot_position);

	if (frametime_issues || Exploits->IsShifting()) { // fast hitchance approx
		result->hitchance = min(10.f / ((ctx.shoot_position - result->best_point.point).Length() * std::tan(EnginePrediction->WeaponInaccuracy())), 1.f);
	}
	else {
		result->hitchance = CalcHitchance(result->angle, result->best_point.record, result->best_point.hitbox);
	}

}

void CRagebot::RunPrediction(const QAngle& angle) {
	if (!(Cheat.LocalPlayer->m_fFlags() & FL_ONGROUND))
		return;

	if (Cheat.LocalPlayer->m_vecVelocity().LengthSqr() < 16.f)
		return;

	if (ctx.cmd->buttons & IN_DUCK)
		return;

	pre_prediction.should_restore = true;
	pre_prediction.m_vecAbsOrigin = Cheat.LocalPlayer->GetAbsOrigin();
	pre_prediction.m_fFlags = Cheat.LocalPlayer->m_fFlags();
	pre_prediction.m_flDuckAmount = Cheat.LocalPlayer->m_flDuckAmount();
	pre_prediction.m_flDuckSpeed = Cheat.LocalPlayer->m_flDuckSpeed();
	pre_prediction.m_vecVelocity = Cheat.LocalPlayer->m_vecVelocity();
	pre_prediction.m_vecAbsVelocity = Cheat.LocalPlayer->m_vecAbsVelocity();
	pre_prediction.m_hGroundEntity = Cheat.LocalPlayer->m_hGroundEntity();
	pre_prediction.m_vecMins = Cheat.LocalPlayer->m_vecMins();
	pre_prediction.m_vecMaxs = Cheat.LocalPlayer->m_vecMaxs();

	CUserCmd stop_cmd = *ctx.cmd;
	CUserCmd* backup_cmd = ctx.cmd;
	ctx.cmd = &stop_cmd;
	AutoStop(true);
	EnginePrediction->Repredict(ctx.cmd, QAngle(0, 0, 0));
	ctx.cmd = backup_cmd;
}

void CRagebot::RestorePrediction() {
	if (!pre_prediction.should_restore)
		return;

	Cheat.LocalPlayer->GetAbsOrigin() = pre_prediction.m_vecAbsOrigin;
	Cheat.LocalPlayer->m_fFlags() = pre_prediction.m_fFlags;
	Cheat.LocalPlayer->m_flDuckAmount() = pre_prediction.m_flDuckAmount;
	Cheat.LocalPlayer->m_flDuckSpeed() = pre_prediction.m_flDuckSpeed;
	Cheat.LocalPlayer->m_vecVelocity() = pre_prediction.m_vecVelocity;
	Cheat.LocalPlayer->m_vecAbsVelocity() = pre_prediction.m_vecAbsVelocity;
	Cheat.LocalPlayer->m_hGroundEntity() = pre_prediction.m_hGroundEntity;
	Cheat.LocalPlayer->m_vecMins() = pre_prediction.m_vecMins;
	Cheat.LocalPlayer->m_vecMaxs() = pre_prediction.m_vecMaxs;
}

void CRagebot::Run() {
	pre_prediction.should_restore = false;

	if (!config.ragebot.aimbot.enabled->get() || !ctx.active_weapon)
		return;

	AutoRevolver();

	if (Cheat.LocalPlayer->m_fFlags() & FL_FROZEN || GameRules()->IsFreezePeriod())
		return;

	if (ctx.active_weapon->IsGrenade())
		return;

	int weapon_id = ctx.active_weapon->m_iItemDefinitionIndex();
	if (!Exploits->IsShifting()) {
		if (weapon_id == Taser) {
			Zeusbot();
			return;
		}

		if (ctx.weapon_info->nWeaponType == WEAPONTYPE_KNIFE) {
			Knifebot();
			return;
		}

		if (GlobalVars->realtime - last_frametime_check > 5.f) {
			last_frametime_check = GlobalVars->realtime;
			frametime_issues = EnginePrediction->frametime() > GlobalVars->interval_per_tick; // fps is below tickrate, limit targets to increase it
		}
	}

	if (!ctx.active_weapon->ShootingWeapon())
		return;

	settings = GetWeaponSettings(weapon_id);

	if (config.ragebot.aimbot.dormant_aim->get() && !Exploits->IsShifting())
		DormantAimbot();

	if (!Exploits->IsShifting()) {
		QAngle vangle; EngineClient->GetViewAngles(vangle);
		RunPrediction(vangle);
	}

	hook_info.disable_interpolation = true;
	ScanTargets();
	hook_info.disable_interpolation = false;

	ScannedTarget_t best_target;
	bool should_autostop = false;

	bool local_on_ground = Cheat.LocalPlayer->m_fFlags() & FL_ONGROUND && EnginePrediction->pre_prediction.m_fFlags & FL_ONGROUND;
	int m_nWeaponMode = Cheat.LocalPlayer->m_bIsScoped() ? 1 : 0;
	float min_jump_inaccuracy_tan = 0.f;

	if (settings.auto_stop->get(2) && !local_on_ground) { // superior "dynamic autostop"
		float flInaccuracyJumpInitial = ctx.weapon_info->_flInaccuracyUnknown;

		float fSqrtMaxJumpSpeed = std::sqrt(cvars.sv_jump_impulse->GetFloat());
		float fSqrtVerticalSpeed = std::sqrt(abs(ctx.local_velocity.z) * 0.7f);

		float flAirSpeedInaccuracy = Math::RemapVal(fSqrtVerticalSpeed,
			fSqrtMaxJumpSpeed * 0.25f,
			fSqrtMaxJumpSpeed,
			0.0f,
			flInaccuracyJumpInitial);

		if (flAirSpeedInaccuracy < 0)
			flAirSpeedInaccuracy = 0;
		else if (flAirSpeedInaccuracy > (2.f * flInaccuracyJumpInitial))
			flAirSpeedInaccuracy = 2.f * flInaccuracyJumpInitial;

		min_jump_inaccuracy_tan = std::tan(ctx.weapon_info->flInaccuracyStand[m_nWeaponMode] + ctx.weapon_info->flInaccuracyJump[m_nWeaponMode] + flAirSpeedInaccuracy);
	}

	float hitchance = settings.hitchance->get() * 0.01f;
	if (!settings.strict_hitchance->get()) {
		if ((weapon_id == Ssg08 && !local_on_ground) ||
			((weapon_id == Scar20 || weapon_id == G3SG1) && GlobalVars->realtime - Exploits->last_teleport_time < TICKS_TO_TIME(16)) ||
			(weapon_id == Awp && Cheat.LocalPlayer->GetAnimstate() && Cheat.LocalPlayer->GetAnimstate()->bLanding))
			hitchance *= 0.8f;
	}

	for (const auto& target : scanned_targets) {
		if (target.best_point.damage <= 1.f)
			continue;

		float jump_max_hc = FastHitchance(target.best_point.record, min_jump_inaccuracy_tan);

		if (target.best_point.damage > target.minimum_damage && ctx.cmd->command_number - last_target_shot < 256 && target.player == last_target) {
			if (target.hitchance > hitchance) {
				best_target = target;
				should_autostop = true;
				break;
			}
			else if (local_on_ground || (settings.auto_stop->get(2) && jump_max_hc >= hitchance * 0.2f)) {
				should_autostop = true;
			}
		}

		if (target.hitchance > hitchance && target.best_point.damage > max(best_target.best_point.damage, target.minimum_damage))
			best_target = target;

		if (target.best_point.damage > target.minimum_damage) {
			if (local_on_ground || (settings.auto_stop->get(2) && jump_max_hc >= hitchance * 0.2f))
				should_autostop = true;

			if (settings.auto_scope->get() && !Cheat.LocalPlayer->m_bIsScoped() && !Cheat.LocalPlayer->m_bResumeZoom() && ctx.weapon_info->nWeaponType == WEAPONTYPE_SNIPER)
				ctx.cmd->buttons |= IN_ATTACK2;
		}

		if (target.best_point.damage > 2) {
			if (settings.auto_stop->get(0) && (local_on_ground || (settings.auto_stop->get(2) && jump_max_hc >= hitchance)) && target.best_point.damage > target.minimum_damage * 0.4f)
				should_autostop = true;

			Exploits->block_charge = true;
		}
	}

	bool can_shoot_this_tick = ctx.active_weapon->CanShoot();

	if (best_target.player && ctx.active_weapon->m_iItemDefinitionIndex() == Revolver && ctx.active_weapon->CanShoot(false))
		ctx.cmd->buttons |= IN_ATTACK;

	if (settings.auto_stop->get(1) && !can_shoot_this_tick && ctx.active_weapon->m_iItemDefinitionIndex() != Revolver)
		return;

	if (should_autostop && !AutoPeek->IsReturning())
		AutoStop();

	if (Exploits->IsShifting())
		return;

	if (!best_target.player || !can_shoot_this_tick)
		return;

	pre_prediction.should_restore = false;

	ctx.cmd->tick_count = TIME_TO_TICKS(best_target.best_point.record->m_flSimulationTime + LagCompensation->GetLerpTime());
	ctx.cmd->viewangles = Math::VectorAngles_p(best_target.best_point.point - ctx.shoot_position) - Cheat.LocalPlayer->m_aimPunchAngle() * cvars.weapon_recoil_scale->GetFloat();
	ctx.cmd->buttons |= IN_ATTACK;

	if (Exploits->GetExploitType() == CExploits::E_DoubleTap)
		Exploits->ForceTeleport();
	else if (Exploits->GetExploitType() == CExploits::E_HideShots)
		Exploits->HideShot();
	if (!config.antiaim.misc.fake_duck->get())
		ctx.send_packet = true;

	AutoPeek->Return();
	last_target = best_target.player;
	last_target_shot = ctx.cmd->command_number;

	if (config.visuals.effects.client_impacts->get() && !config.misc.miscellaneous.gamesense_mode->get()) {
		Color face_col = config.visuals.effects.client_impacts_color->get();

		for (int i = 0; i < best_target.best_point.num_impacts; i++)
			DebugOverlay->AddBox(best_target.best_point.impacts[i], Vector(-1, -1, -1), Vector(1, 1, 1), face_col, config.visuals.effects.impacts_duration->get());
	}

	LagRecord* record = best_target.best_point.record;

	ShotManager->AddShot(ctx.shoot_position, best_target.best_point.point, best_target.best_point.damage, HitboxToDamagegroup(best_target.best_point.hitbox), best_target.hitchance, best_target.best_point.safe_point, record, best_target.best_point.impacts, best_target.best_point.num_impacts);
	if (config.visuals.chams.shot_chams->get())
		Chams->AddShotChams(record);

	if (WorldESP->DebugMessages.size())
		WorldESP->DebugMessagesSane = std::move(WorldESP->DebugMessages);

	WorldESP->DebugMessages.clear();
}


void CRagebot::Zeusbot() {
	const float inaccuracy_tan = std::tan(EnginePrediction->WeaponInaccuracy());

	if (!ctx.active_weapon->CanShoot())
		return;

	for (int i = 0; i < ClientState->m_nMaxClients; i++) {
		CBasePlayer* player = reinterpret_cast<CBasePlayer*>(EntityList->GetClientEntity(i));
		auto& records = LagCompensation->records(i);

		if (!player || player->IsTeammate() || !player->IsAlive() || player->m_bDormant() || player->m_bGunGameImmunity())
			continue;

		LagRecord* backup_record = LagCompensation->BackupData(player);

		for (auto i = records.rbegin(); i != records.rend(); i = std::next(i)) {
			const auto record = &*i;
			if (!LagCompensation->ValidRecord(record)) {

				WorldESP->AddDebugMessage(std::string(("CRageBot::Knifebot")).append((" -> Invalid Record |")).append((" L") + std::to_string(__LINE__)));

				if (record->breaking_lag_comp)
					break;

				continue;
			}

			float distance = ((record->m_vecOrigin + (record->m_vecMaxs + record->m_vecMins) * 0.5f) - ctx.shoot_position).LengthSqr();

			if (distance > 155 * 155)
				continue;

			LagCompensation->BacktrackEntity(record, false);
			record->BuildMatrix();

			const Vector points[]{
				player->GetHitboxCenter(HITBOX_STOMACH, record->clamped_matrix),
				player->GetHitboxCenter(HITBOX_CHEST, record->clamped_matrix),
				player->GetHitboxCenter(HITBOX_UPPER_CHEST, record->clamped_matrix),
				player->GetHitboxCenter(HITBOX_LEFT_UPPER_ARM, record->clamped_matrix),
				player->GetHitboxCenter(HITBOX_RIGHT_UPPER_ARM, record->clamped_matrix)
			};

			for (const auto& point : points) {
				CGameTrace trace = EngineTrace->TraceRay(ctx.shoot_position, point, MASK_SHOT | CONTENTS_GRATE, Cheat.LocalPlayer);

				if (trace.hit_entity != player) {
					CGameTrace clip_trace;
					Ray_t ray(ctx.shoot_position, point);
					if (!EngineTrace->ClipRayToPlayer(ray, MASK_SHOT | CONTENTS_GRATE, player, &clip_trace))
						continue;

					if (clip_trace.fraction < trace.fraction)
						trace = clip_trace;
					else
						continue;
				}

				float hitchance = min(12.f / ((ctx.shoot_position - point).Length() * inaccuracy_tan), 1.f);

				if (hitchance < 0.7f)
					continue;

				WorldESP->AddDebugMessage(std::string(("CRageBot::ZeusBot")).append((" -> Hitchance Success |")).append((" L") + std::to_string(__LINE__)));

				if (config.visuals.effects.client_impacts->get()) {
					Color col = config.visuals.effects.client_impacts_color->get();
					DebugOverlay->AddBox(trace.endpos, Vector(-1, -1, -1), Vector(1, 1, 1), col, config.visuals.effects.impacts_duration->get());
				}

				ctx.cmd->viewangles = Math::VectorAngles(point - ctx.shoot_position);
				ctx.cmd->buttons |= IN_ATTACK;
				ctx.cmd->tick_count = TIME_TO_TICKS(record->m_flSimulationTime + LagCompensation->GetLerpTime());

				if (!config.antiaim.misc.fake_duck->get())
					ctx.send_packet = true;

				if (config.visuals.chams.shot_chams->get())
					Chams->AddShotChams(record);

				LagCompensation->BacktrackEntity(backup_record);
				delete backup_record;
				return;
			}
		}

		LagCompensation->BacktrackEntity(backup_record);
		delete backup_record;
	}
}

void CRagebot::Knifebot() {
	const float inaccuracy_tan = std::tan(EnginePrediction->WeaponInaccuracy());

	if (!ctx.active_weapon->CanShoot())
		return;

	for (int i = 0; i < ClientState->m_nMaxClients; i++) {
		CBasePlayer* player = reinterpret_cast<CBasePlayer*>(EntityList->GetClientEntity(i));
		auto& records = LagCompensation->records(i);

		if (!player || player->IsTeammate() || !player->IsAlive() || player->m_bDormant() || player->m_bGunGameImmunity())
			continue;

		LagRecord* backup_record = LagCompensation->BackupData(player);

		for (auto i = records.rbegin(); i != records.rend(); i = std::next(i)) {
			const auto record = &*i;
			if (!LagCompensation->ValidRecord(record)) {

				WorldESP->AddDebugMessage(std::string(("CRageBot::Knifebot")).append((" -> Invalid Record |")).append((" L") + std::to_string(__LINE__)));

				if (record->breaking_lag_comp)
					break;

				continue;
			}

			Vector target_position = record->m_vecOrigin;
			target_position.z = std::clamp(ctx.shoot_position.z, record->m_vecOrigin.z, record->m_vecOrigin.z + record->m_vecMaxs.z) - 0.01f; // shitty valve tracer is broken if start.z == end.z

			float distance = (target_position - ctx.shoot_position).LengthSqr();

			if (distance > 6400.f) { // out of range
				if (distance < 32768.f)
					Exploits->block_charge = true;
				continue;
			}

			Exploits->block_charge = true;

			Vector vTragetForward = Math::AngleVectors(QAngle(0, record->m_angEyeAngles.yaw));
			vTragetForward.z = 0.f;

			Vector vecLOS = (record->m_vecOrigin - Cheat.LocalPlayer->GetAbsOrigin());
			vecLOS.z = 0.f;
			vecLOS.Normalize();

			float flDot = vecLOS.Dot(vTragetForward);

			bool can_backstab = false;
			if (flDot > 0.475f)
				can_backstab = true;

			int health = player->m_iHealth();
			bool first_swing = ctx.active_weapon->m_flNextPrimaryAttack() + 0.4f < GlobalVars->curtime;

			int left_click_dmg = 25;
			int right_click_dmg = 65;

			if (can_backstab) {
				left_click_dmg = 90;
				right_click_dmg = 180;
			}
			else if (first_swing) {
				left_click_dmg = 40;
			}

			bool should_right_click = false;

			if (right_click_dmg >= health && left_click_dmg < health)
				should_right_click = true;

			float knife_range = should_right_click ? 32.f : 48.f;

			Vector vecDelta = (target_position - ctx.shoot_position).Normalized();
			Vector vecEnd = ctx.shoot_position + vecDelta * knife_range;

			LagCompensation->BacktrackEntity(record);

			CGameTrace trace = EngineTrace->TraceHull(ctx.shoot_position, vecEnd, Vector(-16, -16, -18), Vector(16, 16, 18), MASK_SOLID, Cheat.LocalPlayer);

			if (trace.hit_entity != player || trace.fraction == 1.f) {
				CGameTrace clipped_trace;
				if (EngineTrace->ClipRayToPlayer(Ray_t(ctx.shoot_position, vecEnd + vecDelta * 18.f), MASK_SHOT | CONTENTS_GRATE, player, &clipped_trace) && clipped_trace.fraction < trace.fraction)
					trace = clipped_trace;
			}

			if (trace.hit_entity != player || ((trace.endpos - ctx.shoot_position).LengthSqr() > (knife_range * knife_range)))
				continue;

			ctx.cmd->viewangles = Math::VectorAngles(target_position - ctx.shoot_position);
			ctx.cmd->buttons |= should_right_click ? IN_ATTACK2 : IN_ATTACK;
			ctx.cmd->tick_count = TIME_TO_TICKS(record->m_flSimulationTime + LagCompensation->GetLerpTime());

			if ((should_right_click ? right_click_dmg : left_click_dmg) < health && Exploits->GetExploitType() == CExploits::E_DoubleTap)
				Exploits->ForceTeleport();

			ctx.last_shot_time = GlobalVars->realtime + 0.5f; // prevent from charging

			if (config.visuals.chams.shot_chams->get()) {
				memcpy(record->clamped_matrix, record->bone_matrix, sizeof(matrix3x4_t) * 128);
				Chams->AddShotChams(record);
			}

			LagCompensation->BacktrackEntity(backup_record);
			delete backup_record;
			return;
		}

		LagCompensation->BacktrackEntity(backup_record);
		delete backup_record;
	}
}

void CRagebot::AutoRevolver() {
	if (ctx.cmd->buttons & IN_ATTACK)
		return;

	if (ctx.active_weapon->m_iItemDefinitionIndex() != Revolver)
		return;

	ctx.cmd->buttons &= ~IN_ATTACK2;

	static float next_cock_time = 0.f;
	float time = TICKS_TO_TIME(Cheat.LocalPlayer->m_nTickBase());

	if (ctx.active_weapon->m_flPostponeFireReadyTime() > time) {
		if (time > next_cock_time)
			ctx.cmd->buttons |= IN_ATTACK;
	}
	else {
		next_cock_time = time + 0.25f;
	}
}

void CRagebot::DormantAimbot() {

	const float inaccuracy_tan = std::tan(EnginePrediction->WeaponInaccuracy());

	if (!ctx.active_weapon->CanShoot() && settings.auto_stop->get(2)) {
		WorldESP->AddDebugMessage(std::string(("CRageBot::DormantAimbot")).append(" -> NoShoot |").append((" L") + std::to_string(__LINE__)));

		return;
	}

	for (int i = 0; i < ClientState->m_nMaxClients; i++) {
		CBasePlayer* player = reinterpret_cast<CBasePlayer*>(EntityList->GetClientEntity(i));

		if (!player || !player->m_bDormant() || !player->IsAlive() || player->IsTeammate())
			continue;

		if (EnginePrediction->curtime() - WorldESP->GetESPInfo(i).m_flLastUpdateTime > 5.f)
			continue;

		Vector shoot_target = player->m_vecOrigin() + Vector(0, 0, 36.013f);

		FireBulletData_t bullet;
		if (!AutoWall->FireBullet(Cheat.LocalPlayer, ctx.shoot_position, shoot_target, bullet) || bullet.damage < CalcMinDamage(player) || (bullet.enterTrace.hit_entity != nullptr && bullet.enterTrace.hit_entity->IsPlayer()))
			continue;

		float hc = min(7.f / ((ctx.shoot_position - shoot_target).Length() * inaccuracy_tan), 1.f);

		AutoStop();
		if (hc * 100.f < settings.hitchance->get() || !ctx.active_weapon->CanShoot()) {
			WorldESP->AddDebugMessage(std::string(("CRageBot::DormantAimbot")).append(" -> NoShoot |").append((" L") + std::to_string(__LINE__)));
			continue;
		}

		ctx.cmd->viewangles = Math::VectorAngles_p(shoot_target - ctx.shoot_position) - Cheat.LocalPlayer->m_aimPunchAngle() * cvars.weapon_recoil_scale->GetFloat();
		ctx.cmd->buttons |= IN_ATTACK;

		if (config.visuals.effects.client_impacts->get()) {
			for (const auto& impact : bullet.impacts)
				DebugOverlay->AddBoxOverlay(impact, Vector(-1, -1, -1), Vector(1, 1, 1), QAngle(),
					config.visuals.effects.client_impacts_color->get().r,
					config.visuals.effects.client_impacts_color->get().g,
					config.visuals.effects.client_impacts_color->get().b,
					config.visuals.effects.client_impacts_color->get().a,
					config.visuals.effects.impacts_duration->get());
		}

		if (Exploits->GetExploitType() == CExploits::E_DoubleTap)
			Exploits->ForceTeleport();
		else if (Exploits->GetExploitType() == CExploits::E_HideShots)
			Exploits->HideShot();
		if (!config.antiaim.misc.fake_duck->get())
			ctx.send_packet = true;

		AutoPeek->Return();

		break;
	}

}

CRagebot* Ragebot = new CRagebot;