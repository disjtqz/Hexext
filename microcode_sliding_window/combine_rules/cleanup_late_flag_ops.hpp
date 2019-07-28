#pragma once

COMB_RULE_DECL(sift_down_flagcomps, "Sift down flag-setting comparisons");

COMB_RULE_DECL(interblock_jcc_deps_combiner, "Interblock JCC movement");

COMB_RULE_DECL(merge_shortcircuit_nosideeffects, "Merge blocks implementing short circuit comparisons with no side effects");

COMB_RULE_DECL(distribute_constant_sub_in_const_comp, "Distribute constant sub in comparison with other constant");

COMB_RULE_DECL(replace_boolean_flow_with_boolean_logic, "Rewrite x= 0 if(expr) x = 1 to simply x = expr");

COMB_RULE_DECL(merge_short_circuit_or_with_no_side_effects, "Merge control flow of short-circuit or operations if there are no side effects");

COMB_RULE_DECL(merge_multi_setz_chain_interval, "Merge a series of unconditional comparisons that form an interval to a range comparison");

COMB_RULE_DECL(interblock_flagop_merger, "Hoist up a blocks operations if they only set flags that are also defed in the sole predecessor")

COMB_RULE_DECL(x_minus_y_lt_zero_cmp, "Transform sign substraction compare to less than signed");