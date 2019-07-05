#include "combine_rule_defs.hpp"
#include "combine_core.hpp"

#include "combine_shift_and.hpp"
#include "bnot_and_low.hpp"
#include "shift_mod_bitsize.hpp"
#include "bytewise_shift.hpp"
#include "add_masked_one_conditional.hpp"
#include "bitor_followed_by_signbit_extrq.hpp"
#include "raise_bitlut_test_to_multieq_compare.hpp"
#include "byte_shifts_and_adds_to_ors.hpp"
#include "division_magic_number_rules.hpp"
#include "combine_cndop_and_xor_1.hpp"
#include "shr_shl_bittest_by_same_variable.hpp"
#include "div_and_mul_by_same_const_to_mask.hpp"
#include "combine_x86_64_bitand_high.hpp"
#include "combine_or_shift_seq.hpp"


class negated_sets_t : public mcombiner_rule_t {
public:
	virtual bool run_combine(mcombine_t* state);

	virtual const char* name() const {
		return "Negated sets";
	}
};
//neg (xdu (sets rcx.8, .-1).1, .-1).4, .-1).4
bool negated_sets_t::run_combine(mcombine_t* state) {
	

	minsn_t* insn = state->insn();
	

	if (insn->op() != m_neg)
		return false;

	//if (!is_definitely_topinsn_p(insn))
	//	return false;

	xdu_extraction_t initial_xdu{};

	if (!try_extract_xdu(&insn->l, &initial_xdu))
		return false;

	
	mop_t* firstxdu = initial_xdu.xdu_operand();
	/*
		auto [neginsn, expect_inner_xdu] = firstxdu->descend_to_unary_insn(m_neg, mop_d);

	if (!neginsn)
		return false;

	xdu_extraction_t innerxd{};

	if (!try_extract_xdu(expect_inner_xdu, &innerxd))
		return false;*/




	auto [setsinsn, mreg] = firstxdu->descend_to_unary_insn(m_sets, mop_r);


	if (!setsinsn)
		return false;

	if (mreg->size != insn->d.size)
		return false;

	mop_t backup_mr = *mreg;

	
	insn->opcode = m_sar;

	insn->l = backup_mr;

	insn->r.make_number((backup_mr.size * 8) - 1, 1);

	return true;

}
negated_sets_t negated_sets;

class xor_to_or_t : public mcombiner_rule_t {

public:
	virtual bool run_combine(mcombine_t*);
	virtual const char* name()  const {
		return "Xor non-overlapped to Or";
	}
};

bool xor_to_or_t::run_combine(mcombine_t* state) {
	minsn_t* insn = state->insn();
	if (insn->opcode != m_xor)
		return false;

	potential_valbits_t bitsl = try_compute_opnd_potential_valbits(&insn->r);
	potential_valbits_t bitsr = try_compute_opnd_potential_valbits(&insn->l);

	if (!(bitsl.value() & bitsr.value())) {
	//	msg("%x", state->block()->flags);
		insn->opcode = m_or;
		return true;
	}
	return false;
}

xor_to_or_t xor_to_or{};

class mul_to_and_t : public mcombiner_rule_t {

public:
	virtual bool run_combine(mcombine_t*);

	virtual const char* name() const {
		return "Mul to And";
	}
};

bool mul_to_and_t::run_combine(mcombine_t* state) {
	minsn_t* insn = state->insn();

	if (insn->op() != m_mul)
		return false;

	potential_valbits_t bitsl = try_compute_opnd_potential_valbits(&insn->l);
	potential_valbits_t bitsr = try_compute_opnd_potential_valbits(&insn->r);

	if (bitsl.value() == 1 && bitsr.value() == 1) {
		insn->opcode = m_and;
		return true;
	}
	return false;
}
mul_to_and_t mul_to_and{};

class shr_sar_to_and_not_t : public mcombiner_rule_t {
public:
	virtual bool run_combine(mcombine_t* state);

	virtual const char* name()  const {
		return "Shr/Sar to and not";
	}
};

bool shr_sar_to_and_not_t::run_combine(mcombine_t* state) {
	auto insn = state->insn();

	if (!is_mcode_shift_right(insn->op()))
		return false;

	auto [lb, rb] = try_compute_lr_potential_valbits(insn);

	if (!lb.is_boolish() || !rb.is_boolish() )
		return false;


	/*
		todo: handle nonbyte operands
	*/

	if (!insn->lr_both_size(1))
		return false;

	insn->opcode = m_and;

	mop_t backup_r = insn->r;

	insn->r.make_unary_insn_larg(m_lnot, backup_r, 1);


	return true;

}

shr_sar_to_and_not_t shr_sar_to_and_not{};

class add_onebits_equals_two_to_and_t : public mcombiner_rule_t {
public:
	virtual bool run_combine(mcombine_t* state);

	virtual const char* name()  const {
		return "True plus True test with Two";
	}
};

bool add_onebits_equals_two_to_and_t::run_combine(mcombine_t* state) {
	minsn_t* insn = state->insn();

	if (!is_mcode_zf_cond(insn->op()))
		return false;

	auto [addop, num] = insn->arrange_by(mop_d, mop_n);

	if (!addop || !num->is_equal_to(2ULL, false))
		return false;

	auto [addinsn, addl, addr] = addop->descend_to_binary_insn(m_add);
	if (!addinsn)
		return false;


	auto [lbits, rbits] = try_compute_lr_potential_valbits(addinsn);

	if (!lbits.is_boolish() || !rbits.is_boolish())
		return false;

	if (num != &insn->r)
		return false;

	addinsn->opcode = m_and;

	// (x+y) != 2 -> (x&y) != 1
	//(x+y) == 2 -> (x&y) == 1

	num->nnn->update_value(1ULL);

	return true;
}

add_onebits_equals_two_to_and_t add_onebits_equals_to_and{};

class sets_negated_bool_to_zftest_t : public mcombiner_rule_t {
public:
	virtual bool run_combine(mcombine_t* state);
	virtual const char* name()  const {
		return "Sets negated bool to ZF test";
	}
};
//dis no work
bool sets_negated_bool_to_zftest_t::run_combine(mcombine_t* state) {
	auto insn = state->insn();

	if (insn->op() != m_sets)
		return false;

	auto [neginsn, boolop] = insn->l.descend_to_unary_insn(m_neg);
	if (!neginsn)
		return false;

	auto valbits = try_compute_opnd_potential_valbits(boolop);

	if (!valbits.is_boolish())
		return false;
	mop_t backup_boolop = *boolop;

	insn->opcode = m_setnz;

	insn->l = backup_boolop;

	insn->r.make_number(0ULL, 1);
	return true;
}
sets_negated_bool_to_zftest_t sets_negated_bool_to_zftest{};

class xor_xy_sets_to_sets_and_t : public mcombiner_rule_t {
public:
	virtual bool run_combine(mcombine_t* state);

	virtual const char* name() const {
		return "Xor XY sign to sets and sets";
	}
};

bool xor_xy_sets_to_sets_and_t::run_combine(mcombine_t* state) {
	auto insn = state->insn();
	/*
		cannot run on topinsns for some reason
	*/

	if (is_definitely_topinsn_p(insn))
		return false;
	if (insn->op() != m_sets)
		return false;

	auto [xorinsn, x, y] = insn->l.descend_to_binary_insn(m_xor);
	if (!xorinsn)
		return false;

	mop_t bx = *x;
	mop_t by = *y;
	insn->opcode = m_and;

	insn->l.make_unary_insn_larg(m_sets, bx, 1);
	insn->r.make_unary_insn_larg(m_sets, by, 1);

	return true;
}

xor_xy_sets_to_sets_and_t xor_xy_sets_to_sets_and{};

class stx_stx_combine_t : public mcombiner_rule_t {
public:
	virtual bool run_combine(mcombine_t* state);

	virtual const char* name() const {
		return "Sequential commutative STX/LDX combine";
	}
};

/*
1711ae68 : stx (and (ldx ds.2, (add rbx.8, #52.8).8).4, #4294967263.4).4, ds.2, (add rbx.8, #52.8).8
1711ae6f : stx (or (xdu (and rax.1, #32.1).1, .-1).4, (ldx ds.2, (add rbx.8, #52.8).8).4).4, ds.2, (add rbx.8, #52.8).8
*/
bool stx_stx_combine_t::run_combine(mcombine_t* state) {

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

	if(!is_mcode_commutative(value_first->d->op()) || !is_mcode_commutative(value_second->d->op()))
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

stx_stx_combine_t stx_stx_combine{};
/*

Block 0 - 17089d60 to 17089d60. Flags = 608.
Block 1 - 17089d60 to 17089d96. Flags = 40c.
17089d72 : mov rdx.8, .-1, rbp.8
17089d75 : sub rdx.8, #1.8, rax.8
17089d79 : xds rax.8, .-1, rax.16
17089d7b : mov r9.8, .-1, rsi.8
17089d7e : mov rax.8, .-1, unknown@1184.8
17089d81 : mov r8.8, .-1, r15.8
17089d84 : sdiv unknown@1184.8, #2.8, rax.8
17089d87 : mov rcx.8, .-1, rbx.8
17089d8a : mov rax.8, .-1, rdi.8
17089d8d : setb r8.8, rbp.8, cf.1
17089d8d : seto r8.8, rbp.8, of.1
17089d8d : setz r8.8, rbp.8, zf.1
17089d8d : setp r8.8, rbp.8, rpf.1
17089d8d : sets (sub r8.8, rbp.8).8, .-1, sf.1
17089d90 : jcnd (lnot (xor sf.1, of.1).1, .-1).1, .-1, loc_17089E63.-1
Block 2 - 17089d96 to 17089da3. Flags = 40c.
17089d96 : mov r14.8, .-1, stkvar_96.8
17089d9b : mov stkvar_128.8, .-1, r14.8
Block 3 - 17089da3 to 17089dd5. Flags = 41c.
17089da3 : ldx rds.2, rbx.8, xmm1.16
17089da6 : mov xmm1.16, .-1, stkvar_32.16
17089dab : mov stkvar_32.8, .-1, rax.8
17089db0 : mov #8.1, .-1, t1.1
17089db0 : mov (call _mm_srli_si128, .-1, (xmm1.16,t1.1)).16, .-1, xmm1.16
17089db5 : mov xmm1.8, .-1, rdx.8
17089dba : add rdi.8, rdx.8, rdx.8
17089dbd : ldx rds.2, (add rax.8, #16.8).8, rcx.8
17089dc1 : ldx rds.2, (add rax.8, #8.8).8, rax.8
17089dc5 : sub rcx.8, #1.8, rcx.8
17089dc8 : and rdx.8, rcx.8, rcx.8
17089dc8 : mov #0.1, .-1, cf.1
17089dc8 : mov #0.1, .-1, of.1
17089dc8 : setz rcx.8, #0.8, zf.1
17089dc8 : setp rcx.8, #0.8, rpf.1
17089dc8 : sets rcx.8, .-1, sf.1
17089dcb : mov rsi.8, .-1, rdx.8
17089dce : ldx rds.2, (add rax.8, (mul #8.8, rcx.8).8).8, rcx.8
17089dd2 : icall rcs.2, (ldx rds.2, r14.8).8
Block 4 - 17089dd5 to 17089ddd. Flags = 40c.
17089dd5 : mov #0.1, .-1, cf.1
17089dd5 : mov #0.1, .-1, of.1
17089dd5 : setz rax.1, #0.1, zf.1
17089dd5 : setp rax.1, #0.1, rpf.1
17089dd5 : sets rax.1, .-1, sf.1
17089dd7 : jcnd zf.1, .-1, loc_17089E5E.-1
Block 5 - 17089ddd to 17089e5e. Flags = 40c.
17089ddd : ldx rds.2, rbx.8, xmm1.16
17089de0 : mov xmm1.16, .-1, stkvar_32.16
17089de5 : mov stkvar_32.8, .-1, rax.8
17089dea : mov #8.1, .-1, t1.1
17089dea : low (call _mm_srli_si128, .-1, (xmm1.16,t1.1)).16, .-1, xmm1.8
17089def : mov xmm1.8, .-1, rdx.8
17089df4 : ldx rds.2, rbx.8, xmm1.16
17089df7 : ldx rds.2, (add rax.8, #16.8).8, rcx.8
17089dfb : add rdi.8, rdx.8, rdx.8
17089dfe : ldx rds.2, (add rax.8, #8.8).8, rax.8
17089e02 : sub rcx.8, #1.8, rcx.8
17089e05 : and rdx.8, rcx.8, rcx.8
17089e08 : mov xmm1.16, .-1, stkvar_48.16
17089e0d : mov #8.1, .-1, t1.1
17089e0d : low (call _mm_srli_si128, .-1, (xmm1.16,t1.1)).16, .-1, xmm1.8
17089e12 : ldx rds.2, (add rax.8, (mul #8.8, rcx.8).8).8, rdx.8
17089e16 : mov xmm1.8, .-1, r8.8
17089e1b : mov stkvar_48.8, .-1, rax.8
17089e20 : add rbp.8, r8.8, r8.8
17089e23 : mov rdi.8, .-1, rbp.8
17089e26 : ldx rds.2, rdx.8, xmm0.16
17089e29 : ldx rds.2, (add rax.8, #16.8).8, rcx.8
17089e2d : ldx rds.2, (add rax.8, #8.8).8, rax.8
17089e31 : sub rcx.8, #1.8, rcx.8
17089e34 : and r8.8, rcx.8, rcx.8
17089e37 : ldx rds.2, (add rax.8, (mul #8.8, rcx.8).8).8, rcx.8
17089e3b : sub rdi.8, #1.8, rax.8
17089e3f : stx xmm0.16, rds.2, rcx.8
17089e42 : ldx rds.2, (add rdx.8, #16.8).8, xmm1.16
17089e46 : xds rax.8, .-1, rax.16
17089e48 : mov rax.8, .-1, unknown@1192.8
17089e4b : sdiv unknown@1192.8, #2.8, rax.8
17089e4e : mov rax.8, .-1, rdi.8
17089e51 : stx xmm1.16, rds.2, (add rcx.8, #16.8).8
17089e55 : setb r15.8, rbp.8, cf.1
17089e55 : seto r15.8, rbp.8, of.1
17089e55 : setz r15.8, rbp.8, zf.1
17089e55 : setp r15.8, rbp.8, rpf.1
17089e55 : sets (sub r15.8, rbp.8).8, .-1, sf.1
17089e58 : jcnd (xor sf.1, of.1).1, .-1, loc_17089DA3.-1
Block 6 - 17089e5e to 17089e63. Flags = 40c.
17089e5e : mov stkvar_96.8, .-1, r14.8
Block 7 - 17089e63 to 17089eb0. Flags = 41c.
17089e63 : ldx rds.2, rbx.8, xmm1.16
17089e66 : mov stkvar_104.8, .-1, rbx.8
17089e6b : ldx rds.2, rsi.8, xmm0.16
17089e6e : mov xmm1.16, .-1, stkvar_48.16
17089e73 : mov stkvar_48.8, .-1, rax.8
17089e78 : mov #8.1, .-1, t1.1
17089e78 : low (call _mm_srli_si128, .-1, (xmm1.16,t1.1)).16, .-1, xmm1.8
17089e7d : mov xmm1.8, .-1, rdx.8
17089e82 : add rbp.8, rdx.8, rdx.8
17089e8a : ldx rds.2, (add rax.8, #16.8).8, rcx.8
17089e8e : ldx rds.2, (add rax.8, #8.8).8, rax.8
17089e92 : sub rcx.8, #1.8, rcx.8
17089e95 : and rdx.8, rcx.8, rcx.8
17089e95 : mov #0.1, .-1, cf.1
17089e95 : mov #0.1, .-1, of.1
17089e95 : setz rcx.8, #0.8, zf.1
17089e95 : setp rcx.8, #0.8, rpf.1
17089e95 : sets rcx.8, .-1, sf.1
17089e98 : ldx rds.2, (add rax.8, (mul #8.8, rcx.8).8).8, rdx.8
17089e9c : stx xmm0.16, rds.2, rdx.8
17089e9f : ldx rds.2, (add rsi.8, #16.8).8, xmm1.16
17089ea3 : stx xmm1.16, rds.2, (add rdx.8, #16.8).8
17089eaf : ret #0.8, .-1
			*/

/*
add rbp.8, (low (call _mm_srli_si128, .-1, ((ldx rds.2, rbx.8).16,#8.1)).16, .-1).8, r8.8
*/

class simd_ld_shrtrim_t : public mcombiner_rule_t {
public:
	virtual const char* name() const override {
		return "SIMD128 bytewise shift-right by 8 transformer";
	}
	
	virtual bool run_combine(mcombine_t* state);
};
bool simd_ld_shrtrim_t::run_combine(mcombine_t* state) {

	minsn_t* insn = state->insn();

	if (insn->op() != m_low || insn->d.size != 8 || insn->l.t != mop_d || insn->l.size != 16)
		return false;

	auto intrin = insn->l.d;

	auto role = get_instruction_exrole(intrin);

	if (role != exrole_t::byteshr128)
		return false;

	auto left128 = &intrin->d.f->args[0];
	auto shrcount = &intrin->d.f->args[1];


	if (!shrcount->is_equal_to(8ULL, false))
		return false;
	mop_t copyop;
	copyop = *left128;

	insn->opcode = m_high;
		

	
	insn->l = copyop;

	return true;


}
simd_ld_shrtrim_t simd_ld_shrtrim{};


class detect_xdu_in_xor128_t : public mcombiner_rule_t {
public:
	virtual const char* name() const override {
		return "Detect simd128 xor float negation";
	}
	virtual bool run_combine(mcombine_t* state);

};

/*
mov (call _mm_xor_ps, .-1, (xmm6.16,(xdu xmmword_1800E51C0.8, .-1).16)).16, .-1, xmm6.16
*/
bool detect_xdu_in_xor128_t::run_combine(mcombine_t* state) {

	auto i = state->insn();
	if (hexext::current_topinsn() != i)
		return false;

	if (i->opcode != m_mov)
		return false;
	if (i->d.t != mop_r || !is_simdreg(i->d.r) || i->l.t != mop_d)
		return false;


	auto [x, y] = extract_binary_exrole(i->l.d, exrole_t::xor128);

	if (!x)
		return false;

	auto [xdu, nonxdu] = order_mops(mop_d, x, y);


	if (!xdu) {
		return false;
	}

	auto [xdui,xduop] = xdu->descend_to_unary_insn(m_xdu);

	if (!xdui)
		return false;
	if (xdui->d.size == 16 && xduop->size == 8 && nonxdu->t == mop_r && nonxdu->r == i->d.r) {

		mop_t op1 = *xduop;
		mop_t op2 = *nonxdu;


		i->opcode = m_xor;
		i->l = op1;
		i->l.size = 8;
		i->r = op2;
		i->r.size = 8;
		i->d.size = 8;
		return true;


	}
	return false;



}

detect_xdu_in_xor128_t detect_xdu_in_xor128{};

/*
f.jb (xor xmm6.8, xmmword_1800E51C0.8).8, xmm2.8, 34
*/

class detect_bitwise_negate_floatop_t : public mcombiner_rule_t {
public:
	virtual const char* name() const override {
		return "Transform bitwise negation of floating";
	}
	virtual bool run_combine(mcombine_t* state);
};

static void build_valnum_mlist(mlist_t* out, mblock_t* block, mop_t* mop) {
	fixed_size_vector_t<mop_t*, 32> vec;
	gather_equal_valnums_in_block(block, mop, vec.pass());
	add_mop_to_mlist(mop, out, &block->mba->vars);
	for (auto&& op : vec) {
		add_mop_to_mlist(op, out, &block->mba->vars);
	}

}

static bool uses_favours_floating(mcombine_t* state, minsn_t* top, mlist_t* list, mblock_t* block, bool prior) {


	fixed_size_vector_t<minsn_t*, 16> considereds;
	gather_uses(considereds.pass(), state->bbidx_pool(), top->prev, block,  list,prior);

	if (considereds.size() == 0)
		return false;

	fixed_size_vector_t<minsn_t*, 32> inner_considereds;

	unsigned n_fpuses = 0;
	unsigned n_nonfpuses = 0;
	lvars_t* lvars = &state->block()->mba->vars;
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

						if (!uses_favours_floating(state, top->next, &list2, state->block(), false)) {
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
detect_bitwise_negate_floatop_t detect_bitwise_negate_floatop{};

class locate_abs_value_floatpath_t : public mcombiner_rule_t {
public:
	virtual const char* name() const override {
		return "Combine ABS float mask when float path taken";
	}
	virtual bool run_combine(mcombine_t* state);
};

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
		if(tempnonmask.is_lvalue())
			lvalue_mop_to_argloc(lvars, &aloc, &tempnonmask);
		tinfo_t float_ret_type = get_float_type(tempnonmask.size);

		mlist_t retregs{};
		mlist_t spoiled{};
		spoiled.mem.qclear();

		arg.name = "v";
		arg.ea = BADADDR; //uhhh
		arg.type = float_ret_type;



		arg.size = tempnonmask.size;
		new_helper(callboi, "fabs", mask->size, 1, &arg, &float_ret_type, &aloc, &retregs.reg, &spoiled,1, &arg);

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
locate_abs_value_floatpath_t locate_abs_value_floatpath{};
/*
	disabling many rules that need fixing or need extensive testing
*/
static mcombiner_rule_t* g_allrules[] = {
	&combine_shift_and_rule,
	&raise_bitlut_multieq,
	& negated_sets,
	& xor_to_or,
	& mul_to_and,
	& shr_sar_to_and_not,
	& add_onebits_equals_to_and,
	//& sets_negated_bool_to_zftest,
//	& xor_xy_sets_to_sets_and,
	& stx_stx_combine,
	//& join_zf_jcnd
	//combine_bnot_and_1,
	/*combine_jzf_and_bnot,
	combine_shift_mod_bitsize,
	combine_bytewise_shift,
	combine_sign_shift_neg,
	combine_add_masked_one_conditional,
	combine_signbit_shift_and_bitop,
//combine_byte_shifts_and_adds_to_ors,

	division_magic_num_rule_1,
	shl_and_low,
	//combine_shr_shl_bittest_by_same_variable,
	*/
	&combine_or_shift,
	&div_and_mul_in_conditional_to_modulus_test,
	&combine_cndop_and_xor_1,
	& combine_bnot_and_one,
	&combine_jzf_and_bnot,
	& simd_ld_shrtrim,
	& detect_xdu_in_xor128,
	& detect_bitwise_negate_floatop,
	& locate_abs_value_floatpath
};

void toggle_common_combination_rules(bool enabled) {
	for (auto&& comb : g_allrules) {
		hexext::install_combine_cb(comb, enabled);
	}
}

static  mcombiner_rule_t* const g_x86_rules[] = {
		/*combine_x86_64_bitand_high,
	combine_x86_64_bitor_high*/
	&combine_x86_band_high,
	&combine_x86_bitor_high
};

void toggle_archspec_combination_rules(bool enabled) {
	if (hexext::currarch() == hexext_arch_e::x86) {

		for (auto&& comb : g_x86_rules) {
			hexext::install_combine_cb(comb, enabled);
		}

	}
}