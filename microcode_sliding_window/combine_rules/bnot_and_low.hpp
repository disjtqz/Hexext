#pragma once




class combine_bnot_and_one_t : public mcombiner_rule_t {
public:

	virtual bool run_combine(mcombine_t* state);

	virtual const char* name() const override;
};

class combine_jzf_and_bnot_t : public mcombiner_rule_t {
public:
	virtual bool run_combine(mcombine_t* state);
	virtual const char* name() const override;
};

extern combine_bnot_and_one_t combine_bnot_and_one;

extern combine_jzf_and_bnot_t combine_jzf_and_bnot;