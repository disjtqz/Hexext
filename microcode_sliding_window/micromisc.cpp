
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
#include "cs_core.hpp"
#define __fastcall

uint64_t extend_value_by_size_and_sign(uint64_t value, unsigned int size, bool sign)
{
	unsigned int v4; // ecx
	signed __int64 v5; // rdx
	signed __int64 v6; // rax


	if ((signed int)size < 8)
	{
		v4 = 8 * size - 1;
		if (v4 < 0x40)
			v5 = 1i64 << v4;
		else
			v5 = 0i64;
		v6 = 2 * v5 - 1;
		if (sign && v5 & value)
			return ~v6 | value;
		value &= v6;
	}
	return value;
}

unsigned int __fastcall get_signed_mcode(unsigned int a1)
{
	unsigned int result; // eax

	switch (a1)
	{
	case 0xAu:
		result = 9;
		break;
	case 0x11u:
		result = 22;
		break;
	case 0x12u:
		result = 23;
		break;
	case 0x13u:
		result = 21;
		break;
	case 0x14u:
		result = 24;
		break;
	case 0x1Eu:
		result = 31;
		break;
	case 0x20u:
		result = 33;
		break;
	case 0x39u:
		result = 62;
		break;
	case 0x3Au:
		result = 63;
		break;
	case 0x3Bu:
		result = 61;
		break;
	case 0x3Cu:
		result = 64;
		break;
	default:
		result = a1;
		break;
	}
	return result;
}

unsigned int __fastcall swap_mcode_relation(int a1)
{
	unsigned int result; // eax

	switch (a1)
	{
	case 17:
		result = 20;
		break;
	case 18:
		result = 19;
		break;
	case 19:
		result = 18;
		break;
	case 20:
		result = 17;
		break;
	case 21:
		result = 23;
		break;
	case 22:
		result = 24;
		break;
	case 23:
		result = 21;
		break;
	case 24:
		result = 22;
		break;
	case 57:
		result = 60;
		break;
	case 58:
		result = 59;
		break;
	case 59:
		result = 58;
		break;
	case 60:
		result = 57;
		break;
	case 61:
		result = 63;
		break;
	case 62:
		result = 64;
		break;
	case 63:
		result = 61;
		break;
	case 64:
		result = 62;
		break;
	default:
		result = 0;
		break;
	}
	return result;
}

bool __fastcall is_mcode_propagatable(int a1)
{
	bool result; // al

	switch (a1)
	{
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
	case 16:
	case 17:
	case 18:
	case 19:
	case 20:
	case 21:
	case 22:
	case 23:
	case 24:
	case 25:
	case 26:
	case 27:
	case 28:
	case 29:
	case 30:
	case 31:
	case 32:
	case 33:
	case 34:
	case 35:
	case 36:
	case 37:
	case 38:
	case 39:
	case 41:
	case 42:
	case 43:
	case 45:
	case 46:
	case 47:
	case 48:
	case 49:
	case 50:
	case 66:
	case 67:
	case 69:
	case 70:
	case 71:
	case 72:
	case 73:
	case 74:
	case 75:
	case 76:
	case 77:
	case 78:
		result = 1;
		break;
	default:
		result = 0;
		break;
	}
	return result;
}

bool __fastcall must_mcode_close_block(int a1, bool a2)
{
	if (a1 == 3 || (unsigned int)(a1 - 53) <= 0xC || a1 == 68)
		return 1;
	if (a2 && (unsigned int)(a1 - 66) <= 1)
		return 1;
	return 0;
}




mcode_t __fastcall negate_mcode_relation(mcode_t a1)
{
	mcode_t result; // eax

	switch (a1)
	{
	case m_setnz:
		result = m_setz;
		break;
	case m_setz:
		result = m_setnz;
		break;
	case m_setae:
		result = m_setb;
		break;
	case m_setb:
		result = m_setae;
		break;
	case m_seta:
		result = m_setbe;
		break;
	case m_setbe:
		result = m_seta;
		break;
	case m_setg:
		result = m_setle;
		break;
	case m_setge:
		result = m_setl;
		break;
	case m_setl:
		result = m_setge;
		break;
	case m_setle:
		result = m_setg;
		break;
	case m_jnz:
		result = m_jz;
		break;
	case m_jz:
		result = m_jnz;
		break;
	case m_jae:
		result = m_jb;
		break;
	case m_jb:
		result = m_jae;
		break;
	case m_ja:
		result = m_jbe;
		break;
	case m_jbe:
		result = m_ja;
		break;
	case m_jg:
		result = m_jle;
		break;
	case m_jge:
		result = m_jl;
		break;
	case m_jl:
		result = m_jge;
		break;
	case m_jle:
		result = m_jg;
		break;
	default:
		result = m_nop;
		break;
	}
	return result;
}

mcode_t __fastcall swap_mcode_relation(mcode_t a1)
{
	mcode_t result; // eax

	switch (a1)
	{
	case m_setae:
		result = m_setbe;
		break;
	case m_setb:
		result = m_seta;
		break;
	case m_seta:
		result = m_setb;
		break;
	case m_setbe:
		result = m_setae;
		break;
	case m_setg:
		result = m_setl;
		break;
	case m_setge:
		result = m_setle;
		break;
	case m_setl:
		result = m_setg;
		break;
	case m_setle:
		result = m_setge;
		break;
	case m_jae:
		result = m_jbe;
		break;
	case m_jb:
		result = m_ja;
		break;
	case m_ja:
		result = m_jb;
		break;
	case m_jbe:
		result = m_jae;
		break;
	case m_jg:
		result = m_jl;
		break;
	case m_jge:
		result = m_jle;
		break;
	case m_jl:
		result = m_jg;
		break;
	case m_jle:
		result = m_jge;
		break;
	default:
		result = m_nop;
		break;
	}
	return result;
}

mcode_t __fastcall add_equality_condition_to_opcc(mcode_t a1)
{
	mcode_t result; // eax

	switch (a1)
	{
	case m_setb:
		result = m_setbe;
		break;
	case m_seta:
		result = m_setae;
		break;
	case m_setg:
		result = m_setge;
		break;
	case m_setl:
		result = m_setle;
		break;
	case m_jb:
		result = m_jbe;
		break;
	case m_ja:
		result = m_jae;
		break;
	case m_jg:
		result = m_jge;
		break;
	case m_jl:
		result = m_jle;
		break;
	default:
		result = a1;
		break;
	}
	return result;
}

bool mcode_is_set(mcode_t arg) {
	return arg >= m_sets && arg <= m_seto;
}

/*
	gotta check to see if this works in each version
	iterator should be in rdx when the function returns so we can extract the iterator and
	get the actual cfunc
*/
cfunc_t* get_cached_cfunc(ea_t ea) {
	struct retval {
		uint64_t rax;
		cfunc_t** rdx;
	};
	retval r = reinterpret_cast<retval(*)(ea_t)>(has_cached_cfunc)(ea);
	if (!(r.rax & 0xFF))
		return nullptr;
	else {
		return reinterpret_cast<cfunc_t*>(r.rdx[5]);
	}
}