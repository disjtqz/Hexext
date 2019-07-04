#pragma once
/*
__int64 __fastcall sub_1B0E40(_QWORD *a1, __int64 a2)
{
  unsigned __int8 v3; // [rsp+0h] [rbp-18h]
  unsigned __int8 v4; // [rsp+1h] [rbp-17h]
  unsigned __int8 v5; // [rsp+2h] [rbp-16h]
  unsigned __int8 v6; // [rsp+3h] [rbp-15h]

  sub_1B0DC0(a1, a2, &v3, 4LL);
  return v3 + (v5 << 16) + (v4 << 8) + ((unsigned int)v6 << 24);
}

	This is another canonicalization rule. We have more opportunities for transformations if these adds were ors
*/
bool combine_byte_shifts_and_adds_to_ors(mblock_t* block, minsn_t* insn);
bool combine_byte_shifts_and_adds_to_ors2(mblock_t* block, minsn_t* insn);