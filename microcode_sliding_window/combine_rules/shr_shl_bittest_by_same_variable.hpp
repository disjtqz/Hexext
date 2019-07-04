#pragma once
//shr(and r10.8, (shl r13.8, rbx.1).8).8, rbx.1, rax.8

bool combine_shr_shl_bittest_by_same_variable(mblock_t* block, minsn_t* insn);