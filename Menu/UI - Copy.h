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
	int ragebotsubtab = 0;
	int player = 0;
	int gen = 0;
	int nl = 0;
	int hueta = 0;

	std::string preview = crypt_str("None");

	void dpi_resize(float scale_factor, ImGuiStyle& style);
	void rage_tab();
	void anti_tab();
	void player_tab();
	void visuals_tab();
	void world_tab();
	void general_tab();
	void configs_tab();
	void lua_tab();
	void render_tab();
};
