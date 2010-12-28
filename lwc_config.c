#include "global.h"

static NormPtr version (NormPtr p)
{
	int maj = eval_int (CODE [p++]);
	if (CODE [p++] != ',') parse_error (p, "version maj,min");
	int min = eval_int (CODE [p++]);
	if (maj > VERSION_MAJ || (maj == VERSION_MAJ && min > VERSION_MIN)) {
		fprintf (stderr, "This program requires lwc version %i.%i\n", maj, min);
		exit (0);
	}
	return CODE [p] == ';' ? p + 1 : p;
}

static NormPtr lwcdebug (NormPtr p)
{
#ifdef DEBUG
	if (0);
#define DEBUGFLAG(x) else if (!tokcmp (CODE [p], #x)) debugflag.x=1;
	DEBUGFLAG(GENERAL)
	DEBUGFLAG(PROGRESS)
	DEBUGFLAG(PEXPR)
	DEBUGFLAG(PEXPR_EXTREME)
	DEBUGFLAG(FUNCPROGRESS)
	DEBUGFLAG(OUTPUT_INDENTED)
	DEBUGFLAG(SHOWPROG)
	DEBUGFLAG(DCL_TRACE)
	DEBUGFLAG(TDEF_TRACE)
	DEBUGFLAG(VIRTUALTABLES)
	DEBUGFLAG(VIRTUALBASE)
	DEBUGFLAG(VIRTUALCOMBINE)
	DEBUGFLAG(PARSE_ERRORS_SEGFAULT)
	DEBUGFLAG(NOSTDOUT)
	DEBUGFLAG(REGEXP_DEBUG)
	DEBUGFLAG(EXPR_ERRORS_FATAL)
	else if (!tokcmp (CODE [p], "all")) {
		debugflag.GENERAL = debugflag.PEXPR = debugflag.PEXPR_EXTREME = 
		debugflag.FUNCPROGRESS = debugflag.OUTPUT_INDENTED = debugflag.SHOWPROG = 
		debugflag.DCL_TRACE = debugflag.TDEF_TRACE = debugflag.VIRTUALTABLES = 
		debugflag.VIRTUALBASE = debugflag.VIRTUALCOMBINE = debugflag.REGEXP_DEBUG = 1;
	} else if (CODE [p] == '>') {
		char *s = expand (CODE [++p]);
		if (*s == '"') {
			s [strlen (s) - 1] = 0;
			if (!(logstream = fopen (s + 1, "w")))
				parse_error (p - 1, "Cant open this logfile");
		} else if (*s >= '0' && *s <= '9')
			logstream = fdopen (*s - '0', "w");
		else parse_error (p, "You probably want : lwcdebug > \"file.out\"?");
	} else parse_error (p, "debug flag not available");
#endif
	return CODE [++p] == ';' ? p + 1 : p;
}

static NormPtr prologue (NormPtr p)
{
	NormPtr p2;
	for (p2 = p; CODE [p2] != ';'; p2++);
	func_prologue = intndup (CODE + p, ++p2 - p);
	return p2;
}

static NormPtr new (NormPtr p)
{
	if (!syntax_pattern (p, '=', VERIFY_symbol, ';', -1))
		parse_error (p, "new = func;");
	new_wrap = CODE [p + 1];
	return p + 3;
}

static NormPtr delete (NormPtr p)
{
	if (!syntax_pattern (p, '=', VERIFY_symbol, ';', -1))
		parse_error (p, "delete = func;");
	delete_wrap = CODE [p + 1];
	return p + 3;
}

static NormPtr comment_C (NormPtr p)
{
	char *s = expand (CODE [p++]);
	char *d = alloca (strlen (s));
	if (*s != '"') parse_error (p, "comment_C \"string\";");
	strcpy (d, s + 1);
	d [strlen (d) - 1] = 0;
	output_itoken (GLOBAL, new_symbol (strdup (d)));
	return CODE [p] == ';' ? p + 1 : p;
}

#define BOOLEAN_GLOBAL(x, y, z) \
	static inline NormPtr x (NormPtr p) \
	{\
		y = z;\
		return CODE [p] == ';' ? p + 1 : p;\
	}\

BOOLEAN_GLOBAL (struct_by_value, StructByRef, 0)
BOOLEAN_GLOBAL (struct_by_reference, StructByRef, 1)
BOOLEAN_GLOBAL (inline_virtual_tables, InlineAllVt, 1)
BOOLEAN_GLOBAL (const_virtual_tables, ConstVtables, 1)
BOOLEAN_GLOBAL (virtual_inheritance_declarations, VIDeclarations, 1)
BOOLEAN_GLOBAL (reentrant, Reentrant, 1)
BOOLEAN_GLOBAL (expand_all_auto, ExpandAllAutos, 1)
BOOLEAN_GLOBAL (noexceptions, ExceptionsUsed, 0)
BOOLEAN_GLOBAL (vtptr_noconst, vtptrConst, 0)
BOOLEAN_GLOBAL (no_linkonce, NoLinkonce, 1)
BOOLEAN_GLOBAL (gcc34cleanup, EHUnwind, 1)
BOOLEAN_GLOBAL (no_gcc34cleanup, EHUnwind, 0)
BOOLEAN_GLOBAL (onebigfile, OneBigFile, 1)
BOOLEAN_GLOBAL (no_have_aliases, HaveAliases, 0)
BOOLEAN_GLOBAL (x86stdcalls, StdcallMembers, 1)
BOOLEAN_GLOBAL (no_vtbl, ExportVtbl, 0)


NormPtr lwc_config (NormPtr p)
{
	if (CODE [p++] != '{')
		parse_error (p, "lwc_config");

	while (CODE [p] != '}')
		if (0);
#define		OPTION(x) else if (!tokcmp (CODE [p], #x)) p = x (p + 1);
		OPTION (version)
		OPTION (lwcdebug)
		OPTION (struct_by_value)
		OPTION (struct_by_reference)
		OPTION (comment_C)
		OPTION (prologue)
		OPTION (inline_virtual_tables)
		OPTION (const_virtual_tables)
		OPTION (virtual_inheritance_declarations)
		OPTION (reentrant)
		OPTION (expand_all_auto)
		OPTION (noexceptions)
		OPTION (vtptr_noconst)
		OPTION (no_linkonce)
		OPTION (gcc34cleanup)
		OPTION (no_gcc34cleanup)
		OPTION (onebigfile)
		OPTION (no_have_aliases)
		OPTION (x86stdcalls)
		OPTION (new)
		OPTION (delete)
		OPTION (no_vtbl)
		else parse_error_tok (CODE [p], "No such lwc option");

	return p + 1;
}

//****************************************************************

NormPtr __C__ (NormPtr p)
{
	if (!syntax_pattern (p, '=', VERIFY_string, ';', -1))
		parse_error (p, "__C__ = \"string\";");

	char *cc = expand (CODE [p + 1]);
	char *s2 = (char*) alloca (strlen (cc));
	strcpy (s2, cc + 1);
	s2 [strlen (s2) - 1] = 0;
	output_itoken (INCLUDE, new_symbol (strdup (s2)));
	return p + 3;
}
