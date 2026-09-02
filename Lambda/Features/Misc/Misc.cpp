#include "Misc.h"
#include "../RageBot/Exploits.h"
#include "Prediction.h"
#include "../../Utils/Utils.h"
#include "../../SDK/Interfaces.h"
#include "../../SDK/Config.h"
#include "../../SDK/Globals.h"

#include <string>
#include <vector>

void Miscellaneous::Clantag()
{
	if (!Menu->IsInitialized())
		return;

	static auto removed = false;

	if (!config.misc.miscellaneous.clantag->get() && !removed)
	{
		removed = true;
		Utils::SetClantag("");
		return;
	}

	if (config.misc.miscellaneous.clantag->get())
	{
		auto nci = EngineClient->GetNetChannelInfo();

		if (!nci)
			return;

		static auto time = -1;

		auto ticks = TIME_TO_TICKS(nci->GetAvgLatency(FLOW_OUTGOING)) + (float)GlobalVars->tickcount; //-V807
		auto intervals = 0.4f / GlobalVars->interval_per_tick;

		auto main_time = (int)(ticks / intervals) % 24;

		if (main_time != time && !ClientState->m_nChokedCommands)
		{
			static const char* frames[] = {
				">_____", "L>____", "La>___", "Lam>__",
				"Lamb>_", "Lambd>", "Lambda", ">ambda",
				"_>mbda", "__>bda", "___>da", "____>a",
				"_____<", "____<a", "___<da", "__<bda",
				"_<mbda", "<ambda", "Lambda", "Lambd<",
				"Lamb<_", "Lam<__", "La<___", "L<____",
			};

			Utils::SetClantag(frames[main_time]);
			time = main_time;
		}

		removed = false;
	}
}

static const char* trashtalk_kill_phrases[] = {
	"discord.gg/lmbda stop using your shitty cheat dog",
	"Maybe if you used lambda you wouldnt stress",
	"lmao you just died to a open sourced cheat <3",
	"open source > your paid paste, discord.gg/lmbda",
	"get lambda or get free, pick one",
	"that was a free cheat by the way",
	"imagine paying monthly for that resolver",
	"your config is the only thing holding you back",
	"another one for the open source team <3",
	"lambda beta and you still cant handle it"
};

static const char* trashtalk_death_phrases[] = {
	"your lucky lambda is still in beta, otherwise i wouldve shit on you",
	"sigh, you got lucky this time...",
	"lambda isnt performing very well, maybe its config issue?",
	"hey lambda user, fix your config you bot",
	"one tick off, ill take it next round",
	"thats a config issue not a cheat issue",
	"beta build, beta results, still free though",
	"resolver said no. discord.gg/lmbda",
	"i had 1 hp dont act like you outplayed me",
	"note to self, stop using the default config"
};

static std::vector<std::string> trashtalk_pending;

static int TrashtalkRandom(int limit)
{
	static auto state = 0u;

	if (!state)
		state = (unsigned int)GetTickCount() | 1u;

	state ^= state << 13;
	state ^= state >> 17;
	state ^= state << 5;

	return (int)(state % (unsigned int)limit);
}

void Miscellaneous::Trashtalk(bool killed)
{
	if (!config.misc.miscellaneous.trashtalk->get())
		return;

	const auto event = killed ? 0 : 1;

	if (!config.misc.miscellaneous.trashtalk_events->get(event))
		return;

	auto phrases = killed ? trashtalk_kill_phrases : trashtalk_death_phrases;
	auto count = killed ? (int)_countof(trashtalk_kill_phrases) : (int)_countof(trashtalk_death_phrases);

	if (count < 1)
		return;

	static int previous[2] = { -1, -1 };

	auto index = 0;

	if (count > 1)
	{
		if (previous[event] < 0 || previous[event] >= count)
			index = TrashtalkRandom(count);
		else
		{
			index = TrashtalkRandom(count - 1);

			if (index >= previous[event])
				++index;
		}
	}

	previous[event] = index;

	trashtalk_pending.emplace_back(phrases[index]);
}

void Miscellaneous::FlushTrashtalk()
{
	if (trashtalk_pending.empty())
		return;

	if (!EngineClient->IsInGame())
	{
		trashtalk_pending.clear();
		return;
	}

	for (auto& phrase : trashtalk_pending)
		EngineClient->ExecuteClientCmd(("say \"" + phrase + "\"").c_str());

	trashtalk_pending.clear();
}

void Miscellaneous::FastThrow() {
	static bool fast_throw_triggred = false;
	static int nLastButtons = 0;

	if (!ctx.active_weapon || !ctx.active_weapon->IsGrenade()) {
		fast_throw_triggred = false;
		return;
	}

	CBaseGrenade* grenade = reinterpret_cast<CBaseGrenade*>(ctx.active_weapon);

	if (!(ctx.cmd->buttons & (IN_ATTACK | IN_ATTACK2)) && (nLastButtons & (IN_ATTACK | IN_ATTACK2)) && grenade->m_bPinPulled())
		ctx.grenade_throw_tick = ctx.cmd->command_number;

	nLastButtons = ctx.cmd->buttons;

	if (!config.ragebot.aimbot.doubletap_options->get(3)) {
		fast_throw_triggred = false;
		return;
	}

	if (ctx.tickbase_shift > 0) {
		Exploits->LC_OverrideTickbase(ctx.tickbase_shift);

		float arm_time = max(Cheat.LocalPlayer->m_flNextAttack(), grenade->m_flNextPrimaryAttack());

		if (TICKS_TO_TIME(Cheat.LocalPlayer->m_nTickBase()) + TICKS_TO_TIME(ctx.tickbase_shift - 7) > arm_time)
			Exploits->LC_OverrideTickbase(7);

		if (grenade->m_flThrowTime() > 0.f)
			fast_throw_triggred = true;

		if (fast_throw_triggred)
			Exploits->LC_OverrideTickbase(0);
	}

	if (ctx.cmd->command_number == ctx.grenade_throw_tick + 8 && ctx.grenade_throw_tick != 0)
		ctx.switch_to_main_weapon = true;
}


void Miscellaneous::FastSwitch() {
	if (!ctx.switch_to_main_weapon)
		return;

	ctx.switch_to_main_weapon = false;
	CBaseCombatWeapon* best_weapon = nullptr;
	auto weapons = Cheat.LocalPlayer->m_hMyWeapons();
	int best_type = WEAPONTYPE_KNIFE;
	for (int i = 0; i < MAX_WEAPONS; i++) {
		auto weap = weapons[i];
		if (weap == INVALID_EHANDLE_INDEX)
			break;

		CBaseCombatWeapon* weapon = reinterpret_cast<CBaseCombatWeapon*>(EntityList->GetClientEntityFromHandle(weap));

		if (!weapon)
			continue;

		CCSWeaponData* weap_info = weapon->GetWeaponInfo();

		if (!weap_info)
			continue;

		if (weap_info->nWeaponType >= WEAPONTYPE_SUBMACHINEGUN && weap_info->nWeaponType <= WEAPONTYPE_MACHINEGUN) {
			best_weapon = weapon;
			best_type = weap_info->nWeaponType;
		}
		else if (weap_info->nWeaponType == WEAPONTYPE_PISTOL && best_type == WEAPONTYPE_KNIFE) {
			best_weapon = weapon;
			best_type = weap_info->nWeaponType;
		}
	}

	if (best_weapon)
		ctx.cmd->weaponselect = best_weapon->EntIndex();
}

void Miscellaneous::AutomaticGrenadeRelease() {
	static bool prev_release = false;
	static Vector on_release_move;
	static QAngle on_release_angle;

	if (ctx.should_release_grenade && ctx.active_weapon && ctx.active_weapon->IsGrenade()) {
		if (!prev_release) {
			on_release_move = Vector(ctx.cmd->sidemove, ctx.cmd->forwardmove);
			on_release_angle = ctx.grenade_release_angle;
		}

		ctx.cmd->buttons &= ~(IN_ATTACK | IN_ATTACK2);

		if (ctx.cmd->command_number <= ctx.grenade_throw_tick + 7) {
			ctx.cmd->sidemove = on_release_move.x;
			ctx.cmd->forwardmove = on_release_move.y;
			ctx.cmd->viewangles = on_release_angle;
		}
	}
	else if (!ctx.active_weapon || !ctx.active_weapon->IsGrenade()) {
		ctx.should_release_grenade = false;
	}

	prev_release = ctx.should_release_grenade;
}

static bool s_ShouldClearNotices = false;
void Miscellaneous::PreserveKillfeed() {
	if (!Cheat.InGame || !Cheat.LocalPlayer)
		return;

	static auto spawntime = 0.f;
	static auto status = false;
	static auto clear_deathnotices = reinterpret_cast<void(__thiscall*)(uintptr_t*)>(Utils::PatternScan("client.dll", "55 8B EC 83 EC 0C 53 56 8B 71 58"));

	auto set = false;
	if (spawntime != Cheat.LocalPlayer->m_flSpawnTime() || status != config.visuals.effects.preserve_killfeed->get())
	{
		set = true;
		status = config.visuals.effects.preserve_killfeed->get();
		spawntime = Cheat.LocalPlayer->m_flSpawnTime();
	}

	const auto hud_radar = CSGOHud->FindHudElement("CCSGO_HudRadar");
	const auto death_notice = reinterpret_cast<uintptr_t>(CSGOHud->FindHudElement("CCSGO_HudDeathNotice"));
	if (death_notice == 20)
		return;

	const auto notice_element = reinterpret_cast<uintptr_t*>(death_notice - 0x14);
	if (!death_notice || !notice_element)
		return;

	if (set) {
		const auto lifetime = reinterpret_cast<float*>(death_notice + 0x50);
		*lifetime = status ? FLT_MAX : 1.5f;
	}

	if (s_ShouldClearNotices) {
		s_ShouldClearNotices = false;
		clear_deathnotices(notice_element);
	}
}

void Miscellaneous::ClearKillfeed() {
	s_ShouldClearNotices = true;
}

void Miscellaneous::RadarAngles() {
	if (!Cheat.InGame)
		return;

	static const auto hud_radar = reinterpret_cast<CSGO_HudRadar*>(CSGOHud->FindHudElement("CCSGO_HudRadar"));

	if (!hud_radar)
		return;

	hud_radar->m_vecLocalAngles.y = 0.f;
}