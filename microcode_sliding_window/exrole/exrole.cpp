#include "../cs_core.hpp"
#include <hexrays.hpp>
#include "../micro_on_70.hpp"
#include "../hexutils/hexutils.hpp"
#include "exrole.hpp"
#include "exrole_internal.hpp"

/*
	This series of macros, header files and constant expressions will pre-generate all of the sets of names that belong to a role
*/

#define		ROLE(r, ...)		constexpr auto funcset_##r = make_funcset(exrole_t::r, __VA_ARGS__);
#define X(name)		#name	
#define ROLE_SEP	


#include "role_names/allrolenames.hpp"

#undef ROLE
#undef X
#undef ROLE_SEP

struct split_exrole_comp_t {
	const char* m_name;
	unsigned m_len;
	unsigned m_hashcode;
	constexpr bool operator <(const split_exrole_comp_t& other)const {

		if (m_hashcode == other.m_hashcode) {
			if (m_len < other.m_len) {
				return true;
			}
			else {
				return cs::const_ops::ctstrcmp(m_name, other.m_name) < 0;

			}
		}
		else {
			return m_hashcode < other.m_hashcode;
		}

	}

};
using exrole_lut_ele_t = uint8_t;


struct build_exrole_luts_t {
	template<typename T, typename... Ts>
	static constexpr auto build_flat_funcset_impl(fixed_size_vector_t<cs_funcset_submit_t, 1024>& st, const T& v, const Ts& ... rest) {

		v.add_to_roleset(st);

		if constexpr (sizeof...(rest)) {
			build_flat_funcset_impl(st, rest...);
		}



	}
	template<typename... Ts>
	static constexpr auto build_flat_funcset(const Ts& ... all) {
		fixed_size_vector_t<cs_funcset_submit_t, 1024> tempset{};

		build_flat_funcset_impl(tempset, all...);
		cs::const_ops::quicksort_const(tempset);

		return std::move(tempset);
	}

	


	static constexpr auto trim_initial_flatset() {

		/*
			We reuse our handwritten header files and ask macro sequences to automatically expand the entire list of possible role sets. 
			This means that all we need to do in order to define a new role is to add the role to the X macro file for the role enum 
			and then define a name file that lists all possible names for the role. 
			Then we just need to add it as an include to all role names. 
			And just like a new role has been added. 
		*/
#define	ROLE_SEP	,
#define	X(role)		
#define	ROLE(x, ...)	funcset_##x

		 constexpr auto initial_flatset = build_flat_funcset(
#include "role_names/allrolenames.hpp"
#undef X
#undef ROLE
#undef ROLE_SEP
		);
		constexpr unsigned sz = initial_flatset.size();

		std::array<cs_funcset_submit_t, sz> result{};

		for (unsigned i = 0; i < sz; ++i) {
			result[i] = initial_flatset[i];
		}
		return result;
	}

	//static constexpr auto trimmed_flatset = trim_initial_flatset();



	static constexpr auto build_exrole_lut() {
		constexpr auto trimmed_flatset = trim_initial_flatset();

		std::array<exrole_lut_ele_t, trimmed_flatset.size()> result{};

		for (unsigned i = 0; i < trimmed_flatset.size(); ++i) {
			result[i] = static_cast<exrole_lut_ele_t>(trimmed_flatset[i].m_role);
		}
		return result;
	}


	static constexpr auto splitoff_rest() {
		constexpr auto trimmed_flatset = trim_initial_flatset();
		std::array< split_exrole_comp_t, trimmed_flatset.size()> result{};

		for (unsigned i = 0; i < trimmed_flatset.size(); ++i) {
			result[i].m_hashcode = trimmed_flatset[i].m_hashcode;
			result[i].m_len = trimmed_flatset[i].m_len;
			result[i].m_name = trimmed_flatset[i].m_name;
		}

		return result;
	}
};

static constexpr auto g_lut_exrole = build_exrole_luts_t::build_exrole_lut();
static constexpr auto g_role_search_table = build_exrole_luts_t::splitoff_rest();



static exrole_t find_exrole(unsigned len, unsigned hashcode, const char* name) {
	split_exrole_comp_t searcher{ name,len, hashcode };
	auto lbound = std::lower_bound(g_role_search_table.begin(), g_role_search_table.end(), searcher);

	if (lbound == g_role_search_table.end() || strcmp(lbound->m_name, name))
		return exrole_t::none;
	else {
		uintptr_t diff = lbound - g_role_search_table.begin();
		std::underlying_type_t<exrole_t> ele = g_lut_exrole[diff];
		return *reinterpret_cast<exrole_t*>(&ele);
	}
}




static exrole_t generate_role_for_helper_name(const char* helper) {
	//msg("%s\n", _name);
	unsigned length = strlen(helper);

	unsigned hashcode = cs::const_ops::hash_string(helper);

	return find_exrole(length, hashcode, helper);
}

constexpr unsigned EXROLE_MASK = 1 << 31;


static constexpr unsigned encode_exrole(exrole_t role) {
	return static_cast<unsigned>(role) | EXROLE_MASK;
}

static constexpr exrole_t decode_exrole(unsigned role) {
	if (!(role & EXROLE_MASK)) {
		return exrole_t::none;
	}
	else {
		role = role & ~EXROLE_MASK;
		std::underlying_type_t<exrole_t> _role = role;
		return *reinterpret_cast<exrole_t*>(&_role);
	}
}

struct traversal_traits_tag_exrole_t {
	static constexpr bool maintain_context_ptrs = false;

	static constexpr unsigned AUXSTACK_SIZE = 32;
	static constexpr bool may_term_flow = false;

};

static CS_FORCEINLINE bool find_and_tag_helper_exrole_inline(minsn_t* inner) {
	if (inner->op() != m_call)
		return false;

	if (inner->l.t != mop_h)
		return false;

	exrole_t role = generate_role_for_helper_name(inner->l.helper);

	if (role == exrole_t::none)
		return false;

	cs_assert(inner->d.t == mop_f);

	inner->d.f->role = (funcrole_t)encode_exrole(role);
	return true;
}
bool find_and_tag_helper_exrole(minsn_t* inner) {
	return find_and_tag_helper_exrole_inline(inner);
}
/*
Traverse all instructions of the current function and tag them with their extended roles
*/
void tag_helpers_with_exrole(mbl_array_t* mba) {
	for (auto&& blk : forall_mblocks(mba)) {
		for (auto insn = blk->head; insn; insn = insn->next) {
			traverse_minsn< traversal_traits_tag_exrole_t>(insn, [](mop_t * mop) {

				if (mop->t != mop_d)
					return;

				minsn_t* inner = mop->d;
				find_and_tag_helper_exrole_inline(inner);

			});

		}

	}
}

exrole_t get_instruction_exrole(minsn_t* insn) {
	cs_assert(insn);

	if (insn->op() != m_call || insn->l.t != mop_h)
		return exrole_t::none;

	
	cs_assert(insn->d.t == mop_f);
	return decode_exrole(insn->d.f->role);
}