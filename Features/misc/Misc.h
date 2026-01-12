#include "..\..\includes.hpp"

; struct trail_pos_info
{
	Vector position;
	float time = -1;
};

class misc : public singleton <misc> 
{
public:
	void watermark();
	void NoDuck(CUserCmd* cmd);
	void AutoCrouch(CUserCmd* cmd);
	void SlideWalk(CUserCmd* cmd);
	void slowwalk_movement(CUserCmd* cmd);
	void automatic_peek(CUserCmd* cmd, float wish_yaw);
	void ViewModel();
	void FullBright();
	void PovArrows(player_t* e, Color color);
	void hurtindicator();
	void NightmodeFix();
	void desync_arrows();
	void keybinds();
	void spectators_list();
	void aimbot_hitboxes();
	void fast_stop(CUserCmd* m_pcmd);
	void double_tap_defensive(CUserCmd* m_pcmd);
	void fix_dtteleport(CUserCmd* m_pcmd);
	void reset_double_tap_state();
	bool double_tap(CUserCmd* m_pcmd);
	bool can_fire(CUserCmd* m_pcmd, int max_tickbase_shift);
	void process_fire_command(CUserCmd* m_pcmd, int max_tickbase_shift);
	void hide_shots(CUserCmd* m_pcmd, bool should_work);
	void KillEffect(IGameEvent* pEvent);

	void Trail();

	std::vector<trail_pos_info> trail_pos;

	bool recharging_double_tap = false;

	bool createmove(CUserCmd* m_pcmd);

	void break_lc(CUserCmd* m_pcmd);

	bool double_tap_enabled = false;
	bool double_tap_key = false;

	bool firing_dt = false;

	bool hide_shots_enabled = false;
	bool hide_shots_key = false;

	// breaklc
	bool condition = true;
	bool started_peeking = false;

	int max_choke = 0;

	// lag exploit
	bool recharge_lag = false;

	bool shott = false;
};

class CTeslaInfo
{
public:
	Vector m_vPos;
	Vector m_vAngles;
	int m_nEntIndex;
	const char* m_pszSpriteName;
	float m_flBeamWidth;
	int m_nBeams;
	Vector m_vColor;
	float m_flTimeVisible;
	float m_flRadius;
	float m_flRed;
	float m_flGreen;
	float m_flBlue;
	float m_flBrightness;
};