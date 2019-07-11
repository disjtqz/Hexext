
#include <ida.hpp>
#include <idp.hpp>
#include <loader.hpp>
#include <kernwin.hpp>
#include <ieee.h>
#include <hexrays.hpp>
#include <functional>
#include <array>
#include <list>
#include <string>

#include "micro_on_70.hpp"
#include "exrole/exrole.hpp"
#include "hexutils/hexutils.hpp"
#include "ctree/ctree.hpp"

static struct {
	unsigned did_run_preopt : 1;
	unsigned did_run_locopt : 1;
	unsigned did_run_glbopt : 1;

}g_microgen_state_flags;


bool hexext::ran_glbopt() {
	return g_microgen_state_flags.did_run_glbopt;
}
bool hexext::ran_locopt() {
	return g_microgen_state_flags.did_run_locopt;
}
bool hexext::ran_preopt() {
	return g_microgen_state_flags.did_run_preopt;
}
/*
	micro_filter_api needs these defs
*/
static bool dispatch_microcode_generated(mbl_array_t* mba);
static std::list<asmrewrite_cb_t> g_asm_rewrite_cbs;
#include "internal/micro_filter_api.hpp"


#include "mixins/set_codegen_small.mix"
static std::list < glbopt_cb_t> g_glbopt_cbs;
static std::list<combine_cb_t> g_combine_cbs;
static std::list<microcode_generated_cb_t> g_microcode_generated_cbs;

static std::list<preopt_cb_t> g_preopt_cbs;
static std::list<locopt_cb_t> g_locopt_cbs;

hexdsp_t* hexdsp = nullptr;
void* mblock_t_vftbl = nullptr;
void* codegen_t_vftbl = nullptr;


static micro_filter_api_t g_filter_api{};
/*
	this part is bad and combination rules need a better way to access parents :/
*/
static unsigned g_parent_mop_size = -1;
static minsn_t* g_current_topinsn = nullptr;
static mop_t* g_current_mop = nullptr;

static int dispatch_combinsn(mblock_t* block, minsn_t* insn);
CS_COLD_CODE
void hexext::install_microcode_filter_ex(hexext_micro_filter_t* mcu, bool install) {

	if (install) {
		g_filter_api.install_filter(mcu);
	}
	else {
		g_filter_api.remove_filter(mcu);
	}

}

static int g_did_glbopt_loop = 0;
static bitset_t g_bbidx_pool{};
static int dispatch_glbopt(mbl_array_t* mba) {
	bool did_modify = false;
	for (auto&& cb : g_glbopt_cbs) {
		if (cb(mba)) {
			did_modify = true;
		}
	}
	/*
		combine wont get called for single instruction blocks
	*/
	if (!did_modify) {
		for (mblock_t* blk = mba->blocks; blk; blk = blk->nextb) {
			for (minsn_t* insn = blk->head; insn; insn = insn->next) {
				if (dispatch_combinsn(blk, insn)) {
					did_modify = true;
				}
			}
		}
	}
	if (did_modify) {
		return MERR_LOOP;
	}


	return MERR_OK;
}

mop_t* hexext::current_comb_mop() {
	return g_current_mop;
}
unsigned hexext::get_parent_mop_size() {
	return g_parent_mop_size;
}

minsn_t* hexext::current_topinsn() {
	return g_current_topinsn;
}
struct traverser_fixup {
	static constexpr unsigned AUXSTACK_SIZE = 128;
	static constexpr bool may_term_flow = false;
	static constexpr bool maintain_context_ptrs = false;
};
static void run_combinsn_fixups(mblock_t* block, minsn_t* insn) {



	traverse_minsn< traverser_fixup>(insn, [insn](mop_t * mop) {
		

		if (mop->t == mop_d && mop->size != mop->d->d.size) {
			cs_assert(false);
			//mop->d->d.size = mop->size;
			}

		if (mop->t == mop_d && mop->d->op() == m_nop) {

			msg("%s\n", print_insn(insn, false).data());
			cs_assert(false);
		}

	});
}


static int dispatch_combinsn(mblock_t* block, minsn_t* insn);
static int dispatch_combinsn_recursive(mblock_t* block, mop_t* insn) {
	g_current_mop = insn;
	
	if (insn->t == mop_p) {
		
		if (dispatch_combinsn_recursive(block, &insn->pair->hop) == 1) {
			return 1;
		}
		

		if (dispatch_combinsn_recursive(block, &insn->pair->lop) == 1) {
			return 1;
		}
	}
	else if (insn->t == mop_d) {
		g_parent_mop_size = insn->size;

		return dispatch_combinsn(block, insn->d);
	}
	else if (insn->t == mop_f) {
		for (auto&& arg : insn->f->args) {
			if (dispatch_combinsn_recursive(block, &arg) == 1) {
				return 1;
			}
		}
	}

	else if (insn->t == mop_a) {
		return dispatch_combinsn_recursive(block, insn->a);
	}
	return 0;
}


mcombine_t g_mcombine_state{};
static int dispatch_combinsn(mblock_t* block, minsn_t* insn) {

	if (g_bbidx_pool.high < block->mba->qty) {
		g_bbidx_pool.resize(block->mba->qty);
	}
	/*
		at the time when i was developing this and wrote most of the rules I was accidentally using a version of hexrays i patched a while back 
		to remove calls to mblock verify, minsn verify to speed up decompilation

		so i missed a lot of potential interrs until recently. run_combinsn_fixups is just a bandaid over a gaping wound in this setup



	*/
#if 0
	auto done = [block, insn]() {
		run_combinsn_fixups(block, insn);
		return 1;
	};
#else
	auto done = []() {return 1; };
#endif

	/*if (insn->iprops & IPROP_DONT_COMB)
		return 0;*/
	if (insn->next || insn->prev) {
		g_current_topinsn = insn;
		g_current_mop = nullptr;
	}


	g_mcombine_state.m_block = block;
	g_mcombine_state.m_insn = insn;
	g_mcombine_state.m_bbidx_pool = &g_bbidx_pool;

	/*g_mcombine_state.m_bbidx_pool = 
		fixed_size_vecparm_t<unsigned>::emplace_at(
		alloca(fixed_size_vecparm_t<unsigned>::required_allocation_size(block->mba->qty)), block->mba->qty);

		*/

	for (auto&& cb : g_combine_cbs) {
		if (cb->combine(&g_mcombine_state)) {

			msg("Rule %s performed its transformations successfully.\n", cb->name());
			return done();
			//return 1;
		}
	}
#if 1
	/*
		should probably just use traverse_minsn
	*/

	if (dispatch_combinsn_recursive(block, &insn->d)) {
		return done();
		}
	else if (dispatch_combinsn_recursive(block, &insn->l)) {
	return done();
		}
	else if (dispatch_combinsn_recursive(block, &insn->r)) {
	return done();
		}
#endif
	return 0;
}



static bool dispatch_microcode_generated(mbl_array_t* mba) {

	/*
		this doesnt always get called once microcode is fully generated. i havent looked too much into why because nothing uses this yet, other than dump_mba in testing
	*/
	for (auto&& cb : g_microcode_generated_cbs) {
		if (cb(mba)) {
			return true;
		}
	}
	return false;
}

static const char* stringify_hxe(hexrays_event_t even) {
#define e(name)	case name : return #name;

	switch (even) {
		e(hxe_flowchart)
		e(hxe_prolog)
		e(hxe_preoptimized)
		e(hxe_locopt)
		e(hxe_prealloc)
		e(hxe_glbopt)
		e(hxe_structural)
		//e(hxe_combine)
		e(hxe_resolve_stkaddrs)
	}
#undef e
	return nullptr;
}

static int idaapi hexcb(void* ud, hexrays_event_t event, va_list va) {
	/*
		Just for analyzing the order in which events are dispatched
	*/
#if 0
	if(stringify_hxe(event))
	msg("%s\n", stringify_hxe(event));
#endif
	if (event == hxe_glbopt) {
		g_microgen_state_flags.did_run_glbopt = 1;
		mbl_array_t* mba = va_arg(va, mbl_array_t*);

		
		return dispatch_glbopt(mba);
	}
	else if (event == hxe_prolog) {
		/*
			reset codegen flags
		*/

		g_microgen_state_flags.did_run_glbopt = 0;
		g_microgen_state_flags.did_run_locopt = 0;
		g_microgen_state_flags.did_run_preopt = 0;
	}
	else if (event == hxe_combine) {
		mblock_t* block = va_arg(va, mblock_t*);
		minsn_t* insn = va_arg(va, minsn_t*);

		return dispatch_combinsn(block, insn);
	}

	else if (event == hxe_locopt) {
		g_microgen_state_flags.did_run_locopt = 1;
		mbl_array_t* mba = va_arg(va, mbl_array_t*);
		for (auto&& cb : g_locopt_cbs) {
			cb(mba);
		}
		return 0;
	}
	else if (event == hxe_preoptimized) {
		g_microgen_state_flags.did_run_preopt = 1;
		g_did_glbopt_loop = 0;

		

		

		mbl_array_t* mba = va_arg(va, mbl_array_t*);

		/*
		This seems like the best spot for now to tag all helper functions with their extended roles
		*/
		tag_helpers_with_exrole(mba);
		bool did_preopt = false;
		rerun:
		for (auto&& preopt : g_preopt_cbs) {
			if (preopt(mba)) {
				did_preopt = true;
				goto rerun;
			}
				
		}
		if (did_preopt) {
			/*
				another shitty fixup, but the postpreopt pass will probably spoil all of the lists anyway
			*/
			for (mblock_t* blk = mba->blocks; blk; blk = blk->nextb) {
				blk->flags &= ~MBL_LIST;
			}
			//havent checked yet to see if preoptimization honors MERR_LOOP. rerunning preoptimization probably wont have many benefits anyway
			//return MERR_LOOP;
			return 0;//lets not slow decompilation the fuck down 
		}
		return 0;

	}
	else if (event == hxe_maturity) {
		cfunc_t* cf = va_arg(va, cfunc_t*);
		ctree_maturity_t mat = va_arg(va, ctree_maturity_t);
		hexext::execute_ctree_rules(cf, mat);
	}
	return 0;
}


static void install_cb() {
	install_hexrays_callback(&hexcb, nullptr);
}

static void remove_cb() {
	remove_hexrays_callback(&hexcb, nullptr);
}
CS_COLD_CODE
void hexext_internal::init_hexext() {
	init_hexrays_plugin();
	if (!hexdsp) {
		msg("Daww hexdsp is null.\n");
		return;
	}
	else {
		install_cb();

		install_microcode_filter(&g_filter_api, true);
	}
}
CS_COLD_CODE
void hexext_internal::deinit_hexext() {
	// clean up
	if (!hexdsp)
		return;
	install_microcode_filter(&g_filter_api, false);

	remove_cb();
	term_hexrays_plugin();

}

CS_COLD_CODE
void hexext::install_glbopt_cb(glbopt_cb_t cb)  {
	g_glbopt_cbs.push_back( cb);
}
CS_COLD_CODE
void hexext::install_combine_cb(combine_cb_t cb) {
	g_combine_cbs.push_back(cb);
}
CS_COLD_CODE
void hexext::remove_combine_cb(combine_cb_t cb) {
	g_combine_cbs.remove(cb);
}
CS_COLD_CODE
void hexext::install_combine_cb(combine_cb_t cb, bool enabled) {
	if (enabled) {
		install_combine_cb(cb);
	}
	else
		remove_combine_cb(cb);
}
CS_COLD_CODE
void hexext::install_locopt_cb(locopt_cb_t cb, bool enabled) {
	if (enabled) {
		g_locopt_cbs.push_back(cb);
	}
	else {
		g_locopt_cbs.remove(cb);
	}
}
CS_COLD_CODE
void hexext::install_preopt_cb(preopt_cb_t cb, bool enabled) {
	if (enabled) {
		g_preopt_cbs.push_back(cb);
	}
	else {
		g_preopt_cbs.remove(cb);
	}
}
CS_COLD_CODE
void hexext::install_microcode_generated_cb(microcode_generated_cb_t cb) {
	g_microcode_generated_cbs.push_back(cb);
}
CS_COLD_CODE
void hexext::install_asm_rewriter(asmrewrite_cb_t cb) {
	g_asm_rewrite_cbs.push_back(cb);
}
CS_COLD_CODE
void hexext::remove_asm_rewriter(asmrewrite_cb_t cb) {
	g_asm_rewrite_cbs.remove(cb);
}
CS_COLD_CODE
void hexext::install_asm_rewriter(asmrewrite_cb_t cb, bool enabled) {
	if (enabled) {
		install_asm_rewriter(cb);
	}
	else {
		remove_asm_rewriter(cb);
	}
}

int hexext::microgen_decode_insn(insn_t* insn, ea_t ea) {
	return g_filter_api._microgen_decode_insn(insn, ea);
}

ea_t hexext::microgen_decode_prev_insn(insn_t* insn, ea_t ea) {
	return g_filter_api._microgen_decode_prev_insn(insn, ea);
}
#include "mixins/revert_codegen.mix"