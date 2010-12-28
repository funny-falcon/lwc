#include "global.h"

#define TEMPLATE_COMMA "templ_comma"
#define TEMPLATE_PARENTH "templ_parenth"

typedef struct {
	Token *BODY;
	int len, argc;
} Template;

static Token *OCD;
static int nocd, ncdpt;

static inline void output_templtok (Token i)
{
	if (nocd == ncdpt)
		OCD = (Token*) realloc (OCD, sizeof (Token) * (nocd += 512));
	OCD [ncdpt++] = i;
}

static Template **tpls;

#define CONCAT_CMD 1
#define FINISH_CMD 2

static void _export_token (Token t)
{
static	Token lasttok;
static	bool concatenation = false;

	if (t == FINISH_CMD) {
		if (lasttok) output_itoken (GLOBAL, lasttok);
	} else if (t == CONCAT_CMD) {
		if (concatenation) return;
		if (!lasttok) return;
		concatenation = true;
	} else if (!ISSYMBOL (t) && !ISRESERVED (t) && !ISVALUE (t)) {
		if (lasttok) output_itoken (GLOBAL, lasttok);
		output_itoken (GLOBAL, t);
		lasttok = 0;
		concatenation = false;
	} else if (!concatenation) {
		if (lasttok) output_itoken (GLOBAL, lasttok);
		lasttok = t;
	} else {
		char *tmp = (char*) alloca (strlen (expand (lasttok)) + strlen (expand (t)) + 1);
		strcat (strcpy (tmp, expand (lasttok)), expand (t));
		lasttok = new_symbol (strdup (tmp));
	}
}

static void export_token (Token t)
{
static	int havelev;
	if (t == '>' && !havelev) {
		havelev = 1;
		return;
	}
	if (t == '<' && havelev) {
		_export_token (CONCAT_CMD);
		havelev = 0;
		return;
	}
	if (havelev) {
		_export_token ('>');
		havelev = 0;
	}
	_export_token (t);
}

static void expand_template (Template *t, Token **argv)
{
	NormPtr i, len = t->len;
	Token *BODY = t->BODY;

	for (i = 0; i < len; i++)
		if (ISTPLARG (BODY [i])) {
			Token *p = argv [BODY [i] - ARGBASE];
			while (*p != -1)
				export_token (*p++);
		} else export_token (BODY [i]);
	export_token (FINISH_CMD);
}

static NormPtr expand_parse_template (NormPtr i)
{
	Template *t = tpls [CODE [i++] - IDENTBASE];
	Token **argv = (Token**) alloca (t->argc * sizeof (Token*)), *argvv;
	int argc, c;
	NormPtr s;

	if (CODE [i++] != '(') parse_error (i, "template invokation");
	for (argc = 0; argc < t->argc; i++) {
		s = i;
		while (CODE [i] != ',' && CODE [i] != ')')
			i++;
		argv [argc++] = argvv = (Token*) alloca ((1 + i - s) * sizeof (Token));
		intextract (argvv, &CODE [s], i - s);
		for (c = 0; argvv [c] != -1; c++)
			if (!tokcmp (argvv [c], TEMPLATE_COMMA))
				argvv [c] = ',';
			else if (!tokcmp (argvv [c], TEMPLATE_PARENTH))
				argvv [c] = ')';
		if (CODE [i] == ')') break;
	}
	if (argc < t->argc) parse_error (i, "too few arguments to template");
	if (CODE [i] != ')') parse_error (i, "too many arguments to template");
	expand_template (t, argv);

	return i;
}

static NormPtr templatedef (NormPtr i)
{
	NormPtr pstart = i;
	Token tname = CODE [i++];
	Token targ [32];
	int argc = 0, j, blockno;

	if (!ISSYMBOL (tname)) parse_error (i, "template name missing");
	if (tpls [tname - IDENTBASE]) parse_error (i, "template redefined");
	if (CODE [i++] != '(') parse_error (i, "template name '('");
	for (;;i++) {
		targ [argc] = CODE [i++];
		if (!ISSYMBOL (targ [argc])) parse_error (i, "template argument name");
		argc++;
		if (CODE [i] == ',') continue;
		break;
	}
	if (CODE [i++] != ')' || argc == 0)
		parse_error (i, "bad argument list for template");

	OCD = (Token*) malloc ((nocd = 512) * sizeof (Token));
	ncdpt = 0;

	if (CODE [i++] != '{') parse_error (i, "template '{'");

	for (blockno = 1; CODE [i] != -1; i++)
		if (ISSYMBOL (CODE [i])) {
			for (j = 0; j < argc; j++)
				if (CODE [i] == targ [j]) break;
			if (j < argc) {
				output_templtok (ARGBASE + j);
				continue;
			}
			output_templtok (CODE [i]);
		} else if (CODE [i] == '{') {
			output_templtok ('{');
			++blockno;
		} else if (CODE [i] == '}') {
			if (--blockno == 0) break;
			output_templtok ('}');
		} else output_templtok (CODE [i]);

	if (CODE [i] == -1) parse_error (i, "unclosed template definition");

	Template *t = tpls [tname - IDENTBASE] = (Template*) malloc (sizeof (Template));
	t->BODY = (Token*) realloc (OCD, ncdpt * sizeof (Token));
	t->len = ncdpt;
	t->argc = argc;

	adjust_lines (pstart, pstart - i);

	return i;
}

static void pass ()
{
	NormPtr i;

	for (i = 0; CODE [i] != -1; i++)
		if (CODE [i] == RESERVED_template
		&& CODE [i + 1] != RESERVED_class && CODE [i + 1] != RESERVED_struct)
			i = templatedef (i + 1);
		else if (ISSYMBOL (CODE [i]) && tpls [CODE [i] - IDENTBASE])
			i = expand_parse_template (i);
		else output_itoken (GLOBAL, CODE [i]);

	output_itoken (GLOBAL, -1);
}

void do_templates ()
{
	int i;

	GLOBAL = new_stream ();
	tpls = (Template**) alloca (c_nsym * sizeof (Template*));

	for (i = 0; i < c_nsym; i++)
		tpls [i] = 0;

	pass ();

	for (i = 0; i < c_nsym; i++)
		if (tpls [i]) {
			free (tpls [i]->BODY);
			free (tpls [i]);
		}
	free (CODE);
	CODE = combine_output (GLOBAL);
}
