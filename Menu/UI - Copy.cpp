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

std::string get_wep(int id, int custom_index = -1, bool knife = true)
{
	if (custom_index > -1)
	{
		if (knife)
		{
			switch (custom_index)
			{
			case 0: return crypt_str("weapon_knife");
			case 1: return crypt_str("weapon_bayonet");
			case 2: return crypt_str("weapon_knife_css");
			case 3: return crypt_str("weapon_knife_skeleton");
			case 4: return crypt_str("weapon_knife_outdoor");
			case 5: return crypt_str("weapon_knife_cord");
			case 6: return crypt_str("weapon_knife_canis");
			case 7: return crypt_str("weapon_knife_flip");
			case 8: return crypt_str("weapon_knife_gut");
			case 9: return crypt_str("weapon_knife_karambit");
			case 10: return crypt_str("weapon_knife_m9_bayonet");
			case 11: return crypt_str("weapon_knife_tactical");
			case 12: return crypt_str("weapon_knife_falchion");
			case 13: return crypt_str("weapon_knife_survival_bowie");
			case 14: return crypt_str("weapon_knife_butterfly");
			case 15: return crypt_str("weapon_knife_push");
			case 16: return crypt_str("weapon_knife_ursus");
			case 17: return crypt_str("weapon_knife_gypsy_jackknife");
			case 18: return crypt_str("weapon_knife_stiletto");
			case 19: return crypt_str("weapon_knife_widowmaker");
			}
		}
		else
		{
			switch (custom_index)
			{
			case 0: return crypt_str("ct_gloves");
			case 1: return crypt_str("studded_bloodhound_gloves");
			case 2: return crypt_str("t_gloves");
			case 3: return crypt_str("ct_gloves");
			case 4: return crypt_str("sporty_gloves");
			case 5: return crypt_str("slick_gloves");
			case 6: return crypt_str("leather_handwraps");
			case 7: return crypt_str("motorcycle_gloves");
			case 8: return crypt_str("specialist_gloves");
			case 9: return crypt_str("studded_hydra_gloves");
			}
		}
	}
	else
	{
		switch (id)
		{
		case 0: return crypt_str("knife");
		case 1: return crypt_str("gloves");
		case 2: return crypt_str("weapon_ak47");
		case 3: return crypt_str("weapon_aug");
		case 4: return crypt_str("weapon_awp");
		case 5: return crypt_str("weapon_cz75a");
		case 6: return crypt_str("weapon_deagle");
		case 7: return crypt_str("weapon_elite");
		case 8: return crypt_str("weapon_famas");
		case 9: return crypt_str("weapon_fiveseven");
		case 10: return crypt_str("weapon_g3sg1");
		case 11: return crypt_str("weapon_galilar");
		case 12: return crypt_str("weapon_glock");
		case 13: return crypt_str("weapon_m249");
		case 14: return crypt_str("weapon_m4a1_silencer");
		case 15: return crypt_str("weapon_m4a1");
		case 16: return crypt_str("weapon_mac10");
		case 17: return crypt_str("weapon_mag7");
		case 18: return crypt_str("weapon_mp5sd");
		case 19: return crypt_str("weapon_mp7");
		case 20: return crypt_str("weapon_mp9");
		case 21: return crypt_str("weapon_negev");
		case 22: return crypt_str("weapon_nova");
		case 23: return crypt_str("weapon_hkp2000");
		case 24: return crypt_str("weapon_p250");
		case 25: return crypt_str("weapon_p90");
		case 26: return crypt_str("weapon_bizon");
		case 27: return crypt_str("weapon_revolver");
		case 28: return crypt_str("weapon_sawedoff");
		case 29: return crypt_str("weapon_scar20");
		case 30: return crypt_str("weapon_ssg08");
		case 31: return crypt_str("weapon_sg556");
		case 32: return crypt_str("weapon_tec9");
		case 33: return crypt_str("weapon_ump45");
		case 34: return crypt_str("weapon_usp_silencer");
		case 35: return crypt_str("weapon_xm1014");
		default: return crypt_str("unknown");
		}
	}
}

IDirect3DTexture9* get_skin_preview(const char* weapon_name, const std::string& skin_name, IDirect3DDevice9* device)
{
	IDirect3DTexture9* skin_image = nullptr;
	std::string vpk_path;

	if (strcmp(weapon_name, crypt_str("unknown")) && strcmp(weapon_name, crypt_str("knife")) && strcmp(weapon_name, crypt_str("gloves")))
	{
		if (skin_name.empty() || skin_name == crypt_str("default"))
			vpk_path = crypt_str("resource/flash/econ/weapons/base_weapons/") + std::string(weapon_name) + crypt_str(".png");
		else
			vpk_path = crypt_str("resource/flash/econ/default_generated/") + std::string(weapon_name) + crypt_str("_") + std::string(skin_name) + crypt_str("_light_large.png");
	}
	else
	{
		if (!strcmp(weapon_name, crypt_str("knife")))
			vpk_path = crypt_str("resource/flash/econ/weapons/base_weapons/weapon_knife.png");
		else if (!strcmp(weapon_name, crypt_str("gloves")))
			vpk_path = crypt_str("resource/flash/econ/weapons/base_weapons/ct_gloves.png");
		else if (!strcmp(weapon_name, crypt_str("unknown")))
			vpk_path = crypt_str("resource/flash/econ/weapons/base_weapons/weapon_snowball.png");

	}
	const auto handle = m_basefilesys()->Open(vpk_path.c_str(), crypt_str("r"), crypt_str("GAME"));
	if (handle)
	{
		int file_len = m_basefilesys()->Size(handle);
		char* image = new char[file_len];

		m_basefilesys()->Read(image, file_len, handle);
		m_basefilesys()->Close(handle);

		D3DXCreateTextureFromFileInMemory(device, image, file_len, &skin_image);
		delete[] image;
	}

	if (!skin_image)
	{
		std::string vpk_path;

		if (strstr(weapon_name, crypt_str("bloodhound")) != NULL || strstr(weapon_name, crypt_str("hydra")) != NULL)
			vpk_path = crypt_str("resource/flash/econ/weapons/base_weapons/ct_gloves.png");
		else
			vpk_path = crypt_str("resource/flash/econ/weapons/base_weapons/") + std::string(weapon_name) + crypt_str(".png");

		const auto handle = m_basefilesys()->Open(vpk_path.c_str(), crypt_str("r"), crypt_str("GAME"));

		if (handle)
		{
			int file_len = m_basefilesys()->Size(handle);
			char* image = new char[file_len];

			m_basefilesys()->Read(image, file_len, handle);
			m_basefilesys()->Close(handle);

			D3DXCreateTextureFromFileInMemory(device, image, file_len, &skin_image);
			delete[] image;
		}
	}

	return skin_image;
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

	for (auto i = 0; i < cfg.skins.skinChanger.size(); i++)
		if (!all_skins[i])
			all_skins[i] = get_skin_preview(get_wep(i, (i == 0 || i == 1) ? cfg.skins.skinChanger.at(i).definition_override_vector_index : -1, i == 0).c_str(), cfg.skins.skinChanger.at(i).skin_name, device); //-V810

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

std::string get_config_dir()
{
	std::string folder;

	folder = crypt_str("C:\\Divine\\Configs\\");

	CreateDirectory(folder.c_str(), NULL);

	return folder;
}

void load_config(std::string selected_config)
{
	if (cfg_manager->files.empty())
		return;

	cfg_manager->load(selected_config, false);
	c_lua::get().unload_all_scripts();

	for (auto& script : cfg.scripts.scripts)
		c_lua::get().load_script(c_lua::get().get_script_id(script));

	scripts = c_lua::get().scripts;

	if (selected_script >= scripts.size())
		selected_script = scripts.size() - 1; //-V103

	for (auto& current : scripts)
	{
		if (current.size() >= 5 && current.at(current.size() - 1) == 'c')
			current.erase(current.size() - 5, 5);
		else if (current.size() >= 4)
			current.erase(current.size() - 4, 4);
	}

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

	for (auto i = 0; i < c_lua::get().scripts.size(); ++i)
	{
		auto script = c_lua::get().scripts.at(i);

		if (c_lua::get().loaded.at(i))
			cfg.scripts.scripts.emplace_back(script);
	}

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

void lua_loaded()
{
	c_lua::get().load_script(selected_script);
	c_lua::get().refresh_scripts();

	scripts = c_lua::get().scripts;

	if (selected_script >= scripts.size())
		selected_script = scripts.size() - 1;

	for (auto& current : scripts)
	{
		if (current.size() >= 5 && current.at(current.size() - 1) == 'c')
			current.erase(current.size() - 5, 5);
		else if (current.size() >= 4)
			current.erase(current.size() - 4, 4);
	}

	eventlogs::get().add(crypt_str("Loaded ") + scripts.at(selected_script) + crypt_str(" script"), false);
}

void Refresh_scripts()
{
	c_lua::get().refresh_scripts();
	scripts = c_lua::get().scripts;

	if (selected_script >= scripts.size())
		selected_script = scripts.size() - 1;

	for (auto& current : scripts)
	{
		if (current.size() >= 5 && current.at(current.size() - 1) == 'c')
			current.erase(current.size() - 5, 5);
		else if (current.size() >= 4)
			current.erase(current.size() - 4, 4);
	}
	eventlogs::get().add(crypt_str("Refresh scripts"), false);
}

void Reload_scripts()
{
	c_lua::get().reload_all_scripts();
	c_lua::get().refresh_scripts();

	scripts = c_lua::get().scripts;

	if (selected_script >= scripts.size())
		selected_script = scripts.size() - 1;

	for (auto& current : scripts)
	{
		if (current.size() >= 5 && current.at(current.size() - 1) == 'c')
			current.erase(current.size() - 5, 5);
		else if (current.size() >= 4)
			current.erase(current.size() - 4, 4);
	}
	eventlogs::get().add(crypt_str("Reload") + scripts.at(selected_script) + crypt_str(" script"), false);
}

void Unload_script()
{
	c_lua::get().unload_script(selected_script);
	c_lua::get().refresh_scripts();

	eventlogs::get().add(crypt_str("Unloaded ") + scripts.at(selected_script) + crypt_str(" script"), false);
}

void Open_lua()
{
	std::string folder = crypt_str("C:\\Divine\\Scripts");
	ShellExecute(NULL, crypt_str("open"), folder.c_str(), NULL, NULL, SW_SHOWNORMAL);
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
	*v = pressed;

	if (pressed || hovered)
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(33 / 255.f, 33 / 255.f, 33 / 255.f, g_ctx.gui.pop_anim * 0.85f));
	if (label_size.x > 0.0f)

		ImGui::PushFont(c_menu::get().g_pMenuFont);
	ImGui::RenderText(ImVec2(check_bb.GetTL().x + 12, check_bb.GetTL().y), Buf);
	ImGui::PopFont();

	if (pressed || hovered)
		ImGui::PopStyleColor();

	return pressed;

}

bool draw_lua_button(const char* label, const char* label_id, bool load, bool save, int curr_config, bool create = false)
{
	bool pressed = false;
	ImGui::SetCursorPosX(8);
	if (ImGui::PlusButton(label, 0, ImVec2(240, 26), label_id, ImColor(25, 25, 25, 225), (25, 25, 25, 225)))
		selected_script = curr_config;

	static std::string edit2;
	static char config_name[36] = "\0";
	ImGui::PushStyleVar(ImGuiStyleVar_::ImGuiStyleVar_PopupBorderSize, 0);
	ImGui::PushStyleVar(ImGuiStyleVar_::ImGuiStyleVar_PopupRounding, 4);
	ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(33 / 255.f, 33 / 255.f, 33 / 255.f, g_ctx.gui.pop_anim * 0.85f));
	if (ImGui::BeginPopup(label_id, ImGuiWindowFlags_NoMove))
	{
		ImGui::SetNextItemWidth(min(g_ctx.gui.pop_anim, 0.01f) * ImGui::GetFrameHeight() * 1.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, g_ctx.gui.pop_anim);
		auto clicked = false;

		bool one, ones, oness, two, open = false;
		if (!create)
		{
			if (LabelClick2(crypt_str("Load"), &one, label_id))
				lua_loaded();

			if (LabelClick2(crypt_str("Unload"), &two, label_id))
				Unload_script();

			if (LabelClick2(crypt_str("Reload"), &ones, label_id))
				Reload_scripts();
		}
		else
		{
			ImGui::SetCursorPosX(8);
			if (ImGui::CustomButton(crypt_str("Refresh"), crypt_str("##REFRESH_SCRIPTS"), ImVec2(193, 26 * 1)))
				Refresh_scripts();
			ImGui::SetCursorPosX(8);
			if (ImGui::CustomButton(crypt_str("Open lua folder"), crypt_str("##OPEN_LUA"), ImVec2(193, 26 * 1)))
				Open_lua();
		}
		ImGui::PopStyleVar();
		ImGui::EndPopup();
	}
	ImGui::PopStyleVar(2);
	ImGui::PopStyleColor(1);

	return pressed;
}

__forceinline void padding(float x, float y)
{
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + x * c_menu::get().dpi_scale);
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + y * c_menu::get().dpi_scale);
}

void child_title(const char* label)
{
	ImGui::PushFont(c_menu::get().futura_large);
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 1.f));
	ImGui::PopFont();
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (12 * c_menu::get().dpi_scale));

	ImGui::PushFont(c_menu::get().g_pMenuFont);
	ImGui::Text(label);
	ImGui::PopFont();

	ImGui::PopStyleColor();
}

void draw_combo(const char* name, int& variable, const char* labels[], int count) { ImGui::Combo(std::string(name).c_str(), &variable, labels, count); }
void draw_combo(const char* name, int& variable, bool (*items_getter)(void*, int, const char**), void* data, int count) { ImGui::Combo(std::string(name).c_str(), &variable, items_getter, data, count); }

void draw_multicombo(std::string name, std::vector<int>& variable, const char* labels[], int count, std::string& preview)
{
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

	if (ImGui::BeginCombo(name.c_str(), preview.c_str())) //draw
	{
		ImGui::BeginGroup();
		{

			for (auto i = 0; i < count; i++)
				ImGui::Selectable(labels[i], (bool*)&variable[i], ImGuiSelectableFlags_DontClosePopups);

		}
		ImGui::EndGroup();

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

	ImGui::PushFont(c_menu::get().g_pMenuFont);
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
}

void c_menu::rage_tab()
{
	if (ragebotsubtab == 0)
	{
		ImGui::SetCursorPos({ 100, 100 });
		ImGui::MenuChild("Main", ImVec2(275, 85));
		{
			ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
			ImGui::Checkbox(crypt_str("Master Switch"), &cfg.ragebot.enable);
			ImGui::SliderInt(crypt_str("Field of view"), &cfg.ragebot.field_of_view, 1, 180, false, crypt_str("%d°"));
		}
		ImGui::EndChild();

		ImGui::SetCursorPos({ 100, 220 });
		ImGui::MenuChild("Other", ImVec2(275, 100));
		{
			ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
			ImGui::Checkbox(crypt_str("Automatic scope"), &cfg.ragebot.autoscope);
			ImGui::Checkbox(crypt_str("Knife Bot"), &cfg.ragebot.knife_bot);
			ImGui::Checkbox(crypt_str("Zeus Bot"), &cfg.ragebot.zeus_bot);
		}
		ImGui::EndChild();

		////////////////////////////////////////////////////////////////////////////////////////////////////

		ImGui::SetCursorPos({ 390, 100 });
		ImGui::MenuChild("Exploits", ImVec2(275, 85));
		{
			ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();

			ImGui::Checkbox(crypt_str("Double Tap"), &cfg.ragebot.double_tap);
			ImGui::SameLine();
			draw_keybind(crypt_str(""), &cfg.ragebot.double_tap_key, crypt_str("##HOTKEY_DOUBLETAP"));

			ImGui::Spacing();

			ImGui::Checkbox(crypt_str("Hide Shots"), &cfg.antiaim.hide_shots);
			ImGui::SameLine();
			draw_keybind(crypt_str(""), &cfg.antiaim.hide_shots_key, crypt_str("##HOTKEY_HIDESHOTS"));
		}
		ImGui::EndChild();

		ImGui::SetCursorPos({ 390, 220 });
		ImGui::MenuChild("Helps", ImVec2(275, 100));
		{
			ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();

			ImGui::Checkbox(crypt_str("Lag exploit"), &cfg.ragebot.lag_exploit); // working

			ImGui::Checkbox(crypt_str("Anti exploit"), &cfg.ragebot.anti_exploit); // working

			ImGui::Checkbox(crypt_str("Extended backtack"), &cfg.misc.extended_backtack);
		}
		ImGui::EndChild();
	}
	else if (ragebotsubtab == 1)
	{
		const char* rage_weapon[8] = { crypt_str("Heavy Pistols"), crypt_str("Pistols"), crypt_str("SMG"), crypt_str("Rifles"), crypt_str("Auto Sniper"), crypt_str("Scout"), crypt_str("AWP"), crypt_str("Heavy") };
		ImGui::SetCursorPos({ 100, 100 });
		ImGui::MenuChild("Settings", ImVec2(275, 410));
		{
			ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();

			ImGui::Combo(crypt_str("Weapon"), &hooks::rage_weapon, rage_weapon, ARRAYSIZE(rage_weapon));
			ImGui::Spacing();
			ImGui::Combo(crypt_str("Target Selection"), &cfg.ragebot.weapon[hooks::rage_weapon].selection_type, selection, ARRAYSIZE(selection));

			draw_multicombo(crypt_str("Hitboxes"), cfg.ragebot.weapon[hooks::rage_weapon].hitboxes, hitboxes, ARRAYSIZE(hitboxes), preview);

			ImGui::SliderInt(crypt_str("Minimum damage"), &cfg.ragebot.weapon[hooks::rage_weapon].minimum_visible_damage, 1, 120, true);
			ImGui::SliderInt(crypt_str("Minimum wall damage"), &cfg.ragebot.weapon[hooks::rage_weapon].minimum_damage, 1, 120, true);

			ImGui::SetCursorPosX(9);
			ImGui::PushFont(c_menu::get().MenuFontRender);
			ImGui::Text("Damage Override");
			ImGui::PopFont();
			ImGui::SameLine();
			draw_keybind(crypt_str("Damage Override"), &cfg.ragebot.weapon[hooks::rage_weapon].damage_override_key, crypt_str("##HOTKEY__DAMAGE_OVERRIDE"));

			if (cfg.ragebot.weapon[hooks::rage_weapon].damage_override_key.key > KEY_NONE && cfg.ragebot.weapon[hooks::rage_weapon].damage_override_key.key < KEY_MAX)
				ImGui::SliderInt(crypt_str("Minimum override damage"), &cfg.ragebot.weapon[hooks::rage_weapon].minimum_override_damage, 1, 120, true);
		}
		ImGui::EndChild();

		ImGui::SetCursorPos({ 390, 100 });
		ImGui::MenuChild("Extra", ImVec2(275, 410));
		{
			ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();

			ImGui::Checkbox(crypt_str("Auto Stop"), &cfg.ragebot.weapon[hooks::rage_weapon].autostop);
			if (cfg.ragebot.weapon[hooks::rage_weapon].autostop)
				draw_multicombo(crypt_str("Auto Stop Modifiers"), cfg.ragebot.weapon[hooks::rage_weapon].autostop_modifiers, autostop_modifiers, ARRAYSIZE(autostop_modifiers), preview);

			ImGui::Checkbox(crypt_str("Hitchance"), &cfg.ragebot.weapon[hooks::rage_weapon].hitchance);
			if (cfg.ragebot.weapon[hooks::rage_weapon].hitchance)
				ImGui::SliderInt(crypt_str("Hit Chance"), &cfg.ragebot.weapon[hooks::rage_weapon].hitchance_amount, 0, 100, cfg.ragebot.weapon[hooks::rage_weapon].hitchance_amount ? crypt_str("%d") : crypt_str("None"));

			ImGui::Checkbox(crypt_str("Prefer safe points"), &cfg.ragebot.weapon[hooks::rage_weapon].prefer_safe_points);
			ImGui::SameLine();
			draw_keybind(crypt_str("Force safe points"), &cfg.ragebot.safe_point_key, crypt_str("##HOKEY_FORCE_SAFE_POINTS"));

			ImGui::Checkbox(crypt_str("Prefer body aim"), &cfg.ragebot.weapon[hooks::rage_weapon].prefer_body_aim);
			ImGui::SameLine();
			draw_keybind(crypt_str("Force body aim"), &cfg.ragebot.body_aim_key, crypt_str("##HOKEY_FORCE_BODY_AIM"));

			ImGui::Checkbox(crypt_str("Static point scale"), &cfg.ragebot.weapon[hooks::rage_weapon].static_point_scale);
			if (cfg.ragebot.weapon[hooks::rage_weapon].static_point_scale)
			{
				ImGui::SliderFloat(crypt_str("Head scale"), &cfg.ragebot.weapon[hooks::rage_weapon].head_scale, 0.0f, 1.0f, cfg.ragebot.weapon[hooks::rage_weapon].head_scale ? crypt_str("%.2f") : crypt_str("None"));
				ImGui::SliderFloat(crypt_str("Body scale"), &cfg.ragebot.weapon[hooks::rage_weapon].body_scale, 0.0f, 1.0f, cfg.ragebot.weapon[hooks::rage_weapon].body_scale ? crypt_str("%.2f") : crypt_str("None"));
			}
		}
		ImGui::EndChild();
	}
	else if (ragebotsubtab == 2)
	{
		static auto type = 0;
		ImGui::SetCursorPos({ 100, 100 });
		ImGui::MenuChild("Main", ImVec2(275, 410));
		{
			ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
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
			ImGui::PushFont(c_menu::get().MenuFontRender);
			ImGui::Text("Manual back");
			ImGui::PopFont();
			ImGui::SameLine();
			draw_keybind(crypt_str("Manual back"), &cfg.antiaim.manual_back, crypt_str("##HOTKEY_INVERT_BACK"));

			ImGui::SetCursorPosX(9);
			ImGui::PushFont(c_menu::get().MenuFontRender);
			ImGui::Text("Manual left");
			ImGui::PopFont();
			ImGui::SameLine();
			draw_keybind(crypt_str("Manual left"), &cfg.antiaim.manual_left, crypt_str("##HOTKEY_INVERT_LEFT"));


			ImGui::SetCursorPosX(9);
			ImGui::PushFont(c_menu::get().MenuFontRender);
			ImGui::Text("Manual right");
			ImGui::PopFont();
			ImGui::SameLine();
			draw_keybind(crypt_str("Manual right"), &cfg.antiaim.manual_right, crypt_str("##HOTKEY_INVERT_RIGHT"));

			ImGui::Checkbox(crypt_str("Zero Land"), &cfg.antiaim.pitch_zero_on_land);

			if (cfg.antiaim.manual_back.key > KEY_NONE && cfg.antiaim.manual_back.key < KEY_MAX || cfg.antiaim.manual_left.key > KEY_NONE && cfg.antiaim.manual_left.key < KEY_MAX || cfg.antiaim.manual_right.key > KEY_NONE && cfg.antiaim.manual_right.key < KEY_MAX)
			{
				ImGui::Checkbox(crypt_str("Manuals indicator"), &cfg.antiaim.flip_indicator);
				ImGui::SameLine();
				ImGui::ColorEdit(crypt_str("##invc"), &cfg.antiaim.flip_indicator_color, ALPHA);
			}

			ImGui::Checkbox(crypt_str("Backward Legs"), &cfg.antiaim.backward_legs);

			//ImGui::Checkbox(crypt_str("Static Manuals"), &cfg.antiaim.xd);

		}
		ImGui::EndChild();

		ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
		ImGui::SetCursorPos({ 390, 100 });
		ImGui::MenuChild("Other", ImVec2(275, 410));
		{
			ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
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
}

void c_menu::anti_tab() // player tab
{
	static int local_tab = 0;
	static int enemyorteam_tab = 0;

	ImGui::SetCursorPos({ 100, 100 });
	ImGui::MenuChild("Primary", ImVec2(275, 410));
	{
		ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
		ImGui::Checkbox("Master Switch", &cfg.player.enable);

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

	ImGui::SetCursorPos({ 390, 100 });
	ImGui::MenuChild("Chams", ImVec2(275, 410));
	{
		ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
		if (player == LOCAL)
			draw_combo(crypt_str("Type"), cfg.player.local_chams_type, local_chams_type, ARRAYSIZE(local_chams_type));

		ImGui::Checkbox(crypt_str("Attachments"), &cfg.esp.attachment_chams);

		if (cfg.esp.attachment_chams)
		{
			draw_combo(crypt_str("Attachment chams type"), cfg.esp.attachment_chams_material, chamstype, ARRAYSIZE(chamstype));

			ImGui::SetCursorPosX(9);
			ImGui::Text(crypt_str("Attachment Color "));
			ImGui::SameLine();
			ImGui::ColorEdit(crypt_str("##logcolor"), &cfg.esp.attachment_chams_color, ALPHA);
		}

		//draw_combo(crypt_str("Player models"), cfg.player.player_models, player_models, ARRAYSIZE(player_models));

		if (player != LOCAL || !cfg.player.local_chams_type)
			draw_multicombo(crypt_str("Chams"), cfg.player.type[player].chams, cfg.player.type[player].chams[PLAYER_CHAMS_VISIBLE] ? chamsvisact : chamsvis, cfg.player.type[player].chams[PLAYER_CHAMS_VISIBLE] ? ARRAYSIZE(chamsvisact) : ARRAYSIZE(chamsvis), preview);

		if (cfg.player.type[player].chams[PLAYER_CHAMS_VISIBLE] || player == LOCAL && cfg.player.local_chams_type)
		{

			if (player == LOCAL && cfg.player.local_chams_type)
			{
				ImGui::Checkbox(crypt_str("Enable desync chams"), &cfg.player.fake_chams_enable);
				ImGui::Checkbox(crypt_str("Visualize lag"), &cfg.player.visualize_lag);
				ImGui::Checkbox(crypt_str("Layered"), &cfg.player.layered);


				draw_combo(crypt_str("Player chams material"), cfg.player.fake_chams_type, chamstype, ARRAYSIZE(chamstype));

				//draw_combo(crypt_str("Player models"), cfg.player.player_models, player_models, ARRAYSIZE(player_models));

				ImGui::SetCursorPosX(9);
				ImGui::ColorEdit(crypt_str("##fakechamscolor"), &cfg.player.fake_chams_color, ALPHA);
				ImGui::SameLine();
				ImGui::Text(crypt_str("Color "));

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

				if (player == ENEMY)
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

		if (player == ENEMY || player == TEAM)
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

void c_menu::player_tab() // world tab
{
	if (hueta == 0)
	{
		ImGui::SetCursorPos({ 100, 100 });
		ImGui::MenuChild("World", ImVec2(275, 410));
		{
			ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();

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

		ImGui::SetCursorPos({ 390, 100 });
		ImGui::MenuChild("Render", ImVec2(275, 410));
		{
			ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();

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
			padding(0, 3);

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
				ImGui::SameLine();
				ImGui::ColorEdit(crypt_str("##clientbulletimpacts"), &cfg.esp.client_bullet_impacts_color, ALPHA);
			}

			ImGui::Checkbox(crypt_str("Server impacts"), &cfg.esp.server_bullet_impacts);
			if (cfg.esp.server_bullet_impacts)
			{
				ImGui::SameLine();
				ImGui::ColorEdit(crypt_str("##serverbulletimpacts"), &cfg.esp.server_bullet_impacts_color, ALPHA);
			}

			ImGui::Checkbox(crypt_str("Local tracers"), &cfg.esp.bullet_tracer);
			if (cfg.esp.bullet_tracer)
			{
				ImGui::SameLine();
				ImGui::ColorEdit(crypt_str("##bulltracecolor"), &cfg.esp.bullet_tracer_color, ALPHA);
			}

			ImGui::Checkbox(crypt_str("Enemy tracers"), &cfg.esp.enemy_bullet_tracer);
			if (cfg.esp.enemy_bullet_tracer)
			{
				ImGui::SameLine();
				ImGui::ColorEdit(crypt_str("##enemybulltracecolor"), &cfg.esp.enemy_bullet_tracer_color, ALPHA);
			}
			ImGui::Checkbox("Damage marker", &cfg.esp.damage_marker);
			//draw_multicombo(crypt_str("Hit marker"), cfg.esp.hitmarker, hitmarkers, ARRAYSIZE(hitmarkers), preview);
			ImGui::Checkbox(crypt_str("Kill effect"), &cfg.esp.kill_effect);

			if (cfg.esp.kill_effect)
				ImGui::SliderFloat(crypt_str("Duration"), &cfg.esp.kill_effect_duration, 0.01f, 3.0f);

			ImGui::Checkbox(crypt_str("Penetration crosshair"), &cfg.esp.penetration_reticle);

			ImGui::Checkbox(crypt_str("Grenade prediction"), &cfg.esp.grenade_prediction);

			if (cfg.esp.grenade_prediction)
			{
				ImGui::SameLine();
				ImGui::ColorEdit(crypt_str("##tracergrenpredcolor"), &cfg.esp.grenade_prediction_tracer_color, ALPHA);
			}

			ImGui::Checkbox(crypt_str("Grenade proximity warning"), &cfg.esp.grenade_proximity_warning);


			ImGui::Checkbox(crypt_str("Fire timer"), &cfg.esp.molotov_timer);

			if (cfg.esp.molotov_timer)
			{
				ImGui::SameLine();
				ImGui::ColorEdit(crypt_str("##molotovcolor"), &cfg.esp.molotov_timer_color, ALPHA);
			}

			ImGui::Checkbox(crypt_str("Smoke timer"), &cfg.esp.smoke_timer);
			if (cfg.esp.smoke_timer)
			{
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
	else if (hueta == 1)
	{
		ImGui::SetCursorPos({ 100, 100 });
		static char custom_name[124] = "";
		static int seed = 0;
		bool stat_trak = false;
		static float wear = 0.0f;
		ImGui::MenuChild("General", ImVec2(275, 410)); // 560 430
		{
			ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
			static int next_id = -1;

			// we need to count our items in 1 line
			auto same_line_counter = 0;
			auto current_weapon = 0;

			if (c_menu::get().current_profile == -1)
			{
				for (auto i = 0; i <cfg.skins.skinChanger.size(); i++)
				{
					// do we need update our preview for some reasons?
					if (!all_skins[i])
					{
						cfg.skins.skinChanger.at(i).update();
					}

					next_id = i;
					c_menu::get().current_profile = next_id;

					//L::Print("Skin id logged: " + std::to_string(c_menu::get().current_profile));

					if (c_menu::get().current_profile == 35)
						c_menu::get().current_profile = 0;
				}
			}
			else
			{
				//ui::BeginGroup();
				//ui::PushItemWidth(260 * dpi_scale);
				auto& selected_entry = cfg.skins.skinChanger[c_menu::get().current_profile];
				selected_entry.itemIdIndex = c_menu::get().current_profile;

				// search input later
				static char search_skins[64] = "\0";
				static auto item_index = selected_entry.paint_kit_vector_index;


				// Enabled
				//ImGui::Checkbox("Enabled", &selected_entry.enabled);

				ImGui::Combo(crypt_str("Weapon"), &current_profile, skinchanger_weapons, ARRAYSIZE(skinchanger_weapons));

				if (!current_profile)
				{


					//ImGui::Text(crypt_str("Knife"));
					//ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 5 * c_menu::get().dpi_scale);
					if (ImGui::Combo(crypt_str("Knife"), &selected_entry.definition_override_vector_index, [](void* data, int idx, const char** out_text)
						{
							*out_text = game_data::knife_names[idx].name;
					return true;
						}, nullptr, IM_ARRAYSIZE(game_data::knife_names)))
					{
						SkinChanger::scheduleHudUpdate();
					}
				}
				else if (current_profile == 1)
				{
					//ImGui::Text(crypt_str("Gloves"));
					//ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 5 * c_menu::get().dpi_scale);

					if (ImGui::Combo(crypt_str("Gloves"), &selected_entry.definition_override_vector_index, [](void* data, int idx, const char** out_text)
						{
							*out_text = game_data::glove_names[idx].name;
					return true;
						}, nullptr, IM_ARRAYSIZE(game_data::glove_names)))
					{
						item_index = 0; // set new generated paintkits element to 0;
						SkinChanger::scheduleHudUpdate();
					}
				}

				if (c_menu::get().current_profile != 1)
				{
					ImGui::SetCursorPosX(10); ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2); ImGui::Text(crypt_str("Search"));

					ImGui::InputText(crypt_str("##search"), search_skins, sizeof(search_skins));
				}

				auto main_kits = c_menu::get().current_profile == 1 ? SkinChanger::gloveKits : SkinChanger::skinKits;

				//Broken, kinda, does not pop up when you search the weapons name. also some items are named knife_... for some reason.
				//auto main_kits = c_menu::get().current_profile == 1 ? (cfg::cfg.skins.show_names ? SkinChanger::gloveKits2 : SkinChanger::gloveKits) : (cfg::cfg.skins.show_names ? SkinChanger::skinKits2 : SkinChanger::skinKits);

				auto display_index = 0;

				SkinChanger::displayKits = main_kits;

				// we dont need custom gloves
				if (c_menu::get().current_profile == 1)
				{
					for (auto i = 0; i < main_kits.size(); i++)
					{
						auto main_name = main_kits.at(i).name;

						for (auto i = 0; i < main_name.size(); i++)
							if (iswalpha((main_name.at(i))))
								main_name.at(i) = towlower(main_name.at(i));

						char search_name[64];

						if (!strcmp(game_data::glove_names[selected_entry.definition_override_vector_index].name, crypt_str("Hydra")))
							strcpy_s(search_name, sizeof(search_name), crypt_str("Bloodhound"));
						else
							strcpy_s(search_name, sizeof(search_name), game_data::glove_names[selected_entry.definition_override_vector_index].name);

						for (auto i = 0; i < sizeof(search_name); i++)
							if (iswalpha(search_name[i]))
								search_name[i] = towlower(search_name[i]);

						if (main_name.find(search_name) != std::string::npos)
						{
							SkinChanger::displayKits.at(display_index) = main_kits.at(i);
							display_index++;
						}
					}

					SkinChanger::displayKits.erase(SkinChanger::displayKits.begin() + display_index, SkinChanger::displayKits.end());
				}
				else
				{
					if (strcmp(search_skins, crypt_str(""))) //-V526
					{
						for (auto i = 0; i < main_kits.size(); i++)
						{

							//Setup skin names
							auto main_name = main_kits.at(i).name;

							for (auto i = 0; i < main_name.size(); i++)
								main_name.at(i) = towlower(main_name.at(i));

							//Setup search name
							char search_name[64];
							strcpy_s(search_name, sizeof(search_name), search_skins);

							for (auto i = 0; i < sizeof(search_name); i++)
								search_name[i] = towlower(search_name[i]);


							//Compare.
							if (!main_name.find(search_name))
							{
								SkinChanger::displayKits.at(display_index) = main_kits.at(i);
								display_index++;
							}
						}

						//Finish
						SkinChanger::displayKits.erase(SkinChanger::displayKits.begin() + display_index, SkinChanger::displayKits.end());
					}
					else
						item_index = selected_entry.paint_kit_vector_index;
				}
			skip:

				ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
				if (!SkinChanger::displayKits.empty())
				{
					static float shitpad = 0;

					if (current_profile == 0)
						shitpad = 32;
					else if (current_profile == 1)
						shitpad = -32;
					else
						shitpad = -18;

					ImGui::SetCursorPos({ 4, 166 + shitpad });

					if (item_index < 0 || item_index > 20000)
					{
						L::Print(std::to_string(item_index));
						item_index = 0;
					}

					if (ImGui::ListBox(crypt_str("Skin List"), &item_index, [](void* data, int idx, const char** out_text) {
						while (SkinChanger::displayKits.at(idx).name.find(crypt_str("С‘")) != std::string::npos)
						SkinChanger::displayKits.at(idx).name.replace(SkinChanger::displayKits.at(idx).name.find(crypt_str("С‘")), 2, crypt_str("Рµ"));
					*out_text = SkinChanger::displayKits.at(idx).name.c_str();
					return true; }, nullptr, SkinChanger::displayKits.size(), SkinChanger::displayKits.size() > 9 ? 9 : SkinChanger::displayKits.size()))
					{

						SkinChanger::scheduleHudUpdate();

						auto i = 0;

						while (i < main_kits.size())
						{
							if (main_kits.at(i).id == SkinChanger::displayKits.at(item_index).id)
							{
								selected_entry.paint_kit_vector_index = i;
								break;
							}

							i++;
						}

					}


				}
				ImGui::PopStyleVar();

				//Broken, kinda, does not pop up when you search the weapons name. also some items are named knife_... for some reason.
				//ImGui::Checkbox(crypt_str("Show weapon names"), &cfg::cfg.skins.show_names);

				selected_entry.update();
			}
		}
		ImGui::EndChild();

		ImGui::SetCursorPos({ 390, 100 });
		ImGui::MenuChild("Extra", ImVec2(275, 410));
		{
			ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
			auto& selected_entry = cfg.skins.skinChanger[c_menu::get().current_profile];
			selected_entry.itemIdIndex = c_menu::get().current_profile;

			draw_combo(crypt_str("Player models"), cfg.player.player_models, player_models, ARRAYSIZE(player_models));

			ImGui::Spacing();

			if ((ImGui::CustomButton(crypt_str("Force Update"), crypt_str("##updater"), ImVec2(265, 26), false, c_menu::get().settingicons, "0")))
				SkinChanger::scheduleHudUpdate();
		}
		ImGui::EndChild();
	}
}

void c_menu::general_tab() // misc?
{
	if (gen == 0)
	{
		ImGui::SetCursorPos({ 100, 100 });
		ImGui::MenuChild("Movement", ImVec2(275, 410));
		{
			ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
			ImGui::Checkbox("Bunnyhop", &cfg.misc.bunnyhop);
			draw_combo(crypt_str("Airstrafe"), cfg.misc.airstrafe, strafes, ARRAYSIZE(strafes));
			ImGui::Checkbox(crypt_str("Crouch in air"), &cfg.misc.crouch_in_air);
			ImGui::Checkbox(crypt_str("Fast stop"), &cfg.misc.fast_stop);
			ImGui::Checkbox(crypt_str("Slide walk"), &cfg.misc.slidewalk);
			ImGui::Checkbox(crypt_str("No duck cooldown"), &cfg.misc.noduck);



			//if (cfg.misc.noduck)
				//draw_keybind(crypt_str("Fake duck"), &cfg.misc.fakeduck_key, crypt_str("##FAKEDUCK__HOTKEY"));

			ImGui::SetCursorPosX(9);
			ImGui::Text("Slow walk");
			//ImGui::PopFont();
			ImGui::SameLine();
			draw_keybind(crypt_str("Slow walk"), &cfg.misc.slowwalk_key, crypt_str("##SLOWWALK__HOTKEY"));

			ImGui::SetCursorPosX(9);
			ImGui::Checkbox(crypt_str("Automatic Peek"), &cfg.misc.peekkk);
			if (cfg.misc.peekkk)
			{
				ImGui::SameLine();
				draw_keybind(crypt_str(""), &cfg.misc.automatic_peek, crypt_str("##AUTOPEEK__HOTKEY"));
			}

			ImGui::Checkbox(crypt_str("Weapon chams"), &cfg.esp.weapon_chams);
			ImGui::SameLine();
			ImGui::ColorEdit(crypt_str("##weaponchams"), &cfg.esp.weapon_chams_color, ALPHA);
			
			ImGui::Spacing();

			if (cfg.esp.weapon_chams)
			{
				draw_combo(crypt_str("Weapon chams type"), cfg.esp.weapon_chams_type, chamstype, ARRAYSIZE(chamstype));
			}

			//draw_keybind(crypt_str("Edge jump"), &cfg.misc.edge_jump, crypt_str("##EDGEJUMP__HOTKEY"));
		}
		ImGui::EndChild();

		ImGui::SetCursorPos({ 390, 100 });
		ImGui::MenuChild("Other", ImVec2(275, 410));
		{
			ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
			ImGui::Checkbox(crypt_str("Anti-untrusted"), &cfg.misc.anti_untrusted);
			ImGui::Checkbox(crypt_str("Rank reveal"), &cfg.misc.rank_reveal);
			ImGui::Checkbox(crypt_str("Unlock inventory access"), &cfg.misc.inventory_access);
			ImGui::Checkbox(crypt_str("Gravity ragdolls"), &cfg.misc.ragdolls);
			ImGui::Checkbox(crypt_str("Preserve killfeed"), &cfg.esp.preserve_killfeed);
			ImGui::Checkbox(crypt_str("Aspect ratio"), &cfg.misc.aspect_ratio);

			if (cfg.misc.aspect_ratio)
			{
				padding(0, -5);
				ImGui::SliderFloat(crypt_str("Amount"), &cfg.misc.aspect_ratio_amount, 1.0f, 2.0f);
			}
		}
		ImGui::EndChild();


	}
	else if (gen == 1)
	{

		ImGui::SetCursorPos({ 100, 100 });
		ImGui::MenuChild("Info", ImVec2(275, 410));
		{
			ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
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

		ImGui::SetCursorPos({ 390, 100 });
		ImGui::MenuChild("Extra", ImVec2(275, 410));
		{
			ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
			ImGui::Checkbox(crypt_str("Anti-screenshot"), &cfg.misc.anti_screenshot);
			ImGui::Checkbox(crypt_str("Clantag"), &cfg.misc.clantag_spammer);
			//ImGui::Checkbox(crypt_str("Chat spam"), &cfg.misc.chat);
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

void c_menu::configs_tab()
{
	static bool is_sure_check = false;
	static float started_think = 0;
	static std::string selected_name = "";
	static char config_name[30] = "\0";

	ImGui::SetCursorPos({ 100, 50 });
	ImGui::MenuChild("Config List", ImVec2(275, 465));
	{
		ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
		ImGui::InputText(crypt_str("Config Name"), config_name, sizeof(config_name));
		ImGui::Spacing();ImGui::Spacing();

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

	ImGui::SetCursorPos({ 390, 50 });
	ImGui::MenuChild("Config Settings", ImVec2(275, 465));
	{
		ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
		if ((ImGui::CustomButton(crypt_str("Create new..."), crypt_str("##CreateConfig"), ImVec2(265, 30), true, c_menu::get().settingicons, "3")))
			add_config(config_name);

		if ((ImGui::CustomButton(crypt_str("Open Config Directory"), crypt_str("##OpenConfigDirectory"), ImVec2(265, 26), true, c_menu::get().settingicons, "2")))
		{
			std::string folder;
			auto get_dir = [&folder]() -> void
			{
				folder = crypt_str("C:\\Divine\\Configs\\");
				CreateDirectory(folder.c_str(), NULL);
			};

			get_dir();
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

			if ((ImGui::CustomButton(crypt_str("Reset"), crypt_str("##Resetconfig"), ImVec2(265, 26), false, c_menu::get().settingicons, "")) && !selected_name.empty())
				cfg_manager->setup();
		}
	}
	ImGui::EndChild();
}

void c_menu::lua_tab()
{
	ImGui::SetCursorPos({ 100, 50 });

	static auto should_update = true;

	ImGui::MenuChild("Lua", ImVec2(275, 465));
	{
		ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();

		if (should_update)
		{
			should_update = false;
			scripts = c_lua::get().scripts;

			for (auto& current : scripts)
			{
				if (current.size() >= 5 && current.at(current.size() - 1) == 'c')
					current.erase(current.size() - 5, 5);
				else if (current.size() >= 4)
					current.erase(current.size() - 4, 4);
			}
		}

		ImGui::PushFont(c_menu::get().g_pMenuFont);
		draw_lua_button("Scripts", "#ddsdasdasdadsadasdawadsd", false, false, false, true);
		for (int i = 0; i < scripts.size(); i++)
		{
			selected_script = i;
			bool load, save = false;
			draw_lua_button(scripts.at(i).c_str(), std::string(scripts.at(i) + "idi_naxui").c_str(), load, save, i);
		}
		ImGui::PopFont();

	}
	ImGui::EndChild();

	ImGui::SetCursorPos({ 390, 50 });
	ImGui::MenuChild("Active", ImVec2(275, 465));
	{
		ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();

		auto previous_check_box = false;
		for (auto& current : c_lua::get().scripts)
		{
			auto& items = c_lua::get().items.at(c_lua::get().get_script_id(current));

			for (auto& item : items)
			{
				std::string item_name;

				auto first_point = false;
				auto item_str = false;

				for (auto& c : item.first)
				{
					if (c == '.')
					{
						if (first_point)
						{
							item_str = true;
							continue;
						}
						else
							first_point = true;
					}

					if (item_str)
						item_name.push_back(c);
				}

				switch (item.second.type)
				{
				case NEXT_LINE:
					previous_check_box = false;
					break;
				case CHECK_BOX:
					previous_check_box = true;
					ImGui::Checkbox(item_name.c_str(), &item.second.check_box_value);
					break;
				case COMBO_BOX:
					previous_check_box = false;
					draw_combo(item_name.c_str(), item.second.combo_box_value, [](void* data, int idx, const char** out_text)
						{
							auto labels = (std::vector <std::string>*)data;
					*out_text = labels->at(idx).c_str(); //-V106
					return true;
						}, &item.second.combo_box_labels, item.second.combo_box_labels.size());
					break;
				case SLIDER_INT:
					previous_check_box = false;
					ImGui::SliderInt(item_name.c_str(), &item.second.slider_int_value, item.second.slider_int_min, item.second.slider_int_max);
					break;
				case SLIDER_FLOAT:
					previous_check_box = false;
					ImGui::SliderFloat(item_name.c_str(), &item.second.slider_float_value, item.second.slider_float_min, item.second.slider_float_max);
					break;
				case COLOR_PICKER:
					if (previous_check_box)
						previous_check_box = false;
					else
						ImGui::Text((item_name + ' ').c_str());

					ImGui::SameLine();
					ImGui::ColorEdit((crypt_str("##") + item_name).c_str(), &item.second.color_picker_value, ALPHA, true);
					break;
				}
			}
		}
	}
	ImGui::EndChild();
}

void  c_menu::render_tab()
{
	/*subtabs*/  /*subtabs*/  /*subtabs*/  /*subtabs*/
	ImGui::PushFont(MenuFont);
	{
		switch (tab_static)
		{
		case 0:
		{

			ImGui::SetCursorPos({ 100, 30 });
			ImGui::BeginGroup();
			{
				if (ImGui::subtab("main", !ragebotsubtab))
					ragebotsubtab = 0;  ImGui::SameLine();
				if (ImGui::subtab("weapons", ragebotsubtab == 1))
					ragebotsubtab = 1; ImGui::SameLine();
				if (ImGui::subtab("anti-aim", ragebotsubtab == 2))
					ragebotsubtab = 2; ImGui::SameLine();
			}
			ImGui::EndGroup();

		}break;
		case 1:
		{

			ImGui::SetCursorPos({ 100, 30 });
			ImGui::BeginGroup();
			{
				if (ImGui::subtab("enemy", !player))
					player = 0; ImGui::SameLine();
				if (ImGui::subtab("team", player == 1))
					player = 1; ImGui::SameLine();
				if (ImGui::subtab("local", player == 2))
					player = 2;
			}
			ImGui::EndGroup();

		}break;
		case 2:
		{
			ImGui::SetCursorPos({ 100, 30 });

			ImGui::BeginGroup();
			{
				if (ImGui::subtab("Main", !hueta))
					hueta = 0; ImGui::SameLine();
				if (ImGui::subtab("Skins", hueta == 1))
					hueta = 1; ImGui::SameLine();
			}
			ImGui::EndGroup();

		}break;
		case 5:
		{

			ImGui::SetCursorPos({ 100, 30 });
			ImGui::BeginGroup();
			{
				if (ImGui::subtab("Main", !gen))
					gen = 0; ImGui::SameLine();
				if (ImGui::subtab("Other", gen == 1))
					gen = 1; ImGui::SameLine();
			}
			ImGui::EndGroup();

		}break;
		}
	}
	ImGui::PopFont();
	/*subtabs*/  /*subtabs*/  /*subtabs*/  /*subtabs*/

	ImGui::PushFont(New_icons); // new font
	{
		ImGui::SetCursorPos(ImVec2{ 25, 110 }); //rage
		if (ImGui::tab("R", !tab_static))
			tab_static = 0;

		ImGui::SetCursorPos(ImVec2{ 25, 170 }); //aa == players
		if (ImGui::tab("P", tab_static == 1))
			tab_static = 1;

		ImGui::SetCursorPos(ImVec2{ 25, 230 }); //world
		if (ImGui::tab("W", tab_static == 2))
			tab_static = 2;

		ImGui::SetCursorPos(ImVec2{ 25, 290 }); //cfg
		if (ImGui::tab("C", tab_static == 3))
			tab_static = 3;

		ImGui::SetCursorPos(ImVec2{ 25, 350 }); //lua
		if (ImGui::tab("S", tab_static == 4))
			tab_static = 4;

		ImGui::SetCursorPos(ImVec2{ 25, 440 }); //setting == misc
		if (ImGui::tab("M", tab_static == 5))
			tab_static = 5;
	}
	ImGui::PopFont();

	switch (tab_static)
	{
	case 0:     rage_tab();       break;
	case 1:     anti_tab();       break;
	case 2:	    player_tab(); break;
	case 3:	 	configs_tab(); break;
	case 4:	    lua_tab(); break;
	case 5:	    general_tab(); break;
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
	ss.FramePadding = ImVec2(2, 2);

	auto s = ImVec2{}, p = ImVec2{}, gs = ImVec2{ 910, 800 };
	ImGui::SetNextWindowSize(ImVec2(gs));
	if (ImGui::Begin(("##MENU"), nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar))
	{
		s = ImVec2(ImGui::GetWindowSize().x - ImGui::GetStyle().WindowPadding.x * 2, ImGui::GetWindowSize().y - ImGui::GetStyle().WindowPadding.y * 2);
		p = ImVec2(ImGui::GetWindowPos().x + ImGui::GetStyle().WindowPadding.x, ImGui::GetWindowPos().y + ImGui::GetStyle().WindowPadding.y);

		// Left
		ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x, p.y), ImVec2(p.x + 75, p.y + 560), ImColor(25, 25, 25), ImDrawFlags_RoundCornersTopLeft, 5);
		ImGui::GetWindowDrawList()->AddRect(ImVec2(p.x, p.y), ImVec2(p.x + 75, p.y + 560), ImColor(35, 35, 35), ImDrawFlags_RoundCornersTopLeft, 5, 2.2);

		// Right
		ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x + 74, p.y), ImVec2(p.x + 675, p.y + 560), ImColor(21, 21, 21), ImDrawFlags_RoundCornersBottomRight, 10);
		ImGui::GetWindowDrawList()->AddRect(ImVec2(p.x + 74, p.y), ImVec2(p.x + 675, p.y + 560), ImColor(35, 35, 35), ImDrawFlags_RoundCornersBottomRight, 10, 2.2);

		//logo
		ImGui::SetCursorPos(ImVec2(28, 28));
		ImGui::Image(nigga, ImVec2(35.f, 35.f));

		//logo line
		ImGui::GetWindowDrawList()->AddLine(ImVec2(p.x + 20, p.y + 75), ImVec2(p.x + 55, p.y + 75), ImColor(37, 37, 37), 2.2);

		//hooks
		{
			render_tab();
		}

		//setting line
		ImGui::GetWindowDrawList()->AddLine(ImVec2(p.x + 20, p.y + 407), ImVec2(p.x + 55, p.y + 407), ImColor(37, 37, 37), 2.2);

		//user logo 
		ImGui::SetCursorPos(ImVec2(23, 500));
		ImGui::Image(divine, ImVec2(45, 45));
	}
	ImGui::End();
}