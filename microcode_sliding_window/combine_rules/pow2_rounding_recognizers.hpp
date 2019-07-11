#pragma once
/*

171208cb : or (shr (or (shr rcx.4, #1.1).4, rcx.4).4, #2.1).4, (or (shr rcx.4, #1.1).4, rcx.4).4, rcx.4
171208d9 : or (shr (or (shr rcx.4, #4.1).4, rcx.4).4, #8.1).4, (or (shr rcx.4, #4.1).4, rcx.4).4, rcx.4
171208e0 : or (shr rcx.4, #16.1).4, rcx.4, rcx.4
*/

class recognize_round_down_power2_t : public mcombiner_rule_t {
public:

	virtual const char* name() const override {
		return "Recognize round-down power of 2";
	}

	virtual bool run_combine(mcombine_t* state);
};

extern recognize_round_down_power2_t recognize_round_down_power2;


COMB_RULE_DECL(recognize_overcombined_round_up_pow2, "Recognize monstrous round up pow2 expression");