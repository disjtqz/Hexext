#pragma once

class tzcnt_generator_t : public hexext_micro_filter_t {
public:
	virtual bool match(codegen_ex_t& cdg);

	virtual int apply(codegen_ex_t& cdg);
};

class blsr_generator_t : public hexext_micro_filter_t {
public:
	virtual bool match(codegen_ex_t& cdg);

	virtual int apply(codegen_ex_t& cdg);
};

extern blsr_generator_t blsr_generator;
extern tzcnt_generator_t tzcnt_generator;