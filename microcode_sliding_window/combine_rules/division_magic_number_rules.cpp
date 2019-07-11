#include "combine_rule_defs.hpp"
#include "division_magic_number_rules.hpp"


static uint64_t emulate_mul128_shift(uint64_t multiplier, unsigned shrcount, uint64_t input_value) {
	uint64_t highprod;

#ifdef __AVX2__
	_mulx_u64(multiplier, input_value, &highprod);
#else
	_mul128(multiplier, input_value, (long long*)&highprod);
#endif
	return highprod >> shrcount;
}

/*
shr (
high (
mul (xdu #16397105843297379215.8, .-1).16, (xdu (ldx ds.2, (add rdi.8, #80.8).8).4, .-1).16
).16
).8, #3.1
*/

COMB_RULE_IMPLEMENT(division_magic_num_rule_1) {
	auto insn = state->insn();
	if (insn->opcode != m_shr)
		return false;
	/*mop_t* nonconst;
	mop_t* constoper = insn->get_eitherlr(mop_n, &nonconst);
	*/

	auto [constoper, nonconst] = insn->arrange_by(mop_n);

	if (!constoper)
		return false;

	mop_t* highguy;

	if (!try_extract_high(nonconst, &highguy))
		return false;

	if (highguy->size != 16) {
		return false;
	}

	if (highguy->t != mop_d)
		return false;

	minsn_t* innermul = highguy->d;

	if (innermul->opcode != m_mul)
		return false;

	mop_t* xdu_const;
	mop_t* non_xdu_const;


	xdu_extraction_t extracted;

	if (try_extract_const_xdu(&innermul->l, &extracted)) {
		non_xdu_const = &innermul->r;
	}
	else {
		if (try_extract_const_xdu(&innermul->r, &extracted)) {
			non_xdu_const = &innermul->l;
		}
		else {
			return false;
		}

	}

	xdu_const = extracted.xdu_operand();

	xdu_extraction_t extracted_nonconst{};

	if (!try_extract_xdu(non_xdu_const, &extracted_nonconst)) {
		return false;
	}


	unsigned sim_shift = constoper->nnn->value;
	uint64_t multiplier = xdu_const->nnn->value;
	uint64_t numerator_one = 0;
	/*
		todo: do a binary search instead
	*/
	for (; ; ++numerator_one) {
		if (emulate_mul128_shift(multiplier, sim_shift, numerator_one)) {
			break;
		}
		if (numerator_one == 4096) {
			return false;
		}
	}
	unsigned parent_size = hexext::get_parent_mop_size();
	insn->opcode = m_udiv;


	mop_t tempmop;

	if (extracted_nonconst.m_fromsize == parent_size)
		tempmop = *extracted_nonconst.xdu_operand();

	else {
		tempmop = *extracted_nonconst.m_target;
		tempmop.size = parent_size;
	}


	constoper->nnn = new mnumber_t(numerator_one);
	constoper->size = parent_size;



	*nonconst = std::move(tempmop);
	nonconst->size = parent_size;
	return true;

}