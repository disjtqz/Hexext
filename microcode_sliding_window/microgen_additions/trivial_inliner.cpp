#include "microgen_defs.hpp"
#include "trivial_inliner.hpp"

static bool is_insn_okay_to_inline(insn_t& insn) {
	if (is_basic_block_end(insn, true))
		return false;
	auto itype = insn.itype;
	/*
		hexrays will create a fake block and a jcnd and branch to the next insn ea for these
		we want to avoid the cmovs :(
	*/
	if (itype >= NN_cmova && itype <= NN_fcmovnu)
		return false;
	switch (insn.itype) {
	case NN_pop:                 // Pop a word from the Stack
	case NN_popaw:               // Pop all General Registers
	case NN_popa:                // Pop all General Registers
	case NN_popad:               // Pop all General Registers (use32)
	case NN_popaq:               // Pop all General Registers (use64)
	case NN_popfw:               // Pop Stack into Flags Register
	case NN_popf:                // Pop Stack into Flags Register
	case NN_popfd:               // Pop Stack into Eflags Register
	case NN_popfq:               // Pop Stack into Rflags Register
	case NN_push:                // Push Operand onto the Stack
	case NN_pushaw:              // Push all General Registers
	case NN_pusha:               // Push all General Registers
	case NN_pushad:              // Push all General Registers (use32)
	case NN_pushaq:              // Push all General Registers (use64)
	case NN_pushfw:              // Push Flags Register onto the Stack
	case NN_pushf:               // Push Flags Register onto the Stack
	case NN_pushfd:              // Push Flags Register onto the Stack (use32)
	case NN_pushfq:              // Push Flags Register onto the Stack 
	case NN_rep:
	case NN_repe:
	case NN_repne:
		return false;

	default:
		break;
	}
	return true;
}


static bool is_callea_inlinable(ea_t callea) {
	if (is_noret(callea))
		return false;

	func_t* fn = get_func(callea);
	if (!fn)
		return false;
	//limit inlining
	if (fn->end_ea - fn->start_ea > 64)
		return false;

	if (!fn || callea != fn->start_ea)
		return false;
	insn_t reti{};
	hexext::microgen_decode_prev_insn(&reti, fn->end_ea);
	if (reti.size <= 0 || !is_ret_insn(reti, true))
		return false;
	insn_t currinsn{};

	if (!hexext::microgen_decode_insn(&currinsn, fn->start_ea)) {
		return false;
	}

	while (!is_ret_insn(currinsn, true)) {
		if (!is_insn_okay_to_inline(currinsn))
			return false;

		if (!hexext::microgen_decode_insn(&currinsn, currinsn.ea + currinsn.size))
			return false;
	}
	return true;
}

bool trivial_inliner_x64_t::match(codegen_ex_t& cdg) {
	if (cdg.insn.itype != NN_call || cdg.insn.ops[0].type != o_near)
		return false;

	return is_callea_inlinable(cdg.insn.ops[0].addr);

}

int trivial_inliner_x64_t::apply(codegen_ex_t& cdg) {

	//msg("Inlining a bad boi.\n");
	/*
		we're going to do a realllll wonky hack here
		but it works, trust me
	*/
	insn_t backup_insn = cdg.insn;

	ea_t inline_ea = cdg.insn.ops[0].addr;
	ea_t inline_location_ea = cdg.insn.ea;

	//ehehehehe
	//we're guaranteed no recursion into this filter because we pretest for calls in the inlinee

	hexext::microgen_decode_insn(&cdg.insn, inline_ea);
	//143

	ea_t decode_ea = cdg.insn.ea+cdg.insn.size;
		//NN_retf
	while (!is_ret_insn(cdg.insn, true)) {
		
		cdg.insn.ea = inline_location_ea;
		int merrno = cdg.gen_micro();

		if (merrno != MERR_OK) {
			//we're fucked
			//so fuckt
			//fuckt like a giraffe (they probably fuck crazy)
			return merrno;
		}

		
		hexext::microgen_decode_insn(&cdg.insn, decode_ea);

		decode_ea = cdg.insn.ea + cdg.insn.size;
	}


	//insn_t currinsn{};


	//might not be necessary but best to not chance it
	cdg.insn = backup_insn;
	return MERR_OK;

}

trivial_inliner_x64_t trivial_inliner_x86_64{};

bool operation_then_ret_resolver_x86_64(insn_t& insn) {
	if (!is_call_insn(insn))
		return false;
	ea_t callea = insn.ops[0].addr;

	if (insn.ops[0].type != o_near)
		return false;

	func_t* fn = get_func(callea);
	if (!fn)
		return false;

	if (!fn || callea != fn->start_ea)
		return false;
	insn_t reti{};
	if (hexext::microgen_decode_prev_insn(&reti, fn->end_ea) == BADADDR)
		return false;

	if (reti.size <= 0 || !is_ret_insn(reti, true))
		return false;

	insn_t firstinsn{};

	if (hexext::microgen_decode_insn(&firstinsn, fn->start_ea) <= 0)
		return false;

	if (firstinsn.ea + firstinsn.size != reti.ea)
		return false;
	if (!is_insn_okay_to_inline(firstinsn))
		return false;

	ea_t backup_ea = insn.ea;
	insn = firstinsn;
	insn.ea = backup_ea;
	return true;
}

bool operation_then_ret_resolver_arm32(insn_t& insn) {
	/*
		inlining is more of a bitch on arm than x86
	*/
	return false;
}