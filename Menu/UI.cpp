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
	for (auto i = 0, j = 0; i < count; i++)
	{
		if (variable[i])
		{
			if (j)
			{
				preview += crypt_str(", ...");
				break;
			}
			else
				preview = labels[i];

			j++;
		}
	}
	ImGui::Spacing();

	if (ImGui::BeginCombo(name.c_str(), preview.c_str()))
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
	for (auto& c : text)
		c = toupper(c);

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

	float btn_w = ImGui::CalcTextSize(text.c_str()).x + 8.0f * c_menu::get().dpi_scale;
	if (btn_w < 40.0f * c_menu::get().dpi_scale)
		btn_w = 40.0f * c_menu::get().dpi_scale;

	// when inline (with_bool == true), place button at current cursor (no force-right)
	// otherwise, align to the right as before
	if (!with_bool)
		ImGui::SetCursorPosX(ImGui::GetWindowSize().x - btn_w - 27 * c_menu::get().dpi_scale);
	else
	{
		// Inline: right-align within current content width (no clipping)
		const float cur_x = ImGui::GetCursorPosX();
		const float avail_x = ImGui::GetContentRegionAvail().x;
		const float margin_r = 8.0f * c_menu::get().dpi_scale; // right padding
		const float gap_l = 8.0f * c_menu::get().dpi_scale;    // gap from checkbox
		float x = cur_x + avail_x - btn_w - margin_r;
		if (x < cur_x + gap_l)
			x = cur_x + gap_l;
		ImGui::SetCursorPosX(x);
	}

	if (ImGui::KeybindButton(text.c_str(), unique_id, ImVec2(btn_w, ImGui::CalcTextSize(text.c_str()).y + 6 * c_menu::get().dpi_scale), clicked, ImGuiButtonFlags_::ImGuiButtonFlags_None))
		clicked = true;

	if (clicked)
	{
		hooks::input_shouldListen = true;
		hooks::input_receivedKeyval = &key_bind->key;
	}

	static auto hold = false, toggle = false;

	switch (key_bind->mode)
	{
	case HOLD_ON: hold = true; toggle = false; break;
	case HOLD_OFF: hold = false; toggle = false; break;
	case TOGGLE: toggle = true; hold = false; break;
	case ALWAYS_ON: hold = false; toggle = false; break;
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
		auto set_mode = [&](const char* label, key_bind_mode mode) {
			bool selected = (key_bind->mode == mode);
			if (ImGui::Selectable(label, selected))
			{
				key_bind->mode = mode;
				ImGui::CloseCurrentPopup();
			}
			};

		set_mode(crypt_str("Always On"), ALWAYS_ON);
		set_mode(crypt_str("Toggle"), TOGGLE);
		set_mode(crypt_str("Hold On"), HOLD_ON);
		set_mode(crypt_str("Hold Off"), HOLD_OFF);

		ImGui::EndPopup();
	}
}


void draw_combo(const char* name, int& variable, const char* labels[], int count)
{
	ImGui::Combo(std::string(name).c_str(), &variable, labels, count);
}

void draw_combo(const char* name, int& variable, bool (*items_getter)(void*, int, const char**), void* data, int count)
{
	ImGui::Combo(std::string(name).c_str(), &variable, items_getter, data, count);
}

static bool paint_kit_getter(void* data, int index, const char** output)
{
	auto kits = static_cast<std::vector<SkinChanger::PaintKit>*>(data);

	if (!kits || index < 0 || index >= static_cast<int>(kits->size()))
		return false;

	*output = kits->at(index).name.c_str();
	return true;
}

static bool quality_name_getter(void*, int index, const char** output)
{
	if (index < 0 || index >= static_cast<int>(std::size(game_data::quality_names)))
		return false;

	*output = game_data::quality_names[index].name;
	return true;
}

static bool knife_name_getter(void* data, int index, const char** output)
{
	auto names = static_cast<const game_data::weapon_name*>(data);
	*output = names[index].name;
	return true;
}

std::string get_config_dir()
{
	return get_config_directory();
}

void load_config(std::string selected_config)
{
	if (selected_config.empty())
		return;

	if (!cfg_manager->load(selected_config, false))
	{
		eventlogs::get().add(crypt_str("Failed to load ") + selected_config);
		return;
	}
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
	if (selected_config.empty())
		return;

	cfg.scripts.scripts.clear();

	if (!cfg_manager->save(selected_config))
	{
		eventlogs::get().add(crypt_str("Failed to save ") + selected_config + crypt_str(" to ") + get_config_directory());
		return;
	}

	cfg_manager->config_files();

	eventlogs::get().add(crypt_str("Saved ") + selected_config);
}

void remove_config(std::string selected_config)
{
	if (selected_config.empty())
		return;

	if (!cfg_manager->remove(selected_config))
	{
		eventlogs::get().add(crypt_str("Failed to delete ") + selected_config);
		return;
	}

	eventlogs::get().add(crypt_str("Deleted ") + selected_config);

	cfg_manager->config_files();

	files = cfg_manager->files;

	if (files.empty())
		cfg.selected_config = 0;
	else if (cfg.selected_config >= (int)files.size())
		cfg.selected_config = (int)files.size() - 1;
}

void add_config(std::string name)
{
	const std::string invalid = crypt_str("\\/:*?\"<>|");

	for (auto& c : name)
		if (invalid.find(c) != std::string::npos || (unsigned char)c < ' ')
			c = '_';

	while (!name.empty() && (name.front() == ' ' || name.front() == '.'))
		name.erase(name.begin());

	while (!name.empty() && (name.back() == ' ' || name.back() == '.'))
		name.pop_back();

	if (name.empty())
		name = crypt_str("config");

	const std::string extension = crypt_str(".cfg");

	if (name.size() < extension.size() || name.compare(name.size() - extension.size(), extension.size(), extension) != 0)
		name += extension;

	if (!cfg_manager->save(name))
	{
		eventlogs::get().add(crypt_str("Failed to create ") + name + crypt_str(" in ") + get_config_directory());
		return;
	}

	cfg_manager->config_files();

	files = cfg_manager->files;

	eventlogs::get().add(crypt_str("Added ") + name + crypt_str(" config"));
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
	auto& c = style.Colors;
	const auto accent = accent_color;
	const auto dark = !light_mode;
	c[ImGuiCol_WindowBg] = dark ? ImVec4(0.025f, 0.026f, 0.030f, 1.f) : ImVec4(0.965f, 0.968f, 0.975f, 1.f);
	c[ImGuiCol_ChildBg] = dark ? ImVec4(0.038f, 0.039f, 0.044f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f);
	c[ImGuiCol_PopupBg] = dark ? ImVec4(0.032f, 0.033f, 0.038f, 0.99f) : ImVec4(0.985f, 0.985f, 0.99f, 0.99f);
	c[ImGuiCol_Border] = dark ? ImVec4(0.10f, 0.10f, 0.115f, 1.f) : ImVec4(0.84f, 0.85f, 0.88f, 1.f);
	c[ImGuiCol_BorderShadow] = ImVec4(0.f, 0.f, 0.f, 0.f);
	c[ImGuiCol_FrameBg] = dark ? ImVec4(0.065f, 0.066f, 0.074f, 1.f) : ImVec4(0.925f, 0.93f, 0.945f, 1.f);
	c[ImGuiCol_FrameBgHovered] = dark ? ImVec4(0.09f, 0.09f, 0.102f, 1.f) : ImVec4(0.89f, 0.90f, 0.925f, 1.f);
	c[ImGuiCol_FrameBgActive] = ImVec4(accent.x, accent.y, accent.z, 0.28f);
	c[ImGuiCol_TitleBg] = c[ImGuiCol_TitleBgActive] = c[ImGuiCol_TitleBgCollapsed] = c[ImGuiCol_WindowBg];
	c[ImGuiCol_ScrollbarBg] = ImVec4(0.f, 0.f, 0.f, 0.f);
	c[ImGuiCol_ScrollbarGrab] = ImVec4(accent.x, accent.y, accent.z, 0.45f);
	c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(accent.x, accent.y, accent.z, 0.70f);
	c[ImGuiCol_ScrollbarGrabActive] = accent;
	c[ImGuiCol_CheckMark] = accent;
	c[ImGuiCol_SliderGrab] = ImVec4(accent.x, accent.y, accent.z, 0.82f);
	c[ImGuiCol_SliderGrabActive] = accent;
	c[ImGuiCol_Button] = c[ImGuiCol_FrameBg];
	c[ImGuiCol_ButtonHovered] = c[ImGuiCol_FrameBgHovered];
	c[ImGuiCol_ButtonActive] = ImVec4(accent.x, accent.y, accent.z, 0.35f);
	c[ImGuiCol_Header] = ImVec4(accent.x, accent.y, accent.z, 0.16f);
	c[ImGuiCol_HeaderHovered] = ImVec4(accent.x, accent.y, accent.z, 0.24f);
	c[ImGuiCol_HeaderActive] = ImVec4(accent.x, accent.y, accent.z, 0.34f);
	c[ImGuiCol_Separator] = c[ImGuiCol_Border];
	c[ImGuiCol_SeparatorHovered] = ImVec4(accent.x, accent.y, accent.z, 0.55f);
	c[ImGuiCol_SeparatorActive] = accent;
	c[ImGuiCol_Text] = dark ? ImVec4(0.80f, 0.80f, 0.84f, 1.f) : ImVec4(0.16f, 0.17f, 0.20f, 1.f);
	c[ImGuiCol_TextDisabled] = dark ? ImVec4(0.36f, 0.36f, 0.40f, 1.f) : ImVec4(0.52f, 0.53f, 0.57f, 1.f);

	style.WindowPadding = ImVec2(0.f, 0.f);
	style.FramePadding = ImVec2(5.f, 3.f);
	style.ItemSpacing = ImVec2(6.f, 3.f);
	style.ItemInnerSpacing = ImVec2(5.f, 3.f);
	style.WindowRounding = 11.f;
	style.ChildRounding = 6.f;
	style.FrameRounding = 5.f;
	style.PopupRounding = 6.f;
	style.ScrollbarRounding = 4.f;
	style.GrabRounding = 3.f;
	style.ScrollbarSize = 4.f;
	style.GrabMinSize = 8.f;
	style.FrameBorderSize = 0.f;
	style.PopupBorderSize = 1.f;
	style.ChildBorderSize = 0.f;
	style.WindowBorderSize = 0.f;

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
		ImGui::SetCursorPos(ImVec2(174, 84));
		ImGui::MenuChild("General", ImVec2(280, 300));
		{
			ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
			ImGui::Checkbox(crypt_str("Enable"), &cfg.ragebot.enable);
		}
		ImGui::EndChild();

		ImGui::SetCursorPos(ImVec2(466, 84));
		ImGui::MenuChild("Other", ImVec2(280, 300));
		{
			ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
			ImGui::Checkbox(crypt_str("Automatic scope"), &cfg.ragebot.autoscope);
			ImGui::Checkbox(crypt_str("Knife Bot"), &cfg.ragebot.knife_bot);
			ImGui::Checkbox(crypt_str("Tazer Bot"), &cfg.ragebot.zeus_bot);
		}
		ImGui::EndChild();

		ImGui::SetCursorPos(ImVec2(466, 396));
		ImGui::MenuChild("Exploits", ImVec2(280, 208));
		{
			ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
			ImGui::Checkbox(crypt_str("DT"), &cfg.ragebot.double_tap);
			if (cfg.ragebot.double_tap)
			{
				ImGui::SameLine(0.0f, 8.0f);
				draw_keybind(crypt_str(""), &cfg.ragebot.double_tap_key, crypt_str("##HOTKEY_DOUBLETAP"), true);
			}

			ImGui::Checkbox(crypt_str("HS"), &cfg.antiaim.hide_shots);
			if (cfg.antiaim.hide_shots)
			{
				ImGui::SameLine(0.0f, 8.0f);
				draw_keybind(crypt_str(""), &cfg.antiaim.hide_shots_key, crypt_str("##HOTKEY_HIDESHOTS"), true);
			}
		}
		ImGui::EndChild();

		ImGui::SetCursorPos(ImVec2(174, 396));
		ImGui::MenuChild("Main", ImVec2(280, 208));
		{
			ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
			ImGui::Checkbox(crypt_str("Defensive"), &cfg.ragebot.defensive_doubletap);
			ImGui::Checkbox(crypt_str("Anti exploit"), &cfg.ragebot.anti_exploit);
		}
		ImGui::EndChild();
	}
	else if (rg_tab == 1)
	{
		const char* rage_weapon[8] = { crypt_str("Heavy Pistols"), crypt_str("Pistols"), crypt_str("SMG"), crypt_str("Rifles"), crypt_str("Auto Sniper"), crypt_str("Scout"), crypt_str("AWP"), crypt_str("Heavy") };

		ImGui::SetCursorPos(ImVec2(174, 84));
		ImGui::MenuChild("Settings", ImVec2(280, 520));
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

		ImGui::SetCursorPos(ImVec2(466, 84));

		ImGui::MenuChild("Extra", ImVec2(280, 520));
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
				ImGui::SameLine();
				draw_keybind(crypt_str("Force safe points"), &cfg.ragebot.safe_point_key, crypt_str("##HOKEY_FORCE_SAFE_POINTS"));
			}

			ImGui::Checkbox(crypt_str("Prefer body aim"), &cfg.ragebot.weapon[hooks::rage_weapon].prefer_body_aim);
			if (cfg.ragebot.weapon[hooks::rage_weapon].prefer_body_aim)
			{
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
	ImGui::SetCursorPos(ImVec2(174, 84));
	ImGui::MenuChild("General", ImVec2(280, 520));
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
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.55f, 0.55f, 1.0f));
					ImGui::Text("Invert desync");
					ImGui::PopStyleColor();
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
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.55f, 0.55f, 1.0f));
		ImGui::Text("Manual back");
		ImGui::PopStyleColor();
		ImGui::SameLine();
		draw_keybind(crypt_str("Manual back"), &cfg.antiaim.manual_back, crypt_str("##HOTKEY_INVERT_BACK"));

		ImGui::SetCursorPosX(9);
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.55f, 0.55f, 1.0f));
		ImGui::Text("Manual left");
		ImGui::PopStyleColor();
		ImGui::SameLine();
		draw_keybind(crypt_str("Manual left"), &cfg.antiaim.manual_left, crypt_str("##HOTKEY_INVERT_LEFT"));

		ImGui::SetCursorPosX(9);
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.55f, 0.55f, 1.0f));
		ImGui::Text("Manual right");
		ImGui::PopStyleColor();
		ImGui::SameLine();
		draw_keybind(crypt_str("Manual right"), &cfg.antiaim.manual_right, crypt_str("##HOTKEY_INVERT_RIGHT"));

		if (cfg.antiaim.manual_back.key > KEY_NONE && cfg.antiaim.manual_back.key < KEY_MAX || cfg.antiaim.manual_left.key > KEY_NONE && cfg.antiaim.manual_left.key < KEY_MAX || cfg.antiaim.manual_right.key > KEY_NONE && cfg.antiaim.manual_right.key < KEY_MAX)
		{
			ImGui::Checkbox(crypt_str("Manuals indicator"), &cfg.antiaim.flip_indicator);
			ImGui::SameLine();
			ImGui::SetCursorPosX(ImGui::GetWindowSize().x - 30);
			ImGui::ColorEdit(crypt_str("##invc"), &cfg.antiaim.flip_indicator_color, ALPHA);
		}

		ImGui::Checkbox(crypt_str("Zero Land"), &cfg.antiaim.pitch_zero_on_land);

		draw_combo(crypt_str("Walk-type"), cfg.antiaim.walk_type, walktype, ARRAYSIZE(walktype));

		ImGui::SetCursorPosX(9);
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.55f, 0.55f, 1.0f));
		ImGui::Text("Auto peek");
		ImGui::PopStyleColor();
		ImGui::SameLine();
		draw_keybind(crypt_str(""), &cfg.misc.automatic_peek, crypt_str("##AUTOPEEK__HOTKEY"));

		if (&cfg.misc.automatic_peek.key)
		{
			ImGui::SetCursorPosX(9);
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.55f, 0.55f, 1.0f));
			ImGui::Text("Color");
			ImGui::PopStyleColor();
			ImGui::SameLine();
			ImGui::SetCursorPosX(ImGui::GetWindowSize().x - 30);
			ImGui::ColorEdit(crypt_str("##idsadsadsa"), &cfg.misc.automatic_peek_color, ALPHA);
		}

	}
	ImGui::EndChild();

	ImGui::SetCursorPos(ImVec2(466, 84));

	ImGui::MenuChild("Extra", ImVec2(280, 520));
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
		ImGui::SetCursorPos(ImVec2(174, 84));
		ImGui::MenuChild("Esp", ImVec2(280, 520));
		{
			ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();

			draw_combo(crypt_str("Set Team"), player, set_team, ARRAYSIZE(set_team));

			ImGui::Checkbox("Enable", &cfg.player.enable);
			ImGui::Checkbox(crypt_str("Name"), &cfg.player.type[player].name);

			ImGui::Checkbox(crypt_str("Health Bar"), &cfg.player.type[player].health);

			if (cfg.player.type[player].health)
			{
				ImGui::SetCursorPosX(9);
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.55f, 0.55f, 1.0f));
				ImGui::Text("Color");
				ImGui::PopStyleColor();
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
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.55f, 0.55f, 1.0f));
				ImGui::Text(crypt_str("Color"));
				ImGui::PopStyleColor();
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

		ImGui::SetCursorPos(ImVec2(466, 84));

		ImGui::MenuChild("Chams", ImVec2(280, 520));
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
						ImGui::SetCursorPosX(9);
						ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.55f, 0.55f, 1.0f));
						ImGui::Text(crypt_str("Visible "));
						ImGui::PopStyleColor();
					}

					if (cfg.player.type[player].chams[PLAYER_CHAMS_VISIBLE] && cfg.player.type[player].chams[PLAYER_CHAMS_INVISIBLE])
					{
						ImGui::ColorEdit(crypt_str("##chamsinvisible"), &cfg.player.type[player].xqz_color, ALPHA);
						ImGui::SameLine();
						ImGui::SetCursorPosX(9);
						ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.55f, 0.55f, 1.0f));
						ImGui::Text(crypt_str("Invisible "));
						ImGui::PopStyleColor();
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
		ImGui::SetCursorPos(ImVec2(174, 84));
		ImGui::MenuChild("World", ImVec2(280, 520));
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

		ImGui::SetCursorPos(ImVec2(466, 84));

		ImGui::MenuChild("Render", ImVec2(280, 520));
		{
			ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
			ImGui::Checkbox(crypt_str("Enabled"), &cfg.player.enable);

			ImGui::SliderInt(crypt_str("Field of view"), &cfg.esp.fov, 0, 89);

			//ImGui::Checkbox(crypt_str("Thirdperson"), &cfg.esp);

			ImGui::SetCursorPosX(9);
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.55f, 0.55f, 1.0f));
			ImGui::Text("Thirdperson");
			ImGui::PopStyleColor();
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
		ImGui::SetCursorPos(ImVec2(174, 84));
		ImGui::MenuChild("Movement", ImVec2(280, 520));
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

		ImGui::SetCursorPos(ImVec2(466, 84));

		ImGui::MenuChild("Extra", ImVec2(280, 520));
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
		ImGui::SetCursorPos(ImVec2(174, 84));
		ImGui::MenuChild("Info", ImVec2(280, 520));
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
				ImGui::SetCursorPosX(9);
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.55f, 0.55f, 1.0f));
				ImGui::Text("Color");
				ImGui::PopStyleColor();
				ImGui::SameLine();
				ImGui::SetCursorPosX(ImGui::GetWindowSize().x - 30);
				ImGui::ColorEdit(crypt_str("##logcolor"), &cfg.misc.log_color, ALPHA);
			}
			ImGui::Checkbox(crypt_str("Show CS:GO logs"), &cfg.misc.show_default_log);
		}
		ImGui::EndChild();

		ImGui::SetCursorPos(ImVec2(466, 84));

		ImGui::MenuChild("Extra", ImVec2(280, 520));
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

		ImGui::SetCursorPos(ImVec2(174, 84));
		ImGui::MenuChild("Config List", ImVec2(280, 520));
		{
			ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();

			ImGui::SetCursorPosX(-5);
			ImGui::PushItemWidth(250);
			ImGui::InputTextWithHint(crypt_str("##confignameinput"), crypt_str("Search..."), config_name, sizeof(config_name));
			ImGui::PopItemWidth();

			ImGui::Spacing(); ImGui::Spacing();

			cfg_manager->config_files();
			files = cfg_manager->files;

			for (auto file : files)
			{
				bool is_selected = selected_name == file;

				ImGui::SetCursorPosX(0);
				if (ImGui::cfgtab(file.c_str(), is_selected, ImVec2(250, 28)))
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

		ImGui::SetCursorPos(ImVec2(466, 84));

		ImGui::MenuChild("Config Settings", ImVec2(280, 520));
		{
			ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();

			if ((ImGui::CustomButton(crypt_str("Create new..."), crypt_str("##CreateConfig"), ImVec2(250, 30), true, c_menu::get().settingicons, "3")))
				add_config(config_name);

			if ((ImGui::CustomButton(crypt_str("Open Config Directory"), crypt_str("##OpenConfigDirectory"), ImVec2(250, 30), true, c_menu::get().settingicons, "2")))
			{
				const std::string& folder = get_config_directory();

				ShellExecute(NULL, crypt_str("open"), folder.c_str(), NULL, NULL, SW_SHOWNORMAL);
			}

			if (!selected_name.empty())
			{
				//Load confirmation
				{
					if (prenext_load && m_globals()->m_realtime < load_time + 3.f)
					{
						if ((ImGui::CustomButton(crypt_str(" Confirm?"), crypt_str("##ConfirmLoad"), ImVec2(250, 28), true, c_menu::get().settingicons, "5")) && !selected_name.empty()) {
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
						if ((ImGui::CustomButton(crypt_str(" Load"), crypt_str("##load"), ImVec2(250, 28), true, c_menu::get().settingicons, "5")) && !selected_name.empty()) {
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
						if ((ImGui::CustomButton(crypt_str(" Confirm?"), crypt_str("##ConfirmSave"), ImVec2(250, 28), true, c_menu::get().settingicons, "4")) && !selected_name.empty())
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
						if ((ImGui::CustomButton(crypt_str(" Save"), crypt_str("##Save"), ImVec2(250, 28), true, c_menu::get().settingicons, "4")) && !selected_name.empty())
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
						if ((ImGui::CustomButton(crypt_str("Confirm?"), crypt_str("##ConfirmDelete"), ImVec2(250, 28), true, c_menu::get().settingicons, "7")) && !selected_name.empty())
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
						if ((ImGui::CustomButton(crypt_str("Delete"), crypt_str("##Delete"), ImVec2(250, 28), true, c_menu::get().settingicons, "7")) && !selected_name.empty())
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
		ImGui::SetCursorPos(ImVec2(174, 84));
		ImGui::MenuChild("Scripts", ImVec2(280, 520));
		{
			ImGui::SetCursorPosY(42.0f);
			ImGui::Text("script manager");
			ImGui::TextDisabled("no scripts loaded");
		}
		ImGui::EndChild();

		ImGui::SetCursorPos(ImVec2(466, 84));
		ImGui::MenuChild("Runtime", ImVec2(280, 520));
		{
			ImGui::SetCursorPosY(42.0f);
			ImGui::Checkbox("Developer mode", &cfg.scripts.developer_mode);
		}
		ImGui::EndChild();
	}
}

void c_menu::skins_tab() // skins
{
	if (current_profile < 0 || current_profile >= static_cast<int>(cfg.skins.skinChanger.size()))
		current_profile = 0;

	ImGui::SetCursorPos(ImVec2(174, 84));
	ImGui::MenuChild("Deployed", ImVec2(280, 520));
	{
		ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();

		for (auto i = 0; i < static_cast<int>(cfg.skins.skinChanger.size()); ++i)
		{
			const auto selected = current_profile == i;

			if (ImGui::cfgtab(game_data::weapon_names[i].name, selected, ImVec2(250, 28)))
				current_profile = i;
		}

	}
	ImGui::EndChild();

	ImGui::SetCursorPos(ImVec2(466, 84));

	ImGui::MenuChild("Extra", ImVec2(280, 520));
	{
		ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
		auto& selected_entry = cfg.skins.skinChanger[current_profile];
		selected_entry.itemIdIndex = current_profile;
		selected_entry.update();
		auto changed = false;

		ImGui::PushItemWidth(250.f);
		changed |= ImGui::Combo("quality", &selected_entry.entity_quality_vector_index, quality_name_getter, nullptr, static_cast<int>(std::size(game_data::quality_names)));

		auto& kits = selected_entry.itemId == GLOVE_T_SIDE ? SkinChanger::gloveKits : SkinChanger::skinKits;

		if (!kits.empty())
			changed |= ImGui::Combo("paint kit", &selected_entry.paint_kit_vector_index, paint_kit_getter, &kits, static_cast<int>(kits.size()));
		else
			ImGui::TextDisabled("paint kits unavailable");

		if (selected_entry.itemId == WEAPON_KNIFE)
			changed |= ImGui::Combo("knife model", &selected_entry.definition_override_vector_index, knife_name_getter, const_cast<game_data::weapon_name*>(game_data::knife_names), static_cast<int>(std::size(game_data::knife_names)));
		else if (selected_entry.itemId == GLOVE_T_SIDE)
			changed |= ImGui::Combo("glove model", &selected_entry.definition_override_vector_index, knife_name_getter, const_cast<game_data::weapon_name*>(game_data::glove_names), static_cast<int>(std::size(game_data::glove_names)));

		changed |= ImGui::SliderFloat("wear", &selected_entry.wear, 0.0001f, 1.0f, "%.4f");
		changed |= ImGui::SliderInt("seed", &selected_entry.seed, 0, 1000);
		changed |= ImGui::Checkbox("stat trak", &selected_entry.stat_trak);
		changed |= ImGui::InputText("name", selected_entry.custom_name, sizeof(selected_entry.custom_name));
		ImGui::PopItemWidth();

		draw_combo(crypt_str("Player models"), cfg.player.player_models, player_models, ARRAYSIZE(player_models));

		if (changed)
		{
			selected_entry.update();
			SkinChanger::scheduleHudUpdate();
		}

		if (ImGui::CustomButton(crypt_str("Apply"), crypt_str("##updater"), ImVec2(250, 30), false, c_menu::get().settingicons, "0"))
			SkinChanger::scheduleHudUpdate();
	}
	ImGui::EndChild();
}

void c_menu::subtabs()
{
	auto wpos = ImGui::GetWindowPos();
	auto wdl = ImGui::GetWindowDrawList();
	wdl->AddLine(
		ImVec2(wpos.x + 160, wpos.y + 66),
		ImVec2(wpos.x + width, wpos.y + 66),
		ImGui::GetColorU32(ImGuiCol_Border), 1.f);

	if (tab_static == 0)
	{
		ImGui::SetCursorPos(ImVec2{ 548, 20 });
		ImGui::PushFont(c_menu::get().g_cxm);
		if (ImGui::subtab("", "general", rg_tab == 0)) rg_tab = 0;

		ImGui::SetCursorPos(ImVec2{ 636, 20 });
		if (ImGui::subtab("", "weapons", rg_tab == 1)) rg_tab = 1;
		ImGui::PopFont();
	}
	else if (tab_static == 4)
	{
		ImGui::SetCursorPos(ImVec2{ 548, 20 });
		ImGui::PushFont(c_menu::get().g_cxm);
		if (ImGui::subtab("", "general", mi_tab == 0)) mi_tab = 0;

		ImGui::SetCursorPos(ImVec2{ 636, 20 });
		if (ImGui::subtab("", "info", mi_tab == 1)) mi_tab = 1;
		ImGui::PopFont();
	}
};

void c_menu::tabs()
{
	const char* icons[] = { "F", "D", "G", "G", "A", "V", "C", "C" };
	const char* labels[] = { "ragebot", "anti-aim", "players", "world", "misc", "skins", "configs", "scripts" };
	const float positions[] = { 86.f, 122.f, 182.f, 218.f, 278.f, 314.f, 374.f, 410.f };
	for (int i = 0; i < 8; i++)
	{
		ImGui::SetCursorPos(ImVec2(10.f, positions[i]));
		if (ImGui::tab(icons[i], labels[i], tab_static == i))
			tab_static = i;
	}

	static float tab_alpha = 1.0f;
	static int last_tab = -1;
	if (last_tab != tab_static)
	{
		tab_alpha = 0.0f;
		last_tab = tab_static;
	}
	tab_alpha = ImClamp(tab_alpha + (4.0f * ImGui::GetIO().DeltaTime), 0.0f, 1.0f);

	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, tab_alpha * ImGui::GetStyle().Alpha);

	switch (tab_static)
	{
	case 0: rage_tab();     break;
	case 1: aa_tab();       break;
	case 2: vis_tab = 0; visuals_tab(); break;
	case 3: vis_tab = 1; visuals_tab(); break;
	case 4: misc_tab(); break;
	case 5: skins_tab(); break;
	case 6: lua_tab = 0; settings_tab(); break;
	case 7: lua_tab = 1; settings_tab(); break;
	}

	ImGui::PopStyleVar();
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
	ImGuiStyle previous_style = ss;
	menu_setup(ss);

	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImClamp(public_alpha, 0.f, 1.f));
	ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
	if (ImGui::Begin(("Lambda"), nullptr,
		ImGuiWindowFlags_NoDecoration |
		ImGuiWindowFlags_NoScrollWithMouse |
		ImGuiWindowFlags_NoScrollbar))
	{
		auto wp = ImGui::GetWindowPos();
		auto dl = ImGui::GetWindowDrawList();
		float W = static_cast<float>(width);
		float H = static_cast<float>(height);
		auto background = ImGui::GetColorU32(ImGuiCol_WindowBg);
		auto sidebar = ImGui::GetColorU32(ImGuiCol_ChildBg);
		auto border = ImGui::GetColorU32(ImGuiCol_Border);
		auto text = ImGui::GetColorU32(ImGuiCol_Text);
		auto muted = ImGui::GetColorU32(ImGuiCol_TextDisabled);

		dl->AddRectFilled(wp, wp + ImVec2(W, H), background, 11.f);
		dl->AddRectFilled(wp, wp + ImVec2(160, H), sidebar, 11.f, ImDrawCornerFlags_Left);
		dl->AddLine(wp + ImVec2(160, 0), wp + ImVec2(160, H), border, 1.f);
		dl->AddRect(wp + ImVec2(0.5f, 0.5f), wp + ImVec2(W - 0.5f, H - 0.5f), border, 11.f, ImDrawCornerFlags_All, 1.f);






		auto logo = wp + ImVec2(25, 17);
		auto logo_shadow = light_mode ? ImColor(125, 125, 132, 255) : ImColor(105, 105, 114, 255);
		auto logo_face = light_mode ? ImColor(40, 40, 44, 255) : ImColor(235, 235, 238, 255);
		dl->AddLine(logo + ImVec2(1, 27), logo + ImVec2(11, 6), logo_shadow, 6.f);
		dl->AddLine(logo + ImVec2(7, 1), logo + ImVec2(11, 6), logo_shadow, 6.f);
		dl->AddLine(logo + ImVec2(11, 6), logo + ImVec2(25, 27), logo_shadow, 6.f);
		dl->AddLine(logo + ImVec2(1, 26), logo + ImVec2(11, 5), logo_face, 2.5f);
		dl->AddLine(logo + ImVec2(7, 0), logo + ImVec2(11, 5), logo_face, 2.5f);
		dl->AddLine(logo + ImVec2(11, 5), logo + ImVec2(25, 26), logo_face, 2.5f);

		ImGui::PushFont(g_cxm);
		const char* section_names[] = { "ragebot", "anti-aim", "players", "world", "misc", "skins", "configurations", "scripts" };
		dl->AddText(wp + ImVec2(174, 26), muted, section_names[tab_static]);
		dl->AddText(wp + ImVec2(16, 70), muted, "aimbot");
		dl->AddText(wp + ImVec2(16, 166), muted, "visuals");
		dl->AddText(wp + ImVec2(16, 262), muted, "miscellaneous");
		dl->AddText(wp + ImVec2(16, 358), muted, "other");
		dl->AddCircleFilled(wp + ImVec2(26, 580), 13.f, light_mode ? ImColor(226, 226, 229) : ImColor(32, 32, 36));
		dl->AddCircle(wp + ImVec2(26, 580), 13.f, border, 18, 1.f);
		dl->AddCircleFilled(wp + ImVec2(26, 576), 3.5f, muted);
		dl->AddCircle(wp + ImVec2(26, 586), 6.5f, muted, 14, 2.f);
		dl->AddText(wp + ImVec2(46, 568), text, "lambda user");
		dl->AddText(wp + ImVec2(46, 586), muted, "release build");
		ImGui::PopFont();
		ImGui::SetCursorPos(ImVec2(125, 570));
		if (ImGui::InvisibleButton("##theme", ImVec2(27, 20)))
			light_mode = !light_mode;
		static float theme_anim = 0.f;
		theme_anim = ImClamp(theme_anim + ImGui::GetIO().DeltaTime * 8.f * (light_mode ? 1.f : -1.f), 0.f, 1.f);
		auto toggle_min = wp + ImVec2(125, 570);
		auto toggle_max = toggle_min + ImVec2(27, 18);
		dl->AddRectFilled(toggle_min, toggle_max, light_mode ? ImColor(215, 215, 220) : ImColor(37, 37, 42), 9.f);
		dl->AddRect(toggle_min, toggle_max, border, 9.f);
		auto knob = ImVec2(toggle_min.x + 8.f + theme_anim * 11.f, toggle_min.y + 9.f);
		dl->AddCircleFilled(knob, 5.f, light_mode ? ImColor(255, 255, 255) : ImColor(210, 210, 216));
		if (light_mode)
		{
			dl->AddCircle(knob, 2.f, ImColor(90, 90, 96), 10, 1.f);
			dl->AddLine(knob + ImVec2(0, -4), knob + ImVec2(0, -3), ImColor(90, 90, 96), 1.f);
			dl->AddLine(knob + ImVec2(0, 3), knob + ImVec2(0, 4), ImColor(90, 90, 96), 1.f);
		}
		else
		{
			dl->AddCircleFilled(knob + ImVec2(1, 0), 2.5f, ImColor(76, 76, 84));
			dl->AddCircleFilled(knob + ImVec2(2, -1), 2.5f, ImColor(210, 210, 216));
		}

		ImGui::SetCursorPos(ImVec2(W - 42.f, 14.f));
		if (ImGui::InvisibleButton("##settings", ImVec2(28, 28)))
			settings_open = !settings_open;
		auto gear = wp + ImVec2(W - 28.f, 28.f);
		dl->AddCircleFilled(gear, 8.f, muted, 20);
		dl->AddRectFilled(gear + ImVec2(-2, -13), gear + ImVec2(2, -7), muted, 1.f);
		dl->AddRectFilled(gear + ImVec2(-2, 7), gear + ImVec2(2, 13), muted, 1.f);
		dl->AddRectFilled(gear + ImVec2(-13, -2), gear + ImVec2(-7, 2), muted, 1.f);
		dl->AddRectFilled(gear + ImVec2(7, -2), gear + ImVec2(13, 2), muted, 1.f);
		dl->AddRectFilled(gear + ImVec2(-10, -10), gear + ImVec2(-6, -6), muted, 1.f);
		dl->AddRectFilled(gear + ImVec2(6, -10), gear + ImVec2(10, -6), muted, 1.f);
		dl->AddRectFilled(gear + ImVec2(-10, 6), gear + ImVec2(-6, 10), muted, 1.f);
		dl->AddRectFilled(gear + ImVec2(6, 6), gear + ImVec2(10, 10), muted, 1.f);
		dl->AddCircleFilled(gear, 3.f, background, 16);

		{
			ImGui::PushFont(g_cxm);
			subtabs();
			tabs();
			ImGui::PopFont();
		}
	}
	ImGui::End();

	if (settings_open)
	{
		ImGui::SetNextWindowSize(ImVec2(360, 240), ImGuiCond_Always);
		ImGui::SetNextWindowPos(ImGui::GetIO().DisplaySize * 0.5f, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14.f, 12.f));
		if (ImGui::Begin("Settings##lambda", &settings_open, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
		{
			ImGui::PushFont(g_cxmenufont);
			ImGui::Text("appearance");
			ImGui::Spacing();
			ImGui::ColorEdit3("accent color", &accent_color.x, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha);
			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();
			ImGui::Text("build date");
			ImGui::SameLine(115.f);
			ImGui::TextDisabled("%s", __DATE__);
			ImGui::Text("last updated");
			ImGui::SameLine(115.f);
			ImGui::TextDisabled("01 Sep 2026");
			ImGui::PopFont();
		}
		ImGui::End();
		ImGui::PopStyleVar();
	}
	ImGui::PopStyleVar();
	ss = previous_style;
	menu_setupped = false;
}
