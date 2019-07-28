#include "combine_rule_defs.hpp"
#include "cleanup_late_flag_ops.hpp"

/*
Block 0 - 15874a to 15874a. Flags = 248.
Block 1 - 15874a to 158756. Flags = 48.
15874e : ldx cs.2, (add R0.4, #16.4).4, R12.4
158754 : jge R12.4, R1.4, 3
Block 2 - 158756 to 15875c. Flags = 48.
158756 : mov #1.4, .-1, R12.4
15875a : goto 11, .-1
Block 3 - 15875c to 15875e. Flags = 48.
15875c : jnz R12.4, R1.4, 6
Block 4 - 15875e to 158762. Flags = 48.
158760 : jz R3.4, #0.4, 2
Block 5 - 158762 to 15876c. Flags = 48.
15876a : jnz (and (ldx cs.2, (add R0.4, #32.4).4).1, #2.1).1, #0.1, 2
Block 6 - 15876c to 158784. Flags = 48.
158772 : stx #4294967295.4, cs.2, (add R0.4, #28.4).4
158776 : add R12.4, R2.4, LR.4
15877a : seto R12.4, (sub R1.4, R2.4).4, VF.1
15877a : sets (sub R12.4, (sub R1.4, R2.4).4).4, .-1, NF.1
15877c : mov #0.4, .-1, R12.4
158780 : mov LR.4, .-1, R4.4
158782 : jcnd (lnot (xor NF.1, VF.1).1, .-1).1, .-1, 8
Block 7 - 158784 to 158786. Flags = 48.
158784 : mov R1.4, .-1, R4.4
Block 8 - 158786 to 15878a. Flags = 48.
158788 : jl R2.4, #0.4, 10
Block 9 - 15878a to 15878c. Flags = 48.
15878a : mov LR.4, .-1, R4.4
Block 10 - 15878c to 15878e. Flags = 48.
15878c : stx R4.4, cs.2, (add R0.4, #16.4).4
Block 11 - 15878e to 158796. Flags = 48.
15878e : ldx cs.2, (add R0.4, #20.4).4, LR.4
158794 : jl LR.4, R1.4, 19
Block 12 - 158796 to 158798. Flags = 48.
158796 : jnz R3.4, #0.4, 14
Block 13 - 158798 to 15879c. Flags = 48.
15879a : jz LR.4, R1.4, 19
Block 14 - 15879c to 1587a2. Flags = 48.
15879c : seto R2.4, #4294967295.4, VF.1
15879c : sets (add R2.4, #1.4).4, .-1, NF.1
1587a0 : jg R2.4, #4294967295.4, 16
Block 15 - 1587a2 to 1587a6. Flags = 48.
1587a4 : seto LR.4, (sub R1.4, R2.4).4, VF.1
1587a4 : sets (sub LR.4, (sub R1.4, R2.4).4).4, .-1, NF.1
Block 16 - 1587a6 to 1587a8. Flags = 48.
1587a6 : jcnd (xor NF.1, VF.1).1, .-1, 18
Block 17 - 1587a8 to 1587b0. Flags = 48.
1587ac : stx (add LR.4, R2.4).4, cs.2, (add R0.4, #20.4).4
1587ae : goto 19, .-1
Block 18 - 1587b0 to 1587b2. Flags = 48.
1587b0 : stx R1.4, cs.2, (add R0.4, #20.4).4
Block 19 - 1587b2 to 1587ba. Flags = 48.
1587b2 : ldx cs.2, (add R0.4, #24.4).4, LR.4
1587b8 : jl LR.4, R1.4, 27
Block 20 - 1587ba to 1587bc. Flags = 48.
1587ba : jnz R3.4, #0.4, 22
Block 21 - 1587bc to 1587c0. Flags = 48.
1587be : jz LR.4, R1.4, 27
Block 22 - 1587c0 to 1587c6. Flags = 48.
1587c0 : seto R2.4, #4294967295.4, VF.1
1587c0 : sets (add R2.4, #1.4).4, .-1, NF.1
1587c4 : jg R2.4, #4294967295.4, 24
Block 23 - 1587c6 to 1587ca. Flags = 48.
1587c8 : seto LR.4, (sub R1.4, R2.4).4, VF.1
1587c8 : sets (sub LR.4, (sub R1.4, R2.4).4).4, .-1, NF.1
Block 24 - 1587ca to 1587cc. Flags = 48.
1587ca : jcnd (xor NF.1, VF.1).1, .-1, 26
Block 25 - 1587cc to 1587d0. Flags = 48.
1587cc : add LR.4, R2.4, R1.4
Block 26 - 1587d0 to 1587d2. Flags = 48.
1587d0 : stx R1.4, cs.2, (add R0.4, #24.4).4
Block 27 - 1587d2 to 1587d6. Flags = 48.
1587d2 : mov R12.4, .-1, R0.4
Block 28 - ffffffff to ffffffff. Flags = 248.

*/

COMB_RULE_IMPLEMENT(sift_down_flagcomps) {
	//wait until after initial redundant flag sets are eliminated
	if (!hexext::ran_locopt())
		return false;

	auto i = state->insn();
	auto blk = state->block();
	if (i->op() != m_jcnd)
		return false;

	mlist_t jcnd_use{};
	generate_use_for_insn(blk->mba, i, &jcnd_use);

	if (!mlist_has_any_cc(&jcnd_use))
		return false;
	auto find_unused_tr = [blk](minsn_t * rstart, minsn_t * rend, unsigned reqsize) {
		return allocate_tempreg_unused_in_block_range(blk, reqsize);
	};
	bool did_change = false;
	for (unsigned currcc = mr_cf; currcc < cc_count; ++currcc) {

		if (!jcnd_use.reg.has(currcc))
			continue;



		mlist_t currl{};
		currl.reg.add_(currcc, 1);

		minsn_t* ccdef = find_definition_backwards(blk, i, &currl);

		if (!ccdef)
			continue;


		if (!mcode_is_set(ccdef->op()))
			continue;
		if (find_next_use(blk, ccdef, &currl, nullptr) != i)
			continue;
		/*
			make sure successors dont use the flag
		*/
		fixed_size_vector_t<minsn_t*, 4> succuses{};
		for (auto&& succ : blk->succset) {
			auto succblk = blk->mba->natural[succ];

			gather_uses(succuses.pass(), state->bbidx_pool(), succblk->head, succblk, &currl, false);
			if (succuses.size() != 0)
				break;
		}

		if (succuses.size() != 0)
			continue;

		mop_t l = ccdef->l;
		mop_t r = ccdef->r;
		
		mlist_t interdefs{};
		auto lvars = &blk->mba->vars;


		generate_defs_between(blk, ccdef, i, &interdefs);

		auto decouple_from_insn = [&interdefs, lvars, ccdef, blk, &find_unused_tr, i](mop_t & moppy) {
			if (test_any_submop_in_mlist(&moppy, &interdefs, lvars)) {

				mop_t tempreg{};

				mreg_t tr = find_unused_tr(ccdef->prev, i, moppy.size);

				if (tr == -1)
					return false;

				tempreg.make_reg(tr, moppy.size);

				insert_mov2_before(ccdef->ea, blk, ccdef, &moppy, &tempreg);

				ccdef->l = std::move(tempreg);
			
			}
			return true;
		};
	

		if (!decouple_from_insn(l) || !decouple_from_insn(r)) {
			return false;
		}
		

		blk->remove_from_block(ccdef);

		ccdef->d.erase();
		ccdef->d.size = 1;

		mop_t ccrepl{};
		ccrepl.t = mop_d;
		ccrepl.d = ccdef;
		ccrepl.size = 1;
		replace_cc_flag_mops_with_other_mop(i, currcc, &ccrepl);
		did_change = true;
	}

	return did_change;

}



COMB_RULE_IMPLEMENT(interblock_jcc_deps_combiner) {
	if (!hexext::ran_locopt())
		return false;
	auto i = state->insn();
	auto blk = state->block();
	if (!mcode_is_jconditional(i->op()))
		return false;


	mlist_t lst{};

	generate_use_for_insn(blk->mba, i, &lst);
	bool did_change = false;

	for (unsigned cc = mr_cf; cc < mr_pf + 1; ++cc) {
		if (!lst.reg.has(cc))
			continue;
		/*
			gather single definition for flag
		*/
		fixed_size_vector_t<gather_defblk_res_t, 2> definitions{};

		mlist_t lst_flag{};
		lst_flag.reg.add(cc);
		if (!gather_defs(definitions.pass(), state->bbidx_pool(), blk, &lst_flag) || definitions.size() != 1) {
			continue;
		}


		fixed_size_vector_t<minsn_t*, 2> uses{};
		gather_uses(uses.pass(), state->bbidx_pool(), i->prev, blk , &lst_flag, true);
		if (uses.size() != 0)
			continue;

		mlist_t used_by_def{};
		minsn_t* sifted_down = definitions[0].first;

		if (!mcode_is_set(sifted_down->op()))
			continue;

		generate_use_for_insn(blk->mba, sifted_down, &used_by_def);

		if (!no_redef_in_path(state->bbidx_pool(), definitions[0].second, definitions[0].first, blk, &used_by_def))
			continue;
		//definitions[0].second->remove_from_block(sifted_down);

		//sifted_down->ea = i->ea;


		minsn_t* dupboi = new minsn_t(BADADDR);
		*dupboi = *sifted_down;
		dupboi->d.erase();
		dupboi->d.size = 1;
		mop_t mop{};
		mop.t = mop_d;
		mop.size = 1;
		mop.d = dupboi;
		replace_cc_flag_mops_with_other_mop(i, cc, &mop);

	//	blk->insert_into_block(sifted_down, nullptr);


		did_change = true;
		
	}

	return did_change;

}

COMB_RULE_IMPLEMENT(merge_shortcircuit_nosideeffects) {
#if 0
	auto i = state->insn();
	if (!hexext::ran_locopt())
		return false;
	if (!mcode_is_jcc(i->op()))
		return false;

	auto blk = state->block();

	if (blk->predset.size() != 2)
		return false;

	int common_pred = -1;
	int noncommon_pred = -1;
	/*for (auto&& p : blk->predset) {
		auto pred = blk->mba->natural[p];

		for (auto&& inp : pred->predset) {

			if (blk->predset.has(inp) && inp != p && pred->succset.size() == 1) {

				common_pred = inp;
				noncommon_pred = p;
				goto done_with_inner;
			}
		}
	}
done_with_inner:*/
	mblock_t* target = blk->mba->natural[i->d.b];

	

	mblock_t* fallthrough = resolve_goto_block(blk->nextb);
	
	if (fallthrough == blk->nextb)
		return false;

	mblock_t* merger = nullptr;
	bool is_or = false;
	int new_goto_target = -1;
	int new_jcc_target = i->d.b;
	if (fallthrough->head == fallthrough->tail && fallthrough->head && mcode_is_jcc(fallthrough->head->op()) &&
		fallthrough->head->d.t == mop_b && fallthrough->head->d.b == i->d.b) {
		
		merger = fallthrough;
		is_or = true;

		new_goto_target = fallthrough->nextb->serial;
		
	}


	if (!merger)
		return false;
	


	if (merger->tail != merger->head || !mcode_is_jcc(merger->tail->op()))
		return false;

	minsn_t* dupboi = new minsn_t(BADADDR);
	*dupboi = *i;

	dupboi->d.erase();
	dupboi->d.size = 1;
	dupboi->opcode = (mcode_t)jcc_to_setcc(i->op());

	minsn_t* dupboi2 = new minsn_t(BADADDR);

	*dupboi2 = *merger->tail;

	dupboi2->opcode = (mcode_t)jcc_to_setcc(merger->tail->op());
	dupboi2->d.erase();
	dupboi2->d.size = 1;


	minsn_t* band = new minsn_t(i->ea);

	mop_t dup1{};
	dup1.size = 1;
	dup1.d = dupboi;
	dup1.t = mop_d;
	mop_t dup2{};
	dup2.size = 1;
	dup2.d = dupboi2;
	dup2.t = mop_d;

	
	
	if (!is_or) {
		setup_bitand(band, &dup1, &dup2);
		
	}
	else {
		setup_bitor(band, &dup1, &dup2);
	}

	mop_t testcond{};
	testcond.size = 1;
	testcond.d = band;
	testcond.t = mop_d;

	i->opcode = m_jnz;

	//i->d.make_blkref(new_jcc_target);

	i->l = std::move(testcond);
	i->r.make_number(0ULL, 1);
	fallthrough->head->l.b = new_goto_target;



	



	msg("Got un\n");
	return true;
#else
return false;
#endif
}
/*
180008b41 : jbe (sub rcx.4, #55.4).4, #1.4, 4
Block 2 - 180008b43 to 180008b4b. Flags = 48.
180008b49 : jbe (sub rcx.4, #15.4).4, #1.4, 4
Block 3 - 180008b4b to 180008b4f. Flags = 48.
*/
COMB_RULE_IMPLEMENT(distribute_constant_sub_in_const_comp) {

	auto i = state->insn();

	if (!mcode_is_set(i->op()) && !mcode_is_jcc(i->op()))
		return false;

	auto [inner_insn, const_term] = i->arrange_by(mop_d, mop_n);

	if (!inner_insn)
		return false;


	auto [subtractor, innersubconst, innersubterm] = inner_insn->descend_to_binary_insn(m_sub, mop_n);

	if (!subtractor)
		return false;


	if (innersubconst != &subtractor->r)
		return false;
	uint64_t comp;
	const_term->is_constant(&comp, false);
	uint64_t bound;
	innersubconst->is_constant(&bound, false);

	const_term->nnn->update_value(bound + comp);

	mop_t tempboi{ std::move(*innersubterm) };

	*inner_insn = std::move(tempboi);

	return true;

}
/*
16317c : mov #0.4, . - 1, R0.4
163180 : jl S0.4, R9.4, 19
Block 18 - 163182 to 163184. Flags = 48.
163182 : mov #1.4, . - 1, R0.4
Block 19 - 163184 to 16318a.Flags = 48.
163188 : jz(and R4.4, R0.4).4, #0.4, 22
*/
COMB_RULE_IMPLEMENT(replace_boolean_flow_with_boolean_logic) {
	auto insn = state->insn();

	if (insn != hexext::current_topinsn())
		return false;

	if (!hexext::ran_locopt())
		return false;


	if (insn->opcode != m_mov || !insn->l.is_equal_to(0, false))
		return false;

	mlist_t dest{};
	auto blk = state->block();

	add_mop_to_mlist(&insn->d, &dest, &blk->mba->vars);
	bool redefed = false;

	if (find_next_use(blk, insn, &dest, &redefed))
		return false;

	if (redefed)
		return false;

	auto blkend = blk->tail;

	if (!mcode_is_jcc(blkend->op()))
		return false;

	auto fallthrough_block = blk->nextb;
	if (!fallthrough_block)
		return false;

	if (fallthrough_block->head != fallthrough_block->tail || !fallthrough_block->head)
		return false;



	auto fallthroughop = fallthrough_block->head;
	if (fallthroughop->op() != m_mov || !fallthroughop->d.lvalue_equal(&insn->d) || !fallthroughop->l.is_equal_to(1ULL, false))
		return false;

	auto fallthrough2 = fallthrough_block->nextb;

	auto jcctarg = blk->mba->natural[blkend->d.b];

	if (fallthrough2 != jcctarg)return false;

	//jcc can become setncc, with the mov target as the operand, and we just need to skip over the fallthrough

	

	mcode_t newsetcc = negate_mcode_relation((mcode_t)jcc_to_setcc(blkend->op()));

	if (newsetcc == m_nop)
		return false;




	
#if 0
	blkend->opcode = newsetcc;
	blkend->d = insn->d;
	blkend->d.size = 1;
	if (insn->d.size == 1) {
		insn->make_nop();
		
	}
	else {

	

	}
	#else
	mop_t dupr = blkend->r;
	mop_t dupl = blkend->l;


	setup_subinsn_setcc_of_size(blkend, newsetcc, insn->d.size, &dupl, &dupr);

	blkend->d = insn->d;
	insn->make_nop();

#endif

	if (blkend->is_fpinsn() && blkend->opcode == m_xdu) {
		blkend->l.d->iprops |= IPROP_FPINSN;
		blkend->iprops &= ~IPROP_FPINSN;
	}
	minsn_t* newtail = new minsn_t(blkend->ea);

	newtail->opcode = m_goto;

	newtail->l.make_blkref(fallthrough2->serial);

	blk->insert_into_block(newtail, blkend);
	blk->succset.del(fallthrough_block->serial);
	blk->type = BLT_1WAY;

	fallthrough_block->predset.del(blk->serial);

	return true;


}
/*
Block 7 - 1010075c to 10100760. Flags = 48.
1010075e : jz ecx.1, #0.1, 10
Block 8 - 10100760 to 10100764. Flags = 48.
10100762 : jz eax.1, #0.1, 10
Block 9 - 10100764 to 10100768. Flags = 48.
10100766 : jz ecx.1, eax.1, 14
Block 10 - 10100768 to 1010076e. Flags = 48.
10100769 : mov #0.1, .-1, eax.1
1010076d : goto 48, .-1
*/
COMB_RULE_IMPLEMENT(merge_short_circuit_or_with_no_side_effects) {
	minsn_t* insn = state->insn();

	if (!hexext::ran_locopt() || hexext::ran_glbopt())
		return false;
	mcode_t operation = insn->op();

	if (!mcode_is_jcc(operation))
		return  false;

	mblock_t* blk = state->block();
	mblock_t* fallthrough = blk->nextb;

	if (!fallthrough)
		return false;

	if (fallthrough->predset.size() != 1)
		return false;

	
	if (fallthrough->type != BLT_2WAY)
		return false;

	minsn_t* fallthroughop = fallthrough->head;

	if (fallthroughop != fallthrough->tail)
		return false;

	int targetbb = insn->d.b;


	if (!mcode_is_jcc(fallthroughop->op()) || fallthroughop->d.b != targetbb)
		return false;


	if (minsn_might_have_side_effects(fallthrough->head))
		return false;

	mcode_t insn_as_set = (mcode_t)jcc_to_setcc(operation);
	mcode_t fallthrough_as_set = (mcode_t)jcc_to_setcc(fallthroughop->op());
	if (!mcode_is_set(insn_as_set) || !mcode_is_set(fallthrough_as_set))
		return false;

	minsn_t* dup_curr = minsn_t::cloneptr(insn);
	minsn_t* dup_fallthrough = minsn_t::cloneptr(fallthroughop);


	dup_curr->opcode = insn_as_set;
	dup_fallthrough->opcode = fallthrough_as_set;

	dup_curr->d.erase();
	dup_curr->d.size = 1;

	dup_fallthrough->d.erase();
	dup_fallthrough->d.size = 1;


	mop_t orboi1 = mop_t::subinsn(dup_curr, 1);
	mop_t orboi2 = mop_t::subinsn(dup_fallthrough, 1);

	minsn_t* orguy = new minsn_t(BADADDR);

	setup_bitor(orguy, &orboi1, &orboi2);
	orguy->ea = insn->ea;

	insn->opcode = m_jnz;


	insn->iprops &= ~IPROP_FPINSN;
	insn->l.assign_insn(orguy, 1);

	insn->r.make_number(0, 1);


	fallthroughop->make_nop();

	fallthrough->succset.del(targetbb);
	blk->mba->natural[targetbb]->predset.del(fallthrough->serial);



	blk->flags |= MBL_INCONST;
	fallthrough->flags |= MBL_INCONST;
	fallthrough->type = BLT_1WAY;

	return true;
}

COMB_RULE_IMPLEMENT(merge_multi_setz_chain_interval) {

	minsn_t* insn = state->insn();

	if (insn->opcode != m_or)return false;


	fixed_size_vector_t<minsn_t*, 256> chain{};

	if (!gather_setop_or_chain(chain.pass(), insn))
		return false;

	if (chain.size() < 5)
		return false;
	fixed_size_vector_t<minsn_t*, 256> non_setz{};

	fixed_size_vector_t<minsn_t*, 256> setzs{};
	auto may_make_interval_from = [](minsn_t* ins) {

		if (ins->op() != m_setz)
			return false;
		auto [numboi, mreg] = ins->arrange_by(mop_n, mop_r);

		if (!numboi)
			return false;

		return true;
	};

	for (auto&& setop : chain) {
		if (!may_make_interval_from(setop)) {
			non_setz.push_back(setop);
		}
		else {
			setzs.push_back(setop);
		}
	}
	auto encode_map_key = [](mop_t* mr) {
		return mr->r | (mr->size << 24);
	};

	auto decode_map_mreg = [](unsigned v) {
		return v & ((1 << 24) - 1);
	};
	auto decode_map_mreg_size = [](unsigned v) {
		return v >> 24;
	};
	std::map<unsigned, fixed_size_vector_t<uint64_t, 256>> compset{};

	for (auto&& setz : setzs) {

		auto [mreg, number] = setz->arrange_by(mop_r, mop_n);

		auto& vec = compset[encode_map_key(mreg)];

		vec.push_back(number->nnn->value);
		std::sort(vec.begin(), vec.end());
	}
	

	std::vector<minsn_t*> new_chain{};


	/*fixed_size_vector_t<minsn_t*, 256> outliers{};
	fixed_size_vector_t< comp_interval_t, 128> ivls{};

	for (auto&& comps : compset) {


	}*/

	fixed_size_vector_t<uint64_t, 256> outliers{};
	fixed_size_vector_t< u64_comp_interval_t, 128> ivls{};
	bool joining_ivls_worth_it = false;

	for (auto&& compchain : compset) {
		outliers.reset(); ivls.reset();

		if (!make_compare_interval_from_sorted_range(ivls.pass(), outliers.pass(), compchain.second.pass())) {
			return false; //too complex?
		}

		unsigned chain_cost = outliers.size() + (ivls.size() * 2);

		if (chain_cost < (compchain.second.size())) {
			joining_ivls_worth_it = true;
			break;
		}
	}
	if (!joining_ivls_worth_it)
		return false;

	for (auto&& compchain : compset) {
		outliers.reset(); ivls.reset();

		make_compare_interval_from_sorted_range(ivls.pass(), outliers.pass(), compchain.second.pass());

		mop_t mrop{};
		mrop.make_reg(decode_map_mreg(compchain.first), decode_map_mreg_size(compchain.first));

		for (auto&& ivl : ivls) {

			minsn_t* ivlptr = new minsn_t();

			setup_ucomp_ulbound(ivlptr, &mrop, mrop.size, ivl.low, ivl.high, insn->ea);

			new_chain.push_back(ivlptr);

		}

		for (auto&& outlier : outliers) {
			minsn_t* outlop = new minsn_t();
			mop_t numop{};
			numop.make_number(outlier, mrop.size, insn->ea);
			setup_subinsn_setcc_of_size(outlop, m_setz, mrop.size, &mrop, &numop);
			new_chain.push_back(outlop);
		}

	}

	for (auto&& recycled : non_setz) {
		new_chain.push_back(minsn_t::clonemoveptr(recycled));
	}



	minsn_t* newboi = chain_setops_with_or(new_chain, insn->ea);

	*insn = std::move(*newboi);


	return true;

}
/*
Block 12 - 1bf526 to 1bf544. Flags = 48.
1bf540 : seto R11.4, unknown@688.4, VF.1
1bf540 : sets (sub R11.4, unknown@688.4).4, .-1, NF.1
1bf542 : jge R11.4, unknown@688.4, 14
Block 13 - 1bf544 to 1bf54a. Flags = 48.
1bf548 : seto unknown@680.4, unknown@688.4, VF.1
1bf548 : sets (sub unknown@680.4, unknown@688.4).4, .-1, NF.1
Block 14 - 1bf54a to 1bf54c. Flags = 48.
1bf54a : jcnd (lnot (xor NF.1, VF.1).1, .-1).1, .-1, 16
*/
COMB_RULE_IMPLEMENT(interblock_flagop_merger) {
#if 1
	auto insn = state->insn();
	auto blk = state->block();

	if (!mcode_is_jcc(insn->op()) || !hexext::ran_locopt())
		return false;

	auto fallthrough = blk->nextb;
	if (!fallthrough)
		return false;
	
	if (!fallthrough->nextb || fallthrough->type != BLT_1WAY ||fallthrough->nextb != blk->mba->natural[insn->d.b] ||
		fallthrough->predset.size() != 1)
		return false;
	mlist_t fidefs{};
	mlist_t fiuse{};

	unsigned nsetop = 0;
	for (auto fi = fallthrough->head; fi; fi = fi->next) {
		if (!mcode_is_set(fi->op()) || fi->d.t != mop_r)
			return false;
		if (minsn_might_have_side_effects(fi))
			return false;

		add_mop_to_mlist(&fi->d, &fidefs, &blk->mba->vars);
		generate_use_for_insn(blk->mba, fi, &fiuse);
	//if (test_mop_in_mlist(&fi->d, &fiuse, &blk->mba->vars))
		//	return false;
		nsetop++;
	}
	if (nsetop > 15)
		return false;

	/*if (fiuse.reg.has_common(fidefs.reg)) {
		return false;
	}*/
	/*if (!blk->mustbdef.reg.includes(fidefs.reg))
		return false;*/

	
	mreg_t temp_flags = allocate_tempreg_unused_in_block_range(blk, 16);
	if (temp_flags == -1)return false;

	unsigned mrpos = 0;

	fixed_size_vector_t<minsn_t*, 16> setops_before_masking{};
	fixed_size_vector_t<minsn_t*, 16> masking_ops{};

	mreg_t selector_reg = temp_flags + 15;
	mop_t selector_op{};
	selector_op.make_reg(selector_reg, 1);

	for (auto fi = fallthrough->head; fi; fi = fi->next, mrpos++) {
		minsn_t* setop = minsn_t::cloneptr(fi);
		setop->ea = insn->ea;
		setop->d.r = temp_flags + mrpos;

		setops_before_masking.push_back(setop);

		minsn_t* selector = new minsn_t(insn->ea);

		setup_flagged_bool_select(selector, &selector_op, &fi->d, &setop->d, insn->ea);
		selector->d = fi->d;

		masking_ops.push_back(selector);
	}


	insn->opcode = (mcode_t)jcc_to_setcc(insn->op());
	insn->d.make_reg(selector_reg, 1);

	for (auto&& setop : setops_before_masking) {
		blk->insert_into_block(setop, insn->prev);
	}

	for (auto&& maskop : masking_ops) {
		blk->insert_into_block(maskop, blk->tail);
	}


	minsn_t* gotoguy = new minsn_t(insn->ea);

	gotoguy->opcode = m_goto;

	gotoguy->l.make_blkref(fallthrough->nextb->serial);

	blk->insert_into_block(gotoguy, blk->tail);

	blk->succset.del(fallthrough->serial);

	fallthrough->predset.del(blk->serial);


	for (auto fi = fallthrough->head; fi; fi = fi->next)
		fi->make_nop();
	fallthrough->flags |= MBL_INCONST;


	blk->type = BLT_1WAY;

	return true;

#else
auto insn = state->insn();
auto blk = state->block();
bool has_cflow = hexext::ran_locopt();
if (!mcode_is_jcc(insn->op()))
return false;

auto fallthrough = blk->nextb;
if (!fallthrough)
return false;

if (!fallthrough->nextb || !fallthrough->tail || mcode_is_flow(fallthrough->tail->op()))
	return false;

/*|| fallthrough->nextb != blk->mba->natural[insn->d.b] ||
fallthrough->predset.size() != 1*/

	if (insn->d.t == mop_v) {

	mblock_t* final_target = find_block_by_ea_start(blk->mba, insn->d.g);

	if (!final_target || fallthrough->nextb != final_target)
		return false;


	}
	else {
		if (fallthrough->nextb != blk->mba->natural[insn->d.b] ||
			fallthrough->predset.size() != 1)
			return false;
	}
mlist_t fidefs{};
mlist_t fiuse{};

unsigned nsetop = 0;
for (auto fi = fallthrough->head; fi; fi = fi->next) {
	if (!mcode_is_set(fi->op()) || fi->d.t != mop_r)
		return false;
	if (minsn_might_have_side_effects(fi))
		return false;

	add_mop_to_mlist(&fi->d, &fidefs, &blk->mba->vars);
	generate_use_for_insn(blk->mba, fi, &fiuse);
	//if (test_mop_in_mlist(&fi->d, &fiuse, &blk->mba->vars))
		//	return false;
	nsetop++;
}
if (nsetop > 15)
return false;

/*if (fiuse.reg.has_common(fidefs.reg)) {
	return false;
}*/
/*if (!blk->mustbdef.reg.includes(fidefs.reg))
	return false;*/


mreg_t temp_flags = allocate_tempreg_unused_in_block_range(blk, 16);
if (temp_flags == -1)return false;

unsigned mrpos = 0;

fixed_size_vector_t<minsn_t*, 16> setops_before_masking{};
fixed_size_vector_t<minsn_t*, 16> masking_ops{};

mreg_t selector_reg = temp_flags + 15;
mop_t selector_op{};
selector_op.make_reg(selector_reg, 1);

for (auto fi = fallthrough->head; fi; fi = fi->next, mrpos++) {
	minsn_t* setop = minsn_t::cloneptr(fi);
	setop->ea = insn->ea;
	setop->d.r = temp_flags + mrpos;

	setops_before_masking.push_back(setop);

	minsn_t* selector = new minsn_t(insn->ea);

	setup_flagged_bool_select(selector, &selector_op, &fi->d, &setop->d, insn->ea);
	selector->d = fi->d;

	masking_ops.push_back(selector);
}


insn->opcode = (mcode_t)jcc_to_setcc(insn->op());
insn->d.make_reg(selector_reg, 1);

for (auto&& setop : setops_before_masking) {
	blk->insert_into_block(setop, insn->prev);
}

for (auto&& maskop : masking_ops) {
	blk->insert_into_block(maskop, blk->tail);
}


minsn_t* gotoguy = new minsn_t(insn->ea);

gotoguy->opcode = m_goto;
if (has_cflow) {
	gotoguy->l.make_blkref(fallthrough->nextb->serial);

	blk->succset.del(fallthrough->serial);

	fallthrough->predset.del(blk->serial);
	for (auto fi = fallthrough->head; fi; fi = fi->next)
		fi->make_nop();
	fallthrough->flags |= MBL_INCONST;
}
else {
	gotoguy->l.make_gvar(fallthrough->nextb->start);
}

blk->insert_into_block(gotoguy, blk->tail);







blk->type = BLT_1WAY;

return true;

#endif

}


COMB_RULE_IMPLEMENT(x_minus_y_lt_zero_cmp) {

	auto insn = state->insn();
	if (insn->op() != m_and)
		return false;

	auto [subinsn1, subinsn2] = insn->arrange_by_insn(m_sets, m_setge);


}