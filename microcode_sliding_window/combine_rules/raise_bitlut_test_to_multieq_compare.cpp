#include "combine_rule_defs.hpp"
#include "raise_bitlut_test_to_multieq_compare.hpp"

static bool raise_bitlut_test_to_multieq_impl(mcombine_t* state, mblock_t* blk, minsn_t* insn, minsn_t** out_res, ea_t ea) {
	if (insn->opcode != m_and) {
		return false;

	}
	if (insn->iprops & IPROP_WILDMATCH)
		return false;
	
	mop_t* non_shl;
	mop_t* shlop = insn->get_either_insnop(m_shl, &non_shl);

	if (!shlop || !non_shl)
		return false;

	if (non_shl->t != mop_n)
		return false;

	minsn_t* innershl = shlop->d;
	if (innershl->iprops & IPROP_WILDMATCH)
		return false;
	if (!innershl->l.is_equal_to(1ULL, false))
		return false;

	//if (!innershl->r.is_lvalue())
	//	return false;

	uint64_t bitlut = extend_value_by_size_and_sign(non_shl->nnn->value, non_shl->size, false);

	unsigned n_lut_entries = __popcnt64(bitlut);

	minsn_t* chain_start = new minsn_t(hexext::current_topinsn()->ea);

	minsn_t* current_chain_pos = chain_start;
	
	mop_t* lval = resolve_lvalue(&innershl->r);
	unsigned compsize = innershl->r.size;

	mop_t* r;
	if (lval) {
		r = lval;
		if (lval->size == 1) {
			unsigned defsize = 0;
			if (find_definition_size(state->bbidx_pool(), blk, hexext::current_topinsn(), &defsize, lval)) {
				compsize = defsize;
			}
		}
		else {
			compsize = lval->size;
		}

	}
	else {
		r = &innershl->r;
	}

	
	unsigned n = 0;
	auto make_compare = [innershl, compsize, &n, ea, r](minsn_t * insn, unsigned against) {
		insn->opcode = m_setz;
		insn->l = *r;
		insn->l.size = compsize;

		insn->r.t = mop_n;
		insn->r.size = compsize;
		insn->r.nnn = new mnumber_t(against, ea, n);
		insn->d.size = 1;
		return insn;
	};
	std::vector<minsn_t*> chain{};

	auto add_compare = [&make_compare, &current_chain_pos,&chain](unsigned value) {

		minsn_t* newins = new minsn_t(hexext::current_topinsn()->ea);
		make_compare(newins, value);

		chain.push_back(newins);
	};
	
	chain.reserve(n_lut_entries);


	for (unsigned i = 0; i < n_lut_entries; ++i) {
		add_compare(_tzcnt_u64(bitlut));
		bitlut &= bitlut - 1ULL;
	}


	*out_res = chain_setops_with_or(chain, hexext::current_topinsn()->ea);


	return true;
}

bool raise_bitlut_multieq_t::run_combine(mcombine_t* state) {
	auto blk = state->block();
	auto insn = state->insn();
	if (insn->iprops & IPROP_WILDMATCH)
		return false;
	mcode_t op = insn->opcode;

	if (op != m_jnz && op != m_jz && op != m_setz && op != m_setnz)
		return false;

	if (insn->l.t != mop_d || !insn->r.is_equal_to(0ULL, false))
		return false;
	minsn_t * chain = nullptr;



	bool wecool = raise_bitlut_test_to_multieq_impl(state, blk, insn->l.d, &chain, insn->ea == BADADDR ? insn->r.nnn->ea : insn->ea);
	if (!wecool)
		return false;

	insn->l.erase();

	insn->l.t = mop_d;
	insn->l.d = chain;
	insn->l.size = 1;

	insn->r.size = 1;


	return true;

}

//jnz (and (low (shl #1.8, rcx.1).8, .-1).4, #1331.4).4, #0.4, 18
bool shl_and_low(mblock_t* blk, minsn_t* insn) {
	if ((!mcode_is_set(insn->opcode) && !mcode_is_jconditional(insn->opcode)) || (insn->r.t != mop_n && insn->r.t != mop_z) ) {
		return false;
	}
	

	if (insn->l.t != mop_d)
		return false;

	if (insn->l.d->opcode != m_and)
		return false;

	auto inner = insn->l.d;
	mop_t* non_low;
	mop_t* lowop = inner->get_either_insnop(m_low, &non_low);

	if (!lowop || !non_low)
		return false;

	if (non_low->t != mop_n)
		return false;

	minsn_t* lowins;

	lowins = lowop->d;

	
	lowop->t = lowins->l.t;
	lowop->d = lowins->l.d;
	lowop->size = lowins->l.size;

	lowins->l.t = mop_z;


	//std::swap(lowins->l, *lowop);

	non_low->size = lowop->size;

	insn->l.size = lowop->size;

	if (insn->r.t != mop_z) {
		insn->r.size = lowop->size;
	}
	delete lowins;

	return true;

}

const char* raise_bitlut_multieq_t::name() const {
	return "Raise bitlut lookup to multi-equality compare";
}
raise_bitlut_multieq_t raise_bitlut_multieq{};