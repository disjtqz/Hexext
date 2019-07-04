
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
#include "../exrole/exrole.hpp"
#include "hexutils.hpp"

static uint64_t lut_sizemask[] = {
	0xffull,0xffffull,0xffffffffull, 0xffffffffffffffffull
};

static constexpr unsigned nextpow2(unsigned v) {
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	return v - (v >> 1);
}
template<typename T>
static T rev_bits(T word) {

	T result = (T)0ULL;

	for (unsigned i = 0; i < sizeof(T)*CHAR_BIT; ++i) {
		if (word & (static_cast<T>(1) << i)) {
			result |= (static_cast<T>(1) << ((sizeof(T) * CHAR_BIT) - 1)) >> i;
		}
	}

	return result;
}

static uint64_t lzmask_from(unsigned lz, unsigned size) {

	

}

static unsigned add_bitmag(uint64_t mask1, uint64_t mask2, unsigned size) {

	unsigned maxlz = std::max(_lzcnt_u64(mask1), _lzcnt_u64(mask2));

	maxlz += 1;


	if (maxlz >= (size * 8)) {

		maxlz = (size * 8) - 1;
	}

	//return extend_value_by_size_and_sign((1ULL << maxlz) - 1, size, false);
}

static bool do_rbit(mop_t* argument, potential_valbits_t* out) {
	
	potential_valbits_t argbits = try_compute_opnd_potential_valbits(argument);

	switch (argument->size) {
	case 1:
		out->m_underlying = rev_bits<unsigned char>(argbits.value());
		break;
	case 2:
		out->m_underlying = rev_bits<unsigned short>(argbits.value());
		break;
	case 4:
		out->m_underlying = rev_bits<unsigned int>(argbits.value());
		break;
	case 8:
		out->m_underlying = rev_bits<uint64_t>(argbits.value());
		break;
	default:
		cs_assert(false);
	}

	return true;

}


static bool compute_intrinsic_valbits(mop_t* op, exrole_t role, potential_valbits_t* out) {

	mcallinfo_t* callinf = op->d->d.f;

	cs_assert(callinf);

	switch (role) {
		/*
			If the instruction is pop count then the mask of possible values is 
			the number of possible bits that could be set in the operand rounded up to the next power of to -1


		*/
	case exrole_t::popcnt: {

		cs_assert(callinf->args.size() == 1);

		potential_valbits_t vbarg = try_compute_opnd_potential_valbits(&callinf->args[0]);


		if (vbarg.value() == extend_value_by_size_and_sign(~0ULL, callinf->args[0].size, false)) {
			out->m_underlying = (callinf->args[0].size * 8) - 1;
		}
		else if (vbarg.value() == extend_value_by_size_and_sign(0ULL, callinf->args[0].size, false)) {
			out->m_underlying = 0ULL;
		}
		else {
			out->m_underlying = nextpow2(vbarg.npossible()) ;
			if (out->m_underlying != 1ULL) {
				out->m_underlying -= 1;
			}
		}
		return true;


	}

	case exrole_t::rbit:
	{
		cs_assert(callinf->args.size() == 1);

		return do_rbit(&callinf->args[0], out);
	}
	}


	return false;
}


static bool compute_commutative_valbits(mop_t* op, potential_valbits_t* out) {
	auto d = op->d;
	mcode_t opc = d->opcode;

	potential_valbits_t lbits = try_compute_opnd_potential_valbits(&d->l),
		rbits = try_compute_opnd_potential_valbits(&d->r);

	if (opc == m_or) {
		out->m_underlying = lbits.m_underlying | rbits.m_underlying;
		return true;
	}
	else if (opc == m_xor) {
		out->m_underlying = lbits.m_underlying | rbits.m_underlying;
		return true;
	}

	else if (opc == m_and) {
		out->m_underlying = lbits.m_underlying & rbits.m_underlying;
		return true;
	}
	else if (opc == m_add) {
		if ((lbits.m_underlying & rbits.m_underlying) == 0) {
			out->m_underlying = lbits.m_underlying | rbits.m_underlying;
		}
		else {
			out->m_underlying = extend_value_by_size_and_sign(~0ULL, op->size, false);//add_bitmag(lbits.m_underlying, rbits.m_underlying, op->size);
		}


		return true;
	}
}

static bool compute_inner_insn_valbits(mop_t* op, potential_valbits_t* out) {
	minsn_t* d = op->d;
	if (mcode_is_set(d->opcode) || d->opcode == m_cfadd || d->opcode == m_ofadd) {
		out->m_underlying = 1ULL;

		return true;
	}

	mulp2_const_t constmul{};

	if (try_extract_mulp2_by_const(d, &constmul)) {
		*out = try_compute_opnd_potential_valbits(constmul.mutable_term());

		out->m_underlying <<= constmul.shiftcount();

		out->m_underlying = extend_value_by_size_and_sign(out->m_underlying, op->size, false);
		return true;
	}

	xdu_extraction_t xduext{};

	if (try_extract_xdu(op, &xduext)) {
		*out = try_compute_opnd_potential_valbits(xduext.xdu_operand());

		out->m_underlying = extend_value_by_size_and_sign(out->m_underlying, xduext.fromsize(), false);
		return true;
	}
	udivp2_const_t constudiv{};

	if (try_extract_udivp2_by_const(d, &constudiv)) {
		*out = try_compute_opnd_potential_valbits(constudiv.mutable_term());

		out->m_underlying >>= constudiv.shiftcount();
		return true;
	}

	if (is_mcode_commutative(d->opcode)) {
		return compute_commutative_valbits(op, out);
	}
	
	exrole_t exrole = get_instruction_exrole(d);

	if (exrole != exrole_t::none) {
		return compute_intrinsic_valbits(op, exrole, out);
	}
	
	return false;
	

}


potential_valbits_t try_compute_opnd_potential_valbits(mop_t* op)
{
	potential_valbits_t out{};

	out.m_underlying = lut_sizemask[_tzcnt_u32(op->size)];
	if (op->t == mop_d) {
		compute_inner_insn_valbits(op, &out);
	}
	else if (op->t == mop_n) {
		out.m_underlying = op->nnn->value;
	}
	else if (op->t == mop_r && op->size == 1 && mreg_is_cc(op->r)) {
		out.m_underlying = 1;
	}

	return out;
}
