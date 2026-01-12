#pragma once

#include <array>
#include <numbers>
#include <string>
#include <vector>

#include "..\..\Menu\ImGui\imgui.h"

#include "..\..\Menu\Configs\Configs.h"

struct Vector;

struct Color3 {
    std::array<float, 3> color{ 1.0f, 1.0f, 1.0f };
    float rainbowSpeed = 0.6f;
    bool rainbow = false;
};
#pragma pack(pop)

namespace Helpers
{
    unsigned int calculateColor(Color4 color) noexcept;
    unsigned int calculateColor(Color3 color) noexcept;
    unsigned int calculateColor(int r, int g, int b, int a) noexcept;
}
