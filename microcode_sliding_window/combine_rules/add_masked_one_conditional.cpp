
#include "combine_rule_defs.hpp"
#include "add_masked_one_conditional.hpp"

// !(((_BYTE)v4 + (_BYTE)v5) & 1) 

//jnz (and (add rcx.1, r8.1).1, #1.1).1, #0.1, 4
bool combine_add_masked_one_conditional(mblock_t* block, minsn_t* insn) {
	mcode_t op = insn->opcode;

	if (op != m_setnz && op != m_setz && op != m_jnz && op != m_jz)
		return false;


	if (insn->l.t != mop_d || !insn->r.is_equal_to(0, false)) {
		return false;
	}

	minsn_t* innerins = insn->l.d;


	if (innerins->opcode != m_and)
		return false;

	mop_t* constmask;
	mop_t* inner2 = innerins->get_eitherlr(mop_d, &constmask);

	if (!inner2 || constmask->t != mop_n || __popcnt64(constmask->nnn->value) != 1)
		return false;

	
	minsn_t* innermost = inner2->d;


	if (innermost->opcode != m_add)
		return false;


	mop_t* constieboi = innermost->get_eitherlr(mop_n);

	if (!constieboi || constieboi->nnn->value != constmask->nnn->value) {
		return false;
	}

	innermost->opcode = m_xor;
	return true;

}