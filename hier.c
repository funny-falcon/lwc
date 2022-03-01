#include "global.h"

typedef int initID, vtblID;

typedef struct member_t {
	struct member_t *next;
	Token m, gn;
	typeID t;
} member;

typedef struct puredm_t {
	struct puredm_t *next;
	Token bt;
	NormPtr dcls [16];
} puredm;

typedef struct tconst_t {
	struct tconst_t *next;
	Token m, r;
} tconst;

typedef struct anonunion_t {
	struct anonunion_t *next;
	Token n;
	recID r;
} anonunion;

typedef struct localtdef_t {
	struct localtdef_t *next;
	Token tn, gn;
} localtdef;

typedef struct {
	recID to;
	Token fwd_dcl;
	initID vti;
	Token entry;
} rtti_downcast;

typedef struct {
	recID ot;
	bool array;
	Token obn;
} ctorable_t;

enum { ASTATUS_NORM, ASTATUS_VIRT, ASTATUS_VIRT2, ASTATUS_FSCKD }; // ancestor.status

typedef struct {
	recID rec;
	int status;
	Token *path;
	recID vbase;
	Token *cpath;
	bool zero_displace;
	bool direct;
	int depth;
} ancestor;

enum { AU_STATUS_ND, AU_STATUS_DD, AU_STATUS_NU, AU_STATUS_PV, AU_STATUS_PEND }; // autofunc.status

typedef struct autofunct_t {
	struct autofunct_t *next;
	Token name;
	typeID *arglist;
	Token dname, fname, pname;
	int status;
	bool virtual;
	NormPtr dclPtr;
	Token *proto, *argv;
	typeID type;
} autofunc;

enum { OK_CAN, CANT_INCOMPL, CANT_PUREV, CANT_NUFO }; // structure.caninst

typedef struct {
	Token name;
	Token *code;

	bool notunion, incomplete, deftype, class;
	Token dtor_name;
	bool has_dtor, dtor_nothrow;
	bool has_dtorable, autodtor;
	bool unwind, noctor, evt, byvalue;
	Token idtor, vdtor;

	Token keyfunc;
	bool havekey;

	bool used, printed, vt_inthis;

	// object has const members
	bool constd;

	typeID type_pthis;
	member *firstmember, *lastmember;

	// pure data members
	puredm *pm;

	// member function space
	fspace Funcs;

	// anonymous unions
	anonunion *anonunions;
	int nanons;

	// depends on declaration of these
	recID *depends;

	// ancestors
	ancestor *ancestors;

	// auto functions
	autofunc *autofuncs;

	// class constants
	tconst *consts;

	// virtual table initializers
	initID *vtis;

	// local typedefs
	localtdef *ltdef;

	// has virtual ancestors
	bool has_vbase, has_vbase2;

	// downcast info for virtual inheritance
	rtti_downcast *rtti;
	bool need_recalc_offsets;
	bool have_vi_decls;

	// members that need vt initialization
	ctorable_t *ctorables;
	bool ancestors_have_ctors;

	// can instantiate object
	int caninst;

	// class is merely an alias?
	recID alias_class;
} structure;

static structure *structs;

/******************************************************************************

	database of structures, members and hierarchies

******************************************************************************/

#define STAR_IF(x)	x ? '*' : BLANKT
#define COMMA_IF(x)	x ? ',' : BLANKT
#define ADDR_IF(x)	x ? '&' : BLANKT
#define DOT_IF(x)	x ? '.' : BLANKT
#define POINTSAT_IF(x)	x ? POINTSAT : BLANKT
#define CONST_IF(x)	(x) ? RESERVED_const : BLANKT
#define STATIC_IF(x)	(x) ? RESERVED_static : BLANKT

// #*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#
// structure structs
// #*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#

#ifdef DEBUG
static void show_ancest (recID r)
{
	int i;

	ancestor *a = structs [r].ancestors;
	PRINTF ("Ancestors of "COLS"%s"COLE"\n", SNM (r));

	for (i = 0; a [i].rec != -1; i++) {

		PRINTF ("   ["COLS"%s"COLE"]", SNM (a [i].rec));

		if (a [i].status == ASTATUS_VIRT)
			PRINTF ("\t[virtual"COLS" %s"COLE"]\n\t"COLS, SNM (a [i].vbase));
		else if (a [i].status == ASTATUS_VIRT2)
			PRINTF ("\t[virtual"COLS" %s"COLE"] "COLS"-no data-"COLE"\n\t"COLS, SNM (a [i].vbase));
		else if (a [i].status == ASTATUS_FSCKD)
			PRINTF ("    -"COLS"* fsckd *"COLE" -\n\t"COLS);
		else	PRINTF ("\n\t"COLS);

		INTPRINT (a [i].path);

		if (a [i].cpath) {

			PRINTF (COLE"\n Dirct\t"COLS);
			INTPRINT (a [i].cpath);

		}

		PRINTF (COLE"\n");
	}
}
#endif

static inline bool vparent (int i)
{
	return in2 (i, ASTATUS_VIRT, ASTATUS_VIRT2);
}

inline static bool direct_ancest (ancestor *a)
{
	return a->direct;
}

#include "vtbl.ch"

static ctorable_t *add_ctorable (recID r)
{
	if (!structs [r].ctorables) {
		structs [r].ctorables = (ctorable_t*) malloc (2 * sizeof (ctorable_t));
		structs [r].ctorables [1].ot = -1;
		return &structs [r].ctorables [0];
	}

	int i;

	for (i = 0; structs [r].ctorables [i].ot != -1; i++);

	structs [r].ctorables = (ctorable_t*) realloc (structs [r].ctorables, (i + 2) * sizeof (ctorable_t));
	structs [r].ctorables [i + 1].ot = -1;

	return &structs [r].ctorables [i];
}

static autofunc *add_autofunc (recID r)
{
	autofunc *a = (autofunc*) malloc (sizeof * a);
	a->next = structs [r].autofuncs;
	return structs [r].autofuncs = a;
}

Token add_anonymous_union (recID r, recID u)
{
	anonunion *a = (anonunion*) malloc (sizeof * a);
	a->next = structs [r].anonunions;
	a->r = u;
	structs [r].anonunions = a;
	return a->n = name_anon_union (structs [r].nanons++, name_of_struct (r));
}

// #*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#
// class constants
// #*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#

Token lookup_class_const (recID r, Token m)
{
	tconst *t;
	for (t = structs [r].consts; t; t = t->next)
		if (t->m == m) return t->r;
	return -1;
}

void enter_class_const (recID r, Token m, Token re)
{
	tconst *t;

	if (!structs [r].consts) {
		structs [r].consts = (tconst*) malloc (sizeof (tconst));
		structs [r].consts->m = m;
		structs [r].consts->r = re;
		structs [r].consts->next = 0;
		return;
	}

	for (t = structs [r].consts; t; t = t->next)
		if (t->m == m) {
			t->r = re;
			return;
		}

	t = (tconst*) malloc (sizeof (tconst));
	t->next = structs [r].consts;
	t->m = m;
	t->r = re;
	structs [r].consts = t;
}

static void inherit_class_consts (recID r)
{
	tconst *t;
	ancestor *a;

	for (a = structs [r].ancestors; a->rec != -1; a++)
		if (direct_ancest (a))
			for (t = structs [a->rec].consts; t; t = t->next)
				enter_class_const (r, t->m, t->r);
}

// #*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#
// local typedefs
// #*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#

Token add_local_typedef (recID r, Token tn)
{
	localtdef *t;

	for (t = structs [r].ltdef; t; t = t->next)
		if (t->tn == tn)
			parse_error_toktok (tn, name_of_struct (r), "local typedef redef");

	t = (localtdef*) malloc (sizeof * t);
	t->next = structs [r].ltdef;
	structs [r].ltdef = t;
	t->tn = tn;
	return t->gn = name_local_typedef (name_of_struct (r), tn);
}

bool have_local_typedef (recID r, Token tn)
{
	localtdef *t;
	for (t = structs [r].ltdef; t; t = t->next)
		if (t->tn == tn)
			return true;
	return false;
}

static Token typedef_in (recID r, Token tn)
{
	localtdef *t;

	for (t = structs [r].ltdef; t; t = t->next)
		if (t->tn == tn) return t->gn;
	return 0;
}

Token lookup_local_typedef (recID r, Token tn)
{
	ancestor *a;
	Token ret, t;
	int d = 0, d2;
	bool e = false;

	ret = typedef_in (r, tn);
	if ((a = structs [r].ancestors))
		for (; a->rec != -1; a++)
			if ((t = typedef_in (a->rec, tn))) {
				if (ret) {
					d2 = a->depth;
					if (d2 < d) {
						ret = t;
						d = d2;
						e = false;
					} else if (d2 == d)
						e = true;
				} else {
					d = a->depth;
					ret = t;
				}
                        }
	if (e) expr_errort ("Ambiguous local typedef", tn);

	if (ret && objective.recording)
		usage_typeID (lookup_typedef (ret));

	return ret;
}

static Token direct_vparent (recID rd, recID rb)
{
	ancestor *a;

	if (rd == rb) return 0;

	if ((a = structs [rd].ancestors))
		for (; a->rec != -1; a++)
			if (a->rec == rb && a->status == ASTATUS_VIRT
			&& a->path [1] == -1) return a->path [0];
	return 0;
}

/* generate the code which sets the address of the common shared
   base class instance to the pointers that are supposed to point to it */
static void gen_construction_code_vb (OUTSTREAM o, recID r, Token obj)
{
	int i, j;
	Token *p, m;
	ancestor *a = structs [r].ancestors;

	r = aliasclass (r);
	for (i = 0; a [i].rec != -1; i++)
		if (a [i].status == ASTATUS_VIRT && a [i].vbase == a [i].rec) {
			recID vr = a [i].vbase, r2;
			for (j = 0; a [j].rec != -1; j++)
				if (!is_aliasclass (a [j].rec))
				if ((m = direct_vparent (r2 = aliasclass (a [j].rec), vr))) {
					is_ancestor (r, r2, &p, true);
					outprintf (o, obj, POINTSAT, ISTR (p),
						   '.', m, '=', -1);
				}
			if (direct_vparent (r, vr)) {
				outprintf (o, obj, POINTSAT, a [i].path [0], '=', -1);
			}
			is_ancestor (r, vr, &p, true);
			outprintf (o, '&', obj, POINTSAT, ISTR (p), ';', -1);
		}
}

/* construction code in case base obj is just a vtable */
static void gen_construction_code_vb2 (OUTSTREAM o, recID r, Token obj)
{
	int i, j;
	Token *p, *p1, *p2;
	ancestor *a = structs [r].ancestors;

	r = aliasclass (r);
	for (i = 0; a [i].rec != -1; i++)
		if (a [i].status == ASTATUS_VIRT2) {
			bool hp = 0;

			p1 = a [i].cpath;
			for (j = 0; a [j].rec != -1; j++)
				if (a [j].rec != a [i].rec && direct_ancest (&a [j])
				&& is_ancestor (aliasclass (a [j].rec), a [i].rec, &p, 1)) {
					p2 = a [j].cpath ?: a [j].path;
					if (p1 [0] == p2 [0]) continue;
					outprintf (o, obj, POINTSAT, ISTR (p2), '.',
						   ISTR (p), '.', RESERVED__v_p_t_r_, '=', -1);
					hp = 1;
				}

			if (hp) outprintf (o, obj, POINTSAT, ISTR (a [i].cpath), '.',
					    RESERVED__v_p_t_r_, ';', -1);
		}
}

/* generate the code that calls the init function for a ctorable */
static void call_init_func (OUTSTREAM o, Token obj, ctorable_t *c, Token *path)
{
	if (c->array) outprintf (o, i_call_initialization (c->ot), '(',
			   obj, POINTSAT, ISTR (path), DOT_IF (path [0] != -1), c->obn, ',',
			   RESERVED_sizeof, obj, POINTSAT, ISTR (path),
			   DOT_IF (path [0] != -1), c->obn, '/',
			   RESERVED_sizeof, '(', iRESERVED_struct (c->ot),
			   name_of_struct (c->ot), ')', ')', ';', -1);
	else outprintf (o, i_call_initialization (c->ot), '(',
			'&', obj, POINTSAT, ISTR (path), DOT_IF (path [0] != -1),
		        c->obn, ',', RESERVED_1, ')', ';', -1);
}

/* Construct all members that need construction indeed yes */
static void gen_member_construction (OUTSTREAM o, recID r, Token obj)
{
	int i;
	ctorable_t *c = structs [r].ctorables;
	Token nopath [] = { -1 };
	ancestor *a;

	if (c) for (i = 0; c [i].ot != -1; i++)
		call_init_func (o, obj, &c [i], nopath);

	if (structs [r].ancestors_have_ctors)
		for (a = structs [r].ancestors; a->rec != -1; a++)
			if ((c = structs [a->rec].ctorables))
				for (i = 0; c [i].ot != -1; i++)
					call_init_func (o, obj, &c [i], a->cpath ?: a->path);
}

void gen_construction_code (OUTSTREAM o, recID r, Token obj)
{
	if (structs [r].has_vbase)
		gen_construction_code_vb (o, r, obj);
	if (structs [r].vtis)
		gen_vt_init (o, r, obj, true);
	if (structs [r].has_vbase2)
		gen_construction_code_vb2 (o, r, obj);
	if (structs [r].ctorables || structs [r].ancestors_have_ctors)
		gen_member_construction (o, r, obj);
}
// #*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#
// what is this?
// #*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#

static intnode *structtree;
static int nstructs, nallocstructs;

recID enter_struct (Token e, Token tag, bool unwind, bool noctor, bool evt, bool sclass, bool byvalue)
{
	intnode *in = intfind (structtree, e);
	int pt [3] = { nstructs, '*', -1 };

	if (in) {
		recID r = in->v.i;
		if (unwind && !always_unwind (r))
			structs [r].unwind = true;
		if (noctor)
			structs [r].noctor = true;
		if (byvalue)
			structs [r].byvalue = true;
		return r;
	}

	if (nstructs == nallocstructs)
		structs = (structure*) realloc (structs, (nallocstructs+=96) * sizeof (structure));
	structs [nstructs].name = e;
	structs [nstructs].notunion = tag != RESERVED_union;
	structs [nstructs].deftype = false;
	structs [nstructs].class = tag == RESERVED_class;
	structs [nstructs].incomplete = true;
	structs [nstructs].has_dtor = false; 
	structs [nstructs].idtor = 0;
	structs [nstructs].vdtor = 0;
	structs [nstructs].dtor_name = 0;
	structs [nstructs].dtor_nothrow = false;
	structs [nstructs].Funcs = 0;
	structs [nstructs].has_dtorable = false;
	structs [nstructs].firstmember = 0;
	structs [nstructs].printed = false;
	structs [nstructs].unwind = unwind;
	structs [nstructs].byvalue = byvalue;
	structs [nstructs].noctor = noctor;
	structs [nstructs].evt = evt;
	structs [nstructs].has_vbase = 0;
	structs [nstructs].has_vbase2 = 0;
	structs [nstructs].rtti = 0;
	structs [nstructs].vtis = 0;
	structs [nstructs].ancestors = 0;
	structs [nstructs].ctorables = 0;
	structs [nstructs].autofuncs = 0;
	structs [nstructs].anonunions = 0;
	structs [nstructs].ltdef = 0;
	structs [nstructs].nanons = 0;
	structs [nstructs].consts = 0;
	structs [nstructs].caninst = CANT_INCOMPL;
	structs [nstructs].type_pthis = enter_type (pt);
	structs [nstructs].used = false;
	structs [nstructs].code = 0;
	structs [nstructs].depends = 0;
	structs [nstructs].ancestors_have_ctors = 0;
	structs [nstructs].need_recalc_offsets = 0;
	structs [nstructs].have_vi_decls = 0;
	structs [nstructs].vt_inthis = false;
	structs [nstructs].alias_class = nstructs;
	structs [nstructs].constd = 0;
	structs [nstructs].pm = 0;
	structs [nstructs].keyfunc = sclass ? -1 : 0;
	structs [nstructs].havekey = 0;
	structs [nstructs].autodtor = 0;
	union ival i = { i: nstructs };
	intadd (&structtree, e, i);
	return nstructs++;
}

void keyfunc_candidate (recID o, Token k)
{
	if (!structs [o].keyfunc)
//{PRINTF ("Setting keyfunc of "COLS"%s"COLE":%s\n", SNM(o), expand (k));
		structs [o].keyfunc = k;
//}
}

void possible_keyfunc (recID o, Token k)
{
//PRINTF ("possible %s:%s ", SNM(o), expand(k));
	if (structs [o].keyfunc == k)
//{PRINTF ("-ok-\n");
		structs [o].havekey = 1;
//}else PRINTF ("\n");
}

void set_depend (recID o, recID i)
{
	int j = 2;
	if (structs [o].depends) {
		for (j = 0; structs [o].depends [j] != -1; j++);
		j += 2;
	}

	structs [o].depends = (recID*) realloc (structs [o].depends, j * sizeof (recID));
	structs [o].depends [j - 2] = i;
	structs [o].depends [j - 1] = -1;
}

static void export_struct (recID r)
{
	int i;

	if (structs [r].printed) return;

	if (structs [r].depends)
		for (i = 0; structs [r].depends [i] != -1; i++)
			export_struct (structs [r].depends [i]);

	if (structs [r].vtis)
		export_virtual_table_declaration (r);

	if (structs [r].Funcs)
		export_fspace_lwc (structs [r].Funcs);

	structs [r].printed = true;
	if (structs [r].code)
		outprintf (STRUCTS, ISTR (structs [r].code), -1);
}

void export_structs ()
{
	int r;
	for (r = 0; r < nstructs; r++)
		export_struct (r);
}

static void commit_auto_functions (recID);

/////////////////////////////////////////////////
// some things when declaration completes
/////////////////////////////////////////////////

void remove_struct_from_this (Token *p, recID r)
{
	int i, j;
	if (intchr (p, RESERVED_this)) {
		for (i = 0; p [i] != RESERVED_this; i++);
		while (p [i] != RESERVED_struct && p [i] != '(') i--;
		if (p [i] != '(')
			intcpy (p + i, p + i + 1);
	}
	Token sn = name_of_struct (r);
	for (i = 0; p [i] != -1; p++)
		if (p [i] == RESERVED_struct && p [i + 1] == sn)
			for (j = i; p [j] != -1; j++)
				p [j] = p [j + 1];
}

static void adjust_protos (fspace S, recID r)
{
	if (!S) return;

	funcp *p = (funcp*) S->v.p;
	for (; p; p = p->next)
		if (p->prototype && !(p->flagz & FUNCP_MODULAR)) {
			remove_struct_from_this (p->prototype, r);
			remove_struct_from_def (p->name);
		}
	adjust_protos (S->less, r);
	adjust_protos (S->more, r);
}

static void make_empty_dtor (recID r)
{
	Token proto [] = { RESERVED_static, RESERVED_inline, RESERVED_dtor, '(', ')', ';', -1 };
	SAVE_VAR (CODE, proto);
	bool haskey = structs [r].keyfunc != 0;
	struct_declaration (r, 0, 0);
	if (!haskey && structs [r].keyfunc)
		structs [r].keyfunc = 0;
	RESTOR_VAR (CODE);
	store_define_dtor (structs [r].dtor_name, r);
}

static void study_destruction (recID r)
{
	bool b = false, v = false;
	int i;
	member *m;
	ancestor *a = structs [r].ancestors;

	for (m = structs [r].firstmember; m; m = m->next)
		if (isstructure (m->t) && has_dtor (base_of (m->t))) {
			b = true;
			break;
		}

	if (a) for (i = 0; a [i].rec != -1; i++) {
		if (vparent (a [i].status)) v = true;
		else if (direct_ancest (&a [i]) && has_dtor (a [i].rec)) 
			b = true;
        }
	// - vdtor is the destructor that doesn't call the dtors
	//   of the virtual base classes
	// - idtor calls dtors of dtorable members and vdtors (or normal
	//   dtors in not exist) of direct non virtual parents.
	// - the real dtor calls vdtor and then calls the dtors
	//   of virtual parents

	if (b) {
		if (!structs [r].has_dtor)
			make_empty_dtor (r);
		structs [r].idtor = name_intern_dtor (r);
	}
	if (v) structs [r].vdtor = name_intern_vdtor (r);
}

OUTSTREAM dispatch_vdtor (recID r, OUTSTREAM o)
{
	int i;
	Token *vd = combine_output (o);
	OUTSTREAM n = new_stream ();
	ancestor *a = structs [r].ancestors;

	for (i = 0; vd [i] != '{'; i++)
		output_itoken (n, vd [i]);
	outprintf (n, '{', structs [r].vdtor, '(', RESERVED_this, ')', ';', -1);
	for (i = 0; a [i].rec != -1; i++)
		if (vparent (a [i].status))
			outprintf (n, structs [a [i].rec].vdtor ?:
				   structs [a [i].rec].dtor_name, '(',
				   ADDR_IF (a [i].cpath && a [i].cpath [0] != -1), RESERVED_this,
				   POINTSAT_IF ((a [i].cpath ?: a [i].path) [0] != -1),
				   ISTR (a [i].cpath ?: a [i].path), ')', ';', -1);
	outprintf (n, RESERVED_return, RESERVED_this, ';', '}', -1);

	intsubst1 (vd, structs [r].dtor_name, structs [r].vdtor);
	outprintf (INTERNAL_CODE, ISTR (vd), -1);
	free (vd);

	return n;
}

void make_intern_dtor (recID r)
{
	int i;
	member *m;
	ancestor *a = structs [r].ancestors;
	Token do_alias = HaveAliases ? 0 : -1;
#ifdef	BROKEN_ALIASES
	do_alias = -1;
#endif
	recID afcls = 0;
	OUTSTREAM O = new_stream ();

	outprintf (O, R(static), R(inline), R(void), structs [r].idtor, '(',
		   iRESERVED_struct (r), name_of_struct (r), '*', R(this), ')', '{', -1);

	/* call dtors of dtorable data members */
	for (m = structs [r].firstmember; m; m = m->next)
		if (isstructure (m->t) && has_dtor (base_of (m->t))) {
			outprintf (O, dtor_name (base_of (m->t)),
				   '(', '&', RESERVED_this, POINTSAT, m->m, ')', ';', -1);
			do_alias = -1;
		}

	/* call vdtors of parents (non virtual parents that is) */
	if (a) for (i = 0; a [i].rec != -1; i++)
		if (direct_ancest (&a [i]) && has_dtor (a [i].rec) && !vparent (a [i].status)
		&& !(structs [a [i].rec].autodtor && !idtor (a [i].rec))) {
			bool dointernal = structs [a [i].rec].autodtor;
			outprintf (O,
				/* If this is an auto function call only idtor of parent.
				   else if parent has vbases, call vdtor of parent.
				   else call dtor of parent  */
				   dointernal ? idtor (a [i].rec) :
				   structs [a [i].rec].vdtor ?: 
				   dtor_name (a [i].rec), '(',
				   ISTR (upcast1_this (r, a [i].rec)), ')', ';', -1);
			if (do_alias) do_alias = -1;
			else do_alias = zero_offset (r, a [i].rec) && !dointernal ?
					 dtor_name (afcls = a [i].rec) : -1;
		}
	output_itoken (O, '}');

	/* do it with alias if all that has to be done is call the dtor of one parent
	   which is at offset 0  */
	if (do_alias > 0) {
		free_stream (O);
		outprintf (O = new_stream (), R(static), R(inline), R(void), structs [r].idtor, '(',
			   iRESERVED_struct (r), name_of_struct (r), '*', R(this), ')', 
			   alias_func (afcls, do_alias), ';', -1);
	}
	concate_streams (INTERNAL_CODE, O);
}

static void enter_virtuals (recID r, fspace S)
{
	funcp *p;

	for (p = S->v.p; p; p = p->next)
		if ((p->flagz & FUNCP_VIRTUAL) && p->prototype) {
			vf_args args;
			Token *pr;

			args.r = r;
			args.fname = S->key;
			args.ftype = p->type;
			args.fmemb = p->flagz & FUNCP_PURE ? 0 : p->name;
			args.flagz = p->flagz;
			// pass prototype to make fptr from
			for (pr = p->prototype; ISDCLFLAG (*pr); pr++);
			args.prototype = allocaint (intlen (pr) + 1);
			intcpy (args.prototype, pr);
			intsubst1 (args.prototype, p->name, MARKER);
			args.argv = allocaint (intlen (p->xargs) + 1);
			intcpy (args.argv, p->xargs);
			Enter_virtual_function (&args);
		}
	if (S->less) enter_virtuals (r, S->less);
	if (S->more) enter_virtuals (r, S->more);
}

bool do_alias_class (recID r)
{
	recID alias = 0, rr;
	ancestor *a = structs [r].ancestors;

	if (structs [r].firstmember || structs [r].vt_inthis || !a || structs [r].anonunions)
		return false;

	for (; a->rec != -1; a++) 
		if (direct_ancest (a)) {
			rr = structs [a->rec].alias_class;
			if (alias && rr != alias)
				return false;
			alias = rr;
		}
	structs [r].alias_class = alias;
	return true;
}

recID is_aliasclass (recID r)
{
	if (structs [r].incomplete && structs [r].class) {
		structs [r].deftype = true;
		return -1;
	}
	return structs [r].alias_class == r ? 0 : structs [r].alias_class;
}

recID aliasclass (recID r)
{
	return structs [r].alias_class;
}

static void undo_paths (recID);

recID complete_structure (OUTSTREAM o, recID r)
{
	structs [r].incomplete = false;
	study_destruction (r);
	if (do_alias_class (r)) {
		undo_paths (r);
		adjust_protos (structs [r].Funcs, r);
		structs [r].has_vbase = 0;
		structs [r].has_vbase2 = 0;
	} else {
		if (structs [r].class) {
			if (structs [r].deftype) outprintf (GLOBAL, RESERVED_typedef,
				 RESERVED_struct, name_of_struct (r), name_of_struct (r), ';', -1);
			//else outprintf (GLOBAL, RESERVED_struct, name_of_struct (r), ';', -1);
		}
		if (structs [r].ancestors) make_rtti (r);
	}
	if (structs [r].Funcs)
		enter_virtuals (r, structs [r].Funcs);
	//**+*+*+*+*+*+*+*+*+*+*+*+*
	close_inits (&structs [r]);
	mk_typeid (r);
	if (can_instantiate (r)) {
		if (structs [r].has_vbase)
			decl_vballoc_rec (o, r);
	}
	commit_auto_functions (r);

	if (structs [r].need_recalc_offsets)
		fix_all_offsets (r);
#ifdef	DEBUG
	if (debugflag.VIRTUALTABLES)
		show_vt (r);
#endif
	return is_aliasclass (r);
}

#if 0
void produce_dtorables (OUTSTREAM o, recID r)
{
	member *m;
	for (m = structs [r].firstmember; m; m = m->next)
		if (isstructure (m->t) && has_dtor (base_of (m->t)))
		outprintf (o, structs [base_of (m->t)].dtor_name,
			   '(', '&', RESERVED_this, POINTSAT, m->m, ')', ';', -1);
}
#endif

// #*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#
// parental control
// #*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#

static Token *best_path (Token *p1, Token *p2)
{
	int i, c1, c2;

	if (!p1) return p2;
	if (!p2) return p1;

	for (i = c1 = 0; p1 [i] != -1; i++)
		if (p1 [i] == POINTSAT) c1++;
	for (i = c2 = 0; p2 [i] != -1; i++)
		if (p2 [i] == POINTSAT) c2++;

	if (c1 != c2)
		return c1 < c2 ? p1 : p2;

	for (i = c1 = 0; p1 [i] != -1; i++)
		if (p1 [i] == '.') c1++;
	for (i = c2 = 0; p2 [i] != -1; i++)
		if (p2 [i] == '.') c2++;

	if (c1 != c2)
		return c1 < c2 ? p1 : p2;

	i = 0;
	do if (p1 [i] != p2 [i])
		return strcmp (expand (p1 [i]), expand (p2 [i])) < 0 ? p1 : p2;
	while (p1 [i++] != -1);

	return p1;
}

#if 0
/* sort the ancestors from closest to oldest */
/* do we really need this? */
static void sort_ancestors (recID r)
{
	int i, j;
	ancestor *a, swp;

	a = structs [r].ancestors;
	for (i = 0; a [i].rec != -1; i++)
		for (j = i + 1; a [j].rec != -1; j++)
			if (intlen (a [i].path) > intlen (a [j].path)) {
				swp = a [i];
				a [i] = a [j];
				a [j] = swp;
			}
}
#endif

static Token *ext_path (Token nn, Token *p, bool v)
{
	return p ? p [0] == -1 ? sintprintf (mallocint (2), nn, -1) :
		sintprintf (mallocint (intlen (p)+3), nn, v ? POINTSAT:'.', ISTR (p), -1) : 0;
}

static bool make_ancestors (recID r, recID ps[], recID cvb[])
{
	int i, j, t, k, l, c = 0;
	structure *s;
	ancestor *a, *a2;
	bool hv = false, *vr = (bool*) alloca (sizeof (bool) * intlen (ps));

	for (i = 0; ps [i] != -1; i++) {
		if ((vr [i] = ps [i] >= VIRTUALPAR_BOOST)) {
			ps [i] -= VIRTUALPAR_BOOST;
			hv = true;
		}
		if (!structs [ps [i]].notunion)
			parse_error_ll ("cannot inherit a union");
		if (structs [ps [i]].incomplete)
			parse_error (0, "You cannot inherit from something unborn");
		if (structs [ps [i]].constd)
			structs [r].constd = true;
		set_depend (r, ps [i]);
	}

	for (i = 0, t = 1; ps [i] != -1; i++, t++)
		if ((s = &structs [ps [i]])->ancestors)
			for (j = 0; s->ancestors [j].rec != -1; j++)
				 t++;

	a = structs [r].ancestors = (ancestor*) malloc (sizeof (ancestor) * t);

		/* Somethings like mutliple non-virtual inheritace
		 * of common base class, really ought to be forbidden.
		 * we can parse_error on ASTATUS_FSCKD for that...
		 */
	for (i = t = 0; ps [i] != -1; i++) {
		recID rr = ps [i];
		Token nn = name_inherited (name_of_struct (rr));
		bool V = vr [i], V2;
		bool zero_offset = i == 0 && !(V && vt_only (rr));

		s = &structs [rr];

		for (k = 0; k < t; k++)
			if (a [k].rec == rr)
				break;

		// new ancestor or existing ?
		if (k < t) {
			// existing!
			a [k].direct = true;
			a [k].depth = 1;
			V2 = vparent (a [k].status);
			// both virtual
			if (V && V2) {
				a [k].zero_displace = false;
				if (a [k].vbase != aliasclass (rr))
					goto really_fsckd1;
				free (a [k].path);
				a [k].path = sintprintf (mallocint (2), nn, -1);
				if (a [k].status == ASTATUS_VIRT2) {
					free (a [k].cpath);
					sintprintf (a [k].cpath = mallocint (2), nn, -1);
				}
				for (l = 0; l < c; l++)
					if (cvb [l] == rr) break;
				if (l == c) cvb [c++] = rr;
			} else really_fsckd1: {
				a [k].status = ASTATUS_FSCKD;
				free (a [k].path);
				a [k].path = sintprintf (mallocint (2), nn, -1);
			}
		} else {
			a [t].rec = rr;
			a [t].status = V ? vt_only (rr) ? ASTATUS_VIRT : ASTATUS_VIRT2 : ASTATUS_NORM;
			a [t].zero_displace = zero_offset;
			a [t].direct = true;
			a [t].path = sintprintf (mallocint (2), nn, -1);
			a [t].depth = 1;
			if (V) a [t].vbase = aliasclass (rr);
			a [t].cpath = a [t].status != ASTATUS_VIRT2 ? 0 : sintprintf (mallocint (2), nn, -1);
			++t;
			if (s->ctorables || s->ancestors_have_ctors)
				structs [r].ancestors_have_ctors = true;
		}

		// add ancestors of ancestor
		if ((a2 = s->ancestors))
		for (j = 0; a2 [j].rec != -1; j++) {


			for (k = 0; k < t; k++)
				if (a [k].rec == a2 [j].rec)
					break;

			if (k < t) {
				if (vparent (a [k].status)) {
					a [k].zero_displace = false;
					if (a2 [j].status == a [k].status) {
						if (a [k].vbase != a2 [j].vbase)
							goto really_fsckd2;
					} else if (V) {
						if (a [k].vbase != aliasclass (rr))
							goto really_fsckd2;
					} else goto really_fsckd2;

					for (l = 0; l < c; l++)
						if (cvb [l] == a [k].vbase) break;
					if (l == c) cvb [c++] = a [k].vbase;

					Token *path = ext_path (nn, a2 [j].path, V);
					Token *cpath = ext_path (nn, a2 [j].cpath, V);

					if (best_path (a [k].path, path) == path) {
						free (a [k].path);
						a [k].path = path;
					} else free (path);

					if (cpath)
						if (best_path (a [k].cpath, cpath) == cpath) {
							if (a [k].cpath) free (a [k].cpath);
							a [k].cpath = cpath;
						} else free (cpath); else;
				} else really_fsckd2:
					a [k].status = ASTATUS_FSCKD;
			} else {
				a [t].rec = a2 [j].rec;
				a [t].status = a2 [j].status;
				a [t].path = ext_path (nn, a2 [j].path, V);
				a [t].vbase = a2 [j].vbase;
				a [t].zero_displace = a2 [j].zero_displace && zero_offset;
				a [t].depth = a2 [j].depth + 1;
				a [t].direct = false;
				if (V && !vparent (a2 [j].status)) {
					a [t].status = ASTATUS_VIRT;
					a [t].vbase = aliasclass (rr);
				}
				a [t++].cpath = ext_path (nn, a2 [j].cpath, V);
			}
		}
	}
	a [t].rec = -1;
	cvb [c] = -1;
//	sort_ancestors (r);

	for (i = 0; i < t; i++)
		if (a [i].status == ASTATUS_VIRT)
			structs [r].has_vbase = true;
		else if (a [i].status == ASTATUS_VIRT2)
			structs [r].has_vbase2 = true;
#ifdef DEBUG
	if (debugflag.VIRTUALTABLES) show_ancest (r);
#endif
	return hv;
}

/* If something is an alias class (typedef), undo the path extensions */
static void undo_path (Token *path)
{
	if (path [1] == -1) path [0] = -1;
	else intcpy (path, path + 2);	// overlap but works
}

static void undo_paths (recID r)
{
	ancestor *a = structs [r].ancestors;
	if (!a) return;

	for (; a->rec != -1; a++) {
		if (a->path) undo_path (a->path);
		if (a->cpath) undo_path (a->cpath);
	}
}

bool zero_offset (recID rder, recID rbase)
{
	ancestor *a;

	for (a = structs [rder].ancestors; a; a++)
		if (a->rec == rbase)
			return a->zero_displace;
	return false;
}

static void inherit_auto_functions (recID);

void set_parents (recID r, recID ps[])
{
	//bool have_virtual_inh;
	recID collapsed [16];

	if (ps [0] != -1 && !structs [r].notunion)
		parse_error_ll ("union cannot be part of a hierarchy");

	//have_virtual_inh =
        make_ancestors (r, ps, collapsed);
	inherit_all_virtuales (r);
	inherit_auto_functions (r);
	inherit_class_consts (r);
//	if (have_virtual_inh)		// this is leftover from surgery while implementing
//		make_rtti (r);		// aliasclasses. hmmmmmmmmmmmmmmm. who knows?
	if (collapsed [0] != -1)
		update_dcasts (r, collapsed);
}

void output_parents (OUTSTREAM o, recID r)
{
	int i, ri = 0, j;
	ancestor *a;
	recID rp [64], rec;
	bool virt;

	if (!(a = structs [r].ancestors))
		return;

	for (i = 0; a [i].rec != -1; i++)
		if (direct_ancest (&a [i])) {
			if (is_aliasclass (a [i].rec)) {
				rec = is_aliasclass (a [i].rec);
				for (j = 0; a [j].rec != rec; ++j);;;
				virt = a [j].status == ASTATUS_VIRT;
				if (virt)
				for (j = 0; a [j].rec != -1; j++) {
				if (a [j].path [0] == a [i].path [0] && a [j].path [1] == '.')
					a [j].path [1] = POINTSAT;
				if (a [j].cpath)
				if (a [j].cpath [0] == a [i].path [0] && a [j].cpath [1] == '.')
					a [j].path [1] = POINTSAT;
				}
			} else {
				rec = a [i].rec;
				virt = a [i].status == ASTATUS_VIRT;
			}
			for (j = 0; j < ri; j++)
				if (rp [j] == rec) break;
			if (j < ri) continue;
			rp [ri++] = rec;
			outprintf (o, RESERVED_struct, name_of_struct (rec),
			   STAR_IF (virt), a [i].path [0], ';', -1);
		}
}
// #*#*#*#*#*#*#*#*#*#*#*#*#*#*#
// general queries
// #*#*#*#*#*#*#*#*#*#*#*#*#*#*#

int lookup_struct (Token e)
{
	intnode *n = intfind (structtree, e);
	return n ? n->v.i : 0;
}

int is_struct (Token e)
{
	intnode *n = intfind (structtree, e);
	return n == NULL ? 0 : structs [n->v.i].notunion;
}

Token name_of_struct (recID r)
{
	return structs [r].name;
}

typeID pthis_of_struct (recID r)
{
	return structs [r].type_pthis;
}

fspace FSP (recID r)
{
	return r ? structs [r].Funcs : Global;
}

bool has_const_members (recID r)
{
	return structs [r].constd || structs [r].vt_inthis;
}
// #*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#
// members
// #*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#

static Token ancest_ptr (ancestor *a)
{
	return a->status == ASTATUS_VIRT && a->vbase == a->rec ? POINTSAT : '.';
}

static typeID has_member (recID r, Token m, Token path[], Token *glob_name)
{
	member *M;

	for (M = structs [r].firstmember; M; M = M->next)
		if (M->m == m) {
			path [0] = m;
			path [1] = -1;
			if (glob_name) *glob_name = M->gn;
			return M->t;
		}

	anonunion *a;
	typeID t;

	for (a = structs [r].anonunions; a; a = a->next)
		if ((t = has_member (a->r, m, path + 2, glob_name)) != -1) {
			path [0] = a->n;
			path [1] = '.';
			return t;
		}

	return -1;
}

typeID lookup_variable_member (recID r, Token m, Token *path, bool const_path, Token *glob_name)
{
	int i;
	typeID t, rt = -1;
	ancestor *a;
	Token p [64], nglob = 0;
	bool vptrm = m == RESERVED__v_p_t_r_;

	r = aliasclass (r);

	if ((t = has_member (r, m, p, glob_name)) != -1) {
		rt = t;
		if (path) sintprintf (path, ISTR (p), -1);
	}

	if ((a = structs [r].ancestors))
		for (i = 0; a [i].rec != -1; i++)
			if ((t = has_member (a [i].rec, m, p, &nglob)) != -1) {
				if (rt != -1 || a [i].status == ASTATUS_FSCKD) {
					if (base_of (t) == B_PURE) continue;
					else if (vptrm)
						expr_errort ("Ambiguous vptr for class",
							name_of_struct (r));
					else expr_errort ("Ambigous member", m);
                                }
				rt = t;
				if (glob_name) *glob_name = nglob;
				nglob = 0;
				if (path) sintprintf (path,
					ISTR (const_path && a [i].cpath ? a [i].cpath : a [i].path),
					const_path && a [i].cpath ? '.' : ancest_ptr (&a [i]),
					ISTR (p), -1);
			}

	return rt;
}

static bool is_ctorable (typeID t)
{
	return (isstructure (t) && need_construction (base_of (t)))
		|| is_array_of_ctorable_objects (t);
}

void add_variable_member (recID r, Token m, typeID t, Token gn, bool constant, bool ncto)
{
	typeID lt;

	if (!structs [r].incomplete)
		parse_error_ll ("structure closed");
	if (m != RESERVED__v_p_t_r_ && (lt = lookup_variable_member (r, m, 0, 0, 0)) != -1
	 && base_of (lt) != B_PURE)
		parse_error_tok (m, "member duplicate");
	if (!gn && !ncto && is_ctorable (t)) {
		ctorable_t *cc = add_ctorable (r);
		cc->ot = base_of (t);
		cc->array = !isstructure (t);
		cc->obn = m;
	}
	if (!gn && !structs [r].constd)
		if (constant || (isstructure (t) && structs [base_of (t)].constd))
			structs [r].constd = true;
	member *M = (member*) malloc (sizeof *M);
	M->m = m;
	M->t = t;
	M->gn = gn;
	M->next = 0;
	if (structs [r].firstmember) {
		structs [r].lastmember->next = M;
		structs [r].lastmember = M;
	} else {
		structs [r].firstmember = structs [r].lastmember = M;
	}
}
// #|#|#|#|#|#|#|#|#|#|#|#|#|#|#
// some stuff
// #|#|#|#|#|#|#|#|#|#|#|#|#|#|#

static bool have_exact_function_member (recID, Token, typeID[], flookup*);

bool has_void_ctor (recID r)
{
	typeID args [] = { pthis_of_struct (r), INTERNAL_ARGEND };
	flookup F;

	bool re = have_exact_function_member (r, RESERVED_ctor, args, &F);
	if (re) SET_MAYTHROW (F);
	return re;
}

bool has_copy_ctor (recID r)
{
	typeID args [] = { pthis_of_struct (r), pthis_of_struct (r), INTERNAL_ARGEND };
	flookup F;

	bool re = have_exact_function_member (r, RESERVED_ctor, args, &F);
	if (re) SET_MAYTHROW (F);
	return re;
}

void set_dfunc (recID r, Token dn, bool nothrow, bool autodtor)
{
	structs [r].dtor_nothrow = nothrow;
	structs [r].dtor_name = dn;
	structs [r].has_dtor = true;
	structs [r].autodtor |= autodtor;
}

bool isunion (recID r)
{
	return !structs [r].notunion;
}

bool has_dtor (recID r)
{
	return structs [r].has_dtor;
}

bool always_unwind (recID r)
{
	return structs [r].unwind;
}

bool by_ref (typeID r)
{
	return StructByRef && !structs [base_of (r)].byvalue;
}

Token dtor_name (recID r)
{
	if (!structs [r].dtor_nothrow)
		may_throw = true;
	return structs [r].dtor_name;
}

void set_dtor_nothrow (recID r)
{
	structs [r].dtor_nothrow = true;
}

Token idtor (recID r)
{
	return structs [r].idtor;
}

Token vdtor (recID r)
{
	return structs [r].vdtor;
}

void set_declaration (recID r, Token *code)
{
	structs [r].code = code;
}

static bool has_named_func (recID r, Token f)
{
	if (have_function (structs [r].Funcs, f)) return true;
	ancestor *a;
	if ((a = structs [r].ancestors))
		for (; a->rec != -1; a++)
			if (have_function (structs [a->rec].Funcs, f))
				return true;
	return false;
}

bool has_ctors (recID r)
{
	return has_named_func (r, RESERVED_ctor);
}

bool has_oper_fcall (recID r)
{
	return has_named_func (r, RESERVED_oper_fcall);
}
// #+#+#+#+#+#+#+#+#+#+#+#+#
// function members
// #+#+#+#+#+#+#+#+#+#+#+#+#

// lookup a function in parent classes and inherit the useful
// flags: AUTO, VIRTUAL, MODULAR
int inherited_flagz (recID r, Token f, typeID t)
{
	ancestor *a = structs [r].ancestors;

	if (!a) return 0;

	funcp *p;
	int flagz = 0;
	typeID *pargl = promoted_arglist_t (t);
	typeID *cargv = (typeID*) allocaint (intlen (pargl) + 3);
	intcpy (cargv, pargl);
	free (pargl);

	for (; a->rec != -1; a++) {
		cargv [0] = structs [a->rec].type_pthis;
		if ((p = xlookup_function_dcl (structs [a->rec].Funcs, f, cargv)))
			flagz |= p->flagz;
	}

	return flagz & (FUNCP_AUTO | FUNCP_VIRTUAL | FUNCP_MODULAR);
}

// lookup function flagz in declarations in class
// useful flags: CONST-THIS, MODULAR, AUTO
int exported_flagz (recID r, Token f, typeID t)
{
	typeID *argv = promoted_arglist_t (t);
	funcp *p;
	int ret = FUNCP_UNDEF;

	if ((p = xlookup_function_dcl (structs [r].Funcs, f, argv)))
		ret = p->flagz & (FUNCP_CTHIS | FUNCP_MODULAR | FUNCP_AUTO);
	free (argv);
	return ret;
}

static int looking_for;

static int has_fmember (recID r, Token f, typeID argv[], flookup *F)
{
	if (!structs [r].Funcs) return 0;
	argv [0] = structs [r].type_pthis;
	return xlookup_function (structs [r].Funcs, f, argv, F) == looking_for;
}

static bool _lookup_function_member (recID r, Token f, typeID argv[], flookup *F)
{
	int i, lt;
	flookup tmp;
	recID rt = -1;
	ancestor *a;

	if ((lt = has_fmember (r, f, argv, F)))
		return true;

	if ((a = structs [r].ancestors))
		for (i = 0; a [i].rec != -1; i++)
			if ((lt = has_fmember (a [i].rec, f, argv, &tmp))) {
				if (rt == -1 || isancestor (a [i].rec, rt)) {
					if (a [i].status == ASTATUS_FSCKD)
						goto really_fsckd;
					rt = lt == 1 ? a [i].rec : -2;
					*F = tmp;
				} else if (!isancestor (rt, a [i].rec)) really_fsckd:
					expr_errort ("Ambigous member function", f);
			}

	if (rt == -1)
		return false;
	argv [0] = rt >= 0 ? structs [rt].type_pthis : -1;
	return true;
}

int lookup_function_member (recID r, Token f, typeID argv[], flookup *F, bool definite)
{
	looking_for = 3;
	if (_lookup_function_member (r, f, argv, F))
		return 3;

	// global function exact match > member function overload promotion
	if (!definite && xlookup_function (Global, f, &argv [1], F) == 3)
		return 0;

	looking_for = 2;
	if (_lookup_function_member (r, f, argv, F))
		return 2;

	looking_for = 1;
	return _lookup_function_member (r, f, argv, F);
}

static Token has_fmember_uname (recID r, Token f)
{
	if (!structs [r].Funcs) return 0;
	return xlookup_function_uname (structs [r].Funcs, f);
}

Token lookup_function_member_uname (recID *r, Token f)
{
	int i, lt = 0, rr;
	recID rt = -1;
	ancestor *a;

	if ((lt = has_fmember_uname (*r, f)))
		return lt;

	if ((a = structs [*r].ancestors))
		for (i = 0; a [i].rec != -1; i++)
			if ((rr = has_fmember_uname (a [i].rec, f))) {
				if (rt == -1 || isancestor (a [i].rec, rt)) {
					if (a [i].status == ASTATUS_FSCKD)
						goto really_fsckd;
					rt = a [i].rec;
					lt = rr;
				} else if (!isancestor (rt, a [i].rec)) really_fsckd:
					parse_error_tok (f, "Ambigous member function");
			}
	*r = rt;
	return lt;
}

static bool have_exact_function_member (recID r, Token f, typeID argv[], flookup *F)
{
	looking_for = 2;
	return _lookup_function_member (r, f, argv, F)
	 || (looking_for = 3, _lookup_function_member (r, f, argv, F));
}

Token declare_function_member (recID r, Token f, typeID t, Token *p, Token *a, int fl,
			       Token **d, Token section)
{
	if (!structs [r].notunion)
		parse_error_ll ("unions may not be have member functions");
	funcp *fp = xdeclare_function (&structs [r].Funcs, f,
			 name_member_function (r, f), t, p, a, fl, d, section);
	fp->used = !(fl & FUNCP_AUTO);
	return fp->name;
}
// #*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#
// misc
// #*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#

bool Can_instantiate (recID r)
{
	switch (structs [r].caninst) {
	case CANT_INCOMPL:
		expr_errort ("Can't instantiate incomplete class", name_of_struct (r));
	case CANT_PUREV:
		expr_errort ("Can't instantiate class. Pure virtual functions still in there",
		name_of_struct (r));
	case CANT_NUFO:
		expr_errort ("Can't instantiate class."
			" No unique final overrider for virtual function", name_of_struct (r));
	case OK_CAN:
		return true;
	}
	return true;
}

recID ancest_named (recID r, Token t)
{
	ancestor *a = structs [r].ancestors;

	while (a->path [0] != t || a->path [1] != -1)
		++a;
	return a->rec;
}

bool isancestor (recID rder, recID rbase)
{
	ancestor *a = structs [rder].ancestors;

	if (a) while (a->rec != -1)
		if (a++->rec == rbase) return true;

	return rder == rbase;
}

int is_ancestor (recID rder, recID rbase, Token **path, bool const_path)
{
	ancestor *a = structs [rder].ancestors;

	if (!a) return 0;

	for (; a->rec != -1; a++)
		if (a->rec == rbase) {
			if (path) *path = const_path && a->cpath ? a->cpath : a->path;
			if (a->status == ASTATUS_FSCKD)
				expr_errort ("Ambigouus relationship", name_of_struct (rbase));
			return a->status == ASTATUS_VIRT && a->vbase == a->rec && !const_path ? 2:1;
		}
	return 0;
}

/* HOWTO: anestor status flags.

		ASTATUS_NORM	ASTATUS_VIRT	ASTATUS_VIRT2
		---------------------------------------------
! -> for members	No		Yes		No
! collapse vtis		No		Yes		Yes
! declare vbase		No		Yes		No
! const path		No		Yes		Yes
! vtable good		Yes		No		No
! downcast rtti		No		Yes		Yes

***************************************************************/

/* this here returns true if downcast rtti is needed to go from rbase to rder */
bool is_ancestor_runtime (recID rder, recID rbase, Token **path)
{
	if (is_ancestor (rder, rbase, path, false) == 2 || intchr (*path, POINTSAT))
		return true;
	ancestor *a = structs [rder].ancestors;
	while (a->rec != rbase) a++;
	return a->status == ASTATUS_VIRT2;
}

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
//
// auto functions
//
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*

bool PARENT_AUTOFUNCS;

void add_auto_f (Token ifn, Token fn, recID r, typeID t, bool virtual, bool pure, NormPtr dclPtr,
		 Token *proto, Token *argv)
{
	autofunc *a;
	typeID *va = promoted_arglist_t (t);

	for (a = structs [r].autofuncs; a; a = a->next)
		if (a->name == fn && (arglist_compare (va, a->arglist)
			|| (PARENT_AUTOFUNCS && dclPtr == a->dclPtr)))
				break;

	if (!a) {
		a = add_autofunc (r);
		a->name = fn;
		a->arglist = va;
		a->virtual = virtual;
		a->status = pure ? AU_STATUS_PV : AU_STATUS_DD;
		a->dname = a->fname = ifn;
		a->pname = -1;
	} else {
		free (va);

		//* commented out. does not allow to redefine an auto function
		//* with pure typedefs in its arguments in a class that they
		//* have been specialized...   We only miss report of a rare error
		//if (a->status == AU_STATUS_DD)
		//	parse_error_tok (fn, "auto-function redefined");

		if (virtual && !a->virtual)
			parse_error_tok (fn, "auto-function was not declared virtual");
		a->fname = ifn;
		// smart hack
		// PEND means it's inherited and we expect redef
		// !PEND means that user has supplied new definition
		//  and we undo what we did the first time where PEND was true
		if (a->status != AU_STATUS_PEND)
			a->dname = ifn, a->pname = -1;
		a->status = pure ? AU_STATUS_PV : AU_STATUS_DD;
	}
	a->dclPtr = dclPtr;
	a->proto = intdup (proto);
	a->argv = intdup (argv);
	a->type = t;
}

int borrow_auto_decls (recID r, NormPtr ret[])
{
	int i;
	autofunc *a;

	for (i = 0, a = structs [r].autofuncs; a; a = a->next)
		if (a->status == AU_STATUS_ND) {
			a->status = AU_STATUS_PEND;
			ret [i++] = a->dclPtr;
		}
	ret [i] = -1;
	return i;
}

void repl__CLASS_ (Token **proto, recID r)
{
	if (is_aliasclass (r))
		intsubst (*proto, RESERVED__CLASS_, name_of_struct (r));
	else {
		Token tmp [200];
		int i = 0, j = 0;
		for (;;)
			if ((*proto) [i] == RESERVED__CLASS_) {
				tmp [j++] = RESERVED_struct;
				tmp [j++] = name_of_struct (r);
				++i;
			} else if ((tmp [j++] = (*proto) [i++]) == -1) break;
		free (*proto);
		*proto = intdup (tmp);
	}
}

/* commit all the auto functions to require definition/instantiation */
static void commit_auto_functions (recID r)
{
	autofunc *a;

	for (a = structs [r].autofuncs; a; a = a->next)
		if (a->status != AU_STATUS_NU) {
			if (is_aliasclass (r))
				remove_struct_from_this (a->proto, r);
			if (intchr (a->proto, RESERVED__CLASS_))
				repl__CLASS_ (&a->proto, r);
			commit_auto_define (a->dname, r, a->fname, a->pname, a->virtual,
					    a->proto, a->argv, a->type);
		} else warning_tok ("No unique final overrider for auto function of",
				 name_of_struct (r));
}

/* inherit auto functions and change DD to ND (not declared) */
static void inherit_auto_functions (recID r)
{
	ancestor *a;
	autofunc *af, *hf;
	int i;

	a = structs [r].ancestors;

	for (i = 0; a [i].rec != -1; i++)
	if (direct_ancest (&a [i])) {
		for (af = structs [a [i].rec].autofuncs; af; af = af->next) {
			for (hf = structs [r].autofuncs; hf; hf = hf->next)
				if (hf->name == af->name && arglist_compare (hf->arglist, af->arglist))
					break;
			if (hf) {
				if (af->status == AU_STATUS_PV) continue;
				if (hf->status == AU_STATUS_PV) goto wins;
				if (hf->status == AU_STATUS_ND && af->status == AU_STATUS_DD
				&& hf->dclPtr == af->dclPtr) continue;
				hf->status = AU_STATUS_NU;
			} else {
				hf = add_autofunc (r);
			wins:
				hf->name = af->name;
				hf->dname = af->dname;
				hf->pname = hf->fname = af->fname;
				hf->virtual = af->virtual;
				hf->status = af->status;
				hf->arglist = intdup (af->arglist);
				hf->dclPtr = af->dclPtr;
				hf->proto = af->proto;
				hf->argv = af->argv;
				hf->type = af->type;
				if (hf->status == AU_STATUS_DD)
					hf->status = AU_STATUS_ND;
			}
		}
		if (structs [a [i].rec].autodtor)
			structs [r].autodtor = 1;
	}
}

/* upcast from this to a direct parent using constant path if avail */
Token *upcast1_this (recID rder, recID rbase)
{
static	Token retp [15];
	ancestor *a = structs [rder].ancestors;

	if (aliasclass (rder) == aliasclass (rbase)) 
		sintprintf (retp, RESERVED_this, -1);
	else {
		while (a->rec != rbase) a++;

		if (a->status == ASTATUS_NORM)
			sintprintf (retp, '&', RESERVED_this, POINTSAT, a->path [0], -1);
		else if (a->cpath)
			sintprintf (retp, '&', RESERVED_this, POINTSAT, ISTR (a->cpath), -1);
		else sintprintf (retp, RESERVED_this, POINTSAT, a->path [0], -1);
	}

	return retp;
}

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
//
// pure data members
//
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*

void add_pure_dm (recID r, Token bt, NormPtr dcl)
{
	puredm *p;

	for (p = structs [r].pm; p; p = p->next)
		if (p->bt == bt) {
			if (!intchr (p->dcls, dcl))
				intcatc (p->dcls, dcl);
			return;
		}
	p = (puredm*) malloc (sizeof *p);
	p->next = structs [r].pm;
	structs [r].pm = p;
	p->bt = bt;
	sintprintf (p->dcls, dcl, -1);
}

void gen_pure_dm (recID r, OUTSTREAM O)
{
	puredm *p;
	ancestor *a = structs [r].ancestors;
	int i, j;
	Token t;

	if (a) for (i = 0; a [i].rec != -1; i++)
	if (direct_ancest (&a [i]))
		for (p = structs [a [i].rec].pm; p; p = p->next) {
			if ((t = lookup_local_typedef (r, p->bt))
			&& base_of (lookup_typedef (t)) != B_PURE)
				for (j = 0; p->dcls [j] != -1; j++)
					struct_declaration (r, O, p->dcls [j]);
			else for (j = 0; p->dcls [j] != -1; j++)
				add_pure_dm (r, p->bt, p->dcls [j]);
		}
}

//************************************************************
// renaming functions due to overload adds complexity
//************************************************************

void rename_hier (Token on, Token nn)
{
	int i;
	autofunc *a;

	for (i = 0; i < nstructs; i++)
		if (structs [i].incomplete) {
			for (a = structs [i].autofuncs; a; a = a->next) {
				// one of the two is redundant
				if (a->dname == on) a->dname = nn;
				if (a->fname == on) {
					a->fname = nn;
					if (intchr (a->proto, on))
						intsubst1 (a->proto, on, nn);
				}
			}
			if (structs [i].keyfunc == on)
				structs [i].keyfunc = nn;
		}
}
