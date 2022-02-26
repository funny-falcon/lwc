/******************************************************************************
	Regular Expression compiler

some kind of state machine is definitelly used. A parser too!
******************************************************************************/

#include <ctype.h>
#include "global.h"

static bool	regexp_case;
static int	regexp_partition = 255;
static bool	regexp_packed;
static int	regexp_INFTY = 32000;
static bool	regexp_noextract;
static bool	regexp_noctbl = 1;
static char*	regexp_recipe;
static bool	regexp_anon;
static bool	regexp_strfuncs = 0;


#ifdef DEBUG
void regexp_print (int *retons)
{
	int i;

	for (i = 0; retons [i]; i++)
		if (retons [i] > 255)
			PRINTF (" [%i] ", retons [i]);
		else if (retons [i] > 31)
			PRINTF (" %c ", retons [i]);
		else
			PRINTF (" -%i ", retons [i]);

	PRINTF ("\n");
}
#endif

void rerror (char *s) {
	fprintf (stderr, "regexp error: %s\n", s);
	fprintf (stderr, "regexp error: In \"%s\"\n", regexp_recipe); 
	exit (1);
}


enum {
	RE_SPECIAL_SSTART = 256,	// ^
	RE_SPECIAL_SEND,		// $
	RE_SPECIAL_OPARENTH,		// (
	RE_SPECIAL_CPARENTH,		// )
	RE_SPECIAL_OBRAK,		// [
	RE_SPECIAL_ONBRAK,		// [^
	RE_SPECIAL_RANGE,		// -
	RE_SPECIAL_CBRAK,		// ]
	RE_SPECIAL_DOT,			// .
	RE_SPECIAL_QUESTION,		// ?
	RE_SPECIAL_STAR,		// *
	RE_SPECIAL_PLUS,		// +
	RE_SPECIAL_OR,			// |
	RE_SPECIAL_OBRACE,		// {
	RE_SPECIAL_CBRACE,		// }
	RE_SPECIAL_GRANTED,		// ?/

	RE_SPECIAL_UCLASS,		// \c

	RE_SPECIAL_STORP = 512,
	RE_SPECIAL_CLASS = 2048
};

static inline int isstorp (int i)
{ return i >= RE_SPECIAL_STORP && i < RE_SPECIAL_CLASS; }
static inline int isuclass (int i)
{ return i >= RE_SPECIAL_UCLASS && i < RE_SPECIAL_STORP; }


#define OFFS(x) x-'a'
static struct {
	char *def;
	int op;
} uclass [OFFS ('z')] = {
	 [OFFS ('w')] = { "[a-zA-Z0-9_]", 0 },
	 [OFFS ('s')] = { "[ \t\r\n\f]", 0 },
	 [OFFS ('d')] = { "[0-9]", 0 }
};

typedef int *regstr;



static int escape (char *r, int i, regstr R)
{
#define EPARSE(x, y) break; case x: *R = y; return i;
#define ESCAPE(x) EPARSE (x, x)

	switch (r [i]) {
	default: {
		char *endptr = endptr;
		if (r [i] == 'x' && isxdigit (r [i+1]))
			*R = strtol (r + i + 1, &endptr, 16);
		else if (r [i] == '0')
			*R = strtol (r + i, &endptr, 8);
		else if (r [i] >= '1' && r [i] <= '9')
			rerror ("Backreferences *not* supported");
		else {
			if (isalpha (r [i]))
				*R = RE_SPECIAL_UCLASS + r [i];
			else rerror ("undefined escape sequence");
			return i;
		}
		return endptr - r - 1;
	}
	EPARSE ('n', '\n'); EPARSE ('t', '\t'); EPARSE ('f', '\f');
	EPARSE ('r', '\r'); EPARSE ('a', '\a');
	ESCAPE ('('); ESCAPE (')'); ESCAPE (']'); ESCAPE ('[');
	ESCAPE ('{'); ESCAPE ('}'); ESCAPE ('.'); ESCAPE ('?');
	ESCAPE ('*'); ESCAPE ('+'); ESCAPE ('-'); ESCAPE ('^');
	ESCAPE ('$'); ESCAPE ('|'); ESCAPE ('\\');
	}
}

static int nextr;

static void regexp_descape (char *r, regstr R)
{
#define XPARSE(x,y) break; case x: R [j++] = y
	int i, j, np, b;
	for (np = i = j = 0; r [i]; i++)
		switch (r [i]) {
		default:  R [j++] = r [i];
		XPARSE ('^', RE_SPECIAL_SSTART);
		XPARSE ('$', RE_SPECIAL_SEND);
		XPARSE ('.', RE_SPECIAL_DOT);
		XPARSE ('?', RE_SPECIAL_QUESTION);
		XPARSE ('*', RE_SPECIAL_STAR);
		XPARSE ('+', RE_SPECIAL_PLUS);
		XPARSE ('|', RE_SPECIAL_OR);
		ncase ')': R [j++] = np ? (np--, RE_SPECIAL_CPARENTH) : ')';
		XPARSE ('(', RE_SPECIAL_OPARENTH);
			np++;
			b = regexp_noextract;
			if (r [i + 1] == '?')
				switch (r [i += 2]) {
				default: rerror ("embedded modifier not supported");
				ncase '/': R [j++] = RE_SPECIAL_GRANTED;
				case ':': b = 1;
				}
			if (!b) R [j++] = RE_SPECIAL_STORP + nextr++;
		ncase '[':
			if (r [++i] == '^') {
				R [j++] = RE_SPECIAL_ONBRAK;
				++i;
			} else R [j++] = RE_SPECIAL_OBRAK;
			if (r [i] == '-')
				R [j++] = r [i++];
			for (; r [i]; i++) switch (r [i]) {
			default: R [j++] = r [i];
			ncase ']': R [j++] = RE_SPECIAL_CBRAK; goto close1;
			case '-': R [j++] = RE_SPECIAL_RANGE;
			ncase '\\': i = escape (r, i + 1, &R[j++]);
			}
			close1:
			if (R [j - 2] == RE_SPECIAL_RANGE)
				R [j - 2] = '-';
		ncase '{': {
			char *endptr = endptr;
			R [j++] = RE_SPECIAL_OBRACE;
			++i;
			if (r [i] < '0' || r [i] > '9')
				rerror ("no number after '{'");
			R [j++] = strtol (r + i, &endptr, 10);
			if (r [i = endptr - r] == ',') {
				++i;
				R [j++] = ',';
				if (r [i] >= '0' && r [i] <= '9') {
					R [j++] = strtol (r + i, &endptr, 10);
					i = endptr - r;
				}
			}
			if (r [i] != '}') rerror ("Unclosed '{'");
			R [j++] = RE_SPECIAL_CBRACE;
			}
		ncase '\\': i = escape (r, i + 1, &R [j++]);
		}
	R [j] = 0;
	if (np) rerror ("unclosed parenthesis");
}

typedef char *char_class;

static char_class *CClass;
static int nclasses;

static int add_class (char_class C)
{
	int i;
	for (i = 0; i < nclasses; i++)
		if (!strcmp (CClass [i], C))
			return i + RE_SPECIAL_CLASS;
	CClass [nclasses] = strdup (C);
	return RE_SPECIAL_CLASS + nclasses++;
}

static inline void OR_classes (char_class C1, char_class C2)
{
	int i;
	for (i = 0; i < 258; i++)
		if (C1 [i] == '0' && C2 [i] == '1')
			C1 [i] = '1';
}

static int mk_class (regstr, int*);

static int mk_uclass (int v)
{
	int cb = tolower (v - RE_SPECIAL_UCLASS);
	int iv = cb != v - RE_SPECIAL_UCLASS, i;
	int R [64];

	if (!uclass [OFFS (cb)].def)
		rerror ("undefined user class");

	if (uclass [OFFS (cb)].op)
		rerror ("recusrive character class");
	uclass [OFFS (cb)].op = 1;
	regexp_descape (uclass [OFFS (cb)].def, R);
	mk_class (R, &v);
	v -= RE_SPECIAL_CLASS;
	if (iv)
		for (i = 1; i < 258; i++)
			CClass [v][i] = CClass [v][i] == '1' ? '0' : '1';
	uclass [OFFS (cb)].op = 0;
	return v + RE_SPECIAL_CLASS;
}

static int mk_class (regstr R, int *p)
{
	int i = 0, j1, j2, c;
	int not = R [i++] == RE_SPECIAL_ONBRAK;
	char base_class [258];

	if (!not && R [i + 1] == RE_SPECIAL_CBRAK && R [i] < 256) {
		*p = R [i];
		return i + 2;
	}

	memset (base_class, '0', 258);
	base_class [258] = 0;

	while (R [i] != RE_SPECIAL_CBRAK)
		if (R [i] < 256) {
			if (R [i + 1] != RE_SPECIAL_RANGE) {
				base_class [R [i++]] = '1';
				continue;
			}
			if (R [i + 2] >= 256) rerror ("range to special");
			if (R [i + 2] < R [i])
				j1 = R [i + 2], j2 = R [i];
			else 	j1 = R [i], j2 = R [i + 2];
			while (j1 <= j2)
				base_class [j1++] = '1';
			i += 3;
		} else if (isuclass (R [i])) {
			j1 = mk_uclass (R [i++]) - RE_SPECIAL_CLASS;
			for (c = 0; c < 256; c++)
				if (CClass [j1][c] == '1')
					base_class [c] = '1';
		} else rerror ("was not expecting that in here");

	if (not) 
		for (j1 = 1; j1 < 256; j1++)
			base_class [j1] = base_class [j1] == '0' ? '1' : '0';

	*p = add_class (base_class);
	return i;
}

static void build_ctbl ();
static void compile_classes (regstr R)
{
	int i, j, R2 [256];
	char_class tmp [256];
	CClass = tmp;

	for (i = 0; R [i]; i++)
		R2 [i] = R [i];
	R2 [i] = 0;

	for (i = j = 0; R2 [i]; i++)
		if (R2 [i] == RE_SPECIAL_OBRAK || R2 [i] == RE_SPECIAL_ONBRAK)
			i += mk_class (&R2 [i], &R [j++]);
		else if (isuclass (R2 [i]))
			R [j++] = mk_uclass (R2 [i]);
		else R [j++] = R2 [i];
	R [j] = 0;
	build_ctbl ();
}


static unsigned int *ctbl;

static void build_ctbl ()
{
	int i, j, v;
	char_class C;

	if (nclasses > 31) rerror ("not supported > 32 character classes");

	for (i = 0; i < 258; i++)
		ctbl [i] = 0;

	for (i = 0; i < nclasses; i++)
		for (C = CClass [i], v = 1 << i, j = 0; j < 257; j++)
			if (C [j] == '1')
				ctbl [j] |= v;
}

static int count1s (int b)
{
	int i, s;
	b = 1 << b;
	for (i = 1, s = 0; i < 256; i++)
		if (ctbl [i] & b) ++s;
	return s;
}
//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 
static inline int re_skip_parenthesis (regstr R, int i)
{
	int p;

	for (p = 1; p; i++)
		switch (R [i]) {
			case RE_SPECIAL_OPARENTH: ++p;
			ncase RE_SPECIAL_CPARENTH: --p;
			ncase 0: rerror ("Unclosed parenthesis");
		}
	return i;
}

static int find_or (regstr R, int o[])
{
	int i, n;

	for (i = n = 0; R [i]; i++)
		if (R [i] == RE_SPECIAL_OR)
			o [n++] = i;
		else if (R [i] == RE_SPECIAL_OPARENTH)
			i = re_skip_parenthesis (R, i + 1) - 1;
	if (n) o [n++] = i;

	return n;
}

static int find_and (regstr R, int o[])
{
	int i, n;

	i = n = 0;
	while (R [i]) {
		o [n++] = i;
		i = R [i] == RE_SPECIAL_OPARENTH ? re_skip_parenthesis (R, i + 1) : i + 1;
		if (R [i] != RE_SPECIAL_STAR && R [i] != RE_SPECIAL_PLUS
		&&  R [i] != RE_SPECIAL_QUESTION && R [i] != RE_SPECIAL_OBRACE)
			continue;
		if (R [i] == RE_SPECIAL_OBRACE)
			while (R [i] != RE_SPECIAL_CBRACE) i++;
		i++;
		if (R [i] == RE_SPECIAL_QUESTION) i++;
	}
	o [n] = i;

	return n;
}

// * * * * * * * * * * A.n.a.l.y.s.i.s * * * * * * * * * * * * *
//	generate and optimize the ``subregular'' array which
// 	is more convenient. We can do analysis and put all
//	the info for the code generator in the sr_pool[]

enum { SB_NIL, SB_TERM, SB_OR, SB_AND, SB_REPG, SB_REPN };

typedef int subReg;

typedef struct {
	subReg *expr, expr1, referrer, this;
	subReg rstate;
	int n;
	int store_at;
	int type;
	int min, max;
	int from, to;
	bool granted;
	// optimizations
	bool fsckd;
	int backtrack;
	long long *swtbl;
	int save_fail, use_fail;
	char *strn;
	// for the code generator
	int proton, save_success, gst, usemin;
} subregular;

static subregular **sr_pool;
static int nsr_pool;
static subReg root;

static void free_subregs ()
{
	int i;
	for (i = 0; i < nsr_pool; i++) {
		if (sr_pool [i]->swtbl) free (sr_pool [i]->swtbl);
		if (sr_pool [i]->strn) free (sr_pool [i]->strn);
		free (sr_pool [i]);
	}
}

static void set_repet (subregular *m, regstr R)
{
	int j = 1;
	switch (R [0]) {
	case RE_SPECIAL_STAR: m->min = 0; m->max = regexp_INFTY;
	ncase RE_SPECIAL_PLUS: m->min = 1; m->max = regexp_INFTY;
	ncase RE_SPECIAL_QUESTION: m->min = 0; m->max = 1;
	ncase RE_SPECIAL_OBRACE: m->min = R [j++];
		if (R [j] == ',') {
			if (R [j + 1] != RE_SPECIAL_CBRACE) {
				m->max = R [j + 1];
				j = 5;
			} else {
				m->max = regexp_INFTY;
				j = 4;
			}
		} else {
			m->max = m->min;
			j = 3;
		}
	}
	m->type = R [j] == RE_SPECIAL_QUESTION ? SB_REPN : SB_REPG;
}

static subregular *mallocsubr (int type, int store_at, int from, int to, subReg referrer)
{
	subregular *m = (subregular*) malloc (sizeof *m);
	m->proton = 0;
	m->type = type;
	m->store_at = store_at;
	m->save_success = 0;
	m->from = from;
	m->to = to;
	m->referrer = referrer;
	m->backtrack = -1;
	m->fsckd = 1;
	m->swtbl = 0;
	m->save_fail = m->use_fail = 0;
	m->rstate = -1;
	m->gst = 0;
	m->usemin = 0;
	m->granted = 0;
	m->strn = 0;
	return sr_pool [m->this = nsr_pool++] = m;
}

static subReg make_subregulars (regstr R, subReg referrer)
{
	int n, ors [128], tmp [256], i, j, k, orst [64];
	subregular *m, *mr;

	if (!R [0])
		return mallocsubr (SB_NIL, -1, 0, 0, referrer)->this;

	if ((n = find_or (R, ors))) {
		int from, to;
		from = regexp_INFTY, to = 0;
		for (i = 0; i < n; i++) {
			for (j = 0, k = i ? ors [i-1]+1 : 0; k < ors [i];)
				tmp [j++] = R [k++];
			tmp [j] = 0;
			j = orst [i] = make_subregulars (tmp, -1);
			if (sr_pool [j]->from < from) from = sr_pool [j]->from;
			if (sr_pool [j]->to > to) to = sr_pool [j]->to;
		}
		orst [i] = -1;
		m = mallocsubr (SB_OR, -1, from, to, referrer);
		for (i = 0; i < n; i++)
			sr_pool [orst [i]]->referrer = m->this;
		m->n = i;
		m->expr = intdup (orst);
		return m->this;
	}

	n = find_and (R, ors);

	if (n > 1) {
		int from, to;
		for (from = to = i = 0; i < n; i++) {
			for (j = 0, k = ors [i]; k < ors [i+1];)
				tmp [j++] = R [k++];
			tmp [j] = 0;
			j = orst [i] = make_subregulars (tmp, -1);
			from += sr_pool [j]->from;
			to += sr_pool [j]->to;
		}
		orst [i] = -1;
		m = mallocsubr (SB_AND, -1, from, min (regexp_INFTY, to), referrer);
		for (i = 0; i < n; i++)
			sr_pool [orst [i]]->referrer = m->this;
		m->n = i;
		m->expr = intdup (orst);
		return m->this;
	}

	if (R [0] == RE_SPECIAL_OPARENTH) {
		int have_stor = isstorp (R [1]);
		int skip1 = have_stor || R [1] == RE_SPECIAL_GRANTED;
		int have_repet = R [j = re_skip_parenthesis (R, 1)] != 0;

		for (i = 1 + skip1, k = 0; i < j - 1;)
			tmp [k++] = R [i++];
		tmp [k] = 0;
		k = make_subregulars (tmp, -1);
		if (have_stor) sr_pool [k]->store_at = R [1] - RE_SPECIAL_STORP;
		sr_pool [k]->granted = skip1 && !have_stor;
		if (!have_repet) {
			sr_pool [k]->referrer = referrer;
			return k;
		}

		m = mallocsubr (0, -1, 0, 0, referrer);
		m->expr1 = k;
		set_repet (m, &R [j]);
		m->from = m->min * sr_pool [k]->from;
		m->to = min (regexp_INFTY, m->max * sr_pool [k]->to);
		sr_pool [k]->referrer = m->this;
		return m->this;
	}

	m = mallocsubr (SB_TERM, -1, 1, 1, referrer);
	m->expr1 = R [0];
	if (!R [1])
		return m->this;

	mr = mallocsubr (0, -1, 0, 0, referrer);
	m->referrer = mr->this;
	mr->expr1 = m->this;
	mr->store_at = -1;
	set_repet (mr, &R [1]);
	mr->from = mr->min; mr->to = mr->max;
	return mr->this;
}


static void set_r_granted (subregular *m)
{
	int i;
	m->granted = 1;
	switch (m->type) {
	case SB_REPG:
	case SB_REPN: set_r_granted (sr_pool [m->expr1]);
	ncase SB_OR:
	case SB_AND:
		for (i = 0; i < m->n; i++)
			set_r_granted (sr_pool [m->expr [i]]);
	}
}

static void set_granted ()
{
	int i;
	subregular *m;

	for (i = 2; i < nsr_pool; i++)
		if ((m = sr_pool [i])->granted) {
			if (m->from != m->to)
				rerror ("Granted strings must be fixed size!");
			set_r_granted (m);
		}
}
#ifdef DEBUG
//
// debugging.  print the sr_pool[] through the stages
//
static void print_subregularz ()
{
	int j, n, i;
	subregular *m;

	PRINTF ("root=%i\n", root);
	for (i = 0; i < nsr_pool; i++) {
		m = sr_pool [i];
	PRINTF ("%s%i(%i)[%i-%i]: ", m->store_at >= 0 ? COLS"S"COLE : "S", i, m->this, m->from, m->to);

	switch (m->type) {
	case SB_NIL: PRINTF (COLS"-"COLE"NIL-\n");
	ncase -1: PRINTF (COLS"-"COLE"1 (off)\n");
	ncase SB_TERM: if (!m->strn) {
		int c = m->expr1;
		if (c <= '~')
			if (c >= ' ') PRINTF ("%c\n", c);
			else PRINTF (" ctrl[%i]\n", c);
		else if (c == RE_SPECIAL_DOT) PRINTF (COLS"."COLE"\n");
		else if (c == RE_SPECIAL_SSTART) PRINTF (COLS"^"COLE"\n");
		else if (c == RE_SPECIAL_SEND) PRINTF (COLS"$"COLE"\n");
		else PRINTF ("class[%i]\n", c - RE_SPECIAL_CLASS);
	} else {
		PRINTF ("strncmp (\"%s\")\n", m->strn);
	}
	ncase SB_OR:
		n = m->n;
		for (j = 0; j < n; j++)
			PRINTF ("S%i %s", m->expr [j], j == n-1 ? "" : COLS"| "COLE);
		PRINTF (m->swtbl ? COLS"  *switched*"COLE"\n" : "\n");
	ncase SB_AND:
		n = m->n;
		for (j = 0; j < n; j++)
			PRINTF ("S%i %s", m->expr [j], j == n-1 ? "\n" : COLS"+ "COLE);
	ncase SB_REPG:
	case SB_REPN:
		PRINTF ("S%i "COLS"^"COLE" (%i %s %i)  ", m->expr1, m->min, 
			m->type == SB_REPN ? "->" : "<-", m->max);
		if (m->use_fail) PRINTF (COLS"*use fail%i*   "COLE, m->use_fail);
		if (m->fsckd) PRINTF (COLS"*fsckd*"COLE"\n");
		else if (m->backtrack >= 0)
			PRINTF (COLS"*backtrack %i!*"COLE"\n", m->backtrack);
		else PRINTF (COLS"*unroll*"COLE"\n");
	}
	if (m->rstate != -1)
		PRINTF ("   this=%i rstate ---> S%i\n", m->this, m->rstate);
	}
}
#endif
//
// silly optimizations
//

/*********************************************************
[1]	FSCKD repetitioners - unroll recursion

Imagine this bad case /(fo|foo)*s/ against the string
"foofos".  We can't unroll the recusrion: the fact that
it matches "fo" does not mean we can advance with a
positive match:  "foo" can be matched if we don't advance
/(foo|fo)*s/ is OK on the other hand.

Anyway, if the repetitioner applies on fixed size we
are definitelly OK.  If it is not fixed size and the
subexpression contains ORs or NON-GREEDY repetitioners,
suppose FSCKD.  We could do futher analysis, but let's
suppose that repetitioners on non-fixed size *are* rare.

/(foo??)*s/  is FSCKD.
/(foo?)*s/   is OK.

it has to do with subsets...
*********************************************************/
static int has_ors_or_temperate (subregular *m)
{
	switch (m->type) {
	case SB_NIL:
	case SB_TERM: return 0;
	case SB_REPN:
	case SB_OR:   return 1;
	case SB_REPG:  return has_ors_or_temperate (sr_pool [m->expr1]);
	default:
	case SB_AND: {
		int i;
		for (i = 0; i < m->n; i++)
			if (has_ors_or_temperate (sr_pool [m->expr [i]]))
				return 1;
		return 0;
	} }
}

static void optimize_fsckd_repetitioners ()
{
	int i;
	subregular *m, *m2;

	for (i = 2; i < nsr_pool; i++) {
		m = sr_pool [i];
		if (m->type != SB_REPG && m->type != SB_REPN)
			continue;
		m->fsckd = 0;
		m2 = sr_pool [m->expr1];
		if (m2->from == m2->to)
			continue;
		m->fsckd = has_ors_or_temperate (m2);
		if (m->type == SB_REPN
		&& sr_pool [m->expr1]->to != sr_pool [m->expr1]->from)
			m->fsckd = 1;
	}
}
/*********************************************************
[2]	NO BACKTRACK on greedy repetitioners

This is a very important optimization. Suppose regexp
/a*@/ and the string "aaaaaaab". Once the search for
'a's stops at 'b', there's no need to backtrack hoping
for a less greedy match: the fact that so far we matched
[a] means that there's no way we can match [@]. The opt
is important because such constructs are very frequent.

Another interesting case is this:  /\w+\d\d\d@/
where we do not backtrack more than 3.  This is checked
only if the base of the repetitioner is one character and
as long as it is followed by fixed subexpressions.
Otherwise it gets very complicated.

In the case C1* C2 C3 C4, where C4 and C1 have no common
characters, we also check C2-C4 and C3-C4. Normally
we backtrack no more than 2. If C2 and C4 have no common
and C3 and C4 have no common we backtrack no more than 0!
*********************************************************/
static subReg next_subreg_norep (subReg s)
{
	int i;
	subReg p;
	subregular *m;
	do {
		s = sr_pool [p = s]->referrer;
		if (s == -1) return -1;
		m = sr_pool [s];
		if (m->type == SB_REPG || m->type == SB_REPN)
			return -1;
		if (m->type == SB_OR) continue;
		if (m->type != SB_AND) rerror ("bugs");
		for (i = 0; m->expr [i] != p; i++);
		if (m->expr [i + 1] != -1)
			return m->expr [i + 1];
	} while (1);
}

static void set_class_from_term (char_class C, int c)
{
	int i;
	if (c < 258)
		if (!regexp_case && c < 127 && isalpha (c))
			C [toupper (c)] = C [tolower (c)] = '1';
		else C [c] = '1';
	else if (c == RE_SPECIAL_DOT)
		for (i = 1; i < 256; i++)
			C [i] = '1';
	else {
		c = 1 << (c - RE_SPECIAL_CLASS);
		for (i = 0; i < 258; i++)
			if (ctbl [i] & c)
				C [i] = '1';
	}
}

static void get_first_class (subReg s, char_class C)
{
	if (s == -1) {
		memset (C, '1', 258);
		return;
	}
	int i;
	subregular *m = sr_pool [s];

	switch (m->type) {
	case SB_REPG:
	case SB_REPN: return get_first_class (m->expr1, C);
	case SB_AND: return get_first_class (m->expr [0], C);
	case SB_NIL: for (i = 0; i < 258; i++) C [i] = '1';
	ncase SB_TERM: set_class_from_term (C, m->expr1);
	ncase SB_OR:
		for (i = 0; i < m->n; i++)
			get_first_class (m->expr [i], C);
	}
}

static int no_common (char_class C1, char_class C2)
{
	int i;
	for (i = 0; i < 258; i++)
		if (C2 [i] == '1' && C1 [i] == '1')
			return 0;
	return 1;
}

static int test_repetitioner1 (subregular *m)
{
	char C1 [259], C2 [259];
	memset (C1, '0', 258); C1 [258] = 0;
	memset (C2, '0', 258); C2 [258] = 0;

	get_first_class (m->expr1, C1);
	get_first_class (next_subreg_norep (m->this), C2);
	return no_common (C1, C2);
}

static void get_nth_class_from_fixed (subregular *m, char_class C, int n)
{
	int i, j;
	switch (m->type) {
	case SB_AND:
		j = sr_pool [m->expr [i = 0]]->to;
		while (n >= j)
			j += sr_pool [m->expr [++i]]->to;
		j -= sr_pool [m->expr [i]]->to;
		get_nth_class_from_fixed (sr_pool [m->expr [i]], C, n - j);
	ncase SB_OR:
		for (i = 0; i < m->n; i++)
			get_nth_class_from_fixed (sr_pool [m->expr [i]], C, n);
	ncase SB_TERM:
		set_class_from_term (C, m->expr1);
	ncase SB_NIL: for (i = 0; i < 258; i++) C [i] = '1';
	}
}

static void test_repetitioner2 (subregular *m)
{
	int i, j, k, p;
	subReg this = m->this, s = m->referrer;
	subregular *m2;

	if (s == -1) return;
	m = sr_pool [s];
	if (m->type != SB_AND) return;
	for (i = 0; m->expr [i] != this; i++);
	for (j = 0, k = ++i; i < m->n; i++) {
		m2 = sr_pool [m->expr [i]];
		if (m2->from != m2->to) break;
		j += m2->from;
	}
	if (j < 2) return;

	char C1 [259], C2 [259];
	memset (C1, '0', 258); C1 [258] = 0;
	get_first_class (sr_pool [this]->expr1, C1);

	for (j = 0, i = p = k; i < m->n; i++) {
		m2 = sr_pool [m->expr [i]];
		for (k = 0; k < m2->to; k++, j++) {
			memset (C2, '0', 258); C2 [258] = 0;
			get_nth_class_from_fixed (m2, C2, k);
			if (no_common (C1, C2))
				goto ok;
		}
	}
	return;
ok:;
//PRINTF ("Backtrack distance = %i\n", j);
	int i2, k2, mto;
	for (i2 = p; i2 <= i; i2++) {
		m2 = sr_pool [m->expr [i2]];
		mto = (i2 == i) ? k : m2->to - 1;
		for (k2 = 0; k2 <= mto; k2++) {
			memset (C1, '0', 258); C1 [258] = 0;
			get_nth_class_from_fixed (m2, C1, k2);
			if (no_common (C1, C2)) --j;
			else goto dbreak;
		}
	}
dbreak:
//PRINTF ("Reduced to %i...\n", j);
	sr_pool [this]->backtrack = j;
}

static void optimize_backtracking_repetitioners ()
{
	int i;
	subregular *m;

	for (i = 2; i < nsr_pool; i++) {
		m = sr_pool [i];
		if (m->type != SB_REPG)
			continue;
		m->backtrack = -1;
		if (test_repetitioner1 (m))
			m->backtrack = 0;
		else if (sr_pool [m->expr1]->type == SB_TERM)
			test_repetitioner2 (m);
	}
}
/*********************************************************
[3]	NON-GREEDY jump

If a non-greedy repetitioner is followed by another
repetitioner, drop complexity in failure.  For example
/.*?\w*?@/ and the text "abcdef1234,". Once the second
repetitioner (\w*?) stops at ',', the first one will
jump at it the next time: there is no need to go through
"bcdef1234" because the second repetitioner will always
halt at the same point.

This sounds rare but we make all regular expressions
fixed at the start by adding /^.*?/ in front of them.
So it is quite common and very useful so we won't shift
more than we have to.  The optimization takes place
only when the repetitioner is on a character class and as
long as it is followed by character classes, up to the
first repetitioner.  For example /^.*?abcd\w+/
will do it. It is interesting if the second repetitioner
has a big 'max' and possibly many match characters:
if it will walk far.  An unfortunate case is this
/.*?ab?c\w+/  the (b?) repetitioner is too small for
us to activate the optimization and yet stops us
from proceeding up to the (\w+) which is interesting.

We can do more analysis and even pass it to OR's in
the future. For example: /.*?(bat|bee|fly)\w+/ ....
*********************************************************/
static void make_fixed ()
{
	// make the regular expression fixed to start if
	// possible by adding /^.*?/ in front of it.
	// we can't do that if '^' already exists.
	// a pathological case is (^a|bc)
	char C [259];
	memset (C, '0', 258); C [258] = 0;
	get_first_class (root, C);
	if (C [RE_SPECIAL_SSTART] == '1')
		return;

	subregular *m = sr_pool [root];
	subregular *st = mallocsubr (SB_TERM, -1, 1, 1, 00);
	subregular *dt = mallocsubr (SB_TERM, -1, 1, 1, 00);
	subregular *ng = mallocsubr (SB_REPN, -1, 0, regexp_INFTY, 00);
	subregular *nr = mallocsubr (SB_AND, -1, m->from, m->to, -1);

	st->expr1 = RE_SPECIAL_SSTART;
	st->referrer = nr->this;
	dt->expr1 = RE_SPECIAL_DOT;
	dt->referrer = ng->this;
	ng->expr1 = dt->this;
	ng->referrer = nr->this;
	ng->fsckd = 0;
	ng->min = 0;
	ng->max = regexp_INFTY;
	m->referrer = nr->this;
	nr->n = 3;
	subReg tmp [] = { st->this, ng->this, m->this, -1 };
	nr->expr = intdup (tmp);
	root = nr->this;
}

static subReg descend_to_repterm (subReg s)
{
	subregular *m = sr_pool [s];
	switch (m->type) {
	case SB_OR:
	case SB_TERM:
	case SB_NIL: return -1;
	case SB_AND: return descend_to_repterm (m->expr [0]);
	default: return sr_pool [m->expr1]->type == SB_TERM ?
			// do if repetitioner max > 4 walk far
			m->max >= 4 ? m->this : -1 : -1;
	}
}

static int get_term_sequence (subregular *m, int st, subReg seq[])
{
	int i, n = 0;
	subregular *r;
	for (i = st; i < m->n; i++) {
		r = sr_pool [m->expr [i]];
		switch (r->type) {
		 default: if (sr_pool [r->expr1]->type == SB_TERM) 
			goto keep;
		 case SB_OR:
		 case SB_NIL: seq [n] = -1; return 0;
		 keep:
		 case SB_TERM: seq [n++] = r->this;
		ncase SB_AND: if (!get_term_sequence (r, 0, seq + n))
				return 0;
			while (seq [n] != -1) n++;
		}
	}
	seq [n] = -1;
	return 1;
}

static void distant_jmp (subregular *m)
{
	int j, i;
	subReg sq [64];
	subregular *a = sr_pool [m->referrer];
	if (a->type != SB_AND) return;
	for (j = 0; a->expr [j] != m->this; j++);
	get_term_sequence (a, j + 1, sq);

	for (i = 0; sq [i] != -1; i++)
		if (in2 (sr_pool [sq [i]]->type, SB_REPG, SB_REPN))
			break;
	if (sq [i] == -1) return;
	/* if too small / compared to how far, abort */
	if (sr_pool [sq [i]]->max - i < 4) return;
	for (j = 0; j < i + 1; j++) 
		sr_pool [sq [j]]->save_fail = sq [i];
	m->use_fail = sq [i];
}

static void nongreedy2 (subregular *m, subReg s)
{
	int i, c;
	char C1 [259], C2 [259];
	memset (C1, '0', 258); C1 [258] = 0;
	memset (C2, '0', 258); C2 [258] = 0;

	get_first_class (m->expr1, C1);
	get_first_class (sr_pool [s]->expr1, C2);
	for (i = c = 0; i < 258; i++)
		if (C2 [i] == '1')
			++c;
	/* threshold */
	if (c < 5) return;
	m->use_fail = s;
	sr_pool [s]->save_fail = s;
}

static void optimize_nongreedy_jump ()
{
	int i;
	subregular *m;
	subReg s;

	make_fixed ();
	for (i = 2; i < nsr_pool; i++) {
		m = sr_pool [i];
		if (m->type != SB_REPN || m->fsckd)
			continue;
		if (sr_pool [m->expr1]->type != SB_TERM)
			continue;
		s = next_subreg_norep (m->this);
		if (s == -1 || (s = descend_to_repterm (s)) == -1)
			distant_jmp (m);
		else 	nongreedy2 (m, s);
	}
}
/*********************************************************
[4]	zero min

Convert repetitioners with non-zero minimum times to
repetitioners with zero minimum by expanding *min times
the subexpression. For example /(f+)/ becomes /(ff*)/.

We'll only do this for '+' and if we have 0 backtrack
from optimization [2]. This is excellent because we don't
count times.
*********************************************************/
static subregular *clone_copy (subregular *m)
{
	subregular *n = (subregular*) malloc (sizeof *n);
	*n = *m;
	return sr_pool [n->this = nsr_pool++] = n;
}

static subReg clone_subReg (subReg s, subReg referrer)
{
	int i;
	subregular *m = sr_pool [s];
	subregular *n = clone_copy (m);

	n->referrer = referrer;
	switch (m->type) {
	 case SB_REPN:
	 case SB_REPG: n->expr1 = clone_subReg (m->expr1, n->this);
	ncase SB_AND:
	 case SB_OR: for (i = 0; i < n->n; i++)
			n->expr [i] = clone_subReg (n->expr [i], n->this);
	}
	return n->this;
}

static void optimize_zeromin ()
{
	int i, all = nsr_pool;
	subregular *m;

	for (i = 2; i < all; i++) {
		if (!in2 ((m = sr_pool [i])->type, SB_REPG, SB_REPN))
			continue;
		if (m->min == 1 && m->max == regexp_INFTY) {
			subregular *r = clone_copy (m);
			r->from = r->min = 0;
			r->store_at = -1;
			r->referrer = m->this;
			subReg tmp [] = { clone_subReg (r->expr1, m->this), r->this, -1 };
			m->type = SB_AND;
			m->n = 2;
			m->expr = intdup (tmp);
			// -opt 3 adjust-
			sr_pool [tmp [0]]->save_fail = r->save_fail;
		}
	}
}
/*********************************************************
[5]	SWITCH

For OR expressions with more than 3 operands use a
switch in C.  For example /(Mon|Tue|Web|Thu|Fri|Sat|Sun)/
will greatly benefit.

TODO: non-fixed with no repetitioners is OK /cat|bird|penguin/
*********************************************************/
#define SWITCH_THRESHOLD 3

static void rmv_first_class (subReg s)
{
	int i;
	subregular *m = sr_pool [s];

	switch (m->type) {
	case SB_AND:
		if (sr_pool [m->expr [0]]->type == SB_TERM) {
			sr_pool [m->expr [0]]->type = -1;
			for (i = 0; i < m->n; i++) {
				m->expr [i] = m->expr [i + 1];
			}
			--m->n;
		} else rmv_first_class (m->expr [0]);
		--m->from; --m->to;
	ncase SB_NIL:
	ncase SB_TERM: m->type = SB_NIL; m->from = m->to = 0;
	ncase SB_OR:
		for (i = 0; i < m->n; i++)
			rmv_first_class (m->expr [i]);
		--m->from; --m->to;
	}
}

static void optimize_switch (subregular *m)
{
	int i, j;
	char C [259];
	long long SW [258], v;

	if (m->n > 64) return;	// not impl

	memset (SW, 0, sizeof SW);
	for (i = 0; i < m->n; i++) {
		memset (C, '0', 258); C [258] = 0;
		get_first_class (m->expr [i], C);
		rmv_first_class (m->expr [i]);
		for (v = 1 << i, j = 0; j < 258; j++)
			if (C [j] == '1')
			SW [j] |= v;
	}
	memcpy (m->swtbl = (long long*) malloc (sizeof SW), SW, sizeof SW);
}

static void optimize_switches ()
{
	int i, j;
	subregular *m;

	for (i = 2; i < nsr_pool; i++) {
		m = sr_pool [i];
		if (m->type != SB_OR || m->n <= SWITCH_THRESHOLD || m->from != m->to)
			continue;
		for (j = 0; j < m->n; j++)
			if (!sr_pool [m->expr [j]]->to)
				goto nope;
		optimize_switch (m);
		nope:;
	}
}
/*********************************************************
[6]	minimum length

For the case our regular expression starts with the ^.*?
and the minimum length of the string in order to have
a positive match *is* accountable (non neligible), we
set a threshold to avoid studying strings which are
too small to produce a match anyway.
*********************************************************/
static int minlen;

static Token R_minlen;
static void optimize_minlen ()
{
	int i, j;
	subregular *m = sr_pool [root], *n;
	minlen = 0;
	if (regexp_packed) return;
	if (m->type != SB_AND) return;
	n = sr_pool [m->expr [0]];
	if (n->type != SB_TERM || n->expr1 != RE_SPECIAL_SSTART) return;
	n = sr_pool [m->expr [1]];
	if (n->type != SB_REPN) return;
	n = sr_pool [n->expr1];
	if (n->type != SB_TERM) return;
	for (j = 0, i = 2; i < m->n; i++)
		j += sr_pool [m->expr [i]]->from;
	if (j < 5) return;
	minlen = j;
	sr_pool [m->expr [1]]->usemin = 1;
}

/*********************************************************
[7]	use strncmp/strncasecmp

For long fixed strings, the code gets smaller and
this is good for cache.
*********************************************************/
static int isplain (subReg s, int i)
{
	subregular *m = sr_pool [sr_pool [s]->expr [i]];
	return m->type == SB_TERM && m->expr1 < 255 && !m->granted;
}

static void dostrfunc (subReg s, int f, int t)
{
	int i;
	subregular *m = sr_pool [s], *m2;
	char tmp [255];

	for (i = 0; f + i <= t; i++)
		tmp [i] = sr_pool [m->expr [f + i]]->expr1;
	tmp [i] = 0;
	m2 = sr_pool [m->expr [f]];
	m2->strn = strdup (tmp);
	m2->from = m2->to = strlen (tmp);
	for (i = 1; t + i <= m->n; i++)
		m->expr [f + i] = m->expr [t + i];
	m->n -= t - f;
}

#define DOSTR_ABOVE 2

static void optimize_strfuncs ()
{
	int i, j, k;
	subregular *m;

	for (i = 2; i < nsr_pool; i++)
		if ((m = sr_pool [i])->type == SB_AND)
			for (j = 0; j < m->n;) {
				while (j < m->n && !isplain (i, j)) j++;
				if (j == m->n)
					break;
				for (k = j; k < m->n && isplain (i, k); k++);
				if (k - j > DOSTR_ABOVE)
					dostrfunc (i, j, k - 1);
				++j;
			}
}
/*++++++++++++++++++++++++++++++++++++++++++++++++++
++++++++++++++++++++++++++++++++++++++++++++++++++*/
/*********************************************************
	Do all optimizations in the correct order
*********************************************************/
void print_subregularz ();
static void optimize_regexp ()
{
#ifdef	DEBUG
#define STATUS(x) \
	if (debugflag.REGEXP_DEBUG) {\
		print_subregularz ();\
		PRINTF (x);\
	}
#else
#define STATUS(x)
#endif
	//
	// must be first. we don't do the others on fsckd
	//
	STATUS ("\nOptimize: set fsckd repetitioners:\n")
	optimize_fsckd_repetitioners ();

	//
	// most important optimization
	//
	STATUS ("\nOptimize. Backtracks:\n")
	optimize_backtracking_repetitioners ();

	//
	// make it fixed ^ if possible. root changes
	//
	STATUS ("\nOptimize. Non-greedy jumps:\n")
	optimize_nongreedy_jump ();
	optimize_minlen ();

	//
	// must be after above. others are not prepared
	// for an SB_OR subregular with a swtbl
	//
	STATUS ("\nOptimize. Switch:\n")
	optimize_switches ();

	//
	// put it here to avoid zeromin on
	// subexpressions implemented with switch ()
	//
	STATUS ("\nOptimize. zeromin:\n")
	optimize_zeromin ();

	//
	// use strncmp() and strncasecmp() for fixed sequences
	if (regexp_strfuncs) {
		STATUS ("\nOptimize strfuncs():\n");
		optimize_strfuncs ();
	}
	STATUS (" ");
}

//
//
//
/*********************************************************
	finalize all the info in the sr_pool[]
*********************************************************/
#define OBV 10000


//
// disable extractors inside repetitioners
//
static subReg *e_off;
static int n_e_off;

static void disable_extractors (subregular *m)
{
	int i;
	if (m->store_at >= 0) {
		e_off [n_e_off++] = m->store_at;
		m->store_at = -1;
	}
	switch (m->type) {
	case SB_REPG:
	case SB_REPN: disable_extractors (sr_pool [m->expr1]);
	ncase SB_OR:
	case SB_AND:
		for (i = 0; i < m->n; i++)
			disable_extractors (sr_pool [m->expr [i]]);
	}
}

static void disable_repet_extracts ()
{
	int i, j, k, l;

	e_off = (subReg*) alloca (nsr_pool * sizeof *e_off);
	n_e_off = 0;
	for (i = 0; i < nsr_pool; i++)
		if (in2 (sr_pool [i]->type, SB_REPG, SB_REPN))
			disable_extractors (sr_pool [sr_pool [i]->expr1]);
	if (!n_e_off) return;
	for (i = 0; i < nsr_pool; i++)
		if ((l = sr_pool [i]->store_at) >= 0) {
			for (j = k = 0; j < n_e_off; j++)
				if (l > e_off [j]) ++k;
			sr_pool [i]->store_at -= k;
		}
	nextr -= n_e_off;
}

//
// possible badly set matchers:  (cat)|(dog)|(baboon)
//

static struct {
	subReg ss, se;
	int fixedlen;
	bool ambig;
} extract_point [64];

static bool ambiguous_extracts;

static void ambig_store (subregular *m)
{
	if (m->store_at >= 0)
		ambiguous_extracts = extract_point [m->store_at].ambig = 1;
	if (in2 (m->type, SB_OR, SB_AND)) {
		int i;
		for (i = 0; i < m->n; i++)
			ambig_store (sr_pool [m->expr [i]]);
	}
}

static void check_match_in_or ()
{
	int i, j;
	subregular *m;
	ambiguous_extracts = 0;
	for (i = 0; i < nsr_pool; i++)
		if ((m = sr_pool [i])->type == SB_OR)
			for (j = 0; j < m->n; j++)
				ambig_store (sr_pool [m->expr [j]]);
}
//
// connect return states and mark extractor points
//

static void rstate_connect (subReg s, subReg rstate)
{
	int i;
	subregular *m = sr_pool [s], *m1;
	if (m->store_at >= 0) {
		int fixedlen = m->from == m->to ? m->to : -1;
		extract_point [m->store_at].ss = m->this;
		extract_point [m->store_at].ambig = 0;
		m->save_success = 1;
		if ((extract_point [m->store_at].fixedlen = fixedlen) == -1) {
			if (!rstate) rstate = 1;
			extract_point [m->store_at].se = rstate;
			sr_pool [rstate]->save_success = 1;
		}
	}
	m->rstate = rstate;
	switch (m->type) {
	case SB_TERM:
	case SB_NIL:
	ncase SB_AND:
		for (i = 0; i < m->n - 1; i++)
			rstate_connect (m->expr [i], m->expr [i + 1]);
		rstate_connect (m->expr [i], rstate);
		m->rstate = m->expr [0];
	ncase SB_OR:
		for (i = 0; i < m->n; i++)
			rstate_connect (m->expr [i], rstate);
	ncase SB_REPN:
	case SB_REPG:
		m1 = sr_pool [m->expr1];
		if (m->fsckd) {
			rstate_connect (m->expr1, m->store_at >= 0 ? s + OBV : s);
			if (!m1->from)
				rerror ("can match infinite times the empty string anywhere!");
		} else rstate_connect (m->expr1, m->store_at >= 0 || m1->from != m1->to);
	}
}

static bool fixed_start;
static void do_fixed_start ()
{
	subregular *m1 = sr_pool [root], *m2;
	fixed_start = 0;
	if (m1->type == SB_AND) {
		m2 = sr_pool [m1->expr [0]];
		if (m2->type == SB_TERM && m2->expr1 == RE_SPECIAL_SSTART) {
			m2->type = -1;
			intcpy (m1->expr, m1->expr + 1);
			--m1->n;
			fixed_start = 1;
		}
	}
}

//
// do everything we need for the code generator
//
static void prepare_regexp (char *re)
{
	int r [256];

	nextr = 0;
	nclasses = 0;
	nsr_pool = 0;
	
	// escape and set RE_SPECIAL values
	regexp_descape (re, r);
	
	// compile classes and install class numbers
	compile_classes (r);

	// make sr_pool []
	mallocsubr (SB_NIL, -1, 0, 0, -1);
	mallocsubr (SB_NIL, -1, 0, 0, -1);
	root = make_subregulars (r, -1);
	set_granted ();

	// optimize sr_pool []
	optimize_regexp ();

	// connect rstates for the code generator
	do_fixed_start ();
	disable_repet_extracts ();
	rstate_connect (root, 0);
	check_match_in_or ();
#ifdef	DEBUG
	if (debugflag.REGEXP_DEBUG) {
		PRINTF ("\nrstates:\n");
		print_subregularz ();
	}
#endif
}
//######################################################################
//
//######################################################################
//			Code Generation
//
//	everything we need is in the subregulars sr_pool[]
//######################################################################
static Token catRE (char*);
static Token state_var(char*,int),state_func(int),new_value_char(int),new_value_hex(int), new_value_eint(int);
static Token R_ctbl, R_name;
static Token RGType = RESERVED_char;
#define BRING(x) Token x = RESERVED_ ## x;
#define XCODE(...) { Token xcd [] = { __VA_ARGS__, -1 }; outprintf (REGEXP, ISTR (xcd), -1); }
#define IF(...) if (__VA_ARGS__) {
#define ELIF(...) } else if (__VA_ARGS__) {
#define ELSE } else {
#define ENDIF }

#define PROTO(x) R (static), R (inline), R (int), state_func (x), '(', R (const), \
		 R (unsigned), RGType, '*', a, ')'
#define RETURN0 R (return), R (0)
#define RETURN1 R (return), R (1)
#define JMPVAR(n) state_var ("j", n)
#define STORVAR(n) state_var ("s", n)
#define DECLAREJMP(x,n) outprintf (REGEXPD, R (static), R (const), R (unsigned), RGType, '*', x = JMPVAR (n), ';', -1);
#define DECLARESTOR(x,n) outprintf (REGEXPD, R (static), R (const), R (unsigned), RGType, '*', x = STORVAR(n), ';', -1);
#define STATEF(x) state_func (x), '(', a, ')'
#define STATEFA1(x) state_func (x), '(', a, '+', RESERVED_1, ')'
#define PA '*', a
#define COMPOUND(...) '{', __VA_ARGS__, '}'

static OUTSTREAM REGEXP, REGEXPD;
static bool ctbl_used;

static void gen_end_state (subregular*);
static void gen_action_state (subregular*);
static void gen_or_state (subregular*);
static void gen_switch_state (subregular*);
static void gen_greedy_state (subregular*);
static void gen_nongreedy_state (subregular*);
static void gen_nongreedy_loop_state (subregular*);
static void gen_greedy_loop_state (subregular*);
static void gen_save_interstate (subregular*);
static void proto (subReg);

static void gen_state (subReg s)
{
	if (s <= 1 || s >= OBV) return;
	subregular *m = sr_pool [s];

	if (m->gst == 2) return;
	if (m->gst == 1) { proto (m->this); return; }
	m->gst = 1;
	if (m->save_success)
		gen_save_interstate (m);
	switch (m->type) {
	case SB_AND:
	case SB_NIL:		gen_end_state (m);
	ncase SB_TERM:		gen_action_state (m);
	ncase SB_OR:		m->swtbl ? gen_switch_state (m) : gen_or_state (m);
	ncase SB_REPG:	m->fsckd ? gen_greedy_state (m) : gen_greedy_loop_state (m);
	ncase SB_REPN:	m->fsckd ? gen_nongreedy_state (m) : gen_nongreedy_loop_state (m);
	}
	m->proton = 1;
	m->gst = 2;
}

static void gen_regexp_code ()
{
	int i;

	if (minlen) {
		R_minlen = catRE ("mb");
		outprintf (REGEXPD, R (static), R (const), R (unsigned), RGType, '*',
				 R_minlen, ';', -1);
	}
	BRING (a);
	XCODE (PROTO(0), '{', RETURN1, ';', '}')
	if (nextr) {
		XCODE (PROTO(1), '{', STORVAR (1), '=', a, ';', RETURN1, ';', '}')
		DECLARESTOR (i, 1);
	}
	ctbl_used = false;
	gen_state (root);
}

static void gen_save_interstate (subregular *m)
{
	BRING (a)
	int ist = m->this + OBV;
	int s;

	XCODE (PROTO (ist), ';')
	DECLARESTOR(s, m->this);
	XCODE (PROTO (m->this), '{', R (if), '(', STATEF (ist), ')', '{', s, '=', a, ';')
	XCODE (RETURN1, ';', '}', RETURN0, ';', '}')
	m->this = ist;
}

static void proto (subReg s)
{
	if (s > 0 && (s >= OBV || !sr_pool [s]->proton)) {
		BRING (a)
		outprintf (REGEXPD, PROTO (s), ';', -1);
		sr_pool [s]->proton = 1;
	}
}

//////////////////////////////////////////////////////////
// each of these 7 functions generates a state function //
//////////////////////////////////////////////////////////

//--------------------------------------------
// greedy repetitioner -- unroll
//--------------------------------------------
static void gen_greedy_loop_state (subregular *m)
{
	BRING (a) BRING (x) BRING (i) BRING (X)
	int min = m->min, max = m->max;
	bool havemin = min != 0, havemax = max != regexp_INFTY, havetimes = havemin || havemax;
	bool fixed = sr_pool [m->expr1]->from == sr_pool [m->expr1]->to;
	Token Nmin = new_value_int (min), Nmax = new_value_int (max);
	Token Nfix = new_value_int (sr_pool [m->expr1]->from);
	Token j=j;
	bool save_displ = m->backtrack != 0 && !fixed;
	bool unlimited = regexp_INFTY > regexp_partition && max == regexp_INFTY && save_displ;
	bool xused = havetimes || unlimited || m->backtrack != 0;

	havetimes |= unlimited;

	gen_state (m->expr1); gen_state (m->rstate);

IF (m->save_fail)
	DECLAREJMP (j, m->save_fail)
ENDIF
	XCODE (PROTO (m->this), '{')
IF (xused)		// if count times
	XCODE (R (int), x, '=', R (0), ';')
ENDIF
IF (save_displ)		// if saving displacement for non-fixed with backtracking
	int loopmax = havemax ? max : regexp_partition;
	XCODE (RGType, X, '[', new_value_int (loopmax), ']', ';', X, '[', R (0), ']', '=', a, ';')
ENDIF
	XCODE (R (for), '(', ';', ';', ')', '{', R (if), '(', STATEF (m->expr1), ')', '{')

	/////// if macth subexpr
IF (fixed)		// increase a by fixed displacement
	XCODE (a, ASSIGNA, Nfix, ';')
ELSE			// or by state1 success
	XCODE (a, '=', STORVAR (1), ';')
ENDIF
IF (xused)		// if counting times, times++
	XCODE (PLUSPLUS, x, ';')
ENDIF
IF (save_displ)
	XCODE (X, '[', x, ']', '=', a, ';')
ENDIF

IF (max > 1)		// if the repetitioner is '?' don't go for more
 IF (havemax)		// no more than maximum
	XCODE (R (if), '(', x, '<', Nmax, ')')
 ELIF (unlimited)	// and no more than the sizeof the displacement tbl
	XCODE (R (if), '(', x, '<', new_value_int (regexp_partition), ')')
 ENDIF
	XCODE (R (continue), ';')
ENDIF

IF (unlimited)		// recursion because sizeof displacement tbl exceeded
	XCODE (R (if), '(', STATEF (m->this), ')', RETURN1, ';')
ENDIF
	XCODE ('}', R (break), ';', '}')

IF (m->save_fail)	// save maximum match if have to
	XCODE (j, '=', a, ';')
ENDIF

IF (m->backtrack < 0)		// if not backtracking loop down to minimum
	XCODE (R (while), '(', x, MINUSMINUS, GEQCMP, Nmin, ')')
ELIF (m->backtrack > 0)		// or down to minimum but no more than 'backtrack' value (>0)
	XCODE (R (int), i, '=', new_value_int (m->backtrack + 1), ';')
	XCODE (R (while), '(', x, MINUSMINUS, GEQCMP, Nmin, ANDAND, i, MINUSMINUS, ')')
ENDIF
IF (!m->backtrack && havemin)	// if just once (no loop) ensure >= min
	XCODE (R (if), '(', x, GEQCMP, Nmin, ANDAND, state_func (m->rstate), '(')
ELSE				// ... otherwise, it is definitelly >= min
	XCODE (R (if), '(', state_func (m->rstate), '(')
ENDIF
IF (fixed)		// if fixed pass 'a' to next-state
	XCODE (a, ')', ')', RETURN1, ';')
 IF (m->backtrack)	// if looping, decrease 'a' by fixed displacement
	XCODE (R (else), a, ASSIGNS, Nfix, ';')
 ENDIF
ELSE			// otherwise, if non fixed use the values from the displacement tbl
	XCODE (X, '[', x, '+', R (1), ']', ')', ')', RETURN1, ';')
ENDIF
	XCODE (RETURN0, ';', '}')
}

//--------------------------------------------
// non greedy repetitioner -- unroll
//--------------------------------------------
static void gen_nongreedy_loop_state (subregular *m)
{
	BRING (a) BRING (x)
	int min = m->min, max = m->max;
	bool havemin = min != 0, havemax = max != regexp_INFTY, havetimes = havemin || havemax;
	bool fixed = sr_pool [m->expr1]->from == sr_pool [m->expr1]->to;
	Token Nmin = new_value_int (min), Nmax = new_value_int (max);
	Token Nfix = new_value_int (sr_pool [m->expr1]->to);
	Token j;

	gen_state (m->expr1); gen_state (m->rstate);

	XCODE (PROTO (m->this), '{')
IF (havetimes)		// we are counting times
	XCODE (R (int), x, '=', R (0), ';')
ENDIF
	XCODE (R (for), '(', ';', ';', ')', '{')
IF (minlen)
	XCODE (R (if), '(', a, '>', R_minlen, ')', RETURN0, ';')
ENDIF
	////////// early match
IF (havemin)		// ok if more than minimum
	XCODE (R (if), '(', x, GEQCMP, Nmin, ')', '{')
ENDIF
	XCODE (R (if), '(', STATEF (m->rstate), ')', RETURN1, ';')
IF (m->use_fail)	// jump after failuer of next state
 IF (havetimes)
	XCODE (x, ASSIGNA, JMPVAR (m->use_fail), '-', a, ';')
 ENDIF
	XCODE (a, '=', JMPVAR (m->use_fail), ';')
ENDIF
IF (havemin)
	XCODE ('}')
ENDIF
	////////// repeat
	XCODE (R (if), '(', '!', '(')
IF (havemax)		// if beyond maximum, fail
	XCODE (x, '<', Nmax, ANDAND)
ENDIF
	XCODE (STATEF (m->expr1), ')', ')')
IF (m->save_fail)	// in failure case save it as maximum walk
	DECLAREJMP (j, m->save_fail)
	XCODE (COMPOUND (j, '=', a, ';', RETURN0, ';'))
ELSE			// ... or just return 0
	XCODE (RETURN0, ';')
ENDIF
IF (fixed)		// increase 'a' by fixed displacement
	XCODE (a, ASSIGNA, Nfix, ';')
ELSE
/* XXXXXX non fixed */
/* not implemented, so we mark non-greedy on non-fixed fsckd. rare anyway */
ENDIF
IF (havetimes)		// if counting times at all, increment
	XCODE (PLUSPLUS, x, ';')
ENDIF
	XCODE ('}', '}')
}
//--------------------------------------------
// non greedy repetitioner -- recursive
//--------------------------------------------
static void gen_nongreedy_state (subregular *m)
{
	BRING (a) BRING (x)
	int min = m->min, max = m->max;
	bool havemin = min != 0, havemax = max != regexp_INFTY, havetimes = havemin || havemax;
	Token Nmin = new_value_int (min), Nmax = new_value_int (max);

	gen_state (m->expr1); gen_state (m->rstate);

	XCODE (PROTO (m->this), '{')
IF (havetimes)
	XCODE (R (static), R (int), x, ';')
ENDIF
IF (havemin)
	XCODE (R (if), '(', x, GEQCMP, Nmin, ')')
ENDIF
	XCODE (R (if), '(', STATEF (m->rstate), ')', '{')
IF (havetimes)
	XCODE (x, '=', R (0), ';')
ENDIF
	XCODE (RETURN1, ';', '}')
IF (havemax)
	XCODE (R (if), '(', x, '>', Nmax, ')', COMPOUND (x, '=', R (0), ';', RETURN0, ';'))
ENDIF
IF (havetimes)
	XCODE (PLUSPLUS, x, ';')
ENDIF
	XCODE (R (return), STATEF (m->expr1), ';', '}')
}

//--------------------------------------------
// greedy repetitioner -- recursive
//--------------------------------------------
static void gen_greedy_state (subregular *m)
{
	BRING (a) BRING (x)
	int min = m->min, max = m->max;
	bool havemin = min != 0, havemax = max != regexp_INFTY, havetimes = havemin || havemax;
	Token Nmin = new_value_int (min), Nmax = new_value_int (max);

	gen_state (m->expr1); gen_state (m->rstate);

	XCODE (PROTO (m->this), '{')
IF (havetimes)
	XCODE (R (static), R (int), x, ';')
ENDIF
IF (havemax)
	XCODE (R (if), '(', x, '<', Nmax, ')', '{')
ENDIF
IF (havetimes)
	XCODE (PLUSPLUS, x, ';')
ENDIF
	XCODE (R (if), '(', STATEF (m->expr1), ')', '{')
IF (havetimes)
	XCODE (MINUSMINUS, x, ';')
ENDIF
	XCODE (RETURN1, ';', '}')
IF (havetimes)
	XCODE (R (else), MINUSMINUS, x, ';')
ENDIF
IF (havemax)
	XCODE ('}')
ENDIF
IF (havemin)
	XCODE (R (if), '(', x, '<', Nmin, ')', RETURN0, ';')
ENDIF
	XCODE (R (return), STATEF (m->rstate), ';', '}')
}

//--------------------------------------------
// "this or that" with a switch()
//--------------------------------------------
static void gen_switch_state (subregular *m)
{
	int i, j, h;
	long long utbl [258], v;
	BRING (a)

	memcpy (utbl, m->swtbl, sizeof utbl);
	for (i = 0; i < 258; i++) {
		if (!(v = utbl [i])) continue;
		for (j = i; j < 258; j++)
			if (utbl [j] == v) utbl [j] &= ~v;
		for (h = j = 0; j < 64; j++)
			if (v & (1LL << j))
				gen_state (m->expr [j]);
	}

	memcpy (utbl, m->swtbl, sizeof utbl);
	XCODE (PROTO (m->this), '{', R (switch), '(', '*', a, ')', '{')
	for (i = 0; i < 258; i++) {
		if (!(v = utbl [i])) continue;
		for (j = i; j < 258; j++)
			if (utbl [j] == v) {
				XCODE (R (case), new_value_int (j), ':')
				utbl [j] = 0;
			}
		XCODE (R (return))
		for (h = j = 0; j < 64; j++)
			if (v & (1LL << j)) {
				if (h) XCODE (OROR)
				h = 1;
				XCODE (STATEFA1 (m->expr [j]))
			}
		XCODE (';')
	}
	XCODE (R (default), ':', RETURN0, ';', '}', '}')
}

//--------------------------------------------
// return "this or that"
//--------------------------------------------
static void gen_or_state (subregular *m)
{
	int i;
	BRING (a)

	for (i = 0; i < m->n; i++)
		gen_state (m->expr [i]);

	XCODE (PROTO (m->this), '{', R(return))
	for (i = 0; i < m->n; i++)
		XCODE (STATEF (m->expr [i]), i == m->n - 1 ? ';' : OROR)
	XCODE ('}')
}

//--------------------------------------------
// do actual check for a character match
//--------------------------------------------
static void gen_action_state (subregular *m)
{
	int atom = m->expr1, b, s, i, adv = RESERVED_1;
	BRING (a) BRING (x)

	gen_state (m->rstate);

	XCODE (PROTO (m->this), '{')

IF (!m->granted)
IF (m->strn)
	adv = new_value_int (strlen (m->strn));
	XCODE (R (if), '(', '!', regexp_case ? INTERN_strncmp : INTERN_strncasecmp)
	XCODE ('(', a, ',', new_value_string (m->strn), ',', adv, ')')
ELIF (atom >= RE_SPECIAL_CLASS && (b = count1s (atom - RE_SPECIAL_CLASS)) > 2 && b < 253 && regexp_noctbl)
	bool ones = b < 127;
	XCODE (R (int), x, '=', ones ? R (0) : R (1), ';', R (switch), '(', PA, ')')
	b = 1 << (atom - RE_SPECIAL_CLASS);
	for (i = 1; i < 256; i++)
		if (nz (ctbl [i] & b) == ones) {
			XCODE (R (case))
#ifdef CASE_RANGERS	// the heroic squad who fights the nasty alien outlaws
			int j, c;
			for (j = i + 1, c = 0; j < 256; j++)
				if (nz (ctbl [j] & b) == ones) ++c;
				else break;
			if (c >= 2) {
				XCODE (new_value_eint (i), ELLIPSIS, new_value_eint (--j), ':')
				i = j;
			} else
#endif
				XCODE (new_value_eint (i), ':')
		}
	XCODE (x, '=', ones ? R (1) : R (0), ';', R (if), '(', x);
ELSE
	XCODE (R(if), '(')
 IF (atom == RE_SPECIAL_DOT)
	XCODE (PA)
 ELIF (atom == RE_SPECIAL_SEND)
	XCODE ('!', PA)
 ELIF (atom >= RE_SPECIAL_CLASS)
	s = count1s (b = atom - RE_SPECIAL_CLASS);
  IF (s <= 2)
	for (i = 1; !(ctbl [i] & (1 << b)); i++);
	XCODE (PA, EQCMP, new_value_int (i))
   IF (s == 2)
	for (++i; !(ctbl [i] & (1 << b)); i++);
	XCODE (OROR, PA, EQCMP, new_value_int (i))
   ENDIF
  ELIF (s == 255)
	rerror ("character class whith matches nothing!");
  ELIF (s >= 253)
	for (i = 1; ctbl [i] & ~(1 << b); i++);
	XCODE (PA, NEQCMP, new_value_int (i))
   IF (s == 253)
	for (++i; ctbl [i] & ~(1 << b); i++);
	XCODE (ANDAND, PA, NEQCMP, new_value_int (i))
   ENDIF
  ELSE
	Token bit = new_value_hex (1 << (atom - RE_SPECIAL_CLASS));
	ctbl_used = true;
	XCODE (R_ctbl, '[', PA, ']', '&', bit)
  ENDIF	/* class implemented with ctbl or eq/neq */
 ELIF (atom < ' ' || atom > '~')
	XCODE (PA, EQCMP, new_value_int (atom))
 ELSE
	XCODE (PA, EQCMP, new_value_char (atom))
  IF (!regexp_case && isalpha (atom))
	XCODE (OROR, PA, EQCMP, new_value_char (atom < 'a' ? tolower (atom) : toupper (atom)))
  ENDIF	/* do lower/upper case */
 ENDIF	/* end if */
ENDIF	/* switch or if */
	XCODE (')')
ENDIF	/* !granted */


IF (m->save_fail)	// on the way to a jump saving repetitioner....
	XCODE (m->granted ? BLANKT : '{', R (if),'(',  STATEFA1 (m->rstate), ')', RETURN1, ';')
	XCODE (MINUSMINUS, JMPVAR (m->save_fail), ';', RETURN0, ';', m->granted ? BLANKT : '}')
 IF (!m->granted)
	XCODE (JMPVAR (m->save_fail), '=', a, ';')
 ENDIF
ELSE
	XCODE (R (return), state_func (m->rstate), '(', a, '+', adv, ')', ';')
ENDIF

IF (!m->granted)
	XCODE (RETURN0, ';')
ENDIF
	XCODE ('}')
}

//--------------------------------------------
// redirect to other state
//--------------------------------------------
static void gen_end_state (subregular *m)
{
	BRING (a)
	gen_state (m->rstate);
	XCODE (PROTO (m->this), '{', R (return), STATEF (m->rstate), ';', '}')
}
//######################################################################
//
//######################################################################

static Token catREsym (char *s)
{
	char tmp [200];
	sprintf (tmp, "%s%s", expand (R_name), s);
	return enter_symbol (strdup (tmp));
}
static Token catRE (char *s)
{
	char tmp [200];
	sprintf (tmp, "%s%s", expand (R_name), s);
	return new_symbol (strdup (tmp));
}
static Token state_var (char *s, int i)
{
	char tmp [200];
	sprintf (tmp, "%s%s%i", expand (R_name), s, i);
	return new_symbol (strdup (tmp));
}
static Token state_func (int i)
{
	char tmp [200];
	sprintf (tmp, "%s_state%i", expand (R_name), i);
	return new_symbol (strdup (tmp));
}
static Token new_value_char (int i)
{
	char tmp [20];
	sprintf (tmp, "'%c'", i);
	return new_symbol (strdup (tmp));
}
static Token new_value_eint (int i)
{
	char tmp [20];
	if (i < ' ' || i > '~')
	sprintf (tmp, "'\\x%x'", i);
	else
	sprintf (tmp, "'%c'", i);
	return new_symbol (strdup (tmp));
}

static Token new_value_hex (int i)
{
	char tmp [20];
	sprintf (tmp, "0x%x", i);
	return new_symbol (strdup (tmp));
}

//######################################################################
//
//######################################################################
//		Entries to this module, inits and globals
//######################################################################
static struct {
	bool istatic, iextern, iinline;
} regexp_dclql;
#define STATIC regexp_dclql.istatic ? R (static) : BLANKT
#define EXTERN regexp_dclql.iextern ? R (extern) : BLANKT
#define INLINE regexp_dclql.iinline ? R (inline) : BLANKT

static void export_ctbl ()
{
	char tmp [20];
	int i;

	if (!nclasses || !ctbl_used) return;
	if (nclasses > 31)
		rerror ("Too many character classes. Can do only 32");

	outprintf (REGEXPD, R (const), R (static), !regexp_packed ? R (int)
		   : nclasses < 8 ? R (char) : nclasses < 16 ? R (short)
		   : R (int), R_ctbl, '[', ']', '=', '{', -1);
	for (i = 0; i < 258; i++) {
		sprintf (tmp, "0x%x,", ctbl [i]);
		outprintf (REGEXPD, new_symbol (strdup (tmp)), -1);
	};
	outprintf (REGEXPD, '}', ';', -1);
}

static typeID typeID_matchf;
static void export_once ()
{
static	int have;
	if (have) return;
	have = 1;

	//
	// declare and make known the charp_len structure which holds extracts
	//
	BRING (p) BRING (i)
	outprintf (GLOBAL, R (struct), R (charp_len), '{', R (unsigned), R (char), '*',
		   p, ';', R (int), i, ';', '}', ';', -1);
	recID rr = enter_struct (RESERVED_charp_len, true, 0, 0, 0, 0);
	add_variable_member (rr, p, typeID_charP, 0, false, false);
	add_variable_member (rr, i, typeID_int, 0, false, false);
	complete_structure (0, rr);
	int p0 [] = { rr, '*', -1 };
	int p1 [] = { B_SINT, '(', typeID_charP, enter_type (p0), INTERNAL_ARGEND, -1 };
	typeID_matchf = enter_type (p1);
}

static void make_match_function (char *re)
{
	BRING (a) BRING (i) BRING (X) BRING (p)
	int j;
	Token f = catRE ("_match"), n;
	Token recipe = catRE ("_recipe");

IF (!regexp_anon)
	XCODE (STATIC, R (const), R (char), recipe, '[', ']', '=',
		new_symbol (escape_c_string (re, strlen (re))), ';')
ENDIF
	XCODE (STATIC, INLINE, R (int), f, '(', R (const), R (char), '*', a, ',', R (struct))
	XCODE (R (charp_len), X, '[', ']', ')', '{')
IF (!fixed_start)
	rerror ("Not prepared to handle this kind of regexp. Please move '^' out");
ENDIF
IF (minlen)
	XCODE (R_minlen, '=', a, '+', '(', R (strlen), '(', a, ')', '-')
	XCODE (new_value_int (minlen), ')', ';')
ENDIF
IF (nextr)
 IF (ambiguous_extracts)
	for (j = 0; j < nextr; j++)
		if (extract_point [j].ambig)
			XCODE (STORVAR (extract_point [j].ss), '=', R (0), ';')
 ENDIF
	XCODE (R (if), '(', STATEF (root), ')', '{', R (if), '(', X, ')', '{')
	for (j = 0; j < nextr; j++) {
#define UNCONST '(', R (unsigned), RGType, '*', ')'
		n = new_value_int (j);
	IF (extract_point [j].fixedlen == -1)
		XCODE (X, '[', n, ']', '.', i, '=', STORVAR (extract_point [j].se), '-', '(')
		XCODE (X, '[', n, ']', '.', p, '=', UNCONST, STORVAR (extract_point [j].ss),')',';')
	ELSE
		XCODE (X, '[', n, ']', '.', p, '=', UNCONST, STORVAR (extract_point [j].ss), ';')
		XCODE (X, '[', n, ']', '.', i, '=', new_value_int(extract_point [j].fixedlen),';')
	ENDIF
	}
	XCODE ('}', RETURN1, ';', '}', RETURN0, ';', '}')
ELSE
	XCODE (R (return), STATEF (root), ';', '}')
ENDIF
}

static Token regexp_declarations (bool def)
{
	export_once ();
	BRING (X) BRING (a);
	Token f = catREsym ("_match");
	Token fproto [] = {
		STATIC, EXTERN, INLINE,
		R (int), f, '(', R (const), R (char), '*', a, ',', R (struct),
		R (charp_len), X, '[', ']', ')', -1
	};
	Token recipe = catREsym ("_recipe");
	Token xargs [] = { a, X, -1 };
	Token *dflt1 = mallocint (2);
	dflt1 [0] = R (0);
	dflt1 [1] = -1;
	Token **dflt = (Token**) malloc (2 * sizeof *dflt);
	dflt [0] = dflt1;
	dflt [1] = 0;
	xdeclare_function (&Global, f, f, typeID_matchf, fproto, xargs, 0, dflt, 0);
	enter_global_object (recipe, typeID_charP);
	if (!def) {
	regexp_dclql.iextern = !regexp_dclql.istatic;
	outprintf (GVARS, EXTERN, STATIC, R (const), R (char), recipe, '[', ']', ';', -1);
	}
	return f;
}

static Token regexp_compiler (char *re)
{
	unsigned int sctbl [258];
	ctbl = sctbl;
	subregular *ssr_pool [512];
	sr_pool = ssr_pool;

	export_once ();
	R_ctbl = catRE ("_ctbl");
	char tmp [200];
	sprintf (tmp, "/* REGEXP: %s */\n", re);
	outprintf (REGEXPD, new_symbol (strdup (tmp)), -1);
	regexp_recipe = re;
#ifdef	DEBUG
	if (debugflag.REGEXP_DEBUG)
		PRINTF ("Regexp: %s\n", re);
#endif
	prepare_regexp (re);
	gen_regexp_code ();
	free_subregs ();
	export_ctbl ();
	make_match_function (re);
	return regexp_declarations (1);
}

static NormPtr parse_regexp_dcl (NormPtr p)
{
	if (!issymbol (R_name = CODE [p++]))
		parse_error (p, "No name for RegExp");
	if (CODE [p] != '(') {
		regexp_declarations (0);
		return p;
	}

	if (CODE [p++] != '(')
		parse_error (p, "Syntax error in RegExp");
	if (!ISVALUE (CODE [p]) || type_of_const (CODE [p]) != typeID_charP)
		parse_error (p, "No regular expression string!");
	char *re = expand (CODE [p++]);
	char *req = (char*) alloca (strlen (re));
	strcpy (req, re + 1);
	req [strlen (req) - 1] = 0;

	regexp_strfuncs = regexp_case = 1;
	regexp_anon = regexp_packed = regexp_noextract = regexp_noctbl = 0;
	if (CODE [p] == ',') {
		for (++p; ISSYMBOL (CODE [p]); p++)
		if (!tokcmp (CODE [p], "NOCASE")) regexp_case = 0;
		else if (!tokcmp (CODE [p], "PACKED")) regexp_packed = 1;
		else if (!tokcmp (CODE [p], "NOEX")) regexp_noextract = 1;
		else if (!tokcmp (CODE [p], "NOCTBL")) regexp_noctbl = 1;
		else if (!tokcmp (CODE [p], "NOSTRFUNC")) regexp_strfuncs = 0;
		else parse_error (p, "Unrecognized regexp option");
        }
	if (CODE [p++] != ')')
		parse_error (p, "RegExp definition");

	REGEXP = new_stream ();
	REGEXPD = new_stream ();
	regexp_compiler (req);
	concate_streams (concate_streams (REGEXP_CODE, REGEXPD), REGEXP);

	return p;
}

static NormPtr parse_regexp_uclass (NormPtr p)
{
	if (CODE [++p] != '(') goto aerr;
	if (!isvalue (CODE [++p])) goto aerr;
	char *cc = expand (CODE [p++]);
	if (CODE [p++] != ',') goto aerr;
	if (!ISVALUE (CODE [p])) goto aerr;
	char *cd = expand (CODE [p++]), ce [64];
	if (CODE [p++] != ')') goto aerr;
	if (cc [0] != '\'' || !islower (cc [1])
	|| cd [0] != '"' || cd [1] != '[') goto aerr;
	strcpy (ce, cd + 1);
	ce [strlen (ce) - 1] = 0;
	/* where are regexps when you need them? */
	if (cc [1] == 'r' || cc [1] == 'a' || cc [1] == 't'
	|| cc [1] == 'n' || cc [1] == 'f' || cc [1] == 'x')
		parse_error (p, "Abbreviation of reserved character");
	uclass [OFFS (cc [1])].def = strdup (ce);
	return p;
aerr:
	parse_error (p, "abbrev syntax error:  abbrev ('lowercase char', \"[string]\") ");
	return 0;
}

/******************************
Perl style =~ in expressions
******************************/
Token perlop_regexp (Token recipe)
{
	regexp_dclql.istatic = 1;
	regexp_dclql.iextern = 0;
	regexp_dclql.iinline = 1;
	regexp_noctbl = 0;
	regexp_strfuncs = regexp_anon = regexp_case = regexp_packed = regexp_noextract = 1;
	char *re = expand (recipe);
	char *req = (char*) alloca (strlen (re));
	strcpy (req, re + 1);
	req [strlen (req) - 1] = 0;
	REGEXP = new_stream ();
	REGEXPD = new_stream ();
	R_name = name_anon_regexp ();
	recipe = regexp_compiler (req);
	concate_streams (concate_streams (REGEXP_CODE, REGEXPD), REGEXP);
	return recipe;
}

/******************************
Parse a RegExp, declaration
******************************/
NormPtr parse_RegExp (NormPtr p, int ql)
{
	regexp_dclql.istatic = ql & 1;
	regexp_dclql.iextern = ql & 2;
	regexp_dclql.iinline = ql & 4;
	do {
		p = CODE [p] == RESERVED_abbrev ? parse_regexp_uclass (p) : parse_regexp_dcl (p);
		if (CODE [p] != ',' && CODE [p] != ';')
			parse_error (p, "RegExp declaration separator");
	} while (CODE [p++] != ';');
	return p;
}
