#pragma once
#include "..\..\includes.hpp"
#include "..\lagcompensation\animation_system.h"
#include "aim.h"

; class knifebot : public singleton <knifebot>
{
	void scan_targets();
	void fire(CUserCmd* cmd);
	void knife();
	int determinate_hit_type(bool stab_type, const Vector& delta);
	scanned_target final_target;
	bool work = false;
public:
	void run(CUserCmd* cmd);
};