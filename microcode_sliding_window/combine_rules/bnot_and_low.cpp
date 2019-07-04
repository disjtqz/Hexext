
#include "combine_rule_defs.hpp"
#include "bnot_and_low.hpp"
/*
and (bnot (low (shr (ldx ds.2, (add rcx.8, #4.8).8).4, #4.1).4, .-1).1, .-1).1, #1.1, rax.1

*/
static bool combine_bnot_and_1_impl(mblock_t* blk, minsn_t* insn_) {
	if (hexext::get_parent_mop_size() == -1 || is_definitely_topinsn_p(insn_)) //this one cant run on topinsns. dunno why
		return false;
	if (insn_->opcode != m_and) {
		return false;
	}

	mop_t* insn_op = insn_->get_eitherlr(mop_d);

	mop_t* constop = insn_->get_eitherlr(mop_n);

	if (!insn_op || !constop) {
		return false;
	}



	if (constop->nnn->value != 1ULL) {
		return false;
	}


	minsn_t* inner = insn_op->d;
	if (inner->opcode != m_bnot) {
		return false;
	}



	mop_t* bnot_parm = &inner->l;


	//minsn_t* newop = new minsn_t(BADADDR);

	insn_->opcode = m_setz;

	insn_->d.size = 1;
	constop->nnn->value = 0ULL;

	inner->opcode = m_and;

	inner->r.t = mop_n;

	inner->r.size = inner->l.size;

	inner->r.nnn = new mnumber_t(1ULL);

	if (hexext::get_parent_mop_size() != 1) {
		minsn_t* copyinsn = new minsn_t(BADADDR);
		*copyinsn = *insn_;

		insn_->l.erase();
		insn_->r.erase();
		insn_->d.erase();

		insn_->opcode = m_xdu;

		insn_->d.size = hexext::get_parent_mop_size();

		insn_->l.t = mop_d;
		insn_->l.size = 1;
		insn_->l.d = copyinsn;


	}

	

	return true;

	
}
//jnz (and (bnot (low (shr rsi.4, #3.1).4, .-1).1, .-1).1, #1.1).1, #0.1, 2
static bool combine_jzf_and_bnot_impl(mblock_t* blk, minsn_t* insn) {

	mcode_t op = insn->opcode;

	if (op != m_jz && op != m_jnz)
		return false;

	auto constop = insn->get_eitherlr(mop_n);
	auto insnop = insn->get_eitherlr(mop_d);

	if (!constop || !insnop)
		return false;

	if (constop->nnn->value != 0ULL)
		return false;

	auto inner = insnop->d;

	if (inner->opcode != m_and) {
		return false;
	}
	auto notop = inner->get_either_insnop(m_bnot);


	if (!notop) {
		return false;
	}



	auto negated_jzf = negate_mcode_relation(op);

	insn->opcode = negated_jzf;

	auto old_bnot = notop->d;

	notop->d = old_bnot->l.d;
	notop->t = old_bnot->l.t;

	old_bnot->l.t = mop_z;

	delete old_bnot;

	return true;

	



}

bool combine_bnot_and_one_t::run_combine(mcombine_t* state){
	return combine_bnot_and_1_impl(state->block(), state->insn());
}

const char* combine_bnot_and_one_t::name() const {
	return "Combine bitnot AND pow2";
}

combine_bnot_and_one_t combine_bnot_and_one{};

bool combine_jzf_and_bnot_t::run_combine(mcombine_t* state) {
	return combine_jzf_and_bnot_impl(state->block(), state->insn());
}

const char* combine_jzf_and_bnot_t::name() const {
	return "Combine jzf bittest with bitnot";
}

combine_jzf_and_bnot_t combine_jzf_and_bnot{};