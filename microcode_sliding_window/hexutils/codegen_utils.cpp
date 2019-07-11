
#include <ida.hpp>
#include <idp.hpp>
#include <loader.hpp>
#include <kernwin.hpp>
#include <hexrays.hpp>
#include <functional>
#include "../micro_on_70.hpp"
#include "../mvm/mvm.hpp"
#include "hexutils.hpp"


minsn_t* chain_setops_with_or(std::vector<minsn_t*>& chain, ea_t iea) {
	bool is_odd_boi = chain.size() & 1;

	if (chain.size() == 1)
		return chain[0];

	else {
		minsn_t* pos = new minsn_t(iea);
		mop_t* nextguy = &pos->l;

		pos->opcode = m_or;


		nextguy->t = mop_d;
		nextguy->size = 1;
		nextguy->d = chain[0];

		pos->r.t = mop_d;
		pos->r.size = 1;
		pos->r.d = chain[1];
		pos->d.size = 1;



		for (unsigned i = 1; i < chain.size() / 2; i++) {
			minsn_t* pos2 = new minsn_t(iea);

			pos2->opcode = m_or;
			pos2->l.t = mop_d;
			pos2->l.size = 1;
			pos2->l.d = chain[i * 2];

			pos2->r.t = mop_d;
			pos2->r.size = 1;
			pos2->r.d = chain[(i * 2) + 1];
			pos2->d.size = 1;
			minsn_t* join = new minsn_t(iea);

			join->opcode = m_or;

			join->l.assign_insn(pos2, 1);
			join->r.assign_insn(pos, 1);
			join->d.size = 1;

			pos = join;

		}

		if (is_odd_boi) {

			minsn_t* finl = new minsn_t(iea);
			finl->opcode = m_or;

			finl->l.assign_insn(pos, 1);

			finl->r.assign_insn(chain[chain.size() - 1], 1);

			finl->d.size = 1;
			pos = finl;
		}
		return pos;
	}
}


CS_NOINLINE
static void make_setcc(minsn_t* into, mcode_t setop, mop_t* l, mop_t* r) {
	into->opcode = setop;

	into->d.erase();

	into->l = *l;
	if (r)
		into->r = *r;
	into->d.size = 1;

}

void setup_subinsn_setcc_of_size(minsn_t* into, mcode_t setop, unsigned size, mop_t* l, mop_t* r) {

	cs_assert(mcode_is_set(setop));

	cs_assert(into);
	cs_assert(into->ea != BADADDR);
	cs_assert((int)size > 0);
	cs_assert(l);
	cs_assert((setop == m_sets && r == nullptr && l != nullptr) || (setop != m_sets && r != nullptr && l != nullptr));


	//into->opcode = setop;

	if (size == 1) {
		make_setcc(into, setop, l, r);
	}
	else {
		minsn_t* inner = new minsn_t(into->ea);
		make_setcc(inner, setop, l, r);
		into->l.erase();
		into->r.erase();
		into->d.erase();
		
		into->opcode = m_xdu;

		into->l.t = mop_d;
		into->l.d = inner;
		into->l.size = 1;

		into->d.size = size;
	}
}

void setup_bitand(minsn_t* andtest, mop_t* l, mop_t* r) {
	cs_assert(andtest && l && r && l->t != mop_z && r->t != mop_z && l->size == r->size && (signed)l->size > 0);
	andtest->opcode = m_and;

	andtest->l = *l;
	andtest->r = *r;
	andtest->d.erase();
	andtest->d.size = andtest->r.size;
}

void setup_bitor(minsn_t* andtest, mop_t* l, mop_t* r) {
	cs_assert(andtest && l && r && l->t != mop_z && r->t != mop_z && l->size == r->size && (signed)l->size > 0);
	andtest->opcode = m_or;

	andtest->l = *l;
	andtest->r = *r;
	andtest->d.erase();
	andtest->d.size = andtest->r.size;
}
void setup_ztest_bitand(minsn_t* into, bool z, mop_t* l, mop_t* r, unsigned size ){
	cs_assert(into && (int)size > 0 && l && r && l->size == r->size && into->ea != BADADDR);

	minsn_t* andtest = new minsn_t(into->ea);
	setup_bitand(andtest, l, r);


	mop_t andop{};
	andop.size = andtest->r.size;
	andop.t = mop_d;
	andop.d = andtest;

	mop_t ztest{};
	ztest.make_number(0ULL, andtest->r.size);

	setup_subinsn_setcc_of_size(into, z ? m_setz : m_setnz, size, &andop, &ztest);

}

void insert_mov2_before(ea_t ea, mblock_t* blk, minsn_t* before, mop_t* from, mop_t* to) {
	
	if (before) {
		before = before->prev;

	}

	minsn_t* moveeboi = new minsn_t(ea);
	moveeboi->opcode = m_mov;

	moveeboi->l = *from;
	moveeboi->d = *to;

	blk->insert_into_block(moveeboi, before);

}