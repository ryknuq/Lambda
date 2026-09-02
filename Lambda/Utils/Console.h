#pragma once

#include <string>
#include "../SDK/Misc/Color.h"

class CGameConsole {
public:
	void LambdaTag();
	void Print(const std::string& msg, Color colo_def = Color(232));
	void ColorPrint(const std::string& msg, const Color& color);
	void Log(const std::string& msg);
	void Error(const std::string& error);
};


extern CGameConsole* Console;