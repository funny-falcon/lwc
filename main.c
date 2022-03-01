#include <ctype.h>
#include "global.h"
#include "SYS.h"

int zinit [2];

bool MainModule = 0;
bool HadErrors = 0;
bool StructByRef = 1;
bool InlineAllVt = 0;
bool ConstVtables = 0;
bool VIDeclarations = 0;
bool Reentrant = 0;
bool ExpandAllAutos = 0;
bool Streams_Closed = false;
bool ExceptionsUsed = 1;
bool GlobInitUsed = 0;
bool vtptrConst = 1;
bool NoLinkonce = 0;
bool OneBigFile = 0;
bool HaveAliases = 0;	/* not well removed by gcc and produce bigger binaries */
bool StdcallMembers = 0;
bool ExportVtbl = 1;
bool EHUnwind = EHDEFAULT;
bool GoodOldC = false;

char *current_file;

void init ()
{
	Token t;
	int p [8] = { -1, };
	Token nodcl [] = { -1 };
	Token xargs [] = { RESERVED_x, -1 };

	// null invalid structure && type

	enter_struct (NOOBJ, 1, 0, 0, 0, 0, 0);
	enter_type (p);

	// common types

	p[0] = B_PURE; p[1] = -1;
	typeID_NOTYPE = enter_type (p);
	p[0] = B_SINT; p[1] = -1;
	typeID_int = enter_type (p);
	p[0] = B_UINT;
	typeID_uint = enter_type (p);
	p[0] = B_FLOAT;
	typeID_float = enter_type (p);
	p[0] = B_VOID;
	typeID_void = enter_type (p);
	sintprintf (p, B_SCHAR, '*', -1);
	typeID_charP = enter_type (p);
	p[0] = B_VOID;
	typeID_voidP = enter_type (p);
	p[0] = B_SINT;
	typeID_intP = enter_type (p);
	sintprintf (p, B_SCHAR, '(', typeID_int, INTERNAL_ARGEND, '*', -1);
	typeID_ebn_f = enter_type (p);

	// special objects

	enter_global_object (RESERVED___FUNCTION__, typeID_charP);
	enter_global_object (RESERVED___PRETTY_FUNCTION__, typeID_charP);
	if ((t = Lookup_Symbol ("__builtin_va_list")))
		enter_typedef (t, typeID_voidP);

	if ((t = Lookup_Symbol ("__lwcbuiltin_get_estack"))) {
		sintprintf (p, typeID_voidP, '(', INTERNAL_ARGEND, -1);
		xdeclare_function (&Global, t, t, enter_type (p), nodcl, xargs, FUNCP_NOTHROW, 0,0);
	}
	if ((t = Lookup_Symbol ("__lwcbuiltin_set_estack"))) {
		sintprintf (p, B_VOID, '(', typeID_voidP, INTERNAL_ARGEND, -1);
		xdeclare_function (&Global, t, t, enter_type (p), nodcl, xargs, FUNCP_NOTHROW, 0,0);
	}

	if ((t = Lookup_Symbol ("__builtin_expect"))) {
		sintprintf (p, B_SINT, '(', typeID_int, typeID_int, INTERNAL_ARGEND, -1);
		xdeclare_function (&Global, t, t, enter_type (p), nodcl, xargs, FUNCP_NOTHROW, 0,0);
	}
	if ((t = Lookup_Symbol ("__builtin_stdarg_start"))) {
		sintprintf (p, B_VOID, '(', B_ELLIPSIS, INTERNAL_ARGEND, '*', -1);
		xdeclare_function (&Global, t, t, enter_type (p), nodcl, xargs, 0, 0, 0);
	}
	if ((t = Lookup_Symbol ("__builtin_va_end"))) {
		sintprintf (p, B_VOID, '(', B_ELLIPSIS, INTERNAL_ARGEND, '*', -1);
		xdeclare_function (&Global, t, t, enter_type (p), nodcl, xargs, 0, 0, 0);
	}

	if ((t = Lookup_Symbol ("__builtin_popcount"))) {
		sintprintf (p, B_SINT, '(', typeID_int, INTERNAL_ARGEND, -1);
		xdeclare_function (&Global, t, t, enter_type (p), nodcl, xargs, FUNCP_NOTHROW, 0,0);
	}
	if ((t = Lookup_Symbol ("__builtin_constant_p"))) {
		sintprintf (p, B_SINT, '(', B_ELLIPSIS, INTERNAL_ARGEND, -1);
		xdeclare_function (&Global, t, t, enter_type (p), nodcl, xargs, FUNCP_NOTHROW, 0,0);
	}
	if ((t = Lookup_Symbol ("__builtin_strcmp"))) {
		sintprintf (p, typeID_charP, '(', typeID_charP, typeID_charP, INTERNAL_ARGEND, -1);
		xdeclare_function (&Global, t, t, enter_type (p), nodcl, xargs, FUNCP_NOTHROW, 0,0);
	}
	if ((t = Lookup_Symbol ("__builtin_strchr"))) {
		sintprintf (p, typeID_charP, '(', typeID_charP, typeID_int, INTERNAL_ARGEND, -1);
		xdeclare_function (&Global, t, t, enter_type (p), nodcl, xargs, FUNCP_NOTHROW, 0,0);
	}
	if ((t = Lookup_Symbol ("__builtin_memset"))) {
		sintprintf (p, typeID_charP, '(', typeID_charP, typeID_charP, typeID_int, INTERNAL_ARGEND, -1);
		xdeclare_function (&Global, t, t, enter_type (p), nodcl, xargs, FUNCP_NOTHROW, 0,0);
	}
	if ((t = Lookup_Symbol ("__builtin_memmove"))) {
		sintprintf (p, typeID_charP, '(', typeID_charP, typeID_charP, typeID_int, INTERNAL_ARGEND, -1);
		xdeclare_function (&Global, t, t, enter_type (p), nodcl, xargs, FUNCP_NOTHROW, 0,0);
	}
	if ((t = Lookup_Symbol ("__builtin_memcpy"))) {
		sintprintf (p, typeID_charP, '(', typeID_charP, typeID_charP, typeID_int, INTERNAL_ARGEND, -1);
		xdeclare_function (&Global, t, t, enter_type (p), nodcl, xargs, FUNCP_NOTHROW, 0,0);
	}
	sintprintf (p, B_VOID, '(', typeID_uint, INTERNAL_ARGEND, '*', -1);
	xdeclare_function (&Global, INTERN_alloca, INTERN_alloca,
			   enter_type (p), nodcl, xargs, FUNCP_NOTHROW, 0, 0);
//#ifdef __GNUC__
//	xdeclare_function (&Global, RESERVED_alloca, RESERVED_alloca,
//			   enter_type (p), nodcl, xargs, FUNCP_NOTHROW, 0, 0);
//#endif

	name_derrive_union = new_symbol (DERRIVE_UNION);

	zinit [0] = RESERVED_0, zinit [1] = -1;

	GLOBAL_INIT_FUNC = new_stream ();
	Token GIF = new_symbol (GLOBINITF);
	outprintf (GLOBAL_INIT_FUNC, RESERVED_static, RESERVED_void, GIF, '(', ')',
		   RESERVED___attribute__, '(', '(', RESERVED_constructor, ')', ')', ';',
		   RESERVED_static, RESERVED_void, GIF, '(', ')', '{', -1);
}

void finish ()
{
	if (GlobInitUsed) {
		output_itoken (GLOBAL_INIT_FUNC, '}');
#ifdef	COMMENT_OUTPUT
		output_itoken (GLOBAL, new_symbol (COMMENT_SECTION_OBJCTOR));
#endif
		concate_streams (GLOBAL, GLOBAL_INIT_FUNC);
	}
	if (HadErrors)
		output_itoken (GLOBAL, new_symbol ("\n#error there were errors in lwc expressions\n"));
}

static const char stdc [] =
   "#define __STDC__ 1\n"
   "#define __STDC_VERSION__ 199901L\n"
   "";

#ifdef	DO_CPP
static const char stddef [] = 
#ifdef __GNUC__
	// hairy gcc crap...
	// * * * STILL DOESN'T WORK. shitty Glibc crap
   "#define NULL ((void*)0)\n"
   "typedef __SIZE_TYPE__ size_t;\n"
   "typedef __PTRDIFF_TYPE__ ptrdiff_t;\n"
   "typedef __WCHAR_TYPE__ wchar_t;\n"
   "typedef __WINT_TYPE__ wint_t;\n"
   "void exit (int);\n"
#endif
   "extern void * malloc (size_t);\n"
#if INTER_alloca == RESERVED_alloca
   "extern void * alloca (unsigned int);\n"
#endif
   "extern void free (void*);\n"
   "";
#endif

static const char stdlwc [] =
   ";\n";

#ifndef	DEBUG
#define PROGRESS(x)
#else
#define PROGRESS(x) if (debugflag.PROGRESS) fputs (COLS x COLE"...\n", stderr);
#endif

int main (int argc, char **argv)
{
	srand (time (NULL) ^ getpid () ^ (intptr_t) argv ^ (intptr_t) &argc ^ (intptr_t) main);
#ifdef	DEBUG
	enable_debugs ();
#endif

	PROGRESS ("preprocessing");
	preproc (argc - 1, argv + 1);
	init_processors ();
	initlex ();
	if (!sys_cpp) {
		yydo_mem ((char*)sys_defs, sizeof sys_defs - 1);
		yydo_mem ((char*)stdc, sizeof stdc - 1);
	}
#ifdef	DO_CPP
	yydo_mem ((char*)stddef, sizeof stddef - 1);
#endif
	yydo_mem ((char*)stdlwc, sizeof stdlwc - 1);
	if (yydo (main_file)) {
		fprintf (stderr, "No such file or directory\n");
		return 1;
	}
	unlink (main_file);

#ifdef	DO_CPP
	cleanup_cpp ();
#endif
	init ();
	init_except ();
	do_templates ();
	bogus1 ();

#ifdef	DEBUG
	if (debugflag.SHOWPROG) {
		int i;
		for (i=0; CODE [i] != -1; i++)
			PRINTF ("%i %s %c", i, expand (CODE [i]),i%6 ? '\t':'\n');
		PRINTF ("\n");
	}
	if (debugflag.SHOWCPP) {
		FILE *F = fopen (PREPROCOUT, "w");
		int i;
		for (i=0; CODE [i] != -1; i++)
			fprintf (F, "%s ", expand (CODE [i]));
		fclose (F);
	}
#endif

	FPROTOS = new_stream ();
	GLOBAL = new_stream ();
	INTERNAL_CODE = new_stream ();
	VIRTUALTABLES = new_stream ();
	VTABLE_DECLARATIONS = new_stream ();
	STRUCTS = new_stream ();
	GVARS = new_stream ();
	INCLUDE = new_stream ();
	REGEXP_CODE = new_stream ();
	AUTOFUNCTIONS = new_stream ();
	FUNCDEFCODE = new_stream ();

	output_itoken (INCLUDE, new_symbol (strdup ("/* lightweight c++ "LWC_VERSION" */\n")));
	output_itoken (INCLUDE, new_symbol (strdup ("#ifndef LWC_SIZES\n")));
	output_itoken (INCLUDE, new_symbol (strdup ("#define LWC_SIZES\n")));
	output_itoken (INCLUDE, new_symbol (strdup ("#define usz_t __SIZE_TYPE__\n")));
	output_itoken (INCLUDE, new_symbol (strdup ("#define ssz_t __INTPTR_TYPE__\n")));
	output_itoken (INCLUDE, new_symbol (strdup ("#endif\n")));
#ifdef	COMMENT_OUTPUT
	output_itoken (INCLUDE, new_symbol (COMMENT_SECTION_INCLUDE));
	output_itoken (FPROTOS, new_symbol (COMMENT_SECTION_PROTOTYPES));
	output_itoken (GLOBAL, new_symbol (COMMENT_SECTION_GLOBAL));
	output_itoken (INTERNAL_CODE, new_symbol (COMMENT_SECTION_INNERCODE));
	output_itoken (VIRTUALTABLES, new_symbol (COMMENT_SECTION_VIRTUALTABLES));
	output_itoken (VTABLE_DECLARATIONS, new_symbol (COMMENT_SECTION_VTDEF));
	output_itoken (STRUCTS, new_symbol (COMMENT_SECTION_STRUCTS));
	output_itoken (GVARS, new_symbol (COMMENT_SECTION_GVARS));
	output_itoken (REGEXP_CODE, new_symbol (COMMENT_SECTION_REGEXP));
	output_itoken (AUTOFUNCTIONS, new_symbol (COMMENT_SECTION_AUTOFUNCTIONS));
	output_itoken (FUNCDEFCODE, new_symbol (COMMENT_SECTION_FUNCTIONS));
#endif


	PROGRESS ("parsing translation unit")
	translation_unit ();
 
	PROGRESS ("compiling functions");
	do do_functions ();
	while (specialize_abstracts ());

	PROGRESS ("auto functions")
	define_auto_functions ();

	PROGRESS ("making data structures");
	export_structs ();
	if (ExceptionsUsed)
		decl_except_data ();
	export_virtual_table_instances ();

#ifdef	HAVE_LINKONCE
	export_virtual_definitions ();
#else
	if (MainModule) export_virtual_definitions ();
	else export_virtual_static_definitions ();
#endif

	if (Global) export_fspace (Global);
	outprintf (FPROTOS, RESERVED_void, '*', RESERVED_malloc, '(', RESERVED_usz_t,
		   ')', ';', RESERVED_void, RESERVED_free, '(', ')', ';', -1);

	PROGRESS ("expanding output");
	concate_streams (INCLUDE, GLOBAL);
	GLOBAL = INCLUDE;
	concate_streams (GLOBAL, STRUCTS);
	concate_streams (GLOBAL, VTABLE_DECLARATIONS);
	concate_streams (GLOBAL, FPROTOS);
	concate_streams (GLOBAL, GVARS);
	concate_streams (GLOBAL, REGEXP_CODE);
	concate_streams (GLOBAL, INTERNAL_CODE);
	concate_streams (GLOBAL, AUTOFUNCTIONS);
	concate_streams (GLOBAL, FUNCDEFCODE);
	concate_streams (GLOBAL, VIRTUALTABLES);
	finish ();

#ifdef	DEBUG
	if (!debugflag.NOSTDOUT)
#endif
		export_output (GLOBAL);

	return HadErrors;
}
