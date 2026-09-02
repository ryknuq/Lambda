#pragma once

#include <string>
#include <vector>

extern bool sound_preview_blocked;

void play_hitsound(int index, int volume);
void preview_hitsound(int menu_index, int volume);
const std::vector<std::string>& GetSoundNames();
