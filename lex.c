#include "global.h"
#include "SYS.h"

int *CODE, c_ntok, c_nsym;

/*********************************************************
		expanding array of itokens
*********************************************************/

static void checkline ();
int enter_value (char *txt);

static int concate_str (int s1, int s2)
{
	char s [30];
	sprintf (s, "%%%i+%i", s1, s2);
	return enter_value (s);
}

static void enter_itoken (int i)
{
#define STORE_TOK(x) { output_itoken (GLOBAL, x); ++c_ntok; }
static	int pstring;
static	int perlopst;
static	int pointsat;

	if (!i) return;

	/* =~ is valid C. If followed by string literal, sematically invalid though. Do perlop */
	if (perlopst) {
		if (perlopst == 1) {
			if (i == '~') {
				perlopst = 2;
				return;
			} else {
				STORE_TOK ('=')
				perlopst = 0;
			}
		} else {
			if (i >= STRBASE)
				STORE_TOK (PERLOP)
			else  { STORE_TOK ('=') STORE_TOK ('~') }
			perlopst = 0;
		}
	} else if (i == '=') {
		perlopst = 1;
		if (pstring) {
			STORE_TOK (pstring)
			pstring = 0;
		} else if (pointsat) {
			STORE_TOK (POINTSAT)
			pointsat = 0;
		}
		return;
	}

	if (pointsat) {
		switch (i) {
		case POINTSAT: case MINUSMINUS: case ASSIGNA: case ASSIGNS: case PLUSPLUS:
		case GEQCMP: case OROR: case ANDAND: case EQCMP: case NEQCMP: case LEQCMP:
		case '[': case '*': case '+': case '-': case '!': case '=': case '<': case '>':
		case '~':
			i += ESCBASE;
		ndefault:
			STORE_TOK (POINTSAT)
		}
		pointsat = 0;
	} else if (i == POINTSAT) {
		pointsat = 1;
		if (pstring) {
			STORE_TOK (pstring)
			pstring = 0;
		}
		return;
	}

	if (i >= STRBASE) {
		i -= STRBASE - VALBASE;
		pstring = pstring ? concate_str (pstring, i) : i;
		return;
	}

	if (pstring) {
		STORE_TOK (pstring)
		pstring = 0;
	}

	if (ISRESERVED (i)) {
		if (i == RESERVED_true) i = enter_value ("1");
		else if (i == RESERVED_false) i = enter_value ("0");
		else if (i == RESERVED___extension__) return;
		else if (i == RESERVED_bool) i = RESERVED_int;
		else if (i == RESERVED_alloca) i = INTERN_alloca;
	}

	STORE_TOK (i)

	checkline ();
}

int processor;

int enter_string (char *txt)
{
	bool escape_quotes = *txt == '\'';
	if (!processor && !escape_quotes)
		return enter_value (txt) + STRBASE - VALBASE;
	int r;
	if (processor) {
		txt = TP [processor] (txt + 1, strlen (txt + 1) - 1);
printf ("Willdo [%s]\n", txt);
		r = processor = 0;
		yydo_mem (txt, strlen (txt));
	} else {
		txt = escape_q_string (txt + 1, strlen (txt + 1) - 1);
		r = enter_value (txt) + STRBASE - VALBASE;
	}
	free (txt);
	return r;
}
/***************************************************************
		Symbol/value tables
	we'll take our chances with a simple binary tree
***************************************************************/

typedef struct snode_t {
	struct snode_t *less, *more;
	const char *txt;
	int id;
} snode;

static snode *symbol_tree, *value_tree;

static int symbol_inc, value_inc;
int c_nval;

static snode *newnode (const char *txt, int id)
{
	snode *nn = (snode*) malloc (sizeof (snode));
	nn->less = nn->more = NULL;
	nn->txt = txt;
	nn->id = id;
	return nn;
}

static char **c_symbol, **c_value;
static int csym_alloc, cval_alloc;
 
static int addsym (snode **nn, char *txt)
{
	*nn = newnode (txt = strdup (txt), symbol_inc);
	if (csym_alloc == c_nsym) {
		c_symbol = (char**) realloc (c_symbol, (csym_alloc += 1024) * sizeof (char*));
		memset (c_symbol + c_nsym, 0, 1024 * sizeof (char*));
	}
	if (!c_symbol [symbol_inc - IDENTBASE]) {
		c_symbol [symbol_inc - IDENTBASE] = txt;
		++c_nsym;
	}
	return symbol_inc++;
}

int enter_symbol (char *txt)
{
	snode *nn;
	int c;

	if (!(nn = symbol_tree))
		return addsym (&symbol_tree, txt);

	for (;;)
		if (!(c = strcmp (txt, nn->txt))) return nn->id;
		else if (c < 0)
			if (nn->less) nn = nn->less;
			else return addsym (&nn->less, txt);
		else
			if (nn->more) nn = nn->more;
			else return addsym (&nn->more, txt);

}

static int addval (snode **nn, char *txt)
{
	*nn = newnode (txt = strdup (txt), value_inc);
	if (cval_alloc == c_nval)
		c_value = (char**) realloc (c_value, (cval_alloc += 1024) * sizeof (char*));
	c_value [value_inc - VALBASE] = txt;
	++c_nval;
	return value_inc++;
}

int enter_value (char *txt)
{
	snode *nn;
	int c;

	if (!(nn = value_tree))
		return addval (&value_tree, txt);

	for (;;)
		if (!(c = strcmp (txt, nn->txt))) return nn->id;
		else if (c < 0)
			if (nn->less) nn = nn->less;
			else return addval (&nn->less, txt);
		else
			if (nn->more) nn = nn->more;
			else return addval (&nn->more, txt);

}
/*********************************************************
		Linenumbers
*********************************************************/

typedef struct {
	int line, p;
} toklin;

static toklin*	linenumber;
static int	nlinenumber;

#define TLCHUNK 1024

typedef struct ms_tl_t {
	struct ms_tl_t *next;
	toklin data [TLCHUNK];
	int i;
} ms_tl;

static ms_tl *lfirst, *llast;

static void store_line (int line)
{
	if (llast->i == TLCHUNK) {
		llast->next = (ms_tl*) malloc (sizeof (ms_tl));
		llast = llast->next;
		llast->i = 0;
		llast->next = NULL;
	}
	llast->data [llast->i].line = line;
	llast->data [llast->i++].p = c_ntok;
	++nlinenumber;
}

int line = 1;

static void checkline ()
{
	static int pline = -1;
	if (line != pline)
		store_line (pline = line);
}

static void static_lines ()
{
	int i, k;
	ms_tl *p, *n;
	
	linenumber = (toklin*) malloc (nlinenumber * sizeof (toklin));

	for (k = 0, p = lfirst; p; p = n) {
		for (i = 0; i < p->i; i++)
			linenumber [k++] = p->data [i];
		n = p->next;
		free (p);
	}
}

/* once linenumbers are frozen into one array */
int c_line_of (int p)
{
	int s = 0, e = nlinenumber - 1, m;

	while (e - s > 1)
		if (linenumber [m = (e + s) / 2].p <= p)
			s = m;
		else	e = m;
	return linenumber [e].line;
}
/*********************************************************
		File marks
*********************************************************/

typedef struct {
	char *file;
	int p;
} filemark;

static filemark	*files;
static int	nfiles = -1;

static void store_file (char *file)
{
	if (nfiles == -1) {
		files [++nfiles].file = strdup (file);
		files [nfiles].p = c_ntok;
		return;
	}
	if (!strcmp (file, files [nfiles].file)) return;
	if (files [nfiles].p == c_ntok) {
		free (files [nfiles].file);
		files [nfiles].file = strdup (file);
	} else {
		files [++nfiles].file = strdup (file);
		files [nfiles].p = c_ntok;
	}
}

static void static_files ()
{
	files = realloc (files, ++nfiles * sizeof (filemark));
}

/* once files are frozen into one array */
char *c_file_of (int p)
{
	int s = 0, e = nfiles, m;

	while (e - s > 1)
		if (files [m = (e + s) / 2].p < p)
			s = m;
		else	e = m;
	return files [s].file;
}

/******************************************************************************
	-- fix the file/line to token data
******************************************************************************/
void adjust_lines (NormPtr p, int adj)
{
	int i;
	for (i = 0; i < nfiles; i++)
		if (files [i].p > p) break;
	while (i < nfiles)
		files [i++].p += adj;
	// we fix not the line numbering
}
/******************************************************************************
	-- lexical analyser
******************************************************************************/
char *Cpp;
int Ci, Clen;
/******************************************************************************

******************************************************************************/
static const signed char ll_ctypes [256] = {
	['a']=2, ['b']=2, ['c']=2, ['d']=2, ['e']=4, ['f']=3, ['g']=2, ['h']=2,
	['i']=2, ['j']=2, ['k']=2, ['l']=3, ['m']=2, ['n']=2, ['o']=2, ['p']=4,
	['q']=2, ['r']=2, ['s']=2, ['t']=2, ['u']=3, ['v']=2, ['w']=2, ['x']=2,
	['y']=2, ['z']=2,
	['A']=2, ['B']=2, ['C']=2, ['D']=2, ['E']=4, ['F']=3, ['G']=2, ['H']=2,
	['I']=2, ['J']=2, ['K']=2, ['L']=3, ['M']=2, ['N']=2, ['O']=2, ['P']=4,
	['Q']=2, ['R']=2, ['S']=2, ['T']=2, ['U']=3, ['V']=2, ['W']=2, ['X']=2,
	['Y']=2, ['Z']=2,
	['_']=2,
	['0']=1, ['1']=1, ['2']=1, ['3']=1, ['4']=1, ['5']=1, ['6']=1, ['7']=1,
	['8']=1, ['9']=1,
};

#define ISEXPON(x) (ll_ctypes [(int)x] == 4)
#define ISNIEND(x) (ll_ctypes [(int)x] == 3)
#define ISALPHA(x) (ll_ctypes [(int)x] >= 2)
#define ISDIGIT(x) (ll_ctypes [(int)x] == 1)
#define ISALNUM(x) (ll_ctypes [(int)x] >  0)

//

void skip_comment ()
{
	Ci += 2;

	for (;;) {
		while (Cpp [Ci] != '*') {
			if (Cpp [Ci++] == '\n') ++line;
			if (Ci > Clen) fatal ("unterminated comment");
		}
		if (Cpp [++Ci] != '/') continue;
		break;
	}

	++Ci;
}

void skip_line ()
{
	for (;;) {
		while (Cpp [Ci] != '\n')
			if (++Ci > Clen) return;
		if (Cpp [Ci - 1] == '\\') {
			++line;
			++Ci;
			continue;
		}
		break;
	}
}

int skip_ws ()
{
	int cl = 0;

	for (;;) {
		for (;;) {
			if (Cpp [Ci] == ' ' || Cpp [Ci] == '\t') {
				if (++Ci >= Clen) return cl;
				continue;
			}
			if (Cpp [Ci] == '\n') {
				++line;
				cl = 1;
				if (++Ci >= Clen) return cl;
				continue;
			}
			break;
		}
		if (Cpp [Ci] == '/') {
			if (Cpp [Ci + 1] == '*')
				skip_comment ();
			else if (Cpp [Ci + 1] == '/')
				skip_line ();
			else return cl;
			continue;
		}
		if (Cpp [Ci] == '\\' && Cpp [Ci + 1] == '\n') {
			Ci += 2;
			++line;
			continue;
		}
		return cl;
	}
}

static int loadtext_directive ()
{
	char filename [128], *p = filename;
	int Cis = Ci + 127 > Clen ? Clen : Ci + 127;

	skip_ws ();
	if (Ci >= Clen || Cpp [Ci++] != '<') fatal (expand (RESERVED__loadtext));
	while (Cpp [Ci] != '>') {
		*p++ = Cpp [Ci++];
		if (Ci >= Cis) fatal ("filename too long");
	}
	*p = 0, ++Ci;

	if (!(p = loadtext (filename))) {
		fprintf (stderr, "Can't open file %s\n", filename);
		fatal ("");
	}
	return enter_string (p);
}

static int random_directive ()
{
	char tmp [100];
	sprintf (tmp, "%i", rand ());
	return enter_value (tmp);
}

#ifdef DO_CPP
#define CATCH_BS_GHOST \
	if (Cpp [Ci] == '\\' && Cpp [Ci + 1] == '\n') {\
		Ci += 2;\
		++line;\
		continue;\
	}
#else
#define CATCH_BS_GHOST
#endif

static inline int get_ident ()
{
	char identstr [64], *p = identstr;
	int Cis = Ci + 63 > Clen ? Clen + 1: Ci + 63, i;

	for (;;) {
		while (ISALNUM (Cpp [Ci])) {
			*p++ = Cpp [Ci++];
			if (Ci >= Cis) fatal ("identifier out of file");
		}
		CATCH_BS_GHOST;
		break;
	}
	*p = 0;

	if ((i = enter_symbol (identstr)) == RESERVED__loadtext)
		return loadtext_directive ();
	if (i == RESERVED__LWC_RANDOM_)
		return random_directive ();
	return i;
}

static inline int get_vident ()
{
	char identstr [64], *p = identstr;
	int Cis = Ci + 63 > Clen ? Clen : Ci + 63;

	for (;;) {
		while (ISALNUM (Cpp [Ci])) {
			*p++ = Cpp [Ci++];
			if (Ci >= Cis) fatal ("identifier out of file");
		}
		CATCH_BS_GHOST;
		break;
	}
	*p = 0;
	return enter_value (identstr);
}

static int get_string ()
{
	char *p = &Cpp [Ci], *a;
	int len, qchar;

	Ci += (*p == 'L') ? 2 : 1;
	qchar = Cpp [Ci - 1];
	for (;;) {
		if (qchar == '"')
			while (Cpp [Ci] != '\\' && Cpp [Ci] != '"')
				{ if (++Ci >= Clen) fatal ("forgoten \""); }
		else
			while (Cpp [Ci] != '\\' && Cpp [Ci] != '\'')
				if (++Ci >= Clen) fatal ("forgoten '");
		if (Cpp [Ci] == '\\') {
			if (Cpp [++Ci] == '\n') ++line;
			if (++Ci >= Clen) fatal ("forgotten \"");
			continue;
		}
		break;
	}

	len = &Cpp [++Ci] - p;
	a = (char*) alloca (len + 1);
	strncpy (a, p, len);
	a [len] = 0;
	return enter_string (a);
}

static int get_char_const ()
{
	char cchar [5], *p = cchar;
	int Cis = Ci + 4 > Clen ? Clen : Ci + 4, Cisv = Ci;

	if ((*p++ = Cpp [Ci++]) == 'L')
		*p++ = Cpp [Ci++];

	while (Cpp [Ci] != '\'') {
		if ((*p++ = Cpp [Ci]) == '\\')
			*p++ = Cpp [++Ci];
		//if (++Ci >= Cis) fatal ("character constant too long");
		if (++Ci >= Cis) {
			Ci = Cisv;
			return get_string ();
		}
	}
	*p++ = Cpp [Ci++];
	*p = 0;
	return enter_value (cchar);
}

static inline int get_nconst ()
{
	char nombre [64], *p = nombre;
	int Cis = Ci + 31 > Clen ? Clen : Ci + 31;

	for (;;) {
		while (ISALNUM (Cpp [Ci]) /* XXX && Cpp [Ci] != '_' */) {
			*p++ = Cpp [Ci++];
			if (Ci >= Cis) fatal ("numeric constant too long");
		}
		CATCH_BS_GHOST;
		break;
	}

	if (Cpp [Ci] == '.') {
		*p++ = Cpp [Ci++];
		for (;;) {
			while (ISDIGIT (Cpp [Ci])) {
				*p++ = Cpp [Ci++];
				if (Ci >= Cis) fatal ("numeric constant too long");
			}
			CATCH_BS_GHOST;
			break;
		}
	} else if (ISEXPON (Cpp [Ci])) {
		*p++ = Cpp [Ci++];
		if (Cpp [Ci] == '-' || Cpp [Ci] == '+')
			*p++ = Cpp [Ci++];
		for (;;) {
			while (ISDIGIT (Cpp [Ci])) {
				*p++ = Cpp [Ci++];
				if (Ci >= Cis) fatal ("numeric constant too long");
			}
			CATCH_BS_GHOST;
			break;
		}
	}

	while (ISNIEND (Cpp [Ci])) {
		*p++ = Cpp [Ci++];
		if (Ci >= Cis) fatal ("numeric constant too long");
	}
	*p = 0;

	return enter_value (nombre);
}
/******************************************************************************
	-- main lex loop
******************************************************************************/
static void preproc_line ()
{
	// # <line> "file"
	char file [256], *p1, *p2;
	int l;

	if (skip_ws ()) return;

	if (Cpp [Ci++] == 'p') goto skippy;
	p1 =  (Cpp [++Ci] == 'l') ? strchr (&Cpp [Ci], ' ') : &Cpp [Ci];
	line = strtol (p1, NULL, 10);
	p1 = strchr (&Cpp [Ci], '"') + 1;
	p2 = strchr (p1, '"');
	l = p2 - p1;
	Ci = p2 - Cpp;
        if(l >= sizeof(file))
                fatal ("line directive filename too big");
	strncpy (file, p1, l);
	file [l] = 0;
	store_file (file);
	skippy:
	while (Cpp [Ci] != '\n' && Ci < Clen)
		++Ci;
}

int do_yylex ()
{
	int r;
Again:
	if (Ci >= Clen)
		return THE_END;
	skip_ws ();
	if (Ci >= Clen)
		return THE_END;

	if (ISDIGIT (Cpp [Ci]))
		return get_nconst ();
	else if (ISALPHA (Cpp [Ci]))
		if (Cpp [Ci] == 'L')
			if (Cpp [Ci + 1] == '\'') goto Lchar;
			else if (Cpp [Ci + 1] == '"') goto Lstring;
			else goto eelse;
		else if (in2 (Cpp [Ci + 1], '"', '\''))
			goto Qstring;
		else eelse:	// bad goto. bad bad bad
			return get_ident ();
	else switch (r = Cpp [Ci]) {
		 case '~': case ')':
		 case ';': case ',':
		 case '?': case ':':
		 case '[': case ']':
		 case '{': case '}': case '(':
			++Ci;
		ncase '*':
			if (Cpp [++Ci] == '=') {
				r = ASSIGNM;
				++Ci;
			}
			break;
		Qstring: processor = Cpp [Ci++];
		Lstring:
		case '"':
			return get_string ();
		ncase '\'':
		Lchar:
			return get_char_const ();
		ncase '/':
			if (Cpp [++Ci] == '=') {
				r = ASSIGND;
				++Ci;
			}
		ncase '.':
			if (ISDIGIT (Cpp [Ci + 1]))
				return get_nconst ();
			else if (Cpp [++Ci] == '.') {
				if (Cpp [++Ci] != '.')
					fatal ("the ellipsis is three dots");
				r = ELLIPSIS;
				++Ci;
			}
		ncase '-':
			switch (Cpp [++Ci]) {
			 case '>':  r = POINTSAT; ++Ci;
			ncase '-': r = MINUSMINUS; ++Ci;
			ncase '=': r = ASSIGNS; ++Ci;
			}
		ncase '+':
			switch (Cpp [++Ci]) {
			 case '+':  r = PLUSPLUS; ++Ci;
			ncase '=': r = ASSIGNA; ++Ci;
			}
		ncase '!':
			if (Cpp [++Ci] == '=') {
				r = NEQCMP;
				++Ci;
			}
		ncase '%':
			if (Cpp [++Ci] == '=') {
				r = ASSIGNR;
				++Ci;
			}
		ncase '^':
			if (Cpp [++Ci] == '=') {
				r = ASSIGNBX;
				++Ci;
			}
		ncase '&':
		 case '|':
			++Ci;
			if (Cpp [Ci] == r) {
				r = r == '&' ? ANDAND : OROR;
				++Ci;
			} else if (Cpp [Ci] == '=') {
				r = r == '&' ? ASSIGNBA : ASSIGNBO;
				++Ci;
			}
		ncase '=':
			if (Cpp [++Ci] == '=') {
				r = EQCMP;
				++Ci;
			}
		ncase '<':
		 case '>':
			++Ci;
			if (Cpp [Ci] == r) {
				if (Cpp [++Ci] == '=') {
					++Ci;
					r = r == '>' ? ASSIGNRS : ASSIGNLS;
				} else r = r == '>' ? RSH : LSH;
			} else if (Cpp [Ci] == '=') {
				++Ci;
				r = r == '>' ? GEQCMP : LEQCMP;
			}
		ncase '$':
			if (ISALPHA (Cpp [++Ci]))
				return get_vident ();
		ncase '#':
#ifdef	DO_CPP
			if (!Ci || Cpp [Ci - 1] == '\n')
				r = CPP_DIRECTIVE;
			else if (Cpp [Ci + 1] == '#') {
				++Ci;
				r = CPP_CONCAT;
			}
			++Ci;
#else
			++Ci;
			preproc_line ();
			goto Again;
#endif
		ncase '\n':
			++line;
		 case '\r':
		 case '\f':
			++Ci;
			goto Again;
		ndefault:
			fprintf (stderr, "Ci = %c(%i) at %i/%i\n", Cpp [Ci], Cpp [Ci], Ci, line);
			fatal ("invalid character");
	}

	return r;
}
/******************************************************************************
		Initialization
******************************************************************************/
#define ENTER_SYMBOL(x) \
	symbol_inc = RESERVED_ ## x;\
	enter_symbol (#x);
#define ALIAS_LEX(x, y) \
	symbol_inc = RESERVED_ ## x;\
	enter_symbol (#y);
#define ENTER_VALUE(x) \
	value_inc = RESERVED_ ## x;\
	enter_value (#x);

static void calc_binshift ();

Token RESERVED_attr_stdcall;

void initlex ()
{
	lfirst = llast = (ms_tl*) malloc (sizeof (ms_tl));
	lfirst->next = NULL;
	lfirst->i = 0;
	symbol_inc = 0;
	files = (filemark*) malloc (1024 * sizeof (filemark));
	ENTER_SYMBOL (inline);
	ENTER_SYMBOL (do);
	ENTER_SYMBOL (struct);
	ENTER_SYMBOL (case);
	ENTER_SYMBOL (for);
	ENTER_SYMBOL (short);
	ENTER_SYMBOL (union);
	ENTER_SYMBOL (sizeof);
	ENTER_SYMBOL (register);
	ENTER_SYMBOL (break);
	ENTER_SYMBOL (auto);
	ENTER_SYMBOL (continue);
	ENTER_SYMBOL (const);
	ENTER_SYMBOL (default);
	ENTER_SYMBOL (enum);
	ENTER_SYMBOL (else);
	ENTER_SYMBOL (extern);
	ENTER_SYMBOL (goto);
	ENTER_SYMBOL (if);
	ENTER_SYMBOL (long);
	ENTER_SYMBOL (return);
	ENTER_SYMBOL (signed);
	ENTER_SYMBOL (static);
	ENTER_SYMBOL (switch);
	ENTER_SYMBOL (typedef);
	ENTER_SYMBOL (unsigned);
	ENTER_SYMBOL (linkonce);
	ENTER_SYMBOL (volatile);
	ENTER_SYMBOL (while);
	ENTER_SYMBOL (void);
	ENTER_SYMBOL (int);
	ENTER_SYMBOL (char);
	ENTER_SYMBOL (float);
	ENTER_SYMBOL (double);
	ENTER_SYMBOL (modular);
	ENTER_SYMBOL (class);

	ENTER_SYMBOL (ssz_t);
	ENTER_SYMBOL (usz_t);
#ifdef __LWC_HAS_FLOAT128
	ENTER_SYMBOL (_Float128);
#endif
	// extensive
	ENTER_SYMBOL (__asm__);
	ENTER_SYMBOL (__extension__);
	ENTER_SYMBOL (__attribute__);
	ENTER_SYMBOL (__restrict);
	ENTER_SYMBOL (__thread);
	ENTER_SYMBOL (__unwind__);
	ENTER_SYMBOL (__byvalue__);
	ENTER_SYMBOL (__noctor__);
	// gnu damage
	ALIAS_LEX (typeof, __typeof);
	ALIAS_LEX (const, __const);
	ALIAS_LEX (inline, __inline__);
	ALIAS_LEX (signed, __signed__);
	ALIAS_LEX (volatile, __volatile__);
	ALIAS_LEX (inline, __inline);
	ALIAS_LEX (const, __const__);
	ALIAS_LEX (__asm__, asm);
	ALIAS_LEX (__attribute__, __attribute);
	// our own reserved words
	ENTER_SYMBOL (true);
	ENTER_SYMBOL (false);
	ENTER_SYMBOL (template);
	ENTER_SYMBOL (bool);
	ENTER_SYMBOL (this);
	ENTER_SYMBOL (new);
	ENTER_SYMBOL (delete);
	ENTER_SYMBOL (localloc);
	ENTER_SYMBOL (virtual);
	ENTER_SYMBOL (operator);
	ENTER_SYMBOL (try);
	ENTER_SYMBOL (throw);
	ENTER_SYMBOL (benum);
	ENTER_SYMBOL (typeof);
	ENTER_SYMBOL (specialize);
	ENTER_SYMBOL (postfix);
	ENTER_SYMBOL (dereference);
	ENTER_SYMBOL (RegExp);
	ENTER_SYMBOL (final);
	ENTER_SYMBOL (__declexpr__);
	ENTER_SYMBOL (_lwc_config_);
	ENTER_SYMBOL (__C__);
	Ci = 0;
	// pseudo reserved symbols
	ENTER_SYMBOL (include);
	ENTER_SYMBOL (define);
	ENTER_SYMBOL (undef);
	ENTER_SYMBOL (endif);
	ENTER_SYMBOL (ifdef);
	ENTER_SYMBOL (ifndef);
	ENTER_SYMBOL (elif);
	ENTER_SYMBOL (error);
	ENTER_SYMBOL (line);
	ENTER_SYMBOL (uses);
	symbol_inc = RESERVED___VA_ARGS__;
	enter_symbol ("__VA_ARGS__");
	ENTER_SYMBOL (defined);
	ENTER_SYMBOL (__LINE__);
	ENTER_SYMBOL (__FILE__);
	ENTER_SYMBOL (__TIME__);
	ENTER_SYMBOL (__DATE__);
	ENTER_SYMBOL (_);
	ENTER_SYMBOL (ctor);
	ENTER_SYMBOL (_i_n_i_t_);
	ENTER_SYMBOL (nothrow);
	ENTER_SYMBOL (alias);
	ENTER_SYMBOL (used);
	ENTER_SYMBOL (dtor);
	ENTER_SYMBOL (malloc);
	ENTER_SYMBOL (free);
	ENTER_SYMBOL (alloca);
	ENTER_SYMBOL (__builtin_alloca);
	ALIAS_LEX (alloca, __builtin_alloca);
	ENTER_SYMBOL (private);
	ENTER_SYMBOL (public);
	ENTER_SYMBOL (__typeof__);
	ENTER_SYMBOL (__enumstr__);
	ENTER_SYMBOL (__inset__);
	ENTER_SYMBOL (_v_p_t_r_);
	ENTER_SYMBOL (_CLASS_);
	ENTER_SYMBOL (typeid);
	ENTER_SYMBOL (jmp_buf);
#ifdef	HAVE_BUILTIN_SETJMP
	ALIAS_LEX (setjmp, __builtin_setjmp);
	ALIAS_LEX (longjmp, __builtin_longjmp);
#else
	ENTER_SYMBOL (setjmp);
	ENTER_SYMBOL (longjmp);
#endif
	ENTER_SYMBOL (__on_throw__);
	ENTER_SYMBOL (__emit_vtbl__);

	ENTER_SYMBOL (__section__);
	ENTER_SYMBOL (noreturn);
	ENTER_SYMBOL (__label__);
	ENTER_SYMBOL (__lwc_unwind);
	ENTER_SYMBOL (__lwc_landingpad);
	ENTER_SYMBOL (p);
	ENTER_SYMBOL (a);
	ENTER_SYMBOL (pos);
	ENTER_SYMBOL (len);
	ENTER_SYMBOL (constructor);
	ENTER_SYMBOL (memcpy);
	ENTER_SYMBOL (__builtin_memcpy);
	ENTER_SYMBOL (strncmp);
	ENTER_SYMBOL (strncasecmp);
	ENTER_SYMBOL (__builtin_strncmp);
	ENTER_SYMBOL (__builtin_strncasecmp);
	ENTER_SYMBOL (_loadtext);
	ENTER_SYMBOL (_LWC_RANDOM_);
	ENTER_SYMBOL (__FUNCTION__);
	ENTER_SYMBOL (__PRETTY_FUNCTION__);
	ENTER_SYMBOL (size_t);
	ENTER_SYMBOL (wchar_t);
	ENTER_SYMBOL (min);
	ENTER_SYMBOL (max);
	ENTER_SYMBOL (charp_len);
	ENTER_SYMBOL (abbrev);
	ENTER_SYMBOL (strlen);
	ENTER_SYMBOL (x);
	ENTER_SYMBOL (X);
	ENTER_SYMBOL (y);
	ENTER_SYMBOL (i);
	ENTER_SYMBOL (j);
	ENTER_SYMBOL (s);
	ENTER_SYMBOL (main);
	ENTER_SYMBOL (oper_plus);
	ENTER_SYMBOL (oper_minus);
	ENTER_SYMBOL (oper_thingy);
	ENTER_SYMBOL (oper_fcall);
	ENTER_SYMBOL (oper_comma);
	ENTER_SYMBOL (oper_mod);
	ENTER_SYMBOL (oper_or);
	ENTER_SYMBOL (oper_and);
	ENTER_SYMBOL (oper_xor);
	ENTER_SYMBOL (oper_lsh);
	ENTER_SYMBOL (oper_rsh);
	ENTER_SYMBOL (oper_mul);
	ENTER_SYMBOL (oper_div);
	ENTER_SYMBOL (oper_andand);
	ENTER_SYMBOL (oper_oror);
	ENTER_SYMBOL (oper_as_m);
	ENTER_SYMBOL (oper_as_d);
	ENTER_SYMBOL (oper_as_r);
	ENTER_SYMBOL (oper_as_ba);
	ENTER_SYMBOL (oper_as_bx);
	ENTER_SYMBOL (oper_as_bo);
	ENTER_SYMBOL (oper_as_rs);
	ENTER_SYMBOL (oper_as_ls);
	ENTER_SYMBOL (oper_star);
	ENTER_SYMBOL (oper_excl);
	ENTER_SYMBOL (oper_array);
	ENTER_SYMBOL (oper_plusplus);
	ENTER_SYMBOL (oper_minusminus);
	ENTER_SYMBOL (oper_plusplusp);
	ENTER_SYMBOL (oper_minusminusp);
	ENTER_SYMBOL (oper_add);
	ENTER_SYMBOL (oper_sub);
	ENTER_SYMBOL (oper_gr);
	ENTER_SYMBOL (oper_le);
	ENTER_SYMBOL (oper_greq);
	ENTER_SYMBOL (oper_leq);
	ENTER_SYMBOL (oper_eq);
	ENTER_SYMBOL (oper_neq);
	ENTER_SYMBOL (oper_assign);
	ENTER_SYMBOL (oper_as_a);
	ENTER_SYMBOL (oper_as_s);
	ENTER_SYMBOL (oper_pointsat);
	ENTER_VALUE  (0);
	ENTER_VALUE  (1);
	ENTER_VALUE  (3);
	value_inc = RESERVED_C; enter_value ("\"C\"");
	calc_binshift ();
	RESERVED_attr_stdcall = enter_symbol ("__attribute__((stdcall))");
	GLOBAL = new_stream ();
}

/******************************************************************************
	binshift values
******************************************************************************/
int binshift [32];

static void calc_binshift ()
{
	char tmp [20];
#define S(x) sprintf (tmp, "0x%x", 1 << x); binshift [x] = enter_value (tmp);
	S (16) S (0)  S (31) S (8)  S (24) S (4)  S (12) S (20) S (28) S (2)
	S (6)  S (10) S (14) S (18) S (22) S (26) S (30) S (1)  S (3)  S (5)
	S (7)  S (9)  S (11) S (13) S (15) S (17) S (19) S (21) S (23) S (25)
	S (27) S (29)
#undef S
}
/******************************************************************************
	yydo interface
******************************************************************************/
void fatal (const char *m)
{
	fprintf (stderr, "lex-error: %s\n", m);
	exit (1);
}

char *tfile = "-no file-";

void yydo_mem (char *data, int len)
{
	int token;

	SAVE_VAR (Clen, len);
	SAVE_VAR (Cpp, data);
	SAVE_VAR (Ci, 0);

	while ((token = do_yylex ()) != THE_END)
		if (token == CPP_DIRECTIVE)
#ifdef DO_CPP
			if (!sys_cpp) cpp_directive ();
			else
#endif
				preproc_line ();
#ifdef DO_CPP
		else if (ISSYMBOL (token) && is_macro (token) != -1) {
			Token *E = expand_macro (token);
			int i;
			for (i = 0; E [i] != -1; i++)
				enter_itoken (E [i]);
			free (E);
		}
#endif
			else enter_itoken (token);

	RESTOR_VAR (Clen);
	RESTOR_VAR (Cpp);
	RESTOR_VAR (Ci);
}

int yydo_file (char *file)
{
#ifdef	DEBUG
static	int depth;
	if (debugflag.CPP) {
		int i;
		for (i = depth++; i; --i) PRINTF (" ");
		PRINTF ("lex on file ["COLS"%s"COLE"]\n", file);
	}
#endif

	struct load_file L;
	ctor_load_file_ (&L, file);

	if (!L.success) return -1;

	SAVE_VAR (line, 1);
	SAVE_VAR (tfile, file);
	SAVE_VAR (current_file, file);
	store_file (tfile);

	yydo_mem (L.data, L.len);

	RESTOR_VAR (line);
	RESTOR_VAR (tfile);
	RESTOR_VAR (current_file);
	store_file (tfile);

	dtor_load_file_ (&L);

#ifdef	DEBUG
	if (debugflag.CPP) --depth;
#endif

	return 0;
}

int yydo (char *file)
{
	if (yydo_file (file) == -1)
		return -1;
	enter_itoken (';');
	enter_itoken (-1);
	enter_itoken (-1);
	CODE = combine_output (GLOBAL);

	static_lines ();
	static_files ();

	return 0;
}
/******************************************************************************
	-- Dynamic symbols
	new identifiers may be generated during the translation.
	store these in a dynamic symbol table and make them printable
	by expand() below.
******************************************************************************/

#define DCHUNK 512

static char **dynsym;
static int ndynsym, dynsymalloc;
static snode *ns_tree;

static snode *lookup_dynsym (const char *s)
{
	snode *n = ns_tree;
	snode *r = (snode*) malloc (sizeof *r);
	int c;

	r->less = r->more = 0;
	r->id = -1;
	r->txt = s;

	if (!n) return ns_tree = r;

	for (;;)
		if ((c = strcmp (s, n->txt)) == 0) {
			free (r);
			return n;
		} else if (c > 0)
			if (n->less) n = n->less;
			else return n->less = r;
		else	if (n->more) n = n->more;
			else return n->more = r;
}

Token new_symbol (char *s)
{
	snode *z = lookup_dynsym (s);
	if (z->id != -1) {
		free (s);
		return DYNBASE + z->id;
	}
	if (ndynsym == dynsymalloc) {
		dynsymalloc += DCHUNK;
		dynsym = realloc (dynsym, dynsymalloc * sizeof (char*));
	}
	dynsym [z->id = ndynsym] = s;
	return DYNBASE + ndynsym++;
}

Token new_value_int (int i)
{
	char tmp [20];
	sprintf (tmp, "%i", i);
	//return enter_value (strdup (tmp));
	return enter_value (tmp);
}

Token new_value_string (char *s)
{
	return enter_value (escape_c_string (s, strlen (s)));
}

Token stringify (Token t)
{
	char tmp [512];
	sprintf (tmp, "\"%s\"", expand (t));
	return enter_value (strdup (tmp));
}

Token token_addchar (Token t, int c)
{
	char tmp [512];
	sprintf (tmp, "%s%c", expand (t), c);
	return enter_symbol (strdup (tmp));
}
/******************************************************************************
	-- Add bogus values
******************************************************************************/
void add_extra_values (char **v, int n)
{
	c_value = realloc (c_value, (c_nval + n) * sizeof v[0]);
	memcpy (c_value + c_nval, v, n * sizeof v[0]);
	c_nval += n;
}
/******************************************************************************
	-- Lookup what is a constant
******************************************************************************/
typeID type_of_const (Token c)
{
	char *t = c_value [c - VALBASE];
	if (t [0] == '"' || (t [0] == 'L' && t [1] == '"') || t [0] == '%')
		return typeID_charP;
	if (t [0] == '\'')
		return typeID_int;
	if (ISALPHA(t [0]))
		return typeID_int;
	/* XXX: checkout, '.' missing but 'e' present */
	return strchr (t, '.') ? typeID_float : typeID_int;
}

bool is_literal (Token c)
{
	return ISVALUE (c) && c_value [c - VALBASE][0] == '"';
}
/******************************************************************************
	-- Evaluate constant values
******************************************************************************/
int eval_int (Token c)
{
	if (!ISVALUE (c) || type_of_const (c) != typeID_int)
		parse_error_ll ("Integer value expected");
	/* XXX: evaluate 'a', 'b', '0' character constants */
	return strtol (c_value [c - VALBASE], 0, 10);
}

long long int eval_intll (Token c)
{
	if (!ISVALUE (c) || type_of_const (c) != typeID_int)
		parse_error_ll ("Integer value expected");
	return strtoll (c_value [c - VALBASE], 0, 10);
}
/******************************************************************************
	-- Find specific symbols
	This is very rare. We care for special names which however are not
	reserved. Like "__FUNCTION__" "main", etc.
	The search in N, but this is ok (for now at least)
******************************************************************************/
Token Lookup_Symbol (const char *s)
{
	int i;
	for (i = 0; i < c_nsym; i++)
		if (!strcmp (s, c_symbol [i]))
			return i + IDENTBASE;
	return 0;
}
/******************************************************************************
	-- C expand
	expand the integer normalized tokens to strings
******************************************************************************/

#define pstr(x) escop ? "->" x : x
#define rcase(x) case RESERVED_ ## x: return #x
#define ocase(x, y) case x: return y
#define pcase(x, y) case x: return pstr (y)

char *expand (int token)
{
	bool escop = 0;
	if (token >= IDENTBASE) {
		if (token < DYNBASE)
			return (token - IDENTBASE < c_nsym) ?
				c_symbol [token - IDENTBASE] : "/*bug*/";
		if (token >= STRBASE) token -= STRBASE - VALBASE;
		if (token >= VALBASE) {
			char *v = token - VALBASE < c_nval ? c_value [token - VALBASE] : "/*BUG*/";
			if (*v != '%') return v;
			int t1, t2;
			t1 = strtol (v + 1, &v, 10);
			t2 = strtol (v + 1, 0, 10);
			char *s1 = expand (t1), *s2 = expand (t2);
			char *s3 = strcpy (c_value [token - VALBASE] = (char*)
				 malloc (strlen (s1) + strlen (s2) + 2), s1);
			strcpy (s3 + strlen (s3) - 1, s2 + 1);
			return s3;
		}
		if (token >= ARGBASE) return "*argument*";
		return dynsym [token - DYNBASE];
	}
	if ((escop = token >= ESCBASE)) token -= ESCBASE;
	if (token > 256) switch (token) {
		ocase (ELLIPSIS, "...");
		pcase (POINTSAT, "->");
		pcase (MINUSMINUS, "--");
		pcase (ASSIGNA, "+=");
		pcase (ASSIGNS, "-=");
		ocase (ASSIGNM, "*=");
		ocase (ASSIGND, "/=");
		ocase (ASSIGNR, "%=");
		ocase (ASSIGNBA, "&=");
		ocase (ASSIGNBO, "|=");
		ocase (ASSIGNBX, "^=");
		ocase (ASSIGNRS, ">>=");
		ocase (ASSIGNLS, "<<=");
		ocase (PERLOP, "=~");
		pcase (PLUSPLUS, "++");
		pcase (GEQCMP, ">=");
		ocase (LSH, "<<");
		pcase (OROR, "||");
		pcase (ANDAND, "&&");
		pcase (EQCMP, "==");
		pcase (NEQCMP, "!=");
		ocase (RSH, ">>");
		pcase (LEQCMP, "<=");
		ocase (MARKER, "-MARKER-");
		ocase (DBG_MARK, "\n/*+*+*+*+*+*+*+*+*+*+*+*+*/\n");
		ocase (NOOBJ, "/*not-an-object*/");
		default: return "n/A\n";
		case NOTHING:
		case UWMARK: return "";
	}
	switch (token) {
		case '(': return "(";
		case ')': return ")";
		case '[': return pstr ("[");
		case ']': return "]";
		case ';': return ";";
		case ',': return ",";
		case ':': return ":";
		case '~': return pstr ("~");
		case '?': return "?";
		case '{': return "{";
		case '}': return "}";
		case '.': return ".";
		case '*': return pstr ("*");
		case '/': return "/";
		case '+': return pstr ("+");
		case '-': return pstr ("-");
		case '!': return pstr ("!");
		case '%': return "%";
		case '^': return "^";
		case '&': return "&";
		case '|': return "|";
		case '=': return pstr ("=");
		case '<': return pstr ("<");
		case '>': return pstr (">");
		case '"': return "\"";
		case '#': return "#";
		case CPP_CONCAT: return "##";
		case THE_END: return "/*End of unit*/";
	}
	if (token < 0) return token == BLANKT ? "" : "/*-1*/";
	return "plonk!";
}
