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
/*

 shl (
	 xdu (
		setl (ldx cs.2, (add R0.4, #8.4).4).4, #0.4).1, .-1
	 ).4, #31.1
 ).4
*/
COMB_RULE_IMPLEMENT(combine_ltzero_result_shifted_to_highbit) {
	/*
	| ((*(_DWORD *)(v28 + 8) < 0) << 31);
	*/

	auto i = state->insn();

	
	mulp2_const_t mul_extraction{};

	if (!try_extract_mulp2_by_const(i, &mul_extraction))
		return false;
	/*
		make sure the mul/shift is placing the lowbit in the highbit pos
	*/
	if ((1ULL << mul_extraction.shiftcount()) != highbit_for_size(mul_extraction.mutable_term()->size))
		return false;

	auto mut = mul_extraction.mutable_term();
	auto [xd, opnd] = mut->descend_to_unary_insn(m_xdu, mop_d);

	if (!xd)
		return false;

	auto [setl, zero, nonconst] = opnd->descend_to_binary_insn(m_setl, mop_n);

	if (!setl || !zero->is_equal_to(0ULL, false))
		return false;

	if (nonconst->size != mut->size)
		return false;

	/*
		reuse shift/mul opnd for operand locator info
	*/
	mop_t tester = *mul_extraction.const_term();
	tester.nnn->update_value(highbit_for_size(mul_extraction.mutable_term()->size));


	mop_t testee = *nonconst;
	mop_t dst = i->d;

	setup_bitand(i, &testee, &tester);
	i->d = dst;


	return true;
}