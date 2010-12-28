/***************************************************************************

	C preprocessor

The preprocessor is not a seperate stage but an extension to the lexical
analyser. This is possible and efficient since we avoid two passes of
the source file.

***************************************************************************/

#include "global.h"
#include "SYS.h"

#ifdef DO_CPP
//
// these are borrowed from lex.c
//
extern int skip_ws ();
extern int skip_comment ();
extern int skip_line ();
extern int enter_symbol (char*);
extern int enter_string (char*);

extern int do_yylex ();

static void skip_pp_line ()
{
	// Skip a line but respect that newlines inside:
	// strings, comments, char consts and
	// escaped ones don't count
	for (;;) {
		if (Ci >= Clen) return;

		switch (Cpp [Ci]) {
		 case '/':
			++Ci;
			if (Cpp [Ci] == '*') skip_comment ();
			else if (Cpp [Ci] == '/') skip_line ();
		ncase '\\':
			if (Cpp [++Ci] == '\n') {
				++Ci;
				++line;
			}
		ncase '\n': return;
		ncase '"':
			for (++Ci; Ci < Clen && Cpp [Ci] != '"'; Ci++)
				if (Cpp [Ci] == '\\') ++Ci;
				else if (Cpp [Ci] == '\n') ++line;
			++Ci;
		ncase '\'':
			for (++Ci; Ci < Clen && Cpp [Ci] != '\''; Ci++)
				if (Cpp [Ci] == '\\') ++Ci;
				else if (Cpp [Ci] == '\n') ++line;
			++Ci;
		ndefault:
			++Ci;
		}
	}
}

static Token *get_expanded_line ()
{
	Token t;

	SAVE_VAR (Ci, Ci);
	skip_pp_line ();
	SAVE_VAR (Clen, Ci+1);
	RESTOR_VAR (Ci);

	OUTSTREAM S = new_stream ();
	while ((t = do_yylex ()) != THE_END)
		if (ISSYMBOL (t))
			if (t == RESERVED_defined) {
				if (!issymbol (t = do_yylex ())
				&& (t != '(' || !issymbol (t = do_yylex ())
				|| do_yylex () != ')'))
					parse_error_cpp ("defined (symbol)");
				output_itoken (S, is_macro (t) == -1 ? RESERVED_0 : RESERVED_1);
			} else if (is_macro (t) != -1) {
				Token *E = expand_macro (t);
				outprintf (S, ISTR (E), -1);
				free (E);
			} else goto jmp;
		else jmp: output_itoken (S, t);

	RESTOR_VAR (Clen);
	return combine_output (S);
}

/******************************************************************************

	File inclusion

Search for the file in the usual places and then recurse back to yydo_file.
If the directive is "#uses", then enclose everything inside extern "file" {}

******************************************************************************/
static char **hsearch, **isearch;

static char *fixpath (char *path)
{
	char *p = path, *s, *e;

	while (*p)
	if (p [0] == '.')
		if (p [1] == '/') {
			s = p; e = p + 2;
			while (*s++ = *e++);
		} else if (p [1] == '.' && p [2] == '/' && p > path + 1 && p [-1] == '/') {
			e = p + 3;
			for (s = p - 2; *s != '/' && s > path; s--);
			p = *s == '/' ? ++s : s;
			while (*s++ = *e++);
		} else goto eelse;
	else eelse: ++p;
	return path;
}

static char *FindHeader (char *fn, char *ret, bool sys)
{
	char **p = sys ? isearch : hsearch;
	char tmp [256];

	for (; *p; p++) {
		tmp [0] = 0;
		if (!sys && (**p != '/' || *p [1] != ':' && *p [2] != '/')
		&& strchr (current_file, '/')) {
			strcpy (tmp, current_file);
			char *t = strrchr (tmp, '/');
			if (t) *(t + 1) = 0;
			else tmp [0] = 0;
		}
		strcat (tmp, *p);
		if (strlen (tmp) && tmp [strlen (tmp) - 1] != '/') strcat (tmp, "/");
		strcat (tmp, fn);

		fixpath (tmp);

		if (access (tmp, R_OK) == 0)
			return strcpy (ret, tmp);
	}

	fprintf (stderr, "Could not find file %s\n", fn);
	fatal ("terminated");
	return 0;
}

static intnode *included;

static void include (bool uses)
{
	int closer, i, t;
	char fn [128], path [256];

	skip_ws ();
	closer = Cpp [Ci] == '<' ? '>' : Cpp [Ci] == '"' ? '"' : 0;
	if (!closer) fatal ("#include");

	for (++Ci, i = 0; Cpp [Ci] != closer && Ci < Clen && i < sizeof fn;)
		fn [i++] = Cpp [Ci++];
	fn [i] = 0;

	if (Cpp [Ci++] != closer) fatal ("#include");

	FindHeader (fn, path, closer == '>');
	t = enter_value (path);

	if (!intfind (included, t)) {
		union ival u;
		intadd (&included, t, u);

		if (uses) {
			char path [256];
			sprintf (path, "\"%s\"", fn);
			outprintf (GLOBAL, RESERVED_extern, enter_value (path), '{', -1);
		}
	
		if (yydo_file (path) == -1) {
			fprintf (stderr, "Problematic file [%s]\n", fn);
			fatal ("Can't open file");;
		}

		if (uses) output_itoken (GLOBAL, '}');
	}
}

/******************************************************************************

	Macro Definitions && undefs

******************************************************************************/
typedef struct {
	int argc;
	Token *body;
} macro;

static intnode *macrotree;

int is_macro (Token t)
{
	intnode *n;

	if (n = intfind (macrotree, t))
		return ((macro*) n->v.p)->argc;

	return ISPREDEF (t) ? 0 : -1;
}

static void define ()
{
	int argc = 0, i, margc = 0;
	Token n = do_yylex (), argv [100], t;
	macro *m;
	intnode *mn;

	if (!ISSYMBOL (n)) fatal ("#define");

#ifdef	DEBUG
	if (debugflag.CPP) {
		PRINTF ("define ["COLS"%s"COLE"]\n", expand (n));
	}
#endif

	if (Cpp [Ci] == '(') {
		for (++Ci;;) {
			t = do_yylex ();
			if (t == ')') break;
			if (t == ELLIPSIS) {
				if (do_yylex () != ')')
					parse_error_cpp ("#define X(arg,...) : C99 style");
				break;
			}
			if (!ISSYMBOL (t)) fatal ("#define args");
			argv [argc++] = t;
			t = do_yylex ();
			if (t == ')') break;
			if (t == ELLIPSIS) fatal ("gnu varags not supported");
			if (t != ',') fatal ("#define args separator");
		}
		margc = t == ELLIPSIS ? argc + VARBOOST : argc ?: VOIDARG;
	}

	SAVE_VAR (Ci, Ci);
	skip_pp_line ();
	SAVE_VAR (Clen, Ci+1);
	RESTOR_VAR (Ci);

	OUTSTREAM Def = new_stream ();
	while ((t = do_yylex ()) != THE_END) {
		for (i = 0; i < argc; i++)
			if (argv [i] == t) {
				t = i + ARGBASE;
				break;
			}
		if (t == RESERVED___VA_ARGS__)
			t = argc + ARGBASE;
		output_itoken (Def, t);
	}
	m = (macro*) malloc (sizeof *m);
	m->body = combine_output (Def);
	m->argc = margc;
	if (mn = intfind (macrotree, n)) {
		macro *h = (macro*) mn->v.p;
		if (h->argc != m->argc || intcmp (h->body, m->body))
			fatal ("different macro redefinition");
		free (m->body);
		free (m);
	} else {
		union ival u = { .p m };
		intadd (&macrotree, n, u);
	}

	RESTOR_VAR (Clen);
}

void undef ()
{
	Token m = do_yylex ();
	intnode *n = intfind (macrotree, m);

	if (n) {
		free (((macro*) n->v.p)->body);
		intremove (&macrotree, n);
		free (n);
	}
}

/******************************************************************************
	cpp expression evaluation (#if, #elif)
		a mini-compiler inside the compiler...

	right now works only with integers. floats and other stuff
	evaluate to 1L. symbols evaluate to 0L
******************************************************************************/
static Token *eval_expr;
static NormPtr eep;

static long long int cpp_expression ();

static long long int cpp_cond_getnum ()
{
	Token s = eval_expr [eep++];

	switch (s) {
	case '!': return !cpp_cond_getnum ();
	case '(': return cpp_expression ();
	case '-': return -cpp_cond_getnum ();
	case '~': return ~cpp_cond_getnum ();
	}

	if (ISVALUE (s))
		return type_of_const (s) == typeID_int ?
			eval_intll (s) : 1;

	return 0;
}

static int op_pri (int type)
{
	/* oh not again */
	switch (type) {
		case -1:
		case ')': return 9;
		case ANDAND:
		case OROR: return 8;
		case '|': return 7;
		case '^': return 6;
		case '&': return 5;
		case NEQCMP:
		case EQCMP: return 4;
		case '<':
		case '>':
		case GEQCMP:
		case LEQCMP: return 3;
		case RSH:
		case LSH: return 2;
		case '+':
		case '-': return 1;
		case '*':
		case '/':
		case '%': return 0;
		default: return -1;
	}
}

#define CALC(operator) \
/* PRINTF ("calculating %lli %s %lli\n", Arr [ce-1], #operator, Arr[ce+1]); */\
	Arr [ce - 1] = Arr [ce - 1] operator Arr [ce + 1];\
	break;

static long long int cpp_expressionr ()
{
	long long int Arr [64];
	int pri, ce;

	Arr [0] = 1;

	/* DO NOT TRY THIS AT HOME */
	for (;;) {
		Arr [Arr [0]] = cpp_cond_getnum ();
		pri = op_pri (eval_expr [eep]);
		if (pri == -1)
			if (eval_expr [eep] == '?' || eval_expr [eep] == ':')
				return Arr [1];
			else parse_error_cpp ("confusing expression in #if");
		while (Arr [0] > 1)
			if (op_pri (Arr [ce = Arr [0] - 1]) <= pri) {
				switch (Arr [ce]) {
				case '*': CALC (*);
				case '/': CALC (/);
				case '%': CALC (%);
				case '+': CALC (+);
				case '-': CALC (-);
				case RSH: CALC (>>);
				case LSH: CALC (<<);
				case '<': CALC (<);
				case '>': CALC (>);
				case EQCMP: CALC (==);
				case NEQCMP: CALC (!=);
				case GEQCMP: CALC (>=);
				case LEQCMP: CALC (<=);
				case '&': CALC (&);
				case '|': CALC (|);
				case '^': CALC (^);
				case ANDAND: CALC (&&);
				}
				Arr [0] -= 2;
			} else break;
		if (pri == 9) return Arr [1];
		if (pri == 8) {
			if (eval_expr [eep] == ANDAND && Arr [1] == 0) {
				for (++eep; eval_expr [eep] != ')' && eval_expr [eep] != OROR
				&& eval_expr [eep] != -1; eep++)
				if (eep == '(')
					 eep = skip_buffer_parenthesis (eval_expr, eep + 1) - 1;
				if (eval_expr [eep] != OROR) return 0;
				++eep;
				Arr [0] = 1;
				continue;
			}
			if (eval_expr [eep] == OROR && Arr [1] != 0) {
				for (++eep; eval_expr [eep] != ')' && eval_expr [eep] != -1; eep++)
				if (eep == '(')
					 eep = skip_buffer_parenthesis (eval_expr, eep + 1) - 1;
				return 1;
			}
		}
		Arr [++Arr [0]] = eval_expr [eep++];
		++Arr [0];
	}
}

static long long int cpp_expression ()
{
static	bool expectcond;

	long long int rez = cpp_expressionr ();
	switch (eval_expr [eep]) {
	case ')': ++eep; break;
	case '?': {
		++eep;
		SAVE_VAR (expectcond, true);
		long long int r1 = cpp_expression ();
		RESTOR_VAR (expectcond);
		rez = rez ? r1 : cpp_expression ();
	} break;
	case ':': if (expectcond) ++eep;
		  else parse_error_cpp ("':' without '?'");
	ndefault: if (expectcond) parse_error_cpp ("missing ':'");
	}
	return rez;
}

static bool cpp_condition ()
{
	bool r;
	eval_expr = get_expanded_line ();
#ifdef	DEBUG
	if (debugflag.CPP) {
		PRINTF ("Conditional expression :");
		INTPRINT (eval_expr);
		PRINTF ("\n");
	}
#endif
	eep = 0;
	r = cpp_expression () != 0;
	free (eval_expr);
	return r;
}

/******************************************************************************
	Conditional compilation
		don't try this at home....
******************************************************************************/
static int conditional_pp;

static void skip_to_endif (bool checkelse)
{
	Token t;

	for (;;) {
		t = do_yylex ();

		if (t != CPP_DIRECTIVE) {
			skip_pp_line ();
			if (Ci >= Clen)
				parse_error_cpp ("#if without endif");
			continue;
		}

		t = do_yylex ();

		if (t == RESERVED_endif) break;

		if (t == RESERVED_if || t == RESERVED_ifdef || t == RESERVED_ifndef) {
			skip_to_endif (false);
			continue;
		}

		if (!checkelse) continue;
		if (t == RESERVED_else) {
			skip_pp_line ();
			++conditional_pp;
			return;
		}

		if (t == RESERVED_elif)
			if (cpp_condition ()) {
				++conditional_pp;
				return;
			}
	}
}

static bool ifdef ()
{
	Token m = do_yylex ();
	return is_macro (m) != -1;
}

/******************************************************************************
		Miscellanery
******************************************************************************/
static void cpp_error ()
{
	int ii = Ci;
	skip_pp_line ();
	Ci -= ii;
	char *msg = (char*) alloca (2 + Ci);
	strncpy (msg, Cpp + ii, Ci);
	msg [Ci] = 0;
	fprintf (stderr, "ERROR: %s\n", msg);
	exit (1);
}

/******************************************************************************

	Process a preprocessing line directive

******************************************************************************/

void cpp_directive ()
{
	if (skip_ws ()) return;

	Token t = do_yylex ();

	switch (t) {
	 case RESERVED_include:
	 case RESERVED_uses:
		include (t == RESERVED_uses);
	ncase RESERVED_define:
		define ();
	ncase RESERVED_undef:
		undef ();
	ncase RESERVED_ifdef:
		if (ifdef ()) ++conditional_pp;
		else skip_to_endif (true);
	ncase RESERVED_ifndef:
		if (!ifdef ()) ++conditional_pp;
		else skip_to_endif (true);
	ncase RESERVED_else:
		if (!conditional_pp)
			parse_error_cpp ("#else not in conditional");
		--conditional_pp;
		skip_to_endif (false);
	ncase RESERVED_endif:
		if (!conditional_pp)
			parse_error_cpp ("#endif without #startif");
		--conditional_pp;
		skip_pp_line ();
	ncase RESERVED_if:
		if (!cpp_condition ()) skip_to_endif (true);
		else ++conditional_pp;
	ncase RESERVED_elif:
		if (!conditional_pp) parse_error_cpp ("#elif without #if");
		--conditional_pp;
		skip_to_endif (false);
	ncase RESERVED_error:
		cpp_error ();
	ncase RESERVED_line:
	ndefault:
		skip_pp_line ();
	//	PRINTF ("not implemented\n");
	}
}

void setup_cpp (int argc, char **argv)
{
	int i, j, id;
	char *Idir [128];

	for (id = i = 0; i < argc; i++)
	if (argv [i][0] == '-')
		switch (argv [i][1]) {
		case 'I':
			Idir [id++] = argv [i][2] == 0 ? argv [++i] : argv [i] + 2;
		}

	i = 1 + id + sizeof search_dirs / sizeof search_dirs [0];
	hsearch = (char**) malloc (i * sizeof *hsearch);
	hsearch [0] = "";
	for (i = 1, j = 0; j < id; j++)
		hsearch [i++] = strdup (Idir [j]);
	for (j = 0; search_dirs [j]; j++)
		hsearch [i++] = (char*) search_dirs [j];
	hsearch [i] = 0;
	isearch = hsearch + 1;
}

static void freeintnode (intnode *n)
{
	if (n->less) freeintnode (n->less);
	if (n->more) freeintnode (n->more);
	free (n);
}

static void freemacronode (intnode *n)
{
	if (n->less) freeintnode (n->less);
	if (n->more) freeintnode (n->more);
	free (((macro*)n->v.p)->body);
	free (n);
}

void cleanup_cpp ()
{
	free (hsearch);
	if (included) freeintnode (included);
	if (macrotree) freemacronode (macrotree);
}

/************************************************************************

	Macro expansions

We deviate from the standard which sais that ``macro arguments are
expanded *before* the macro expansion unless they're followed by
'#' or '##' and then there is a second rescan to reexpand'' doh

************************************************************************/

static Token *expand_predef (Token m)
{
static	Token timetok;
	char tmp [129];
	Token *ret = mallocint (2);
	ret [1] = -1;

	switch (m) {
	 case RESERVED___LINE__:
		sprintf (tmp, "%i", line);
		ret [0] = enter_value (tmp);
	ncase RESERVED___FILE__:
		sprintf (tmp, "\"%s\"", tfile);
		ret [0] = enter_string (tmp);
	ncase RESERVED___TIME__:
	 case RESERVED___DATE__:
		if (!timetok) {
			time_t t = time (0);
			sprintf (tmp, "\"%s", ctime (&t));
			*(strchr (tmp, '\n')) = '"';
			timetok = enter_string (tmp);
			if (strstr (tmp, "Jan 1 "))
				fputs ("Happy new year!\n", stderr);
		}
		ret [0] = timetok;
	}
	return ret;
}

static Token stringize (Token arg [])
{
	char *tmp, *s;
	int l, i, j;

	for (l = 0, i = 4; arg [l] != -1; l++) {
		i += strlen (s = expand (arg [l])) + 1;
		for (j = 0; s [j]; j++)
			if (s [j] == '\\'
			||  s [j] == '"') i++;
	}

	tmp = (char*) malloc (i);
	tmp [0] = '"';
	for (l = 0, i = 1; arg [l] != -1; l++) {
		s = expand (arg [l]);
		while (*s) {
			if (*s == '\\' || *s == '"')
				tmp [i++] = '\\';
			tmp [i++] = *s++;
		}
		if (arg [l + 1] != -1) tmp [i++] = ' ';
	}
	tmp [i++] = '"';
	tmp [i] = 0;

	return enter_string (tmp);
}

static Token concat (OUTSTREAM S, Token a1[], Token a2[])
{
	int i, l;
	Token con, t;

	if (a1 [i = 0] != -1)
		while (a1 [i + 1] != -1)
			output_itoken (S, a1 [i++]);

	if (a2 [0] == -1) return a1 [i];

	{
#		define EXPAND(x) expand (x == -1 ? BLANKT : x)
		char *tmp;
		tmp = strcat (strcpy (
		(char*) malloc (l = strlen (EXPAND (a1 [i])) + strlen (EXPAND (a2 [0])) + 1),
		EXPAND (a1 [i])), EXPAND (a2 [0]));

		if ((ISIDENT (a1 [i]) || a1 [i] == -1)
		&&  (ISIDENT (a2 [0]) || a2 [0] == -1))
			con = enter_symbol (tmp);
		else {
			SAVE_VAR (Cpp, tmp);
			SAVE_VAR (Ci, 0);
			SAVE_VAR (Clen, l - 1);
			con = 0;
			while ((t = do_yylex ()) != THE_END) {
				if (con) output_itoken (S, con);
				con = t;
			}
			RESTOR_VAR (Cpp);
			RESTOR_VAR (Ci);
			RESTOR_VAR (Clen);
		}
	}

	if (a2 [1] == -1) return con;

	output_itoken (S, con);
	for (i = 1; a2 [i + 1] != -1; i++)
		output_itoken (S, a2 [i]);

	return a2 [i];
}

static Token *expanding, *tops;
static Token *expand_macro_arglist (Token, int, Token*);

static Token *expand_macro_argv (Token m, Token **argv)
{
	Token *B = ((macro*) intfind (macrotree, m)->v.p)->body;
	Token a1 [] = { 0, -1 }, a2 [] = { 0, -1 }, *a1p, *a2p, b;
	int i, n;
	OUTSTREAM S = new_stream ();

	/* stringnify and concatenate */
	for (;(b = *B++) != -1;) {
		if (b == '#') {
			b = *B++;
			if (!ISTPLARG (b))
				parse_error_cpp ("'#' not followed by macro argument");
			output_itoken (S, stringize (argv [b - ARGBASE]));
			continue;
		}
		if (*B == CPP_CONCAT) {
			while (*B == CPP_CONCAT) {
				if (!ISTPLARG (b)) a1 [0] = b;
				a1p = ISTPLARG (b) ? argv [b - ARGBASE] : a1;
				b = B [1];
				if (b == -1) parse_error_cpp ("'##' is last");
				if (!ISTPLARG (b)) a2 [0] = b;
				a2p = ISTPLARG (b) ? argv [b - ARGBASE] : a2;
				b = concat (S, a1p, a2p);
				B += 2;
			}
			output_itoken (S, b);
			continue;
		} 
		if (ISTPLARG (b))
			outprintf (S, ISTR (argv [b - ARGBASE]), -1);
		else output_itoken (S, b);
	}

	B = combine_output (S);
	for (i = 0; B [i] != -1; i++)
		if (is_macro (B [i]) != -1)
			goto further_expands;
	return B;

further_expands:

	/* macro contains further macros, non-recusrive */
	*tops++ = m;
	*tops = -1;

	S = new_stream ();
	for (i = 0; B [i] != -1; i++)
		if ((n = is_macro (B [i])) == -1) noexpand:
			output_itoken (S, B [i]);
		else {
			Token m, *E;

			m = B [i];
			for (E = expanding; *E != -1; E++)
				if (*E == m)
					goto noexpand;

			if (ISPREDEF (m))
				E = expand_predef (m);
			else if (n == 0)
				E = expand_macro_argv (m, 0);
			else {
				if (B [++i] != '(') parse_error_cpp ("No arguments to macro");
				E = expand_macro_arglist (m, n, &B [++i]);
				i = skip_buffer_parenthesis (B, i) - 1;
			}
			outprintf (S, ISTR (E), -1);
			free (E);
		}

	*--tops = -1;
	free (B);
	return combine_output (S);
}

static Token *expand_macro_arglist (Token m, int argn, Token *s)
{
	int argc, i;
	Token **parg;
	NormPtr pstart, pend;

	for (i = 0, argc = 2; s [i] != ')'; i++)
		if (s [i] == ',') ++argc;
	parg = (Token**) alloca (argc * sizeof (Token*));

	pstart = 0, argc = 0;
	for (;;) {
		pend = pstart;
		while (s [pend] != ',' && s [pend] != ')')
			if (s [pend] == '(')
				pend = skip_buffer_parenthesis (s, pend + 1);
			else if (s [pend] == -1)
				parse_error_cpp ("Incomplete macro call");
			else ++pend;
		parg [argc] = allocaint (2 + pend - pstart);
		intextract (parg [argc++], s + pstart, pend - pstart);
		if (s [pend] == ')') break;
		pstart = pend + 1;
	}
	parg [argc] = 0;

	if (argc != argn)
		if (argn < VOIDARG || argn == VOIDARG && argc != 1
		|| (argn >= VARBOOST && argn - VARBOOST > argc))
			 parse_error_cpp ("Argument number mismatch");

	if (argn >= VARBOOST) {
		int i, l;
		int j = argn - VARBOOST;
		Token *varg;
		for (i = j, l = 10; parg [i]; i++)
			l += intlen (parg [i]) + 1;
		varg = allocaint (l);
		varg [0] = -1;
		for (i = j; parg [i]; i++) {
			intcat (varg, parg [i]);
			if (parg [i + 1]) intcatc (varg, ',');
		}
		parg [j] = varg;
		parg [j + 1] = 0;
	}

	return expand_macro_argv (m, parg);
}

Token *expand_macro (Token m)
{
#ifdef	DEBUG
	if (0&&debugflag.CPP)
		PRINTF ("expanding macro "COLS"%s"COLE"\n", expand (m));
#endif
	if (ISPREDEF (m))
		return expand_predef (m);

	Token rarray [64];
	tops = expanding = rarray;
	int argn = is_macro (m);

	if (!argn) return expand_macro_argv (m, 0);

	if (do_yylex () != '(') parse_error_cpp ("No arguments to macro");

	Token arglist [256], t;
	int i = 0;

	while ((arglist [i++] = t = do_yylex ()) != ')')
		switch (t) {
		case THE_END: parse_error_cpp ("Incomplete arglist");
		case '(': {
			int po = 1;
			while (po) switch (t = arglist [i++] = do_yylex ()) {
				 case '(': ++po;
				ncase ')': --po;
				ncase THE_END: parse_error_cpp ("Incomplete arglist");
			}
		} }

	arglist [i] = -1;
	return expand_macro_arglist (m, argn, arglist);
}
#endif
