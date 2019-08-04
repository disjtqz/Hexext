
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
#include "frame.hpp"

#include "micro_on_70.hpp"
#define __fastcall	

minsn_t* __fastcall mblock_t::insert_into_block( minsn_t* nm, minsn_t* om)
{
	/*
	   if ( (flags & MBL_PUSH) == 0
      && (i->opcode == m_push || i->opcode == m_pop) )
	*/
	minsn_t* v3; // rax
	minsn_t* result; // rax
	minsn_t* v5; // rax

	if (nm->op() == m_push || nm->op() == m_pop) {
		flags |= MBL_PUSH;
	}
	nm->prev = om;
	if (om)
	{
		if (nm->ea == -1i64)
			nm->ea = om->ea;
		nm->next = om->next;
		om->next = nm;
	}
	else
	{
		if (nm->ea == -1i64)
		{
			v3 = this->head;
			if (v3)
				nm->ea = v3->ea;
		}
		nm->next = this->head;
		this->head = nm;
	}
	if (this->tail == om)
	{
		this->tail = nm;
		result = nm;
	}
	else
	{
		v5 = nm->next;
		if (v5)
			v5->prev = nm;
		result = nm;
	}
	return result;
}

void __fastcall mbl_array_t::regenerate_natural()
{
	__int64 v1; // rax
	mbl_array_t* v2; // rbx
	mblock_t** v3; // rax
	mblock_t* v4; // r8
	int v5; // ecx
	__int64 v6; // rdx

	v1 = this->qty;
	v2 = this;
	if ((_DWORD)v1)
	{
		v3 = (mblock_t * *)qrealloc(this->natural, 8 * v1);
		v4 = v2->blocks;
		v5 = 0;
		v2->natural = v3;
		if (v2->qty > 0)
		{
			v6 = 0i64;
			do
			{
				++v6;
				++v5;
				v2->natural[v6 - 1] = v4;
				v4 = v4->nextb;
			} while (v5 < v2->qty);
		}
	}
}

signed __int64 __fastcall mbl_array_t::something_involving_stack_elements( minsn_t* a2)
{
	unsigned __int64 v2; // rbx
	mbl_array_t* v3; // rdi
	minsn_t* v4; // rsi
	func_t* v5; // rax
	signed __int64 v6; // rcx
	signed __int64 result; // rax

	v2 = a2->ea;
	v3 = this;
	v4 = a2;
	v5 = get_func(a2->ea);
	v6 = v3->field_68 + get_spd(v5, v2);

	result = v6 + 8;
	if (!(((unsigned int)v4->iprops >> 2) & 1))
		result = v6;
	return result;
}

size_t __fastcall mblock_t::get_reginsn_qty() const
{
	minsn_t* insn; // rcx
	size_t i; // rax
	int iprops; // er8
	size_t currcount; // rdx

	insn = this->head;
	for (i = 0i64; insn; i = currcount)
	{
		iprops = insn->iprops;
		currcount = i + 1;
		insn = insn->next;
		if (iprops & 0x80)
			currcount = i;
	}
	return i;
}

minsn_t* __fastcall mblock_t::remove_from_block(minsn_t* m)
{
	minsn_t* head; // rax
	minsn_t* tail; // rax
	minsn_t* result; // rax

	if (!m)
		return nullptr;
	head = this->head;
	if (m == head)
		this->head = head->next;
	else {
		if(m->prev)
			m->prev->next = m->next;
	}
	tail = this->tail;
	if (m == tail)
		this->tail = tail->prev;
	else {
		if(m->next)
			m->next->prev = m->prev;
	}
	result = m->next;
	m->prev = 0i64;
	m->next = 0i64;
	this->flags |= 4u;
	if (!this->head)
		this->mba->flags1 |= 0x400u;
	return result;
}
 mblock_t::mblock_t()
{
	mblock_t* result; // rax
	/*
		we yank the vftbl from the first mblock we see in the core codegen function
	*/
#ifdef __EA64__
	this->vtable = mblock_t_vftbl;
	this->field_20 = 0i64;
	this->field_28 = 0i64;
	this->field_30 = 0i64;
	this->field_58 = 0i64;
	this->field_60 = 0;
	this->field_68 = 0i64;
	this->field_70 = 0i64;
	this->field_78 = 0i64;
/*	this->field_80 = 0i64;
	this->field_88 = 0;
	this->field_90 = 0i64;
	this->field_98 = 0i64;
	this->field_A0 = 0i64;
	this->field_A8 = 0i64;
	this->field_B0 = 0;
	this->field_B8 = 0i64;
	this->field_C0 = 0i64;
	this->field_C8 = 0i64;
	this->field_D0 = 0i64;
	this->field_D8 = 0;
	this->field_E0 = 0i64;
	this->field_E8 = 0i64;
	this->field_F0 = 0i64;
	this->field_F8 = 0i64;
	this->field_100 = 0;
	this->field_108 = 0i64;
	this->field_110 = 0i64;
	this->field_118 = 0i64;*/
	this->field_120 = 0i64;
	this->field_128 = 0;
	
	this->nextb = 0i64;
	this->prevb = 0i64;
	this->head = 0i64;
	this->tail = 0i64;
	this->serial = 0;
	this->mba = 0i64;
	this->type = 0;
	this->maxbsp = 0i64;
	this->minbstkref = 0i64;
	this->minbargref = 0i64;
	this->flags = 8;
	this->start = -1i64;
	this->end = -1i64;
#else
	auto a1 = this;
	a1->vtable = mblock_t_vftbl;
	a1->field_20 = 0i64;
	a1->field_28 = 0i64;
	a1->field_30 = 0i64;
	a1->field_50 = 0i64;
	a1->field_58 = 0;
	a1->field_60 = 0i64;
	a1->field_68 = 0i64;
	a1->field_70 = 0i64;
	a1->mustbuse.reg.bitmap = 0i64;
	a1->mustbuse.reg.high = 0;
	a1->mustbuse.mem.bag_.array = 0i64;
	a1->mustbuse.mem.bag_.n = 0i64;
	a1->mustbuse.mem.bag_.alloc = 0i64;
	a1->maybuse.reg.bitmap = 0i64;
	a1->maybuse.reg.high = 0;
	a1->maybuse.mem.bag_.array = 0i64;
	a1->maybuse.mem.bag_.n = 0i64;
	a1->maybuse.mem.bag_.alloc = 0i64;
	a1->mustbdef.reg.bitmap = 0i64;
	a1->mustbdef.reg.high = 0;
	a1->mustbdef.mem.bag_.array = 0i64;
	a1->mustbdef.mem.bag_.n = 0i64;
	a1->mustbdef.mem.bag_.alloc = 0i64;
	a1->maybdef.reg.bitmap = 0i64;
	a1->maybdef.reg.high = 0;
	a1->maybdef.mem.bag_.array = 0i64;
	a1->maybdef.mem.bag_.n = 0i64;
	a1->maybdef.mem.bag_.alloc = 0i64;
	*(_QWORD*)a1->gap_118 = 0i64;
	a1->field_120 = 0;

	a1->nextb = 0i64;
	a1->prevb = 0i64;
	a1->head = 0i64;
	a1->tail = 0i64;
	a1->serial = 0;
	a1->mba = 0i64;
	a1->maxbsp = 0i64;
	*(_QWORD*)& a1->minbstkref = 0i64;
	result = a1;
	a1->flags = 8;
	a1->start = BADADDR;
	a1->end = BADADDR;
#endif

}

 mblock_t* mbl_array_t::create_new_block_for_preopt(mblock_t* after) {

	 std::map<unsigned, mblock_t*> remap_bbidx{};


	 unsigned af = after->serial;

	 mblock_t* result = new mblock_t();
	 result->flags = 0x200;//mbl fake

	 result->prevb = after;
	 result->nextb = after->nextb;
	 if (result->nextb) {
		 result->nextb->prevb = result;
	 }
	 after->nextb = result;
	 this->qty++;
	 for (mblock_t* blk = blocks; blk; blk = blk->nextb) {
		 if (blk->tail && blk->tail->op() == m_jtbl && blk->tail->r.t == mop_c) {
			 for (auto&& bboi : blk->tail->r.c->targets) {
				 remap_bbidx[bboi] = natural[bboi];
			 }
		 }
	 }
	 regenerate_natural();
	 unsigned idx = 0;


	 for (mblock_t* start = blocks; start; start = start->nextb, ++idx) {
		 start->serial = idx;
	 }
	 /*
		gotta remap case table targets if we inserted before one of the cases
		this is just a quick fix, probably could be done muchhh better
	 */
	 for (mblock_t* start = blocks; start; start = start->nextb) {
		 if (start->tail && start->tail->op() == m_jtbl && start->tail->r.t == mop_c) {
			 for (auto&& bboi : start->tail->r.c->targets) {
				 bboi = remap_bbidx[bboi]->serial;
			 }
		 }
	 }

	 result->mba = this;
	 result->minbargref = minargref;
	 result->minbstkref = minstkref;
	 return result;
 }
 mblock_t* dup_mblock(mblock_t* other) {
	 /*
	 	void* vtable;
	mblock_t* nextb;
	mblock_t* prevb;
	int flags;
	_BYTE gap_1C[4];
	_QWORD field_20;
	_QWORD field_28;
	_QWORD field_30;
	ea_t start;
	ea_t end;
	minsn_t* head;
	minsn_t* tail;
	_QWORD field_58;
	_DWORD field_60;
	_BYTE gap_64[4];
	_QWORD field_68;
	_QWORD field_70;
	_QWORD field_78;
	mlist_t mustbuse;
	mlist_t maybuse;
	mlist_t mustbdef;
	mlist_t maybdef;
	_QWORD field_120;
	_DWORD field_128;
	_BYTE gap_12C[4];
	int serial;
	_BYTE gap_134[4];
	mbl_array_t* mba;
	_DWORD type;
	_BYTE gap_144[4];
	_QWORD maxbsp;
	_QWORD minbstkref;
	_QWORD minbargref;
	qvector<int> predset;
	qvector<int> succset;
	 */
	 mblock_t* result = new mblock_t;
	 memset(result, 0, sizeof(mblock_t));
	 result->vtable = other->vtable;
	 result->flags = other->flags;
	 result->start = other->start;
	 result->end = other->end;


	 for (auto i = other->head; i; i = i->next) {
		 result->insert_into_block(new minsn_t(*i), result->tail);
	 }
	 result->serial = other->serial;
	 result->mustbuse.mem.copy(&other->mustbuse.mem);

	 result->maybuse.mem.copy(&other->maybuse.mem);
	 result->mustbdef.mem.copy(&other->mustbdef.mem);
	 result->maybdef.mem.copy(&other->maybdef.mem);

	 result->mustbuse.reg.copy(&other->mustbuse.reg);

	 result->maybuse.reg.copy(&other->maybuse.reg);
	 result->mustbdef.reg.copy(&other->mustbdef.reg);
	 result->maybdef.reg.copy(&other->maybdef.reg);

	 /*
	 _QWORD maxbsp;
	_QWORD minbstkref;
	_QWORD minbargref;
	qvector<int> predset;
	qvector<int> succset;
	 */
	 result->maxbsp = other->maxbsp;
	 result->minbstkref = other->minbstkref;
	 result->minbargref = other->minbargref;
	 result->predset = other->predset;
	 result->succset = other->succset;
	 result->type = other->type;
	 return result;
 }

 void release_dup_mblock(mblock_t* block) {
	 while (block->head) {
		 minsn_t* torel = block->head;
		 block->remove_from_block(block->head);
		 delete torel;
	 }
	 block->mustbuse.~mlist_t();
	 block->maybuse.~mlist_t();
	 block->mustbdef.~mlist_t();
	 block->maybdef.~mlist_t();
	 block->predset.~qvector();
	 block->succset.~qvector();
	 delete block;
 }
 dup_mbl_array_t* dup_mba(mbl_array_t* mba) {

	 mbl_array_t* result = new mbl_array_t;

	 memset(result, 0, sizeof(mbl_array_t));

	 result->natural = (mblock_t**) qalloc(sizeof(mblock_t*) * mba->qty);
	 result->qty = mba->qty;
	 result->flags1 = mba->flags1;
	 for (unsigned i = 0; i < result->qty; ++i) {
		 result->natural[i] = dup_mblock(mba->natural[i]);
		 result->natural[i]->mba = result;
		 if (i != 0) {
			 result->natural[i - 1]->nextb = result->natural[i];
			 result->natural[i]->prevb = result->natural[i - 1];
		 }
	 }

	 result->blocks = result->natural[0];
	 return (dup_mbl_array_t * )result;
 }

 void release_dup_mba(dup_mbl_array_t* mba) {
	 for (unsigned i = 0; i < mba->qty; ++i) {
		 release_dup_mblock(mba->natural[i]);
	 }
	 qfree(mba->natural);
	 delete mba;
 }