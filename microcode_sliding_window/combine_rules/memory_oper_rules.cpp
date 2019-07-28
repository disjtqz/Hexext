#include "combine_rule_defs.hpp"
#include "memory_oper_rules.hpp"
#include <bitset>


/*
1711ae68 : stx (and (ldx ds.2, (add rbx.8, #52.8).8).4, #4294967263.4).4, ds.2, (add rbx.8, #52.8).8
1711ae6f : stx (or (xdu (and rax.1, #32.1).1, .-1).4, (ldx ds.2, (add rbx.8, #52.8).8).4).4, ds.2, (add rbx.8, #52.8).8
*/
COMB_RULE_IMPLEMENT(stx_stx_combine) {

	auto ins = state->insn();
	auto blk = state->block();
	if (!is_definitely_topinsn_p(ins) || ins->op() != m_stx)
		return false;

	if (!ins->next || ins->next->op() != m_stx)
		return false;


	mop_t* value_first;
	mop_t* value_second;

	if (!try_extract_equal_stx_dests(ins, ins->next, &value_first, &value_second))return false;


	if (value_first->t != mop_d || value_second->t != mop_d) {
		return false;
	}

	if (!is_mcode_commutative(value_first->d->op()) || !is_mcode_commutative(value_second->d->op()))
		return false;

	//auto [andinsn,ff,fff] = value_first->descend_to_binary_insn(m_and, mop_d);

	auto andinsn = value_first->d;

	if (!andinsn)
		return false;

	auto orinsn = value_second->d;//value_second->descend_to_binary_insn(m_or, mop_d);

	if (!orinsn)
		return false;

	auto [and_ldx, and_nonldx] = andinsn->arrange_by_insn(m_ldx);

	if (!and_ldx)
		return false;

	auto [or_ldx, or_nonldx] = orinsn->arrange_by_insn(m_ldx);

	if (!or_ldx)
		return false;

	if (!equal_ldx_srcs(or_ldx->d, and_ldx->d))
		return false;
	if (!ldx_src_equals_stx_dest(or_ldx->d, ins))
		return false;
	/*
		now we just gotta combine em
	*/
	minsn_t* dupboi = new minsn_t(BADADDR);
	*dupboi = *andinsn;

	minsn_t* lel = or_ldx->d;

	or_ldx->d = dupboi;

	delete lel;

	//blk->remove_from_block(ins);

	ins->opcode = m_nop;
	//delete ins;
	return true;

}

COMB_RULE_IMPLEMENT(preload_repetitive_ldx) {
#if 0
	auto insn = state->insn();

	if (hexext::current_topinsn() != insn)
		return false;

	if (hexext::ran_locopt())
		return false;



	fixed_size_vector_t<mop_t*, 64> inner_ldxs{};

	gather_inner_insns_by_opcode(m_ldx, inner_ldxs.pass(), insn);

	if (inner_ldxs.size() < 2)
		return false;

	std::set<uint64_t> set_encoded{};


	auto compute_key = [](mop_t* ldxop, uint64_t* keyout) {
		mreg_t segreg{};
		mop_t* base = nullptr;
		mop_t* displ = nullptr;
		if (!extract_ldx_displ(ldxop->d, &segreg, &base, &displ))
			return false;
		if (!base || base->t != mop_r)
			return false;

		mreg_t basereg = base->r;
		if (segreg != mr_data_seg)
			return false;
		if (is_kreg(basereg))
			return false;
		uint64_t disp = 0;

		if (displ)
			disp = displ->nnn->value;



		/*
			low 6 -> accsize

		*/
#ifdef __EA64__

		basereg /= 8;
#else
		basereg /= 4;
#endif

		uint64_t key = ((uint64_t)basereg | (_tzcnt_u32(ldxop->size) << 8)) | ((uint64_t)disp << 15);
		*keyout = key;
		return true;
	};


	fixed_size_vector_t<mop_t*, 64> to_hoist{};


	for (auto&& ldxop : inner_ldxs) {
		uint64_t key;
		if (!compute_key(ldxop, &key))
			continue;
		//found equal ldx disp+base+size group
		if (set_encoded.contains(key)) {

			for (auto&& fnldx : inner_ldxs) {
				uint64_t key2;
				if (!compute_key(fnldx, &key2))
					continue;

				if (key2 == key) {
					to_hoist.push_back(fnldx);
				}
			}
			break;

		}
		else {
			set_encoded.insert(key);
		}
	}

	if (to_hoist.size() < 2)
		return false;



	mop_t* first = to_hoist[0];


	auto blk = state->block();
	auto unitsize = first->size;
	mreg_t tempval = allocate_tempreg_unused_in_block_range(blk, unitsize);



	if (tempval == -1)
		return false;//:( no tempreg available

	minsn_t* prior_boi = new minsn_t(BADADDR);

	*prior_boi = *first->d;
	prior_boi->d.make_reg(tempval, unitsize);

	blk->insert_into_block(prior_boi, insn->prev);

	for (auto&& to_rep : to_hoist) {
		to_rep->make_reg(tempval, unitsize);
	}
	prior_boi->iprops |= IPROP_DONT_COMB;
	return true;
#else
return false; //causes infinite loops because hexrays immediately recombines them :/
#endif
}