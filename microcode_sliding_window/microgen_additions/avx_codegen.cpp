#include "microgen_defs.hpp"

#include "avx_codegen.hpp"


static bool avx_op_has_valid_dest(insn_t& ins) {
	return  ins.ops[0].type == o_reg;
}

static bool avx_operands_are_xmmonly(insn_t& ins) {

	for (auto&& opnd : ins.ops) {
		if (opnd.type == o_void) {
			return true;
		}
		if (get_dtype_size(opnd.dtype) != 16) {
			return false;
		}
	}
	return true;

}

static unsigned n_operands_for_insn(insn_t& insn) {
	for (unsigned i = 0; i < UA_MAXOP; ++i) {
		if (insn.ops[i].type == o_void) {
			return i;
		}
	}
	return UA_MAXOP;
}


static bool is_okay_xmm_avx_operation(insn_t& insn) {


	return avx_op_has_valid_dest(insn) && avx_operands_are_xmmonly(insn) && n_operands_for_insn(insn) == 3;
}

static mcode_t mcode_for_avxop(int itype) {
	switch (itype) {
	case NN_vaddss:
		return m_fadd;
	case NN_vsubss:
		return m_fsub;

	case NN_vdivss:
		return m_fdiv;

	case NN_vmulss:
		return m_fmul;
	}
}

bool avx_floatops_generator_t::match(codegen_ex_t& cdg) {

	int itype = cdg.insn.itype;


	if (itype == NN_vaddss || itype == NN_vsubss /*|| itype == NN_vdivss || itype == NN_vmulss*/) {
		if (is_okay_xmm_avx_operation(cdg.insn))
			return true;
	}

	return false;
}

static void translate_operand(codegen_ex_t& cdg, op_t& op, mop_t* out) {

	if (op.type == o_reg) {
		out->r = reg2mreg(op.reg);
		out->t = mop_r;
	}
	else if (op.type == o_imm) {
		out->t = mop_n;
		out->nnn = new mnumber_t(op.value);
	}
	else {
		out->r = cdg.load_operand(op.n);
		out->t = mop_r;
	}

	out->size = get_dtype_size(op.dtype);


}


int avx_floatops_generator_t::apply(codegen_ex_t& cdg) {
	mcode_t opc = mcode_for_avxop(cdg.insn.itype);

	

	mop_t l, r, d;

	unsigned n_ops = n_operands_for_insn(cdg.insn);

	if (n_ops == 3) {
		translate_operand(cdg, cdg.insn.ops[1], &l);
		translate_operand(cdg, cdg.insn.ops[2], &r);
		translate_operand(cdg, cdg.insn.ops[0], &d);

	}

	l.oprops |= OPROP_FLOAT;
	r.oprops |= OPROP_FLOAT;
	d.oprops |= OPROP_FLOAT;
	minsn_t* expr = cdg.emit_blank();

	expr->opcode = opc;

	std::swap(expr->l, l);
	std::swap(expr->r, r);
	std::swap(expr->d, d);
/*	expr->l = l;
	expr->r = r;
	expr->d = d;*/

	return MERR_OK;

}

avx_floatops_generator_t g_floatops_generator{};

bool rewrite_xmmops_with_avx(codegen_ex_t& cdg) {
	int itype = cdg.insn.itype;
	unsigned sz = get_dtype_size(cdg.insn.ops[0].dtype);
	op_t& op0 = cdg.insn.ops[0];
	op_t& op1 = cdg.insn.ops[1];
	op_t& op2 = cdg.insn.ops[2];

	if (itype == NN_vcomiss) {
		if (sz == 16) {
			cdg.insn.itype = NN_comiss;
			return true;
		}
	}
	else if (itype == NN_vcomisd) {
		if (sz == 16) {
			cdg.insn.itype = NN_comisd;
			return true;
		}
	}
	else if (itype == NN_vmovss) {
		if ((sz == 16 || get_dtype_size(cdg.insn.ops[1].dtype) == 16) && op2.type == o_void) {
			cdg.insn.itype = NN_movss;
			return true;
		}
	}
	else if (itype == NN_vmovaps) {
		if (sz == 16) {
			cdg.insn.itype = NN_movaps;
			return true;
		}
	}
	else if (itype == NN_vxorps) {
		if (sz == 16 && op0.type == op1.type && op1.type == op2.type && op1.type == o_reg && op1.reg == op2.reg && op0.reg == op1.reg) {

			cdg.insn.itype = NN_xorps;
			op2.type = o_void;
			return true;
		}
	}
	return false;
}