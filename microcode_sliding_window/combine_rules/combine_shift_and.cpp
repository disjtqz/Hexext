
#include "combine_rule_defs.hpp"
#include "combine_shift_and.hpp"

const char* combine_shift_and_t::name() const {
	return "Combine SHR-AND";
}

const char* combine_shift_and_t::description() const {
	return "Changes bitwise tests in the format of (($VAR >> $CONSTSHIFT) & 1) to ($VAR & (1 << $CONSTSHIFT)), which looks more pleasant and is easier to use with enums.";
}

bool combine_shift_and_t::run_combine(mcombine_t* state) {
	minsn_t* insn_ = state->insn();
	if (!insn_->either(mop_d) || !insn_->either(mop_n))
		return false;

	if (insn_->opcode != m_setz && insn_->opcode != m_setnz && insn_->opcode != m_jnz && insn_->opcode != m_jz)
		return false;

	auto insn = insn_->get_eitherlr(mop_d)->d;

	auto num = insn_->get_eitherlr(mop_n);

	if (num->nnn->value) {
		return false;
	}



	if (insn->opcode != m_and)
		return false;

	if (insn->r.t != mop_n)
		return false;

	if (insn->l.t != mop_d)
		return false;

	auto inner = insn->l.d;
	uint64_t shiftcount = 0u;
	mop_t * extracted = nullptr;
	unsigned size_overide = 0;

	if (inner->opcode == m_shr && inner->r.t == mop_n) {
		shiftcount = inner->r.nnn->value;
		extracted = &inner->l;
		size_overide = insn->l.size;
	}
	else if (inner->opcode == m_high) {
		shiftcount = (inner->l.size / 2) * 8;

		extracted = &inner->l;
		size_overide = insn->l.size;
	}
	else if (inner->opcode == m_low && inner->l.t == mop_d && inner->l.d->opcode == m_shr && inner->l.d->r.t == mop_n) {
		size_overide = inner->l.size;
		shiftcount = inner->l.d->r.nnn->value;

		extracted = &inner->l.d->l;

	}
	else {
		return false;
	}


	if (!shiftcount) {
		return false;
	}
	insn->r.nnn->value <<= shiftcount;
	insn->l.copy(*extracted);


	insn->r.size = insn->l.size;
	insn->d.size = insn->r.size;
	num->size = insn->l.size;

	insn_->get_eitherlr(mop_d)->size = insn->l.size;

	delete inner;


	return true;
}

combine_shift_and_t combine_shift_and_rule{};