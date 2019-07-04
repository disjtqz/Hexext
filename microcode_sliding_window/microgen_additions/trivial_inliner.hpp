#pragma once

class trivial_inliner_x64_t : public hexext_micro_filter_t {
public:
	virtual bool match(codegen_ex_t& cdg);

	virtual int apply(codegen_ex_t& cdg);
};

extern trivial_inliner_x64_t trivial_inliner_x86_64;
/*
	asm rewriter that takes a function that does
	INSN
	retn
	and replaces it with INSN
*/
bool operation_then_ret_resolver_x86_64(insn_t& insn);

bool operation_then_ret_resolver_arm32(insn_t& insn);

