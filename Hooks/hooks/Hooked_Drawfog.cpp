#include "..\hooks.hpp"

bool __fastcall hooks::hooked_drawfog(void* ecx, void* edx)
{
	return !cfg.esp.removals[REMOVALS_FOGS] || cfg.esp.fog;
}