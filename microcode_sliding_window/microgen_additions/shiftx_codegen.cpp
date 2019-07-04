#include "microgen_defs.hpp"

#include "shiftx_codegen.hpp"



bool shiftx_generator_t::match(codegen_ex_t& cdg) {
	int itype = cdg.insn.itype;

	return itype == NN_sarx || itype == NN_shlx || itype == NN_shrx;
}

int shiftx_generator_t::apply(codegen_ex_t& cdg) {
	mcode_t code;
	int itype = cdg.insn.itype;

	if (itype == NN_sarx) {
		code = m_sar;
	}
	else if (itype == NN_shlx)
		code = m_shl;
	else {
		code = m_shr;
	}

	unsigned sz = get_dtype_size(cdg.insn.ops[0].dtype);
	mreg_t dest = cdg.load_operand(0);// reg2mreg(cdg.insn.ops[0].reg);

	mreg_t shifter = cdg.load_operand(2);//reg2mreg(cdg.insn.ops[2].reg);


	mreg_t src;

	//if (cdg.insn.ops[1].type == o_reg) {
		//src = cdg.load//reg2mreg(cdg.insn.ops[1].reg);
	//}
//	else {
		src = cdg.load_operand(1);
//	}

	minsn_t* insn = cdg.emit_blank();

	insn->opcode = code;

	insn->l.t = mop_r;
	insn->r.t = mop_r;
	insn->d.t = mop_r;

	insn->l.size = sz;
	insn->r.size = 1;
	insn->d.size = sz;

	insn->l.r = src;
	insn->r.r = shifter;
	insn->d.r = dest;

	return MERR_OK;




}

shiftx_generator_t shiftx_generator{};