#pragma once

#include <string>
#include <vector>
#include "../SDK/Misc/Color.h"

struct log_segment_t {
	std::string text;
	Color color;
};

std::vector<log_segment_t> ParseLogSegments(const std::string& msg, Color color_def = Color(232), Color accent = Color(191, 198, 227));

class CGameConsole {
public:
	void LambdaTag();
	void Print(const std::string& msg, Color colo_def = Color(232));
	void ColorPrint(const std::string& msg, const Color& color);
	void Log(const std::string& msg);
	void Error(const std::string& error);
	void Event(const std::string& msg);
};


extern CGameConsole* Console;