#pragma once

class microfilter_fixup_bittest_t : public hexext_micro_filter_t {
public:
	virtual bool match(codegen_ex_t& cdg);

	virtual int apply(codegen_ex_t& cdg);
};

extern microfilter_fixup_bittest_t bittest_fixup;