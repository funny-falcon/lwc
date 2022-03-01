#ifndef GLOBAL_INCLUDED
#define GLOBAL_INCLUDED
_lwc_config_ {
	lwcdebug PARSE_ERRORS_SEGFAULT;
};

extern "setjmp.h" {
#include <setjmp.h>
}
#include <setjmp.h>
#define MSPEC 20
#define BITFIELD_Q 64

_lwc_config_ {
//	lwcdebug PEXPR;
//	lwcdebug > "log.out";
};

//
// the types we'll be using
//
typedef int NormPtr;
typedef int RegionPtr, ObjPtr, typeID, ArglPtr, Symbol, *Vspec;
typedef int exprID;

#include "config.h"
#include "norm.h"
#include "mem_pool.h"

//
// preprocessing
//
extern void preproc (int, char**);
extern void ncc_keys ();


//
// program options
//
extern bool usage_only, include_values, include_strings, multiple, abs_paths;
extern char *sourcefile, *preprocfile, *cwd;

//
// inform
//
extern char* StrDup (char*);
extern char* StrDup (char*, int);
extern void debug (const char*, NormPtr, int);
extern void prcode (NormPtr, int);
extern void prcode (NormPtr, int, Symbol[]);
extern void prcode (NormPtr, int, Symbol);
extern void printtype (int, int*);
extern void printtype (typeID);
extern int syntax_error (NormPtr, char* = NULL);
extern int syntax_error (char*, char*);
extern int syntax_error (NormPtr, char*, char*);
extern void half_error (char*, char* = NULL);
extern void warning (char*, char = 0);
extern char *expand (int);
extern int cline_of (NormPtr);
extern int cfile_of (NormPtr);
extern char *in_file (NormPtr);



#if 0
class EXPR_ERROR {public: EXPR_ERROR () {}};

#endif




//
// lex & normalized C source
//
struct token
{
	int at_line;
	unsigned int type;
	char *p;
	int len;
};
extern token CTok;

struct cfile_i
{
	int	indx;
	char	*file;
};

struct clines_i
{
	int	ftok, line;
};

extern int*		CODE;
extern int		C_Ntok;
extern char**		C_Syms;
extern int		C_Nsyms;
extern char**		C_Strings;
extern int		C_Nstrings;
extern cfile_i*		C_Files;
extern int		C_Nfiles;
extern clines_i*	C_Lines;
extern int		C_Nlines;

extern double*		C_Floats;
extern signed char*	C_Chars;
extern short int*	C_Shortints;
extern int*	C_Ints;
extern unsigned int*	C_Unsigned;
extern int long*	C_Longs;
extern unsigned int long*	C_ULongs;

extern struct __builtins__ {
	int bt__builtin_alloca;
	int bt__builtin_return_address;
	int bt__FUNCTION__;
	int bt__PRETTY_FUNCTION__;
} ccbuiltins;

extern void enter_token ();
extern void enter_file_indicator (char*);
extern void prepare ();
extern void make_norm ();
extern int getint (int);
extern void yynorm (char*, int);


//
// utilities
//
extern void	intcpycat (int*, const int*, const int*);
extern int*	intdup (int*);
extern int	intcmp (int*, int*);
extern void	intncpy (int*, int*, int);
static inline	void intcpy (int *d, const int *s)
		 { while ((*d++ = *s++) != -1); }
static inline	int intlen (const int *i)
		 { int l=0; while (*i++ != -1) l++; return l; }

class load_file
{
	int fd;
   public:
	load_file (char*);
	int success;
	char *data;
	int len;
	~load_file ();
};

#define ZC_OK 0
#define ZC_NA 1
#define ZC_AC 2
#define ZC_FF 3

//
// the compilation
//

class declarator
{
	NormPtr p;
	int dp;
inline	void dcl ();
	void dirdcl ();
	void bitfield ();
	void arglist_to_specs ();
	NormPtr parse (NormPtr);

#ifdef GNU_VIOLATIONS
	NormPtr bt_typeof (NormPtr);
#endif
	NormPtr builtin (NormPtr);
	NormPtr bt_enum (NormPtr);
	NormPtr bt_typedef (NormPtr);
	NormPtr bt_struct (NormPtr);

	void complete_size ();
	void argument_conversions ();
	void semantics ();
	bool do_argument_conversions;
	ObjPtr basetype;
   public:
	bool have_extern, have_static, have_typedef, have_const,
	     have_init, have_code, is_anonymous;

	declarator (bool b = false) { do_argument_conversions = b; }
	NormPtr parse_base (NormPtr);
	NormPtr parse_dcl (NormPtr);

	typeID gentype;
	NormPtr args;
	int symbol;
	int spec [MSPEC];
};

class declaration
{
virtual	void semantics (NormPtr);
	NormPtr parse_initializer (NormPtr);
   public:
	declarator D;
	declaration (bool b = false) { D.ctor (b); }
	NormPtr parse (NormPtr);
};

class declaration_instruct : declaration
{
	void semantics (NormPtr);
};

extern void parse_C ();


//
// CDB interface
//
enum VARSPC {
	EXTERN, STATIC, DEFAULT
};

typedef bool Ok;

extern typeID VoidType, SIntType;

enum BASETYPE {
	S_CHAR = -20, U_CHAR, S_SINT, U_SINT, S_INT, U_INT,
	S_LINT, U_LINT, S_LONG, U_LONG, FLOAT, DOUBLE, VOID,
	_BTLIMIT
};
#define INTEGRAL(x) (x >= S_CHAR && x <= U_LONG)
#define TYPEDEF_BASE 50000

struct type {
	int base;
	Vspec spec;
};
#define ISFUNCTION(t) ((t).spec [0] == '(')
#define T_BASETYPE(t) ((t).base < _BTLIMIT)
#define T_BASESTRUCT(t) (t > 0 && t < TYPEDEF_BASE)

#define ARGLIST_OPEN -2
#define SPECIAL_ELLIPSIS -3

#define INCODE (!INGLOBAL && !INSTRUCT)
extern bool INGLOBAL, INSTRUCT;
extern ArglPtr NoArgSpec;
extern void init_cdb ();
extern typeID		gettype (type);
extern typeID		gettype (int, int*);
extern ArglPtr		make_arglist (typeID*);
extern typeID*		ret_arglist (ArglPtr);
extern void		opentype (typeID, type);
extern int		base_of (typeID);
extern int*		spec_of (typeID);
extern int		esizeof_objptr (ObjPtr);
extern int		sizeof_typeID (typeID);
extern int		sizeof_type (int, Vspec);
extern int		sizeof_type (type);
extern int		ptr_increment (int, Vspec);
extern Ok		introduce_obj (Symbol, typeID, VARSPC);
extern Ok		introduce_tdef (Symbol, typeID);
extern ObjPtr		lookup_typedef (Symbol);
extern Ok		introduce_enumconst (Symbol, int);
extern Ok		introduce_enumtag (Symbol);
extern Ok		valid_enumtag (Symbol);
extern RegionPtr	introduce_anon_struct (bool);
extern RegionPtr	introduce_named_struct (Symbol, bool);
extern RegionPtr	use_struct_tag (Symbol, bool);
extern RegionPtr	fwd_struct_tag (Symbol, bool);
extern Ok		function_definition (Symbol, NormPtr, NormPtr, NormPtr);
extern Ok		function_no (int, NormPtr*, NormPtr*);
extern void		open_compound ();
extern void		close_region ();
extern Symbol		struct_by_name (RegionPtr);
extern void		functions_of_file ();
extern bool		have_function (Symbol);
extern bool		rename_struct (typeID, Symbol);

//
// CDB lookups
//
struct lookup_object {
	bool enumconst;
	int ec;
	ObjPtr base;
	RegionPtr FRAME;
	int displacement;
	int spec [50];
	lookup_object (Symbol);
};

struct lookup_function {
	bool fptr;
	ObjPtr base;
	RegionPtr FRAME;
	int displacement;
	int spec [50];
	lookup_function (Symbol);
};

struct lookup_member {
	ObjPtr base;
	int spec [50];
	int displacement;
	lookup_member (Symbol, RegionPtr);
};

//
// cc-expressions
//
enum COPS {
	VALUE, FVALUE, DVALUE, SVALUE, UVALUE, ULONGVALUE, AVALUE,
	SYMBOL,
	FCALL, ARRAY, MEMB,
	PPPOST, MMPOST,
	PPPRE, MMPRE, LNEG, OCPL, PTRIND, ADDROF, UPLUS, UMINUS, CAST, SIZEOF,
	MUL, DIV, REM, ADD, SUB, SHL, SHR,
	BEQ, BNEQ, CGR, CGRE, CLE, CLEE,
	BAND, BOR, BXOR,
	IAND, IOR,
	GCOND, COND, // assignments taken from norm.h defines
	COMMA, ARGCOMMA,
	COMPOUND_RESULT
};

struct subexpr
{
	int action;
	union {
		int using_result;
		long int value;
		unsigned long int uvalue;
		double fvalue;
		Symbol symbol;
		exprID e;
	} voici;
	exprID e;
	union {
		typeID cast;
		Symbol member;
		exprID eelse;
		typeID result_type;
	} voila;
};

struct exprtree
{
	subexpr *ee;
	int ne;
	exprID first;
};


extern exprtree	CExpr;
extern int	last_result;
//extern subexpr	*&ee;
extern NormPtr	ExpressionPtr;


extern typeID	typeof_expression ();
extern int	cc_int_expression ();

extern struct lrt {
	int base;
	int spec [MSPEC];
} last_result_type;

//
// expand initializer
//

class dcle
{
	struct {
		int p;
		bool marked;
		int c, max;
		int s;
	} nests [30];
	int ni;
	Symbol *dclstr;
	NormPtr p;
	void openarray ();
	void openstruct ();
	NormPtr skipbracket (NormPtr);
	bool opennest ();
	bool closenest ();
	Symbol pexpr [30];
   public:
	dcle (Symbol);
	bool open_bracket ();
	bool tofield ();
	bool comma ();
	bool tostruct (RegionPtr);
	bool designator (Symbol[]);
	bool close_bracket ();
	Symbol* mk_current ();
	void printexpr ();
	~dcle ();
};

//
// different behaviour of the compiler
//
extern class ncci
{
	public:
	virtual void cc_expression () = 0;
	virtual void new_function (Symbol) = 0;
	virtual void inline_assembly (NormPtr, int) = 0;
	virtual void finir () { }
} *ncc;

class ncci_usage : ncci
{
	public:
	void cc_expression ();
	void new_function (Symbol);
	void inline_assembly (NormPtr, int);
	void finir ();
};

class ncci_cc : ncci
{
	public:
	void cc_expression ();
	void new_function (Symbol);
	void inline_assembly (NormPtr, int);
};

extern void set_compilation ();
extern void set_usage_report ();

typedef struct {
	jmp_buf env;
} jmpbufs;

extern void	set_catch	(jmpbufs);
extern void	clear_catch	();

#endif
