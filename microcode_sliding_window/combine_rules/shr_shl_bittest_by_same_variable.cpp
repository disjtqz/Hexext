#include "combine_rule_defs.hpp"
#include "shr_shl_bittest_by_same_variable.hpp"


/*
shr(
	and r10.8, (
		shl r13.8, rbx.1
		).8
	).8, 
rbx.1, rax.8
*/
bool combine_shr_shl_bittest_by_same_variable(mblock_t* block, minsn_t* insn) {

	if (insn->opcode != m_shr)
		return false;

	mop_t* non_andop=&insn->r;

	mop_t* and_insnop = &insn->l;


	if (and_insnop->t != mop_d || and_insnop->d->opcode != m_and)
		return false;


	minsn_t* andinsn = and_insnop->d;


	mop_t* non_shlop;
	mop_t* shl_insnop = andinsn->get_either_insnop(m_shl, &non_shlop);

	if (!shl_insnop)
		return false;
	minsn_t* shlinsn = shl_insnop->d;


	if (!non_andop->lvalue_equal_ignore_size(&shlinsn->r)) {
		return false;
	}

	/*
		we want (v5 & (unsigned __int64)(v12 << v16)) >> v16
		to become

		(v5 >> v16) & v12
	*/

	insn->opcode = m_and;

	mop_t shifted{};
	mop_t shifter{};
	mop_t mask{};

	shifter = *non_andop;
	shifted = *non_shlop;
	mask = shlinsn->l;
	*non_andop = mask;

	andinsn->opcode = m_shr;
	andinsn->l = shifted;
	andinsn->r = shifter;
	andinsn->r.size = 1;

	return true;


}