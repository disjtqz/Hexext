#include "mvm_defs_internal.hpp"
#ifndef __EA64__
#include "mvm_arm32.hpp"


/*
	t0 seems to come after fnf

	t0 - t1 - t2 - off

	pc and cs come after lr
*/
void mvm_arm32_init() {
	using namespace _mvm_internal;

	auto lr = find_mreg_by_name("lr")->m_micro_reg;

	insert_abs_mreg("pc", lr + 4, 4);
	insert_abs_mreg("cs", lr + 8, 2);

	mr_code_seg = lr + 8;
	mr_data_seg = lr + 8;


	insert_abs_mreg("t0", 80, 16);

	insert_abs_mreg("t1", 96, 16);
	insert_abs_mreg("t2", 112, 16);
	insert_abs_mreg("off", 128, 16);
	add_tempregs_to_list("t0", "t1", "t2", "off");
}
#endif