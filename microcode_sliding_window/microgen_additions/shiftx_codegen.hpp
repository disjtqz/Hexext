#pragma once

class shiftx_generator_t : public hexext_micro_filter_t {
public:
	virtual bool match(codegen_ex_t& cdg);

	virtual int apply(codegen_ex_t& cdg);
};
extern shiftx_generator_t shiftx_generator;