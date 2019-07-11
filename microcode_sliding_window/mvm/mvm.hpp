#pragma once
struct mreg_info_t {
	uint16_t m_orig_reg;
	uint16_t m_micro_reg;
	uint8_t m_size;
	char name[15];

	unsigned end() const {
		return m_micro_reg + m_size;
	}
	mreg_t mreg() const {
		return m_micro_reg;
	}
};

extern std::vector<mreg_info_t> g_mreg_info;
extern unsigned g_max_mreg;

extern mreg_t mr_data_seg;
extern mreg_t mr_code_seg;
extern rlist_t rlist_tempregs;

extern rlist_t rlist_all_ccs;
namespace _mvm_internal {
	
	void insert_abs_mreg(const char* name, unsigned mreg, unsigned size);

	mreg_info_t* find_mreg_by_name(const char* name);



	void add_tempreg_to_list(const char* name);
	template<typename T, typename... Ts>
	static inline void add_tempregs_to_list(T name, Ts... names) {
		add_tempreg_to_list(name);
		if constexpr (sizeof...(Ts)) {
			add_tempregs_to_list(names...);
		}
	}
}

extern mreg_t mr_eax, mr_ebx,
mr_ecx, mr_edx;

bool mreg_has_highbyte_x86(mreg_t mr);

unsigned mreg_sizeof(mreg_t mr);

bool is_kreg(mreg_t mr);

bool is_simdreg(mreg_t mr);

extern mreg_t mr_t0;
extern mreg_t mr_t1;
extern mreg_t mr_t2;