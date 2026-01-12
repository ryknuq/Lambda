#include "misc.h"
#include "fakelag.h"
#include "..\ragebot\aim.h"
#include "..\visuals\worldesp.h"
#include "prediction_system.h"
#include "logs.h"
#include "../../../menu/UI.h"
#include "../../utils/Render/Render.h"
#include "../Misc/Fakelag.h"
#include <imguiCustom.h>

void misc::watermark()
{
	if (!cfg.menu.watermark)
		return;

	ImVec2 p;
	ImVec2 size_menu;

	if (ImGui::Begin("Watermark", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar))
	{
		size_menu = ImGui::GetWindowSize();
		p = ImGui::GetWindowPos();
		auto draw = ImGui::GetWindowDrawList();
		ImGuiStyle& style = ImGui::GetStyle();

		std::string water = "Lambda | beta | semxxz | at game";
		draw->AddRectFilled(ImVec2(p.x + 250, p.y + 30), ImVec2(p.x + 0, p.y + 0), ImColor(0, 0, 0, 120), 0, 15);
		draw->AddLine(ImVec2(p.x + 250, p.y + 1), ImVec2(p.x + 0, p.y + 0), ImColor(47, 151, 232, 255), 3.000000);
		ImGui::PushFont(c_menu::get().gotham);
		draw->AddText(ImVec2(p.x + 10, p.y + 9), ImColor(193, 193, 194), water.c_str());

		ImGui::SetWindowSize(ImVec2(220, 25 ));
		ImGui::SetCursorPosY(25);
		ImGui::Columns(2, "fart1", false);
		ImGui::Columns(1);
		ImGui::PopFont();
	}
	ImGui::End();
}

void misc::keybinds()
{
	if (!cfg.menu.keybinds)
		return;

	const auto freq = 1.f;
	const auto real_time = m_globals()->m_realtime * freq;
	const int r = std::floor(std::sin(real_time + 0.f) * 127.f + 128.f);
	const int g = std::floor(std::sin(real_time + 2.f) * 127.f + 128.f);
	const int b = std::floor(std::sin(real_time + 4.f) * 127.f + 128.f);

	int specs = 0;
	int modes = 0;
	std::string spect = "";
	std::string mode = "";

	if (cfg.ragebot.double_tap && cfg.ragebot.double_tap_key.key > KEY_NONE && cfg.ragebot.double_tap_key.key < KEY_MAX && misc::get().double_tap_key) {
		spect += crypt_str("Doubletap");
		spect += crypt_str("\n");
		specs++;

		switch (cfg.ragebot.double_tap_key.mode) {
		case 0: {
			mode += crypt_str("[holding]  ");
		}break;
		case 1: {
			mode += crypt_str("[toggled]  ");
		}break;
		}
		mode += crypt_str("\n");
		modes++;
	}

	if (key_binds::get().get_key_bind_state(3)) {
		spect += crypt_str("Safe point");
		spect += crypt_str("\n");
		specs++;

		switch (cfg.ragebot.safe_point_key.mode) {
		case 0: {
			mode += crypt_str("[holding]  ");
		}break;
		case 1: {
			mode += crypt_str("[toggled]  ");
		}break;
		}
		mode += crypt_str("\n");
		modes++;
	}

	if (g_ctx.globals.current_weapon != -1 && key_binds::get().get_key_bind_state(4 + g_ctx.globals.current_weapon))
	{
		spect += crypt_str("Damage override");
		spect += crypt_str("\n");
		specs++;

		switch (cfg.ragebot.safe_point_key.mode) {
		case 0: {
			mode += crypt_str("[holding]  ");
		}break;
		case 1: {
			mode += crypt_str("[toggled]  ");
		}break;
		}
		mode += crypt_str("\n");
		modes++;
	}

	if (cfg.antiaim.hide_shots && cfg.antiaim.hide_shots_key.key > KEY_NONE && cfg.antiaim.hide_shots_key.key < KEY_MAX && misc::get().hide_shots_key) {
		spect += crypt_str("Hide shots");
		spect += crypt_str("\n");
		specs++;

		switch (cfg.antiaim.hide_shots_key.mode) {
		case 0: {
			mode += crypt_str("[holding]  ");
		}break;
		case 1: {
			mode += crypt_str("[toggled]  ");
		}break;
		}
		mode += crypt_str("\n");
		modes++;
	}

	if (key_binds::get().get_key_bind_state(16)) {
		spect += crypt_str("Flip desync");
		spect += crypt_str("\n");
		specs++;

		switch (cfg.antiaim.flip_desync.mode) {
		case 0: {
			mode += crypt_str("[holding]  ");
		}break;
		case 1: {
			mode += crypt_str("[toggled]  ");
		}break;
		}
		mode += crypt_str("\n");
		modes++;
	}

	if (key_binds::get().get_key_bind_state(17)) {
		spect += crypt_str("Thirdperson");
		spect += crypt_str("\n");
		specs++;

		switch (cfg.misc.thirdperson_toggle.mode) {
		case 0: {
			mode += crypt_str("[holding]  ");
		}break;
		case 1: {
			mode += crypt_str("[toggled]  ");
		}break;
		}
		mode += crypt_str("\n");
		modes++;
	}

	if (key_binds::get().get_key_bind_state(18)) {
		spect += crypt_str("Auto peek");
		spect += crypt_str("\n");
		specs++;

		switch (cfg.misc.automatic_peek.mode) {
		case 0: {
			mode += crypt_str("[holding]  ");
		}break;
		case 1: {
			mode += crypt_str("[toggled]  ");
		}break;
		}
		mode += crypt_str("\n");
		modes++;
	}

	if (key_binds::get().get_key_bind_state(19)) {
		spect += crypt_str("Edge jump");
		spect += crypt_str("\n");
		specs++;

		switch (cfg.misc.edge_jump.mode) {
		case 0: {
			mode += crypt_str("[holding]  ");
		}break;
		case 1: {
			mode += crypt_str("[toggled]  ");
		}break;
		}
		mode += crypt_str("\n");
		modes++;
	}

	if (key_binds::get().get_key_bind_state(20)) {
		spect += crypt_str("Fake duck");
		spect += crypt_str("\n");
		specs++;

		switch (cfg.misc.fakeduck_key.mode) {
		case 0: {
			mode += crypt_str("[holding]  ");
		}break;
		case 1: {
			mode += crypt_str("[toggled]  ");
		}break;
		}
		mode += crypt_str("\n");
		modes++;
	}

	if (key_binds::get().get_key_bind_state(21)) {
		spect += crypt_str("Slow walk");
		spect += crypt_str("\n");
		specs++;

		switch (cfg.misc.slowwalk_key.mode) {
		case 0: {
			mode += crypt_str("[holding]  ");
		}break;
		case 1: {
			mode += crypt_str("[toggled]  ");
		}break;
		}
		mode += crypt_str("\n");
		modes++;
	}

	if (key_binds::get().get_key_bind_state(22)) {
		spect += crypt_str("Body aim");
		spect += crypt_str("\n");
		specs++;

		switch (cfg.ragebot.body_aim_key.mode) {
		case 0: {
			mode += crypt_str("[holding]  ");
		}break;
		case 1: {
			mode += crypt_str("[toggled]  ");
		}break;
		}
		mode += crypt_str("\n");
		modes++;
	}

	ImVec2 p;
	ImVec2 size_menu;

	static bool window_set = false;
	float speed = 10;
	static bool finish = false;
	static bool other_bind_pressed = false;
	static int sub_tabs = false;

	if (key_binds::get().get_key_bind_state(4 + hooks::rage_weapon) || key_binds::get().get_key_bind_state(16) || key_binds::get().get_key_bind_state(18) || key_binds::get().get_key_bind_state(20)
		|| key_binds::get().get_key_bind_state(21) || key_binds::get().get_key_bind_state(17) || key_binds::get().get_key_bind_state(22) || key_binds::get().get_key_bind_state(13) || key_binds::get().get_key_bind_state(14) || key_binds::get().get_key_bind_state(15)
		|| misc::get().double_tap_key || misc::get().hide_shots_key || key_binds::get().get_key_bind_state(0) || key_binds::get().get_key_bind_state(3) || key_binds::get().get_key_bind_state(23))
		other_bind_pressed = true;
	else
		other_bind_pressed = false;

	if (cfg.menu.windowsize_x_saved != cfg.menu.oldwindowsize_x_saved)
	{
		if (cfg.menu.windowsize_x_saved > cfg.menu.oldwindowsize_x_saved)
			cfg.menu.windowsize_x_saved -= cfg.menu.speed;

		if (cfg.menu.windowsize_x_saved < cfg.menu.oldwindowsize_x_saved)
			cfg.menu.windowsize_x_saved += cfg.menu.speed;
	}

	if (cfg.menu.windowsize_y_saved != cfg.menu.oldwindowsize_y_saved)
	{
		if (cfg.menu.windowsize_y_saved > cfg.menu.oldwindowsize_y_saved)
			cfg.menu.windowsize_y_saved -= cfg.menu.speed;

		if (cfg.menu.windowsize_y_saved < cfg.menu.oldwindowsize_y_saved)
			cfg.menu.windowsize_y_saved += cfg.menu.speed;
	}

	if (cfg.menu.windowsize_x_saved == cfg.menu.oldwindowsize_x_saved && cfg.menu.windowsize_y_saved == cfg.menu.oldwindowsize_y_saved)
		finish = true;
	else
		finish = false;

	if (!cfg.antiaim.flip_desync.key || !cfg.misc.automatic_peek.key || !cfg.misc.fakeduck_key.key || !cfg.misc.slowwalk_key.key || !cfg.ragebot.double_tap_key.key || !cfg.misc.fakeduck_key.key || !cfg.misc.thirdperson_toggle.key || !hooks::menu_open)
	{
		cfg.menu.windowsize_x_saved = 0;
		cfg.menu.windowsize_y_saved = 0;
	}

	static float alpha_menu = 0.1f;

	if (other_bind_pressed)
		if (alpha_menu < 1.f)
			alpha_menu += 0.05f;
		else
			if (alpha_menu > 0.1f)
				alpha_menu -= 0.05f;

	bool theme = true;

	if ((cfg.menu.keybinds))
	{
		if ((other_bind_pressed && alpha_menu > 0.1f) || hooks::menu_open)
		{
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha_menu);
			if (ImGui::Begin("Binds", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar))
			{
				auto b_alpha = alpha_menu;
				size_menu = ImGui::GetWindowSize();
				p = ImGui::GetWindowPos();
				auto draw = ImGui::GetWindowDrawList();
				std::string keybinds = "Keybinds";

				draw->AddRectFilled(ImVec2(p.x + 250, p.y + 25), ImVec2(p.x + 0, p.y + 0), ImColor(0, 0, 0, 120), 0, 15);
				draw->AddLine(ImVec2(p.x + 250, p.y + 1), ImVec2(p.x + 0, p.y + 0), ImColor(47, 151, 232, 255), 3.000000);
				ImGui::PushFont(c_menu::get().gotham);
				draw->AddText(ImVec2(p.x + 77, p.y + 9), ImColor(193, 193, 194), keybinds.c_str());

				if (specs > 0) spect += "\n";
				if (modes > 0) mode += "\n";
				ImVec2 size = ImGui::CalcTextSize(spect.c_str());
				ImVec2 size2 = ImGui::CalcTextSize(mode.c_str());
				ImGui::SetWindowSize(ImVec2(220, 25 + size.y));
				ImGui::SetCursorPosY(25);
				ImGui::Columns(2, "fart1", false);

				ImGui::SetColumnWidth(0, 220 - (size2.x + 8));
				ImGui::TextColored(ImVec4(1.f, 1.f, 1.f, alpha_menu), spect.c_str());
				ImGui::NextColumn();

				ImGui::TextColored(ImVec4(1.f, 1.f, 1.f, alpha_menu), mode.c_str());
				ImGui::Columns(1);
				ImGui::PopFont();
			}
			ImGui::End();
			ImGui::PopStyleVar();
		}
	}
}

void misc::spectators_list()
{
	if (!cfg.misc.spectators_list)
		return;

	const auto freq = 1.f;

	const auto real_time = m_globals()->m_realtime * freq;
	const int r = std::floor(std::sin(real_time + 0.f) * 127.f + 128.f);
	const int g = std::floor(std::sin(real_time + 2.f) * 127.f + 128.f);
	const int b = std::floor(std::sin(real_time + 4.f) * 127.f + 128.f);

	std::vector <std::string> spectators;

	int specs = 0;
	int modes = 0;
	std::string spect = "";
	std::string mode = "";
	ImVec2 p;
	ImVec2 size_menu;

	static bool window_set = false;
	static float windowsize_x = 0;
	static float windowsize_y = 0;
	static float oldwindowsize_x = 300;
	static float oldwindowsize_y = 300;
	static float speed = 10;
	static bool finish = false;
	static int sub_tabs = false;
	static float alpha_menu = 0.1f;


	if (windowsize_x != oldwindowsize_x)
	{
		if (windowsize_x > oldwindowsize_x)
			windowsize_x -= speed;

		if (windowsize_x < oldwindowsize_x)
			windowsize_x += speed;
	}
	if (windowsize_y != oldwindowsize_y)
	{
		if (windowsize_y > oldwindowsize_y)
			windowsize_y -= speed;

		if (windowsize_y < oldwindowsize_y)
			windowsize_y += speed;
	}

	if (windowsize_x == oldwindowsize_x && windowsize_y == oldwindowsize_y)
		finish = true;
	else
		finish = false;

	for (int i = 1; i < m_globals()->m_maxclients; i++)
	{
		auto e = static_cast<player_t*>(m_entitylist()->GetClientEntity(i));

		if (!e)
			continue;

		if (e->is_alive())
			continue;

		if (e->IsDormant())
			continue;

		if (e->m_hObserverTarget().Get() != g_ctx.local())
			continue;

		player_info_t player_info;
		m_engine()->GetPlayerInfo(i, &player_info);
		spect += player_info.szName;
		spect += "\n";
		specs++;
	}

	bool theme = true;

	if ((spect.size() > 0) || hooks::menu_open)
		if (alpha_menu < 1.f)
			alpha_menu += 0.05f;
		else
			if (alpha_menu > 0.1f)
				alpha_menu -= 0.05f;

	if ((cfg.misc.spectators_list))
	{
		if ((spect.size() > 0 && alpha_menu > 0.1f) || hooks::menu_open)
		{
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha_menu);
			if (ImGui::Begin("Spectators", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar))
			{
				auto b_alpha = alpha_menu;
				size_menu = ImGui::GetWindowSize();
				p = ImGui::GetWindowPos();
				auto draw = ImGui::GetWindowDrawList();

				std::vector<std::string> keybind = { "Spectators" };
				draw->AddRectFilled({ p.x + 0, p.y + 0 }, { p.x + 220, p.y + 25 }, ImColor(15, 15, 15));

				draw->AddRectFilledMultiColor({ p.x + 0, p.y + 2 }, { p.x + 220, p.y + 5 }, ImColor(15, 15, 15), ImColor(15, 15, 15), ImColor(15, 15, 15, 25), ImColor(15, 15, 15, 25));
				draw->AddRectFilled({ p.x + 0, p.y + 0 }, { p.x + 220, p.y + 4 }, ImColor(15, 15, 15), 6);

				ImGui::PushFont(c_menu::get().g_pMenuFont);
				draw->AddText(ImVec2(p.x + 80, p.y + 5), ImColor(200, 200, 200), keybind.at(0).c_str());

				if (specs > 0) spect += "\n";
				if (modes > 0) mode += "\n";
				ImVec2 size = ImGui::CalcTextSize(spect.c_str());
				ImVec2 size2 = ImGui::CalcTextSize(mode.c_str());
				ImGui::SetWindowSize(ImVec2(220, 25 + size.y));
				ImGui::SetCursorPosY(25);
				ImGui::Columns(2, "fart1", false);

				ImGui::SetColumnWidth(0, 220 - (size2.x + 8));
				ImGui::TextColored(ImVec4(1.f, 1.f, 1.f, alpha_menu), spect.c_str());
				ImGui::NextColumn();

				ImGui::TextColored(ImVec4(1.f, 1.f, 1.f, alpha_menu), mode.c_str());
				ImGui::Columns(1);
				ImGui::PopFont();
			}
			ImGui::End();
			ImGui::PopStyleVar();
		}
	}
}

void misc::NoDuck(CUserCmd* cmd)
{
	if (!cfg.misc.noduck)
		return;

	if (m_gamerules()->m_bIsValveDS())
		return;

	cmd->m_buttons |= IN_BULLRUSH;
}

void misc::AutoCrouch(CUserCmd* cmd)
{
	if (fakelag::get().condition)
	{
		g_ctx.globals.fakeducking = false;
		return;
	}

	if (!(g_ctx.local()->m_fFlags() & FL_ONGROUND && engineprediction::get().backup_data.flags & FL_ONGROUND))
	{
		g_ctx.globals.fakeducking = false;
		return;
	}

	if (m_gamerules()->m_bIsValveDS())
	{
		g_ctx.globals.fakeducking = false;
		return;
	}

	if (!key_binds::get().get_key_bind_state(20))
	{
		g_ctx.globals.fakeducking = false;
		return;
	}

	if (!g_ctx.globals.fakeducking && m_clientstate()->iChokedCommands != 7)
		return;

	if (m_clientstate()->iChokedCommands >= 7)
		cmd->m_buttons |= IN_DUCK;
	else
		cmd->m_buttons &= ~IN_DUCK;

	g_ctx.globals.fakeducking = true;
}

void misc::SlideWalk(CUserCmd* cmd) 
{
	if (!g_ctx.local()->is_alive()) //-V807
		return;

	if (g_ctx.local()->get_move_type() == MOVETYPE_LADDER)
		return;

	if (!(g_ctx.local()->m_fFlags() & FL_ONGROUND && engineprediction::get().backup_data.flags & FL_ONGROUND))
		return;

	if (antiaim::get().condition(cmd, true) && cfg.misc.slidewalk)
	{
		if (cmd->m_forwardmove > 0.0f)
		{
			cmd->m_buttons |= IN_BACK;
			cmd->m_buttons &= ~IN_FORWARD;
		}
		else if (cmd->m_forwardmove < 0.0f)
		{
			cmd->m_buttons |= IN_FORWARD;
			cmd->m_buttons &= ~IN_BACK;
		}

		if (cmd->m_sidemove > 0.0f)
		{
			cmd->m_buttons |= IN_MOVELEFT;
			cmd->m_buttons &= ~IN_MOVERIGHT;
		}
		else if (cmd->m_sidemove < 0.0f)
		{
			cmd->m_buttons |= IN_MOVERIGHT;
			cmd->m_buttons &= ~IN_MOVELEFT;
		}
	}
	else
	{
		auto buttons = cmd->m_buttons & ~(IN_MOVERIGHT | IN_MOVELEFT | IN_BACK | IN_FORWARD);

		if (cfg.misc.slidewalk)
		{
			if (cmd->m_forwardmove <= 0.0f)
				buttons |= IN_BACK;
			else
				buttons |= IN_FORWARD;

			if (cmd->m_sidemove > 0.0f)
				goto LABEL_15;
			else if (cmd->m_sidemove >= 0.0f)
				goto LABEL_18;

			goto LABEL_17;
		}
		else
			goto LABEL_18;

		if (cmd->m_forwardmove <= 0.0f) //-V779
			buttons |= IN_FORWARD;
		else
			buttons |= IN_BACK;

		if (cmd->m_sidemove > 0.0f)
		{
		LABEL_17:
			buttons |= IN_MOVELEFT;
			goto LABEL_18;
		}

		if (cmd->m_sidemove < 0.0f)
			LABEL_15:

		buttons |= IN_MOVERIGHT;

	LABEL_18:
		cmd->m_buttons = buttons;
	}
}

void misc::slowwalk_movement(CUserCmd* cmd)
{
	if (!g_ctx.local() || !g_ctx.local()->is_alive())
		return;

	if (g_ctx.local()->get_move_type() == MOVETYPE_LADDER)
		return;

	if (!(g_ctx.local()->m_fFlags() & FL_ONGROUND && engineprediction::get().backup_data.flags & FL_ONGROUND))
		return;

	if (!key_binds::get().get_key_bind_state(21))
		return;

	// Only apply slowwalk if moving
	if (cmd->m_forwardmove == 0.0f && cmd->m_sidemove == 0.0f)
		return;

	// Scale movement commands to ~65% of max speed (noticeable but not slow)
	const float SLOWWALK_SCALE = 0.65f;

	cmd->m_forwardmove *= SLOWWALK_SCALE;
	cmd->m_sidemove *= SLOWWALK_SCALE;
}

void misc::automatic_peek(CUserCmd* cmd, float wish_yaw)
{
	if (!g_ctx.globals.weapon->is_non_aim() && key_binds::get().get_key_bind_state(18))
	{
		if (g_ctx.globals.start_position.IsZero())
		{
			g_ctx.globals.start_position = g_ctx.local()->GetAbsOrigin();

			if (!(engineprediction::get().backup_data.flags & FL_ONGROUND))
			{
				Ray_t ray;
				CTraceFilterWorldAndPropsOnly filter;
				CGameTrace trace;

				ray.Init(g_ctx.globals.start_position, g_ctx.globals.start_position - Vector(0.0f, 0.0f, 1000.0f));
				m_trace()->TraceRay(ray, MASK_SOLID, &filter, &trace);
				
				if (trace.fraction < 1.0f)
					g_ctx.globals.start_position = trace.endpos + Vector(0.0f, 0.0f, 2.0f);
			}
		}
		else
		{
			auto revolver_shoot = g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_REVOLVER && !g_ctx.globals.revolver_working && (cmd->m_buttons & IN_ATTACK || cmd->m_buttons & IN_ATTACK2);

			if (cmd->m_buttons & IN_ATTACK && g_ctx.globals.weapon->m_iItemDefinitionIndex() != WEAPON_REVOLVER || revolver_shoot)
				g_ctx.globals.fired_shot = true;

			if (g_ctx.globals.fired_shot)
			{
				auto current_position = g_ctx.local()->GetAbsOrigin();
				auto difference = current_position - g_ctx.globals.start_position;

				if (difference.Length2D() > 5.0f)
				{
					auto velocity = Vector(difference.x * cos(wish_yaw / 180.0f * M_PI) + difference.y * sin(wish_yaw / 180.0f * M_PI), difference.y * cos(wish_yaw / 180.0f * M_PI) - difference.x * sin(wish_yaw / 180.0f * M_PI), difference.z);

					cmd->m_forwardmove = -velocity.x * 20.0f;
					cmd->m_sidemove = velocity.y * 20.0f;
				}
				else
				{
					g_ctx.globals.fired_shot = false;
					g_ctx.globals.start_position.Zero();
				}
			}
		}
	}
	else
	{
		g_ctx.globals.fired_shot = false;
		g_ctx.globals.start_position.Zero();
	}
}

void misc::ViewModel()
{
	if (cfg.esp.viewmodel_fov)
	{
		auto viewFOV = (float)cfg.esp.viewmodel_fov + 68.0f;
		static auto viewFOVcvar = m_cvar()->FindVar(crypt_str("viewmodel_fov"));

		if (viewFOVcvar->GetFloat() != viewFOV)
		{
			*(float*)((DWORD)&viewFOVcvar->m_fnChangeCallbacks + 0xC) = 0.0f;
			viewFOVcvar->SetValue(viewFOV);
		}
	}
	
	if (cfg.esp.viewmodel_x)
	{
		auto viewX = (float)cfg.esp.viewmodel_x / 2.0f;
		static auto viewXcvar = m_cvar()->FindVar(crypt_str("viewmodel_offset_x")); 

		if (viewXcvar->GetFloat() != viewX) 
		{
			*(float*)((DWORD)&viewXcvar->m_fnChangeCallbacks + 0xC) = 0.0f;
			viewXcvar->SetValue(viewX);
		}
	}

	if (cfg.esp.viewmodel_y)
	{
		auto viewY = (float)cfg.esp.viewmodel_y / 2.0f;
		static auto viewYcvar = m_cvar()->FindVar(crypt_str("viewmodel_offset_y"));

		if (viewYcvar->GetFloat() != viewY)
		{
			*(float*)((DWORD)&viewYcvar->m_fnChangeCallbacks + 0xC) = 0.0f;
			viewYcvar->SetValue(viewY);
		}
	}

	if (cfg.esp.viewmodel_z)
	{
		auto viewZ = (float)cfg.esp.viewmodel_z / 2.0f;
		static auto viewZcvar = m_cvar()->FindVar(crypt_str("viewmodel_offset_z"));

		if (viewZcvar->GetFloat() != viewZ) 
		{
			*(float*)((DWORD)&viewZcvar->m_fnChangeCallbacks + 0xC) = 0.0f;
			viewZcvar->SetValue(viewZ);
		}
	}
}

void misc::FullBright()
{		
	if (!cfg.player.enable)
		return;

	static auto mat_fullbright = m_cvar()->FindVar(crypt_str("mat_fullbright"));

	if (mat_fullbright->GetBool() != cfg.esp.bright)
		mat_fullbright->SetValue(cfg.esp.bright);
}

void misc::PovArrows(player_t* e, Color color)
{
	auto isOnScreen = [](Vector origin, Vector& screen) -> bool
	{
		if (!math::WorldToScreen(origin, screen))
			return false;

		static int iScreenWidth, iScreenHeight;
		m_engine()->GetScreenSize(iScreenWidth, iScreenHeight);

		auto xOk = iScreenWidth > screen.x;
		auto yOk = iScreenHeight > screen.y;

		return xOk && yOk;
	};

	Vector screenPos;

	if (isOnScreen(e->GetAbsOrigin(), screenPos))
		return;

	Vector viewAngles;
	m_engine()->GetViewAngles(viewAngles);

	static int width, height;
	m_engine()->GetScreenSize(width, height);

	auto screenCenter = Vector2D(width * 0.5f, height * 0.5f);
	auto angleYawRad = DEG2RAD(viewAngles.y - math::calculate_angle(g_ctx.globals.eye_pos, e->GetAbsOrigin()).y - 90.0f);

	auto radius = cfg.player.distance;
	auto size = cfg.player.size;

	auto newPointX = screenCenter.x + ((((width - (size * 3)) * 0.5f) * (radius / 100.0f)) * cos(angleYawRad)) + (int)(6.0f * (((float)size - 4.0f) / 16.0f));
	auto newPointY = screenCenter.y + ((((height - (size * 3)) * 0.5f) * (radius / 100.0f)) * sin(angleYawRad));

	std::array <Vector2D, 3> points
	{
		Vector2D(newPointX - size, newPointY - size),
		Vector2D(newPointX + size, newPointY),
		Vector2D(newPointX - size, newPointY + size)
	};

	math::rotate_triangle(points, viewAngles.y - math::calculate_angle(g_ctx.globals.eye_pos, e->GetAbsOrigin()).y - 90.0f);
	g_Render->TriangleFilled(points.at(0).x, points.at(0).y, points.at(1).x, points.at(1).y, points.at(2).x, points.at(2).y, Color(color.r(), color.g(), color.b(), math::clamp(color.a(), 0, 125)));
	g_Render->Triangle(points.at(0).x, points.at(0).y, points.at(1).x, points.at(1).y, points.at(2).x, points.at(2).y, Color(color.r(), color.g(), color.b(), color.a()));
}

void misc::NightmodeFix()
{
	static auto in_game = false;

	if (m_engine()->IsInGame() && !in_game)
	{
		in_game = true;

		g_ctx.globals.change_materials = true;
		worldesp::get().changed = true;

		static auto skybox = m_cvar()->FindVar(crypt_str("sv_skyname"));
		worldesp::get().backup_skybox = skybox->GetString();
		return;
	}
	else if (!m_engine()->IsInGame() && in_game)
		in_game = false;

	static auto player_enable = cfg.player.enable;

	if (player_enable != cfg.player.enable)
	{
		player_enable = cfg.player.enable;
		g_ctx.globals.change_materials = true;
		return;
	}

	static auto setting = cfg.esp.nightmode;

	if (setting != cfg.esp.nightmode)
	{
		setting = cfg.esp.nightmode;
		g_ctx.globals.change_materials = true;
		return;
	}

	static auto setting_world = cfg.esp.world_color;

	if (setting_world != cfg.esp.world_color)
	{
		setting_world = cfg.esp.world_color;
		g_ctx.globals.change_materials = true;
	}

	static auto setting_props = cfg.esp.props_color;

	if (setting_props != cfg.esp.props_color)
	{
		setting_props = cfg.esp.props_color;
		g_ctx.globals.change_materials = true;
	}
}

void misc::desync_arrows()
{
	if (!g_ctx.local()->is_alive())
		return;

	if (!cfg.antiaim.enable)
		return;

	if ((cfg.antiaim.manual_back.key <= KEY_NONE || cfg.antiaim.manual_back.key >= KEY_MAX) && (cfg.antiaim.manual_left.key <= KEY_NONE || cfg.antiaim.manual_left.key >= KEY_MAX) && (cfg.antiaim.manual_right.key <= KEY_NONE || cfg.antiaim.manual_right.key >= KEY_MAX))
		antiaim::get().manual_side = SIDE_NONE;

	if (!cfg.antiaim.flip_indicator)
		return;

	static int width, height;
	m_engine()->GetScreenSize(width, height);

	static auto alpha = 1.0f;
	static auto switch_alpha = false;

	if (alpha <= 0.0f || alpha >= 1.0f)
		switch_alpha = !switch_alpha;

	alpha += switch_alpha ? 2.0f * m_globals()->m_frametime : -2.0f * m_globals()->m_frametime;
	alpha = math::clamp(alpha, 0.0f, 1.0f);

	auto color = cfg.antiaim.flip_indicator_color;
	color.SetAlpha(cfg.antiaim.flip_indicator_color.a());

	if (antiaim::get().manual_side == SIDE_BACK) {
		render::get().triangle(Vector2D(width / 2, height / 2 + 80), Vector2D(width / 2 - 10, height / 2 + 60), Vector2D(width / 2 + 10, height / 2 + 60), color);
		render::get().triangle_def(Vector2D(width / 2, height / 2 + 80), Vector2D(width / 2 - 10, height / 2 + 60), Vector2D(width / 2 + 10, height / 2 + 60), Color(color.r(), color.g(), color.b(), 255));
	}
	else if (antiaim::get().manual_side == SIDE_LEFT) {
		render::get().triangle(Vector2D(width / 2 - 55, height / 2 + 10), Vector2D(width / 2 - 75, height / 2), Vector2D(width / 2 - 55, height / 2 - 10), color);
		render::get().triangle_def(Vector2D(width / 2 - 55, height / 2 + 10), Vector2D(width / 2 - 75, height / 2), Vector2D(width / 2 - 55, height / 2 - 10), Color(color.r(), color.g(), color.b(), 255));
	}
	else if (antiaim::get().manual_side == SIDE_RIGHT) {
		render::get().triangle(Vector2D(width / 2 + 55, height / 2 - 10), Vector2D(width / 2 + 75, height / 2), Vector2D(width / 2 + 55, height / 2 + 10), color);
		render::get().triangle_def(Vector2D(width / 2 + 55, height / 2 - 10), Vector2D(width / 2 + 75, height / 2), Vector2D(width / 2 + 55, height / 2 + 10), Color(color.r(), color.g(), color.b(), 255));
	}
	if (cfg.antiaim.desync == 2) {
		if (antiaim::get().flip == false) {
			render::get().rect(width / 2 - 10, height / 2 + 46, 20, 4, Color(color.r(), color.g(), color.b(), 255));
			render::get().rect_filled(width / 2 - 10, height / 2 + 46, 20, 4, Color(color));
		}
	}
	else
	{
		if (antiaim::get().flip == false) {
			render::get().rect(width / 2 - 50, height / 2 - 10, 4, 20, Color(color.r(), color.g(), color.b(), 255));
			render::get().rect_filled(width / 2 - 50, height / 2 - 10, 4, 20, Color(color));
		}
		else if (antiaim::get().flip == true)
		{
			render::get().rect(width / 2 + 46, height / 2 - 10, 4, 20, Color(color.r(), color.g(), color.b(), 255));
			render::get().rect_filled(width / 2 + 46, height / 2 - 10, 4, 20, Color(color));
		}
	}
}

void misc::aimbot_hitboxes()
{
	if (!cfg.player.enable)
		return;

	if (!cfg.player.lag_hitbox)
		return;

	auto player = (player_t*)m_entitylist()->GetClientEntity(aim::get().last_target_index);

	if (!player)
		return;

	auto model = player->GetModel();

	if (!model)
		return;

	auto studio_model = m_modelinfo()->GetStudioModel(model);

	if (!studio_model)
		return;
	
	auto hitbox_set = studio_model->pHitboxSet(player->m_nHitboxSet());

	if (!hitbox_set)
		return;

	for (auto i = 0; i < hitbox_set->numhitboxes; i++)
	{
		auto hitbox = hitbox_set->pHitbox(i);

		if (!hitbox)
			continue;

		if (hitbox->radius == -1.0f)
			continue;

		auto min = ZERO;
		auto max = ZERO;

		math::vector_transform(hitbox->bbmin, aim::get().last_target[aim::get().last_target_index].record.matrixes_data.main[hitbox->bone], min);
		math::vector_transform(hitbox->bbmax, aim::get().last_target[aim::get().last_target_index].record.matrixes_data.main[hitbox->bone], max);

		m_debugoverlay()->AddCapsuleOverlay(min, max, hitbox->radius, cfg.player.lag_hitbox_color.r(), cfg.player.lag_hitbox_color.g(), cfg.player.lag_hitbox_color.b(), cfg.player.lag_hitbox_color.a(), 4.0f, 0, 1);
	}
}

void misc::fast_stop(CUserCmd* m_pcmd)
{
	if (!cfg.misc.fast_stop)
		return;

	if (!(g_ctx.local()->m_fFlags() & FL_ONGROUND && engineprediction::get().backup_data.flags & FL_ONGROUND))
		return;

	auto pressed_move_key = m_pcmd->m_buttons & IN_FORWARD || m_pcmd->m_buttons & IN_MOVELEFT || m_pcmd->m_buttons & IN_BACK || m_pcmd->m_buttons & IN_MOVERIGHT || m_pcmd->m_buttons & IN_JUMP;

	if (pressed_move_key)
		return;

	if (!((antiaim::get().type == ANTIAIM_LEGIT ? cfg.antiaim.desync : cfg.antiaim.type[antiaim::get().type].desync) && (antiaim::get().type == ANTIAIM_LEGIT ? !cfg.antiaim.legit_lby_type : !cfg.antiaim.lby_type) && (!g_ctx.globals.weapon->is_grenade() || cfg.esp.on_click & !(m_pcmd->m_buttons & IN_ATTACK) && !(m_pcmd->m_buttons & IN_ATTACK2))) || antiaim::get().condition(m_pcmd))  
	{
		auto velocity = g_ctx.local()->m_vecVelocity();

		if (velocity.Length2D() > 20.0f)
		{
			Vector direction;
			Vector real_view;

			math::vector_angles(velocity, direction);
			m_engine()->GetViewAngles(real_view);

			direction.y = real_view.y - direction.y;

			Vector forward;
			math::angle_vectors(direction, forward);

			static auto cl_forwardspeed = m_cvar()->FindVar(crypt_str("cl_forwardspeed"));
			static auto cl_sidespeed = m_cvar()->FindVar(crypt_str("cl_sidespeed"));

			auto negative_forward_speed = -cl_forwardspeed->GetFloat();
			auto negative_side_speed = -cl_sidespeed->GetFloat();

			auto negative_forward_direction = forward * negative_forward_speed;
			auto negative_side_direction = forward * negative_side_speed;

			m_pcmd->m_forwardmove = negative_forward_direction.x;
			m_pcmd->m_sidemove = negative_side_direction.y;
		}
	}
	else
	{
		auto velocity = g_ctx.local()->m_vecVelocity();

		if (velocity.Length2D() > 20.0f)
		{
			Vector direction;
			Vector real_view;

			math::vector_angles(velocity, direction);
			m_engine()->GetViewAngles(real_view);

			direction.y = real_view.y - direction.y;

			Vector forward;
			math::angle_vectors(direction, forward);

			static auto cl_forwardspeed = m_cvar()->FindVar(crypt_str("cl_forwardspeed"));
			static auto cl_sidespeed = m_cvar()->FindVar(crypt_str("cl_sidespeed"));

			auto negative_forward_speed = -cl_forwardspeed->GetFloat();
			auto negative_side_speed = -cl_sidespeed->GetFloat();

			auto negative_forward_direction = forward * negative_forward_speed;
			auto negative_side_direction = forward * negative_side_speed;

			m_pcmd->m_forwardmove = negative_forward_direction.x;
			m_pcmd->m_sidemove = negative_side_direction.y;
		}
		else
		{
			auto speed = 1.01f;

			if (m_pcmd->m_buttons & IN_DUCK || g_ctx.globals.fakeducking)
				speed *= 2.94117647f;

			static auto switch_move = false;

			if (switch_move)
				m_pcmd->m_sidemove += speed;
			else
				m_pcmd->m_sidemove -= speed;

			switch_move = !switch_move;
		}
	}
}

bool misc::createmove(CUserCmd* m_pcmd)
{
	this->double_tap_defensive(m_pcmd);
	this->slowwalk_movement(m_pcmd);

	return true;
}

void misc::break_lc(CUserCmd* m_pcmd) // breaking lc with fakelags
{
	auto choked = m_clientstate()->iChokedCommands;
	static auto fluctuate_ticks = 0;
	static auto switch_ticks = false;
	static auto random_factor = min(rand() % 16 + 1, cfg.antiaim.triggers_fakelag_amount);

	if (!g_ctx.globals.exploits && cfg.antiaim.fakelag && cfg.antiaim.fakelag_enablers[FAKELAG_PEEK] && started_peeking)
	{
		if (choked < max_choke)
			g_ctx.send_packet = false;
		else
		{
			started_peeking = false;

			random_factor = min(rand() % 16 + 1, cfg.antiaim.triggers_fakelag_amount);
			switch_ticks = !switch_ticks;
			fluctuate_ticks = switch_ticks ? cfg.antiaim.triggers_fakelag_amount : max(cfg.antiaim.triggers_fakelag_amount - 2, 1);

			g_ctx.send_packet = true;
		}
	}

	max_choke = min(max_choke, 5);

	if (choked < max_choke)
		g_ctx.send_packet = false;
	else
	{
		started_peeking = false;
		random_factor = min(rand() % 16 + 1, max_choke);
		switch_ticks = !switch_ticks;
		fluctuate_ticks = switch_ticks ? max_choke : max(max_choke - 2, 1);
		g_ctx.send_packet = true;
	}

	if (g_ctx.send_packet)
	{
		started_peeking = true;
		g_ctx.globals.next_tickbase_shift++;
		started_peeking = false;
	}
	else
	{
		g_ctx.globals.ticks_allowed = 16;
		g_ctx.globals.next_tickbase_shift++;
	}
}
void misc::double_tap_defensive(CUserCmd* m_pcmd)
{
	if (!cfg.ragebot.lag_exploit)
		return;

	// Ensure player velocity is less than max speed before proceeding
	if (g_ctx.local()->m_vecVelocity().Length2D() < g_ctx.local()->GetMaxPlayerSpeed())
	{
		g_ctx.globals.m_Peek.m_bIsPeeking = false;
		g_ctx.globals.tickbase_shift = 2;  // Break LC (Lag Compensation)
		return;
	}

	// Get the active combat weapon, skip if no weapon or revolver is equipped
	const auto pCombatWeapon = g_ctx.local()->m_hActiveWeapon().Get();
	if (!pCombatWeapon || pCombatWeapon->m_iItemDefinitionIndex() == WEAPON_REVOLVER || (m_pcmd->m_buttons & IN_ATTACK))
		return;

	// Predict eye position based on player velocity and interval
	Vector predicted_eye_pos = g_ctx.globals.eye_pos + (engineprediction::get().backup_data.velocity * m_globals()->m_intervalpertick);

	// Iterate over all players
	for (auto i = 1; i <= m_globals()->m_maxclients; i++)
	{
		auto e = static_cast<player_t*>(m_entitylist()->GetClientEntity(i));
		if (!e || !e->valid(true))
			continue;

		// Get player records
		auto records = &player_records[i];
		if (records->empty())
			continue;

		// Get the most recent valid record for this player
		auto record = &records->front();
		if (!record->valid())
			continue;

		// Adjust player animation data
		record->adjust_player();

		// Check prediction for each choked command tick
		for (int next_chock = 1; next_chock <= m_clientstate()->iChokedCommands; ++next_chock)
		{
			predicted_eye_pos *= next_chock;  // Update the predicted position

			// Perform wall penetration check
			auto fire_data = autowall::get().wall_penetration(predicted_eye_pos, e->hitbox_position_matrix(HITBOX_HEAD, record->matrixes_data.first), e);
			if (fire_data.valid && fire_data.damage >= 1)
			{
				g_ctx.globals.m_Peek.m_bIsPeeking = true;
				break;  // Exit once a valid hit is found
			}
		}
	}

	// Shift time handling for teleport behavior
	if (++g_ctx.globals.shift_time > 15)
		g_ctx.globals.shift_time = 0;

	if (g_ctx.local()->m_vecVelocity().Length2D() > g_ctx.local()->GetMaxPlayerSpeed())
		g_ctx.globals.shift_time = (g_ctx.local()->m_fFlags() & FL_ONGROUND) ? 1 : 4;

	// Clamp shift time for tickbase manipulation
	g_ctx.globals.shift_time = std::clamp(g_ctx.globals.shift_time, 1, 2);

	// Apply tickbase shift based on shift time
	if (g_ctx.globals.shift_time > 0)
		g_ctx.globals.tickbase_shift = 14;  // Maximum tickbase shift

	// Handle peek state and tickbase shift adjustments
	if (m_gamerules()->m_bIsValveDS() || g_ctx.globals.m_Peek.m_bIsPeeking)
	{
		if (!g_ctx.globals.m_Peek.m_bIsPrevPeek)
		{
			g_ctx.globals.m_Peek.m_bIsPrevPeek = true;
			g_ctx.globals.tickbase_shift = 15;  // High tickbase shift during peek
			return;
		}
	}
	else
	{
		g_ctx.globals.m_Peek.m_bIsPrevPeek = false;
	}

	// Default shift to 2 if no specific condition is met
	g_ctx.globals.tickbase_shift = 2;

	// Reset shift time to 14 if needed
	g_ctx.globals.shift_time = 14;
}


bool can_shift_shot(int ticks)
{
	if (!g_ctx.local() || !g_ctx.local()->m_hActiveWeapon())
		return false;

	auto tickbase = g_ctx.local()->m_nTickBase();
	auto curtime = m_globals()->m_intervalpertick * (tickbase - ticks);

	if (curtime < g_ctx.local()->m_flNextAttack())
		return false;

	if (curtime < g_ctx.local()->m_hActiveWeapon()->m_flNextPrimaryAttack())
		return false;

	return true;
}

bool misc::double_tap(CUserCmd* m_pcmd)
{
	double_tap_enabled = true;

	static auto recharge_rapid_fire = false;
	static bool firing_dt = false;

	if (recharge_rapid_fire)
	{
		recharge_rapid_fire = false;
		recharging_double_tap = true;

		g_ctx.globals.ticks_allowed = 0;
		g_ctx.globals.tickbase_shift = 0;
		return false;
	}

	auto max_tickbase_shift = g_ctx.globals.weapon->get_max_tickbase_shift();

	if (recharging_double_tap)
	{
		if (can_shift_shot(max_tickbase_shift) && !aim::get().should_stop)
		{
			recharging_double_tap = false;
			double_tap_key = true;
			firing_dt = false;
		}
		else if (m_pcmd->m_buttons & IN_ATTACK)
			firing_dt = true;
	}

	if (!cfg.ragebot.enable)
	{
		double_tap_enabled = false;
		double_tap_key = false;

		g_ctx.globals.ticks_allowed = 0;
		g_ctx.globals.tickbase_shift = 0;
		return false;
	}

	if (!cfg.ragebot.double_tap)
	{
		double_tap_enabled = false;
		double_tap_key = false;

		g_ctx.globals.ticks_allowed = 0;
		g_ctx.globals.tickbase_shift = 0;
		return false;
	}

	static bool was_in_dt = false;

	if (cfg.ragebot.double_tap_key.key <= KEY_NONE || cfg.ragebot.double_tap_key.key >= KEY_MAX)
	{
		double_tap_enabled = false;
		double_tap_key = false;

		g_ctx.globals.ticks_allowed = 0;
		g_ctx.globals.tickbase_shift = 0;
		return false;
	}

	if (double_tap_key && cfg.ragebot.double_tap_key.key != cfg.antiaim.hide_shots_key.key)
		hide_shots_key = false;

	if (!double_tap_key || g_ctx.local()->m_bGunGameImmunity() || g_ctx.local()->m_fFlags() & FL_FROZEN || g_ctx.globals.fakeducking)
	{
		double_tap_enabled = false;

		if (!firing_dt && was_in_dt)
		{
			g_ctx.globals.trigger_teleport = true;
			g_ctx.globals.teleport_amount = max_tickbase_shift;

			was_in_dt = false;
		}

		g_ctx.globals.ticks_allowed = 0;
		g_ctx.globals.tickbase_shift = 0;

		return false;
	}

	if (m_gamerules()->m_bIsValveDS())
	{
		double_tap_enabled = false;
		g_ctx.globals.ticks_allowed = 0;
		g_ctx.globals.tickbase_shift = 0;
		return false;
	}

	if (antiaim::get().freeze_check)
		return true;

	was_in_dt = true;

	if (!g_ctx.globals.weapon->is_grenade() && g_ctx.globals.weapon->m_iItemDefinitionIndex() != WEAPON_TASER
		&& g_ctx.globals.weapon->m_iItemDefinitionIndex() != WEAPON_REVOLVER
		&& (m_pcmd->m_buttons & IN_ATTACK || m_pcmd->m_buttons & IN_ATTACK2 && g_ctx.globals.weapon->is_knife())) //-V648
	{
		auto next_command_number = m_pcmd->m_command_number + 1;
		auto user_cmd = m_input()->GetUserCmd(next_command_number);

		memcpy(user_cmd, m_pcmd, sizeof(CUserCmd));
		user_cmd->m_command_number = next_command_number;

		util::copy_command(user_cmd, max_tickbase_shift);

		if (g_ctx.globals.aimbot_working)
		{
			g_ctx.globals.double_tap_aim = true;
			g_ctx.globals.double_tap_aim_check = true;
		}

		recharge_rapid_fire = true;
		double_tap_enabled = false;
		double_tap_key = false;

		g_ctx.send_packet = true;
		firing_dt = true;
	}
	else if (!g_ctx.globals.weapon->is_grenade() && g_ctx.globals.weapon->m_iItemDefinitionIndex() != WEAPON_TASER && g_ctx.globals.weapon->m_iItemDefinitionIndex() != WEAPON_REVOLVER)
		g_ctx.globals.tickbase_shift = max_tickbase_shift;

	return true;
}

/*bool misc::double_tap(CUserCmd* m_pcmd)
{
	double_tap_enabled = true;

	static auto recharge_rapid_fire = false;
	static bool firing_dt = false;
	static bool was_in_dt = false;

	// Check if double-tap is enabled in the config
	if (!cfg.ragebot.enable || !cfg.ragebot.double_tap || g_ctx.local()->m_bGunGameImmunity() || g_ctx.local()->m_fFlags() & FL_FROZEN || g_ctx.globals.fakeducking)
	{
		double_tap_enabled = false;
		double_tap_key = false;
		g_ctx.globals.ticks_allowed = 0;
		g_ctx.globals.tickbase_shift = 0;
		return false;
	}

	// Handle the recharge and reset conditions
	if (recharge_rapid_fire)
	{
		recharge_rapid_fire = false;
		recharging_double_tap = true;
		g_ctx.globals.ticks_allowed = 0;
		g_ctx.globals.tickbase_shift = 0;
		return false;
	}

	// Get the maximum tickbase shift for the current weapon
	auto max_tickbase_shift = g_ctx.globals.weapon->get_max_tickbase_shift();

	// Handle recharging state, enable or disable double-tap
	if (recharging_double_tap)
	{
		if (can_shift_shot(max_tickbase_shift) && !aim::get().should_stop)
		{
			recharging_double_tap = false;
			double_tap_key = true;
			firing_dt = false;
		}
		else if (m_pcmd->m_buttons & IN_ATTACK)
		{
			firing_dt = true;
		}
	}

	// Early exit if double-tap key is invalid or on a Valve DS server
	if (cfg.ragebot.double_tap_key.key <= KEY_NONE || cfg.ragebot.double_tap_key.key >= KEY_MAX || m_gamerules()->m_bIsValveDS())
	{
		double_tap_enabled = false;
		double_tap_key = false;
		g_ctx.globals.ticks_allowed = 0;
		g_ctx.globals.tickbase_shift = 0;
		return false;
	}

	// Disable hide shots key if needed
	if (double_tap_key && cfg.ragebot.double_tap_key.key != cfg.antiaim.hide_shots_key.key)
		hide_shots_key = false;

	// If double-tap key is not pressed or player can't fire, disable double-tap
	if (!double_tap_key || !can_fire(m_pcmd, max_tickbase_shift))
	{
		if (!firing_dt && was_in_dt)
		{
			g_ctx.globals.trigger_teleport = true;
			g_ctx.globals.teleport_amount = max_tickbase_shift;
			was_in_dt = false;
		}

		double_tap_enabled = false;
		double_tap_key = false;
		g_ctx.globals.ticks_allowed = 0;
		g_ctx.globals.tickbase_shift = 0;
		return false;
	}

	// Mark that we were in double-tap mode
	was_in_dt = true;

	// Check if the weapon is valid and the player is allowed to fire
	if (!g_ctx.globals.weapon->is_grenade() && g_ctx.globals.weapon->m_iItemDefinitionIndex() != WEAPON_TASER && g_ctx.globals.weapon->m_iItemDefinitionIndex() != WEAPON_REVOLVER
		&& (m_pcmd->m_buttons & IN_ATTACK || (m_pcmd->m_buttons & IN_ATTACK2 && g_ctx.globals.weapon->is_knife())))
	{
		// Prepare a new user command for the second shot (tickbase shift)
		auto next_command_number = m_pcmd->m_command_number + 1;
		auto user_cmd = m_input()->GetUserCmd(next_command_number);

		memcpy(user_cmd, m_pcmd, sizeof(CUserCmd));
		user_cmd->m_command_number = next_command_number;

		// Apply the tickbase shift for double-tap
		util::copy_command(user_cmd, max_tickbase_shift);

		// If the aimbot is working, enable double-tap aimbot logic
		if (g_ctx.globals.aimbot_working)
		{
			g_ctx.globals.double_tap_aim = true;
			g_ctx.globals.double_tap_aim_check = true;
		}

		// Recharge rapid fire and disable double-tap state until next time
		recharge_rapid_fire = true;
		double_tap_enabled = false;
		double_tap_key = false;

		g_ctx.send_packet = true;
		firing_dt = true;
	}
	else
	{
		// If we are not firing, just set the tickbase shift to the max value
		g_ctx.globals.tickbase_shift = max_tickbase_shift;
	}

	return true;
}

bool misc::can_fire(CUserCmd* m_pcmd, int max_tickbase_shift)
{
	// Check if the player is allowed to fire (not using grenades or restricted weapons)
	return !g_ctx.globals.weapon->is_grenade() && g_ctx.globals.weapon->m_iItemDefinitionIndex() != WEAPON_TASER
		&& g_ctx.globals.weapon->m_iItemDefinitionIndex() != WEAPON_REVOLVER
		&& (m_pcmd->m_buttons & IN_ATTACK || (m_pcmd->m_buttons & IN_ATTACK2 && g_ctx.globals.weapon->is_knife()));
}*/

void misc::hide_shots(CUserCmd* m_pcmd, bool should_work)
{
	hide_shots_enabled = true;

	if (!cfg.ragebot.enable)
	{
		hide_shots_enabled = false;
		hide_shots_key = false;

		if (should_work)
		{
			g_ctx.globals.ticks_allowed = 0;
			g_ctx.globals.next_tickbase_shift = 0;
		}

		return;
	}

	if (!cfg.antiaim.hide_shots)
	{
		hide_shots_enabled = false;
		hide_shots_key = false;

		if (should_work)
		{
			g_ctx.globals.ticks_allowed = 0;
			g_ctx.globals.next_tickbase_shift = 0;
		}

		return;
	}

	if (cfg.antiaim.hide_shots_key.key <= KEY_NONE || cfg.antiaim.hide_shots_key.key >= KEY_MAX)
	{
		hide_shots_enabled = false;
		hide_shots_key = false;

		if (should_work)
		{
			g_ctx.globals.ticks_allowed = 0;
			g_ctx.globals.next_tickbase_shift = 0;
		}

		return;
	}

	if (!should_work && double_tap_key)
	{
		hide_shots_enabled = false;
		hide_shots_key = false;
		return;
	}

	if (!hide_shots_key)
	{
		hide_shots_enabled = false;
		g_ctx.globals.ticks_allowed = 0;
		g_ctx.globals.next_tickbase_shift = 0;
		return;
	}

	double_tap_key = false;

	if (g_ctx.local()->m_bGunGameImmunity() || g_ctx.local()->m_fFlags() & FL_FROZEN)
	{
		hide_shots_enabled = false;
		g_ctx.globals.ticks_allowed = 0;
		g_ctx.globals.next_tickbase_shift = 0;
		return;
	}

	if (g_ctx.globals.fakeducking)
	{
		hide_shots_enabled = false;
		g_ctx.globals.ticks_allowed = 0;
		g_ctx.globals.next_tickbase_shift = 0;
		return;
	}

	if (antiaim::get().freeze_check)
		return;

	g_ctx.globals.next_tickbase_shift = m_gamerules()->m_bIsValveDS() ? 6 : 9;

	auto revolver_shoot = g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_REVOLVER && !g_ctx.globals.revolver_working && (m_pcmd->m_buttons & IN_ATTACK || m_pcmd->m_buttons & IN_ATTACK2);
	auto weapon_shoot = m_pcmd->m_buttons & IN_ATTACK && g_ctx.globals.weapon->m_iItemDefinitionIndex() != WEAPON_REVOLVER || m_pcmd->m_buttons & IN_ATTACK2 && g_ctx.globals.weapon->is_knife() || revolver_shoot;

	if (g_ctx.send_packet && !g_ctx.globals.weapon->is_grenade() && weapon_shoot)
		g_ctx.globals.tickbase_shift = g_ctx.globals.next_tickbase_shift;
}