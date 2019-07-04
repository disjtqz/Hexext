#include "combine_rule_defs.hpp"
#include "bytewise_shift.hpp"
//(v2 << 24 >> 31)
//(shr (shl rax.4, #24.1).4, #31.1).4,
bool combine_bytewise_shift(mblock_t* blk, minsn_t* insn) {

	if (insn->opcode != m_shr) {
		return false;
	}


	if (insn->l.t != mop_d || insn->r.t != mop_n) {
		return false;
	}
	mop_t* non_const = &insn->l;

	mop_t* num = &insn->r;






	minsn_t* shlins = non_const->d;
	uint64_t shlval;
	
	mop_t* constvalptr;
	
	if (shlins->opcode != m_shl) {
		if (shlins->opcode == m_mul) {

			mop_t* constval = shlins->get_eitherlr(mop_n);

			if (!constval)
				return false;

			constvalptr = constval;
			uint64_t v = constval->nnn->value;

			if (__popcnt64(v) == 1) {
				shlval = _tzcnt_u64(v);
			}
			else {
				return false;
			}
		}
		else
			return false;
	}
	else {
		if (shlins->r.t != mop_n)
			return false;
		constvalptr = &shlins->r;

		shlval = shlins->r.nnn->value;
	}





	unsigned shlsize = non_const->size;

	if (shlval != ((shlsize * 8) - 8))
		return false;

	uint64_t shrval = num->nnn->value;

	if (shrval < shlval)
		return false;


	uint64_t diff = shrval - shlval;


	shlins->opcode = m_and;

	constvalptr->nnn->value = 0xFFULL;

	shlins->r.size = shlins->l.size;


	num->nnn->value = diff;


	return true;



	



}
//xor (shl rax.4, #25.1).4, (and (neg (shr (xdu rax.1, .-1).4, #31.1).4, .-1).4, #79764919.4).4, rcx.4
bool combine_sign_shift_neg(mblock_t* blk, minsn_t* insn) {

	if (insn->opcode != m_neg) {
		return false;

	}


	
	mop_t* innerins = insn->get_eitherlr(mop_d);

	if (!innerins)
		return false;



	minsn_t* inner = innerins->d;

	if (inner->opcode != m_shr) {
		return false;
	}

	mop_t* shrconst;

	mop_t* shrins = inner->get_eitherlr(mop_d, &shrconst);

	if (!shrins || shrconst->t != mop_n)
		return false;


	if (shrins->d->opcode != m_xdu)
		return false;


	mop_t* xduparm = &shrins->d->l;


	if ((uint64_t)((xduparm->size * 8) - 1) != shrconst->nnn->value)
		return false;


	shrins->d->opcode = m_xds;

	inner->opcode = m_sar;


	insn->opcode = m_mov;

	return true;
}