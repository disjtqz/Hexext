
#include <ida.hpp>
#include <idp.hpp>
#include <loader.hpp>
#include <kernwin.hpp>
#include <ieee.h>
#include <hexrays.hpp>
#include <functional>
#include <array>
#include <list>

#include "micro_on_70.hpp"
#include "hexdefs.hpp"
#define __fastcall
void  bitset_t::resize( int a2)
{
	bitset_t* v2=this; // rbx
	signed int v3; // edi
	unsigned int* v4=nullptr; // rsi
	void** local_28; // [rsp+20h] [rbp-28h]
	const char* local_20; // [rsp+28h] [rbp-20h]
	__int64 local_18; // [rsp+30h] [rbp-18h]


	if (a2 >= (signed int)this->high)
	{
		v3 = (a2 + 32) & 0xFFFFFFE0;
		v4 = (unsigned int*)qrealloc(this->bitmap, v3 / 8);
		
		memset(&v4[(signed int)v2->high / 32], 0, (signed int)(v3 - v2->high) / 8);
		v2->bitmap = (mbitmap_t*)v4;
		v2->high = v3;
	}
}
 bitset_t::bitset_t( const bitset_t* m)
{
	const bitset_t* v2; // rdi
	bitset_t* v3; // rbx
	int v4; // edx

	v2 = m;
	this->high = 0;
	v3 = this;
	this->bitmap = 0i64;
	if (this != m)
	{
		v4 = m->high;
		if (!v4)
		{
			this->high = 0;
			return;
		}
		this->resize(v4);
		memcpy(v3->bitmap, v2->bitmap, (signed int)v2->high / 8);
		v3->high = v2->high;
	}
	return;
}

 int  bitset_t::goup( int reg)const
 {
	 int v2; // er8
	 int v3; // er9
	 int v4; // eax
	 int* v5; // r10
	 int* v6; // rdx
	 int result; // eax
	 __int64 v8; // rcx
	 int i; // edx

	 v2 = (signed int)this->high / 32;
	 v3 = reg & 0x1F;
	 v4 = reg / 32;
	 if (reg / 32 >= v2 || (v5 = this->bitmap, !(this->bitmap[v4] >> v3)))
	 {
		 if (++v4 >= v2)
			 return this->high;
		 v5 = this->bitmap;
		 v3 = 0;
		 v6 = &this->bitmap[v4];
		 while (!*v6)
		 {
			 ++v4;
			 ++v6;
			 if (v4 >= v2)
				 return this->high;
		 }
	 }
	 v8 = v4;
	 result = v3 + 32 * v4;
	 for (i = v5[v8] >> v3; !(i & 1); i >>= 1)
		 ++result;
	 return result;
 }

 bool  bitset_t::empty() const
 {
	 int* v1; // r8
	 int v2; // eax
	 int v3; // ecx

	 v1 = this->bitmap;
	 v2 = (signed int)this->high / 32 - 1;
	 if (v2 < 0)
		 return 1;
	 while (1)
	 {
		 v3 = *v1;
		 ++v1;
		 if (v3)
			 break;
		 if (--v2 < 0)
			 return 1;
	 }
	 return 0;
 }

 void  bitset_t::fill_with_ones( int maxbit)
 {
	 int v2; // ebp
	 bitset_t* v3; // rsi
	 void* result; // rax
	 int v5; // ebx
	 int v6; // edi
	 __int64 v7; // rcx

	 v2 = maxbit;
	 v3 = this;

	 resize( maxbit);

	 result = memset(v3->bitmap, -1, 4i64 * (v2 / 32));
	 v5 = 32 * (v2 / 32);
	 if (v5 <= v2)
	 {
		 v6 = __ROL4__(1, v5);
		 do
		 {
			 resize( v5);

			 v7 = v5 / 32;
			 result = (void*)(unsigned int)v3->bitmap[v7];
			 if (!((unsigned int)result & v6))
			 {
				 result = (void*)(v6 | (unsigned int)result);
				 v3->bitmap[v7] = (signed int)result;
			 }
			 ++v5;
			 v6 = __ROL4__(v6, 1);
		 } while (v5 <= v2);
	 }
	 return;
 }

bool  bitset_t::has(  int a2) const
 {
	 int v3; // ecx

	 if (a2 < 0 || a2 >= (signed int)this->high)
		 return 0;
	 v3 = this->bitmap[a2 / 32];
	 return _bittest((const long*)&v3, a2 & 0x1F);
 }

bool  bitset_t::has_all( int bit, int width) const
{
	unsigned __int8 v5; // al
	int v6; // ecx

	if (--width < 0)
		return 1;
	while (1)
	{
		if (bit++ >= 0)
		{
			if (bit < (signed int)this->high)
			{
				v6 = this->bitmap[bit / 32];
				v5 = _bittest((const long*)&v6, bit & 0x1F);
			}
			else
			{
				v5 = 0;
			}
		}
		else
		{
			v5 = 0;
		}
		if (!v5)
			break;
		if (--width < 0)
			return 1;
	}
	return 0;
}

bool  bitset_t::has_any( int bit, int width) const
{
	unsigned __int8 v5; // al
	int v6; // ecx

	if (--width < 0)
		return 0;
	while (1)
	{
		if (bit++ >= 0)
		{
			if (bit < (signed int)this->high)
			{
				v6 = this->bitmap[bit / 32];
				v5 = _bittest((const long*)&v6, bit & 0x1F);
			}
			else
			{
				v5 = 0;
			}
		}
		else
		{
			v5 = 0;
		}
		if (v5)
			break;
		if (--width < 0)
			return 0;
	}
	return 1;
}

bool  bitset_t::has_common( const bitset_t* ml) const
{
	signed int v2; // eax
	int* v3; // r8
	int v4; // eax
	char* v5; // r9
	bool v6; // zf

	v2 = ml->high;
	if ((signed int)this->high < v2)
		v2 = this->high;
	if (v2 <= 0)
		return 0;
	v3 = ml->bitmap;
	v4 = v2 / 32 - 1;
	if (v4 < 0)
		return 0;
	v5 = (char*)((char*)this->bitmap - (char*)v3);
	while (1)
	{
		v6 = (*v3 & *(int*)((_BYTE*)v3 + (_QWORD)v5)) == 0;
		++v3;
		if (!v6)
			break;
		if (--v4 < 0)
			return 0;
	}
	return 1;
}

bool __fastcall bitset_t::add(int bit)
{
	signed int v4; // er8
	__int64 v5; // rcx
	int v6; // eax

	resize( bit);
	v4 = 1 << (bit & 0x1F);
	v5 = bit / 32;
	v6 = this->bitmap[v5];
	if (v6 & v4)
		return 0;
	this->bitmap[v5] = v4 | v6;
	return 1;
}
bool __fastcall bitset_t::add_( int bit, int width)
{
	int v3; // ebx
	int v6; // esi
	int v7; // eax
	unsigned int v8; // ebx
	__int64 v9; // r11
	int v10; // eax
	int v11; // esi
	unsigned int v12; // edx
	signed __int64 v13; // rbp
	int v14; // er9
	signed int v15; // eax
	int v16; // er10
	__int64 v17; // rcx
	int v18; // er8
	int v19; // edx
	int v20; // er11
	signed __int64 v21; // rcx
	bool v22; // r8
	bool v23; // zf
	signed int v24; // ecx
	__int64 v25; // rdx
	int v26; // edx
	int v27; // ecx
	char v28; // al

	v3 = bit;
	
	if (!width)
		return 0;
	v6 = bit + width - 1;
	
	bitset_t::resize( v6);
	v7 = v3;
	v8 = v3 & 0x1F;
	v9 = v7 / 32;
	v10 = v6;
	v11 = v6 & 0x1F;
	v12 = 31;
	v13 = v10 / 32;
	if ((_DWORD)v9 == (_DWORD)v13)
		v12 = v11;
	v14 = 0;
	v15 = 1 << v8;
	v16 = 0;
	if (v8 <= v12)
	{
		v17 = v12 - v8 + 1;
		do
		{
			v16 |= v15;
			v15 *= 2;
			--v17;
		} while (v17);
	}
	v18 = this->bitmap[v9];
	v19 = v16 | this->bitmap[v9];
	this->bitmap[v9] = v19;
	v20 = v9 + 1;
	v21 = v20;
	v22 = v19 != v18;
	if (v20 < v13)
	{
		v20 = v13;
		do
		{
			v23 = this->bitmap[v21] == -1;
			this->bitmap[v21] = -1;
			if (!v23)
				v22 = 1;
			++v21;
		} while (v21 < v13);
	}
	if (v20 == (_DWORD)v13)
	{
		v24 = 1;
		v25 = (unsigned int)(v11 + 1);
		do
		{
			v14 |= v24;
			v24 *= 2;
			--v25;
		} while (v25);
		v26 = this->bitmap[v13];
		v27 = v14 | this->bitmap[v13];
		this->bitmap[v13] = v27;
		v28 = v22;
		if (v27 != v26)
			v28 = 1;
		v22 = v28;
	}
	return v22;
}

bool __fastcall bitset_t::add__( const bitset_t* ml)
{
	bool v3; // bl
	signed int v4; // edx
	int* v6; // r9
	int* v7; // r10
	int i; // eax
	int v9; // edx
	int v10; // er8

	v3 = 0;
	v4 = ml->high;
	if (v4 > 0)
	{
		bitset_t::resize(v4 - 1);
		v6 = this->bitmap;
		v7 = ml->bitmap;
		for (i = (signed int)ml->high / 32 - 1; i >= 0; *(v6 - 1) = v10 | v9)
		{
			v9 = *v6;
			++v6;
			v10 = *v7;
			++v7;
			if (~v9 & v10)
				v3 = 1;
			--i;
		}
	}
	return v3;
}

bool bitset_t::sub_17195690()
{
	signed int v1; // er9
	int v3; // eax
	int* i; // rcx
	bool v5; // zf

	v1 = this->high;
	if (v1 > 0)
	{
		v3 = v1 / 32 - 1;
		for (i = &this->bitmap[v1 / 32]; v3 >= 0; --v3)
		{
			v5 = *(i - 1) == 0;
			--i;
			if (!v5)
				break;
			this->high -= 32;
		}
	}
	return v1 != this->high;
}

bool __fastcall bitset_t::sub( int bit)
{
	signed int v3; // er8
	__int64 v4; // rcx
	int v5; // edx

	
	if (bit >= (signed int)this->high)
		return 0;
	v3 = 1 << (bit & 0x1F);
	v4 = bit / 32;
	v5 = this->bitmap[v4];
	if (!(v5 & v3))
		return 0;
	this->bitmap[v4] = v5 & ~v3;
	sub_17195690();
	return 1;
}

bool __fastcall bitset_t::cut_at( int maxbit)
{
	int v3; // kr00_4
	int v4; // edx
	int v5; // eax

	
	if (maxbit >= (signed int)this->high)
		return 0;
	this->high = maxbit;
	v3 = maxbit;
	v4 = (maxbit >> 31) & 0x1F;
	v5 = ((v4 + v3) & 0x1F) - v4;
	if (((v4 + v3) & 0x1F) != v4)
	{
		this->bitmap[(v4 + v3) >> 5] &= (1 << v5) - 1;
		this->high = (this->high + 31) & 0xFFFFFFE0;
	}

	bitset_t::sub_17195690();
	return 1;
}

bool __fastcall bitset_t::sub_( int bit, int width)
{
	char v6; // bl
	int i; // er11
	signed int v8; // edx
	__int64 v9; // rcx
	int v10; // er8
	signed int v11; // eax
	int v12; // eax
	__int64 v13; // rdx
	int v14; // eax
	signed __int64 j; // rcx
	bool v16; // zf
	char v17; // al

	if (bit + width >= (signed int)this->high)
		return bitset_t::cut_at( bit);
	v6 = 0;
	for (i = width - 1; i >= 0; --i)
	{
	
		if (bit < (signed int)this->high && (v8 = 1 << (bit & 0x1F), v9 = bit / 32, v10 = this->bitmap[v9], v10 & v8))
		{
			this->bitmap[v9] = v10 & ~v8;
			v11 = this->high;
			if (v11 > 0)
			{
				v12 = v11 / 32;
				v13 = v12;
				v14 = v12 - 1;
				for (j = (signed __int64)& this->bitmap[v13]; v14 >= 0; --v14)
				{
					v16 = *(_DWORD*)(j - 4) == 0;
					j -= 4i64;
					if (!v16)
						break;
					this->high -= 32;
				}
			}
			v17 = 1;
		}
		else
		{
			v17 = 0;
		}
		v6 |= v17;
		++bit;
	}
	return v6;
}

bool __fastcall bitset_t::sub__( const bitset_t* ml)
{
	signed int v2; // eax
	char v3; // r10
	int* v4; // r11
	int* v5; // r8
	int i; // eax
	int v7; // edx
	bool v8; // zf
	int v9; // edx
	signed int v10; // er11
	int v11; // eax
	int* j; // r8

	v2 = ml->high;
	v3 = 0;
	if ((signed int)this->high < v2)
		v2 = this->high;
	if (v2 <= 0)
		return 0;
	v4 = ml->bitmap;
	v5 = this->bitmap;
	for (i = v2 / 32 - 1; i >= 0; --i)
	{
		v7 = *v4;
		++v4;
		v8 = (v7 & *v5) == 0;
		++v5;
		v9 = ~v7;
		if (!v8)
			v3 = 1;
		*(v5 - 1) &= v9;
	}
	v10 = this->high;
	if (v10 > 0)
	{
		v11 = v10 / 32 - 1;
		for (j = &this->bitmap[v10 / 32]; v11 >= 0; --v11)
		{
			v8 = *(j - 1) == 0;
			--j;
			if (!v8)
				break;
			this->high -= 32;
		}
	}
	return v3 | (v10 != this->high);
}

int __fastcall bitset_t::count() const
{
	int v1; // er8
	int v2; // eax
	__int64 v3; // rbx
	int* v4; // r11
	int v5; // er9
	signed int v6; // edx
	signed __int64 v7; // r10
	int v8; // ecx
	int v9; // eax

	v1 = 0;
	v2 = (signed int)this->high / 32;
	v3 = v2;
	if (v2 > 0)
	{
		v4 = this->bitmap;
		do
		{
			v5 = *v4;
			if (*v4)
			{
				if (v5 == -1)
				{
					v1 += 32;
				}
				else
				{
					v6 = 1;
					v7 = 16i64;
					do
					{
						v8 = v1 + 1;
						if (!(v6 & v5))
							v8 = v1;
						v9 = 2 * v6;
						v6 *= 4;
						v1 = v8 + 1;
						if (!(v5 & v9))
							v1 = v8;
						--v7;
					} while (v7);
				}
			}
			++v4;
			--v3;
		} while (v3);
	}
	return v1;
}

int __fastcall bitset_t::count_( int bit) const
{
	int i; // eax
	int v4; // edx

	for (i = bit; i >= 0; ++i)
	{
		if (i >= (signed int)this->high)
			break;
		v4 = this->bitmap[i / 32];
		if (!_bittest((const long*)&v4, i & 0x1F))
			break;
	}
	return i - bit;
}

int __fastcall bitset_t::last() const
{
	int v1; // er8
	int result; // eax
	bool i; // sf
	int v4; // edx

	v1 = this->high;
	result = v1 - 1;
	for (i = v1 - 1 < 0; !i; i = result < 0)
	{
		if (result >= 0 && result < v1)
		{
			v4 = this->bitmap[result / 32];
			if (_bittest((const long*)&v4, result & 0x1F))
				break;
		}
		--result;
	}
	return result;
}

bool __fastcall bitset_t::intersect( const bitset_t* ml)
{
	signed int v2; // eax
	bool v3; // r10
	signed int v5; // eax
	int* v6; // r8
	int* v7; // rcx
	int i; // er11
	signed int v9; // eax
	int v10; // eax
	__int64 v11; // rdx
	int v12; // eax
	signed __int64 j; // r8
	bool v14; // zf

	v2 = ml->high;
	v3 = 0;
	if ((signed int)this->high > v2)
	{
		v3 = 1;
		this->high = v2;
	}
	v5 = this->high;
	v6 = ml->bitmap;
	v7 = this->bitmap;
	for (i = v5 / 32 - 1; i >= 0; --i)
	{
		if (~*v6 & *v7)
		{
			v3 = 1;
			*v7 &= *v6;
		}
		++v6;
		++v7;
	}
	v9 = this->high;
	if (v9 > 0)
	{
		v10 = v9 / 32;
		v11 = v10;
		v12 = v10 - 1;
		for (j = (signed __int64)& this->bitmap[v11]; v12 >= 0; --v12)
		{
			v14 = *(_DWORD*)(j - 4) == 0;
			j -= 4i64;
			if (!v14)
				break;
			this->high -= 32;
		}
	}
	return v3;
}
bool __fastcall bitset_t::is_subset_of( const bitset_t* ml) const
{
	signed int v2; // eax
	int* v3; // r8
	int v4; // eax
	char* v5; // r9
	int v6; // ecx
	int v7; // edx

	v2 = this->high;
	if (v2 <= (signed int)ml->high)
	{
		v3 = ml->bitmap;
		v4 = v2 / 32 - 1;
		if (v4 < 0)
			return 1;
		v5 = (char*)((char*)this->bitmap - (char*)v3);
		while (1)
		{
			v6 = *v3;
			v7 = *(int*)((char*)v3 + (_QWORD)v5);
			++v3;
			if (~v6 & v7)
				break;
			if (--v4 < 0)
				return 1;
		}
	}
	return 0;
}

int __fastcall bitset_t::compare_( const bitset_t* rhs) const
{
	signed int v2; // eax
	signed int v3; // edx
	signed int v4; // ecx
	char result; // al

	v2 = this->high;
	if (v2 == rhs->high)
	{
		if (v2)
			result = memcmp(this->bitmap, rhs->bitmap, v2 / 8);
		else
			result = 0;
	}
	else
	{
		v3 = rhs->high;
		v4 = this->high;
		if (v2 >= v3)
			result = v4 > v3;
		else
			result = -1;
	}
	return result;
}
int bitset_t::compare(const bitset_t& rhs) const {
	return compare_(&rhs);
}

void __fastcall bitset_t::copy(const bitset_t* other)
{
	int v4; // edx

	if (this != other)
	{
		v4 = other->high;
		if (!v4)
		{
			this->high = 0;
			return ;
		}
		bitset_t::resize( v4);
		memcpy(this->bitmap, other->bitmap, (signed int)other->high / 8);
		this->high = other->high;
	}
}



bool ivlset_t::has(ivl_t& ivl) {

	for (auto&& i : bag) {
		if (i.includes(ivl))
			return true;
	}

	return false;

}

void ivlset_t::add(ivlset_t& other) {
	for (auto&& i : other) {
		add(i);
	}
}

void __fastcall sub_170850F0(ivlset_t* a1, size_t a2)
{
	ivl_t* v4; // rax
	ivl_t local_18; // [rsp+20h] [rbp-18h]

	if (a2 >= a1->bag_.n)
	{
		if (a2 > a1->bag_.alloc)
			a1->bag_.array = (ivl_t*)qvector_reserve(a1, a1->bag_.array, a2, sizeof(ivl_t));
		for (; a1->bag_.n < a2; ++a1->bag_.n)
		{
			v4 = &a1->bag_.array[a1->bag_.n];
			if (v4)
				* v4 = local_18;
		}
	}
	else
	{
		a1->bag_.n = a2;
	}
}
void __fastcall sub_170EE4B0(ivlset_t* a, ivl_t* a2, ivl_t* a3)
{
	ivl_t* v3; // rsi
	ivlset_t* v4; // rbx
	signed __int64 v5; // rdi
	unsigned __int64 v6; // r8
	ivl_t* v7; // rdi

	v3 = a3;
	v4 = a;
	v5 = (char*)a2 - (char*)a->bag_.array;
	v6 = a->bag_.n + 1;
	if (v6 > a->bag_.alloc)
		a->bag_.array = (ivl_t*)qvector_reserve(a, a->bag_.array, v6, 16i64);
	v7 = (ivl_t*)((char*)v4->bag_.array + (v5 & 0xFFFFFFFFFFFFFFF0ui64));
	memmove(
		&v7[1],
		v7,
		((_QWORD)v4->bag_.array + 16 * v4->bag_.n - (_QWORD)v7) & 0xFFFFFFFFFFFFFFF0ui64);
	if (v7)
		* v7 = *v3;
	++v4->bag_.n;
}

void __fastcall sub_170EE270(ivlset_t* a1, ivl_t* a2, ivl_t* a3)
{
	ivl_t* v3; // rdi
	ivl_t* v4; // rsi
	ivlset_t* v5; // rbx

	v3 = a3;
	v4 = a2;
	v5 = a1;
	memmove(
		a2,
		a3,
		((_QWORD)a1->bag_.array + 16 * a1->bag_.n - (_QWORD)a3) & 0xFFFFFFFFFFFFFFF0ui64);
	v5->bag_.n -= v3 - v4;
}

bool __fastcall ivlset_t::add_( ivlset_t* ivs)
{
	ivl_t* v2; // r14
	bool v3; // bp
	unsigned __int64 v6; // r9
	unsigned __int64 v7; // r10
	unsigned __int64 v8; // rcx
	unsigned __int64 v9; // rax
	unsigned __int64 v10; // rax
	unsigned __int64 v11; // rax
	unsigned __int64 v12; // rsi
	unsigned __int64 v13; // rcx
	unsigned __int64 v14; // rdi
	ivl_t* v15; // rdx
	unsigned __int64 v16; // r8
	signed __int64 v17; // rdi
	ivl_t* v18; // rdi
	char v20; // si
	unsigned __int64 v21; // r11
	__int64 v22; // rcx
	unsigned __int64 v23; // r10
	unsigned __int64 v24; // rax

	v2 = ivs->bag_.array;
	v3 = 0;
	if (ivs->bag_.array != &ivs->bag_.array[ivs->bag_.n])
	{
		v6 = allmem.size;
		v7 = allmem.off;
		do
		{
			v8 = v2->size;
			if (v8)
			{
				v9 = v2->off;
				if (v2->off >= v7 && v2->off <= v7 && v8 >= v6 && v8 <= v6)
				{
					if (this->bag_.n == 1)
					{
						v10 = this->bag_.array->off;
						if (v10 >= v7 && v10 <= v7)
						{
							v11 = this->bag_.array->size;
							if (v11 >= v6 && v11 <= v6)
								goto LABEL_25;
						}
					}
					sub_170850F0(this, 1ui64);
					*this->bag_.array = allmem;
				LABEL_24:
					v6 = allmem.size;
					v3 = 1;
					v7 = allmem.off;
					goto LABEL_25;
				}
				v12 = v9 + v8;
				if (v9 + v8 >= v9)
				{
					v13 = this->bag_.n;
					v14 = 0i64;
					if (!v13)
						goto LABEL_19;
					v15 = this->bag_.array;
					while (1)
					{
						v16 = v15->off + v15->size;
						if (v16 >= v9)
							break;
						++v14;
						++v15;
						if (v14 >= v13)
							goto LABEL_19;
					}
					if (v15->off <= v12)
					{
						v20 = 0;
						if (v15->off > v9)
						{
							v15->off = v9;
							v20 = 1;
							v6 = allmem.size;
							v7 = allmem.off;
						}
						if (v16 >= v2->off + v2->size)
						{
							if (!v20)
								goto LABEL_25;
						}
						else
						{
							v16 = v2->off + v2->size;
						}
						v21 = v15->off;
						v15->size = v16 - v15->off;
						LODWORD(v16) = v14 + 1;
						if ((signed int)v14 + 1 < this->bag_.n)
						{
							v22 = (signed int)v16;
							do
							{
								v23 = v15->size;
								if (v23 + v21 < this->bag_.array[v22].off)
									break;
								v24 = this->bag_.array[v22].off
									+ this->bag_.array[v22].size
									- v21;
								if (v24 > v23)
									v15->size = v24;
								LODWORD(v16) = v16 + 1;
								++v22;
							} while ((signed int)v16 < this->bag_.n);
						}
						v16 = (signed int)v16;
						if (v14 + 1 != (signed int)v16)
							sub_170EE270(this, &this->bag_.array[v14 + 1], &this->bag_.array[v16]);
					}
					else
					{
					LABEL_19:
						v17 = sizeof(ivl_t) * v14;
						if (v13 + 1 > this->bag_.alloc)
							this->bag_.array = (ivl_t*)qvector_reserve(
								this,
								this->bag_.array,
								v13 + 1,
								sizeof(ivl_t));
						v18 = (ivl_t*)((char*)this->bag_.array + (v17 & 0xFFFFFFFFFFFFFFF0ui64));
						memmove(
							&v18[1],
							v18,
							((_QWORD)this->bag_.array + sizeof(ivl_t) * this->bag_.n - (_QWORD)v18) & 0xFFFFFFFFFFFFFFF0ui64);
						if (v18)
							* v18 = *v2;
						++this->bag_.n;
					}
					goto LABEL_24;
				}
			}
		LABEL_25:
			++v2;
		} while (v2 != &ivs->bag_.array[ivs->bag_.n]);
	}
	return v3;
}
bool __fastcall ivlset_t::has_common_( const ivlset_t* ivs)
{
	unsigned __int64 v2; // rdi
	unsigned __int64 v3; // rbx
	ivl_t* v4; // r8
	ivl_t* v5; // rax
	unsigned __int64 v6; // r10
	unsigned __int64 v7; // r11
	unsigned __int64 v8; // r9

	v2 = this->bag_.n;
	if (!v2)
		return 0;
	v3 = ivs->bag_.n;
	if (!v3)
		return 0;
	v4 = this->bag_.array;
	v5 = ivs->bag_.array;
	while (1)
	{
	LABEL_4:
		v6 = v4->off;
		v7 = v4->size;
		v8 = v5->off;
		if (v5->off < v7 + v4->off && v6 < v8 + v5->size)
			return 1;
		if (v7 + v6 <= v8)
			break;
	LABEL_9:
		if (v8 + v5->size <= v4->off)
		{
			while (1)
			{
				++v5;
				if (v5 == &ivs->bag_.array[v3])
					return 0;
				if (v5->off + v5->size > v4->off)
					goto LABEL_4;
			}
		}
	}
	while (1)
	{
		++v4;
		if (v4 == &this->bag_.array[v2])
			return 0;
		if (v4->off + v4->size > v8)
			goto LABEL_9;
	}
}

int __fastcall ivlset_t::compare( ivlset_t* ivs)
{
#ifdef __EA64__
	unsigned __int64 v2; // r11
	unsigned __int64 v3; // r8
	int result; // eax
	int v5; // er10
	ivl_t* v6; // rcx
	unsigned __int64* v7; // r9
	char* v8; // rbx
	unsigned __int64 v9; // r8
	unsigned __int64 v10; // rax

	v2 = this->bag_.n;
	v3 = ivs->bag_.n;
	if (v2 == v3)
	{
		v5 = 0;
		if (v2)
		{
			v6 = this->bag_.array;
			v7 = &ivs->bag_.array->size;
			v8 = (char*)((char*)ivs->bag_.array - (char*)v6);
			while (1)
			{
				v9 = *(unsigned __int64*)((char*)& v6->off + (_QWORD)v8);
				if (v6->off < v9)
					break;
				if (v6->off > v9)
					return 1;
				v10 = v6->size;
				if (v10 < *v7)
					break;
				if (v6->off > v9 || v10 >= *v7 && v10 > * v7)
					return 1;
				++v5;
				v7 += 2;
				++v6;
				if (v5 >= v2)
					goto LABEL_14;
			}
			result = -1;
		}
		else
		{
		LABEL_14:
			result = 0;
		}
	}
	else
	{
		result = -1;
		if (v2 > v3)
			result = 1;
	}
	return result;
#else
	cs_assert(false);
#endif
}

bool __fastcall ivlset_t::add(ivl_t* ivl)
{
	unsigned __int64 v2; // r9
	unsigned __int64 v5; // rax
	unsigned __int64 v6; // rdx
	unsigned __int64 v7; // rax
	unsigned __int64 v9; // r11
	unsigned __int64 v10; // rcx
	unsigned __int64 v11; // r9
	ivl_t* v12; // rdx
	unsigned __int64 v13; // r8
	char v14; // r11
	unsigned __int64 v15; // r10
	int v16; // ecx
	unsigned __int64 v17; // rdi
	__int64 v18; // r8
	unsigned __int64 v19; // r11
	unsigned __int64 v20; // rax

	v2 = ivl->size;
	if (!v2)
		return 0;
	v5 = ivl->off;
	if (ivl->off >= allmem.off
		&& ivl->off <= allmem.off
		&& v2 >= allmem.size
		&& v2 <= allmem.size)
	{
		if (this->bag_.n != 1
			|| (v6 = this->bag_.array->off, v6 < allmem.off)
			|| v6 > allmem.off
			|| (v7 = this->bag_.array->size, v7 < allmem.size)
			|| v7 > allmem.size)
		{
			sub_170850F0(this, 1ui64);
			*this->bag_.array = allmem;
			return 1;
		}
		return 0;
	}
	v9 = v5 + v2;
	if (v5 + v2 < v5)
		return 0;
	v10 = this->bag_.n;
	v11 = 0i64;
	if (!v10)
		goto LABEL_17;
	v12 = this->bag_.array;
	while (1)
	{
		v13 = v12->off + v12->size;
		if (v13 >= v5)
			break;
		++v11;
		++v12;
		if (v11 >= v10)
			goto LABEL_17;
	}
	if (v12->off > v9)
	{
	LABEL_17:
		sub_170EE4B0(this, &this->bag_.array[v11], ivl);
		return 1;
	}
	v14 = 0;
	if (v12->off > v5)
	{
		v12->off = v5;
		v14 = 1;
	}
	v15 = ivl->off + ivl->size;
	if (v13 >= v15)
	{
		if (v14)
			goto LABEL_24;
		return 0;
	}
	v13 = v15;
LABEL_24:
	v16 = v11 + 1;
	v17 = v12->off;
	v12->size = v13 - v12->off;
	if ((signed int)v11 + 1 < this->bag_.n)
	{
		v18 = v16;
		do
		{
			v19 = v12->size;
			if (v19 + v17 < this->bag_.array[v18].off)
				break;
			v20 = this->bag_.array[v18].off + this->bag_.array[v18].size - v17;
			if (v20 > v19)
				v12->size = v20;
			++v16;
			++v18;
		} while (v16 < this->bag_.n);
	}
	if (v11 + 1 != v16)
		sub_170EE270(this, &this->bag_.array[v11 + 1], &this->bag_.array[v16]);
	return 1;
}

ivlset_t* __fastcall ivlset_t::copy(const ivlset_t* rhs)
{
	unsigned __int64 v4; // r8
	__int64 v5; // rdx
	unsigned __int64 v6; // r8
	unsigned __int64 v7; // rax
	unsigned __int64 v9; // rcx
	ivl_t* v10; // rdx

	if (this == rhs)
		return this;
	v4 = rhs->bag_.n;
	if (this->bag_.n < v4)
		v4 = this->bag_.n;
	if (v4)
	{
		v5 = 0i64;
		do
		{
			++v5;
			this->bag_.array[v5 - 1] = rhs->bag_.array[v5 - 1];
			--v4;
		} while (v4);
	}
	v6 = rhs->bag_.n;
	v7 = this->bag_.n;
	if (v7 <= v6)
	{
		if (v6 > this->bag_.alloc)
			this->bag_.array = (ivl_t*)qvector_reserve(this, this->bag_.array, v6, sizeof(ivl_t));
		for (; this->bag_.n < rhs->bag_.n; ++this->bag_.n)
		{
			v9 = this->bag_.n;
			v10 = &this->bag_.array[v9];
			if (v10)
				* v10 = rhs->bag_.array[v9];
		}
		return this;
	}
	do
		this->bag_.n = --v7;
	while (v7 > rhs->bag_.n);
	return this;
}