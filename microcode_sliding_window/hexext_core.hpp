#pragma once
#include <array>
#include <algorithm>

namespace hexext::core {
	template<typename T, T value, T... vals>
	static constexpr bool any_impl(T v);


	template<typename T,  T value, T... vals>
	static constexpr bool any_impl(T v) {
		if (v == value) {
			return true;
		}
		else {
			if constexpr (sizeof...(vals) > 0) {
				
				return any_impl<T, vals...>(v);
			}
			else
				return false;
		}
	}
	template<typename T, T... vals>
	static constexpr bool any(T v) {
		return any_impl<T, vals...>(v);
	}

	template<int... ignores>
	static constexpr unsigned long long  hash_string(const char* x) {
		unsigned long long  sumx = 0x811c9dc5;
		unsigned ix = 0;
		uint32_t carry = 0;
		for (; x[ix]; ++ix) {
			char c = x[ix];
			if(c != '_'){
			//if (!any<int, ignores...>((int)c)) {
				unsigned long long  oldsum = sumx;
				sumx += x[ix];
				if (oldsum > sumx)
					++carry;
				oldsum = sumx;
				sumx += (sumx << 1) + (sumx << 4) + (sumx << 7) + (sumx << 8) + (sumx << 24);
				if (oldsum > sumx)
					carry += 1;
			}
		}
		return (sumx ^ (sumx >> 32)) + carry;
	}


	template<typename T>
	static bool set_has(std::set<T>& set, T& key) {
		return set.find(key) != set.end();
	}


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

	template<typename indexer, typename key_type, int l, int r>
	static inline auto unroll_searcher(key_type& v) {
		constexpr int m = (l + r) / 2;
		if constexpr (l > r) {
			return indexer::default_value();
		}

		if (indexer::at(m) == v) {
			return indexer::value_at(m);
		}
		if (l == r) {
			return indexer::default_value();
		}

		if (v < indexer::at(m)) {
			return unroll_searcher<indexer, key_type, l, m - 1>(v);
		}
		else {
			return unroll_searcher<indexer, key_type, m + 1, r>(v);
		}

	}
}