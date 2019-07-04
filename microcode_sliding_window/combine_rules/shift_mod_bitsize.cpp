#include "combine_rule_defs.hpp"
#include "shift_mod_bitsize.hpp"

//shl #1.4, (and rbx.1, #31.1).1, r8.4
bool combine_shift_mod_bitsize(mblock_t* blk, minsn_t* insn) {

	if (!is_mcode_shift(insn->opcode))
		return false;

	
	mop_t* shifted = &insn->l;

	mop_t* shifter = &insn->r;



	if (shifter->t != mop_d || shifter->d->opcode != m_and) {
		return false;
	}

	auto shiftins = shifter->d;

	mop_t* non_const;

	mop_t* const_mask_shift = shiftins->get_eitherlr(mop_n, &non_const);

	if (!const_mask_shift)
		return false;
	//get highbit of shifted value
	const uint64_t maxbit = (shifted->size * 8ULL);


	/*
		assume shifts are mod maxbit
	*/

	if (const_mask_shift->nnn->value != (maxbit - 1ULL)) {
		return false;
	}

	shifter->t = non_const->t;

	shifter->d = non_const->d;

	non_const->t = mop_z;

	delete shiftins;

	return true;
	



}