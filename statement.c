#include "global.h"

#define BLOCK_START_MAYTHROW(X) \
	CLEAR_MAYTHROW; int pos = get_stream_pos (X); SAVE_VAR (did_unwind, false);
#define BLOCK_END_MAYTHROW(X) \
	if (TEST_MAYTHROW && did_unwind) { wipeout_unwind (X, pos); } RESTOR_VAR (did_unwind);

NormPtr statement (OUTSTREAM, NormPtr);

static NormPtr parenth_expression (OUTSTREAM o, NormPtr p)
{
	exprret E;
	if (CODE [p++] != '(')
		parse_error (p, "missing '(' after reserved keyword");
	p = parse_expression (p, &E, NORMAL_EXPR);
	if (!E.ok) parse_error (p, "compilation stops here");
	if (CODE [p++] != ')')
		parse_error (p, "missing ')' after reserved keyword");
	outprintf (o, '(', ISTR (E.newExpr), ')', -1);
	free (E.newExpr);
	return p;
}

static NormPtr __asm___statement (OUTSTREAM o, NormPtr p)
{
	NormPtr p2 = p;
	if (CODE [p] == RESERVED_volatile)
		++p;
	if (CODE [p++] != '(')
		parse_error (p, "__asm__ '('");
	p = skip_parenthesis (p);
	if (CODE [p++] != ';')
		parse_error (p, "missing ';' after __asm__");
	outprintf (o, RESERVED___asm__, NSTRNEXT, CODE + p2, p - p2, ';', - 1);
	return p;
}

static NormPtr if_statement (OUTSTREAM o, NormPtr p)
{
	output_itoken (o, RESERVED_if);
	p = statement (o, parenth_expression (o, p));
	// oh my god. this must be the horrible if-else ambiguity!
	if (CODE [p] == RESERVED_else) {
		output_itoken (o, RESERVED_else);
		p = statement (o, p + 1);
	}
	return p;
}

/* general statmenent at catchpoint which may catch longbreak/longcontinue */
static NormPtr loop_body_statement (OUTSTREAM o, NormPtr p, int cp)
{
	Token lbrk, lcnt;
	OUTSTREAM t = new_stream ();

	add_catchpoint (cp);
	p = statement (t, p);
	rmv_catchpoint (&lbrk, &lcnt);

	if (lcnt || lbrk) output_itoken (o, '{');
	concate_streams (o, t);
	if (lcnt) outprintf (o, lcnt, ':', lbrk ? BLANKT : ';', -1);
	if (lbrk) outprintf (o, RESERVED_continue, ';', lbrk, ':', RESERVED_break, ';', -1);
	if (lcnt || lbrk) output_itoken (o, '}');

	return p;
}

static struct {
	OUTSTREAM caselabels;
	Token addr1, dflt, base, array;
} cl, nocl;

static NormPtr switch_statement (OUTSTREAM o, NormPtr p)
{
	bool dcl;
	/* Feature: switch (declaration) */
	if ((dcl = CODE [p] == '(' && is_dcl_start (CODE [p + 1]) && CODE [p + 2] != '.')) {
		NormPtr p2;
		p = skip_parenthesis (p2 = p + 1) - 1;
		CODE [p] = ';';
		open_local_scope ();
		output_itoken (o, '{');
		local_declaration (o, p2);
		CODE [p++] = ')';
		outprintf (o, RESERVED_switch, '(', recent_obj (), ')', -1);
	/* ^^^^^^^^^^^^^^^^^^^^^^^ */
	} else {
		output_itoken (o, RESERVED_switch);
		p = parenth_expression (o, p);
	}

	/* Feature: gather switch label addresses */
	SAVE_VAR (cl, nocl);
	if (CODE [p] == ':') {
		if (!syntax_pattern (p, ':', VERIFY_symbol, ',', VERIFY_symbol, '[', ']', -1))
			parse_error (p, "The syntax is 'switch () : name1, name2 []'");
		outprintf (cl.caselabels = new_stream (), RESERVED_static, RESERVED_const,
			   RESERVED_int, cl.array = CODE [p + 3], '[', ']', '=', '{', -1);
		cl.base = CODE [p + 1];
		p += 6;
	}
	/* :::::::::::::::::::::::::::::::::::::: */

	p = loop_body_statement (o, p, 1);

	if (dcl) {
		close_local_scope ();
		output_itoken (o, '}');
	}

	/* make label addresses */
	if (cl.caselabels) {
		Token *dcl = combine_output (cl.caselabels), v = cl.dflt ?: cl.addr1;
		int i;
		for (i = 0; dcl [i] != -1; i++)
			if (dcl [i] == RESERVED_0) dcl [i] = v;
		outprintf (o, ISTR (dcl), '}', ';', RESERVED_static, RESERVED_void,
			   '*', cl.base, '=', ANDAND, v, ';', -1);
		enter_local_obj (cl.array, typeID_intP);
		enter_local_obj (cl.base, typeID_voidP);
		free (dcl);
	}
	RESTOR_VAR (cl);
	/* :::::::::::::::::::: */

	return p;
}

static NormPtr do_statement (OUTSTREAM o, NormPtr p)
{
	OUTSTREAM S = new_stream ();

	p = loop_body_statement (S, p, 2);

	if (CODE [p] == RESERVED_while) {
		output_itoken (o, RESERVED_do);
		concate_streams (o, S);
		output_itoken (o, RESERVED_while);
		p = parenth_expression (o, p + 1);
		if (CODE [p++] != ';')
			parse_error (p, "this is special. You need a ';'");
		output_itoken (o, ';');
	/* Feature: do without while */
	} else {
		outprintf (o, RESERVED_for, '(', ';', ';', ')', '{', - 1);
		concate_streams (o, S);
		outprintf (o, RESERVED_break, ';', '}', - 1);
	}
	/* ^^^^^^^^^^^^^^^^^^^^^^^^^ */

	return p;
}

static NormPtr for_statement (OUTSTREAM o, NormPtr p)
{
	bool first_dcl;
	exprret E1, E2, E3;

	if (CODE [p++] != '(')
		parse_error (p, "for '('");
	/* Feature: first part may be declaration */
	if ((first_dcl = is_dcl_start (CODE [p]))) {
		output_itoken (o, '{');
		open_local_scope ();
		p = local_declaration (o, p) - 1;
		(E1.newExpr = mallocint (1)) [0] = -1;
		E1.ok = true;
	} else p = parse_expression (p, &E1, NORMAL_EXPR);
	if (CODE [p++] != ';')
		parse_error (p, "for (';'");
	p = parse_expression (p, &E2, NORMAL_EXPR);
	if (CODE [p++] != ';')
		parse_error (p, "for (;';'");
	p = parse_expression (p, &E3, NORMAL_EXPR);
	if (CODE [p++] != ')')
		parse_error (p, "for (;';')");
	if (!(E1.ok && E2.ok && E3.ok))
		parse_error (p, "compilation halted at for()");
	outprintf (o, RESERVED_for, '(', ISTR (E1.newExpr), ';',
		   ISTR (E2.newExpr), ';', ISTR (E3.newExpr), ')', -1);
	free (E1.newExpr);
	free (E2.newExpr);
	free (E3.newExpr);

	p = loop_body_statement (o, p, 2);

	if (first_dcl) {
		close_local_scope ();
		output_itoken (o, '}');
	}
	return p;
}

static NormPtr while_statement  (OUTSTREAM o, NormPtr p)
{
	output_itoken (o, RESERVED_while);
	p = loop_body_statement (o, parenth_expression (o, p), 2);
	return p;
}

static NormPtr return_statement (OUTSTREAM o, NormPtr p)
{
	exprret E;
	bool he;
	p = parse_expression_retconv (p, &E, return_typeID, NORMAL_EXPR);
	if (!E.ok) return p + 1;

	/* Feature: a 'return;' in destructors, returns this */
	if (!(he = E.newExpr [0] != -1) && (he = objective.isdtor))
		sintprintf (frealloc (&E.newExpr, 2), RESERVED_this, -1);

	/* Feature: before return, call internal destruction */
	if (objective.isdtor && idtor (objective.class)) {
		Token *t = mallocint (intlen (E.newExpr) + 7);
		sintprintf (t, idtor (objective.class), '(', RESERVED_this, ')', ',',
			    ISTR (E.newExpr), -1);
		free (E.newExpr);
		E.newExpr = t;
	}

	/* Feature: destructors for all locals */
	if (func_has_dtors ()) {
		outprintf (o, '{', -1);
		if (he) outprintf (o, RESERVED___typeof__, '(', ISTR (E.newExpr), ')',
			   internal_identifier1 (), '=', ISTR (E.newExpr), ';', -1);
		gen_all_destructors (o);
		outprintf (o, RESERVED_return, he ? internal_identifier1 () : BLANKT, ';', '}', -1);
	} else
	/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
		outprintf (o, RESERVED_return, ISTR (E.newExpr), ';', -1);
	free (E.newExpr);
	if (CODE [p] != ';')
		parse_error (p, "expecting ';' after return");
	return p + 1;
}

static NormPtr case_statement (OUTSTREAM o, NormPtr p)
{
static	int uniq;
	Token v = 0;
	exprret E;
	output_itoken (o, RESERVED_case);
	p = parse_expression (p, &E, NORMAL_EXPR);
	if (E.ok) outprintf (o, ISTR (E.newExpr), -1);

	/* gather labels */
	if (cl.caselabels) {
		v = toktokcat (RESERVED_case, E.newExpr [1] != -1 ?
			 internal_identifiern (uniq++) : E.newExpr [0]);
		if (!cl.addr1) cl.addr1 = v;
		outprintf (cl.caselabels, '[', ISTR (E.newExpr), ']', '=',
			   ANDAND, v, '-', ANDAND, RESERVED_0, ',', -1);
	}

	free (E.newExpr);
	/* extension '...' in case */
	if (CODE [p] == ELLIPSIS) {
		if (v) parse_error (p, "Can't gather labels for '...' case");
		p = parse_expression (p, &E, NORMAL_EXPR);
		if (E.ok) {
			outprintf (o, ELLIPSIS, ISTR (E.newExpr), -1);
			free (E.newExpr);
		}
	}
	if (CODE [p] != ':') parse_error (p, "case ':' error");
	output_itoken (o, ':');
	/* label equiv */
	if (v) outprintf (o, v, ':', -1);
	return p + 1;
}

static NormPtr expression_statement (OUTSTREAM o, NormPtr p)
{
	exprret E;
	p = parse_expression (p, &E, NORMAL_EXPR);
	if (CODE [p++] != ';') parse_error (p, "expression';'");
	if (E.ok) {
		outprintf (o, ISTR (E.newExpr), ';', -1);
		free (E.newExpr);
	}
	return p;
}

static inline bool is_declaration (NormPtr p)
{
	return is_dcl_start (CODE [p]) && !(ISSYMBOL (CODE [p]) && CODE [p + 1] == '.');
}

#ifdef DEBUG
bool special_debug;
#endif

NormPtr compound_statement (OUTSTREAM o, NormPtr p)
{
	bool noend = false;

#ifdef	DEBUG
	SAVE_VAR (special_debug, CODE [p] == RESERVED_0);
	if (CODE [p] == RESERVED_0) ++p;
#endif
	BLOCK_START_MAYTHROW (o);
	open_local_scope ();
	output_itoken (o, '{');
	while (CODE [p] != '}') {
		noend = in4 (CODE [p], RESERVED_return, RESERVED_break,
			    RESERVED_continue, RESERVED_goto);
		p = is_declaration (p) ? local_declaration (o, p) : statement (o, p);
	}
	/* Feature: destructors of local objects */
	if (scope_has_dtors () && !noend)
		gen_auto_destruction (o, !may_throw);
	/* ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ */
	output_itoken (o, '}');
	close_local_scope ();
	BLOCK_END_MAYTHROW (o);
#ifdef	DEBUG
	RESTOR_VAR (special_debug);
#endif
	return p + 1;
}

static NormPtr jump_statement (OUTSTREAM o, NormPtr p, Token t)
{
	int cont = t == RESERVED_continue ? 2 : 1;
	bool bhd;

	/* Feature: break 2; */
	int break_n = ISVALUE (CODE [p]) ? eval_int (CODE [p++]) : 1;
	/* ^^^^^^^^^^^^^^^^^ */

	if (CODE [p] != ';')
		parse_error (p, "break/continue ';'");

	/* Feature: destructors of local objects */
	if ((bhd = break_has_dtors (cont)) || break_n != 1) {
		Token lbl;

		if (bhd) output_itoken (o, '{');
		if ((lbl = gen_break_destructors (o, cont, break_n)))
			outprintf (o, RESERVED_goto, lbl, ';', -1);
		else outprintf (o, t, ';', -1);
		if (bhd) output_itoken (o, '}');
	} else
	/* ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ */
		outprintf (o, t, ';', -1);
	return p + 1;
}

static NormPtr goto_statement (OUTSTREAM o, NormPtr p)
{
	/* GNUC: computed goto */
	if (CODE [p] == '*') {
		outprintf (o, RESERVED_goto, '*', -1);
		return expression_statement (o, p + 1);
	}
	/* Feature: goto (...) ? l1 : l2 */
	if (CODE [p] == '(' && CODE [skip_parenthesis (p + 1)] == '?') {
		outprintf (o, '{', RESERVED_if, -1);
		p = parenth_expression (o, p);
		syntax_pattern (p, '?', VERIFY_symbol, ':', VERIFY_symbol, ';', -1);
		outprintf (o, RESERVED_goto, CODE [p + 1], ';',
			   RESERVED_goto, CODE [p + 3], ';', '}', -1);
		return p + 4;
	}

	if (!ISSYMBOL (CODE [p]) || CODE [p + 1] != ';')
		parse_error (p, "'goto label;' only");
	outprintf (o, RESERVED_goto, CODE [p], ';', -1);
	return p + 2;
}

NormPtr statement (OUTSTREAM o, NormPtr p)
{
	Token t = CODE [p++];

	if (ISSYMBOL (t) && CODE [p] == ':') {
		outprintf (o, t, CODE [p++], -1);
		return CODE [p] == '}' ? p : statement (o, p);
	}

	switch (t) {
	case '{':		return compound_statement (o, p);
	case RESERVED_case:	return case_statement (o, p);
	case RESERVED_return:	return return_statement (o, p);
	case RESERVED_while:	return while_statement (o, p);
	case RESERVED_for:	return for_statement (o, p);
	case RESERVED_do:	return do_statement (o, p);
	case RESERVED_switch:	return switch_statement (o, p);
	case RESERVED_if:	return if_statement (o, p);
	case RESERVED_else:	parse_error (p, "else without if");
	case RESERVED_throw:	return throw_statement (o, p);
	case RESERVED_try:	return try_statement (o, p);
	case RESERVED___on_throw__:
				return on_throw_statement (o, p);
	case RESERVED___asm__:	return __asm___statement (o, p);
	case RESERVED_continue:
	case RESERVED_break:	return jump_statement (o, p, t);
	case RESERVED_goto:	return goto_statement (o, p);
	default:		return expression_statement (o, p - 1);
	case ';':
		output_itoken (o, ';');
		return p;
	case RESERVED_default:
		if (CODE [p] != ':') parse_error (p, "default:");
		outprintf (o, RESERVED_default, ':', -1);
		if (cl.caselabels) {
			cl.dflt = tokstrcat (RESERVED_default, "_label");
			outprintf (o, cl.dflt, ':', -1);
		}
		return p + 1;
	}
}

/* This is a compound statement inside an expression */
Token *rewrite_compound_statement (Token *t)
{
	OUTSTREAM o = new_stream ();

	SAVE_VAR (CODE, t);
	SAVE_VAR (last_location, 0);

	output_itoken (o, '(');
	compound_statement (o, 0);
	output_itoken (o, ')');

	RESTOR_VAR (CODE);
	RESTOR_VAR (last_location);

	return combine_output (o);
}
