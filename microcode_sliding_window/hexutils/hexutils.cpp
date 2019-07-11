
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
#include "../micro_on_70.hpp"
#include <intrin.h>
#include "../mvm/mvm.hpp"
#include "hexutils.hpp"



static bool is_sign_sar(mop_t* mop) {
	return mop->t == mop_d && mop->d->opcode == m_sar && mop->d->r.is_equal_to((mop->d->l.size * 8) - 1, false);
}


bool try_extract_mulp2_by_const(minsn_t* insn, mulp2_const_t* out) {

	
	
	minsn_t* shlins = insn;
	uint64_t shlval;
	
	mop_t* nonconstptr;
	mop_t* constvalptr;
	
	out->m_targetinsn = insn;

	if (shlins->opcode != m_shl) {
		if (shlins->opcode == m_mul) {

			mop_t* constval = shlins->get_eitherlr(mop_n, &nonconstptr);

			if (!constval)
				return false;

			constvalptr = constval;
			uint64_t v = constval->nnn->value;

			if (__popcnt64(v) == 1) {
				out->m_is_mul = true;
				shlval = _tzcnt_u64(v);
			}
			else {
				return false;
			}
		}
		else
			return false;
	}
	else {

		if (shlins->r.t != mop_n)
			return false;

		constvalptr = &shlins->r;
		nonconstptr = &shlins->l;
		shlval = shlins->r.nnn->value;
	}

	
	out->m_actual_insn = insn;
	out->m_shiftcount = shlval;
	out->m_numop = constvalptr;
	out->m_multiplicand = nonconstptr;
	return true;
}

bool try_extract_udivp2_by_const(minsn_t* insn, udivp2_const_t* out) {



	minsn_t* shlins = insn;
	uint64_t shlval;

	mop_t* nonconstptr;
	mop_t* constvalptr;

	out->m_targetinsn = insn;

	if (shlins->opcode != m_shr) {
		if (shlins->opcode == m_udiv) {

			mop_t* constval = &shlins->r;
			nonconstptr = &shlins->l;


			if (constval->t != mop_n)
				return false;

			constvalptr = constval;
			uint64_t v = constval->nnn->value;

			if (__popcnt64(v) == 1) {
				out->m_is_udiv = true;
				shlval = _tzcnt_u64(v);
			}
			else {
				return false;
			}
		}
		else
			return false;
	}
	else {

		if (shlins->r.t != mop_n)
			return false;

		constvalptr = &shlins->r;
		nonconstptr = &shlins->l;
		shlval = shlins->r.nnn->value;
	}


	out->m_actual_insn = insn;
	out->m_shiftcount = shlval;
	out->m_numop = constvalptr;
	out->m_numerator = nonconstptr;
	return true;
}


bool try_extract_xdu(mop_t* mop, xdu_extraction_t* out) {

	if (mop->t != mop_d) {
		return false;
	}
	return try_extract_xdu(mop->d, out);
}
bool try_extract_xdu(minsn_t* mop, xdu_extraction_t* out) {
	auto ins = mop;

	if (ins->opcode != m_xdu)
		return false;

	out->m_extended_op = &ins->l;

	out->m_fromsize = ins->l.size;
	out->m_tosize = ins->d.size;
	out->m_targeti = mop;
	out->m_tosize_known = 1;

	return true;
}



bool try_extract_mul_by_constant(minsn_t* CS_RESTRICT insn, mul_by_constant_extraction_t* CS_RESTRICT out) {
	if (insn->opcode == m_mul) {
		mop_t* nonconst;
		mop_t* constie = insn->get_eitherlr(mop_n, &nonconst);

		if (!constie) {
			return false;
		}

		out->m_is_shift = false;
		out->m_constant = constie;
		out->m_mulinsn = insn;
		out->m_mutable = nonconst;
		out->m_mulvalue = constie->nnn->value;

		return true;
	}
	else if (insn->opcode == m_shl) {

		if (insn->r.t != mop_n) {
			return false;
		}
		out->m_is_shift = true;
		out->m_constant = &insn->r;

		out->m_mulinsn = insn;
		out->m_mutable = &insn->l;
		out->m_mulvalue = extend_value_by_size_and_sign(1ULL << insn->r.nnn->value, insn->l.size, false);
		
		return true;
		
	}
	else {
		return false;
	}
}

bool try_extract_udiv_by_constant(minsn_t* CS_RESTRICT insn, udiv_by_constant_extraction_t* CS_RESTRICT out) {

	if (insn->opcode == m_udiv) {
		if (insn->r.t != mop_n) {
			return false;
		}
		out->m_is_shift = false;
		out->m_constant = &insn->r;
		out->m_divinsn = insn;
		out->m_mutable = &insn->l;
		out->m_divvalue = insn->r.nnn->value;
		return true;
	}
	else if (insn->opcode == m_shr) {
		if (insn->r.t != mop_n) {
			return false;
		}
		out->m_is_shift = true;
		out->m_constant = &insn->r;
		out->m_divinsn = insn;
		out->m_mutable = &insn->l;
		out->m_divvalue = extend_value_by_size_and_sign(1ULL << insn->r.nnn->value, insn->l.size, false);
		return true;
	}
	else {
		return false;
	}
}
static constexpr unsigned g_oplut_length = mop_last;
using oplut_t = uint16_t;

static constexpr void init_oplut(oplut_t & lut) {

}

template<mopt_t op, mopt_t... ops>
static constexpr void oplut_make_impl(oplut_t* lut) {
	*lut |= 1 << (op & 0x1F);
	if constexpr (sizeof...(ops) != 0) {
		oplut_make_impl<ops...>(lut);
	}
	else {

	}
}

template<mopt_t... ops>
static constexpr oplut_t make_oplut() {
	oplut_t result = 0;
	oplut_make_impl<ops...>(&result);
	return result;
}

static inline bool instruction_set_is_member(uint16_t lut, mopt_t op) {
	const long v = lut;

	return _bittest(&v, op);
}

static constexpr oplut_t lut_is_mlist_addable = make_oplut<mop_r, mop_S, mop_l, mop_v>();

static inline void add_mop_to_mlist_inline(mop_t* CS_RESTRICT mop, mlist_t* CS_RESTRICT mlist, lvars_t* CS_RESTRICT lvars) {
	auto t = mop->t;
	if (!instruction_set_is_member(lut_is_mlist_addable, t))
		return;
	
	unsigned sz = mop->size;
	if (t == mop_r) {
		mlist->reg.add_(mop->r, sz);
	}
	else if (t == mop_S) {
		ivl_t v{ mop->s->off , sz};

		mlist->mem.add(v);
	}
	else if (t == mop_l) {
		lvar_ref_t* CS_RESTRICT ref = mop->l;
		unsigned off = ref->off;
		lvar_t& CS_RESTRICT lvar = (*lvars)[ref->idx];

		if (lvar.is_reg_var()) {
			mlist->reg.add_(lvar.get_reg1() + off, sz);
		}
		else if (lvar.is_stk_var()) {
			ivl_t v{ lvar.location.stkoff(), sz};
		
			mlist->mem.add(v);
		}

	}
	else if (t == mop_v) {
		ivl_t v{ mop->g, sz };

		mlist->mem.add(v);
	}

}
void add_mop_to_mlist(mop_t* CS_RESTRICT mop, mlist_t* CS_RESTRICT mlist, lvars_t* CS_RESTRICT lvars) {
	add_mop_to_mlist_inline(mop, mlist, lvars);
}
static inline bool test_mop_in_mlist_local(mop_t* CS_RESTRICT mop, mlist_t* CS_RESTRICT mlist, lvars_t* CS_RESTRICT lvars) {
	auto t = mop->t;
	if (!instruction_set_is_member(lut_is_mlist_addable, t))
		return false;

	unsigned sz = mop->size;
	if (t == mop_r) {
		//mlist->reg.add_(mop->r, sz);
		return mlist->reg.has_any(mop->r, sz);
	}
	else if (t == mop_S) {
		ivl_t v{ mop->s->off , sz };

	//	mlist->mem.add(v);
		return mlist->mem.has(v);
	}
	else if (t == mop_l) {
		lvar_ref_t* CS_RESTRICT ref = mop->l;
		unsigned off = ref->off;
		lvar_t& CS_RESTRICT lvar = (*lvars)[ref->idx];

		if (lvar.is_reg_var()) {
			//mlist->reg.add_(lvar.get_reg1() + off, sz);
			return mlist->reg.has_any(lvar.get_reg1() + off, sz);
		}
		else if (lvar.is_stk_var()) {
			ivl_t v{ lvar.location.stkoff(), sz };
			return mlist->mem.has(v);
			//mlist->mem.add(v);
		}
		else {
			cs_assert(false);
		}
	}
	else if (t == mop_v) {
		ivl_t v{ mop->g, sz };

		//mlist->mem.add(v);
		return mlist->mem.has(v);
	}
	return false;
}

bool test_mop_in_mlist(mop_t* CS_RESTRICT mop, mlist_t* CS_RESTRICT mlist, lvars_t* CS_RESTRICT lvars) {
	return test_mop_in_mlist_local(mop, mlist, lvars);
}

struct gen_use_traversal_traits {
	static constexpr unsigned AUXSTACK_SIZE = 128;
	static constexpr bool maintain_context_ptrs = false;
	static constexpr bool may_term_flow = false;
};

struct test_submop_trav {
	static constexpr unsigned AUXSTACK_SIZE = 64;
	static constexpr bool maintain_context_ptrs = false;
	static constexpr bool may_term_flow = true;
};
bool test_any_submop_in_mlist(mop_t* CS_RESTRICT mop, mlist_t* CS_RESTRICT mlist, lvars_t* CS_RESTRICT lvars) {
	
	bool res = false;
	traverse_micro < test_submop_trav>(mop, [&res, mlist, lvars](mop_t * m) {
		if (test_mop_in_mlist(m, mlist, lvars)) {
			res = true;
			return true;
			}
		return false;
		});

	return res;

}
void generate_use_for_insn(mbl_array_t* mba, minsn_t* insn, mlist_t* CS_RESTRICT mlist) {

	lvars_t* CS_RESTRICT lvars = &mba->vars;


	traverse_minsn<gen_use_traversal_traits>(insn, [mlist, mba, lvars](mop_t * mop) {
		add_mop_to_mlist_inline(mop, mlist, lvars);
	});
}

void generate_def_for_insn(mbl_array_t* mba, minsn_t* insn, mlist_t* CS_RESTRICT mlist) {
	add_mop_to_mlist_inline(&insn->d, mlist, &mba->vars);

	traverse_minsn<gen_use_traversal_traits>(insn, [mlist](mop_t * mop) {
		if (mop->t != mop_f)
			return;

		auto cl = mop->f;

		mlist->mem.add_(&cl->spoiled.mem);
		mlist->reg.add(cl->spoiled.reg);


	});
}



/*
	Will return true if it did not exceed the max size of the fixed size array that was passed in by the user
*/
bool gather_user_subinstructions(lvars_t* lvars,minsn_t* insn, mlist_t* CS_RESTRICT list, fixed_size_vecptr_t<minsn_t*> into) {

	bool did_exceed_size = false;
	traverse_minsn< gen_use_traversal_traits>(insn, [&did_exceed_size, list, into, lvars](mop_t * mop) {
		if (mop->t != mop_d) {
			return;
		}

		minsn_t*  d = mop->d;

		if (test_mop_in_mlist_local(&d->l, list, lvars) || test_mop_in_mlist_local(&d->r, list, lvars) ) {
	do_the_deed:
			did_exceed_size |= !(into->push_back(d));
			return;
		}


		if (d->d.t == mop_f) {

			for (auto&& arg : d->d.f->args) {
				if (test_mop_in_mlist_local(&arg, list, lvars)) {
					goto do_the_deed;
				}
			}
		}



	});

	return !did_exceed_size;


}





minsn_t* find_definition_backwards(mblock_t* CS_RESTRICT blk, minsn_t* CS_RESTRICT insn, mlist_t* CS_RESTRICT mlist) {
	minsn_t* curr;
	if (insn == nullptr)
		insn = blk->tail;

	else {
		insn = insn->prev;
	}
	curr = insn;

	if (!curr)
		return nullptr;
	mlist_t l{};
	for (; curr; curr = curr->prev) {
		
		generate_def_for_insn(blk->mba, curr, &l);

		if (l.mem.has_common_(&mlist->mem) || l.reg.has_common(mlist->reg)) {
			return curr;
		}
	}
	return nullptr;
}

minsn_t* find_redefinition(mblock_t* CS_RESTRICT blk, minsn_t* CS_RESTRICT insn, mlist_t* CS_RESTRICT mlist) {
	minsn_t* curr;
	if (!insn)
		curr = blk->head;
	else
		curr = insn->next;
	mlist_t l{};
	for (; curr; curr = curr->next) {

		generate_def_for_insn(blk->mba, curr, &l);

		if (l.mem.has_common_(&mlist->mem) || l.reg.has_common(mlist->reg)) {
			return curr;
		}
	}
	return nullptr;
}

minsn_t* find_next_use(mblock_t* blk, minsn_t* insn, mlist_t* mlist, bool* redefed ) {
	minsn_t* curr = insn->next;
	mlist_t l{};
	mlist_t def{};
	if(redefed)
	*redefed = false;
	for (; curr; curr = curr->next) {

		generate_use_for_insn(blk->mba, curr, &l);

		if (l.mem.has_common_(&mlist->mem) || l.reg.has_common(mlist->reg)) {
			return curr;
		}

		generate_def_for_insn(blk->mba, curr, &def);

		if (def.mem.has_common_(&mlist->mem) || def.reg.has_common(mlist->reg))
		{
			if (redefed)
			*redefed = true;
			return nullptr;
		}

	}
	return nullptr;
}
minsn_t* find_prev_use(mblock_t* blk, minsn_t* insn, mlist_t* mlist, bool* defed) {
	minsn_t* curr = insn->prev;
	mlist_t l{};
	mlist_t def{};
	*defed = false;
	for (; curr; curr = curr->prev) {
		//gotta check for def first
		generate_def_for_insn(blk->mba, curr, &def);

		if (def.mem.has_common_(&mlist->mem) || def.reg.has_common(mlist->reg))
		{
			*defed = true;
			return nullptr;
		}

		generate_use_for_insn(blk->mba, curr, &l);

		if (l.mem.has_common_(&mlist->mem) || l.reg.has_common(mlist->reg)) {
			return curr;
		}

	
		
	}
	return nullptr;
}
minsn_t* find_intervening_use(mblock_t* blk, minsn_t* insn1, minsn_t* insn2, mlist_t* mlist) {
	minsn_t* curr = insn1->next;
	mlist_t l{};
	for (; curr != insn2; curr = curr->next) {

		generate_use_for_insn(blk->mba, curr, &l);

		if (l.mem.has_common_(&mlist->mem) || l.reg.has_common(mlist->reg)) {
			return curr;
		}
	}
	return nullptr;
}

template<typename bitset_type>
CS_NOINLINE
static bool find_definition_size_impl(mblock_t* block, minsn_t* insn, unsigned* size_out, mlist_t* lst, bitset_type* CS_RESTRICT visited) {
	visited->add(block->serial);



	minsn_t* definition = find_definition_backwards(block, insn, lst);

	if (!definition) {
		int defsize = -1;

		if (block->predset.size() == 0)
			return false;
		for (auto&& predidx : block->predset) {

			if (visited->has(predidx) ) {
				return false;
			}
			else {

				unsigned size;
				mblock_t* predblock = block->mba->natural[predidx];

				if (!find_definition_size_impl(predblock, predblock->tail, &size, lst, visited)) {
					return false;
				}
				if (defsize == -1) {
					defsize = size;
				}
				else {
					/*
						inbound definition has different sizes
					*/
					if (defsize != size) {
						return false;
					}
				}

			}

		}

		*size_out = defsize;

		return true;

	}

	else {
		/*
			ignore implicit zero extensions from x86_64 ops
		*/
		if (definition->opcode == m_xdu && definition->d.size == 8) {
			*size_out = definition->l.size;
		}
		else
			*size_out = definition->d.size;
		return true;
	}
}
bool find_definition_size(bitset_t* visited_pool, mblock_t* block, minsn_t* insn, unsigned* size_out, mop_t* mreg) {

	mlist_t lst{};
	add_mop_to_mlist_inline(mreg, &lst, &block->mba->vars);

	//std::set<int> visited{};

	auto qty = block->mba->qty;
	if (qty < 64) {
		bitset64_t tiny{};

		return find_definition_size_impl(block, insn, size_out, &lst, &tiny);
	}
	else if (qty < 128) {
		bitset128_t tiny{};

		return find_definition_size_impl(block, insn, size_out, &lst, &tiny);
	}
	else {
		visited_pool->clear();
		return find_definition_size_impl(block, insn, size_out, &lst, visited_pool);
	}
}


struct locate_mreg_traversal_traits {
	static constexpr unsigned AUXSTACK_SIZE = 64;
	static constexpr bool maintain_context_ptrs = false;
	static constexpr bool may_term_flow = true;
};


mop_t* locate_first_mreg_use_in_insn(minsn_t* insn, mreg_t mreg, unsigned mreg_size) {

	mop_t* result = nullptr;

	traverse_minsn<locate_mreg_traversal_traits>(insn, [&result, mreg, mreg_size](mop_t * CS_RESTRICT mop) {
		
		if (mop->t == mop_f) {
			mcallinfo_t* callinf = mop->f;

			if (callinf->pass_regs.reg.has_any(mreg, mreg_size)) {
				result = mop;
				return true;
			}

			if (callinf->spoiled.reg.has_any(mreg, mreg_size)) {
				return true;//redefeddd
			}
		}
		
		if (mop->t != mop_r)
			return false;

		unsigned mopreg = mop->r;
		unsigned mopsz = mop->size;


		if (interval::includes(mreg, mreg_size, mopreg, mopsz) || interval::includes(mopreg, mopsz, mreg, mreg_size)) {
			result = mop;
			return true;
		}



		return false;

		});

	return result;
}
mop_t* locate_mreg_def_in_insn(minsn_t* insn, mreg_t mreg, unsigned mreg_size) {

	mop_t* result = nullptr;

	if (insn->d.t == mop_r && interval::includes(mreg, mreg_size, insn->d.r, insn->d.size) || interval::includes(insn->d.r, insn->d.size, mreg, mreg_size)) {
		result = &insn->d;
	}


	return result;
}
mblock_t* find_block_by_ea_start(mbl_array_t* CS_RESTRICT mba, ea_t ea) {

	for (mblock_t* CS_RESTRICT blk = mba->blocks; blk; blk = blk->nextb) {

		if (blk->start == ea) {
			return blk;
		}
	}

	return nullptr;
}



bool redefs_mreg_without_use(mblock_t* CS_RESTRICT block, mreg_t mreg, unsigned mreg_size) {
	for (minsn_t* CS_RESTRICT insn = block->head; insn; insn = insn->next) {
		if (locate_first_mreg_use_in_insn(insn, mreg, mreg_size)) {
			return false;
		}

		if (locate_mreg_def_in_insn(insn, mreg, mreg_size)) {
			return true;
		}
	}
	return false;
}

bool redefs_mreg_without_use_or_does_not_use_and_returns(mblock_t* CS_RESTRICT block, mreg_t mreg, unsigned mreg_size) {
	if (redefs_mreg_without_use(block, mreg, mreg_size)) {
		return true;
	}
	bool redef;

	if (block->tail && block->tail->op() == m_ret && !find_next_mreg_use(block->head, mreg, mreg_size, &redef)) {
		return true;
	}

	return false;
}

minsn_t* find_next_mreg_use(minsn_t* CS_RESTRICT start, mreg_t mreg, unsigned mreg_size, bool* redefed) {


	*redefed = false;

	for (; start; start = start->next) {
		if (locate_first_mreg_use_in_insn(start, mreg, mreg_size)) {
			return start;
		}
		if (locate_mreg_def_in_insn(start, mreg, mreg_size)) {
			*redefed = true;
			return nullptr;
		}
	}
	return nullptr;
}

minsn_t* find_prev_mreg_use(minsn_t* CS_RESTRICT start, mreg_t mreg, unsigned mreg_size, bool* redefed) {


	*redefed = false;

	for (; start; start = start->prev) {
		if (locate_first_mreg_use_in_insn(start, mreg, mreg_size)) {
			return start;
		}
		if (locate_mreg_def_in_insn(start, mreg, mreg_size)) {
			*redefed = true;
			return nullptr;
		}
	}
	return nullptr;
}
minsn_t* find_mreg_def_backwards(minsn_t* CS_RESTRICT start, mreg_t mreg, unsigned mreg_size) {

	for (; start; start = start->prev) {

		if (locate_mreg_def_in_insn(start, mreg, mreg_size)) {
			return start;
		}
	}
	return nullptr;
}

std::set<minsn_t*> gather_reg2reg_movs_with_no_use_or_redef(mblock_t* blk) {
	std::set<minsn_t*> result{};


	for (auto insn = blk->head; insn; insn = insn->next) {
		if (insn->op() != m_mov) {
			continue;
		}

		if (insn->l.t != mop_r || insn->d.t != mop_r)
			continue;
		bool redef_d = false;
		//if has a use in the block, or a redefinition in the block, continue
		if (find_next_mreg_use(insn->next, insn->d.r, insn->d.size, &redef_d) || redef_d) {
			continue;
		}
		
		if (find_next_mreg_use(insn->next, insn->l.r, insn->l.size, &redef_d) || redef_d)
			continue;


		result.insert(insn);
		//result.push_back(insn);
	}

	return result;
}

bool block_does_not_use_or_redef_mreg(mblock_t* blk, mreg_t mr, unsigned size) {
	bool redef_d = false;
	if (find_next_mreg_use(blk->head, mr, size, &redef_d) || redef_d) {
		return false;
	}
	return true;
}


bool extract_stx_displ(minsn_t* stxinsn, mreg_t* segreg, mop_t** base, mop_t** displ, mop_t** value) {

	if (stxinsn->op() != m_stx)
		return false;
	//selector must be reg
	if (stxinsn->r.optype() != mop_r)
		return false;

	//if (stxinsn->l.optype() != mop_r)
	//	return false;

	*segreg = stxinsn->r.r;
	*value = &stxinsn->l;

	if (stxinsn->d.t == mop_r) {
		*base = &stxinsn->d;
		*displ = nullptr;
		return true;
	}
	else {
		if (stxinsn->d.t != mop_d)
			return false;

		if (stxinsn->d.d->op() != m_add) {
			return false;
		}
		mop_t* non_regguy;
		mop_t* regguy = stxinsn->d.d->get_eitherlr(mop_r, &non_regguy);
		if (!regguy || non_regguy->t != mop_n)
			return false;

		*base = regguy;
		*displ = non_regguy;
		return true;
	}
}
// ldx  {l=sel,r=off}, d 

bool extract_ldx_displ(minsn_t* stxinsn, mreg_t* segreg, mop_t** base, mop_t** displ) {

	if (stxinsn->op() != m_ldx)
		return false;
	//selector must be reg
	//if (stxinsn->r.optype() != mop_r)
	//	return false;

	if (stxinsn->l.optype() != mop_r)
		return false;

	*segreg = stxinsn->l.r;
	//*value = &stxinsn->l;

	if (stxinsn->r.t == mop_r) {
		*base = &stxinsn->r;
		*displ = nullptr;
		return true;
	}
	else {
		if (stxinsn->r.t != mop_d)
			return false;

		if (stxinsn->r.d->op() != m_add) {
			return false;
		}
		mop_t* non_regguy;
		mop_t* regguy = stxinsn->r.d->get_eitherlr(mop_r, &non_regguy);
		if (!regguy || non_regguy->t != mop_n)
			return false;

		*base = regguy;
		*displ = non_regguy;
		return true;
	}
}

bool try_extract_equal_stx_dests(minsn_t* x, minsn_t* y, mop_t** value_outx, mop_t** value_outy) {
	mreg_t segregx;
	mop_t* basex;
	mop_t* displx;
	mop_t* valuex;

	if (!extract_stx_displ(x, &segregx, &basex, &displx, &valuex))
		return false;

	mreg_t segregy;
	mop_t* basey;
	mop_t* disply;
	mop_t* valuey;

	if (!extract_stx_displ(y, &segregy, &basey, &disply, &valuey))
		return false;

	if (segregx != segregy)
		return false;

	if (displx != nullptr) {
		if (disply == nullptr)
			return false;
		if (disply->nnn->value != displx->nnn->value)
			return false;
	}
	else if (disply != nullptr) {
		return false;
	}

	if (!basey->lvalue_equal(basex))
		return false;

	*value_outx = valuex;
	*value_outy = valuey;
	return true;

}

bool equal_ldx_srcs(minsn_t* x, minsn_t* y) {
	mreg_t segregx;
	mop_t* basex;
	mop_t* displx;


	if (!extract_ldx_displ(x, &segregx, &basex, &displx))
		return false;

	mreg_t segregy;
	mop_t* basey;
	mop_t* disply;


	if (!extract_ldx_displ(y, &segregy, &basey, &disply))
		return false;

	if (segregx != segregy)
		return false;

	if (displx != nullptr) {
		if (disply == nullptr)
			return false;
		if (disply->nnn->value != displx->nnn->value)
			return false;
	}
	else if (disply != nullptr) {
		return false;
	}

	if (!basey->lvalue_equal(basex))
		return false;

	return true;

}

bool ldx_src_equals_stx_dest(minsn_t* ldx, minsn_t* sty) {
	mreg_t segregx;
	mop_t* basex;
	mop_t* displx;
	

	if (!extract_ldx_displ(ldx, &segregx, &basex, &displx))
		return false;

	mreg_t segregy;
	mop_t* basey;
	mop_t* disply;
	mop_t* valuey;

	if (!extract_stx_displ(sty, &segregy, &basey, &disply, &valuey))
		return false;

	if (segregx != segregy)
		return false;

	if (displx != nullptr) {
		if (disply == nullptr)
			return false;
		if (disply->nnn->value != displx->nnn->value)
			return false;
	}
	else if (disply != nullptr) {
		return false;
	}

	if (!basey->lvalue_equal(basex))
		return false;

	return true;

}

bool minsn_has_any_tempreg(minsn_t* insn) {
	bool result = false;
	traverse_minsn< gen_use_traversal_traits>(insn, [&result](mop_t * mop) {
		if (mop->t != mop_r)
			return;
		if (rlist_tempregs.has_any(mop->r, mop->size)) {
			result = true;
		}
		});
	return result;
}
bool get_static_value(mop_t* v, uint64_t* out) {

	if (v->t != mop_v)
		return false;
	ea_t g = v->g;

	switch (v->size) {
	case 1:
		*out = get_byte(g);
		break;
	case 2:
		*out = get_word(g);
		break;
	case 4:
		*out = get_dword(g);
		break;
	case 8:
		*out = get_qword(g);
		break;
	default:
		return false;
	}
	return true;
}
/*
returns false if a redef was reached or if we hit the max size of the fixed vec
to indicate the path must end
*/
static bool gather_single_block_uses(fixed_size_vecptr_t<minsn_t*> uses, minsn_t* start, mblock_t* blk, mlist_t* list, bool prior = false) {

	minsn_t* pos = start;
	bool redefed = false;
	
	while ((pos = (prior ? find_prev_use : find_next_use)(blk, pos, list, &redefed)) && !redefed) {
		if (!uses->push_back(pos)) {
			return false;
		}
	}

	return redefed;
}
template<typename T>
struct usage_path_gatherer_tmpl_t {


	/*
		returns true if we all good
		returns false if we exceeded our fixed size array and need to terminate
	*/
	CS_NOINLINE
	static bool gather_uses_in_path(fixed_size_vecptr_t<minsn_t*> uses,
		/*
			fixed size vector, intended to be reused
			acts as a set of visited basic blocks
		*/
		T visited_pool,
		mblock_t* blk,
		mlist_t* list,
		bool prior,
		minsn_t* start = nullptr) {

		if (start)
			if (!gather_single_block_uses(uses, start, blk, list, prior))
				return true;

		for (auto&& succ : prior ? blk->predset : blk->succset ) {

			if (visited_pool->has(succ)) {
				continue;
			}
			visited_pool->add(succ);
			//std::make_heap(visited_pool->begin(), visited_pool->end());
			//gather uses in path only returns false if we exceeded the pool
			auto nblk = blk->mba->natural[succ];

			if (!gather_uses_in_path(uses, visited_pool, nblk, list, prior, nblk->head)) {
				return false;
			}

		}

		return true;

	}
};
//used if bb indexes are less than 256
using usage_gatherer_bblt64_t = usage_path_gatherer_tmpl_t<bitset64_t*>;

//used if they're less than 65536 (basically all of them should be)
using usage_gatherer_bblt128_t = usage_path_gatherer_tmpl_t<bitset128_t*>;
//big ass function mayne
using usage_gatherer_default_t = usage_path_gatherer_tmpl_t<bitset_t*>;


bool gather_uses(fixed_size_vecptr_t<minsn_t*> uses,

	bitset_t* visited_pool,
	minsn_t* start,
	mblock_t* blk,
	mlist_t* list, bool prior) {
	bool redefed = false;
	
	
	/*if (!gather_single_block_uses(uses, start, blk, list))
		return;

	

	if (!blk->succset.size())
		return;*/



	//gather_uses_in_path(uses, visited_pool, blk, list, start);

	//visited_pool->reset();

	

	mbl_array_t* mba = blk->mba;

	if (mba->qty < 64) {
		bitset64_t visited{};

		return usage_gatherer_bblt64_t::gather_uses_in_path(uses, &visited, blk, list, prior,start);
	}
	else if (mba->qty < 128) {
		bitset128_t visited{};

		return usage_gatherer_bblt128_t::gather_uses_in_path(uses, &visited, blk, list, prior,start);
	}
	else {
		visited_pool->clear();
		return usage_gatherer_default_t::gather_uses_in_path(uses, visited_pool, blk, list, prior, start);
	}

}



template<typename T>
struct def_path_gatherer_tmpl_t {


	CS_NOINLINE
		static bool gather_inbound_defs(fixed_size_vecptr_t<minsn_t*> defs,

			T visited_pool,
			mblock_t* blk,
			mlist_t* list) {


		minsn_t* def = find_definition_backwards(blk, nullptr, list);


		if (!def) {
			for (auto&& def : blk->predset) {

				if (visited_pool->has(def)) {
					continue;
				}
				visited_pool->add(def);

				auto nblk = blk->mba->natural[def];

				if (!gather_inbound_defs(defs, visited_pool, nblk, list)) {
					return false;
				}

			}
		}
		else {
			if (!defs->push_back(def)) {
				return false;
			}
		}

		return true;

	}

	CS_NOINLINE
		static bool no_redef_in_path(

			T visited_pool,
			mblock_t* blk,
			minsn_t* after,
			mblock_t* blockto,
			mlist_t* list) {



		if (blk->serial == blockto->serial)
			return true;

		visited_pool->add(blk->serial);
		minsn_t* def = find_redefinition(blk, after, list);


		if (!def) {
			//hit single succ, no redef, succ is blockto
			for (auto&& def : blk->succset) {
				if (visited_pool->has(def))
					continue;
				if (!no_redef_in_path(visited_pool, blk->mba->natural[def], nullptr, blockto, list)) {
					return false;
				}
			}
			return true;
		}
		else {
			return false;
		}

	}
};

template<typename T>
struct defblk_path_gatherer_tmpl_t {


	CS_NOINLINE
		static bool gather_inbound_defs_and_block(fixed_size_vecptr_t<gather_defblk_res_t> defs,

			T visited_pool,
			mblock_t* blk,
			mlist_t* list) {


		minsn_t* def = find_definition_backwards(blk, nullptr, list);


		if (!def) {
			for (auto&& def : blk->predset) {

				if (visited_pool->has(def)) {
					continue;
				}
				visited_pool->add(def);

				auto nblk = blk->mba->natural[def];

				if (!gather_inbound_defs_and_block(defs, visited_pool, nblk, list)) {
					return false;
				}

			}
		}
		else {

			if (!defs->push_back({ def, blk })) {
				return false;
			}
		}

		return true;

	}
};

bool gather_defs(fixed_size_vecptr_t<minsn_t*> defs,

	bitset_t* visited_pool,
	mblock_t* blk,
	mlist_t* list) {

	mbl_array_t* mba = blk->mba;

	if (mba->qty < 64) {
		bitset64_t visited{};

		return def_path_gatherer_tmpl_t<bitset64_t*>::gather_inbound_defs(defs, &visited, blk, list);

	}
	else if (mba->qty < 128) {
		bitset128_t visited{};

		return def_path_gatherer_tmpl_t<bitset128_t*>::gather_inbound_defs(defs, &visited, blk, list);
	}
	else {
		visited_pool->clear();
		return def_path_gatherer_tmpl_t<bitset_t*>::gather_inbound_defs(defs, visited_pool, blk, list);
	}

}

bool gather_defs(fixed_size_vecptr_t<gather_defblk_res_t> defs,

	bitset_t* visited_pool,
	mblock_t* blk,
	mlist_t* list) {

	mbl_array_t* mba = blk->mba;

	if (mba->qty < 64) {
		bitset64_t visited{};

		return defblk_path_gatherer_tmpl_t<bitset64_t*>::gather_inbound_defs_and_block(defs, &visited, blk, list);

	}
	else if (mba->qty < 128) {
		bitset128_t visited{};

		return defblk_path_gatherer_tmpl_t<bitset128_t*>::gather_inbound_defs_and_block(defs, &visited, blk, list);
	}
	else {
		visited_pool->clear();
		return defblk_path_gatherer_tmpl_t<bitset_t*>::gather_inbound_defs_and_block(defs, visited_pool, blk, list);
	}

}

bool no_redef_in_path(bitset_t* visited_pool,
	mblock_t* blkfrom,
	minsn_t* start,
	mblock_t* blockto,
	mlist_t* list) {

	mbl_array_t* mba = blkfrom->mba;

	if (mba->qty < 64) {
		bitset64_t visited{};

		return def_path_gatherer_tmpl_t<bitset64_t*>::no_redef_in_path(&visited, blkfrom, start, blockto, list);

	}
	else if (mba->qty < 128) {
		bitset128_t visited{};

		return def_path_gatherer_tmpl_t<bitset128_t*>::no_redef_in_path(&visited, blkfrom, start, blockto, list);
	}
	else {
		visited_pool->clear();
		return def_path_gatherer_tmpl_t<bitset_t*>::no_redef_in_path(visited_pool, blkfrom, start, blockto, list);
	}
}
bool mop_is_non_kreg_lvalue(mop_t* mop) {
	return mop->is_lvalue() && (mop->t != mop_r || !is_kreg(mop->r));
}

void lvalue_mop_to_argloc(lvars_t* lvars, argloc_t* aloc, mop_t* mop) {
	if (mop->t == mop_r) {
		aloc->set_reg1(mop->r);
	}
	else if (mop->t == mop_S) {
		aloc->set_stkoff(mop->s->off);
	}
	else if (mop->t == mop_l) {
		lvar_t& CS_RESTRICT lvar = (*lvars)[mop->l->idx];
		*aloc = lvar.location;
	}
	else {
		cs_assert(false);//cant handle whatever this is, you need to check first
	}
}
bool gather_equal_valnums_in_block(mblock_t* blk, mop_t* mop, fixed_size_vecptr_t<mop_t*> vec) {
	if (mop->valnum == 0)
		return true;
	unsigned vnum = mop->valnum;
	bool did_exceed = false;
	for (minsn_t* head = blk->head; head; head = head->next) {
		traverse_minsn<gen_use_traversal_traits>(head, [vnum, vec,&did_exceed](mop_t * mop) {
			if (mop->valnum == vnum) {
				if (!vec->push_back(mop)) {
					did_exceed = true;
				}
			}
			});
	}

	return did_exceed;
}


struct has_any_ldx_traits {
	static constexpr unsigned AUXSTACK_SIZE = 48;
	static constexpr bool maintain_context_ptrs = false;
	static constexpr bool may_term_flow = false;
};
bool has_any_ldx(minsn_t* CS_RESTRICT insn) {
	bool fug = false;
	traverse_minsn< has_any_ldx_traits>(insn, [&fug](mop_t * mop) {
		if (mop->t != mop_d)
			return false;

		if (mop->d->op() == m_ldx) {
			fug = true;
			return true;
		}
		return false;

	});
	return fug;
}

void replace_cc_flag_mops_with_other_mop(minsn_t* insn, mreg_t ccmr, mop_t* mop) {

	traverse_minsn<gen_use_traversal_traits>(insn, [ccmr, mop](mop_t * repr) {
		if (repr->t == mop_r && repr->r == ccmr) {
			*repr = *mop;
			}
		});

}

mreg_t allocate_tempreg_unused_in_block_range(mblock_t* blk, unsigned size) {
	mlist_t uses{};

	generate_uses_between(blk, nullptr, nullptr, &uses);

	rlist_t temps = rlist_tempregs;

	temps.sub(uses.reg);


	for (mreg_t i = mr_t0; i < rlist_tempregs.high; i += size) {
		if (!temps.has_any(i, size)) {
			return i;
		}
	}

	return -1;
}

mop_t* resolve_lvalue(mop_t* input) {
	mop_t* current = input;

	do {
		mop_t* lowoper = nullptr;
		if (try_extract_low(current, &lowoper)) {
			current = lowoper;
			continue;
		}

		xdu_extraction_t xduext{};

		if (try_extract_xdu(current, &xduext)) {
			current = xduext.xdu_operand();
		}
		else {
			break;
		}

	} while (true);

	if (current && current->is_lvalue()) {
		return current;
	}
	return nullptr;
}
mblock_t* resolve_goto_block(mblock_t* blk) {

	if (blk && blk->tail && blk->tail->op() == m_goto && blk->tail->l.t == mop_b) {
		return blk->mba->natural[blk->tail->l.b];
	}
	return blk;
}