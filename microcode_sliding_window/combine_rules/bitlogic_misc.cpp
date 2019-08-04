#include "combine_rule_defs.hpp"
#include "bitlogic_misc.hpp"


COMB_RULE_IMPLEMENT(popcnt_bool_fold) {
	auto i = state->insn();
	mop_t* popterm = extract_unary_exrole(i, exrole_t::popcnt);

	if (!popterm)
		return false;

	potential_valbits_t bts = try_compute_opnd_potential_valbits(popterm);

	if (!bts.is_boolish()) {
		return false;
	}

	mop_t temp = *popterm;

	i->opcode = m_mov;

	i->l = temp;

	i->d.erase();
	i->d.size = i->l.size;

	return true;
}