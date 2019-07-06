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