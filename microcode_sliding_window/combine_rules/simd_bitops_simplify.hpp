#pragma once

/*
add rbp.8, (low (call _mm_srli_si128, .-1, ((ldx rds.2, rbx.8).16,#8.1)).16, .-1).8, r8.8
*/

class simd_ld_shrtrim_t : public mcombiner_rule_t {
public:
	virtual const char* name() const override {
		return "SIMD128 bytewise shift-right by 8 transformer";
	}

	virtual bool run_combine(mcombine_t* state);
};


class detect_xdu_in_xor128_t : public mcombiner_rule_t {
public:
	virtual const char* name() const override {
		return "Detect simd128 xor float negation";
	}
	virtual bool run_combine(mcombine_t* state);

};

extern simd_ld_shrtrim_t simd_ld_shrtrim;

extern detect_xdu_in_xor128_t detect_xdu_in_xor128;