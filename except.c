#include "global.h"

static Token longjmp_stack_t, unwind_t, longjmp_base;
static Token longjmp_ctx_top;
static Token longjmp_ctx;

void init_except ()
{
	longjmp_stack_t = new_symbol ("longjmp_StaCk");
	unwind_t = new_symbol ("AuToDt_t");
	longjmp_ctx_top = new_symbol ("longjmp_StaCkTop");
	longjmp_ctx = new_symbol ("longjmp_CoNtExT");
	longjmp_base = new_symbol ("longjmp_iNiTObjFaKe");
}

static char _lwcbuiltin_estack [] = 
"static inline void *__lwcbuiltin_get_estack ()\n"
"{\n"
"	return longjmp_StaCkTop;\n"
"}\n"
"static inline void __lwcbuiltin_set_estack (void *v)\n"
"{\n"
"	longjmp_StaCkTop = v;\n"
"}\n";

static char _Unwind_gcc [] =
" /*"COLS"*** gcc unwind internals ***"COLE"*/\n"
" static _Unwind_Reason_Code"
" ThRoW_sToP ( )"
" {"
"  return _URC_NO_REASON ;"
" }"
""
" void __lwc_unwind (void*) __attribute__ ((noreturn, noinline));"
" void __lwc_unwind ( void *i )"
" {"
"  struct _Unwind_Exception * exc = malloc ( sizeof * exc ) ;"
"  exc -> exception_class = 0 ;"
"  exc -> exception_cleanup = 0 ;"
"  longjmp_StaCkTop -> X = i ;"
"  longjmp_StaCkTop -> i = exc ;"
#ifndef __USING_SJLJ_EXCEPTIONS__
"  _Unwind_ForcedUnwind ( exc , ThRoW_sToP,  0 ) ;"
#else
"  _Unwind_SjLj_ForcedUnwind ( exc , ThRoW_sToP , 0 ) ;"
#endif
" \n/* did you forget -fexceptions ? */ * ( int * ) 0 = 0 ;"
" __lwc_unwind (i);"
"} \n"
" void __lwc_landingpad ( int * i ) "
" {"
"  if ( ! * i ) "
"   longjmp ( * longjmp_StaCkTop -> x , 1 ) ;"
" }";

#define IFEH if (EHUnwind) {
#define IFNEH if (!EHUnwind) {
#define ELSE } else {
#define ENDIF }

void decl_except_data ()
{
	outprintf (INCLUDE, include_sys_header_s ("setjmp.h"), -1);
IFEH	// include <unwind.h>
	outprintf (INCLUDE, include_sys_header_s ("unwind.h"), -1);
ENDIF

IFNEH	// declare manual unwind accounting
	outprintf (STRUCTS,
		RESERVED_struct, unwind_t, '{',
		RESERVED_const, RESERVED_struct, unwind_t, '*', RESERVED_X, ';',
		RESERVED_void, '*', RESERVED_x, ';',
		RESERVED_void, '*', '(', '*' , RESERVED_y, ')', '(', RESERVED_void, '*', ')', ';',
		'}', ';', -1);
ENDIF

	outprintf (STRUCTS,
		RESERVED_struct, longjmp_stack_t, '{',
		RESERVED_jmp_buf, '*', RESERVED_x, ';',
		RESERVED_struct, longjmp_stack_t, '*', RESERVED_y, ';',
		RESERVED_void, '*', RESERVED_X, ';', -1);
IFEH	// cleanup eh mess
	outprintf (STRUCTS, RESERVED_void, '*', RESERVED_i, ';', -1);
ELSE	// accounting member
	outprintf (STRUCTS, RESERVED_const, RESERVED_struct, unwind_t, '*', RESERVED_i, ';', -1);
ENDIF
	outprintf (STRUCTS, '}', ';', -1);

	if (MainModule)
		outprintf (GVARS, Reentrant ? RESERVED___thread : BLANKT,
			   RESERVED_struct, longjmp_stack_t, longjmp_base, ',', '*',
			   RESERVED___restrict, longjmp_ctx_top, '=', '&', longjmp_base, ';', -1);
	else
		outprintf (GVARS, RESERVED_extern,
			   Reentrant ? RESERVED___thread : BLANKT,
			   RESERVED_struct, longjmp_stack_t, '*', longjmp_ctx_top, ';', -1);

#define EHTOP longjmp_ctx_top, POINTSAT
#define ADTOP EHTOP, RESERVED_i
	if (!MainModule) {
		outprintf (GVARS, RESERVED_extern, RESERVED_void, RESERVED___lwc_unwind,
			   '(', RESERVED_void, '*', ')', -1);
#ifdef	HAVE_GNUC_ATTR_NORETURN
		outprintf (GVARS, RESERVED___attribute__, '(', '(',RESERVED_noreturn,')', ')', -1);
#endif
		output_itoken (GVARS, ';');
IFEH		// landing pad prototype
		outprintf (GVARS, RESERVED_extern, RESERVED_void, RESERVED___lwc_landingpad,
			   '(', RESERVED_int, '*', ')', ';', -1);
ENDIF
	} else
		if (!EHUnwind)
		outprintf (GVARS, RESERVED_void, RESERVED___lwc_unwind, '(', RESERVED_void, '*',')',
			   RESERVED___attribute__, '(', '(', RESERVED_noreturn, ')', ')', ';',
			   RESERVED_void, RESERVED___lwc_unwind, '(', RESERVED_void, '*',
			   RESERVED_X, ')', '{',
			   EHTOP, RESERVED_X, '=', RESERVED_X, ';',
			   RESERVED_while, '(', ADTOP, ')', '{',
			   ADTOP, POINTSAT, RESERVED_y, '(', ADTOP, POINTSAT, RESERVED_x, ')', ';',
			   ADTOP, '=', ADTOP, POINTSAT, RESERVED_X, ';', '}',
			   RESERVED_longjmp, '(', '*',
			   EHTOP, RESERVED_x, ',', RESERVED_1, ')', ';', '}', -1);
		else output_itoken (GVARS, new_symbol (_Unwind_gcc));
	if (!EHUnwind)
		outprintf (GVARS, new_symbol (_lwcbuiltin_estack), -1);
}


/* member 'r' is normally a recID, but if a symbol, it's
   the name of the custom dtor function.  Useful for arrdtor  */
#define DTOR_OF(x) ISSYMBOL(x) ? x : dtor_name (x)

bool did_unwind;

void push_unwind (OUTSTREAM o, recID r, Token arg)
{
	if (EHUnwind) return;
	did_unwind = true;
#define NOWARNONTYPE '(', RESERVED___typeof__, '(', u, '.', RESERVED_y, ')', ')'
	Token u = name_unwind_var (arg);
	outprintf (o, UWMARK, RESERVED_const, RESERVED_struct, unwind_t, u, '=', '{',
		   '.', RESERVED_X, '=', ADTOP, ',',
		   '.', RESERVED_x, '=', '&', arg, ',',
		   '.', RESERVED_y, '=', NOWARNONTYPE, DTOR_OF (r), ',',
		   '}', ';', ADTOP, '=', '&', u, ';', UWMARK, -1);
}

void pop_unwind (OUTSTREAM o)
{
	if (EHUnwind) return;
	outprintf (o, UWMARK, ADTOP, '=', ADTOP, POINTSAT, RESERVED_X, ';', UWMARK, -1);
}

void remove_unwind_stuff (Token *str)
{
	int i, j, k;
	for (i = 0; str [i] != -1;)
		if (str [i] == UWMARK) {
			for (j = i + 1; str [j] != UWMARK; j++);
			for (k = i, j += 1;;)
				if ((str [k++] = str [j++]) == -1)
					break;
		} else i++;
}

void leave_escope (OUTSTREAM o)
{
IFEH
	outprintf (o, internal_identifier3 (), '=', RESERVED_1, ';', -1);
ENDIF
	outprintf (o, longjmp_ctx_top, '=', longjmp_ctx_top, POINTSAT, RESERVED_y, ';',  -1);
}

NormPtr throw_statement (OUTSTREAM o, NormPtr p)
{
	may_throw = true;

	outprintf (o, RESERVED___lwc_unwind, '(', -1);

	if (CODE [p] == ';')
		output_itoken (o, RESERVED_0);
	else { 
		exprret E;
		p = parse_expression_retconv (p, &E, typeID_voidP, NORMAL_EXPR);
		if (!E.ok) return p + 1;
		outprintf (o, ISTR (E.newExpr), -1);

		free (E.newExpr);
		if (CODE [p] != ';')
			parse_error (p, "excepted ';' after throw expression");
	}

	outprintf (o, ')', ';', -1);

	return p + 1;
}

NormPtr try_statement (OUTSTREAM o, NormPtr p)
{
	Token x = RESERVED_x;
	Token y = RESERVED_y;
	Token X = RESERVED_X;
	Token i = RESERVED_i;
	Token ii = internal_identifier2 ();
	Token i3 = internal_identifier3 ();
	Token retcode = 0;
	bool have_retcode;

	outprintf (o, '{', RESERVED_struct, longjmp_stack_t, longjmp_ctx, ';', -1);

	if (have_retcode = CODE [p] == '(') {
		NormPtr p2;
		p = skip_parenthesis (p2 = p + 1) - 1;
		CODE [p] = ';';
		open_local_scope ();
		local_declaration (o, p2);
		CODE [p++] = ')';
		retcode = recent_obj ();
	}

	outprintf (o, RESERVED_jmp_buf, ii, ';', longjmp_ctx, '.', x, '=', '&', ii, ';',
		   longjmp_ctx, '.', y, '=', longjmp_ctx_top, ';', longjmp_ctx_top,
		   '=', '&', longjmp_ctx, ';', longjmp_ctx, '.', X, '=', RESERVED_0, ';', -1);

IFNEH	// init manual accounting
	outprintf (o, longjmp_ctx, '.', RESERVED_i, '=', RESERVED_0, ';', -1);
ENDIF
	outprintf (o,
		   RESERVED_if, '(', '!', '(', RESERVED_setjmp, '(', ii, ')', ')',
		   ')', '{', -1);

IFEH	// cleanup with landing pad
	outprintf (o, RESERVED_int, i3, cleanup_func (RESERVED___lwc_landingpad),
				 '=', RESERVED_0, ';', -1);
ENDIF

	SAVE_VAR (may_throw, may_throw);
	int pos = get_stream_pos (o);
	p = statement (o, p);
	nowipeout_unwind (o, pos);
	RESTOR_VAR (may_throw);

IFEH	// mark landing pad normal termination
	outprintf (o, i3, '=', RESERVED_1, ';', -1);
ENDIF
	outprintf (o, longjmp_ctx_top, '=', longjmp_ctx, '.', y, ';', '}', -1);

	outprintf (o, RESERVED_else, '{', -1);
	if (have_retcode)
		outprintf (o, retcode, '=', longjmp_ctx, '.', RESERVED_X, ';', -1);
IFEH	// cleanup eh mess
	outprintf (o, RESERVED_free, '(', longjmp_ctx, '.', i, ')', ';', -1);
ENDIF
	outprintf (o, longjmp_ctx_top, '=', longjmp_ctx, '.', y, ';', -1);
	if (CODE [p] == RESERVED_else)
		p = statement (o, p + 1);
	output_itoken (o, '}');

	output_itoken (o, '}');

	if (have_retcode) close_local_scope ();

	return p;
}

NormPtr on_throw_statement (OUTSTREAM o, NormPtr p)
{
	Token x = RESERVED_x;
	Token y = RESERVED_y;
	Token X = RESERVED_X;
	Token ii = internal_identifier2 ();
	Token i3 = internal_identifier3 ();
	Token retcode = 0;
	bool have_retcode;

	// the exception handler is removed when the scope closes
	add_auto_destruction (LEAVE_ESCOPE, -1, 0);

	outprintf (o, RESERVED_struct, longjmp_stack_t, longjmp_ctx, ';', -1);

	if (have_retcode = CODE [p] == '(') {
		NormPtr p2;
		p = skip_parenthesis (p2 = p + 1) - 1;
		CODE [p] = ';';
		open_local_scope ();
		local_declaration (o, p2);
		CODE [p++] = ')';
		retcode = recent_obj ();
	}

	outprintf (o, RESERVED_jmp_buf, ii, ';', longjmp_ctx, '.', x, '=', '&', ii, ';',
		   longjmp_ctx, '.', y, '=', longjmp_ctx_top, ';', longjmp_ctx_top,
		   '=', '&', longjmp_ctx, ';', longjmp_ctx, '.', X, '=', RESERVED_0, ';', -1);

IFNEH	// init manual accounting
	outprintf (o, longjmp_ctx, '.', RESERVED_i, '=', RESERVED_0, ';', -1);
ELSE	// install landing pad
	outprintf (o, RESERVED_int, i3, cleanup_func (RESERVED___lwc_landingpad),
				 '=', RESERVED_0, ';', -1);
ENDIF

	outprintf (o,
		   RESERVED_if, '(', '(', RESERVED_setjmp, '(', ii, ')', ')', ')', '{', -1);
IFNEH	// init manual accounting
	outprintf (o, longjmp_ctx, '.', RESERVED_i, '=', RESERVED_0, ';', -1);
ENDIF
	if (have_retcode)
		outprintf (o, retcode, '=', longjmp_ctx, '.', RESERVED_X, ';', -1);

	p = statement (o, p);
	output_itoken (o, '}');
	if (have_retcode) close_local_scope ();

	return p;
}
