
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


#include "micro_on_70.hpp"
#define __fastcall	
void  mop_t::erase()
{
	mop_addr_t* v2; // rdi
	minsn_t* v3; // rdi
	mcallinfo_t* v4; // rdi
	mcases_t* v5; // rdi
	mop_pair_t* v6; // rdi
	scif_t* v7; // rdi

	switch ((unsigned __int8)this->t)
	{
	case mop_n:
		qfree(this->nnn);
		break;
	case mop_str:
	case mop_h:
		qfree(this->nnn);
		break;
	case mop_d:
		v3 = this->d;
		delete v3;
		break;
	case mop_S:
		qfree(this->nnn);
		break;
	case mop_f:
		v4 = this->f;
		if (v4)
		{
			delete v4;
		}
		break;
	case mop_l:
		qfree(this->nnn);
		break;
	case mop_a:
		v2 = this->a;
		if (v2)
		{
			this->a->erase();
			qfree(v2);
		}
		break;
	case mop_c:
		v5 = this->c;
		if (v5)
		{
			delete v5;
		}
		break;
	case mop_fn:
		qfree(this->nnn);
		break;
	case mop_p:
		v6 = this->pair;
		if (v6)
		{
			delete v6;
		}
		break;
	case mop_sc:
		v7 = this->scif;
		if (v7)
		{
			delete v7;
		}
		break;
	default:
		break;
	}
	this->t = 0;

	this->oprops = 0;
	this->valnum = 0;
	this->size = -1;
}

void hexapi mop_t::copy(const mop_t& rop) {
	const mop_t* v2; // rdi
	mop_t* v3; // rbx
	int v4; // er8
	minsn_t* v5; // rax
	mnumber_t* v6; // rcx
	__int64 v7; // r8
	__int64 v8; // r9
	mnumber_t* v9; // rcx

	v2 = &rop;
	v3 = this;
	this->size = rop.size;
	v4 = (unsigned __int8)rop.t;
	this->t = v4;
	this->oprops = rop.oprops;
	this->valnum = rop.valnum;
	switch (rop.t) {
	case mop_z:
		return;
	case mop_n:
		nnn = new mnumber_t(*rop.nnn);
		break;

	case mop_str:
		cstr = qstrdup(rop.cstr);
		break;

	case mop_h:
		helper = qstrdup(rop.helper);
		break;
	case mop_d:
		d = new minsn_t(*rop.d);
		break;
	case mop_S:
		s = new stkvar_ref_t(*rop.s);
		break;


	case mop_fn:
		fpc = new fnumber_t(*rop.fpc);
		break;
	case mop_f:
		f = new mcallinfo_t(*rop.f);
		break;
	case mop_l:
		l = new lvar_ref_t(*rop.l);
		break;
	case mop_a:
		a = new mop_addr_t(*rop.a);
		break;
	case mop_c:
		c = new mcases_t(*rop.c);
		break;
	case mop_p:
		pair = new mop_pair_t(*rop.pair);
		break;
	case mop_sc:
		scif = new scif_t(*rop.scif);
		break;

	default:
		d = rop.d;
		break;

	}
}

void __fastcall mop_t::swap(  mop_t& rop)
{
	mopt_t v3; // r8
	char v5; // r8
	unsigned __int16 v6; // ax
	unsigned __int16 v7; // dx
	int v8; // ecx
	mnumber_t* v9; // rcx
	mop_t* other = &rop;
	v3 = this->t;
	this->t = other->t;
	other->t = v3;
	v5 = this->oprops;
	this->oprops = other->oprops;
	other->oprops = v5;
	v6 = other->valnum;
	v7 = this->valnum;
	this->valnum = v6;
	other->valnum = v7;
	v8 = this->size;
	this->size = other->size;
	other->size = v8;
	v9 = this->nnn;
	this->nnn = other->nnn;
	other->nnn = v9;
}

bool __fastcall mop_t::is_constant(uint64_t* out, bool is_signed) const
{
	const mop_t* targetmop; // rax
	minsn_t* v4; // rcx

	targetmop = this;
	if (this->size > 8u && this->t == mop_d)
	{
		v4 = this->d;
		if (v4->opcode == m_xds)
			targetmop = &v4->l;
	}
	if (targetmop->t != mop_n)
		return 0;
	if (out)
		* out = extend_value_by_size_and_sign(targetmop->nnn->org_value, targetmop->size, is_signed);
	return 1;
}