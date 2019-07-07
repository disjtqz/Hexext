#pragma once

struct bitset64_t {
	uint64_t m_bits;
	bitset64_t() : m_bits(0ULL) {}
	bool has(unsigned bt) const {
		return (m_bits & (1ULL << bt)) != 0;
	}

	void add(unsigned bt) {
		m_bits |= (1ULL << bt);
	}


	void clear() {
		m_bits = 0ULL;
	}
};

struct bitset128_t {
	uint64_t m_bits[2];
	bitset128_t() : m_bits() {}
	bool has(unsigned bt) const {
		//on x86-64 shifts are mod size
		return m_bits[bt / 64] & (1ULL << bt);

		
	}
	void add(unsigned bt)  {
		m_bits[bt / 64] |= (1ULL << bt);
	}
	void clear() {
		m_bits[0] = 0ULL;
		m_bits[1] = 0ULL;
	}
};


