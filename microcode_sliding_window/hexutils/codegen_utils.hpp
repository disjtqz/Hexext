#pragma once

minsn_t* chain_setops_with_or(std::vector<minsn_t*>& chain, ea_t iea);


/*
	gotta generate a set instruction but we might need to extend
	using xdu

	r is unneeded if the mcode is sets
*/
void setup_subinsn_setcc_of_size(minsn_t* into, mcode_t setop, unsigned size, mop_t* l, mop_t* r = nullptr);

void setup_bitand(minsn_t* andtest, mop_t* l, mop_t* r);
void setup_bitor(minsn_t* andtest, mop_t* l, mop_t* r);
/*
	
	generate setnz/z (x & y), 0 to size
	size is the size of the result of the setcc op, not the size for the bitand
*/

void setup_ztest_bitand(minsn_t* into, bool z,  mop_t* l, mop_t* r, unsigned size = 1);

void insert_mov2_before(ea_t ea,mblock_t* blk, minsn_t* before, mop_t* from, mop_t* to);
