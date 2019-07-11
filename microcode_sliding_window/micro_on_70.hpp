#pragma once
#include "hexdefs.hpp"
#include "cs_core.hpp"
template<typename T>
struct qvector_raw_t {
	T* array;
	size_t n, alloc;
};
constexpr int NOSIZE = -1; ///< wrong or unexisting operand size

/*
	These registers should be the same for all architectures. 
*/
const mreg_t mr_none = mreg_t(-1);
const mreg_t mr_cf = mreg_t(0);      // carry bit
const mreg_t mr_zf = mreg_t(1);      // zero bit
const mreg_t mr_sf = mreg_t(2);      // sign bit
const mreg_t mr_of = mreg_t(3);      // overflow bit
const mreg_t mr_pf = mreg_t(4);      // parity bit
const int    cc_count = mr_pf - mr_cf + 1; // number of condition code registers
const mreg_t mr_cc = mreg_t(5);       // synthetic condition code, used internally
const mreg_t mr_first = mreg_t(8);       // the first processor specific register


enum mcode_t
{
#define X(name, code)	name = code,
#include "mcodexmacro.h"
#undef X
	m_max
};


template <class T>
struct ivl_tpl  // an interval
{
public:
	// forbid the default constructor
	ivl_tpl(void) {}

	T off;
	T size;
	ivl_tpl(T _off, T _size) : off(_off), size(_size) {}
	bool valid() const { return last() >= off; }
	T end() const { return off + size; }
	T last() const { return off + size - 1; }
};
template <class Ivl, class T>
class ivlset_tpl // set of intervals
{
public:
	typedef qvector<Ivl> bag_t;
	union {
		bag_t bag;
		qvector_raw_t<Ivl> bag_;
	};
	// we do not store the empty intervals in bag so size == 0 denotes
	// MAX_VALUE<T>+1, e.g. 0x100000000 for uint32
	static bool ivl_all_values(const Ivl &ivl) { return ivl.off == 0 && ivl.size == 0; }


	ivlset_tpl(void) : bag() {}
	ivlset_tpl(const Ivl& ivl) { if (ivl.valid()) bag.push_back(ivl); }

	~ivlset_tpl() {
		bag.~qvector<Ivl>();
	}

	DEFINE_MEMORY_ALLOCATION_FUNCS()

		void swap(ivlset_tpl& r) { bag.swap(r.bag); }
	const Ivl& getivl(int idx) const { return bag[idx]; }
	const Ivl& lastivl(void) const { return bag.back(); }
	size_t nivls(void) const { return bag.size(); }
	bool empty(void) const { return bag.empty(); }
	void clear(void) { bag.clear(); }
	void qclear(void) { bag.qclear(); }
	bool all_values() const { return nivls() == 1 && ivl_all_values(bag[0]); }
	void set_all_values() { clear(); bag.push_back(Ivl(0, 0)); }
	bool single_value(T v) const { return nivls() == 1 && bag[0].off == v && bag[0].size == 1; }

	bool operator==(const Ivl& v) const { return nivls() == 1 && bag[0] == v; }
	bool operator!=(const Ivl& v) const { return !(*this == v); }

	typedef typename bag_t::iterator iterator;
	typedef typename bag_t::const_iterator const_iterator;
	const_iterator begin(void) const { return bag.begin(); }
	const_iterator end(void)   const { return bag.end(); }
	iterator begin(void) { return bag.begin(); }
	iterator end(void) { return bag.end(); }

};
//using ivl_t = ivl_tpl<uval_t>;

class ivl_t : public ivl_tpl<uval_t> {
public:
	typedef ivl_tpl<uval_t> inherited;
	ivl_t() {}
	ivl_t(uval_t _off, uval_t _size) : inherited(_off, _size) {}
	ivl_t(sval_t _off, uval_t _size) : inherited((uval_t)_off, _size) {}
	bool empty(void) const { return size == 0; }
	void clear(void) { size = 0; }
	void print(qstring* vout) const;
	const char* hexapi dstr(void) const;

	bool extend_to_cover(const ivl_t& r) // extend interval to cover 'r'
	{
		uval_t new_end = end();
		bool changed = false;
		if (off > r.off)
		{
			off = r.off;
			changed = true;
		}
		if (new_end < r.end())
		{
			new_end = r.end();
			changed = true;
		}
		if (changed)
			size = new_end - off;
		return changed;
	}
	void intersect(const ivl_t& r)
	{
		uval_t new_off = qmax(off, r.off);
		uval_t new_end = end();
		if (new_end > r.end())
			new_end = r.end();
		if (new_off < new_end)
		{
			off = new_off;
			size = new_end - off;
		}
		else
		{
			size = 0;
		}
	}

	// do *this and ivl overlap?
	bool overlap(const ivl_t& ivl) const
	{
		return interval::overlap(off, size, ivl.off, ivl.size);
	}
	// does *this include ivl?
	bool includes(const ivl_t& ivl) const
	{
		return interval::includes(off, size, ivl.off, ivl.size);
	}
	// does *this contain off2?
	bool contains(uval_t off2) const
	{
		return interval::contains(off, size, off2);
	}
};
//using ivlset_t = ivlset_tpl<ivl_t, uval_t>;
class ivlset_t : public ivlset_tpl<ivl_t, uval_t> {
public:
	ivlset_t() : ivlset_tpl() {}
	ivlset_t(const ivlset_t& other)  {
		copy(&other);
	}
	bool add(ivl_t* ivl);
	bool add(ivl_t& ivl) {
		return add(&ivl);
	}

	bool has(ivl_t& ivl);
	void add(ivlset_t& other);
	bool  add_(ivlset_t* ivs);
	bool has_common_( const ivlset_t* ivs);
	int  compare(ivlset_t* ivs);
	ivlset_t* copy(const ivlset_t* rhs);

	


	DEFINE_MEMORY_ALLOCATION_FUNCS()
};
using mbitmap_t = int;


static ivl_t allmem{ (uval_t)0, BADADDR };

class bitset_t {
public:
	mbitmap_t *bitmap;    ///< pointer to bitmap
	size_t high;          ///< highest bit+1 (multiply of bitset_width)
	DECLARE_COMPARISONS(bitset_t);
	DEFINE_MEMORY_ALLOCATION_FUNCS()
		bitset_t(void) : bitmap(NULL), high(0) {}
	hexapi bitset_t(const bitset_t& m) : bitset_t(&m) {}          // copy constructor
	hexapi bitset_t(const bitset_t* m);



	~bitset_t(void)
	{
		qfree(bitmap);
		bitmap = NULL;
	}
	void swap(bitset_t& r)
	{
		std::swap(bitmap, r.bitmap);
		std::swap(high, r.high);
	}

	bool hexapi empty(void) const;
	void resize(int a2);
	void hexapi fill_with_ones(int maxbit);
	bool hexapi has(int bit) const;
	bool hexapi has_all(int bit, int width) const;
	bool hexapi has_any(int bit, int width) const; // test presence of bits
	bool hexapi has_common(const bitset_t* ml) const;// has common elements?
	bool hexapi has_common(const bitset_t& ml) const {
		return has_common(&ml);
	}
	bool hexapi add(int bit);                    // add a bit
	bool hexapi add_(int bit, int width);         // add bits
	bool  add__( const bitset_t* ml);
	bool add(const bitset_t& ml) {
		return add__(&ml);

	}
	bool add(int bit, int width) {
		return add_(bit, width);
	}
	bool  sub_17195690();
	bool hexapi sub(int bit);                    // delete a bit
	bool hexapi sub_(int bit, int width);         // delete bits

	bool hexapi sub(int bit, int width) {
		return sub_(bit, width);
	}

	bool hexapi sub__(const bitset_t* ml);         // delete another bitset
	bool hexapi sub(const bitset_t& ml) {
		return sub__(&ml);
	}

	bool hexapi cut_at(int maxbit);              // delete bits >= maxbit
	int hexapi count(void) const;         // number of set bits
	int hexapi count_(int bit) const;      // get number set bits starting from 'bit'
	int hexapi count(int bit) const {
		return count_(bit);
	}
	int hexapi last(void) const;          // get the number of the last bit (-1-no bits)
	bool hexapi intersect(const bitset_t* ml);    // intersect sets. returns true if changed
	bool hexapi intersect(const bitset_t& ml) {
		return intersect(&ml);
	}
	bool hexapi is_subset_of(const bitset_t* ml) const; // is subset of?
	bool hexapi is_subset_of(const bitset_t& ml) const {
		return is_subset_of(&ml);
	}
	bool includes(const bitset_t& ml) const { return ml.is_subset_of(*this); }

	int compare_(const bitset_t* rhs) const;
	void copy(const bitset_t* other);
	
	inline void clear() {
		unsigned isize = (high / 32) + ((high % 32) != 0);

		memset(bitmap, 0, isize * sizeof(bitmap[0]));
	}

	bitset_t& operator=(const bitset_t& m) { // assignment operator
		copy(&m);
		return *this;
	}
#ifndef SWIG
	class iterator
	{
		friend class bitset_t;
		int i;
		iterator(int n) : i(n) {}
	public:
		bool operator==(const iterator& n) const { return i == n.i; }
		bool operator!=(const iterator& n) const { return i != n.i; }
		int operator*(void) const { return i; }
	};
	typedef iterator const_iterator;
	iterator itat(int n) const { return iterator(goup(n)); }
	iterator begin(void) const { return itat(0); }
	iterator end(void)   const { return iterator(high); }
	int front(void)      const { return *begin(); }
	int back(void)       const { return *end(); }
	void inc(iterator& p, int n = 1) const { p.i = goup(p.i + n); }
#endif
private:
	int hexapi goup(int reg) const;
};



DECLARE_TYPE_AS_MOVABLE(bitset_t);
using rlist_t = bitset_t;

struct mlist_t {

	rlist_t reg;
	ivlset_t mem;

};


using mopt_t = uint8_t;
static constexpr mopt_t mop_z = 0,  ///< none
mop_r = 1,  ///< register (they exist until MMAT_LVARS)
mop_n = 2,  ///< immediate number constant
mop_str = 3,  ///< immediate string constant
mop_d = 4,  ///< result of another instruction
mop_S = 5,  ///< local stack variable (they exist until MMAT_LVARS)
mop_v = 6,  ///< global variable
mop_b = 7,  ///< micro basic block (mblock_t)
mop_f = 8,  ///< list of arguments
mop_l = 9,  ///< local variable
mop_a = 10, ///< mop_addr_t: address of operand (mop_l, mop_v, mop_S, mop_r)
mop_h = 11, ///< helper function
mop_c = 12, ///< mcases
mop_fn = 13, ///< floating point constant
mop_p = 14, ///< operand pair
mop_sc = 15, ///< scattered
mop_last = 16;

struct mnumber_t : public operand_locator_t {
	uint64 value;
	uint64_t org_value;
	mnumber_t(uint64 v, ea_t _ea = BADADDR, int n = 0)
		: operand_locator_t(_ea, n), value(v), org_value(v) {}
	DEFINE_MEMORY_ALLOCATION_FUNCS()
		DECLARE_COMPARISONS(mnumber_t)
	{
		if (value < r.value)
			return -1;
		if (value > r.value)
			return -1;
		return 0;
	}
	// always use this function instead of manually modifying the 'value' field
	void update_value(uint64 val64)
	{
		value = val64;
		org_value = val64;
	}
};


struct mcases_t                  // #cases
{
	casevec_t values;             ///< expression values for each target
	intvec_t targets;             ///< target block numbers
	DEFINE_MEMORY_ALLOCATION_FUNCS();
};

struct lvar_ref_t
{
	mbl_array_t* const mba;
	int off;
	int idx;
	DEFINE_MEMORY_ALLOCATION_FUNCS();
	lvar_ref_t(mbl_array_t* mb, int of, int id) : mba(mb), off(of), idx(id) {}
	 
};

struct stkvar_ref_t
{
	mbl_array_t* const mba;
	sval_t off;           // stack offset (from the stack bottom)
	DEFINE_MEMORY_ALLOCATION_FUNCS();

	stkvar_ref_t(mbl_array_t* mb, sval_t of) : mba(mb), off(of) {

	}
};

struct scif_t
{
	DEFINE_MEMORY_ALLOCATION_FUNCS();
	vdloc_t baseclass_0;
	mbl_array_t* mba;
	qstring name;
	tinfo_t type;
};

struct minsn_bin_descend_t {
	struct minsn_t* insn;
	struct mop_t* op1;
	struct mop_t* op2;
};

struct minsn_unary_descend_t {
	struct minsn_t* insn;
	struct mop_t* op1;
};


class mop_t {
public:

	inline mop_t(mop_t&& other) noexcept {
		t = other.t;
		size = other.size;
		oprops = other.oprops;
		valnum = other.valnum;
		d = other.d;
		other.t = mop_z;
		other.size = -1;
	}

	inline mop_t& operator =(mop_t&& other) noexcept {
		erase();
		t = other.t;
		size = other.size;
		oprops = other.oprops;
		valnum = other.valnum;
		d = other.d;
		other.t = mop_z;
		other.size = -1;
		return *this;
	}

	mopt_t optype() const {
		cs_assume_m(t < mop_last);
		return t;
	}
	bool is_constant(uint64_t* out, bool is_signed) const;

	bool is_equal_to( uint64_t value, bool is_signed) const
	{
		uint64_t v; // [rsp+48h] [rbp+20h]

		return is_constant( &v, is_signed) && v == value;
	}

	DEFINE_MEMORY_ALLOCATION_FUNCS();

	void hexapi copy(const mop_t& rop);
	mop_t& hexapi assign(const mop_t& rop) {
		erase();
		copy(rop);
		return *this;
	}


	void assign_insn(minsn_t* ins, unsigned size) {
		d = ins;
		this->size = size;
		t = mop_d;
	}

	void steal_from(mop_t* out) {

		t = out->t;
		d = out->d;
		size = out->size;
		oprops = out->oprops;
		valnum = out->valnum;
		
		out->t = mop_z;
	}


	inline minsn_bin_descend_t
		descend_to_binary_insn(mcode_t opcode);

	inline minsn_unary_descend_t
		descend_to_unary_insn(mcode_t opcode);

	inline minsn_unary_descend_t
		descend_to_unary_insn(mcode_t opcode, mopt_t optype);



	inline minsn_bin_descend_t
		descend_to_binary_insn(mcode_t opcode, mopt_t firstop_type);

	inline minsn_bin_descend_t
		descend_to_binary_insn(mcode_t opcode, bool (*firstop_type)(mcode_t));
	inline minsn_bin_descend_t
		descend_to_binary_insn(mcode_t opcode, mopt_t firstop_type, mopt_t secondop_type);

	bool lvalue_equal_ignore_size(mop_t* mop) {
		if (!is_lvalue())
			return false;

		if (t != mop->t)
			return false;

		if (t == mop_r)
			return r == mop->r;
		else if (t == mop_S) {
			return s->off == mop->s->off;
		}
		else if (t == mop_v) {
			return g == mop->g;
		}
		else if (t == mop_l) {
			return l->idx == mop->l->idx;
		}
		else
			return false;
	}

	bool lvalue_equal(mop_t* mop) {
		if (!is_lvalue())
			return false;

		if (t != mop->t)
			return false;

		if (size != mop->size)
			return false;
		if (t == mop_r)
			return r == mop->r;
		else if (t == mop_S) {
			return s->off == mop->s->off;
		}
		else if (t == mop_v) {
			return g == mop->g;
		}
		else if (t == mop_l) {
			return l->idx == mop->l->idx;
		}
		else
			return false;
	}
	void hexapi swap(mop_t& rop);
	void hexapi erase(void);

	mop_t(void) { zero(); }
	mop_t(const mop_t& rop) { copy(rop); }
	mop_t(mreg_t _r, int _s) : t(mop_r), oprops(0), valnum(0), size(_s), r(_r) {}
	mop_t& operator=(const mop_t& rop) { return assign(rop); }
	~mop_t(void)
	{
		erase();
	}
	void zero(void) { t = mop_z; oprops = 0; valnum = 0; size = NOSIZE; nnn = NULL; }

	void erase_but_keep_size(void) { int s2 = size; erase(); size = s2; }
	/// Create a block reference operand without erasing previous data.
/// \param blknum block number
/// Note: this function does not erase the previous contents of the operand;
///       call erase() if necessary
	void _make_blkref(int blknum)
	{
		t = mop_b;
		b = blknum;
	}
	/// Create a global variable operand.
	void make_blkref(int blknum) { erase(); _make_blkref(blknum); }

	/// Create a helper operand.
	/// A helper operand usually keeps a built-in function name like "va_start"
	/// It is essentially just an arbitrary identifier without any additional info.
	void  make_helper(const char* name) {
		erase();
		t = mop_h;
		helper = qstrdup(name);
	}
	bool is_lvalue() const {
		return t == mop_r || t == mop_S || t == mop_v || t == mop_l;
	}

	/// Create a constant string operand.
	void _make_strlit(const char* str)
	{
		t = mop_str;
		cstr = ::qstrdup(str);
	}
	void _make_strlit(qstring* str) // str is consumed
	{
		t = mop_str;
		cstr = str->extract();
	}

	/// Create a call info operand without erasing previous data.
	/// \param fi callinfo
	/// Note: this function does not erase the previous contents of the operand;
	///       call erase() if necessary
	void _make_callinfo(struct mcallinfo_t* fi)
	{
		t = mop_f;
		f = fi;
	}

	/// Create a 'switch cases' operand without erasing previous data.
	/// Note: this function does not erase the previous contents of the operand;
	///       call erase() if necessary
	void _make_cases(mcases_t* _cases)
	{
		t = mop_c;
		c = _cases;
	}

	/// Create a pair operand without erasing previous data.
	/// Note: this function does not erase the previous contents of the operand;
	///       call erase() if necessary
	void _make_pair(struct mop_pair_t* _pair)
	{
		t = mop_p;
		pair = _pair;
	}

	/// Create a register operand without erasing previous data.
	/// \param reg  micro register number
	/// Note: this function does not erase the previous contents of the operand;
	///       call erase() if necessary
	void _make_reg(mreg_t reg)
	{
		t = mop_r;
		r = reg;
	}
	void _make_reg(mreg_t reg, int _size)
	{
		t = mop_r;
		r = reg;
		size = _size;
	}
	/// Create a register operand.
	void make_reg(mreg_t reg) { erase(); _make_reg(reg); }
	void make_reg(mreg_t reg, int _size) { erase(); _make_reg(reg, _size); }

	/// Create a local variable operand.
	/// \param mba pointer to microcode
	/// \param idx index into mba->vars
	/// \param off offset from the beginning of the variable
	/// Note: this function does not erase the previous contents of the operand;
	///       call erase() if necessary
	void _make_lvar(mbl_array_t* mba, int idx, sval_t off = 0)
	{
		t = mop_l;
		l = new lvar_ref_t(mba, idx, off);
	}

	/// Create a global variable operand without erasing previous data.
	/// \param ea  address of the variable
	/// Note: this function does not erase the previous contents of the operand;
	///       call erase() if necessary
	void _make_gvar(ea_t ea)
	{
		t = mop_v;
		g = ea;
	}
	/// Create a global variable operand.
	void make_gvar(ea_t ea) { erase(); _make_gvar(ea); }

	/// Create a stack variable operand.
	/// \param mba pointer to microcode
	/// \param off decompiler stkoff
	/// Note: this function does not erase the previous contents of the operand;
	///       call erase() if necessary
	void _make_stkvar(mbl_array_t* mba, sval_t off)
	{
		t = mop_S;
		s = new stkvar_ref_t(mba, off);
	}

	void make_number(uint64_t value, unsigned size, ea_t ea = BADADDR, unsigned n = 0) {
		t = mop_n;
		this->size = size;
		nnn = new mnumber_t(value, ea, n);
	}

	inline void make_unary_insn_larg(mcode_t op, mop_t& larg, unsigned size, ea_t addr = BADADDR);

	void make_reg(mreg_t reg, unsigned size) {
		t = mop_r;
		this->size = size;
		r = reg;
	}

	mopt_t t;
	uint8_t oprops;
#define OPROP_IMPDONE 0x01 ///< imported operand (a pointer) has been dereferenced
#define OPROP_UDT     0x02 ///< a struct or union
#define OPROP_FLOAT   0x04 ///< possibly floating value
#define OPROP_CCFLAGS 0x08 ///< condition codes register value
#define OPROP_UDEFVAL 0x10 ///< uses undefined value


	void set_impptr_done(void) { oprops |= OPROP_IMPDONE; }
	void set_udt(void) { oprops |= OPROP_UDT; }
	void set_undef_val(void) { oprops |= OPROP_UDEFVAL; }
	bool is_impptr_done(void) const { return (oprops & OPROP_IMPDONE) != 0; }
	bool is_udt(void)         const { return (oprops & OPROP_UDT) != 0; }
	bool probably_floating(void) const { return (oprops & OPROP_FLOAT) != 0; }
	bool is_ccflags(void)     const { return (oprops & OPROP_CCFLAGS) != 0; }
	bool is_undef_val(void)   const { return (oprops & OPROP_UDEFVAL) != 0; }

	/// Value number.
/// Zero means unknown.
/// Operands with the same value number are equal.
	uint16 valnum;

	/// Operand size.
	/// Usually it is 1,2,4,8 or NOSIZE but for UDTs other sizes are permitted
	int size;

	/// The following union holds additional details about the operand.
	/// Depending on the operand type different kinds of info are stored.
	/// You should access these fields only after verifying the operand type.
	/// All pointers are owned by the operand and are freed by its destructor.
	union
	{
		mreg_t r;           // mop_r   register number
		mnumber_t *nnn;     // mop_n   immediate value
		minsn_t *d;         // mop_d   result (destination) of another instruction
		struct stkvar_ref_t *s;    // mop_S   stack variable
		ea_t g;             // mop_v   global variable (its linear address)
		int b;              // mop_b   block number (used in jmp,call instructions)
		struct mcallinfo_t *f;     // mop_f   function call information
		struct lvar_ref_t *l;      // mop_l   local variable
		struct mop_addr_t *a;      // mop_a   variable whose address is taken
		char *helper;       // mop_h   helper function name
		char *cstr;         // mop_str string constant
		mcases_t *c;        // mop_c   cases
		fnumber_t *fpc;     // mop_fn  floating point constant
		struct mop_pair_t *pair;   // mop_p   operand pair
		struct scif_t *scif;       // mop_sc  scattered operand info
	};




};
struct mop_pair_t
{
	DEFINE_MEMORY_ALLOCATION_FUNCS();
	mop_t lop;            ///< low operand
	mop_t hop;            ///< high operand
};
struct mop_addr_t :public mop_t {
	DEFINE_MEMORY_ALLOCATION_FUNCS();
	int insize;   // how many bytes of the pointed operand can be read
	int outsize;  // how many bytes of the pointed operand can be written

};


class mcallarg_t : public mop_t // #callarg
{
public:
	DEFINE_MEMORY_ALLOCATION_FUNCS();
	ea_t ea;                      ///< address where the argument was initialized.
								  ///< BADADDR means unknown.
	tinfo_t type;                 ///< formal argument type
	qstring name;                 ///< formal argument name
	argloc_t argloc;              ///< ida argloc
	mcallarg_t() : type() {}

	inline mcallarg_t& operator = (mop_t& other) {
		*reinterpret_cast<mop_t*>(this) = other;
		return *this;
	}
};
using mcallargs_t = qvector<mcallarg_t>;
typedef qvector<minsn_t*> minsnptrs_t;
typedef qvector<mop_t*> mopptrs_t;
typedef qvector<mop_t> mopvec_t;
typedef std::set<minsn_t *> minsn_ptr_set_t;


/// Function roles.
/// They are used to calculate use/def lists and to recognize functions
/// without using string comparisons.
enum funcrole_t
{
	ROLE_UNK,                  ///< unknown function role
	ROLE_EMPTY,                ///< empty, does not do anything (maybe spoils regs)
	ROLE_MEMSET,               ///< memset(void *dst, uchar value, size_t count);
	ROLE_MEMSET32,             ///< memset32(void *dst, uint32 value, size_t count);
	ROLE_MEMSET64,             ///< memset32(void *dst, uint64 value, size_t count);
	ROLE_MEMCPY,               ///< memcpy(void *dst, const void *src, size_t count);
	ROLE_STRCPY,               ///< strcpy(char *dst, const char *src);
	ROLE_STRLEN,               ///< strlen(const char *src);
	ROLE_STRCAT,               ///< strcat(char *dst, const char *src);
	ROLE_TAIL,                 ///< char *tail(const char *str);
	ROLE_BUG,                  ///< BUG() helper macro: never returns, causes exception
	ROLE_ALLOCA,               ///< alloca() function
	ROLE_BSWAP,                ///< bswap() function (any size)
	ROLE_PRESENT,              ///< present() function (used in patterns)
	ROLE_CONTAINING_RECORD,    ///< CONTAINING_RECORD() macro
	ROLE_FASTFAIL,             ///< __fastfail()
	ROLE_READFLAGS,            ///< __readeflags, __readcallersflags
	ROLE_IS_MUL_OK,            ///< is_mul_ok
	ROLE_SATURATED_MUL,        ///< saturated_mul
	ROLE_BITTEST,              ///< [lock] bt
	ROLE_BITTESTANDSET,        ///< [lock] bts
	ROLE_BITTESTANDRESET,      ///< [lock] btr
	ROLE_BITTESTANDCOMPLEMENT, ///< [lock] btc
	ROLE_VA_ARG,               ///< va_arg() macro
	ROLE_VA_COPY,              ///< va_copy() function
	ROLE_VA_START,             ///< va_start() function
	ROLE_VA_END,               ///< va_end() function
	ROLE_ROL,                  ///< rotate left
	ROLE_ROR,                  ///< rotate right
	ROLE_CFSUB3,               ///< carry flag after subtract with carry
	ROLE_OFSUB3,               ///< overflow flag after subtract with carry
	ROLE_ABS,                  ///< integer absolute value
};
struct mcallinfo_t               // #callinfo
{
	/*
	/*

#pragma pack(push, 1)
struct CHANGE_MY_NAME
{
	__int64 field_0;
	int solid_args;
	int call_spd;
	int stkargs_top;
	cm_t cc;
	char pad[3];
	__int64 field_18;
	_QWORD field_20;
	unsigned __int64 field_28;
	__int64 field_30;
	_QWORD field_38;
	unsigned __int64 field_40;
	_DWORD field_48;
	_BYTE gap_4C[4];
	_DWORD field_50;
	_BYTE gap_54[12];
	bitset_t field_60;
	qvector<ivl_t> field_70;
	bitset_t field_88;
	qvector<ivl_t> field_98;
	bitset_t field_B0;
	qvector<ivl_t> field_C0;
	qvector<ivl_t> field_D8;
	bitset_t field_F0;
	qvector<ivl_t> field_100;
	_DWORD field_118;
	int field_11C;
};

*/
	
public:
	DEFINE_MEMORY_ALLOCATION_FUNCS();
	ea_t callee;                  ///< address of the called function, if known
	int solid_args;               ///< number of solid args.
								  ///< there may be variadic args in addtion
	int call_spd;                 ///< sp value at call insn
	int stkargs_top;              ///< first offset past stack arguments
	cm_t cc;                      ///< calling convention
	mcallargs_t args;             ///< call arguments
	mopvec_t retregs;             ///< return register(s) (e.g., AX, AX:DX, etc.)
								  ///< this vector is built from return_regs
	tinfo_t return_type;          ///< type of the returned value
	argloc_t return_argloc;       ///< location of the returned value

	mlist_t return_regs;          ///< list of values returned by the function
	mlist_t spoiled;              ///< list of spoiled locations (includes return_regs)
	mlist_t pass_regs;            ///< passthrough registers: registers that depend on input
								  ///< values (subset of spoiled)
	ivlset_t visible_memory;      ///< what memory is visible to the call?
	mlist_t dead_regs;            ///< registers defined by the function but never used.
								  ///< upon propagation we do the following:
								  ///<   - dead_regs += return_regs
								  ///<   - retregs.clear() since the call is propagated
	int flags;                    ///< combination of \ref FCI_... bits
  /// \defgroup FCI_ Call properties
  //@{
#define FCI_PROP    0x01        ///< call has been propagated
#define FCI_DEAD    0x02        ///< some return registers were determined dead
#define FCI_FINAL   0x04        ///< call type is final, should not be changed
#define FCI_NORET   0x08        ///< call does not return
#define FCI_PURE    0x10        ///< pure function
#define FCI_NOSIDE  0x20        ///< call does not have side effects
#define FCI_SPLOK   0x40        ///< spoiled/visible_memory lists have been
								///< optimized. for some functions we can reduce them
								///< as soon as information about the arguments becomes
								///< available. in order not to try optimize them again
								///< we use this bit.
//@}
	funcrole_t role;              ///< function role
};

struct minsn_t {
	mcode_t opcode;
	int iprops;           ///< combination of \ref IPROP_ bits
	minsn_t *next;        ///< next insn in doubly linked list. check also nexti()
	minsn_t *prev;        ///< prev insn in doubly linked list. check also previ()
	ea_t ea;              ///< instruction address
	mop_t l;              ///< left operand
	mop_t r;              ///< right operand
	mop_t d;              ///< destination operand

	mcode_t op() const {
		cs_assume_m((unsigned)opcode < (unsigned)m_max);
		return opcode;
	}


	inline minsn_t(minsn_t&& other) : l(std::move(other.l)), r(std::move(other.r)), d(std::move(other.d)) {
		opcode = other.opcode;
		iprops = other.iprops;
		next = other.next;
		prev = other.prev;
		ea = other.ea;
		other.opcode = m_nop;
		other.next = nullptr;
		other.prev = nullptr;
		other.ea = BADADDR;

	}

	inline minsn_t& operator =(minsn_t&& other) {
		opcode = other.opcode;
		iprops = other.iprops;
		next = other.next;
		prev = other.prev;
		ea = other.ea;
		l = std::move(other.l);
		r = std::move(other.r);
		d = std::move(other.d);
		other.opcode = m_nop;
		other.next = nullptr;
		other.prev = nullptr;
		other.ea = BADADDR;
		return *this;
	}
	
	bool lr_both_size(unsigned sz) {
		return l.size == sz && r.size == sz;
	}

	void hexapi init(ea_t _ea) {
		this->ea = _ea;
		this->next = nullptr;
		this->prev = nullptr;
		this->opcode =(mcode_t) 0;
		this->iprops = 0;
	}
	void hexapi copy(const minsn_t& m);


	bool either(mopt_t ty) {
		return l.t == ty || r.t == ty;
	}


	mop_t* __fastcall find_num_op(mop_t** other);
	bool is_like_move() const {
		mcode_t v13 = this->opcode;
		return v13 == m_mov ||v13 == m_f2f && this->l.size == this->d.size || (unsigned int)(v13 - m_xds) <= 2;
	}

	mop_t* get_eitherlr(mopt_t ty) {

		if (r.t == ty) {
			return &r;
		}
		else if (l.t == ty) {
			return &l;
		}
		else
			return nullptr;
	}

	std::pair<mop_t*, mop_t*> arrange_by(mopt_t ty) {
		mop_t* second;
		mop_t* first = get_eitherlr(ty, &second);

		return { first, second };
	}


	std::pair<mop_t*, mop_t*> arrange_by_either(mopt_t ty, mopt_t ty2) {
		auto [x, y] = arrange_by(ty);

		if (!x) {
			return arrange_by(ty2);
		}
		else {
			return { x, y };
		}
	}
	std::pair<mop_t*, mop_t*> arrange_by(mopt_t ty, mopt_t ty2) {
		mop_t* second;
		mop_t* first = get_eitherlr(ty, &second);
		if (!second || second->optype() != ty2) {
			return {};
		}
		return { first, second };
	}

	std::pair<mop_t*, mop_t*> arrange_by_insn(mcode_t ty) {
		mop_t* second;
		mop_t* first = get_either_insnop(ty, &second);

		return { first, second };
	}

	std::pair<mop_t*, mop_t*> arrange_by_insn(mcode_t ty, mcode_t ty2) {
		auto [one, two] = arrange_by_insn(ty);
		if (!one || two->t != mop_d || two->d->opcode != ty2)
			return { nullptr, nullptr };

		return { one, two };
	}

	mop_t* get_nonlr(mopt_t ty) {
		if (r.t != ty && r.t != mop_z) {
			return &r;
		}
		else if (l.t != ty && l.t != mop_z) {
			return &l;
		}
		return nullptr;
	}

	bool both(mopt_t ty) {
		return l.t == ty && r.t == ty;
	}

	mop_t* get_eitherlr(mopt_t ty, mop_t** out_non) {

		if (r.t == ty) {
			*out_non = &l;
			return &r;
		}
		else if (l.t == ty) {
			*out_non = &r;
			return &l;
		}
		else {
			*out_non = nullptr;
			return nullptr;
		}
	}
	mop_t* get_either_insnop(mcode_t mcode) {
		if (r.t == mop_d && r.d->opcode == mcode) {
			return &r;
		}
		else if (l.t == mop_d && l.d->opcode == mcode)
			return &l;

		else {
			return nullptr;
		}
	}
	mop_t* get_either_insnop(mcode_t mcode, mop_t** out_non) {
		if (r.t == mop_d && r.d->opcode == mcode) {
			*out_non = &l;
			return &r;
		}
		else if (l.t == mop_d && l.d->opcode == mcode){
			* out_non = &r;
		return &l;
	}
		else {
			*out_non = nullptr;
			return nullptr;
		}
	}

	mop_t* get_either_lvalue(mop_t** out_nonlvalue) {
		if (l.is_lvalue()) {
			*out_nonlvalue = &r;
			return &l;
		}
		else if (r.is_lvalue()) {
			*out_nonlvalue = &l;
			return &r;
		}
		else {
			return nullptr;
		}
	}

	
  /// Constructor
	minsn_t(ea_t _ea) { init(_ea); }
	minsn_t(const minsn_t& m) { next = prev = NULL; copy(m); }
	DEFINE_MEMORY_ALLOCATION_FUNCS();

		/// Assignment operator. It does not copy prev/next fields.
		minsn_t& operator=(const minsn_t& m) { copy(m); return *this; }

	/// Swap two instructions.
	/// The prev/next fields are not modified by this function
	/// because it would corrupt the doubly linked list.
	void hexapi swap(minsn_t* m);
	void swap(minsn_t& m) {
		swap(&m);
	}

	  /// \defgroup IPROP_ instruction property bits
  //@{
  // bits to be used in patterns:
#define IPROP_OPTIONAL  0x0001 ///< optional instruction
#define IPROP_PERSIST   0x0002 ///< persistent insn; they are not destroyed
#define IPROP_WILDMATCH 0x0004 ///< match multiple insns

  // instruction attributes:
#define IPROP_CLNPOP    0x0008 ///< the purpose of the instruction is to clean stack
							   ///< (e.g. "pop ecx" is often used for that)
/*#define IPROP_FPINSN    0x0010 ///< floating point insn
#define IPROP_FARCALL   0x0020 ///< call of a far function using push cs/call sequence
#define IPROP_TAILCALL  0x0040 ///< tail call*/

#define IPROP_FPINSN	64
#define IPROP_ASSERT    0x0080 ///< assertion: usually mov #val, op.
							   ///< assertions are used to help the optimizer.
							   ///< assertions are ignored when generating ctree

  // instruction history:
#define IPROP_SPLIT     0x0700 ///< the instruction has been split:
#define IPROP_SPLIT1    0x0100 ///<   into 1 byte
#define IPROP_SPLIT2    0x0200 ///<   into 2 bytes
#define IPROP_SPLIT4    0x0300 ///<   into 4 bytes
#define IPROP_SPLIT8    0x0400 ///<   into 8 bytes
#define IPROP_COMBINED  0x0800 ///< insn has been modified because of a partial reference
#define IPROP_EXTSTX    0x1000 ///< this is m_ext propagated into m_stx
#define IPROP_IGNLOWSRC 0x2000 ///< low part of the instruction source operand
							   ///< has been created artificially
							   ///< (this bit is used only for 'and x, 80...')
#define IPROP_INV_JX    0x4000 ///< inverted conditional jump
#define IPROP_WAS_NORET 0x8000 ///< was noret icall
#define IPROP_MULTI_MOV 0x10000 ///< the minsn was generated as part of insn that moves multiple registers
								///< (example: STM on ARM may transfer multiple registers)

								///< bits that can be set by plugins:
#define IPROP_DONT_PROP 0x20000 ///< may not propagate
#define IPROP_DONT_COMB 0x40000 ///< may not combine this instruction with others
  //@}

	bool is_fpinsn() const {
		return (iprops & IPROP_FPINSN) != 0;
	}
};

/// Basic block types
enum mblock_type_t
{
	BLT_NONE = 0, ///< unknown block type
	BLT_STOP = 1, ///< stops execution regularly (must be the last block)
	BLT_0WAY = 2, ///< does not have successors (tail is a noret function)
	BLT_1WAY = 3, ///< passes execution to one block (regular or goto block)
	BLT_2WAY = 4, ///< passes execution to two blocks (conditional jump)
	BLT_NWAY = 5, ///< passes execution to many blocks (switch idiom)
	BLT_XTRN = 6, ///< external block (out of function address)
};
#define MBL_PUSH	0x10
#define MBL_PROP   (1 << 10)
#define MBL_INCONST (1<< 0xBu)
#define MBL_FAKE	0x200
#define MBL_LIST	8
#ifdef __EA64__
struct mblock_t
{

	mblock_t();
	size_t hexapi get_reginsn_qty(void) const;
	minsn_t*  remove_from_block(minsn_t* m);

	minsn_t* insert_into_block(minsn_t* nm, minsn_t* om);
	/*
#define MBL_PRIV        0x0001  ///< private block - no instructions except
	///< the specified are accepted (used in patterns)
#define MBL_NONFAKE     0x0000  ///< regular block
#define MBL_FAKE        0x0002  ///< fake block (after a tail call)
#define MBL_GOTO        0x0004  ///< this block is a goto target
#define MBL_TCAL        0x0008  ///< aritifical call block for tail calls
#define MBL_PUSH        0x0010  ///< needs "convert push/pop instructions"
#define MBL_DMT64       0x0020  ///< needs "demote 64bits"
#define MBL_COMB        0x0040  ///< needs "combine" pass
#define MBL_PROP        0x0080  ///< needs 'propagation' pass
#define MBL_DEAD        0x0100  ///< needs "eliminate deads" pass
#define MBL_LIST        0x0200  ///< use/def lists are ready (not dirty)
#define MBL_INCONST     0x0400  ///< inconsistent lists: we are building them
#define MBL_CALL        0x0800  ///< call information has been built
#define MBL_BACKPROP    0x1000  ///< performed backprop_cc
#define MBL_NORET       0x2000  ///< dead end block: doesn't return execution control
#define MBL_DSLOT       0x4000  ///< block for delay slot
#define MBL_VALRANGES   0x8000  ///< should optimize using value ranges*/

#if 0
	void* vtable;
	mblock_t* nextb;
	mblock_t* prevb;
	int flags;
	_BYTE gap_1C[4];
	_QWORD field_20;
	_QWORD field_28;
	_QWORD field_30;
	ea_t start;
	ea_t end;
	minsn_t* head;
	minsn_t* tail;
	_QWORD field_58;
	_DWORD field_60;
	_BYTE gap_64[4];
	_QWORD field_68;
	_QWORD field_70;
	_QWORD field_78;
	_QWORD field_80;
	_DWORD field_88;
	_BYTE gap_8C[4];
	_QWORD field_90;
	_QWORD field_98;
	_QWORD field_A0;
	_QWORD field_A8;
	_DWORD field_B0;
	_BYTE gap_B4[4];
	_QWORD field_B8;
	_QWORD field_C0;
	_QWORD field_C8;
	_QWORD field_D0;
	_DWORD field_D8;
	_BYTE gap_DC[4];
	_QWORD field_E0;
	_QWORD field_E8;
	_QWORD field_F0;
	_QWORD field_F8;
	_DWORD field_100;
	_BYTE gap_104[4];
	_QWORD field_108;
	_QWORD field_110;
	_QWORD field_118;
	_QWORD field_120;
	_DWORD field_128;
	_BYTE gap_12C[4];
	int serial;
	_BYTE gap_134[4];
	mbl_array_t* mba;
	_DWORD field_140;
	_BYTE gap_144[4];
	_QWORD maxbsp;
	_QWORD minbstkref;
	_QWORD minbargref;
	qvector<int> predset;
	qvector<int> succset;

#else
	void* vtable;
	mblock_t* nextb;
	mblock_t* prevb;
	int flags;
	_BYTE gap_1C[4];
	_QWORD field_20;
	_QWORD field_28;
	_QWORD field_30;
	ea_t start;
	ea_t end;
	minsn_t* head;
	minsn_t* tail;
	_QWORD field_58;
	_DWORD field_60;
	_BYTE gap_64[4];
	_QWORD field_68;
	_QWORD field_70;
	_QWORD field_78;
	mlist_t mustbuse;
	mlist_t maybuse;
	mlist_t mustbdef;
	mlist_t maybdef;
	_QWORD field_120;
	_DWORD field_128;
	_BYTE gap_12C[4];
	int serial;
	_BYTE gap_134[4];
	mbl_array_t* mba;
	_DWORD type;
	_BYTE gap_144[4];
	_QWORD maxbsp;
	_QWORD minbstkref;
	_QWORD minbargref;
	qvector<int> predset;
	qvector<int> succset;
#endif
	DEFINE_MEMORY_ALLOCATION_FUNCS()
};
#else
struct mblock_t
{
	mblock_t();
	size_t hexapi get_reginsn_qty(void) const;
	minsn_t* remove_from_block(minsn_t* m);

	minsn_t* insert_into_block(minsn_t* nm, minsn_t* om);
	void* vtable;
	mblock_t* nextb;
	mblock_t* prevb;
	int flags;
	int field_1C;
	_QWORD field_20;
	_QWORD field_28;
	_QWORD field_30;
	_DWORD start;
	_DWORD end;
	minsn_t* head;
	minsn_t* tail;
	_QWORD field_50;
	_DWORD field_58;
	_BYTE gap_5C[4];
	_QWORD field_60;
	_QWORD field_68;
	_QWORD field_70;
	mlist_t mustbuse;
	mlist_t maybuse;
	mlist_t mustbdef;
	mlist_t maybdef;
	_BYTE gap_118[4];
	int field_11C;
	int field_120;
	_BYTE gap_124[4];
	signed int serial;
	_BYTE gap_12C[4];
	mbl_array_t* mba;
	_QWORD maxbsp;
	int minbstkref;
	int minbargref;
	qvector<int> predset;
	qvector<int> succset;
};

#endif
struct mba_ranges_t
{
	func_t *pfn;          ///< function to decompile
	rangevec_t ranges;    ///< empty ? function_mode : snippet mode
	mba_ranges_t(func_t *_pfn = NULL) : pfn(_pfn) {}
	mba_ranges_t(const rangevec_t &r) : pfn(NULL), ranges(r) {}
	ea_t start(void) const { return (ranges.empty() ? *pfn : ranges[0]).start_ea; }
	bool empty(void) const { return pfn == NULL && ranges.empty(); }
	void clear(void) { pfn = NULL; ranges.clear(); }
	bool is_snippet(void) const { return !ranges.empty(); }
	bool range_contains(ea_t ea) const;
	bool is_fragmented(void) const { return ranges.empty() ? pfn->tailqty > 0 : ranges.size() > 1; }
};

enum mba_maturity_t
{
	MMAT_ZERO,         ///< microcode does not exist
	MMAT_GENERATED,    ///< generated microcode
	MMAT_PREOPTIMIZED, ///< preoptimized pass is complete
	MMAT_LOCOPT,       ///< local optimization of each basic block is complete.
					   ///< control flow graph is ready too.
					   MMAT_CALLS,        ///< detected call arguments
					   MMAT_GLBOPT1,      ///< performed the first pass of global optimization
					   MMAT_GLBOPT2,      ///< most global optimization passes are done
					   MMAT_GLBOPT3,      ///< completed all global optimization
					   MMAT_LVARS,        ///< allocated local variables
};
/*

*/
// bits to describe the microcode, set by the decompiler
#define MBA_PRCDEFS  0x00000001 ///< use precise defeas for chain-allocated lvars
#define MBA_NOFUNC   0x00000002 ///< function is not present, addresses might be wrong
#define MBA_PATTERN  0x00000004 ///< microcode pattern, callinfo is present
#define MBA_LOADED   0x00000008 ///< loaded gdl, no instructions (debugging)
#define MBA_RETFP    0x00000010 ///< function returns floating point value
#define MBA_SPLINFO  0x00000020 ///< (final_type ? idb_spoiled : spoiled_regs) is valid
#define MBA_PASSREGS 0x00000040 ///< has mcallinfo_t::pass_regs
#define MBA_THUNK    0x00000080 ///< thunk function
#define MBA_CMNSTK   0x00000100 ///< stkvars+stkargs should be considered as one area

					 // bits to describe analysis stages and requests
#define MBA_PREOPT   0x00000200 ///< preoptimization stage complete
#define MBA_CMBBLK   0x00000400 ///< request to combine blocks
#define MBA_ASRTOK   0x00000800 ///< assertions have been generated
#define MBA_CALLS    0x00001000 ///< callinfo has been built
#define MBA_ASRPROP  0x00002000 ///< assertion have been propagated
#define MBA_SAVRST   0x00004000 ///< save-restore analysis has been performed
#define MBA_RETREF   0x00008000 ///< return type has been refined
#define MBA_GLBOPT   0x00010000 ///< microcode has been optimized globally
#define MBA_OVERVAR  0x00020000 ///< an overlapped variable has been detected
#define MBA_LVARS0   0x00040000 ///< lvar pre-allocation has been performed
#define MBA_LVARS1   0x00080000 ///< lvar real allocation has been performed
#define MBA_DELPAIRS 0x00100000 ///< pairs have been deleted once
#define MBA_CHVARS   0x00200000 ///< can verify chain varnums

					 // bits that can be set by the caller:
#define MBA_SHORT    0x00400000 ///< use short display
#define MBA_COLGDL   0x00800000 ///< display graph after each reduction
#define MBA_INSGDL   0x01000000 ///< display instruction in graphs
#define MBA_NICE     0x02000000 ///< apply transformations to c code
#define MBA_REFINE   0x04000000 ///< may refine return value size
#define MBA_DELASRT  0x08000000 ///< may delete assertions at the end of glbopt
#define MBA_WINGR32  0x10000000 ///< use wingraph32
#define MBA_NUMADDR  0x20000000 ///< display definition addresses for numbers
#define MBA_VALNUM   0x40000000 ///< display value numbers

#define MBA_INITIAL_FLAGS  (MBA_INSGDL|MBA_NICE|MBA_CMBBLK|MBA_REFINE|MBA_DELASRT\
        |MBA_PRCDEFS|MBA_WINGR32|MBA_VALNUM)

#define MBA2_LVARNAMES_OK  0x00000001 // may verify lvar_names?
#define MBA2_LVARS_RENAMED 0x00000002 // accept empty names now?
#define MBA2_OVER_CHAINS   0x00000004 // has overlapped chains?
#define MBA2_VALRNG_DONE   0x00000008 // calculated valranges?
#define MBA2_IS_CTR        0x00000010 // is constructor?
#define MBA2_IS_DTR        0x00000020 // is destructor?

#define MBA2_INITIAL_FLAGS  (MBA2_LVARNAMES_OK|MBA2_LVARS_RENAMED)
#define MBA2_ALL_FLAGS    0x0000003F

#ifdef __EA64__
struct __declspec(align(8)) mbl_array_t
{
	
	signed __int64 something_involving_stack_elements(minsn_t* a2);
	_DWORD flags1;
	_DWORD field_4;
	_QWORD field_8;
	_QWORD field_10;
	_QWORD field_18;
	_QWORD field_20;
	_QWORD field_28;
	_QWORD field_30;
	_DWORD field_38;
	_BYTE field_3C;
	_BYTE gap_3D[3];
	_QWORD field_40;
	_QWORD field_48;
	_QWORD field_50;
	_QWORD field_58;
	_QWORD field_60;
	_QWORD field_68;
	_QWORD field_70;
	_QWORD minstkref;
	_QWORD field_80;
	_QWORD minargref;
	_QWORD field_90;
	_QWORD field_98;
	_QWORD field_A0;
	_QWORD field_A8;
	_QWORD field_B0;
	_QWORD field_B8;
	_QWORD field_C0;
	_QWORD field_C8;
	_QWORD field_D0;
	_QWORD field_D8;
	ivlset_t field_E0;
	_QWORD field_F8;
	_DWORD field_100;
	_BYTE gap_104[4];
	_QWORD field_108;
	_QWORD field_110;
	_QWORD field_118;
	_BYTE field_120;
	_BYTE gap_121[3];
	_DWORD field_124;
	_QWORD field_128;
	_QWORD field_130;
	_QWORD field_138;
	_QWORD field_140;
	_DWORD field_148;
	_BYTE gap_14C[4];
	_QWORD field_150;
	_QWORD field_158;
	_QWORD field_160;
	_DWORD field_168;
	_BYTE gap_16C[4];
	_QWORD field_170;
	_QWORD field_178;
	_QWORD field_180;
	_QWORD field_188;
	lvars_t vars;
	_QWORD field_1A8;
	_QWORD field_1B0;
	_QWORD field_1B8;
	_DWORD field_1C0;
	_DWORD field_1C4;
	_QWORD field_1C8;
	_QWORD field_1D0;
	_QWORD* field_1D8;
	_QWORD field_1E0;
	_QWORD* field_1E8;
	_QWORD field_1F0;
	_QWORD* field_1F8;
	_QWORD field_200;
	_QWORD field_208;
	_QWORD field_210;
	_QWORD field_218;
	_DWORD field_220;
	_BYTE gap_224[4];
	_QWORD field_228;
	_QWORD field_230;
	char** field_238;
	_QWORD field_240;
	_BYTE gap_248[160];
	_QWORD field_2E8;
	_QWORD field_2F0;
	_QWORD field_2F8;
	_QWORD field_300;
	int qty;
	_BYTE gap_30C[4];
	mblock_t* blocks;
	mblock_t** natural;
	struct mbl_graph_t* mba_graph;
	_QWORD field_328;
	_QWORD field_330;
	_QWORD* field_338;
	_QWORD field_340;
	_QWORD* field_348;
	_QWORD field_350;
	_QWORD field_358;
	_QWORD field_360;
	_QWORD field_368;
	_WORD field_370;
	_BYTE gap_372[6];
	_QWORD* field_378;
	_QWORD field_380;
	_QWORD field_388;
	_QWORD field_390;
	_QWORD field_398;
	_QWORD field_3A0;
	_QWORD field_3A8;
	_QWORD field_3B0;
	_QWORD field_3B8;
	_QWORD* field_3C0;
	_QWORD field_3C8;
	_QWORD* field_3D0;
	_QWORD field_3D8;
	_QWORD* field_3E0;
	_QWORD field_3E8;
	_QWORD* field_3F0;
	_QWORD field_3F8;
	ivlset_t field_400;
	_DWORD field_418;
	_BYTE gap_41C[4];
	_QWORD field_420;
	_QWORD field_428;
	_QWORD field_430;
	_QWORD field_438;
	_QWORD field_440;
	_QWORD field_448;
	_QWORD field_450;
	_QWORD field_458;
	_QWORD field_460;
	_QWORD field_468;
	_QWORD field_470;
	_QWORD* field_478;
	_BYTE gap_480[8];
	_QWORD field_488;
	_DWORD field_490;
	_BYTE gap_494[4];
	_QWORD* field_498;
	_QWORD field_4A0;
	_QWORD field_4A8;
	_QWORD field_4B0;
	__int64 field_4B8;
	_QWORD* field_4C0;
	_QWORD field_4C8;
	_QWORD* field_4D0;
	_QWORD field_4D8;
	_QWORD* field_4E0;
	_QWORD field_4E8;
	void regenerate_natural();

	mblock_t* create_new_block_for_preopt(mblock_t* after);


};
#else
struct mbl_array_t
{
	void regenerate_natural();

	mblock_t* create_new_block_for_preopt(mblock_t* after);

	signed __int64 something_involving_stack_elements(minsn_t* a2);
	int flags1;
	int field_4;
	int field_8;
	int field_C;
	int gap_10;
	int field_14;
	__int64 field_18;
	__int64 field_20;
	__int64 field_28;
	int field_30;
	char cc;
	char field_35;
	__int64 field_38;
	__int64 field_40;
	__int64 field_48;
	__int64 field_50;
	_DWORD minstkref;
	int gap_5C;
	__int64 minargref;
	__int64 field_68;
	int field_70;
	__int64 field_78;
	__int64 field_80;
	__int64 field_88;
	__int64 field_90;
	__int64 field_98;
	__int64 field_A0;
	__int64 field_A8;
	__int64 field_B0;
	__int64 field_B8;
	_QWORD field_C0;
	_DWORD field_C8;
	_BYTE gap_CC[4];
	_QWORD field_D0;
	_QWORD field_D8;
	_QWORD field_E0;
	_BYTE field_E8;
	_BYTE gap_E9[3];
	_DWORD field_EC;
	_QWORD field_F0;
	_QWORD field_F8;
	_QWORD field_100;
	_QWORD field_108;
	_DWORD field_110;
	_BYTE gap_114[4];
	_QWORD field_118;
	_QWORD field_120;
	_QWORD field_128;
	_DWORD field_130;
	_DWORD field_134;
	_QWORD field_138;
	_QWORD field_140;
	_QWORD field_148;
	lvars_t vars;
	qvector<int> argidx;
	_DWORD field_180;
	_DWORD field_184;
	_QWORD field_188;
	_QWORD field_190;
	_QWORD* field_198;
	_QWORD field_1A0;
	_QWORD* field_1A8;
	_QWORD field_1B0;
	_QWORD* field_1B8;
	_QWORD field_1C0;
	_QWORD field_1C8;
	_QWORD field_1D0;
	_QWORD field_1D8;
	_DWORD field_1E0;
	_BYTE gap_1E4[4];
	_DWORD field_1E8;
	_DWORD field_1EC;
	char** field_1F0;
	_QWORD field_1F8;
	_BYTE gap_200[120];
	_DWORD field_278;
	_BYTE gap_27C[4];
	_QWORD field_280;
	_QWORD field_288;
	_QWORD field_290;
	int qty;
	mblock_t* blocks;
	mblock_t** natural;
	_QWORD mba_graph;
	_QWORD field_2B8;
	_QWORD field_2C0;
	_QWORD* field_2C8;
	_QWORD field_2D0;
	_QWORD* field_2D8;
	_QWORD field_2E0;
	_QWORD field_2E8;
	_QWORD field_2F0;
	_QWORD field_2F8;
	_WORD field_300;
	_BYTE gap_302[6];
	_QWORD* field_308;
	_QWORD field_310;
	_QWORD field_318;
	_QWORD field_320;
	_QWORD field_328;
	_QWORD field_330;
	_QWORD field_338;
	_QWORD field_340;
	_QWORD field_348;
	_QWORD* field_350;
	_QWORD field_358;
	_QWORD* field_360;
	_QWORD field_368;
	_QWORD* field_370;
	_QWORD field_378;
	_QWORD* field_380;
	_QWORD field_388;
	_QWORD field_390;
	_QWORD field_398;
	_QWORD field_3A0;
	_DWORD field_3A8;
	_BYTE gap_3AC[4];
	_QWORD field_3B0;
	_QWORD field_3B8;
	_QWORD field_3C0;
	_QWORD field_3C8;
	_QWORD field_3D0;
	_QWORD field_3D8;
	_QWORD field_3E0;
	_QWORD field_3E8;
	_QWORD field_3F0;
	_QWORD field_3F8;
	_QWORD field_400;
	_QWORD* field_408;
	_QWORD* field_410;
	_DWORD field_418;
	_DWORD field_41C;
	_QWORD* field_420;
	_QWORD field_428;
	_QWORD field_430;
	_QWORD field_438;
	signed int field_440;
	_BYTE gap_444[4];
	_QWORD* field_448;
	_QWORD field_450;
	_QWORD* field_458;
	_QWORD field_460;
	_QWORD* field_468;
	_QWORD field_470;
};

#endif

/*
	duplicate of codegen_t, but has undocumented virtual function emit
*/
class codegen_ex_t
{
public:
	mbl_array_t* mba;
	mblock_t* mb;
	insn_t insn;
	char ignore_micro;

	codegen_ex_t(mbl_array_t* m)
		: mba(m), mb(NULL), ignore_micro(IM_NONE) {}
	virtual ~codegen_ex_t(void)
	{
	}

	// Analyze prolog/epilog of the function to decompile
	// If found, allocate and fill 'mba->pi' structure.
	virtual int idaapi analyze_prolog(
		const class qflow_chart_t& fc,
		const class bitset_t& reachable) = 0;

	// Generate microcode for one instruction
	virtual int idaapi gen_micro() = 0;

	// Generate microcode to load one operand
	virtual mreg_t idaapi load_operand(int opnum) = 0;


	mreg_t load_constant_value(uint64_t v, unsigned size) {
		op_t backup = insn.ops[0];
		op_t& tempop = insn.ops[0];

		tempop.type = o_imm;
		tempop.value = v;
		tempop.dtype = get_dtype_by_size(size);

		mreg_t result = load_operand(0);

		insn.ops[0] = backup;

		return result;
	}
	
	minsn_t* emit_blank() {
		/*
			one of the parameters for emit actually affects iprops on instructions. i noticed that when i originally had each param -1 instead of 0
			it was setting the float property
			havent fiddled around to see which does what yet
		*/
		minsn_t* res = emit(m_ext, dt_byte, 0, 0, 0, 0);

		/*res->opcode = ;
		res->d.erase();
		res->l.erase();
		res->r.erase();
		res->iprops = 0;*/

		minsn_t* oldprev = res->prev;
		minsn_t* oldnext = res->next;

		new (res) minsn_t(res->ea);
		res->prev = oldprev;
		res->next = oldnext;
		return res;
	}

	minsn_t* undefine_flag(mreg_t reg) {
		return emit(m_und, dt_byte, 0, 0, reg, 0);
	}


	//this one is secretly in Hexrays for 7.0, but it wasnt documented at the time


	/// Emit one microinstruction.
/// The L, R, D arguments usually mean the register number. However, they depend
/// on CODE. For example:
///   - for m_goto and m_jcnd L is the target address
///   - for m_ldc L is the constant value to load
/// \param code  instruction opcode
/// \param width operand size in bytes
/// \param l     left operand
/// \param r     right operand
/// \param d     destination operand
/// \param offsize for ldx/stx, the size of the offset operand.
///                for ldc, operand number of the constant value
/// \return created microinstruction. can be NULL if the instruction got
///         immediately optimized away.
	virtual minsn_t* hexapi emit(mcode_t code, op_dtype_t dtype, uval_t l, uval_t r, uval_t d, int offsize) = 0;
}; //-

struct hexext_micro_filter_t
{
	/// check if the filter object is to be appied
	/// \return success
	virtual bool match(codegen_ex_t& cdg) {
		return false;
	}
	/// generate microcode for an instruction
	/// \return MERR_... code:
	///   MERR_OK      - user-defined call generated, go to the next instruction
	///   MERR_INSN    - not generated - the caller should try the standard way
	///   else         - error
	virtual int apply(codegen_ex_t& cdg) {
		return MERR_OK;
	}
};





#include "hexutils/micropool.hpp"

namespace hexext_internal {
	CS_COLD_CODE
	void init_hexext();
	CS_COLD_CODE
	void deinit_hexext();
}

using glbopt_cb_t = bool (*)(mbl_array_t*);

class mcombine_t {
public:
	minsn_t* m_insn;
	mblock_t* m_block;
	//fixed_size_vecptr_t<unsigned> m_bbidx_pool;
	bitset_t* m_bbidx_pool;

	minsn_t* insn() {
		return m_insn;
	}

	mblock_t* block() {
		return m_block;
	}

	decltype(m_bbidx_pool) bbidx_pool() {
		return m_bbidx_pool;
	}
};

class mcombiner_rule_t {
protected:

	virtual bool run_combine(mcombine_t* state) = 0;
public:
	virtual const char* name() const {
		return "Unnamed rule";
	}
	virtual const char* description() const {
		return "<Missing description>";
	}


	bool combine(mcombine_t* state) {
		return run_combine(state);
	}
};
#define		COMB_RULE_NAME(nam)	virtual const char* name() const override {	return nam; }

#define		COMB_RULE_RUN()		virtual bool run_combine(mcombine_t* state)

#define		COMB_RULE_DECL(identifer, nam)	\
class identifer##_t : public mcombiner_rule_t {\
public:\
	COMB_RULE_NAME(nam);\
COMB_RULE_RUN();\
};\
extern identifer##_t identifer;

#define		COMB_RULE_IMPLEMENT(identifier)	\
identifier##_t identifier{};\
bool identifier##_t ::run_combine(mcombine_t* state)

using combine_cb_t = mcombiner_rule_t*;
using asmrewrite_cb_t = bool (*)(insn_t&);

using microcode_generated_cb_t = bool(*)(mbl_array_t*);

using locopt_cb_t = bool(*)(mbl_array_t*);
using preopt_cb_t = bool(*)(mbl_array_t*);
enum class hexext_arch_e {
	x86,
	ARM
};
namespace hexext {
	CS_COLD_CODE
	void install_glbopt_cb(glbopt_cb_t cb);
	CS_COLD_CODE
	void install_combine_cb(combine_cb_t cb);
	CS_COLD_CODE
	void remove_combine_cb(combine_cb_t cb);
	CS_COLD_CODE
	void install_combine_cb(combine_cb_t cb, bool enabled);
	CS_COLD_CODE
	void install_microcode_generated_cb(microcode_generated_cb_t cb);
	CS_COLD_CODE
	void install_microcode_filter_ex(hexext_micro_filter_t* mcu, bool install = true);
	CS_COLD_CODE
	void install_locopt_cb(locopt_cb_t cb, bool enabled);
	CS_COLD_CODE
	void install_preopt_cb(preopt_cb_t cb, bool enabled);
	CS_COLD_CODE
	void install_asm_rewriter(asmrewrite_cb_t cb);
	CS_COLD_CODE
	void remove_asm_rewriter(asmrewrite_cb_t cb);
	CS_COLD_CODE
	void install_asm_rewriter(asmrewrite_cb_t cb, bool enabled);

	unsigned get_parent_mop_size();

	minsn_t* current_topinsn();
	mop_t* current_comb_mop();
	hexext_arch_e currarch();
	/*
	decode instruction ran through asm rewrite callbacks
	*/
	ea_t microgen_decode_prev_insn(insn_t* insn, ea_t ea);

	int microgen_decode_insn(insn_t* insn, ea_t ea);

	bool ran_preopt();
	bool ran_locopt();
	bool ran_glbopt();
}

#include <string>

CS_COLD_CODE
std::string print_mop(mop_t* mop);

CS_COLD_CODE
std::string mcode_to_string(mcode_t op);

CS_COLD_CODE
std::string print_insn(minsn_t* ins, bool addr = true);

CS_COLD_CODE
std::string print_mreg(int mr);



CS_COLD_CODE
void dump_mba(mbl_array_t* mba);

uint64_t  extend_value_by_size_and_sign(uint64_t value, unsigned int size, bool sign);
unsigned swap_mcode_relation(int a1);
unsigned get_signed_mcode(unsigned int a1);
bool is_mcode_propagatable(int a1);
bool  must_mcode_close_block(int a1, bool a2);
mreg_t reg2mreg(unsigned r);

mcode_t negate_mcode_relation(mcode_t a1);
mcode_t swap_mcode_relation(mcode_t a1);
mcode_t add_equality_condition_to_opcc(mcode_t a1);

inline bool is_mcode_shift(mcode_t code) {
	return code == m_shl || code == m_shr || code == m_sar;
}

inline bool is_mcode_shift_right(mcode_t code) {
	return code == m_shr || code == m_sar;
}
bool mcode_is_set(mcode_t arg);
static constexpr bool mcode_is_jconditional(mcode_t arg) {
	return arg >= m_jcnd && arg < m_jtbl;
}

static constexpr bool mcode_is_jcc(mcode_t arg) {
	return arg >= m_jnz && arg < m_jtbl;
}

static constexpr int jcc_to_setcc(mcode_t arg) {
	return (arg - m_jnz) + m_setnz;
}


static constexpr bool mcode_is_flow(mcode_t arg) {
	return (mcode_is_jconditional(arg) || arg == m_goto || arg == m_call || arg == m_icall || arg == m_jtbl || arg == m_ret || arg == m_ijmp );
}

static constexpr bool is_mcode_commutative(mcode_t mcode)
{
	return mcode == m_add
		|| mcode == m_mul
		|| mcode == m_or
		|| mcode == m_and
		|| mcode == m_xor
		|| mcode == m_setz
		|| mcode == m_setnz
		|| mcode == m_cfadd
		|| mcode == m_ofadd;
}

static constexpr bool is_mcode_zf_cond(mcode_t op) {
	return op == m_jz || op == m_jnz || op == m_setz || op == m_setnz;
}
static constexpr bool is_mcode_nz_cond(mcode_t op) {
	return op == m_jnz ||  op == m_setnz;
}
static constexpr bool is_mcode_z_cond(mcode_t op) {
	return op == m_jz || op == m_setz;
}

static constexpr bool is_mcode_conv_to_f(mcode_t op) {
	return op == m_i2f || op == m_u2f;
}

static constexpr bool mreg_is_cc(mreg_t mr) {
	return mr >= mr_cf && mr <= mr_cc;
}

static constexpr bool is_mcode_fpu(mcode_t op) {
	return op >= m_f2i;
}

/*
	first time we see an mblock in micro_filter_api we store its vtable into this variable so that we have it when we create new mblocks later
	this could cause problems if theres any situation in which pblock_t gets generated, which i havent observed yet. im thinking pblock_t 
	is something only used in house at hexrays. i dont know what it does exactly, but if it does pattern matching it would be super useful to have
*/
extern void* mblock_t_vftbl;

/*
	we store codegen_t's vftbl for a fucking awesome trick that allows us to fully map out the microregisters for the current processor
	so we can properly implement reg2mreg, mreg2reg, and stringify mregs
*/

extern void* codegen_t_vftbl;

inline minsn_bin_descend_t
mop_t::descend_to_binary_insn(mcode_t opcode) {

	if (optype() != mop_d)
		return {};

	if (d->opcode != opcode)
		return {};

	return { d, &d->l, &d->r };
}

inline minsn_unary_descend_t
mop_t::descend_to_unary_insn(mcode_t opcode) {
	if (optype() != mop_d)
		return {};

	if (d->opcode != opcode)
		return {};

	return { d, d->get_nonlr(mop_z) };
}

inline minsn_unary_descend_t
mop_t::descend_to_unary_insn(mcode_t opcode, mopt_t optype) {

	auto [insn, op] = descend_to_unary_insn(opcode);

	if (!insn || op->t != optype)
		return {};

	return { insn, op };
}



inline minsn_bin_descend_t
mop_t::descend_to_binary_insn(mcode_t opcode, mopt_t firstop_type) {
	auto [insn, l, r] = descend_to_binary_insn(opcode);

	if (!insn)
		return {};
	mop_t* second;

	mop_t* first = insn->get_eitherlr(firstop_type, &second);
	if (!first)
		return {};

	return { insn, first, second };
}

inline minsn_bin_descend_t
mop_t::descend_to_binary_insn(mcode_t opcode, bool (*firstop_type)(mcode_t)) {
	auto [insn, l, r] = descend_to_binary_insn(opcode);

	if (!insn)
		return {};
	mop_t* second;
	if (l->t != mop_d) {
		if (r->t != mop_d) {
			return {};
		}

		if (!firstop_type(r->d->op())) {
			return {};
		}
		return { insn, r, l };
	}
	else {
		if (!firstop_type(l->d->op())) {

			if (r->t != mop_d) {
				return {};
			}
			else {
				if (!firstop_type(r->d->op())) {
					return {};
				}
				else {
					return { insn, r, l };
				}
			}
		}
		else
			return { insn, l, r };
	}

}


inline minsn_bin_descend_t
mop_t::descend_to_binary_insn(mcode_t opcode, mopt_t firstop_type, mopt_t secondop_type) {
	auto [insn, first, second] = descend_to_binary_insn(opcode, firstop_type);

	if (!insn || second->t != secondop_type)
		return {};

	return { insn, first, second };
}

inline void mop_t::make_unary_insn_larg(mcode_t op, mop_t& larg, unsigned sz, ea_t addr){
	erase();

	d = new minsn_t(addr);


	this->size = sz;
	t = mop_d;

	d->d.size = sz;
	d->opcode = op;
	if (is_mcode_fpu(op)) {
		d->iprops |= IPROP_FPINSN;
	}
	d->l = larg;

}
#include "gdl.hpp"
// abstract graph interface
class simple_graph_t : public gdl_graph_t
{
public:
	qstring title;
	bool colored_gdl_edges;
private:
	friend class iterator;
	virtual int goup(int node) const;
};
struct voff_t
{
	sval_t off;         ///< register number or stack offset
	mopt_t type;        ///< mop_r - register, mop_S - stack, mop_z - undefined
	voff_t() : off(-1), type(mop_z) {}
	voff_t(mopt_t _type, sval_t _off) : off(_off), type(_type) {}
	voff_t(const mop_t& op) : off(-1), type(mop_z)
	{
		if (op.t == mop_r || op.t == mop_S)
			set(op.t, op.t == mop_r ? op.r : op.s->off);
	}

	void set(mopt_t _type, sval_t _off) { type = _type; off = _off; }
	void set_stkoff(sval_t stkoff) { set(mop_S, stkoff); }
	void set_reg(mreg_t mreg) { set(mop_r, mreg); }
	DECLARE_COMPARISONS(voff_t)
	{
		int code = ::compare(type, r.type);
		return code != 0 ? code : ::compare(off, r.off);
	}
};
struct chain_t : public intvec_t // sequence of block numbers
{
	voff_t k;             ///< Value offset of the chain.
						  ///< (what variable is this chain about)

	int width;            ///< size of the value in bytes
	int varnum;           ///< allocated variable index (-1 - not allocated yet)
	uchar flags;          ///< combination \ref CHF_ bits
  /// \defgroup CHF_ Chain properties
  //@{
#define CHF_INITED     0x01 ///< is chain initialized? (valid only after lvar allocation)
#define CHF_REPLACED   0x02 ///< chain operands have been replaced?
#define CHF_OVER       0x04 ///< overlapped chain
#define CHF_FAKE       0x08 ///< fake chain created by widen_chains()
#define CHF_PASSTHRU   0x10 ///< pass-thru chain, must use the input variable to the block
#define CHF_TERM       0x20 ///< terminating chain; the variable does not survive across the block

	chain_t() : width(0), varnum(-1), flags(CHF_INITED) {}
	chain_t(mopt_t t, sval_t off, int w = 1, int v = -1)
		: k(t, off), width(w), varnum(v), flags(CHF_INITED) {}
	chain_t(const voff_t& _k, int w = 1)
		: k(_k), width(w), varnum(-1), flags(CHF_INITED) {}
	const voff_t& key() const { return k; }
	bool operator<(const chain_t& r) const { return key() < r.key(); }
};

#define SIZEOF_BLOCK_CHAINS  24
struct block_chains_t
{

	//size_t body[SIZEOF_BLOCK_CHAINS / sizeof(size_t)];

	//std::set<chain_t> body;

	void* body[SIZEOF_BLOCK_CHAINS / sizeof(size_t)];
};

//graph chains
enum gctype_t
{
	GC_REGS_AND_STKVARS, ///< registers and stkvars (restricted memory only)
	GC_ASR,              ///< all the above and assertions
	GC_XDSU,             ///< only registers calculated with FULL_XDSU
	GC_END,              ///< number of chain types
	GC_DIRTY_ALL = (1 << (2 * GC_END)) - 1, ///< bitmask to represent all chains
};


#pragma push(pack, 1)
struct graph_chains_t : public qvector<block_chains_t> {
	int lock;
	unsigned char pad[32];
};


struct mbl_graph_t : public simple_graph_t
{

	mbl_array_t* mba;     ///< pointer to the mbl array
	int dirty;            ///< what kinds of use-def chains are dirty?
	int chain_stamp;      ///< we increment this counter each time chains are recalculated
	int butt;
	//graph chains
	unsigned long long padagain[2];
	graph_chains_t gcs[2 * GC_END]; ///< cached use-def chains
};
#pragma 

minsn_t* new_helper(
	minsn_t* callins,
	const char* helper_name,
	unsigned res_size,
	unsigned nargs,
	mcallarg_t* args,
	tinfo_t* return_type,
	argloc_t* return_argloc,
	rlist_t* return_regs,
	mlist_t* spoiled,
	unsigned nretregs,
	mop_t* retregs);
/*
	Initializes the MVM for the current architecture if it is not already initialized
*/
CS_COLD_CODE
void ensure_mvm_init();

cfunc_t* get_cached_cfunc(ea_t ea);

static inline std::pair<mop_t*, mop_t*> order_mops(mopt_t ty, mop_t* x, mop_t* y) {
	if (x->t == ty)
		return { x,y };
	if (y->t == ty)
		return { y,x };
	return { nullptr,nullptr };
}