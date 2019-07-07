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

				ccdef->l = tempreg;
			
			}
			return true;
		};
		/*
		if (test_any_submop_in_mlist(&l, &interdefs, lvars)) {

			mop_t tempreg{};
			
			mreg_t tr = find_unused_tr(ccdef->prev, i, l.size);

			if (tr == -1)
				return false;
			
			tempreg.make_reg(tr, l.size);
			
			insert_mov2_before(ccdef->ea, blk, ccdef, &l, &tempreg);

			ccdef->l = tempreg;

		}*/

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