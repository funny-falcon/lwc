#include "global.h"

/******************************************************************************

	Welcome to the dcl module.  A parser of C declarations.

	This is what's called `a kludge'.
	However, based on the good foundaries of the other modules
	we can advance adding extensions here, making the code even
	more hairy.

	Eventually this becomes a parser for LWC declarations.

******************************************************************************/

recID current_scope [16];
int top_scope;
#define PUSH_CURRENT_SCOPE(x) current_scope[++top_scope]=x
#define POP_CURRENT_SCOPE --top_scope

#define BASETYPE_ABSTRACT -2
#define BASETYPE_SKIP -3

typedef struct {
	int dclqual_i;
	Token dclqual [8];
	Token shortname [80];
	recID basetype;
	typeID basetype2;
	bool istypedef, isvirtual, isextern, isstatic, isauto, isvolatile, is__noctor__,
	     isinline, isconst, islinkonce, ismodular, is__unwind__, isfinal, is__emit_vtbl__,
	     is__byvalue__;
	Token section;
	Token class_name;
	Token btname;
	bool anon_union, maybe_ctor;
} Dclbase;

static Dclbase *outer_dclqual;

Token bt_macro;
Token bt_replace;

static NormPtr collect_dclqual (Dclbase *b, NormPtr p, int remove_attributes)
{
more:
	while (ISDCLFLAG (CODE [p]))
		b->dclqual_i |= 1 << (CODE [p++] - RESERVED_auto);
	/* Feature: __section__ ("foo") declaration spec */
	if (CODE [p] == RESERVED___section__) {
		if (CODE [p + 1] != '(' || !is_literal (CODE [p + 2]) || CODE [p + 3] != ')')
			parse_error (p, "__section__ '(' 'string literal' ')'");
		b->section = CODE [p + 2];
		p += 4;
		goto more;
	}
	/* ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ */
	if (remove_attributes && CODE [p] == RESERVED___attribute__) {
		/* cygwin: uses attributes between dclquals. Ignore it */
		p = skip_parenthesis (p + 2);
		goto more;
	}
	return p;
}

/* Take the declaration qualifiers from a decl and
 * enable them for another decl */
static void append_dclqual (Dclbase *b, Dclbase *m)
{
	if (b->isstatic) {
		if (!intchr (m->dclqual, RESERVED_extern)
		&& !intchr (m->dclqual, RESERVED_static))
			intcatc (m->dclqual, RESERVED_static);
		m->isstatic = 1;
	}
	if (b->isinline) {
		if (!intchr (m->dclqual, RESERVED_inline))
			intcatc (m->dclqual, RESERVED_inline);
		m->isinline = 1;
	}
	if (b->isvirtual) m->isvirtual = 1;
	if (b->isauto) m->isauto = 1;
	if (b->ismodular) m->ismodular = 1;
	if (b->isfinal) m->isfinal = 1;
	if (b->section && !m->section) m->section = b->section;
}

static void sumup_dclqual (Dclbase *b)
{
#define ADDFLAG(x) b->is ## x = b->dclqual_i & (1 << (RESERVED_ ## x - RESERVED_auto))
	int i, j = 0;
	for (i = RESERVED_auto + 1; ISCDCLFLAG (i); i++)
		if ((b->dclqual_i & (1 << (i - RESERVED_auto))))
			b->dclqual [j++] = i;
	b->dclqual [j] = -1;
	ADDFLAG(typedef);	ADDFLAG(virtual);
	ADDFLAG(extern);	ADDFLAG(static);
	ADDFLAG(auto);		ADDFLAG(inline);
	ADDFLAG(const);		ADDFLAG(linkonce);
	ADDFLAG(modular);	ADDFLAG(__unwind__);
	ADDFLAG(volatile);	ADDFLAG(final);
	ADDFLAG(__noctor__);	ADDFLAG(__emit_vtbl__);
	ADDFLAG(__byvalue__);
}

static NormPtr bt_builtin (Dclbase *b, NormPtr p)
{
	Token bt = RESERVED_int;
	int sh, lo, si, us, i = 0;

	for (sh = lo = si = us = 0; ISTBASETYPE (CODE [p]); p++)
		switch (CODE [p]) {
		 case RESERVED_short: sh++;
		ncase RESERVED_long: lo++;
		ncase RESERVED_signed: si++;
		ncase RESERVED_unsigned: us++;
		ncase RESERVED_ssz_t: us++;
		ncase RESERVED_usz_t: us++;
		ndefault: bt = CODE [p];
		}

	switch (bt) {
	 case RESERVED_float:
		 b->basetype = B_FLOAT;
		 b->shortname [i++] = RESERVED_float;
#ifdef __LWC_HAS_FLOAT128
	 ncase RESERVED__Float128:
		 b->basetype = B_FLOAT128;
#endif
	ncase RESERVED_void:
		 b->basetype = B_VOID;
		 b->shortname [i++] = RESERVED_void;
	ncase RESERVED_double:
		if (lo) {
		 b->basetype = B_LDOUBLE;
		 b->shortname [i++] = RESERVED_long;
		 b->shortname [i++] = RESERVED_double;
		} else {
		 b->basetype = B_DOUBLE;
		 b->shortname [i++] = RESERVED_double;
		}
	ncase RESERVED_char:
		if (us) {
		 b->basetype = B_UCHAR;
		 b->shortname [i++] = RESERVED_unsigned;
		} else
		 b->basetype = B_SCHAR;
		b->shortname [i++] = RESERVED_char;
	ncase RESERVED_int:
		if (lo) {
		 if (us) {
		  b->basetype = lo > 1 ? B_ULLONG : B_ULONG;
		  b->shortname [i++] = RESERVED_unsigned;
		 } else
		  b->basetype = lo > 1 ? B_SLLONG : B_SLONG;
		 b->shortname [i++] = RESERVED_long;
		 if (lo > 1)
		  b->shortname [i++] = RESERVED_long;
		} else if (sh) {
		 if (us) {
		  b->basetype = B_USINT;
		  b->shortname [i++] = RESERVED_unsigned;
		 } else
		  b->basetype = B_SSINT;
		 b->shortname [i++] = RESERVED_short;
		 b->shortname [i++] = RESERVED_int;
		} else {
		 if (us) {
		  b->basetype = B_UINT;
		  b->shortname [i++] = RESERVED_unsigned;
		 } else
		  b->basetype = B_SINT;
		 b->shortname [i++] = RESERVED_int;
		}
	ndefault:
		parse_error (p, "BUG");
	}
	b->shortname [i] = -1;
	return p;
}

static recID is_object_in_global (NormPtr p)
{
	while (CODE [p] != '.' && CODE [p] != '(' && CODE [p] != ';' && CODE [p] != ',')
		p++;
	if (CODE [p] != '.') return 0;
	return lookup_struct (CODE [p - 1]);
}

static NormPtr bt_name (Dclbase *b, NormPtr p)
{
	recID r;
	Token x, y, t;
	bool iname = false;

	if ((x = CODE [p]) == RESERVED__CLASS_) {
		if ((x = name_of_struct (current_scope [top_scope])) == NOOBJ) {
			if ((r = is_object_in_global (p))) {
				x = name_of_struct (r);
				goto keep;
			} else parse_error (p, "_CLASS_ invalid here");
                }
		if (PARENT_AUTOFUNCS) keep: {
			/* Special case: _CLASS_ in the prototype of an
			   auto-function.  Leave it as _CLASS_ and later
			   at commit_auto, replace with actual type.  The
			   problem is that we don't yet know if this is
			   an aliasclass or not.  */
			b->basetype = lookup_struct (x) ?: x;
			sintprintf (b->shortname, RESERVED__CLASS_, -1);
			b->maybe_ctor = true;
			return p + 1;
		}
		if (objective.recording)
			usage_notok ();
	}

	b->btname = x;

	if (0)
            redo: iname = 1;
        switch (y = is_typename (x)) {
	case 0:
		/* Feature: template abstract classes */
		if (bt_macro && x == bt_macro) {
			x = bt_replace;
			goto renamed;
		}
		/* Feature: local typedefs in global scope */
		if (top_scope) {
			int i;
			for (i = top_scope; i; --i)
				if ((t = lookup_local_typedef (current_scope [i], x))) {
					x = t;
					goto redo;
				}
		}
		if ((r = is_object_in_global (p)))
			if ((x = lookup_local_typedef (r, x)))
				goto redo;
		/* - - - - - - - - - - - - - - - - - - - - */
		if (is_extern_templ_func (&p)) {
			b->basetype = BASETYPE_SKIP;
			return p;
		}
		/* ////////////////////////////////// */
		parse_error_pt (p, x, "Not good for a base type");
	ncase 1:;
		typeID tt = b->basetype2 = lookup_typedef (x);
		b->basetype = -1;
		if (iname && open_typeID (tt)[1] == -1)
			name_of_simple_type (b->shortname, tt);
		else {
			b->shortname [0] = x;
			b->shortname [1] = -1;
		}
	ncase 2:
	renamed:
		/* Feature: structure.constructor not a base type */
		if (CODE [p + 1] == '.') {
			Token t = CODE [p + 2];
			if (t == RESERVED_ctor
			|| t == RESERVED_dtor || t == CODE [p]
			|| (t == '~' && in2 (CODE [p + 3], CODE [p], RESERVED_dtor))) {
				b->basetype = B_VOID;
				b->shortname [0] = -1;
				return p;
			}
		}
		/* ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ */
		b->basetype = lookup_struct (x) ?: x;
		if (lookup_struct (x) && is_aliasclass (b->basetype))
			sintprintf (b->shortname, x, -1);
		else	sintprintf (b->shortname, RESERVED_struct, x, -1);
		b->maybe_ctor = true;
	ncase 3:
		b->basetype = B_SINT;
		b->shortname [0] = RESERVED_enum;
		b->shortname [1] = x;
		b->shortname [2] = -1;
	ndefault:
		/* local typedef redirection */
		/* XXXX: could we loop ??? */
		x = y;
		goto redo;
	}
	return p + 1;
}

/* Feature: do typeof */
static NormPtr bt_typeof (Dclbase *b, NormPtr p)
{
	exprret E;

	if (CODE [p++] != '(')
		parse_error (p, "typeof '('");
	p = parse_expression (p, &E, NORMAL_EXPR);
	if (!E.ok) parse_error (p, "compilation halted due to expression error");
	if (CODE [p++] != ')')
		parse_error (p, "typeof (...')'");

	sintprintf (b->shortname, RESERVED___typeof__, '(', ISTR (E.newExpr), ')', -1);
	free (E.newExpr);
	b->basetype = -1;
	b->basetype2 = E.type;

	return p;
}
/* ~~~~~~~~~~~~~~~~~ */

static NormPtr enum_dcl (Dclbase *b, NormPtr p, bool benum, enumID e)
{
	int i = 0, j = 0;
	Token ev [2048];

	ev [j++] = b->shortname [1];
	outprintf (GLOBAL, ISTR (b->shortname), '{', -1);
	for (;; p++) {
		if (CODE [p] == '}') break;
		if (!ISSYMBOL (CODE [p]))
			parse_error (p, "enum noise internal");
		enter_enumconst (ev [j++] = CODE [p], e);
		output_itoken (GLOBAL, CODE [p++]);
		if (benum)
			outprintf (GLOBAL, '=', binshift [31 & i++], -1);
		else if (CODE [p] == '=') {
			exprret E;
			p = parse_const_expression (1 + p, &E);
			if (E.ok) {
				outprintf (GLOBAL, '=', ISTR (E.newExpr), - 1);
				free (E.newExpr);
			}
		}

		if (CODE [p] == ',') {
			output_itoken (GLOBAL, ',');
			continue;
		}

		if (CODE [p] == '}')
			break;
		parse_error_ll ("error in enum");
	}
	outprintf (GLOBAL, '}', ';', -1);
	ev [j++] = -1;
	enter_enum_syms (e, ev, j);
	return p + 1;
}

static NormPtr bt_enum (Dclbase *b, NormPtr p, bool benum)
{
	enumID e;

	b->basetype = B_SINT;
	b->shortname [0] = RESERVED_enum;
	b->shortname [2] = -1;
	if (CODE [p] == '{') {
		e = enter_enum (b->shortname [1] = name_anonymous_enum ());
		return enum_dcl (b, p + 1, benum, e);
	}
	if (!ISSYMBOL (CODE [p]))
		parse_error_ll ("erroneous enum declaration");
	e = enter_enum (b->shortname [1] = CODE [p++]);
	if (CODE [p] == '{')
		return enum_dcl (b, p + 1, benum, e);
	return p;
}

static NormPtr parse_structure (Token, Dclbase*, NormPtr, Token, Token*);
static NormPtr parse_inheritance (NormPtr, recID, Token[]);

static NormPtr bt_struct (Dclbase *b, NormPtr p, Token struct_or_union)
{
	Token abstract_parents [64] = { -1, };
	Token tag = struct_or_union == RESERVED_union ? RESERVED_union : RESERVED_struct;

	bool anon = CODE [p] == '{' || CODE [p] == ':';

	if (!anon && !ISSYMBOL (CODE [p]))
		parse_error_ll ("Das is verbotten");
	Token name = anon ? name_anonymous_struct () : CODE [p++];

	if (anon) b->anon_union = true;

	bool need_fwd = !lookup_struct (name);

	if (have_abstract (name))
		parse_error_tok (name, "already have abstract class named that");

	b->basetype = enter_struct (name, struct_or_union, b->is__unwind__, b->is__noctor__, 
				    b->is__emit_vtbl__, b->isstatic, b->is__byvalue__);

	/* Feature: Inheritance */
	if (CODE [p] == ':')
		p = parse_inheritance (p + 1, b->basetype, abstract_parents);
	/* # # # # # # # # # # #*/

	if (CODE [p] == '{') {
		p = parse_structure (name, b, p + 1, tag, abstract_parents);

		if (is_dcl_start (CODE [p]) && CODE [p] != name)
			warning_tok ("(?) Forgot semicolon after the"
				     " definition of class", name);
	}

	sintprintf (b->shortname, is_aliasclass (b->basetype) ? BLANKT : tag,
		    name, -1);

	if (need_fwd && !is_aliasclass (b->basetype))
		outprintf (GLOBAL, ISTR (b->shortname), ';', -1);

	return p;
}

static Token specialized (Token, NormPtr);

/* Feature: Unique Specialization for template class */
static NormPtr bt_specialize (Dclbase *b, NormPtr p)
{
	Token abstract_parents [64] = { -1, };
	Token c = CODE [p++], sp;

	if (!ISSYMBOL (c) || !have_abstract (c) || CODE [p] != '{')
		parse_error (p, "No such abstract class");

	/* * * * * * * * * * * */
	sp = specialized (c, ++p);
	/* * * * * * * * * * * */

	b->basetype = enter_struct (sp, true, b->is__unwind__, 0, 0, 0, 0);
	parse_inheritance (p - 2, b->basetype, abstract_parents);

	if (abstract_has_special (c, b->basetype))
		p = skip_braces (p);
	else {
		Token abp [] = { c, -1 };
		outprintf (GLOBAL, iRESERVED_struct (b->basetype), sp, ';', -1);
		SAVE_VARC (b->isstatic, 0, s);
		p = parse_structure (sp, b, p, RESERVED_struct, abp);
		RESTOR_VARC (b->isstatic, s);
	}
	sintprintf (b->shortname, iRESERVED_struct (b->basetype), sp, -1);

	return p;
}

/* Feature: Base type abstract class */
static NormPtr bt_abstract (Dclbase *b, NormPtr p)
{
	Token an, par [32], c;
	recID rpar [32];
	int pc = 0, rc = 0;
	NormPtr p1;

	if (!ISSYMBOL (CODE [p]))
		parse_error (p, "anonymous abstract not possible");

	an = CODE [p++];
	if (CODE [p] == ':') {
		++p;
		do if ((c = CODE [p++]) == RESERVED_virtual)
			if (!issymbol (c = CODE [p++]) || !lookup_object (c))
				goto err;
			else rpar [rc++] = lookup_object (c) + VIRTUALPAR_BOOST;
		   else if (!issymbol (c)) err:
			parse_error (p, "Symbol expected");
		   else if (lookup_object (c))
			rpar [rc++] = lookup_object (c);
		   else if (have_abstract (c))
			par [pc++] = c;
		   else parse_error_tok (c, "No such class/template-class defined");
		while (CODE [p++] == ',');
		--p;
	}
	par [pc] = rpar [rc] = -1;
	if (CODE [p++] != '{')
		parse_error (p, "template class ... '{'");
	p = skip_braces (p1 = p);
	enter_abstract (an, par, rpar, p1);
	b->basetype = BASETYPE_ABSTRACT;
	return p;
}
/* ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ */

/* Feature: RegExp as a basetype. send to regexp.c */
static NormPtr bt_regexp (Dclbase *b, NormPtr p)
{
	b->basetype = BASETYPE_SKIP;
	return parse_RegExp (p, (b->isstatic!=0) + 2 * (b->isextern!=0) + 4 * (b->isinline!=0));
}

static NormPtr parse_basetype (Dclbase *b, NormPtr p)
{
	b->section = 0;
	b->dclqual_i = 0;
	b->anon_union = false;
	b->maybe_ctor = false;
	b->btname = 0;
	p = collect_dclqual (b, p, 1);
	sumup_dclqual (b);

	switch (CODE [p]) {
	case RESERVED_ctor: b->basetype = B_VOID, b->shortname [0] = -1;
	ncase RESERVED_dtor: b->basetype = typeID_voidP, b->shortname [0] = -1;
	ncase RESERVED_enum: p = bt_enum (b, p + 1, false);
	ncase RESERVED_benum: p = bt_enum (b, p + 1, true);
	ncase RESERVED_struct:
	  case RESERVED_class: 
	  case RESERVED_union: p = bt_struct (b, p + 1, CODE [p]);
	ncase RESERVED_template: p = bt_abstract (b, p + 2);
	ncase RESERVED_specialize: p = bt_specialize (b, p + 1);
	ncase RESERVED_typeof: p = bt_typeof (b, p + 1);
	ncase RESERVED_RegExp: p = bt_regexp (b, p + 1);
	ndefault:
		if (ISTBASETYPE (CODE [p]))
			p = bt_builtin (b, p);
		else if (ISSYMBOL (CODE [p]))
			/* Feature: constructor with class name */
			if (in2 (CODE [p], b->class_name, bt_macro)
			&& CODE [p + 1] == '(') {
				b->basetype = B_VOID;
				b->shortname [0] = -1;
				CODE [p] = RESERVED_ctor;
			/* ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ */
			} else p = bt_name (b, p);
		else if (CODE [p] == '~' && in3 (CODE [p + 1], RESERVED_dtor, b->class_name,
				 bt_macro)) {
		     /* Feature: ~obj  is a dtor */
			b->basetype = typeID_voidP;
			b->shortname [0] = -1;
			CODE [p + 1] = RESERVED_dtor;
		} else parse_error_pt (p, CODE [p], "Bad basetype");
	}

	if (b->basetype == BASETYPE_SKIP) return p;

	p = collect_dclqual (b, p, 0);
	sumup_dclqual (b);
	p = collect_dclqual (b, p, 0);
	return p;
}

static void ctor_basetype (Dclbase *b, recID r)
{
	if (b->shortname [0] != -1)
		parse_error_ll ("Constructor may not have return type");
	b->shortname [0] = RESERVED_void;
	b->shortname [1] = -1;
	b->basetype = B_VOID;
}

static void dtor_basetype (Dclbase *b, recID r)
{
	if (b->shortname [0] != -1)
		parse_error_ll ("Constructor may not have return type");
	sintprintf (b->shortname, RESERVED_void, '*', -1);
	b->basetype = typeID_voidP;
}

typedef struct {
	Token cname;
	recID rec;
	int dclhype [128];
	Token dclstr [512];
	int dclhi, ip;
	NormPtr objp;
	int argc;
	Token argv [32];
	NormPtr argp [32];
	Token *dflt_arge [32];
	int dflt_argc;
	bool reference;
	bool constant;
	int ctordtor;
	NormPtr begins;
	bool have_this;
	NormPtr VLA_expr;
	// these are drivers for dirdcl()
	bool const_in_base;
	bool maybe_ctor;
	bool allowed_va;
} Dclobj;

#define DCLOBJ_INIT { .const_in_base = 0, .maybe_ctor = 0, .allowed_va = 0 }

/* add 'this' keyword in function prototype */
static void add_this (Dclobj *d, bool const_this)
{
	int i;
	Token *tmp = allocaint (intlen (d->dclstr));

	/* alter the output declaration */
	if (d->dclstr [d->objp + 1] != '(') parse_error_ll ("BUG 234");
	i = d->objp + 2;
	intcpy (tmp, d->dclstr + i);
	if (const_this) d->dclstr [i++] = RESERVED_const;
	d->dclstr [i++] = RESERVED_struct;
	d->dclstr [i++] = name_of_struct (d->rec);
	d->dclstr [i++] = '*';
	d->dclstr [i++] = RESERVED_const;
	d->dclstr [i++] = RESERVED_this;
	if (tmp [0] != ')')
		d->dclstr [i++] = ',';
	intcpy (d->dclstr + i, tmp);
	d->ip = intlen (d->dclstr);
	intcpy (tmp, d->argv);
	d->have_this = true;
}


static typeID dcltype (Dclbase *b, Dclobj *d)
{
	int typet [64];
	if (b->basetype == -1) {
		int *td = open_typeID (b->basetype2);
		typet [0] = td [0];
		intcpy (typet + 1, d->dclhype);
		intcat (typet, td + 1);
	} else {
		typet [0] = b->basetype;
		typet [1] = -1;
		if (d) intcat (typet, d->dclhype);
	}
	if (d->reference)
		typet [0] += REFERENCE_BOOST;
	return enter_type (typet);
}

static NormPtr dirdcl (Dclobj *d, NormPtr p, int inargs);

static NormPtr dcl (Dclobj *d, NormPtr p, int inargs)
{
	int ns = 0, ps;

	/* Feature: references */
	if (!d->ip && (d->reference = CODE [p] == '&'))
		++ns, ++p, d->dclstr [d->ip++] = '*';
	/* &-&-&-&-&-&-&-&-&-& */

	for (;; d->dclstr [d->ip++] = CODE [p++])
		if (CODE [p] == '*') ns++;
		else if (CODE [p] == RESERVED___attribute__) {
			ps = skip_parenthesis (p + 2) - 1;
			while (p < ps) d->dclstr [d->ip++] = CODE [p++];
		} else if (CODE [p] != RESERVED_const
			&& CODE [p] != RESERVED_volatile
			&& CODE [p] != RESERVED___restrict) break;
	p = dirdcl (d, ps = p, inargs);
	while (ns--) d->dclhype [d->dclhi++] = '*';
	d->constant = CODE [ps - 1] == RESERVED_const && p - ps == 1;
	return p;
}

static NormPtr arglist (Dclobj*, NormPtr);

static int count_argc (NormPtr p)
{
	if (CODE [p] == ')') return 0;
	int c = 1;
	while (CODE [p] != ')')
		switch (CODE [p++]) {
		 case '(': p = skip_parenthesis (p);
		ncase ',': ++c;
		}
	return c;
}

static NormPtr operator_overload (NormPtr p, int memb)
{
	if (in2 (CODE [p], RESERVED_new, RESERVED_delete)) {
		if (CODE [p + 1] != '(' || CODE [p + 2] != ')')
			parse_error (p, "the syntax is 'operator {new|delete} ();'");
		return p;
	}

	bool postfix = CODE [p] == RESERVED_postfix;

	if (postfix) ++p;
	Token op = CODE [p];
	if (op == '[' || (op == '(' && in2 (CODE [p + 1], ')', RESERVED_oper_fcall))) ++p;

	/*** kludgy: "operator +" is replaced by "operator operfunc_name" in
	 *** the CODE[] so next time we return early.  A premature optimization!
	 ***/
	if (!isoperator (op)) {
		if (isoperfunc (op))
			return p;
		else parse_error_pt (p, op, "not an operator");
        }
	if (CODE [p + 1] != '(')
		parse_error (p, "operator overload is a function");
	memb += count_argc (p + 2);
	CODE [p] = op == '(' ? RESERVED_oper_fcall :
		   memb == 1 ? postfix ? isunary_postfix_overloadable (op) :
		   isunary_overloadable (op) : memb == 2 ? isbinary_overloadable (op) : 0;
	if (!CODE [p]) parse_error (p, "Can't overload that.");
	return p;
}

static NormPtr dirdcl (Dclobj *d, NormPtr p, int inargs)
{
	if (CODE [p] == '(' && (d->maybe_ctor ? !is_expression (p) : 1)) {
		d->dclstr [d->ip++] = '(';
		p = dcl (d, p + 1, inargs);
		d->dclstr [d->ip++] = CODE [p];
		if (CODE [p++] != ')')
			parse_error (p, "Missing parenthesis in declaration");
	} else if (ISSYMBOL (CODE [p])) {
		/* Feature: member functions in global */
		if (CODE [p + 1] == '.') {
			Token bn, n, bm;
			NormPtr ppp = p;
			bn = bm = CODE [p];
			p += 2;
			if (bn == bt_macro) bn = bt_replace;
			/* Feature: ctor with class name */
			if (CODE [p] == bm)
				CODE [p] = RESERVED_ctor;
			else if (CODE [p] == '~' && in2 (CODE [p + 1], bm, RESERVED_dtor))
				CODE [++p] = RESERVED_dtor;
			else if (CODE [p] == RESERVED_operator)
				p = operator_overload (p + 1, 1);
			else if (!ISSYMBOL (CODE [p]))
				parse_error (p, "Missing member");
			d->cname = CODE [p];
			d->ctordtor = CODE [p] == RESERVED_ctor ? 1 :
					CODE [p] == RESERVED_dtor ? 2 : 0;
			d->rec = lookup_object (bn);
			if (!d->rec) {
				/* Feature: out-of-class template func */
				if (!is_template_function (&ppp, d->begins))
					parse_error_pt (p, bn, "No such class");
				d->ctordtor = BASETYPE_SKIP;
				return ppp;
			}
			d->dclstr [d->objp = d->ip++] = 
			n = name_member_function (d->rec, CODE [p]);
			if (in2 (CODE [p], RESERVED_new, RESERVED_delete))
				enter_newdel_overload (CODE [p], d->rec, n);
			++p;
		} else
		/* ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ */
			d->dclstr [d->objp = d->ip++] = CODE [p++];
	} else if (CODE [p] == RESERVED_this)
		d->dclstr [d->objp = d->ip++] = CODE [p++];
	else if (CODE [p] == RESERVED_operator) {
		/* Feature: operator overloading */
		p = operator_overload (p + 1, top_scope > 0);
		d->dclstr [d->objp = d->ip++] = CODE [p++];
		/* *+*+*+*+*+*+*+*+*+*+*+*+*+*+* */
	} else if (CODE [p] == '~' && CODE [p + 1] == RESERVED_dtor) {
		d->dclstr [d->objp = d->ip++] = RESERVED_dtor;
		p += 2;
	} else d->dclstr [d->objp = d->ip++] = BLANKT;

	for (;;) {
		switch (CODE [p]) {
		case '(':
			if (d->rec) PUSH_CURRENT_SCOPE(d->rec);
			/* Constructor: stop at constructor initialization */
			if (!is_dcl_start (CODE [p+1]) && CODE [p+1] != ')' && CODE [p+1] != '.'){
				if (d->rec) POP_CURRENT_SCOPE;
				return p;
			}
			/* + + + + + + + + + + + + + + + + + + + + + + + +*/
			d->dclstr [d->ip++] = '(';
			p = arglist (d, p + 1);
			if (d->rec) POP_CURRENT_SCOPE;
			continue;
		case '[':
			/* Feature: elliptic varargs */
			if (CODE [p + 1] == ELLIPSIS && CODE [p + 2] == ']') {
				sintprintf (&d->dclstr [d->ip], '[', ']', ',', RESERVED_int, 0, -1);
				d->ip = intlen (d->dclstr);
				d->dclhype [d->dclhi++] = B_ELLIPSIS;
				return p + 3;
			}
			/* ......................... */
			d->dclstr [d->ip++] = '[';
			d->dclhype [d->dclhi] = d->dclhi || inargs ? '*' : '[';
			d->dclhi++;
			if (CODE [++p] == ']') p += 1;
			else if (CODE [p] == RESERVED___restrict) {
				/* int foo (char a[__restrict]); is allowed */
				d->dclstr [d->ip++] = RESERVED___restrict;
				p += 2;
			} else {
				exprret E;
				NormPtr p0 = p;

				p = parse_expression (p, &E, NORMAL_EXPR) + 1;
				if (E.ok && !E.isconstant) {
					/* Feature: store VLA info */
					if (d->dclhi > 1)
						parse_error (p, "non-constant [expression]");
					d->VLA_expr = p0;
					d->dclhype [0] = '*';
					d->dclstr [d->ip - 1] = d->dclstr [d->ip - 2];
					d->dclstr [d->ip - 2] = '*';
					++d->objp;
					free (E.newExpr);
					continue;
					/*  *  *  *  *  *  *  *  *  */
				}
				if (E.ok) {
					NormPtr ps;
					for (ps = 0; E.newExpr [ps] != -1;)
						d->dclstr [d->ip++] = E.newExpr [ps++];
					free (E.newExpr);
				}
			}
			d->dclstr [d->ip++] = ']';
			continue;
		}
		break;
	}
	return p;
}

static NormPtr append_attributes (Dclobj *d, NormPtr p)
{
	while (CODE [p] == RESERVED___attribute__ || CODE [p] == RESERVED___asm__) {
		if (CODE [p + 1] != '(') parse_error (p, "__attribute__ '('");
		NormPtr p2 = skip_parenthesis (p + 2);
		while (p < p2)
			d->dclstr [d->ip++] = CODE [p++];
	}
	return p;
}

static NormPtr parse_dcl (Dclobj *d, NormPtr p, bool inargs)
{
	d->have_this = 0;
	d->argc = -1;
	d->rec = 0, d->objp = -1;
	d->dclhi = d->ip = 0;
	d->cname = -1;
	d->ctordtor = 0;
	d->argv [0] = -1;
	d->VLA_expr = 0;
	p = dcl (d, p, inargs);
	p = append_attributes (d, p);
	if (!d->constant && !d->dclhi)
		d->constant = d->const_in_base;
	d->dclhype [d->dclhi] = -1;
	if (!d->allowed_va && d->dclhype [d->dclhi - 1] == ELLIPSIS)
		parse_error (p, "[...] not allowed here");
	d->dclstr [d->ip] = -1;
	return p;
}

static NormPtr arglist (Dclobj *d, NormPtr p)
{
	int i, nb = 0;
	bool elliptic;
	typeID t;
	Dclbase b;
	Dclobj di = DCLOBJ_INIT;

	b.class_name = 0;
	d->dclhype [d->dclhi++] = '(';
	d->dflt_argc = 0;

	di.allowed_va = true;

	if (CODE [p] == RESERVED_void && CODE [p + 1] == ')') {
		++p;
	} else while (CODE [p] != ')') {
		if (CODE [p] == ELLIPSIS) {
			d->dclstr [d->ip++] = ELLIPSIS;
			d->dclhype [d->dclhi++] = B_ELLIPSIS;
			if (CODE [++p] != ')')
				parse_error (p, "'...' must be last");
			break;
		}
		p = parse_dcl (&di, parse_basetype (&b, p), 1);
		if (b.basetype == BASETYPE_ABSTRACT)
			parse_error (p, "abstract is not a real base type");

		t = d->dclhype [d->dclhi++] = dcltype (&b, &di);

		/* Feature: elliptic varargs */
		elliptic = typeID_elliptic (t);
		if (elliptic && CODE [p] != ')')
			parse_error (p, "[...] must be last");
		/* ......................... */

		for (i = 0; b.dclqual [i] != -1; i++)
			d->dclstr [d->ip++] = b.dclqual [i];
		for (i = 0; b.shortname [i] != -1; i++)
			d->dclstr [d->ip++] = b.shortname [i];

		/* Feature: structure by reference */
		if (isstructure (t) && by_ref (t)) {
			d->dclstr [d->ip++] = '*';
			d->dclstr [d->ip++] = RESERVED_const;
		}
		/* ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ */

		if (di.dclstr [di.objp] == BLANKT)
			di.dclstr [di.objp] = internal_identifiern (nb++);

		if (elliptic) {
			Token n = di.dclstr [di.objp];
			di.dclstr [di.objp] = token_addchar (n, 'v');
			di.dclstr [intlen (di.dclstr) - 1] = token_addchar (n, 'c');
			d->dclhype [d->dclhi++] = typeID_int;
		}

		for (i = 0; di.dclstr [i] != -1; i++)
			d->dclstr [d->ip++] = di.dclstr [i];

		d->argv [++d->argc] = di.dclstr [di.objp];
		if (elliptic) d->argv [++d->argc] = di.dclstr [intlen (di.dclstr) - 1];

		/* Feature: default function arguments */
		if (CODE [p] == '=') {
			if (elliptic) parse_error (p, "Doh! default varargs!");
			NormPtr p2;
			p = skip_expression (CODE, p2 = p + 1, INIT_EXPR);
			d->dflt_arge [d->dflt_argc++] = intndup (CODE + p2, p - p2);
		} else if (d->dflt_argc)
			parse_error (p, "Default arguments ought to be last");
		/* ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ */

		if (CODE [p] == ',') {
			d->dclstr [d->ip++] = ',';
			p++;
		}
	}
	d->dflt_arge [d->dflt_argc] = 0;
	d->dclstr [d->ip++] = ')';
	d->dclhype [d->dclhi++] = INTERNAL_ARGEND;
	d->argv [++d->argc] = -1;
	return p + 1;
}

static inline NormPtr parse_bitfield (Dclobj *d, NormPtr p)
{
	if (CODE [p] == ':') {
		d->dclstr [d->ip++] = ':';
		exprret E;
		p = parse_const_expression (++p, &E);
		if (E.ok) {
			NormPtr ps;
			for (ps = 0; E.newExpr [ps] != -1;)
				d->dclstr [d->ip++] = E.newExpr [ps++];
			free (E.newExpr);
		}
		d->dclstr [d->ip] = -1;
	}
	return p;
}

bool is_array_of_ctorable_objects (typeID t)
{
	int *tt = open_typeID (t);
	return tt [1] == '[' && tt [2] == -1 && tt [0] >= 0 && need_construction (tt [0]);
}

bool is_array_of_objects (typeID t)
{
	int *tt = open_typeID (t);
	return tt [1] == '[' && tt [2] == -1 && tt [0] >= 0;
}

/* mark the position of a function definition to be parsed later */
static void mark_function_def
	 (Dclbase *b, Dclobj *d, NormPtr p, recID r, typeID rett, Token cname, bool dto)
{
	if (Streams_Closed)
		return;

	Token *tmp = allocaint (intlen (b->dclqual) +
			intlen (b->shortname) + intlen (d->dclstr) + 3);
	intcpy (tmp, b->dclqual);
	intcat (tmp, b->shortname);
	intcat (tmp, d->dclstr);
	(CODE [p - 1] == RESERVED_alias ? store_definition_alias : store_definition)
			(d->dclstr [d->objp], tmp, d->argv, d->dclhype + 1,
			  p, r, rett, cname, dto, b->isauto ? DT_AUTO : DT_NORM,
			 !b->ismodular);
}

static void add_dclqual (Token q, Token *ts)
{
	int i;
	for (i = 0; ts [i] != -1; i++)
		if (ts [i] == q) return;
	ts [i++] = q;
	ts [i] = -1;
}

static Token pdcl_function (Dclbase *b, Dclobj *d, typeID t, int fflagz)
{
	Token f = d->dclstr [d->objp];
	Token **dflt_arg;
	Token *tmp = allocaint (intlen (b->dclqual) +
			intlen (b->shortname) + intlen (d->dclstr) + 3);
	Token *tmpargv = d->argv;

	if (d->cname == RESERVED_oper_pointsat
	&& (!isstructptr (funcreturn (t)) || isreference (funcreturn (t))))
		parse_error_ll ("overloaded operator -> must return pointer to struct");
	intcpy (tmp, b->dclqual);
	intcat (tmp, b->shortname);
	intcat (tmp, d->dclstr);
	if (d->dflt_argc == 0) dflt_arg = 0;
	else {
		int i = d->dflt_argc;
		dflt_arg = (Token**) malloc ((i + 1) * sizeof (Token*));
		memcpy (dflt_arg, d->dflt_arge, i * sizeof (Token*));
		dflt_arg [i] = 0;
	}
	if (d->have_this) {
		tmpargv = allocaint (intlen (d->argv) + 2);
		tmpargv [0] = RESERVED_this;
		intcpy (tmpargv + 1, d->argv);
	}
	return d->rec ?
	 declare_function_member (d->rec, d->cname, t, tmp, tmpargv, fflagz, dflt_arg, b->section) :
	 xdeclare_function (&Global, f, f, t, tmp, tmpargv, fflagz, dflt_arg, b->section)->name;
}

static NormPtr virtual_class_fwd (recID r, OUTSTREAM D, NormPtr p)
{
	for (;;) {
		if (!ISSYMBOL (CODE [p]))
			parse_error (p, "name expected");
		virtual_inheritance_decl (D, r, CODE [p++]);
		if (CODE [p] == ';') return p + 1;
		if (CODE [p++] != ',')
			parse_error (p, "separator");
	}	
}

/* * * * * * Get the name of a class based on the local typedefs * * * * * */
static void mk_specialized (char nm[], NormPtr p, Token have[], bool anydcl)
{
	char tmp [64];
	int i;
	while (CODE [p] != '}' && CODE [p] != -1) {
		if (CODE [p] == RESERVED_const
		&& ISSYMBOL (CODE [p + 1]) && CODE [p + 3] == ';') {
			p = skip_declaration (p);
			continue;
		}

		if (CODE [p] != RESERVED_typedef) {
			if (anydcl) {
				p = skip_declaration (p);
				continue;
			} else parse_error (p, "specialized object may not have"
						" declarations other than typedefs");
                }
		Dclbase b = { .dclqual_i = 0, .section = 0, .class_name = -1 };
		Dclobj d = DCLOBJ_INIT;
		typeID dt;

		p = parse_basetype (&b, last_location = p);
		d.const_in_base = b.isconst;

		if (b.basetype == BASETYPE_ABSTRACT)
			return;

		for (;;) {
			p = parse_dcl (&d, p, 0);

			if (d.dclstr [d.objp] == BLANKT)
				parse_error (p, "Abstract typedef declaration in structure?");

			dt = dcltype (&b, &d);

			for (i = 0; have [i] != -1; i++)
				if (have [i] == d.dclstr [d.objp])
					goto do_not;

			have [i++] = d.dclstr [d.objp];
			have [i] = -1;
			strcat (strcat (strcat (nm, "_"),
				expand (d.dclstr [d.objp])), nametype (tmp, dt));

		do_not:
			if (CODE [p] == ';') break;
			if (CODE [p++] != ',')
				parse_error_pt (p, CODE [p-1], "parse error in declaration separator(3)");
		}
		++p;
	}
}

static void specialize_do (char nm[], Token a, Token have[])
{
	SAVE_VAR (bt_macro, a);
	SAVE_VAR (bt_replace, a);
	Token *ab_par = parents_of_abstract (a);
	mk_specialized (nm, dcl_of_abstract (a), have, true);
	while (*ab_par != -1)
		specialize_do (nm, *ab_par++, have);
	RESTOR_VAR (bt_macro);
	RESTOR_VAR (bt_replace);
}

static Token specialized (Token c, NormPtr p)
{
	char name [1024];
	Token have [64] = { -1, };

	sprintf (name, "%s_SpEc_", expand (c));
	mk_specialized (name, p, have, false);
	specialize_do (name, c, have);
	return new_symbol (strdup (name));
}
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
#define INHERIT_FLAG(x, y) b.x |= flagz & FUNCP_ ## y; if (b.x) flagz |= FUNCP_ ## y

NormPtr struct_declaration (recID r, OUTSTREAM D, NormPtr p)
{
	NormPtr ps, dstart;
	Dclbase b;
	Dclobj d = DCLOBJ_INIT;
	typeID dt;
	bool isdtor;

	/* Feature: skip local typedefs */
	if (CODE [p] == RESERVED_typedef)
		return skip_declaration (p);
	/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

	b.dclqual_i = 0;
	b.section = 0;
	last_location = p;

	/* Decorative feature: private && public disappear */
	if ((CODE [p] == RESERVED_public || CODE [p] == RESERVED_private)
	&& CODE [p + 1] == ':')
		return p + 2;
	/* ::::::::::::::::::::::::::::::::::::::::::::::: */

	if (CODE [p] == ';') return p + 1;

	/* Feature: force creation of virtual table at object */
	if (CODE [ps = collect_dclqual (&b, p, 0)] == ';') {
		sumup_dclqual (&b);
		if (!b.isvirtual)
			parse_error (p, "bad specifications");
		Here_virtualtable (D, r, b.isinline, b.isconst, b.isvolatile);
		return ps + 1;
	}
	/* /////////////////////////////////////////////// */

	/* Feature: "virtual class" */
	if (CODE [p] == RESERVED_virtual && CODE [p + 1] == RESERVED_class)
		return virtual_class_fwd (r, D, p + 2);
	/* //////////////////////// */

	/* Feature: class const macros */
	if (CODE [p] == RESERVED_const && ISSYMBOL (CODE [p + 1]) && !is_typename (CODE [p + 1])) {
		enter_class_const (r, CODE [p + 1], CODE [p + 2]);
		if (CODE [p + 3] != ';')
			parse_error (p, "const X Y ';'");
		return p + 4;
	}
	/* ////////////////////////// */

	b.class_name = name_of_struct (r);
	p = parse_basetype (&b, dstart = p);
	d.maybe_ctor = b.maybe_ctor;
	d.const_in_base = b.isconst;

	if (b.basetype == BASETYPE_ABSTRACT) {
		if (CODE [p] == ';') return p + 1;
		else parse_error (p, "It is not possible to have objects of abstract classes");
        }
	if (b.basetype == BASETYPE_SKIP)
		return p;

	for (;;) {
		if (CODE [p] == ':') {
			d.ip = 0;
			p = parse_bitfield (&d, p);
			outprintf (D, ISTR (b.dclqual), ISTR (b.shortname),
				   ISTR (d.dclstr), ';',  -1);
			goto skippy;
		}

		p = parse_bitfield (&d, parse_dcl (&d, ps = p, 0));

		if (d.dclstr [d.objp] == BLANKT) {
			/* Feature: anonymous transparent unions */
			if (b.anon_union && d.dclstr [1] == -1) {
				d.dclstr [d.objp] = add_anonymous_union (r, b.basetype);
				goto print_member;
			}
			/* ..................................... */
			parse_error_pt (p, name_of_struct (r),"Abstract declaration in structure?");
		}

		isdtor = false;

		/* Constructors: add return type to ctor function */
		if (d.dclstr [d.objp] == RESERVED_ctor) {
			ctor_basetype (&b, r);
			if (d.dclhype [0] != '(')
				parse_error (p, "Constructor is a function");
		}
		/* +^+^+^+^+^+^+^+^+^+^+^+^+^+^+^+^+^+^+^+^+^+^+ */
		if ((isdtor = d.dclstr [d.objp] == RESERVED_dtor)) {
			dtor_basetype (&b, r);
			if (d.dclhype [0] != '(')
				parse_error (p, "Destructor is a function");
			if (d.dclhype [1] != INTERNAL_ARGEND)
				parse_error (p, "Destructor may not have arguments");
		}
		/* -^-^-^-^-^-^-^-^-^-^-^-^-^-^-^-^-^-^-^-^-^-^- */

		dt = dcltype (&b, &d);
		if (isfunction (dt)) {

			int flagz = 0;

			/* Feature: class's qualifiers apply to member functions */
			if (outer_dclqual)
			append_dclqual (outer_dclqual, &b);
			/* ------------------------------- */

			/* Feature: some specifiers *after* arglist */
			bool const_fn = false, final_fn = b.isfinal;
			for (;; p++) switch (CODE [p]) {
				default: goto break2;
				ncase RESERVED_const: const_fn = true, flagz |= FUNCP_CTHIS;
				ncase RESERVED_final: final_fn = true;
				ncase RESERVED_nothrow: flagz |= FUNCP_NOTHROW;
				}
		break2:
			if (final_fn) flagz |= FUNCP_FINAL;
			/* ---------------------------------------- */

			bool pure_virtual = CODE [p] == '=' && CODE [p + 1] == RESERVED_0;

			Token fname = d.cname = d.dclstr [d.objp], mfname;

			/* Feature: operator new */
			if (fname == RESERVED_new)
				b.ismodular = true;
			/* ^^^^^^^^^^^^^^^^^^^^^ */

			/* Feature: member functions */
			d.rec = r;
			dt = makemember (dt, r);
			/* Inherited specifiers */
			flagz |= inherited_flagz (r, fname, dt);
			INHERIT_FLAG (isvirtual, VIRTUAL);
			INHERIT_FLAG (isauto, AUTO);
			INHERIT_FLAG (ismodular, MODULAR);
			if (b.isstatic) flagz |= FUNCP_STATIC;
			if (b.isinline) flagz |= FUNCP_INLINE;
			if (pure_virtual) flagz |= FUNCP_PURE;
			if (!b.ismodular)
				add_this (&d, const_fn);

			/* definitions in-class, add static inline */
			if (CODE [p] == '{') {
				if (!(issymbol (CODE [p + 1]) && lookup_object (CODE [p + 1])
				 && CODE [p + 2] == '}')) /* special syntax for autos */ {
					add_dclqual (RESERVED_static, b.dclqual);
					add_dclqual (RESERVED_inline, b.dclqual);
				}
				/*** XXX: we could put these in linkonce because they
				 *** won't go away by inlinement because they are needed
				 *** as their address is put in vtbl initialization.
				 *** otoh, they are usually small, so... let them be
				 ***/
				//if (!OneBigFile && b.isvirtual)
				//	flagz |= FUNCP_LINKONCE, b.islinkonce = 1;
			}
			/* ^^^^^^^^^^^^^^^^^^^^^^^^^ */

			mfname = d.dclstr [d.objp] = name_member_function (r, d.dclstr [d.objp]);
			d.dclstr [d.objp] = pdcl_function (&b, &d, dt, flagz);
			if (isdtor) set_dfunc (r, mfname, flagz | FUNCP_NOTHROW, b.isauto);

			/* Feature: new/delete */
			if (in2 (fname, RESERVED_new, RESERVED_delete))
				enter_newdel_overload (fname, r, d.dclstr [d.objp]);
			/* ~~~~~~~~~~~~~~~~~~~ */
				

			if (pure_virtual) p += 2;

			/* Feature: virtual nd pure virtual function */
			if (b.isvirtual) {
				last_location = p;
				Make_virtual_table (D, r, fname, dt);
			}
			/* % % % % % % % % % % % % % % % % % % % % % */

			/* Feature: auto functions */
			if (b.isauto) {
				Token proto [256];
				sintprintf (proto, ISTR (b.dclqual), ISTR (b.shortname),
						ISTR (d.dclstr), -1);
				add_auto_f (d.dclstr [d.objp], fname, r, dt, b.isvirtual,
					    pure_virtual, dstart, proto, d.argv);
			}
			/* ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ */

			 /* Feature: inline members + aliases */
			if (CODE [p] == '{' || (CODE [p] == RESERVED_alias && CODE [p + 1] == '(')){
				mark_function_def (&b, &d, ++p, r, funcreturn (dt), fname, isdtor);
				if (b.islinkonce)
					xmark_section_linkonce (FSP (r), fname, d.dclstr [d.objp]);
				return CODE [p - 1] == '{' ? skip_braces (p) :
					 skip_parenthesis (p + 1);
			} else if (!b.isauto && !b.islinkonce)
				keyfunc_candidate (r, d.dclstr [d.objp]);
			/* ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ */

		} else {

			if (d.VLA_expr)
				parse_error (p, "VLA in structure not possible");

			if (isstructure (dt))	/* has virtual table && ok */
				Can_instantiate (base_of (dt));

			/* Feature: pure data members */
			if (base_of (dt) == B_PURE) {
				add_variable_member (r, d.dclstr [d.objp], dt, 0, d.constant, 0);
				add_pure_dm (r, b.btname, dstart);
			} else
			/* Feature: Virtual variables */
			if (b.isvirtual || Is_implied_virtual_variable (r, d.dclstr [d.objp])) {
				Token *proto = allocaint (intlen (d.dclstr) + 16);
				sintprintf (proto, ISTR (b.dclqual), ISTR (b.shortname),
					    ISTR (d.dclstr), -1);
				intsubst (proto, d.dclstr [d.objp], MARKER);
				Token *expr = 0, asgn = CODE [p];
				if (in3 (CODE [p], '=', ASSIGNBO, ASSIGNBA)) {
					exprret E;
					p = parse_expression (p + 1, &E, NORMAL_EXPR);
					if (E.ok) expr = E.newExpr;
				}
				add_virtual_varmemb (r, d.dclstr [d.objp], asgn, dt,
						     expr, proto, 1, D);
			/* %%%%%%%%%%%%%%%%%%%%%%%%%% */
			} else
			/* Feature: modular data members */
			if (b.ismodular || outer_dclqual->ismodular) {
				Token rn = d.dclstr [d.objp];
				d.dclstr [d.objp] = name_member_function (r, rn);
				outprintf (GVARS, ISTR (b.dclqual), ISTR (b.shortname),
					   ISTR (d.dclstr), linkonce_data (d.dclstr [d.objp]),
					   ';', -1);
				add_variable_member (r, rn, dt, d.dclstr [d.objp], false, 0);
			/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
			} else {
				add_variable_member (r, d.dclstr [d.objp], dt, 0, d.constant,
							b.is__noctor__);
			print_member:
				outprintf (D, ISTR (b.dclqual), ISTR (b.shortname),
					   ISTR (d.dclstr), ';',  -1);
			}
		}
	skippy:
		if (CODE [p] == ';') break;
		if (CODE [p++] != ',')
			parse_error_pt (p, CODE [p-1], "parse error in declaration separator(1)");
	}

	return p + 1;
}

/* Feature: parse local typedef member */
static NormPtr struct_typedef (recID r, NormPtr p)
{
	Dclbase b;
	Dclobj d = DCLOBJ_INIT;
	typeID dt;

	if (CODE [p] != RESERVED_typedef)
		return skip_declaration (p);

	/* Feature: pure typedefs */
	if (issymbol (CODE [p + 1]) && CODE [p + 2] == '=' && CODE [p + 3] == RESERVED_0) {
		if (CODE [p + 4] != ';')
			parse_error (p, "typedef NAME = 0 ';'");
		if (!have_local_typedef (r, CODE [++p]))
			enter_typedef (add_local_typedef (r, CODE [p]), typeID_NOTYPE);
		return p + 4;
	}
	/* ^^^^^^^^^^^^^^^^^^^^^^ */

	b.dclqual_i = 0;
	b.section = 0;
	b.class_name = name_of_struct (r);
	p = parse_basetype (&b, last_location = p);
	d.const_in_base = b.isconst;

	if (b.basetype == BASETYPE_ABSTRACT) {
		if (CODE [p] == ';') return p + 1;
		else parse_error (p, "It is not possible to have objects of abstract classes");
        }
	for (;;) {

		p = parse_dcl (&d, p, 0);

		if (d.dclstr [d.objp] == BLANKT)
			parse_error (p, "Abstract typedef declaration in structure?");

		dt = dcltype (&b, &d);

		if (!have_local_typedef (r, d.dclstr [d.objp])) {
			enter_typedef (d.dclstr [d.objp] = add_local_typedef
				 (r, d.dclstr [d.objp]), dt);
			outprintf (GLOBAL, ISTR (b.dclqual), ISTR (b.shortname),
				   ISTR (d.dclstr), ';',  -1);
		}

		if (CODE [p] == ';') break;
		if (CODE [p++] != ',')
			parse_error_pt (p, CODE [p-1], "parse error in declaration separator(2)");
	}

	return p + 1;
}
/* ----------------------------- */

static void parse_all_typedefs (recID r, NormPtr p)
{
	while (CODE [p] != '}' && CODE [p] != -1)
		p = struct_typedef (r, p);
}

static NormPtr parse_all_declarations (recID r, OUTSTREAM o, NormPtr p)
{
static	recID parsing;
	if (parsing)
		set_depend (parsing, r);
	SAVE_VAR (parsing, r);

	while (CODE [p] != '}' && CODE [p] != -1)
		p = struct_declaration (r, o, p);
	if (CODE [p++] == -1)
		parse_error_ll ("missing '}' in structure declaration");

	RESTOR_VAR (parsing);
	return p;
}

static void include_abstract (Token a, recID r, OUTSTREAM o, bool typedefs)
{
	SAVE_VAR (bt_macro, a);

	if (!typedefs) enter_abstract_derrived (a, r);
	Token *ab_par = parents_of_abstract (a);
	if (typedefs) parse_all_typedefs (r, dcl_of_abstract (a));
	else parse_all_declarations (r, o, dcl_of_abstract (a));
	while (*ab_par != -1)
		include_abstract (*ab_par++, r, o, typedefs);

	RESTOR_VAR (bt_macro);
}

static void reparse_auto_functions (recID r, OUTSTREAM o)
{
	NormPtr rep [256];
	int i;

	borrow_auto_decls (r, rep);

	PUSH_CURRENT_SCOPE (r);
	SAVE_VAR (PARENT_AUTOFUNCS, true);
	for (i = 0; rep [i] != -1; i++)
		struct_declaration (r, o, rep [i]);
	RESTOR_VAR (PARENT_AUTOFUNCS);
	POP_CURRENT_SCOPE;
}

static NormPtr parse_structure (Token name, Dclbase *b, NormPtr p, Token s_or_u, Token *ab_par)
{
	recID r = b->basetype, alias;

	PUSH_CURRENT_SCOPE (r);
	SAVE_VAR (outer_dclqual, b);
	SAVE_VAR (bt_replace, name_of_struct (r));

#ifdef	DEBUG
	if (debugflag.DCL_TRACE)
		PRINTF ("Entering structure ["COLS"%s"COLE"]\n", expand (name_of_struct (r)));
#endif

	OUTSTREAM STRUCTDCL = Streams_Closed ? 0 : new_stream ();
	outprintf (STRUCTDCL, s_or_u, name, '{', -1);

	/* Feature: Inheritance */
	output_parents (STRUCTDCL, r);
	/* ^^^^^^^^^^^^^^^^^^^^ */

	/* Feature: one pass for typedefs  */
	SAVE_VAR (ab_par, ab_par);
	parse_all_typedefs (r, p);
	while (*ab_par != -1)
		include_abstract (*ab_par++, r, STRUCTDCL, 1);
	RESTOR_VAR (ab_par);
	/* + + + + + + + + + + + + + + + + */

	/* Feature: realize pure data from parents */
	gen_pure_dm (r, STRUCTDCL);
	/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

	/* Feature: expand abstract parents */
	while (*ab_par != -1)
		include_abstract (*ab_par++, r, STRUCTDCL, 0);
	/* ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ */

	p = parse_all_declarations (r, STRUCTDCL, p);

	/* Feature: reparse auto functions from parent decls */
	reparse_auto_functions (r, STRUCTDCL);
	/* ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ */

	RESTOR_VAR (outer_dclqual);
	RESTOR_VAR (bt_replace);
	POP_CURRENT_SCOPE;

	alias = complete_structure (STRUCTDCL, r);
	output_itoken (STRUCTDCL, '}');
	if (CODE [p] == RESERVED___attribute__) {
		NormPtr p2 = p++;
		if (CODE [p++] != '(') parse_error (p, "__attribute__ '('");
		p = skip_parenthesis (p);
		outprintf (STRUCTDCL, NSTRNEXT, CODE + p2, p - p2, - 1);
	}
	output_itoken (STRUCTDCL, ';');

	Token *dcl;
	if (alias) {
		/* Feature: alias classes are just typedefs */
		free_stream (STRUCTDCL);
		outprintf (GLOBAL, RESERVED_typedef,
			    RESERVED_struct, name_of_struct (alias),
			    name_of_struct (r), ';', -1);
		dcl = 0;
		/* ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ */
	} else dcl = combine_output (STRUCTDCL);

	set_declaration (r, dcl);
	return p;
}

/* * * Parse the inheritance specifications * * */
/* * * support for abstract classes * * */
static NormPtr parse_inheritance (NormPtr p, recID r, Token aparents[])
{
	bool vp;
	recID parents [32], iparents [32];
	int i = 0, j = 0, k;

	for (;;p++) {
		if ((vp = CODE [p] == RESERVED_virtual))
			++p;
		if (!ISSYMBOL (CODE [p]))
			parse_error_pt (p, CODE [p], "Expected parent class");
	
		if (have_abstract (CODE [p])) {
			real_abstract_parents (CODE [p], iparents);
			for (k = 0; iparents [k] != -1; k++)
				parents [i++] = iparents [k];
			aparents [j++] = CODE [p++];
		} else {
			parents [i] = lookup_object (CODE [p++]);
			if (!parents [i])
				parse_error_pt (p, CODE [p-1], "No such class");
			if (vp) parents [i] += VIRTUALPAR_BOOST;
			++i;
		}

		if (CODE [p] != ',') break;
	}

	if (CODE [p] != '{')
		parse_error (p, "Syntax error after parent classes");

	parents [i] = -1;
	set_parents (r, parents);
	aparents [j] = -1;
	return p;
}
/* -------------------------------------------- */

static NormPtr copy_designator (NormPtr p, Token des [])
{
	int i = 0;
	while (1) switch (CODE [p++]) {
	case '.':
		des [i++] = '.';
		des [i++] = CODE [p++];
		continue;
	case '[': {
		NormPtr p2 = p;
		des [i++] = '[';
		p = skip_expression (CODE, p, NORMAL_EXPR);
		if (CODE [p++] != ']')
			parse_error (p, "Das is verbotten");
		while (p2 < p)
			des [i++] = CODE [p2++];
		}
		continue;
	default: --p;
	case '=':
		des [i++] = '=';
		des [i] = -1;
		return p;
	}
}


static NormPtr aggregate_initializer (OUTSTREAM O, NormPtr p, typeID ret)
{
	exprret E;
	NormPtr p2;
	typeID t, t1 = isstructure (ret) ? ret : ptrdown (ret);

	output_itoken (O, '{');
	while (1) {
		if (CODE [p] == '}') {
			output_itoken (O, '}');
			return p + 1;
		}
		if (CODE [p] == '.' || CODE [p] == '[') {
			Token designator [128];
			p = copy_designator (p, designator);
			t = typeof_designator (ret, designator);
			outprintf (O, ISTR (designator), -1);
		} else t = t1;
		if (CODE [p] == '{')
			p = aggregate_initializer (O, p + 1, t);
		else {
			p = parse_expression_retconv (p2 = p, &E, t, INIT_EXPR);
			if (E.ok) {
				outprintf (O, ISTR (E.newExpr), -1);
				free (E.newExpr);
			}
		}
		if (CODE [p] == '}') {
			output_itoken (O, '}');
			return p + 1;
		}
		if (CODE [p++] != ',')
			parse_error (p, "X Verbotten X");
		output_itoken (O, ',');
	}
}

/* Feature: initialize an (no-POD) object with designators */
static NormPtr special_initializer (OUTSTREAM O, NormPtr p, typeID t, Token obj)
{
	exprret E;
	NormPtr p2;
	Token designator [256];

	while (1) {
		if (CODE [p] == '}') {
			output_itoken (O, '}');
			return p + 1;
		}
		designator [0] = obj;
		if (CODE [p] == '.' || CODE [p] == '[')
			p = copy_designator (p, designator + 1);
		else parse_error (p, "This object can only be initialized with designators!");

		p = parse_expression (p2 = p, &E, INIT_EXPR);
		if (E.ok) {
			intcat (designator, E.newExpr);
			free (E.newExpr);
			rewrite_designator (t, designator);
			outprintf (O, ISTR (designator), ';', -1);
		}
		if (CODE [p] == '}')
			return p + 1;
		if (CODE [p++] != ',')
			parse_error (p, "X Verbotten X");
	}
}

static NormPtr local_initializer (OUTSTREAM O, NormPtr p, typeID t)
{
	output_itoken (O, '=');
	if (CODE [p] == '{')
		return aggregate_initializer (O, p + 1, t);
	exprret E;
	p = parse_expression_retconv (p, &E, t, INIT_EXPR);
	if (E.ok) {
		outprintf (O, ISTR (E.newExpr), -1);
		free (E.newExpr);
	}
	return p;
}

/* Generate code for a constructor call. p points to the ctor
   arguments code. if p is -1 then it's the implict constructor
   added automatically.      */
static NormPtr call_ctor (OUTSTREAM O, Token obj, NormPtr p)
{
	Token *ctexpr;
	NormPtr ps;

	if (O == GLOBAL_INIT_FUNC) GlobInitUsed = true;

	if (p != -1)
		if (CODE [p] != '=') {
			Token ctor = CODE [p] == '(' ? RESERVED_ctor : CODE [p++];

			p = skip_parenthesis ((ps = p) + 1);
			ctexpr = allocaint (p - ps + 5);
			sintprintf (ctexpr, obj, '.', ctor, -1);
			intextract (ctexpr + 3, CODE + ps, p - ps);
		} else {
			p = skip_expression (CODE, ps = p + 1, INIT_EXPR);
			ctexpr = allocaint (p - ps + 7);
			sintprintf (ctexpr, obj, '.', RESERVED_ctor, '(', -1);
			intextract (ctexpr + 4, CODE + ps, p - ps);
			intcatc (ctexpr, ')');
		}
	else {
		ctexpr = allocaint (6);
		sintprintf (ctexpr, obj, '.', RESERVED_ctor, '(', ')', -1);
	}
	if ((ctexpr = rewrite_ctor_expr (ctexpr))) {
		outprintf (O, ISTR (ctexpr), ';', -1);
		free (ctexpr);
	}
	return p;
}

static inline bool isstructarr (typeID t)
{
	int *s = open_typeID (t);
	return base_of (t) >= 0 && in2 (s [1], '[', '*') && s [2] == -1;
}

/* Feature: call the ctor for each element of array of objects */
NormPtr gen_array_ctors (OUTSTREAM O, NormPtr p, typeID t, Token obj, int *N, bool throwing)
{
	NormPtr ps;
	int rest = *N;
	int cnt = 0;
	bool enc, dot, autosize = rest == 0;
	Token *ctexpr, cexpr [128];
	Token u1 = name_arrdto_var (obj);
	bool dstruct = has_dtor (base_of (t));
	Token dto=0, tcnt;

	if (dstruct) dto = i_arrdtor_func (base_of (t));

	if (isstructarr (t) && always_unwind (base_of (t))) throwing = true;
	throwing &= dstruct;

	if (dstruct) {
		if (EHUnwind)
			outprintf (O, RESERVED_struct, arrdtor, u1, cleanup_func (dto), '=', '{',
				   obj, ',', RESERVED_0, '}', ';', -1);
		else {
			outprintf (O, RESERVED_struct, arrdtor, u1, '=', '{', obj, ',', RESERVED_0,
				   '}', ';', -1);
			add_auto_destruction (u1, dto, throwing);
			if (throwing) push_unwind (O, dto, u1);
		}
        }
	open_local_scope ();
	enter_local_obj (obj, t);
	while (CODE [p] != '}') {
		dot = CODE [p] == '.';
		p = skip_expression (CODE, ps = p + dot, INIT_EXPR);
		sintprintf (cexpr, obj, '[', tcnt = new_value_int (cnt++), ']', '.', -1);
		if (!dot) intcatc (cexpr, RESERVED_ctor);
		if ((enc = (CODE [ps] != '(' || CODE [p - 1] != ')') && !dot))
			intcatc (cexpr, '(');
		intextract (cexpr + intlen (cexpr), &CODE [ps], p - ps);
		if (enc) intcatc (cexpr, ')');
		CLEAR_MAYTHROW;
		if ((ctexpr = rewrite_ctor_expr (intdup (cexpr)))) {
			if (throwing && !TEST_MAYTHROW)
				outprintf (O, u1, '.', RESERVED_i, '=', tcnt, ';', -1);
			outprintf (O, '{', ISTR (ctexpr), ';', '}', -1);
			free (ctexpr);
		} else TEST_MAYTHROW;
		if (CODE [p] == ',') ++p;
		else if (CODE [p] != '}') parse_error (p, "Initializer expression separator");
	}

	if (throwing)
		outprintf (O, u1, '.', RESERVED_i, '=', new_value_int (cnt), ';', -1);

	if (autosize) *N = cnt;
	else if (rest > cnt) {
		Token cc = internal_identifiern (4);

		outprintf (O, RESERVED_int, cc, ';',
			   RESERVED_for, '(', cc, '=', new_value_int (cnt), ';',
			   cc, '<', new_value_int (rest), ';', cc, PLUSPLUS, -1);
		if (throwing)
			outprintf (O, ',', PLUSPLUS, u1, '.', RESERVED_i, -1);
		output_itoken (O, ')');
		enter_local_obj (cc, typeID_int);
		sintprintf (cexpr, obj, '[', cc, ']', '.', RESERVED_ctor, '(', ')', -1);
		if ((ctexpr = rewrite_ctor_expr (intdup (cexpr)))) {
			outprintf (O, ISTR (ctexpr), ';', -1);
			free (ctexpr);
		}
		cnt = rest;
	} else if (rest < cnt)
		parse_error_ll ("Excess elements in array ctors!");

	if (dstruct && !throwing)
		outprintf (O, u1, '.', RESERVED_i, '=', new_value_int (cnt), ';', -1);
	close_local_scope ();

	return p + 1;
}

static NormPtr multiple_array_ctors (OUTSTREAM O, NormPtr p, typeID t, Token obj, Token *n, Token **M, bool isunwind)
{
	int cnt = 0, have_cnt;

	if ((have_cnt = n [intlen (n) - 2] != '[')) {
		if (n [intlen (n) - 3] != '[')
			parse_error_tok (obj, "Can't evaluate complex expression for sizeof :(");
		cnt = eval_int (n [intlen (n) - 2]);
	}

	OUTSTREAM N = new_stream ();
	p = gen_array_ctors (N, p + 1, t, obj, &cnt, isunwind);
	*M = combine_output (N);

	if (!have_cnt)
		outprintf (O, BACKSPACE, new_value_int (cnt), ']', -1);
	return p;
}

static void vla2alloca (OUTSTREAM O, Token obj, NormPtr ep)
{
	outprintf (O, '=', '(', RESERVED___typeof__, '(', obj, ')', ')',
		   INTERN_alloca, '(', RESERVED_sizeof, '*', obj, '*', '(', -1);
	exprret E;
	parse_expression_retconv (ep, &E, typeID_int, NORMAL_EXPR);
	if (E.ok) {
		outprintf (O, ISTR (E.newExpr), ')', ')', -1);
		free (E.newExpr);
	} else outprintf (O, RESERVED_0, ')', ')', -1);
}

// avoid warning
#define NW(x) x=x

Token local_name;
typeID local_type;

NormPtr local_declaration (OUTSTREAM O, NormPtr p)
{
	bool hadinit, NW(spec_init), isdefinition, NW(mult_init), NW(ctor_assign);
	int static_ctorable;
	Dclbase b;
	Dclobj d = DCLOBJ_INIT;
	typeID dt;
	Token *para_code=para_code;
	bool specialization = CODE [p] == RESERVED_specialize;

	if (CODE [p] == ';') return p + 1;

	last_location = p;
	b.class_name = 0;
	p = parse_basetype (&b, p);
	d.maybe_ctor = b.maybe_ctor;
	d.const_in_base = b.isconst;

	if (b.basetype == BASETYPE_ABSTRACT)
		parse_error (p, "It is not possible to have objects of abstract classes");
	if (b.basetype == BASETYPE_SKIP)
		return p;

	if (CODE [p] == ';') {
		/* Feature: abstract declaration for an object with a constructor */
		if (!specialization && b.basetype >= 0 && has_void_ctor (b.basetype)) {
			Token t = internal_identifier1 ();

			Can_instantiate (b.basetype);
			open_local_scope ();
			enter_local_obj (t, dcltype (&b, 0));
			outprintf (O, '{', ISTR (b.dclqual), ISTR (b.shortname), t, ';', -1);
			if (need_construction (b.basetype))
				alloc_and_init_dcl (O, b.basetype, t, false);
			call_ctor (O, t, -1);
			if (has_dtor (b.basetype))
				outprintf (O, dtor_name (b.basetype),
					   '(', '&', t, ')', ';', -1);
			outprintf (O, '}', -1);
			close_local_scope ();
		}
		/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
		return p + 1;
	}

	for (;; p++) {
		p = parse_dcl (&d, p, 0);

		dt = dcltype (&b, &d);

		/* Feature: pure typedefs */
		if (base_of (dt) == B_PURE) {
			if (!objective.recording)
				parse_error (p, "Only auto-functions may use pure typedefs");
			usage_call_pure ();
		}

		if (d.dclstr [d.objp] == BLANKT || d.dclstr [d.objp] == RESERVED__) {
			/* Feature: anoymous obj call ctor thingy */
			if (((isstructure (dt) && (CODE [p] == '('))
			|| (CODE [p + 1] == '(' && ISSYMBOL (CODE [p])))) {
				Token t = internal_identifier1 ();
				int basetype = base_of (dt);

				Can_instantiate (basetype);
				open_local_scope ();
				enter_local_obj (t, dt);
				outprintf (O, '{', ISTR (b.dclqual), ISTR (b.shortname),
					   t, ';', -1);
				if (need_construction (basetype))
					alloc_and_init_dcl (O, basetype, t, false);
				p = call_ctor (O, t, p);
				if (has_dtor (basetype))
					outprintf (O, dtor_name (basetype),
						   '(', '&', t, ')', ';', -1);
				outprintf (O, '}', -1);
				close_local_scope ();
				if (CODE [p] != ';')
					parse_error (p, "Anonymous object (constructor) ';'");
				break;
			} else
				parse_error (p, "Abstract declaration in local");
			/* * * * * * * * * * * * * * * * * * * * */
		}


		if (isfunction (dt)) {
			pdcl_function (&b, &d, dt, 0);
		} else {

			OUTSTREAM D = b.isstatic ? new_stream () : O;

			outprintf (D, ISTR (b.dclqual), ISTR (b.shortname),
				   ISTR (d.dclstr), -1);

			/* Feature: linkonce statics */
			if (!b.istypedef && b.islinkonce)
				output_itoken (D, linkonce_data_f (d.dclstr [d.objp]));
			/* ^^^^^^^^^^^^^^^^^^^^^^^^^ */

			if ((hadinit = CODE [p] == '=')) {
				SAVE_VAR (local_name, d.dclstr [d.objp]);
				SAVE_VAR (local_type, dt);

				/* Cases of initialization */
				spec_init = CODE [p + 1] == '{' &&
					 isstructure (dt) && need_construction (base_of (dt));
					 //|| has_parents (base_of (dt)));
				mult_init = d.dclstr [intlen (d.dclstr) - 1] == ']' &&
						is_array_of_objects (dt) && CODE [p + 1] == '{' &&
						has_ctors (base_of (dt)) && CODE [p + 2] != '{';
				ctor_assign = CODE [p + 1] != '{' && isstructure (dt) &&
						has_ctors (base_of (dt));
				if (ctor_assign)
					if (typeof_expression (p + 1, INIT_EXPR) == dt
					&& !has_copy_ctor (base_of (dt)))
						ctor_assign = false;
				if (!spec_init && !ctor_assign) {
					if (!mult_init) p = local_initializer (D, p + 1, dt);
					else p = multiple_array_ctors (D, p + 1, dt, d.dclstr
						[d.objp], d.dclstr, &para_code, b.is__unwind__);
                                }
				RESTOR_VAR (local_name);
				RESTOR_VAR (local_type);
			}
			if (d.VLA_expr)
				vla2alloca (D, d.dclstr [d.objp], d.VLA_expr);

			output_itoken (D, ';');

			isdefinition = !b.istypedef && !b.isextern;
			static_ctorable = 2;

			/* name is declared after initialization expression
			   therefore int x = x; is ok where the local x is
			   assigned the value of the global x  */
			if (b.istypedef) enter_typedef (d.dclstr [d.objp], dt);
			else enter_local_obj (d.dclstr [d.objp], dt);


			OUTSTREAM I = new_stream ();

			if (isstructure (dt) && isdefinition)	/* has virtual table && ok */
				Can_instantiate (base_of (dt));

			/* Feature: Set virtual table */
			if (isdefinition) {
			if (isstructure (dt) && need_construction (base_of (dt)))
				alloc_and_init_dcl (I, base_of (dt), d.dclstr [d.objp], false);
			else if (is_array_of_ctorable_objects (dt))
				alloc_and_init_dcl (I, base_of (dt), d.dclstr [d.objp], true);
			else static_ctorable--;
			}
			/* ^^^^^^^^^^^^^^^^^^^^^^^^^^ */

			/* Feature: aggregate initializer opened */
			if (hadinit && spec_init)
				p = special_initializer (I, p + 2, dt, d.dclstr [d.objp]);
			/* ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ */

			/* Feature: append code for multiple array ctors */
			if (hadinit && mult_init) outprintf (I, ISTR (para_code), -1);
			/* ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ */

			/* Feature: construction of objects */
			if (CODE [p] == '(' || (CODE [p + 1] == '(' && ISSYMBOL (CODE [p])))
				p = call_ctor (I, d.dclstr [d.objp], p);
			else if (!hadinit && isstructure (dt)
			       && has_void_ctor (base_of (dt)))
					call_ctor (I, d.dclstr [d.objp], -1);
			else if (hadinit && ctor_assign)
				p = call_ctor (I, d.dclstr [d.objp], p);
			else static_ctorable--;
			/* ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ */

			Token *initcode = combine_output (I);

			/* Feature: static objects that are ctored once */
			if (b.isstatic) {
				if ((static_ctorable = b.isstatic && static_ctorable)) {
					Token nn = name_glob_static_local (d.dclstr [d.objp]);
					Token *dc = combine_output (D);
					intsubst (dc, d.dclstr [d.objp], nn);
					intsubst (initcode, d.dclstr [d.objp], nn);
					outprintf (GVARS, ISTR (dc), -1);
					outprintf (GLOBAL_INIT_FUNC, ISTR (initcode), -1);
					GlobInitUsed = 1;
					free (dc);
					initcode [0] = -1;
					globalized_recent_obj (nn);
				} else outprintf (O, ISTR (combine_output (D)), -1);
                        }
			/* ++++++++++++++++++++++++++++++++++++++++++++ */


			/* Feature: mark for auto-destruction */
			if (isstructure (dt) && has_dtor (base_of (dt))
			 && !b.isstatic && isdefinition)
				if (!EHUnwind) {
					bool unwd = b.is__unwind__ || always_unwind (base_of (dt));
					add_auto_destruction (d.dclstr [d.objp], base_of (dt),unwd);
					if (initcode [0] != -1)
						outprintf (O, ISTR (initcode), -1);
					if (unwd)
						push_unwind (O, base_of (dt), d.dclstr [d.objp]);
				} else {
					outprintf (O, BACKSPACE, cleanup_func (dtor_name
						   (base_of (dt))), -1);
					if (initcode [0] == -1)
						output_itoken (O, ';');
					else outprintf (O, '=', '(', '{', ISTR (initcode),
							d.dclstr [d.objp], ';', '}', ')', ';',-1);
				}
			else if (initcode [0] != -1)
				outprintf (O, ISTR (initcode), -1);
			/* ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ */

			free (initcode);
		}
		if (CODE [p] == '{') parse_error (0, "Nested functions ?");
		if (CODE [p] == ';') break;
		if (CODE [p] != ',')
			parse_error_pt (p, CODE [p],  "unacceptable declaration, separator;");
	}

	return p + 1;
}

static NormPtr global_initializer (OUTSTREAM OSTR, Token obj, NormPtr p, typeID t)
{
	if (CODE [p] == '{') {
		output_itoken (OSTR, '=');
		return aggregate_initializer (OSTR, p + 1, t);
		//NormPtr p2 = skip_braces (p + 1);
		//outprintf (OSTR, '=', NSTRNEXT, CODE + p, p2 - p, - 1);
		//return p2;
	}
	exprret E;
	p = parse_expression_retconv (p, &E, t, INIT_EXPR);
	if (E.ok) {
		if (E.isconstant)
			outprintf (OSTR, '=', ISTR (E.newExpr), - 1);
		else {
			outprintf (GLOBAL_INIT_FUNC, obj, '=', ISTR (E.newExpr), ';', - 1);
			GlobInitUsed = true;
		}
		free (E.newExpr);
	}
	return p;
}

static NormPtr skip_attribute(NormPtr p, const char *attribute_str)
{
        if (CODE [p++] != '(')
                parse_error (p, attribute_str);
        p = skip_parenthesis (p);
        return p;
}

static NormPtr global_declaration (NormPtr p, bool linkonce)
{
static	int ctorno;
	Dclbase b;
	Dclobj d = DCLOBJ_INIT;
	typeID dt;
	bool isdefinition;

	if (CODE [p] == ';') return p + 1;

	d.begins = last_location = p;
	b.class_name = 0;
	p = parse_basetype (&b, p);
	d.maybe_ctor = b.maybe_ctor;
	d.const_in_base = b.isconst;

	if (CODE [p] == ';') return p + 1;

	if (b.basetype == BASETYPE_ABSTRACT)
		parse_error (p, "It is not possible to have objects of abstract classes");
	if (b.basetype == BASETYPE_SKIP)
		return p;

	for (;;) {
		p = parse_dcl (&d, p, 0);

		/* Skip extern abstract func */
		if (d.ctordtor == BASETYPE_SKIP)
			return p;
		/*        -  - - -  -        */

		if (d.dclstr [d.objp] == BLANKT)
			parse_error (p, "Missing object name");

		/* Constructors: add type to ctor function */
		if (d.ctordtor && d.rec)
			(d.ctordtor == 1 ? ctor_basetype : dtor_basetype) (&b, d.rec);
		/* + + + + + + + + + + + + + + + + + + + + */

		dt = dcltype (&b, &d);
		if (isfunction (dt) && !b.istypedef) {
			bool abstract = false, const_fn = false;
			int flagz = 0;

			/* Feature: adding '*this' */
			while (in2 (CODE [p], RESERVED_const, RESERVED_nothrow))
				if (CODE [p++] == RESERVED_const) const_fn = true;
				else flagz |= FUNCP_NOTHROW;
			if (d.rec) {
				dt = makemember (dt, d.rec);
				flagz |= exported_flagz (d.rec, d.cname, dt) |
					inherited_flagz (d.rec, d.cname, dt);

				if (flagz & FUNCP_VIRTUAL && flagz & FUNCP_UNDEF)
					parse_error (p, "Declare in the class: virtual!");

				if (d.cname == RESERVED_new)
					b.ismodular = true;

				/* Feature: auto function definition ? */
				b.isauto |= flagz & FUNCP_AUTO;
				const_fn |= flagz & FUNCP_CTHIS;
				b.ismodular |= flagz & FUNCP_MODULAR;
				if (b.isstatic) flagz |= FUNCP_STATIC;
				if (b.isinline) flagz |= FUNCP_INLINE;
				if (!b.ismodular) {
					add_this (&d, const_fn);
					if (is_aliasclass (d.rec))
						remove_struct_from_this (d.dclstr, d.rec);
				} else flagz |= FUNCP_MODULAR;
			}
			/* ^^^^^^^^^^^^^^^^^^^^^^^ */
			/* Feature: ctor() as a global */
			if (d.dclstr [d.objp] == RESERVED_ctor && !d.rec) {
				outprintf (GLOBAL_INIT_FUNC, d.dclstr [d.objp] = d.cname =
					   name_global_ctor (ctorno++), '(', ')', ';', -1);
				GlobInitUsed = 1;
			}
			/* *************************** */

			if (GoodOldC)
				flagz |= FUNCP_NOTHROW;

			if (d.cname == -1)
				d.cname = d.dclstr [d.objp];

			if (!abstract)
				d.dclstr [d.objp] = pdcl_function (&b, &d, dt, flagz);

			if (CODE [p] == '{') {
				mark_function_def (&b, &d, ++p, d.rec,
						   funcreturn (dt), d.cname, d.ctordtor == 2);
				if (d.rec)
					possible_keyfunc (d.rec, d.dclstr [d.objp]);
				if (b.islinkonce || linkonce)
					xmark_section_linkonce (FSP (d.rec), d.cname,
								d.dclstr [d.objp]);
				return skip_braces (p);
			}

		} else {
			OUTSTREAM OSTR = b.istypedef ? GLOBAL : GVARS;

			if (d.VLA_expr)
				parse_error (p, "VLA in global not possible");

			if (b.istypedef) {
				if (enter_typedef (d.dclstr [d.objp], dt))
					outprintf (OSTR, ISTR (b.dclqual), ISTR (b.shortname),
						   ISTR (d.dclstr), -1);
				goto skipper;
			} else enter_global_object (d.dclstr [d.objp], dt);

			outprintf (OSTR, ISTR (b.dclqual), ISTR (b.shortname),
				   ISTR (d.dclstr), -1);

			if (!b.istypedef && b.islinkonce)
				output_itoken (OSTR, linkonce_data (d.dclstr [d.objp]));


			isdefinition = !b.istypedef && !b.isextern;

			if (isstructure (dt) && isdefinition)	/* if has virtual table ok */
				Can_instantiate (base_of (dt));

			/* Feature: Set virtual table and/or virtual base class */
			if (isdefinition) {
			if (isstructure (dt) && need_construction (base_of (dt)))
				alloc_and_init_dcl (GLOBAL_INIT_FUNC, base_of (dt),
						 d.dclstr [d.objp], false);
			else if (is_array_of_ctorable_objects (dt))
				alloc_and_init_dcl (GLOBAL_INIT_FUNC, base_of (dt),
						 d.dclstr [d.objp], true);
			}
			/* ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ */

			/* here we don't need to parse initialization expression
			   before declaring the object name because it's global
			   and the object name could not possibly refer to
			   an outer scope  */
			if (CODE [p] == '(' || (ISSYMBOL (CODE [p]) && CODE [p + 1] == '('))
				p = call_ctor (GLOBAL_INIT_FUNC, d.dclstr [d.objp], p);
			else if (CODE [p] == '=')
				p = global_initializer (OSTR, d.dclstr [d.objp], p + 1, dt);
			else if (isstructure (dt) && has_void_ctor (base_of (dt)) && !b.isextern)
				call_ctor (GLOBAL_INIT_FUNC, d.dclstr [d.objp], -1);

		skipper:
			output_itoken (OSTR, ';');
		}
                try_attribute:
		if (CODE [p] == ';') break;
                switch(CODE [p]) {
                    case RESERVED___asm__: p = skip_attribute(++p, "__asm__ '('"); goto try_attribute;
                    case RESERVED___attribute__: p = skip_attribute(++p, "__attribute__ '('"); goto try_attribute;
                    case ',': break;
                    default:
                        parse_error_pt (p, CODE [p], "invalid declaration separator");
                }
                ++p;
	}

	return p + 1;
}

void reparse_template_func (Token m, recID r, NormPtr p)
{
	SAVE_VAR (bt_macro, m);
	SAVE_VAR (bt_replace, name_of_struct (r));
	PUSH_CURRENT_SCOPE (r);

	global_declaration (p, 1);

	POP_CURRENT_SCOPE;
	RESTOR_VAR (bt_macro);
	RESTOR_VAR (bt_replace);
}

typeID eval_cast (Token **t)
{
	Dclbase b;
	Dclobj d = DCLOBJ_INIT;

	SAVE_VAR (CODE, *t);

	b.class_name = -1;

	parse_dcl (&d, parse_basetype (&b, 0), 0);
	if (b.basetype == BASETYPE_ABSTRACT)
		parse_error_ll ("abstract is not a real base type");
	frealloc (t, intlen (b.shortname) + intlen (d.dclstr) + 1);
	intcpy (*t, b.shortname);
	intcat (*t, d.dclstr);

	RESTOR_VAR (CODE);

	return dcltype (&b, &d);
}

static NormPtr alt_translation_unit (NormPtr p)
{
	if (CODE [p] != RESERVED_C)
		output_itoken (INCLUDE, include_sys_header (CODE [p]));

	if (CODE [++p] != '{')
		parse_error (p, "extern \"string\" '{'");
	++p;

	SAVE_VAR (GLOBAL, 0);
	SAVE_VAR (STRUCTS, 0);
	SAVE_VAR (GVARS, 0);
	SAVE_VAR (StructByRef, false);
	SAVE_VAR (Streams_Closed, true);
	SAVE_VAR (GoodOldC, true);

	while (CODE [p] != '}' && CODE [p] != -1)
		p = global_declaration (p, 0);

	if (CODE [p++] != '}')
		parse_error (p, "missing '}' in extern header region");

	RESTOR_VAR (GLOBAL);
	RESTOR_VAR (STRUCTS);
	RESTOR_VAR (GVARS);
	RESTOR_VAR (StructByRef);
	RESTOR_VAR (Streams_Closed);
	RESTOR_VAR (GoodOldC);

	return p;
}

void translation_unit ()
{
	NormPtr p = 0;

	while (CODE [p] != -1)
		switch (CODE [p]) {
		 case RESERVED__lwc_config_:
			p = lwc_config (p + 1);
		ncase RESERVED___C__:
			p = __C__ (p + 1);
		ncase RESERVED_extern:
			if (ISVALUE (CODE [p + 1])
			&& type_of_const (CODE [p + 1]) == typeID_charP) {
				p = alt_translation_unit (p + 1);
				continue;
			}
		default:
			p = global_declaration (p, 0);
		}
}
