#include "combine_rule_defs.hpp"
#include "combine_cndop_and_xor_1.hpp"



/*
jz (
	and (
		xor (
		low (
		shr stkvar_128.4, #1.1
		).4, .-1
		).1, #1.1
	).1, #1.1
).1, #0.1, 15

*/

static bool combine_cndop_and_xor_1_impl(mblock_t* block, minsn_t* insn) {

	mcode_t op = insn->opcode;

	if (op != m_jnz && op != m_jz && op != m_setz && op != m_setnz) {
		return false;
	}

	if (!insn->r.is_equal_to(0ULL, false))
		return false;

	mop_t* inner_andop = &insn->l;

	if (inner_andop->t != mop_d)
		return false;

	minsn_t* innerand = inner_andop->d;

	if (innerand->opcode != m_and)
		return false;

	mop_t* non_xorop;
	mop_t* xorop = innerand->get_either_insnop(m_xor,&non_xorop);
	if (!xorop || !non_xorop->is_equal_to(1ULL, false))
		return false;

	minsn_t* xorinsn = xorop->d;
	mop_t* nonxorconst;
	mop_t* xorconst = xorinsn->get_eitherlr(mop_n,&nonxorconst);

	if (!xorconst || !xorconst->is_equal_to(1ULL, false))
		return false;

	insn->opcode = negate_mcode_relation(op);

	mop_t temp = *nonxorconst;
	*xorop = temp;
	return true;

}

bool combine_cndop_and_xor_1_t::run_combine(mcombine_t* state) {
	return combine_cndop_and_xor_1_impl(state->block(), state->insn());
}

const char* combine_cndop_and_xor_1_t::name() const {
	return "Combine conditional op ANDXOR sequence";
}

combine_cndop_and_xor_1_t combine_cndop_and_xor_1{};