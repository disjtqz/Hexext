/*
	assumes that primitive operations >>, <<, |, ^, & and ~ have been defined for
	aggregates
*/
#define BITSIM_METHOD static inline

using bt_t = typename SimulationTraits::bit_type;

template<size_t n>
using bt_aggr_t =  typename SimulationTraits::template bit_aggregate_type<n>;

BITSIM_METHOD bt_t half_adder(bt_t x, bt_t y, bt_t& carry_out) {

	carry_out = x & y;
	return x ^ y;

}

BITSIM_METHOD bt_t add_bit(bt_t x, bt_t y, bt_t carry_in, bt_t& carry_out) {

	bt_t cfa{};

	bt_t s = half_adder(x, y, cfa);
	bt_t cfb{};
	bt_t fullsum = half_adder(carry_in, s, cfb);

	carry_out = cfb ^ cfa;
	return fullsum;
}
/*
	verified
*/
template<size_t tmpl_n>
BITSIM_METHOD bt_aggr_t<tmpl_n> add_aggrs(bt_aggr_t<tmpl_n>& lhs, bt_aggr_t<tmpl_n>& rhs, bt_t& cf_out) {
	bt_aggr_t<tmpl_n> result{};
	result.give_context(lhs);

	bt_t next_cf{};
	result[0] = half_adder(lhs[0], rhs[0], next_cf);
	for (unsigned i = 1; i < tmpl_n; ++i) {
		result[i] = add_bit(lhs[i], rhs[i], next_cf, next_cf);
	}

	cf_out = next_cf;
	return result;

}
/*
	verified
*/
template<size_t tmpl_n>
BITSIM_METHOD bt_aggr_t<tmpl_n> mul_aggrs(bt_aggr_t<tmpl_n>& lhs, bt_aggr_t <tmpl_n> & rhs) {
	bt_aggr_t<tmpl_n> result{};
	result.give_context(lhs);
	result.set_all_to_constant(0);
	bt_t discarded_cf{};
	
	for (unsigned i = 0; i < tmpl_n; ++i) {
		auto shifted = (lhs << i);

		auto shifted_mask = rhs.bit_to_mask(i) & shifted;

		result = add_aggrs(result, shifted_mask, discarded_cf);
	}

	return result;

}