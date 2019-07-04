#pragma once
bool rewrite_xmmops_with_avx(codegen_ex_t& cdg);

class avx_floatops_generator_t : public hexext_micro_filter_t {
public:
	virtual bool match(codegen_ex_t& cdg);

	virtual int apply(codegen_ex_t& cdg);
};

extern avx_floatops_generator_t g_floatops_generator;