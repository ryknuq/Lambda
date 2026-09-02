#include "Console.h"

#include "../SDK/Interfaces.h"


CGameConsole* Console = new CGameConsole;

inline int HexToDec(char h) {
	if (h >= 'A')
		return 10 + h - 'A';
	return h - '0';
}

Color ParseColorFromHex(const std::string& clr) {
	return Color(
		HexToDec(clr[0]) * 16 + HexToDec(clr[1]),
		HexToDec(clr[2]) * 16 + HexToDec(clr[3]),
		HexToDec(clr[4]) * 16 + HexToDec(clr[5])
	);
}


void CGameConsole::LambdaTag() {
	CVar->ConsoleColorPrintf(Color(99, 130, 255), "lambda | ");
}

void CGameConsole::Print(const std::string& msg, Color color_def) {
	Color current_clr = color_def;
	std::string str;
	std::string clr_to_parse;
	int clr_digits = -1;
	for (auto c : msg) {
		if (c != '\a' && clr_digits == -1) {
			str += c;
			continue;
		}

		if (c == '\a') {
			if (!str.empty())
				ColorPrint(str, current_clr);
			str.clear();
			clr_digits = 0;
			continue;
		}

		clr_to_parse += c;
		clr_digits++;

		if (clr_digits == 6) {
			clr_digits = -1;
			if (clr_to_parse == "ACCENT")
				current_clr = Color(191, 198, 227);
			else if (clr_to_parse == "_MAIN_")
				current_clr = Color(232);
			else
				current_clr = ParseColorFromHex(clr_to_parse);
			clr_to_parse.clear();
		}
	}
	if (!str.empty())
		ColorPrint(str, current_clr);
}

void CGameConsole::ColorPrint(const std::string& msg, const Color& color) {
	CVar->ConsoleColorPrintf(color, msg.c_str());
}

void CGameConsole::Log(const std::string& msg) {
	CVar->ConsoleColorPrintf(Color(99, 130, 255), "lambda | ");
	CVar->ConsoleColorPrintf(Color(232), msg.c_str());
	CVar->ConsolePrintf("\n");
}

void CGameConsole::Error(const std::string& error) {
	LambdaTag();

	CVar->ConsoleColorPrintf(Color(255, 50, 50), error.c_str());
	CVar->ConsolePrintf("\n");
}