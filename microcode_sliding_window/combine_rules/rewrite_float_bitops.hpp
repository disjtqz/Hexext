#pragma once

class detect_bitwise_negate_floatop_t : public mcombiner_rule_t {
public:
	virtual const char* name() const override {
		return "Transform bitwise negation of floating";
	}
	virtual bool run_combine(mcombine_t* state);
};

class locate_abs_value_floatpath_t : public mcombiner_rule_t {
public:
	virtual const char* name() const override {
		return "Combine ABS float mask when float path taken";
	}
	virtual bool run_combine(mcombine_t* state);
};

extern detect_bitwise_negate_floatop_t detect_bitwise_negate_floatop;
extern locate_abs_value_floatpath_t locate_abs_value_floatpath;