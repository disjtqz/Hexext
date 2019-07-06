#include "combine_rule_defs.hpp"
#include "simd_bitops_simplify.hpp"

bool simd_ld_shrtrim_t::run_combine(mcombine_t* state) {

	minsn_t* insn = state->insn();

	if (insn->op() != m_low || insn->d.size != 8 || insn->l.t != mop_d || insn->l.size != 16)
		return false;

	auto intrin = insn->l.d;

	auto role = get_instruction_exrole(intrin);

	if (role != exrole_t::byteshr128)
		return false;

	auto left128 = &intrin->d.f->args[0];
	auto shrcount = &intrin->d.f->args[1];


	if (!shrcount->is_equal_to(8ULL, false))
		return false;
	mop_t copyop;
	copyop = *left128;

	insn->opcode = m_high;



	insn->l = copyop;

	return true;


}

/*
mov (call _mm_xor_ps, .-1, (xmm6.16,(xdu xmmword_1800E51C0.8, .-1).16)).16, .-1, xmm6.16
*/
bool detect_xdu_in_xor128_t::run_combine(mcombine_t* state) {

	auto i = state->insn();
	if (hexext::current_topinsn() != i)
		return false;

	if (i->opcode != m_mov)
		return false;
	if (i->d.t != mop_r || !is_simdreg(i->d.r) || i->l.t != mop_d)
		return false;


	auto [x, y] = extract_binary_exrole(i->l.d, exrole_t::xor128);

	if (!x)
		return false;

	auto [xdu, nonxdu] = order_mops(mop_d, x, y);


	if (!xdu) {
		return false;
	}

	auto [xdui, xduop] = xdu->descend_to_unary_insn(m_xdu);

	if (!xdui)
		return false;
	if (xdui->d.size == 16 && xduop->size == 8 && nonxdu->t == mop_r && nonxdu->r == i->d.r) {

		mop_t op1 = *xduop;
		mop_t op2 = *nonxdu;


		i->opcode = m_xor;
		i->l = op1;
		i->l.size = 8;
		i->r = op2;
		i->r.size = 8;
		i->d.size = 8;
		return true;


	}
	return false;



}


simd_ld_shrtrim_t simd_ld_shrtrim{};

detect_xdu_in_xor128_t detect_xdu_in_xor128{};