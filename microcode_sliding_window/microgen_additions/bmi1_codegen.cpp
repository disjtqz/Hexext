#include "microgen_defs.hpp"
#include "bmi1_codegen.hpp"





bool tzcnt_generator_t::match(codegen_ex_t& cdg) {
	return cdg.insn.itype == NN_tzcnt;
}

int tzcnt_generator_t::apply(codegen_ex_t& cdg) {



	unsigned unitsize = get_dtype_size(cdg.insn.ops[0].dtype);

	mreg_t srcreg = cdg.load_operand(1);
	mreg_t dstreg = cdg.load_operand(0);
	ea_t addr = cdg.insn.ea;
	
	minsn_t* zfsetter = cdg.emit_blank();

	zfsetter->opcode = m_setnz;


	zfsetter->d.t = mop_r;
	zfsetter->d.size = 1;
	zfsetter->d.r = mr_zf;


	zfsetter->r.t = mop_n;
	zfsetter->r.nnn = new mnumber_t(0ULL);
	zfsetter->r.size = unitsize;

	minsn_t* innerzfsetter = new minsn_t(addr);

	innerzfsetter->opcode = m_and;

	innerzfsetter->l.t = mop_r;
	innerzfsetter->l.size = unitsize;
	innerzfsetter->l.r = srcreg;

	innerzfsetter->r.t = mop_n;
	innerzfsetter->r.nnn = new mnumber_t(1ULL);
	innerzfsetter->r.size = unitsize;

	innerzfsetter->d.size = unitsize;

	zfsetter->l.t = mop_d;
	zfsetter->l.size = unitsize;
	zfsetter->l.d = innerzfsetter;


	minsn_t* cfsetter = cdg.emit_blank();
	cfsetter->opcode = m_setz;

	cfsetter->l.make_reg(srcreg, unitsize);


	cfsetter->r.make_number(0ULL, unitsize, cdg.insn.ea);

	cfsetter->d.t = mop_r;
	cfsetter->d.r = mr_cf;
	cfsetter->d.size = 1;

	/*
		this should all be refactored into a new_helper function 
	*/

	mcallarg_t anarg{};
	anarg.argloc._set_reg1(srcreg);

	anarg.name = "src";
	anarg.type = get_int_type_by_width_and_sign(unitsize, type_unsigned);
	anarg.ea = cdg.insn.ea;

	anarg.t = mop_r;

	anarg.size = unitsize;
	anarg.r = srcreg;

	tinfo_t rettype = get_int_type_by_width_and_sign(unitsize, type_unsigned);

	rlist_t retregs{};
	argloc_t retargloc{};
	mlist_t spoiled{};


	mop_t retreg{};

	retreg.size = unitsize;
	retreg.t = mop_r;
	retreg.r = dstreg;
	retregs.add_(dstreg, unitsize);
	spoiled.reg.add_(dstreg, unitsize);
	retargloc.set_reg1(dstreg);

	minsn_t* callins = cdg.emit_blank();

	new_helper(callins, unitsize == 4 ? "_tzcnt_u32" : "_tzcnt_u64", unitsize, 1, & anarg, & rettype, & retargloc, & retregs, & spoiled, 1, & retreg);



	cdg.undefine_flag(mr_pf);
	cdg.undefine_flag(mr_of);
	//fuck your pf
	cdg.undefine_flag(mr_sf);
	return MERR_OK;
}


bool blsr_generator_t::match(codegen_ex_t & cdg) {
	if (cdg.insn.itype == NN_blsr)
		return true;
	return false;
}

int blsr_generator_t::apply(codegen_ex_t & cdg) {

	unsigned unitsize = get_dtype_size(cdg.insn.ops[0].dtype);


	mreg_t src = cdg.load_operand(1);

	ea_t addr = cdg.insn.ea;



	mreg_t dst = cdg.load_operand(0);//reg2mreg(cdg.insn.ops[0].reg);
	minsn_t* operation = cdg.emit_blank();

	operation->opcode = m_and;

	operation->d.size = unitsize;
	operation->d.r = dst;
	operation->d.t = mop_r;

	operation->l.t = mop_r;
	operation->l.size = unitsize;
	operation->l.r = src;

	minsn_t* inner_sub = new minsn_t(addr);
	/*
		fun story: i forgot to set this guy m_sub while writing this.
		the instruction actually made it all the way to the initial ctree generation before triggering an interr

		and it made this whole process take an extra 10 minutes for such a simple instruction
	*/
	inner_sub->opcode = m_sub;
	inner_sub->l.t = mop_r;
	inner_sub->l.r = src;
	inner_sub->l.size = unitsize;

	inner_sub->r.t = mop_n;
	inner_sub->r.nnn = new mnumber_t(1ULL, addr, 3);
	inner_sub->r.size = unitsize;


	inner_sub->d.size = unitsize;
	operation->r.t = mop_d;
	operation->r.size = unitsize;
	operation->r.d = inner_sub;

	/*
		we got yo initial calculation done, but now we gotta set some flags

	*/

	minsn_t* sf_setter = cdg.emit_blank();

	sf_setter->opcode = m_sets;

	sf_setter->l.t = mop_r;
	sf_setter->l.r = dst;
	sf_setter->l.size = unitsize;

	sf_setter->d.t = mop_r;
	sf_setter->d.r = mr_sf;
	sf_setter->d.size = 1;

	minsn_t* zf_setter = cdg.emit_blank();

	zf_setter->opcode = m_setz;

	zf_setter->l.t = mop_r;
	zf_setter->l.r = dst;
	zf_setter->l.size = unitsize;

	zf_setter->r.t = mop_n;
	zf_setter->r.nnn = new mnumber_t(0ULL, cdg.insn.ea, 2);

	zf_setter->r.size = unitsize;

	zf_setter->d.t = mop_r;
	zf_setter->d.r = mr_zf;

	zf_setter->d.size = 1;

	/*
		whoo we gotta set of to 0

		when i started writing this ones codegen i honestly expected it to go much faster
		i didnt take all the flags into account

	*/
	minsn_t* zeroof = cdg.emit_blank();
	zeroof->opcode = m_mov;

	zeroof->l.make_number(0ULL, 1, cdg.insn.ea, 4);

	zeroof->d.make_reg(mr_of, 1);



	/*
		oh great we also have to set cf to 1 if the source is zero fuck me

		at this point, i finally implemented make_number and make_reg because im feelin that car-pool tunnal
	*/


	minsn_t* cf_setter = cdg.emit_blank();
	cf_setter->opcode = m_setz;

	cf_setter->l.make_reg(src, unitsize);

	cf_setter->r.make_number(0ULL, unitsize, addr, 5);

	cf_setter->d.make_reg(mr_cf, 1);
	cdg.undefine_flag(mr_pf);
	return MERR_OK;
	//thank god that shit is over


}

tzcnt_generator_t tzcnt_generator{};
blsr_generator_t blsr_generator{};