#include "microgen_defs.hpp"
#include "under_debugger_elim.hpp"

static bool is_x86_condbranch(int itype) {

	switch (itype) {
	case NN_ja:                  // Jump if Above (CF=0 & ZF=0)
	case NN_jae:                 // Jump if Above or Equal (CF=0)
	case NN_jb:                  // Jump if Below (CF=1)
	case NN_jbe:                 // Jump if Below or Equal (CF=1 | ZF=1)
	case NN_jc:                  // Jump if Carry (CF=1)
	case NN_je:                  // Jump if Equal (ZF=1)
	case NN_jg:                  // Jump if Greater (ZF=0 & SF=OF)
	case NN_jge:                 // Jump if Greater or Equal (SF=OF)
	case NN_jl:                  // Jump if Less (SF!=OF)
	case NN_jle:                 // Jump if Less or Equal (ZF=1 | SF!=OF)
	case NN_jna:                 // Jump if Not Above (CF=1 | ZF=1)
	case NN_jnae:                // Jump if Not Above or Equal (CF=1)
	case NN_jnb:                 // Jump if Not Below (CF=0)
	case NN_jnbe:                // Jump if Not Below or Equal (CF=0 & ZF=0)
	case NN_jnc:                 // Jump if Not Carry (CF=0)
	case NN_jne:                 // Jump if Not Equal (ZF=0)
	case NN_jng:                 // Jump if Not Greater (ZF=1 | SF!=OF)
	case NN_jnge:                // Jump if Not Greater or Equal (ZF=1)
	case NN_jnl:                 // Jump if Not Less (SF=OF)
	case NN_jnle:                // Jump if Not Less or Equal (ZF=0 & SF=OF)
	case NN_jno:                 // Jump if Not Overflow (OF=0)
	case NN_jnp:                 // Jump if Not Parity (PF=0)
	case NN_jns:                 // Jump if Not Sign (SF=0)
	case NN_jnz:                 // Jump if Not Zero (ZF=0)
	case NN_jo:                  // Jump if Overflow (OF=1)
	case NN_jp:                  // Jump if Parity (PF=1)
	case NN_jpe:                 // Jump if Parity Even (PF=1)
	case NN_jpo:                 // Jump if Parity Odd  (PF=0)
	case NN_js:                  // Jump if Sign (SF=1)
	case NN_jz:                  // Jump if Zero (ZF=1)
		return true;
	default:
		return false;

	}
}

static bool is_mov_under_debugger_at(ea_t ea) {

	insn_t target{};



	if (decode_insn(&target, ea) == 0)
		return false;
	if (target.itype == NN_mov) {
		qstring nam = get_name(target.ops[1].addr);

		if (nam.size() && !strcmp(nam.c_str(), "under_debugger")) {

			return true;
		}
	}
	return false;
}


bool remove_under_debugger_interrs(insn_t& in) {


	if (is_x86_condbranch(in.itype)) {

		if (is_mov_under_debugger_at(in.ops[0].addr)) {
			in.itype = NN_nop;
			return true;
		}
		else {
			if (is_mov_under_debugger_at(in.ea + in.size)) {
				in.itype = NN_jmp;
				return true;
			}

		}
	}
	return false;

}