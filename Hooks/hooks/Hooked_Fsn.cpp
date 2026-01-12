#include "..\hooks.hpp"
#include "..\..\features\ragebot\aim.h"
#include "..\..\features\lagcompensation\animation_system.h"
#include "..\..\features\visuals\nightmode.h"
#include "..\..\features\visuals\otheresp.h"
#include "..\..\features\misc\misc.h"
#include "..\..\utils\nSkinz\SkinChanger.h"
#include "..\..\features\misc\fakelag.h"
#include "..\..\features\visuals\worldesp.h"
#include "..\..\features\misc\logs.h"
#include "..\..\features\misc\prediction_system.h"
#include "..\..\features\lagcompensation\local_animations.h"
#include <sstream>
#include <iomanip>

using FrameStageNotify_t = void(__stdcall*)(ClientFrameStage_t);

Vector flb_aim_punch;
Vector flb_view_punch;

Vector* aim_punch;
Vector* view_punch;

void weather()
{
	static ClientClass* client_class = nullptr;

	if (!client_class)
		client_class = m_client()->GetAllClasses();

	while (client_class)
	{
		if (client_class->m_ClassID == CPrecipitation)
			break;

		client_class = client_class->m_pNext;
	}

	if (!client_class)
		return;

	auto entry = m_entitylist()->GetHighestEntityIndex() + 1;
	auto serial = math::random_int(0, 4095);

	g_ctx.globals.m_networkable = client_class->m_pCreateFn(entry, serial);

	if (!g_ctx.globals.m_networkable)
		return;

	auto m_precipitation = g_ctx.globals.m_networkable->GetIClientUnknown()->GetBaseEntity();

	if (!m_precipitation)
		return;

	g_ctx.globals.m_networkable->PreDataUpdate(0);
	g_ctx.globals.m_networkable->OnPreDataChanged(0);

	static auto m_nPrecipType = netvars::get().get_offset(crypt_str("CPrecipitation"), crypt_str("m_nPrecipType"));
	static auto m_vecMins = netvars::get().get_offset(crypt_str("CBaseEntity"), crypt_str("m_vecMins"));
	static auto m_vecMaxs = netvars::get().get_offset(crypt_str("CBaseEntity"), crypt_str("m_vecMaxs"));

	*(int*)(uintptr_t(m_precipitation) + m_nPrecipType) = 0;
	*(Vector*)(uintptr_t(m_precipitation) + m_vecMaxs) = Vector(32768.0f, 32768.0f, 32768.0f);
	*(Vector*)(uintptr_t(m_precipitation) + m_vecMins) = Vector(-32768.0f, -32768.0f, -32768.0f);

	m_precipitation->GetCollideable()->OBBMaxs() = Vector(32768.0f, 32768.0f, 32768.0f);
	m_precipitation->GetCollideable()->OBBMins() = Vector(-32768.0f, -32768.0f, -32768.0f);

	m_precipitation->set_abs_origin((m_precipitation->GetCollideable()->OBBMins() + m_precipitation->GetCollideable()->OBBMins()) * 0.5f);
	m_precipitation->m_vecOrigin() = (m_precipitation->GetCollideable()->OBBMaxs() + m_precipitation->GetCollideable()->OBBMins()) * 0.5f;

	m_precipitation->OnDataChanged(0);
	m_precipitation->PostDataUpdate(0);
}

void remove_smoke()
{
	if (cfg.player.enable && cfg.esp.removals[REMOVALS_SMOKE])
	{
		static auto smoke_count = *reinterpret_cast<uint32_t**>(util::FindSignature(crypt_str("client.dll"), crypt_str("A3 ? ? ? ? 57 8B CB")) + 0x1);
		*(int*)smoke_count = 0;
	}

	if (g_ctx.globals.should_remove_smoke == cfg.player.enable && cfg.esp.removals[REMOVALS_SMOKE])
		return;

	g_ctx.globals.should_remove_smoke = cfg.player.enable && cfg.esp.removals[REMOVALS_SMOKE];

	static std::vector <const char*> smoke_materials =
	{
		"effects/overlaysmoke",
		"particle/beam_smoke_01",
		"particle/particle_smokegrenade",
		"particle/particle_smokegrenade1",
		"particle/particle_smokegrenade2",
		"particle/particle_smokegrenade3",
		"particle/particle_smokegrenade_sc",
		"particle/smoke1/smoke1",
		"particle/smoke1/smoke1_ash",
		"particle/smoke1/smoke1_nearcull",
		"particle/smoke1/smoke1_nearcull2",
		"particle/smoke1/smoke1_snow",
		"particle/smokesprites_0001",
		"particle/smokestack",
		"particle/vistasmokev1/vistasmokev1",
		"particle/vistasmokev1/vistasmokev1_emods",
		"particle/vistasmokev1/vistasmokev1_emods_impactdust",
		"particle/vistasmokev1/vistasmokev1_fire",
		"particle/vistasmokev1/vistasmokev1_nearcull",
		"particle/vistasmokev1/vistasmokev1_nearcull_fog",
		"particle/vistasmokev1/vistasmokev1_nearcull_nodepth",
		"particle/vistasmokev1/vistassmokev1_smokegrenade",
		"particle/vistasmokev1/vistasmokev4_emods_nocull",
		"particle/vistasmokev1/vistasmokev4_nearcull",
		"particle/vistasmokev1/vistasmokev4_nocull"
	};

	for (auto material_name : smoke_materials)
	{
		auto material = m_materialsystem()->FindMaterial(material_name, nullptr);

		if (!material)
			continue;

		material->SetMaterialVarFlag(MATERIAL_VAR_NO_DRAW, g_ctx.globals.should_remove_smoke);
	}
}


void __stdcall hooks::hooked_fsn(ClientFrameStage_t stage)
{
	static auto original_fn = client_hook->get_func_address <FrameStageNotify_t>(37);
	g_ctx.local((player_t*)m_entitylist()->GetClientEntity(m_engine()->GetLocalPlayer()), true);

	if (!g_ctx.available())
	{
		nightmode::get().clear_stored_materials();
		return original_fn(stage);
	}

	if (stage == FRAME_START)
		key_binds::get().update_key_binds();

	aim_punch = nullptr;
	view_punch = nullptr;

	flb_aim_punch.Zero();
	flb_view_punch.Zero();

	if (g_ctx.globals.updating_skins && m_clientstate()->iDeltaTick > 0)
		g_ctx.globals.updating_skins = false;

	SkinChanger::run(stage);
	local_animations::get().run(stage);

	if (stage == FRAME_NET_UPDATE_POSTDATAUPDATE_START && g_ctx.local()->is_alive())
	{
		auto viewmodel = g_ctx.local()->m_hViewModel().Get();

		if (viewmodel && engineprediction::get().viewmodel_data.weapon == viewmodel->m_hWeapon().Get() && engineprediction::get().viewmodel_data.sequence == viewmodel->m_nSequence() && engineprediction::get().viewmodel_data.animation_parity == viewmodel->m_nAnimationParity())
		{
			viewmodel->m_flCycle() = engineprediction::get().viewmodel_data.cycle;
			viewmodel->m_flAnimTime() = engineprediction::get().viewmodel_data.animation_time;
		}
	}

	if (stage == FRAME_RENDER_START)
	{
		if (cfg.esp.client_bullet_impacts)
		{
			static auto last_count = 0;
			auto& client_impact_list = *(CUtlVector <client_hit_verify_t>*)((uintptr_t)g_ctx.local() + 0x11C50);

			for (auto i = client_impact_list.Count(); i > last_count; --i)
				m_debugoverlay()->BoxOverlay(client_impact_list[i - 1].position, Vector(-2.0f, -2.0f, -2.0f), Vector(2.0f, 2.0f, 2.0f), QAngle(0.0f, 0.0f, 0.0f), cfg.esp.client_bullet_impacts_color.r(), cfg.esp.client_bullet_impacts_color.g(), cfg.esp.client_bullet_impacts_color.b(), cfg.esp.client_bullet_impacts_color.a(), 4.0f);

			if (client_impact_list.Count() != last_count)
				last_count = client_impact_list.Count();
		}

		remove_smoke();

		if (cfg.esp.removals[REMOVALS_FLASH] && g_ctx.local()->m_flFlashDuration() && cfg.player.enable)
			g_ctx.local()->m_flFlashDuration() = 0.0f;

		if (*(bool*)m_postprocessing() != (cfg.player.enable && cfg.esp.removals[REMOVALS_POSTPROCESSING] && (!cfg.esp.world_modulation || !cfg.esp.exposure)))
			*(bool*)m_postprocessing() = cfg.player.enable && cfg.esp.removals[REMOVALS_POSTPROCESSING] && (!cfg.esp.world_modulation || !cfg.esp.exposure);

		if (cfg.esp.removals[REMOVALS_RECOIL] && cfg.player.enable)
		{
			aim_punch = &g_ctx.local()->m_aimPunchAngle();
			view_punch = &g_ctx.local()->m_viewPunchAngle();

			flb_aim_punch = *aim_punch;
			flb_view_punch = *view_punch;

			(*aim_punch).Zero();
			(*view_punch).Zero(); //-V656
		}

		auto get_original_scope = false;

		if (g_ctx.local()->is_alive())
		{
			g_ctx.globals.in_thirdperson = key_binds::get().get_key_bind_state(17);

			if (cfg.player.enable && cfg.esp.removals[REMOVALS_SCOPE])
			{
				auto weapon = g_ctx.local()->m_hActiveWeapon().Get();

				if (weapon)
				{
					get_original_scope = true;

					g_ctx.globals.scoped = g_ctx.local()->m_bIsScoped() && weapon->m_zoomLevel();
					g_ctx.local()->m_bIsScoped() = weapon->m_zoomLevel();
				}
			}
		}

		if (!get_original_scope)
			g_ctx.globals.scoped = g_ctx.local()->m_bIsScoped();
	}

	if (stage == FRAME_NET_UPDATE_END)
	{
		static auto rain = false;

		if (rain != cfg.esp.rain || g_ctx.globals.should_update_weather)
		{
			rain = cfg.esp.rain;

			if (g_ctx.globals.m_networkable)
			{
				g_ctx.globals.m_networkable->Release();
				g_ctx.globals.m_networkable = nullptr;
			}

			if (rain)
				weather();

			g_ctx.globals.should_update_weather = false;
		}
	}

	if (stage == FRAME_RENDER_END)
	{
		static auto r_drawspecificstaticprop = m_cvar()->FindVar(crypt_str("r_drawspecificstaticprop"));

		if (r_drawspecificstaticprop->GetBool())
			r_drawspecificstaticprop->SetValue(FALSE);

		if (g_ctx.globals.change_materials)
		{
			if (cfg.esp.nightmode && cfg.player.enable)
				nightmode::get().apply();
			else
				nightmode::get().remove();

			g_ctx.globals.change_materials = false;
		}

		worldesp::get().skybox_changer();
		worldesp::get().fog_changer();

		misc::get().FullBright();
		misc::get().ViewModel();

		static auto cl_foot_contact_shadows = m_cvar()->FindVar(crypt_str("cl_foot_contact_shadows"));

		if (cl_foot_contact_shadows->GetBool())
			cl_foot_contact_shadows->SetValue(FALSE);

		static auto zoom_sensitivity_ratio_mouse = m_cvar()->FindVar(crypt_str("zoom_sensitivity_ratio_mouse"));

		if (cfg.player.enable && cfg.esp.removals[REMOVALS_ZOOM] && cfg.esp.fix_zoom_sensivity && zoom_sensitivity_ratio_mouse->GetFloat() == 1.0f) //-V550
			zoom_sensitivity_ratio_mouse->SetValue(0.0f);
		else if ((!cfg.player.enable || !cfg.esp.removals[REMOVALS_ZOOM] || !cfg.esp.fix_zoom_sensivity) && !zoom_sensitivity_ratio_mouse->GetFloat())
			zoom_sensitivity_ratio_mouse->SetValue(1.0f);

		static auto r_modelAmbientMin = m_cvar()->FindVar(crypt_str("r_modelAmbientMin"));

		if (cfg.esp.world_modulation && cfg.esp.ambient && r_modelAmbientMin->GetFloat() != cfg.esp.ambient * 0.05f) //-V550
			r_modelAmbientMin->SetValue(cfg.esp.ambient * 0.05f);
		else if ((!cfg.esp.world_modulation || !cfg.esp.ambient) && r_modelAmbientMin->GetFloat())
			r_modelAmbientMin->SetValue(0.0f);
	}

	if (stage == FRAME_NET_UPDATE_END)
	{
		auto current_shot = g_ctx.shots.end();

		auto net_channel = m_engine()->GetNetChannelInfo();
		auto latency = net_channel ? net_channel->GetLatency(FLOW_OUTGOING) + net_channel->GetLatency(FLOW_INCOMING) + 1.0f : 0.0f;

		for (auto& shot = g_ctx.shots.begin(); shot != g_ctx.shots.end(); ++shot)
		{
			if (shot->end)
			{
				current_shot = shot;
				break;
			}
			else if (shot->impacts && m_globals()->m_tickcount - 1 > shot->event_fire_tick)
			{
				current_shot = shot;
				current_shot->end = true;
				break;
			}
			else if (g_ctx.globals.backup_tickbase - TIME_TO_TICKS(latency) > shot->fire_tick)
			{
				current_shot = shot;
				current_shot->end = true;
				current_shot->latency = true;
				break;
			}
		}

		if (current_shot != g_ctx.shots.end())
		{
			if (!current_shot->latency)
			{
				current_shot->shot_info.should_log = true; //-V807

				if (!current_shot->hurt_player)
				{
					misc::get().aimbot_hitboxes();

					// OCCLUSION CHECK FIRST - Must check if autowall actually returned invalid
					if (current_shot->occlusion && current_shot->impact_hit_player)
					{
						// BRANCH 1: OCCLUSION MISS (only if autowall said so AND we hit something)
						current_shot->shot_info.result = crypt_str("Occlusion");
						current_shot->shot_info.miss_reason = crypt_str("Occlusion");
						current_shot->shot_info.was_occluded = true;

						++g_ctx.globals.missed_shots[current_shot->last_target];
						++g_ctx.globals.miss_reason_count[1];

						if (cfg.misc.events_to_log[EVENTLOG_HIT])
						{
							std::stringstream log;
							log << crypt_str("MISS [Occlusion] - ") << current_shot->shot_info.weapon_name
								<< crypt_str(" | Target: ") << current_shot->shot_info.target_name
								<< crypt_str(" | Dist: ") << std::fixed << std::setprecision(0) << current_shot->shot_info.distance_to_target << crypt_str("u")
								<< crypt_str(" | Lat: ") << current_shot->shot_info.network_latency_ms << crypt_str("ms")
								<< crypt_str(" | HC: ") << current_shot->shot_info.hitchance << crypt_str("%")
								<< crypt_str(" | BT: ") << current_shot->shot_info.backtrack_ticks << crypt_str("t");

							eventlogs::get().add(log.str());
						}
					}
					else if (!current_shot->impact_hit_player)
					{
						// BRANCH 2: SPREAD MISS (checked third)
						// No impact hit = pure spread/inaccuracy miss
						current_shot->shot_info.result = crypt_str("Spread");
						current_shot->shot_info.miss_reason = crypt_str("Spread");

						++g_ctx.globals.missed_shots[current_shot->last_target];
						++g_ctx.globals.miss_reason_count[0];

						if (cfg.misc.events_to_log[EVENTLOG_HIT])
						{
							std::stringstream log;
							log << crypt_str("MISS [Spread] - ") << current_shot->shot_info.weapon_name
								<< crypt_str(" | Target: ") << current_shot->shot_info.target_name
								<< crypt_str(" | Dist: ") << std::fixed << std::setprecision(0) << current_shot->shot_info.distance_to_target << crypt_str("u")
								<< crypt_str(" | Lat: ") << current_shot->shot_info.network_latency_ms << crypt_str("ms")
								<< crypt_str(" | HC: ") << current_shot->shot_info.hitchance << crypt_str("%")
								<< crypt_str(" | Inaccuracy: ") << std::fixed << std::setprecision(3) << g_ctx.globals.inaccuracy;

							eventlogs::get().add(log.str());
						}
					}
					else if (current_shot->target_position_at_fire.DistTo(current_shot->target_position_at_impact) > 16.0f)
					{
						// BRANCH 3: PREDICTION ERROR MISS
						current_shot->shot_info.result = crypt_str("Prediction");
						current_shot->shot_info.miss_reason = crypt_str("Prediction");
						current_shot->shot_info.prediction_error = true;

						++g_ctx.globals.missed_shots[current_shot->last_target];
						++g_ctx.globals.miss_reason_count[2];

						if (cfg.misc.events_to_log[EVENTLOG_HIT])
						{
							std::stringstream log;
							float move_distance = current_shot->target_position_at_fire.DistTo(current_shot->target_position_at_impact);
							log << crypt_str("MISS [Prediction] - ") << current_shot->shot_info.weapon_name
								<< crypt_str(" | Target: ") << current_shot->shot_info.target_name
								<< crypt_str(" | Dist: ") << std::fixed << std::setprecision(0) << current_shot->shot_info.distance_to_target << crypt_str("u")
								<< crypt_str(" | Lat: ") << current_shot->shot_info.network_latency_ms << crypt_str("ms")
								<< crypt_str(" | Player Moved: ") << std::fixed << std::setprecision(1) << move_distance << crypt_str("u")
								<< crypt_str(" | HC: ") << current_shot->shot_info.hitchance << crypt_str("%")
								<< crypt_str(" | BT: ") << current_shot->shot_info.backtrack_ticks << crypt_str("t");

							eventlogs::get().add(log.str());
						}
					}
					else
					{
						// BRANCH 4: RESOLVER or CORRECTION MISS
						std::string miss_reason = crypt_str("Resolver");
						int miss_reason_idx = 4;

						if (current_shot->shot_info.target_animation_sequence > 0 &&
							current_shot->shot_info.target_animation_cycle > 0.0f)
						{
							miss_reason = crypt_str("Correction");
							miss_reason_idx = 3;
							current_shot->shot_info.correction_failed = true;
						}

						current_shot->shot_info.result = miss_reason;
						current_shot->shot_info.miss_reason = miss_reason;
						current_shot->shot_info.resolver_side = current_shot->side;

						++g_ctx.globals.missed_shots[current_shot->last_target];
						++g_ctx.globals.miss_reason_count[miss_reason_idx];
						lagcompensation::get().player_resolver[current_shot->last_target].last_side = (resolver_side)current_shot->side;

						if (cfg.misc.events_to_log[EVENTLOG_HIT])
						{
							std::stringstream log;
							log << crypt_str("MISS [") << miss_reason << crypt_str("] - ")
								<< current_shot->shot_info.weapon_name << crypt_str(" | Target: ")
								<< current_shot->shot_info.target_name
								<< crypt_str(" | Dist: ") << std::fixed << std::setprecision(0) << current_shot->shot_info.distance_to_target << crypt_str("u")
								<< crypt_str(" | Lat: ") << current_shot->shot_info.network_latency_ms << crypt_str("ms");

							if (miss_reason == crypt_str("Resolver"))
							{
								std::string side_name = crypt_str("Unknown");
								if (current_shot->side == RESOLVER_LEFT)
									side_name = crypt_str("Left");
								else if (current_shot->side == RESOLVER_RIGHT)
									side_name = crypt_str("Right");
								else if (current_shot->side == RESOLVER_ZERO)
									side_name = crypt_str("Zero");
								log << crypt_str(" | Resolved Side: ") << side_name;
							}

							log << crypt_str(" | HC: ") << current_shot->shot_info.hitchance << crypt_str("%")
								<< crypt_str(" | BT: ") << current_shot->shot_info.backtrack_ticks << crypt_str("t");

							eventlogs::get().add(log.str());
						}
					}
				}
				else if (cfg.misc.events_to_log[EVENTLOG_HIT])
				{
					// Log successful HIT
					++g_ctx.globals.total_shots_hit;  // Track HIT

					std::stringstream log;
					log << crypt_str("HIT [") << current_shot->shot_info.client_hitbox << crypt_str("] - ")
						<< current_shot->shot_info.weapon_name << crypt_str(" | Target: ")
						<< current_shot->shot_info.target_name
						<< crypt_str(" | Damage: ") << current_shot->shot_info.server_damage
						<< crypt_str(" | Dist: ") << std::fixed << std::setprecision(0) << current_shot->shot_info.distance_to_target << crypt_str("u")
						<< crypt_str(" | Lat: ") << current_shot->shot_info.network_latency_ms << crypt_str("ms")
						<< crypt_str(" | HC: ") << current_shot->shot_info.hitchance << crypt_str("%")
						<< crypt_str(" | BT: ") << current_shot->shot_info.backtrack_ticks << crypt_str("t");

					eventlogs::get().add(log.str());
				}
			}

			g_ctx.shots.erase(current_shot);
		}
	}

	lagcompensation::get().fsn(stage);

	original_fn(stage);

	static DWORD* death_notice = nullptr;

	if (g_ctx.local()->is_alive())
	{
		if (!death_notice)
			death_notice = util::FindHudElement <DWORD>(crypt_str("CCSGO_HudDeathNotice"));

		if (death_notice)
		{
			auto local_death_notice = (float*)((uintptr_t)death_notice + 0x50);

			if (local_death_notice)
				*local_death_notice = cfg.esp.preserve_killfeed ? FLT_MAX : 1.5f;

			if (g_ctx.globals.should_clear_death_notices)
			{
				g_ctx.globals.should_clear_death_notices = false;

				using Fn = void(__thiscall*)(uintptr_t);
				static auto clear_notices = (Fn)util::FindSignature(crypt_str("client.dll"), crypt_str("55 8B EC 83 EC 0C 53 56 8B 71 58"));

				clear_notices((uintptr_t)death_notice - 0x14);
			}
		}
	}
	else
		death_notice = 0;
}