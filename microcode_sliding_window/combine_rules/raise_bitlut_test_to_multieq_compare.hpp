#pragma once
bool raise_bitlut_test_to_multieq(mblock_t* blk, minsn_t* insn);

bool shl_and_low(mblock_t* blk, minsn_t* insn);

class raise_bitlut_multieq_t : public mcombiner_rule_t {
public:
	virtual bool run_combine(mcombine_t*);
	virtual const char* name() const;
};

extern raise_bitlut_multieq_t raise_bitlut_multieq;