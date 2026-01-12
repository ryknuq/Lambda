#include <ShlObj_core.h>
#include <unordered_map>
#include "UI.h"
#include "../menu/ImGui/code_editor.h"
#include "../constchars.h"
#include "../features/misc/logs.h"
#include "../utils/lg/logging.h"

#define ALPHA (ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaBar| ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_Float)
#define NOALPHA (ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_Float)

std::vector <std::string> files;
std::vector <std::string> scripts;
std::string editing_script;

auto selected_script = 0;
auto loaded_editing_script = false;

static auto menu_setupped = false;
static auto should_update = true;

IDirect3DTexture9* all_skins[36];


__forceinline void padding(float x, float y)
{
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + x * c_menu::get().dpi_scale);
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + y * c_menu::get().dpi_scale);
}

void draw_multicombo(std::string name, std::vector<int>& variable, const char* labels[], int count, std::string& preview)
{
	auto hashname = crypt_str("##") + name;

	for (auto i = 0, j = 0; i < count; i++)
	{
		if (variable[i])
		{
			if (j)
				preview += crypt_str(", ") + (std::string)labels[i];
			else
				preview = labels[i];

			j++;
		}
	}
	ImGui::Spacing();

	if (ImGui::BeginCombo(hashname.c_str(), preview.c_str()))
	{
		ImGui::Spacing();
		ImGui::BeginGroup();
		{
			for (auto i = 0; i < count; i++)
				ImGui::Selectable(labels[i], (bool*)&variable[i], ImGuiSelectableFlags_DontClosePopups);
		}
		ImGui::EndGroup();
		ImGui::Spacing();

		ImGui::EndCombo();
	}

	preview = crypt_str("None");
}


bool LabelClick(const char* label, bool* v, const char* unique_id)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;

	char Buf[64];
	_snprintf(Buf, 62, crypt_str("%s"), label);

	char getid[128];
	sprintf_s(getid, 128, crypt_str("%s%s"), label, unique_id);


	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(getid);
	const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

	const ImRect check_bb(window->DC.CursorPos, ImVec2(label_size.y + style.FramePadding.y * 2 + window->DC.CursorPos.x, window->DC.CursorPos.y + label_size.y + style.FramePadding.y * 2));
	ImGui::ItemSize(check_bb, style.FramePadding.y);

	ImRect total_bb = check_bb;

	if (label_size.x > 0)
	{
		ImGui::SameLine(0, style.ItemInnerSpacing.x);
		const ImRect text_bb(ImVec2(window->DC.CursorPos.x, window->DC.CursorPos.y + style.FramePadding.y), ImVec2(window->DC.CursorPos.x + label_size.x, window->DC.CursorPos.y + style.FramePadding.y + label_size.y));

		ImGui::ItemSize(ImVec2(text_bb.GetWidth(), check_bb.GetHeight()), style.FramePadding.y);
		total_bb = ImRect(ImMin(check_bb.Min, text_bb.Min), ImMax(check_bb.Max, text_bb.Max));
	}

	if (!ImGui::ItemAdd(total_bb, id))
		return false;

	bool hovered, held;
	bool pressed = ImGui::ButtonBehavior(total_bb, id, &hovered, &held);
	if (pressed)
		*v = !(*v);

	if (*v)
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(25 / 255.f, 25 / 255.f, 25 / 225.f, 225.f));
	if (label_size.x > 0.0f)

		ImGui::PushFont(c_menu::get().g_cxm);
	ImGui::RenderText(ImVec2(check_bb.GetTL().x + 12, check_bb.GetTL().y), Buf);
	ImGui::PopFont();

	if (*v)
		ImGui::PopStyleColor();

	return pressed;

}

void draw_keybind(const char* label, key_bind* key_bind, const char* unique_id, bool with_bool = false, bool with_color = false)
{
	// reset bind if we re pressing esc
	if (key_bind->key == KEY_ESCAPE)
		key_bind->key = KEY_NONE;


	auto clicked = false;
	auto text = (std::string)m_inputsys()->ButtonCodeToString(key_bind->key);
	auto s = ImGui::GetWindowSize();
	if (key_bind->key <= KEY_NONE || key_bind->key >= KEY_MAX) {
		text = crypt_str("< >");
	}
	else

		// if we clicked on keybind
		if (hooks::input_shouldListen && hooks::input_receivedKeyval == &key_bind->key)
		{
			clicked = true;
			text = crypt_str("...");
		}

	if (text == crypt_str("MOUSE5"))
		text = crypt_str("M5");
	else if (text == crypt_str("MOUSE4"))
		text = crypt_str("M4");
	else if (text == crypt_str("MOUSE3"))
		text = crypt_str("M3");
	else if (text == crypt_str("MOUSE1"))
		text = crypt_str("M1");
	else if (text == crypt_str("MOUSE2"))
		text = crypt_str("M2");
	else if (text == crypt_str("INSERT"))
		text = crypt_str("INS");
	else if (text == crypt_str("DELETE"))
		text = crypt_str("DEL");
	else if (text == crypt_str("SHIFT"))
		text = crypt_str("SHT");

	auto textsize = ImGui::CalcTextSize(text.c_str()).x + 2;

	ImGui::SetCursorPosX(ImGui::GetWindowSize().x - (ImGui::GetWindowSize().x - ImGui::CalcItemWidth()) + max(3, textsize));
	if (ImGui::KeybindButton(text.c_str(), unique_id, ImVec2(ImGui::CalcTextSize(text.c_str()).x + 8, ImGui::CalcTextSize(text.c_str()).y + 6), clicked, ImGuiButtonFlags_::ImGuiButtonFlags_None))
		clicked = true;

	if (clicked)
	{
		hooks::input_shouldListen = true;
		hooks::input_receivedKeyval = &key_bind->key;
	}

	static auto hold = false, toggle = false;

	switch (key_bind->mode)
	{
	case HOLD:
		hold = true;
		toggle = false;
		break;
	case TOGGLE:
		toggle = true;
		hold = false;
		break;
	}

	if (cfg.scripts.developer_mode && ImGui::IsItemHovered())
	{
		for (auto& item : cfg_manager->items)
		{
			if (key_bind == item->pointer)
			{
				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 6.0f));
				ImGui::SetTooltip(item->name.c_str());
				ImGui::PopStyleVar();
				break;
			}
		}
	}

	if (ImGui::BeginPopup(unique_id))
	{
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6 * c_menu::get().dpi_scale);
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ((ImGui::GetCurrentWindow()->Size.x / 2) - (ImGui::CalcTextSize(crypt_str("Hold")).x / 2)));
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 11);

		if (LabelClick(crypt_str("Hold"), &hold, unique_id))
		{
			if (hold)
			{
				toggle = false;
				key_bind->mode = HOLD;
			}
			else if (toggle)
			{
				hold = false;
				key_bind->mode = TOGGLE;
			}
			else
			{
				toggle = false;
				key_bind->mode = HOLD;
			}

			ImGui::CloseCurrentPopup();
		}

		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ((ImGui::GetCurrentWindow()->Size.x / 2) - (ImGui::CalcTextSize(crypt_str("Toggle")).x / 2)));
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 11);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 9 * c_menu::get().dpi_scale);

		if (LabelClick(crypt_str("Toggle"), &toggle, unique_id))
		{
			if (toggle)
			{
				hold = false;
				key_bind->mode = TOGGLE;
			}
			else if (hold)
			{
				toggle = false;
				key_bind->mode = HOLD;
			}
			else
			{
				hold = false;
				key_bind->mode = TOGGLE;
			}

			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}


void draw_combo(const char* name, int& variable, const char* labels[], int count) { ImGui::Combo(std::string(name).c_str(), &variable, labels, count); }
void draw_combo(const char* name, int& variable, bool (*items_getter)(void*, int, const char**), void* data, int count) { ImGui::Combo(std::string(name).c_str(), &variable, items_getter, data, count); }

std::string get_config_dir()
{
	// Get the executable directory and construct the full path
	char exePath[MAX_PATH];
	GetModuleFileNameA(NULL, exePath, MAX_PATH);
	std::string exeDir(exePath);
	exeDir = exeDir.substr(0, exeDir.find_last_of("\\/"));
	
	std::string folder = crypt_str("C:\\Program Files (x86)\\Steam\\steamapps\\common\\Counter-Strike Global Offensive\\Lambda\\");

	CreateDirectory(folder.c_str(), NULL);

	return folder;
}

void load_config(std::string selected_config)
{
	if (cfg_manager->files.empty())
		return;

	cfg_manager->load(selected_config, false);
	//c_lua::get().unload_all_scripts();

	//for (auto& script : cfg.scripts.scripts)
		//c_lua::get().load_script(c_lua::get().get_script_id(script));

	//scripts = c_lua::get().scripts;

	//if (selected_script >= scripts.size())
		//selected_script = scripts.size() - 1; //-V103

	//for (auto& current : scripts)
	//{
		//if (current.size() >= 5 && current.at(current.size() - 1) == 'c')
			//current.erase(current.size() - 5, 5);
		//else if (current.size() >= 4)
			//current.erase(current.size() - 4, 4);
	//}

	for (auto i = 0; i < cfg.skins.skinChanger.size(); ++i)
		all_skins[i] = nullptr;

	cfg.scripts.scripts.clear();

	c_menu::get().loaded_config = selected_config;
	cfg_manager->load(selected_config, true);
	cfg_manager->config_files();

	eventlogs::get().add(crypt_str("Loaded ") + selected_config);

}

void save_config(std::string selected_config)
{
	if (selected_config == "")
		return;

	if (cfg_manager->files.empty())
		return;

	cfg.scripts.scripts.clear();

	cfg_manager->save(selected_config);
	cfg_manager->config_files();

	eventlogs::get().add(crypt_str("Saved ") + selected_config);
}

void remove_config(std::string selected_config)
{
	eventlogs::get().add(crypt_str("Deleted ") + selected_config);

	cfg_manager->remove(selected_config);
	cfg_manager->config_files();

	files = cfg_manager->files;

	if (cfg.selected_config >= files.size())
		cfg.selected_config = files.size() - 1;

	for (auto& current : files)
		if (current.size() > 2)
			current.erase(current.size() - 3, 3);
}

void add_config(std::string name)
{
	if (name.empty())
		name = crypt_str("config");

	if (name.find(crypt_str(".cfg")) == std::string::npos)
		name += crypt_str(".cfg");

	eventlogs::get().add(crypt_str("Added ") + name + crypt_str(" config"));

	cfg_manager->save(name);
}

bool LabelClick2(const char* label, bool* v, const char* unique_id)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;

	char Buf[64];
	_snprintf(Buf, 62, crypt_str("%s"), label);

	char getid[128];
	sprintf_s(getid, 128, crypt_str("%s%s"), label, unique_id);


	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(getid);
	const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

	const ImRect check_bb(window->DC.CursorPos, ImVec2(label_size.y + style.FramePadding.y * 2 + window->DC.CursorPos.x, window->DC.CursorPos.y + label_size.y + style.FramePadding.y * 2));
	ImGui::ItemSize(check_bb, style.FramePadding.y);

	ImRect total_bb = check_bb;

	if (label_size.x > 0)
	{
		ImGui::SameLine(0, style.ItemInnerSpacing.x);
		const ImRect text_bb(ImVec2(window->DC.CursorPos.x, window->DC.CursorPos.y + style.FramePadding.y), ImVec2(window->DC.CursorPos.x + label_size.x, window->DC.CursorPos.y + style.FramePadding.y + label_size.y));

		ImGui::ItemSize(ImVec2(text_bb.GetWidth(), check_bb.GetHeight()), style.FramePadding.y);
		total_bb = ImRect(ImMin(check_bb.Min, text_bb.Min), ImMax(check_bb.Max, text_bb.Max));
	}

	if (!ImGui::ItemAdd(total_bb, id))
		return false;

	bool hovered, held;
	bool pressed = ImGui::ButtonBehavior(total_bb, id, &hovered, &held);
	if (pressed)
		*v = !(*v);

	if (*v)
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(25 / 255.f, 25 / 255.f, 25 / 225.f, 225.f));
	if (label_size.x > 0.0f)

		ImGui::PushFont(c_menu::get().g_cxm);
	ImGui::RenderText(ImVec2(check_bb.GetTL().x + 12, check_bb.GetTL().y), Buf);
	ImGui::PopFont();

	if (*v)
		ImGui::PopStyleColor();

	return pressed;

}



void c_menu::menu_setup(ImGuiStyle& style)
{
	ImGui::StyleColorsClassic();
	ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Once);
	ImGui::SetNextWindowBgAlpha(min(style.Alpha, 0.94f));

	styles.WindowPadding = style.WindowPadding;
	styles.WindowRounding = style.WindowRounding;
	styles.WindowMinSize = style.WindowMinSize;
	styles.ChildRounding = style.ChildRounding;
	styles.PopupRounding = style.PopupRounding;
	styles.FramePadding = style.FramePadding;
	styles.FrameRounding = style.FrameRounding;
	styles.ItemSpacing = style.ItemSpacing;
	styles.ItemInnerSpacing = style.ItemInnerSpacing;
	styles.TouchExtraPadding = style.TouchExtraPadding;
	styles.IndentSpacing = style.IndentSpacing;
	styles.ColumnsMinSpacing = style.ColumnsMinSpacing;
	styles.ScrollbarSize = style.ScrollbarSize;
	styles.ScrollbarRounding = style.ScrollbarRounding;
	styles.GrabMinSize = style.GrabMinSize;
	styles.GrabRounding = style.GrabRounding;
	styles.TabRounding = style.TabRounding;
	styles.TabMinWidthForUnselectedCloseButton = style.TabMinWidthForUnselectedCloseButton;
	styles.DisplayWindowPadding = style.DisplayWindowPadding;
	styles.DisplaySafeAreaPadding = style.DisplaySafeAreaPadding;
	styles.MouseCursorScale = style.MouseCursorScale;

	menu_setupped = true;
}

void c_menu::dpi_resize(float scale_factor, ImGuiStyle& style)
{
	style.WindowPadding = (styles.WindowPadding * scale_factor);
	style.WindowRounding = (styles.WindowRounding * scale_factor);
	style.WindowMinSize = (styles.WindowMinSize * scale_factor);
	style.ChildRounding = (styles.ChildRounding * scale_factor);
	style.PopupRounding = (styles.PopupRounding * scale_factor);
	style.FramePadding = (styles.FramePadding * scale_factor);
	style.FrameRounding = (styles.FrameRounding * scale_factor);
	style.ItemSpacing = (styles.ItemSpacing * scale_factor);
	style.ItemInnerSpacing = (styles.ItemInnerSpacing * scale_factor);
	style.TouchExtraPadding = (styles.TouchExtraPadding * scale_factor);
	style.IndentSpacing = (styles.IndentSpacing * scale_factor);
	style.ColumnsMinSpacing = (styles.ColumnsMinSpacing * scale_factor);
	style.ScrollbarSize = (styles.ScrollbarSize * scale_factor);
	style.ScrollbarRounding = (styles.ScrollbarRounding * scale_factor);
	style.GrabMinSize = (styles.GrabMinSize * scale_factor);
	style.GrabRounding = (styles.GrabRounding * scale_factor);
	style.TabRounding = (styles.TabRounding * scale_factor);
	if (styles.TabMinWidthForUnselectedCloseButton != FLT_MAX)
		style.TabMinWidthForUnselectedCloseButton = (styles.TabMinWidthForUnselectedCloseButton * scale_factor);
	style.DisplayWindowPadding = (styles.DisplayWindowPadding * scale_factor);
	style.DisplaySafeAreaPadding = (styles.DisplaySafeAreaPadding * scale_factor);
	style.MouseCursorScale = (styles.MouseCursorScale * scale_factor);
}

void c_menu::rage_tab() // rage tab
{
	if (rg_tab == 0)
	{
		ImGui::SetCursorPos(ImVec2(80, 80));
		ImGui::MenuChild("General", ImVec2(310, 350));
		{
			ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
			ImGui::Checkbox(crypt_str("Enable"), &cfg.ragebot.enable);
			ImGui::SliderInt(crypt_str("Field of view"), &cfg.ragebot.field_of_view, 1, 180, false, crypt_str("%d°"));
		}
		ImGui::EndChild();

		ImGui::SetCursorPos(ImVec2(410, 80));
		ImGui::MenuChild("Other", ImVec2(310, 350));
		{
			ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
			ImGui::Checkbox(crypt_str("Automatic scope"), &cfg.ragebot.autoscope);
			ImGui::Checkbox(crypt_str("Knife Bot"), &cfg.ragebot.knife_bot);
			ImGui::Checkbox(crypt_str("Tazer Bot"), &cfg.ragebot.zeus_bot);
		}
		ImGui::EndChild();

		ImGui::SetCursorPos(ImVec2(410, 450));
		ImGui::MenuChild("Exploits", ImVec2(310, 125));
		{
			ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
			ImGui::Checkbox(crypt_str("DT"), &cfg.ragebot.double_tap);
			if (cfg.ragebot.double_tap)
			{
				ImGui::SetCursorPosX(9);
				ImGui::Text(crypt_str("DT KEY"));
				ImGui::SameLine();
				draw_keybind(crypt_str(""), &cfg.ragebot.double_tap_key, crypt_str("##HOTKEY_DOUBLETAP"));
			}

			ImGui::Checkbox(crypt_str("HS"), &cfg.antiaim.hide_shots);
			if (cfg.antiaim.hide_shots)
			{
				ImGui::SetCursorPosX(9);
				ImGui::Text(crypt_str("HS KEY"));
				ImGui::SameLine();
				draw_keybind(crypt_str(""), &cfg.antiaim.hide_shots_key, crypt_str("##HOTKEY_HIDESHOTS"));
			}
		}
		ImGui::EndChild();

		ImGui::SetCursorPos(ImVec2(80, 450));
		ImGui::MenuChild("Main", ImVec2(310, 125));
		{
			ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
			ImGui::Checkbox(crypt_str("Defensive"), &cfg.ragebot.lag_exploit); // working
			ImGui::Checkbox(crypt_str("Anti exploit"), &cfg.ragebot.anti_exploit); // working
		}
		ImGui::EndChild();
	}
	else if (rg_tab == 1)
	{
		const char* rage_weapon[8] = { crypt_str("Heavy Pistols"), crypt_str("Pistols"), crypt_str("SMG"), crypt_str("Rifles"), crypt_str("Auto Sniper"), crypt_str("Scout"), crypt_str("AWP"), crypt_str("Heavy") };

		ImGui::SetCursorPos(ImVec2(80, 80));
		ImGui::MenuChild("Settings", ImVec2(310, 450));
		{
			ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
			ImGui::Combo(crypt_str("Weapon"), &hooks::rage_weapon, rage_weapon, ARRAYSIZE(rage_weapon));
			ImGui::Spacing();
			ImGui::Combo(crypt_str("Target Selection"), &cfg.ragebot.weapon[hooks::rage_weapon].selection_type, selection, ARRAYSIZE(selection));

			draw_multicombo(crypt_str("Hitboxes"), cfg.ragebot.weapon[hooks::rage_weapon].hitboxes, hitboxes, ARRAYSIZE(hitboxes), preview);

			ImGui::SliderInt(crypt_str("Minimum damage"), &cfg.ragebot.weapon[hooks::rage_weapon].minimum_visible_damage, 1, 120, true);
			ImGui::SliderInt(crypt_str("Minimum wall damage"), &cfg.ragebot.weapon[hooks::rage_weapon].minimum_damage, 1, 120, true);

			ImGui::SetCursorPosX(9);
			ImGui::PushFont(c_menu::get().g_cxmenufont);
			ImGui::Text("Damage Override");
			ImGui::PopFont();
			ImGui::SameLine();
			draw_keybind(crypt_str("Damage Override"), &cfg.ragebot.weapon[hooks::rage_weapon].damage_override_key, crypt_str("##HOTKEY__DAMAGE_OVERRIDE"));

			if (cfg.ragebot.weapon[hooks::rage_weapon].damage_override_key.key > KEY_NONE && cfg.ragebot.weapon[hooks::rage_weapon].damage_override_key.key < KEY_MAX)
				ImGui::SliderInt(crypt_str("Minimum override damage"), &cfg.ragebot.weapon[hooks::rage_weapon].minimum_override_damage, 1, 120, true);
		}
		ImGui::EndChild();

		ImGui::SetCursorPos(ImVec2(410, 80));

		ImGui::MenuChild("Extra", ImVec2(310, 450));
		{
			ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
			ImGui::Checkbox(crypt_str("Auto Stop"), &cfg.ragebot.weapon[hooks::rage_weapon].autostop);
			if (cfg.ragebot.weapon[hooks::rage_weapon].autostop)
				draw_multicombo(crypt_str("Auto Stop Modifiers"), cfg.ragebot.weapon[hooks::rage_weapon].autostop_modifiers, autostop_modifiers, ARRAYSIZE(autostop_modifiers), preview);

			ImGui::Checkbox(crypt_str("Hitchance"), &cfg.ragebot.weapon[hooks::rage_weapon].hitchance);
			if (cfg.ragebot.weapon[hooks::rage_weapon].hitchance)
				ImGui::SliderInt(crypt_str("Hit Chance"), &cfg.ragebot.weapon[hooks::rage_weapon].hitchance_amount, 0, 100, cfg.ragebot.weapon[hooks::rage_weapon].hitchance_amount ? crypt_str("%d") : crypt_str("None"));

			ImGui::Checkbox(crypt_str("Prefer safe points"), &cfg.ragebot.weapon[hooks::rage_weapon].prefer_safe_points);
			if (cfg.ragebot.weapon[hooks::rage_weapon].prefer_safe_points)
			{
				ImGui::Text(crypt_str("SAFE KEY"));
				ImGui::SameLine();
				draw_keybind(crypt_str("Force safe points"), &cfg.ragebot.safe_point_key, crypt_str("##HOKEY_FORCE_SAFE_POINTS"));
			}

			ImGui::Checkbox(crypt_str("Prefer body aim"), &cfg.ragebot.weapon[hooks::rage_weapon].prefer_body_aim);
			if (cfg.ragebot.weapon[hooks::rage_weapon].prefer_body_aim)
			{
				ImGui::Text(crypt_str("BODY KEY"));
				ImGui::SameLine();
				draw_keybind(crypt_str("Force body aim"), &cfg.ragebot.body_aim_key, crypt_str("##HOKEY_FORCE_BODY_AIM"));
			}

			ImGui::Checkbox(crypt_str("Static point scale"), &cfg.ragebot.weapon[hooks::rage_weapon].static_point_scale);
			if (cfg.ragebot.weapon[hooks::rage_weapon].static_point_scale)
			{
				ImGui::SliderFloat(crypt_str("Head scale"), &cfg.ragebot.weapon[hooks::rage_weapon].head_scale, 0.0f, 1.0f, cfg.ragebot.weapon[hooks::rage_weapon].head_scale ? crypt_str("%.2f") : crypt_str("None"));
				ImGui::SliderFloat(crypt_str("Body scale"), &cfg.ragebot.weapon[hooks::rage_weapon].body_scale, 0.0f, 1.0f, cfg.ragebot.weapon[hooks::rage_weapon].body_scale ? crypt_str("%.2f") : crypt_str("None"));
			}
		}
		ImGui::EndChild();
	}
}

void c_menu::aa_tab() // antiaim tab
{
	static auto type = 0;
	ImGui::SetCursorPos(ImVec2(80, 80));
	ImGui::MenuChild("General", ImVec2(310, 450));
	{
		ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
		ImGui::Checkbox(crypt_str("Enable"), &cfg.antiaim.enable);
		draw_combo(crypt_str("Anti-aim type"), cfg.antiaim.antiaim_type, antiaim_type, ARRAYSIZE(antiaim_type));

		if (cfg.antiaim.antiaim_type)
		{
			draw_combo(crypt_str("Desync"), cfg.antiaim.desync, desync, ARRAYSIZE(desync));

			if (cfg.antiaim.desync == 1)
			{
				draw_combo(crypt_str("LBY type"), cfg.antiaim.legit_lby_type, lby_type, ARRAYSIZE(lby_type));

				if (cfg.antiaim.desync)
				{
					ImGui::SetCursorPosX(9);
					ImGui::PushFont(c_menu::get().MenuFontRender);
					ImGui::Text("Invert desync");
					ImGui::PopFont();
					ImGui::SameLine();
					draw_keybind(crypt_str("Invert desync"), &cfg.antiaim.flip_desync, crypt_str("##HOTKEY_INVERT_DESYNC"));
				}
			}
		}
		else
		{
			draw_combo(crypt_str("Pitch"), cfg.antiaim.type[type].pitch, pitch, ARRAYSIZE(pitch));
			draw_combo(crypt_str("Yaw"), cfg.antiaim.type[type].yaw, yaw, ARRAYSIZE(yaw));
			draw_combo(crypt_str("Base angle"), cfg.antiaim.type[type].base_angle, baseangle, ARRAYSIZE(baseangle));

			if (cfg.antiaim.type[type].yaw)
			{
				ImGui::SetCursorPosX(1);
				ImGui::SliderInt(cfg.antiaim.type[type].yaw == 1 ? crypt_str("Jitter range") : crypt_str("Spin range"), &cfg.antiaim.type[type].range, 1, 180);
			}
		}

		ImGui::SetCursorPosX(9);
		ImGui::PushFont(c_menu::get().g_cxmenufont);
		ImGui::Text("Manual back");
		ImGui::PopFont();
		ImGui::SameLine();
		draw_keybind(crypt_str("Manual back"), &cfg.antiaim.manual_back, crypt_str("##HOTKEY_INVERT_BACK"));

		ImGui::SetCursorPosX(9);
		ImGui::PushFont(c_menu::get().g_cxmenufont);
		ImGui::Text("Manual left");
		ImGui::PopFont();
		ImGui::SameLine();
		draw_keybind(crypt_str("Manual left"), &cfg.antiaim.manual_left, crypt_str("##HOTKEY_INVERT_LEFT"));


		ImGui::SetCursorPosX(9);
		ImGui::PushFont(c_menu::get().g_cxmenufont);
		ImGui::Text("Manual right");
		ImGui::PopFont();
		ImGui::SameLine();
		draw_keybind(crypt_str("Manual right"), &cfg.antiaim.manual_right, crypt_str("##HOTKEY_INVERT_RIGHT"));

		if (cfg.antiaim.manual_back.key > KEY_NONE && cfg.antiaim.manual_back.key < KEY_MAX || cfg.antiaim.manual_left.key > KEY_NONE && cfg.antiaim.manual_left.key < KEY_MAX || cfg.antiaim.manual_right.key > KEY_NONE && cfg.antiaim.manual_right.key < KEY_MAX)
		{
			ImGui::Checkbox(crypt_str("Manuals indicator"), &cfg.antiaim.flip_indicator);
			ImGui::SameLine();
			ImGui::ColorEdit(crypt_str("##invc"), &cfg.antiaim.flip_indicator_color, ALPHA);
		}

		ImGui::Checkbox(crypt_str("Zero Land"), &cfg.antiaim.pitch_zero_on_land);

		draw_combo(crypt_str("Walk-type"), cfg.antiaim.walk_type, walktype, ARRAYSIZE(walktype));

		ImGui::SetCursorPosX(9);
		ImGui::PushFont(c_menu::get().g_cxmenufont);
		ImGui::Text("Auto peek");
		ImGui::PopFont();
		ImGui::SameLine();
		draw_keybind(crypt_str(""), &cfg.misc.automatic_peek, crypt_str("##AUTOPEEK__HOTKEY"));

		if (&cfg.misc.automatic_peek.key)
		{
			ImGui::Text("Color");
			ImGui::SameLine();
			ImGui::ColorEdit(crypt_str("##idsadsadsa"), &cfg.misc.automatic_peek_color, ALPHA);
		}

	}
	ImGui::EndChild();

	ImGui::SetCursorPos(ImVec2(410, 80));

	ImGui::MenuChild("Extra", ImVec2(310, 450));
	{
		ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
		draw_combo(crypt_str("Desync"), cfg.antiaim.type[type].desync, desync, ARRAYSIZE(desync));
		if (cfg.antiaim.type[type].desync)
		{
			if (type == ANTIAIM_STAND)
			{
				draw_combo(crypt_str("LBY type"), cfg.antiaim.lby_type, lby_type, ARRAYSIZE(lby_type));
			}

			if (type != ANTIAIM_STAND || !cfg.antiaim.lby_type)
			{
				ImGui::SliderInt(crypt_str("Desync range"), &cfg.antiaim.type[type].desync_range, 1, 60);
				ImGui::SliderInt(crypt_str("Inverted desync range"), &cfg.antiaim.type[type].inverted_desync_range, 1, 60);
				ImGui::SliderInt(crypt_str("Body lean"), &cfg.antiaim.type[type].body_lean, 0, 100);
				ImGui::SliderInt(crypt_str("Inverted body lean"), &cfg.antiaim.type[type].inverted_body_lean, 0, 100);
			}

			if (cfg.antiaim.type[type].desync == 1)
			{
				ImGui::SetCursorPosX(9);
				ImGui::PushFont(c_menu::get().MenuFontRender);
				ImGui::Text("Invert desync");
				ImGui::PopFont();
				ImGui::SameLine();
				draw_keybind(crypt_str("Invert desync"), &cfg.antiaim.flip_desync, crypt_str("##HOTKEY_INVERT_DESYNC"));
			}

		}

		ImGui::Checkbox(crypt_str("Fake Lag"), &cfg.antiaim.fakelag);
		if (cfg.antiaim.fakelag)
		{
			ImGui::Combo(crypt_str("Fake Lag Type"), &cfg.antiaim.fakelag_type, fakelags, ARRAYSIZE(fakelags));
			ImGui::SliderInt(crypt_str("Limit"), &cfg.antiaim.fakelag_amount, 1, 14);

			//
			draw_multicombo(crypt_str("Fake Lag Triggers"), cfg.antiaim.fakelag_enablers, lagstrigger, ARRAYSIZE(lagstrigger), preview);

			auto enabled_fakelag_triggers = false;

			for (auto i = 0; i < ARRAYSIZE(lagstrigger); i++)
				if (cfg.antiaim.fakelag_enablers[i])
					enabled_fakelag_triggers = true;

			if (enabled_fakelag_triggers)
				ImGui::SliderInt(crypt_str("Triggers Limit"), &cfg.antiaim.triggers_fakelag_amount, 1, 14);
		}
	}
	ImGui::EndChild();
}

void c_menu::visuals_tab() // players + visuals
{
	static int local_tab = 0;
	static int enemyorteam_tab = 0;

	if (vis_tab == 0)
	{
		ImGui::SetCursorPos(ImVec2(80, 80));
		ImGui::MenuChild("Esp", ImVec2(310, 450));
		{
			ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();

			draw_combo(crypt_str("Set Team"), player, set_team, ARRAYSIZE(set_team));

			ImGui::Checkbox("Enable", &cfg.player.enable);
			ImGui::Checkbox(crypt_str("Name"), &cfg.player.type[player].name);

			ImGui::Checkbox(crypt_str("Health Bar"), &cfg.player.type[player].health);

			if (cfg.player.type[player].health)
			{
				ImGui::SetCursorPosX(9);
				ImGui::Text("Color");
				ImGui::SameLine();
				ImGui::ColorEdit(crypt_str("##healthcolor"), &cfg.player.type[player].health_color, ALPHA);
			}

			for (auto i = 0, j = 0; i < ARRAYSIZE(flags); i++)
			{
				if (cfg.player.type[player].flags[i])
				{
					if (j)
						preview += crypt_str(", ") + (std::string)flags[i];
					else
						preview = flags[i];

					j++;
				}
			}

			draw_multicombo(crypt_str("Flags"), cfg.player.type[player].flags, flags, ARRAYSIZE(flags), preview);
			draw_multicombo(crypt_str("Weapon"), cfg.player.type[player].weapon, weaponplayer, ARRAYSIZE(weaponplayer), preview);

			if (cfg.player.type[player].weapon[WEAPON_ICON] || cfg.player.type[player].weapon[WEAPON_TEXT])
			{
				ImGui::SetCursorPosX(9);
				ImGui::Text(crypt_str("Color"));
				ImGui::SameLine();
				ImGui::ColorEdit(crypt_str("##weapcolor"), &cfg.player.type[player].weapon_color, ALPHA);
			}

			ImGui::Checkbox(crypt_str("Ammo bar"), &cfg.player.type[player].ammo);

			if (cfg.player.type[player].ammo)
			{
				ImGui::SetCursorPosX(9);
				ImGui::Text("Color");
				ImGui::SameLine();
				ImGui::ColorEdit(crypt_str("##ammocolor"), &cfg.player.type[player].ammobar_color, ALPHA);
			}

			ImGui::Checkbox(crypt_str("Glow"), &cfg.player.type[player].glow);

			if (cfg.player.type[player].glow)
			{
				ImGui::SetCursorPosX(9);
				ImGui::Text("Color");
				ImGui::SameLine();
				ImGui::ColorEdit(crypt_str("##glowcolor"), &cfg.player.type[player].glow_color, ALPHA);
			}

			if (cfg.player.type[player].footsteps)
			{
				ImGui::SliderInt(crypt_str("Thickness"), &cfg.player.type[player].thickness, 1, 10);
				ImGui::SliderInt(crypt_str("Radius"), &cfg.player.type[player].radius, 50, 500);
			}
		}
		ImGui::EndChild();

		ImGui::SetCursorPos(ImVec2(410, 80));

		ImGui::MenuChild("Chams", ImVec2(310, 450));
		{
			ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();

			if (player == 2)
				draw_combo(crypt_str("Type"), cfg.player.local_chams_type, local_chams_type, ARRAYSIZE(local_chams_type));

			if (player != 2 || !cfg.player.local_chams_type)
				draw_multicombo(crypt_str("Chams"), cfg.player.type[player].chams, cfg.player.type[player].chams[PLAYER_CHAMS_VISIBLE] ? chamsvisact : chamsvis, cfg.player.type[player].chams[PLAYER_CHAMS_VISIBLE] ? ARRAYSIZE(chamsvisact) : ARRAYSIZE(chamsvis), preview);

			if (cfg.player.type[player].chams[PLAYER_CHAMS_VISIBLE] || player == 2 && cfg.player.local_chams_type)
			{

				if (player == 2 && cfg.player.local_chams_type)
				{
					ImGui::Checkbox(crypt_str("Enable desync chams"), &cfg.player.fake_chams_enable);
					ImGui::Checkbox(crypt_str("Visualize lag"), &cfg.player.visualize_lag);
					ImGui::Checkbox(crypt_str("Layered"), &cfg.player.layered);


					draw_combo(crypt_str("Player chams material"), cfg.player.fake_chams_type, chamstype, ARRAYSIZE(chamstype));

					//draw_combo(crypt_str("Player models"), cfg.player.player_models, player_models, ARRAYSIZE(player_models));
					ImGui::SetCursorPosX(9);
					ImGui::Text(crypt_str("Color "));
					ImGui::SameLine();
					ImGui::ColorEdit(crypt_str("##fakechamscolor"), &cfg.player.fake_chams_color, ALPHA);

					if (cfg.player.fake_chams_type != 6)
					{
						ImGui::Checkbox(crypt_str("Double material "), &cfg.player.fake_double_material);
						ImGui::SameLine();
						ImGui::ColorEdit(crypt_str("##doublematerialcolor"), &cfg.player.fake_double_material_color, ALPHA);
					}
					ImGui::Checkbox(crypt_str("Animated material"), &cfg.player.fake_animated_material);
					ImGui::SameLine();
					ImGui::ColorEdit(crypt_str("##animcolormat"), &cfg.player.fake_animated_material_color, ALPHA);
				}
				else
				{
					draw_combo(crypt_str("Player chams material"), cfg.player.type[player].chams_type, chamstype, ARRAYSIZE(chamstype));

					if (cfg.player.type[player].chams[PLAYER_CHAMS_VISIBLE])
					{
						ImGui::ColorEdit(crypt_str("##chamsvisible"), &cfg.player.type[player].chams_color, ALPHA);
						ImGui::SameLine();
						ImGui::PushFont(c_menu::get().MenuFontRender);
						ImGui::SetCursorPosX(9);
						ImGui::Text(crypt_str("Visible "));
						ImGui::PopFont();
					}

					if (cfg.player.type[player].chams[PLAYER_CHAMS_VISIBLE] && cfg.player.type[player].chams[PLAYER_CHAMS_INVISIBLE])
					{
						ImGui::ColorEdit(crypt_str("##chamsinvisible"), &cfg.player.type[player].xqz_color, ALPHA);
						ImGui::SameLine();
						ImGui::PushFont(c_menu::get().MenuFontRender);
						ImGui::SetCursorPosX(9);
						ImGui::Text(crypt_str("Invisible "));;
						ImGui::PopFont();
					}

					if (cfg.player.type[player].chams_type != 6)
					{
						ImGui::Checkbox(crypt_str("Double material "), &cfg.player.type[player].double_material);
						ImGui::SameLine();
						ImGui::ColorEdit(crypt_str("##doublematerialcolor"), &cfg.player.type[player].double_material_color, ALPHA);
					}

					ImGui::Checkbox(crypt_str("Animated material"), &cfg.player.type[player].animated_material);
					ImGui::SameLine();
					ImGui::ColorEdit(crypt_str("##animcolormat"), &cfg.player.type[player].animated_material_color, ALPHA);

					if (player == 0)
					{
						ImGui::Checkbox(crypt_str("Backtrack chams"), &cfg.player.backtrack_chams);

						if (cfg.player.backtrack_chams)
						{
							draw_combo(crypt_str("Backtrack chams material"), cfg.player.backtrack_chams_material, chamstype, ARRAYSIZE(chamstype));

							ImGui::ColorEdit(crypt_str("##backtrackcolor"), &cfg.player.backtrack_chams_color, ALPHA);
							ImGui::SameLine();
							ImGui::PushFont(c_menu::get().MenuFontRender);
							ImGui::SetCursorPosX(9);
							ImGui::Text(crypt_str("Color "));
							ImGui::PopFont();

						}
					}
				}
			}

			if (player == 0 || player == 1)
			{
				ImGui::Checkbox(crypt_str("Ragdoll chams"), &cfg.player.type[player].ragdoll_chams);

				if (cfg.player.type[player].ragdoll_chams)
				{
					draw_combo(crypt_str("Ragdoll chams material"), cfg.player.type[player].ragdoll_chams_material, chamstype, ARRAYSIZE(chamstype));

					ImGui::ColorEdit(crypt_str("##ragdollcolor"), &cfg.player.type[player].ragdoll_chams_color, ALPHA);
					ImGui::SameLine();
					ImGui::PushFont(c_menu::get().MenuFontRender);
					ImGui::SetCursorPosX(9);
					ImGui::Text(crypt_str("Color "));
					ImGui::PopFont();
				}
			}
			else if (!cfg.player.local_chams_type)
			{
				ImGui::Checkbox(crypt_str("Transparency in scope"), &cfg.player.transparency_in_scope);

				if (cfg.player.transparency_in_scope)
					ImGui::SliderFloat(crypt_str("Alpha"), &cfg.player.transparency_in_scope_amount, 0.0f, 1.0f);
			}
		}
		ImGui::EndChild();
	}
	else if (vis_tab == 1)
	{
		ImGui::SetCursorPos(ImVec2(80, 80));
		ImGui::MenuChild("World", ImVec2(310, 450));
		{
			ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
			ImGui::Checkbox(crypt_str("Rain"), &cfg.esp.rain);
			ImGui::Checkbox(crypt_str("Full bright"), &cfg.esp.bright);

			draw_combo(crypt_str("Skybox"), cfg.esp.skybox, skybox, ARRAYSIZE(skybox));

			if (cfg.esp.skybox)
			{
				ImGui::Text(crypt_str("Color "));
				ImGui::SameLine();
				ImGui::ColorEdit(crypt_str("##skyboxcolor"), &cfg.esp.skybox_color, NOALPHA);
			}

			if (cfg.esp.skybox == 21)
			{
				static char sky_custom[64] = "\0";

				if (!cfg.esp.custom_skybox.empty())
					strcpy_s(sky_custom, sizeof(sky_custom), cfg.esp.custom_skybox.c_str());

				ImGui::Text(crypt_str("Name"));
				ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);

				if (ImGui::InputText(crypt_str("##customsky"), sky_custom, sizeof(sky_custom)))
					cfg.esp.custom_skybox = sky_custom;

				ImGui::PopStyleVar();
			}

			ImGui::Checkbox(crypt_str("Color modulation"), &cfg.esp.nightmode);

			if (cfg.esp.nightmode)
			{
				ImGui::Text(crypt_str("World color "));
				ImGui::SameLine();
				ImGui::ColorEdit(crypt_str("##worldcolor"), &cfg.esp.world_color, ALPHA);

				ImGui::Text(crypt_str("Props color "));
				ImGui::SameLine();
				ImGui::ColorEdit(crypt_str("##propscolor"), &cfg.esp.props_color, ALPHA);
			}

			ImGui::Checkbox(crypt_str("World modulation"), &cfg.esp.world_modulation);

			if (cfg.esp.world_modulation)
			{
				ImGui::SliderFloat(crypt_str("Bloom"), &cfg.esp.bloom, 0.0f, 750.0f);
				ImGui::SliderFloat(crypt_str("Exposure"), &cfg.esp.exposure, 0.0f, 2000.0f);
				ImGui::SliderFloat(crypt_str("Ambient"), &cfg.esp.ambient, 0.0f, 1500.0f);
			}

			ImGui::Checkbox(crypt_str("Fog modulation"), &cfg.esp.fog);

			if (cfg.esp.fog)
			{
				ImGui::SliderInt(crypt_str("Distance"), &cfg.esp.fog_distance, 0, 2500);
				ImGui::SliderInt(crypt_str("Density"), &cfg.esp.fog_density, 0, 100);

				ImGui::Text(crypt_str("Color "));
				ImGui::SameLine();
				ImGui::ColorEdit(crypt_str("##fogcolor"), &cfg.esp.fog_color, NOALPHA);
			}
		}
		ImGui::EndChild();

		ImGui::SetCursorPos(ImVec2(410, 80));

		ImGui::MenuChild("Render", ImVec2(310, 450));
		{
			ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
			ImGui::Checkbox(crypt_str("Enabled"), &cfg.player.enable);

			ImGui::SliderInt(crypt_str("Field of view"), &cfg.esp.fov, 0, 89);

			//ImGui::Checkbox(crypt_str("Thirdperson"), &cfg.esp);

			ImGui::SetCursorPosX(9);
			ImGui::Text("Thirdperson");
			//ImGui::PopFont();
			ImGui::SameLine();
			draw_keybind(crypt_str("Thirdperson"), &cfg.misc.thirdperson_toggle, crypt_str("##TPKEY__HOTKEY"));

			if (cfg.misc.thirdperson_toggle.key > KEY_NONE && cfg.misc.thirdperson_toggle.key < KEY_MAX)
			{
				ImGui::SliderInt(crypt_str("Third person distance"), &cfg.misc.thirdperson_distance, 50, 300);
				ImGui::Checkbox(crypt_str("Third person when dead"), &cfg.misc.thirdperson_when_spectating);
			}

			draw_multicombo(crypt_str("Indicators"), cfg.esp.indicators, indicators, ARRAYSIZE(indicators), preview);

			draw_multicombo(crypt_str("Removals"), cfg.esp.removals, removals, ARRAYSIZE(removals), preview);

			if (cfg.esp.removals[REMOVALS_ZOOM])
				ImGui::Checkbox(crypt_str("Fix zoom sensivity"), &cfg.esp.fix_zoom_sensivity);

			if (cfg.esp.removals[REMOVALS_SCOPE])
				ImGui::Checkbox(crypt_str("Custom Scope"), &cfg.esp.scopee);

			if (cfg.esp.scopee)
			{
				ImGui::Text("  Scope color");
				ImGui::SameLine();
				ImGui::ColorEdit(crypt_str("##removals_scope_color"), &cfg.esp.removals_scope_color, ALPHA);
				ImGui::SliderInt(crypt_str("Scope speed"), &cfg.esp.removals_scope_speed, 1, 10);
				ImGui::SliderInt(crypt_str("Scope distance"), &cfg.esp.removals_scope_distance, 1, 20);
				ImGui::SliderInt(crypt_str("Scope length"), &cfg.esp.removals_scope_length, 1, 20);
			}

			ImGui::Checkbox(crypt_str("Client impacts"), &cfg.esp.client_bullet_impacts);
			if (cfg.esp.client_bullet_impacts)
			{
				ImGui::Text("Color");
				ImGui::SameLine();
				ImGui::ColorEdit(crypt_str("##clientbulletimpacts"), &cfg.esp.client_bullet_impacts_color, ALPHA);
			}

			ImGui::Checkbox(crypt_str("Server impacts"), &cfg.esp.server_bullet_impacts);
			if (cfg.esp.server_bullet_impacts)
			{
				ImGui::Text("Color");
				ImGui::SameLine();
				ImGui::ColorEdit(crypt_str("##serverbulletimpacts"), &cfg.esp.server_bullet_impacts_color, ALPHA);
			}

			ImGui::Checkbox(crypt_str("Local tracers"), &cfg.esp.bullet_tracer);
			if (cfg.esp.bullet_tracer)
			{
				ImGui::Text("Color");
				ImGui::SameLine();
				ImGui::ColorEdit(crypt_str("##bulltracecolor"), &cfg.esp.bullet_tracer_color, ALPHA);
			}

			ImGui::Checkbox(crypt_str("Enemy tracers"), &cfg.esp.enemy_bullet_tracer);
			if (cfg.esp.enemy_bullet_tracer)
			{
				ImGui::Text("Color");
				ImGui::SameLine();
				ImGui::ColorEdit(crypt_str("##enemybulltracecolor"), &cfg.esp.enemy_bullet_tracer_color, ALPHA);
			}
			ImGui::Checkbox("Damage marker", &cfg.esp.damage_marker);
			draw_multicombo(crypt_str("Hit marker"), cfg.esp.hitmarker, hitmarkers, ARRAYSIZE(hitmarkers), preview);

			ImGui::Checkbox(crypt_str("Penetration crosshair"), &cfg.esp.penetration_reticle);

			ImGui::Checkbox(crypt_str("Grenade prediction"), &cfg.esp.grenade_prediction);

			if (cfg.esp.grenade_prediction)
			{
				ImGui::Text("Color");
				ImGui::SameLine();
				ImGui::ColorEdit(crypt_str("##tracergrenpredcolor"), &cfg.esp.grenade_prediction_tracer_color, ALPHA);
			}

			ImGui::Checkbox(crypt_str("Grenade proximity warning"), &cfg.esp.grenade_proximity_warning);


			ImGui::Checkbox(crypt_str("Fire timer"), &cfg.esp.molotov_timer);

			if (cfg.esp.molotov_timer)
			{
				ImGui::Text("Color");
				ImGui::SameLine();
				ImGui::ColorEdit(crypt_str("##molotovcolor"), &cfg.esp.molotov_timer_color, ALPHA);
			}

			ImGui::Checkbox(crypt_str("Smoke timer"), &cfg.esp.smoke_timer);
			if (cfg.esp.smoke_timer)
			{
				ImGui::Text("Color");
				ImGui::SameLine();
				ImGui::ColorEdit(crypt_str("##smokecolor"), &cfg.esp.smoke_timer_color, ALPHA);
			}

			ImGui::Checkbox(crypt_str("Bomb indicator"), &cfg.esp.bomb_timer);
			draw_multicombo(crypt_str("Weapon ESP"), cfg.esp.weapon, weaponesp, ARRAYSIZE(weaponesp), preview);

			if (cfg.esp.weapon[WEAPON_ICON] || cfg.esp.weapon[WEAPON_TEXT] || cfg.esp.weapon[WEAPON_DISTANCE])
			{
				ImGui::SameLine();
				ImGui::ColorEdit(crypt_str("##weaponcolor"), &cfg.esp.weapon_color, ALPHA);
			}

			if (cfg.esp.weapon[WEAPON_BOX])
			{
				ImGui::SameLine();
				ImGui::ColorEdit(crypt_str("##weaponboxcolor"), &cfg.esp.box_color, ALPHA);
			}

			if (cfg.esp.weapon[WEAPON_GLOW])
			{
				ImGui::SameLine();
				ImGui::ColorEdit(crypt_str("##weaponglowcolor"), &cfg.esp.weapon_glow_color, ALPHA);
			}

			if (cfg.esp.weapon[WEAPON_AMMO])
			{
				ImGui::SameLine();
				ImGui::ColorEdit(crypt_str("##weaponammocolor"), &cfg.esp.weapon_ammo_color, ALPHA);
			}
		}
		ImGui::EndChild();
	}
}

void c_menu::misc_tab() // misc
{
	if (mi_tab == 0)
	{
		ImGui::SetCursorPos(ImVec2(80, 80));
		ImGui::MenuChild("Movement", ImVec2(310, 450));
		{
			ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
			ImGui::Checkbox("Bunnyhop", &cfg.misc.bunnyhop);
			ImGui::Checkbox(crypt_str("Airstrafe"), (bool*)&cfg.misc.airstrafe);
			ImGui::Checkbox(crypt_str("Crouch in air"), &cfg.misc.crouch_in_air);
			ImGui::Checkbox(crypt_str("Fast stop"), &cfg.misc.fast_stop);
			ImGui::Checkbox(crypt_str("Slide walk"), &cfg.misc.slidewalk);
			ImGui::Checkbox(crypt_str("No duck cooldown"), &cfg.misc.noduck);

			ImGui::SetCursorPosX(9);
			ImGui::Text("Slow walk");
			ImGui::SameLine();
			draw_keybind(crypt_str("Slow walk"), &cfg.misc.slowwalk_key, crypt_str("##SLOWWALK__HOTKEY"));

			if (cfg.misc.slowwalk_key.key > KEY_NONE && cfg.misc.slowwalk_key.key < KEY_MAX)
			{
				ImGui::SliderInt(crypt_str("Strength"), &cfg.misc.slowwalk_speed, 0, 100, true);
			}

			ImGui::Spacing();


			ImGui::Checkbox(crypt_str("Weapon chams"), &cfg.esp.weapon_chams);

			ImGui::Spacing();

			if (cfg.esp.weapon_chams)
			{
				ImGui::Text("Weapon Color");
				ImGui::SameLine();
				ImGui::ColorEdit(crypt_str("##weaponchams"), &cfg.esp.weapon_chams_color, ALPHA);
				ImGui::Spacing();
				draw_combo(crypt_str("Weapon chams type"), cfg.esp.weapon_chams_type, chamstype, ARRAYSIZE(chamstype));
			}
		}
		ImGui::EndChild();

		ImGui::SetCursorPos(ImVec2(410, 80));

		ImGui::MenuChild("Extra", ImVec2(310, 450));
		{
			ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
			ImGui::Checkbox(crypt_str("Unlock inventory access"), &cfg.misc.inventory_access);
			ImGui::Checkbox(crypt_str("Preserve killfeed"), &cfg.esp.preserve_killfeed);
			ImGui::Checkbox(crypt_str("Aspect ratio"), &cfg.misc.aspect_ratio);

			if (cfg.misc.aspect_ratio)
			{
				padding(0, -5);
				ImGui::SliderFloat(crypt_str("Amount"), &cfg.misc.aspect_ratio_amount, 1.0f, 2.0f);
			}

			ImGui::Checkbox(crypt_str("Attachments"), &cfg.esp.attachment_chams);

			if (cfg.esp.attachment_chams)
			{
				draw_combo(crypt_str("Attachment chams type"), cfg.esp.attachment_chams_material, chamstype, ARRAYSIZE(chamstype));
				ImGui::SetCursorPosX(9);
				ImGui::Text(crypt_str("Attachment Color "));
				ImGui::SameLine();
				ImGui::ColorEdit(crypt_str("##logcolor"), &cfg.esp.attachment_chams_color, ALPHA);
			}

		}
		ImGui::EndChild();
	}
	else if (mi_tab == 1)
	{
		ImGui::SetCursorPos(ImVec2(80, 80));
		ImGui::MenuChild("Info", ImVec2(310, 450));
		{
			ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
			ImGui::Checkbox(crypt_str("Watermark"), &cfg.menu.watermark);

			ImGui::Checkbox(crypt_str("Keybinds"), &cfg.menu.keybinds);

			ImGui::Checkbox(crypt_str("Spectators list"), &cfg.misc.spectators_list);
			draw_combo(crypt_str("Hitsound"), cfg.esp.hitsound, sounds, ARRAYSIZE(sounds));

			draw_multicombo(crypt_str("Logs"), cfg.misc.events_to_log, events, ARRAYSIZE(events), preview);
			padding(0, 3);
			draw_multicombo(crypt_str("Logs output"), cfg.misc.log_output, events_output, ARRAYSIZE(events_output), preview);

			if (cfg.misc.events_to_log[EVENTLOG_HIT] || cfg.misc.events_to_log[EVENTLOG_ITEM_PURCHASES] || cfg.misc.events_to_log[EVENTLOG_BOMB])
			{
				ImGui::Text(crypt_str("Color "));
				ImGui::SameLine();
				ImGui::ColorEdit(crypt_str("##logcolor"), &cfg.misc.log_color, ALPHA);
			}
			ImGui::Checkbox(crypt_str("Show CS:GO logs"), &cfg.misc.show_default_log);
		}
		ImGui::EndChild();

		ImGui::SetCursorPos(ImVec2(410, 80));

		ImGui::MenuChild("Extra", ImVec2(310, 450));
		{
			ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
			ImGui::Checkbox(crypt_str("Clantag"), &cfg.misc.clantag_spammer);
			ImGui::Checkbox(crypt_str("Enable buybot"), &cfg.misc.buybot_enable);

			if (cfg.misc.buybot_enable)
			{
				draw_combo(crypt_str("Snipers"), cfg.misc.buybot1, mainwep, ARRAYSIZE(mainwep));
				padding(0, 3);
				draw_combo(crypt_str("Pistols"), cfg.misc.buybot2, secwep, ARRAYSIZE(secwep));
				padding(0, 3);
				draw_multicombo(crypt_str("Other"), cfg.misc.buybot3, grenades, ARRAYSIZE(grenades), preview);
			}
		}
		ImGui::EndChild();
	}
}

void c_menu::settings_tab() // cfg + lua
{
	if (lua_tab == 0)
	{
		static bool is_sure_check = false;
		static float started_think = 0;
		static std::string selected_name = "";
		static char config_name[30] = "\0";

		ImGui::SetCursorPos(ImVec2(80, 80));
		ImGui::MenuChild("Config List", ImVec2(310, 450));
		{
			ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
			ImGui::InputText(crypt_str("Config Name"), config_name, sizeof(config_name));
			ImGui::Spacing(); ImGui::Spacing();

			cfg_manager->config_files();
			files = cfg_manager->files;

			for (auto file : files)
			{
				bool is_selected = selected_name == file;

				ImGui::SetCursorPosX(7);
				if (ImGui::cfgtab(file.c_str(), is_selected, ImVec2(243, 35)))
				{
					selected_name = is_selected ? "" : file;

					is_sure_check = false;
					started_think = 0;
				}
			}

			if (selected_name.empty())
			{
				selected_name = "";
			}
		}
		ImGui::EndChild();

		ImGui::SetCursorPos(ImVec2(410, 80));

		ImGui::MenuChild("Config Settings", ImVec2(310, 450));
		{
			ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
			if ((ImGui::CustomButton(crypt_str("Create new..."), crypt_str("##CreateConfig"), ImVec2(265, 30), true, c_menu::get().settingicons, "3")))
				add_config(config_name);

			if ((ImGui::CustomButton(crypt_str("Open Config Directory"), crypt_str("##OpenConfigDirectory"), ImVec2(265, 26), true, c_menu::get().settingicons, "2")))
			{
				std::string folder = crypt_str("C:\\Program Files (x86)\\Steam\\steamapps\\common\\Counter-Strike Global Offensive\\Lambda\\");
				CreateDirectory(folder.c_str(), NULL);

				ShellExecute(NULL, crypt_str("open"), folder.c_str(), NULL, NULL, SW_SHOWNORMAL);
			}

			if (!selected_name.empty())
			{
				//Load confirmation
				{
					if (prenext_load && m_globals()->m_realtime < load_time + 3.f)
					{
						if ((ImGui::CustomButton(crypt_str(" Confirm?"), crypt_str("##ConfirmLoad"), ImVec2(265, 26), true, c_menu::get().settingicons, "5")) && !selected_name.empty()) {
							load_config(selected_name);
							prenext_load = false;
						}
					}
					else
						prenext_load = false;
				}

				//Load button
				{
					if (!prenext_load)
					{
						if ((ImGui::CustomButton(crypt_str(" Load"), crypt_str("##load"), ImVec2(265, 26), true, c_menu::get().settingicons, "5")) && !selected_name.empty()) {
							load_time = m_globals()->m_realtime;
							prenext_load = true;
							//load_config(selected_name);
						}
					}
				}

				//Save confirmation
				{
					if (prenext_save && m_globals()->m_realtime < save_time + 3.f)
					{
						if ((ImGui::CustomButton(crypt_str(" Confirm?"), crypt_str("##ConfirmSave"), ImVec2(265, 26), true, c_menu::get().settingicons, "4")) && !selected_name.empty())
						{
							save_config(selected_name);
							prenext_save = false;
						}
					}
					else
						prenext_save = false;
				}

				//Save button
				{
					if (!prenext_save)
					{
						if ((ImGui::CustomButton(crypt_str(" Save"), crypt_str("##Save"), ImVec2(265, 26), true, c_menu::get().settingicons, "4")) && !selected_name.empty())
						{
							save_time = m_globals()->m_realtime;
							prenext_save = true;
						}
					}
				}

				//Delete confirmation
				{
					if (prenext_delete && m_globals()->m_realtime < delete_time + 3.f)
					{
						if ((ImGui::CustomButton(crypt_str("Confirm?"), crypt_str("##ConfirmDelete"), ImVec2(265, 26), true, c_menu::get().settingicons, "7")) && !selected_name.empty())
						{
							prenext_delete = false;
							remove_config(selected_name); selected_name = "";
						}
					}
					else
						prenext_delete = false;
				}

				//Delete button
				{
					if (!prenext_delete)
					{
						if ((ImGui::CustomButton(crypt_str("Delete"), crypt_str("##Delete"), ImVec2(265, 26), true, c_menu::get().settingicons, "7")) && !selected_name.empty())
						{
							delete_time = m_globals()->m_realtime;
							prenext_delete = true;
						}
					}
				}
			}
		}
		ImGui::EndChild();
	}
	else if (lua_tab == 1)
	{

	}
}

void c_menu::skins_tab() // skins
{
	static char custom_name[124] = "";
	static int seed = 0;
	bool stat_trak = false;
	static float wear = 0.0f;
	ImGui::SetCursorPos(ImVec2(80, 80));
	ImGui::MenuChild("Deployed", ImVec2(310, 450));
	{

	}
	ImGui::EndChild();

	ImGui::SetCursorPos(ImVec2(410, 80));

	ImGui::MenuChild("Extra", ImVec2(310, 450));
	{
		ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
		auto& selected_entry = cfg.skins.skinChanger[c_menu::get().current_profile];
		selected_entry.itemIdIndex = c_menu::get().current_profile;

		draw_combo(crypt_str("Player models"), cfg.player.player_models, player_models, ARRAYSIZE(player_models));

		ImGui::Spacing();

		if ((ImGui::CustomButton(crypt_str("Force Update"), crypt_str("##updater"), ImVec2(265, 26), false, c_menu::get().settingicons, "0")))
			SkinChanger::scheduleHudUpdate();
	}
	ImGui::EndChild();
}

void c_menu::subtabs()
{
	if (tab_static == 2)
	{
		ImGui::SetCursorPos(ImVec2{ 70 , -1 });
		ImGui::PushFont(g_last);
		if (ImGui::subtab("P", "", vis_tab == 0))
			vis_tab = 0;
		ImGui::PopFont();

		ImGui::SetCursorPos(ImVec2{ 120 , -1 });
		ImGui::PushFont(g_last);
		if (ImGui::subtab("W", "", vis_tab == 1))
			vis_tab = 1;
		ImGui::PopFont();
	}
	else if (tab_static == 0)
	{
		ImGui::SetCursorPos(ImVec2{ 70 , -1 });
		ImGui::PushFont(g_icons);
		if (ImGui::subtab("F", "", rg_tab == 0))
			rg_tab = 0;
		ImGui::PopFont();

		ImGui::SetCursorPos(ImVec2{ 120 , -1 });
		ImGui::PushFont(g_icons);
		if (ImGui::subtab("Z", "", rg_tab == 1))
			rg_tab = 1;
		ImGui::PopFont();
	}
	else if (tab_static == 3)
	{
		ImGui::SetCursorPos(ImVec2{ 70 , -1 });
		ImGui::PushFont(g_icons);
		if (ImGui::subtab("A", "", mi_tab == 0))
			mi_tab = 0;
		ImGui::PopFont();

		ImGui::SetCursorPos(ImVec2{ 120 , -1 });
		ImGui::PushFont(g_last);
		if (ImGui::subtab("M", "", mi_tab == 1))
			mi_tab = 1;
		ImGui::PopFont();
	}
	else if (tab_static == 4)
	{
		ImGui::SetCursorPos(ImVec2{ 70 , -1 });
		ImGui::PushFont(g_icons);
		if (ImGui::subtab("A", "", lua_tab == 0))
			lua_tab = 0;
		ImGui::PopFont();

		ImGui::SetCursorPos(ImVec2{ 120 , -1 });
		ImGui::PushFont(g_icons);
		if (ImGui::subtab("V", "", lua_tab == 1))
			lua_tab = 1;
		ImGui::PopFont();
	}
};

void c_menu::tabs()
{
	ImGui::PushFont(g_icons);
	ImGui::SetCursorPosY(90);
	ImGui::SetCursorPosX(7);
	ImGui::BeginGroup();
	{
		if (ImGui::tab("F", "", !tab_static)) tab_static = 0;
	}
	ImGui::EndGroup();

	ImGui::SetCursorPosY(140);
	ImGui::SetCursorPosX(7);
	ImGui::BeginGroup();
	{
		if (ImGui::tab("D", "", tab_static == 1)) tab_static = 1;
	}
	ImGui::EndGroup();

	ImGui::SetCursorPosY(190);
	ImGui::SetCursorPosX(7);
	ImGui::BeginGroup();
	{
		if (ImGui::tab("G", "", tab_static == 2)) tab_static = 2;
	}
	ImGui::EndGroup();

	ImGui::SetCursorPosY(240);
	ImGui::SetCursorPosX(7);
	ImGui::BeginGroup();
	{
		if (ImGui::tab("A", "", tab_static == 3)) tab_static = 3;
	}
	ImGui::EndGroup();

	ImGui::SetCursorPosY(290);
	ImGui::SetCursorPosX(7);
	ImGui::BeginGroup();
	{
		if (ImGui::tab("V", "", tab_static == 4)) tab_static = 4;
	}
	ImGui::EndGroup();

	ImGui::SetCursorPosY(340);
	ImGui::SetCursorPosX(7);
	ImGui::BeginGroup();
	{
		if (ImGui::tab("C", "", tab_static == 5)) tab_static = 5;
	}
	ImGui::EndGroup();
	ImGui::PopFont();

	switch (tab_static)
	{
	case 0:     rage_tab();       break;
	case 1:     aa_tab();       break;
	case 2:     visuals_tab();     break;
	case 3:     misc_tab();     break;
	case 4:     settings_tab();     break;
	case 5:     skins_tab();     break;
	}
}

void c_menu::draw(bool is_open)
{
	if (is_open && public_alpha < 1)
		c_menu::get().public_alpha += 0.55f;
	else if (!is_open && public_alpha > 0)
		c_menu::get().public_alpha -= 0.55f;

	if (public_alpha < 0.01f)
		return;

	auto& ss = ImGui::GetStyle();
	ss.FrameRounding = 6;
	ss.ChildRounding = 10;
	ss.PopupRounding = 5;
	ss.ScrollbarRounding = 0;
	ss.ScrollbarSize = 5;
	ss.FramePadding = ImVec2(2, 1);

	auto s = ImVec2{}, p = ImVec2{}, gs = ImVec2{ 916, 646 };
	ImGui::SetNextWindowSize(ImVec2(gs));
	if (ImGui::Begin(("Lambda"), nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar))
	{
		s = ImVec2(ImGui::GetWindowSize().x - ImGui::GetStyle().WindowPadding.x * 2, ImGui::GetWindowSize().y - ImGui::GetStyle().WindowPadding.y * 2);
		p = ImVec2(ImGui::GetWindowPos().x + ImGui::GetStyle().WindowPadding.x, ImGui::GetWindowPos().y + ImGui::GetStyle().WindowPadding.y);
		auto draw = ImGui::GetWindowDrawList();

		float dpi_scale = 1.f;

		//?????? ???
		draw->AddRectFilled(ImVec2(p.x, p.y), ImVec2(p.x + 730, p.y + 600), ImColor(29, 29, 29), 10);
		//??????? ??? ?????
		draw->AddRectFilled(ImVec2(p.x + 50, p.y), ImVec2(p.x + 730, p.y + 55), ImColor(37, 37, 37), 10, ImDrawCornerFlags_Top);
		//????? ????
		draw->AddRectFilled(p, ImVec2(p.x + 55, p.y + 600), ImColor(37, 37, 37), 10, ImDrawCornerFlags_Left);

		//?????? ????
		draw->AddRectFilled(ImVec2(p.x, p.y + 580), ImVec2(p.x + 730, p.y + 600), ImColor(25, 25, 25), 10, ImDrawCornerFlags_Bot);

		ImGui::SetCursorPos(ImVec2(-1, -1));
		ImGui::Image(logo, ImVec2(ImVec2(75.f * dpi_scale, 75.f * dpi_scale)));


		ImGui::PushFont(g_cxm);
		std::string Welcome = "Welcome to : ";
		draw->AddText(ImVec2(p.x + 510, p.y + 583), ImColor(100, 100, 100), Welcome.c_str());
		ImGui::PopFont();

		std::string USER = "Semxxz";

		ImGui::PushFont(g_cxm);
		draw->AddText(ImVec2(p.x + 585, p.y + 583), ImColor(100, 125, 200), USER.c_str());
		ImGui::PopFont();

		ImGui::PushFont(g_cxm);
		std::string day = "  | Till : LT";
		draw->AddText(ImVec2(p.x + 630, p.y + 583), ImColor(100, 100, 100), day.c_str());
		ImGui::PopFont();


		ImGui::PushFont(g_cxm);
		std::string Lambda = "Lambda for Counter Strike: Global Offensive";
		draw->AddText(ImVec2(p.x + 5, p.y + 583), ImColor(100, 100, 100), Lambda.c_str());
		ImGui::PopFont();

		{
			subtabs();
			tabs();
		}

	}
	ImGui::End();
}