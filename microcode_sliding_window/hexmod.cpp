/*
	for debugging/investigation purposes
*/
#include <Windows.h>

static void* g_hexmod_base = nullptr;


bool init_hexmod(bool is_arm) {

	const char* modname;

	if (is_arm) {
#ifdef __EA64__
		modname = "hexarm64.dll";
#else
		modname = "hexarm.dll";
#endif
	}
	else {
#ifdef __EA64__
		modname = "hexx64.dll";
#else
		modname = "hexrays.dll";
#endif
	}

	g_hexmod_base = (void*)GetModuleHandleA(modname);


	
	return g_hexmod_base != nullptr;
}

void* get_hexmod() {
	return g_hexmod_base;
}