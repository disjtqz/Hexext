#pragma once
#include <intrin.h>
#include <vector>
#include <array>
#include <type_traits>
#include <algorithm>
#undef __LINE__
#define CS_RESTRICT	__restrict

#ifdef _MSC_VER

#define	CS_NORETURN		__declspec(noreturn)
#define CS_NOINLINE		__declspec(noinline)

#define		CS_DATA_SEG(name)		__declspec(allocate(name))
#define		CS_CODE_SEG(name)		__declspec(code_seg(name)) 


#define		CS_COLD_CODE			CS_CODE_SEG(".cold")
#define		CS_FORCEINLINE			__forceinline
#define		cs_assume_m(...)		__assume(__VA_ARGS__)
#else

//gnu versions here

#endif
/*
CS_NORETURN CS_NOINLINE
void ass_fail(const char* msg_);
*/
/*
CF 125
ZF 126
NF 127
VF 128

*/

namespace cs {
	struct cs_assert_fail_descr_t {
		const char* _err_msg;
		const char* _file_name;
		 const char* fn_name;
	};
	CS_COLD_CODE
		CS_NOINLINE
		CS_NORETURN
		void _cs_assert_fail_raise(const cs_assert_fail_descr_t* descr);

}
namespace cs::_cs_assert_impl {

	template<typename T/*, unsigned line_no*/>
	struct _cs_assert_fail {
		static constexpr cs_assert_fail_descr_t _descr{ T::__msg(), T::__file() , T::fn()};

		CS_NOINLINE
			CS_NORETURN
			CS_CODE_SEG(".assert_failures")
			static void raise_failure() {
			_cs_assert_fail_raise(&_descr);
		}

	};
}
#if 0
#define cs_assert(...)		\
	do {\
		static  const char* __assert_msg = #__VA_ARGS__;\
		static  const char* __assert_failure_file = __FILE__;\
		using __failure_handler = cs::_cs_assert_impl::_cs_assert_fail<&__assert_msg,&__assert_failure_file, __LINE__>;\
		if(!(__VA_ARGS__)) \
			__failure_handler::raise_failure();\
	}while(false)
#else
#define cs_assert(...)		\
	do {\
		static constexpr const char* __funcy = __FUNCTION__;\
		class _arg_passer {\
		public:\
			static constexpr const char* __msg() {\
				return #__VA_ARGS__; \
			}\
			static constexpr const char* __file() {\
				return __FILE__;\
			}\
static constexpr const char* fn(){return __funcy;}\
		};\
\
		using __failure_handler = cs::_cs_assert_impl::_cs_assert_fail<_arg_passer>;\
		if(!(__VA_ARGS__)) \
			__failure_handler::raise_failure();\
	}while(false)
#endif
#define or	or_
#define and and_

#define xor xor_


template<typename T, typename High16Type, unsigned alignment_bits, typename AlignmentType>
class tagged_ptr_t {
	static_assert(sizeof(High16Type) == 2);

	static constexpr uintptr_t alignment_mask = (((uintptr_t)1) << alignment_bits) - 1;

	static constexpr uintptr_t high16_mask = (0xFFFFULL << 48);
	static constexpr uintptr_t normalize_mask = ~(alignment_mask | high16_mask);
	uintptr_t m_data;

	static inline uintptr_t normalize(uintptr_t other) {
		return other & normalize_mask;
	}

	static inline uintptr_t encode(T* p, High16Type high16, AlignmentType alignmentv) {
		uintptr_t res = normalize(p);
		res |= ((unsigned long long) * reinterpret_cast<unsigned short*>(&high16)) << 48;
		res |= alignmentv;
		return res;
	}

public:

	tagged_ptr_t(T* data) : m_data(encode(data, High16Type(), AlignmentType())) {}


	tagged_ptr_t(T* data, High16Type high16, AlignmentType algn) : m_data(encode(data, high16, algn)) {}



	T* ptr() {
		return reinterpret_cast<T*>((((intptr_t)normalize(m_data)) << 16) >> 16);
	}

};


template<typename BitHolderType, typename SizeType, typename PopulationType>
struct sparse_indexer_t {
	BitHolderType* m_bits;
	SizeType m_size;
	PopulationType m_population;

	void allocate(unsigned size) {
		auto oldbits = m_bits;

		auto newbits = new BitHolderType[size];

		if (oldbits) {
			memcpy(newbits, oldbits, m_size * sizeof(BitHolderType));
			memset(&newbits[m_size], 0, (size - m_size) * sizeof(BitHolderType));
			delete[] oldbits;
		}
		else {
			memset(newbits, 0, size * sizeof(BitHolderType));
		}

		m_bits = newbits;
		m_size = size;


	}
	static inline auto pop_count(BitHolderType value) {
		if constexpr (sizeof(BitHolderType) == 2) {
			return __popcnt16(value);
		}

		else if constexpr (sizeof(BitHolderType) == 4) {
			return __popcnt(value);
		}
		else if constexpr (sizeof(BitHolderType) == 8) {
			return __popcnt64(value);
		}
		else {
			cs_assert(false);
		}


	}
public:
	sparse_indexer_t() : m_bits(nullptr), m_size(0), m_population(0) {}


	sparse_indexer_t(unsigned n) : sparse_indexer_t() {
		allocate(n);
	}

	bool find_index(unsigned idx, unsigned* out_idx) const {
		unsigned currpop = 0;
		bool found = false;
		unsigned i = 0;
		BitHolderType currbit;
		for (; i != idx / 32; ++i) {
			currbit = m_bits[i];
			currpop += pop_count(currbit);
			/*if (currpop > idx) {
				found = true;
				break;
			}*/
		}

		/*if (!found) {
			return false;
		}*/

		currpop -= pop_count(currbit);
		unsigned bitidx = 0;

		unsigned baseidx = (i - 1) / (sizeof(BitHolderType) * 8);


		*out_idx = baseidx + pop_count(((1 << idx) - 1) & currbit);



		//*out_idx = baseidx + _lzcnt_u64(currbit);
		return true;
	}

	bool set_at(unsigned index) {
		if (!_bittestandset((long*)& m_bits[index / 32], index & 0x1F)) {
			m_population++;
			return true;
		}
		return false;
	}

	bool reset_at(unsigned index) {

		if (_bittestandreset((long*)& m_bits[index / 32], index & 0x1F)) {
			m_population--;
			return true;
		}
		return false;
	}
};


template<typename T>
struct sparse_vector_t {
	sparse_indexer_t<unsigned int, unsigned, unsigned> m_indexer;
	std::vector<T> m_data;

public:
	sparse_vector_t() : m_indexer(), m_data() {}

	sparse_vector_t(unsigned maxalloc) : m_indexer(maxalloc), m_data() {}


	bool insert_at(unsigned index, T&& value) {
		unsigned idx;

		bool need_reallocate = m_indexer.set_at(index);

		if (!m_indexer.find_index(index, &idx)) {
			return false;
		}
		if (need_reallocate) {
			m_data.push_back(T());

			memmove(&m_data[idx + 1], &m_data[idx], m_data.size() - idx);
		}

		m_data[idx] = value;

		return true;
	}

	bool remove_at(unsigned index) {
		unsigned idx;

		if (!m_indexer.find_index(index, &idx)) {
			return false;
		}

		m_indexer.reset_at(index);

		m_data.erase(&m_data[idx], &m_data[idx + 1]);

		return true;
	}
};

namespace cs {

	static inline unsigned bool_to_flag(bool b, unsigned idx) noexcept {
		return b ? (1 << idx) : 0;
	}

	template<typename T1, typename T2>
	constexpr bool gnu_typeof_eq_v = std::is_same_v< std::remove_cv_t<std::remove_reference_t<T1>>, std::remove_cv_t<std::remove_reference_t<T2>>>;


	namespace __CS_INTERNAL__ {
		template<typename TTest, typename TOther, typename... TRest>
		constexpr bool gnu_is_any_eq_v_impl() {
			if constexpr (gnu_typeof_eq_v<TTest, TOther>) {
				return true;
			}
			else {
				if constexpr (sizeof...(TRest)) {
					return gnu_is_any_eq_v_impl<TTest, TRest...>();
				}
				else
					return false;
			}
		}
	}

	template<typename TTest, typename... T2>
	constexpr bool gnu_is_any_eq_v = __CS_INTERNAL__::gnu_is_any_eq_v_impl<TTest, T2...>();

	constexpr int hash_string(const char* x) {
		long long int sumx = 0x811c9dc5;
		int carry = 0;
		unsigned ix = 0;
		for (; x[ix]; ++ix) {
			long long int oldsum = sumx;
			sumx += x[ix];
			if (oldsum > sumx)
				++carry;
			oldsum = sumx;
			sumx += (sumx << 1) + (sumx << 4) + (sumx << 7) + (sumx << 8) + (sumx << 24);
			if (oldsum > sumx)
				carry += 1;

		}
		return (sumx ^ (sumx >> 32)) + carry;
	}

	constexpr unsigned conststrlen(const char* s) {
		unsigned i = 0;
		for (; s[i]; ++i);
		return i;
	}
}

namespace cs::const_ops {

	template<typename  tmpl_eletype, size_t tmpl_nelements>
	constexpr void quicksort_const(std::array<tmpl_eletype, tmpl_nelements>& arr) {
		for (unsigned i = 0; i < tmpl_nelements - 1; ++i) {
			tmpl_eletype m = arr[i];
			unsigned k = i;
			for (unsigned j = i + 1; j < tmpl_nelements; ++j) {
				tmpl_eletype xj = arr[j];
				if (xj < m) {
					m = xj;
					k = j;
				}

			}
			tmpl_eletype a = arr[i];
			arr[i] = m;
			arr[k] = a;
		}
	}
	

	constexpr unsigned hash_string(const char* x) {
		unsigned long long int sumx = 0x811c9dc5;
		int carry = 0;
		unsigned ix = 0;
		for (; x[ix]; ++ix) {
			unsigned long long int oldsum = sumx;
			sumx += x[ix];
			if (oldsum > sumx)
				++carry;
			oldsum = sumx;
			sumx += (sumx << 1) + (sumx << 4) + (sumx << 7) + (sumx << 8) + (sumx << 24);
			if (oldsum > sumx)
				carry += 1;

		}
		return (unsigned)((sumx ^ (sumx >> 32)) + carry);
	}

	constexpr unsigned string_length(const char* s) {
		unsigned i = 0;

		for (; s[i]; ++i)
			;
		return i;
	}

	constexpr bool str_contains_char(const char* s, char c) {
		for (unsigned i = 0; s[i]; ++i) {
			if (s[i] == c)
				return true;
		}
		return false;
	}



	constexpr bool str_conforms(const char* str, const char* pat) {

		for (unsigned i = 0; str[i]; ++i) {
			if (!str_contains_char(pat, str[i])) {
				return false;
			}
		}
		return true;
	}

	constexpr const char* PATTERN_WHITESPACE = "\t\n \v\f\r";


	constexpr int cseek_next_in_pattern(const char* src, unsigned idx_start, const char* pat) {

		for (unsigned i = idx_start; src[i]; ++i) {
			if (str_contains_char(pat, src[i])) {
				return (int)i;
			}
		}
		return -1;
	}

	constexpr int cseek_next_not_in_pattern(const char* src, unsigned idx_start, const char* pat) {

		for (unsigned i = idx_start; src[i]; ++i) {
			if (!str_contains_char(pat, src[i])) {
				return (int)i;
			}
		}
		return -1;
	}

	constexpr int cseek_next_in_pattern_backwards(const char* src, unsigned idx_start, const char* pat) {

		for (int i = idx_start; i >= 0; --i) {
			if (str_contains_char(pat, src[i])) {
				return i;
			}
		}
		return -1;
	}


	constexpr unsigned ccount_instances_until(const char* src, unsigned idx_start, unsigned length, char count) {

		unsigned n = 0;

		for (unsigned i = idx_start; i < idx_start + length; ++i) {
			if (src[i] == count)
				n++;
		}
		return n;
	}


	constexpr void cstrcpy(char* destination, const char* src, unsigned src_start_offs, unsigned src_slice_length) {
		unsigned i = src_start_offs;
		for (; i < src_start_offs + src_slice_length; ++i) {
			destination[i - src_start_offs] = src[i];
		}
		destination[i] = 0;
	}

	constexpr bool cstreq(const char* s1, const char* s2) {

		unsigned i = 0;
		for (; s1[i] == s2[i] && s1[i] != 0; ++i) {

		}

		return s1[i] == 0 && s2[i] == 0;
	}

	constexpr char _constexpr_assert(bool expr, const char* str, unsigned sz) {
		if (!expr) {
			throw str;

		}
		else {
			return 0;
		}
	}

#define cs_constexpr_assert(...)	cs::const_ops::_constexpr_assert((__VA_ARGS__), #__VA_ARGS__, sizeof(#__VA_ARGS__));
}

/*
No constructor available. This type is only available as a target to cast to from  fixed size vector T
*/
template<typename T>
class fixed_size_vecparm_t {
	unsigned m_curr_size;
	unsigned m_max_size;
	T m_data_start;
	/*
		casts arent needed anymore but whatevs
	*/
	T* get_data() {
		return reinterpret_cast<T*>(&m_data_start);
	}
	
	const T* get_data() const {
		return reinterpret_cast<const T*>(&m_data_start);
	}
public:

	 bool push_back( T v) {
		if (m_curr_size + 1 < m_max_size) {
			get_data()[m_curr_size++] = v;
		}
		else {
			return false;
		}
		return true;
	}

	 T& operator [](unsigned index) {
		return get_data()[index];
	}

	 unsigned size() const {
		return m_curr_size;
	}
	 unsigned max_size() const {
		 return m_max_size;
	 }

	 T* begin() {
		return &get_data()[0];
	}

	 T* end() {
		return &get_data()[m_curr_size];
	}
	 void reset() {
		 m_curr_size = 0;
	 }

	 template<typename T2>
	 fixed_size_vecparm_t<T2>* cast_down() {
		 static_assert(sizeof(T2) < sizeof(T));
		 return reinterpret_cast<fixed_size_vecparm_t<T2>*>(this);
	 }

	 static fixed_size_vecparm_t<T>* emplace_at(void* wher, unsigned nelements) {

		 auto thiz = reinterpret_cast<fixed_size_vecparm_t<T>*>(wher);
		 thiz->m_curr_size = 0;
		 thiz->m_max_size = nelements;
		 return thiz;
	 }

	 static unsigned required_allocation_size(unsigned nelements) {

		 return (sizeof(fixed_size_vecparm_t<T>) - sizeof(T)) + (sizeof(T) * nelements);
	 }


};

template<typename T, unsigned max_size>
class fixed_size_vector_t {
	unsigned m_curr_size;
	const unsigned m_max_size;
	std::array<T, max_size> m_storage;

public:
	/*
	This is not intended to handle datatypes that have destructors
	*/
	
	//static_assert(!std::is_destructible_v<T>);
	constexpr fixed_size_vector_t() : m_curr_size(0u), m_max_size(max_size), m_storage() {}

	constexpr bool push_back(const T& v) {
		if (m_curr_size + 1 < max_size) {
			m_storage[m_curr_size++] = v;
		}
		else {
			return false;
		}
		return true;
	}

	constexpr T& operator [](unsigned index) {
		return m_storage[index];
	}
	constexpr const T& operator [](unsigned index) const {
		return m_storage[index];
	}
	constexpr unsigned size() const {
		return m_curr_size;
	}

	constexpr T* begin() {
		return &m_storage[0];
	}

	constexpr T& back() {
		return m_storage[size() - 1];
	}

	constexpr T* end() {
		return &m_storage[m_curr_size];
	}
	/*
	Recast the fixed size array to a type that is not aware intrinsically of what its maximum size is.
	*/
	fixed_size_vecparm_t<T>* pass() {
		return reinterpret_cast<fixed_size_vecparm_t<T>*>(this);
	}

	constexpr void reset() {
		m_curr_size = 0u;
	}

	
};

template<typename T>
using fixed_size_vecptr_t = fixed_size_vecparm_t<T>*;

constexpr uint64_t highbit_for_size(unsigned size) {

	return (1ULL << ((size * 8) - 1));
}

namespace cs::const_ops {
	template<typename  tmpl_eletype, unsigned tmpl_nelements>
	constexpr void quicksort_const(fixed_size_vector_t<tmpl_eletype, tmpl_nelements>& arr) {
		for (unsigned i = 0; i < arr.size(); ++i) {
			tmpl_eletype m = arr[i];
			unsigned k = i;
			for (unsigned j = i + 1; j < arr.size(); ++j) {
				tmpl_eletype xj = arr[j];
				if (xj < m) {
					m = xj;
					k = j;
				}

			}
			tmpl_eletype a = arr[i];
			arr[i] = m;
			arr[k] = a;
		}
	}
	/*
		needed for exroles
	*/
	constexpr int ctstrcmp(const char* s1, const char* s2) {
		//const unsigned char* s1 = (const unsigned char*)p1;
		//const unsigned char* s2 = (const unsigned char*)p2;
		unsigned char c1=0, c2=0;
		do
		{
			c1 = (unsigned char)* s1++;
			c2 = (unsigned char)* s2++;
			if (c1 == '\0')
				return c1 - c2;
		} while (c1 == c2);
		return c1 - c2;
	}
}