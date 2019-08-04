#include "combine_rule_defs.hpp"
#include "combine_x86_64_bitand_high.hpp"

#include "../mvm/mvm.hpp"
/*
c4eca7a : ldx ds.2, (add rbx.8, #8.8).8, rax.4
6c4eca80 : jnz (and rax^1.1, #1.1).1, #0.1, 73
*/
static bool combine_x86_64_bitand_high(mblock_t* block, minsn_t* insn) {

	if (!is_mcode_zf_cond(insn->opcode))
		return false;

	

	mop_t* nonconst;
	mop_t* cmpconst = insn->get_eitherlr(mop_n, &nonconst);

	if (!cmpconst || nonconst->t != mop_d || !cmpconst->is_equal_to(0ULL, false))
		return false;

	minsn_t* andinsn = nonconst->d;

	if (andinsn->opcode != m_and || nonconst->size != 1)
		return false;



	mop_t* andmask;
	mop_t* andreg = andinsn->get_eitherlr(mop_r, &andmask);

	if (!andreg || andmask->t != mop_n)
		return false;

	if (!mreg_has_highbyte_x86(andreg->r - 1))
		return false;

	mlist_t lst{};

	lst.reg.add_(andreg->r - 1, 8);

	minsn_t* definition = find_definition_backwards(block, hexext::current_topinsn(), &lst);

	if (!definition)
		return false;
	unsigned defsize = definition->d.size;
	if (definition->opcode == m_xdu) {
		defsize = definition->l.size;
	}

	






	if (defsize == 1)
		return false;


	andmask->size = defsize;
	
	andmask->nnn->value <<= 8;
	andmask->nnn->org_value <<= 8;
	andreg->r -= 1;
	andreg->size = defsize;
	nonconst->size = defsize;
	cmpconst->size = defsize;
	/*block->mustbuse.reg.add_(andreg->r, andreg->size);
	block->maybuse.reg.add_(andreg->r, andreg->size);
	block->flags &= ~MBL_LIST;
	
	block->flags |= /*MBL_PROP | MBL_INCONST;*/


	return true;
}

static bool combine_x86_64_bitor_high(mblock_t* block, minsn_t* insn) {

}

/*
	disabled. this one changes the blocks use lists so bad idea for now hombre
*/

bool combine_x86_band_high_t::run_combine(mcombine_t* state) {
//	return combine_x86_64_bitand_high(state->block(), state->insn());
	return false;
}

const char* combine_x86_band_high_t::name() const {
	return "Combine BITAND with x86 Highreg";
}

bool combine_x86_bitor_high_t::run_combine(mcombine_t* state) {
	auto block = state->block();
	auto insn = state->insn();
	if (insn->opcode != m_or)
		return false;


	mop_t * nonconst;

	mop_t * consty = insn->get_eitherlr(mop_n, &nonconst);

	if (!consty || consty->size != 1 || nonconst->t != mop_r || !mreg_has_highbyte_x86(nonconst->r - 1) || !insn->d.lvalue_equal_ignore_size(nonconst))
		return false;


	unsigned defsize;

	if (!find_definition_size(state->bbidx_pool(), block, insn, &defsize, nonconst) || defsize == 1)
		return false;


	nonconst->r -= 1;
	consty->nnn->value <<= 8;
	consty->nnn->org_value <<= 8;
	consty->size = defsize;
	nonconst->size = defsize;
	insn->d.r -= 1;
	insn->d.size = defsize;
	return true;
	//return combine_x86_64_bitor_high(state->block(), state->insn());
}

const char* combine_x86_bitor_high_t::name() const {
	return "Combine BITOR with x86 Highreg";
}

combine_x86_bitor_high_t combine_x86_bitor_high{};
combine_x86_band_high_t combine_x86_band_high{};

COMB_RULE_IMPLEMENT(highbyte_used_with_lowbyte) {
	auto i = state->insn();
	if (!hexext::ran_locopt())
		return false;
	if (!is_mcode_commutative(i->op()) || i->d.size != 1)
		return false;
#define		THEFLAG		(-1)
	auto [rreg, lreg] = i->arrange_by(mop_r, mop_r);
	if (!rreg)return false;
	mop_t* highbyte_term = nullptr;
	mop_t* nonhighbyte_term = nullptr;
	if (mreg_has_highbyte_x86(rreg->r-1)) {
		highbyte_term = rreg;
		nonhighbyte_term = lreg;
	}
	else if (mreg_has_highbyte_x86(lreg->r-1)) {
		highbyte_term = lreg;
		nonhighbyte_term = rreg;
	}
	else {
		return false;
	}
	if (highbyte_term->r - 1 != nonhighbyte_term->r)
		return false;
	unsigned sz=1;
	if (!find_definition_size(state->bbidx_pool(), state->block(), hexext::current_topinsn(), &sz, nonhighbyte_term) || sz < 2)
		return false;
	sz = 2;

	minsn_t* shrguy = new minsn_t(i->ea);

	shrguy->opcode = m_shr;

	shrguy->l = *nonhighbyte_term;

	shrguy->l.size = sz;

	shrguy->r.make_number(8, 1);
	shrguy->d.size = sz;
	shrguy->iprops|= THEFLAG;

	
	minsn_t* xduterm = new minsn_t(i->ea);
	xduterm->opcode = m_and;

	xduterm->l.assign_insn(shrguy, sz);
	xduterm->r.make_number(0xFF, sz);

	xduterm->d.size =sz;
	//highbyte_term->assign_insn(xduterm, 1);

	xduterm->iprops |= THEFLAG;;
	minsn_t* xduboi2 = new minsn_t(i->ea);
	xduboi2->opcode = m_xdu;

	xduboi2->l = *nonhighbyte_term;

	

	xduboi2->d.size = sz;
	xduboi2->iprops |= THEFLAG;


	minsn_t* dup = minsn_t::cloneptr(i);
	dup->l.assign_insn(xduterm,sz);
	dup->r.assign_insn(xduboi2,sz);

	dup->d.erase();
	dup->d.size = sz;
	dup->iprops |= THEFLAG;
	i->opcode = m_low;

	i->r.erase();
	i->l.assign_insn(dup,sz);

	i->d.size = 1;
	i->iprops |= THEFLAG;

	return true;
}