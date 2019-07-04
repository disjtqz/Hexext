#include <ida.hpp>
#include <idp.hpp>
#include <loader.hpp>
#include <kernwin.hpp>
#include <ieee.h>
#include <hexrays.hpp>
#include <functional>
#include <array>
#include <list>
#include <string>
#include "../cs_core.hpp"
#include <intrin.h>

#include "../cs_intrin.hpp"
#include "../micro_on_70.hpp"
#include "../hexutils/hexutils.hpp"

#include "../mvm/mvm.hpp"
#include "prepostprocess.hpp"

static inline mblock_t* extract_single_successor(mblock_t* block, bool* is_noret = nullptr, bool disallow_calls = false) {

	if (!block || !block->tail)
		return nullptr;

	auto tailop = block->tail->op();

	if (tailop == m_goto && block->tail->l.optype() == mop_v) {
		return find_block_by_ea_start(block->mba, block->tail->l.g);
	}

	else if (!mcode_is_flow(tailop)) {
		//fallthrough
		return block->nextb;
	}
	else if (!disallow_calls && (tailop == m_call || tailop == m_icall)) {
		if (tailop == m_call && block->tail->l.optype() == mop_v) {
			if (!func_does_return(block->tail->l.g)) {
				if(is_noret)
				*is_noret = true;
				return nullptr;
			}
		}
		return block->nextb;
	}
	return nullptr;
}

static inline mblock_t* resolve_goto_only_block(mblock_t* blk) {
	if (!blk)
		return nullptr;
	while (blk->head == blk->tail&& blk->head != nullptr && blk->tail->op() == m_goto && blk->tail->l.optype() == mop_v) {

		mblock_t* nt = find_block_by_ea_start(blk->mba, blk->tail->l.g);

		if (!nt)
			return blk;
	}
	return blk;

}

static inline bool is_ret_block(mblock_t* block) {
	return block && block->tail&& block->tail->op() == m_ret;
}

struct jcnd_extraction_t {
	mblock_t* input_block;
	mblock_t* consequent;
	mblock_t* alternate;
	mop_t* condition;
};



static inline bool extract_jcnd_ifelse(mblock_t* block, jcnd_extraction_t* jcnd) {
	if (!block || !block->tail || block->tail->op() != m_jcnd || block->tail->d.optype() != mop_v) {
		return false;
	}
	mblock_t* conseq = find_block_by_ea_start(block->mba, block->tail->d.g);

	if (!conseq || !block->nextb)
		return false;

	jcnd->input_block = block;
	jcnd->consequent = conseq;
	jcnd->alternate = block->nextb;
	jcnd->condition = &block->tail->l;
	return true;

//block->mba
}
/*
	we can assume an ea's diff from start ea will not exceed 32 bits
	otherwise its a hugeass program
*/
struct preopt_graph_node_t {
	enum preopt_node_flags_e {
		_preopt_node_is_noret = 1,
		_preopt_node_is_ret = 2,
		_preopt_node_self_loop = 4
	};
	std::set<unsigned> m_succ;
	std::set<unsigned> m_pred;

	unsigned m_flags;


	

	preopt_graph_node_t() : m_succ(), m_pred(), m_flags(0) {}
	void set_noret() {
		m_flags |= _preopt_node_is_noret;
	}

	void set_ret() {
		m_flags |= _preopt_node_is_ret;
	}

	//bool is_self_loop()

	bool is_noret() const {
		return m_flags & _preopt_node_is_noret;
	}

	bool is_ret() const {
		return m_flags & _preopt_node_is_ret;
	}
	
	auto& successors() {
		return m_succ;
	}
	

};

struct preopt_graph_t {
	mbl_array_t* m_mba;
	ea_t m_start;
	ea_t m_end;
	std::map<unsigned, preopt_graph_node_t> m_graph;

	std::map<unsigned, mblock_t*> m_blocks;

	preopt_graph_t() : m_mba(nullptr),m_start(BADADDR), m_end(BADADDR), m_graph() {

	}

	preopt_graph_t(mbl_array_t* mba) : m_mba(mba), m_graph() {
		m_start = mba->blocks->start;
		m_end = mba->natural[mba->qty - 1]->end;
	}

	unsigned encode_ea(ea_t ea) const {
		return (unsigned)(ea - m_start);
	}

	ea_t decode_ea(unsigned ea) const {
		return ((ea_t)ea) + m_start;
	}
	preopt_graph_node_t& get_node(ea_t ea) {
		return m_graph[encode_ea(ea)];
	}

	preopt_graph_node_t& get_node_enc(unsigned enc) {
		return m_graph[enc];
	}

	preopt_graph_node_t& get_node(mblock_t* blk) {
		return get_node(blk->start);
	}

	mblock_t* get_block_enc(unsigned encoded) {
		return m_blocks[encoded];
	}
	void link_successor(ea_t predaddr, ea_t succaddr) {

		preopt_graph_node_t& prednode = get_node(predaddr);
		preopt_graph_node_t& succnode = get_node(succaddr);

		prednode.m_succ.insert(encode_ea(succaddr));
		succnode.m_pred.insert(encode_ea(predaddr));
	}

	void add_block_to_graph(mblock_t* CS_RESTRICT block) {
		bool is_noret = false;
		mblock_t* single_succ = extract_single_successor(block, &is_noret);

		m_blocks[encode_ea(block->start)] = block;

		if (single_succ) {
			link_successor(block->start, single_succ->start);
			return;
		}
		if (is_noret) {
			get_node(block->start).set_noret();
			return;
		}
		if (is_ret_block(block)) {
			get_node(block->start).set_ret();
			return;
		}

		jcnd_extraction_t jcnd{};

		if (extract_jcnd_ifelse(block, &jcnd)) {

			link_successor(block->start, jcnd.alternate->start);
			link_successor(block->start, jcnd.consequent->start);
			return;
		}
	}

	void regenerate_graph() {
		m_graph.clear();

		for (auto&& mb : forall_mblocks(m_mba)) {
			add_block_to_graph(mb);
		}
	}

};



static bool preopt_elim_unused_flags(mbl_array_t* mba) {
	bool did_change = false;

	for (mblock_t* block = mba->blocks; block; block = block->nextb) {
	rerun:
		for (minsn_t* insn = block->head; insn; insn = insn->next) {

			if (insn->d.t == mop_r) {
				mreg_t r = insn->d.r;

				if (r == mr_cf || r == mr_zf || r == mr_pf || r == mr_sf || r == mr_of) {
					bool redef;
					/*
						if it has a use or is redefed continue
					*/
					if (find_next_mreg_use(insn->next, r, 1, &redef) || redef) {
						continue;
					}
					if (block->tail->op() == m_call || block->tail->op() == m_icall) {
						minsn_t* deadguy = block->remove_from_block(insn);

						delete insn;
						did_change = true;
						goto rerun;
					}

					if (block->tail->op() == m_goto && block->tail->l.optype() == mop_v) {
						mblock_t* targblock = find_block_by_ea_start(mba, block->tail->l.g);

						if (!targblock)
							continue;
						bool redef;
						if (redefs_mreg_without_use_or_does_not_use_and_returns(targblock, r, 1) || (!find_next_mreg_use(targblock->head, r, 1, &redef) && targblock->tail &&
							(targblock->tail->op() == m_call || targblock->tail->op() == m_icall))) {
							minsn_t* deadguy = block->remove_from_block(insn);

							delete insn;
							did_change = true;
							goto rerun;
						}

					}

					if (block->tail->op() == m_jcnd && block->tail->d.optype() == mop_v) {


						mblock_t* nextblock = block->nextb;
						mblock_t* targetblock = find_block_by_ea_start(mba, block->tail->d.g);

						if (!nextblock || !targetblock)
							continue;

						if (redefs_mreg_without_use_or_does_not_use_and_returns(nextblock, r, 1) && redefs_mreg_without_use_or_does_not_use_and_returns(targetblock, r, 1)) {
							minsn_t* deadguy = block->remove_from_block(insn);

							delete insn;
							did_change = true;
							goto rerun;
							//return true;
						}


					}

				}
			}

		}
	}
	return did_change;
}

static bool preopt_merge_goto_goto(mbl_array_t* mba) {
	bool did_change = false;

	for (mblock_t* block = mba->blocks; block; block = block->nextb) {
	rerun:
		auto tail = block->tail;

		
		if (!tail)
			continue;

		if (tail->op() != m_goto || tail->l.optype() != mop_v) {
			continue;
		}

		mblock_t* succ = find_block_by_ea_start(mba, tail->l.g);

		if (!succ)
			continue;

		auto succtail = succ->tail;

		if (!succtail)
			continue;

		if (succtail->op() == m_goto || succtail->op() == m_ret) {

			did_change = true;

			block->remove_from_block(tail);

			ea_t tailea = tail->ea;
			delete tail;

			for (minsn_t* targins = succ->head; targins; targins = targins->next) {

				minsn_t* newguy = new minsn_t(tailea);

				*newguy = *targins;

				newguy->ea = tailea;

				block->insert_into_block(newguy, block->tail);

			}
			goto rerun;



		}
	}
	return did_change;
}

static bool preopt_make_condbranch_that_falls_through_to_ret_condbranch_to_ret(mbl_array_t* mba) {
	bool did_change = false;
rerun:
	for (mblock_t* block = mba->blocks; block; block = block->nextb) {

		if (block->flags & MBL_FAKE)
			continue;
		minsn_t* tail = block->tail;

		if (!tail || tail->op() != m_jcnd || tail->d.t != mop_v)
			continue;

		mblock_t * jcnd_target = find_block_by_ea_start(mba, tail->d.g);

		if (!jcnd_target || (jcnd_target->flags & MBL_FAKE))
			continue;

		mblock_t * jcnd_fallthrough = block->nextb;

		if (!jcnd_fallthrough || (jcnd_fallthrough->flags & MBL_FAKE))
			continue;

		if (!jcnd_fallthrough->tail || jcnd_fallthrough->tail->op() != m_ret || jcnd_fallthrough->head->ea == tail->ea)
			continue;

		if (tail->l.t == mop_d && tail->l.d->opcode == m_lnot) {
			mop_t temp = tail->l.d->l;

			tail->l = temp;

		}
		else {
			minsn_t* new_jcnd_arg = new minsn_t(BADADDR);

			new_jcnd_arg->opcode = m_lnot;


			new_jcnd_arg->l = tail->l;


			tail->l.erase();
			tail->l.assign_insn(new_jcnd_arg, 1);

			new_jcnd_arg->d.size = 1;
		}
		ea_t old_target = tail->d.g;
		mblock_t* newblock = mba->create_new_block_for_preopt(block);

		tail->d.g = jcnd_fallthrough->start == BADADDR ? jcnd_fallthrough->head->ea : jcnd_fallthrough->start;
		tail->d.size = -1;

		//tail->d.make_blkref(jcnd_fallthrough->serial);





		minsn_t* newblockop = new minsn_t(tail->ea);
		newblockop->ea = tail->ea;
		newblockop->opcode = m_goto;


		mblock_t* oldtarg = find_block_by_ea_start(mba, old_target);

		//newblockop->l.make_blkref(oldtarg->serial);
		newblockop->l.make_gvar(old_target);
		newblockop->l.size = -1;

		newblock->maxbsp = block->maxbsp;
		newblock->minbargref = block->minbargref;
		newblock->minbstkref = block->minbstkref;

		newblock->insert_into_block(newblockop, nullptr);
		newblock->start = tail->ea;
		newblock->end = jcnd_fallthrough->start;
		newblock->flags |= 1024;//needs propagation
		//newblock->flags = jcnd_fallthrough->flags;
		//newblock->flags |= 0x200;

		did_change = true;
		goto rerun;
		//newblock->insert_into_block()




	}
	return did_change;
}


static bool merge_common_mov(mbl_array_t* mba) {
	bool did_change = false;

	auto dest_and_src_are_lvalue = [](minsn_t * i) {
		return i->d.is_lvalue() && i->l.is_lvalue();

	};
rerun:
	for (mblock_t* block = mba->blocks; block; block = block->nextb) {

		//if (block->flags & MBL_FAKE)
		//	continue;

		if (!block->tail || block->tail->op() != m_jcnd || block->tail->d.t != mop_v)
			continue;

		mblock_t * fallthrough = block->nextb;

		mblock_t * target = find_block_by_ea_start(mba, block->tail->d.g);

		/*if (target->flags & MBL_FAKE)
			continue;
		if (fallthrough->flags & MBL_FAKE)
			continue;*/
		if (!target || !fallthrough)
			continue;

		if (!target->head || !fallthrough->head)
			continue;

		minsn_t * targethead = target->head;
		minsn_t * fallthroughhead = fallthrough->head;

		if (targethead->op() != fallthroughhead->op() || targethead->op() != m_mov)
			continue;

		if (!dest_and_src_are_lvalue(targethead) || !dest_and_src_are_lvalue(fallthroughhead))
			continue;

		if (!targethead->d.lvalue_equal(&fallthroughhead->d) || !targethead->l.lvalue_equal(&fallthroughhead->l))
			continue;

		if (minsn_has_any_tempreg(targethead))
			continue;

		minsn_t * common_mov = new minsn_t(BADADDR);
		*common_mov = *targethead;

		common_mov->ea = block->tail->ea;

		block->insert_into_block(common_mov, block->tail->prev);

		target->remove_from_block(targethead);
		fallthrough->remove_from_block(fallthroughhead);

		delete targethead;
		delete fallthroughhead;
		did_change = true;
		goto rerun;
	}
	return did_change;
}




static bool merge_goto_to_nonflow_ending_block(mbl_array_t * mba) {
	bool did_change = false;
rerun:
	for (mblock_t* block = mba->blocks; block; block = block->nextb) {
		if (block->flags & MBL_FAKE)
			continue;

		if (!block->tail || block->tail->op() != m_goto || block->tail->l.t != mop_v)
			continue;

		ea_t gotoaddr = block->tail->l.g;

		mblock_t * target = find_block_by_ea_start(mba, gotoaddr);

		if (!target)
			continue;
	
		if (target->flags & MBL_FAKE)
			continue;


		if (!target->tail)
			continue;

		if (mcode_is_flow(target->tail->op()))
			continue;

		mblock_t * target_fallthrough = target->nextb;

		if (!target_fallthrough)
			continue;

		if (target_fallthrough->flags & MBL_FAKE)
			continue;

		if (!target_fallthrough->head || target_fallthrough->head->ea == target->tail->ea)
			continue;

		block->tail->l.g = target_fallthrough->head->ea;

		for (minsn_t* insn = target->head; insn; insn = insn->next) {
			minsn_t* dup = new minsn_t(BADADDR);
			*dup = *insn;
			dup->ea = block->tail->ea;

			block->insert_into_block(dup, block->tail->prev);

		}
		did_change = true;
		goto rerun;

	}
	return did_change;
}


static bool shift_down_regdef_that_reaches_common_fallthrough(mbl_array_t* mba, preopt_graph_t& graph) {
	bool didchange = false;

	for (auto&& blk : forall_mblocks(mba)) {

		//jcnd_extraction_t jcnd{};
		//if (!extract_jcnd_ifelse(blk, &jcnd))
		//	continue;


		if (!blk->tail || blk->tail->op() != m_jcnd)
			continue;

		auto& node = graph.get_node(blk);

		auto& succ = node.successors();
		//eugh
		auto firstsucc = *succ.begin();
		auto secsucc = *(succ.begin().operator++());
		mblock_t* consucc = graph.get_block_enc(firstsucc);
			
			//extract_single_successor(jcnd.consequent, nullptr, true);

		mblock_t* altsucc = graph.get_block_enc(secsucc);


		auto& connode = graph.get_node_enc(firstsucc);
		auto& altnode = graph.get_node_enc(secsucc);

		if (connode.successors().size() != 1 || altnode.successors().size() != 1 || *connode.successors().begin() != *altnode.successors().begin())
			continue;

			
			// extract_single_successor(jcnd.alternate, nullptr, true);

	//	consucc = resolve_goto_only_block(consucc);
	//	altsucc = resolve_goto_only_block(altsucc);
	


		auto r2rs = gather_reg2reg_movs_with_no_use_or_redef(blk);

		if (!r2rs.size())
			continue;

		std::vector<minsn_t* > common_unuseds{};
		common_unuseds.reserve(r2rs.size());
		for (auto&& r2r : r2rs) {
			if (
				block_does_not_use_or_redef_mreg(consucc, r2r->d.r, r2r->d.size)
				&& block_does_not_use_or_redef_mreg(consucc, r2r->l.r, r2r->l.size)
				&& block_does_not_use_or_redef_mreg(altsucc, r2r->d.r, r2r->d.size)
				&& block_does_not_use_or_redef_mreg(altsucc, r2r->l.r, r2r->l.size)
				) {
				common_unuseds.push_back(r2r);
			}
		}
		if (!common_unuseds.size())
			continue;

		for (auto&& unused_r2r : common_unuseds) {
			blk->remove_from_block(unused_r2r);
			unused_r2r->ea = consucc->head->ea;
			consucc->insert_into_block(unused_r2r, nullptr);
		}
		didchange = true;
	}
	return didchange;
}
static bool shift_down_regdef_that_is_unused(mbl_array_t* mba, preopt_graph_t& graph) {
	bool didchange = false;
	for (auto&& blk : forall_mblocks(mba)) {
		if (!blk->tail || blk->flags & MBL_FAKE)
			continue;
		//auto fallthrough = extract_single_successor(blk, nullptr, true);
		auto& node = graph.get_node(blk);

		if (node.successors().size() != 1 || blk->tail->op() == m_call || blk->tail->op() == m_icall)
			continue;

		auto& fallthrough_node = graph.get_node_enc(*node.successors().begin());

		if (fallthrough_node.m_pred.size() != 1)
			continue;

		mblock_t* fallthrough = graph.get_block_enc(*node.successors().begin());

		if (!fallthrough || !fallthrough->head || fallthrough->flags & MBL_FAKE) {
			continue;
		}

		auto r2r = gather_reg2reg_movs_with_no_use_or_redef(blk);

		if (!r2r.size())
			continue;

		for (auto&& r2 : r2r) {
			if (minsn_has_any_tempreg(r2))
				continue;
			blk->remove_from_block(r2);
			r2->ea = fallthrough->head->ea;

			fallthrough->insert_into_block(r2, nullptr);

		}
		didchange = true;
	}
	return didchange;
}

static bool run_hoisting_pass(mbl_array_t* mba) {
	unsigned didchange = 0;
	bool didchangeatall = false;
	preopt_graph_t graph{ mba };
	graph.regenerate_graph();
	do {
		didchange = 0;
		//didchange |= preopt_elim_unused_flags(mba);
		/*
			cant do this until i have all tempreg for all archs :(
			if a tempreg crosses bb boundaries hexrays will freak the fuck out
		*/

		//we got em all now
		//nah terrible performance
#if 0
		didchange |= merge_common_mov(mba);

		didchange |= shift_down_regdef_that_is_unused(mba, graph);
		//didchange |= shift_down_regdef_that_reaches_common_fallthrough(mba, graph);
#endif
		if (didchange) {
			didchangeatall = true;
		}
	
	} while (didchange != 0);
	return didchangeatall;
}

bool prepostprocess_run(mbl_array_t* mba) {
	bool didchange = false; 
	bool didchangeatall = false;

	bool has_any_jtbl = false;

	for (auto&& blk : forall_mblocks(mba)) {

		if (blk->tail && blk->tail->op() == m_jtbl) {
			has_any_jtbl = true;
			break;
		}
	}

	do {
		didchange = false;
		didchange |= run_hoisting_pass(mba);

		if (preopt_merge_goto_goto(mba)) {
			didchange = true;
			run_hoisting_pass(mba);
		}
		

		/*
			i crai when control flow optimizations deserve to DIE

			no this'll be revived eventually, i just need to figure out why its causing default cases in switch tables to also turn into jmpouts
			even with the bbidx fixup
		*/

	/*	if (!has_any_jtbl && merge_goto_to_nonflow_ending_block(mba)) {
			didchange = true;
		}*/
		didchangeatall |= didchange;
	} while (didchange);

	return didchangeatall;
}