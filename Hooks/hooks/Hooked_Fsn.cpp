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

static bool is_smoke_material(IMaterial* material)
{
	if (!material || material->IsErrorMaterial())
		return false;

	auto name = material->GetName();

	if (!name)
		return false;

	char path[192];
	auto length = 0;

	while (name[length] && length < (int)sizeof(path) - 1)
	{
		auto character = name[length];

		if (character >= 'A' && character <= 'Z')
			character += 32;
		else if (character == '\\')
			character = '/';

		path[length] = character;
		++length;
	}

	path[length] = '\0';

	if (!strstr(path, crypt_str("smoke")))
		return false;

	static const char* effect_paths[] =
	{
		crypt_str("particle/"),
		crypt_str("particles/"),
		crypt_str("effects/"),
		crypt_str("sprites/"),
		crypt_str("overlays/"),
		crypt_str("decals/")
	};

	for (auto effect_path : effect_paths)
	{
		if (!strncmp(path, effect_path, strlen(effect_path)))
			return true;
	}

	return false;
}

void suppress_smoke_effects()
{
	static auto did_smoke_effect = netvars::get().get_offset(crypt_str("CSmokeGrenadeProjectile"), crypt_str("m_bDidSmokeEffect"));

	if (!did_smoke_effect)
		return;

	if (!cfg.player.enable || !cfg.esp.removals[REMOVALS_SMOKE])
		return;

	for (auto i = 1; i <= m_entitylist()->GetHighestEntityIndex(); ++i)
	{
		auto e = static_cast <entity_t*> (m_entitylist()->GetClientEntity(i));

		if (!e || e->is_player())
			continue;

		auto client_class = e->GetClientClass();

		if (!client_class || client_class->m_ClassID != CSmokeGrenadeProjectile)
			continue;

		*(bool*)((uintptr_t)e + did_smoke_effect) = true;
	}
}

void remove_smoke()
{
	const auto enabled = cfg.player.enable && cfg.esp.removals[REMOVALS_SMOKE];

	if (enabled)
	{
		static auto smoke_count = []() -> int*
		{
			const auto address = util::FindSignature(crypt_str("client.dll"), crypt_str("A3 ? ? ? ? 57 8B CB"));

			return address ? *reinterpret_cast <int**> (address + 0x1) : nullptr;
		}();

		if (smoke_count)
			*smoke_count = 0;
	}

	static auto applied = false;
	static auto next_pass = 0.0f;

	if (!enabled && !applied)
		return;

	if (g_ctx.globals.should_remove_smoke != enabled)
	{
		g_ctx.globals.should_remove_smoke = enabled;
		next_pass = 0.0f;
	}

	if (m_globals()->m_realtime < next_pass)
		return;

	next_pass = m_globals()->m_realtime + 0.25f;
	applied = enabled;

	auto materialsystem = m_materialsystem();

	for (auto handle = materialsystem->FirstMaterial(); handle != materialsystem->InvalidMaterial(); handle = materialsystem->NextMaterial(handle))
	{
		auto material = materialsystem->GetMaterial(handle);

		if (!is_smoke_material(material))
			continue;

		if (material->GetMaterialVarFlag(MATERIAL_VAR_NO_DRAW) == enabled)
			continue;

		material->SetMaterialVarFlag(MATERIAL_VAR_NO_DRAW, enabled);
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

	if (stage == FRAME_NET_UPDATE_POSTDATAUPDATE_START)
		suppress_smoke_effects();

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
		suppress_smoke_effects();

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
					static auto weapon_accuracy_nospread = m_cvar()->FindVar(crypt_str("weapon_accuracy_nospread"));
					const auto no_spread = weapon_accuracy_nospread && weapon_accuracy_nospread->GetBool();

					if (current_shot->impacts)
						lagcompensation::get().resolver_shot_feedback(current_shot->last_target, current_shot->shoot_position, current_shot->impact_position, false, 0);

					// OCCLUSION CHECK FIRST - Must check if autowall actually returned invalid
					if (current_shot->occlusion)
					{
						// BRANCH 1: OCCLUSION MISS (only if autowall said so AND we hit something)
						current_shot->shot_info.result = crypt_str("Occlusion");
						current_shot->shot_info.miss_reason = crypt_str("Occlusion");
						current_shot->shot_info.was_occluded = true;

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
					else if (!current_shot->impact_hit_player && !no_spread && !(current_shot->shot_info.path_deviation >= 0.0f && current_shot->shot_info.path_deviation < 6.0f))
					{
						// BRANCH 2: SPREAD MISS (checked third)
						// No impact hit = pure spread/inaccuracy miss
						current_shot->shot_info.result = crypt_str("Spread");
						current_shot->shot_info.miss_reason = crypt_str("Spread");

						++g_ctx.globals.miss_reason_count[0];

						if (cfg.misc.events_to_log[EVENTLOG_HIT])
						{
							std::stringstream log;
							log << crypt_str("MISS [Spread] - ") << current_shot->shot_info.weapon_name
								<< crypt_str(" | Target: ") << current_shot->shot_info.target_name
								<< crypt_str(" | Dist: ") << std::fixed << std::setprecision(0) << current_shot->shot_info.distance_to_target << crypt_str("u")
								<< crypt_str(" | Lat: ") << current_shot->shot_info.network_latency_ms << crypt_str("ms")
								<< crypt_str(" | HC: ");

							if (current_shot->shot_info.hitchance > 100)
								log << crypt_str("MA");
							else
								log << current_shot->shot_info.hitchance << crypt_str("%");

							log << crypt_str(" | BT: ") << current_shot->shot_info.backtrack_ticks << crypt_str("t")
								<< crypt_str(" | Inaccuracy: ") << std::fixed << std::setprecision(3) << current_shot->shot_info.fire_inaccuracy;

							if (current_shot->shot_info.path_deviation >= 0.0f)
								log << crypt_str(" | Dev: ") << std::fixed << std::setprecision(1) << current_shot->shot_info.path_deviation << crypt_str("u");

							eventlogs::get().add(log.str());
						}
					}
					else if (current_shot->shot_info.backtrack_ticks == 0 && current_shot->target_position_at_fire.DistTo(current_shot->target_position_at_impact) > 16.0f)
					{
						// BRANCH 3: PREDICTION ERROR MISS
						current_shot->shot_info.result = crypt_str("Prediction");
						current_shot->shot_info.miss_reason = crypt_str("Prediction");
						current_shot->shot_info.prediction_error = true;

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
						// BRANCH 4: RESOLVER / BACKTRACK FAILURE / CORRECTION MISS
						std::string miss_reason = crypt_str("Resolver");
						int miss_reason_idx = 4;

						bool is_bot = false;
						auto player = (player_t*)m_entitylist()->GetClientEntity(current_shot->last_target);
						if (player)
						{
							player_info_t info;
							if (m_engine()->GetPlayerInfo(current_shot->last_target, &info) && info.fakeplayer)
								is_bot = true;
						}

						if (is_bot)
						{
							miss_reason = crypt_str("Record mismatch");
							miss_reason_idx = 3;
						}
						else if (current_shot->shot_info.backtrack_ticks > 0)
						{
							miss_reason = crypt_str("Backtrack failure");
							miss_reason_idx = 4;
						}
						else if (current_shot->shot_info.target_animation_sequence > 0 &&
							current_shot->shot_info.target_animation_cycle > 0.0f)
						{
							miss_reason = crypt_str("Correction");
							miss_reason_idx = 3;
							current_shot->shot_info.correction_failed = true;
						}

						current_shot->shot_info.result = miss_reason;
						current_shot->shot_info.miss_reason = miss_reason;
						current_shot->shot_info.resolver_side = current_shot->side;

						if (!is_bot)
							++g_ctx.globals.missed_shots[current_shot->last_target];
						++g_ctx.globals.miss_reason_count[miss_reason_idx];
						if (!is_bot)
						{
							lagcompensation::get().player_resolver[current_shot->last_target].last_side = (resolver_side)current_shot->side;
							lagcompensation::get().resolver_feedback(current_shot->last_target, (resolver_side)current_shot->side, false);
						}

						if (cfg.misc.events_to_log[EVENTLOG_HIT])
						{
							std::stringstream log;
							log << crypt_str("MISS [") << miss_reason << crypt_str("] - ")
								<< current_shot->shot_info.weapon_name << crypt_str(" | Target: ")
								<< current_shot->shot_info.target_name
								<< crypt_str(" | Dist: ") << std::fixed << std::setprecision(0) << current_shot->shot_info.distance_to_target << crypt_str("u")
								<< crypt_str(" | Lat: ") << current_shot->shot_info.network_latency_ms << crypt_str("ms");

							if (!is_bot)
							{
								std::string side_name = crypt_str("Unknown");

								switch (current_shot->side)
								{
								case RESOLVER_ORIGINAL:
									side_name = crypt_str("Original");
									break;
								case RESOLVER_ZERO:
									side_name = crypt_str("Zero");
									break;
								case RESOLVER_FIRST:
									side_name = crypt_str("Positive");
									break;
								case RESOLVER_SECOND:
									side_name = crypt_str("Negative");
									break;
								case RESOLVER_LOW_FIRST:
									side_name = crypt_str("Positive 50%");
									break;
								case RESOLVER_LOW_SECOND:
									side_name = crypt_str("Negative 50%");
									break;
								case RESOLVER_HIGH_FIRST:
									side_name = crypt_str("Positive 75%");
									break;
								case RESOLVER_HIGH_SECOND:
									side_name = crypt_str("Negative 75%");
									break;
								case RESOLVER_DESYNC_FIRST:
									side_name = crypt_str("Positive 25%");
									break;
								case RESOLVER_DESYNC_SECOND:
									side_name = crypt_str("Negative 25%");
									break;
								case RESOLVER_LEFT:
									side_name = crypt_str("Left");
									break;
								case RESOLVER_RIGHT:
									side_name = crypt_str("Right");
									break;
								default:
									break;
								}

								log << crypt_str(" | Resolved Side: ") << side_name;
							}

							log << crypt_str(" | HC: ") << current_shot->shot_info.hitchance << crypt_str("%")
								<< crypt_str(" | BT: ") << current_shot->shot_info.backtrack_ticks << crypt_str("t")
								<< crypt_str(" | SP: ") << (current_shot->shot_info.point_was_safe ? crypt_str("yes") : crypt_str("no"));

							eventlogs::get().add(log.str());
						}
					}
				}
				else if (cfg.misc.events_to_log[EVENTLOG_HIT])
				{
					// Log successful HIT
					++g_ctx.globals.total_shots_hit;  // Track HIT

					std::stringstream log;
					log << crypt_str("HIT [") << current_shot->shot_info.server_hitbox << crypt_str("] / [") << current_shot->shot_info.client_hitbox << crypt_str("] - ")
						<< current_shot->shot_info.weapon_name << crypt_str(" | Target: ")
						<< current_shot->shot_info.target_name
						<< crypt_str(" | [") << current_shot->shot_info.server_damage << crypt_str("] / [") << current_shot->shot_info.client_damage << crypt_str("]")
						<< crypt_str(" | Dist: ") << std::fixed << std::setprecision(0) << current_shot->shot_info.distance_to_target << crypt_str("u")
						<< crypt_str(" | Lat: ") << current_shot->shot_info.network_latency_ms << crypt_str("ms")
						<< crypt_str(" | HC: ") << current_shot->shot_info.hitchance << crypt_str("%")
						<< crypt_str(" | BT: ") << current_shot->shot_info.backtrack_ticks << crypt_str("t");

					eventlogs::get().add(log.str());
				}
			}
			else if (cfg.misc.events_to_log[EVENTLOG_HIT] && !current_shot->hurt_player)
			{
				std::stringstream log;
				log << crypt_str("MISS [Timeout] - ") << current_shot->shot_info.weapon_name
					<< crypt_str(" | Target: ") << current_shot->shot_info.target_name
					<< crypt_str(" | Dist: ") << std::fixed << std::setprecision(0) << current_shot->shot_info.distance_to_target << crypt_str("u")
					<< crypt_str(" | Lat: ") << current_shot->shot_info.network_latency_ms << crypt_str("ms")
					<< crypt_str(" | HC: ") << current_shot->shot_info.hitchance << crypt_str("%")
					<< crypt_str(" | BT: ") << current_shot->shot_info.backtrack_ticks << crypt_str("t");

				eventlogs::get().add(log.str());
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
