#pragma once
#include "../includes.hpp"

; class c_menu : public singleton<c_menu>
{
public:
	void draw(bool is_open);
	void menu_setup(ImGuiStyle& style);

	//Fonts
	ImFont* futura;
	ImFont* futura_large;
	ImFont* futura_small;
	ImFont* gotham;
	ImFont* ico_menu;
	ImFont* ico_bottom;
	ImFont* weapons;
	ImFont* MenuFontRender;

	// New fonts
	ImFont* OpenSans;
	ImFont* OpenSansBold;
	ImFont* Timers;
	ImFont* g_pMenuFont;
	ImFont* keybinds;
	ImFont* IconFont;
	ImFont* LexendDecaFont;
	ImFont* MenuFont;
	ImFont* settingicons;
	ImFont* New_icons;

	IDirect3DTexture9* logo = nullptr;
	IDirect3DTexture9* foto = nullptr;

	// NEW
	IDirect3DTexture9* nigga = nullptr;
	IDirect3DTexture9* divine = nullptr;

	IDirect3DDevice9* device;

	// NEW MENU FONTS
	ImFont* g_icons;
	ImFont* g_last;
	ImFont* g_cxmenufont;
	ImFont* g_cxm;
	ImFont* g_widgets;

	float public_alpha;
	float color_buffer[4] = { 1.f, 1.f, 1.f, 1.f };
    float dpi_scale = 1.f;

	std::string loaded_config = crypt_str("");

	bool config_bind = false;

	float save_time = m_globals()->m_realtime;
	bool  prenext_save = false;

	float load_time = m_globals()->m_realtime;
	bool  prenext_load = false;

	float delete_time = m_globals()->m_realtime;
	bool  prenext_delete = false;
private:

	struct
	{
        ImVec2 DisplayWindowPadding;
		ImVec2 DisplaySafeAreaPadding;
		ImVec2 WindowPadding;
		ImVec2 WindowMinSize;
		ImVec2 FramePadding;
		ImVec2 ItemSpacing;
		ImVec2 ItemInnerSpacing;
		ImVec2 TouchExtraPadding;

		float  WindowRounding;
		float  ChildRounding;
		float  PopupRounding;
		float  FrameRounding;
		float  IndentSpacing;
		float  ColumnsMinSpacing;
		float  ScrollbarSize;
		float  ScrollbarRounding;
		float  GrabMinSize;
		float  GrabRounding;
		float  TabRounding;
		float  TabMinWidthForUnselectedCloseButton;
		float  MouseCursorScale;

	} styles;

	bool update_dpi = false;
	bool update_scripts = false;

	ImGuiStyle style;

	float child_height;
	float preview_alpha = 1.f;

	int active_tab_index;
	int width = 850, height = 560;
	int active_tab;
	int current_profile = -1;

	int tab_static;
	int rg_tab = 0;
	int vis_tab = 0;
	int mi_tab = 0;
	int lua_tab = 0;

	int player;

	std::string preview = crypt_str("None");

	void dpi_resize(float scale_factor, ImGuiStyle& style);
	void tabs();
	void subtabs();
	void rage_tab();
	void aa_tab();
	void visuals_tab();
	void misc_tab();
	void settings_tab();
	void skins_tab();
};
