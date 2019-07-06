
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
#include "cs_core.hpp"
#include "micro_on_70.hpp"


#include "mixins/set_codegen_small.mix"


std::string print_mop(mop_t* mop);

CS_COLD_CODE
std::string mcode_to_string(mcode_t op) {
#define	X(name, code)	\
case name: {const char* bleh = #name; return &bleh[2];}
	switch (op) {
#include "mcodexmacro.h"
	}
	return std::to_string(op);
}
CS_COLD_CODE
std::string print_insn(minsn_t* ins, bool addr) {
	std::string result{ mcode_to_string(ins->opcode) };

	if (ins->iprops & IPROP_FPINSN) {
		result = "f." + result;
	}

	result += " ";
	result += print_mop(&ins->l) + ", " + print_mop(&ins->r) + (ins->d.t == mop_z ? "" : ", " + print_mop(&ins->d));
	if (!addr)
		return result;
	else {
		char buffer[32];
		qsnprintf(buffer, 32, "%llx", (unsigned long long)ins->ea);

		return std::string(buffer) + " : " + result;
	}
}


//static constexpr auto g_mreg_descr_table = build_mreg_descrs();



CS_COLD_CODE
std::string print_mop(mop_t* mop) {
	auto t = mop->t;

	std::string result = "";
	bool needsize = true;
	switch (t) {
	case mop_r:
		result += print_mreg(mop->r);
		break;
	case mop_b:
		result += std::to_string(mop->b);
		needsize = false;
		break;
	case mop_n: {
		result += "#" + std::to_string(mop->nnn->org_value);
		break;
	case mop_h:
		result += mop->helper;
		needsize = false;
		break;
	case mop_str:
		result += mop->cstr;
		needsize = false;
		break;
	case mop_a:
		result += "&";
		result += print_mop((mop_t*)mop->a);
		needsize = false;
		break;
	case mop_v:
	{
		qstring nam = get_name(mop->g);

		result += nam.c_str();
		break;
	}
	case mop_f: {
		needsize = false;

		result += "(";
		bool first = true;
		for (auto&& arg : mop->f->args) {
			if (!first) {
				result += ",";
			}
			else {
				first = false;
			}
			result += print_mop(&arg);
		}
		result += ")";
		break;
	}
	case mop_S: {
		result += "stkvar_";
		result += std::to_string(mop->s->off);
		break;
	}
	case mop_c: {
		needsize = false;
		result += "cases(";
		bool first = true;
		for (auto&& cas : mop->c->targets) {
			if (!first) {
				result += ",";
			}
			else {
				first = false;
			}
			result += std::to_string(cas);
		}
		result += ")";
		break;
	}

	case mop_l:
		break;
	case mop_fn: {
		double val;
		
		cs_assert(ph.realcvt((void*)& val, mop->fpc->fnum, 3) == 1);
		result += std::to_string(val);
		break;
	}
	case mop_d:
		result += "(" + print_insn(mop->d, false) + ")";

		break;


	}

	}
	return needsize ? (result + "." + std::to_string(mop->size)) : result;
}


CS_COLD_CODE
void dump_mba(mbl_array_t* mba) {

	for (mblock_t* blk = mba->blocks; blk; blk = blk->nextb) {
		msg("Block %d - %llx to %llx. Flags = %x.\n", blk->serial, (uint64_t)blk->start, (uint64_t)blk->end, (int)blk->flags);
		for (minsn_t* ins = blk->head; ins; ins = ins->next) {
			msg("%s\n", print_insn(ins).data());
		}
	}
}

#include "mixins/revert_codegen.mix"