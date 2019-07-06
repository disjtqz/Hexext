
#include "ctree_defs.hpp"

std::array<std::set<ctree_rule_t>, CMAT_FINAL + 1> g_rules;



bool hexext::execute_ctree_rules(cfunc_t* cfunc, ctree_maturity_t maturity) {

	bool didchange = false;
	ctree_transform_state_t state{ cfunc };
	for (auto&& rule : g_rules[maturity]) {
		didchange |= rule(&state);
	}

	return didchange;
}
#include "late_if_invert.hpp"
#include "../mixins/set_codegen_small.mix"
void hexext::install_ctree_transformation_rule(ctree_maturity_t maturity, ctree_rule_t rule, bool install) {
	auto& ruleset = g_rules[maturity];

	if (install) {
		ruleset.insert(rule);
	}
	else {
		ruleset.erase(rule);
	}
}

static ctree_rule_t g_allrules[] = {
	perform_late_if_inversion
};

void toggle_ctree_rules(bool install) {
	
	for (auto&& rule : g_allrules) {
		hexext::install_ctree_transformation_rule(CMAT_TRANS2, rule, install);
	}
}
#include "../mixins/revert_codegen.mix"