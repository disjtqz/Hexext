#include "combine_rule_defs.hpp"
#include "bitor_followed_by_signbit_extrq.hpp"

//22cddf : shr (or rsi.4, rcx.4).4, #31.1, mreg(1568).4

bool combine_signbit_shift_and_bitop(mblock_t* blk, minsn_t* insn) {
	if (insn->opcode != m_shr)
		return false;

	if (insn->r.t != mop_n) {
		return false;
	}

	if (insn->r.nnn->value != (insn->l.size * 8) - 1)
		return false;

	
	if (insn->l.t != mop_d ) {
		return false;
	}

	
	auto inner = insn->l.d;
	auto innermopcode = insn->l.d->opcode;

	if (innermopcode != m_or && innermopcode != m_and && innermopcode != m_xor) {
		return false;
	}
	
	insn->opcode = innermopcode;

	inner->opcode = m_sets;

	minsn_t* otherinner = new minsn_t(BADADDR);

	otherinner->opcode = m_sets;



	std::swap(inner->r, otherinner->l);


	inner->d.size = 1;

	delete insn->r.nnn;
	insn->r.t = mop_d;
	insn->r.d = otherinner;

	otherinner->d.size = 1;

	
	insn->l.size = 1;
	insn->r.size = 1;
	insn->d.size = 1;

	if (hexext::current_comb_mop() != nullptr) {
		hexext::current_comb_mop()->size = 1;
	}

	//hexext::current_topinsn

	return true;


}