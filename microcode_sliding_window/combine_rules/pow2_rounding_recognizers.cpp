#include "combine_rule_defs.hpp"
#include "pow2_rounding_recognizers.hpp"

#include "../hexutils/moptrace.hpp"


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

	if (insn != hexext::current_topinsn())

		return false;
	
	if (insn->op() != m_sub || !insn->r.is_equal_to(1ULL, false))
		return false;
	auto blk = state->block();


	
	using tracer_t = moptrace_t<1, trace_flags::TRACE_SIGN_EXTEND_DESTS | trace_flags::ALLOW_PARTIAL_REDEF | trace_flags::TRACE_ZERO_EXTEND_DESTS>;


	tracer_t tracer{};

	auto find_shr_by = [&tracer, blk](minsn_t* start, mop_t* target, unsigned n) {
		tracer.set(blk, start, target);
		return tracer.trace([n](minsn_t* tr) {
			udivp2_const_t cst{};
			if (try_extract_udivp2_by_const(tr, &cst)) {
				if (cst.shiftcount() == n) {
					return true;
				}
			}
			return false;
			});
	};

	auto find_or_of = [&tracer, blk](minsn_t* start, mop_t* x, mop_t* y) {
		tracer.set(blk, start, x);
		mlist_t tmp{};
		add_mop_to_mlist(x, &tmp, &blk->mba->vars);
		add_mop_to_mlist(y, &tmp, &blk->mba->vars);

		return tracer.trace([&tmp, blk](minsn_t* tr) {

			if (tr->op() == m_or && tr->both_lvalue()) {
				mlist_t l{};
				generate_use_for_insn(blk->mba, tr, &l);

				if (l.mem.compare(&tmp.mem) == 0 && l.reg.compare(tmp.reg) == 0 ) {
					return true;
				}
			}
			return false;
			});
	};



	mop_t* focus = &insn->d;
	/*
v--;
v |= v >> 1;
v |= v >> 2;
v |= v >> 4;
v |= v >> 8;
v |= v >> 16;
v++;
*/
	minsn_t* pos = insn;



	for (unsigned i = 0; i < 5; ++i) {

		minsn_t* shifter = find_shr_by(pos, focus, 1 << i);

		if (!shifter)
			return false;

		pos = find_or_of(shifter, focus, &shifter->d);
		if (!pos)
			return false;
	}

	tracer.set(blk, pos, &pos->d);

	minsn_t* result = tracer.trace([](minsn_t* i) {
			
		if (i->op() == m_add) {
			auto [constval, nonconst] = i->arrange_by(mop_n);
			if (constval->is_equal_to(1ULL, false) && nonconst->is_lvalue()) {
				return true;
			}
		}

		return false;
		});

	if (!result)return false;

	mreg_t temp = allocate_tempreg_unused_in_block_range(blk, result->d.size);
	if (temp == -1)return false;
	mop_t tempreg{};
	tempreg.make_reg(temp, result->d.size);
	insert_mov2_before(insn->ea, blk, insn, &insn->l, &tempreg);

	minsn_t* intrboi = new minsn_t(result->ea);

	mcallarg_t arg{};

	tinfo_t float_ret_type = get_int_type_by_width_and_sign(insn->l.size, type_unsigned);
	arg = tempreg;
	arg.name = "v";
	arg.ea = BADADDR; 
	arg.type = float_ret_type;

	new_helper_late(intrboi, "round_up_pow2", insn->l.size, 1, &arg, &float_ret_type, &result->d);
	
	result->r.erase();
	result->opcode = m_mov;
	result->l.t = mop_d;
	result->l.size = tempreg.size;
	result->l.d = intrboi;


	return true;

}