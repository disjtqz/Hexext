#include "combine_rule_defs.hpp"
#include "div_and_mul_by_same_const_to_mask.hpp"
/*
jnz rsi.4, (mul #3.4, (udiv rsi.4, #3.4).4).4, 15

mul #3.4, (udiv rsi.4, #3.4).4

*/
static bool combine_divmul_of_lvalue_in_conditional_to_modulus_check(mblock_t* block, minsn_t* insn) {

	if (!is_mcode_zf_cond(insn->opcode))
		return false;

	mop_t* nonlval;
	mop_t* lvalue_comp = insn->get_either_lvalue(&nonlval);

	if (!lvalue_comp || nonlval->t != mop_d) {
		return false;
	}



	minsn_t* insn_comp_term = nonlval->d;
	

	mul_by_constant_extraction_t constmul{};
	if (!try_extract_mul_by_constant(insn_comp_term, &constmul)) {
		return false;
	}

	udiv_by_constant_extraction_t constudiv{};
	mop_t* mutable_insn = constmul.mutable_term();
	if (mutable_insn->t != mop_d)
		return false;

	if (!try_extract_udiv_by_constant(mutable_insn->d, &constudiv))
		return false;

	if (lvalue_comp->lvalue_equal_ignore_size(constudiv.mutable_term())
		&& constudiv.divisor() == constmul.multiplier()) {

		//( ((x/y)*y) (!=|==) x)


	}
	else {
		return false;
	}
	/*
		transform into (value % (constant)  == 0) or (value % (constant) != 0) 
	*/

	mutable_insn->d = nullptr;
	mutable_insn->t = mop_z;
	minsn_t* modinsn = constudiv.divinsn();
	modinsn->opcode = m_umod;

	modinsn->r.nnn->value = constudiv.divisor();

	modinsn->r.size = modinsn->l.size;

	lvalue_comp->erase();
	lvalue_comp->t = mop_n;
	lvalue_comp->size = modinsn->r.size; lvalue_comp->nnn = new mnumber_t(0ULL);

	delete nonlval->d;

	nonlval->d = modinsn;

	return true;


}

bool div_and_mul_in_conditional_to_modulus_test_t::run_combine(mcombine_t* state) {
	return combine_divmul_of_lvalue_in_conditional_to_modulus_check(state->block(), state->insn());
}

const char* div_and_mul_in_conditional_to_modulus_test_t::name() const {
	return "Div and mul in conditional to modulus test";
}

div_and_mul_in_conditional_to_modulus_test_t div_and_mul_in_conditional_to_modulus_test{};