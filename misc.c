#include "global.h"

//#define errstream stdout
#define errstream stderr

/*****************************************************************************
	check syntax patterns.
*****************************************************************************/
bool syntax_pattern (NormPtr p, Token first, ...)
{
	va_list ap;
	va_start (ap, first);
	for (;first != -1; first = va_arg (ap, Token))
		switch (first) {
		default: if (CODE [p++] != first) return 0;
		ncase VERIFY_symbol: if (!issymbol (CODE [p++])) return 0;
		ncase VERIFY_string: if (!isvalue (CODE [p++])) return 0;
		}
	va_end (ap);
	return 1;
}

/*****************************************************************************
	-- Expression long-jumps
	If an exception is raised with expr_error, it longjmps
	Some allocated token strings will not be freed.
	We know that. And we prefer to leave it that way.
*****************************************************************************/

typedef struct except_stck_t {
	struct except_stck_t *up;
	jmp_buf *env;
	NormPtr location;
	Token *expression;
	void *scope;
	int elevel, rlevel;
} except_stck;

static except_stck *bottom;

void set_catch (jmp_buf *env, NormPtr p, Token *t, int level)
{
	except_stck *e = (except_stck*) malloc (sizeof * e);
	e->location = p;
	e->expression = t;
	e->env = env;
	e->up = bottom;
	e->scope = active_scope ();
	e->rlevel = e->elevel = level;
	bottom = e;
}

static void dothrow (int level)
{
	bottom->rlevel = level;
	longjmp (*bottom->env, 1);
}

void clear_catch ()
{
	except_stck *e = bottom;
	int rlevel = e->rlevel;
	int rethrow = rlevel && rlevel > e->elevel;
	bottom = e->up;
	restore_scope (e->scope);
	free (e);
	if (rethrow)
		dothrow (rlevel);
}

void raise_skip_function ()
{
	dothrow (1);
}

#define EER "expression error: "
#define WARN "Warning: "

static void throw_it (bool noerror)
{
	int i;
	char *MSG = noerror ? WARN : EER;

	if (!bottom || (bottom->rlevel && !bottom->up))
		parse_error (last_location, "Fatal");

	fprintf (errstream, "%sin expression: ", MSG);
	for (i = 0; bottom->expression [i] != -1; i++)
		fprintf (errstream, "%s ", expand (bottom->expression [i]));
	fprintf (errstream, "\n%sWhich is located in file %s:%i\n", MSG,
		c_file_of (bottom->location), c_line_of (bottom->location));
	if (in_function) fprintf (stderr, "%sin function %s();\n", MSG, expand (in_function));
	if (noerror) return;
#ifdef	DEBUG
        fflush(errstream);
	if (debugflag.PARSE_ERRORS_SEGFAULT)
		*((int*)0)=0;
	if (debugflag.EXPR_ERRORS_FATAL) exit (1);
#endif
	HadErrors = 1;
	dothrow (0);
}

void expr_error (const char *description)
{
	fprintf (errstream, EER"%s\n", description);
	throw_it (0);
}

void expr_errort (const char *description, Token t)
{
	fprintf (errstream, EER"%s `%s'\n", description, expand (t));
	throw_it (0);
}

void expr_errortt (const char *description, Token t1, Token t2)
{
	fprintf (errstream, EER"%s `%s' `%s'\n", description, expand (t1), expand (t2));
	throw_it (0);
}

void expr_warn (const char *description)
{
	fprintf (errstream, WARN"%s\n", description);
	throw_it (1);
}

void expr_warnt (const char *description, Token t)
{
	fprintf (errstream, WARN"%s `%s'\n", description, expand (t));
	throw_it (1);
}

void expr_warntt (const char *description, Token t1, Token t2)
{
	fprintf (errstream, WARN"%s `%s' `%s'\n", description, expand (t1), expand (t2));
	throw_it (1);
}

static intnode *undeftree;

void expr_error_undef (Token t, int w)
{
	if (intfind (undeftree, t)) dothrow (0);
	union ival i;
	intadd (&undeftree, t, i);
	fprintf (errstream, EER"Un%s '%s'\n", w ? "declared function" : "defined object", expand (t));
	throw_it (0);
}

void parse_error (NormPtr i, const char *p)
{
	fprintf (errstream, "%s:%i [Token %i]: %s\n", c_file_of(i), c_line_of(i), i, p);
	NormPtr n = i - 10;
	if (n < 0) n = 0;
	fprintf (errstream, " Somewhere in ### ");
	for (; n < i + 10 && CODE [n] != -1; n++)
		fprintf (errstream, "%s ", expand (CODE [n]));
	fprintf (errstream, " ###\n");
#ifdef	DEBUG
	if (debugflag.PARSE_ERRORS_SEGFAULT)
		*((int*)0)=0;
#endif
	exit (1);
}

NormPtr last_location;
Token in_function;

void parse_error_tok (Token t, const char *p)
{
	char *tmp = (char*) alloca (strlen (p) + strlen (expand (t)) + 8);
	sprintf (tmp, "%s : `%s'", p, expand (t));
	parse_error (last_location, tmp);
}

void parse_error_cpp (const char *p)
{
	fprintf (errstream, "Cpp error near line %i: %s\n", line, p);
	exit (1);
}

void parse_error_toktok (Token t1, Token t2, const char *p)
{
	char *tmp = (char*) alloca (strlen (p) + strlen (expand (t1)) + strlen (expand (t2)) + 9);
	sprintf (tmp, "%s : `%s' `%s'", p, expand (t1), expand (t2));
	parse_error (last_location, tmp);
}

void parse_error_pt (NormPtr i, Token t, const char *p)
{
	char *tmp = (char*) alloca (strlen (p) + strlen (expand (t)) + 8);
	sprintf (tmp, "%s : `%s'", p, expand (t));
	parse_error (i, tmp);
}

void parse_error_ll (const char *p)
{
	parse_error (last_location, p);
}

void warning_tok (const char *p, Token t)
{
	fprintf (errstream, "Warning %s:%i %s '%s'\n", c_file_of (last_location),
						       c_line_of (last_location), p, expand (t));
}
/*************************************************************
* print a type. debuging routine which shalln't be used at all
*************************************************************/

#ifdef DEBUG
void debug_pr_type (typeID t)
{
	int i;
	PRINTF ("type[%i]: ", t);
	int *tt = open_typeID (t);
	PRINTF ("(%i)", tt [0]);
	for (i = 1; tt [i] != -1; i++)
		if (tt [i] == '*' || tt [i] == '(' || tt [i] == '[')
			PRINTF ("%c", tt [i]);
		else PRINTF ("|%i|", tt [i]);
	PRINTF ("\n");
}
#endif

/******************************************************************************
	name
******************************************************************************/

void name_of_simple_type (Token *s, typeID t)
{
	recID r;
	switch (r = dbase_of (t)) {
	default: sintprintf (s, iRESERVED_struct (r), name_of_struct (r), -1);
#define Case(x,...) break; case x: sintprintf (s, __VA_ARGS__, -1);
	Case (B_SCHAR, RESERVED_char);
	Case (B_UCHAR, RESERVED_unsigned, RESERVED_char);
	Case (B_SSINT, RESERVED_short, RESERVED_int);
	Case (B_USINT, RESERVED_unsigned, RESERVED_short, RESERVED_int);
	Case (B_SINT, RESERVED_int);
	Case (B_UINT, RESERVED_unsigned, RESERVED_int);
	Case (B_SSIZE_T, RESERVED_ssz_t);
	Case (B_USIZE_T, RESERVED_usz_t);
	Case (B_SLONG, RESERVED_long);
	Case (B_ULONG, RESERVED_unsigned, RESERVED_long);
	Case (B_SLLONG, RESERVED_long, RESERVED_long);
	Case (B_ULLONG, RESERVED_unsigned, RESERVED_long, RESERVED_long);
	Case (B_FLOAT, RESERVED_float);
	Case (B_DOUBLE, RESERVED_double);
	Case (B_LDOUBLE, RESERVED_long, RESERVED_double);
#ifdef __LWC_HAS_FLOAT128
	Case (B_FLOAT128, RESERVED__Float128);
#endif
	Case (B_VOID, RESERVED_void);
	Case (B_PURE, -1);
#undef	Case
	}
}
/******************************************************************************
	miscellanery
******************************************************************************/

/* integer/float promotion */
typeID bt_promotion (typeID t)
{
	int *tt = open_typeID (t);
	if (tt [1] == '[') {
		int *tn = allocaint (intlen (tt) + 1);
		intcpy (tn, tt);
		tn [1] = '*';
		return enter_type (tn);
	}
	if (tt [0] >= 0 || tt [1] != -1)
		return t;
	if (tt [0] <= B_ULLONG)
		return typeID_int;
	if (tt [0] <= B_LDOUBLE)
		return typeID_float;
	return t;
}

/* turn a type to pointer to type */
typeID ptrup (typeID t)
{
	int *tt, *tn;
	tt = open_typeID (t);
	tn = allocaint (intlen (tt) + 2);
	tn [0] = tt [0];
	tn [1] = '*';
	intcpy (tn + 2, tt + 1);
	return enter_type (tn);
}

/* the effect of pointer indirection */
typeID ptrdown (typeID t)
{
	int *tt = open_typeID (t), *tn;
	if (tt [1] != '*' && tt [1] != '[')
		expr_error ("* not applicable");
	tn = allocaint (intlen (tt));
	tn [0] = tt [0];
	intcpy (tn + 1, tt + 2);
	return enter_type (tn);
}

/* base type of [...] */
typeID elliptic_type (typeID t)
{
	if (!typeID_elliptic (t))
		return t;
	int *tt = open_typeID (t), *tn;
	tn = allocaint (intlen (tt));
	tn [0] = tt [0];
	intcpy (tn + 1, tt + 2);
	return enter_type (tn);
}

/* given a function type return the return type */
typeID funcreturn (typeID t)
{
	int *tt = open_typeID (t), *tn, i;
	if (tt [1] != '(')
		expr_error ("bug: request return type of no function");
	for (i = 2; tt [i] != INTERNAL_ARGEND; i++);
	tn = allocaint (intlen (&tt [i]) + 2);
	tn [0] = tt [0];
	intcpy (tn + 1, tt + i + 1);
	return enter_type (tn);
}

/* take a function type and add the first rec* argument */
typeID makemember (typeID t, recID r)
{
	int *tt = open_typeID (t), *tn;
	if (tt [1] != '(')
		expr_error ("bug: request to makemember on no function");
	tn = allocaint (intlen (tt) + 4);
	tn [0] = tt [0];
	tn [1] = '(';
	tn [2] = pthis_of_struct (r);
	intcpy (tn + 3, tt + 2);
	return enter_type (tn);
}

/* remove the reference boost */
typeID dereference (typeID t)
{
	int *tt = open_typeID (t), *tn;
	if (tt [0] < REFERENCE_BASE)
		return t;
	tn = allocaint (intlen (tt) + 1);
	intcpy (tn, tt);
	tn [0] -= REFERENCE_BOOST;
	return enter_type (tn);
}

Token *build_type (typeID t, Token o, Token ret[]) 
{
/* XXX: elliptics */
	if (is_reference (t))
		t = ptrdown (dereference (t));

	Token tmp [100], *dcls = &tmp [20], *dcle = dcls;
	Token *st = open_typeID (t);
	int i = 1, b = 0;

	if (o) {
		*(++dcle) = -1;
		*dcls-- = o;
	} else  *dcls-- = -1;

	for (;;i++) {
		switch (st [i]) {
		case '*':
			*dcls-- = '*';
			b = 1;
			continue;
		case '[':
			if (b) *dcls-- = '(', *dcle++ = ')', b = 0;
			*dcle++ = '['; *dcle++ = ']';
			continue;
		case '(':
			if (b) *dcls-- = '(', *dcle++ = ')', b = 0;
			*dcle++ = '(';
			for (i++;;)
				if (st [i] == B_ELLIPSIS) {
					*dcle++ = ELLIPSIS;
					break;
				} else {
					if (st [i] == INTERNAL_ARGEND) break;
					Token arg [50];
					intcpy (dcle, build_type (st [i++], 0, arg));
					dcle += intlen (dcle);
					*dcle++ = ',';
				}
			if (dcle [-1] == ',') --dcle;
			*dcle++ = ')';
			continue;
		case -1: break;
		default: PRINTF ("UNKNWOWN %i\n", st [i]);
		}
		break;
	}
	*dcle = -1;

	if (st [0] >= 0)
		if (ISSYMBOL (st [0])) sintprintf (ret, st [0], -1);
		else sintprintf (ret, isunion (st [0]) ? RESERVED_union : iRESERVED_struct (st [0]),
			    name_of_struct (st [0]), -1);
	else switch (st [0]) {
	 case B_UCHAR:  sintprintf (ret, RESERVED_unsigned, RESERVED_char, -1);
	ncase B_SCHAR:  sintprintf (ret, RESERVED_char, -1);
	ncase B_USINT:  sintprintf (ret, RESERVED_unsigned, RESERVED_short, RESERVED_int, -1);
	ncase B_SSINT:  sintprintf (ret, RESERVED_short, RESERVED_int, -1);
	ncase B_UINT:   sintprintf (ret, RESERVED_unsigned, RESERVED_int, -1);
	ncase B_SINT:   sintprintf (ret, RESERVED_int, -1);
	ncase B_SSIZE_T:   sintprintf (ret, RESERVED_ssz_t, -1);
	ncase B_USIZE_T:   sintprintf (ret, RESERVED_usz_t, -1);
	ncase B_ULONG:  sintprintf (ret, RESERVED_unsigned, RESERVED_long, -1);
	ncase B_SLONG:  sintprintf (ret, RESERVED_long, -1);
	ncase B_ULLONG: sintprintf (ret, RESERVED_unsigned, RESERVED_long, RESERVED_long, -1);
	ncase B_SLLONG: sintprintf (ret, RESERVED_long, RESERVED_long, -1);
	ncase B_FLOAT:  sintprintf (ret, RESERVED_float, -1);
	ncase B_DOUBLE: sintprintf (ret, RESERVED_double, -1);
#ifdef __LWC_HAS_FLOAT128
	ncase B_FLOAT128: sintprintf (ret, RESERVED__Float128, -1);
#endif
	ncase B_VOID:   sintprintf (ret, RESERVED_void, -1);
	}

	intcat (ret, dcls + 1);

	return ret;
}

typeID typeof_designator (typeID t, Token d[])
{
	exprret E;
	Token x = internal_identifier1 ();
	Token expr [100];

	sintprintf (expr, x, ISTR (d), -1);
	expr [intlen (expr) - 1] = -1;
	open_local_scope ();
	enter_local_obj (x, t);
	parse_expression_string (intdup (expr), &E);
	close_local_scope ();

	if (!E.ok) return typeID_void;

	if (E.newExpr [0] == x)
		intcpy (d, E.newExpr + 1), intcatc (d, '=');
	free (E.newExpr);

	return E.type;
}

/***************************************************************
	overloadable
***************************************************************/
Token isunary_overloadable (Token t)
{
	switch (t) {
	case '!': return RESERVED_oper_excl;
	case '*': return RESERVED_oper_star;
	case '+': return RESERVED_oper_plus;
	case '-': return RESERVED_oper_minus;
	case '~': return RESERVED_oper_thingy;
	case POINTSAT: return RESERVED_oper_pointsat;
	case PLUSPLUS: return RESERVED_oper_plusplus;
	case MINUSMINUS: return RESERVED_oper_minusminus;
	}
	return 0;
}

Token isbinary_overloadable (Token t)
{
	switch (t) {
	case '%': return RESERVED_oper_mod;
	case '|': return RESERVED_oper_or;
	case '&': return RESERVED_oper_and;
	case '^': return RESERVED_oper_xor;
	case LSH: return RESERVED_oper_lsh;
	case RSH: return RESERVED_oper_rsh;
	case '*': return RESERVED_oper_mul;
	case '/': return RESERVED_oper_div;
	case '>': return RESERVED_oper_gr;
	case '<': return RESERVED_oper_le;
	case '+': return RESERVED_oper_add;
	case '-': return RESERVED_oper_sub;
	case ',': return RESERVED_oper_comma;
	case '[': return RESERVED_oper_array;
	case OROR: return RESERVED_oper_oror;
	case EQCMP: return RESERVED_oper_eq;
	case GEQCMP: return RESERVED_oper_greq;
	case LEQCMP: return RESERVED_oper_leq;
	case NEQCMP: return RESERVED_oper_neq;
	case ANDAND: return RESERVED_oper_andand;
	case '=': return RESERVED_oper_assign;
	case ASSIGNA: return RESERVED_oper_as_a;
	case ASSIGNS: return RESERVED_oper_as_s;
	case ASSIGNM: return RESERVED_oper_as_m;
	case ASSIGND: return RESERVED_oper_as_d;
	case ASSIGNR: return RESERVED_oper_as_r;
	case ASSIGNBA: return RESERVED_oper_as_ba;
	case ASSIGNBX: return RESERVED_oper_as_bx;
	case ASSIGNBO: return RESERVED_oper_as_bo;
	case ASSIGNRS: return RESERVED_oper_as_rs;
	case ASSIGNLS: return RESERVED_oper_as_ls;
	}
	return 0;
}

Token isunary_postfix_overloadable (Token t)
{
	switch (t) {
	case PLUSPLUS: return RESERVED_oper_plusplusp;
	case MINUSMINUS: return RESERVED_oper_minusminusp;
	}
	return 0;
}
/***************************************************************
	#include<>
***************************************************************/
Token include_sys_header_s (char *s)
{
	char tmp [128];
	sprintf (tmp, "\n#include <%s>\n", s);
	return new_symbol (strdup (tmp));
}
Token include_sys_header (Token t)
{
	char fn [128];
	strcpy (fn, expand (t) + 1);
	fn [strlen (fn) - 1] = 0;
	return include_sys_header_s (fn);
}
/***************************************************************
	alias ("func")
***************************************************************/
Token alias_func (recID r, Token f)
{
	char tmp [512];
	//xmark_function_USED (FSP (r), f);
	sprintf (tmp, "__attribute__ ((alias (\"%s\")))", expand (f));
	return new_symbol (strdup (tmp));
}
/***************************************************************
	linkonce section text
***************************************************************/
Token linkonce_data (Token s)
{
	if (NoLinkonce) return BLANKT;

	char tmp [512];
	sprintf (tmp, "__attribute__ ((__section__(\""SECTION_LINKONCE_DATA"%s\")))",
		expand (s));
	return new_symbol (strdup (tmp));
}

Token linkonce_data_f (Token s)
{
	if (NoLinkonce) return BLANKT;

	char tmp [512];
	sprintf (tmp, "__attribute__ ((__section__(\""SECTION_LINKONCE_DATA"%s_%s\")))",
		expand (in_function), expand (s));
	return new_symbol (strdup (tmp));
}

Token linkonce_text (Token s)
{
	if (NoLinkonce) return BLANKT;

	char tmp [512];
	sprintf (tmp, "__attribute__ ((__section__(\""SECTION_LINKONCE_TEXT"%s\")))",
		 expand (s));
	return new_symbol (strdup (tmp));
}

Token linkonce_rodata (Token s)
{
	if (NoLinkonce) return BLANKT;

	char tmp [512];
	sprintf (tmp, "__attribute__ ((__section__(\""SECTION_LINKONCE_RODATA"%s\")))",
		 expand (s));
	return new_symbol (strdup (tmp));
}

Token cleanup_func (Token s)
{
	char tmp [512];
	sprintf (tmp, "__attribute__ ((cleanup(%s)))",
		 expand (s));
	return new_symbol (strdup (tmp));
}

Token section_vtblz (Token t)
{
	char tmp [512];
	sprintf (tmp, "__attribute__ ((section (\".rodata.vtblz%s\")))", expand (t));
	return new_symbol (strdup (tmp));
}

/***************************************************************
	internal typeof 'expression'
***************************************************************/
typeID typeof_expression (NormPtr p, int i)
{
	exprret E;
	parse_expression (p, &E, i);
	if (!E.ok) parse_error (p, "can't proceed after expression error");
	free (E.newExpr);
	return E.type;
}
/***************************************************************
	This is used to do the -very useful- anonymous object
	C++ thingy.

	We are trying to determine wheter we have an object
	declaration or a constructor call expression.

	The check is lazy though. It will return true if
	the code is DEFINITELLY an expression.
	There are cases where it may or may not be.
	Return false and let programmer make it obvious.

	In a declaration valid characters are only
		( * ) [ ] const volatile
	only one symbol and anything inside the[]
***************************************************************/

bool is_expression (NormPtr p)
{
	NormPtr p2 = skip_parenthesis (++p);
	int nsym = 0;
	Token t;

	while (p < p2)
		switch (t = CODE [p++]) {
		case RESERVED_const:
		case RESERVED_volatile:
		case '(': case ')':
		case '*': continue;

		case '[': p = skip_brackets (p);
			  continue;

		default:
			if (ISSYMBOL (t))
				if (nsym) return true;
				else ++nsym;
			else return true;
		}
	return false;
}
/******************************************************************************
	As an exception to everything, __builtin_va_arg looks like a function,
	but the second argument is a type.  This breaks the entire parser
	code.  Adding a check to the parser for whether the function name
	is "__builtin_va_arg", is not worth it.

	This bogus here will replace the token string:
		'__builtin_va_arg' '(' 'var' ',' 'type' ')'
	With the pseudo value:
		"__builtin_va_arg (var,type)"

	which is internally treated as a constant value
	of integer type
******************************************************************************/

#define strspcat(x, y) strcat (strcat (x, " "), y)

void bogus1 ()
{
	Token t = Lookup_Symbol ("__builtin_va_arg");
	NormPtr p, pe, ps;
	int symi = 0;
	char tmpstr [256];
	char *newsym [1640];

	if (!t) return;

	for (p = 0; CODE [p] != -1; p++)
		if (CODE [p] == t) {
			pe = p;
			strcpy (tmpstr, expand (t));
			if (CODE [++p] != '(') parse_error (p, "__builtin_va_arg");
			ps = skip_parenthesis (p + 1);
			while (p < ps)
				strspcat (tmpstr, expand (CODE [p++]));
			CODE [pe++] = symi + c_nval + VALBASE;
			newsym [symi++] = strdup (tmpstr);
			ps = p;
			p = pe;
			while ((CODE [pe++] = CODE [ps++]) != -1);
		}
	add_extra_values (newsym, symi);
	CODE [++p] = -1;
}
