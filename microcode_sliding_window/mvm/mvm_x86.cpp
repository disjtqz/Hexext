#include "mvm_defs_internal.hpp"
#ifndef __EA64__
#include "mvm_x86.hpp"

#define X(name)		mreg_info_t* mri_x86_##name = nullptr;
#include "xmacro_x86_mvm_regs.hpp"
#undef X

void mvm_x86_init() {

	using namespace _mvm_internal;

	auto fps = 42;

	/*
		the tempregs are weird af on x86
	*/

	constexpr unsigned T0_OFFSET = 0,
		T1_OFFSET = T0_OFFSET + 12,
		T2_OFFSET = T1_OFFSET + 12,
		TT_OFFSET = T2_OFFSET + 12,
		OFF_OFFSET = TT_OFFSET + 14, //why???
		SEG_OFFSET = OFF_OFFSET + 4;

	insert_abs_mreg("t0", fps + T0_OFFSET, 12);
	insert_abs_mreg("t1", fps + T1_OFFSET, 12);
	insert_abs_mreg("t2", fps + T2_OFFSET, 12);
	insert_abs_mreg("tt", fps + TT_OFFSET, 14);
	insert_abs_mreg("off", fps + OFF_OFFSET, 4);
	insert_abs_mreg("seg", fps + SEG_OFFSET, 2);

	mr_code_seg = find_mreg_by_name("cs")->m_micro_reg;
	mr_data_seg = find_mreg_by_name("ds")->m_micro_reg;

	add_tempregs_to_list("t0", "t1", "t2", "tt", "off", "seg");

#define X(name)		mri_x86_##name = find_mreg_by_name(#name);
#include "xmacro_x86_mvm_regs.hpp"
#undef X

}

#endif