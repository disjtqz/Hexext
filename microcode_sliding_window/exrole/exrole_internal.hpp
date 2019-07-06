#pragma once
/*
	We recalculate the hash codes for the intrinsic function name along with their lengths for quick comparison. 
	This is all done at compile time using constant expressions. 
	After we pre-generate the full table we split it off into two separate tables, 
	one containing the name along with the hash code for the name and the length of the name. 
	The second table contains the actual corresponding role for the name. 
	This reduces cache pollution when we are searching the table to find the role. 
	In order to get the actual corresponding role for the name we just have to subtract the start of the first table from 
	the pointer into the first table that matched the name giving us the index. 
	We then look up that index in the second table giving us the role

	In theory this should make calculating the role for an intrinsic relatively inexpensive but we still should do it infrequently
*/
class cs_funcdescr_t {
	const char* const m_name;
	const unsigned m_length;
	const unsigned m_hashcode;
public:
	constexpr cs_funcdescr_t() : m_name(nullptr), m_length(0), m_hashcode(0)
	{}
	constexpr cs_funcdescr_t(const char* name) : m_name(name), m_length(cs::const_ops::string_length(name)), m_hashcode(cs::const_ops::hash_string(name))
	{}


	constexpr const char* name() const {
		return m_name;
	}

	constexpr unsigned length() const {
		return m_length;
	}

	constexpr unsigned hashcode() const {
		return m_hashcode;
	}


};



struct cs_funcset_submit_t {
	unsigned m_len;
	unsigned m_hashcode;
	const char* m_name;
	exrole_t m_role;
	constexpr bool operator <(const cs_funcset_submit_t& other)const {

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
	//cs_funcclass_t m_cls;
};

using cs_funcclass_tree_t = std::map< cs_funcset_submit_t, exrole_t, std::less<cs_funcset_submit_t>>;//, 
	//cs::mem::low32_allocator<std::pair<const cs_funcset_submit_t, cs_funcclass_t>>>;

template<typename... Ts>
class cs_funcset_t {
	const std::array<cs_funcdescr_t, sizeof...(Ts)> m_arr;
	const exrole_t m_cls;
public:
	constexpr cs_funcset_t(exrole_t cls, Ts&& ... args) : m_arr({ args... }), m_cls(cls)
	{}

	template<unsigned n>

	constexpr void add_to_roleset(fixed_size_vector_t<cs_funcset_submit_t, n>& tree) const{
		for (unsigned i = 0; i < sizeof...(Ts); ++i) {
			auto&& descr = m_arr[i];
			cs_funcset_submit_t sub{ descr.length(),descr.hashcode(),descr.name(),m_cls };

			tree.push_back(sub);
		}
	}

};

template<typename... Ts>
static constexpr auto make_funcset(exrole_t cls, Ts&& ...args) {
	return cs_funcset_t{ cls, cs_funcdescr_t{args} ... };
}


