#pragma once

#include "Menu/ImGui/imgui.h"

#include "Menu/Configs/Configs.h"

struct Color4;
struct ColorToggle;
struct ColorToggleRounding;
struct ColorToggleThickness;
struct ColorToggleThicknessRounding;

namespace ImGui
{
    void progressBarFullWidth(float fraction, float height);
    void textUnformattedCentered(const char* text);
}
