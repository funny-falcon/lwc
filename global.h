/*
 *
 * Copyright (C) 2003, Stelios Xanthakis
 *
 */
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#ifdef SYS_HAS_MMAP
#include <sys/mman.h>
#endif
#include <stdarg.h>
#include <setjmp.h>
#include <time.h>


#define COLS "\033[01;37m"
#define COLE "\033[0m"
#define XSTR COLS"%s"COLE

//*****************************************************************************
// compiler.h
//*****************************************************************************

#ifdef __GNUC__		// ------- gcc near 3.2 ------
#define COMPILER "gcc"
#define INTERN_memcpy RESERVED___builtin_memcpy
#define INTERN_alloca RESERVED___builtin_alloca
#define INTERN_strncmp RESERVED___builtin_strncmp
#define INTERN_strncasecmp RESERVED_strncasecmp
#define CASE_RANGERS
#define HAVE_GNUC_LOCAL_LABELS
#define HAVE_GNUC_ATTR_NORETURN
#if __GNUC__ >= 4
#define BROKEN_ALIASES
#endif
#ifdef alloca
#undef alloca
#endif
#define alloca __builtin_alloca
#else			// ------ generic ------
#include <alloca.h>
#define COMPILER "generic"
#define INTERN_memcpy RESERVED_memcpy
#define INTERN_alloca RESERVED_alloca
#define INTERN_strncmp RESERVED_strncmp
#define INTERN_strncasecmp RESERVED_strncasecmp
#undef HAVE_GNUC_LOCAL_LABELS
#undef HAVE_GNUC_ATTR_NORETURN
#endif

//*****************************************************************************
// ld.h
//*****************************************************************************

#define SECTION_LINKONCE_DATA   ".gnu.linkonce.d."
#define SECTION_LINKONCE_TEXT   ".gnu.linkonce.t."
#define SECTION_LINKONCE_RODATA ".gnu.linkonce.r."
#define HAVE_LINKONCE
#define HAVE_BUILTIN_MEMCPY
#define SIZEOF_UCONTEXT "512"	// should suffice for the near future

//*****************************************************************************
// version.h
//*****************************************************************************

#define VERSION_MAJ 2
#define VERSION_MIN 0
#define LWC_VERSION "2.0"

#include "norm.h"

extern int zinit [];

//*****************************************************************************
// global.h
//*****************************************************************************

typedef int NormPtr, Token, recID, typeID, enumID;
typedef int bool;

#define true 1
#define false 0

#define ncase break; case
#define ndefault break; default

#define allocaint(x) (int*) alloca ((x) * sizeof (int))
#define mallocint(x) (int*) malloc ((x) * sizeof (int))
#define reallocint(x, y) (int*) realloc (x, (y) * sizeof (int))
static inline int* frealloc (int **a, int x) { free (*a); return *a = mallocint (x); }

//*****************************************************************************
// debug.h
//*****************************************************************************

#define DEBUG

#ifdef DEBUG
extern struct debugflag_s {
	bool
	GENERAL,
	PROGRESS,
	PEXPR,
	PEXPR_EXTREME,
	SHOWPROG,
	FUNCPROGRESS,
	OUTPUT_INDENTED,
	PARSE_ERRORS_SEGFAULT,
	EXPR_ERRORS_FATAL,
	DCL_TRACE,
	TDEF_TRACE,
	VIRTUALTABLES,
	VIRTUALBASE,
	VIRTUALCOMBINE,
	AUTOF,
	NOSTDOUT,
	SHOWCPP,
	CPP,
	REGEXP_DEBUG;
} debugflag;

#define PREPROCOUT "preproc.i"

extern void	enable_debugs	();
extern FILE	*logstream;
#define PRINTF(...)	fprintf (logstream, __VA_ARGS__)
#define CPRINTF(...)	fprintf (logstream, COLS __VA_ARGS__ COLE)
#define INTPRINT(x)	fintprint (logstream, x)
#define INTPRINTN(x)	{ INTPRINT(x); PRINTF("\n"); }
#define INTPRINTF(x,y)	{ PRINTF(x); INTPRINT(y); PRINTF("\n"); }
#endif

//*****************************************************************************
// config.h
//*****************************************************************************

#define MAGIC_DIGIT ""
#define GLOBINITF "GLoBaL_ConStRuCtOr"
#define DERRIVE_UNION "un"
#define PREPROCFILE ".preprocfile"
#define COMMENT_OUTPUT

#ifdef COLOR_IN_COMMENTS
#define REM_COLS COLS
#define REM_COLE COLE
#else
#define REM_COLS ""
#define REM_COLE ""
#endif

#define COMMENT_SECTION_FUNCTIONS \
"\n/*"REM_COLS"******** Program function definitions ********"REM_COLE"*/\n"

#define COMMENT_SECTION_AUTOFUNCTIONS \
"\n/*"REM_COLS"******** auto-function instantiations ********"REM_COLE"*/\n"

#define COMMENT_SECTION_OBJCTOR \
"\n/*"REM_COLS"*********** Module _Init Function  ***********"REM_COLE"*/\n"

#define COMMENT_SECTION_PROTOTYPES \
"\n/*"REM_COLS"************ Function Prototypes  ***********"REM_COLE"*/\n"

#define COMMENT_SECTION_INNERCODE \
"\n/*"REM_COLS"************ Internal Functions  ************"REM_COLE"*/\n"

#define COMMENT_SECTION_VIRTUALTABLES \
"\n/*"REM_COLS"*************** Virtual tables ***************"REM_COLE"*/\n"

#define COMMENT_SECTION_VTDEF \
"\n/*"REM_COLS"************* Virtua Table declarations *************"REM_COLE"*/\n"

#define COMMENT_SECTION_STRUCTS \
"\n/*"REM_COLS"************* Structures *************"REM_COLE"*/\n"

#define COMMENT_SECTION_GVARS \
"\n/*"REM_COLS"************* Global variables *************"REM_COLE"*/\n"

#define COMMENT_SECTION_INCLUDE \
"\n/*"REM_COLS"************* system headers *************"REM_COLE"*/\n"

#define COMMENT_SECTION_GLOBAL \
"\n/*"REM_COLS"************* global scope *************"REM_COLE"*/\n"

#define COMMENT_SECTION_REGEXP \
"\n/*"REM_COLS"************* regular expressions *************"REM_COLE"*/\n"

//*****************************************************************************
// preproc.h
//*****************************************************************************

extern void	preproc (int, char**);
extern char	*current_file, *main_file;

//*****************************************************************************
// lwc_config.h
//*****************************************************************************

extern NormPtr	lwc_config (NormPtr);
extern NormPtr	__C__ (NormPtr);

//*****************************************************************************
// main.h
//*****************************************************************************

extern bool MainModule, HadErrors, Reentrant, ExpandAllAutos;
extern bool StructByRef, InlineAllVt, ConstVtables, VIDeclarations;
extern bool Streams_Closed, ExceptionsUsed, GlobInitUsed, vtptrConst;
extern bool NoLinkonce, OneBigFile, HaveAliases, StdcallMembers, ExportVtbl;
extern bool EHUnwind;
extern bool GoodOldC;

extern int max_symbol_len;

//*****************************************************************************
// output.h
//*****************************************************************************

#define EOST		-1
#define STRNEXT		-2
#define NSTRNEXT	-3
#define BLANKT		-4
#define NONULL		-5
#define BACKSPACE	-6

#define ISTR(x) STRNEXT, x
#define R(x) RESERVED_ ## x

typedef struct outstream *OUTSTREAM;

extern OUTSTREAM GLOBAL, GLOBAL_INIT_FUNC, FUNCDEFCODE, INTERNAL_CODE, FPROTOS, AUTOFUNCTIONS;
extern OUTSTREAM VTABLE_DECLARATIONS, VIRTUALTABLES, STRUCTS, GVARS, INCLUDE, REGEXP_CODE;

extern OUTSTREAM	new_stream ();

extern void		free_stream	(OUTSTREAM);
extern int		ntokens		(OUTSTREAM);
extern void             output_itoken	(OUTSTREAM, int);
extern void		outprintf	(OUTSTREAM, Token, ...);
extern void		backspace_token	(OUTSTREAM);
extern int		*combine_output	(OUTSTREAM);
extern OUTSTREAM	concate_streams (OUTSTREAM, OUTSTREAM);
extern int		get_stream_pos	(OUTSTREAM);
extern void		wipeout_unwind	(OUTSTREAM, int);
extern void		nowipeout_unwind(OUTSTREAM, int);
extern void		export_output	(OUTSTREAM);

//*****************************************************************************
// cpp.h
//*****************************************************************************

extern bool	sys_cpp;
extern void	setup_cpp	(int, char**);
extern void	cleanup_cpp	();
extern void	cpp_directive	();

#define VARBOOST 1000
#define VOIDARG	  999
extern int	is_macro	(Token);
extern Token*	expand_macro	(Token);

//*****************************************************************************
// lex.h
//*****************************************************************************

extern Token	RESERVED_attr_stdcall;

extern void	fatal		(const char*);
extern int	*CODE, c_ntok;
extern int	c_nsym, c_nval;

extern char	*Cpp;
extern int	Ci, Clen;
extern int	line;
extern char	*tfile;

extern Token	enter_symbol	(char*);

extern int	c_line_of	(NormPtr);
extern char	*c_file_of	(NormPtr);

extern void	adjust_lines	(NormPtr, int);

extern void	initlex		();
extern int	enter_value	(char*);
extern int	yydo		(char*);
extern int	yydo_file	(char*);
extern void 	yydo_mem	(char*, int);
extern char	*expand		(Token);
inline
static char	*EXPC		(NormPtr p) { return expand (CODE [p]); }
extern Token	new_symbol	(char*);
extern Token	new_value_int		(int);
extern Token	new_value_string	(char*);
extern Token	stringify		(Token);
extern Token	token_addchar		(Token, int);

extern void	add_extra_values	(char**, int);

extern Token	Lookup_Symbol	(const char*);

extern typeID	type_of_const	(Token);
extern bool	is_literal	(Token);

extern int		eval_int	(Token);
extern long long int	eval_intll	(Token);

extern Token 	binshift [];

#define tokcmp(tok, str) strcmp (expand (tok), str)

//*****************************************************************************
// templates.h
//*****************************************************************************

extern void	do_templates ();

//*****************************************************************************
// util.h
//*****************************************************************************

extern NormPtr	skip_parenthesis	(NormPtr);
extern NormPtr	skip_brackets		(NormPtr);
extern NormPtr	skip_braces		(NormPtr);
extern NormPtr	skip_declaration	(NormPtr);
extern NormPtr	skip_buffer_parenthesis	(Token*, NormPtr);
extern NormPtr	skip_buffer_brackets	(Token*, NormPtr);
extern NormPtr	skip_buffer_braces	(Token*, NormPtr);

extern bool	intchr		(int*, int);
extern void	intsubst	(int*, int, int);
extern void	intsubst1	(int*, int, int);
extern Token	*intcpy		(int*, int*);
extern void	intcat		(int*, int*);
extern void	intncat		(int*, int*, int);
extern void	intcatc		(int*, int);
extern void	intextract	(int*, int*, int);
extern int	intlen		(int*);
extern int	*intdup		(int*);
extern int	*intdup1	(int*);
extern int	*argtdup	(typeID*);
extern int	*intndup	(int*, int);
extern int	intcmp		(int*, int*);
extern void	fintprint	(FILE*, Token*);
extern Token	*sintprintf	(Token*, Token, ...);

extern char*	escape_q_string	(char*, int);
extern char*	loadtext	(char*);

struct load_file {
    int fd;
    char *data;
    int len;
    int success;
};
extern void ctor_load_file_ (struct load_file *const, char*);
extern void dtor_load_file_ (struct load_file *const);

typedef struct intnode_t {
	struct intnode_t *less, *more;
	int key;
	union ival {
		void *p;
		int i;
	} v;
} intnode;

extern intnode	*intfind	(intnode*, int);
extern void	intadd		(intnode**, int, union ival);
extern void	intremove	(intnode**, intnode*);

extern void	debug_pr_type	(typeID);

//*****************************************************************************
// misc.h
//*****************************************************************************

#define VERIFY_symbol 0
#define VERIFY_string 1

extern bool	syntax_pattern		(NormPtr, Token, ...);

extern void	set_catch		(jmp_buf*, NormPtr, Token*, int);
extern void	raise_skip_function	();
extern void	expr_error		(const char*);
extern void	expr_errort		(const char*, Token);
extern void	expr_errortt		(const char*, Token, Token);
extern void	expr_error_undef	(Token, int);
extern void	expr_warn		(const char*);
extern void	expr_warnt		(const char*, Token);
extern void	expr_warntt		(const char*, Token, Token);
extern void	clear_catch		();

extern NormPtr	last_location;
extern Token	in_function;
extern bool	may_throw;

#define SET_MAYTHROW(X) may_throw |= !(X.flagz&FUNCP_NOTHROW)

extern void	parse_error		(NormPtr, const char*);
extern void	parse_error_tok		(Token, const char*);
extern void	parse_error_cpp		(const char*);
extern void	parse_error_toktok	(Token, Token, const char*);
extern void	parse_error_pt		(NormPtr, Token, const char*);
extern void	parse_error_ll		(const char*);

extern void	warning_tok		(const char*, Token);

extern void	name_of_simple_type	(Token*, typeID);

extern typeID	bt_promotion	(typeID);
extern typeID	ptrup		(typeID);
extern typeID	ptrdown		(typeID);
extern typeID	funcreturn	(typeID);
extern typeID	dereference	(typeID);
extern typeID	makemember	(typeID, recID);

extern Token*	build_type	(typeID, Token, Token[]);
extern typeID	typeof_designator	(typeID, Token[]);

extern Token	isunary_overloadable		(Token);
extern Token	isunary_postfix_overloadable	(Token);
extern Token	isbinary_overloadable		(Token);

extern void	remove_struct_from_this	(Token*, recID);
extern void	add_struct_to_this	(Token*);

extern Token	include_sys_header_s	(char*);
extern Token	include_sys_header	(Token);

extern Token	alias_func	(recID, Token);
extern Token	linkonce_data	(Token);
extern Token	linkonce_data_f	(Token);
extern Token	linkonce_text	(Token);
extern Token	linkonce_rodata	(Token);
extern Token	cleanup_func	(Token);
extern Token	section_vtblz	(Token);

extern typeID	typeof_expression	(NormPtr, int);

extern bool	is_expression	(NormPtr);

extern void	bogus1	();

//*****************************************************************************
// fspace.h
//*****************************************************************************

typedef struct funcp_t {
	struct funcp_t	*next;
	Token		name;
	typeID		type;
	Token		*prototype;
	typeID		*ovarglist;
	Token		**dflt_args;
	Token		*xargs;
	Token		section;
	int		flagz;
	bool		used;
} funcp;

#define FUNCP_LINKONCE	1
#define FUNCP_AUTO	2
#define FUNCP_MODULAR	4
#define FUNCP_CTHIS	8
#define FUNCP_VIRTUAL	16
#define FUNCP_PURE	32
#define FUNCP_FINAL	64
#define FUNCP_STATIC	128
#define FUNCP_INLINE	256
#define FUNCP_UNDEF	512
#define FUNCP_NOTHROW	1024
#define FUNCP_USED	2048

typedef intnode *fspace;

extern fspace Global;

extern typeID*	promoted_arglist	(typeID*);
extern typeID*	promoted_arglist_t	(typeID);
extern char*	nametype		(char *ret, typeID);
extern char*	type_string		(char*, typeID);
extern bool	arglist_compare		(typeID*, typeID*);
extern funcp*	xdeclare_function	(fspace*, Token, Token, typeID, Token*, Token*,
					 int, Token**, Token);
extern Token	declare_function_member	(recID, Token, typeID, Token*, Token*, int, Token**, Token);
extern funcp*	xlookup_function_dcl	(fspace, Token, typeID[]);

typedef struct {
	Token	oname;
	typeID	t;
	Token	**dflt_args;
	Token	*prototype, *xargs;
	int	flagz;
} flookup;

extern bool	have_function		(fspace, Token);
extern int	xlookup_function	(fspace, Token, typeID[], flookup*);
extern Token	xlookup_function_uname	(fspace, Token);
extern typeID	lookup_function_symbol	(Token);
extern void	xmark_section_linkonce	(fspace, Token, Token);
extern void	xmark_nothrow		(fspace, Token, Token);
extern int	xmark_function_USED	(fspace, Token);
extern void	export_fspace		(fspace);
extern void	export_fspace_lwc	(fspace);

extern OUTSTREAM	printproto	(OUTSTREAM, Token[], Token, bool);
extern OUTSTREAM	printproto_si	(OUTSTREAM, Token[], Token, bool);

//*****************************************************************************
// cdb.h
//*****************************************************************************

extern void	enter_enumconst	(Token, enumID);
extern int	is_enumconst	(Token);
extern enumID	id_of_enumconst	(Token);

extern enumID	enter_enum	(Token);
extern enumID	lookup_enum	(Token);
extern void	enter_enum_syms	(enumID, Token[], int);
extern Token*	enum_syms	(enumID);
inline
static bool	is_enum		(Token t)	{ return lookup_enum (t) != -1; }

extern bool	enter_typedef	(Token, typeID);
extern typeID	lookup_typedef	(Token);

extern Token	new_wrap, delete_wrap;
extern void	enter_newdel_overload	(Token, recID, Token);
extern Token	lookup_newdel_operator	(Token, recID);

extern typeID	enter_type	(int*);
extern int	*open_typeID	(typeID);
extern bool	isfunction	(typeID);
extern bool	typeID_elliptic	(typeID);
extern typeID	elliptic_type	(typeID);
extern int	is_typename	(Token);
extern bool	is_dcl_start	(Token);
extern recID	lookup_object	(Token);

extern void	enter_abstract		(Token, Token*, Token*, NormPtr);
extern bool	have_abstract		(Token);
extern int	real_abstract_parents	(Token, Token[]);
extern bool	abstract_has_special	(Token, recID);
extern NormPtr	dcl_of_abstract		(Token);
extern Token*	parents_of_abstract	(Token);

extern bool	is_template_function	(NormPtr*, NormPtr);
extern bool	is_extern_templ_func	(NormPtr*);
extern void	enter_abstract_derrived	(Token, recID);
extern bool	specialize_abstracts	();

extern void	enter_global_object	(Token, typeID);
extern typeID	lookup_global_object	(Token);

extern void	open_local_scope	();
extern Token	recent_obj		();
extern void	globalized_recent_obj	(Token);
extern void	add_catchpoint		(int);
extern void	enter_local_obj		(Token, typeID);
extern void	undo_local_obj		(Token);
extern void	add_auto_destruction	(Token, recID, bool);
extern bool	scope_has_dtors		();
extern bool	break_has_dtors		(int);
extern bool	func_has_dtors		();
#define		REFERENCE_BOOST		10000
extern typeID	lookup_local_obj	(Token, Token*);
extern void	gen_auto_destruction	(OUTSTREAM, bool);
extern Token	gen_break_destructors	(OUTSTREAM, int, int);
extern void	gen_all_destructors	(OUTSTREAM);
extern int	close_local_scope	();
extern void	rmv_catchpoint		(Token*, Token*);

extern void*	reopen_local_scope	(void*);
extern void	restore_local_scope	(void*);

extern void*	active_scope		();
extern void	restore_scope		(void*);

static inline int base_of (typeID t)
{ return open_typeID (t)[0]; }

enum {
	B_SCHAR = -32, B_UCHAR, B_SSINT, B_USINT, B_SINT, B_UINT,
	B_SLONG, B_ULONG, B_SLLONG, B_ULLONG, B_SSIZE_T, B_USIZE_T,
        B_FLOAT, B_DOUBLE, B_LDOUBLE,
	B_VOID, B_ELLIPSIS, B_PELLIPSIS, B_PURE, INTERNAL_ARGEND /* -13 */
};

#define REFERENCE_BASE (REFERENCE_BOOST + B_SCHAR)

static inline int dbase_of (typeID t)
{ recID r = base_of (t); return r >= REFERENCE_BASE ? r - REFERENCE_BOOST : r; }

extern typeID typeID_NOTYPE, typeID_int, typeID_float, typeID_charP;
extern typeID typeID_voidP ,typeID_void, typeID_uint, typeID_ebn_f, typeID_intP;

//*****************************************************************************
// hier.h
//*****************************************************************************

// - build db

#define		VIRTUALPAR_BOOST	10000
extern recID	enter_struct		(Token, Token, bool, bool, bool, bool);
extern void	set_depend		(recID, recID);
extern void	set_declaration		(recID, Token*);
extern void	set_parents		(recID, recID[]);
extern void	output_parents		(OUTSTREAM, recID);
extern void	add_variable_member	(recID, Token, typeID, Token, bool, bool);
extern Token	add_anonymous_union	(recID, recID);
extern Token	add_local_typedef	(recID, Token);
extern Token	have_local_typedef	(recID, Token);
extern bool	PARENT_AUTOFUNCS;
extern void	add_auto_f		(Token, Token, recID, typeID, bool, bool, NormPtr,
					 Token*, Token*);
extern void	add_pure_dm		(recID, Token, NormPtr);
extern void	set_dfunc		(recID, Token, bool, bool);
extern Token	dtor_name		(recID);
extern void	set_dtor_nothrow	(recID);
extern void	enter_class_const	(recID, Token, Token);
extern Token	add_function_member	(recID, Token);
extern void	possible_keyfunc	(recID, Token);
extern void	keyfunc_candidate	(recID, Token);
extern int	Here_virtualtable	(OUTSTREAM, recID, bool, bool, bool);
extern void	add_virtual_varmemb	(recID, Token, Token, typeID, Token*, Token*,
					 bool, OUTSTREAM);
extern void	rename_hier		(Token, Token);
extern int	virtual_inheritance_decl	(OUTSTREAM, recID, Token);
typedef struct {
	recID	r;
	Token	fname;
	typeID	ftype;
	Token	fmemb;
	Token	*prototype, *argv;
	int	flagz;
	OUTSTREAM scopeout;
} vf_args;
extern void	Make_virtual_table	(OUTSTREAM, recID, Token, typeID);
extern void	Enter_virtual_function	(vf_args*);
extern recID	complete_structure	(OUTSTREAM, recID);
extern recID	is_aliasclass		(recID);
extern recID	aliasclass		(recID);
#define iRESERVED_struct(r) (is_aliasclass (r) ? BLANKT : RESERVED_struct)

extern void	mk_typeid	(recID);

// - use db

extern recID	lookup_struct	(Token);
extern int	is_struct	(Token);
extern Token	name_of_struct	(recID);
static 
inline char*	SNM		(recID r)	{ return expand (name_of_struct (r)); }
extern typeID	pthis_of_struct	(recID);
extern fspace	FSP		(recID);
extern bool	has_const_members	(recID);

extern int	inherited_flagz			(recID, Token, typeID);
extern int	exported_flagz			(recID, Token, typeID);
extern typeID	lookup_variable_member		(recID, Token, Token*, bool, Token*);
extern typeID	lookup_virtual_varmemb		(recID, Token, Token*, bool, Token**);
extern Token	get_class_vptr			(recID);
extern Token	lookup_class_const		(recID, Token);
extern int	lookup_function_member		(recID, Token, typeID[], flookup*, bool);
extern Token	lookup_function_member_uname	(recID*, Token);
extern int	lookup_virtual_function_member	(recID, Token, typeID[], Token*, flookup*);
extern bool	Is_implied_virtual_variable	(recID, Token);
extern bool	Is_pure_virtual			(recID, Token, typeID[], flookup*);
typedef struct {
	typeID t;
	Token rec, *memb, *expr;
} vtvar;
extern vtvar	access_virtual_variable		(recID, Token);

extern bool	isunion		(recID);
extern bool	has_void_ctor	(recID);
extern bool	has_copy_ctor	(recID);
extern bool	has_dtor	(recID);
extern bool	always_unwind	(recID);
extern Token	idtor		(recID);
extern Token	vdtor		(recID);
extern bool	has_ctors	(recID);
extern bool	has_oper_fcall	(recID);
extern Token	have_fmemb_dcl	(recID, Token);

extern OUTSTREAM	dispatch_vdtor	(recID, OUTSTREAM);

extern void	make_intern_dtor	(recID);

extern bool	zero_offset		(recID, recID);
extern bool	isancestor		(recID, recID);
extern int	is_ancestor		(recID, recID, Token**, bool);
extern int	is_ancestor_runtime	(recID, recID, Token**);
extern Token*	upcast1_this		(recID, recID);

extern recID	ancest_named	(recID, Token);
extern void	downcast_rtti	(recID, recID, recID, Token[]);

extern void purify_vfunc	(Token);

extern void export_virtual_table_instances	();
extern void export_virtual_definitions		();
extern void export_virtual_static_definitions	();
extern void export_structs			();

extern bool Can_instantiate		(recID);
extern bool need_construction		(recID);
extern bool need_vbase_alloc		(recID);

extern void gen_vt_init			(OUTSTREAM, recID, Token, Token);
extern void produce_dtorables		(OUTSTREAM, recID);
extern void gen_construction_code	(OUTSTREAM, recID, Token);

extern Token	lookup_local_typedef	(recID, Token);
extern int	borrow_auto_decls	(recID, NormPtr[]);
extern void	gen_pure_dm		(recID, OUTSTREAM);
extern void	repl__CLASS_		(Token**, recID);

//*****************************************************************************
// inames.h
//*****************************************************************************

extern Token	arrdtor;
extern Token	name_anonymous_struct	();
extern Token	name_anonymous_union	();
extern Token	name_anonymous_enum	();
extern Token	name_anon_union		(int, Token);
extern Token	name_overload_fun	(Token, char*);
extern Token	name_member_function	(recID, Token);
extern Token	name_member_function_virtual	(Token, Token);
extern Token	name_local_typedef	(Token, Token);
extern Token	name_internal_object	();
extern Token	name_uniq_var		(int);
extern Token	internal_identifier1	();
extern Token	internal_identifier2	();
extern Token	internal_identifier3	();
extern Token	internal_identifiern	(int);
extern Token	name_define_new		(Token, Token);
extern Token	name_inherited		(Token);
extern Token	name_storage_inherit	(recID);
extern Token	name_typeid_var		(Token);
extern Token	name_unwind_var		(Token);
extern Token	name_glob_static_local	(Token);
extern Token	name_arrdto_var		(Token);
extern Token	name_downcast		(Token, Token);
extern Token	name_downcast_safe	(Token, Token);
extern Token	name_upcast_safe	(Token, Token);
extern Token	name_virtual_slot	(recID, Token, typeID);
extern Token	name_virtual_variable	(recID, Token);
extern Token	name_instance		(recID, recID);
extern Token	name_arrdtor		(recID);
extern Token	name_virtual_table	(recID);
extern Token	name_virtual_inner	(recID, recID, Token, typeID);
extern Token	name_name_enumerate	(Token, int);
extern Token	name_rtti_slot		(recID, recID);
extern Token	name_intern_ctor	(recID);
extern Token	name_intern_dtor	(recID);
extern Token	name_intern_vdtor	(recID);
extern Token	name_derrive_memb	(recID);
extern Token	name_anon_regexp	();
extern Token	name_derrive_union;
extern Token	tokstrcat		(Token, char*);
extern Token	toktokcat		(Token, Token);
extern Token	name_global_ctor	(int);
extern Token	name_ebn_func		(Token);
extern Token	name_longbreak		();
extern Token	name_longcontinue	();

//*****************************************************************************
// icode.h
//*****************************************************************************

extern Token	i_call_initialization	(recID);
extern Token	i_downcast_function	(recID, recID);
extern Token	i_downcast_null_safe	(recID, recID);
extern Token	i_upcast_null_safe	(recID, recID, Token*, bool);
extern Token	i_member_virtual	(recID, Token, typeID*, flookup*);
extern Token	i_trampoline_func	(flookup*, bool[]);
extern Token	i_enum_by_name		(Token);
extern Token	i_typeid_var		(Token);
extern Token	i_arrdtor_func		(recID);

extern void	alloc_and_init		(OUTSTREAM, recID, Token, Token, Token);
extern void	alloc_and_init_dcl	(OUTSTREAM, recID, Token, bool);

//*****************************************************************************
// breakexpr.h
//*****************************************************************************

typedef enum {
	NOAMBIG,
	PARENTH_CAST, PARENTH_SYNTAX, PARENTH_FCALL, PARENTH_POSTFIX,
	SIZEOF_TYPE, SIZEOF_EXPRESSION,
} exambg;

typedef struct {
	Token *expr;
	exambg et;
	NormPtr operators [128];
	int nop;
	bool esc_op;

	Token **branch;
} bexpr;

extern void	break_expr	(bexpr*);

enum { CONST_EXPR, INIT_EXPR, NORMAL_EXPR };

extern NormPtr	skip_expression	(Token*, NormPtr, int);

//*****************************************************************************
// rexpr.h
//*****************************************************************************

extern struct obflag {
	bool yes;
	recID class;
	bool have_this;
	typeID classP;
	Token func, efunc;
	bool polymorph, recording;
	bool isdtor;
} objective;

typedef struct {
	Token *newExpr;
	bool isconstant, ok;
	typeID type;
} exprret;

extern void	parse_expression_string		(Token*, exprret*);
extern NormPtr	parse_expression		(NormPtr, exprret*, int);
extern NormPtr	parse_expression_retconv	(NormPtr, exprret*, typeID, int);
extern NormPtr	parse_const_expression		(NormPtr, exprret*);

extern void	rewrite_designator	(typeID, Token[]);
extern Token	*rewrite_ctor_expr	(Token*);

extern bool	ispointer	(typeID);
extern bool	isstructure	(typeID);
extern bool	isstructptr	(typeID);
extern bool	isreference	(typeID);

extern bool	is_object_in_scope	(Token);

//*****************************************************************************
// statement.h
//*****************************************************************************

extern NormPtr	compound_statement		(OUTSTREAM, NormPtr);
extern NormPtr	statement			(OUTSTREAM, NormPtr);
extern Token*	rewrite_compound_statement	(Token*);

//*****************************************************************************
// dcl.h
//*****************************************************************************

extern Token	local_name;
extern typeID	local_type;
extern recID	class_scope;

extern bool	is_array_of_ctorable_objects	(typeID);

extern NormPtr	gen_array_ctors		(OUTSTREAM, NormPtr, typeID, Token, int*, bool);
extern NormPtr	struct_declaration	(recID, OUTSTREAM, NormPtr);
extern NormPtr	local_declaration	(OUTSTREAM, NormPtr);
extern void	reparse_template_func	(Token, recID, NormPtr);
extern typeID	eval_cast		(Token**);
extern void	translation_unit	();
extern Token	bt_macro, bt_replace;

extern recID	current_scope[];
extern int	top_scope;

//*****************************************************************************
// fdb.h
//*****************************************************************************

extern Token *func_prologue;

typedef enum { DT_NORM, DT_AUTO, DT_ABSTRACT } deftype;

extern typeID	return_typeID;
extern void	store_definition
	(Token, Token*, Token*, typeID*, NormPtr, recID, typeID, Token, bool, deftype, bool);
extern void	store_definition_alias
	(Token, Token*, Token*, typeID*, NormPtr, recID, typeID, Token, bool, deftype, bool);
extern void	store_define_dtor	(Token, recID);
extern void	commit_auto_define	(Token, recID, Token, Token, bool, Token*, Token*, typeID);
extern void	define_auto_functions	();
extern void	commit_abstract_define	(Token, Token, recID, bool);
extern void	rename_fdb		(Token, Token);
extern void	remove_struct_from_def	(Token);
extern void	do_functions		();

// -- usage

extern void usage_tconst	(Token);
extern void usage_upcast	(recID);
extern void usage_fcall		(Token);
extern void usage_memb		(Token);
extern void usage_vvar		(Token*);
extern void usage_typeID	(typeID);
extern void usage_notok		();
extern void usage_call_pure	();
extern void usage_set_pure	();

//*****************************************************************************
// except.h
//*****************************************************************************

#define LEAVE_ESCOPE -1
extern bool did_unwind;
extern void push_unwind		(OUTSTREAM, recID, Token);
extern void pop_unwind		(OUTSTREAM);
extern void leave_escope	(OUTSTREAM);
extern void remove_unwind_stuff	(Token*);
extern NormPtr throw_statement		(OUTSTREAM, NormPtr);
extern NormPtr try_statement		(OUTSTREAM, NormPtr);
extern NormPtr on_throw_statement	(OUTSTREAM, NormPtr);
extern void init_except		();
extern void decl_except_data	();

//*****************************************************************************
// regexp.h
//*****************************************************************************

extern Token	perlop_regexp	(Token);
extern NormPtr	parse_RegExp	(NormPtr, int);

//*****************************************************************************
// textp.h
//*****************************************************************************

extern char	*escape_c_string (char*, int);
extern char	*interpolate_string (char*, int);
extern void	init_processors ();
typedef char	*(*text_processor) (char*, int);

extern int processor;
extern text_processor TP [128];

//*****************************************************************************
// test.h
//*****************************************************************************

#define SAVE_VAR(x, y) __typeof__(x) _tmp_ ## x = x; x = y
#define RESTOR_VAR(x) x = _tmp_ ## x

#define SAVE_VARC(x, y, tmp) __typeof__(x) tmp = x; x = y
#define RESTOR_VARC(x, tmp) x = tmp;

#define SAVE_CODE(x) SAVE_VAR (CODE, x); SAVE_VAR (last_location, 0);
#define RESTOR_CODE  RESTOR_VAR (CODE); RESTOR_VAR (last_location);

#define CLEAR_MAYTHROW bool mthback = may_throw; may_throw = 0;
#define TEST_MAYTHROW ({int rez = !may_throw; may_throw |= mthback; rez;})

static inline int max (int i, int j) { return i > j ? i : j; }
static inline int min (int i, int j) { return i < j ? i : j; }
static inline int in2 (int a, int b, int c) { return a == b || a == c; }
static inline int in3 (int a, int b, int c, int d) { return in2 (a, b, c) || a == d; }
static inline int in4 (int a, int b, int c, int d, int e)
		 { return in2 (a, b, c) || in2 (a, d, e); }
static inline int xor (int a, int b) { return a ? !b : !!b; }
static inline int nz (int a) { return a != 0; }

static inline bool is_reference (typeID t)
{ return open_typeID (t)[0] >= REFERENCE_BASE; }
