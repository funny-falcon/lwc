#include "global.h"

/******************************************************************************

	For some things we need some internal static inline functions,
	where otherwise we would have to use statements in
	expressions and hacks.

	However, these are created only if/when needed.

******************************************************************************/

static intnode *dinits;

Token i_call_initialization (recID r)
{
	intnode *n = intfind (dinits, r);
	if (n) return n->v.i;

	Token n1 = name_of_struct (r);
	Token fn = name_intern_ctor (r);
	Token xx = RESERVED_x;
	Token yy = RESERVED_y;
	Token ii = RESERVED_i;
	Token XX = RESERVED_X;

	/** ** cheat:
	-	static inline void A__ICoNsTrUcTioN (struct A *x, const int u)
	-	{
	-		int i;
	-		for (i = 0; i < y; i++) {
	-			struct A *X = &xx [i];
	-			.. here we gen_construction_code from hier.c ...
	-		}
	-	}
	** **/

	OUTSTREAM IC = new_stream ();

	outprintf (IC, RESERVED_static, RESERVED_inline, RESERVED_void,
		   fn, '(', iRESERVED_struct (r), n1, '*', xx, ',', RESERVED_const,
		   RESERVED_int, yy, ')', '{', RESERVED_int, ii, ';', RESERVED_for, '(',
		   ii, '=', RESERVED_0, ';', ii, '<', yy, ';', ii, PLUSPLUS, ')', '{',
		   iRESERVED_struct (r), n1, '*', XX, '=', '&', xx, '[', ii, ']', ';', -1);

	gen_construction_code (IC, r, XX);

	outprintf (IC, '}', '}', -1);

	concate_streams (INTERNAL_CODE, IC);

	union ival i = { .i = fn };
	intadd (&dinits, r, i);

	return fn;
}

static intnode *dinitos;

Token i_init_object (recID r)
{
	intnode *n = intfind (dinitos, r);
	if (n) return n->v.i;

	/*
		static inline struct A *InItObJ (void *p)
		{
			A__ICoNsTrUcTioN (p, 1);
			return p;
		}
	*/
	Token fn = name_init_func (r);
	Token n1 = name_of_struct (r);
	Token xx = RESERVED_x;
	OUTSTREAM IC = new_stream ();

	outprintf (IC, RESERVED_static, RESERVED_inline, iRESERVED_struct (r), n1, '*', fn, '(',
		RESERVED_void, '*', xx, ')', '{', i_call_initialization (r), '(', xx, ',',
		RESERVED_1, ')', ';', RESERVED_return, xx, ';', '}', -1);

	concate_streams (INTERNAL_CODE, IC);

	union ival i = { .i = fn };
	intadd (&dinitos, r, i);

	return fn;
}

static intnode *dcasts;

Token i_downcast_function (recID rb, recID rd)
{
	int bdi = rb + rd * 3000;
	intnode *n = intfind (dcasts, bdi);
	if (n) return n->v.i;

	rd = aliasclass (rd);
	rb = aliasclass (rb);
	bdi = rb + rd * 3000;
	if ((n = intfind (dcasts, bdi))) return n->v.i;

	Token base = name_of_struct (rb), der = name_of_struct (rd);
	Token fname = name_downcast (base, der);
	Token x = RESERVED_x;
	Token *path;
	bool va = is_ancestor_runtime (rd, rb, &path);

	/** ** cheat:
	-	static inline struct der *downcast (struct base *x)
	-	{
	-		return (struct der*)((void*)x - (int) &(((struct der*)0)->path));
	-	}
	- or:
	-	static inline struct der *downcast (struct base *x)
	-	{
	-		return (struct der*)((void*)x + x->_v_p_t_r_->base_der_DoWnCaSt);
	-	}
	- or broken into the above two:
	-	static inline struct der *downcast (struct base *x)
	-	{
	-		return downcast (downcast (x));
	-	}
	** **/

#define VOIDCAST '(', RESERVED_void, '*', ')'

	OUTSTREAM IC = new_stream ();
	outprintf (IC, RESERVED_static, RESERVED_inline,
		   iRESERVED_struct (rd), der, '*', fname, '(', RESERVED_const,
		   iRESERVED_struct (rb), base, '*', x, ')', '{', RESERVED_return, -1);

	if (!va) {
		outprintf (IC,
			'(', iRESERVED_struct (rd), der, '*', ')',
			'(', VOIDCAST, x, '-', '(', RESERVED_ssz_t, ')', '&',
			'(', '(', '(', iRESERVED_struct (rd), der, '*', ')',
			RESERVED_0, ')', POINTSAT, ISTR (path), ')', ')', -1);
	} else if (path [1] == -1) {
		Token path [128];

		downcast_rtti (rd, rb, rb, path);
		outprintf (IC,
			 '(', iRESERVED_struct (rd), der, '*', ')', '(',
			 VOIDCAST, x, '+', x, POINTSAT, ISTR (path), ')', -1);
	} else {

		/* In this case, we can implement the first half of the downcast
		   -which is plain- with one of the above and call
		   only one DoWnCaSt function.  Typically it makes no difference
		   because gcc can do great work recursively inlining all these.
		   So AFA assembly is concerned we don't have to do this optimization.
		   Having fewer DoWnCaSt functions in the output, would be
		   a better thing tho.
		 */
		int i;
		recID mid;

		if (path [1] == POINTSAT)
			mid = ancest_named (rd, path [0]);
		else for (mid = rd, i = 0; path [i + 1] != POINTSAT && path [i + 1] != -1; i+= 2)
			mid = ancest_named (mid, path [i]);

		outprintf (IC,
			'(', iRESERVED_struct (rd), name_of_struct (rd), '*', ')',
			i_downcast_function (mid, rd), '(',
			i_downcast_function (rb, mid), '(', x, ')',
			')', -1);
	}

	outprintf (IC, ';', '}', -1);
	concate_streams (INTERNAL_CODE, IC);

	union ival i = { .i = fname };
	intadd (&dcasts, bdi, i);
	return fname;
}

static intnode *dcasts_safe;

Token i_downcast_null_safe (recID rb, recID rd)
{
	int bdi = rb + rd * 3000;
	intnode *n = intfind (dcasts_safe, bdi);
	if (n) return n->v.i;

	/* ** cheat
	- 	static inline struct der* downcast_safe (struct base *x)
	-	{
	-		return x ? Downcast (x) : 0;
	-	}
	** **/

	OUTSTREAM IC = new_stream ();
	Token base = name_of_struct (rb), der = name_of_struct (rd);
	Token fname = name_downcast_safe (base, der);
	Token x = RESERVED_x;

	outprintf (IC, RESERVED_static, RESERVED_inline, iRESERVED_struct (rd),
		   der, '*', fname, '(', iRESERVED_struct (rb),
		   base, '*', x, ')', '{', RESERVED_return,
		   x, '?', i_downcast_function (rb, rd), '(', x, ')', ':',
		   RESERVED_0, ';', '}', -1);

	concate_streams (INTERNAL_CODE, IC);

	union ival i = { .i = fname };
	intadd (&dcasts_safe, bdi, i);
	return fname;
}

static intnode *upcasts;

Token i_upcast_null_safe (recID rb, recID rd, Token *path, bool ptrpath)
{
	int bdi = rb + rd * 3000;
	intnode *n = intfind (upcasts, bdi);
	if (n) return n->v.i;

	/* ** cheat
	- 	static inline struct base* upcast_safe (struct der *x)
	-	{
	-		return x ? &x->path : 0;
	-	}
	** **/

	Token base = name_of_struct (rb), der = name_of_struct (rd);
	Token fname = name_upcast_safe (base, der);
	Token x = RESERVED_x;

	outprintf (INTERNAL_CODE, RESERVED_static, RESERVED_inline,
		   iRESERVED_struct (rb), base, '*', fname,
		   '(', iRESERVED_struct (rd), der, '*', x, ')', '{',
		   RESERVED_return, x, '?', ptrpath ? BLANKT : '&', x,
		   POINTSAT, ISTR (path), ':', RESERVED_0, ';', '}', -1);

	union ival i = { .i = fname };
	intadd (&upcasts, bdi, i);
	return fname;
}

static intnode *vcalls;

Token i_member_virtual (recID r, Token n, typeID *argt, flookup *rF)
{
	Token vpath [128], *proto;
	flookup F;
	int i;
	bool modular = rF->flagz & FUNCP_MODULAR;

	rF->flagz &= ~FUNCP_MODULAR;
	if (!lookup_virtual_function_member (r, n, argt, vpath, &F))
		parse_error_ll ("error && bug");

	rF->oname = F.oname = vpath [intlen (vpath) - 1];
	if (intfind (vcalls, F.oname))
		return F.oname;

	// create the new prototype
	proto = allocaint (intlen (F.prototype) + 10);
	intcpy (proto, F.prototype);
	if (modular) {
		// add 'this'
		for (i = 0; proto [i] != MARKER; i++);
		while (proto [i] != '(') i++;
		Token *rest = allocaint (intlen (proto + i) + 1);
		intcpy (rest, proto + ++i);
		sintprintf (proto + i, RESERVED_const, iRESERVED_struct (r),
			    name_of_struct (r), '*', RESERVED_this, -1);
		i += 5;
		if (rest [0] != ')')
			proto [i++] = ',';
		sintprintf (proto + i, ISTR (rest), -1);
	}
	intsubst1 (proto, MARKER, F.oname);
	rF->prototype = intdup (proto);

	i = modular && F.xargs [0] == -1 ? 0 : 1;
	outprintf (INTERNAL_CODE, intchr (proto, RESERVED_static) ? BLANKT : RESERVED_static,
		   intchr (proto, RESERVED_inline) ? BLANKT : RESERVED_inline,
		   ISTR (proto), '{', RESERVED_return, RESERVED_this, POINTSAT,
		   ISTR (vpath), '(',
		   modular ? (F.xargs [0] != -1 ? F.xargs [0] : BLANKT) : RESERVED_this, -1);
	for (; F.xargs [i] != -1; i++)
		outprintf (INTERNAL_CODE, ',', F.xargs [i], -1);
	outprintf (INTERNAL_CODE, ')', ';', '}', -1);

	union ival I;
	intadd (&vcalls, F.oname, I);
	return F.oname;
}

static intnode *trftree;

Token i_trampoline_func (flookup *F, bool argb[])
{
	int i, j;
	char snm [32];
	Token tfn;

	if (!F->oname)
		parse_error_ll ("Can't do it");

	for (i = 0; argb [i] != -1; i++)
		snm [i] = argb [i] ? '1' : '0';
	snm [i] = 0; strcat (snm, "trampoline");

	tfn = tokstrcat (F->oname, snm);
	if (intfind (trftree, tfn))
		return tfn;

	Token *proto = mallocint (intlen (F->prototype) + 1);
	intcpy (proto, F->prototype);
	if (intchr (proto, F->oname))
		intsubst (proto, F->oname, tfn);
	else if (intchr (proto, MARKER))
		intsubst (proto, MARKER, tfn);
	else parse_error_ll ("Nasty alien outlaw bug");

	if (intchr (proto, RESERVED__CLASS_))
		repl__CLASS_ (&proto, base_of (open_typeID (F->t)[2]));

	for (i = 0; argb [i] != -1; i++)
		if (argb [i]) {
			for (j = 2; proto [j] != F->xargs [i]; j++);
			if (proto [j - 1] == RESERVED_const)
				proto [j - 2] = BLANKT;
			proto [j - 1] = BLANKT;
		}

	outprintf (INTERNAL_CODE, intchr (proto, RESERVED_static) ? BLANKT : RESERVED_static,
		   intchr (proto, RESERVED_inline) ? BLANKT : RESERVED_inline,
		   ISTR (proto), '{', -1);

	outprintf (INTERNAL_CODE, RESERVED_return, F->oname, '(', -1);
	for (i = 0; argb [i] != -1; i++) {
		outprintf (INTERNAL_CODE, argb [i] ? '&' : BLANKT, F->xargs [i],
			   argb [i + 1] == -1 ? BLANKT : ',', -1);
	}
	outprintf (INTERNAL_CODE, ')', ';', '}', -1);
	free (proto);

	union ival I;
	intadd (&trftree, tfn, I);
	return tfn;
}

static intnode *ebntree;

Token i_enum_by_name (Token e)
{
	int i;
	intnode *n = intfind (ebntree, e);
	if (n)
		return n->v.i;

	if (!is_enum (e)) parse_error_tok (e, "__enumstr__: enumerator not defined");

	Token fname = name_ebn_func (e), *consts = enum_syms (lookup_enum (e));

	outprintf (INTERNAL_CODE, RESERVED_const, RESERVED_char, '*', fname, '(',
		   RESERVED_int, ')', linkonce_text (fname), ';',
		   RESERVED_const, RESERVED_char, '*', fname, '(', RESERVED_int, RESERVED_X, ')',
		   '{', RESERVED_switch, '(', RESERVED_X, ')', '{', -1);
	for (i = 0; consts [i] != -1; i++)
		outprintf (INTERNAL_CODE, RESERVED_case, consts [i], ':',
			   RESERVED_return, new_value_string (expand (consts [i])), ';', -1);
	outprintf (INTERNAL_CODE, RESERVED_default, ':', RESERVED_return,
		   RESERVED_0, ';', '}', '}', -1);

	union ival I = { .i = fname };
	intadd (&ebntree, e, I);
	return fname;
}

static intnode *typeidt;

Token i_typeid_var (Token c)
{
	intnode *n = intfind (typeidt, c);
	if (n)
		return n->v.i;

	Token fname = name_typeid_var (c);

	outprintf (GVARS, RESERVED_const, RESERVED_char, fname, '[', ']',
		   linkonce_rodata (fname), '=', stringify (c), ';', -1);

	union ival I = { .i = fname };
	intadd (&typeidt, c, I);
	return fname;
}

Token arrdtor;
static intnode *arrdtortree;

Token i_arrdtor_func (recID r)
{
	Token i = RESERVED_i, p = RESERVED_p;
	intnode *n = intfind (arrdtortree, r);
	if (n)
		return n->v.i;

	if (!arrdtor) {
		arrdtor = new_symbol ("ArRdToR");
		outprintf (INTERNAL_CODE, RESERVED_struct, arrdtor, '{', RESERVED_void, '*',
			   p, ';', RESERVED_int, i, ';', '}', ';', -1);
	}

	Token fname = name_arrdtor (r);

	outprintf (INTERNAL_CODE, RESERVED_static, RESERVED_inline, RESERVED_void, fname, '(',
		   RESERVED_struct, arrdtor, '*', RESERVED_this, ')', '{',
		   iRESERVED_struct (r), name_of_struct (r), '*', p, '=',
		   RESERVED_this, POINTSAT, p, ';', RESERVED_int, i, '=',
		   RESERVED_this, POINTSAT, i, ';', RESERVED_while, '(', i, ')',
		   dtor_name (r), '(', '&', p, '[', MINUSMINUS, i, ']', ')', ';', '}', -1);

	union ival I = { .i = fname };
	intadd (&arrdtortree, r, I);
	return fname;
}

/***********************************************
	These are called by expressions
***********************************************/

void alloc_and_init (OUTSTREAM o, recID r, Token tid, Token alloc, Token vec)
{
	Token sn = name_of_struct (r);
	Token ic;

	/** Cheat:
	-	= (struct sn*) malloc_alloca (sizeof (struct sn));
	-	sn___CoNsTruCTion (x, 1);
	- or vector
	-	= (struct sn*) malloc_alloca (sizeof (struct sn) * n);
	-	sn___CoNsTruCTion (x, n);
	- or with custom allocator
	-	= (struct sn*) allocator_fn ();
	-	sn___CoNsTruCTion (x, 1);
	**/
	outprintf (o, '=', '(', iRESERVED_struct (r), sn, '*', ')', alloc, '(', -1);
	if (in2 (alloc, new_wrap, INTERN_alloca))
		outprintf (o, RESERVED_sizeof, '(', iRESERVED_struct (r), sn, ')', -1);
	if (vec) outprintf (o, '*', vec, -1);
	outprintf (o, ')', ';', -1);

	if (need_construction (r)) {
		ic = i_call_initialization (r);
		outprintf (o, ic, '(', tid, ',', vec ?: RESERVED_1, ')', ';', -1);
	}
}

void alloc_and_init_dcl (OUTSTREAM o, recID r, Token tid, bool array)
{
	Token ic;

	if (o == GLOBAL_INIT_FUNC) GlobInitUsed = true;
	/** The classic:
	-	A__CoNsTruCTioN (&a, 1);
	- or array
	-	A__CoNsTruCTioN (a, sizeof a / sizeof a [0]);
	**/
	ic = i_call_initialization (r);
	if (array) outprintf (o, ic, '(', tid, ',', RESERVED_sizeof, tid,
			'/', RESERVED_sizeof, tid, '[', RESERVED_0, ']', ')', ';', -1);
	else outprintf (o, ic, '(', '&', tid, ',', RESERVED_1, ')', ';', -1);
}
