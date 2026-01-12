#include "..\hooks.hpp"
#include "..\..\menu\UI.h"
#include "..\..\features\lagcompensation\animation_system.h"
#include "..\..\features\visuals\playeresp.h"
#include "..\..\features\visuals\otheresp.h"
#include "..\..\features\misc\logs.h"
#include "..\..\features\visuals\worldesp.h"
#include "..\..\features\misc\misc.h"
#include "..\..\features\visuals\GrenadePrediction.h"
#include "..\..\features\visuals\bullettracers.h"
#include "..\..\features\visuals\dormantesp.h"
#include "..\..\features\lagcompensation\local_animations.h"
#include "..\..\Features\Visuals\MiscEsp.h"

using PaintTraverse_t = void(__thiscall*)(void*, vgui::VPANEL, bool, bool);

bool reload_fonts()
{
	static int old_width, old_height;
	static int width, height;

	m_engine()->GetScreenSize(width, height);

	if (width != old_width || height != old_height)
	{
		old_width = width;
		old_height = height;

		return true;
	}

	return false;
}

void __fastcall hooks::hooked_painttraverse(void* ecx, void* edx, vgui::VPANEL panel, bool force_repaint, bool allow_force)
{
	static auto original_fn = panel_hook->get_func_address <PaintTraverse_t>(41);
	g_ctx.local((player_t*)m_entitylist()->GetClientEntity(m_engine()->GetLocalPlayer()), true);

	static auto set_console = true;

	if (set_console)
	{
		set_console = false;

		m_cvar()->FindVar(crypt_str("developer"))->SetValue(FALSE);
		m_cvar()->FindVar(crypt_str("con_filter_enable"))->SetValue(TRUE);
		m_cvar()->FindVar(crypt_str("con_filter_text"))->SetValue(crypt_str(""));
		m_engine()->ExecuteClientCmd(crypt_str("clear"));
		m_cvar()->ConsoleColorPrintf(Color(100, 125, 200), "Lambda - Beta version. \n");
		m_cvar()->ConsoleColorPrintf(Color::LightBlue, "Last build: " __DATE__ " / " __TIME__);
		m_cvar()->ConsoleColorPrintf(Color(100, 125, 200), "\n");
	}

	static auto log_value = true;

	if (log_value != cfg.misc.show_default_log)
	{
		log_value = cfg.misc.show_default_log;

		if (log_value)
			m_cvar()->FindVar(crypt_str("con_filter_text"))->SetValue(crypt_str(""));
		else
			m_cvar()->FindVar(crypt_str("con_filter_text"))->SetValue(crypt_str("IrWL5106TZZKNFPz4P4Gl3pSN?J370f5hi373ZjPg%VOVh6lN"));
	}

	static vgui::VPANEL panel_id = 0;
	static auto in_game = false;

	if (!in_game && m_engine()->IsInGame())
	{
		in_game = true;

		for (auto i = 1; i < 65; i++)
		{
			g_ctx.globals.fired_shots[i] = 0;
			g_ctx.globals.missed_shots[i] = 0;
			player_records[i].clear();
			lagcompensation::get().is_dormant[i] = false;
			playeresp::get().esp_alpha_fade[i] = 0.0f;
			playeresp::get().health[i] = 100;
			c_dormant_esp::get().m_cSoundPlayers[i].reset();
			otheresp::get().damage_marker[i].reset();
		}

		antiaim::get().freeze_check = false;
		g_ctx.globals.next_lby_update = FLT_MIN;
		g_ctx.globals.last_lby_move = FLT_MIN;
		g_ctx.globals.last_aimbot_shot = 0;
		g_ctx.globals.bomb_timer_enable = true;
		g_ctx.globals.backup_model = false;
		g_ctx.globals.should_remove_smoke = false;
		g_ctx.globals.should_update_beam_index = true;
		g_ctx.globals.should_update_playerresource = true;
		g_ctx.globals.should_update_gamerules = true;
		g_ctx.globals.should_update_radar = true;
		g_ctx.globals.kills = 0;
		g_ctx.shots.clear();
		g_ctx.globals.commands.clear();
		SkinChanger::model_indexes.clear();
		SkinChanger::player_model_indexes.clear();
	}
	else if (in_game && !m_engine()->IsInGame())
	{
		in_game = false;

		g_ctx.globals.should_update_weather = true;
		g_ctx.globals.m_networkable = nullptr;

		misc::get().double_tap_enabled = false;
		misc::get().double_tap_key = false;

		misc::get().hide_shots_enabled = false;
		misc::get().hide_shots_key = false;

		// Save tracking statistics when leaving game
		if (g_ctx.globals.total_shots_fired > 0)
		{
			// Create tracking directory in game csgo folder
			const char* tracking_dir = "csgo\\Lambda\\Tracking";
			CreateDirectoryA(tracking_dir, nullptr);

			// Generate timestamped filename
			time_t now = time(nullptr);
			struct tm* time_info = localtime(&now);
			char filename[256];
			strftime(filename, sizeof(filename), "csgo\\Lambda\\Tracking\\%Y-%m-%d_%H-%M-%S.txt", time_info);

			// Open file for writing
			FILE* file = fopen(filename, "w");
			if (file)
			{
				// Calculate accuracy
				int total_misses = 0;
				for (int i = 0; i < 6; i++)  // Changed from 5 to 6 to include Ping
					total_misses += g_ctx.globals.miss_reason_count[i];

				float accuracy = (g_ctx.globals.total_shots_hit * 100.0f) / (float)g_ctx.globals.total_shots_fired;

				// Write header
				fprintf(file, "====================================\n");
				fprintf(file, "LAMBDA SHOT STATISTICS\n");
				fprintf(file, "====================================\n\n");
				fprintf(file, "Map:     %s\n", g_ctx.globals.current_map.c_str());
				fprintf(file, "Accuracy: %.1f%% (%d/%d)\n\n", accuracy, g_ctx.globals.total_shots_hit, g_ctx.globals.total_shots_fired);

				// Write miss breakdown
				fprintf(file, "MISS REASONS:\n");
				fprintf(file, "  Spread:     %d\n", g_ctx.globals.miss_reason_count[0]);
				fprintf(file, "  Occlusion:  %d\n", g_ctx.globals.miss_reason_count[1]);
				fprintf(file, "  Prediction: %d\n", g_ctx.globals.miss_reason_count[2]);
				fprintf(file, "  Correction: %d\n", g_ctx.globals.miss_reason_count[3]);
				fprintf(file, "  Resolver:   %d\n", g_ctx.globals.miss_reason_count[4]);
				fprintf(file, "  Ping:       %d\n\n", g_ctx.globals.miss_reason_count[5]);

				fprintf(file, "====================================\n");

				fclose(file);
			}

			// Reset for next session
			for (int i = 0; i < 6; i++)  // Changed from 5 to 6 to include Ping
				g_ctx.globals.miss_reason_count[i] = 0;
			g_ctx.globals.total_shots_fired = 0;
			g_ctx.globals.total_shots_hit = 0;
			g_ctx.globals.current_map = crypt_str("unknown");
		}
	}

	static uint32_t HudZoomPanel = 0;

	if (!HudZoomPanel)
		if (!strcmp(crypt_str("HudZoom"), m_panel()->GetName(panel)))
			HudZoomPanel = panel;

	if (HudZoomPanel == panel && cfg.player.enable && cfg.esp.removals[REMOVALS_SCOPE])
		return;

	original_fn(ecx, panel, force_repaint, allow_force);

	if (!panel_id)
	{
		auto panelName = m_panel()->GetName(panel);

		if (!strcmp(panelName, crypt_str("MatSystemTopPanel")))
			panel_id = panel;
	}

	if (reload_fonts())
	{
		static auto create_font = [](const char* name, int size, int weight, DWORD flags) -> vgui::HFont
			{
				g_ctx.last_font_name = name;

				auto font = m_surface()->FontCreate();
				m_surface()->SetFontGlyphSet(font, name, size, weight, NULL, NULL, flags);

				return font;
			};

		fonts[LOGS] = create_font(crypt_str("Raleway"), 15, FW_DONTCARE, FONTFLAG_ANTIALIAS);
		fonts[ESP] = create_font(crypt_str("Smallest Pixel-7"), 11, FW_MEDIUM, FONTFLAG_OUTLINE);
		fonts[NAME] = create_font(crypt_str("Square Sans Serif 7"), 11, FW_MEDIUM, FONTFLAG_ANTIALIAS | FONTFLAG_DROPSHADOW);
		fonts[SUBTABWEAPONS] = create_font(crypt_str("undefeated"), 13, FW_DONTCARE, FONTFLAG_ANTIALIAS);
		fonts[KNIFES] = create_font(crypt_str("icomoon"), 13, FW_DONTCARE, FONTFLAG_ANTIALIAS);
		fonts[GRENADES] = create_font(crypt_str("undefeated"), 20, FW_MEDIUM, FONTFLAG_OUTLINE);
		fonts[INDICATORFONT] = create_font(crypt_str("Verdana"), 25, FW_HEAVY, FONTFLAG_ANTIALIAS | FONTFLAG_DROPSHADOW);
		fonts[DAMAGE_MARKER] = create_font(crypt_str("Raleway"), 17, FW_MEDIUM, FONTFLAG_ANTIALIAS);
		fonts[WATERMARK] = create_font(crypt_str("Raleway"), 12, FW_EXTRALIGHT, FONTFLAG_NONE | FONTFLAG_NONE);
		fonts[SEMXXZ] = create_font(crypt_str("Calibri Bold"), 26, FW_LIGHT, FONTFLAG_ANTIALIAS | FONTFLAG_DROPSHADOW);
		fonts[HUETA] = create_font(crypt_str("Calibri Bold"), 17, FW_LIGHT, FONTFLAG_ANTIALIAS | FONTFLAG_DROPSHADOW);
		g_ctx.last_font_name.clear();
	}

	if (panel_id == panel)
	{
		if (g_ctx.available())
		{
			static auto alive = false;

			if (!alive && g_ctx.local()->is_alive())
			{
				alive = true;
				g_ctx.globals.should_clear_death_notices = true;
			}
			else if (alive && !g_ctx.local()->is_alive())
			{
				alive = false;

				for (auto i = 1; i < m_globals()->m_maxclients; i++)
				{
					g_ctx.globals.fired_shots[i] = 0;
					g_ctx.globals.missed_shots[i] = 0;
				}

				local_animations::get().local_data.prediction_animstate = nullptr;
				local_animations::get().local_data.animstate = nullptr;

				g_ctx.globals.weapon = nullptr;
				g_ctx.globals.should_choke_packet = false;
				g_ctx.globals.should_send_packet = false;
				g_ctx.globals.kills = 0;
				g_ctx.globals.should_buy = 3;
			}

			g_ctx.globals.bomb_carrier = -1;

			misc::get().desync_arrows();

			auto weapon = g_ctx.local()->m_hActiveWeapon().Get();

			if (cfg.player.enable && cfg.esp.removals[REMOVALS_SCOPE] && g_ctx.globals.scoped && weapon->is_sniper() && cfg.esp.scopee)
			{

			}
			else if (cfg.player.enable && cfg.esp.removals[REMOVALS_SCOPE] && g_ctx.globals.scoped && weapon->is_sniper())
			{
				static int w, h;
				m_engine()->GetScreenSize(w, h);

				render::get().line(w / 2, 0, w / 2, h, Color::Black);
				render::get().line(0, h / 2, w, h / 2, Color::Black);
			}

			if (g_ctx.local()->is_alive())
			{
				SkinChanger::InitCustomModels();
			}

			bullettracers::get().draw_beams();
		}

		static auto framerate = 0.0f;
		framerate = 0.9f * framerate + 0.1f * m_globals()->m_absoluteframetime;

		if (framerate <= 0.0f)
			framerate = 1.0f;

		g_ctx.globals.framerate = (int)(1.0f / framerate);
		auto nci = m_engine()->GetNetChannelInfo();

		if (nci)
		{
			auto latency = m_engine()->IsPlayingDemo() ? 0.0f : nci->GetAvgLatency(FLOW_OUTGOING);

			if (latency)
			{
				static auto cl_updaterate = m_cvar()->FindVar(crypt_str("cl_updaterate"));
				latency -= 0.5f / cl_updaterate->GetFloat();
			}

			g_ctx.globals.ping = (int)(max(0.0f, latency) * 1000.0f);
		}

		time_t lt;
		struct tm* t_m;

		lt = time(nullptr);
		t_m = localtime(&lt);

		auto time_h = t_m->tm_hour;
		auto time_m = t_m->tm_min;
		auto time_s = t_m->tm_sec;

		std::string time;

		if (time_h < 10)
			time += "0";

		time += std::to_string(time_h) + ":";

		if (time_m < 10)
			time += "0";

		time += std::to_string(time_m) + ":";

		if (time_s < 10)
			time += "0";

		time += std::to_string(time_s);
		g_ctx.globals.time = std::move(time);

		static int w, h;
		m_engine()->GetScreenSize(w, h);

		static auto alpha = 0;
		auto speed = 800.0f * m_globals()->m_frametime;

		if (menu_open)
		{
			if (alpha + speed <= 150)
				alpha += speed;

			render::get().rect_filled(0, 0, w, h, Color(0, 0, 0, alpha));
		}
		else
		{
			if (alpha - speed >= 0)
				alpha -= speed;

			render::get().rect_filled(0, 0, w, h, Color(0, 0, 0, alpha));
		}

		static std::vector <const char*> molly_materials =
		{
			"particle/fire_burning_character/fire_env_fire.vmt",
			"particle/fire_burning_character/fire_env_fire_depthblend.vmt",
			"particle/fire_burning_character/fire_burning_character_depthblend.vmt",
			"particle/fire_burning_character/fire_burning_character.vmt",
			"particle/fire_burning_character/fire_burning_character_nodepth.vmt",
			"particle/particle_flares/particle_flare_001.vmt",
			"particle/particle_flares/particle_flare_004.vmt",
			"particle/particle_flares/particle_flare_004b_mod_ob.vmt",
			"particle/particle_flares/particle_flare_004b_mod_z.vmt",
			"particle/fire_explosion_1/fire_explosion_1_bright.vmt",
			"particle/fire_explosion_1/fire_explosion_1b.vmt",
			"particle/fire_particle_4/fire_particle_4.vmt",
			"particle/fire_explosion_1/fire_explosion_1_oriented.vmt"
		};

		for (auto material_name : molly_materials)
		{
			auto material = m_materialsystem()->FindMaterial(material_name, nullptr);

			if (!material)
				continue;

			material->SetMaterialVarFlag(MATERIAL_VAR_WIREFRAME, true);
		}

		//misc::get().watermark();
		misc::get().NightmodeFix();
		otheresp::get().indicators();
		otheresp::get().draw_indicators();
		//otheresp::get().peek_indicator();
	}
}