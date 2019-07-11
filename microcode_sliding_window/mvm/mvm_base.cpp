#include "mvm_defs_internal.hpp"

#ifndef __EA64__
#include "mvm_arm32.hpp"
#include "mvm_x86.hpp"
#else
#include "mvm_aarch64.hpp"
#include "mvm_x86_64.hpp"
#endif

#include "mvm_x86_common.hpp"
mreg_t mr_data_seg = -1;
mreg_t mr_code_seg = -1;
rlist_t rlist_tempregs{};
rlist_t rlist_all_ccs{};
mreg_t mr_eax = -1, mr_ebx = -1,
mr_ecx = -1, mr_edx = -1;
mreg_t mr_t0 = -1,
mr_t1 = -1,
mr_t2 = -1;
std::vector<mreg_info_t> g_mreg_info{};
unsigned g_max_mreg = 0;
static bool g_did_init_mregs = false;
#include "../mixins/set_codegen_small.mix"

static int reg2mreg_using_cdg(unsigned r, mreg_t* out, qstring* regname, unsigned* size_out) {
	/*
		ye gods, here be dark magicks
		

		we're tricking the decompiler into telling us the whole layout of the mvm register file in a way
		that works on all four 7.0 decompilers ;)
	*/
	struct blank_codegen_t {
		void* vftbl;

		char restdata[sizeof(codegen_ex_t) - sizeof(void*)];
	};

	blank_codegen_t cdg{};

	cdg.vftbl = codegen_t_vftbl;


	codegen_ex_t* bcdg = reinterpret_cast<codegen_ex_t*>(&cdg);

	//qstring regname{};

	unsigned sz = 64;


	while ((get_reg_name(regname, r, sz) == -1 || regname->empty()) && sz) {
		sz >>= 1;
	}
	/*
		special case. this was causing crashes on ARM64
		possibly arm32 too? havent verified yet
	*/
	if (hexext::currarch() == hexext_arch_e::ARM && !strcmp(regname->c_str(), "PC")) {
		return -1;
	}

	//cs_assert(!regname.empty());
	if (regname->empty() && r > 2000)
		return 0;
	bitrange_t regrange{};
	get_reg_info(regname->c_str(), &regrange);

	reg_info_t reginfo{};

	unsigned regsize = regrange.bytesize();
	if (regsize == 0) {
		parse_reg_name(&reginfo, regname->c_str());
		regsize = reginfo.size;
	}
	//get the proper full size regname
	//get_reg_name(regname, r, regsize);

	bcdg->insn.ops[0].type = o_reg;

	bcdg->insn.ops[0].dtype = get_dtype_by_size(regsize);

	bcdg->insn.ops[0].n = 0;
	bcdg->insn.ops[0].reg = r;
	auto bleh = bcdg->load_operand(0);
	if (bleh == -1)
		return -1;
	*out = bleh;
	*size_out = regsize;

	return 1;
}

static int compare_mreg_info(const void* _x, const void* _y) {
	const mreg_info_t* x = reinterpret_cast<const mreg_info_t*>(_x);
	const mreg_info_t* y = reinterpret_cast<const mreg_info_t*>(_y);

	if (x->m_micro_reg == y->m_micro_reg)
		return 0;
	else if (x->m_micro_reg < y->m_micro_reg)
		return -1;
	else
		return 1;
}
CS_NOINLINE
static void sort_mregs() {
	std::sort(&g_mreg_info[0], &g_mreg_info[g_max_mreg], [](const mreg_info_t & x, const mreg_info_t & y) {

		return x.m_micro_reg < y.m_micro_reg;

		});
}
static void preinit_mreg_info() {
	g_max_mreg = 0;

	qstring buffer{};
	unsigned curr_reg = 0;
	//g_mreg_info.push_back({});
	do {
		mreg_info_t _curr_entry{};

		mreg_info_t* curr_entry = &_curr_entry;//&g_mreg_info[g_max_mreg];

		unsigned sz = 0;
		mreg_t mr = -1;

		int res = reg2mreg_using_cdg(curr_reg, &mr, &buffer, &sz);
		if (!res)
			break;

		if (res != -1) {
			curr_entry->m_micro_reg = mr;
			curr_entry->m_orig_reg = curr_reg;
			curr_entry->m_size = sz;
			qstrncpy(curr_entry->name, buffer.c_str(), 16);
			g_mreg_info.push_back(_curr_entry);
			g_max_mreg++;

		}
		buffer.clear();
		curr_reg++;
	} while (true);
	//yeah this is garbage
	//g_mreg_info.pop_back();
	sort_mregs();
	

		

	//use qsort for smaller binary size
	//qsort(&g_mreg_info[0], g_max_mreg, sizeof(mreg_info_t), compare_mreg_info);
	g_did_init_mregs = true;

	for (unsigned r = mr_cf; r < mr_pf + 1; ++r) {
		rlist_all_ccs.add(r);
	}

}



CS_COLD_CODE
std::string print_mreg(int mr) {

	for (unsigned i = 0; i < g_max_mreg; ++i) {
		auto& descr = g_mreg_info[i];

		if (descr.m_micro_reg == mr) {
			return descr.name;
		}

		else if (descr.m_micro_reg < mr && descr.m_micro_reg + descr.m_size > mr) {
			return std::string(descr.name) + "^" + std::to_string(mr - descr.m_micro_reg);
		}
	}

	return "unknown@" + std::to_string(mr);

}


mreg_t reg2mreg(unsigned r) {


	for (unsigned i = 0; i < g_max_mreg; ++i) {
		if (g_mreg_info[i].m_orig_reg == i) {
			return g_mreg_info[i].m_micro_reg;
		}
	}
	cs_assert(false);
}

void _mvm_internal::insert_abs_mreg(const char* name, unsigned mreg, unsigned size) {

	unsigned i = 0;

	for (; i < g_max_mreg; ++i)
		if (g_mreg_info[i].m_micro_reg > mreg)
			break;

	mreg_info_t v{};
	v.m_micro_reg = mreg;
	v.m_orig_reg = 0xFFFF;
	v.m_size = size;
	qstrncpy(v.name, name, sizeof(v.name));
	if (i < g_max_mreg) {
		g_mreg_info.insert(g_mreg_info.begin() + i, v);
	}
	else {
		g_mreg_info.push_back(v);
	}
	g_max_mreg = g_mreg_info.size();

}

mreg_info_t* _mvm_internal::find_mreg_by_name(const char* name) {
	reg_info_t rinfo{};

	if (parse_reg_name(&rinfo, name)) {

		auto r = rinfo.reg;

		for (auto&& mreginfo : g_mreg_info) {
			if (mreginfo.m_orig_reg == r) {
				return &mreginfo;
			}
		}
	}
	/*
		probably a special reg
		search by name
	*/

	for (auto&& mreginfo : g_mreg_info) {
		if (!stricmp(mreginfo.name, name)) {
			return &mreginfo;
		}
	}

	/*
		this function is really only used by initial mvm setup so failing to find an mreg should be considered an error
	*/
	cs_assert(false);

}
CS_COLD_CODE
void ensure_mvm_init() {
	if (g_did_init_mregs)
		return;
	g_mreg_info.reserve(8192);
	preinit_mreg_info();


#ifdef __EA64__

	if (hexext::currarch() == hexext_arch_e::x86) {
		mvm_x86_64_init();
	}
	else {
		mvm_aarch64_init();
	}

#else

	if (hexext::currarch() == hexext_arch_e::x86) {
		mvm_x86_init();
	}
	else {
		mvm_arm32_init();
	}


#endif
	using namespace _mvm_internal;
	if (hexext::currarch() == hexext_arch_e::x86) {

	

		mr_eax = find_mreg_by_name("eax")->m_micro_reg;
		mr_ebx = find_mreg_by_name("ebx")->m_micro_reg;
		mr_ecx = find_mreg_by_name("ecx")->m_micro_reg;
		mr_edx = find_mreg_by_name("edx")->m_micro_reg;

	}


	mr_t0 = find_mreg_by_name("t0")->m_micro_reg;

	mr_t1 = find_mreg_by_name("t1")->m_micro_reg;

	mr_t2 = find_mreg_by_name("t2")->m_micro_reg;
	//g_mreg_info.shrink_to_fit();
}

void _mvm_internal::add_tempreg_to_list(const char* name) {
	auto mr = find_mreg_by_name(name);
	rlist_tempregs.add_(mr->m_micro_reg, mr->m_size);

}

#include "../mixins/revert_codegen.mix"

bool mreg_has_highbyte_x86(mreg_t mr) {
	cs_assert(hexext::currarch() == hexext_arch_e::x86);

	return mr == mr_eax || mr == mr_ebx || mr == mr_ecx || mr == mr_edx;

}

unsigned mreg_sizeof(mreg_t mr) {
	
	for (auto&& mreg : g_mreg_info) {
		if (interval::contains(mreg.m_micro_reg, mreg.m_size, mr)) {
			return mreg.m_size;
		}
	}
	cs_assert(false);
}

bool is_kreg(mreg_t mr) {
	auto& last = g_mreg_info.back();
	if (mr > last.m_size + last.m_micro_reg) {
		return true;
	}
	return false;
}

bool is_simdreg(mreg_t mr) {
	if (hexext::currarch() == hexext_arch_e::x86) {
		return x86_is_simdreg(mr);
	}
	else {
		return false;
	}
}