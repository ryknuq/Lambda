#pragma once
#include "../../UI/UI.h"
#include "../../Utils/Console.h"

#include <deque>
#include <mutex>

class CElements {
	struct anim_row_t {
		uintptr_t id = 0;
		std::string title;
		std::string value;
		std::string extra;
		bool active = false;
		bool hidden = false;
		bool present = false;
		float alpha = 0.f;
		float y = 0.f;
		bool placed = false;
		float slash = 0.f;
		float hover = 0.f;
	};

	struct log_part_t {
		std::string text;
		ImVec4 color;
		float width = 0.f;
	};

	struct log_t {
		std::vector<log_part_t> parts;
		float width = 0.f;
		float birth = 0.f;
		float x = -12.f;
		float y = 0.f;
		bool placed = false;
	};

	struct bomb_state_t {
		bool live = false;
		float remaining = 0.f;
		float timer_length = 40.f;
		bool defusing = false;
		float defuse_remaining = 0.f;
		bool can_defuse = false;
	};

	struct panel_state_t {
		float alpha = 0.f;
		float gear = 0.f;
		float spin = 0.f;
		float spin_target = 0.f;
		float open = 0.f;
		bool settings = false;
	};

	panel_state_t m_BombPanel;
	panel_state_t m_KeybindPanel;
	panel_state_t m_SpectatorPanel;
	panel_state_t m_LogPanel;

	int m_nBombIndex = -1;
	float m_flNextBombScan = 0.f;
	float m_flBombBar = 0.f;
	float m_flBombDefuse = 0.f;
	float m_flLogWidth = 0.f;

	bomb_state_t m_BombState;
	std::vector<anim_row_t> m_KeybindRows;
	std::vector<anim_row_t> m_SpectatorRows;

	std::mutex m_LogMutex;
	std::vector<std::string> m_PendingLogs;
	std::deque<log_t> m_Logs;

	void DrawBombTimer();
	void DrawKeybinds();
	void DrawSpectators();
	void DrawEventLog();

public:
	void Draw();
	void AddLog(const std::string& msg);
};

extern CElements* Elements;
