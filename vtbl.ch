//*******************************************
//	THIS FILE IS INCLUDED BY HIER.C
//*******************************************

/******************************************************************************

	virtual table stuff.
	virtual inheritance is what complicates things a lot

******************************************************************************/

// #*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#
// virtual table structs
// #*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#

typedef struct vt_entry_t {
	struct vt_entry_t *next;
	Token *prototype, *xproto, *xargs;
	Token entry_name;
	typeID type;
	bool isfunction, isconst;
	Token fname;
	typeID *arglist;
	recID introduced_at;
	Token *zpath;
	int flagz;
} vt_entry;

typedef struct {
	initID initTbl;
	recID object;
} vt_init_entry;

typedef struct virtual_table {
	recID appeared_at;
	vt_entry *first_entry;
	Token vtname;
	bool inlined, constd, volatiled;
	intnode *vu_tree;
	vt_init_entry *vti;
	int nvti, nvtalloc;
	bool typeid;
} virtual_table;

// vti_entry.status
enum { VTI_STATUS_NI, VTI_STATUS_ND, VTI_STATUS_DD, VTI_STATUS_NU, VTI_STATUS_FINAL };

typedef struct vti_entry_t {
	struct vti_entry_t *next;
	vt_entry *vt_ent;
	/* code, value: explained
	  - a virtual function has value and optionaly code.
	     the code is the virtuallity function definition.
	  - a virtual variable has no value and only code --> the init expression
	  - likewise for downcast data --> only code and no value   */
	Token value;
	Token *code;
	int status;
	bool cast_avoid_downcast;
} vti_entry;

typedef struct vt_initializer_t {
	Token instance_name;
	vtblID v_t_i;
	bool open, unused;
	bool exportdef;
	recID rec;

	vti_entry *first_entry;
} vt_initializer;

static virtual_table *VirtualTable;
static int nVTBL, nVTBLalloc;

static vt_initializer *VirtualInitializer;
static int nVTI, nVTIalloc;

#ifdef DEBUG
static void show_vt (recID r)
{
	if (!structs [r].vtis)
		return;
	PRINTF ("\n+++Virtual tables of ["COLS"%s"COLE"]:\n", expand (name_of_struct (r)));
	int i;
	for (i = 0; structs [r].vtis [i] != -1; i++) {
		PRINTF ("  --- Vtable %i:\n", i);
		vti_entry *v = VirtualInitializer [structs [r].vtis [i]].first_entry;
		for (;v;v = v->next) {
			PRINTF ("    + Entry ["COLS"%s"COLE"] value ["COLS"%s"COLE"] ["COLS"%i"COLE"\n",
				expand (v->vt_ent->entry_name), v->value ? expand (v->value) : "-no value-",
				v->status);
			if (!v->value) {
				PRINTF ("      + "COLS"+ + >"COLE" constant expression ["COLS);
				if (v->code) INTPRINT (v->code);
				else PRINTF ("- no code -");
				PRINTF (COLE"]\n");	
			}
		}
	}
}
#endif

static vtblID new_virtual_table ()
{
	if (nVTBL == nVTBLalloc)
		VirtualTable = (virtual_table*) realloc (VirtualTable,
			 (nVTBLalloc += 50) * sizeof (virtual_table));
	VirtualTable [nVTBL].first_entry = 0;
	VirtualTable [nVTBL].vu_tree = 0;
	VirtualTable [nVTBL].typeid = false;
	VirtualTable [nVTBL].inlined = InlineAllVt;
	VirtualTable [nVTBL].constd = ConstVtables;
	VirtualTable [nVTBL].volatiled = !vtptrConst;
	VirtualTable [nVTBL].nvti = VirtualTable [nVTBL].nvtalloc = 0;
	VirtualTable [nVTBL].vti = 0;
	return nVTBL++;
}

// #+#+#+#+#+#+#+#+#+#+#+#+#+#+#+#+#+#+#
// unionify the virtual table so order
// of declaration not important.
// foreach new entry of the v-table,
// we store the hierarchy path in a
// way that can be expanded with unions
// afterwards in the generation of the
// v-table declaration.
// #-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#

#define PARMUL 10000

static void add_parent_child (vtblID vt, recID par, recID child)
{
	int mval = par * PARMUL + child;
	if (intfind (VirtualTable [vt].vu_tree, mval))
		return;
	union ival i;
	intadd (&VirtualTable [vt].vu_tree, mval, i);
}

/* Make Unionified path of virtual table entry */
static Token *vt_path (Token entn, recID root, recID node)
{
	Token zpath [128];
	int zpi, i=i;
	ancestor *a;
	recID r=r;

	zpath [127] = -1;
	zpath [zpi = 126] = entn;

	while (node != root) {
		for (a = structs [node].ancestors; a->rec != -1; a++)
			if (a->path [1] == -1 && structs [r = a->rec].vtis)
				for (i = 0; structs [r].vtis [i] != -1; i++)
					if (VirtualTable [VirtualInitializer
						 [structs [r].vtis [i]].v_t_i].appeared_at == root)
						goto dbreak;
	dbreak:
		zpi -= 4;
		sintprintf (zpath + zpi, name_derrive_union, '.', name_derrive_memb (node), '.', NONULL);
		add_parent_child (VirtualInitializer [structs [r].vtis [i]].v_t_i, r, node);
		node = r;
	}

	return intdup (&zpath [zpi]);
}

static vt_entry *add_vt_entry (vtblID v, vt_entry *e)
{
	vt_entry *ep = VirtualTable [v].first_entry;

	/* make unionified path to vt entry */
	e->zpath = vt_path (e->entry_name, VirtualTable [v].appeared_at, e->introduced_at);

	if (!ep) VirtualTable [v].first_entry = e;
	else {
		while (ep->next) ep = ep->next;
		ep->next = e;
	}
	return e;
}

static struct {
	int n;
	recID *vchild;
} get_child_r;

static void get_children_r (intnode *n, recID par)
{
	int i = n->key;
	if (i / PARMUL == par)
		get_child_r.vchild [get_child_r.n++] = i % PARMUL;
	if (n->less) get_children_r (n->less, par);
	if (n->more) get_children_r (n->more, par);
}

static void get_children (vtblID vt, recID par)
{
	if (VirtualTable [vt].vu_tree)
		get_children_r (VirtualTable [vt].vu_tree, par);
}

static void declare_vtable_for (vtblID vt, recID r)
{
	int n = 0, i, j;
	vt_entry *ve;
	vt_entry *ve_r [128];
	recID children [32];

	for (ve = VirtualTable [vt].first_entry; ve; ve = ve->next)
		if (ve->introduced_at == r)
			ve_r [n++] = ve;

	// bubble sort entries alphabetically
	for (i = 0; i < n; i++)
		for (j = i + 1; j < n; j++)
			if (strcmp (expand (ve_r [i]->entry_name), expand (ve_r [j]->entry_name)) > 0)
				ve = ve_r [i], ve_r [i] = ve_r [j], ve_r [j] = ve;

	for (i = 0; i < n; i++)
		outprintf (STRUCTS, ISTR (ve_r [i]->prototype), ';', -1);

	// now if derrived classes also use this virtual table
	// for new entries, do the unionification thing
	get_child_r.n = 0;
	get_child_r.vchild = children;
	get_children (vt, r);
	if (!(n = get_child_r.n)) return;

	outprintf (STRUCTS, RESERVED_union, '{', -1);
	for (i = 0; i < n; i++) {
		outprintf (STRUCTS, RESERVED_struct, '{', -1);
		declare_vtable_for (vt, children [i]);
		outprintf (STRUCTS, '}', name_derrive_memb (children [i]), ';', -1);
	}
	outprintf (STRUCTS, '}', name_derrive_union, ';', -1);
}

static void declare_vtable (vtblID i)
{
	outprintf (STRUCTS, RESERVED_struct,
		   VirtualTable [i].vtname, '{', -1);
	declare_vtable_for (i, VirtualTable [i].appeared_at);
	outprintf (STRUCTS, '}', ';', -1);
}

static void export_virtual_table_declaration (recID r)
{
	vtblID i;

	if (structs [r].vtis) for (i = 0; i < nVTBL; i++)
		if (VirtualTable [i].appeared_at == r)
			declare_vtable (i);
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// some more allocator/initializers
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

static initID new_v_t_initializer (recID obj)
{
	if (nVTI == nVTIalloc)
		VirtualInitializer = (vt_initializer*) realloc (VirtualInitializer,
			 (nVTIalloc += 50) * sizeof (vt_initializer));

	VirtualInitializer [nVTI].first_entry = 0;
	VirtualInitializer [nVTI].open = true;
	VirtualInitializer [nVTI].unused = false;
//	VirtualInitializer [nVTI].exportdef = structs [obj].statik;
	VirtualInitializer [nVTI].exportdef = false;
	VirtualInitializer [nVTI].rec = obj;

	return nVTI++;
}

static void add_initializer (vtblID vt, initID vi, recID obj)
{
	virtual_table *vp = &VirtualTable [vt];

	if (vp->nvti == vp->nvtalloc)
		vp->vti = (vt_init_entry*) realloc (vp->vti,
			(vp->nvtalloc += 20) * sizeof (vt_init_entry));

	vp->vti [vp->nvti].initTbl = vi;
	vp->vti [vp->nvti++].object = obj;
}

static void add_vti_entry (initID it, vti_entry *e)
{
#ifdef	DEBUG
	if (debugflag.VIRTUALTABLES)
		PRINTF ("++Adding entry "COLS"%s"COLE" to initializer %i\n", expand (e->vt_ent->entry_name), it);
#endif
	vti_entry *ep = VirtualInitializer [it].first_entry;
	if (!ep)
		VirtualInitializer [it].first_entry = e;
	else {
		while (ep->next) ep = ep->next;
		ep->next = e;
	}
}

//---------------------------------------
// build the path to the virtual table
//---------------------------------------

static Token *pathto_vt (Token path [], initID i, bool const_path)
{
	recID rd, ra;

	rd = aliasclass (VirtualInitializer [i].rec);
	ra = aliasclass (VirtualTable [VirtualInitializer [i].v_t_i].appeared_at);

	if (rd == ra)
		sintprintf (path, RESERVED__v_p_t_r_, -1);
	else {
		Token *p;

		bool v = is_ancestor (rd, ra, &p, const_path) == 2;
		sintprintf (path, ISTR (p), v ? POINTSAT : '.', RESERVED__v_p_t_r_, -1);
	}

	return path;
}

/* this is a path that helps us decide whether there is virtual base vti */
static Token *pathto_vt_virt (Token path[], initID i)
{
	recID rd = VirtualInitializer [i].rec;
	recID ra = VirtualTable [VirtualInitializer [i].v_t_i].appeared_at;
	ancestor *a;

	pathto_vt (path, i, false);
	if ((a = structs [rd].ancestors) && ra != rd) {
		while (a->rec != ra) ++a;
		if (a->status == ASTATUS_VIRT2)
			path [intlen (path) - 2] = POINTSAT;
	}

	return path;
}
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// best virtual table to add a new entry is the most
// recently created
// virtual tables reached through virtual inheritance
// dont count because we cannot ensure that the order of
// declarations will not matter.
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

static initID best_vti (recID r)
{
	int i, j, rc = 1000;
	initID ii, ir = -1;
	Token pathto [128];

	if (!structs [r].vtis)
		return -1;

	for (i = 0; (ii = structs [r].vtis [i]) != -1; i++) {
		pathto_vt_virt (pathto, ii);
		for (j = 0; pathto [j] != POINTSAT; j++)
			if (pathto [j] == -1) {
				if (j < rc) {
					rc = j;
					ir = structs [r].vtis [i];
				}
				break;
			}
	}
	return ir;
}

static bool has_vtable (recID r)
{
	if (!structs [r].vtis)
		return false;
	int i, j;
	initID ii;
	Token pathto [128];

	for (i = 0; (ii = structs [r].vtis [i]) != -1; i++) {
		pathto_vt_virt (pathto, ii);
		for (j = 0; pathto [j] != POINTSAT; j++)
			if (pathto [j] == -1)
				return true;
	}
	return false;
}

static inline bool vt_only (recID r)
{
	return structs [r].firstmember || structs [r].ancestors || !has_vtable (r)
		|| VirtualTable [VirtualInitializer [structs [r].vtis [0]].v_t_i].inlined;
}
// #-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#
// Inheritance of virtual tables
// #-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#

/* class <obj> inherits virtual initializer <iid> maybe by virtual inheritance */
static initID inherit_virtual_tbl (recID obj, initID iid)
{
#ifdef	DEBUG
	if (debugflag.VIRTUALTABLES)
		PRINTF ("++Inherit virtual initializer %i in object %i(%s)\n", iid, obj, SNM(obj));
#endif
	vtblID vt = VirtualInitializer [iid].v_t_i;
	bool unused = VirtualInitializer [iid].unused;

	initID ii = new_v_t_initializer (obj);
	add_initializer (vt, ii, obj);
	/* New initializer inherits values from parent */
	vti_entry *vp, *vc;
	for (vp = VirtualInitializer [iid].first_entry; vp; vp = vp->next) {
		vc = (vti_entry*) malloc (sizeof * vc);
		vc->next = 0;
		vc->vt_ent = vp->vt_ent;
		vc->value = vp->value;
		vc->code = vp->value && !unused ? 0 : vp->code;
		vc->status = (vp->status == VTI_STATUS_DD) ? VTI_STATUS_ND : vp->status;
		vc->cast_avoid_downcast = vp->cast_avoid_downcast;
		add_vti_entry (ii, vc);
	}
	/* once of these */
	VirtualInitializer [ii].v_t_i = vt;
	VirtualInitializer [ii].instance_name =
		 name_instance (VirtualTable [vt].appeared_at, obj);
	return ii;
}

static void collapse_virtuals (recID);

/* Inherit virtual table initializers from parents
   Must be called after parents are set */
static void inherit_all_virtuales (recID obj)
{
	int j, ti = 0;
	recID p;
	initID tmp [64];
	ancestor *a = structs [obj].ancestors;

	if (a) for (;(p = a->rec) != -1; a++)
		if (direct_ancest (a) && structs [p].vtis)
			for (j = 0; structs [p].vtis [j] != -1; j++)
				tmp [ti++] = inherit_virtual_tbl (obj, structs [p].vtis [j]);

	if (ti) {
		tmp [ti] = -1;
		structs [obj].vtis = intdup (tmp);
		collapse_virtuals (obj);
	} else structs [obj].vtis = 0;
}

/* two initializers of the same virtual table
   are actually initializing the same thing
   because of virtual inheritance, or not ? */
static bool are_the_same (initID i1, initID i2)
{
	int i, j;
	Token p1 [128], p2 [128];
	pathto_vt_virt (p1, i1), pathto_vt_virt (p2, i2);

	for (i = 0; p1 [i] != -1; i++)
		if (p1 [i] == POINTSAT)
			for (j = 0; p2 [j] != -1; j++)
				if (p2 [j] == p1 [i-1] && p2 [j + 1] == POINTSAT)
					goto double_break;
	return false; /* not */
double_break:;
	vti_entry *v1 = VirtualInitializer [i1].first_entry;
	vti_entry *v2 = VirtualInitializer [i2].first_entry;

	/* do the combination of virtual tables.
	 * in the case of functions, there must be a unique final overrider.
	 * for virtual variables, currently, no, but the value wins
	 * for downcast offsets, always pick the v1 if both have.
	 *   the latter is probably impossible unless double virtual inh?
	 * 'final' wins non-final.
	 */
	while (v1) {
		if (v1->status == VTI_STATUS_NI) {
			if (v2->status != VTI_STATUS_NI) copy: {
				v1->value = v2->value;
				v1->code = v2->code;
				v1->status = v2->status;
			}
		} else if (v2->status != VTI_STATUS_NI) {
			if (v1->value == RESERVED_0) {
				if (v2->value != RESERVED_0) {
					v1->value = v2->value;
					v1->code = v2->code;
				}
			} else if (v1->value == 0) {
				if (v1->code == 0 && v2->code != 0)
					v1->code = v2->code;
			} else if (v2->value != RESERVED_0
				&& v2->value != v1->value) {
					if (v2->status == VTI_STATUS_FINAL) {
						if (v2->status != v1->status)
							goto copy;
						else parse_error_ll ("final functions");
					} else if (v1->status != VTI_STATUS_FINAL)
						v1->status = VTI_STATUS_NU;
			}
		}
		v1 = v1->next;
		v2 = v2->next;
	}
	VirtualInitializer [i2].unused = true;
	return true;
}


/* collapse initializers of virtual base classes to a common one */
/* this is the whole point of virtual inheritance */
static void collapse_virtuals (recID r)
{
	int en = 0;
	int i, j;
	initID *pa = structs [r].vtis;

	for (i = 1; pa [i] != -1; i++) {
		vtblID vt = VirtualInitializer [pa [i]].v_t_i;
		Token in = VirtualInitializer [pa [i]].instance_name;

		for (j = 0; j < i; j++) {
			if (VirtualInitializer [pa [j]].instance_name == in)
				VirtualInitializer [pa [i]].instance_name =
					name_name_enumerate (in, en++);
			if (VirtualInitializer [pa [j]].v_t_i == vt)
				if (are_the_same (pa [j], pa [i])) {
					// disappear pa [i]
					for (j = i; pa [j] != -1; j++)
						pa [j] = pa [j + 1];
					i--;
					break;
				}
		}
	}
}
// #-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#
// Virtual Functions
//  declarations of them...
// #-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#

static int lookup_virtual_f_ininit (initID i, Token n, typeID *va, Token **entn)
{
	vtblID vi = VirtualInitializer [i].v_t_i;
	vt_entry *ve;

#ifdef	DEBUG
	if (debugflag.VIRTUALTABLES)
		PRINTF (" ++in virtual table %i, initializer %i\n", vi, i);
#endif
	for (ve = VirtualTable [vi].first_entry; ve; ve = ve->next)
		if (ve->isfunction && ve->fname == n && arglist_compare (va, ve->arglist)) {
			vti_entry *vie;
			*entn = ve->zpath;
			for (vie = VirtualInitializer [i].first_entry; vie; vie = vie->next)
				if (vie->vt_ent->entry_name == ve->entry_name)
					if (vie->status == VTI_STATUS_NI) break;
					else return vie->status; else;
		}
	return VTI_STATUS_NI;
}

typedef struct {
	int status;
	initID iid;
	Token *entn;
} vlookup;

static void lookup_virtual_f (recID r, Token n, typeID t, vlookup *v)
{
#ifdef	DEBUG
	if (debugflag.VIRTUALTABLES)
		PRINTF ("++Looking for %s...\n", expand (n));
#endif
	typeID *va = promoted_arglist_t (t);
	int i = 0, ret;
	Token *ent_ret;

	v->status = VTI_STATUS_NI;

	if (v->iid != -1)
		while (structs [r].vtis [i] != -1)
			if (structs [r].vtis [i++] == v->iid)
				break;

	for (; structs [r].vtis [i] != -1; i++)
	if ((ret = lookup_virtual_f_ininit (structs [r].vtis [i], n, va, &ent_ret)) != VTI_STATUS_NI) {
		v->status = ret;
		v->iid = structs [r].vtis [i];
		v->entn = ent_ret;
		break;
	}
	free (va);
}

// 
// Force generation of virtual table with no entries at specific object
//
initID Here_virtualtable (OUTSTREAM o, recID r, bool inlined, bool constd, bool volatiled)
{
#ifdef	DEBUG
	if (debugflag.VIRTUALTABLES)
		PRINTF ("++New virtual table introduced at class %s (%i)\n", SNM (r), r);
#endif
	int i = 0;

	if (structs [r].vtis)
		for (; structs [r].vtis [i] != -1; i++)
			if (VirtualTable [VirtualInitializer [structs [r].vtis [i]].v_t_i].
			appeared_at == r)
				return -1;

	//if (!inlined)
		add_variable_member (r, RESERVED__v_p_t_r_, typeID_voidP, 0, false, false);
	vtblID vt = new_virtual_table ();
	VirtualTable [vt].appeared_at = r;
	VirtualTable [vt].vtname = name_virtual_table (r);
	VirtualTable [vt].inlined = inlined;
	VirtualTable [vt].constd = constd;
	VirtualTable [vt].volatiled = volatiled;
	outprintf (o, RESERVED_struct, VirtualTable [vt].vtname,
		   CONST_IF (constd), STAR_IF (!inlined),
		   CONST_IF (!inlined && !volatiled),
//		   !inlined && vtptrConst? RESERVED_const : BLANKT,
		   RESERVED__v_p_t_r_, ';', -1);
	initID vi = new_v_t_initializer (r);
	VirtualInitializer [vi].v_t_i = vt;
	VirtualInitializer [vi].instance_name = name_instance (r, r);
	add_initializer (vt, vi, r);
	structs [r].vtis = (initID*) realloc (structs [r].vtis, (i+2) * sizeof (int));
	structs [r].vtis [i] = vi;
	structs [r].vtis [i + 1] = -1;
	structs [r].vt_inthis = true;

	return vi;
}

// #.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#
/* Enter_virtual_function, may do 3 things:
   1.  complain because virtual already has value
   2.  set value to virtual slot (creating internal vf)
   3.  create a new virtual table entry and set value
 ********************************************************/
void Make_virtual_table (OUTSTREAM o, recID r, Token fname, typeID t)
{
	if (!structs [r].vtis)
		Here_virtualtable (o, r, InlineAllVt, ConstVtables, !vtptrConst);
	else {
		vlookup V = { .iid = -1 };
		lookup_virtual_f (r, fname, t, &V);
		if (V.status == VTI_STATUS_NI && !has_vtable (r))
		Here_virtualtable (o, r, InlineAllVt, ConstVtables, !vtptrConst);
	}
}

static void New_virtual_function (vf_args*);
static void Override_virtual_function (vf_args*, vlookup*);
void Enter_virtual_function (vf_args *args)
{
#ifdef	DEBUG
	if (debugflag.VIRTUALTABLES)
	PRINTF ("\n++ENTER VIRTUAL: ["COLS"%s"COLE"] (%s)\n", expand (args->fname), SNM(args->r));
#endif
	vlookup V = { .iid = -1 };
	lookup_virtual_f (args->r, args->fname, args->ftype, &V);

	// [3]
	if (V.status == VTI_STATUS_NI)
		return New_virtual_function (args);

	// [1]
	if (V.status == VTI_STATUS_DD)
		parse_error_tok (args->fname, "Virtual function redefined?");

	// [2]
	while (V.status != VTI_STATUS_NI) {
		if (V.status == VTI_STATUS_FINAL)
			parse_error_tok (args->fname, "Virtual function has been declared final");
		Override_virtual_function (args, &V);
		lookup_virtual_f (args->r, args->fname, args->ftype, &V);
	}
}

static void New_virtual_function (vf_args *args)
{
	// [by convention, add it to the first virtual table if many]
	vtblID vt = VirtualInitializer [best_vti (args->r)].v_t_i;
	int i;
	// + + + + new virtual table entry + + + +
	vt_entry *ve = (vt_entry*) malloc (sizeof * ve);
	*ve = (vt_entry) {
		.next = 0, .entry_name = name_virtual_slot (args->r, args->fname, args->ftype),
		.isfunction = true, .type = args->ftype, .fname = args->fname,
		.arglist = promoted_arglist_t (args->ftype), .introduced_at = args->r,
		.xproto = intdup (args->prototype), .xargs = intdup (args->argv),
		.flagz = args->flagz, .isconst = true
		/* XXX if final, give a nice warning */
	};
	// make pointer to function from function
	{
		Token *fpt = mallocint (intlen (args->prototype) + 5);
		int i = 0, j = 0;
		while (args->prototype [i] != MARKER)
			fpt [j++] = args->prototype [i++];
		fpt [j++] = '(';
		fpt [j++] = '*';
		if (StdcallMembers)
			fpt [j++] = RESERVED_attr_stdcall;
		fpt [j++] = ve->entry_name; ++i;
		fpt [j++] = ')';
		while ((fpt [j++] = args->prototype [i++]) != -1);
		ve->prototype = fpt;
	}
	// add to virtual table slots. table may be empty.
	add_vt_entry (vt, ve);
	// + + + + in all other initializers this entry is NULL/NI + + + +
	for (i = 0; i < VirtualTable [vt].nvti; i++) {
		vti_entry *vte = (vti_entry*) malloc (sizeof * vte);
		vte->next = 0;
		vte->vt_ent = ve;
		vte->code = 0;
		vte->cast_avoid_downcast = 0;

		if (VirtualTable [vt].vti [i].object != args->r) {
			vte->value = RESERVED_0;
			vte->status = VTI_STATUS_NI;
		} else {
			vte->value = args->fmemb ?: RESERVED_0;
			vte->status = VTI_STATUS_DD;
		}
		add_vti_entry (VirtualTable [vt].vti [i].initTbl, vte);
	}
}

void add_struct_to_this (Token *p)
{
	int i, j, k;
	for (i = 0; p [i] != RESERVED_this; i++);
	while (p [i] != '(') i--;
	for (k = i; p [k] != -1; k++);
	for (j = k++; j != i; )
		p [k--] = p [j--];
	p [i+1] = RESERVED_struct;
}

static void Override_virtual_function (vf_args *args, vlookup *v)
{
	Token entn = v->entn [intlen (v->entn) - 1];
	vtblID vt = VirtualInitializer [v->iid].v_t_i;
	recID dow;
	Token *proto = args->prototype;
	bool modular = args->flagz & FUNCP_MODULAR;

	// find the virtual table entry from vlookup data
	// get the downcast info
	{
		vt_entry *vp = VirtualTable [vt].first_entry;
		while (vp->entry_name != entn) vp = vp->next;
		dow = vp->introduced_at;
	}

	// create virtuallity function name
	Token vfn;
	if (args->fmemb) 
		if (!modular) {
			// rewrite the prototype
			vfn = name_virtual_inner (args->r, dow, args->fname, args->ftype);
			proto = allocaint (intlen (args->prototype) + 4);
			intcpy (proto, args->prototype);
			intsubst1 (proto, MARKER, vfn);
			intsubst1 (proto, name_of_struct (args->r), name_of_struct (dow));
			if (is_aliasclass (args->r) && !is_aliasclass (dow))
				add_struct_to_this (proto);
			else if (!is_aliasclass (args->r) && is_aliasclass (dow))
				remove_struct_from_this (proto, dow);
		} else vfn = args->fmemb;
	else vfn = RESERVED_0;

	// set the value vfn for the entry at the initializer
	vti_entry *vtp = VirtualInitializer [v->iid].first_entry;
	while (vtp->vt_ent->entry_name != entn)
		vtp = vtp->next;
	vtp->value = vfn;
	vtp->status = args->flagz & FUNCP_FINAL ? VTI_STATUS_FINAL : VTI_STATUS_DD;
	vtp->cast_avoid_downcast = 0;
	// create the code of the virtuallity function
	if (args->fmemb && !modular) 
		if (aliasclass (args->r) == aliasclass (dow))
			vtp->value = args->fmemb;
		else if (zero_offset (args->r, dow)) {
			vtp->value = args->fmemb;
			vtp->cast_avoid_downcast = 1;
		} else {
			int i;
			Token dc = i_downcast_function (dow, args->r);
			OUTSTREAM tmp = new_stream ();
			outprintf (tmp, //STRNEXT, proto, linkonce_text (vfn), ';',
				   ISTR (proto), '{', RESERVED_return, args->fmemb,
				   '(', dc, '(', RESERVED_this, ')', -1);
			for (i = 1; args->argv [i] != -1; i++)
				outprintf (tmp, ',', args->argv [i], -1);
			outprintf (tmp, ')', ';', '}', -1);
			vtp->code = combine_output (tmp);
		}
}

void purify_vfunc (Token f)
{
	int i;
	vti_entry *v;

	for (i = 0; i < nVTI; i++)
		for (v = VirtualInitializer [i].first_entry; v; v = v->next)
			if (v->value == f || v->code && intchr (v->code, f)) {
				v->value = RESERVED_0;
				VirtualInitializer [i].unused = true;
			}
}
// #*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#
// more virtuallity
// #*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#

static void close_inits (structure *s)
{
	if (!s->vtis) return;
	int i;
	for (i = 0; s->vtis [i] != -1; i++)
		VirtualInitializer [s->vtis [i]].open = false;
}

static void v_t_instances (vtblID vi)
{
	int i, hp, ns, ne;

	for (hp = i = ns = ne = 0; i < VirtualTable [vi].nvti; i++)
		if (!VirtualInitializer [VirtualTable [vi].vti [i].initTbl].unused) {
			if (structs [VirtualInitializer [VirtualTable
					[vi].vti [i].initTbl].rec].keyfunc != -1)
						++ne; else ++ns;
			hp = 1;
		}

	if (!hp)
		return; // ----> leave

	if (ne) {
		outprintf (VTABLE_DECLARATIONS, RESERVED_extern,
			   CONST_IF (VirtualTable [vi].constd), RESERVED_struct,
			    VirtualTable [vi].vtname, -1);
		for (hp = i = 0; i < VirtualTable [vi].nvti; i++)
			if (!VirtualInitializer [VirtualTable [vi].vti [i].initTbl].unused
			&& structs [VirtualInitializer [VirtualTable
				 [vi].vti [i].initTbl].rec].keyfunc != -1) {
				outprintf (VTABLE_DECLARATIONS, COMMA_IF (hp),
					   VirtualInitializer [VirtualTable [vi].vti
					   [i].initTbl].instance_name, -1);
				hp = 1;
			}
		output_itoken (VTABLE_DECLARATIONS, ';');
	}
	if (ns) {
		outprintf (VTABLE_DECLARATIONS, RESERVED_static,
			   CONST_IF (VirtualTable [vi].constd), RESERVED_struct,
			    VirtualTable [vi].vtname, -1);
		for (hp = i = 0; i < VirtualTable [vi].nvti; i++)
			if (!VirtualInitializer [VirtualTable [vi].vti [i].initTbl].unused
			&& structs [VirtualInitializer [VirtualTable
				 [vi].vti [i].initTbl].rec].keyfunc == -1) {
				outprintf (VTABLE_DECLARATIONS, COMMA_IF (hp),
					   VirtualInitializer [VirtualTable [vi].vti
					   [i].initTbl].instance_name, -1);
				hp = 1;
			}
		output_itoken (VTABLE_DECLARATIONS, ';');
	}
}

void export_virtual_table_instances ()
{
	int i;
	for (i = 0; i < nVTBL; i++)
		v_t_instances (i);
}

static void virtual_init_definition (initID i, bool stc)
{
	Token inst = VirtualInitializer [i].instance_name;
	bool constd = VirtualTable [VirtualInitializer [i].v_t_i].constd;
	vti_entry *ve;

	// emit the code of the virtuallity downcasting functions
	for (ve = VirtualInitializer [i].first_entry; ve; ve = ve->next)
		if (ve->code && ve->value && ve->value != RESERVED_0)
			outprintf (VIRTUALTABLES, RESERVED_inline, RESERVED_static,
				   ISTR (ve->code), -1);

	outprintf (VIRTUALTABLES, CONST_IF (constd), STATIC_IF (stc), RESERVED_struct,
		   VirtualTable [VirtualInitializer [i].v_t_i].vtname,
		   inst, -1);

#ifdef	HAVE_LINKONCE
	/* Not used after the discovery of the "key function" technique */
	if (0)
		outprintf (VIRTUALTABLES, (constd ? linkonce_rodata : linkonce_data) (inst), -1);
	//else	output_itoken (VIRTUALTABLES, section_vtblz (structs));
#endif
	if (constd)
		output_itoken (VIRTUALTABLES, section_vtblz (
			name_of_struct (VirtualTable [VirtualInitializer [i].v_t_i].appeared_at)));

	for (ve = VirtualInitializer [i].first_entry; ve; ve = ve->next)
		if (ve->value || ve->code)
			break;
	if (!ve) goto closure;

	outprintf (VIRTUALTABLES, '=', '{', -1);
	for (ve = VirtualInitializer [i].first_entry; ve; ve = ve->next)
		if (ve->status != VTI_STATUS_NI)
			if (ve->cast_avoid_downcast && ve->value && ve->value != RESERVED_0)
			// cast avoid downcast
			outprintf (VIRTUALTABLES, '.', ISTR (ve->vt_ent->zpath),
				   '=', '(', RESERVED___typeof__, '(', inst, '.',
				   ISTR (ve->vt_ent->zpath), ')', ')', ve->value, ',', -1);
			else if (ve->value)		// functions have this
			outprintf (VIRTUALTABLES, '.',
				   ISTR (ve->vt_ent->zpath), '=', ve->value, ',', -1);
			else if (ve->code)	// virtual variables with const initi
			outprintf (VIRTUALTABLES, '.',
				   ISTR (ve->vt_ent->zpath), '=', ISTR (ve->code), ',', -1);
	outprintf (VIRTUALTABLES, '}', -1);

closure:
	outprintf (VIRTUALTABLES, ';', -1);
}

void export_virtual_definitions ()
{
	int i;
	for (i = 0; i < nVTI; i++)
		if (!VirtualInitializer [i].unused &&
		 (ExportVtbl || structs [VirtualInitializer [i].rec].evt)) {
			recID r = VirtualInitializer [i].rec;
			if (structs [r].havekey)
				virtual_init_definition (i, 0);
			else if (structs [r].keyfunc == -1)
				virtual_init_definition (i, 1);
			else if (!structs [r].keyfunc) {
//				PRINTF (COLS"******** NO KEYFUNC FOR %s *****\n"COLE, SNM(r));
//				exit (1);
			} else {
//				PRINTF (COLS"WILL NOT EMIT vtbl FOR %s *****\n"COLE, SNM(r));
			}
		}
}

void export_virtual_static_definitions ()
{
	int i;
	for (i = 0; i < nVTI; i++)
		if (!VirtualInitializer [i].unused
		&& VirtualInitializer [i].exportdef)
			virtual_init_definition (i, 1);
}
// #/#/#/#/#/#/#/#/#/#/#/#/#/#/#/#/#/#/#
// allocate structure & all virtual base
// classes in one structure.
// Mainly useful to free() all of them
// with one call.
// #/#/#/#/#/#/#/#/#/#/#/#/#/#/#/#/#/#/#

static void mark_vtinit_unused (recID r)
{
	initID i, j;
	for (i = 0; (j = structs [r].vtis [i]) != -1; i++)
		VirtualInitializer [j].unused = true;
}

/* We can instantiate a class if it has no pure virtual
   functions and there are unique final overriders for
   all of them  */
static bool can_instantiate (recID r)
{
	if (structs [r].vtis) {

		initID i;
		for (i = 0; structs [r].vtis [i] != -1; i++) {
			vti_entry *v = VirtualInitializer [structs [r].vtis [i]].first_entry;
			for (; v; v = v->next)
				if (v->status != VTI_STATUS_NI && v->value == RESERVED_0) {
					structs [r].caninst = CANT_PUREV;
					mark_vtinit_unused (r);
					return false;
				} else if (v->status == VTI_STATUS_NU) {
					structs [r].caninst = CANT_NUFO;
					mark_vtinit_unused (r);
					return false;
				}
		}
	}

	structs [r].caninst = OK_CAN;
	return true;
}

/* We have either the definition of a common instance (class no longer abstract),
 * or collapse of virtual tables due to virtual inheritance.
 * vbase is the virtual base, r is the current class and vbpath the path
 * that leads to the vb instance. Calculate all related offsets and set
 * their values at the entries of the [r] virtual initializer  */
#define	VOIDCAST '(', RESERVED_void, '*', ')'
static void dcast_offsets (recID vbase, recID r, Token *vbpath)
{
	if (!structs [vbase].rtti) return;
	if (!structs [vbase].vtis) return;

	int i, j, ii;
	recID dc;
	Token rtn, *path, d [128];
	vti_entry *ve=ve;

	for (i = 0; (dc = structs [vbase].rtti [i].to) != -1; i++)
		if (dc == r || is_ancestor (r, dc, &path, true)) {
			rtn = structs [vbase].rtti [i].entry;

			if (dc == r)
				sintprintf (d, VOIDCAST, RESERVED_0, -1);
			else {
				sintprintf (d, VOIDCAST, '&', '(', '(', iRESERVED_struct (r),
				name_of_struct (r), '*', ')',
				RESERVED_0, ')', POINTSAT, ISTR (path), -1);
			}

			sintprintf (d + intlen (d), '-', VOIDCAST, '&', '(', '(',
				iRESERVED_struct (r), name_of_struct (r), '*', ')', 
				RESERVED_0, ')', POINTSAT, ISTR (vbpath), -1);

			for (j = 0; (ii = structs [r].vtis [j]) != -1; j++)
				for (ve = VirtualInitializer [ii].first_entry; ve; ve = ve->next)
					if (ve->vt_ent->entry_name == rtn)
						goto dbreak;
			parse_error_ll ("Bug bug bug ()");
		dbreak:
			ve->code = intdup (d);
#ifdef DEBUG
	if (debugflag.VIRTUALBASE) {
		PRINTF ("\n "COLS"+"COLE"Changed offset of "COLS"%s"COLE" in the vtable of "COLS"%s"COLE" to \n\t"COLS,
			expand (rtn), SNM (r));
		INTPRINT (d);
		PRINTF (COLE"\n\n");
	}
#endif
		}
}

static inline bool ancestor_is_vbaseclass (ancestor *a)
{
	return (a->status == ASTATUS_VIRT || a->status == ASTATUS_VIRT2) && a->rec == a->vbase;
}

static void fix_all_offsets (recID r)
{
	ancestor *a;

	for (a = structs [r].ancestors; a->rec != -1; a++)
		if (ancestor_is_vbaseclass (a) && a->cpath)
			dcast_offsets (a->rec, r, a->cpath);
}

static void update_ancestor_paths (recID r, recID vb, Token sn)
{
	Token *p;
	ancestor *a;

	for (a = structs [r].ancestors; a->rec != -1; a++)
		if (a->status == ASTATUS_VIRT && a->vbase == vb) {
			Token n = name_inherited (name_of_struct (vb));
			Token tmp [64];
			Token *p = &a->path [intlen (a->path)];
			while (p >= a->path && *p != n) p--;
			if (p [1] == -1 || p < a->path)
				sintprintf (tmp, sn, -1);
			else
				sintprintf (tmp, sn, '.', ISTR (p + 2), -1);
			a->cpath = intdup (tmp);
		}

	/* this is rare (chained virtual inheritance) */
	for (a = structs [r].ancestors; a->rec != -1; a++)
		if ((a->status == ASTATUS_VIRT || a->status == ASTATUS_VIRT2)
		&&  a->vbase != vb && (a->cpath && intchr (a->cpath, POINTSAT))
		&&  is_ancestor (vb, a->rec, &p, true)) {
			Token tmp [64];
			sintprintf (tmp, sn, '.', ISTR (p), -1);
			if (a->cpath) free (a->cpath);
			a->cpath = intdup (tmp);
			//i = 0;   (restart? very rare) XXX
		}
#ifdef DEBUG
	if (debugflag.VIRTUALTABLES) show_ancest (r);
#endif
}

/* declare common shared instance && update const ancestor paths */
static void decl_vballoc_rec (OUTSTREAM o, recID r)
{
	ancestor *a;
	recID vr;

	for (a = structs [r].ancestors; a->rec != -1; a++)
		if (a->status == ASTATUS_VIRT && a->vbase == (vr = a->rec)
		&& !a->cpath) {
			Token sn = name_storage_inherit (vr);
			outprintf (o, iRESERVED_struct (vr), name_of_struct (vr), sn, ';', -1);
			sintprintf (a->cpath = mallocint (2), sn, -1);
			update_ancestor_paths (r, vr, sn);
			structs [r].need_recalc_offsets = true;
		}
}

/* Update all the downcast offsets so they all use the same
 * shared instance.  This is called after multiple inheritance */
static void update_dcasts (recID r, recID vbs [])
{
	int i;
	ancestor *a;

	if (!structs [r].need_recalc_offsets)
		for (i = 0; vbs [i] != -1; i++) {
			for (a = structs [r].ancestors; a->rec != vbs [i]; a++)
				;;;;;
			if (a->cpath) {
				structs [r].need_recalc_offsets = true;
				return;
			}
		}
}
// #*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#
// Requesting virtual functions from expressions
// #*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#

bool lookup_virtual_function_member (recID r, Token f, typeID argv[],
				     Token *vpath, flookup *F)
{
	if (!structs [r].vtis)
		return false;

	argv [0] = pthis_of_struct (r);
	typeID *va = promoted_arglist (argv);
	int i, ret, status;
	initID iid=iid;	/* this is done to avoid warning for uninitialized possible use */
	Token *ent_ret, *entn=entn, entname=entname;

	ret = status = VTI_STATUS_NI;

	for (i = 0; structs [r].vtis [i] != -1; i++)
	if ((ret = lookup_virtual_f_ininit (structs [r].vtis[i], f, va, &ent_ret)) != VTI_STATUS_NI)
		if (status != VTI_STATUS_NI)
			parse_error (0, "Ambiguity virtual inheritance");
		else {
			status = ret;
			iid = structs [r].vtis [i];
			entn = ent_ret;
			entname = entn [intlen (entn) - 1];
		}
	free (va);

	if (status == VTI_STATUS_NI || status == VTI_STATUS_FINAL)
		return false;

#ifdef	DEBUG
	if (debugflag.VIRTUALTABLES)
		PRINTF ("++ Virtual call for ["COLS"%s"COLE"]\n", expand (f));
#endif
	/* We must return: The path that leads to the virtual table entry in <vpath>
	 * The type of the <this> argument in argv [0]
	 * And finally, the function's type in <F>
	 *  Let's go and do this
	 */
	vtblID vt = VirtualInitializer [iid].v_t_i;
	recID bo_rec;
	{
		vt_entry *ve;
		for (ve = VirtualTable [vt].first_entry; ve; ve = ve->next)
			if (ve->entry_name == entname) break;
		bo_rec = base_of (ve->arglist [0]);
		F->t = ve->type;
		F->prototype = ve->xproto;
		F->xargs = ve->xargs;
		F->flagz = ve->flagz;
		F->oname = 0;
	}
	//[1]
	{
		Token tpath [128];
		sintprintf (vpath, ISTR (pathto_vt (tpath, iid, false)),
			    VirtualTable [vt].inlined ? '.' : POINTSAT, ISTR (entn), -1);
	}
	//[2]
	argv [0] = pthis_of_struct (bo_rec);
	return true;
}

bool Is_pure_virtual (recID r, Token f, typeID argt [], flookup *F)
{
	if (!structs [r].vtis)
		return false;

	argt [0] = pthis_of_struct (r);
	typeID *va = promoted_arglist (argt);
	int i, j;
	for (j = 0; (i = structs [r].vtis [j]) != -1; j++) {
		vtblID vi = VirtualInitializer [i].v_t_i;
		vt_entry *ve;
		for (ve = VirtualTable [vi].first_entry; ve; ve = ve->next)
		if (ve->isfunction && ve->fname == f && arglist_compare (va, ve->arglist)) {
			vti_entry *vie;
			free (va);
			F->t = ve->type;
			F->oname = RESERVED_0;
			F->dflt_args = 0;
			for (vie = VirtualInitializer [i].first_entry; ; vie = vie->next)
				if (vie->vt_ent->entry_name == ve->entry_name)
				switch (vie->status) {
				case VTI_STATUS_NI: return false;
				case VTI_STATUS_NU: return true;
				default: return vie->value == RESERVED_0;
				}
		}
	}
	free (va);
	return false;
}

// #+#+#+#+#+#+#+#+#+#+#+#+#+#+#+#+#+#+#+#
// Setting of pointer to virtual table at
// construction of objects
// Construction of common virtual base
// instances
// #+#+#+#+#+#+#+#+#+#+#+#+#+#+#+#+#+#+#+#

bool need_construction (recID r)
{
	return structs [r].has_vbase || (!structs [r].noctor && (structs [r].vtis
		 || structs [r].ctorables || structs [r].ancestors_have_ctors));
}

bool need_vbase_alloc (recID r)
{
	return structs [r].has_vbase;
}

/* assignm apropriate virtual table initializer to object */
void gen_vt_init (OUTSTREAM o, recID r, Token obj, bool throuptr)
{
	Token path [128];
	int i;

	for (i = 0; structs [r].vtis [i] != -1; i++) {
		initID ii = structs [r].vtis [i];
		vtblID vt = VirtualInitializer [ii].v_t_i;
		bool constd = VirtualTable [vt].constd;
		bool inlined = VirtualTable [vt].inlined;
		bool volatiled = VirtualTable [vt].volatiled;
		Token vtn = VirtualTable [vt].vtname;
		Token instn = VirtualInitializer [ii].instance_name;

		if (inlined) // write vtbl despite possible constnesses
#ifdef	HAVE_BUILTIN_MEMCPY	// this is wrong. nothing to do with "__builtin"
			outprintf (o, INTERN_memcpy, '(',
				  '(', RESERVED_void, '*', ')', 
				   '&', obj, throuptr ? POINTSAT : '.',
				   ISTR (pathto_vt (path, ii, true)),
				   ',', '&', instn, ',', RESERVED_sizeof, '(',
				   instn, ')', ')', ';', -1);
#else
			outprintf (o, '*', '(', '(', RESERVED_struct,
				   VirtualTable [vt].vtname, '*', ')',
				   '&', obj, throuptr ? POINTSAT : '.',
				   ISTR (pathto_vt (path, ii, true)),
				   ')', '=', instn, ';', -1);
#endif
		else {
			if (!volatiled)
			outprintf (o, '*', '(', RESERVED_struct, vtn, '*', '*',
				   ')', '&', obj, throuptr ? POINTSAT : '.',
				   ISTR (pathto_vt (path, ii, true)), '=', -1);//'&', instn, ';', -1);
			else outprintf (o, obj, throuptr ? POINTSAT : '.',
			   	ISTR (pathto_vt (path, ii, true)), '=', -1);//'&', instn, ';', -1);
			if (constd) outprintf (o, '(', RESERVED_void, '*', ')', -1);
			outprintf (o, '&', instn, ';', -1);
		}
	}
}

// #*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#
// rtti: when we want to downcast
// from a virtual base class to a
// derrived class.  We have to do
// that in virtual functns anyway
// #*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#

void downcast_rtti (recID rder, recID rbase, recID rs, Token path[])
{
	int i;
	for (i = 0; structs [rbase].rtti [i].to != rder; i++)
		;
	rtti_downcast *rtti = &structs [rbase].rtti [i];
	initID vti;
	if ((vti = rtti->vti) == -1)
		parse_error (0, "Can't downcast. No virtual table. No RTTI.");

	vt_entry *ve;
	vtblID vt = VirtualInitializer [vti].v_t_i;

	for (ve = VirtualTable [vt].first_entry;; ve = ve->next)
		if (ve->entry_name == rtti->entry) break;

	for (vti = 0; VirtualTable [vt].vti [vti].object != rs; vti++);
	vti = VirtualTable [vt].vti [vti].initTbl;

	Token vpath [128];
	sintprintf (path, ISTR (pathto_vt (vpath, vti, false)),
		    VirtualTable [vt].inlined ? '.' : POINTSAT, ISTR (ve->zpath), -1);
}

static int new_rtti_entry (recID obj)
{
	int i = 0;

	if (structs [obj].rtti)
		for (i = 1; structs [obj].rtti [i].to != -1; i++);
	structs [obj].rtti = (rtti_downcast*) realloc (structs [obj].rtti,
				(i + 2) * sizeof (rtti_downcast));
	structs [obj].rtti [i + 1].to = -1;

	return i;
}

static int lookup_rtti (recID obj, Token c)
{
	int i;

	if (structs [obj].rtti)
		for (i = 0; structs [obj].rtti [i].to != -1; i++)
			if (structs [obj].rtti [i].fwd_dcl == c)
				return i;
	return -1;
}

int virtual_inheritance_decl (OUTSTREAM O, recID obj, Token c)
{
	int i = lookup_rtti (obj, c);

	if (i != -1) return i;
	if (O) structs [obj].have_vi_decls = true;

	i = new_rtti_entry (obj);
	structs [obj].rtti [i].fwd_dcl = c;
	structs [obj].rtti [i].to = 0;

	initID ii = best_vti (obj);
	if (ii == -1) {
		if (!O) parse_error_tok (name_of_struct (obj),
				"In order to do virtual inheritance, please "
				"create a virtual table for this class");
		Here_virtualtable (O, obj, false, ConstVtables, !vtptrConst);
		ii = best_vti (obj);
	}

	structs [obj].rtti [i].vti = ii;

	/* make virtual table entry */
	vtblID vt = VirtualInitializer [ii].v_t_i;
	vt_entry *ve = (vt_entry*) malloc (sizeof * ve);
	ve->next = 0;
	ve->entry_name = structs [obj].rtti [i].entry = name_rtti_slot (obj, c);
	ve->fname = 0;
	ve->isfunction = false;
	ve->introduced_at = obj;
	ve->prototype = mallocint (5);
	ve->isconst = true;
	sintprintf (ve->prototype, /*RESERVED_const,*/ RESERVED_int, ve->entry_name, -1);
	add_vt_entry (vt, ve);

	return i;
}

static void add_rtti (recID obj, recID der)
{
	vtblID vt;
	initID I;
	vt_entry *ve;
	int i, j;

	i = lookup_rtti (obj, name_of_struct (der));
	if (i == -1)
		if (!structs [obj].vtis) {
			// no virtual table! downcast impossible
			i = new_rtti_entry (obj);
			structs [obj].rtti [i].to = der;
			structs [obj].rtti [i].fwd_dcl = name_of_struct (der);
			structs [obj].rtti [i].vti = -1;
			return;
		} else if (VIDeclarations || structs [obj].have_vi_decls)
			parse_error_tok (name_of_struct (der), "Virtual inheritance not declared");
		else i = virtual_inheritance_decl (0, obj, name_of_struct (der));

	/* activate the virtual table entry */
	structs [obj].rtti [i].to = der;
	I = structs [obj].rtti [i].vti;
	vt = VirtualInitializer [I].v_t_i;
	for (ve = VirtualTable [vt].first_entry;
		ve->entry_name != structs [obj].rtti [i].entry; ve = ve->next);

	ve->type = pthis_of_struct (der);
	for (j = 0; j < VirtualTable [vt].nvti; j++) {
		vti_entry *vte = (vti_entry*) malloc (sizeof * vte);
		vte->next = 0;
		vte->vt_ent = ve;
		vte->code = 0; vte->value = 0;
		vte->status = VirtualTable [vt].vti [j].object == der ?
				VTI_STATUS_DD : VTI_STATUS_NI;
		vte->cast_avoid_downcast = 0;
		add_vti_entry (VirtualTable [vt].vti [j].initTbl, vte);
	}
}

static void make_rtti (recID r)
{
	ancestor *a;

	for (a = structs [r].ancestors; a->rec != -1; a++)
		if (a->path [1] == -1
		&& (a->status == ASTATUS_VIRT || a->status == ASTATUS_VIRT2))
			add_rtti (a->rec, r);
}

// #<#<#<#<#<#<#<#<#<#<#<#<#<#<#<#
// Virtual data members
// #<#<#<#<#<#<#<#<#<#<#<#<#<#<#<#

typeID lookup_virtual_varmemb (recID r, Token m, Token *path, bool const_path, Token **cval)
{
	int		i;
	initID		ii;
	vtblID		vt;
	vt_entry	*ve;
	vti_entry	*vte;
	typeID		rett = -1;

	if (!structs [r].vtis)
		return -1;

	for (i = 0; (ii = structs [r].vtis [i]) != -1; i++) {
		vt = VirtualInitializer [ii].v_t_i;
		for (ve = VirtualTable [vt].first_entry; ve; ve = ve->next)
		if (!ve->isfunction && ve->fname == m)
			for (vte = VirtualInitializer [ii].first_entry; vte; vte = vte->next)
			if (vte->vt_ent->entry_name==ve->entry_name && vte->status != VTI_STATUS_NI)
				if (rett != -1) expr_errort ("ambiguous virtual member", m);
				else {
					Token vpath [128];
					if (path) {
					if (!const_path)
					sintprintf (path, ISTR (pathto_vt (vpath, ii, false)), 
						    VirtualTable [vt].inlined ? '.' : POINTSAT,
						    ISTR (vte->vt_ent->zpath), -1);
					else sintprintf (path, VirtualInitializer [ii].
							 instance_name, '.',
							 ISTR (vte->vt_ent->zpath), -1);
					}
					if (cval)
						*cval = !path || VirtualTable [vt].constd
							 || ve->isconst ?
							vte->code ?: zinit : 0;
					rett = ve->type;
					if (objective.recording && VirtualInitializer [ii].unused
					&& const_path)
						usage_set_pure ();
				}
	}
	return rett;
}

/* <classname>.<member> syntax */
vtvar access_virtual_variable (recID r, Token m)
{
	vtvar		ret = { .t = -1 };
	Token		have = -1;
	int		i;
	initID		ii;
	vtblID		vt;
	vt_entry	*ve;
	vti_entry	*vte;

	if (!structs [r].vtis)
		return ret;

	for (i = 0; (ii = structs [r].vtis [i]) != -1; i++) {
		vt = VirtualInitializer [ii].v_t_i;
		for (ve = VirtualTable [vt].first_entry; ve; ve = ve->next)
		if (!ve->isfunction && ve->fname == m)
			for (vte = VirtualInitializer [ii].first_entry; vte; vte = vte->next)
			if (vte->vt_ent->entry_name == ve->entry_name && vte->status != VTI_STATUS_NI)
				if (have != -1) expr_errort ("ambiguous virtual member", m);
				else {
					if (VirtualTable [vt].inlined)
						expr_errort ("can't access virtual variable of inlined virtual table", m);
					ret.rec = have = VirtualInitializer [ii].instance_name;
					ret.memb = ve->zpath;
					ret.t = ve->type;
					ret.expr = vte->code;
				}
	}
	return ret;
}

bool Is_implied_virtual_variable (recID r, Token m)
{
	return lookup_virtual_varmemb (r, m, 0, 0, 0) != -1;
}

void add_virtual_varmemb (recID r, Token m, Token a, typeID t, Token *expr, Token *proto,
			  bool cnst, OUTSTREAM o)
{
	int i;
	vt_entry *ve;
	vti_entry *vte;
	vtblID vt;
	bool isconst = proto [0] == RESERVED_const || proto [1] == RESERVED_const;

	if (!structs [r].vtis) {
spagetti:;
	/* XXX: why don't we call Here_virtual_table() here ?????? */
	/* XXX: We demand to see the persons who wrote this */
		/* Virtualize Class */
		add_variable_member (r, RESERVED__v_p_t_r_, typeID_voidP, 0, false, false);
		vt = new_virtual_table ();
		bool inlined = VirtualTable [vt].inlined;
		bool constd = VirtualTable [vt].constd;

		if (m == RESERVED_typeid)
			VirtualTable [vt].typeid = true;
		VirtualTable [vt].appeared_at = r;
		VirtualTable [vt].vtname = name_virtual_table (r);
		outprintf (o, RESERVED_struct, VirtualTable [vt].vtname,
			   CONST_IF (constd), STAR_IF (!inlined),
		   	   !inlined && vtptrConst? RESERVED_const : BLANKT,
			   RESERVED__v_p_t_r_, ';', -1);

		Token entry_name = name_virtual_variable (r, m);
		ve = (vt_entry*) malloc (sizeof * ve);

		intsubst (proto, MARKER, entry_name);
		*ve = (vt_entry) {
			.next = 0, .entry_name = entry_name, .isconst = isconst,
			.isfunction = false, .type = t, .fname = m,
			.prototype = intdup (proto), .introduced_at = r
		};
		add_vt_entry (vt, ve);

		initID vi = new_v_t_initializer (r);
		VirtualInitializer [vi].v_t_i = vt;
		VirtualInitializer [vi].instance_name = name_instance (r, r);
		add_initializer (vt, vi, r);

		vte = (vti_entry*) malloc (sizeof * vte);
		*vte = (vti_entry) {
			.next = 0, .vt_ent = ve, .value = 0,
			.code = expr, .status = VTI_STATUS_DD
		};
		VirtualInitializer [vi].first_entry = vte;

		structs [r].vtis = mallocint (2);
		structs [r].vtis [0] = vi;
		structs [r].vtis [1] = -1;
		return;
	}
	Token *initexpr;
	typeID ht = lookup_virtual_varmemb (r, m, 0, 0, &initexpr);
	if (ht == -1) {
		if (!has_vtable (r))
			 goto spagetti;

		/* New Entry To Best Virtual Table */
		vt = VirtualInitializer [best_vti (r)].v_t_i;
		if (m == RESERVED_typeid)
			VirtualTable [vt].typeid = true;
		ve = (vt_entry*) malloc (sizeof * ve);
		Token entry_name = name_virtual_variable (r, m);

		intsubst (proto, MARKER, entry_name);
		*ve = (vt_entry) {
			.next = 0, .entry_name = entry_name, .isconst = isconst,
			.isfunction = false, .type = t, .fname = m,
			.prototype = intdup (proto), .introduced_at = r
		};
		add_vt_entry (vt, ve);

		for (i = 0; i < VirtualTable [vt].nvti; i++) {
			vte = (vti_entry*) malloc (sizeof * vte);
			*vte = (vti_entry) {
				.next = 0, .vt_ent = ve, .value = 0
			};
			if (VirtualTable [vt].vti [i].object != r) {
				vte->code = 0;
				vte->status = VTI_STATUS_NI;
			} else {
				vte->code = expr;
				vte->status = VTI_STATUS_DD;
			}
			add_vti_entry (VirtualTable [vt].vti [i].initTbl, vte);
		}
		return;
	}
	if (ht != t)
		parse_error_tok (m, "Virtual member redefined with different type");
	if (in2 (a, ASSIGNBO, ASSIGNBA) && initexpr) {
		Token *xx = mallocint (intlen (expr) + intlen (initexpr) + 5);
		sintprintf (xx, '(', ISTR (initexpr), ')', a == ASSIGNBO ? '|' : '&',
			   ISTR (expr), -1);
		free (expr);
		expr = xx;
	}
	initID ii;
	for (i = 0; (ii = structs [r].vtis [i]) != -1; i++) {
		vt = VirtualInitializer [ii].v_t_i;
		for (ve = VirtualTable [vt].first_entry; ve; ve = ve->next)
		if (ve->fname == m)
			for (vte = VirtualInitializer [ii].first_entry; vte; vte = vte->next)
			if (vte->vt_ent->entry_name == ve->entry_name && vte->status != VTI_STATUS_NI) {
				if (vte->status == VTI_STATUS_DD) parse_error_tok (m, "redefined?");
				vte->status = VTI_STATUS_DD;
				vte->code = expr;
				return;
			}
	}
	parse_error_tok (m, "Some strange kind of bug (:)");
}

void mk_typeid (recID r)
{
	int i;
	vti_entry *v;
	vt_entry *ve;

	if (!structs [r].vtis)
		return;

	for (i = 0; structs [r].vtis [i] != -1; i++)
		if (VirtualTable [VirtualInitializer [structs [r].vtis [i]].v_t_i].typeid)
			for (v = VirtualInitializer [structs [r].vtis [i]].first_entry; v;
			     v = v->next)
				if (!(ve = v->vt_ent)->isfunction && ve->fname == RESERVED_typeid) {
					if (v->status != VTI_STATUS_NI)
						v->code = sintprintf (mallocint (2),
							i_typeid_var (name_of_struct (r)), -1);
					break;
				}
}

Token get_class_vptr (recID r)
{
	int i;
	initID *j = structs [r].vtis;
	int vid = -1;

	if (!j) expr_errort ("No _v_p_t_r_ for class", name_of_struct (r));

	if (j [1] == -1)
		vid = j [0];
	else for (i = 0; j [i] != -1; i++)
		if (VirtualTable [VirtualInitializer [j[i]].v_t_i].appeared_at == r)
			vid = j [i];
	if (vid == -1)
		expr_errort ("Ambiguous _v_p_t_r_ for class", name_of_struct (r));
	if (objective.recording && VirtualInitializer [vid].unused)
		usage_set_pure ();
	return VirtualInitializer [vid].instance_name;
}
