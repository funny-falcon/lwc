#include "global.h"

extern bool special_debug;
//******************************************************************************
//		Database of the locations of function definitions
//		&& parsing of all of them later
//		&& re-parsing of auto-functions with checks
//******************************************************************************

Token *func_prologue;
typeID return_typeID;

typedef struct funcd_t {
	struct funcd_t *next;
	Token	name, cname;
	recID	object;
	bool	have_this;
	Token	*dclstr;
	Token	*argv;
	typeID	*argt;
	NormPtr	body;
	NormPtr	alias;
	typeID	ret_type;
	bool	is_destructor;
	bool	polymorph;
	bool	done;
	bool	virt;
	Token	in_abstract;
} funcd;

static struct {
	funcd *first, *last;
} definition [3];

static void _store_definition
 (Token name, Token *dclstr, Token *argv, typeID *argt, NormPtr body,
  recID r, typeID rett, Token cname, bool b, int ff, bool poly, bool have_this,
  NormPtr alias)
{
	funcd *f = (funcd*) malloc (sizeof * f);
	f->is_destructor = b;
	f->name = name;
	f->cname = cname;
	f->dclstr = intdup (dclstr);
	f->argv = intdup (argv);
	f->argt = argtdup (argt);
	f->body = body;
	f->alias = alias;
	f->object = r;
	f->next = 0;
	f->ret_type = rett;
	f->polymorph = poly;
	f->have_this = have_this;
	f->in_abstract = bt_macro;
	f->virt = 0;
	f->done = 0;
	if (definition [ff].last) definition [ff].last->next = f;
	else definition [ff].first = f;
	definition [ff].last = f;
}

void store_definition
 (Token name, Token *dclstr, Token *argv, typeID *argt, NormPtr body,
  recID r, typeID rett, Token cname, bool b, deftype dt, bool ht)
{
	_store_definition (name, dclstr, argv, argt, body, r, rett,
			   cname, b, dt, true, ht, -1);
}

void store_definition_alias
 (Token name, Token *dclstr, Token *argv, typeID *argt, NormPtr alias,
  recID r, typeID rett, Token cname, bool b, deftype dt, bool ht)
{
	_store_definition (name, dclstr, argv, argt, -1, r, rett,
			   cname, b, dt, true, ht, alias);
}

#define CODELESS -1

void store_define_dtor (Token name, recID r)
{
	Token argv [] = { -1 };
	typeID argt [] = { pthis_of_struct (r), INTERNAL_ARGEND };
	Token dclstr [] = {
			RESERVED_static, RESERVED_inline, RESERVED_void,
			'*', name, '(', RESERVED_struct,
			name_of_struct (r), '*', RESERVED_this, ')', -1
		};
	_store_definition (name, dclstr, argv, argt, CODELESS, r,
			   typeID_voidP, name, true, DT_NORM, true, true, -1);
}

/**********************************************************************
	Rename things on overload
**********************************************************************/
void rename_fdb (Token on, Token nn)
{
	int i;
	funcd *f;

	for (i = DT_NORM; i <= DT_AUTO; i++)
		for (f = definition [i].first; f; f = f->next)
			if (f->name == on) {
				intsubst (f->dclstr, on, nn);
				f->name = nn;
				break;
			}
}

void remove_struct_from_def (Token n)
{
	int i;
	funcd *f;

	for (i = DT_NORM; i <= DT_AUTO; i++)
		for (f = definition [i].first; f; f = f->next)
			if (f->name == n) {
				remove_struct_from_this (f->dclstr, f->object);
				break;
			}
}
/**********************************************************************
	--- Usage && comparison
	record usage of important things to be able to
	compare whether two functions are the same
**********************************************************************/
typedef enum { AF_NOTYET, AF_TRUE, AF_FALSE, AF_DEPENDS } AutoBool;
enum { USE_END, USE_TCONST, USE_FCALL, USE_VVAR, USE_MEMB, USE_UPCAST, USE_LTDEF, USE_TYPEID };

typedef struct {
	int status;
	union {
		Token t;
		Token *s;
		recID r;
		typeID T;
		struct {
			Token t1, t2;
		} tt;
	} u;
} usaged;

static bool textok = true;
static bool calls_pure = false;
static usaged *udata;
static int nudata, nudata_alloc;

static int add_udata ()
{
#define CHUNK 32
	if (nudata + 1 >= nudata_alloc)
		udata = (usaged*) realloc (udata, sizeof (usaged) * (nudata_alloc += CHUNK));
	return nudata++;
}

void usage_tconst (Token v)
{
	if (textok) {
		int n = add_udata ();
		udata [n].status = USE_TCONST;
		udata [n].u.t = v;
	}
}

void usage_typeID (typeID t)
{
	if (textok) {
		int n = add_udata ();
		udata [n].status = USE_TYPEID;
		udata [n].u.T = t;
	}
	if (base_of (t) == B_PURE) {
		usage_call_pure ();
		raise_skip_function ();
	}
}

void usage_fcall (Token v)
{
	if (textok) {
		int n = add_udata ();
		udata [n].status = USE_FCALL;
		udata [n].u.t = v;
	}
}

void usage_notok ()
{
	textok = false;
}

void usage_call_pure ()
{
	usage_fcall (RESERVED_0);
	calls_pure = true;
}

void usage_vvar (Token *t)
{
	if (!t) usage_notok ();
	else if (textok) {
		int n = add_udata ();
		udata [n].status = USE_VVAR;
		udata [n].u.s = t;
	}
}

void usage_memb (Token v)
{
	if (textok) {
		int n = add_udata ();
		udata [n].status = USE_MEMB;
		udata [n].u.t = v;
	}
}

void usage_set_pure ()
{
	calls_pure = true;
}

void usage_upcast (recID r)
{
	if (textok) {
		int n = add_udata ();
		udata [n].status = USE_UPCAST;
		udata [n].u.r = r;
	}
}

static usaged *reset_udata ()
{
	textok = true;
	calls_pure = false;
	if (!udata) return 0;

	int n = add_udata ();
	udata [n].status = USE_END;
	usaged *r = realloc (udata, sizeof (usaged) * nudata);
	nudata = nudata_alloc = 0;
	udata = 0;
	return r;
}

static AutoBool is_another_autof (Token);

/* AutoBool means:
	AF_TRUE: redefine
	AF_FALSE: dispatch
	AF_DEPENDS: depends
 */
static int compare_usages (usaged *u1, recID rd, usaged *u2, recID rb, Token deps[])
{
	int i, nd = 0, j;

	if (!u1) return AF_FALSE;

	for (i = 0; u1 [i].status != USE_END; i++)
		switch (u1 [i].status) {
		 case USE_LTDEF:
		 case USE_TCONST:
			if (u1 [i].u.t != u2 [i].u.t) return AF_TRUE;
		ncase USE_TYPEID:
			if (u1 [i].u.T != u2 [i].u.T) return AF_TRUE;
		ncase USE_FCALL:
			if (u1 [i].u.t == u2 [i].u.t) continue;
			switch (is_another_autof (u1 [i].u.t)) {
			case AF_TRUE: return AF_TRUE;
			case AF_FALSE: continue;
			default:
                            for (j = 0; j < nd; j++)
				if (deps [j] == u1 [i].u.t)
					break;
                            if (j == nd)
				deps [nd++] = u1 [i].u.t;
			}
		ncase USE_VVAR:
			if (intcmp (u1 [i].u.s, u2 [i].u.s))
				return AF_TRUE;

		ncase USE_MEMB: {
			Token p1 [64], p2 [64], *p;
			lookup_variable_member (rd, u1[i].u.t, p1, true, 0);
			lookup_variable_member (rb, u1[i].u.t, p2 + 2, true, 0);
			p2 [1] = is_ancestor (rd, rb, &p, true) == 2 ? POINTSAT : '.';
			p2 [0] = p [0];
			if (intcmp (p1, p2))
				return AF_TRUE;
			}
		ncase USE_UPCAST: 
			/* if different, will catch fcalls */
			if (u1 [i].u.r != u2 [i].u.r)
				continue;
			{
			Token *p1, p2 [64], *p;
			is_ancestor (rd, u1 [i].u.r, &p1, true);
			if (rb != u1 [i].u.r) {
				is_ancestor (rb, u1 [i].u.r, &p, true);
				intcpy (p2 + 2, p);
				p2 [1] = is_ancestor (rd, rb, &p, true) == 2 ? POINTSAT : '.';
				p2 [0] = p [0];
				if (p2 [2] == -1) p2 [1] = -1;
				if (p [0] == -1) intcpy (p2, p2 + 2);
			} else {
				is_ancestor (rd, rb, &p, true);
				intcpy (p2, p);
			}
			if (intcmp (p1, p2))
				return AF_TRUE;
			}
		}

	deps [nd] = -1;

	return nd ? AF_DEPENDS : AF_FALSE;
}
/**********************************************************************
	--- fwd prototype
**********************************************************************/
static OUTSTREAM do_function_catch (funcd*);
/**********************************************************************
	--- Auto functions, store & instantiate
**********************************************************************/
static struct adf {
	struct adf *next;
	Token dname, fname, pname;
	recID r;
	Token *proto, *argv, *dcltext;
	struct adf **callers;
	bool textok, cpure;
	usaged *udata;
	int status;
	Token *deps;
	bool have_this;
	bool virt;
	bool aliasing;
	bool aliased;
	int ncallers;
	typeID ft;
} *first, *last;

static intnode * autoftree;

void commit_auto_define (Token dname, recID r, Token fname, Token pname, bool virt, Token *proto, Token *argv, typeID ft)
{
	struct adf *a = (struct adf*) malloc (sizeof * a);
	a->next = 0;
	a->dname = dname;
	a->fname = fname;
	a->pname = pname;
	a->r = r;
	a->status = AF_NOTYET;
	a->udata = 0;
	a->deps = 0;
	a->dcltext = 0;
	a->proto = intdup (proto);
	a->argv = intdup (argv);
	a->callers = 0;
	a->virt = virt;
	a->aliasing = 0;
	a->aliased = 0;
	a->ncallers = 0;
	a->cpure = false;
	a->ft = ft;

	if (last) last->next = a;
	else first = a;
	last = a;
	union ival u = { .p = a };
	intadd (&autoftree, a->fname, u);
}

static void add_caller (struct adf *a, struct adf *c)
{
	if (!a->ncallers)
		a->callers = (struct adf**) malloc (32 * sizeof *a->callers);
	else if (a->ncallers % 32 == 0)
		a->callers = (struct adf**) realloc (a->callers, a->ncallers += 32);
	a->callers [a->ncallers++] = c;
}

static AutoBool is_another_autof (Token t)
{
	intnode *n = intfind (autoftree, t);
	if (!n) return AF_TRUE;
	struct adf *a = (struct adf*) n->v.p;
	return a->status == AF_NOTYET ? AF_DEPENDS : a->status;
}

static AutoBool check_dependancies (Token dep[])
{
	int i, r = AF_FALSE;

	for (i = 0; dep [i] != -1; i++)
	switch (((struct adf*) intfind (autoftree, dep [i])->v.p)->status) {
	case AF_TRUE: return AF_TRUE;
	case AF_DEPENDS: r = AF_DEPENDS;
	}

	return r;
}

/* the case where the definition of an auto-function is
 * { PARENT_CLASS }
 * make f->body point to parent functions definition body
 * if available or return false.  */
static bool autofork (funcd *f)
{
	funcd *s;
	NormPtr p = f->body;

	if (p == -1 || !(issymbol (CODE [p]) && CODE [p + 1] == '}'))
		return true;

	Token o = CODE [p];
	recID r = lookup_object (o);

	if (!r) parse_error (p, "Not an class in auto-start");
	if (!isancestor (f->object, r)) parse_error (p, "Not derrived");

	for (s = definition [DT_NORM].first; s; s = s->next)
		if (s->cname == f->cname && s->object == r
		&& arglist_compare (s->argt, f->argt)) {
			f->body = s->body;
			for (p = 0; f->argv [p] != -1; p++)
				intsubst1 (f->dclstr, f->argv [p], s->argv [p]);
			f->argv = s->argv;
			return true;
		}

	return false;
}

/* instantiate the definition of an auto function for a specific class
 * and then parse it */
static Token *instantiate_auto (struct adf *a)
{
	funcd *f;
	Token *dproto;
	int i;

	for (f = definition [DT_AUTO].first; f; f = f->next)
		if (f->name == a->dname)
			break;

	if (!f) return 0;
	if (!autofork (f)) return 0;

	a->have_this = f->have_this;
	a->aliasing = f->alias != -1;

	dproto = intdup (a->proto);
	for (i = 0; a->argv [i] != -1; i++)
		intsubst1 (dproto, a->argv [i], f->argv [i]);

	funcd F = {
		.name = a->fname, .cname = f->cname, .object = a->r,
		.dclstr = dproto, .argv = f->argv,
		.argt = argtdup (open_typeID (a->ft) + 3),
		.body = f->body, .alias = f->alias, .ret_type = funcreturn (a->ft),
		.is_destructor = f->is_destructor, .polymorph = false,
		.have_this = f->have_this, .virt = a->virt
	};

	return combine_output (do_function_catch (&F));
}

static AutoBool same_code (struct adf *f)
{
	struct adf *a = (struct adf*) intfind (autoftree, f->pname)->v.p;
	Token depends [64];

	if (!a->textok)
		return AF_TRUE;

	AutoBool r = compare_usages (f->udata, f->r, a->udata, a->r, depends);
	if (r == AF_DEPENDS)
		f->deps = intdup (depends);
	return r;
}

static void dispatch_to_parent (struct adf *a)
{
	int i = 0;
	struct adf *p = (struct adf*) intfind (autoftree, a->pname)->v.p;
	recID r = p->r;
	OUTSTREAM O = new_stream ();

//PRINTF ("DISPATCH TO PARENT: %s %s\n", expand (a->fname), expand (a->pname)); 
	if (HaveAliases && zero_offset (a->r, r)) {
		outprintf (printproto_si (O, a->proto, a->fname, 1), alias_func (r, a->pname), ';', -1);
		p->aliased = true;
	} else {
		outprintf (printproto (O,a->proto,a->fname, 0), '{', RESERVED_return,a->pname,'(',-1);

		if (a->have_this)
 			outprintf (O, ISTR (upcast1_this (a->r, r)), -1);
		else if (a->argv [0] != -1)
			outprintf (O, a->argv [i++], -1);

		for (; a->argv [i] != -1; i++)
			outprintf (O, ',', a->argv [i], -1);
		outprintf (O, ')', ';', '}', -1);
	}

	free (a->dcltext);
	a->dcltext = combine_output (O);
}

static void rpure_r (struct adf *a)
{
	int i;
	if (a->dcltext) {
		free (a->dcltext);
		a->dcltext = 0;
		if (a->virt) purify_vfunc (a->fname);
	}
	for (i = 0; i < a->ncallers; i++)
		rpure_r (a->callers [i]);
}

static void remove_pure ()
{
	struct adf *a, *c;
	usaged *u;
	intnode *n;
	int i;
	Token t;

	for (a = first; a; a = a->next)
		if (a->dcltext && !a->cpure && a->udata)
			for (u = a->udata, i = 0; u [i].status != USE_END; i++)
				if (u [i].status == USE_FCALL) {
					t = u [i].u.t;
					n = intfind (autoftree, t);
					if (!n) continue;
					c = (struct adf*) n->v.p;
					if (c->r == a->r)
						add_caller (c, a);

				}

	for (a = first; a; a = a->next)
		if (a->cpure) rpure_r (a);
}

void define_auto_functions ()
{
	struct adf *a, *fd;

	//
	// first, parse all of them, hypothetically
	//
	objective.recording = true;
	for (a = first; a; a = a->next)
		if ((a->dcltext = instantiate_auto (a)))
			a->textok = textok, a->cpure = calls_pure, a->udata = reset_udata ();
	objective.recording = false;

	remove_pure ();

	if (ExpandAllAutos)
		goto free_Skylarov;

	//
	// now define or dispatch depending whether
	// the two versions do different things
	//
	for (a = first; a; a = a->next)
	if (a->dcltext)
		if ((a->status = (a->pname == -1 ? AF_TRUE : same_code (a))) == AF_FALSE)
			dispatch_to_parent (a);

	//
	// autofunctions calling autofunctions
	// try to resolve known
	//
	for (fd = 0, a = first; a; a = a->next)
	if (a->status == AF_DEPENDS) {
		switch (a->status = check_dependancies (a->deps)) {
		case AF_FALSE:
			dispatch_to_parent (a);
		ncase AF_DEPENDS:
			if (!fd) fd = a;
			continue;
		}

		if (fd) {
			a = fd;
			fd = 0;
		}
	}

	//
	// circular dependancies left. suppose false
	//
	for (a = first; a; a = a->next)
		if (a->status == AF_DEPENDS)
			dispatch_to_parent (a);

free_Skylarov:
	//
	// print the code (and the prototype + linkonce)
	//
	for (a = first; a; a = a->next)
		if (a->dcltext) {
			printproto (FPROTOS, a->proto, a->fname, 1);
			bool staticinline = intchr (a->proto, RESERVED_static) || intchr (a->proto, RESERVED_inline);
			if (!a->aliasing) {
				if (!OneBigFile && !staticinline)
					output_itoken (FPROTOS, linkonce_text (a->fname));
				else if (staticinline&&0)
				/* Disabled: was for gcc alias bug */
					outprintf (FPROTOS, RESERVED___attribute__, '(', '(', RESERVED_used, ')', ')', -1);
			}
			output_itoken (FPROTOS, ';');
			outprintf (AUTOFUNCTIONS, ISTR (a->dcltext), -1);
		}

	// (Do you remember malloc?)
	for (a = first; a; a = a->next) {
		if (a->dcltext) free (a->dcltext);
		if (a->udata) free (a->udata);
		if (a->deps) free (a->deps);
		if (a->proto) free (a->proto);
		if (a->argv) free (a->argv);
		if (a->ncallers) free (a->callers);
	}
}
/**********************************************************************
	--- Abstract Class functions, store & instantiate
**********************************************************************/
/**********************************************************************
	Parse all functions
**********************************************************************/
bool may_throw;

static typeID elliptic_arg (typeID t)
{
	return typeID_elliptic (t) ? ptrup (elliptic_type (t)) : t;
}

static OUTSTREAM do_alias_function (funcd *f)
{
	recID cls = f->object;
	Token fnm = 0;
	NormPtr p = f->alias;
	OUTSTREAM o = new_stream ();

	if (ISSYMBOL (CODE [p + 1]) && CODE [p + 2] == ')')
		fnm = CODE [p + 1];
	else parse_error (p, "alias (func)");
	
	fnm = lookup_function_member_uname (&cls, fnm);
	if (objective.recording)
		usage_fcall (fnm);
#ifndef	BROKEN_ALIASES
	if (HaveAliases)
		outprintf (printproto (o, f->dclstr, f->name, 1), alias_func (cls, fnm), ';', -1);
	else
#endif
	 {
		outprintf (printproto_si (o, f->dclstr, f->name, 1), '{',
				RESERVED_return, fnm, '(',  -1);
		if (f->have_this)
			outprintf (o, RESERVED_this, -1);
		/*XXX: this will break if there are arguments. Add them! */
		outprintf (o, ')', ';', '}', -1);
	}
	return o;
}

static OUTSTREAM do_function (funcd *f)
{
#ifdef	DEBUG
	if (debugflag.FUNCPROGRESS)
		PRINTF ("FUNCTION ["COLS"%s"COLE"]\n", expand(f->name));
#endif
	f->done = true;

	/* Feature: alias functions */
	if (f->alias != -1)
		return do_alias_function (f);
	/* ~~~~~~~~~~~~~~~~~~~~~~~~ */

	int i;
	bool MAIN;	// and gcc warnings get sillier all the time....
	OUTSTREAM o = new_stream ();

	return_typeID = f->ret_type;
	if (objective.recording)
		usage_typeID (return_typeID);
	if ((MAIN = f->name == RESERVED_main))
		MainModule = true;
	objective.isdtor = f->is_destructor;

	/* abstract template class type */
	SAVE_VAR (bt_macro, f->in_abstract);
	SAVE_VAR (bt_replace, name_of_struct (f->object));
	SAVE_VAR (in_function, f->name);
	SAVE_VAR (may_throw, false);
	current_scope [++top_scope] = f->object;

	/* set expression semantics in member functions */
	if ((objective.yes = f->object != 0)) {
		objective.func = f->name;
		objective.class = f->object;
		objective.efunc = f->cname;
		objective.classP = pthis_of_struct (f->object);
		objective.have_this = f->have_this;
		objective.polymorph = f->polymorph;
	}
	open_local_scope ();
	for (i = 0; f->argt [i] != -1; i++)
		if (f->argv [i] != -1) {
			typeID t = elliptic_arg (f->argt [i]);
			enter_local_obj (f->argv [i], t);
			if (objective.recording)
				usage_typeID (t);
		} else break;


	if (objective.yes)
		printproto (o, f->dclstr, f->name, 0);
	else	outprintf (o, ISTR (f->dclstr), - 1);
	if (f->body != CODELESS)
		compound_statement (o, f->body);
	else outprintf (o, '{', '}', -1);


	/* Feature: dtors call dtors of dtorable members */
	if (f->is_destructor) {
		backspace_token (o);
		Token dtn;
		if ((dtn = idtor (f->object))) {
			make_intern_dtor (f->object);
			outprintf (o, dtn, '(', R(this), ')', ';', -1);
		}
		outprintf (o, RESERVED_return, RESERVED_this, ';', '}', -1);
		if (objective.recording)
			usage_notok ();
	}
	/* ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ */

	/* Everybody's favorite addition */
	if (MAIN)
		outprintf (o, BACKSPACE, RESERVED_return, RESERVED_0, ';', '}', -1);

	if (!may_throw) {
		xmark_nothrow (objective.yes ? FSP (f->object) : Global, f->cname, f->name);
		if (f->is_destructor) set_dtor_nothrow (f->object);
	}
//PRINTF (COLS"%i"COLE"may_throw=%s\n", may_throw, expand (f->name));
	close_local_scope ();

	RESTOR_VAR (bt_macro);
	RESTOR_VAR (bt_replace);
	RESTOR_VAR (in_function);
	RESTOR_VAR (may_throw);
	--top_scope;

	if (f->is_destructor && vdtor (f->object))
		o = dispatch_vdtor (f->object, o);

	return o;
}

static OUTSTREAM do_function_catch (funcd *f)
{
	SAVE_VAR (bt_macro, bt_macro);
	SAVE_VAR (bt_replace, bt_replace);
	SAVE_VAR (in_function, in_function);
	SAVE_VAR (top_scope, top_scope);
	SAVE_VAR (may_throw, may_throw);

	jmp_buf ENV;
	OUTSTREAM O;
	if (!setjmp (ENV)) {
		set_catch (&ENV, 0, 0, 1);
		O = do_function (f);
	} else {
		/* function has been skipped because it references pure typedef */
		O = new_stream ();
		if (f->is_destructor && idtor (f->object)) {
			make_intern_dtor (f->object);
			outprintf (INTERNAL_CODE, ISTR (f->dclstr), '{', idtor (f->object), '(',
				   RESERVED_this, ')', ';', RESERVED_return, RESERVED_this,
				   ';', '}', -1);
		}
		RESTOR_VAR (bt_macro);
		RESTOR_VAR (bt_replace);
		RESTOR_VAR (in_function);
		RESTOR_VAR (top_scope);
		RESTOR_VAR (may_throw);
	}
	clear_catch ();
	return O;
}

void do_functions ()
{
	funcd *f;

	for (f = definition [DT_NORM].first; f; f = f->next)
		if (!f->done)
			concate_streams (FUNCDEFCODE, do_function (f));
}
