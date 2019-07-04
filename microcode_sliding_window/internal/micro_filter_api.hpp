#pragma once

/*
	included by micro_hooks.cpp.
*/
class micro_filter_api_t : public microcode_filter_t {
	bool m_recursion_guard;
	std::set<hexext_micro_filter_t*> m_filters;

	mbl_array_t* m_curr_mba;

	bool m_have_dispatched_generated;

	void update_from_codegen(codegen_t& cdg);
	hexext_micro_filter_t* m_selected_filter;

	ea_t m_curr_insn_end_ea;
	ea_t m_expected_fn_end;


	bool m_did_change_insn;
public:

	virtual bool match(codegen_t& cdg);

	virtual int apply(codegen_t& cdg);

	void install_filter(hexext_micro_filter_t* mcu) {
		m_filters.insert(mcu);
	}

	void remove_filter(hexext_micro_filter_t* mcu) {
		m_filters.erase(mcu);
	}

	bool preprocess_insn(insn_t& insn) {
		bool did_change = false;
	rerun:
		for (auto&& filter : g_asm_rewrite_cbs) {
			if (filter(insn)) {
				did_change = true;
				goto rerun;
			}
		}
		return did_change;
	}


	int _microgen_decode_insn(insn_t* insn, ea_t ea) {
		int result = decode_insn(insn, ea);

		preprocess_insn(*insn);
		return result;
	}
	ea_t _microgen_decode_prev_insn(insn_t* insn, ea_t ea) {
		ea_t result = decode_prev_insn(insn, ea);
		preprocess_insn(*insn);
		return result;
	}
};


bool micro_filter_api_t::match(codegen_t& cdg) {
	codegen_t_vftbl = *reinterpret_cast<void**>(&cdg);
	//if (m_recursion_guard)
	//	return false;
	if (cdg.mb) {
		mblock_t_vftbl = cdg.mb->vtable;
	}
	ensure_mvm_init();
	bool did_change = preprocess_insn(cdg.insn);

	m_did_change_insn = did_change;

	update_from_codegen(cdg);
	m_selected_filter = nullptr;
	for (auto&& filter : m_filters) {
		if (filter->match(*reinterpret_cast<codegen_ex_t*>(&cdg))) {
			m_selected_filter = filter;
			break;
		}
	}
	if (!m_selected_filter && m_recursion_guard) {
		return false;
	}

	return true;
}

int micro_filter_api_t::apply(codegen_t& cdg) {
	int result = MERR_OK;
	if (m_selected_filter) {
		result = m_selected_filter->apply(*reinterpret_cast<codegen_ex_t*>(&cdg));
	}
	else {
		m_recursion_guard = true;
		result = cdg.gen_micro();
		m_recursion_guard = false;

	}

	if (m_curr_insn_end_ea == m_expected_fn_end) {
	//	assert(!m_have_dispatched_generated);
		//while (dispatch_microcode_generated(m_curr_mba)
			//);
		dispatch_microcode_generated(m_curr_mba);
		m_have_dispatched_generated = true;
	}

	return result;
}
void micro_filter_api_t::update_from_codegen(codegen_t& cdg) {
	if (cdg.mba != m_curr_mba) {
		m_have_dispatched_generated = false;

		m_curr_mba = cdg.mba;
		m_expected_fn_end = get_func(cdg.insn.ea)->end_ea;
	}
	m_curr_insn_end_ea = cdg.insn.ea + cdg.insn.size;
}
