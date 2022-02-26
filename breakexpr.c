#include "global.h"

/******************************************************************************

	Break an expression string by returning
	pointer(s) to root operator of the expression tree

******************************************************************************/

//
// optimized is-operator checks
//

#define E(x) ESCBASE + x
static bool esc_op;

static const signed char priorities [128] = {
	['*'] = 13, ['/'] = 13, ['%'] = 13,
	['+'] = 12, ['-'] = 12,
	['<'] = 10, ['>'] = 10,
	['&'] = 8, ['^'] = 7, ['|'] = 6
};

static int priority (Token op)
{
	esc_op = false;

	if (op < 128)
		return priorities [op];

	switch (op) {
	case LSH:
	case RSH:	return 11;
	case PERLOP:
	case GEQCMP:
	case LEQCMP:	return 10;
	case EQCMP:
	case NEQCMP:	return 9;
	case ANDAND:	return 5;
	case OROR:	return 4;
	default:	return 0;
	}
}

static int priority_esc (Token op)
{
	if (op < ESCBASE)
		return priority (op);
	if (op > ESCBASE) {
		esc_op = true;
		switch (op) {
		case E('+'):
		case E('-'):	return 12;
		case E(GEQCMP):
		case E(LEQCMP):
		case E('<'):
		case E('>'):	return 10;
		case E(EQCMP):
		case E(NEQCMP):	return 9;
		case E(ANDAND):	return 5;
		case E(OROR):	return 4;
		}
	}
	esc_op = false;
	return 0;
}

static inline bool isassignment (Token t)
{
	esc_op = false;
	return t == '=' || ISASSIGNMENT (t);
}

static bool isassignment_esc (Token t)
{
	if (t < ESCBASE)
		return isassignment (t);
	return esc_op = t > ESCBASE && (t == E('=') || t == E(ASSIGNA) || t == E(ASSIGNS));
}

static const signed char unaries [128] = {
	['-'] = 1, ['!'] = 1, ['+'] = 1, ['*'] = 1, ['&'] = 2, ['~'] = 1
};

static inline bool isunary (Token t)
{
	esc_op = false;
	if (t < 128) return unaries [t];
	return t == PLUSPLUS || t == MINUSMINUS;
}

static inline bool isunary_esc (Token t)
{
	if (t < ESCBASE)
		return isunary (t);
	if (t == RESERVED_delete || t == RESERVED_dereference) return 2;
	return esc_op = isunary (t - ESCBASE) == 1;
}

//
// here begins the parser
//

static NormPtr expression (bexpr*, NormPtr);
static NormPtr unary_expression (bexpr*, NormPtr);
static NormPtr primary_expression (bexpr*, NormPtr);
static NormPtr postfix_expression (bexpr*, NormPtr);
static NormPtr argument_expression_list (bexpr*, NormPtr);
static NormPtr binary_expression (bexpr*, NormPtr, int);
static NormPtr logical_OR_expression (bexpr*, NormPtr);
static NormPtr conditional_expression (bexpr*, NormPtr);
static NormPtr assignment_expression (bexpr*, NormPtr);

static NormPtr unary_expression (bexpr *e, NormPtr p)
{
	NormPtr ps = p, ps2;

	if (e->expr [p] == -1)
		return p;

	if (isunary_esc (e->expr [p])) {
		bool lesc_op = esc_op;
		p = unary_expression (e, p + 1);
		if (e->nop == 0)
			parse_error_ll ("unary operator w/o operand");
		e->nop = 1;
		e->operators [0] = ps;
		e->esc_op = lesc_op;
		return p;
	}

	/* new operator */
	if (e->expr [p] == RESERVED_new || e->expr [p] == RESERVED_localloc) {
		e->nop = 2;
		e->operators [0] = p++;
		e->esc_op = false;
		if (!ISSYMBOL (e->expr [p]))
			parse_error_ll ("new/localloc may be followed by symbol only");
		if (e->expr [++p] == '(')
			p = skip_buffer_parenthesis (e->expr, p + 1);
		else if (e->expr [p] == '[')
			p = skip_buffer_brackets (e->expr, p + 1);
		/* Feature: alt ctor */
		else if (ISSYMBOL (e->expr [p]) && e->expr [p + 1] == '(')
			p = skip_buffer_parenthesis (e->expr, p + 2);
		if (e->expr [p] == POINTSAT)
			return postfix_expression (e, p);
		e->operators [1] = p;
		return p;
	}

	/* __declexpr__ operator */
	if (e->expr [p] == RESERVED___declexpr__) {
		e->nop = 2;
		e->operators [0] = p++;
		e->esc_op = false;
		if (e->expr [p] != '(')
			parse_error_ll ("__declexpr__ '('");
		return e->operators [1] = skip_buffer_parenthesis (e->expr, p + 1);
	}

	if (e->expr [p] == RESERVED_sizeof) {
		if (e->expr [p + 1] == '(' && is_dcl_start (e->expr [p + 2])) {
			/* * Feature: classname.symbol is an expression * */
			if (ISSYMBOL (e->expr [p + 2]) && e->expr [p + 3] == '.')
				goto eelse;
			/* * * * * * * * */
			e->et = SIZEOF_TYPE;
			e->nop = 1;
			e->operators [0] = p;
			e->esc_op = false;
			p = skip_buffer_parenthesis (e->expr, p + 2);
			return p;
		} else eelse: {
			e->et = SIZEOF_EXPRESSION;
			p = unary_expression (e, p + 1);
			e->nop = 1;
			e->operators [0] = ps;
			e->esc_op = false;
			return p;
		}
        }
	if (e->expr [p] == '(')
		if (is_dcl_start (e->expr [p + 1])
		&& !(e->expr [p + 2] == '(' && e->expr [p + 3] != '*')
		&& !(ISSYMBOL (e->expr [p + 1]) && e->expr [p + 2] == '.')) {
			ps = p;
			p = skip_buffer_parenthesis (e->expr, p + 1);
			ps2 = p - 1;
			p = unary_expression (e, p);
			e->et = PARENTH_CAST;
			e->nop = 2;
			e->operators [0] = ps;
			e->operators [1] = ps2;
			e->esc_op = false;
			return p;
		}

	p = primary_expression (e, p);
	return postfix_expression (e, p);
}

static NormPtr skip_statement_expression (bexpr *e, NormPtr p)
{
	int bno = 1;
	Token *code = e->expr;

	while (code [p] != -1 && bno)
		switch (code [p++]) {
		case '{': bno++;
		ncase '}': bno--;
		}

	if (code [p - 1] == -1) parse_error (0, "missing '}'");

	return p - 1;
}

static NormPtr primary_expression (bexpr *e, NormPtr p)
{
	if (e->expr [p] == '(') {
		NormPtr ps = p++;
		if (e->expr [p] == '{') {
			e->operators [0] = p++;
			p = e->operators [1] = skip_statement_expression (e, p);
			e->nop = 2;
			if (e->expr [++p] != ')')
				parse_error (p, "({statement})");
			return p + 1;
		}
		p = expression (e, p);
		if (e->expr [p] != ')')
			parse_error (p, "missing ')'");
		e->et = PARENTH_SYNTAX;
		e->nop = 2;
		e->operators [0] = ps;
		e->operators [1] = p;
		e->esc_op = false;
		return p + 1;
	}

	if (ISSYMBOL (e->expr [p]) || e->expr [p] == RESERVED_postfix
	|| ISVALUE (e->expr [p]) || e->expr [p] == RESERVED_this) {
		e->nop = 1;
		e->operators [0] = p;
		return p + 1;
	}

	if (e->expr [p] == '{') {
		/* "compound statement in expression" */
		e->nop = 2;
		e->operators [0] = p;
		e->operators [1] = p = skip_statement_expression (e, p + 1);
		return p + 1;
	}

	return p;
}

static NormPtr postfix_expression (bexpr *e, NormPtr p)
{
	NormPtr ps;
	bool esc_op = false;

	switch (e->expr [p]) {
	case ESCBASE + PLUSPLUS:
	case ESCBASE + MINUSMINUS:
		esc_op = true;
	case PLUSPLUS:
	case MINUSMINUS:
		e->nop = 1;
		e->operators [0] = p;
		e->esc_op = esc_op;
		return postfix_expression (e, p + 1);
	case '[' + ESCBASE:
		esc_op = true;
	case '[':
		ps = p;
		p = expression (e, p + 1);
		e->nop = 2;
		e->operators [0] = ps;
		e->operators [1] = p;
		e->esc_op = esc_op;
		return postfix_expression (e, p + 1);
	case POINTSAT + ESCBASE:
		esc_op = true;
	case '.':
	case POINTSAT:
		e->nop = 1;
		e->operators [0] = p++;
		if (!ISSYMBOL (e->expr [p]))
			parse_error (p, "member indentifier must follow . and ->");
		e->esc_op = esc_op;
		return postfix_expression (e, p + 1);
	case '(':
		ps = p;
		p = argument_expression_list (e, p + 1);
		if (e->expr [p] != ')')
			parse_error (p, "missing ')' in function call");
		e->et = PARENTH_FCALL;
		e->operators [0] = ps;
		e->operators [e->nop++] = p;
		e->esc_op = false;
		return postfix_expression (e, p + 1);
	}
	return p;
}

static NormPtr binary_expression (bexpr *e, NormPtr p, int pri)
{
static	int xpri;
	bool lesc_op = esc_op;
	NormPtr ps = p;

	e->nop = 0;
	p = unary_expression (e, p + 1);
	if (e->nop == 0)
		parse_error (p, "two operands expected");

	xpri = priority_esc (e->expr [p]);
	while (xpri > pri)
		p = binary_expression (e, p, xpri);

	e->nop = 1;
	e->operators [0] = ps;
	e->esc_op = lesc_op;

	return xpri < pri ? p : binary_expression (e, p, pri);
}

static NormPtr logical_OR_expression (bexpr *e, NormPtr p)
{
	p = unary_expression (e, p);

	if (e->nop == 0)
		return p;

	int pri;
	while ((pri = priority_esc (e->expr [p])))
		p = binary_expression (e, p, pri);

	return p;
}

static NormPtr conditional_expression (bexpr *e, NormPtr p)
{
	p = logical_OR_expression (e, p);

	if (e->nop == 0 || e->expr [p] != '?')
		return p;

	NormPtr ps1 = p, ps2;
	p = expression (e, p + 1);
	if (e->expr [p] != ':')
		parse_error (p, "missing ':'");
	ps2 = p;
	p = conditional_expression (e, p + 1);
	e->nop = 2;
	e->operators [0] = ps1;
	e->operators [1] = ps2;
	e->esc_op = false;

	return p;
}

static NormPtr assignment_expression (bexpr *e, NormPtr p)
{
	p = conditional_expression (e, p);

	if (e->nop == 0 || !isassignment_esc (e->expr [p]))
		return p;

	bool lesc_op = esc_op;
	NormPtr ps = p;

	e->nop = 0;
	p = assignment_expression (e, p + 1);
	if (e->nop == 0)
		parse_error (p, "missing right operand in assignment");
	e->nop = 1;
	e->operators [0] = ps;
	e->esc_op = lesc_op;

	return p;
}

static NormPtr argument_expression_list (bexpr *e, NormPtr p)
{
	int commas [128];
	int nc = 0, i;

	p = assignment_expression (e, p);
	if (e->nop == 0)
		return p;

	while (e->expr [p] == ',') {
		commas [nc++] = p++;
		e->nop = 0;
		p = assignment_expression (e, p);
		if (e->nop == 0)
			parse_error_ll ("missing argument?");
	}

	for (i = 0; i < nc; i++)
		e->operators [i + 1] = commas [i];
	e->nop = i + 1;
	e->esc_op = false;

	return p;
}

static NormPtr expression (bexpr *e, NormPtr p)
{
	p = assignment_expression (e, p);
	if (e->nop == 0)
		return p;

	while (e->expr [p] ==  ',') {
		NormPtr po = p;
		e->nop = 0;
		p = assignment_expression (e, p + 1);
		if (e->nop == 0)
			expr_error ("missing expression in ,,");
		e->nop = 1;
		e->operators [0] = po;
		e->esc_op = false;
	}

	return p;
}

void break_expr (bexpr *e)
{
#ifdef	DEBUG
	if (debugflag.PEXPR_EXTREME)
		INTPRINTF ("Request to break: ", e->expr);
#endif

	esc_op = e->esc_op = false;
	e->nop = 0;
	e->et = NOAMBIG;
	if (e->expr [expression (e, 0)] != -1)
		expr_error ("Invalid expression");

	if (e->esc_op)
		e->expr [e->operators [0]] -= ESCBASE;
}

NormPtr skip_expression (Token *code, NormPtr p, int et)
{
	bexpr E;
	E.expr = code;
	E.nop = 0;
	return et == NORMAL_EXPR ? expression (&E, p) :
	       et == INIT_EXPR ? assignment_expression (&E, p) :
		conditional_expression (&E, p);
}
