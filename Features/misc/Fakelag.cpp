// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "..\ragebot\aim.h"
#include "fakelag.h"
#include "misc.h"
#include "prediction_system.h"
#include "logs.h"

void fakelag::Fakelag(CUserCmd* m_pcmd)
{
	if (cfg.antiaim.fakelag && !g_ctx.globals.exploits)
	{
		static auto force_choke = false;

		if (force_choke)
		{
			force_choke = false;
			g_ctx.send_packet = false;
			return;
		}

		if (g_ctx.local()->m_fFlags() & FL_ONGROUND && !(engineprediction::get().backup_data.flags & FL_ONGROUND))
		{
			force_choke = true;
			g_ctx.send_packet = false;
			return;
		}
	}

	auto choked = m_clientstate()->iChokedCommands; //-V807
	auto valve_ds = m_gamerules()->m_bIsValveDS(); //-V807

	auto peek_choke = g_ctx.local()->m_fFlags() & FL_ONGROUND && engineprediction::get().backup_data.flags & FL_ONGROUND && !valve_ds && key_binds::get().get_key_bind_state(20); //-V807

	if (peek_choke)
		max_choke = 14;
	else if (!g_ctx.globals.exploits && cfg.antiaim.fakelag)
	{
		switch (cfg.antiaim.fakelag_type)
		{
		default:
			max_choke = cfg.antiaim.fakelag_amount;
			break;
		}
	}
	else
	{
		condition = true;

		if (!g_ctx.globals.exploits && antiaim::get().condition(m_pcmd, false))
			return;

		max_choke = 1;
	}

	if (valve_ds)
		max_choke = m_engine()->IsVoiceRecording() ? 1 : min(max_choke, 6);

	if (misc::get().recharging_double_tap)
		max_choke = g_ctx.globals.exploits ? 1 : 2;

	g_ctx.send_packet = choked >= max_choke;
}

void fakelag::Createmove()
{
	if (FakelagCondition(g_ctx.get_command()))
		return;

	auto m_pcmd = g_ctx.get_command();

	if (cfg.antiaim.fakelag && !g_ctx.globals.exploits && !misc::get().recharging_double_tap)
	{
		bool has_triggers = false;
		for (size_t i = 0; i < cfg.antiaim.fakelag_enablers.size(); i++)
		{
			if (cfg.antiaim.fakelag_enablers[i])
			{
				has_triggers = true;
				break;
			}
		}

		if (has_triggers && misc::get().break_lc(m_pcmd))
			return;
	}

	Fakelag(m_pcmd);
}


bool fakelag::FakelagCondition(CUserCmd* m_pcmd)
{
	condition = false;

	if (g_ctx.local()->m_bGunGameImmunity() || g_ctx.local()->m_fFlags() & FL_FROZEN)
		condition = true;

	if (antiaim::get().freeze_check && !misc::get().double_tap_enabled && !misc::get().hide_shots_enabled)
		condition = true;

	return condition;
}