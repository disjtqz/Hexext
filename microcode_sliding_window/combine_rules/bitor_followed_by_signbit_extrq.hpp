#pragma once

bool combine_signbit_shift_and_bitop(mblock_t* blk, minsn_t* insn);
/*
class combine_sets_result_shifted_to_highbit_t : public mcombiner_rule_t {
public:
	COMB_RULE_NAME("Combine Sets result shifted to highbit");

	COMB_RULE_RUN();
};*/

COMB_RULE_DECL(combine_ltzero_result_shifted_to_highbit, "Combine Sets result shifted to highbit");