#pragma once
#include "cs_core.hpp"
#define			X86_OP		 static __forceinline 

template<typename T>
X86_OP uint8_t sbc(T src1, T src2, std::make_unsigned_t<T>& result_out, uint8_t b_in = 0) noexcept {

	if constexpr (sizeof(T) == 1) {
		return _subborrow_u8(b_in, src1, src2, &result_out);
	}
	else if constexpr (sizeof(T) == 2) {
		return _subborrow_u16(b_in, src1, src2, &result_out);
	}
	else if constexpr (sizeof(T) == 4) {
		return _subborrow_u32(b_in, src1, src2, &result_out);
	}
	else {
		return _subborrow_u64(b_in, src1, src2, &result_out);
	}
}

template<typename T>
X86_OP uint8_t adc(T src1, T src2, std::make_unsigned_t<T>& result_out, uint8_t c_in = 0) noexcept {


	if constexpr (sizeof(T) == 1) {
		return _addcarry_u8(c_in, src1, src2, &result_out);
	}
	else if constexpr (sizeof(T) == 2) {
		return _addcarry_u16(c_in, src1, src2, &result_out);
	}
	else if constexpr (sizeof(T) == 4) {
		return _addcarry_u32(c_in, src1, src2, &result_out);
	}
	else {
		return _addcarry_u64(c_in, src1, src2, &result_out);
	}
}

//double precision left shift
X86_OP uint64_t shld(const uint64_t low, const uint64_t high, const uint8_t shift) noexcept {
	return __shiftleft128(low, high, shift);
}

//double precision right shift
X86_OP uint64_t shrd(const uint64_t low, const uint64_t high, const uint8_t shift) noexcept {
	return __shiftright128(low, high, shift);
}

template<typename T>
X86_OP T pdep(T value, T mask) noexcept {
	static_assert(std::is_integral_v<T>);
	if constexpr (sizeof(T) == 8) {
		return _pdep_u64(value, mask);
	}
	else {
		return _pdep_u32(value, mask);
	}
}
template<typename T>
X86_OP T pext(T value, T mask) noexcept {
	static_assert(std::is_integral_v<T>);
	if constexpr (sizeof(T) == 8) {
		return _pext_u64(value, mask);
	}
	else {
		return _pext_u32(value, mask);
	}
}

template<typename T>
X86_OP T bswap(T value) noexcept {

	if constexpr (sizeof(T) == 2) {
		return _byteswap_ushort(value);
	}
	else if constexpr (sizeof(T) == 4) {
		return _byteswap_ulong(value);
	}
	else {
		return _byteswap_uint64(value);
	}
}
template<typename T>
X86_OP T bzhi(T value, uint32_t index) noexcept {
	static_assert(std::is_integral_v<T>);
	if constexpr (sizeof(T) == 8) {
		return _bzhi_u64(value, index);
	}
	else {
		return _bzhi_u32(value, index);
	}
}

template<typename T>
X86_OP T mulx(T a, T b) noexcept {
	static_assert(std::is_integral_v<T>);

	if constexpr (sizeof(T) == 8) {
		uint64_t unused;
		return _mulx_u64(a, b, &unused);
	}
	else {
		uint32_t unused;
		return _mulx_u32(a, b, &unused);
	}

}

template<typename T>
X86_OP T mulx(T a, T b, std::make_unsigned_t<T>& high) noexcept {
	static_assert(std::is_integral_v<T>);

	if constexpr (sizeof(T) == 8) {
		return _mulx_u64(a, b, &high);
	}
	else {
		return _mulx_u32(a, b, &high);
	}

}



template<typename T>
X86_OP T rol(std::make_unsigned_t<T> value, uint8_t sh) noexcept {

	if constexpr (sizeof(T) == 1) {
		return _rotl8(value, sh);
	}
	else if constexpr (sizeof(T) == 2) {
		return _rotl16(value, sh);
	}
	else {
		return (value << sh) | (value >> (sizeof(value) * 8 - sh));
	}
}

template<typename T>
X86_OP T ror(std::make_unsigned_t<T> value, uint8_t sh) noexcept {
	if constexpr (sizeof(T) == 1) {
		return _rotr8(value, sh);
	}
	else if constexpr (sizeof(T) == 2) {
		return _rotr16(value, sh);
	}
	else {
		return (value << sh) | (value >> (sizeof(value) * 8 - sh));
	}
}

template<typename T>
X86_OP T rorx(std::make_unsigned_t<T> value, uint32_t sh) noexcept {

	if constexpr (sizeof(T) == 8) {
		return _rorx_u64(value, sh);
	}
	else {
		return _rorx_u32(value, sh);
	}
}


template<typename T>
X86_OP T shlx(T value, uint32_t shift) noexcept {
	static_assert(std::is_integral_v<T>);
	if constexpr (sizeof(T) == 8)
		return _shlx_u64(value, shift);
	else
		return _shlx_u32(value, shift);
}
template<typename T>
X86_OP T shrx(T value, uint32_t shift) noexcept {
	static_assert(std::is_integral_v<T>);
	if constexpr (sizeof(T) == 8)
		return _shrx_u64(value, shift);
	else
		return _shrx_u32(value, shift);
}
template<typename T>
X86_OP T sarx(T value, uint32_t shift) noexcept {
	static_assert(std::is_integral_v<T>);
	if constexpr (sizeof(T) == 8)
		return _sarx_i64(value, shift);
	else
		return _sarx_i32(value, shift);
}

template<typename T>
X86_OP T bextr(T value, uint32_t start, uint32_t length) noexcept {
	if constexpr (sizeof(T) == 8) {
		return _bextr_u64(value, start, length);
	}
	else
		return _bextri_u32(value, start, length);
}




template<typename TData>
X86_OP auto popcount(const TData data) noexcept {

	if constexpr (sizeof(TData) == 4) {
		return __popcnt(data);
	}
	else if constexpr (sizeof(TData) == 8) {
		return __popcnt64(data);
	}
	else {
		return __popcnt16(data);
	}
}
template<typename TData>
X86_OP size_t tzcnt(const TData data) noexcept {

	if constexpr (sizeof(TData) == 1) {
		return  _tzcnt_u32(data) - 24;
	}
	else if constexpr (sizeof(TData) == 2) {
		return _tzcnt_u32(data) - 16;
	}
	else if constexpr (sizeof(TData) == 4) {
		return _tzcnt_u32(data);
	}
	else {
		return _tzcnt_u64(data);
	}
}

template<typename TData>
X86_OP size_t lzcnt(const TData data)noexcept {
	if constexpr (sizeof(TData) < 4) {
		return  __lzcnt16(data);
	}
	else if constexpr (sizeof(TData) == 4) {
		return __lzcnt(data);
	}
	else {
		return __lzcnt64(data);
	}
}