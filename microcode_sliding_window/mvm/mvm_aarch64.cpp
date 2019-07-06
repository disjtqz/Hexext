#include "mvm_defs_internal.hpp"
#ifdef __EA64__


#include "mvm_aarch64.hpp"
#include "../mixins/set_codegen_small.mix"
/*
	it looks like there are FOUR DIFFERENT FUCKING POSSIBLE REGISTER FILE LAYOUTS FOR AARCH64
	in every layout the tempregs have different predecessors in the file

	but off, the last tempreg, always comes before q0
*/
void mvm_aarch64_init() {
	using namespace _mvm_internal;

	auto q0 = find_mreg_by_name("q0");

	/*
		t0,t1,t2,off
	*/


	/*
		in two cases, cs comes after q31

	*/
	if (find_mreg_by_name("q31")->m_micro_reg == 832) {
		insert_abs_mreg("cs", 848, 2);
		mr_code_seg = 848;
		mr_data_seg = 848;
	}
	else {
		warning("We don't know the offset of CS in the mreg register file!");
		cs_assert(false);
	}

	unsigned base = q0->m_micro_reg - (16 * 4);


	insert_abs_mreg("t0", base, 16);

	insert_abs_mreg("t1", base + 16, 16);
	insert_abs_mreg("t2", base + 32, 16);
	insert_abs_mreg("off", base + 48, 16);

	add_tempregs_to_list("t0", "t1", "t2",  "off");

	//insert_abs_mreg("cs", find_mreg_by_name("pc")->m_micro_reg + sizeof(ea_t), 2);
}
#include "../mixins/revert_codegen.mix"
#endif