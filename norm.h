
#define ESCBASE		10000
#define IDENTBASE	20000
#define DYNBASE		500000
#define ARGBASE		990000
#define VALBASE		1000000
#define STRBASE		2000000
#define XCOMMENT	3000000

// control sequences
enum {
	THE_END = 1,
	RETRY,
	CPP_CONCAT,
	CPP_DIRECTIVE
};

// C/C++ operators
enum {
	ELLIPSIS = 650,
	POINTSAT,
	MINUSMINUS,
	PLUSPLUS,
	GEQCMP,
	LSH,
	OROR,
	ANDAND,
	EQCMP,
	NEQCMP,
	RSH,
	LEQCMP,
	PERLOP,
};

// '='
enum {
	ASSIGNA = 670,
	ASSIGNS,
	ASSIGNM,
	ASSIGND,
	ASSIGNR,
	ASSIGNBA,
	ASSIGNBX,
	ASSIGNBO,
	ASSIGNRS,
	ASSIGNLS
};

#define ISASSIGNMENT(x) (x >= ASSIGNA && x <= ASSIGNLS)

#define NOTHING		695
#define NOOBJ		697
#define MARKER		699
#define DBG_MARK	698
#define UWMARK		696

enum {
	// C declaration flags
	RESERVED_auto = IDENTBASE,
	RESERVED_const,
	RESERVED_extern,
	RESERVED_static,
	RESERVED_inline,
	RESERVED_typedef,
	RESERVED___thread,
	RESERVED_register,
	RESERVED_volatile,
	RESERVED_final,
	RESERVED_linkonce,
	RESERVED___unwind__,
	RESERVED___byvalue__,
	RESERVED___noctor__,
	RESERVED___emit_vtbl__,
	RESERVED_modular,
	RESERVED_virtual,
#define ISDCLFLAG(x) (x >= RESERVED_auto && x <= RESERVED_virtual)
#define ISCDCLFLAG(x) (x <= RESERVED_volatile)

	// these can define a type
	RESERVED_long,
	RESERVED_short,
	RESERVED_signed,
	RESERVED_unsigned,
	RESERVED_ssz_t,
	RESERVED_usz_t,

	// C base types
	RESERVED_void,
	RESERVED_char,
	RESERVED_int,
#ifdef __LWC_HAS_FLOAT128
	RESERVED__Float128,
#endif
	RESERVED_float,
	RESERVED_double,
#define ISTBASETYPE(x) (x >= RESERVED_long && x <= RESERVED_double)
#define ISBUILTIN(x) (x >= RESERVED_void && x <= RESERVED_double)

	// C aggregate specs
	RESERVED_enum,
	RESERVED_benum,
	RESERVED_struct,
	RESERVED_class,
	RESERVED_union,
#define ISAGGRSPC(x) (x >= RESERVED_enum && x <= RESERVED_union)

	// standard C reserved
	RESERVED_break,
	RESERVED_case,
	RESERVED_continue,
	RESERVED_default,
	RESERVED_do,
	RESERVED_else,
	RESERVED_for,
	RESERVED_goto,
	RESERVED_if,
	RESERVED_return,
	RESERVED_sizeof,
	RESERVED_switch,
	RESERVED_while,

	RESERVED___asm__,
	RESERVED___extension__,
	RESERVED___attribute__,
	RESERVED___restrict,

	// our own lwc reserved words
	RESERVED_template,
	RESERVED_bool,
	RESERVED_true,
	RESERVED_false,
	RESERVED_this,
	RESERVED_new,
	RESERVED_delete,
	RESERVED_localloc,
	RESERVED_operator,
	RESERVED_try,
	RESERVED_throw,
	RESERVED_typeof,
	RESERVED_specialize,
	RESERVED_postfix,
	RESERVED_dereference,
	RESERVED_RegExp,
	RESERVED___declexpr__,
	RESERVED__lwc_config_,
	RESERVED___C__,

#define ISRESERVED(x) (x >= RESERVED_auto && x <= RESERVED__lwc_config_)

	// * * * * * * * * * * * * * * * * * * *
	// these below fail the ISRESERVED test
	// and pass the ISSYMBOL test. they are
	// not reserved and can be used for
	// various other things
	// * * * * * * * * * * * * * * * * * * *

#define SYMBASE RESERVED_include
	RESERVED_include,
	RESERVED_define,
	RESERVED_undef,
	RESERVED_endif,
	RESERVED_ifdef,
	RESERVED_ifndef,
	RESERVED_elif,
	RESERVED_error,
	RESERVED_line,
	RESERVED_uses,
	RESERVED___VA_ARGS__,
	RESERVED_defined,

	RESERVED___LINE__,
	RESERVED___FILE__,
	RESERVED___TIME__,
	RESERVED___DATE__,
#define ISPREDEF(x) (x >= RESERVED___LINE__ && x <= RESERVED___DATE__)

	RESERVED__,
	RESERVED_ctor,
	RESERVED_dtor,
	RESERVED__i_n_i_t_,
	RESERVED_nothrow,
	RESERVED_alias,
	RESERVED_used,
	RESERVED_public,
	RESERVED_private,
	RESERVED___typeof__,
	RESERVED___enumstr__,
	RESERVED___inset__,
	RESERVED__v_p_t_r_,
	RESERVED__CLASS_,
	RESERVED_typeid,
	RESERVED__loadtext,
	RESERVED__LWC_RANDOM_,
	RESERVED_main,
	RESERVED_size_t,
	RESERVED_wchar_t,
	RESERVED_malloc,
	RESERVED_free,
	RESERVED_alloca,
	RESERVED___builtin_alloca,
	RESERVED_jmp_buf,
	RESERVED_setjmp,
	RESERVED_longjmp,
	RESERVED___on_throw__,

	RESERVED___builtin_memcpy,
	RESERVED_memcpy,
	RESERVED_strncmp,
	RESERVED___builtin_strncmp,
	RESERVED_strncasecmp,
	RESERVED___builtin_strncasecmp,
	RESERVED___FUNCTION__,
	RESERVED___PRETTY_FUNCTION__,
	RESERVED___section__,
	RESERVED___label__,
	RESERVED_noreturn,
	RESERVED_constructor,
	RESERVED___lwc_unwind,
	RESERVED___lwc_landingpad,

	RESERVED_p,
	RESERVED_pos,
	RESERVED_len,
	RESERVED_min,
	RESERVED_max,
	RESERVED_charp_len,
	RESERVED_abbrev,
	RESERVED_strlen,
	RESERVED_a,
	RESERVED_x,
	RESERVED_X,
	RESERVED_y,
	RESERVED_j,
	RESERVED_s,
	RESERVED_i,

	// operator overloaders -safe-
	RESERVED_oper_plus,
	RESERVED_oper_minus,
	RESERVED_oper_thingy,
	RESERVED_oper_fcall,
	RESERVED_oper_comma,
	RESERVED_oper_mod,
	RESERVED_oper_or,
	RESERVED_oper_and,
	RESERVED_oper_xor,
	RESERVED_oper_lsh,
	RESERVED_oper_rsh,
	RESERVED_oper_mul,
	RESERVED_oper_div,
	RESERVED_oper_andand,
	RESERVED_oper_oror,
	RESERVED_oper_as_m,
	RESERVED_oper_as_d,
	RESERVED_oper_as_r,
	RESERVED_oper_as_ba,
	RESERVED_oper_as_bx,
	RESERVED_oper_as_bo,
	RESERVED_oper_as_rs,
	RESERVED_oper_as_ls,

	// operator overloaders -unsafe-
	RESERVED_oper_star,
	RESERVED_oper_excl,
	RESERVED_oper_array,
	RESERVED_oper_plusplus,
	RESERVED_oper_minusminus,
	RESERVED_oper_plusplusp,
	RESERVED_oper_minusminusp,
	RESERVED_oper_add,
	RESERVED_oper_sub,
	RESERVED_oper_gr,
	RESERVED_oper_le,
	RESERVED_oper_greq,
	RESERVED_oper_leq,
	RESERVED_oper_eq,
	RESERVED_oper_neq,
	RESERVED_oper_assign,
	RESERVED_oper_as_a,
	RESERVED_oper_as_s,
	RESERVED_oper_pointsat
};

#define ISIDENT(x) (x >= IDENTBASE && x < VALBASE)
#define ISSYMBOL(x) (x >= SYMBASE && x < VALBASE)
static inline int issymbol (int x) { return ISSYMBOL (x); }
#define ISVALUE(x) (x >= VALBASE)
static inline int isvalue (int x) { return ISVALUE (x); }
#define ISSYMVAL(x) (x >= SYMBASE)
static inline int isoperator (int x)
{ return x <= '~' || (x > ELLIPSIS && x <= ASSIGNLS) || x == RESERVED_sizeof; }
static inline int isescoperator (int x)
{ return x > ESCBASE && x < IDENTBASE ? x - ESCBASE : 0; }
#define ISTPLARG(x) (x >= ARGBASE && x < VALBASE)
static inline int isaggrspc (int x) { return ISAGGRSPC(x); }
static inline int isdclflag (int x) { return ISDCLFLAG (x); }


static inline int isoperfunc (int x) { return x >= RESERVED_oper_plus && x <= RESERVED_oper_pointsat; }
static inline int isunsafeop (int x) { return x >= RESERVED_oper_star && x <= RESERVED_oper_pointsat; }

// these are special values -- they match ISVALUE
enum {
	RESERVED_0 = VALBASE,
	RESERVED_1,
	RESERVED_3,
	RESERVED_C
};

