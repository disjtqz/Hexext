#pragma once


class combine_or_shift_t : public mcombiner_rule_t {
public:
	virtual bool run_combine(mcombine_t*);
	virtual const char* name() const;
};

extern combine_or_shift_t combine_or_shift;