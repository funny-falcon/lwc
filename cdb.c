#include "global.h"

/******************************************************************************
	enumeration constants
	only interested for the existance and not the actual value
******************************************************************************/

static intnode *enumconsttree;

void enter_enumconst (Token e, enumID id)
{
	if (intfind (enumconsttree, e))
		parse_error_ll ("enumeration constant redef");
	union ival i = { .i = id };
	intadd (&enumconsttree, e, i);
}

int is_enumconst (Token e)
{
	return intfind (enumconsttree, e) != NULL;
}

enumID id_of_enumconst (Token e)
{
	return intfind (enumconsttree, e)->v.i;
}

/******************************************************************************
	enum tags:
	only interested for the existance of an enum tag
******************************************************************************/

static intnode *enumtree;
static int en_inc;

enumID enter_enum (Token e)
{
	intnode *n;
	if ((n = intfind (enumtree, e)))
		return n->v.i;
	union ival i = { .i = en_inc };
	intadd (&enumtree, e, i);
	return en_inc++;
}

enumID lookup_enum (Token e)
{
	intnode *n = intfind (enumtree, e);
	return n ? n->v.i : -1;
}

static intnode *enumsyms;

void enter_enum_syms (enumID id, Token ev[], int n)
{
	if (intfind (enumsyms, id))
		parse_error_ll ("enum redefined");
	union ival i = { .p = memcpy (mallocint (n), ev, n * sizeof ev [0]) };
	intadd (&enumsyms, id, i);
}

Token *enum_syms (enumID e)
{
	intnode *n = intfind (enumsyms, e);
	return n ? (Token*) n->v.p + 1 : 0;
}

/******************************************************************************
	typedef:
	store/return the type of it
	if a typedef is used, print it in GLOBAL
******************************************************************************/

static intnode *typedeftree;

bool enter_typedef (Token e, typeID t)
{
#ifdef	DEBUG
	if (debugflag.TDEF_TRACE)
		PRINTF ("Enter typedef ["COLS"%s"COLE"]\n", expand (e));
#endif
	intnode *n;
	if ((n = intfind (typedeftree, e))) {
		if (n->v.i != t)
			parse_error_ll ("typedef redef");
		return 0;
	}
	union ival i = { .i = t };
	intadd (&typedeftree, e, i);
	return 1;
}

typeID lookup_typedef (Token e)
{
	intnode *n = intfind (typedeftree, e);
	return n ? n->v.i : -1;
}

/******************************************************************************
	new/delete overloaders
	a simple dictionary or class:new_func and another class:del_func
******************************************************************************/

static intnode *new_tree, *delete_tree;

Token new_wrap = RESERVED_malloc, delete_wrap = RESERVED_free;

void enter_newdel_overload (Token a, recID r, Token fn)
{
	union ival i = { .i = fn };
	intadd (a == RESERVED_new ? &new_tree : &delete_tree, r, i);
}

Token lookup_newdel_operator (Token a, recID r)
{
	intnode *n = intfind (a == RESERVED_new ? new_tree : delete_tree, r);
	return n ? n->v.i : 0;
}

/******************************************************************************
	abstract classes:
	- store location of declaration and parents
	- store location of functions declared outside the class
	- store classes that inherit from it
	Finally, do for each template class: for each derrived class:
	reparse all functions declared outside the body of the class.
******************************************************************************/

static intnode *abstracttree;

typedef struct {
	Token	name;
	Token	*par;
	recID	*rpar;
	NormPtr	p;
	recID	*der;
	bool	*derd;
	int	nder;
	NormPtr *ext;
	int	next;
} ainfo;

void enter_abstract (Token a, Token *par, Token *rpar, NormPtr p)
{
	if (is_struct (a))
		parse_error (p, "Already have a class named that");

	if (intfind (abstracttree, a))
		parse_error_tok (a, "abstract class redeclared");

	int i;

	for (i = 0; par [i] != -1; i++)
		if (!intfind (abstracttree, par [i]))
			parse_error_tok (par [i], "No such abstract class to inherit");

	ainfo *A = (ainfo*) malloc (sizeof (ainfo));
	A->name = a;
	A->p = p;
	A->par = intdup (par);
	A->rpar = intdup (rpar);
	A->nder = 0;
	A->der = 0;
	A->derd = 0;
	A->next = 0;
	A->ext = 0;
	union ival u = { .p = A };
	intadd (&abstracttree, a, u);
}

void enter_abstract_derrived (Token a, recID der)
{
	ainfo *n = (ainfo*) intfind (abstracttree, a)->v.p;

	if (n->nder % 32 == 0) {
		n->der = (recID*) realloc (n->der, sizeof (recID) * (n->nder + 32));
		n->derd = (bool*) realloc (n->derd, sizeof (bool) * (n->nder + 32));
	}
	n->derd [n->nder] = false;
	n->der [n->nder++] = der;
}

void enter_abstract_external (Token a, NormPtr p)
{
	ainfo *n = (ainfo*) intfind (abstracttree, a)->v.p;

	if (n->next % 32 == 0)
		n->ext = (recID*) realloc (n->ext, sizeof (recID) * (n->next + 32));
	n->ext [n->next++] = p;
}

bool abstract_has_special (Token a, recID spec)
{
	ainfo *n = (ainfo*) intfind (abstracttree, a)->v.p;
	int i;

	for (i = 0; i < n->nder; i++)
		if (n->der [i] == spec)
			return true;
	return false;
}

bool have_abstract (Token a)
{
	return intfind (abstracttree, a) != 0;
}

int real_abstract_parents (Token a, recID ip [])
{
	int i, j;
	intnode *n = intfind (abstracttree, a);
	Token *pp = ((ainfo*) n->v.p)->par;

	for (i = j = 0; pp [j] != -1; j++)
		i += real_abstract_parents (pp [j], &ip [i]);
	recID *rp = ((ainfo*) n->v.p)->rpar;
	for (j = 0; rp [j] != -1; j++)
		ip [i++] = rp [j];
	ip [i] = -1;
	return i;
}

NormPtr dcl_of_abstract (Token a)
{
	intnode *n = intfind (abstracttree, a);
	return n ? ((ainfo*) n->v.p)->p : -1;
}

Token *parents_of_abstract (Token a)
{
	intnode *n = intfind (abstracttree, a);
	return n ? ((ainfo*) n->v.p)->par : 0;
}

static bool do_for_class (intnode *n)
{
	ainfo *a = (ainfo*) n->v.p;
	int i, j;
	bool r = false;

	if (a->next)
		for (i = 0; i < a->nder; i++)
			if (!a->derd [i]) {
				for (j = 0; j < a->next; j++)
					reparse_template_func (a->name, a->der [i], a->ext [j]);
				a->derd [i] = true;
				r = true;
			}

	if (n->less) r |= do_for_class (n->less);
	if (n->more) r |= do_for_class (n->more);
	return r;
}

bool specialize_abstracts ()
{
	return abstracttree ? do_for_class (abstracttree) : false;
}
/******************************************************************************
	type IDs
	store/return the type string of a typeID
******************************************************************************/

static int **types;
static int ntypes, nalloc;

typeID enter_type (int *ts)
{
	int i;
	for (i = 0; i < ntypes; i++)
		if (!intcmp (types [i], ts))
			return i;
	if (ntypes == nalloc)
		types = (int**) realloc (types, (nalloc += 64) * sizeof (int*));
	types [ntypes] = intdup (ts);
	return ntypes++;
}

int *open_typeID (typeID t)
{
	return types [t];
}

bool isfunction (typeID t)
{
	return open_typeID (t) [1] == '(';
}

bool typeID_elliptic (typeID t)
{
	return open_typeID (t) [1] == B_ELLIPSIS;
}

//++++++++++++++++++++++++++++++++++++++++++++
typeID typeID_NOTYPE, typeID_int, typeID_float, typeID_charP,
       typeID_voidP, typeID_void, typeID_uint, typeID_ebn_f, typeID_intP;

/******************************************************************************
	is_typename, returns:
		0: is not
		1: is typedef
		2: is a structure without 'struct'
		3: is enum without 'enum'
		SYMBOL: it is a local typedef translated
******************************************************************************/

int is_typename (Token t)
{
	int i;
	Token l;

	if (lookup_typedef (t) != -1) return 1;
	if (is_struct (t)) return 2;
	if (is_enum (t)) return 3;
	for (i = top_scope; i; --i)
		if ((l = lookup_local_typedef (current_scope [i], t)))
			return l;
	return 0;
}

bool is_dcl_start (Token t)
{
	if (ISRESERVED (t))
		return ISAGGRSPC (t) || t == RESERVED_specialize || t == RESERVED_RegExp
			|| t == RESERVED_typeof || ISTBASETYPE(t) || ISDCLFLAG(t);
	if (!ISSYMBOL (t)) return false;
	return (!is_object_in_scope (t) && is_typename (t) > 0) || t == bt_macro
		 || t == RESERVED__CLASS_;
}

/* A name can be the name of an object or the name of a typedef'd object */
recID lookup_object (Token t)
{
	recID r;
	typeID td;

	if (t == RESERVED__CLASS_)
		return objective.class;
	if ((r = lookup_struct (t)))
		return r;
	if ((td = lookup_typedef (t)) != -1 && isstructure (td))
		return base_of (td);
	return 0;
}
/******************************************************************************
	-- definitions of template functions outside the class
******************************************************************************/

/*
 * The basetype could be class's name of an abstract class
 * 	Z *Z.foo () { }
 * 	something_undeclared Z.foo () { }
 * search for the same name followed by a dot, before ( { ; ,
 */
bool is_extern_templ_func (NormPtr *pp)
{
	NormPtr s = *pp, p = *pp;

	while (CODE [p] != '.' && CODE [p] != '(' && CODE [p] != ';'
		 && CODE [p] != ',' && CODE [p] != '{')
			++p;

	if (CODE [p--] != '.')
		return false;

	if (!have_abstract (CODE [p]))
		return false;

	while (isdclflag (CODE [--s]));
	*pp = p;

	return is_template_function (pp, s + 1);
}

bool is_template_function (NormPtr *pp, NormPtr s)
{
	NormPtr p = *pp;
	Token n;

	if (!have_abstract (n = CODE [p]))
		return false;

	if (CODE [p + 1] != '.') return false;

	enter_abstract_external (n, s);

	*pp = skip_declaration (p);

	return true;
}
/******************************************************************************
	-- global variables
******************************************************************************/

static intnode *globalobjtree;

void enter_global_object (Token e, typeID t)
{
#ifdef	DEBUG
	if (debugflag.DCL_TRACE)
		PRINTF ("DECLARING GLOBAL OBJECT ["COLS"%s"COLE"]\n", expand (e));
#endif
	intnode *n = intfind (globalobjtree, e);
	if (n && n->v.i != t)
		parse_error_tok (e, "global object redefined");
	union ival i = { .i = t };
	intadd (&globalobjtree, e, i);
}

typeID lookup_global_object (Token e)
{
	intnode *n = intfind (globalobjtree, e);
	return n ? n->v.i : -1;
}

/******************************************************************************
	local object scopes as found in declarations inside
	compound statements.
******************************************************************************/

typedef struct listobj_t {
	struct listobj_t *next;
	Token n, gn;
	typeID t;
} listobj;

typedef struct autodtor_t {
	struct autodtor_t *next;
	Token o;
	recID r;
	bool ljmp;
} autodtor;

typedef struct codescope_t {
	listobj *first;
	autodtor *dtors;

	// 1 for break, 2 for continue
	int catchpoint;

	// longbreak label name
	Token lbrk, lcnt;

	struct codescope_t *outer;
} codescope;

static codescope *inner;

void open_local_scope ()
{
	codescope *c = (codescope*) malloc (sizeof * c);
	c->first = 0;
	c->dtors = 0;
	c->catchpoint = 0;
	c->lcnt = c->lbrk = 0;
	c->outer = inner;
	inner = c;
}

void *reopen_local_scope (void *n)
{
	codescope *r = inner;
	inner = (codescope*) n;
	return r;
}

void restore_local_scope (void *n)
{
	inner = (codescope*)n;
}

Token recent_obj ()
{
	return inner->first->n;
}

void globalized_recent_obj (Token gn)
{
	inner->first->gn = gn;
}

void add_catchpoint (int ct)
{
	codescope *c = (codescope*) malloc (sizeof * c);
	c->first = 0;
	c->dtors = 0;
	c->lcnt = c->lbrk = 0;
	c->catchpoint = ct;
	c->outer = inner;
	inner = c;
}

void enter_local_obj (Token n, typeID t)
{
#ifdef	DEBUG
	if (debugflag.DCL_TRACE)
		PRINTF ("DECLARING LOCAL OBJECT ["COLS"%s"COLE"]\n", expand (n));
#endif
	listobj *l;
	for (l = inner->first; l; l = l->next)
		if (l->n == n)
			parse_error_tok (n, "local object redefined");
	l = (listobj*) malloc (sizeof * l);
	l->n = n;
	l->gn = 0;
	l->t = t;
	l->next = inner->first;
	inner->first = l;
}

static void undo_auto_destruction (Token);

void undo_local_obj (Token n)
{
	listobj *l, *p;
	for (p = 0, l = inner->first; l; l = (p = l)->next)
		if (l->n == n) break;
	if (!p) inner->first = l->next;
	else p->next = l->next;
	free (l);
	undo_auto_destruction (n);
}

typeID lookup_local_obj (Token n, Token *gn)
{
	listobj *l;
	codescope *c;
	for (c = inner; c; c = c->outer)
		for (l = c->first; l; l = l->next)
			if (l->n == n) {
				if (gn) *gn = l->gn;
				/* Feature: structure by reference */
				if (c->outer == 0 && isstructure (l->t) && by_ref (l->t))
					return l->t + REFERENCE_BOOST;
				/* ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ */
				return l->t;
			}
	return -1;
}

int close_local_scope ()
{
	listobj *l, *ln;
	for (l = inner->first; l; l = ln) {
		ln = l->next;
		free (l);
	}
	autodtor *al, *aln;
	for (al = inner->dtors; al; al = aln) {
		aln = al->next;
		free (al);
	}
	codescope *f = inner;
	inner = inner->outer;
	free (f);
	return !!inner;
}

void *active_scope ()
{
	return inner;
}

void restore_scope (void *s)
{
	while (inner != s) close_local_scope ();
}

void rmv_catchpoint (Token *lbrk, Token *lcnt)
{
	if (!inner->catchpoint) parse_error_ll ("BUG. No catchpoint!");
	if (inner->first) parse_error_ll ("BUG. Catchpoint has dcls");
	codescope *f = inner;
	*lbrk = f->lbrk;
	*lcnt = f->lcnt;
	inner = inner->outer;
	free (f);
}
/**************************************************************
	Automatic destruction of local objects
	
	most of this is unused if EHUnwind is true
		and the cleanup attribute is used instead.
***************************************************************/

void add_auto_destruction (Token obj, recID rec, bool ljmp)
{
	autodtor *na = (autodtor*) malloc (sizeof * na);
	na->next = inner->dtors;
	na->o = obj;
	na->r = rec;
	na->ljmp = ljmp;
	inner->dtors = na;
}

static void undo_auto_destruction (Token n)
{
	autodtor *a, *p;
	for (p = 0, a = inner->dtors; a; a = (p = a)->next)
		if (a->o == n) {
			if (!p) inner->dtors = a->next;
			else p->next = a->next;
			if (a->ljmp) pop_unwind (0);
			free (a);
			return;
		}
}

bool scope_has_dtors ()
{
	return inner->dtors != 0;
}

/* member 'r' is normally a recID, but if a symbol, it's
   the name of the custom dtor function.  Useful for arrdtor  */
#define DTOR_OF(x) ISSYMBOL(x) ? x : dtor_name (x)

void gen_auto_destruction (OUTSTREAM o, bool nothrow)
{
	autodtor *a;
	for (a = inner->dtors; a; a = a->next)
		if (a->o == LEAVE_ESCOPE) leave_escope (o);
		else {
			if (a->ljmp && !nothrow) pop_unwind (o);
			outprintf (o, DTOR_OF (a->r), '(', '&', a->o, ')', ';', -1);
		}
}

bool break_has_dtors (int cp)
{
	codescope *c;
	for (c = inner; c && c->catchpoint < cp; c = c->outer)
		if (c->dtors)
			return true;
	/* let C compiler complain if !c */
	return false;
}

Token gen_break_destructors (OUTSTREAM o, int cp, int depth)
{
	Token have [100] = { -1, };
	autodtor *a;
	codescope *c = inner, *d, *l = 0;

	for (;;) {
	continue_2:
		for (; c && c->catchpoint < cp; c = c->outer)
			for (a = c->dtors; a; a = a->next)
				if (a->o == LEAVE_ESCOPE) leave_escope (o);
				else {
					if (a->ljmp) pop_unwind (o);
					if (!intchr (have, a->o)) {
						outprintf (o, DTOR_OF (a->r), '(', '&',
							   a->o, ')', ';', -1);
						intcatc (have, a->o);
					} else warning_tok ("outmost dtor not called due"
							    " to name conflict!!", a->o);
				}

		/* Handle: break n */
		if (--depth && c)
			for (d = c = c->outer; d; d = d->outer)
				if (d->catchpoint >= cp) {
					l = d;
					goto continue_2;
				}
		break;
	}

	return !l ? 0 : cp == 2 ?
	 (l->lcnt ?: (l->lcnt = name_longcontinue ())) :
	 (l->lbrk ?: (l->lbrk = name_longbreak ()));
}

bool func_has_dtors ()
{
	codescope *c;
	for (c = inner; c; c = c->outer)
		if (c->dtors)
			return true;
	return false;
}

void gen_all_destructors (OUTSTREAM o)
{
	Token have [100] = { -1, };
	codescope *c;
	autodtor *a;
	for (c = inner; c; c = c->outer)
		for (a = c->dtors; a; a = a->next)
			if (a->o == LEAVE_ESCOPE) leave_escope (o);
			else {
				if (a->ljmp) pop_unwind (o);
				if (!intchr (have, a->o)) {
					outprintf (o, DTOR_OF (a->r), '(', '&', a->o, ')', ';', -1);
					intcatc (have, a->o);
				} else warning_tok ("outmost dtor not called due to name"
						" conflict!", a->o);
			}
}
