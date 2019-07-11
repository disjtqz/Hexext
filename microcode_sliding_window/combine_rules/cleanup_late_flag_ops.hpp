#pragma once

COMB_RULE_DECL(sift_down_flagcomps, "Sift down flag-setting comparisons");

COMB_RULE_DECL(interblock_jcc_deps_combiner, "Interblock JCC movement");

COMB_RULE_DECL(merge_shortcircuit_nosideeffects, "Merge blocks implementing short circuit comparisons with no side effects");

COMB_RULE_DECL(distribute_constant_sub_in_const_comp, "Distribute constant sub in comparison with other constant");