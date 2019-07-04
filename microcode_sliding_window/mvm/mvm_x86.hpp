#pragma once


void mvm_x86_init();


#define X(name)		extern mreg_info_t* mri_x86_##name;
#include "xmacro_x86_mvm_regs.hpp"
#undef X
