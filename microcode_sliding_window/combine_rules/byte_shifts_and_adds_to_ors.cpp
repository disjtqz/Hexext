#include "combine_rule_defs.hpp"
#include "../hexutils/hexutils.hpp"
//add (add (xdu stkvar_0.1, .-1).4, (add (shl (xdu stkvar_2.1, .-1).4, #16.1).4, (shl (xdu stkvar_1.1, .-1).4, #8.1).4).4).4, (shl (xdu stkvar_3.1, .-1).4, #24.1).4

//add (shl (xdu stkvar_2.1, .-1).4, #16.1).4, (shl (xdu stkvar_1.1, .-1).4, #8.1).4).4
bool combine_byte_shifts_and_adds_to_ors(mblock_t* block, minsn_t* insn) {


	if ((insn->opcode != m_add && insn->opcode != m_xor) || !insn->both(mop_d))
		return false;


	

	mulp2_const_t leftmul{}, rightmul{};


	if (!try_extract_mulp2_by_const(insn->l.d, &leftmul) || !try_extract_mulp2_by_const(insn->r.d, &rightmul))
		return false;



	xdu_extraction_t leftxdu{}, rightxdu{};


	if (!try_extract_xdu(leftmul.mutable_term(), &leftxdu) || !try_extract_xdu(rightmul.mutable_term(), &rightxdu))
		return false;

	/*
		ensure both terms of the addition operation are zero extended from the same size, that both are shifted by multiples of the 
		size they were extended from and that their shifts would not cause their bits to overlap

		if this holds true, the add/xor may become an or because the bits will never overlap
	*/
	if (leftmul.shiftcount() == rightmul.shiftcount() || leftxdu.fromsize() != rightxdu.fromsize() ||
		leftmul.shiftcount() % leftxdu.fromsize() || rightmul.shiftcount() % rightxdu.tosize()) {
		return false;
	}

	insn->opcode = m_or;

	return true;
}

bool combine_byte_shifts_and_adds_to_ors2(mblock_t* block, minsn_t* insn) {

	if ((insn->opcode != m_add && insn->opcode != m_xor) || !insn->both(mop_d))
		return false;

	potential_valbits_t lbits{};
	potential_valbits_t rbits{};

	lbits = try_compute_opnd_potential_valbits(&insn->l);
	rbits = try_compute_opnd_potential_valbits(&insn->r);

	if ((lbits.value() & rbits.value()) == 0) {
		insn->opcode = m_or;
		return true;
	}

	return false;

}