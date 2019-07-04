#pragma once


//bool combine_divmul_of_lvalue_in_conditional_to_modulus_check(mblock_t* block, minsn_t* insn);

class div_and_mul_in_conditional_to_modulus_test_t : public mcombiner_rule_t {
public:
	virtual bool run_combine(mcombine_t* state);
	virtual const char* name() const override;
};

extern div_and_mul_in_conditional_to_modulus_test_t div_and_mul_in_conditional_to_modulus_test;