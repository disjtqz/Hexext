#include "ctree_defs.hpp"
#include "late_if_invert.hpp"
/*
	from hexrays_sample3
*/
static void do_invert_if(cinsn_t* i) //lint !e818 could be declared as const*
{
	QASSERT(30198, i->op == cit_if);
	cif_t & cif = *i->cif;
	// create an inverted condition and swap it with the if-condition
	cexpr_t * notcond = lnot(new cexpr_t(cif.expr));
	notcond->swap(cif.expr);
	delete notcond;
	// swap if branches
	qswap(cif.ielse, cif.ithen);
}




struct block_weight_t : public ctree_visitor_t
{
	uint32_t m_weight;

	block_weight_t()
		: ctree_visitor_t(CV_FAST | CV_INSNS), m_weight(0u){}

	virtual int idaapi visit_insn(cinsn_t* i) override
	{
		
		++m_weight;
		return 0;
	}
};


static uint32_t generate_block_weight(citem_t* insn) {
	block_weight_t weight{};
	weight.apply_to(insn, nullptr);
	return weight.m_weight;
}

struct ida_local inverter_t : public ctree_visitor_t
{
	std::vector<cinsn_t*> m_targets;
	inverter_t()
		: ctree_visitor_t(CV_FAST | CV_INSNS) {
		m_targets.reserve(128);
	
	}

	int idaapi visit_insn(cinsn_t* i)
	{
		/*if (i->op == cit_if && i->ea == ea)
		{
			found = i;
			return 1; // stop enumeration
		}
		return 0;*/

		if (i->op != cit_if)
			return 0;

		auto cif = i->cif;

		if (cif->ithen && cif->ielse) {

			auto thenweight = generate_block_weight(cif->ithen);

			auto elseweight = generate_block_weight(cif->ielse);

			if (elseweight < thenweight) {
				m_targets.push_back(i);
			}

		}
		return 0;
	}
};

struct invert_retthen_t : public ctree_visitor_t {
	bool m_did_invert_any;
	bool m_is_void;
	cinsn_t* m_body;
	invert_retthen_t(bool is_void, cinsn_t* body) : ctree_visitor_t(CV_FAST | CV_INSNS) , m_did_invert_any(false), m_is_void(is_void), m_body(body){

	}

	virtual int idaapi visit_insn(cinsn_t* i) override;
};

int idaapi invert_retthen_t::visit_insn(cinsn_t* i) {

	if (i->op != cit_block)
		return 0;

	auto blk = i->cblock;
	if (!m_is_void || i != m_body) {
		if (blk->size() < 2)
			return 0;

		if (blk->back().op != cit_return)
			return 0;
	}

	

	auto end = blk->end();
	end--;
	if(!m_is_void || i != m_body)
		end--;
	auto hopefully_if = end;



	auto hopif = hopefully_if->op;

	if (hopif != cit_if)
		return 0;

	if (hopefully_if->cif->ielse)
		return 0;


	if (hopefully_if->cif->ithen->op == cit_return || (hopefully_if->cif->ithen->op == cit_block && hopefully_if->cif->ithen->cblock->back().op == cit_return))
		return 0;
	

	cinsn_t* clone_ret;

	if (((citem_t*)hopefully_if->cif->ithen)->contains_label()) {
		return 0;
	}
	
	if(!m_is_void)
		clone_ret = new cinsn_t(blk->back());
	else {
		creturn_t* cret = new creturn_t();

		clone_ret = new cinsn_t();
		clone_ret->creturn = cret;
		clone_ret->op = cit_return;

	}
	


	
	cinsn_t* newblock = new_block();
	newblock->cblock->push_back(*clone_ret);

	delete clone_ret;
	clone_ret = newblock;
	cexpr_t* notcond = lnot(new cexpr_t(hopefully_if->cif->expr));
	notcond->swap(hopefully_if->cif->expr);

	delete notcond;
	hopefully_if->cif->ithen->swap(*clone_ret);

	/*
	for (auto&& insn : *clone_ret->cblock) {


		blk->insert(hopefully_if, insn);
	}*/

	cinsn_t* new_ret = new cinsn_t(blk->back());

	if(!m_is_void || i != m_body)
	blk->pop_back();

	for (auto&& insn : *clone_ret->cblock)
		blk->push_back(insn);
	if(!m_is_void || i != m_body)
		blk->push_back(*new_ret);

	delete clone_ret;
	delete new_ret;
	m_did_invert_any = true;
	return 1;

}

static bool do_ret_swap(cfunc_t* func) {
	bool did_it = false;
	bool did_any = false;
	do {
		bool is_void;
		tinfo_t typ{};
		if (!func->get_func_type(&typ)) {
			is_void = false;
		}
		else {
			is_void = typ.get_rettype().is_void();
		}
		invert_retthen_t inverter{ is_void , &func->body};

		inverter.apply_to(&func->body, nullptr);
		did_it = inverter.m_did_invert_any;
		did_any |= did_it;
	} while (did_it);
	return did_any;
}

bool perform_late_if_inversion(ctree_transform_state_t* cfunc) {
	inverter_t inverter{};
	do_ret_swap(cfunc->cfunc());
	inverter.apply_to(&cfunc->cfunc()->body, nullptr);

	if (inverter.m_targets.size() == 0)
		return false;

	for (auto&& targ : inverter.m_targets) {
		do_invert_if(targ);
	}


	return true;



}