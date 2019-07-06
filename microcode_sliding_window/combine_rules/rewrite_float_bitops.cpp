#include "combine_rule_defs.hpp"
#include "rewrite_float_bitops.hpp"


/*
f.jb (xor xmm6.8, xmmword_1800E51C0.8).8, xmm2.8, 34
*/

static void build_valnum_mlist(mlist_t* out, mblock_t* block, mop_t* mop) {
	fixed_size_vector_t<mop_t*, 32> vec;
	gather_equal_valnums_in_block(block, mop, vec.pass());
	add_mop_to_mlist(mop, out, &block->mba->vars);
	for (auto&& op : vec) {
		add_mop_to_mlist(op, out, &block->mba->vars);
	}

}

static bool uses_favours_floating(mcombine_t* state, minsn_t* top, mlist_t* list, mblock_t* block, bool prior) {

	cs_assert(top && state && list && block);
	fixed_size_vector_t<minsn_t*, 16> considereds;

	gather_uses(considereds.pass(), state->bbidx_pool(), top->prev, block, list, prior);

	if (considereds.size() == 0)
		return false;

	fixed_size_vector_t<minsn_t*, 32> inner_considereds;

	unsigned n_fpuses = 0;
	unsigned n_nonfpuses = 0;
	lvars_t * lvars = &state->block()->mba->vars;
	for (auto&& user_insn : considereds) {

		inner_considereds.reset();

		gather_user_subinstructions(lvars, user_insn, list, inner_considereds.pass());

		for (auto&& user : inner_considereds) {
			if (user->is_fpinsn()) {
				n_fpuses++;
			}
			else {
				n_nonfpuses++;
			}
		}
	}

	return n_fpuses > n_nonfpuses;
}

static bool uses_favour_floating_either_direction(mcombine_t* state, minsn_t* top, mlist_t* list, mblock_t* block) {
	return uses_favours_floating(state, top, list, block, false) || uses_favours_floating(state, top, list, block, true);
}

bool detect_bitwise_negate_floatop_t::run_combine(mcombine_t* state) {
	minsn_t* insn = state->insn();





	auto [xorop, nonxorop] = insn->arrange_by_insn(m_xor);

	mop_t* maskval, * nonmaskval;
	ea_t ea;
	if (xorop) {
		auto [_xorop2, _maskval, _nonmaskval] = xorop->descend_to_binary_insn(m_xor, mop_v);
		if (!_xorop2)
			return false;
		maskval = _maskval;
		nonmaskval = _nonmaskval;
		ea = _xorop2->ea;
	}
	else if (insn->opcode == m_xor) {

		auto [_maskval, _nonmaskval] = insn->arrange_by(mop_v);

		maskval = _maskval;
		nonmaskval = _nonmaskval;
		ea = insn->ea;
	}
	else
		return false;




	if (!maskval)return false;


	uint64_t mask;
	if (!get_static_value(maskval, &mask))
		return false;

	if (mask != highbit_for_size(nonmaskval->size))
		return false;
	if (is_mcode_conv_to_f(insn->op()))
		return false;
	/*
		try to classify the value
	*/


	if (!insn->is_fpinsn()) {
		if (!mop_seems_floaty_p(nonmaskval)) {
			if (!mop_is_non_kreg_lvalue(nonmaskval))
				return false;

			mlist_t list{};

			//add_mop_to_mlist(nonmaskval, &list, &state->block()->mba->vars);
			build_valnum_mlist(&list, state->block(), nonmaskval);

			auto top = hexext::current_topinsn();
			if (!uses_favour_floating_either_direction(state, top, &list, state->block())) {

				if (insn->opcode == m_xor && insn->d.t != mop_z) {


					if (!mop_seems_floaty_p(&insn->d)) {
						mlist_t list2{};

						build_valnum_mlist(&list2, state->block(), &insn->d);

						if (!top->next || !uses_favours_floating(state, top->next, &list2, state->block(), false)) {
							return false;
						}
						else {

							//weewww
						}

					}

					else {
						//we good
					}



				}
				else {
					return false;
				}

			}
		}
	}

	if (insn->opcode != m_xor) {
		mop_t newmop{};
		mop_t nonmask_copy = *nonmaskval;

		newmop.make_unary_insn_larg(m_fneg, nonmask_copy, nonmaskval->size, ea);

		*xorop = newmop;
	}
	else {
		insn->opcode = m_fneg;
		insn->iprops |= IPROP_FPINSN;
		mop_t newmop{};
		newmop = *nonmaskval;
		insn->l = newmop;
		insn->l.oprops |= OPROP_FLOAT;
		if (insn->d.t != mop_z) {
			insn->d.oprops |= OPROP_FLOAT;
		}

		insn->r.erase();

	}

	return true;

}

bool locate_abs_value_floatpath_t::run_combine(mcombine_t* state) {

	auto insn = state->insn();

	bool already_know_definitely_floating = false;
	if (insn->op() != m_and) {

		if (insn->is_fpinsn()) {
			mop_t* andlad = insn->get_either_insnop(m_and);

			if (!andlad)
				return false;
			insn = andlad->d;
			already_know_definitely_floating = true;
		}

	}

	auto [mask, nonmask] = insn->arrange_by_either(mop_n, mop_v);

	if (!mask)
		return false;
	/*
	if (!mop_is_non_kreg_lvalue(nonmask))
		return false;*/
	bool is_probably_floating = already_know_definitely_floating || mop_seems_floaty_p(nonmask);
	if (!nonmask->is_lvalue()) {


		if (!is_probably_floating)
			return false;
	}
	else {


	}

	uint64_t expect = extend_value_by_size_and_sign(~highbit_for_size(mask->size), mask->size, false);

	if (!mask->is_equal_to(expect, false)) {
		uint64_t gval;
		if (!get_static_value(mask, &gval))
			return false;

		if (gval != expect)
			return false;
	}





	lvars_t* lvars = &state->block()->mba->vars;

	auto top = hexext::current_topinsn();

	if (!is_probably_floating) {
		mlist_t list{};
		build_valnum_mlist(&list, state->block(), nonmask);
		is_probably_floating = uses_favour_floating_either_direction(state, top, &list, state->block());
	}



	if (is_probably_floating) {

		minsn_t* callboi = new minsn_t(insn->ea);
		mop_t tempnonmask = *nonmask;

		mcallarg_t arg{};


		arg = tempnonmask;



		argloc_t aloc{};
		if (tempnonmask.is_lvalue())
			lvalue_mop_to_argloc(lvars, &aloc, &tempnonmask);
		tinfo_t float_ret_type = get_float_type(tempnonmask.size);

		mlist_t retregs{};
		mlist_t spoiled{};
		spoiled.mem.qclear();

		arg.name = "v";
		arg.ea = BADADDR; //uhhh
		arg.type = float_ret_type;



		arg.size = tempnonmask.size;
		new_helper(callboi, "fabs", mask->size, 1, &arg, &float_ret_type, &aloc, &retregs.reg, &spoiled, 1, &arg);

		callboi->r.erase();

		callboi->d.f->flags |= FCI_PROP;

		callboi->d.size = tempnonmask.size;

		insn->r.erase();
		insn->opcode = m_mov;
		insn->l.t = mop_d;
		insn->l.size = tempnonmask.size;
		insn->l.d = callboi;
		//callboi->iprops |= IPROP_FPINSN;
		insn->iprops |= IPROP_FPINSN;

		return true;//this seems a lil dangerous
	}

	return false;

}
detect_bitwise_negate_floatop_t detect_bitwise_negate_floatop{};

locate_abs_value_floatpath_t locate_abs_value_floatpath{};