
#include <ida.hpp>
#include <idp.hpp>
#include <loader.hpp>
#include <kernwin.hpp>
#include <ieee.h>
#include <hexrays.hpp>
#include "micro_on_70.hpp"
#define __fastcall	


void hexapi minsn_t::copy(const minsn_t& m) {
	const minsn_t* v2; // rbx
	const mop_t* v3; // rbp
	const mop_t* v4; // rsi
	minsn_t* v5; // rdi
	__int64 result; // rax
	const minsn_t* a2 = &m;

	if (a2 != this)
	{
		v2 = a2;
		v3 = &a2->l;
		v4 = &this->l;
		v5 = this;
		this->l = m.l;
		this->r = m.r;
		this->d = m.d;
		
		v5->ea = v2->ea;
		v5->opcode = v2->opcode;
		result = (unsigned int)v2->iprops;
		v5->iprops = result;
	}
}

void __fastcall minsn_t::swap( minsn_t* other) {
	mopt_t v2; // r8
	char v4; // r8
	unsigned __int16 v5; // cx
	int v6; // ecx
	mnumber_t* v7; // rcx
	mnumber_t* v8; // rcx
	mnumber_t* v9; // rcx
	unsigned __int64 v10; // rcx

	v2 = this->l.t;
	this->l.t = other->l.t;
	other->l.t = v2;
	v4 = this->l.oprops;
	this->l.oprops = other->l.oprops;
	other->l.oprops = v4;
	v5 = this->l.valnum;
	this->l.valnum = other->l.valnum;
	other->l.valnum = v5;
	v6 = this->l.size;
	this->l.size = other->l.size;
	other->l.size = v6;
	v7 = this->l.nnn;
	this->l.nnn = other->l.nnn;
	other->l.nnn = v7;
	LOBYTE(v7) = this->r.t;
	this->r.t = other->r.t;
	other->r.t = (char)v7;
	LOBYTE(v7) = this->r.oprops;
	this->r.oprops = other->r.oprops;
	other->r.oprops = (char)v7;
	LOWORD(v7) = this->r.valnum;
	this->r.valnum = other->r.valnum;
	other->r.valnum = (unsigned __int16)v7;
	LODWORD(v7) = this->r.size;
	this->r.size = other->r.size;
	other->r.size = (signed int)v7;
	v8 = this->r.nnn;
	this->r.nnn = other->r.nnn;
	other->r.nnn = v8;
	LOBYTE(v8) = this->d.t;
	this->d.t = other->d.t;
	other->d.t = (char)v8;
	LOBYTE(v8) = this->d.oprops;
	this->d.oprops = other->d.oprops;
	other->d.oprops = (char)v8;
	LOWORD(v8) = this->d.valnum;
	this->d.valnum = other->d.valnum;
	other->d.valnum = (unsigned __int16)v8;
	LODWORD(v8) = this->d.size;
	this->d.size = other->d.size;
	other->d.size = (signed int)v8;
	v9 = this->d.nnn;
	this->d.nnn = other->d.nnn;
	other->d.nnn = v9;
	v10 = this->ea;
	this->ea = other->ea;
	other->ea = v10;
	LODWORD(v10) = this->opcode;
	this->opcode = other->opcode;
	other->opcode = (mcode_t)v10;
	LODWORD(v10) = this->iprops;
	this->iprops = other->iprops;
	other->iprops = v10;
}

mop_t* __fastcall minsn_t::find_num_op(mop_t** other)
{
	mop_t* result; // rax

	if (this->r.t == mop_n)
	{
		if (other)
			* other = &this->l;
		result = &this->r;
	}
	else
	{
		result = &this->l;
		if (this->l.t == mop_n)
		{
			if (other)
				* other = &this->r;
		}
		else
		{
			result = NULL;
		}
	}
	return result;
}


minsn_t* new_helper(
	minsn_t* callins,
	const char* helper_name,
	unsigned res_size,
	unsigned nargs,
	mcallarg_t* args,
	tinfo_t* return_type,
	argloc_t* return_argloc,
	rlist_t* return_regs,
	mlist_t* spoiled,
	unsigned nretregs,
	mop_t* retregs) {

	mcallinfo_t* tzcnt_callinfo = new mcallinfo_t();

	tzcnt_callinfo->callee = BADADDR;
	tzcnt_callinfo->solid_args = nargs;

	for (unsigned i = 0; i < nargs; ++i) {
		tzcnt_callinfo->args.push_back(args[i]);
	}
	tzcnt_callinfo->cc =
		112;
	tzcnt_callinfo->return_type = *return_type;

	tzcnt_callinfo->return_argloc = *return_argloc;

	tzcnt_callinfo->return_regs.reg.add(*return_regs);
	tzcnt_callinfo->spoiled.reg.add(spoiled->reg);
	tzcnt_callinfo->spoiled.mem.add_(&spoiled->mem);
	//tzcnt_callinfo->pass_regs.reg.add_(dstreg, unitsize);
	//pass regs triggers interr 51087


	for (unsigned i = 0; i < nretregs; ++i)
		tzcnt_callinfo->retregs.push_back(retregs[i]);



	callins->opcode = m_call;

	callins->l.t = mop_h;
	callins->l.helper = qstrdup(helper_name);
	callins->d.t = mop_f;
	callins->d.size = res_size;
	callins->d.f = tzcnt_callinfo;

	return callins;
}