#include "microgen_defs.hpp"
#include "thunk_function_resolver.hpp"

bool resolve_thunk_function_address_x86_64(insn_t& insn) {
	if (is_call_insn(insn) && (insn.ops[0].type == o_near)) {
		func_t* fn = get_func(insn.ops[0].addr);
		if (!fn || !(fn->flags& FUNC_THUNK))
			return false;
		
		ea_t ea = BADADDR;
		ea_t calcea = calc_thunk_func_target(fn, &ea);
		if (calcea == BADADDR)
			return false;

		insn.ops[0].addr = ea == BADADDR ? calcea : ea;

		if (ea != BADADDR) {
			insn.ops[0].type = o_mem;
			insn.itype = 18;
			insn.ops[0].dtype = dt_qword;
		}
		return true;
	}
	else if (is_call_insn(insn) && insn.ops[0].type == o_mem) {
		tinfo_t item_typ{};
		if (!get_type(insn.ops[0].addr, &item_typ, GUESSED_NONE))
			return false;
		if (item_typ.is_const() && item_typ.is_funcptr()) {

			ea_t realea;

#ifdef __EA64__
			realea = get_qword(insn.ops[0].addr);
#else
			realea = get_dword(insn.ops[0].addr);
#endif

			insn.ops[0].type = o_near;
			insn.ops[0].addr = realea;
			insn.itype = 16;
			return true;

		}
	}
	return false;
}

bool resolve_thunk_function_address_arm32(insn_t& insn) {
	if (is_call_insn(insn) &&  (insn.ops[0].type == o_near)) {
		func_t* fn = get_func(insn.ops[0].addr);
		if (!fn || !(fn->flags & FUNC_THUNK))
			return false;

		ea_t ea = BADADDR;
		ea_t calcea = calc_thunk_func_target(fn, &ea);
		if (calcea == BADADDR)
			return false;

		
		/*
			we cant do direct calls to function pointers on arm :/
			oh shit nope we actually CAN
			oh shit nope we cant because it looks nobueno

			this works really well though
		*/
	

		insn.ops[0].addr = calcea;//ea == BADADDR? calcea : ea;
		//if (ea != BADADDR) {

			insn.itype = ARM_blx1;
			insn.ops[0].type = o_near;
			insn.ops[0].dtype = dt_dword;
		//}
		return true;
	}
	/*else if (is_call_insn(insn) && insn.ops[0].type == o_mem) {
		tinfo_t item_typ{};
		if (!get_type(insn.ops[0].addr, &item_typ, GUESSED_NONE))
			return false;
		if (item_typ.is_const() && item_typ.is_funcptr()) {

			ea_t realea;


			realea = get_dword(insn.ops[0].addr);

			insn.ops[0].type = o_near;
			insn.ops[0].addr = realea;
			insn.itype = ARM_blx1;
			return true;

		}
	}*/
	return false;
}