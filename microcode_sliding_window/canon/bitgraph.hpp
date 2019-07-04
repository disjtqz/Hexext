#pragma once

#include "bitsim/bitsim.hpp"

struct bg_node_ref_t;
enum class bg_origin_e : unsigned {
	CONSTANT,
	XOR,
	OR,
	NOT,
	AND,
	ABSTRACT
};
using bg_nodeidx_t = unsigned;
constexpr bg_nodeidx_t BADNODEIDX = 0xFFFFFFFF;

static constexpr unsigned nancestors_for_origin_op(bg_origin_e op) {
	if (op == bg_origin_e::CONSTANT || op == bg_origin_e::ABSTRACT)
		return 0;
	else if (op != bg_origin_e::NOT) {
		return 2;
	}
	return 1;
}

class bg_man_t {
	struct bg_node_internal_t {
		uint64_t m_value : 1;
		uint64_t m_is_known : 1;
		uint64_t m_op : 3;
		uint64_t m_ancestor1 : 29;
		uint64_t m_ancestor2 : 29;
		uint64_t m_extrabit : 1;

		inline void set_op(bg_origin_e origin_op) {
			m_op = static_cast<unsigned>(origin_op);
		}
	};

	std::vector< bg_node_internal_t> m_nodes;

	bg_node_internal_t* node_for_idx(bg_nodeidx_t idx) {
		cs_assert(idx < m_nodes.size());
		return &m_nodes[idx];
	}

	const bg_node_internal_t* node_for_idx(bg_nodeidx_t idx) const {
		cs_assert(idx < m_nodes.size());
		return &m_nodes[idx];
	}

	bg_node_internal_t* new_node() {
		m_nodes.resize(m_nodes.size() + 1);
		return &m_nodes[m_nodes.size() - 1];
	}


	bg_nodeidx_t node_to_nodeidx(const bg_node_internal_t * p) const {
		cs_assert(p != nullptr);
		return p - &m_nodes[0];
	}

	bg_nodeidx_t new_constant_node_impl(unsigned value) {

		cs_assert((value & 1) == value);
		auto res = new_node();
		res->m_value = value;
		res->m_is_known = 1;
		res->set_op(bg_origin_e::CONSTANT);
		return node_to_nodeidx(res);
	}

public:

	bg_man_t() : m_nodes() {
		new_constant_node_impl(0);//node 0 == constant 0
		new_constant_node_impl(1); // node 1 == constant 1

	}

	bg_origin_e get_origin_op(bg_nodeidx_t idx) const {
		unsigned orig = node_for_idx(idx)->m_op;
		return *reinterpret_cast<bg_origin_e*>(&orig);
	}

	unsigned nancestors(bg_nodeidx_t idx) const {
		return nancestors_for_origin_op(get_origin_op(idx));
	}
	unsigned get_value(bg_nodeidx_t idx) const {
		return node_for_idx(idx)->m_value;
	}

	bool is_known(bg_nodeidx_t idx) const {
		return node_for_idx(idx)->m_is_known == 1;
	}

	bg_nodeidx_t get_ancestor1(bg_nodeidx_t idx) const {
		cs_assert(nancestors(idx) != 0);
		return (bg_nodeidx_t)node_for_idx(idx)->m_ancestor1;
	}

	bg_nodeidx_t get_ancestor2(bg_nodeidx_t idx) const {
		cs_assert(nancestors(idx) == 2);
		return (bg_nodeidx_t)node_for_idx(idx)->m_ancestor2;
	}

	bg_nodeidx_t new_constant_node(unsigned value) {

		cs_assert((value & 1) == value);
		//constants are special, node 0 is constant 0, node 1 is constant 1
		//so we can just return value
		return static_cast<bg_nodeidx_t>(value);
		/*
		auto res = new_node();
		res->m_value = value;
		res->m_is_known = 1;
		res->set_op(bg_origin_e::CONSTANT);
		return node_to_nodeidx(res);*/
	}


	bg_nodeidx_t new_unary_node(bg_origin_e op, bg_nodeidx_t ancestor) {
		cs_assert(nancestors_for_origin_op(op) == 1);

		auto res = new_node();

		res->set_op(op);
		res->m_is_known = 0;
		res->m_ancestor1 = ancestor;
		return node_to_nodeidx(res);
	}

	bg_nodeidx_t new_binary_node(bg_origin_e op, bg_nodeidx_t ancestor1, bg_nodeidx_t ancestor2) {
		cs_assert(nancestors_for_origin_op(op) == 2);
		auto res = new_node();
		res->set_op(op);
		res->m_is_known = 0;
		res->m_ancestor1 = ancestor1;
		res->m_ancestor2 = ancestor2;
		return node_to_nodeidx(res);
	}
	bg_nodeidx_t new_abstract() {
		auto res = new_node();
		res->set_op(bg_origin_e::ABSTRACT);
		res->m_is_known = 0;
		return node_to_nodeidx(res);
	}

};

class bg_node_ref_t {
	bg_nodeidx_t m_idx;
	bg_man_t* m_manager;


	bool is_initialized() const {
		return m_idx != BADNODEIDX && m_manager != nullptr;
	}
	void assert_initialized() const {
		cs_assert(is_initialized());
	}
public:
	bg_node_ref_t() : m_idx(BADNODEIDX), m_manager(nullptr) {}
	bg_node_ref_t(bg_man_t* manager) : m_idx(BADNODEIDX), m_manager(manager) {}
	bg_node_ref_t(bg_nodeidx_t idx, bg_man_t* manager) : m_idx(idx), m_manager(manager) {}

	bool is_constant() const {
		assert_initialized();
		return m_manager->is_known(m_idx);
	}

	unsigned value() const {
		assert_initialized();
		return m_manager->get_value(m_idx);
	}

	bg_origin_e origin() const {
		assert_initialized();
		return m_manager->get_origin_op(m_idx);
	}

	

	bg_node_ref_t ancestor1() const {
		assert_initialized();

		return bg_node_ref_t{ m_manager->get_ancestor1(m_idx),m_manager };
	}
	bg_node_ref_t ancestor2() const {
		assert_initialized();

		return bg_node_ref_t{ m_manager->get_ancestor2(m_idx),m_manager };
	}

	bg_node_ref_t unary(bg_origin_e un_op) {
		assert_initialized();
		return bg_node_ref_t{ m_manager->new_unary_node(un_op, m_idx), m_manager };
	}

	bg_node_ref_t binary(bg_origin_e bin_op, bg_node_ref_t other) {
		assert_initialized();
		other.assert_initialized();
		return bg_node_ref_t{ m_manager->new_binary_node(bin_op, m_idx, other.m_idx), m_manager };
	}

	bg_node_ref_t create_constant(unsigned value) {
		assert_initialized();
		return bg_node_ref_t{ m_manager->new_constant_node(value), m_manager };
	}

	bg_node_ref_t operator ~() {
		assert_initialized();
		if (!is_constant()) {
			return unary(bg_origin_e::NOT);
		}
		else {
			return create_constant(value() ^ 1);
		}
	}

	bool both_constant(bg_node_ref_t other) {
		assert_initialized();
		other.assert_initialized();
		return is_constant() && other.is_constant();
	}

	bool either_constant(bg_node_ref_t other) {
		assert_initialized();
		other.assert_initialized();
		return is_constant() || other.is_constant();
	}

	bool either_constant_val(bg_node_ref_t other, unsigned v) {
		assert_initialized();
		other.assert_initialized();
		return (is_constant() && value() == v) || (other.is_constant() && other.value() == v);
	}

	bool either_constant1(bg_node_ref_t other) {
		return either_constant_val(other, 1);
	}
	bool either_constant0(bg_node_ref_t other) {
		return either_constant_val(other, 0);
	}

	bg_node_ref_t select_constant(bg_node_ref_t other) {
		assert_initialized();
		other.assert_initialized();

		if (is_constant()) {
			return *this;
		}
		else if (other.is_constant()) {
			return other;
		}
		else {
			return bg_node_ref_t();
		}
	}

	bg_node_ref_t select_non_constant(bg_node_ref_t other) {
		assert_initialized();
		other.assert_initialized();
		if (is_constant() && other.is_constant()) {
			return bg_node_ref_t{};
		}
		else {
			if (is_constant()) {
				return other;
			}
			else {
				return *this;
			}
		}
	}

	bg_node_ref_t operator |(bg_node_ref_t other) {

		if (both_constant(other)) {
			//(1/0) | (1/0)
			return create_constant(value() | other.value());
		}
		else {
			if (!either_constant(other)) {
				//value1 | value2
				return binary(bg_origin_e::OR, other);
			}

			bg_node_ref_t cval = select_constant(other);

			bg_node_ref_t rval = select_non_constant(other);

			cs_assert(cval.is_initialized() && rval.is_initialized());

			if (cval.value() == 1) {
				//1 | value == 1
				return create_constant(1);
			}
			else {
				//0 | value == value
				return rval;
			}
		}

	}

	bg_node_ref_t operator &(bg_node_ref_t other) {
		if (both_constant(other)) {
			return create_constant(value() & other.value());
		}
		else {
			if (!either_constant(other)) {
				return binary(bg_origin_e::AND, other);
			}
			bg_node_ref_t cval = select_constant(other);

			bg_node_ref_t rval = select_non_constant(other);

			cs_assert(cval.is_initialized() && rval.is_initialized());

			if (cval.value() == 1) {
				return rval;
			}
			else {
				return create_constant(0);
			}
		}
	}

	bg_node_ref_t operator ^(bg_node_ref_t other) {

		if (both_constant(other)) {
			return create_constant(other.value() ^ value());
		}
		else {
			if (!either_constant(other)) {
				return binary(bg_origin_e::XOR, other);
			}

			bg_node_ref_t cval = select_constant(other);
			bg_node_ref_t rval = select_non_constant(other);

			cs_assert(cval.is_initialized() && rval.is_initialized());

			if (cval.value() == 1) {
				return rval.unary(bg_origin_e::NOT);
			}
			else {
				return rval;
			}
		}
	}


};



template<size_t tmpl_nbits>
class bg_aggr_t {
	bg_man_t* m_manager;
	std::array<bg_node_ref_t, tmpl_nbits> m_bits;

	bool is_initialized() const {
		return m_manager != nullptr;
	}

	bool assert_initialized() const {
		cs_assert(is_initialized());
	}

public:
	using my_type = bg_aggr_t<tmpl_nbits>;
	bg_aggr_t() :m_manager(nullptr), m_bits() {}
	bg_aggr_t(bg_man_t* manager) : m_manager(manager), m_bits() {}

	bg_aggr_t(my_type& other) : m_manager(other.m_manager), m_bits(other.m_bits) {}

	template<size_t n>
	void give_context(bg_aggr_t<n>& other) {
		other.assert_initialized();
		m_manager = other.m_manager;
	}
	void make_abstract_at(unsigned i) {
		assert_initialized();
		cs_assert(i < AGGR_NBITS);

		m_bits[i] = bg_node_ref_t{ m_manager->new_abstract(), m_manager };
	}

	void set_from_other(my_type& other) {
		other.assert_initialized();

		for (unsigned i = 0; i < AGGR_NBITS; ++i) {
			this->operator[](i) = other[i];
		}
	}

	void make_constant_at(unsigned i, unsigned bit) {
		assert_initialized();
		cs_assert(i < AGGR_NBITS);

		m_bits[i] = bg_node_ref_t{ m_manager->new_constant_node(bit), m_manager };
	}

	void make_fully_abstract() {
		for (unsigned i = 0; i < AGGR_NBITS; ++i) {
			make_abstract_at(i);
		}
	}

	void set_constant(uint64_t constval) {
		static_assert(AGGR_NBITS <= 64, "Haven't implemented aggregates > 64 bits yet");
		for (unsigned i = 0; i < AGGR_NBITS; ++i) {
			make_constant_at(i, (constval & (1 << i)) != 0 ? 1 : 0);
		}
	}

	void set_all_to_constant(unsigned v) {
		for (unsigned i = 0; i < AGGR_NBITS; ++i) {
			make_constant_at(i, v);
		}
	}

	my_type bit_to_mask(unsigned bitidx) {
		cs_assert(bitidx < AGGR_NBITS);
		my_type result{};
		result.give_context(*this);

		for (unsigned i = 0; i < AGGR_NBITS; ++i) {
			result[i] = this->operator[](bitidx);
		}
		return result;
	}

	uint64_t to_u64_consts() {

		uint64_t res = 0ULL;

		for (unsigned i = 0; i < AGGR_NBITS; ++i) {
			if (this->operator[](i).is_constant()) {
				res |= (uint64_t)this->operator[](i).value() << i;
			}
		}
		return res;
	}

	static constexpr unsigned AGGR_NBITS = tmpl_nbits;


	template<size_t tmpl_otherbits>
	using largest_aggr_t = bg_aggr_t< (tmpl_nbits > tmpl_otherbits ? tmpl_nbits : tmpl_otherbits)>;

	bg_node_ref_t& operator [](unsigned idx) {
		return m_bits[idx];
	}

	const bg_node_ref_t& operator [](unsigned idx) const {
		return m_bits[idx];
	}




private:
	template<size_t tmpl_otherbits, typename TCallable>
	largest_aggr_t<tmpl_otherbits> _binary_operator(bg_aggr_t<tmpl_otherbits>& other, TCallable&& operate) {
		assert_initialized();
		other.assert_initialized();
		largest_aggr_t<tmpl_otherbits> result{ m_manager };

		if constexpr (tmpl_otherbits == tmpl_nbits) {
			for (unsigned i = 0; i < tmpl_otherbits; ++i) {
				result[i] = operate(this->operator[](i),other[i]);
			}
		}
		else  {
			constexpr unsigned mincommon = (tmpl_otherbits < tmpl_nbits ? tmpl_otherbits : tmpl_nbits);

			for (unsigned i = 0; i < mincommon; ++i) {
				result[i] = operate(this->operator[](i),other[i]);
			}
			if constexpr (AGGR_NBITS < other.AGGR_NBITS) {
				for (unsigned i = mincommon; i < other.AGGR_NBITS; ++i) {
					result[i] = other[i];
				}
			}
			else {
				for (unsigned i = mincommon; i < AGGR_NBITS; ++i) {
					result[i] = this->operator[](i);
				}
			}

		}

		return result;
	}
public:
	template<size_t tmpl_otherbits>
	largest_aggr_t<tmpl_otherbits> operator ^(bg_aggr_t<tmpl_otherbits>& other) {
		return _binary_operator(other, [](bg_node_ref_t & lhs, bg_node_ref_t & rhs) {
			return lhs ^ rhs;
		});
	}
	template<size_t tmpl_otherbits>
	largest_aggr_t<tmpl_otherbits> operator |(bg_aggr_t<tmpl_otherbits>& other) {
		return _binary_operator(other, [](bg_node_ref_t & lhs, bg_node_ref_t & rhs) {
			return lhs | rhs;
			});
	}
	template<size_t tmpl_otherbits>
	largest_aggr_t<tmpl_otherbits> operator &(bg_aggr_t<tmpl_otherbits>& other) {
		return _binary_operator(other, [](bg_node_ref_t & lhs, bg_node_ref_t & rhs) {
			return lhs & rhs;
		});
	}

	my_type operator ~() {
		assert_initialized();
		my_type result{ m_manager };

		for (unsigned i = 0; i < AGGR_NBITS; ++i) {
			result[i] = ~this->operator[](i);
		}
	}

	my_type operator >>(unsigned count) {
		assert_initialized();
		my_type result{m_manager};

		if (count == 0) {
			result.set_from_other(*this);
		}
		else if (count >= AGGR_NBITS) {
			result.set_all_to_constant(0);
			
		}
		else {
			for (unsigned i = 0; i < count; ++i) {
				result.make_constant_at(AGGR_NBITS - 1 - i, 0);
			}
			for (unsigned i = count; i < AGGR_NBITS; ++i) {
				result[i - count] = this->operator[](i);
			}


		}

		return result;
	}
	my_type operator <<(unsigned count) {
		assert_initialized();
		my_type result{ m_manager };

		if (count == 0) {
			result.set_from_other(*this);
		}
		else if (count >= AGGR_NBITS) {
			result.set_all_to_constant(0);

		}
		else {
			for (unsigned i = 0; i < count; ++i) {
				result.make_constant_at(i, 0);
			}
			for (unsigned i = count; i < AGGR_NBITS; ++i) {
				result[i] = this->operator[](i - count);
			}


		}

		return result;
	}
};

struct bitgraph_sim_traits {
	using bit_type = bg_node_ref_t;
	template<size_t n>
	using bit_aggregate_type = bg_aggr_t<n>;
};
using bitgraph_simulation_t = cs::bitsim::bitsim_tmpl_t< bitgraph_sim_traits>;