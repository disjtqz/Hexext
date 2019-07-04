#include "microgen_defs.hpp"
#include "microgen_core.hpp"

#include "fixup_bittest_codegen.hpp"
#include "shiftx_codegen.hpp"
#include "avx_codegen.hpp"
#include "bmi1_codegen.hpp"
#include "under_debugger_elim.hpp"
#include "remove_nullsub_calls.hpp"
#include "trivial_inliner.hpp"
#include "thunk_function_resolver.hpp"
static std::array g_asm_rewriters_x86_64{
	remove_under_debugger_interrs,
	remove_nullsub_call,
	resolve_thunk_function_address_x86_64,
	operation_then_ret_resolver_x86_64
	//,
	//rewrite_xmmops_with_avx
};

static hexext_micro_filter_t* g_mcu_filters_x86_64[] ={
	&bittest_fixup,
	&shiftx_generator,
	//dis guy loves to cause interrs
	&tzcnt_generator,

	& blsr_generator
#ifdef __EA64__
	,

	//somehow dis guy is, other than bittest fixup, the most stable of them all??
	//untested on x86 tho
	&trivial_inliner_x86_64
#endif
};


static std::array g_asm_rewriters_arm32 = {
	//resolve_thunk_function_address_arm32,
	remove_nullsub_call
};

void toggle_microgen_additions(bool enabled) {

	if (hexext::currarch() == hexext_arch_e::x86) {
		for (auto&& rewriter : g_asm_rewriters_x86_64) {
			hexext::install_asm_rewriter(rewriter, enabled);
		}

		for (auto&& mcu_filter : g_mcu_filters_x86_64) {
			hexext::install_microcode_filter_ex(mcu_filter, enabled);
		}
	}
	else {
#ifndef __EA64__
		for (auto&& rewriter : g_asm_rewriters_arm32) {
			hexext::install_asm_rewriter(rewriter, enabled);
		}
#else

#endif
	}

}