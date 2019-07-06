#pragma once
/*
	Adding new roles is very easy. The majority of the actual logic is taken care of automatically

	first you add your role to the exrole_exmacro.hpp macro header file.

	Then you add a header in the role names subdirectory. The name of the header should match the name of the role but it does not have to.

	Then in your role name header file you define every name that could match the role.
	You place these names in the ROLE macro, after the name of the role you defined in the exrole_exmacro header file. 
	Then you simply add an include of your
	names header file into the role_name/allrolenames.hpp header file. 
	Be sure to place the ROLE_SEP macro between your header and other headers. Do not place ROLE_SEP after the last header


	At this point the decompiler will be aware of your new role and will tag function intrinsics that match any of the names that 
	you defined in your names file automatically for you. 
	Since the roles are not modified across optimization passes and are 
	always maintained by the decompiler, even when copying instructions or operands, 
	you will be able to check for your new role in any optimization rules that you define. 
	Pretty fuckin' easy right
*/

/*
	This is one of my favorite little tricks that I've implemented this plug-in. 
	It seems that the role of field is maintained throughout all transformation passes as long as the call is 
	never eliminated by the optimizer. This lets us tag function calls with custom information as the 
	decompiler will not object to a role that it does not recognize. We can use this field to do a variety of things,
	I have a lot of ideas about this, but primarily we can use it to implement custom roles 
	that allow us to match up intrinsic function calls without having to do string comparisons.
*/
enum class exrole_t : uint8_t {
	/*
		Include our X macro header that will define all of the function roles automatically for us
	*/
#define FCLASS(name)		name,
#include "exrole_exmacro.hpp"

#undef FCLASS
	none
};
/*
	This iterates over all instructions that call intrinsic functions and tags them with their proper extended roles.
*/
void tag_helpers_with_exrole(mbl_array_t* mba);

/*
	Returns the function call instruction intrinsic role if it is a call instruction. Otherwise it will return the none role.
*/
exrole_t get_instruction_exrole(minsn_t* insn);

struct exrole_call_binary_t {
	mop_t* first;
	mop_t* second;
};

/*
	Returns the two operands of a extended roll call if it is a call to an extended roll.
	Otherwise returns two null operands
*/
static inline exrole_call_binary_t extract_binary_exrole(minsn_t* insn, exrole_t role) {

	exrole_t r = get_instruction_exrole(insn);
	
	if (r != role) {
		return { nullptr, nullptr };
	}
	cs_assert(insn->d.f->args.size() == 2);

	return { &insn->d.f->args[0], &insn->d.f->args[1] };

}
/*
	finds the extended role for a call to a helper function and tags the instruction with the corresponding role
*/
bool find_and_tag_helper_exrole(minsn_t* inner);