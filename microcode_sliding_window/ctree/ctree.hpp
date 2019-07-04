#pragma once

struct ctree_transform_state_t {
	cfunc_t* m_cfunc;
	
	cfunc_t* cfunc() {
		return m_cfunc;
	}
};
using ctree_rule_t = bool (*)(ctree_transform_state_t*);
namespace hexext {
	void install_ctree_transformation_rule(ctree_maturity_t maturity, ctree_rule_t rule, bool install);

	bool execute_ctree_rules(cfunc_t* cfunc, ctree_maturity_t maturity);


}

void toggle_ctree_rules(bool install);