#pragma once


template<typename T>
struct alignas(T) micropool_entry_t {
	union {
		T m_data;
		micropool_entry_t<T>* m_next;
	};
};

struct micropool_t {
	unsigned m_max_bytes;
	unsigned m_curr_bytes;

	micropool_entry_t<minsn_t> m_insns;
	micropool_entry_t<mnumber_t> m_nums;
};