#pragma once
#include "../cs_core.hpp"

#include "traverse_micro.hpp"
#include "smaller_bitsets.hpp"

struct mulp2_const_t {
	mop_t* m_numop;
	minsn_t* m_actual_insn;
	minsn_t* m_targetinsn;
	mop_t* m_multiplicand;

	struct {
		unsigned m_is_mul : 1;
		unsigned m_shiftcount : 9;
	};

	bool is_mul() const {
		return m_is_mul;
	}

	bool is_shl() const {
		return !m_is_mul;
	}

	mop_t* const_term() {
		return m_numop;
	}

	mop_t* mutable_term() {
		return m_multiplicand;
	}

	minsn_t* mulinsn() {
		return m_actual_insn;
	}

	minsn_t* targetedinsn() {
		return m_targetinsn;
	}

	unsigned shiftcount() const {
		return m_shiftcount;
	}

};



bool try_extract_mulp2_by_const(minsn_t* insn, mulp2_const_t* out);

struct udivp2_const_t {
	mop_t* m_numop;
	minsn_t* m_actual_insn;
	minsn_t* m_targetinsn;
	mop_t* m_numerator;

	struct {
		unsigned m_is_udiv : 1;
		unsigned m_shiftcount : 9;
	};

	bool is_mul() const {
		return m_is_udiv;
	}

	bool is_shl() const {
		return !m_is_udiv;
	}

	mop_t* const_term() {
		return m_numop;
	}

	mop_t* mutable_term() {
		return m_numerator;
	}

	minsn_t* mulinsn() {
		return m_actual_insn;
	}

	minsn_t* targetedinsn() {
		return m_targetinsn;
	}

	unsigned shiftcount() const {
		return m_shiftcount;
	}



};


bool try_extract_udivp2_by_const(minsn_t* insn, udivp2_const_t* out);

struct xdu_extraction_t {
	mop_t* m_extended_op;
	union {
		mop_t* m_target;
		minsn_t* m_targeti;
	};
	struct {
		unsigned m_fromsize : 7;
		unsigned m_tosize : 7;
		unsigned m_tosize_known : 1;
	};

	unsigned fromsize() const {
		return m_fromsize;
	}

	unsigned tosize() const {
		return m_tosize;
	}
	bool is_tosize_known() const {
		return m_tosize_known;
	}

	mop_t* xdu_operand() {
		return m_extended_op;
	}
};

bool try_extract_xdu(mop_t* mop, xdu_extraction_t* out);
bool try_extract_xdu(minsn_t* mop, xdu_extraction_t* out);
struct potential_valbits_t {
	uint64_t m_underlying;

	uint64_t value() const {
		return m_underlying;
	}

	bool is_boolish() const {
		return m_underlying == 1;
	}

	unsigned npossible() const {
		return __popcnt64(m_underlying);
	}
};

potential_valbits_t try_compute_opnd_potential_valbits(mop_t* op);

static inline std::pair<potential_valbits_t, potential_valbits_t> try_compute_lr_potential_valbits(minsn_t* insn) {
	return { try_compute_opnd_potential_valbits(&insn->l) , try_compute_opnd_potential_valbits(&insn->r) };
}

static inline bool try_extract_const_xdu(mop_t* mop, xdu_extraction_t* out) {
	if (try_extract_xdu(mop, out)) {
		if (out->xdu_operand()->t != mop_n) {
			return false;
		}
		return true;
	}
	return false;
}

static inline bool try_extract_high(mop_t* mop, mop_t** highoperand) {
	if (mop->t != mop_d) {
		return false;
	}
	if (mop->d->opcode != m_high)
		return false;

	*highoperand = &mop->d->l;

	return true;
}
static inline bool try_extract_low(mop_t* mop, mop_t** highoperand) {
	if (mop->t != mop_d) {
		return false;
	}
	if (mop->d->opcode != m_low)
		return false;

	*highoperand = &mop->d->l;

	return true;
}


struct mul_by_constant_extraction_t {
	mop_t* m_constant;//constant term
	mop_t* m_mutable;//mutable term
	minsn_t* m_mulinsn;
	uint64_t m_mulvalue;
	bool m_is_shift;
	
	

	bool is_shift() {
		return m_is_shift;
	}

	uint64_t multiplier() const {
		return m_mulvalue;
	}

	minsn_t* mulinsn() {
		return m_mulinsn;
	}

	mop_t* mutable_term() {
		return m_mutable;
	}

	mop_t* constant_term() {
		return m_constant;
	}

};

bool try_extract_mul_by_constant(minsn_t* CS_RESTRICT insn, mul_by_constant_extraction_t* CS_RESTRICT out);



struct udiv_by_constant_extraction_t {
	mop_t* m_constant;//constant term
	mop_t* m_mutable;//mutable term
	minsn_t* m_divinsn;
	uint64_t m_divvalue;
	bool m_is_shift;



	bool is_shift() {
		return m_is_shift;
	}

	uint64_t divisor() const {
		return m_divvalue;
	}

	minsn_t* divinsn() {
		return m_divinsn;
	}

	mop_t* mutable_term() {
		return m_mutable;
	}

	mop_t* constant_term() {
		return m_constant;
	}
};

bool try_extract_udiv_by_constant(minsn_t* CS_RESTRICT insn, udiv_by_constant_extraction_t* CS_RESTRICT out);
void add_mop_to_mlist(mop_t* CS_RESTRICT mop, mlist_t* CS_RESTRICT mlist, lvars_t* CS_RESTRICT lvars);
//only operates on lvalue-esque mops.
bool test_mop_in_mlist(mop_t* CS_RESTRICT mop, mlist_t* CS_RESTRICT mlist, lvars_t* CS_RESTRICT lvars);
//recurses through submops
bool test_any_submop_in_mlist(mop_t* CS_RESTRICT mop, mlist_t* CS_RESTRICT mlist, lvars_t* CS_RESTRICT lvars);

void generate_use_for_insn(mbl_array_t* mba, minsn_t* insn, mlist_t* CS_RESTRICT mlist);
void generate_def_for_insn(mbl_array_t* mba, minsn_t* insn, mlist_t* CS_RESTRICT mlist);
bool gather_user_subinstructions(lvars_t* lvars, minsn_t* insn, mlist_t* CS_RESTRICT list, fixed_size_vecptr_t<minsn_t*> into);
minsn_t* find_definition_backwards(mblock_t* CS_RESTRICT blk, minsn_t* CS_RESTRICT insn, mlist_t* CS_RESTRICT mlist);

minsn_t* find_redefinition(mblock_t* CS_RESTRICT blk, minsn_t* CS_RESTRICT insn, mlist_t* CS_RESTRICT mlist);
minsn_t* find_next_use(mblock_t* blk, minsn_t* insn, mlist_t* mlist, bool* redefed);
minsn_t* find_prev_use(mblock_t* blk, minsn_t* insn, mlist_t* mlist, bool* defed);
bool find_definition_size(bitset_t* visited_pool,  mblock_t* block, minsn_t* insn, unsigned* size_out, mop_t* mreg);
mop_t* locate_first_mreg_use_in_insn(minsn_t* insn, mreg_t mreg, unsigned mreg_size);


mblock_t* find_block_by_ea_start(mbl_array_t* CS_RESTRICT mba, ea_t ea);
mop_t* locate_mreg_def_in_insn(minsn_t* insn, mreg_t mreg, unsigned mreg_size);

bool redefs_mreg_without_use(mblock_t* CS_RESTRICT block, mreg_t mreg, unsigned mreg_size);

bool redefs_mreg_without_use_or_does_not_use_and_returns(mblock_t* CS_RESTRICT block, mreg_t mreg, unsigned mreg_size);

minsn_t* find_next_mreg_use(minsn_t* CS_RESTRICT start, mreg_t mreg, unsigned mreg_size, bool* redefed);

minsn_t* find_prev_mreg_use(minsn_t* CS_RESTRICT start, mreg_t mreg, unsigned mreg_size, bool* redefed);

bool block_does_not_use_or_redef_mreg(mblock_t* blk, mreg_t mr, unsigned size);


minsn_t* find_mreg_def_backwards(minsn_t* CS_RESTRICT start, mreg_t mreg, unsigned mreg_size);
std::set<minsn_t*> gather_reg2reg_movs_with_no_use_or_redef(mblock_t* blk);


bool has_any_ldx(minsn_t* CS_RESTRICT insn);

static inline bool is_definitely_topinsn_p(minsn_t* insn) {
	return (insn->next != nullptr || insn->prev != nullptr) && insn->ea != BADADDR;
}

class mblock_iterator_t {
	mblock_t* m_current;
public:
	inline mblock_iterator_t(mbl_array_t* mba) : m_current(mba->blocks) {}

	inline mblock_iterator_t(mblock_t* blk) : m_current(blk) {}

	inline mblock_iterator_t& operator ++() {
		m_current = m_current->nextb;
		return *this;
	}

	inline mblock_t* operator *() {
		return m_current;
	}

	inline bool operator !=(const mblock_iterator_t& other) const {
		return m_current != other.m_current;
	}

};

struct mba_iterator_t {
	mbl_array_t* m_mba;
public:
	mba_iterator_t(mbl_array_t* mba) : m_mba(mba) {}

	inline mblock_iterator_t begin() {
		return mblock_iterator_t{ m_mba };
	}
	inline mblock_iterator_t end() {
		return mblock_iterator_t{ (mblock_t*)nullptr };
	}
};

static inline mba_iterator_t forall_mblocks(mbl_array_t* mba) {
	return mba_iterator_t{ mba };
}

bool extract_stx_displ(minsn_t* stxinsn, mreg_t* segreg, mop_t** base, mop_t** displ, mop_t** value);

bool extract_ldx_displ(minsn_t* stxinsn, mreg_t* segreg, mop_t** base, mop_t** displ);

bool try_extract_equal_stx_dests(minsn_t* x, minsn_t* y, mop_t** value_out, mop_t** value_outy);

bool equal_ldx_srcs(minsn_t* x, minsn_t* y);

bool ldx_src_equals_stx_dest(minsn_t* ldx, minsn_t* sty);


bool minsn_has_any_tempreg(minsn_t* insn);


bool get_static_value(mop_t* v, uint64_t* out);

bool gather_uses(fixed_size_vecptr_t<minsn_t*> uses,
	/*
		fixed size vector, intended to be reused
		acts as a set of visited basic blocks
	*/
	bitset_t* visited_pool,
	minsn_t* start,
	mblock_t* blk,
	mlist_t* list, bool prior);
/*
	returns false if it failed to gather all uses due to the size of the defs vector
*/
bool gather_defs(fixed_size_vecptr_t<minsn_t*> defs,

	bitset_t* visited_pool,
	mblock_t* blk,
	mlist_t* list);

struct gather_defblk_res_t {
	minsn_t* first;
	mblock_t* second;
};

bool gather_defs(fixed_size_vecptr_t<gather_defblk_res_t> defs,

	bitset_t* visited_pool,
	mblock_t* blk,
	mlist_t* list);


bool no_redef_in_path(bitset_t* visited_pool,
	mblock_t* blkfrom,
	minsn_t* start,
	mblock_t* blockto,
	mlist_t* list);

void lvalue_mop_to_argloc(lvars_t* lvars, argloc_t* aloc, mop_t* mop);


bool mop_is_non_kreg_lvalue(mop_t* mop);
//return false if exceeded vec
bool gather_equal_valnums_in_block(mblock_t* blk, mop_t* mop, fixed_size_vecptr_t<mop_t*> vec);


static inline bool mop_seems_floaty_p(mop_t* mop) {
	return (mop->oprops & OPROP_FLOAT) || (mop->t == mop_d && mop->d->is_fpinsn());
}

static inline bool mlist_has_any_cc(mlist_t* ml) {
	return ml->reg.has_any(mr_cf, mr_pf + 1);
}

static inline void generate_defs_between(mblock_t* blk, minsn_t* after_this, minsn_t* stop_at_this, mlist_t* l) {
	if (!after_this) after_this = blk->head;
	for (minsn_t* p = after_this->next; p != stop_at_this; p = p->next) {
		generate_def_for_insn(blk->mba, p, l);
	}
}
static inline void generate_uses_between(mblock_t* blk, minsn_t* after_this, minsn_t* stop_at_this, mlist_t* l) {
	if (!after_this)after_this = blk->head;
	for (minsn_t* p = after_this->next; p != stop_at_this; p = p->next) {
		generate_use_for_insn(blk->mba, p, l);
	}
}

void replace_cc_flag_mops_with_other_mop(minsn_t* insn, mreg_t ccmr, mop_t* mop);
mreg_t allocate_tempreg_unused_in_block_range(mblock_t* blk, unsigned size);
/*
	traverse xdu and low until we get out lvalue
*/
mop_t* resolve_lvalue(mop_t* input);

mblock_t* resolve_goto_block(mblock_t* blk);

#include "codegen_utils.hpp"
#include "micro_executor_template.hpp"