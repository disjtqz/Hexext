#include "combine_rule_defs.hpp"
#include "combine_or_shift_seq.hpp"
/*
 *v5 = v15 | ((v17 | ((v14 | (v16 << 8)) << 8)) << 8);


 or rdi.4, (
	shl (
		or rbx.4, , 
		#8.1
	).4
 ).4

 shl (or r11.4, (shl rcx.4, #8.1).4).4, #8.1).4

  shl (or r11, (shl rcx, #8)), #8)
*/
static bool combine_or_shift_seq(mblock_t* block, minsn_t* insn) {



	if (insn->opcode != m_shl || insn->d.t != mop_z) {
		return false;
	}

	if (insn->l.t != mop_d || insn->r.t != mop_n)
		return false;


	if (insn->l.d->opcode != m_or)
		return false;

	minsn_t* orinsn = insn->l.d;

	mop_t* non_innershl;
	mop_t* innershl = orinsn->get_either_insnop(m_shl, &non_innershl);

	if (!innershl )
		return false;

	minsn_t* shlinnerinsn = innershl->d;

	if (shlinnerinsn->r.t != mop_n)
		return false;


	

	unsigned unitsize = non_innershl->size;

	
	minsn_t* innerguy1 = new minsn_t(shlinnerinsn->ea);

	minsn_t* innerguy2 = new minsn_t(shlinnerinsn->ea);

	innerguy1->opcode = m_shl;
	innerguy2->opcode = m_shl;
	innerguy1->d.size = unitsize;
	innerguy2->d.size = unitsize;
	innerguy1->l = *non_innershl;

	innerguy2->l = shlinnerinsn->l;
	innerguy2->r = shlinnerinsn->r;

	innerguy2->r.nnn->value += insn->r.nnn->value;
	innerguy2->r.nnn->org_value += insn->r.nnn->value;

	innerguy1->r = insn->r;
	insn->opcode = m_or;

	insn->l.erase();
	insn->r.erase();
	insn->l.assign_insn(innerguy1, unitsize);

	insn->r.assign_insn(innerguy2, unitsize);

	return true;
}

 bool combine_or_shift_t::run_combine(mcombine_t* state) {
	return combine_or_shift_seq(state->block(), state->insn());
}

 const char* combine_or_shift_t::name() const {
	 return "Combine or-shift sequence";
 }

 combine_or_shift_t combine_or_shift{};