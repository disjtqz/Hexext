#include "microgen_defs.hpp"
#include "fixup_bittest_codegen.hpp"

microfilter_fixup_bittest_t bittest_fixup{};

bool microfilter_fixup_bittest_t::match(codegen_ex_t& cdg) {
	auto& in = cdg.insn;

	if (in.itype != NN_bt)
		return false;

	/*if (in.ops[0].type != o_reg) {
		return false;
	}*/



	return true;

}
int microfilter_fixup_bittest_t::apply(codegen_ex_t& cdg) {

	auto unitsize = get_dtype_size(cdg.insn.ops[0].dtype);

	ea_t addr = cdg.insn.ea;
	mblock_t* mb = cdg.mb;
	minsn_t* shifter = new minsn_t(addr);
	shifter->opcode = m_and;

	if (cdg.insn.ops[1].type == o_imm) {
		shifter->l.nnn = new mnumber_t(1ULL << cdg.insn.ops[1].value, cdg.insn.ea);
		shifter->l.t = mop_n;
		
	}
	else {
		minsn_t* innershifter = new minsn_t(addr);

		innershifter->opcode = m_shl;
		innershifter->l.t = mop_n;
		innershifter->l.nnn = new mnumber_t(1ULL, addr);
		innershifter->l.size = unitsize;

		innershifter->r.t = mop_r;
		innershifter->r.r = cdg.load_operand(1);//reg2mreg(cdg.insn.ops[1].reg);
		innershifter->r.size = 1;

		shifter->l.t = mop_d;
		shifter->l.d = innershifter;

		innershifter->d.size = unitsize;

	}
	shifter->l.size = unitsize;
	shifter->r.t = mop_r;

	if (cdg.insn.ops[0].type == o_reg) {
		shifter->r.r = cdg.load_operand(0);//reg2mreg(cdg.insn.ops[0].reg);
	}
	else {
		shifter->r.r = cdg.load_operand(0);
	}
	shifter->r.size = unitsize;

	shifter->d.size = unitsize;

	minsn_t* tester = cdg.emit_blank();


	tester->opcode = m_setnz;

	tester->l.t = mop_d;
	tester->l.size = unitsize;
	tester->l.d = shifter;

	tester->r.t = mop_n;
	tester->r.size = unitsize;
	tester->r.nnn = new mnumber_t(0ULL, addr);

	tester->d.size = 1;
	tester->d.t = mop_r;
	tester->d.r = mr_cf;

	//OF, SF, AF, and PF

	cdg.undefine_flag(mr_of);
	cdg.undefine_flag(mr_sf);
	cdg.undefine_flag(mr_pf);
	return MERR_OK;



}
