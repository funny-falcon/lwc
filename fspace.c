/*****************************************************************************

	Function namespaces.  Overloading.

Overloading happens only when we have a second declaration of
the same name with different types in the same function space.
Then some things need to be renamed and are:

	- prototype of function definition: rename_fdb
	- name of auto functions to be reparsed: rename_hier

*****************************************************************************/

#include "global.h"

/**************************************************
	Declarations, comparison of
	prototypes, default arguments
	and properties.
**************************************************/
typeID *promoted_arglist (typeID *tt)
{
	typeID tr [64];
	int i, j;

	for (j = i = 0; tt [i] != INTERNAL_ARGEND; i++)
		tr [j++] = tt [i] == B_ELLIPSIS ? B_ELLIPSIS :
			isstructure (tt [i]) ?
			ptrup (tt [i]) : bt_promotion (tt [i]);
	tr [j] = -1;
	return intdup (tr);
}

typeID *promoted_arglist_t (typeID t)
{
	return promoted_arglist (open_typeID (t) + 2);
}

static char *mangle_type (char *p, typeID t, bool promo)
{
	if (is_reference (t)) t = ptrdown (dereference (t));
	int *ot = open_typeID (t);
	int i;
	for (i = 1; ot [i] != -1; i++)
		if (ot [i] == '*' || ot [i] == '[') *p++ = 'P';
		else if (in2 (ot [i], '(', B_ELLIPSIS)) {
			*p++ = ot [i] == '(' ? 'F' : 'E';
			while (!in2 (ot [i], INTERNAL_ARGEND, -1)) i++;
			if (ot [i] == -1) break;
		} else parse_error (0, "BUG arglist");
	if (ot [0] >= 0) {
		char *tp = expand (ISSYMBOL (ot [0]) ? ot [0] :
				 name_of_struct (ot [0]));
		/* Feature: structure by reference */
		if (i == 1) *p++ = 'P';
		/* ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ */
		*p++ = 'S';
		while (*tp) *p++ = *tp++;
	} else if (ot [1] == -1 && promo) {
		if (ot [0] < B_FLOAT) *p++ = 'i';
		else if (ot [0] < B_VOID) *p++ = 'f';
		else if (ot [0] == B_VOID) *p++ = 'v';
	} else switch (ot [0]) {
#define uif if (!promo) *p++ = 'u';
		 case B_UCHAR: uif
		 case B_SCHAR: *p++ = 'c';
		ncase B_USINT: uif
		 case B_SSINT: *p++ = 's';
		ncase B_UINT: uif
		 case B_SINT: *p++ = 'i';
		ncase B_ULONG: uif
		 case B_SLONG: *p++ = 'l';
		ncase B_ULLONG: uif
		 case B_SLLONG: *p++ = 'L';
		ncase B_FLOAT: *p++ = 'f';
		ncase B_DOUBLE: *p++ = 'F';
		ncase B_LDOUBLE: *p++ = 'D';
		ndefault: *p++ = 'v';
	}
	return p;
}

char *nametype (char *ret, typeID t)
{
	*mangle_type (ret, t, 0) = 0;
	return ret;
}

bool arglist_compare (typeID *l1, typeID *l2)
{
	for (; *l1 != -1 && *l2 != -1; l1++, l2++)
		if (*l1 != *l2) {
			if (isstructptr (*l1) && isstructptr (*l2)) {
				recID r1 = aliasclass (base_of (*l1));
				recID r2 = aliasclass (base_of (*l2));
				if (r1 == r2 || is_ancestor (r1, r2, 0, 0)
				||  is_ancestor (r2, r1, 0, 0)) continue;
			}
			return false;
		}
	return *l1 == *l2;
}

char *type_string (char *r, typeID t)
{
	int argc;
	char *p = r;
	int *ot = open_typeID (t) + 2;

	for (argc = 0; *ot != INTERNAL_ARGEND; argc++)
		if (*ot == B_ELLIPSIS) *p++ = 'E';
		else p = mangle_type (p, *ot++, 1);
	sprintf (p, "%i", argc);
	return r;
}
 
static void addflag (funcp *p, Token sp)
{
	Token *n = mallocint (intlen (p->prototype) + 2);
	sintprintf (n, sp, ISTR (p->prototype), -1);
	free (p->prototype);
	p->prototype = n;
}

static funcp *_declare_function (fspace *SF, Token e, Token f, typeID t, Token *proto,
				 Token *xargs, int flagz, Token section)
{
	char st [256];
	funcp *p, *w;
	typeID *ovarglist = promoted_arglist_t (t);
	bool rename;
	Token un;
	bool used = flagz & FUNCP_VIRTUAL;
	fspace S;

	if (!(S = intfind (*SF, e))) {
		/*** first time ***/
		p = (funcp*) malloc (sizeof *p);
		*p = (funcp) {
			.next = 0, .name = f, .type = t,
			.prototype = proto ? intdup (proto) : 0,
			.ovarglist = ovarglist, .dflt_args = 0,
			.xargs = intdup (xargs), .flagz = flagz, .used = used,
			.section = section
		};
		union ival u = { .p = p };
		intadd (SF, e, u);
		return p;
	}

	p = (funcp*) S->v.p;

	for (rename = true, un = p->name, w = p; w; w = w->next)
		if (w->name != un) {
			rename = false;
			break;
		}

	if (rename) {
		/*** second time ***/
		// overload existing functions declared previously
		for (w = p; w; w = w->next)
			if (arglist_compare (ovarglist, w->ovarglist)) {
				// same function prototype again. no overload
				if (t != w->type)
{fprintf (stderr, "BAD FUNCTION[%s]\n", expand (e));
					parse_error_ll ("overloading match, type mis-match");
}
				if (flagz & FUNCP_STATIC && !intchr (w->prototype, RESERVED_static))
					addflag (w, RESERVED_static);
				if (flagz & FUNCP_INLINE && !intchr (w->prototype, RESERVED_inline))
					addflag (w, RESERVED_inline);
				free (ovarglist);
				return w;
			}
		Token nf1 = name_overload_fun (f, type_string (st, p->type));
		rename_fdb (p->name, nf1);
		rename_hier (p->name, nf1);
		for (w = p; w; p = w, w = w->next) {
			if (w->prototype)
				intsubst (w->prototype, w->name, nf1);
			w->name = nf1;
		}

		Token nf2 = name_overload_fun (f, type_string (st, t));
		if (nf2 == nf1) parse_error_tok (nf1, "Different functions, same overloaded name!");
		if (proto)
			intsubst (proto, f, nf2);
		p->next = (funcp*) malloc (sizeof *p);
		p = p->next;
		*p = (funcp) {
			.next = 0, .name = nf2, .type = t,
			.prototype = proto ? intdup (proto) : 0,
			.ovarglist = ovarglist, .dflt_args = 0,
			.xargs = intdup (xargs), .flagz = flagz, .used = used
		};
		return p;
	}

	/*** >2 time ***/
	for (w = p; w; p = w, w = w->next)
		if (arglist_compare (ovarglist, w->ovarglist)) {
			if (t != w->type)
{fprintf (stderr, "BAD FUNCTION[%s]\n", expand (e));
				parse_error_ll ("overload match, type mismatch");
}
			if (flagz & FUNCP_STATIC && !intchr (w->prototype, RESERVED_static))
				addflag (w, RESERVED_static);
			if (flagz & FUNCP_INLINE && !intchr (w->prototype, RESERVED_inline))
				addflag (w, RESERVED_inline);
			free (ovarglist);
			return w;
		}

	Token nf = name_overload_fun (f, type_string (st, t));
	for (w = p; w; p = w, w = w->next)
		if (w->name == nf)
			parse_error_tok (nf, "Error: overloaded functions boil down to same name.");
	if (proto)
		intsubst (proto, f, nf);
	p->next = (funcp*) malloc (sizeof *p);
	p = p->next;
	*p = (funcp) {
		.next = 0, .name = nf, .type = t,
		.prototype = proto ? intdup (proto) : 0,
		.ovarglist = ovarglist, .dflt_args = 0,
		.xargs = intdup (xargs), .flagz = flagz, .used = used,
		.section = section
	};
	return p;
}

static typeID rmv_last_arg (typeID ft)
{
	int *oo = open_typeID (ft);
	int *o = allocaint (intlen (oo) + 2);
	intcpy (o, oo);
	int i;
	for (i = 0; o [i] != INTERNAL_ARGEND; i++);
	while ((o [i-1] = o [i]) != -1) i++;
	return enter_type (o);
}

funcp *xdeclare_function (fspace *S, Token e, Token f, typeID t, Token *proto,
			Token *xargs, int flagz, Token **dflt, Token section)
{
#ifdef	DEBUG
	if (debugflag.DCL_TRACE) {
		PRINTF ("DECLARING FUNCTION ["COLS"%s"COLE", %s]\n", expand (f), expand (e));
		PRINTF ("dflt=%p OF TYPE: ", dflt);
		debug_pr_type (t);
		PRINTF ("flagz=%i\n", flagz);
	}
#endif
	int dargc, i;
	funcp *p;

	if (Streams_Closed) proto = 0;

	f = (p = _declare_function (S, e, f, t, proto, xargs, flagz, section))->name;
	if (!dflt) return p;

	for (dargc = 0; dflt [dargc]; dargc++);

	proto = p->prototype;
	for (i = 0, --dargc; i <= dargc; i++) {
		t = rmv_last_arg (t);
		p = p->next = (funcp*) malloc (sizeof *p);
		*p = (funcp) {
			.next = 0, .name = f, .type = t,
			.prototype = proto, .ovarglist = promoted_arglist_t (t),
			.dflt_args = &dflt [dargc - i], .section = section,
			.xargs = intdup (xargs), .flagz = flagz, .used = flagz & FUNCP_VIRTUAL 
		};
	}

	return p;
}

void xmark_section_linkonce (fspace S, Token fn, Token ofn)
{
	funcp *p = (funcp*) intfind (S, fn)->v.p;
	while (p->name != ofn)
		p = p->next;
	p->flagz |= FUNCP_LINKONCE;
	p->used = true;
}

void xmark_nothrow (fspace S, Token fn, Token ofn)
{
	intnode *n = intfind (S, fn);
	if (!n) return;
	funcp *p = (funcp*) n->v.p;
	while (p->name != ofn)
		p = p->next;
	p->flagz |= FUNCP_NOTHROW;
}

/* function is used by alias so if static inline, mark __attribute__ ((used)) */
int xmark_function_USED (fspace S, Token ofn)
{
	funcp *p;
	for (p = (funcp*) S->v.p; p; p = p->next)
		if (p->name == ofn) {
			p->flagz |= FUNCP_USED;
			return 1;
		}
	if (S->less && xmark_function_USED (S->less, ofn))
		return 1;
	return S->more ? xmark_function_USED (S->more, ofn) : 0;
}

/* lookup a declaration. exact arglist match */
funcp *xlookup_function_dcl (fspace S, Token f, typeID argv[])
{
	S = intfind (S, f);
	if (!S) return 0;

	funcp *p;
	for (p = (funcp*) S->v.p; p; p = p->next)
		if (arglist_compare (p->ovarglist, argv))
			return p;
	return 0;
}

/**************************************************
	Lookups in function spaces
**************************************************/

fspace Global;

typeID lookup_function_symbol (Token e)
{
	intnode *n = intfind (Global, e);
	if (!n) return -1;
	funcp *p = (funcp*) n->v.p;
	if (p->next)
		expr_errort ("function is overloaded. can't get address by name", e);
	p->used = 1;
	return p->type;
}

bool have_function (fspace S, Token f)
{
	return intfind (S, f) != 0;
}

Token xlookup_function_uname (fspace S, Token f)
{
	intnode *n = intfind (S, f);
	if (!n) return 0;
	funcp *p = (funcp*) n->v.p;
	if (p->next)
		parse_error_tok (f, "function is overloaded. can't get alias by name");
	p->used = 1;
	return p->name;
}

/* returns:
-1. lists, for one, have different argc
 0. call list doesn't match
 1. match after promotion
 2. exact match

The second list is the call argument list while the first
the list of the function prototype. It matters
 */
static int callist_compare (typeID *l1, typeID *l2)
{
	if (*l1 == typeID_void && *l2 == -1)
		return 2;
	int ret = 2;
	for (; *l2 != -1; l1++, l2++)
		if (*l1 != *l2) {
			if (in2 (B_ELLIPSIS, *l1, *l2) || typeID_elliptic (*l2)
			 || (*l1 >= 0 && typeID_elliptic (*l1)))
				return ret;
			if (*l1 < 0) return -1;
			/* Feature: Hierarchy matches types */
			if (isstructptr (*l1) && isstructptr (*l2)) {
				recID r1 = aliasclass (dbase_of (*l1));
				recID r2 = aliasclass (base_of (*l2));
				if (r1 == r2) continue;
				if (is_ancestor (r2, r1, 0, 0))
					goto candidate;
			}
			/* ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ */
			if (ispointer (*l1) && ispointer (*l2))
				if (*l1 == typeID_voidP || *l2 == typeID_voidP)
					goto candidate;
			if (is_reference (*l1) && bt_promotion (ptrdown (dereference (*l1))) == *l2)
				goto candidate;
			if (*l2 == typeID_int && *l1 == typeID_float)
				goto candidate;
			if (*l1 == typeID_int && *l2 == typeID_float)
				goto candidate;
			/* XXX: Comparing '0' with pointer is ok.... */
			ret = 0; continue;
		candidate:
			ret = 1;
		}
	return *l1 == *l2 || *l1 == B_ELLIPSIS || typeID_elliptic (*l1) ? ret : -1;
}

/* the return value means:
	-1: name does not even exist
	 0: no match
	 1: matches in number of arguments
	 2: -candidate- conversions match
	 3: exact match
 */
int xlookup_function (fspace S, Token e, typeID argv[], flookup *ret)
{
	S = intfind (S, e);
	funcp *o, *match = 0;
	int mt = -1, error = 0, rez;

	if (!S) return -1;
	o = (funcp*) S->v.p;
	typeID *ovarglist = promoted_arglist (argv);

	for (; o; o = o->next) {
		rez = callist_compare (o->ovarglist, ovarglist);
		if (rez == -1) continue;
		if (mt < rez) {
			mt = rez;
			match = o;
			error = 0;
		} else if (mt == rez) {
			error = 1;
		}
	}

	if (!ret) return mt + 1;

	if (error) expr_errort ("AMBIGOUS overload call", e);

	free (ovarglist);
	if (mt == -1) return 0;
	match->used = true;
	ret->oname = match->name;
	ret->t = match->type;
	if (ret->dflt_args = match->dflt_args) {
		for (o = (funcp*) S->v.p; o->name != match->name; o = o->next);
		o->used = true;
	}
	ret->xargs = match->xargs;
	ret->flagz = match->flagz;
	ret->prototype = match->prototype;
	return mt + 1;
}

/**************************************************

	Export the prototypes used

**************************************************/

OUTSTREAM printproto (OUTSTREAM O, Token proto[], Token fnm, bool palias)
{
	int i = 0;
	if (palias) {
		if (StdcallMembers)
			for (; proto [i] != -1; i++) {
				if (proto [i] == fnm) {
					output_itoken (O, RESERVED_attr_stdcall);
					break;
				}
				output_itoken (O, proto [i]);
		}
		outprintf (O, ISTR (proto + i), -1);
	} else {
		for (i = 0; proto [i] != -1; i++) {
			if (proto [i] == RESERVED___attribute__)
				break;
			output_itoken (O, proto [i]);
		}
	}
	return O;
}

OUTSTREAM printproto_si (OUTSTREAM O, Token proto[], Token fnm, bool palias)
{
	if (!intchr (proto, RESERVED_static))
		output_itoken (O, RESERVED_static);
	if (!intchr (proto, RESERVED_inline))
		output_itoken (O, RESERVED_inline);
	return printproto (O, proto, fnm, palias);
}

void export_fspace (fspace F)
{
	funcp *p;

	for (p = (funcp*) F->v.p; p; p = p->next)
		if (p->used && !p->dflt_args && p->prototype
		 && !(p->flagz & (/*FUNCP_PURE|*/FUNCP_AUTO))) {
			outprintf (FPROTOS, ISTR (p->prototype), -1);
			if (p->flagz & FUNCP_LINKONCE && !(p->flagz & FUNCP_STATIC))
				output_itoken (FPROTOS, linkonce_text (p->name));
			else if (p->section)
				outprintf (FPROTOS, RESERVED___attribute__, '(', '(',
					   RESERVED___section__, '(', p->section, ')', ')', ')',-1);
			output_itoken (FPROTOS, ';');
		}

	if (F->less) export_fspace (F->less);
	if (F->more) export_fspace (F->more);
}

void export_fspace_lwc (fspace F)
{
	funcp *p;

	for (p = (funcp*) F->v.p; p; p = p->next)
		if (p->used && !p->dflt_args && p->prototype
		 && !(p->flagz & (/*FUNCP_PURE|*/FUNCP_AUTO))) {
			printproto (FPROTOS, p->prototype, p->name, 1);
			if (p->flagz & FUNCP_LINKONCE && !(p->flagz & FUNCP_STATIC))
				output_itoken (FPROTOS, linkonce_text (p->name));
			else if (p->section)
				outprintf (FPROTOS, RESERVED___attribute__, '(', '(',
					   RESERVED___section__, '(', p->section, ')', ')', ')',-1);
			if (p->flagz & FUNCP_USED)
				outprintf (FPROTOS, RESERVED___attribute__, '(', '(', RESERVED_used, ')', ')', -1);
			output_itoken (FPROTOS, ';');
		}

	if (F->less) export_fspace_lwc (F->less);
	if (F->more) export_fspace_lwc (F->more);
}
