#include "combine_rule_defs.hpp"
#include "pow2_rounding_recognizers.hpp"


enum class mval_e {
	any,
	reg,
	num,
	addr,
	stkvr,
};

class mcapture_t {

	const mval_e m_value_kind;
	const unsigned m_idx;

public:
	constexpr mcapture_t(mval_e kind, unsigned idx) : m_value_kind(kind), m_idx(idx) {}


	constexpr mval_e kind() const {
		return m_value_kind;
	}

	constexpr unsigned idx() const {
		return m_idx;
	}


};



/*
171208c0 : mov rcx.4, .-1, rax.4

171208c2 : shr rax.4, #1.1, rax.4
171208c4 : or rax.4, rcx.4, rcx.4
171208c6 : mov rcx.4, .-1, rax.4

171208c8 : shr rax.4, #2.1, rax.4
171208cb : or rax.4, rcx.4, rcx.4
171208cd : mov rcx.4, .-1, rax.4

171208cf : shr rax.4, #4.1, rax.4
171208d2 : or rax.4, rcx.4, rcx.4
171208d4 : mov rcx.4, .-1, rax.4

171208d6 : shr rax.4, #8.1, rax.4
171208d9 : or rax.4, rcx.4, rcx.4
171208db : mov rcx.4, .-1, rax.4

171208dd : shr rax.4, #16.1, rax.4
171208e0 : or rax.4, rcx.4, rcx.4

171208e2 : mov rcx.4, .-1, rax.4
171208e4 : shr rax.4, #1.1, rax.4
*/
bool recognize_round_down_power2_t::run_combine(mcombine_t* state) {
	if (!is_definitely_topinsn_p(state->insn()))
		return false;

	auto insn = state->insn();
	auto block = state->block();

	
	udivp2_const_t udiv{};

	if (!try_extract_udivp2_by_const(insn, &udiv) || udiv.shiftcount() != 1)
		return false;

	mlist_t lst{};

	generate_def_for_insn(block->mba, insn, &lst);
	bool rdf = false;
	minsn_t* nextuse = find_next_use(block, insn, &lst, &rdf);

	if (!nextuse || rdf )
		return false;

	if (nextuse->opcode == m_mov) {
		mlist_t newlist{};

	}

	if (nextuse->opcode != m_or)
		return false;
	return false;


}

/*
add (
or (
or (
shr (
or (
shr (
or (
shr (
or (shr (sub rbp.4, #1.4).4, #1.1).4, (sub rbp.4, #1.4).4).4, #2.1).4, (or (shr (sub rbp.4, #1.4).4, #1.1).4, (sub rbp.4, #1.4).4).4).4, #4.1).4, (or (shr (or (shr (sub rbp.4, #1.4).4, #1.1).4, (sub rbp.4, #1.4).4).4, #2.1).4, (or (shr (sub rbp.4, #1.4).4, #1.1).4, (sub rbp.4, #1.4).4).4).4).4, #8.1).4, (or (shr (or (shr (or (shr (sub rbp.4, #1.4).4, #1.1).4, (sub rbp.4, #1.4).4).4, #2.1).4, (or (shr (sub rbp.4, #1.4).4, #1.1).4, (sub rbp.4, #1.4).4).4).4, #4.1).4, (or (shr (or (shr (sub rbp.4, #1.4).4, #1.1).4, (sub rbp.4, #1.4).4).4, #2.1).4, (or (shr (sub rbp.4, #1.4).4, #1.1).4, (sub rbp.4, #1.4).4).4).4).4).4, (shr (or (shr (or (shr (or (shr (or (shr (sub rbp.4, #1.4).4, #1.1).4, (sub rbp.4, #1.4).4).4, #2.1).4, (or (shr (sub rbp.4, #1.4).4, #1.1).4, (sub rbp.4, #1.4).4).4).4, #4.1).4, (or (shr (or (shr (sub rbp.4, #1.4).4, #1.1).4, (sub rbp.4, #1.4).4).4, #2.1).4, (or (shr (sub rbp.4, #1.4).4, #1.1).4, (sub rbp.4, #1.4).4).4).4).4, #8.1).4, (or (shr (or (shr (or (shr (sub rbp.4, #1.4).4, #1.1).4, (sub rbp.4, #1.4).4).4, #2.1).4, (or (shr (sub rbp.4, #1.4).4, #1.1).4, (sub rbp.4, #1.4).4).4).4, #4.1).4, (or (shr (or (shr (sub rbp.4, #1.4).4, #1.1).4, (sub rbp.4, #1.4).4).4, #2.1).4, (or (shr (sub rbp.4, #1.4).4, #1.1).4, (sub rbp.4, #1.4).4).4).4).4).4, #16.1).4
).4
, #1.4, rax.4



((((((((
(unsigned int)(v12 - 1) >> 1) | (v12 - 1)) >> 2) | 
((unsigned int)(v12 - 1) >> 1) | (v12 - 1)) >> 4) | 
((((unsigned int)(v12 - 1) >> 1) | (v12 - 1)) >> 2) | 
((unsigned int)(v12 - 1) >> 1) | (v12 - 1)) >> 8) | 
((((((unsigned int)(v12 - 1) >> 1) | (v12 - 1)) >> 2) | 
((unsigned int)(v12 - 1) >> 1) | (v12 - 1)) >> 4) | 
((((unsigned int)(v12 - 1) >> 1) | (v12 - 1)) >> 2) | 
((unsigned int)(v12 - 1) >> 1) | (v12 - 1) | 
((((((((((unsigned int)(v12 - 1) >> 1) | (v12 - 1)) >> 2)
| ((unsigned int)(v12 - 1) >> 1) | (v12 - 1)) >> 4) |
((((unsigned int)(v12 - 1) >> 1) | (v12 - 1)) >> 2) | 
((unsigned int)(v12 - 1) >> 1) | (v12 - 1)) >> 8) | 
((((((unsigned int)(v12 - 1) >> 1) | (v12 - 1)) >> 2) | 
((unsigned int)(v12 - 1) >> 1) | (v12 - 1)) >> 4) | 
((((unsigned int)(v12 - 1) >> 1) | (v12 - 1)) >> 2) |
((unsigned int)(v12 - 1) >> 1) | (v12 - 1)) >> 16))
				  + 1;

*/

COMB_RULE_IMPLEMENT(recognize_overcombined_round_up_pow2) {
	auto insn = state->insn();


	auto [constval, insnval] = insn->arrange_by(mop_n, mop_d);
	if (insn->op() != m_add || !constval || !constval->is_equal_to(1,false))
		return false;

	return false;

}