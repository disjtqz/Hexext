
X(m_nop, 0x00)// nop                       // no operation
X(m_und, 0x01)// stx  l,    {r=sel, d=off} // store register to memory     *F
X(m_ext, 0x02)// ldx  {l=sel,r=off}, d     // load register from memory    *F
X(m_ijmp, 0x03)// ldc  l=const,     d       // load constant
X(m_stx, 0x04)// mov  l,           d       // move                         *F
X(m_ldx, 0x05)// neg  l,           d       // negate
X(m_ldc, 0x06)// lnot l,           d       // logical not
X(m_mov, 0x07)// bnot l,           d       // bitwise not
X(m_neg, 0x08)// xds  l,           d       // extend (signed)
X(m_xds, 0x09)// xdu  l,           d       // extend (unsigned)
X(m_xdu, 0x0A)// low  l,           d       // take low part
X(m_low, 0x0B)// high l,           d       // take high part
X(m_high, 0x0C)// add  l,   r,      d       // l + r -> dst
X(m_sets, 0x0D)// sub  l,   r,      d       // l - r -> dst
X(m_setp, 0x0E)// mul  l,   r,      d       // l * r -> dst
X(m_setnz, 0x0F)// udiv l,   r,      d       // l / r -> dst
X(m_setz, 0x10)// sdiv l,   r,      d       // l / r -> dst
X(m_setae, 0x11)// umod l,   r,      d       // l % r -> dst
X(m_setb, 0x12)// smod l,   r,      d       // l % r -> dst
X(m_seta, 0x13)// or   l,   r,      d       // bitwise or
X(m_setbe, 0x14)// and  l,   r,      d       // bitwise and
X(m_setg, 0x15)// xor  l,   r,      d       // bitwise xor
X(m_setge, 0x16)// shl  l,   r,      d       // shift logical left
X(m_setl, 0x17)// shr  l,   r,      d       // shift logical right
X(m_setle, 0x18)// sar  l,   r,      d       // shift arithmetic right
X(m_seto, 0x19)// cfadd l,  r,    d=carry   // calculate carry    bit of (l+r)
X(m_lnot, 0x1A)// ofadd l,  r,    d=overf   // calculate overflow bit of (l+r)
X(m_add, 0x1B)// cfshl l,  r,    d=carry   // calculate carry    bit of (l<<r)
X(m_sub, 0x1C)// cfshr l,  r,    d=carry   // calculate carry    bit of (l>>r)
X(m_mul, 0x1D)// sets  l,          d=byte  SF=1          Sign
X(m_udiv, 0x1E)// seto  l,  r,      d=byte  OF=1          Overflow of (l-r)
X(m_sdiv, 0x1F)// setp  l,  r,      d=byte  PF=1          Unordered/Parity  *F
X(m_umod, 0x20)// setnz l,  r,      d=byte  ZF=0          Not Equal         *F
X(m_smod, 0x21)// setz  l,  r,      d=byte  ZF=1          Equal             *F
X(m_bnot, 0x22)// setae l,  r,      d=byte  CF=0          Above or Equal    *F
X(m_or, 0x23)// setb  l,  r,      d=byte  CF=1          Below             *F
X(m_and, 0x24)// seta  l,  r,      d=byte  CF=0 & ZF=0   Above             *F
X(m_xor, 0x25)// setbe l,  r,      d=byte  CF=1 | ZF=1   Below or Equal    *F
X(m_shl, 0x26)// setg  l,  r,      d=byte  SF=OF & ZF=0  Greater
X(m_rol, 0x27)// setge l,  r,      d=byte  SF=OF         Greater or Equal
X(m_unk11, 0x28)// setl  l,  r,      d=byte  SF!=OF        Less
X(m_shr, 0x29)// setle l,  r,      d=byte  SF!=OF | ZF=1 Less or Equal
X(m_sar, 0x2A)// jcnd   l,         d       // d is mop_v or mop_b
X(m_ror, 0x2B)// jnz    l, r,      d       // ZF=0          Not Equal      *F
X(m_u2f, 0x2C)// jz     l, r,      d       // ZF=1          Equal          *F
X(m_cfadd, 0x2D)// jae    l, r,      d       // CF=0          Above or Equal *F
X(m_ofadd, 0x2E)// jb     l, r,      d       // CF=1          Below          *F
X(m_cfshl, 0x2F)// ja     l, r,      d       // CF=0 & ZF=0   Above          *F
X(m_cfshr, 0x30)// jbe    l, r,      d       // CF=1 | ZF=1   Below or Equal *F
X(m_f2u, 0x31)// jg     l, r,      d       // SF=OF & ZF=0  Greater
X(m_unk2, 0x32)// jge    l, r,      d       // SF=OF         Greater or Equal
X(m_push, 0x33)// jl     l, r,      d       // SF!=OF        Less
X(m_pop, 0x34)// jle    l, r,      d       // SF!=OF | ZF=1 Less or Equal
X(m_goto, 0x35)// jtbl   l, r=mcases        // Table jump
X(m_jcnd, 0x36)// ijmp       {r=sel, d=off} // indirect unconditional jump
X(m_jnz, 0x37)// goto   l                  // l is mop_v or mop_b
X(m_jz, 0x38)// call   l          d       // l is mop_v or mop_b or mop_h
X(m_jae, 0x39)// icall  {l=sel, r=off} d   // indirect call
X(m_jb, 0x3A)// ret
X(m_ja, 0x3B)// push   l
X(m_jbe, 0x3C)// pop               d
X(m_jg, 0x3D)// und               d       // undefine
X(m_jge, 0x3E)// ext  in1, in2,  out1      // external insn, not microcode *F
X(m_jl, 0x3F)// f2i     l,    d      int(l) => d; convert fp -> integer   +F
X(m_jle, 0x40)// f2u     l,    d      uint(l)=> d; convert fp -> uinteger  +F
X(m_jtbl, 0x41)// i2f     l,    d      fp(l)  => d; convert integer -> fp e +F
X(m_call, 0x42)// i2f     l,    d      fp(l)  => d; convert uinteger -> fp  +F
X(m_icall, 0x43)// f2f     l,    d      l      => d; change fp precision     +F
X(m_ret, 0x44)// fneg    l,    d      -l     => d; change sign             +F
X(m_f2i, 0x45)// fadd    l, r, d      l + r  => d; add                     +F
X(m_unk3, 0x46)// fsub    l, r, d      l - r  => d; subtract                +F
X(m_i2f, 0x47)// fmul    l, r, d      l * r  => d; multiply                +F
X(m_unk1, 0x48)// fdiv    l, r, d      l / r  => d; divide                  +F
X(m_f2f, 73)
X(m_fneg, 74)
X(m_fadd, 75)
X(m_fsub, 76)
X(m_fmul, 77)
X(m_fdiv, 78)
