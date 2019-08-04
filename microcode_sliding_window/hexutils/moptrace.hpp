#pragma once
struct moptrace_state_t {
	mlist_t m_list;
	mop_t* m_focus;
	mblock_t* m_block;
	minsn_t* m_redef_pos;
	minsn_t* m_prev_position;
	minsn_t* m_position;
	potential_valbits_t m_value_bitmask;
	bool m_redefed;
	moptrace_state_t() : m_list(), m_focus(nullptr), m_block(nullptr), m_redef_pos(nullptr), m_prev_position(nullptr), m_position(nullptr), m_value_bitmask(), m_redefed(false) {}
	~moptrace_state_t() {
		m_focus = nullptr;
	}

	minsn_t* next_for() {
		cs_assert(!was_redefed());
		m_prev_position = m_position;
		m_redef_pos = nullptr;
		return (m_position = find_next_use(m_block, m_position, &m_list, &m_redefed, &m_redef_pos));
	}

	bool adjust_for_partial_redef() {
		if (!was_redefed())
			return true;

		if (!m_redef_pos)
			return false;

		mlist_t df{};
		generate_def_for_insn(m_block->mba, m_redef_pos, &df);

		m_list.reg.sub(df.reg);
		//cant handle memory rn
		if (m_list.mem.bag_.n && df.mem.bag_.n)
			return false;


		if (!m_list.reg.empty() || !m_list.mem.empty()) {
			m_redefed = false;
			m_prev_position = nullptr;
			m_position = m_redef_pos;
			m_redef_pos = nullptr;
			return true;
		}

		return false;


	}

	bool was_redefed() const {
		return m_redefed;
	}

	void add_alias(mop_t* alias) {
		add_mop_to_mlist(alias, &m_list, &m_block->mba->vars);
	}

	void add_alias(mlist_t* alias) {
		m_list.reg.add(alias->reg);
		m_list.mem.add_(&alias->mem);
	}
};

namespace trace_flags {
	enum {
		ALLOW_PARTIAL_REDEF = 1,
		TRACE_ZERO_EXTEND_DESTS = 2,
		TRACE_SIGN_EXTEND_DESTS = 4,
	};
}

template<size_t stackdepth, unsigned _tr_flags = trace_flags::ALLOW_PARTIAL_REDEF>
class moptrace_t {
	fixed_size_vector_t<moptrace_state_t, stackdepth> m_state_stack;
public:
	moptrace_t() : m_state_stack() {}

	~moptrace_t() {
		for (auto&& state : m_state_stack) {
			if (state.m_focus)
				state.~moptrace_state_t();
		}
		m_state_stack.reset();
	}

	moptrace_state_t* state() {
		return &m_state_stack[0];
	}

	void set(mblock_t* block, minsn_t* after, mop_t* focus) {
		state()->m_block = block;
		state()->m_position = after;
		state()->m_focus = focus;
		state()->m_list.clear();

		if (after && &after->d == focus) {
			state()->m_value_bitmask = try_compute_insn_potential_valbits(after);
		}
		else {
			state()->m_value_bitmask.set_all();
		}
		add_mop_to_mlist(focus, &state()->m_list, &block->mba->vars);
	}

	void set(mblock_t* block, minsn_t* after) {
		state()->m_block = block;
		state()->m_position = after;
		state()->m_focus = nullptr;
		state()->m_list.clear();
		state()->m_value_bitmask.set_all();
	}
	template<typename TCallable>
	inline minsn_t* trace(TCallable&& callable) {
		minsn_t* current = state()->next_for();
		bool running = true;
		do {
			for (; current; current = state()->next_for()) {
				if (current->op() == m_mov && test_mop_in_mlist(&current->l, &state()->m_list, &state()->m_block->mba->vars)) {
					state()->add_alias(&current->d);
				}
				else if (current->op() == m_xdu && (_tr_flags & trace_flags::TRACE_ZERO_EXTEND_DESTS)
					&& test_mop_in_mlist(&current->l, &state()->m_list, &state()->m_block->mba->vars)) {
					state()->add_alias(&current->d);
				}
				else if (current->op() == m_xds && (_tr_flags & trace_flags::TRACE_SIGN_EXTEND_DESTS)
					&& test_mop_in_mlist(&current->l, &state()->m_list, &state()->m_block->mba->vars)) {
					state()->add_alias(&current->d);
				}
				else {
					if (callable(current)) {
						return current;
					}
				}
			}
			running = false;
			if (state()->was_redefed()) {
				if constexpr (_tr_flags & trace_flags::ALLOW_PARTIAL_REDEF) {
					if (state()->adjust_for_partial_redef()) {
						running = true;
						current = state()->next_for();
					}
				}

			}
		} while (running);
		return nullptr;
	}
	potential_valbits_t value_possible_bits() {
		return state()->m_value_bitmask;
	}
};
