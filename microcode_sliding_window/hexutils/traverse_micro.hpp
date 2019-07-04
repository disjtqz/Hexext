#pragma once
namespace cs::traversal_helpers {

	enum {
		TAG_MINSN = 0,
		TAG_CALL = 1,
		TAG_PAIR = 2
	};

	struct context_ptr_t {
		uintptr_t m_contextptr;


		void set_ptr_with_tag(void* p, unsigned tag) {
			m_contextptr = ((uintptr_t)p) | tag;
		}

		unsigned tag() {
			return m_contextptr & 3;
		}
		bool is_insn() {
			return tag() == TAG_MINSN;
		}

		bool is_call() {
			return tag() == TAG_CALL;
		}

		bool is_pair() {
			return tag() == TAG_PAIR;
		}

		void set_minsn(minsn_t* p) {
			set_ptr_with_tag((void*)p, TAG_MINSN);
		}

		void set_call(mcallinfo_t* cinfo) {
			set_ptr_with_tag((void*)cinfo, TAG_CALL);
		}

		void set_pair(mop_pair_t* pa) {
			set_ptr_with_tag((void*)pa, TAG_PAIR);
		}

		template<typename T>
		T* get() {
			return reinterpret_cast<T*>(m_contextptr & (~3ULL));
		}

		minsn_t* insn() {
			return get<minsn_t>();
		}

		mcallinfo_t* callinfo() {
			return get<mcallinfo_t>();
		}

		mop_pair_t* pair() {
			return get<mop_pair_t>();
		}
	};
}

enum traversal_exflags_e : uint64_t {

	EXFLAGS_TRACK_PARENTINSN = 1,

};

template<typename traversal_traits, uint64_t exflags = 0,typename TParam = void, typename TCallable = void>
static inline void traverse_micro(TParam* parm, TCallable&& callable) {
	constexpr unsigned AUXSTACK_SIZE = traversal_traits::AUXSTACK_SIZE;
	constexpr bool maintain_context_ptrs = traversal_traits::maintain_context_ptrs;

	constexpr bool may_term_flow = traversal_traits::may_term_flow;
	using cs::gnu_typeof_eq_v;
	using cs::gnu_is_any_eq_v;
	using namespace cs::traversal_helpers;
	/*
	only need to store references to mop_f, mop_d, and mop_p

	//low 3 bits : tag
	//upper 16 bits : index

	*/
	static constexpr unsigned SIZELUT = (sizeof(mcallarg_t) << 8) | (sizeof(mop_t));

	struct empty_context_t {
	};

	struct auxstack_ele_t : public std::conditional_t< maintain_context_ptrs, context_ptr_t, empty_context_t> {
		uintptr_t ctx;



		inline void encode_position_and_length_and_sizeof(mop_t* mopptr, unsigned idx, unsigned l, unsigned sizeof_tag = 0) {
			uintptr_t temp = (uintptr_t)mopptr;

			temp &= ~0xFFFF000000000000ULL;
			temp |= ((unsigned long long)idx) << 56;

			temp |= ((unsigned long long)l) << 48;
			temp |= sizeof_tag;
			ctx = temp;
		}

		inline void increment_idx() {
			ctx += 1ULL << 56;
		}

		void decode_position_and_length_and_size(mop_t** out_mopptr, unsigned* out_idx, unsigned* out_l, unsigned* sizeout) {
			*out_mopptr = reinterpret_cast<mop_t*>((((intptr_t)ctx & ~(3ULL)) << 16) >> 16);
			*out_idx = ctx >> 56;
			*out_l = (ctx >> 48) & 0xff;



			*sizeout = (unsigned char)(SIZELUT >> ((unsigned)(ctx & 3ULL) * 8));
		}



	};

	std::array< auxstack_ele_t, AUXSTACK_SIZE> auxstack{};
	unsigned auxstack_pos = 0u;

	auto acquire_next = [&auxstack, &auxstack_pos]() {
		if (auxstack_pos == 0) {
			return (mop_t*)nullptr;
		}
		auxstack_ele_t* ele = &auxstack[auxstack_pos - 1];

		mop_t* p;
		unsigned idx;
		unsigned l;
		unsigned size;
		ele->decode_position_and_length_and_size(&p, &idx, &l, &size);


		mop_t* result = reinterpret_cast<mop_t*>(
			((char*)p) + (size * idx));

		if (idx + 1 < l) {
			ele->increment_idx();
		}
		else {
			--auxstack_pos;
		}

		return result;

	};

	auto increment_to_next = [&auxstack, &auxstack_pos, AUXSTACK_SIZE]() {
		cs_assert(auxstack_pos + 1 < AUXSTACK_SIZE);
		return &auxstack[auxstack_pos++];
	};

	auto encode_minsn = [&auxstack, &auxstack_pos, &increment_to_next](minsn_t * __restrict i) {

		mop_t* mops = &i->l;
		if (mops[0].t | mops[1].t | mops[2].t) {
			auxstack_ele_t* ele = increment_to_next();

			ele->encode_position_and_length_and_sizeof(mops, 0, 3);
		}
		else {
		}
	};
	auto encode_single_mop = [&auxstack, &auxstack_pos, &increment_to_next](mop_t * __restrict i) {


		auxstack_ele_t* ele = increment_to_next();

		ele->encode_position_and_length_and_sizeof(i, 0, 1);

	};
	auto encode_callinfo = [&auxstack, &auxstack_pos, &increment_to_next](mcallinfo_t * __restrict mc) {

		if (mc->args.size()) {
			auxstack_ele_t* ele = increment_to_next();
			ele->encode_position_and_length_and_sizeof(&mc->args[0], 0, mc->args.size(), 1);
		}
	};

	auto encode_pair = [&auxstack, &auxstack_pos, &increment_to_next](mop_pair_t * __restrict mp) {
		auxstack_ele_t* ele = increment_to_next();

		ele->encode_position_and_length_and_sizeof(&mp->lop, 0, 2);
	};






	static_assert(gnu_is_any_eq_v<TParam, minsn_t, mop_t, mop_addr_t, mcallinfo_t, mop_pair_t>);
	if constexpr (gnu_typeof_eq_v< TParam, minsn_t>) {
		encode_minsn(parm);
	}
	else if constexpr (gnu_typeof_eq_v<TParam, mop_t>) {
		encode_single_mop(parm);
	}
	else if constexpr (gnu_typeof_eq_v<TParam, mop_addr_t>) {
		encode_single_mop(parm);
	}
	else if constexpr (gnu_typeof_eq_v<TParam, mcallinfo_t>) {
		encode_callinfo(parm);
	}
	else if constexpr (gnu_typeof_eq_v<TParam, mop_pair_t>) {
		encode_pair(parm);
	}

	mop_t* m;


	while (auxstack_pos) {
		m = acquire_next();
	step:
		auto t = m->t;
		if (t != mop_z) {
			if constexpr (!may_term_flow) {
				callable(m);
			}
			else {
				if (callable(m))
					return;
			}



			if (t == mop_d) {
				encode_minsn(m->d);
			}
			else if (t == mop_a) {
				m = m->a;
				goto step;
			}

			else if (t == mop_f) {
				encode_callinfo(m->f);
			}
			else if (t == mop_p) {
				encode_pair(m->pair);
			}
		}
	}

}
/*
	iterate over all mops using a fixed size stack of AUXSTACK_SIZE

	faster than a recursive approach like the one hexrays uses internally
*/
template<typename traversal_traits, typename TCallable>
static inline void traverse_minsn(minsn_t * insn, TCallable && callable) {
	/*
	only need to store references to mop_f, mop_d, and mop_p

	//low 3 bits : tag
	//upper 16 bits : index

	*/

	traverse_micro<traversal_traits>(insn, callable);
}

