#pragma once
/*
jz (and (xor (low (shr stkvar_128.4, #1.1).4, .-1).1, #1.1).1, #1.1).1, #0.1, 15

*/

//bool combine_cndop_and_xor_1(mblock_t* block, minsn_t* insn);

class combine_cndop_and_xor_1_t : public mcombiner_rule_t {
public:
	virtual bool run_combine(mcombine_t* state);
	virtual const char* name() const override;
};

extern combine_cndop_and_xor_1_t combine_cndop_and_xor_1;