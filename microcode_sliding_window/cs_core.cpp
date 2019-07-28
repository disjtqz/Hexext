#include "cs_core.hpp"

#include "pro.h"
#include "kernwin.hpp"
#include <hexrays.hpp>
#include "mixins/set_codegen_small.mix"
CS_COLD_CODE
CS_NOINLINE
CS_NORETURN
void cs::_cs_assert_fail_raise(const cs_assert_fail_descr_t* descr) {

	char errbuff[4096] = { 0 };

	qstrncpy(errbuff, "Assertion failed in function ", sizeof(errbuff));
	char line_text[32];

	//itoa(descr->line_no, line_text, 10);

	qstrncat(errbuff, descr->fn_name, sizeof(errbuff));
	qstrncat(errbuff, line_text, sizeof(errbuff));

	qstrncat(errbuff, " in file ", sizeof(errbuff));

	qstrncat(errbuff, descr->_file_name, sizeof(errbuff));




	/*MessageBoxA(nullptr, (descr->_err_msg), errbuff, MB_ICONERROR);
	if (IsDebuggerPresent()) {
		__debugbreak();
	}

	exit(1);*/


	warning(errbuff);

	if (under_debugger) {
		__debugbreak();
	}

	interr(66666);
}
#include "mixins/revert_codegen.mix"

bool make_compare_interval_from_sorted_range(fixed_size_vecptr_t<u64_comp_interval_t> ivls, fixed_size_vecptr_t<uint64_t> outliers,
	fixed_size_vecptr_t<uint64_t> input) {

	

	if (!input->size())
		return true;
	bool in_range = false;
	uint64_t range_start = 0;

	uint64_t previous = input->operator[](0);

	for (unsigned i = 1; i < input->size(); ++i) {
		uint64_t val = input->operator[](i);

		if (in_range && val != (previous + 1)) {

			u64_comp_interval_t intvs{};
			intvs.low = range_start;
			intvs.high = previous;
			if (!ivls->push_back(intvs))
				return false;
			in_range = false;
			
		}
		else if (!in_range && val != (previous + 1)) {

			if (!outliers->push_back(previous))
				return false;
		}
		else if (!in_range && val == (previous + 1)) {
			in_range = true;
			range_start = previous;
		}
		previous = val;
	}

	return true;

}