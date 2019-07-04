#pragma once

/*bool combine_x86_64_bitand_high(mblock_t* block, minsn_t* insn);

bool combine_x86_64_bitor_high(mblock_t* block, minsn_t* insn);
*/
class combine_x86_band_high_t : public mcombiner_rule_t {
public:
	virtual bool run_combine(mcombine_t* state);

	virtual const char* name() const override;
};

class combine_x86_bitor_high_t : public mcombiner_rule_t {
public:
	virtual bool run_combine(mcombine_t* state);

	virtual const char* name() const override;
};

extern combine_x86_band_high_t combine_x86_band_high;
extern combine_x86_bitor_high_t combine_x86_bitor_high;