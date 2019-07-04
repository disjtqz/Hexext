#include "microgen_defs.hpp"
#include "remove_nullsub_calls.hpp"
/*
	changed this to be multiarch
*/
bool remove_nullsub_call(insn_t& in) {

	if (is_call_insn(in) && in.ops[0].type == o_near) {
		qstring nam = get_name(in.ops[0].addr);

		if (nam.find("nullsub_", 0) == 0) {
			//i guess aarch64 nop is arm32 nop too?
			if (hexext::currarch() == hexext_arch_e::ARM) {
				in.itype = ARM_nop;
			}
			else {
				in.itype = NN_nop;
			}


			in.ops[0].type = o_void;
			return true;
		}
	}
	return false;

}