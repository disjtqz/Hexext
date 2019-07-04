#include "mvm_defs_internal.hpp"
#ifdef __EA64__
#include "mvm_x86_64.hpp"

#define X(name)		mreg_info_t* mri_x86_##name = nullptr;
#include "xmacro_x86_64_mvm_regs.hpp"
#undef X
/*
X(t0, 144, 16)
X(t1, 160, 16)
X(t2, 176, 16)
X(tt, 192, 16)
X(off, 208, 8)
X(seg, 216, 8)
*/
void mvm_x86_64_init() {
	using namespace _mvm_internal;


	insert_abs_mreg("t0", 144, 16);
	insert_abs_mreg("t1", 160, 16);
	insert_abs_mreg("t2", 176, 16);
	insert_abs_mreg("tt", 192, 16);
	insert_abs_mreg("off", 208, 8);
	insert_abs_mreg("seg", 216, 8);

	mr_code_seg = find_mreg_by_name("cs")->m_micro_reg;
	mr_data_seg = find_mreg_by_name("ds")->m_micro_reg;

	add_tempregs_to_list("t0", "t1", "t2", "tt", "off", "seg");


#define X(name)		mri_x86_##name = find_mreg_by_name(#name);
#include "xmacro_x86_64_mvm_regs.hpp"
#undef X
}

#endif