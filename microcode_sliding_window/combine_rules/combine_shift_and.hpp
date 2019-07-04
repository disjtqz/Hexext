#pragma once

class combine_shift_and_t : public mcombiner_rule_t {

public:
	virtual bool run_combine(mcombine_t*);
	virtual const char* name() const;
	virtual const char* description() const;
};
extern combine_shift_and_t combine_shift_and_rule;
//bool combine_shift_and(mblock_t* blk, minsn_t* insn_);