#include "mvm_defs_internal.hpp"

#ifdef __EA64__
#include "mvm_x86_64.hpp"
#else

#include "mvm_x86.hpp"
#endif

bool x86_is_simdreg(mreg_t mr) {
	if (interval::contains(mri_x86_mm0->mreg(), mri_x86_mm7->end(), mr))
		return true;

	mreg_t xmmstart = mri_x86_xmm0->mreg();
	mreg_t ymmstart = mri_x86_ymm0->mreg();
	mreg_info_t* endxmm;
	mreg_info_t* endymm;
#ifdef __EA64__
	endxmm = mri_x86_xmm15;
	endymm = mri_x86_ymm15;
#else
	endxmm = mri_x86_xmm7;
	endymm = mri_x86_ymm7;
#endif
	return interval::contains(xmmstart, endxmm->end(), mr) || interval::contains(ymmstart, endymm->end(), mr);
}